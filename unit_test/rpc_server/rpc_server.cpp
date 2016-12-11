#include "../../Include/xsimple_rpc.hpp"

int main()
{
	xsimple_rpc::rpc_server server;
	server.bind("127.0.0.1", 9000);
	server.regist("add", [](int a, int b) { return a + b; });
	server.start();
	getchar();
}