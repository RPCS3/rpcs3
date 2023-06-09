#pragma once

#include <util/types.hpp>
#include <util/logs.hpp>
#include <deque>

namespace rsx
{
	struct frame_statistics_t
	{
		u32 draw_calls;
		u32 submit_count;

		s64 setup_time;
		s64 vertex_upload_time;
		s64 textures_upload_time;
		s64 draw_exec_time;
		s64 flip_time;

		u32 vertex_cache_request_count;
		u32 vertex_cache_miss_count;
	};

	struct frame_time_t
	{
		u64 preempt_count;
		u64 timestamp;
		u64 tsc;
	};

	struct display_flip_info_t
	{
		std::deque<u32> buffer_queue;
		u32 buffer;
		bool skip_frame;
		bool emu_flip;
		bool in_progress;
		frame_statistics_t stats;

		inline void push(u32 _buffer)
		{
			buffer_queue.push_back(_buffer);
		}

		inline bool pop(u32 _buffer)
		{
			if (buffer_queue.empty())
			{
				return false;
			}

			do
			{
				const auto index = buffer_queue.front();
				buffer_queue.pop_front();

				if (index == _buffer)
				{
					buffer = _buffer;
					return true;
				}
			} while (!buffer_queue.empty());

			// Need to observe this happening in the wild
			rsx_log.error("Display queue was discarded while not empty!");
			return false;
		}
	};

	class vblank_thread
	{
		std::shared_ptr<named_thread<std::function<void()>>> m_thread;

	public:
		vblank_thread() = default;
		vblank_thread(const vblank_thread&) = delete;

		void set_thread(std::shared_ptr<named_thread<std::function<void()>>> thread);

		vblank_thread& operator=(thread_state);
		vblank_thread& operator=(const vblank_thread&) = delete;
	};
}
