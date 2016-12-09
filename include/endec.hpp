#pragma once
#include <type_traits>
#include <string>
#include <vector>

namespace detail
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

	template <typename T>
	inline typename std::enable_if<std::is_arithmetic<T>::value, std::size_t>::type
		get_sizeof(T)
	{
		return sizeof(T);
	}

	template <typename T>
	inline typename std::enable_if<std::is_same<std::string, T>::value, std::size_t>::type
		get_sizeof(const T &t)
	{
		return sizeof(uint32_t) + t.size();
	}

	template<typename C, typename T = C::value_type>
	inline typename std::enable_if<!is_string<C>::value, std::size_t>::type
		get_sizeof(const C &c)
	{
		return sizeof(uint32_t) + c.size() * get_sizeof(typename T());
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
		put(unsigned char *&ptr, bool value)
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

	template<typename T>
	inline typename std::enable_if < 
		std::is_arithmetic<T>::value || 
		std::is_same<T, std::string>::value , void > ::type
		put(uint8_t *& ptr, const std::vector<T> & vec)
	{
		put(ptr, (uint32_t)vec.size());
		for (auto &itr : vec)
			put<T>(ptr, itr);
	}

	template<typename C, typename T = typename C::value_type>
	inline typename std::enable_if<std::is_same<std::vector<T>, C>::value && !is_string<T>::value, std::vector<T>>::type
	 get(uint8_t *&ptr)
	{
		std::vector<T> vec;
		auto size = get<uint32_t>(ptr);
		for (uint32_t i = 0; i < size; ++i)
		{
			vec.emplace_back(get<T>(ptr));
		}
		return std::move(vec);
	}

	template<typename Last>
	void put_tp_impl(uint8_t *&ptr, Last&& last)
	{
		put(ptr, last);
	}

	template<typename First, typename ... Rest>
	void put_tp_impl(uint8_t *&ptr, First&& first, Rest&&...rest)
	{
		put(ptr, first);
		put_tp_impl(ptr, std::forward<Rest>(rest)...);
	}

	template<std::size_t ... Tndexes, typename ... Args>
	void put_tp_helper(uint8_t *&ptr, std::index_sequence<Tndexes...>, std::tuple<Args...>&& tup)
	{
		put_tp_impl(ptr, std::forward<Args>(std::get<Tndexes>(tup))...);
	}

	template<typename ... Args>
	void put_tp(uint8_t *&ptr, std::tuple<Args...>& tup)
	{
		put_tp_helper(ptr, std::make_index_sequence<std::tuple_size<decltype(tup)>::value>(), std::tuple<Args...>(tup));
	}

	template<typename ... Args>
	void put_tp(uint8_t *&ptr, std::tuple<Args...>&& tup)
	{
		using tuple_type = std::tuple<Args...>;
		put_tp_helper(ptr, std::make_index_sequence<4>(), std::forward<tuple_type>(tup));
	}
}



class endec
{
public:
	endec()
	{

	}
	endec(const std::string &string)
	{

	}
	endec(std::string &&string)
	{

	}
	template<typename T, class = decltype(&T::decode)>
	T get()
	{
		T value;
		value.decode(ptr_);
		return std::move(value);
	}
	 
private:
	uint8_t *ptr_;
	std::string buffer_;
};