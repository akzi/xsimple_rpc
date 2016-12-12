#include "../../Include/xsimple_rpc.hpp"

int main()
{
	xsimple_rpc::rpc_server server;
	server.bind("127.0.0.1", 9001);
	server.regist("add", [](int a, int b) { return a + b; });
	server.regist("hello", [](std::string &hello) { return std::string("hello world"); });
	server.regist("add_str", [](int a, std::string str) {return str + std::to_string(a); });
	server.start();
	getchar();
}