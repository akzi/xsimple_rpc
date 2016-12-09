#pragma once
#include "detail/detail.hpp"
namespace xsimple_rpc
{
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
			ppro_.bind(ip_, port);
			return *this;
		}
	private:
		
		xnet::proactor_pool ppro_;
		
	};

	class client
	{
	public:
		template<typename Proto, typename ...Args>
		auto rpc_call(Args && ...args)
		{
			return rpc_call_impl(xutil::function_traits<typename Proto::func_type>(), Proto::rpc_name, std::forward<Args>(args)...);
		}

	private:

		template<typename Ret, typename ...Args>
		Ret rpc_call_impl(
			const xutil::function_traits<Ret(Args...)>&, 
			const std::string &rpc_name, 
			typename std::remove_reference<typename std::remove_const<Args>::type>::type&& ...args)
		{
			std::string buffer;
			auto tp = std::forward_as_tuple(args...);
			buffer.resize(endec::get_sizeof(tp) + endec::get_sizeof(rpc_name));
			uint8_t*ptr = (uint8_t*)buffer.data();
			endec::put(ptr, rpc_name);
			endec::put(ptr, std::move(tp));

			return endec::get<Ret>(ptr);
		}

		template<typename ...Args>
		void rpc_call_impl(
			const xutil::function_traits<void(Args...)>&,
			const std::string &rpc_name,
			typename std::remove_reference<typename std::remove_const<Args>::type>::type&& ...args)
		{
			std::string buffer;
			auto tp = std::forward_as_tuple(args...);
			buffer.resize(endec::get_sizeof(tp) + endec::get_sizeof(rpc_name));
			uint8_t*ptr = (uint8_t*)buffer.data();
			endec::put(ptr, rpc_name);
			endec::put(ptr, std::move(tp));
		}
	};
}