#pragma once
#include "detail/detail.hpp"
namespace xsimple_rpc
{
	class client
	{
	public:
		using get_response = std::function<std::string()>;
		using cancel_get_response = std::function<void()>;

		template<typename Proto, typename ...Args>
		auto rpc_call(Args ...args)
		{
			return rpc_call_impl(xutil::function_traits<typename Proto::func_type>(),
				Proto::rpc_name, 
				std::forward<Args>(args)...);
		}
	private:
		friend class rpc_engine;
		template<typename Ret, typename ...Args>
		Ret rpc_call_impl(
			const xutil::function_traits<Ret(Args...)>&,
			const std::string &rpc_name,
			typename std::remove_reference<typename std::remove_const<Args>::type>::type&& ...args)
		{
			auto req_id = gen_req_id();
			auto buffer = make_req(rpc_name, req_id, std::forward_as_tuple(args...));
			if (!send_rpc_)
				throw std::logic_error("client isn't connected");

			auto result = send_rpc_(std::move(buffer), req_id);
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
			auto get_resp = send_rpc_(std::move(buffer), req_id);
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
			static const std::string magic_code = "xsimple_rpc";
			std::string buffer;
			uint32_t buffer_size = uint32_t(
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
		using send_rpc_result = std::pair <get_response,cancel_get_response>;
		std::function<send_rpc_result(std::string &&, int64_t)> send_rpc_;
		cancel_get_response cancel_get_response_;
	};
	
	class rpc_engine
	{
	private:
		struct connect_req
		{
			std::mutex mutex_;
			std::condition_variable cv_;
			xnet::connection conn_;
			std::string error_code_;
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
			std::mutex mtx_;
			std::condition_variable cv_;
			xnet::connection conn_;
			xnet::msgbox<std::shared_ptr<rpc_req>> msgbox_;
			std::list<std::string> send_buffer_list_;
			std::list<std::shared_ptr<rpc_req>> req_item_list_;
		};
	public:
		using get_response = std::function<std::string()>;
		using cancel_get_response = std::function<void()>;
		rpc_engine(std::size_t threadsize_)
		{
			proactor_pool_.set_size(threadsize_);
			init();
		}
		client connect(const std::string &ip, int port)
		{
			auto connect_req_ptr = std::make_shared<connect_req>();
			std::weak_ptr<connect_req> connect_req_wptr = connect_req_ptr;
			auto session = std::make_shared<rpc_session>();
			send_msg([ip, port, connect_req_wptr, this] {
				std::lock_guard<std::mutex> locker(connectors_mutex_);
				auto connector = proactor_pool_.get_current_proactor().get_connector();
				auto index = connector_index_++;
				connector.bind_fail_callback([index, connect_req_wptr](std::string errorc_code)
				{
					auto ptr = connect_req_wptr.lock();
					ptr->error_code_ = errorc_code;
					ptr->cv_.notify_one();
				})
				.bind_success_callback([index, connect_req_wptr](xnet::connection &&conn)
				{
					auto ptr = connect_req_wptr.lock();
					ptr->conn_ = std::move(conn);
					ptr->cv_.notify_one();
				});
				connector.async_connect(ip, port);
				connectors_.emplace(index, std::move(connector));
			});
			std::unique_lock<std::mutex> locker(connect_req_ptr->mutex_);
			connect_req_ptr->cv_.wait(locker, [&connect_req_ptr] {
				return connect_req_ptr->error_code_.size() || 
					connect_req_ptr->conn_.valid();
			});
			if (connect_req_ptr->error_code_.size())
				throw std::runtime_error("connect error: "+ connect_req_ptr->error_code_);
			
			client _client;
			_client.send_rpc_ = [this](std::string &&buffer, int64_t req_id){
				return send_rpc(0,buffer, req_id);
			};
		}
	private:
		using msgbox = xnet::msgbox<std::function<void()>>;

		template<typename T>
		void send_msg(T &&item)
		{
			std::lock_guard<std::mutex> locker(msgboxs_mutex_);
			msgboxs_[msgboxs_index_% msgboxs_.size()]->send(std::forward<T>(item));
			++msgboxs_index_;
		}
		std::pair<get_response, cancel_get_response>
			send_rpc(int64_t session_id, std::string &buffer, int64_t req_id)
		{
			auto item = std::make_shared<rpc_req>();
			item->req_buffer_ = std::move(buffer);
			item->req_id_ = req_id;

			std::lock_guard<std::mutex> locker(mutex_);
			auto itr = rpc_sessions_.find(session_id);
			if (itr == rpc_sessions_.end())
				throw std::runtime_error("can't find rpc_session");
			auto msg = item;
			auto session = itr->second;
			session->msgbox_.send(std::move(msg));

			return{ [session, item] {
				std::unique_lock<std::mutex> locker(session->mtx_);
				session->cv_.wait(locker);
				if (item->status_ != rpc_req::status::e_ok)
					throw std::logic_error(
						item->status_ == rpc_req::status::e_cancel ? 
						"rpc call cancel":
						"rpc call error");
				return item->result_;
			},[item] {
				item->status_ = rpc_req::status::e_cancel;
				item->cv_.notify_one();
			} };
		}
		void init()
		{
			msgboxs_.reserve(proactor_pool_.get_size());
			proactor_pool_.regist_run_before([this] {
				std::lock_guard<std::mutex> locker(mutex_);
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
		std::mutex mutex_;
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