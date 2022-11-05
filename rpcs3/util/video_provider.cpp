#include "stdafx.h"
#include "video_provider.h"
#include "Emu/RSX/Overlays/overlay_message.h"

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

	bool video_provider::set_image_sink(std::shared_ptr<image_sink> sink, recording_mode type)
	{
		media_log.notice("video_provider: setting new image sink. sink=%d, type=%s", !!sink, type);

		if (type == recording_mode::stopped)
		{
			// Prevent misuse. type is supposed to be a valid state.
			media_log.error("video_provider: cannot set image sink with type %s", type);
			return false;
		}

		std::lock_guard lock(m_mutex);

		if (m_image_sink)
		{
			// cell has preference
			if (m_type == recording_mode::cell && m_type != type)
			{
				media_log.warning("video_provider: cannot set image sink with type %s if type %s is active", type, m_type);
				return false;
			}

			if (m_type != type || m_image_sink != sink)
			{
				media_log.warning("video_provider: stopping current image sink of type %s", m_type);
				m_image_sink->stop();
			}
		}

		m_type = sink ? type : recording_mode::stopped;
		m_image_sink = sink;

		if (m_type == recording_mode::stopped)
		{
			m_active = false;
		}

		return true;
	}

	void video_provider::set_pause_time(usz pause_time_ms)
	{
		std::lock_guard lock(m_mutex);
		m_pause_time_ms = pause_time_ms;
	}

	bool video_provider::can_consume_frame()
	{
		std::lock_guard lock(m_mutex);

		if (!m_image_sink)
			return false;

		const usz timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(steady_clock::now() - m_encoder_start).count() - m_pause_time_ms;
		const s64 pts = m_image_sink->get_pts(timestamp_ms);
		return pts > m_last_pts_incoming;
	}

	void video_provider::present_frame(std::vector<u8>& data, const u32 width, const u32 height, bool is_bgra)
	{
		std::lock_guard lock(m_mutex);

		if (!m_image_sink || m_image_sink->has_error)
		{
			g_recording_mode = recording_mode::stopped;
			rsx::overlays::queue_message(localized_string_id::RECORDING_ABORTED);
		}

		if (g_recording_mode == recording_mode::stopped)
		{
			m_active = false;
			return;
		}

		if (!m_active.exchange(true))
		{
			m_current_encoder_frame = 0;
			m_last_pts_incoming = -1;
		}

		if (m_current_encoder_frame == 0)
		{
			m_encoder_start = steady_clock::now();
		}

		// Calculate presentation timestamp.
		const usz timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(steady_clock::now() - m_encoder_start).count() - m_pause_time_ms;
		const s64 pts = m_image_sink->get_pts(timestamp_ms);

		// We can just skip this frame if it has the same timestamp.
		if (pts <= m_last_pts_incoming)
		{
			return;
		}

		m_last_pts_incoming = pts;

		m_current_encoder_frame++;
		m_image_sink->add_frame(data, width, height, is_bgra ? AVPixelFormat::AV_PIX_FMT_BGRA : AVPixelFormat::AV_PIX_FMT_RGBA, timestamp_ms);
	}
}
