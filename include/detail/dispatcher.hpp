#pragma once
namespace xsimple_rpc
{
	class dispatcher
	{
	public:
		typedef std::function<xjson::obj_t(const xjson::obj_t&)> decorate_function;

		dispatcher()
		{

		}

		template<typename FuncType>
		void regist(std::string &&funcname, FuncType func)
		{
			add_function(std::move(funcname), to_function(func));
		}
		template<typename Ret, typename Class, typename ...Params>
		void regist(std::string &&funcname, Ret(Class::*func)(Params...), Class &inst)
		{
			add_function(std::move(funcname),
							 std::function<Ret(Params...)>{
				[&inst, func](Params&&...params)->Ret {
					return (inst.*func)(std::forward<Params>(params)...);
			}});
		}

		template<typename OBJ>
		xjson::obj_t invoke(const std::string &funcname, OBJ&& _obj)
		{
			auto itr = function_map_.find(funcname);
			if(itr == function_map_.end())
				throw not_found_function(funcname.c_str());
			return itr->second(std::forward<OBJ>(_obj));
		}
	private:
		template<typename FuncType>
		typename xutil::function_traits<FuncType>::stl_function_type
			to_function(FuncType func)
		{
			return xutil::function_traits<FuncType>::stl_function_type{ func };
		}


		template<typename Ret, typename ...Params>
		void add_function(std::string &&funcname, std::function<Ret(Params...)> &&func)
		{
			return add_function(
				std::move(funcname), std::move(func), 
				xutil::make_index_sequence<sizeof ...(Params)>{});
		}

		template<typename ...Params>
		void add_function(std::string &&funcname, std::function<void(Params...)> &&func)
		{
			std::function<xjson::obj_t(Params...)> wrapper_func{
				[func](Params&&... params)->xjson::obj_t{
					func(std::forward<Params>(params)...);
					xjson::obj_t ret;
					return std::move(ret);
				}
			};
			return add_function(
				std::move(funcname),std::move(wrapper_func),
				xutil::make_index_sequence<sizeof ...(Params)>{});
		}

		template<typename Ret, typename ...Params, std::size_t ...Is>
		void add_function(std::string &&funcname,
							  std::function<Ret(Params...)> &&func, 
							  xutil::index_sequence<Is...>)
		{
			insert_function(std::move(funcname), 
					   [func](const xjson::obj_t& vec) ->xjson::obj_t{
				return func(vec.get<typename std::decay<Params>::type>(Is)...);
			});
		}

		void insert_function(std::string &&funcname, decorate_function &&func)
		{
			auto ret = function_map_.emplace(
				std::piecewise_construct,
				std::forward_as_tuple(std::move(funcname)),
				std::forward_as_tuple(std::move(func)));

			if(!ret.second)
			{
				throw std::exception("funcname exist");
			}
		}
		std::map<std::string, decorate_function> function_map_;
	};
}