#pragma once

#include "util/types.hpp"
#include "util/atomic.hpp"
#include "Utilities/mutex.h"

#include <deque>
#include <cmath>

namespace utils
{
	class image_sink
	{
	public:
		image_sink() = default;

		virtual void stop(bool flush = true) = 0;
		virtual void add_frame(std::vector<u8>& frame, const u32 width, const u32 height, s32 pixel_format, usz timestamp_ms) = 0;

		s64 get_pts(usz timestamp_ms) const
		{
			return static_cast<s64>(std::round((timestamp_ms * m_framerate) / 1000.f));
		}

		usz get_timestamp_ms(s64 pts) const
		{
			return static_cast<usz>(std::round((pts * 1000) / static_cast<float>(m_framerate)));
		}

		atomic_t<bool> has_error{false};

		struct encoder_frame
		{
			encoder_frame() = default;
			encoder_frame(usz timestamp_ms, u32 width, u32 height, s32 av_pixel_format, std::vector<u8>&& data)
				: timestamp_ms(timestamp_ms), width(width), height(height), av_pixel_format(av_pixel_format), data(std::move(data))
			{}

			s64 pts = -1; // Optional
			usz timestamp_ms = 0;
			u32 width = 0;
			u32 height = 0;
			s32 av_pixel_format = 0; // NOTE: Make sure this is a valid AVPixelFormat
			std::vector<u8> data;
		};

	protected:
		shared_mutex m_mtx;
		std::deque<encoder_frame> m_frames_to_encode;
		atomic_t<bool> m_flush = false;
		u32 m_framerate = 0;
	};
}
