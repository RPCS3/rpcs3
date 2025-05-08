#include "stdafx.h"
#include "video_provider.h"
#include "Emu/RSX/Overlays/overlay_message.h"
#include "Emu/Cell/timers.hpp"

extern "C"
{
#include <libavutil/pixfmt.h>
}

LOG_CHANNEL(media_log, "Media");

atomic_t<recording_mode> g_recording_mode = recording_mode::stopped;

template <>
void fmt_class_string<recording_mode>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](recording_mode value)
	{
		switch (value)
		{
		case recording_mode::stopped: return "stopped";
		case recording_mode::rpcs3: return "rpcs3";
		case recording_mode::cell: return "cell";
		}

		return unknown;
	});
}

namespace utils
{
	video_provider::~video_provider()
	{
		g_recording_mode = recording_mode::stopped;
	}

	bool video_provider::set_video_sink(std::shared_ptr<video_sink> sink, recording_mode type)
	{
		media_log.notice("video_provider: setting new video sink. sink=%d, type=%s", !!sink, type);

		if (type == recording_mode::stopped)
		{
			// Prevent misuse. type is supposed to be a valid state.
			media_log.error("video_provider: cannot set video sink with type %s", type);
			return false;
		}

		std::scoped_lock lock(m_video_mutex, m_audio_mutex);

		if (m_video_sink)
		{
			// cell has preference
			if (m_type == recording_mode::cell && m_type != type)
			{
				media_log.warning("video_provider: cannot set video sink with type %s if type %s is active", type, m_type);
				return false;
			}

			if (m_type != type || m_video_sink != sink)
			{
				media_log.warning("video_provider: stopping current video sink of type %s", m_type);
				m_video_sink->stop();
			}
		}

		m_type = sink ? type : recording_mode::stopped;
		m_video_sink = sink;
		m_active = (m_type != recording_mode::stopped);

		if (!m_active)
		{
			m_last_video_pts_incoming = -1;
			m_last_audio_pts_incoming = -1;
			m_start_time_us.store(umax);
		}

		return true;
	}

	void video_provider::set_pause_time_us(usz pause_time_us)
	{
		std::scoped_lock lock(m_video_mutex, m_audio_mutex);
		m_pause_time_us = pause_time_us;
	}

	recording_mode video_provider::check_mode()
	{
		if (!m_video_sink || m_video_sink->has_error)
		{
			g_recording_mode = recording_mode::stopped;
			rsx::overlays::queue_message(localized_string_id::RECORDING_ABORTED);
		}

		if (g_recording_mode == recording_mode::stopped)
		{
			m_active = false;
		}

		return g_recording_mode;
	}

	bool video_provider::can_consume_frame()
	{
		if (!m_active)
		{
			return false;
		}

		std::lock_guard lock_video(m_video_mutex);

		if (!m_video_sink || !m_video_sink->use_internal_video)
		{
			return false;
		}

		const usz elapsed_us = get_system_time() - m_start_time_us;
		ensure(elapsed_us >= m_pause_time_us);

		const usz timestamp_ms = (elapsed_us - m_pause_time_us) / 1000;
		const s64 pts = m_video_sink->get_pts(timestamp_ms);
		return pts > m_last_video_pts_incoming;
	}

	void video_provider::present_frame(std::vector<u8>& data, u32 pitch, u32 width, u32 height, bool is_bgra)
	{
		if (!m_active)
		{
			return;
		}

		std::lock_guard lock_video(m_video_mutex);

		if (check_mode() == recording_mode::stopped)
		{
			return;
		}

		const u64 current_time_us = get_system_time();

		if (m_start_time_us.compare_and_swap_test(umax, current_time_us))
		{
			media_log.notice("video_provider: start time = %d", current_time_us);
		}

		// Calculate presentation timestamp.
		const usz elapsed_us = current_time_us - m_start_time_us;
		ensure(elapsed_us >= m_pause_time_us);

		const usz timestamp_ms = (elapsed_us - m_pause_time_us) / 1000;
		const s64 pts = m_video_sink->get_pts(timestamp_ms);

		// We can just skip this frame if it has the same timestamp.
		if (pts <= m_last_video_pts_incoming)
		{
			return;
		}

		if (m_video_sink->add_frame(data, pitch, width, height, is_bgra ? AVPixelFormat::AV_PIX_FMT_BGRA : AVPixelFormat::AV_PIX_FMT_RGBA, timestamp_ms))
		{
			m_last_video_pts_incoming = pts;
		}
	}

	void video_provider::present_samples(const u8* buf, u32 sample_count, u16 channels)
	{
		if (!buf || !sample_count || !channels || !m_active)
		{
			return;
		}

		std::lock_guard lock_audio(m_audio_mutex);

		if (!m_video_sink || !m_video_sink->use_internal_audio)
		{
			return;
		}

		if (check_mode() == recording_mode::stopped)
		{
			return;
		}

		const u64 current_time_us = get_system_time();

		if (m_start_time_us.compare_and_swap_test(umax, current_time_us))
		{
			media_log.notice("video_provider: start time = %d", current_time_us);
		}

		// Calculate presentation timestamp.
		const usz elapsed_us = current_time_us - m_start_time_us;
		ensure(elapsed_us >= m_pause_time_us);

		const usz timestamp_us = elapsed_us - m_pause_time_us;
		const s64 pts = m_video_sink->get_audio_pts(timestamp_us);

		// We can just skip this sample if it has the same timestamp.
		if (pts <= m_last_audio_pts_incoming)
		{
			return;
		}

		if (m_video_sink->add_audio_samples(buf, sample_count, channels, timestamp_us))
		{
			m_last_audio_pts_incoming = pts;
		}
	}
}
