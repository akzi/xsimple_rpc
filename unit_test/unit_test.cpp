#include <type_traits>
#include <vector>
#include "../include/xsimple_rpc.hpp"
#include "../../xtest/include/xtest.hpp"
xtest_run;


using namespace xsimple_rpc::detail;

struct MyStruct
{
	std::string hello;
	int world;
	std::vector<int> ints;
	std::size_t bytes() const
	{
		return endec::get_sizeof(hello) + 
			endec::get_sizeof(world) + 
			endec::get_sizeof(ints);
	}
	void encode(uint8_t *&ptr) const
	{
		endec::put(ptr, hello);
		endec::put(ptr, world);
		endec::put(ptr, ints);
	}
	void decode(uint8_t *&ptr)
	{
		hello = endec::get<decltype(hello)>(ptr);
		world = endec::get<decltype(world)>(ptr);
		ints = endec::get<decltype(ints)>(ptr);
	}

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
		obj.world = 192982772; 20 + 4 + 4 + 4;
		obj.ints = { 1,2,3,4,5 };
		std::string buffer_;
		buffer_.resize(obj.bytes());
		uint8_t *ptr = (uint8_t*)buffer_.data();
		obj.encode(ptr);
		ptr = (uint8_t*)buffer_.data();
		ob2.decode(ptr);

		xassert(ob2.hello == obj.hello);
		xassert(ob2.world == obj.world);
		xassert(ob2.ints == obj.ints);
	}
}

XTEST_SUITE(rpc_server)
{
	XUNIT_TEST(regist)
	{
		using namespace xsimple_rpc;

		rpc_server rpc_server_;
		rpc_server_.regist("hello world", [](int)->std::string { return{}; });

		MyStruct obj;
		rpc_server_.regist("func", &MyStruct::func, obj);
		rpc_server_.regist("func2", &MyStruct::func2, obj);
		rpc_server_.regist("func3", &MyStruct::func3, obj);
	}
}