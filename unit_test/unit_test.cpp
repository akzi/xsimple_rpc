#include <type_traits>
#include <vector>
#include "../include/xsimple_rpc.hpp"
#include "../../../xtest/include/xtest.hpp"
xtest_run;


struct MyStruct
{
	std::string hello;
	int world;
	std::vector<int> ints;
	std::size_t bytes()
	{
		return detail::get_sizeof(hello) + 
			detail::get_sizeof(world) + 
			detail::get_sizeof(ints);
	}
	void encode(uint8_t *&ptr)
	{
		detail::put(ptr, hello);
		detail::put(ptr, world);
		detail::put(ptr, ints);
	}
	void decode(uint8_t *&ptr)
	{
		hello = detail::get<decltype(hello)>(ptr);
		world = detail::get<decltype(world)>(ptr);
		ints = detail::get<decltype(ints)>(ptr);
	}
};


#include <tuple>
#include <iostream>
#include <string>
#include <utility>

int func1(int arg1, int , double arg3, const std::string& arg4)
{
	return 0;
}

int func2(int arg1, int arg2)
{
	std::cout << "call func2(" << arg1 << ", " << arg2 << ")" << std::endl;
	return arg1 + arg2;
}

template<typename F, typename T, std::size_t... I>
auto apply_impl(F f, const T& t, std::index_sequence<I...>) -> decltype(f(std::get<I>(t)...))
{
	return f(std::get<I>(t)...);
}

template<typename F, typename T>
auto apply(F f, const T& t) -> decltype(apply_impl(f, t, std::make_index_sequence<std::tuple_size<T>::value>()))
{
	return apply_impl(f, t, std::make_index_sequence<std::tuple_size<T>::value>());
}

template<typename Ret, typename ...Args>
int encode(const std::function<Ret(Args...)> &func, typename std::remove_reference<typename std::remove_const<Args>::type>::type &&...args)
{
	uint8_t buffer[1024];
	uint8_t*ptr = buffer;
	detail::put_tp(ptr, std::forward_as_tuple(args...));

	return 0;
}
XTEST_SUITE(endnc)
{
	XUNIT_TEST(encode)
	{
		MyStruct obj, ob2;
		obj.hello = "hello";
		obj.world = 192982772;
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
	XUNIT_TEST(decode)
	{
		encode(xutil::to_function(func1), 1, 2, 1, std::string(""));
	}
}