#pragma once
namespace xsimple_rpc
{
	class server
	{
	public:
		server(std::size_t io_threads = std::thread::hardware_concurrency())
			:rpc_engine_(io_threads)
		{
			
		}
		template<typename Rpc_Func>
		server &regist(const std::string &name, Rpc_Func && rpc_func)
		{
			func_register_.regist(name, std::forward<Rpc_Func >(rpc_func));
			return *this;
		}
		server &bind(std::string &ip_, int port)
		{
			rpc_engine_.get_proactor_pool().bind(ip_, port);
			rpc_engine_.get_proactor_pool().regist_accept_callback(
				std::bind(&server::accept_callback, this, std::placeholders::_1));
			return *this;
		}
	private:
		void accept_callback(xnet::connection &&conn)
		{
			enum recv_step
			{
				e_msg_len,
				e_msg_data,
			};
			std::lock_guard<std::mutex> locker(conns_mutex_);
			auto itr = conns_.emplace(conns_.end(), std::move(conn));
			auto &_conn = conns_.back();
			recv_step step = e_msg_len;
			_conn.regist_recv_callback([itr, step,this](char *data, std::size_t len) mutable 
			{
				if (len == 0)
				{
					goto close_conn;
				}
				if (step == e_msg_len && len == sizeof(uint32_t))
				{
					step = e_msg_data;
					uint8_t *ptr = (uint8_t*)data;
					itr->async_recv(detail::endec::get<uint8_t>(ptr));
					return;
				}else if (step == e_msg_data)
				{
					recv_msg_callback(data, len);
					itr->async_recv(sizeof(uint32_t));
					return;
				}
			close_conn:
				itr->close();
				std::lock_guard<std::mutex> locker(conns_mutex_);
				conns_.erase(itr);
				return;
			});
		}
		bool recv_msg_callback(char *data, std::size_t len)
		{
			uint8_t *ptr = (uint8_t*)data;
			if (detail::endec::get<std::string>(ptr) != magic_code)
				return false;
			auto req_id = detail::endec::get<uint64_t>(ptr);
			auto req_name = detail::endec::get<std::string>(ptr);
		}

		func_register func_register_;
		std::mutex conns_mutex_;
		std::list<xnet::connection> conns_;
		rpc_engine rpc_engine_;
	};
}