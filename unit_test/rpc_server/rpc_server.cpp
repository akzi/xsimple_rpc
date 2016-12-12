#include "../../Include/xsimple_rpc.hpp"

int main()
{
	xsimple_rpc::rpc_server server;
	server.bind("127.0.0.1", 9001);
	server.regist("add", [](int a, int &b) { return a + b; });
	server.regist("hello", [](const std::string &hello) { return std::string("hello world"); });
	server.regist("add_str", [](int a, const std::string &str ) {return str + std::to_string(a); });


	//struct test

	struct MyStruct
	{
		std::string hello;
		int world;
		std::vector<int> ints;

		XENDEC(hello, world, ints);

		std::string func(int, int)
		{
			return hello;
		}

		std::string func2()
		{
			return hello;
		}
		void func3()const
		{
			return;
		}
	};
	MyStruct obj;
	obj.hello = "hello";
	obj.world = 192982772;
	obj.ints = { 1,2,3,4,5 };

	server.regist("func",&MyStruct::func,obj);
	server.regist("func2", &MyStruct::func2, obj);
	server.regist("func3", &MyStruct::func3, obj);
	server.regist("get_obj", [&] { return obj; });
	server.start();
	getchar();
}