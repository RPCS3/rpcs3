#pragma once

#include <unordered_map>

namespace utils
{
	struct media_info
	{
		s32 av_codec_id = 0; // 0 = AV_CODEC_ID_NONE
		s32 bitrate_bps = 0; // Bit rate in bit/s
		s32 sample_rate; // Samples per second
		s64 duration_us = 0; // in AV_TIME_BASE fractional seconds (= microseconds)

		std::unordered_map<std::string, std::string> metadata;

		// Convenience function for metadata
		template <typename T>
		T get_metadata(const std::string& key, const T& def) const;
	};

	std::pair<bool, media_info> get_media_info(const std::string& path, s32 av_media_type);
}
