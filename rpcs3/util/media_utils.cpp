#include "stdafx.h"
#include "media_utils.h"
#include "Emu/System.h"

#include <random>

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
			if (!success) media_log.error("get_image_info: failed to get image info for '%s'", path);
			return { success, std::move(info) };
		}

		// Only print FFMPEG errors, fatals and panics
		av_log_set_level(AV_LOG_ERROR);

		AVDictionary* av_dict_opts = nullptr;
		if (int err = av_dict_set(&av_dict_opts, "probesize", "96", 0); err < 0)
		{
			media_log.error("av_dict_set: returned with error=%d='%s'", err, av_error_to_string(err));
			return { false, std::move(info) };
		}

		AVFormatContext* av_format_ctx = avformat_alloc_context();

		// Open input file
		if (int err = avformat_open_input(&av_format_ctx, path.c_str(), nullptr, &av_dict_opts); err < 0)
		{
			// Failed to open file
			av_dict_free(&av_dict_opts);
			avformat_free_context(av_format_ctx);
			media_log.notice("avformat_open_input: could not open file. error=%d='%s' file='%s'", err, av_error_to_string(err), path);
			return { false, std::move(info) };
		}
		av_dict_free(&av_dict_opts);

		// Find stream information
		if (int err = avformat_find_stream_info(av_format_ctx, nullptr); err < 0)
		{
			// Failed to load stream information
			avformat_close_input(&av_format_ctx);
			avformat_free_context(av_format_ctx);
			media_log.notice("avformat_find_stream_info: could not load stream information. error=%d='%s' file='%s'", err, av_error_to_string(err), path);
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

	struct scoped_av
	{
		AVFormatContext* format = nullptr;
		const AVCodec* codec = nullptr;
		AVCodecContext* context = nullptr;
		AVFrame* frame = nullptr;
		AVStream* stream = nullptr;
		SwrContext* swr = nullptr;
		SwsContext* sws = nullptr;
		std::function<void()> kill_callback = nullptr;

		~scoped_av()
		{
			// Clean up
			if (frame)
			{
				av_frame_unref(frame);
				av_frame_free(&frame);
			}
			if (swr)
				swr_free(&swr);
			if (sws)
				sws_freeContext(sws);
			if (context)
				avcodec_close(context);
			// AVCodec is managed by libavformat, no need to free it
			// see: https://stackoverflow.com/a/18047320
			if (format)
				avformat_free_context(format);
			//if (stream)
			//	av_free(stream);
			if (kill_callback)
				kill_callback();
		}
	};

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
		duration_ms = 0;
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
			scoped_av av;

			// Get format from audio file
			av.format = avformat_alloc_context();
			if (int err = avformat_open_input(&av.format, path.c_str(), nullptr, nullptr); err < 0)
			{
				media_log.error("audio_decoder: Could not open file '%s'. Error: %d='%s'", path, err, av_error_to_string(err));
				has_error = true;
				return;
			}
			if (int err = avformat_find_stream_info(av.format, nullptr); err < 0)
			{
				media_log.error("audio_decoder: Could not retrieve stream info from file '%s'. Error: %d='%s'", path, err, av_error_to_string(err));
				has_error = true;
				return;
			}

			// Find the first audio stream
			AVStream* stream = nullptr;
			unsigned int stream_index;
			for (stream_index = 0; stream_index < av.format->nb_streams; stream_index++)
			{
				if (av.format->streams[stream_index]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
				{
					stream = av.format->streams[stream_index];
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
			av.codec = avcodec_find_decoder(stream->codecpar->codec_id);
			if (!av.codec)
			{
				media_log.error("audio_decoder: Failed to find decoder for stream #%u in file '%s'", stream_index, path);
				has_error = true;
				return;
			}

			// Allocate context
			av.context = avcodec_alloc_context3(av.codec);
			if (!av.context)
			{
				media_log.error("audio_decoder: Failed to allocate context for stream #%u in file '%s'", stream_index, path);
				has_error = true;
				return;
			}

			// Open decoder
			if (int err = avcodec_open2(av.context, av.codec, nullptr); err < 0)
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
			av.frame = av_frame_alloc();
			if (!av.frame)
			{
				media_log.error("audio_decoder: Error allocating the frame");
				has_error = true;
				return;
			}

			duration_ms = stream->duration / 1000;

			AVPacket* packet = av_packet_alloc();
			std::unique_ptr<AVPacket, decltype([](AVPacket* p){av_packet_unref(p);})> packet_(packet);

			// Iterate through frames
			while (thread_ctrl::state() != thread_state::aborting && av_read_frame(av.format, packet) >= 0)
			{
				if (int err = avcodec_send_packet(av.context, packet); err < 0)
				{
					media_log.error("audio_decoder: Queuing error: %d='%s'", err, av_error_to_string(err));
					has_error = true;
					return;
				}

				while (thread_ctrl::state() != thread_state::aborting)
				{
					if (int err = avcodec_receive_frame(av.context, av.frame); err < 0)
					{
						if (err == AVERROR(EAGAIN) || err == averror_eof)
							break;

						media_log.error("audio_decoder: Decoding error: %d='%s'", err, av_error_to_string(err));
						has_error = true;
						return;
					}

					// Resample frames
					u8* buffer;
					const int align = 1;
					const int buffer_size = av_samples_alloc(&buffer, nullptr, dst_channels, av.frame->nb_samples, dst_format, align);
					if (buffer_size < 0)
					{
						media_log.error("audio_decoder: Error allocating buffer: %d='%s'", buffer_size, av_error_to_string(buffer_size));
						has_error = true;
						return;
					}

					const int frame_count = swr_convert(av.swr, &buffer, av.frame->nb_samples, const_cast<const uint8_t**>(av.frame->data), av.frame->nb_samples);
					if (frame_count < 0)
					{
						media_log.error("audio_decoder: Error converting frame: %d='%s'", frame_count, av_error_to_string(frame_count));
						has_error = true;
						if (buffer)
							av_free(buffer);
						return;
					}

					// Append resampled frames to data
					{
						std::scoped_lock lock(m_mtx);
						data.resize(m_size + buffer_size);

						if (m_swap_endianness)
						{
							// The format is float 32bit per channel.
							const auto write_byteswapped = [](const void* src, void* dst) -> void
							{
								*static_cast<f32*>(dst) = *static_cast<const be_t<f32>*>(src);
							};

							for (size_t i = 0; i < (buffer_size - sizeof(f32)); i += sizeof(f32))
							{
								write_byteswapped(buffer + i, data.data() + m_size + i);
							}
						}
						else
						{
							memcpy(&data[m_size], buffer, buffer_size);
						}

						const s64 timestamp_ms = stream->time_base.den ? (1000 * av.frame->best_effort_timestamp * stream->time_base.num) / stream->time_base.den : 0;
						timestamps_ms.push_back({m_size, timestamp_ms});
						m_size += buffer_size;
					}

					if (buffer)
						av_free(buffer);

					media_log.notice("audio_decoder: decoded frame_count=%d buffer_size=%d timestamp_us=%d", frame_count, buffer_size, av.frame->best_effort_timestamp);
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
				ensure(m_context.current_track < m_context.playlist.size());
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
		: utils::image_sink()
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

	s64 video_encoder::last_pts() const
	{
		return m_last_pts;
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

	void video_encoder::set_audio_bitrate(u32 bitrate)
	{
		m_audio_bitrate_bps = bitrate;
	}

	void video_encoder::set_audio_codec(s32 codec_id)
	{
		m_audio_codec_id = codec_id;
	}

	void video_encoder::add_frame(std::vector<u8>& frame, u32 pitch, u32 width, u32 height, s32 pixel_format, usz timestamp_ms)
	{
		// Do not allow new frames while flushing
		if (m_flush)
			return;

		std::lock_guard lock(m_mtx);
		m_frames_to_encode.emplace_back(timestamp_ms, pitch, width, height, pixel_format, std::move(frame));
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

			auto& thread = *m_thread;
			thread = thread_state::aborting;
			thread();

			m_thread.reset();
		}

		std::lock_guard lock(m_mtx);
		m_frames_to_encode.clear();
		has_error = false;
		m_flush = false;
		m_paused = false;
		m_running = false;
	}

	void video_encoder::encode()
	{
		if (m_running)
		{
			// Resume
			m_flush = false;
			m_paused = false;
			media_log.success("video_encoder: resuming recording of '%s'", m_path);
			return;
		}

		m_last_pts = 0;

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

			// TODO: audio encoding

			// Reset variables at all costs
			scoped_av av;
			av.kill_callback = [this]()
			{
				m_flush = false;
				m_running = false;
			};

			const AVPixelFormat out_format = static_cast<AVPixelFormat>(m_out_format.av_pixel_format);
			const char* av_output_format = nullptr;

			const auto find_format = [&](const AVCodec* codec) -> const char*
			{
				if (!codec)
					return nullptr;

				// Try to find a preferable output format
				std::vector<const AVOutputFormat*> oformats;

				void* opaque = nullptr;
				for (const AVOutputFormat* oformat = av_muxer_iterate(&opaque); !!oformat; oformat = av_muxer_iterate(&opaque))
				{
					if (avformat_query_codec(oformat, codec->id, FF_COMPLIANCE_STRICT) == 1)
					{
						media_log.notice("video_encoder: Found output format '%s'", oformat->name);

						switch (codec->id)
						{
						case AV_CODEC_ID_MPEG4:
							if (strcmp(oformat->name, "avi") == 0)
								return oformat->name;
							break;
						case AV_CODEC_ID_H264:
						case AV_CODEC_ID_MJPEG:
							// TODO
							break;
						default:
							break;
						}

						oformats.push_back(oformat);
					}
				}

				// Fallback to first found format
				if (!oformats.empty() && oformats.front())
				{
					const AVOutputFormat* oformat = oformats.front();
					media_log.notice("video_encoder: Falling back to output format '%s'", oformat->name);
					return oformat->name;
				}

				return nullptr;
			};

			AVCodecID used_codec = static_cast<AVCodecID>(m_video_codec_id);

			// Find specified codec first
			if (const AVCodec* encoder = avcodec_find_encoder(used_codec); !!encoder)
			{
				media_log.success("video_encoder: Found requested video_codec %d = %s", static_cast<int>(used_codec), encoder->name);
				av_output_format = find_format(encoder);

				if (av_output_format)
				{
					media_log.success("video_encoder: Found requested output format '%s'", av_output_format);
				}
				else
				{
					media_log.error("video_encoder: Could not find a format for the requested video_codec %d = %s", static_cast<int>(used_codec), encoder->name);
				}
			}
			else
			{
				media_log.error("video_encoder: Could not find requested video_codec %d", static_cast<int>(used_codec));
			}

			// Fallback to some other codec
			if (!av_output_format)
			{
				void* opaque = nullptr;
				for (const AVCodec* codec = av_codec_iterate(&opaque); !!codec; codec = av_codec_iterate(&opaque))
				{
					if (av_codec_is_encoder(codec))
					{
						media_log.notice("video_encoder: Found video_codec %d = %s", static_cast<int>(codec->id), codec->name);
						av_output_format = find_format(codec);

						if (av_output_format)
						{
							media_log.success("video_encoder: Found fallback output format '%s'", av_output_format);
							break;
						}
					}
				}
			}

			if (!av_output_format)
			{
				media_log.error("video_encoder: Could not find any output format");
				has_error = true;
				return;
			}

			if (int err = avformat_alloc_output_context2(&av.format, nullptr, av_output_format, path.c_str()); err < 0)
			{
				media_log.error("video_encoder: avformat_alloc_output_context2 failed. Error: %d='%s'", err, av_error_to_string(err));
				has_error = true;
				return;
			}

			if (!av.format)
			{
				media_log.error("video_encoder: avformat_alloc_output_context2 failed");
				has_error = true;
				return;
			}

			if (!(av.codec = avcodec_find_encoder(av.format->oformat->video_codec)))
			{
				media_log.error("video_encoder: avcodec_find_encoder failed");
				has_error = true;
				return;
			}

			if (!(av.stream = avformat_new_stream(av.format, nullptr)))
			{
				media_log.error("video_encoder: avformat_new_stream failed");
				has_error = true;
				return;
			}

			av.stream->id = static_cast<int>(av.format->nb_streams - 1);

			if (!(av.context = avcodec_alloc_context3(av.codec)))
			{
				media_log.error("video_encoder: avcodec_alloc_context3 failed");
				has_error = true;
				return;
			}

			media_log.notice("video_encoder: using video_codec = %d", static_cast<int>(av.format->oformat->video_codec));
			media_log.notice("video_encoder: using video_bitrate = %d", m_video_bitrate_bps);
			media_log.notice("video_encoder: using out width = %d", m_out_format.width);
			media_log.notice("video_encoder: using out height = %d", m_out_format.height);
			media_log.notice("video_encoder: using framerate = %d", m_framerate);
			media_log.notice("video_encoder: using gop_size = %d", m_gop_size);
			media_log.notice("video_encoder: using max_b_frames = %d", m_max_b_frames);

			av.context->codec_id = av.format->oformat->video_codec;
			av.context->bit_rate = m_video_bitrate_bps;
			av.context->width = static_cast<int>(m_out_format.width);
			av.context->height = static_cast<int>(m_out_format.height);
			av.context->time_base = {.num = 1, .den = static_cast<int>(m_framerate)};
			av.context->framerate = {.num = static_cast<int>(m_framerate), .den = 1};
			av.context->pix_fmt = out_format;
			av.context->gop_size = m_gop_size;
			av.context->max_b_frames = m_max_b_frames;

			if (av.format->oformat->flags & AVFMT_GLOBALHEADER)
			{
				av.context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
			}

			if (int err = avcodec_open2(av.context, av.codec, nullptr); err != 0)
			{
				media_log.error("video_encoder: avcodec_open2 failed. Error: %d='%s'", err, av_error_to_string(err));
				has_error = true;
				return;
			}

			if (!(av.frame = av_frame_alloc()))
			{
				media_log.error("video_encoder: av_frame_alloc failed");
				has_error = true;
				return;
			}

			av.frame->format = av.context->pix_fmt;
			av.frame->width = av.context->width;
			av.frame->height = av.context->height;

			if (int err = av_frame_get_buffer(av.frame, 32); err < 0)
			{
				media_log.error("video_encoder: av_frame_get_buffer failed. Error: %d='%s'", err, av_error_to_string(err));
				has_error = true;
				return;
			}

			if (int err = avcodec_parameters_from_context(av.stream->codecpar, av.context); err < 0)
			{
				media_log.error("video_encoder: avcodec_parameters_from_context failed. Error: %d='%s'", err, av_error_to_string(err));
				has_error = true;
				return;
			}

			av_dump_format(av.format, 0, path.c_str(), 1);

			if (int err = avio_open(&av.format->pb, path.c_str(), AVIO_FLAG_WRITE); err != 0)
			{
				media_log.error("video_encoder: avio_open failed. Error: %d='%s'", err, av_error_to_string(err));
				has_error = true;
				return;
			}

			if (int err = avformat_write_header(av.format, nullptr); err < 0)
			{
				media_log.error("video_encoder: avformat_write_header failed. Error: %d='%s'", err, av_error_to_string(err));

				if (int err = avio_close(av.format->pb); err != 0)
				{
					media_log.error("video_encoder: avio_close failed. Error: %d='%s'", err, av_error_to_string(err));
				}

				has_error = true;
				return;
			}

			const auto flush = [&]()
			{
				while ((thread_ctrl::state() != thread_state::aborting || m_flush) && !has_error)
				{
					AVPacket* packet = av_packet_alloc();
					std::unique_ptr<AVPacket, decltype([](AVPacket* p){ if (p) av_packet_unref(p); })> packet_(packet);

					if (!packet)
					{
						media_log.error("video_encoder: av_packet_alloc failed");
						has_error = true;
						return;
					}

					if (int err = avcodec_receive_packet(av.context, packet); err < 0)
					{
						if (err == AVERROR(EAGAIN) || err == averror_eof)
							break;

						media_log.error("video_encoder: avcodec_receive_packet failed. Error: %d='%s'", err, av_error_to_string(err));
						has_error = true;
						return;
					}

					av_packet_rescale_ts(packet, av.context->time_base, av.stream->time_base);
					packet->stream_index = av.stream->index;

					if (int err = av_interleaved_write_frame(av.format, packet); err < 0)
					{
						media_log.error("video_encoder: av_interleaved_write_frame failed. Error: %d='%s'", err, av_error_to_string(err));
						has_error = true;
						return;
					}
				}
			};

			s64 last_pts = -1;

			while ((thread_ctrl::state() != thread_state::aborting || m_flush) && !has_error)
			{
				encoder_frame frame_data;
				{
					m_mtx.lock();

					if (m_frames_to_encode.empty())
					{
						m_mtx.unlock();

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
						const u64 sleeptime = m_paused ? 10000 : 1;
						thread_ctrl::wait_for(sleeptime);
						continue;
					}

					frame_data = std::move(m_frames_to_encode.front());
					m_frames_to_encode.pop_front();

					m_mtx.unlock();

					media_log.trace("video_encoder: adding new frame. timestamp=%d", frame_data.timestamp_ms);
				}

				// Calculate presentation timestamp.
				const s64 pts = get_pts(frame_data.timestamp_ms);

				// We need to skip this frame if it has the same timestamp.
				if (pts <= last_pts)
				{
					media_log.notice("video_encoder: skipping frame. last_pts=%d, pts=%d", last_pts, pts);
					continue;
				}

				if (int err = av_frame_make_writable(av.frame); err < 0)
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
				                              av.context->width, av.context->height, out_format, SWS_BICUBIC, nullptr, nullptr, nullptr);
				if (!av.sws)
				{
					media_log.error("video_encoder: sws_getCachedContext failed");
					has_error = true;
					break;
				}

				if (int err = sws_scale(av.sws, in_data, in_line, 0, frame_data.height, av.frame->data, av.frame->linesize); err < 0)
				{
					media_log.error("video_encoder: sws_scale failed. Error: %d='%s'", err, av_error_to_string(err));
					has_error = true;
					break;
				}

				av.frame->pts = pts;

				if (int err = avcodec_send_frame(av.context, av.frame); err < 0)
				{
					media_log.error("video_encoder: avcodec_send_frame failed. Error: %d='%s'", err, av_error_to_string(err));
					has_error = true;
					break;
				}

				flush();

				last_pts = av.frame->pts;

				m_last_pts = last_pts;
			}

			if (int err = avcodec_send_frame(av.context, nullptr); err != 0)
			{
				media_log.error("video_encoder: final avcodec_send_frame failed. Error: %d='%s'", err, av_error_to_string(err));
			}

			flush();

			if (int err = av_write_trailer(av.format); err != 0)
			{
				media_log.error("video_encoder: av_write_trailer failed. Error: %d='%s'", err, av_error_to_string(err));
			}

			if (int err = avio_close(av.format->pb); err != 0)
			{
				media_log.error("video_encoder: avio_close failed. Error: %d='%s'", err, av_error_to_string(err));
			}
		});
	}
}
