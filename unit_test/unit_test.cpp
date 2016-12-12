#include <type_traits>
#include <vector>
#include "../include/xsimple_rpc.hpp"
#include "../../xtest/include/xtest.hpp"
xtest_run;



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

using namespace xsimple_rpc::detail;

namespace funcs
{
	DEFINE_RPC_PROTO(hello, void(int, bool, const MyStruct&))
}



XTEST_SUITE(endnc)
{
	XUNIT_TEST(decode)
	{
		MyStruct obj, ob2;
		obj.hello = "hello";
		obj.world = 192982772;
		obj.ints = { 1,2,3,4,5 };

		std::string buffer;
		buffer.resize(obj.xget_sizeof());
		uint8_t *ptr = (uint8_t *)buffer.data();
		obj.xencode(ptr);

		ptr = (uint8_t *)buffer.data();
		ob2.xdecode(ptr);

		xassert(obj.hello== ob2.hello);
		xassert(obj.world == ob2.world);
		xassert(obj.ints == ob2.ints);
	}
}

XTEST_SUITE(rpc_server)
{
	XUNIT_TEST(regist)
	{
		using namespace xsimple_rpc;

		MyStruct obj;
		obj.hello = "hello";
		obj.world = 192982772;
		obj.ints = { 1,2,3,4,5 };


		rpc_server rpc_server_;
		rpc_server_.regist("hello world", [](int)->std::string { return{}; });

		

		rpc_server_.regist("func", &MyStruct::func, obj);
		rpc_server_.regist("func2", &MyStruct::func2, obj);
		rpc_server_.regist("func3", &MyStruct::func3, obj);
		rpc_server_.regist("get_mystruct", [&] {return obj; });
	}
}