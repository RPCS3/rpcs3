#pragma once

#include "image_sink.h"

enum class recording_mode
{
	stopped = 0,
	rpcs3,
	cell
};

namespace utils
{
	class video_provider
	{
	public:
		video_provider() = default;
		~video_provider();

		bool set_image_sink(std::shared_ptr<image_sink> sink, recording_mode type);
		void set_pause_time(usz pause_time_ms);
		bool can_consume_frame();
		void present_frame(std::vector<u8>& data, const u32 width, const u32 height, bool is_bgra);

	private:
		recording_mode m_type = recording_mode::stopped;
		std::shared_ptr<image_sink> m_image_sink;
		shared_mutex m_mutex{};
		atomic_t<bool> m_active{false};
		atomic_t<usz> m_current_encoder_frame{0};
		steady_clock::time_point m_encoder_start{};
		s64 m_last_pts_incoming = -1;
		usz m_pause_time_ms = 0;
	};

} // namespace utils
