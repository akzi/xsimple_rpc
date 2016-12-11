#pragma once
namespace xsimple_rpc
{
	struct lock_free 
	{
		void lock()
		{

		}
		void unlock()
		{

		}
	};
	template<typename mutex = lock_free>
	class func_register
	{
	public:

		func_register()
		{

		}
		template<typename String, typename FuncType>
		void regist(String&&funcname, FuncType &&func)
		{
			regist_help(std::forward<String>(funcname), xutil::to_function(std::move(func)));
		}
		std::string invoke(const std::string &func_name, uint8_t *ptr)
		{
			auto func = get_function(func_name);
			return func(ptr);
		}
	private:
		template<typename Ret, typename ...Args>
		void regist_help(const std::string &funcname, std::function<Ret(Args...)> &&func)
		{
			return regist_impl(std::move(funcname), std::move(func), std::make_index_sequence<sizeof ...(Args)>{});
		}
		template<typename ...Args>
		void regist_help(const std::string &funcname, std::function<void(Args...)> &&func)
		{
			std::function<std::string(Args...)> wrapper_func(
				[func](Args&&... args)->std::string {
				func(std::forward<Args>(args)...);
				return{};
			});
			return regist_impl(funcname, std::move(wrapper_func), std::make_index_sequence<sizeof ...(Args)>());
		}
		template<typename Ret>
		void regist_help(const std::string &funcname, std::function<Ret(void)> &&func)
		{
			return regist_impl(std::move(funcname), std::move(func));
		}
		void regist_help(const std::string &funcname, std::function<void()> &&func)
		{
			return regist_impl(std::move(funcname), std::move([func]()->std::string {
				func();
				return{};
			}));
		}
		void regist_impl(const std::string &funcname, std::function<void()> &&func)
		{
			auto func_impl = [func](uint8_t*) ->std::string
			{
				std::string buffer;
				func();
				return{};
			};
			std::lock_guard<mutex> locker(mtex_);
			functions_.emplace(funcname, std::move(func_impl));
		}
		template<typename Ret>
		void regist_impl(const std::string &funcname, std::function<Ret()> &&func)
		{
			auto func_impl = [func](uint8_t*) ->std::string
			{
				std::string buffer;
				auto ret = func();
				auto buffer_ptr_ = (uint8_t *)(buffer.data());
				buffer.resize(detail::endec::get_sizeof(ret));
				detail::endec::put(buffer_ptr_, ret);
				return std::move(buffer);
			};
			std::lock_guard<mutex> locker(mtex_);
			functions_.emplace(funcname, std::move(func_impl));
		}

		template<typename Ret, typename ...Args, std::size_t ...Indexes>
		void regist_impl(const std::string &funcname, std::function<Ret(Args...)> &&func, std::index_sequence<Indexes...>)
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
			std::lock_guard<mutex> locker(mtex_);
			functions_.emplace(funcname, std::move(func_impl));
		}
		std::function<std::string(uint8_t *&)> &get_function(const std::string &funcname)
		{
			std::lock_guard<mutex> locker(mtex_);
			auto itr = functions_.find(funcname);
			if (itr == functions_.end())
				throw std::runtime_error("not find function with name:" + funcname);
			return itr->second;
		}
		mutex mtex_;
		std::map<std::string, std::function<std::string(uint8_t *&)>> functions_;
	};
}