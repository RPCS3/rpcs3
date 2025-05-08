#pragma once

#include "video_sink.h"

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

		bool set_video_sink(std::shared_ptr<video_sink> sink, recording_mode type);
		void set_pause_time_us(usz pause_time_us);

		bool can_consume_frame();
		void present_frame(std::vector<u8>& data, u32 pitch, u32 width, u32 height, bool is_bgra);

		void present_samples(const u8* buf, u32 sample_count, u16 channels);

	private:
		recording_mode check_mode();

		recording_mode m_type = recording_mode::stopped;
		std::shared_ptr<video_sink> m_video_sink;
		shared_mutex m_video_mutex{};
		shared_mutex m_audio_mutex{};
		atomic_t<bool> m_active{false};
		atomic_t<usz> m_start_time_us{umax};
		s64 m_last_video_pts_incoming = -1;
		s64 m_last_audio_pts_incoming = -1;
		usz m_pause_time_us = 0;
	};

} // namespace utils
