#pragma once

#include "util/types.hpp"
#include "util/atomic.hpp"
#include "Utilities/mutex.h"

#include <deque>
#include <cmath>

namespace utils
{
	class video_sink
	{
	public:
		video_sink() = default;
		virtual ~video_sink() = default;

		virtual void stop(bool flush = true) = 0;
		virtual void pause(bool flush = true) = 0;
		virtual void resume() = 0;

		bool add_frame(std::vector<u8>& frame, u32 pitch, u32 width, u32 height, s32 pixel_format, usz timestamp_ms)
		{
			// Do not allow new frames while flushing or paused
			if (m_flush || m_paused)
				return false;

			std::lock_guard lock(m_video_mtx);
			m_frames_to_encode.emplace_back(timestamp_ms, pitch, width, height, pixel_format, std::move(frame));
			return true;
		}

		bool add_audio_samples(const u8* buf, u32 sample_count, u16 channels, usz timestamp_us)
		{
			// Do not allow new samples while flushing or paused
			if (m_flush || m_paused || !buf || !sample_count || !channels)
				return false;

			std::vector<u8> sample(buf, buf + sample_count * channels * sizeof(f32));
			std::lock_guard lock(m_audio_mtx);
			m_samples_to_encode.emplace_back(timestamp_us, sample_count, channels, std::move(sample));
			return true;
		}

		s64 get_pts(usz timestamp_ms) const
		{
			return static_cast<s64>(std::round((timestamp_ms * m_framerate) / 1000.0));
		}

		s64 get_audio_pts(usz timestamp_us) const
		{
			static constexpr f64 us_per_sec = 1000000.0;
			const f64 us_per_block = us_per_sec / (m_sample_rate / static_cast<f64>(m_samples_per_block));
			return static_cast<s64>(std::round(timestamp_us / us_per_block));
		}

		usz get_timestamp_ms(s64 pts) const
		{
			return static_cast<usz>(std::round((pts * 1000) / static_cast<f64>(m_framerate)));
		}

		usz get_audio_timestamp_us(s64 pts) const
		{
			static constexpr f64 us_per_sec = 1000000.0;
			const f64 us_per_block = us_per_sec / (m_sample_rate / static_cast<f64>(m_samples_per_block));
			return static_cast<usz>(pts * us_per_block);
		}

		atomic_t<bool> has_error{false};

		struct encoder_frame
		{
			encoder_frame() = default;
			encoder_frame(usz timestamp_ms, u32 pitch, u32 width, u32 height, s32 av_pixel_format, std::vector<u8>&& data)
				: timestamp_ms(timestamp_ms), pitch(pitch), width(width), height(height), av_pixel_format(av_pixel_format), data(std::move(data))
			{}

			s64 pts = -1; // Optional
			usz timestamp_ms = 0;
			u32 pitch = 0;
			u32 width = 0;
			u32 height = 0;
			s32 av_pixel_format = 0; // NOTE: Make sure this is a valid AVPixelFormat
			std::vector<u8> data;
		};

		struct encoder_sample
		{
			encoder_sample() = default;
			encoder_sample(usz timestamp_us, u32 sample_count, u16 channels, std::vector<u8>&& data)
				: timestamp_us(timestamp_us), sample_count(sample_count), channels(channels), data(std::move(data))
			{
			}

			usz timestamp_us = 0;
			u32 sample_count = 0;
			u16 channels = 0;
			std::vector<u8> data;
		};

		// These two variables should only be set once before we start encoding, so we don't need mutexes or atomics.
		bool use_internal_audio = false; // True if we want to fetch samples from cellAudio
		bool use_internal_video = false; // True if we want to fetch frames from rsx

	protected:
		shared_mutex m_video_mtx;
		std::deque<encoder_frame> m_frames_to_encode;
		shared_mutex m_audio_mtx;
		std::deque<encoder_sample> m_samples_to_encode;
		atomic_t<bool> m_paused = false;
		atomic_t<bool> m_flush = false;
		u32 m_framerate = 30;
		u32 m_sample_rate = 48000;
		static constexpr u32 m_samples_per_block = 256;
	};
}
