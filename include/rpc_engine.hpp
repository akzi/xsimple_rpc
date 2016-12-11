#pragma once
namespace xsimple_rpc
{
	using namespace detail;
	class rpc_engine
	{
	public:
		rpc_engine(std::size_t threadsize_)
		{
			proactor_pool_.set_size(threadsize_);
			init();
		}
		client connect(const std::string &ip, int port, int timeout_millis)
		{
			auto session = std::make_shared<rpc_session>();
			std::weak_ptr<rpc_session> session_wptr;

			auto msgbox = get_msgbox();
			auto item = [ip, port, session_wptr, this, msgbox]
			{
				std::lock_guard<std::mutex> locker(connectors_mutex_);
				auto connector = proactor_pool_.get_current_proactor().get_connector();
				connector_index_++;
				auto index = connector_index_;
				connector.bind_fail_callback([this, index, session_wptr](std::string errorc_code)
				{
					xnet::guard<std::function<void()>> guard([this, index] {del_connector(index); });
					auto ptr = session_wptr.lock();
					if (!ptr)
						return;
					std::unique_lock<std::mutex> locker(ptr->mtx_);
					ptr->last_error_code_ = errorc_code;
					ptr->cv_.notify_one();
				}); 
				connector.bind_success_callback([this, index, session_wptr, msgbox](xnet::connection &&conn)
				{
					xnet::guard<std::function<void()>> guard([this, index] {del_connector(index); });
					auto ptr = session_wptr.lock();
					if (!ptr)
						return;
					std::unique_lock<std::mutex> locker(ptr->mtx_);
					ptr->conn_ = std::move(conn);
					ptr->cv_.notify_one();
					ptr->msgbox_ = msgbox;

				});
				connector.async_connect(ip, port);
				connectors_.emplace(index, std::move(connector));
			};
			msgbox->send(std::move(item));
			std::unique_lock<std::mutex> locker(session->mtx_);
			auto res = session->cv_.wait_for(locker,
				std::chrono::milliseconds(timeout_millis), [&session] {
				return session->last_error_code_.size() || session->conn_.valid();
			});
			if (!res)
				throw std::exception("connect_timeout");

			if (session->last_error_code_.size())
				throw std::runtime_error("connect error: " + session->last_error_code_);

			auto session_id = add_session(session);
			client _client;
			_client.send_req = [this, session_id](std::string &&buffer, int64_t req_id) {
				return do_rpc_call(session_id, std::move(buffer), req_id);
			};
			_client.close_rpc_ = [session_id, this, session] {
				session->msgbox_->send([this, session_id] {
					del_session(session_id);
				});
			};
			return std::move(_client);
		}
	private:
		using msgbox_t = xnet::msgbox<std::function<void()>>;
		using get_response = std::function<std::string()>;
		using cancel_get_response = std::function<void()>;

		std::shared_ptr<msgbox_t> get_msgbox()
		{
			std::lock_guard<std::mutex> locker(msgboxs_mutex_);
			auto msgbox = msgboxs_[msgboxs_index_% msgboxs_.size()];
			++msgboxs_index_;
			return msgbox;
		}
		std::pair<get_response, cancel_get_response>
			do_rpc_call(int64_t session_id, std::string &&buffer, int64_t req_id)
		{
			auto item = std::make_shared<rpc_req>();
			item->req_buffer_ = std::move(buffer);
			item->req_id_ = req_id;

			std::lock_guard<std::mutex> locker(session_mutex_);
			auto itr = rpc_sessions_.find(session_id);
			if (itr == rpc_sessions_.end())
				throw std::runtime_error("can't find rpc_session");
			auto msg = item;
			auto session = itr->second;
			session->push_rpc_req(item);
			session->msgbox_->send([session] { session->do_send_rpc(); });

			return{ [session, item] {
				std::unique_lock<std::mutex> locker(session->mtx_);
				session->cv_.wait(locker);
				if (item->status_ == rpc_req::status::e_cancel)
					throw rpc_cancel();
				else if (item->status_ == rpc_req::status::e_rpc_error)
					throw rpc_error(item->result_);
				return item->result_;
			},[session, item] {
				item->status_ = rpc_req::status::e_cancel;
				session->cv_.notify_one();
			} };
		}
		void init()
		{
			msgboxs_.reserve(proactor_pool_.get_size());
			proactor_pool_.regist_run_before([this] {
				std::lock_guard<std::mutex> locker(session_mutex_);
				auto &pro = proactor_pool_.get_current_proactor();
				msgboxs_.emplace_back(new msgbox_t(pro));
				auto &msgbox = msgboxs_.back();
				msgbox->regist_notify([&pro, &msgbox] {
					do
					{
						auto item = msgbox->recv();
						if (!item.first)
							return;
						item.second();
					} while (true);
				});
			}).start();
		}
		void del_session(int64_t session_id)
		{
			std::lock_guard<std::mutex> locker(session_mutex_);
			rpc_sessions_.erase(rpc_sessions_.find(session_id));
		}
		int64_t add_session(std::shared_ptr<rpc_session> &session)
		{
			std::lock_guard<std::mutex> locker(session_mutex_);
			++session_id_;
			rpc_sessions_.emplace(session_id_, session);
			return session_id_;
		}

		int64_t add_connector(xnet::connector &&connector)
		{
			std::lock_guard<std::mutex> locker(connectors_mutex_);
			++connector_index_;
			connectors_.emplace(connector_index_, std::move(connector));
			return connector_index_;
		}
		void del_connector(int64_t index)
		{
			std::lock_guard<std::mutex> locker(connectors_mutex_);
			connectors_.erase(connectors_.find(index));
		}
		std::mutex session_mutex_;
		int64_t session_id_ = 1;
		std::map<int64_t, std::shared_ptr<rpc_session>> rpc_sessions_;

		std::mutex msgboxs_mutex_;
		std::size_t msgboxs_index_ = 0;
		std::vector<std::shared_ptr<msgbox_t>> msgboxs_;

		std::mutex connectors_mutex_;
		int64_t connector_index_ = 0;
		std::map<int64_t, xnet::connector> connectors_;

		xnet::proactor_pool proactor_pool_;
	};
}