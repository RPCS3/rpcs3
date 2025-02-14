#pragma once

#include "Utilities/StrUtil.h"
#include "Utilities/Thread.h"
#include "util/video_sink.h"
#include "Emu/Cell/Modules/cellMusic.h"

#include <unordered_map>
#include <deque>

namespace utils
{
	std::string av_error_to_string(int error);

	struct ffmpeg_codec
	{
		int codec_id{};
		std::string name;
		std::string long_name;
	};

	std::vector<ffmpeg_codec> list_ffmpeg_decoders();
	std::vector<ffmpeg_codec> list_ffmpeg_encoders();

	struct media_info
	{
		std::string path;
		std::string sub_type; // The sub type if available (png, jpg...)

		s32 audio_av_codec_id = 0; // 0 = AV_CODEC_ID_NONE
		s32 video_av_codec_id = 0; // 0 = AV_CODEC_ID_NONE
		s32 audio_bitrate_bps = 0; // Bit rate in bit/s
		s32 video_bitrate_bps = 0; // Bit rate in bit/s
		s32 sample_rate = 0; // Samples per second
		s64 duration_us = 0; // in AV_TIME_BASE fractional seconds (= microseconds)
		s32 width = 0;  // Width if available
		s32 height = 0; // Height if available
		s32 orientation = 0; // Orientation if available (= CellSearchOrientation)

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

		void set_context(music_selection_context context);
		void set_swap_endianness(bool swapped);
		void clear();
		void stop();
		void decode();
		u32 set_next_index(bool next);

		shared_mutex m_mtx;
		static constexpr s32 sample_rate = 48000;
		std::vector<u8> data;
		atomic_t<u64> m_size = 0;
		atomic_t<u32> track_fully_decoded{0};
		atomic_t<u32> track_fully_consumed{0};
		atomic_t<bool> has_error{false};
		std::deque<std::pair<u64, u64>> timestamps_ms;

	private:
		bool m_swap_endianness = false;
		music_selection_context m_context{};
		std::unique_ptr<named_thread<std::function<void()>>> m_thread;
	};

	class video_encoder : public utils::video_sink
	{
	public:
		video_encoder();
		~video_encoder();

		struct frame_format
		{
			s32 av_pixel_format = 0; // NOTE: Make sure this is a valid AVPixelFormat
			u32 width = 0;
			u32 height = 0;
			u32 pitch = 0;

			std::string to_string() const
			{
				return fmt::format("{ av_pixel_format=%d, width=%d, height=%d, pitch=%d }", av_pixel_format, width, height, pitch);
			}
		};

		std::string path() const;
		s64 last_video_pts() const;

		void set_path(const std::string& path);
		void set_framerate(u32 framerate);
		void set_video_bitrate(u32 bitrate);
		void set_output_format(frame_format format);
		void set_video_codec(s32 codec_id);
		void set_max_b_frames(s32 max_b_frames);
		void set_gop_size(s32 gop_size);
		void set_sample_rate(u32 sample_rate);
		void set_audio_channels(u32 channels);
		void set_audio_bitrate(u32 bitrate);
		void set_audio_codec(s32 codec_id);
		void pause(bool flush = true) override;
		void stop(bool flush = true) override;
		void resume() override;
		void encode();

	private:
		std::string m_path;
		s64 m_last_audio_pts = 0;
		s64 m_last_video_pts = 0;

		// Thread control
		std::unique_ptr<named_thread<std::function<void()>>> m_thread;
		atomic_t<bool> m_running = false;

		// Video parameters
		u32 m_video_bitrate_bps = 0;
		s32 m_video_codec_id = 12; // AV_CODEC_ID_MPEG4
		s32 m_max_b_frames = 2;
		s32 m_gop_size = 12;
		frame_format m_out_format{};

		// Audio parameters
		u32 m_channels = 2;
		u32 m_audio_bitrate_bps = 320000;
		s32 m_audio_codec_id = 86018; // AV_CODEC_ID_AAC
	};
}
