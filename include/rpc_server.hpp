#pragma once
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
			return *this;
		}
	private:
	};
}