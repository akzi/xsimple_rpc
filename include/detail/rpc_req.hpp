#pragma once
namespace xsimple_rpc
{
	namespace detail
	{
		struct rpc_req
		{
			enum class status
			{
				e_ok,
				e_cancel,
				e_rpc_error,
			};
			status status_ = status::e_ok;
			int64_t req_id_;
			std::string result_;
			std::string req_buffer_;
		};
		template<typename ...Args>
		inline std::string make_req(const std::string &rpc_name, int64_t req_id, std::tuple<Args...> &&tp)
		{
			using namespace detail;
			std::string buffer;
			uint32_t buffer_size = uint32_t(
				endec::get_sizeof(req_id) +
				endec::get_sizeof(tp) +
				endec::get_sizeof(rpc_name) +
				endec::get_sizeof(magic_code) +
				endec::get_sizeof(uint32_t()));

			buffer.resize(buffer_size);
			uint8_t*ptr = (uint8_t*)buffer.data();
			endec::put(ptr, buffer_size);
			endec::put(ptr, magic_code);
			endec::put(ptr, req_id);
			endec::put(ptr, rpc_name);
			endec::put(ptr, std::move(tp));
			return std::move(buffer);
		}

		template<typename ...Args>
		inline std::string make_resp(const std::string &rpc_name, int64_t req_id, std::string &&data)
		{
			using namespace detail;
			std::string buffer;
			uint32_t buffer_size = uint32_t(
				endec::get_sizeof(req_id) +
				endec::get_sizeof(data) +
				endec::get_sizeof(rpc_name) +
				endec::get_sizeof(magic_code) +
				endec::get_sizeof(uint32_t()));

			buffer.resize(buffer_size);
			uint8_t*ptr = (uint8_t*)buffer.data();
			endec::put(ptr, buffer_size);
			endec::put(ptr, magic_code);
			endec::put(ptr, req_id);
			endec::put(ptr, rpc_name);
			endec::put(ptr, std::move(data));
			return std::move(buffer);
		}
	}
}