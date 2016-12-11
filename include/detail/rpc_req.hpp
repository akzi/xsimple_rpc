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
	}
}