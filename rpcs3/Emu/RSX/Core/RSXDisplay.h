#pragma once

#include <util/types.hpp>
#include <util/logs.hpp>
#include <deque>
#include <unordered_map>

template <typename T>
class named_thread;

namespace rsx
{
	enum class surface_antialiasing : u8;

	struct framebuffer_dimensions_t
	{
		u16 width;
		u16 height;
		u8 samples_x;
		u8 samples_y;

		inline u32 samples_total() const
		{
			return static_cast<u32>(width) * height * samples_x * samples_y;
		}

		inline bool operator > (const framebuffer_dimensions_t& that) const
		{
			return samples_total() > that.samples_total();
		}

		std::string to_string(bool skip_aa_suffix = false) const;

		static framebuffer_dimensions_t make(u16 width, u16 height, rsx::surface_antialiasing aa);
	};

	struct framebuffer_statistics_t
	{
		std::unordered_map<rsx::surface_antialiasing, framebuffer_dimensions_t> data;

		// Replace the existing data with this input if it is greater than what is already known
		void add(u16 width, u16 height, rsx::surface_antialiasing aa);

		// Returns a formatted string representing the statistics collected over the frame.
		std::string to_string(bool squash) const;
	};

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

		u32 program_cache_lookups_total;
		u32 program_cache_lookups_ellided;

		framebuffer_statistics_t framebuffer_stats;
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
