#pragma once
#include <type_traits>
#include <string>
#include <vector>
#include <list>

#define DEFINE_RPC_PROTO(name, proto)\
struct name\
{\
	using func_type = proto ;\
	static constexpr char *rpc_name = #name;\
};

namespace endec
{
	template <typename>
	struct _Is_string :std::false_type
	{
	};

	template<>
	struct _Is_string<std::string> :std::true_type
	{
	};
	template<typename T>
	struct is_string :_Is_string<typename std::remove_reference<typename std::remove_const<T>::type>::type>
	{

	};

	template<typename T>
	struct remove_const_ref
	{
		using type = typename std::remove_reference<typename std::remove_const<T>::type>::type;
	};


	template <typename T>
	inline typename std::enable_if<std::is_arithmetic<T>::value, std::size_t>::type
		get_sizeof(T)
	{
		return sizeof(T);
	}

	template <typename T>
	inline typename std::enable_if<std::is_member_function_pointer<decltype(&T::bytes)>::value, std::size_t>::type
		get_sizeof(const T& value)
	{
		return value.bytes();
	}

	template<typename Container, typename T = Container::value_type>
	inline std::size_t get_sizeof(const Container &c)
	{
		return sizeof(uint32_t) + c.size() * get_sizeof(typename T());
	}
	
	template<typename First, typename ...Rest>
	inline std::size_t get_sizeof(const First &first, const Rest &...rest)
	{
		return get_sizeof(first) + get_sizeof(rest...);
	}

	template<typename ...Args, std::size_t ...Indexes>
	inline std::size_t get_sizeof(const std::tuple<Args...> &tp, std::index_sequence<Indexes...>)
	{
		return get_sizeof(std::get<Indexes>(tp)...);
	}
	template<typename ...Args>
	inline std::size_t get_sizeof(const std::tuple<Args...> &tp)
	{
		return get_sizeof(tp, std::make_index_sequence<sizeof...(Args)>());
	}

	template<typename T>
	inline typename std::enable_if<std::is_member_function_pointer<decltype(&T::decode)>::value, T>::type
		get(uint8_t *&ptr)
	{
		T value;
		value.decode(ptr);
		return std::move(value);
	}

	template<typename T>
	inline typename std::enable_if<std::is_member_function_pointer<decltype(&T::encode)>::value, void>::type
		put(uint8_t *&ptr, const T &value)
	{
		value.encode(ptr);
	}

	//bool 
	template<typename T>
	inline typename std::enable_if<std::is_same<T, bool>::value, T>::type
		get(uint8_t *&ptr)
	{
		uint8_t value = ptr[0];
		ptr += sizeof(value);
		return value > 0;
	}
	template<typename T>
	inline typename std::enable_if<std::is_same<T, bool>::value, void>::type
		put(unsigned char *&ptr, T value)
	{
		*ptr = value ? 1 : 0;
		ptr += sizeof(char);
	}

	//integral
	template<typename T>
	inline typename std::enable_if<std::is_same<T, uint8_t>::value, T>::type
		get(uint8_t *&ptr)
	{
		uint8_t value = ptr[0];
		ptr += sizeof(value);
		return value;
	}

	template<typename T>
	inline typename std::enable_if<std::is_same<T, int8_t>::value, T>::type
		get(uint8_t &&buffer)
	{
		return static_cast<T>(get<uint8_t>(ptr));
	}

	template<typename T>
	inline typename std::enable_if<std::is_same<T, uint8_t>::value, void>::type
		put(unsigned char *&ptr, T value)
	{
		*ptr = value;
		ptr += sizeof(value);
	}

	template<typename T>
	inline typename std::enable_if<std::is_same<T, int8_t>::value, void>::type
		put(unsigned char *&ptr, T value)
	{
		return put(ptr, (uint8_t)value);
	}


	template<typename T>
	inline typename std::enable_if<std::is_same<T, uint16_t>::value, T>::type
		get(uint8_t *&ptr)
	{
		uint16_t value = 
			(((uint16_t)ptr[0]) << 8) |
			((uint16_t)ptr[1]);
		ptr += sizeof(value);
		return value;
	}

	template<typename T>
	inline typename std::enable_if<std::is_same<T, uint16_t>::value, void>::type
		put(uint8_t *ptr, T value)
	{
		ptr[0] = (unsigned char)(((value) >> 8) & 0xff);
		ptr[1] = (unsigned char)(value & 0xff);
		ptr += sizeof(value);
	}

	template<typename T>
	inline typename std::enable_if<std::is_same<T, int16_t>::value, T>::type
		get(std::string &&buffer)
	{
		return static_cast<T>(get<uint16_t>(ptr));
	}

	template<typename T>
	inline typename std::enable_if<std::is_same<T, int16_t>::value, void>::type
		put(unsigned char *&ptr, T value)
	{
		return put(ptr, (uint16_t)value);
	}

	template<typename T>
	inline typename std::enable_if<std::is_same<T, uint32_t>::value, T>::type
		get(uint8_t *&ptr)
	{
		uint32_t value =
			(((uint32_t)ptr[0]) << 24) |
			(((uint32_t)ptr[1]) << 16) |
			(((uint32_t)ptr[2]) << 8) |
			((uint32_t)ptr[3]);
		ptr += sizeof(value);
		return value;
	}


	template<typename T>
	inline typename std::enable_if<std::is_same<T, uint32_t>::value, void>::type
		put(unsigned char *&ptr, T value)
	{
		ptr[0] = (unsigned char)(((value) >> 24) & 0xff);
		ptr[1] = (unsigned char)(((value) >> 16) & 0xff);
		ptr[2] = (unsigned char)(((value) >> 8) & 0xff);
		ptr[3] = (unsigned char)(value & 0xff);
		ptr += sizeof(value);
	}

	template<typename T>
	inline typename std::enable_if<std::is_same<T, int32_t>::value, T>::type
		get(uint8_t *&ptr)
	{
		return static_cast<T>(get<uint32_t>(ptr));
	}

	template<typename T>
	inline typename std::enable_if<std::is_same<T, int32_t>::value, void>::type
		put(unsigned char *&ptr, T value)
	{
		return put(ptr, uint32_t(value));
	}

	template<typename T>
	inline typename std::enable_if<std::is_same<T, uint64_t>::value, T>::type
		get(unsigned char *&ptr)
	{
		uint64_t value =
				((((uint64_t)ptr[0]) << 56) |
				(((uint64_t)ptr[1]) << 48) |
				(((uint64_t)ptr[2]) << 40) |
				(((uint64_t)ptr[3]) << 32) |
				(((uint64_t)ptr[4]) << 24) |
				(((uint64_t)ptr[5]) << 16) |
				(((uint64_t)ptr[6]) << 8) |
				((uint64_t)ptr[7]));
		ptr += sizeof(value);
		return value;
	}

	template<typename T>
	inline typename std::enable_if<std::is_same<T, uint64_t>::value, void>::type
		put(unsigned char *&ptr, T value)
	{
		ptr[0] = (unsigned char)(((value) >> 56) & 0xff);
		ptr[1] = (unsigned char)(((value) >> 48) & 0xff);
		ptr[2] = (unsigned char)(((value) >> 40) & 0xff);
		ptr[3] = (unsigned char)(((value) >> 32) & 0xff);
		ptr[4] = (unsigned char)(((value) >> 24) & 0xff);
		ptr[5] = (unsigned char)(((value) >> 16) & 0xff);
		ptr[6] = (unsigned char)(((value) >> 8) & 0xff);
		ptr[7] = (unsigned char)(value & 0xff);
		ptr += sizeof(value);
	}

	template<typename T>
	inline typename std::enable_if<std::is_same<T, int64_t>::value, T>::type
		get(uint8_t *&ptr)
	{
		return static_cast<T>(get<uint16_t>(ptr));
	}

	template<typename T>
	inline typename std::enable_if<std::is_same<T, int64_t>::value, void>::type
		put(unsigned char *&ptr, T value)
	{
		return put(ptr, uint64_t(value));
	}

	template<typename T>
	inline typename std::enable_if<std::is_floating_point<T>::value, T>::type
		get(std::string &&buffer)
	{
		auto value = *reinterpret_cast<T*>(ptr);
		ptr += sizeof(value);
		return value;
	}
	template<typename T>
	inline typename std::enable_if<std::is_floating_point<T>::value, void>::type
		put(unsigned char *&ptr, T value)
	{
		T &ref = *reinterpret_cast<T*>(ptr);
		ref = value;
		ptr += sizeof(value);
	}

	//string 
	template<typename T>
	inline typename std::enable_if<std::is_same<T, std::string>::value, T>::type
		get(uint8_t *&ptr)
	{
		auto len = get<uint32_t>(ptr);
		std::string result((char*)ptr, len);
		ptr += len;
		return std::move(result);
	}

	template<typename T>
	inline typename std::enable_if<std::is_same<T, std::string>::value, void>::type
		put(uint8_t *&ptr, const T &value)
	{
		put(ptr, (uint32_t)value.size());
		memcpy(ptr, value.data(), value.size());
		ptr += value.size();
	}

	template<typename Container, typename value_type = typename Container::value_type>
	inline typename std::enable_if<!is_string<Container>::value, void>::type
		put(uint8_t *& ptr, const Container &container)
	{
		put(ptr, (uint32_t)container.size());
		for (auto &itr : container)
			put<value_type>(ptr, itr);
	}

	template<typename Container, typename value_type = typename Container::value_type>
	inline typename std::enable_if<!is_string<Container>::value &&
		std::is_member_function_pointer<decltype(&Container::emplace_back<value_type>)>::value, Container>::type
		get(uint8_t *&ptr)
	{
		Container container;
		auto size = get<uint32_t>(ptr);
		for (uint32_t i = 0; i < size; ++i)
			container.emplace_back(get<value_type>(ptr));
		return std::move(container);
	}

	template<typename Last>
	inline void put_tp_impl(uint8_t *&ptr, Last&& last)
	{
		put(ptr, last);
	}

	template<typename First, typename ... Rest>
	inline void put_tp_impl(uint8_t *&ptr, First&& first, Rest&&...rest)
	{
		put(ptr, first);
		put_tp_impl(ptr, std::forward<Rest>(rest)...);
	}

	template<std::size_t ... Tndexes, typename ... Args>
	inline void put_tp_helper(uint8_t *&ptr, std::index_sequence<Tndexes...>, std::tuple<Args...>&& tup)
	{
		put_tp_impl(ptr, std::forward<Args>(std::get<Tndexes>(tup))...);
	}

	template<std::size_t ... Tndexes, typename ... Args>
	inline void put_tp_helper(uint8_t *&ptr, std::index_sequence<Tndexes...>, std::tuple<Args...>& tup)
	{
		put_tp_impl(ptr, std::forward<Args>(std::get<Tndexes>(tup))...);
	}
	
	template<typename ... Args>
	inline void put(uint8_t *&ptr, std::tuple<Args...>& tup)
	{
		put_tp_helper(ptr, std::make_index_sequence<sizeof...(Args)>(), tup);
	}

	template<typename ... Args>
	inline void put(uint8_t *&ptr, std::tuple<Args...>&& tup)
	{
		using tuple_type = std::tuple<Args...>;
		put_tp_helper(ptr, std::make_index_sequence<sizeof...(Args)>(), std::forward<tuple_type>(tup));
	}

	//
	template<typename Last>
	inline std::tuple<Last> get_tp_impl(uint8_t *&ptr, std::index_sequence<0>)
	{
		using value_type = typename std::remove_reference<typename std::remove_const<Last>::type>::type;
		return std::forward_as_tuple(get<value_type>(ptr));
	}

	template<typename First, typename ... Rest, std::size_t ... Tndexes>
	inline std::tuple<First, Rest...> get_tp_impl(uint8_t *&ptr, std::index_sequence<Tndexes...>)
	{
		using value_type = typename std::remove_reference<typename std::remove_const<First>::type>::type;
		return std::tuple_cat(std::forward_as_tuple(get<value_type>(ptr)), get_tp_impl<Rest...>(ptr, std::make_index_sequence<sizeof ...(Rest)>()));
	}

	template<typename ...Args>
	inline std::tuple<Args...> get_tp_helper(uint8_t *&ptr)
	{
		return get_tp_impl<Args...>(ptr, std::make_index_sequence<sizeof ...(Args)>());
	}

	template<typename Tuple, typename First = typename std::tuple_element<0, Tuple>::type>
	inline typename std::enable_if<1, Tuple>::type
		get(uint8_t *&ptr)
	{
		return get_tp_helper<Tuple>(ptr);
	}
}