#pragma once

#include <unordered_map>
#include <atomic>
#include <deque>
#include <mutex>
#include <thread>
#include "Utilities/StrUtil.h"
#include "Utilities/Thread.h"

namespace utils
{
	struct media_info
	{
		std::string path;

		s32 audio_av_codec_id = 0; // 0 = AV_CODEC_ID_NONE
		s32 video_av_codec_id = 0; // 0 = AV_CODEC_ID_NONE
		s32 audio_bitrate_bps = 0; // Bit rate in bit/s
		s32 video_bitrate_bps = 0; // Bit rate in bit/s
		s32 sample_rate = 0; // Samples per second
		s64 duration_us = 0; // in AV_TIME_BASE fractional seconds (= microseconds)

		std::unordered_map<std::string, std::string> metadata;

		// Convenience function for metadata
		template <typename T>
		T get_metadata(const std::string& key, const T& def) const;
	};

	std::pair<bool, media_info> get_media_info(const std::string& path, s32 av_media_type);

	template <typename D>
	void parse_metadata(D& dst, const utils::media_info& mi, const std::string& key, const std::string& def, usz max_length)
	{
		std::string value = mi.get_metadata<std::string>(key, def);
		if (value.size() > max_length)
		{
			value.resize(max_length);
		}
		strcpy_trunc(dst, value);
	};

	class audio_decoder
	{
	public:
		audio_decoder();
		~audio_decoder();

		void set_path(const std::string& path);
		void set_swap_endianness(bool swapped);
		void stop();
		void decode();

		std::mutex m_mtx;
		const s32 sample_rate = 48000;
		std::vector<u8> data;
		atomic_t<u64> m_size = 0;
		atomic_t<u64> duration_ms = 0;
		atomic_t<bool> has_error{false};
		std::deque<std::pair<u64, u64>> timestamps_ms;

	private:
		bool m_swap_endianness = false;
		std::string m_path;
		std::unique_ptr<named_thread<std::function<void()>>> m_thread;
	};
}
