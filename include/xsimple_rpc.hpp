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
}