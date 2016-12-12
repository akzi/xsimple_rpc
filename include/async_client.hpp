#pragma once
namespace xsimple_rpc
{
	class async_client
	{
	public:
		async_client(xnet::connection &&conn)
			:conn_(std::move(conn))
		{

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
			conn_.regist_recv_callback([this](char *data, std::size_t len) mutable{
				if (len == 0)
				{
					close();
					return;
				}
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
					{
						close();
						return;
					}
					auto req_id = endec::get<uint64_t>(ptr);
					auto resp = endec::get<std::string>(ptr);
				}
			});

			conn_.regist_send_callback([this](std::size_t len) {
				if (len == 0)
				{
					conn_.close();
					return;
				}
			});
		}
		async_client(client &&other)
		{
		}
		async_client &operator=(client && other)
		{
			return *this;
		}
		template<typename Proto, typename Tuple = typename xutil::function_traits<typename Proto::func_type>::tupe_type, typename Callback>
		void rpc_call(const Tuple &tuple, Callback &&callback)
		{
			rpc_call_help(Proto::rpc_name(), tuple, xutil::to_function(callback));
		}
		template<typename Ret, typename ...Args>
		void rpc_call_help(const std::string &rpc_name, const std::tuple<Args...> &_tuple, std::function<void(Ret)>&&callback)
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