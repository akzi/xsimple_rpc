#include "../../Include/xsimple_rpc.hpp"




//define rpc
namespace rpc
{
	DEFINE_RPC_PROTO(add, int(int,int));
}
int main()
{
	xsimple_rpc::rpc_engine engine;
	engine.start();
	try
	{
		auto client = engine.connect("127.0.0.1", 9000, 0);
		int result = client.rpc_call<rpc::add>(1, 2);
		std::cout << "result: " << result << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

	getchar();
	return 0;
}