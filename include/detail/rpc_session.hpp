#pragma once
namespace xsimple_rpc
{
	namespace detail
	{
		struct rpc_session
		{
			void push_rpc_req(std::shared_ptr<rpc_req> & rpc_req)
			{
				std::lock_guard<std::mutex> locker(mtx_);
				req_item_list_.push_back(rpc_req);
			}

			void do_send_rpc()
			{
				std::lock_guard<std::mutex> locker(mtx_);
				if (is_send_ || req_item_list_.empty())
					return;
				is_send_ = true;
				auto item = req_item_list_.front();
				conn_.async_send(std::move(item->req_buffer_));
				wait_rpc_resp_list_.emplace_back(std::move(item));
				req_item_list_.pop_front();
			}

			void init_conn()
			{
				conn_.regist_send_callback([this](std::size_t len) {

					if (len == 0)
					{
						close();
						conn_.close();
						return;
					}
					is_send_ = false;
					do_send_rpc();
				});
				conn_.regist_recv_callback([this](char *data, std::size_t len) {

					if (len == 0 || recv_data_callback(data, len) == false)
					{
						close();
						conn_.close();
						return;
					}
				});
				conn_.async_recv(sizeof(uint32_t));
			}
			bool recv_data_callback(char *data, std::size_t len)
			{
				uint8_t *ptr = reinterpret_cast<uint8_t*>(data);
				if (msg_len_ == 0 && len == sizeof(msg_len_))
				{
					msg_len_ = endec::get<uint32_t>(ptr);
					static std::size_t min_msg_len =
						endec::get_sizeof(magic_code) +
						endec::get_sizeof(uint32_t()) +
						endec::get_sizeof(uint64_t());
					if (msg_len_ < min_msg_len)
					{
						std::cout << "msg error" << std::endl;
						return false;
					}
					conn_.async_recv((std::size_t)msg_len_ - sizeof(int32_t));
					msg_len_ = 0;
					return true;
				}

				auto magic_code = endec::get<std::string>(ptr);
				auto req_id = endec::get<int64_t>(ptr);
				if (req_id == 0)
				{
					heartbeat_callback();
					return true;
				}
				auto item = wait_rpc_resp_list_.front();
				wait_rpc_resp_list_.pop_front();
				if (item->req_id_ != req_id)
					return false;
				conn_.async_recv(sizeof(uint32_t));
				item->result_ = endec::get<std::string>(ptr);
				cv_.notify_one();
				return true;
			}
			void heartbeat_callback()
			{

			}
			void close()
			{
				is_close_ = true;
			}
			uint32_t msg_len_ = 0;
			bool is_close_ = false;
			bool is_send_ = false;
			std::string last_error_code_;
			std::mutex mtx_;
			std::condition_variable cv_;
			xnet::connection conn_;
			int64_t msgbox_index_ = 0;
			std::list<std::shared_ptr<rpc_req>> req_item_list_;
			std::list<std::shared_ptr<rpc_req>> wait_rpc_resp_list_;
		};
	}
	
}