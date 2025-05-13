#include "stdafx.h"
#include "media_utils.h"
#include "Emu/System.h"

#include <random>
#include <thread>

#ifdef _MSC_VER
#pragma warning(push, 0)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/dict.h"
#include "libavutil/opt.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}
constexpr int averror_eof = AVERROR_EOF; // workaround for old-style-cast error
constexpr int averror_invalid_data = AVERROR_INVALIDDATA; // workaround for old-style-cast error
#ifdef _MSC_VER
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif

LOG_CHANNEL(media_log, "Media");

namespace utils
{
	template <typename T>
	static inline void write_byteswapped(const void* src, void* dst)
	{
		*reinterpret_cast<T*>(dst) = *reinterpret_cast<const be_t<T>*>(src);
	}

	template <typename T>
	static inline void copy_samples(const u8* src, u8* dst, usz sample_count, bool swap_endianness)
	{
		if (swap_endianness)
		{
			for (usz i = 0; i < sample_count; i++)
			{
				write_byteswapped<T>(src + i * sizeof(T), dst + i * sizeof(T));
			}
		}
		else
		{
			std::memcpy(dst, src, sample_count * sizeof(T));
		}
	}

	const auto free_packet = [](AVPacket* p)
	{
		if (p)
		{
			av_packet_unref(p);
			av_packet_free(&p);
		}
	};

	const auto av_log_callback = [](void* avcl, int level, const char* fmt, va_list vl) -> void
	{
		if (level > av_log_get_level())
		{
			return;
		}

		constexpr int line_size = 1024;
		char line[line_size]{};
		int print_prefix = 1;

		if (int err = av_log_format_line2(avcl, level, fmt, vl, line, line_size, &print_prefix); err < 0)
		{
			media_log.error("av_log: av_log_format_line2 failed. Error: %d='%s'", err, av_error_to_string(err));
			return;
		}

		std::string msg = line;
		fmt::trim_back(msg, "\n\r\t ");

		if (level <= AV_LOG_ERROR)
			media_log.error("av_log: %s", msg);
		else if (level <= AV_LOG_WARNING)
			media_log.warning("av_log: %s", msg);
		else
			media_log.notice("av_log: %s", msg);
	};

	template <>
	std::string media_info::get_metadata(const std::string& key, const std::string& def) const
	{
		if (metadata.contains(key))
		{
			return ::at32(metadata, key);
		}

		return def;
	}

	template <>
	s64 media_info::get_metadata(const std::string& key, const s64& def) const
	{
		if (metadata.contains(key))
		{
			s64 result{};
			if (try_to_int64(&result, ::at32(metadata, key), smin, smax))
			{
				return result;
			}
		}

		return def;
	}

	std::string av_error_to_string(int error)
	{
		char av_error[AV_ERROR_MAX_STRING_SIZE]{};
		av_make_error_string(av_error, AV_ERROR_MAX_STRING_SIZE, error);
		return av_error;
	}

	std::vector<ffmpeg_codec> list_ffmpeg_codecs(bool list_decoders)
	{
		std::vector<ffmpeg_codec> codecs;
		void* opaque = nullptr;
		for (const AVCodec* codec = av_codec_iterate(&opaque); !!codec; codec = av_codec_iterate(&opaque))
		{
			if (list_decoders ? av_codec_is_decoder(codec) : av_codec_is_encoder(codec))
			{
				codecs.emplace_back(ffmpeg_codec{
					.codec_id = static_cast<int>(codec->id),
					.name = codec->name ? codec->name : "unknown",
					.long_name = codec->long_name ? codec->long_name : "unknown"
				});
			}
		}
		std::sort(codecs.begin(), codecs.end(), [](const ffmpeg_codec& l, const ffmpeg_codec& r){ return l.name < r.name; });
		return codecs;
	}

	std::vector<ffmpeg_codec> list_ffmpeg_decoders()
	{
		return list_ffmpeg_codecs(true);
	}

	std::vector<ffmpeg_codec> list_ffmpeg_encoders()
	{
		return list_ffmpeg_codecs(false);
	}

	std::pair<bool, media_info> get_media_info(const std::string& path, s32 av_media_type)
	{
		media_info info{};
		info.path = path;

		if (av_media_type == AVMEDIA_TYPE_UNKNOWN) // Let's use this for image info
		{
			const bool success = Emu.GetCallbacks().get_image_info(path, info.sub_type, info.width, info.height, info.orientation);
			if (!success) media_log.error("get_media_info: failed to get image info for '%s'", path);
			return { success, std::move(info) };
		}

		// Only print FFMPEG errors, fatals and panics
		av_log_set_callback(av_log_callback);
		av_log_set_level(AV_LOG_ERROR);

		AVDictionary* av_dict_opts = nullptr;
		if (int err = av_dict_set(&av_dict_opts, "probesize", "96", 0); err < 0)
		{
			media_log.error("get_media_info: av_dict_set: returned with error=%d='%s'", err, av_error_to_string(err));
			return { false, std::move(info) };
		}

		AVFormatContext* av_format_ctx = avformat_alloc_context();

		// Open input file
		if (int err = avformat_open_input(&av_format_ctx, path.c_str(), nullptr, &av_dict_opts); err < 0)
		{
			// Failed to open file
			av_dict_free(&av_dict_opts);
			avformat_free_context(av_format_ctx);
			media_log.notice("get_media_info: avformat_open_input: could not open file. error=%d='%s' file='%s'", err, av_error_to_string(err), path);
			return { false, std::move(info) };
		}
		av_dict_free(&av_dict_opts);

		// Find stream information
		if (int err = avformat_find_stream_info(av_format_ctx, nullptr); err < 0)
		{
			// Failed to load stream information
			avformat_close_input(&av_format_ctx);
			media_log.notice("get_media_info: avformat_find_stream_info: could not load stream information. error=%d='%s' file='%s'", err, av_error_to_string(err), path);
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
			media_log.notice("get_media_info: Failed to match stream of type %d in file='%s'", av_media_type, path);
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

		return { true, std::move(info) };
	}

	struct scoped_av
	{
		struct ctx
		{
			const AVCodec* codec = nullptr;
			AVCodecContext* context = nullptr;
			AVStream* stream = nullptr;
			AVPacket* packet = nullptr;
			AVFrame* frame = nullptr;
		};

		ctx audio{};
		ctx video{};

		AVFormatContext* format_context = nullptr;
		SwrContext* swr = nullptr;
		SwsContext* sws = nullptr;
		std::function<void()> kill_callback = nullptr;

		~scoped_av()
		{
			// Clean up
			if (audio.frame)
			{
				av_frame_unref(audio.frame);
				av_frame_free(&audio.frame);
			}
			if (video.frame)
			{
				av_frame_unref(video.frame);
				av_frame_free(&video.frame);
			}
			free_packet(audio.packet);
			free_packet(video.packet);
			if (swr)
				swr_free(&swr);
			if (sws)
				sws_freeContext(sws);
			if (audio.context)
				avcodec_free_context(&audio.context);
			if (video.context)
				avcodec_free_context(&video.context);
			// AVCodec is managed by libavformat, no need to free it
			// see: https://stackoverflow.com/a/18047320
			if (format_context)
				avformat_close_input(&format_context);
			//if (stream)
			//	av_free(stream);
			if (kill_callback)
				kill_callback();
		}
	};

	static std::string channel_layout_name(const AVChannelLayout& ch_layout)
	{
		std::vector<char> ch_layout_buf(64);
		int len = av_channel_layout_describe(&ch_layout, ch_layout_buf.data(), ch_layout_buf.size());
		if (len < 0)
		{
			media_log.error("av_channel_layout_describe failed. Error: %d='%s'", len, av_error_to_string(len));
			return {};
		}

		if (len > static_cast<int>(ch_layout_buf.size()))
		{
			// Try again with a bigger buffer
			media_log.notice("av_channel_layout_describe needs a bigger buffer: len=%d", len);
			ch_layout_buf.clear();
			ch_layout_buf.resize(len);

			len = av_channel_layout_describe(&ch_layout, ch_layout_buf.data(), ch_layout_buf.size());
			if (len < 0)
			{
				media_log.error("av_channel_layout_describe failed. Error: %d='%s'", len, av_error_to_string(len));
				return {};
			}
		}

		return ch_layout_buf.data();
	}

	// check that a given sample format is supported by the encoder
	static bool check_sample_fmt(const AVCodec* codec, enum AVSampleFormat sample_fmt)
	{
		if (!codec) return false;

		for (const AVSampleFormat* p = codec->sample_fmts; p && *p != AV_SAMPLE_FMT_NONE; p++)
		{
			if (*p == sample_fmt)
			{
				return true;
			}
		}
		return false;
	}

	// just pick the highest supported samplerate
	static int select_sample_rate(const AVCodec* codec)
	{
		if (!codec || !codec->supported_samplerates)
			return 48000;

		int best_samplerate = 0;
		for (const int* samplerate = codec->supported_samplerates; samplerate && *samplerate != 0; samplerate++)
		{
			if (!best_samplerate || abs(48000 - *samplerate) < abs(48000 - best_samplerate))
			{
				best_samplerate = *samplerate;
			}
		}
		return best_samplerate;
	}

	AVChannelLayout get_preferred_channel_layout(int channels)
	{
		switch (channels)
		{
		case 2:
			return AV_CHANNEL_LAYOUT_STEREO;
		case 6:
			return AV_CHANNEL_LAYOUT_5POINT1;
		case 8:
			return AV_CHANNEL_LAYOUT_7POINT1;
		default:
			break;
		}
		return {};
	}

	static constexpr AVChannelLayout empty_ch_layout = {};

	// select layout with the exact channel count
	static const AVChannelLayout* select_channel_layout(const AVCodec* codec, int channels)
	{
		if (!codec) return nullptr;

		const AVChannelLayout preferred_ch_layout = get_preferred_channel_layout(channels);
		const AVChannelLayout* found_ch_layout = nullptr;

		for (const AVChannelLayout* ch_layout = codec->ch_layouts;
			 ch_layout && memcmp(ch_layout, &empty_ch_layout, sizeof(AVChannelLayout)) != 0;
			 ch_layout++)
		{
			media_log.notice("select_channel_layout: listing channel layout '%s' with %d channels", channel_layout_name(*ch_layout), ch_layout->nb_channels);

			if (ch_layout->nb_channels == channels && memcmp(ch_layout, &preferred_ch_layout, sizeof(AVChannelLayout)) == 0)
			{
				found_ch_layout = ch_layout;
			}
		}

		return found_ch_layout;
	}

	audio_decoder::audio_decoder()
	{
	}

	audio_decoder::~audio_decoder()
	{
		stop();
	}

	void audio_decoder::set_context(music_selection_context context)
	{
		m_context = std::move(context);
	}

	void audio_decoder::set_swap_endianness(bool swapped)
	{
		m_swap_endianness = swapped;
	}

	void audio_decoder::clear()
	{
		track_fully_decoded = 0;
		track_fully_consumed = 0;
		has_error = false;
		m_size = 0;
		timestamps_ms.clear();
		data.clear();
	}

	void audio_decoder::stop()
	{
		if (m_thread)
		{
			auto& thread = *m_thread;
			thread = thread_state::aborting;
			track_fully_consumed = 1;
			track_fully_consumed.notify_one();
			thread();
			m_thread.reset();
		}

		clear();
	}

	void audio_decoder::decode()
	{
		stop();

		media_log.notice("audio_decoder: %d entries in playlist. Start decoding...", m_context.playlist.size());

		const auto decode_track = [this](const std::string& path)
		{
			media_log.notice("audio_decoder: decoding %s", path);

			// Only print FFMPEG errors, fatals and panics
			av_log_set_callback(av_log_callback);
			av_log_set_level(AV_LOG_ERROR);

			scoped_av av;

			// Get format from audio file
			av.format_context = avformat_alloc_context();
			if (int err = avformat_open_input(&av.format_context, path.c_str(), nullptr, nullptr); err < 0)
			{
				media_log.error("audio_decoder: Could not open file '%s'. Error: %d='%s'", path, err, av_error_to_string(err));
				has_error = true;
				return;
			}
			if (int err = avformat_find_stream_info(av.format_context, nullptr); err < 0)
			{
				media_log.error("audio_decoder: Could not retrieve stream info from file '%s'. Error: %d='%s'", path, err, av_error_to_string(err));
				has_error = true;
				return;
			}

			// Find the first audio stream
			AVStream* stream = nullptr;
			unsigned int stream_index;
			for (stream_index = 0; stream_index < av.format_context->nb_streams; stream_index++)
			{
				if (av.format_context->streams[stream_index]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
				{
					stream = av.format_context->streams[stream_index];
					break;
				}
			}
			if (!stream)
			{
				media_log.error("audio_decoder: Could not retrieve audio stream from file '%s'", path);
				has_error = true;
				return;
			}

			// Find decoder
			av.audio.codec = avcodec_find_decoder(stream->codecpar->codec_id);
			if (!av.audio.codec)
			{
				media_log.error("audio_decoder: Failed to find decoder for stream #%u in file '%s'", stream_index, path);
				has_error = true;
				return;
			}

			// Allocate context
			av.audio.context = avcodec_alloc_context3(av.audio.codec);
			if (!av.audio.context)
			{
				media_log.error("audio_decoder: Failed to allocate context for stream #%u in file '%s'", stream_index, path);
				has_error = true;
				return;
			}

			// Open decoder
			if (int err = avcodec_open2(av.audio.context, av.audio.codec, nullptr); err < 0)
			{
				media_log.error("audio_decoder: Failed to open decoder for stream #%u in file '%s'. Error: %d='%s'", stream_index, path, err, av_error_to_string(err));
				has_error = true;
				return;
			}

			// Prepare resampler
			av.swr = swr_alloc();
			if (!av.swr)
			{
				media_log.error("audio_decoder: Failed to allocate resampler for stream #%u in file '%s'", stream_index, path);
				has_error = true;
				return;
			}

			const int dst_channels = 2;
			const AVChannelLayout dst_channel_layout = AV_CHANNEL_LAYOUT_STEREO;
			const AVSampleFormat dst_format = AV_SAMPLE_FMT_FLT;

			int set_err = 0;
			if ((set_err = av_opt_set_int(av.swr, "in_channel_count", stream->codecpar->ch_layout.nb_channels, 0)) ||
				(set_err = av_opt_set_int(av.swr, "out_channel_count", dst_channels, 0)) ||
				(set_err = av_opt_set_chlayout(av.swr, "in_channel_layout", &stream->codecpar->ch_layout, 0)) ||
				(set_err = av_opt_set_chlayout(av.swr, "out_channel_layout", &dst_channel_layout, 0)) ||
				(set_err = av_opt_set_int(av.swr, "in_sample_rate", stream->codecpar->sample_rate, 0)) ||
				(set_err = av_opt_set_int(av.swr, "out_sample_rate", sample_rate, 0)) ||
				(set_err = av_opt_set_sample_fmt(av.swr, "in_sample_fmt", static_cast<AVSampleFormat>(stream->codecpar->format), 0)) ||
				(set_err = av_opt_set_sample_fmt(av.swr, "out_sample_fmt", dst_format, 0)))
			{
				media_log.error("audio_decoder: Failed to set resampler options: Error: %d='%s'", set_err, av_error_to_string(set_err));
				has_error = true;
				return;
			}

			if (int err = swr_init(av.swr); err < 0 || !swr_is_initialized(av.swr))
			{
				media_log.error("audio_decoder: Resampler has not been properly initialized: %d='%s'", err, av_error_to_string(err));
				has_error = true;
				return;
			}

			// Prepare to read data
			av.audio.frame = av_frame_alloc();
			if (!av.audio.frame)
			{
				media_log.error("audio_decoder: Error allocating the frame");
				has_error = true;
				return;
			}

			AVPacket* packet = av_packet_alloc();
			if (!packet)
			{
				media_log.error("audio_decoder: Error allocating the packet");
				has_error = true;
				return;
			}

			std::unique_ptr<AVPacket, decltype(free_packet)> packet_(packet);
			bool is_first_error = true;

			// Iterate through frames
			while (thread_ctrl::state() != thread_state::aborting && av_read_frame(av.format_context, packet) >= 0)
			{
				if (int err = avcodec_send_packet(av.audio.context, packet); err < 0)
				{
					if (is_first_error)
					{
						is_first_error = false;

						if (err == averror_invalid_data)
						{
							// Some mp3s contain some invalid data at the beginning of the stream. They work fine if we just ignore them.
							// So let's skip the first invalid data error. Maybe there is a better way, but let's just roll with it for now.
							media_log.warning("audio_decoder: Ignoring first error: %d='%s'", err, av_error_to_string(err));
							continue;
						}
					}
					media_log.error("audio_decoder: Queuing error: %d='%s'", err, av_error_to_string(err));
					has_error = true;
					return;
				}

				while (thread_ctrl::state() != thread_state::aborting)
				{
					if (int err = avcodec_receive_frame(av.audio.context, av.audio.frame); err < 0)
					{
						if (err == AVERROR(EAGAIN) || err == averror_eof)
							break;

						media_log.error("audio_decoder: Decoding error: %d='%s'", err, av_error_to_string(err));
						has_error = true;
						return;
					}

					// Resample frames
					u8* buffer = nullptr;
					const int align = 1;
					const int buffer_size = av_samples_alloc(&buffer, nullptr, dst_channels, av.audio.frame->nb_samples, dst_format, align);
					if (buffer_size < 0)
					{
						media_log.error("audio_decoder: Error allocating buffer: %d='%s'", buffer_size, av_error_to_string(buffer_size));
						has_error = true;
						return;
					}

					const int frame_count = swr_convert(av.swr, &buffer, av.audio.frame->nb_samples, const_cast<const uint8_t**>(av.audio.frame->data), av.audio.frame->nb_samples);
					if (frame_count < 0)
					{
						media_log.error("audio_decoder: Error converting frame: %d='%s'", frame_count, av_error_to_string(frame_count));
						has_error = true;
						if (buffer)
							av_freep(&buffer);
						return;
					}

					// Append resampled frames to data
					{
						std::scoped_lock lock(m_mtx);
						data.resize(m_size + buffer_size);

						// The format is float 32bit per channel.
						copy_samples<f32>(buffer, &data[m_size], buffer_size / sizeof(f32), m_swap_endianness);

						const s64 timestamp_ms = stream->time_base.den ? (1000 * av.audio.frame->best_effort_timestamp * stream->time_base.num) / stream->time_base.den : 0;
						timestamps_ms.push_back({m_size, timestamp_ms});
						m_size += buffer_size;
					}

					if (buffer)
						av_freep(&buffer);

					media_log.notice("audio_decoder: decoded frame_count=%d buffer_size=%d timestamp_us=%d", frame_count, buffer_size, av.audio.frame->best_effort_timestamp);
				}
			}
		};

		m_thread = std::make_unique<named_thread<std::function<void()>>>("Music Decode Thread", [this, decode_track]()
		{
			for (const std::string& track : m_context.playlist)
			{
				media_log.notice("audio_decoder: playlist entry: %s", track);
			}

			if (m_context.playlist.empty())
			{
				media_log.error("audio_decoder: Can not play empty playlist");
				has_error = true;
				return;
			}

			m_context.current_track = m_context.first_track;

			if (m_context.context_option == CELL_SEARCH_CONTEXTOPTION_SHUFFLE && m_context.playlist.size() > 1)
			{
				// Shuffle once if necessary
				media_log.notice("audio_decoder: shuffling initial playlist...");
				auto engine = std::default_random_engine{};
				std::shuffle(std::begin(m_context.playlist), std::end(m_context.playlist), engine);
			}

			while (thread_ctrl::state() != thread_state::aborting)
			{
				media_log.notice("audio_decoder: about to decode: %s (index=%d)", ::at32(m_context.playlist, m_context.current_track), m_context.current_track);

				decode_track(::at32(m_context.playlist, m_context.current_track));
				track_fully_decoded = 1;

				if (has_error)
				{
					media_log.notice("audio_decoder: stopping with error...");
					break;
				}

				// Let's only decode one track at a time. Wait for the consumer to finish reading the track.
				media_log.notice("audio_decoder: waiting until track is consumed...");
				thread_ctrl::wait_on(track_fully_consumed, 0);
				track_fully_consumed = false;
			}

			media_log.notice("audio_decoder: finished playlist");
		});
	}

	u32 audio_decoder::set_next_index(bool next)
	{
		return m_context.step_track(next);
	}

	video_encoder::video_encoder()
		: utils::video_sink()
	{
	}

	video_encoder::~video_encoder()
	{
		stop();
	}

	std::string video_encoder::path() const
	{
		return m_path;
	}

	s64 video_encoder::last_video_pts() const
	{
		return m_last_video_pts;
	}

	void video_encoder::set_path(const std::string& path)
	{
		m_path = path;
	}

	void video_encoder::set_framerate(u32 framerate)
	{
		m_framerate = framerate;
	}

	void video_encoder::set_video_bitrate(u32 bitrate)
	{
		m_video_bitrate_bps = bitrate;
	}

	void video_encoder::set_output_format(video_encoder::frame_format format)
	{
		m_out_format = std::move(format);
	}

	void video_encoder::set_video_codec(s32 codec_id)
	{
		m_video_codec_id = codec_id;
	}

	void video_encoder::set_max_b_frames(s32 max_b_frames)
	{
		m_max_b_frames = max_b_frames;
	}

	void video_encoder::set_gop_size(s32 gop_size)
	{
		m_gop_size = gop_size;
	}

	void video_encoder::set_sample_rate(u32 sample_rate)
	{
		m_sample_rate = sample_rate;
	}

	void video_encoder::set_audio_channels(u32 channels)
	{
		m_channels = channels;
	}

	void video_encoder::set_audio_bitrate(u32 bitrate)
	{
		m_audio_bitrate_bps = bitrate;
	}

	void video_encoder::set_audio_codec(s32 codec_id)
	{
		m_audio_codec_id = codec_id;
	}

	void video_encoder::pause(bool flush)
	{
		if (m_thread)
		{
			m_paused = true;
			m_flush = flush;

			if (flush)
			{
				// Let's assume this will finish in a timely manner
				while (m_flush && m_running)
				{
					std::this_thread::sleep_for(1us);
				}
			}
		}
	}

	void video_encoder::stop(bool flush)
	{
		media_log.notice("video_encoder: Stopping video encoder. flush=%d", flush);

		if (m_thread)
		{
			m_flush = flush;

			if (flush)
			{
				// Let's assume this will finish in a timely manner
				while (m_flush && m_running)
				{
					std::this_thread::sleep_for(1ms);
				}
			}

			// Join thread
			m_thread.reset();
		}

		std::scoped_lock lock(m_video_mtx, m_audio_mtx);
		m_frames_to_encode.clear();
		m_samples_to_encode.clear();
		has_error = false;
		m_flush = false;
		m_paused = false;
		m_running = false;
	}

	void video_encoder::resume()
	{
		media_log.notice("video_encoder: Resuming video encoder");

		m_flush = false;
		m_paused = false;
	}

	void video_encoder::encode()
	{
		if (m_running)
		{
			// Resume
			resume();
			media_log.success("video_encoder: resuming recording of '%s'", m_path);
			return;
		}

		m_last_audio_pts = 0;
		m_last_video_pts = 0;

		stop();

		if (const std::string dir = fs::get_parent_dir(m_path); !fs::is_dir(dir))
		{
			media_log.error("video_encoder: Could not find directory: '%s' for file '%s'", dir, m_path);
			has_error = true;
			return;
		}

		media_log.success("video_encoder: Starting recording of '%s'", m_path);

		m_thread = std::make_unique<named_thread<std::function<void()>>>("Video Encode Thread", [this, path = m_path]()
		{
			m_running = true;

			// Only print FFMPEG errors, fatals and panics
			av_log_set_callback(av_log_callback);
			av_log_set_level(AV_LOG_ERROR);

			// Reset variables at all costs
			scoped_av av;
			av.kill_callback = [this]()
			{
				m_flush = false;
				m_running = false;
			};

			// Let's list the encoders first
			std::vector<const AVCodec*> audio_codecs;
			std::vector<const AVCodec*> video_codecs;
			void* opaque = nullptr;
			while (const AVCodec* codec = av_codec_iterate(&opaque))
			{
				if (codec->type == AVMediaType::AVMEDIA_TYPE_AUDIO)
				{
					media_log.notice("video_encoder: Found audio codec %d = %s", static_cast<int>(codec->id), codec->name);
					audio_codecs.push_back(codec);
				}
				else if (codec->type == AVMediaType::AVMEDIA_TYPE_VIDEO)
				{
					media_log.notice("video_encoder: Found video codec %d = %s", static_cast<int>(codec->id), codec->name);
					video_codecs.push_back(codec);
				}
			}

			const AVPixelFormat out_pix_format = static_cast<AVPixelFormat>(m_out_format.av_pixel_format);

			const auto find_format = [&](AVCodecID video_codec, AVCodecID audio_codec) -> const AVOutputFormat*
			{
				// Try to find a preferable output format
				std::vector<const AVOutputFormat*> oformats;

				void* opaque = nullptr;
				for (const AVOutputFormat* oformat = av_muxer_iterate(&opaque); !!oformat; oformat = av_muxer_iterate(&opaque))
				{
					media_log.notice("video_encoder: Listing output format '%s' (video_codec=%d, audio_codec=%d)", oformat->name, static_cast<int>(oformat->video_codec), static_cast<int>(oformat->audio_codec));
					if (avformat_query_codec(oformat, video_codec, FF_COMPLIANCE_NORMAL) == 1 &&
						avformat_query_codec(oformat, audio_codec, FF_COMPLIANCE_NORMAL) == 1)
					{
						oformats.push_back(oformat);
					}
				}

				for (const AVOutputFormat* oformat : oformats)
				{
					if (!oformat) continue;
					media_log.notice("video_encoder: Found compatible output format '%s' (video_codec=%d, audio_codec=%d)", oformat->name, static_cast<int>(oformat->video_codec), static_cast<int>(oformat->audio_codec));
				}

				// Select best match
				for (const AVOutputFormat* oformat : oformats)
				{
					if (oformat && oformat->video_codec == video_codec && oformat->audio_codec == audio_codec)
					{
						media_log.notice("video_encoder: Using matching output format '%s' (video_codec=%d, audio_codec=%d)", oformat->name, static_cast<int>(oformat->video_codec), static_cast<int>(oformat->audio_codec));
						return oformat;
					}
				}

				// Fallback to first found format
				if (const AVOutputFormat* oformat = oformats.empty() ? nullptr : oformats.front())
				{
					media_log.notice("video_encoder: Using suboptimal output format '%s' (video_codec=%d, audio_codec=%d)", oformat->name, static_cast<int>(oformat->video_codec), static_cast<int>(oformat->audio_codec));
					return oformat;
				}

				return nullptr;
			};

			const AVCodecID video_codec = static_cast<AVCodecID>(m_video_codec_id);
			const AVCodecID audio_codec = static_cast<AVCodecID>(m_audio_codec_id);
			const AVOutputFormat* out_format = find_format(video_codec, audio_codec);

			if (out_format)
			{
				media_log.success("video_encoder: Found requested output format '%s'", out_format->name);
			}
			else
			{
				media_log.error("video_encoder: Could not find a format for the requested video_codec %d and audio_codec %d", m_video_codec_id, m_audio_codec_id);

				// Fallback to some other codec
				for (const AVCodec* video_codec : video_codecs)
				{
					for (const AVCodec* audio_codec : audio_codecs)
					{
						out_format = find_format(video_codec->id, audio_codec->id);

						if (out_format)
						{
							media_log.success("video_encoder: Found fallback output format '%s'", out_format->name);
							break;
						}
					}

					if (out_format)
					{
						break;
					}
				}
			}

			if (!out_format)
			{
				media_log.error("video_encoder: Could not find any output format");
				has_error = true;
				return;
			}

			if (int err = avformat_alloc_output_context2(&av.format_context, out_format, nullptr, nullptr); err < 0)
			{
				media_log.error("video_encoder: avformat_alloc_output_context2 for '%s' failed. Error: %d='%s'", out_format->name, err, av_error_to_string(err));
				has_error = true;
				return;
			}

			if (!av.format_context)
			{
				media_log.error("video_encoder: avformat_alloc_output_context2 failed");
				has_error = true;
				return;
			}

			const auto create_context = [this, &av](bool is_video) -> bool
			{
				const std::string type = is_video ? "video" : "audio";
				scoped_av::ctx& ctx = is_video ? av.video : av.audio;

				if (is_video)
				{
					if (!(ctx.codec = avcodec_find_encoder(av.format_context->oformat->video_codec)))
					{
						media_log.error("video_encoder: avcodec_find_encoder for video failed. video_codec=%d", static_cast<int>(av.format_context->oformat->video_codec));
						return false;
					}
				}
				else
				{
					if (!(ctx.codec = avcodec_find_encoder(av.format_context->oformat->audio_codec)))
					{
						media_log.error("video_encoder: avcodec_find_encoder for audio failed. audio_codec=%d", static_cast<int>(av.format_context->oformat->audio_codec));
						return false;
					}
				}

				if (!(ctx.stream = avformat_new_stream(av.format_context, nullptr)))
				{
					media_log.error("video_encoder: avformat_new_stream for %s failed", type);
					return false;
				}

				ctx.stream->id = is_video ? 0 : 1;

				if (!(ctx.context = avcodec_alloc_context3(ctx.codec)))
				{
					media_log.error("video_encoder: avcodec_alloc_context3 for %s failed", type);
					return false;
				}

				if (av.format_context->oformat->flags & AVFMT_GLOBALHEADER)
				{
					ctx.context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
				}

				return true;
			};

			if (!create_context(true))
			{
				has_error = true;
				return;
			}

			if (!create_context(false))
			{
				has_error = true;
				return;
			}

			media_log.notice("video_encoder: using audio_codec = %d", static_cast<int>(av.format_context->oformat->audio_codec));
			media_log.notice("video_encoder: using sample_rate = %d", m_sample_rate);
			media_log.notice("video_encoder: using audio_bitrate = %d", m_audio_bitrate_bps);
			media_log.notice("video_encoder: using audio channels = %d", m_channels);
			media_log.notice("video_encoder: using video_codec = %d", static_cast<int>(av.format_context->oformat->video_codec));
			media_log.notice("video_encoder: using video_bitrate = %d", m_video_bitrate_bps);
			media_log.notice("video_encoder: using out width = %d", m_out_format.width);
			media_log.notice("video_encoder: using out height = %d", m_out_format.height);
			media_log.notice("video_encoder: using framerate = %d", m_framerate);
			media_log.notice("video_encoder: using gop_size = %d", m_gop_size);
			media_log.notice("video_encoder: using max_b_frames = %d", m_max_b_frames);

			// select audio parameters supported by the encoder
			if (av.audio.context)
			{
				if (const AVChannelLayout* ch_layout = select_channel_layout(av.audio.codec, m_channels))
				{
					media_log.notice("video_encoder: found channel layout '%s' with %d channels", channel_layout_name(*ch_layout), ch_layout->nb_channels);

					if (int err = av_channel_layout_copy(&av.audio.context->ch_layout, ch_layout); err != 0)
					{
						media_log.error("video_encoder: av_channel_layout_copy failed. Error: %d='%s'", err, av_error_to_string(err));
						has_error = true;
						return;
					}
				}
				else
				{
					media_log.notice("video_encoder: select_channel_layout returned nullptr, trying with own layout...");

					const AVChannelLayout new_ch_layout = get_preferred_channel_layout(m_channels);

					if (memcmp(&new_ch_layout, &empty_ch_layout, sizeof(AVChannelLayout)) == 0)
					{
						media_log.error("video_encoder: unsupported audio channel count: %d", m_channels);
						has_error = true;
						return;
					}

					if (int err = av_channel_layout_copy(&av.audio.context->ch_layout, &new_ch_layout); err != 0)
					{
						media_log.error("video_encoder: av_channel_layout_copy failed. Error: %d='%s'", err, av_error_to_string(err));
						has_error = true;
						return;
					}
				}

				m_sample_rate = select_sample_rate(av.audio.codec);

				av.audio.context->codec_id = av.format_context->oformat->audio_codec;
				av.audio.context->codec_type = AVMEDIA_TYPE_AUDIO;
				av.audio.context->bit_rate = m_audio_bitrate_bps;
				av.audio.context->sample_rate = m_sample_rate;
				av.audio.context->time_base = {.num = 1, .den = av.audio.context->sample_rate};
				av.audio.context->sample_fmt = AV_SAMPLE_FMT_FLTP; // AV_SAMPLE_FMT_FLT is not supported in regular AC3
				av.audio.stream->time_base = av.audio.context->time_base;

				// check that the encoder supports the format
				if (!check_sample_fmt(av.audio.codec, av.audio.context->sample_fmt))
				{
					media_log.error("video_encoder: Audio encoder does not support sample format %s", av_get_sample_fmt_name(av.audio.context->sample_fmt));
					has_error = true;
					return;
				}

				if (int err = avcodec_open2(av.audio.context, av.audio.codec, nullptr); err != 0)
				{
					media_log.error("video_encoder: avcodec_open2 for audio failed. Error: %d='%s'", err, av_error_to_string(err));
					has_error = true;
					return;
				}

				if (!(av.audio.packet = av_packet_alloc()))
				{
					media_log.error("video_encoder: av_packet_alloc for audio packet failed");
					has_error = true;
					return;
				}

				if (!(av.audio.frame = av_frame_alloc()))
				{
					media_log.error("video_encoder: av_frame_alloc for audio frame failed");
					has_error = true;
					return;
				}

				av.audio.frame->format = AV_SAMPLE_FMT_FLTP;
				av.audio.frame->nb_samples = av.audio.context->frame_size;

				if (int err = av_channel_layout_copy(&av.audio.frame->ch_layout, &av.audio.context->ch_layout); err < 0)
				{
					media_log.error("video_encoder: av_channel_layout_copy for audio frame failed. Error: %d='%s'", err, av_error_to_string(err));
					has_error = true;
					return;
				}

				if (int err = av_frame_get_buffer(av.audio.frame, 0); err < 0)
				{
					media_log.error("video_encoder: av_frame_get_buffer for audio frame failed. Error: %d='%s'", err, av_error_to_string(err));
					has_error = true;
					return;
				}

				if (int err = avcodec_parameters_from_context(av.audio.stream->codecpar, av.audio.context); err < 0)
				{
					media_log.error("video_encoder: avcodec_parameters_from_context for audio failed. Error: %d='%s'", err, av_error_to_string(err));
					has_error = true;
					return;
				}

				// Log channel layout
				media_log.notice("video_encoder: av_channel_layout='%s'", channel_layout_name(av.audio.frame->ch_layout));
			}

			// select video parameters supported by the encoder
			if (av.video.context)
			{
				av.video.context->codec_id = av.format_context->oformat->video_codec;
				av.video.context->codec_type = AVMEDIA_TYPE_VIDEO;
				av.video.context->bit_rate = m_video_bitrate_bps;
				av.video.context->width = static_cast<int>(m_out_format.width);
				av.video.context->height = static_cast<int>(m_out_format.height);
				av.video.context->time_base = {.num = 1, .den = static_cast<int>(m_framerate)};
				av.video.context->framerate = {.num = static_cast<int>(m_framerate), .den = 1};
				av.video.context->pix_fmt = out_pix_format;
				av.video.context->gop_size = m_gop_size;
				av.video.context->max_b_frames = m_max_b_frames;
				av.video.stream->time_base = av.video.context->time_base;

				if (int err = avcodec_open2(av.video.context, av.video.codec, nullptr); err != 0)
				{
					media_log.error("video_encoder: avcodec_open2 for video failed. Error: %d='%s'", err, av_error_to_string(err));
					has_error = true;
					return;
				}

				if (!(av.video.packet = av_packet_alloc()))
				{
					media_log.error("video_encoder: av_packet_alloc for video packet failed");
					has_error = true;
					return;
				}

				if (!(av.video.frame = av_frame_alloc()))
				{
					media_log.error("video_encoder: av_frame_alloc for video frame failed");
					has_error = true;
					return;
				}

				av.video.frame->format = av.video.context->pix_fmt;
				av.video.frame->width = av.video.context->width;
				av.video.frame->height = av.video.context->height;

				if (int err = av_frame_get_buffer(av.video.frame, 0); err < 0)
				{
					media_log.error("video_encoder: av_frame_get_buffer for video frame failed. Error: %d='%s'", err, av_error_to_string(err));
					has_error = true;
					return;
				}

				if (int err = avcodec_parameters_from_context(av.video.stream->codecpar, av.video.context); err < 0)
				{
					media_log.error("video_encoder: avcodec_parameters_from_context for video failed. Error: %d='%s'", err, av_error_to_string(err));
					has_error = true;
					return;
				}
			}

			media_log.notice("video_encoder: av_dump_format");
			for (u32 i = 0; i < av.format_context->nb_streams; i++)
			{
				av_dump_format(av.format_context, i, path.c_str(), 1);
			}

			// open the output file, if needed
			if (!(av.format_context->flags & AVFMT_NOFILE))
			{
				if (int err = avio_open(&av.format_context->pb, path.c_str(), AVIO_FLAG_WRITE); err != 0)
				{
					media_log.error("video_encoder: avio_open failed. Error: %d='%s'", err, av_error_to_string(err));
					has_error = true;
					return;
				}
			}

			if (int err = avformat_write_header(av.format_context, nullptr); err < 0)
			{
				media_log.error("video_encoder: avformat_write_header failed. Error: %d='%s'", err, av_error_to_string(err));

				if (int err = avio_closep(&av.format_context->pb); err != 0)
				{
					media_log.error("video_encoder: avio_closep failed. Error: %d='%s'", err, av_error_to_string(err));
				}

				has_error = true;
				return;
			}

			const auto flush = [&](scoped_av::ctx& ctx)
			{
				while ((thread_ctrl::state() != thread_state::aborting || m_flush) && !has_error && ctx.context)
				{
					if (int err = avcodec_receive_packet(ctx.context, ctx.packet); err < 0)
					{
						if (err == AVERROR(EAGAIN) || err == averror_eof)
							break;

						media_log.error("video_encoder: avcodec_receive_packet failed. Error: %d='%s'", err, av_error_to_string(err));
						has_error = true;
						return;
					}

					av_packet_rescale_ts(ctx.packet, ctx.context->time_base, ctx.stream->time_base);
					ctx.packet->stream_index = ctx.stream->index;

					if (int err = av_interleaved_write_frame(av.format_context, ctx.packet); err < 0)
					{
						media_log.error("video_encoder: av_write_frame failed. Error: %d='%s'", err, av_error_to_string(err));
						has_error = true;
						return;
					}
				}
			};

			u32 audio_sample_remainder = 0;
			s64 last_audio_pts = -1;
			s64 last_audio_frame_pts = 0;
			s64 last_video_pts = -1;

			// Allocate audio buffer for our audio frame
			std::vector<u8> audio_frame;
			u32 audio_frame_sample_count = 0;
			const bool sample_fmt_is_planar = av.audio.context && av_sample_fmt_is_planar(av.audio.context->sample_fmt) != 0;
			const int sample_fmt_bytes = av.audio.context ? av_get_bytes_per_sample(av.audio.context->sample_fmt) : 0;
			ensure(sample_fmt_bytes == sizeof(f32)); // We only support FLT or FLTP for now

			if (av.audio.frame)
			{
				audio_frame.resize(av.audio.frame->nb_samples * av.audio.frame->ch_layout.nb_channels * sizeof(f32));
				last_audio_frame_pts -= av.audio.frame->nb_samples;
			}

			encoder_sample last_samples;
			u32 leftover_sample_count = 0;

			while ((thread_ctrl::state() != thread_state::aborting || m_flush) && !has_error)
			{
				// Fetch video frame
				encoder_frame frame_data;
				bool got_frame = false;
				{
					m_video_mtx.lock();

					if (m_frames_to_encode.empty())
					{
						m_video_mtx.unlock();
					}
					else
					{
						frame_data = std::move(m_frames_to_encode.front());
						m_frames_to_encode.pop_front();
						m_video_mtx.unlock();

						got_frame = true;

						// Calculate presentation timestamp.
						const s64 pts = get_pts(frame_data.timestamp_ms);

						// We need to skip this frame if it has the same timestamp.
						if (pts <= last_video_pts)
						{
							media_log.trace("video_encoder: skipping frame. last_pts=%d, pts=%d, timestamp_ms=%d", last_video_pts, pts, frame_data.timestamp_ms);
						}
						else if (av.video.context)
						{
							media_log.trace("video_encoder: adding new frame. timestamp_ms=%d", frame_data.timestamp_ms);

							if (int err = av_frame_make_writable(av.video.frame); err < 0)
							{
								media_log.error("video_encoder: av_frame_make_writable failed. Error: %d='%s'", err, av_error_to_string(err));
								has_error = true;
								break;
							}

							u8* in_data[4]{};
							int in_line[4]{};

							const AVPixelFormat in_format = static_cast<AVPixelFormat>(frame_data.av_pixel_format);

							if (int ret = av_image_fill_linesizes(in_line, in_format, frame_data.width); ret < 0)
							{
								fmt::throw_exception("video_encoder: av_image_fill_linesizes failed (ret=0x%x): %s", ret, utils::av_error_to_string(ret));
							}

							if (int ret = av_image_fill_pointers(in_data, in_format, frame_data.height, frame_data.data.data(), in_line); ret < 0)
							{
								fmt::throw_exception("video_encoder: av_image_fill_pointers failed (ret=0x%x): %s", ret, utils::av_error_to_string(ret));
							}

							// Update the context in case the frame format has changed
							av.sws = sws_getCachedContext(av.sws, frame_data.width, frame_data.height, in_format,
							                              av.video.context->width, av.video.context->height, out_pix_format, SWS_BICUBIC, nullptr, nullptr, nullptr);
							if (!av.sws)
							{
								media_log.error("video_encoder: sws_getCachedContext failed");
								has_error = true;
								break;
							}

							if (int err = sws_scale(av.sws, in_data, in_line, 0, frame_data.height, av.video.frame->data, av.video.frame->linesize); err < 0)
							{
								media_log.error("video_encoder: sws_scale failed. Error: %d='%s'", err, av_error_to_string(err));
								has_error = true;
								break;
							}

							av.video.frame->pts = pts;

							if (int err = avcodec_send_frame(av.video.context, av.video.frame); err < 0)
							{
								media_log.error("video_encoder: avcodec_send_frame for video failed. Error: %d='%s'", err, av_error_to_string(err));
								has_error = true;
								break;
							}

							flush(av.video);

							last_video_pts = av.video.frame->pts;
							m_last_video_pts = last_video_pts;
						}
					}
				}

				// Fetch audio sample
				encoder_sample sample_data;
				bool got_sample = false;
				{
					m_audio_mtx.lock();

					if (m_samples_to_encode.empty())
					{
						m_audio_mtx.unlock();
					}
					else
					{
						sample_data = std::move(m_samples_to_encode.front());
						m_samples_to_encode.pop_front();
						m_audio_mtx.unlock();

						got_sample = true;

						if (sample_data.channels != av.audio.frame->ch_layout.nb_channels)
						{
							fmt::throw_exception("video_encoder: Audio sample channel count %d does not match frame channel count %d", sample_data.channels, av.audio.frame->ch_layout.nb_channels);
						}

						// Calculate presentation timestamp.
						const s64 pts = get_audio_pts(sample_data.timestamp_us);

						// We need to skip this frame if it has the same timestamp.
						if (pts <= last_audio_pts)
						{
							media_log.trace("video_encoder: skipping sample. last_pts=%d, pts=%d, timestamp_us=%d", last_audio_pts, pts, sample_data.timestamp_us);
						}
						else if (av.audio.context)
						{
							media_log.trace("video_encoder: adding new sample. timestamp_us=%d", sample_data.timestamp_us);

							static constexpr bool swap_endianness = false;

							const auto send_frame = [&]()
							{
								if (audio_frame_sample_count < static_cast<u32>(av.audio.frame->nb_samples))
								{
									return;
								}

								audio_frame_sample_count = 0;

								if (int err = av_frame_make_writable(av.audio.frame); err < 0)
								{
									media_log.error("video_encoder: av_frame_make_writable failed. Error: %d='%s'", err, av_error_to_string(err));
									has_error = true;
									return;
								}

								// NOTE: The ffmpeg channel layout should match our downmix channel layout
								if (sample_fmt_is_planar)
								{
									const int channels = av.audio.frame->ch_layout.nb_channels;
									const int samples = av.audio.frame->nb_samples;

									for (int ch = 0; ch < channels; ch++)
									{
										f32* dst = reinterpret_cast<f32*>(av.audio.frame->data[ch]);

										for (int sample = 0; sample < samples; sample++)
										{
											dst[sample] = *reinterpret_cast<f32*>(&audio_frame[(sample * channels + ch) * sizeof(f32)]);
										}
									}
								}
								else
								{
									std::memcpy(av.audio.frame->data[0], audio_frame.data(), audio_frame.size());
								}

								av.audio.frame->pts = last_audio_frame_pts + av.audio.frame->nb_samples;

								if (int err = avcodec_send_frame(av.audio.context, av.audio.frame); err < 0)
								{
									media_log.error("video_encoder: avcodec_send_frame failed: %d='%s'", err, av_error_to_string(err));
									has_error = true;
									return;
								}

								flush(av.audio);

								last_audio_frame_pts = av.audio.frame->pts;
							};

							const auto add_encoder_sample = [&](bool add_new_sample, u32 silence_to_add = 0)
							{
								const auto update_last_pts = [&](u32 samples_to_add)
								{
									const u32 sample_count = audio_sample_remainder + samples_to_add;
									const u32 pts_to_add = sample_count / m_samples_per_block;
									audio_sample_remainder = sample_count % m_samples_per_block;
									last_audio_pts += pts_to_add;
								};

								// Copy as many old samples to our audio frame as possible
								if (leftover_sample_count > 0)
								{
									const u32 samples_to_add = std::min(leftover_sample_count, av.audio.frame->nb_samples - audio_frame_sample_count);

									if (samples_to_add > 0)
									{
										const u8* src = &last_samples.data[(last_samples.sample_count - leftover_sample_count) * last_samples.channels * sizeof(f32)];
										u8* dst = &audio_frame[audio_frame_sample_count * last_samples.channels * sizeof(f32)];
										copy_samples<f32>(src, dst, samples_to_add * last_samples.channels, swap_endianness);
										audio_frame_sample_count += samples_to_add;
										leftover_sample_count -= samples_to_add;
										update_last_pts(samples_to_add);
									}

									if (samples_to_add < leftover_sample_count)
									{
										media_log.error("video_encoder: audio frame buffer is already filled entirely by last sample package...");
									}
								}
								else if (silence_to_add > 0)
								{
									const u32 samples_to_add = std::min<s32>(silence_to_add, av.audio.frame->nb_samples - audio_frame_sample_count);

									if (samples_to_add > 0)
									{
										u8* dst = &audio_frame[audio_frame_sample_count * av.audio.frame->ch_layout.nb_channels * sizeof(f32)];
										std::memset(dst, 0, samples_to_add * sample_data.channels * sizeof(f32));
										audio_frame_sample_count += samples_to_add;
										update_last_pts(samples_to_add);
									}
								}
								else if (add_new_sample)
								{
									// Copy as many new samples to our audio frame as possible
									const u32 samples_to_add = std::min<s32>(sample_data.sample_count, av.audio.frame->nb_samples - audio_frame_sample_count);

									if (samples_to_add > 0)
									{
										const u8* src = sample_data.data.data();
										u8* dst = &audio_frame[audio_frame_sample_count * sample_data.channels * sizeof(f32)];
										copy_samples<f32>(src, dst, samples_to_add * sample_data.channels, swap_endianness);
										audio_frame_sample_count += samples_to_add;
										update_last_pts(samples_to_add);
									}

									if (samples_to_add < sample_data.sample_count)
									{
										// Save this sample package for the next loop if it wasn't fully used.
										leftover_sample_count = sample_data.sample_count - samples_to_add;
									}
									else
									{
										// Mark this sample package as fully used.
										leftover_sample_count = 0;
									}

									last_samples = std::move(sample_data);
								}

								send_frame();
							};

							for (u32 sample = 0; !has_error;)
							{
								if (leftover_sample_count > 0)
								{
									// Add leftover samples
									add_encoder_sample(false);
								}
								else if (pts > (last_audio_pts + 1))
								{
									// Add silence to fill the gap
									const u32 silence_to_add = static_cast<u32>(pts - (last_audio_pts + 1));
									add_encoder_sample(false, silence_to_add);
								}
								else if (sample == 0)
								{
									// Add new samples
									add_encoder_sample(true);
									sample++;
								}
								else
								{
									break;
								}
							}

							m_last_audio_pts = last_audio_pts;
						}
					}
				}

				if (!got_frame && !got_sample)
				{
					if (m_flush)
					{
						m_flush = false;

						if (!m_paused)
						{
							// We only stop the thread after a flush if we are not paused
							break;
						}
					}

					// We only actually pause after we process all frames
					const u64 sleeptime_us = m_paused ? 10000 : 1;
					thread_ctrl::wait_for(sleeptime_us);
					continue;
				}
			}

			if (av.video.context)
			{
				if (int err = avcodec_send_frame(av.video.context, nullptr); err != 0)
				{
					media_log.error("video_encoder: final avcodec_send_frame failed. Error: %d='%s'", err, av_error_to_string(err));
				}
			}

			if (av.audio.context)
			{
				if (int err = avcodec_send_frame(av.audio.context, nullptr); err != 0)
				{
					media_log.error("video_encoder: final avcodec_send_frame failed. Error: %d='%s'", err, av_error_to_string(err));
				}
			}

			flush(av.video);
			flush(av.audio);

			if (int err = av_write_trailer(av.format_context); err != 0)
			{
				media_log.error("video_encoder: av_write_trailer failed. Error: %d='%s'", err, av_error_to_string(err));
			}

			if (int err = avio_closep(&av.format_context->pb); err != 0)
			{
				media_log.error("video_encoder: avio_closep failed. Error: %d='%s'", err, av_error_to_string(err));
			}
		});
	}
}
