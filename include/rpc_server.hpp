#pragma once
namespace xsimple_rpc
{
	class rpc_server
	{
	public:
		rpc_server(std::size_t io_threads = std::thread::hardware_concurrency())
			:rpc_engine_(io_threads)
		{
		
		}
		template<typename Rpc_Func>
		rpc_server &regist(const std::string &name, Rpc_Func && rpc_func)
		{
			func_register_.regist(name, std::forward<Rpc_Func >(rpc_func));
			return *this;
		}
		template<typename Ret, typename Class, typename ...Args>
		rpc_server &regist(std::string &&funcname, Ret(Class::*func)(Args...), Class &inst)
		{
			func_register_.regist(std::move(funcname), std::function<Ret(Args...)>{
				[&inst, func](Args&&...args)->Ret {
					return (inst.*func)(std::forward<Args>(args)...);
				}});
			return *this;
		}
		template<typename Ret, typename Class, typename ...Args>
		rpc_server &regist(std::string &&funcname, Ret(Class::*func)(Args...)const, Class &inst)
		{
			func_register_.regist(std::move(funcname), std::function<Ret(Args...)>{
				[&inst, func](Args&&...args)->Ret {
					return (inst.*func)(std::forward<Args>(args)...);
				}});
			return *this;
		}
		rpc_server &bind(const std::string &ip_, int port)
		{
			rpc_engine_.get_proactor_pool().regist_accept_callback(
				std::bind(&rpc_server::accept_callback, this, std::placeholders::_1));
			rpc_engine_.get_proactor_pool().bind(ip_, port);
			return *this;
		}
		rpc_server &start()
		{
			rpc_engine_.start();
			return *this;
		}
	private:
		struct rpc_session
		{
			void do_send_resp(std::string &&resp)
			{
				if (is_send_)
				{
					resp_list_.emplace_back(std::move(resp));
					return;
				}
				conn_.async_send(std::move(resp));
			}
			void init()
			{
				conn_.regist_send_callback([this](std::size_t len) {
					if (len == 0)
					{
						conn_.close();
						close_callback_();
						return;
					}
					if (resp_list_.empty())
					{
						is_send_ = false;
						return;
					}
					conn_.async_send(std::move(resp_list_.front()));
					resp_list_.pop_front();
				});
			}
			bool is_send_ = false;
			std::list<std::string> resp_list_;
			std::function<void()> close_callback_;
			xnet::connection conn_;
		};
		void accept_callback(xnet::connection &&conn)
		{
			enum recv_step
			{
				e_msg_len,
				e_msg_data,
			};
			std::lock_guard<std::mutex> locker(conns_mutex_);
			auto itr = conns_.emplace(conns_.end(),rpc_session());
			auto &session = conns_.back();
			session.conn_ = std::move(conn);
			session.init();
			auto &_conn = session.conn_;
			recv_step step = e_msg_len;

			auto close_session = [this, &_conn, itr] {
				_conn.close();
				std::lock_guard<std::mutex> locker(conns_mutex_);
				conns_.erase(itr);
				return;
			};
			session.close_callback_ = close_session;
			_conn.regist_recv_callback([itr, step,close_session,&_conn,&session,this] 
			(char *data, std::size_t len) mutable
			{
				if (len == 0)
				{
					goto close_conn;
				}
				if (step == e_msg_len && len == sizeof(uint32_t))
				{
					step = e_msg_data;
					uint8_t *ptr = (uint8_t*)data;
					_conn.async_recv(detail::endec::get<uint32_t>(ptr) - sizeof(uint32_t));
					return;
				}else if (step == e_msg_data)
				{
					step = e_msg_len;
					if (!recv_msg_callback(data, len, session))
						goto close_conn;
					_conn.async_recv(sizeof(uint32_t));
					return;
				}
			close_conn:
				close_session();
			});
			_conn.async_recv(sizeof(uint32_t));
		}
		bool recv_msg_callback(char *data, std::size_t len, rpc_session &session)
		{
			uint8_t *ptr = (uint8_t*)data;
			if (detail::endec::get<std::string>(ptr) != magic_code)
				return false;
			uint64_t req_id = detail::endec::get<uint64_t>(ptr);
			std::string req_name = detail::endec::get<std::string>(ptr);
			try
			{
				auto result = func_register_.invoke(req_name, ptr);
				session.do_send_resp(detail::make_resp(req_name, req_id, std::move(result)));
			}
			catch (const std::exception& e)
			{
				std::cout << e.what() << std::endl;
				return false;
			}
			return true;
		}
		func_register<std::mutex> func_register_;
		std::mutex conns_mutex_;
		std::list<rpc_session> conns_;
		rpc_engine rpc_engine_;
	};
}