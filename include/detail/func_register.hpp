#pragma once
namespace xsimple_rpc
{
	class func_register
	{
	public:

		func_register()
		{

		}
		template<typename String, typename FuncType>
		void regist(String&&funcname, FuncType &&func)
		{
			add_function(std::forward<String>(funcname), xutil::to_function(std::move(func)));
		}
		template<typename Ret, typename Class, typename ...Args>
		void regist(std::string &&funcname, Ret(Class::*func)(Args...), Class &inst)
		{
			add_function(std::move(funcname), std::function<Ret(Args...)>{
				[&inst, func](Args&&...args)->Ret {
					return (inst.*func)(std::forward<Args>(args)...);
			}});
		}

	private:
		template<typename Ret, typename ...Args>
		void add_function(const std::string &funcname, std::function<Ret(Args...)> &&func)
		{
			return add_function(std::move(funcname), std::move(func), std::make_index_sequence<sizeof ...(Args)>{});
		}

		template<typename ...Args>
		void add_function(const std::string &funcname, std::function<void(Args...)> &&func)
		{
			std::function<std::string(Args...)> wrapper_func(
				[func](Args&&... args)->std::string {
					func(std::forward<Args>(args)...);
					return {};
				});
			return add_function(funcname, std::move(wrapper_func), std::make_index_sequence<sizeof ...(Args)>());
		}

		template<typename Ret, typename ...Args, std::size_t ...Indexes>
		void add_function(const std::string &funcname, std::function<Ret(Args...)> &&func, std::index_sequence<Indexes...>)
		{
			auto func_impl = [func](uint8_t *&ptr) ->std::string 
			{
				std::string buffer;
				std::tuple<typename endec::remove_const_ref<Args>::type...> tp 
					= detail::endec::get<typename endec::remove_const_ref<Args>::type...>(ptr);
				auto ret = func(std::get<Indexes>(tp)...);
				auto buffer_ptr_ = (uint8_t *)(buffer.data());
				buffer.resize(detail::endec::get_sizeof(ret));
				detail::endec::put(buffer_ptr_, ret);
				return std::move(buffer);
			};
			functions_.emplace(funcname, std::move(func_impl));
		}


		std::map<std::string, std::function<std::string(uint8_t *&)>> functions_;
	};
}