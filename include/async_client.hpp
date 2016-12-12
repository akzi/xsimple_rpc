#pragma once
namespace xsimple_rpc
{
	class async_client
	{
	public:
		async_client(xnet::connection &&conn)
			:conn_(std::move(conn))
		{
			init();
		}
		async_client()
		{

		}
		~async_client()
		{
			conn_.close();
		}
		enum step
		{
			e_msg_len,
			e_msg_data,
		};

		void init()
		{
			conn_.regist_recv_callback([this](char *data, std::size_t len) {
				
				if (len == 0)
					goto laber_close;

				if (step_ == e_msg_len)
				{
					step_ = e_msg_data;
					uint8_t *ptr = (uint8_t*)data;
					uint32_t msg_len = endec::get<uint32_t>(ptr);
					conn_.async_recv(msg_len - sizeof(uint32_t));
					return;
				}
				else if (step_ == e_msg_data)
				{
					step_ = e_msg_len;
					uint8_t *ptr = (uint8_t*)data;
					if (endec::get<std::string>(ptr) != magic_code)
						goto laber_close;
					auto req_id = endec::get<uint64_t>(ptr);
					auto resp = endec::get<std::string>(ptr);
					if (callbacks_.empty() || callbacks_.front().first != req_id)
						goto laber_close;
					callbacks_.front().second(resp);
					callbacks_.pop_front();
					conn_.async_recv(sizeof(uint32_t));
					return;
				}
			laber_close:
				close();
				return;
			});

			conn_.regist_send_callback([this](std::size_t len) {

				if (len == 0)
				{
					conn_.close();
					return;
				}
				
				if (send_buffers_.empty())
				{
					is_send_ = false;
					return;
				}
				conn_.async_send(std::move(send_buffers_.front()));
				send_buffers_.pop_front();
			});
			conn_.async_recv(sizeof(uint32_t));
		}
		async_client(client &&other)
		{
		}
		async_client &operator=(client && other)
		{
			return *this;
		}
		template<typename Proto, typename Tuple, typename Callback>
		void rpc_call(Tuple &&tuple, Callback &&callback)
		{
			rpc_call_help(xutil::function_traits<typename Proto::func_type>(), 
				Proto::rpc_name(), 
				std::move(tuple), 
				xutil::to_function(std::forward<Callback>(callback)));
		}
		template<typename Proto, typename Callback>
		void rpc_call(Callback &&callback)
		{
			rpc_call_help(xutil::function_traits<typename Proto::func_type>(),
				Proto::rpc_name(),
				xutil::to_function(std::forward<Callback>(callback)));
		}

		void rpc_call_help(xutil::function_traits<void()>,
			const std::string &rpc_name,
			std::function<void()>&&callback)
		{
			using detail::make_req;
			auto id = gen_id();
			auto req = make_req(rpc_name, id);
			callbacks_.emplace_back(id, [this, callback](const std::string &) {
				callback();
			});
			if (is_send_)
			{
				send_buffers_.emplace_back(std::move(req));
				return;
			}
			is_send_ = true;
			conn_.async_send(std::move(req));
		}

		template<typename Ret>
		void rpc_call_help(xutil::function_traits<Ret()>,
			const std::string &rpc_name,
			std::function<void(typename std::remove_reference<Ret>::type)>&&callback)
		{
			using detail::make_req;
			auto id = gen_id();
			auto req = make_req(rpc_name, id);
			callbacks_.emplace_back(id, [this, callback](const std::string &resp) {
				uint8_t *ptr = (uint8_t *)resp.data();
				callback(endec::get<Ret>(ptr));
			});
			if (is_send_)
			{
				send_buffers_.emplace_back(std::move(req));
				return;
			}
			is_send_ = true;
			conn_.async_send(std::move(req));
		}

		template<typename Ret, typename ...Args>
		void rpc_call_help(xutil::function_traits<Ret(Args...)>,
			const std::string &rpc_name, 
			std::tuple<typename endec::remove_const_ref<Args>::type&&...> &&_tuple, 
			std::function<void(typename std::remove_reference<Ret>::type)>&&callback)
		{
			using detail::make_req;
			auto id = gen_id();
			auto req = make_req(rpc_name, id, std::move(_tuple));
			callbacks_.emplace_back(id, [this, callback](const std::string &resp) {
				uint8_t *ptr = (uint8_t *)resp.data();
				callback(endec::get<Ret>(ptr));
			});
			if (is_send_)
			{
				send_buffers_.emplace_back(std::move(req));
				return;
			}
			is_send_ = true;
			conn_.async_send(std::move(req));
		}
		void close()
		{

		}
		int64_t gen_id()
		{
			static int64_t id = 0;
			return ++id;
		}
		step step_ = e_msg_len;
		bool is_send_ = false;
		std::list<std::string> send_buffers_;
		std::list<std::pair<int64_t, std::function<void(const std::string &)>>> callbacks_;
		xnet::connection conn_;
	};
}