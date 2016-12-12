#include <type_traits>
#include <vector>
#include "../include/xsimple_rpc.hpp"
#include "../../xtest/include/xtest.hpp"
xtest_run;

#if 0

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
	XUNIT_TEST(hello)
	{
		int i = 0;
		std::pair<int,int> ret = std::make_pair(i++,++i);

		std::cout << ret.first <<": "<< ret.second << std::endl;
	}
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

	XUNIT_TEST(map_put)
	{
		std::map<std::string, MyStruct> map;
		std::string buffer;
		buffer.resize(endec::get_sizeof(map));
		uint8_t *ptr = (uint8_t *)buffer.data();
		endec::put(ptr, map);
	}

	XUNIT_TEST(map_get)
	{
		std::map<std::string, MyStruct> map;
		std::string buffer;
		buffer.resize(endec::get_sizeof(map));
		uint8_t *ptr = (uint8_t *)buffer.data();
		endec::put(ptr, map);

		ptr = (uint8_t *)buffer.data();
		auto res = endec::get<decltype(map)>(ptr);
	}

	XUNIT_TEST(set_put)
	{
		std::set<std::string> set;

		std::string buffer;
		buffer.resize(endec::get_sizeof(set));
		uint8_t *ptr = (uint8_t *)buffer.data();
		endec::put(ptr, set);
	}

	XUNIT_TEST(set_get)
	{
		std::set<std::string> obj;

		std::string buffer;
		buffer.resize(endec::get_sizeof(obj));
		uint8_t *ptr = (uint8_t *)buffer.data();
		endec::put(ptr, obj);

		ptr = (uint8_t *)buffer.data();
		auto res = endec::get<decltype(obj)>(ptr);
	}

	XUNIT_TEST(list_put)
	{
		std::list<std::string> obj;

		std::string buffer;
		buffer.resize(endec::get_sizeof(obj));
		uint8_t *ptr = (uint8_t *)buffer.data();
		endec::put(ptr, obj);
	}

	XUNIT_TEST(list_get)
	{
		std::list<std::string> obj;

		std::string buffer;
		buffer.resize(endec::get_sizeof(obj));
		uint8_t *ptr = (uint8_t *)buffer.data();
		endec::put(ptr, obj);

		ptr = (uint8_t *)buffer.data();
		auto res = endec::get<decltype(obj)>(ptr);
	}

	XUNIT_TEST(pair_put)
	{
		std::pair<std::string, std::string> obj;

		std::string buffer;
		buffer.resize(endec::get_sizeof(obj));
		uint8_t *ptr = (uint8_t *)buffer.data();
		endec::put(ptr, obj);
	}

	XUNIT_TEST(pair_get)
	{
		std::pair<std::string, std::string> obj;

		std::string buffer;
		buffer.resize(endec::get_sizeof(obj));
		uint8_t *ptr = (uint8_t *)buffer.data();
		endec::put(ptr, obj);

		ptr = (uint8_t *)buffer.data();
		auto res = endec::get<decltype(obj)>(ptr);
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
#endif

XTEST_SUITE(async_client)
{
	XUNIT_TEST(call)
	{
		struct rpc
		{
			DEFINE_RPC_PROTO(add, int(int, int));
		};

		using namespace xsimple_rpc;
		async_client client;
		client.rpc_call<rpc::add>(std::forward_as_tuple(1,2), [](int result ) {
			std::cout << result << std::endl;
		});
	}
}







#if  0
#include <iostream>
#include <type_traits>
#include <utility>
#include <string>

namespace DSH {


	struct AnyType
	{
		AnyType(...) {};
	};


	//!
	//! detail is the my coock
	//!
	namespace DSH_Detail {


		template<typename Callable, typename... Args>
		inline auto Invoke(Callable&& c, Args&&... args) -> decltype(std::forward<Callable>(c)(std::forward<Args>(args)...)) {
			return std::forward<Callable>(c)(std::forward<Args>(args)...);
		}


	} /// End of DSH_Detail


	template<typename Callable, typename... Args>
	struct Invokeable
	{
		template<typename Other = Callable>
		static auto virtual_invoke(std::nullptr_t) -> decltype(DSH_Detail::Invoke(std::declval<Other>(), std::declval<Args>()...), std::declval<std::true_type>());
		static auto virtual_invoke(...)->std::false_type;
		static constexpr bool value = std::is_same<typename std::remove_reference<decltype(virtual_invoke(nullptr))>::type, std::true_type>::value;
	};


	struct CallableWrapper
	{
		template<typename Callable, typename... Args>
		static typename std::enable_if<Invokeable<Callable, Args...>::value>::type
			Invoke(Callable&& c, Args&&... args)
		{
			DSH_Detail::Invoke(std::forward<Callable>(c), std::forward<Args>(args)...);
		}


		template<typename Callable, typename... Args>
		static typename std::enable_if<!(Invokeable<Callable, Args...>::value)>::type
			Invoke(Callable&& c, Args&&... args) { }
	};


	struct ElementWrapper {


		template<
			std::size_t N, typename Tuple, typename Callable
		>
			static void
			call(Tuple&& t, std::size_t index, Callable&& f, typename std::enable_if<N != 0>::type* = nullptr)
		{
			if (index == N) CallableWrapper::Invoke(std::forward<Callable>(f), std::get<N>(std::forward<Tuple>(t)));
			else
				ElementWrapper::call<N - 1>(std::forward<Tuple>(t), index, std::forward<Callable>(f));
		}




		template<
			std::size_t N, typename Tuple, typename Callable
		>
			static void
			call(Tuple&& t, std::size_t index, Callable&& f, typename std::enable_if<N == 0>::type* = nullptr)
		{
			if (index == N) CallableWrapper::Invoke(std::forward<Callable>(f), std::get<N>(std::forward<Tuple>(t)));
		}




	};




} /// NameSpace DSH




namespace DSH {
	template<typename Tuple, typename Callable>
	void index_tuple(Tuple&&t, std::size_t index, Callable&& f)
	{
		constexpr std::size_t size = std::tuple_size<typename std::remove_reference<Tuple>::type>::value;
		ElementWrapper::call<size - 1>(
			std::forward<Tuple>(t),
			index,
			std::forward<Callable>(f)
			);
	}




}/// NameSpace DSH








int test()
{
	std::tuple<int, double, bool, std::string> t{ 1, 2.1, true, "abc" };


	DSH::index_tuple(t, 0, [](int x) { std::cout << x << std::endl; });
	DSH::index_tuple(t, 1, [](double x) { std::cout << x << std::endl; });
	DSH::index_tuple(t, 2, [](bool x) { std::cout << std::boolalpha << x << std::endl; });
	DSH::index_tuple(t, 3, [](std::string x) { std::cout << x << std::endl; });
	DSH::index_tuple(t, 3, [](int x) { std::cout << x << std::endl; });


	return 0;
}

#endif;