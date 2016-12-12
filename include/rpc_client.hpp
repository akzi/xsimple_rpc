#pragma once
namespace xsimple_rpc
{
	class client
	{
	private:
		using get_response = std::function<std::string()>;
		using cancel_get_response = std::function<void()>;
		using send_rpc_result = std::pair <get_response, cancel_get_response>;
		friend class rpc_engine;
	public:
		
		client(client &&other)
		{
			move_reset(std::move(other));
		}
		client &operator=(client && other)
		{
			move_reset(std::move(other));
		}
		~client()
		{
			if (close_rpc_)
				close_rpc_();
		}
		template<typename Proto, typename ...Args>
		auto rpc_call(Args ...args)
		{
			return rpc_call_impl(xutil::function_traits<typename Proto::func_type>(),
				Proto::rpc_name,
				std::forward<Args>(args)...);
		}
	private:
		client()
		{

		}
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
			using detail::make_req;
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
			using detail::make_req;
			auto req_id = gen_req_id();
			auto buffer = make_req(rpc_name, req_id, std::forward_as_tuple(args...));
			auto get_resp = send_req(std::move(buffer), req_id);
			set_cancel_get_response(std::move(get_resp.second));
			xnet::guard guard([&] {
				reset_cancel_get_response();
			});
			get_resp.first();
		}

		int64_t gen_req_id()
		{
			static std::atomic_int64_t req_id = 1;
			return req_id++;
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
}