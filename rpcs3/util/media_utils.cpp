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

	std::string error_to_string(int error)
	{
		char av_error[AV_ERROR_MAX_STRING_SIZE];
		av_make_error_string(av_error, AV_ERROR_MAX_STRING_SIZE, error);
		return av_error;
	}

	std::pair<bool, media_info> get_media_info(const std::string& path, s32 av_media_type)
	{
		media_info info;

		// Only print FFMPEG errors, fatals and panics
		av_log_set_level(AV_LOG_ERROR);

		AVDictionary* av_dict_opts = nullptr;
		if (int err = av_dict_set(&av_dict_opts, "probesize", "96", 0); err < 0)
		{
			media_log.error("av_dict_set: returned with error=%d='%s'", err, error_to_string(err));
			return { false, std::move(info) };
		}

		AVFormatContext* av_format_ctx = avformat_alloc_context();

		// Open input file
		if (int err = avformat_open_input(&av_format_ctx, path.c_str(), nullptr, &av_dict_opts); err < 0)
		{
			// Failed to open file
			av_dict_free(&av_dict_opts);
			avformat_free_context(av_format_ctx);
			media_log.notice("avformat_open_input: could not open file. error=%d='%s' file='%s'", err, error_to_string(err), path);
			return { false, std::move(info) };
		}
		av_dict_free(&av_dict_opts);

		// Find stream information
		if (int err = avformat_find_stream_info(av_format_ctx, nullptr); err < 0)
		{
			// Failed to load stream information
			avformat_close_input(&av_format_ctx);
			avformat_free_context(av_format_ctx);
			media_log.notice("avformat_find_stream_info: could not load stream information. error=%d='%s' file='%s'", err, error_to_string(err), path);
			return { false, std::move(info) };
		}

		// Derive first stream id and type from avformat context.
		// We are only interested in the first matching stream for now.
		int audio_stream_index = -1;
		int video_stream_index = -1;
		for (uint i = 0; i < av_format_ctx->nb_streams; i++)
		{
			switch (av_format_ctx->streams[i]->codecpar->codec_type)
			{
			case AVMEDIA_TYPE_AUDIO:
				if (audio_stream_index < 0)
					audio_stream_index = i;
				break;
			case AVMEDIA_TYPE_VIDEO:
				if (video_stream_index < 0)
					video_stream_index = i;
				break;
			default:
				break;
			}
		}

		// Abort if there is no natching stream or if the stream isn't the first one
		if ((av_media_type == AVMEDIA_TYPE_AUDIO && audio_stream_index != 0) ||
			(av_media_type == AVMEDIA_TYPE_VIDEO && video_stream_index != 0))
		{
			// Failed to find a stream
			avformat_close_input(&av_format_ctx);
			avformat_free_context(av_format_ctx);
			media_log.notice("Failed to match stream of type %d in file='%s'", av_media_type, path);
			return { false, std::move(info) };
		}

		// Get video info if available
		if (video_stream_index >= 0)
		{
			const AVStream* stream = av_format_ctx->streams[video_stream_index];
			info.video_av_codec_id = stream->codecpar->codec_id;
			info.video_bitrate_bps = static_cast<s32>(stream->codecpar->bit_rate);
		}

		// Get audio info if available
		if (audio_stream_index >= 0)
		{
			const AVStream* stream = av_format_ctx->streams[audio_stream_index];
			info.audio_av_codec_id = stream->codecpar->codec_id;
			info.audio_bitrate_bps = static_cast<s32>(stream->codecpar->bit_rate);
			info.sample_rate = stream->codecpar->sample_rate;
		}

		info.duration_us = av_format_ctx->duration;

		AVDictionaryEntry* tag = nullptr;
		while ((tag = av_dict_get(av_format_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
		{
			info.metadata[tag->key] = tag->value;
		}

		avformat_close_input(&av_format_ctx);
		avformat_free_context(av_format_ctx);

		return { true, std::move(info) };
	}
}
