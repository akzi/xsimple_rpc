#pragma once
#include <functional>
#include <memory>
#include <map>
#include <vector>
#include <utility>
#include <tuple>
#include <mutex>
#include <condition_variable>
#include <atomic>
//deps
#include "../../../xnet/include/xnet.hpp"
#include "../../../xutil/include/guard.hpp"
#include "../../../xutil/include/function_traits.hpp"
#include "../../../xutil/include/make_index_sequence.hpp"
#include "../../../xutil/include/has_member_traits.hpp"
#include "endec.hpp"
#include "func_register.hpp"
#include "rpc_req.hpp"
#include "rpc_session.hpp"
#include "gen_endec.hpp"
#include "../rpc_client.hpp"
