#pragma once
#include "detail/detail.hpp"
namespace xsimple_rpc
{
	static const std::string magic_code = "xsimple_rpc";

	struct rpc_cancel: std::exception
	{
		virtual char const* what() const override 
		{ return "rpc_cancel_error"; }
	};
	struct rpc_error: std::exception
	{
		rpc_error(std::string error_code)
			:error_code_(error_code)
		{
		}
		virtual char const* what() const override
		{
			return error_code_.c_str();
		}
		std::string error_code_;
	};
	class client
	{
	private:
		using get_response = std::function<std::string()>;
		using cancel_get_response = std::function<void()>;
		using send_rpc_result = std::pair <get_response, cancel_get_response>;
		friend class rpc_engine;
	public:
		client()
		{
		}
		client(client &&other)
		{
			move_reset(std::move(other));
		}
		client &operator=(client && other)
		{
			move_reset(std::move(other));
		}
		template<typename Proto, typename ...Args>
		auto rpc_call(Args ...args)
		{
			return rpc_call_impl(xutil::function_traits<typename Proto::func_type>(),
				Proto::rpc_name, 
				std::forward<Args>(args)...);
		}
	private:
		
		void move_reset(client &&other)
		{
			if (&other == this)
				return;
			std::lock_guard <std::mutex> locker(other.mutex_);
			send_req = std::move(other.send_req);
			close_rpc_ = std::move(other.close_rpc_);
		}
		
		template<typename Ret, typename ...Args>
		Ret rpc_call_impl(
			const xutil::function_traits<Ret(Args...)>&,
			const std::string &rpc_name,
			typename std::remove_reference<typename std::remove_const<Args>::type>::type&& ...args)
		{
			auto req_id = gen_req_id();
			auto buffer = make_req(rpc_name, req_id, std::forward_as_tuple(args...));
			if (!send_req)
				throw std::logic_error("client isn't connected");

			auto result = send_req(std::move(buffer), req_id);
			set_cancel_get_response(std::move(result.second));
			auto resp = result.first();

			xnet::guard guard([&] {
				reset_cancel_get_response();
			});
			uint8_t *beg = (uint8_t*)resp.data();
			uint8_t *end = beg + resp.size();
			auto res = endec::get<Ret>(beg);
			if (beg != end)
				throw std::runtime_error("rpc resp error");
			return std::move(res);
		}

		template<typename ...Args>
		void rpc_call_impl(
			const xutil::function_traits<void(Args...)>&,
			const std::string &rpc_name,
			typename std::remove_reference<typename std::remove_const<Args>::type>::type&& ...args)
		{
			auto req_id = gen_req_id();
			auto buffer = make_req(rpc_name, req_id, std::forward_as_tuple(args...));
			auto get_resp = send_req(std::move(buffer), req_id);
			set_cancel_get_response(std::move(get_resp.second));
			get_resp.first();
			reset_cancel_get_response();
		}
		int64_t gen_req_id()
		{
			static std::atomic_int64_t req_id = 1;
			return req_id++;
		}

		template<typename ...Args>
		std::string make_req(const std::string &rpc_name, int64_t req_id, std::tuple<Args...> &&tp)
		{
			std::string buffer;
			uint32_t buffer_size = uint32_t(
				endec::get_sizeof(req_id)+
				endec::get_sizeof(tp) +
				endec::get_sizeof(rpc_name) +
				endec::get_sizeof(magic_code) +
				endec::get_sizeof(uint32_t()));

			buffer.resize(buffer_size);
			uint8_t*ptr = (uint8_t*)buffer.data();
			endec::put(ptr, buffer_size);
			endec::put(ptr, magic_code);
			endec::put(ptr, req_id);
			endec::put(ptr, rpc_name);
			endec::put(ptr, std::move(tp));
			return std::move(buffer);
		}
		void reset_cancel_get_response()
		{
			std::lock_guard<std::mutex> locker(mutex_);
			cancel_get_response_ = nullptr;
		}
		void set_cancel_get_response(cancel_get_response && handle)
		{
			std::lock_guard<std::mutex> locker(mutex_);
			cancel_get_response_ = std::move(handle);
		}
		std::mutex mutex_;
		cancel_get_response cancel_get_response_;
		std::function<send_rpc_result(std::string &&, int64_t)> send_req;
		std::function<void()> close_rpc_;
	};
	
	struct rpc_req
	{
		enum class status
		{
			e_ok,
			e_cancel,
			e_rpc_error,
		};
		status status_ = status::e_ok;
		int64_t req_id_;
		std::string result_;
		std::string req_buffer_;
	};

	struct rpc_session
	{
		
		void push_rpc_req(std::shared_ptr<rpc_req> & rpc_req)
		{
			std::lock_guard<std::mutex> locker(mtx_);
			req_item_list_.push_back(rpc_req);
		}

		void do_send_rpc()
		{
			std::lock_guard<std::mutex> locker(mtx_);
			if (is_send_ || req_item_list_.empty())
				return;
			is_send_ = true;
			auto item = req_item_list_.front();
			conn_.async_send(std::move(item->req_buffer_));
			wait_rpc_resp_list_.emplace_back(std::move(item));
			req_item_list_.pop_front();
		}

		void init_conn()
		{
			conn_.regist_send_callback([this](std::size_t len) {

				if (len == 0)
				{
					close();
					conn_.close();
					return;
				}
				is_send_ = false;
				do_send_rpc();
			});
			conn_.regist_recv_callback([this](char *data, std::size_t len) {
			
				if (len == 0 || recv_data_callback(data, len) == false)
				{
					close();
					conn_.close();
					return;
				}
			});
		}
		bool recv_data_callback(char *data, std::size_t len)
		{
			uint8_t *ptr = reinterpret_cast<uint8_t*>(data);
			if (msg_len_ == 0 && len == sizeof(msg_len_))
			{
				msg_len_ = endec::get<uint32_t>(ptr);
				static std::size_t min_msg_len = 
					endec::get_sizeof(std::string("heartbeat")) +
					endec::get_sizeof(magic_code) +
					endec::get_sizeof(uint32_t()) +
					endec::get_sizeof(uint64_t());
				if (msg_len_ < min_msg_len)
				{
					std::cout << "msg error" << std::endl;
					return false;
				}
				conn_.async_recv((std::size_t)msg_len_);
				msg_len_ = 0;
				return true;
			}

			auto magic_code = endec::get<std::string>(ptr);
			auto req_id = endec::get<int64_t>(ptr);
			auto rpc_name = endec::get<std::string>(ptr);
			if (req_id == 0)
			{
				heartbeat_callback();
				return true;
			}
			auto item = wait_rpc_resp_list_.front();
			wait_rpc_resp_list_.pop_front();
			if (item->req_id_ != req_id)
				return false;
			conn_.async_recv(sizeof(uint32_t));
			item->result_ = endec::get<std::string>(ptr);
			cv_.notify_one();
		}
		void heartbeat_callback()
		{

		}
		void close()
		{
			is_close_ = true;
		}
		uint32_t msg_len_ = 0;
		bool is_close_ = false;
		bool is_send_ = false;
		std::string last_error_code_;
		std::mutex mtx_;
		std::condition_variable cv_;
		xnet::connection conn_;
		std::unique_ptr<xnet::msgbox<std::function<void()>>> msgbox_;
		std::list<std::shared_ptr<rpc_req>> req_item_list_;
		std::list<std::shared_ptr<rpc_req>> wait_rpc_resp_list_;
	};

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
			send_msg([ip, port, session_wptr, this] {
				std::lock_guard<std::mutex> locker(connectors_mutex_);
				auto connector = proactor_pool_.get_current_proactor().get_connector();
				auto index = connector_index_++;
				connector.bind_fail_callback([index, session_wptr](std::string errorc_code)
				{
					auto ptr = session_wptr.lock();
					if (!ptr)
						return;
					std::unique_lock<std::mutex> locker(ptr->mtx_);
					ptr->last_error_code_ = errorc_code;
					ptr->cv_.notify_one();
				})
				.bind_success_callback([this, index, session_wptr](xnet::connection &&conn)
				{
					auto ptr = session_wptr.lock();
					if (!ptr) 
						return;
					std::unique_lock<std::mutex> locker(ptr->mtx_);
					ptr->conn_ = std::move(conn);
					ptr->cv_.notify_one();
					ptr->msgbox_.reset(new xnet::msgbox<std::function<void()>>(
						proactor_pool_.get_current_proactor()));
				});
				connector.async_connect(ip, port);
				connectors_.emplace(index, std::move(connector));
			});
			std::unique_lock<std::mutex> locker(session->mtx_);
			auto res = session->cv_.wait_for(locker, 
				std::chrono::milliseconds(timeout_millis),[&session] {
				return session->last_error_code_.size() || session->conn_.valid();
			});
			if (!res)
				throw std::exception("connect_timeout");

			if (session->last_error_code_.size())
				throw std::runtime_error("connect error: " + session->last_error_code_);

			auto session_id = add_session(session);
			client _client;
			_client.send_req = [this, session_id](std::string &&buffer, int64_t req_id){
				return do_rpc_call(session_id,std::move(buffer), req_id);
			};
			_client.close_rpc_ = [session_id, this, session] {
				session->msgbox_->send([this, session_id]{
					del_session(session_id);
				});
			};
			return std::move(_client);
		}
	private:
		using msgbox = xnet::msgbox<std::function<void()>>;
		using get_response = std::function<std::string()>;
		using cancel_get_response = std::function<void()>;
		template<typename T>
		void send_msg(T &&item)
		{
			std::lock_guard<std::mutex> locker(msgboxs_mutex_);
			msgboxs_[msgboxs_index_% msgboxs_.size()]->send(std::forward<T>(item));
			++msgboxs_index_;
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
		void do_rpc_call_impl(std::weak_ptr<rpc_session> session_wptr)
		{
			auto session = session_wptr.lock();
			if (!session)
				return;
			session->do_send_rpc();
		}
		void init()
		{
			msgboxs_.reserve(proactor_pool_.get_size());
			proactor_pool_.regist_run_before([this] {
				std::lock_guard<std::mutex> locker(session_mutex_);
				auto &pro = proactor_pool_.get_current_proactor();
				msgboxs_.emplace_back(new msgbox(pro));
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
		std::mutex session_mutex_;
		int64_t session_id_ = 1;
		std::map<int64_t, std::shared_ptr<rpc_session>> rpc_sessions_;

		std::mutex msgboxs_mutex_;
		std::size_t msgboxs_index_ = 0;		
		std::vector<std::unique_ptr<msgbox>> msgboxs_;

		std::mutex connectors_mutex_;
		int64_t connector_index_ = 0;
		std::map<int64_t, xnet::connector> connectors_;

		xnet::proactor_pool proactor_pool_;

	};

	class server
	{
	public:
		server()
		{

		}
		server &regist(const std::string &name)
		{

		}
		server &bind(std::string &ip_, int port)
		{
			return *this;
		}
	private:
	};
}