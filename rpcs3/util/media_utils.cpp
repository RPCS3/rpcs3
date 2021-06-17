#include "stdafx.h"
#include "media_utils.h"
#include "logs.hpp"
#include "Utilities/StrUtil.h"

#ifdef _MSC_VER
#pragma warning(push, 0)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
extern "C" {
#include "libavformat/avformat.h"
#include "libavutil/dict.h"
}
#ifdef _MSC_VER
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif

LOG_CHANNEL(media_log, "Media");

namespace utils
{
	template <>
	std::string media_info::get_metadata(const std::string& key, const std::string& def) const
	{
		if (metadata.contains(key))
		{
			return metadata.at(key);
		}

		return def;
	}

	template <>
	s64 media_info::get_metadata(const std::string& key, const s64& def) const
	{
		if (metadata.contains(key))
		{
			s64 result{};
			if (try_to_int64(&result, metadata.at(key), smin, smax))
			{
				return result;
			}
		}

		return def;
	}

	std::pair<bool, media_info> get_media_info(const std::string& path, s32 av_media_type)
	{
		media_info info;

		// Only print FFMPEG errors, fatals and panics
		av_log_set_level(AV_LOG_ERROR);

		AVDictionary* av_dict_opts = nullptr;
		if (int err = av_dict_set(&av_dict_opts, "probesize", "96", 0); err < 0)
		{
			media_log.error("av_dict_set: returned with error=%d", err);
			return { false, std::move(info) };
		}

		AVFormatContext* av_format_ctx = avformat_alloc_context();

		// Open input file
		if (int err = avformat_open_input(&av_format_ctx, path.c_str(), nullptr, &av_dict_opts); err < 0)
		{
			// Failed to open file
			av_dict_free(&av_dict_opts);
			avformat_free_context(av_format_ctx);
			media_log.notice("avformat_open_input: could not open file. error=%d file='%s'", err, path);
			return { false, std::move(info) };
		}
		av_dict_free(&av_dict_opts);

		// Find stream information
		if (int err = avformat_find_stream_info(av_format_ctx, nullptr); err < 0)
		{
			// Failed to load stream information
			avformat_free_context(av_format_ctx);
			media_log.notice("avformat_find_stream_info: could not load stream information. error=%d file='%s'", err, path);
			return { false, std::move(info) };
		}

		// Derive first stream id and type from avformat context
		// we are only interested in the first stream for now
		int stream_index = -1;
		for (uint i = 0; i < 1 && i < av_format_ctx->nb_streams; i++)
		{
			if (av_format_ctx->streams[i]->codecpar->codec_type == av_media_type)
			{
				stream_index = i;
				break;
			}
		}

		if (stream_index == -1)
		{
			// Failed to find a stream
			avformat_free_context(av_format_ctx);
			media_log.notice("Could not find the desired stream of type %d in file='%s'", av_media_type, path);
			return { false, std::move(info) };
		}

		AVStream* stream = av_format_ctx->streams[stream_index];
		AVCodecParameters* codec = stream->codecpar;
		AVDictionaryEntry* tag = nullptr;

		info.av_codec_id = codec->codec_id;
		info.bitrate_bps = codec->bit_rate;
		info.sample_rate = codec->sample_rate;
		info.duration_us = av_format_ctx->duration;

		while (tag = av_dict_get(av_format_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))
		{
			info.metadata[tag->key] = tag->value;
		}

		avformat_close_input(&av_format_ctx);
		avformat_free_context(av_format_ctx);

		return { true, std::move(info) };
	}
}
