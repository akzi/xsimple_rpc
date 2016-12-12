#include "../../Include/xsimple_rpc.hpp"




//define rpc
namespace rpc
{
	DEFINE_RPC_PROTO(add, int(int,int));
	DEFINE_RPC_PROTO(hello, std::string(std::string &));
	DEFINE_RPC_PROTO(add_str, std::string(int, std::string));
}
int main()
{
	xsimple_rpc::rpc_engine engine;
	engine.start();
	try
	{
		auto client = engine.connect("127.0.0.1", 9001, 0);
		int result = client.rpc_call<rpc::add>(1, 2);
		std::cout << "result: " << result << std::endl;
		auto result2 = client.rpc_call<rpc::hello>(std::string("hello"));
		std::cout << result2 << std::endl;;

		std::cout << client.rpc_call<rpc::add_str>(12345,std::string("hello")) << std::endl;;
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

	getchar();
	return 0;
}