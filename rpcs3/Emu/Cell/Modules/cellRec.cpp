#include "stdafx.h"
#include "Emu/Cell/lv2/sys_memory.h"
#include "Emu/Cell/timers.hpp"
#include "Emu/Cell/PPUModule.h"
#include "Emu/IdManager.h"
#include "Emu/system_config.h"
#include "Emu/VFS.h"
#include "Emu/Audio/AudioBackend.h"
#include "cellRec.h"
#include "cellSysutil.h"
#include "util/media_utils.h"
#include "util/video_provider.h"

extern "C"
{
#include <libavutil/pixfmt.h>
}

LOG_CHANNEL(cellRec);

extern atomic_t<recording_mode> g_recording_mode;

template<>
void fmt_class_string<CellRecError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_REC_ERROR_OUT_OF_MEMORY);
			STR_CASE(CELL_REC_ERROR_FATAL);
			STR_CASE(CELL_REC_ERROR_INVALID_VALUE);
			STR_CASE(CELL_REC_ERROR_FILE_OPEN);
			STR_CASE(CELL_REC_ERROR_FILE_WRITE);
			STR_CASE(CELL_REC_ERROR_INVALID_STATE);
			STR_CASE(CELL_REC_ERROR_FILE_NO_DATA);
		}

		return unknown;
	});
}

// Helper to distinguish video type
enum : s32
{
	VIDEO_TYPE_MPEG4  = 0x0000,
	VIDEO_TYPE_AVC_MP = 0x1000,
	VIDEO_TYPE_AVC_BL = 0x2000,
	VIDEO_TYPE_MJPEG  = 0x3000,
	VIDEO_TYPE_M4HD   = 0x4000,
};

// Helper to distinguish video quality
enum : s32
{
	VIDEO_QUALITY_0 = 0x000, // small
	VIDEO_QUALITY_1 = 0x100, // middle
	VIDEO_QUALITY_2 = 0x200, // large
	VIDEO_QUALITY_3 = 0x300, // youtube
	VIDEO_QUALITY_4 = 0x400,
	VIDEO_QUALITY_5 = 0x500,
	VIDEO_QUALITY_6 = 0x600, // HD720
	VIDEO_QUALITY_7 = 0x700,
};

enum class rec_state : u32
{
	closed  = 0x2710,
	stopped = 0x2711,
	started = 0xBEEF, // I don't know the value of this
};

struct rec_param
{
	s32 ppu_thread_priority = CELL_REC_PARAM_PPU_THREAD_PRIORITY_DEFAULT;
	s32 spu_thread_priority = CELL_REC_PARAM_SPU_THREAD_PRIORITY_DEFAULT;
	s32 capture_priority = CELL_REC_PARAM_CAPTURE_PRIORITY_HIGHEST;
	s32 use_system_spu = CELL_REC_PARAM_USE_SYSTEM_SPU_DISABLE;
	s32 fit_to_youtube = 0;
	s32 xmb_bgm = CELL_REC_PARAM_XMB_BGM_DISABLE;
	s32 mpeg4_fast_encode = CELL_REC_PARAM_MPEG4_FAST_ENCODE_DISABLE;
	u32 ring_sec = 0;
	s32 video_input = CELL_REC_PARAM_VIDEO_INPUT_DISABLE;
	s32 audio_input = CELL_REC_PARAM_AUDIO_INPUT_DISABLE;
	s32 audio_input_mix_vol = CELL_REC_PARAM_AUDIO_INPUT_MIX_VOL_MIN;
	s32 reduce_memsize = CELL_REC_PARAM_REDUCE_MEMSIZE_DISABLE;
	s32 show_xmb = 0;
	std::string filename;
	std::string metadata_filename;
	CellRecSpursParam spurs_param{};
	struct
	{
		std::string game_title;
		std::string movie_title;
		std::string description;
		std::string userdata;

		std::string to_string() const
		{
			return fmt::format("{ game_title='%s', movie_title='%s', description='%s', userdata='%s' }", game_title, movie_title, description, userdata);
		}
	} movie_metadata{};
	struct
	{
		bool is_set = false;
		u32 type = 0;
		u64 start_time = 0;
		u64 end_time = 0;
		std::string title;
		std::vector<std::string> tags;

		std::string to_string() const
		{
			std::string scene_metadata_tags;
			for (usz i = 0; i < tags.size(); i++)
			{
				if (i > 0) scene_metadata_tags += ", ";
				fmt::append(scene_metadata_tags, "'%s'", tags[i]);
			}
			return fmt::format("{ is_set=%d, type=%d, start_time=%d, end_time=%d, title='%s', tags=[ %s ] }", is_set, type, start_time, end_time, title, scene_metadata_tags);
		}
	} scene_metadata{};

	std::string to_string() const
	{
		std::string priority;
		for (usz i = 0; i < 8; i++)
		{
			if (i > 0) priority += ", ";
			fmt::append(priority, "%d", spurs_param.priority[i]);
		}
		return fmt::format("ppu_thread_priority=%d, spu_thread_priority=%d, capture_priority=%d, use_system_spu=%d, fit_to_youtube=%d, "
			"xmb_bgm=%d, mpeg4_fast_encode=%d, ring_sec=%d, video_input=%d, audio_input=%d, audio_input_mix_vol=%d, reduce_memsize=%d, "
			"show_xmb=%d, filename='%s', metadata_filename='%s', spurs_param={ pSpurs=*0x%x, spu_usage_rate=%d, priority=[ %s ], "
			"movie_metadata=%s, scene_metadata=%s",
			ppu_thread_priority, spu_thread_priority, capture_priority, use_system_spu, fit_to_youtube, xmb_bgm, mpeg4_fast_encode, ring_sec,
			video_input, audio_input, audio_input_mix_vol, reduce_memsize, show_xmb, filename, metadata_filename, spurs_param.pSpurs, spurs_param.spu_usage_rate,
			priority, movie_metadata.to_string(), scene_metadata.to_string());
	}

	bool use_external_audio() const
	{
		return audio_input != CELL_REC_PARAM_AUDIO_INPUT_DISABLE             // != DISABLE means that cellRec will add samples on its own
			&& audio_input_mix_vol > CELL_REC_PARAM_AUDIO_INPUT_MIX_VOL_MIN; // We need to mix cellRec audio with internal audio
	}

	bool use_internal_audio() const
	{
		return audio_input == CELL_REC_PARAM_AUDIO_INPUT_DISABLE             // DISABLE means that cellRec won't add samples on its own
		    || audio_input_mix_vol < CELL_REC_PARAM_AUDIO_INPUT_MIX_VOL_MAX; // We need to mix cellRec audio with internal audio
	}

	bool use_internal_video() const
	{
		return video_input == CELL_REC_PARAM_VIDEO_INPUT_DISABLE; // DISABLE means that cellRec won't add frames on its own
	}
};

static constexpr u32 rec_framerate = 30; // Always 30 fps
static constexpr u32 rec_channels = 2; // Always 2 channels

class rec_video_sink final : public utils::video_sink
{
public:
	rec_video_sink() : utils::video_sink()
	{
		m_framerate = rec_framerate;
	}

	void set_sample_rate(u32 sample_rate)
	{
		m_sample_rate = sample_rate;
	}

	void stop(bool flush = true) override
	{
		cellRec.notice("Stopping video sink. flush=%d", flush);

		std::scoped_lock lock(m_video_mtx, m_audio_mtx);
		m_flush = flush;
		m_paused = false;
		m_frames_to_encode.clear();
		m_samples_to_encode.clear();
		has_error = false;
	}

	void pause(bool flush = true) override
	{
		cellRec.notice("Pausing video sink. flush=%d", flush);

		std::scoped_lock lock(m_video_mtx, m_audio_mtx);
		m_flush = flush;
		m_paused = true;
	}

	void resume() override
	{
		cellRec.notice("Resuming video sink");

		std::scoped_lock lock(m_video_mtx, m_audio_mtx);
		m_flush = false;
		m_paused = false;
	}

	encoder_frame get_frame()
	{
		std::lock_guard lock_video(m_video_mtx);

		if (!m_frames_to_encode.empty())
		{
			encoder_frame frame = std::move(m_frames_to_encode.front());
			m_frames_to_encode.pop_front();
			return frame;
		}

		return {};
	}

	encoder_sample get_sample()
	{
		std::lock_guard lock(m_audio_mtx);

		if (!m_samples_to_encode.empty())
		{
			encoder_sample block = std::move(m_samples_to_encode.front());
			m_samples_to_encode.pop_front();
			return block;
		}

		return {};
	}
};

struct rec_info
{
	vm::ptr<CellRecCallback> cb{};
	vm::ptr<void> cbUserData{};
	atomic_t<rec_state> state = rec_state::closed;
	rec_param param{};

	// These buffers are used by the game to inject one frame into the recording.
	// This apparently happens right before a frame is rendered to the screen.
	// It is only possible to inject a frame if the game has the external input mode enabled.
	vm::bptr<u8> video_input_buffer{}; // Used by the game to inject a frame right before it would render a frame to the screen.
	vm::bptr<u8> audio_input_buffer{}; // Used by the game to inject audio: 2-channel interleaved (left-right) * 256 samples * sizeof(f32) at 48000 kHz

	// Wrapper for our audio data
	struct audio_block
	{
		// 2-channel interleaved (left-right), 256 samples, float
		static constexpr usz block_size = rec_channels * CELL_REC_AUDIO_BLOCK_SAMPLES * sizeof(f32);
		std::array<u8, block_size> block{};
		s64 pts{};
	};

	std::vector<utils::video_sink::encoder_frame> video_ringbuffer;
	std::vector<audio_block> audio_ringbuffer;
	usz video_ring_pos = 0;
	usz audio_ring_pos = 0;
	usz video_ring_frame_count = 0;
	usz audio_ring_block_count = 0;

	usz next_video_ring_pos()
	{
		const usz pos = video_ring_pos;
		video_ring_pos = (video_ring_pos + 1) % video_ringbuffer.size();
		return pos;
	}

	usz next_audio_ring_pos()
	{
		const usz pos = audio_ring_pos;
		audio_ring_pos = (audio_ring_pos + 1) % audio_ringbuffer.size();
		return pos;
	}

	std::shared_ptr<rec_video_sink> sink;
	std::shared_ptr<utils::video_encoder> encoder;
	std::unique_ptr<named_thread<std::function<void()>>> video_provider_thread;
	atomic_t<bool> paused = false;

	// Video parameters
	utils::video_encoder::frame_format output_format{};
	utils::video_encoder::frame_format input_format{};
	u32 video_bps = 512000;
	s32 video_codec_id = 12; // AV_CODEC_ID_MPEG4
	s32 max_b_frames = 2;
	static constexpr u32 fps = rec_framerate; // Always 30 fps

	// Audio parameters
	u32 sample_rate = 48000;
	u32 audio_bps = 64000;
	s32 audio_codec_id = 86018; // AV_CODEC_ID_AAC
	static constexpr u32 channels = rec_channels; // Always 2 channels

	// Recording duration
	atomic_t<u64> recording_time_start = 0;
	atomic_t<u64> pause_time_start = 0;
	atomic_t<u64> pause_time_total = 0;
	atomic_t<u64> recording_time_total = 0;

	shared_mutex mutex;

	void set_video_params(s32 video_format);
	void set_audio_params(s32 audio_format);

	void start_video_provider();
	void pause_video_provider();
	void stop_video_provider(bool flush);
};

void rec_info::set_video_params(s32 video_format)
{
	const s32 video_type = video_format & 0xf000;
	const s32 video_quality = video_format & 0xf00;

	switch (video_format)
	{
	case CELL_REC_PARAM_VIDEO_FMT_MPEG4_SMALL_512K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_MPEG4_MIDDLE_512K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_512K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_AVC_MP_SMALL_512K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_AVC_MP_MIDDLE_512K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_AVC_BL_SMALL_512K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_AVC_BL_MIDDLE_512K_30FPS:
		video_bps = 512000;
		break;
	case CELL_REC_PARAM_VIDEO_FMT_MPEG4_SMALL_768K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_MPEG4_MIDDLE_768K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_768K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_AVC_MP_SMALL_768K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_AVC_MP_MIDDLE_768K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_AVC_BL_SMALL_768K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_AVC_BL_MIDDLE_768K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_M4HD_SMALL_768K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_M4HD_MIDDLE_768K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_YOUTUBE:
		video_bps = 768000;
		break;
	case CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_1024K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_AVC_MP_MIDDLE_1024K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_AVC_BL_MIDDLE_1024K_30FPS:
		video_bps = 1024000;
		break;
	case CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_1536K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_AVC_MP_MIDDLE_1536K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_AVC_BL_MIDDLE_1536K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_M4HD_LARGE_1536K_30FPS:
		video_bps = 1536000;
		break;
	case CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_2048K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_M4HD_LARGE_2048K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_M4HD_HD720_2048K_30FPS:
		video_bps = 2048000;
		break;
	case CELL_REC_PARAM_VIDEO_FMT_MJPEG_SMALL_5000K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_MJPEG_MIDDLE_5000K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_M4HD_HD720_5000K_30FPS:
		video_bps = 5000000;
		break;
	case CELL_REC_PARAM_VIDEO_FMT_MJPEG_LARGE_11000K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_MJPEG_HD720_11000K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_M4HD_HD720_11000K_30FPS:
		video_bps = 11000000;
		break;
	case CELL_REC_PARAM_VIDEO_FMT_MJPEG_HD720_20000K_30FPS:
		video_bps = 20000000;
		break;
	case CELL_REC_PARAM_VIDEO_FMT_MJPEG_HD720_25000K_30FPS:
		video_bps = 25000000;
		break;
	case 0x3791:
	case 0x37a1:
	case 0x3790:
	case 0x37a0:
	default:
		// TODO: confirm bitrate
		video_bps = 512000;
		break;
	}

	max_b_frames = 2;

	switch (video_type)
	{
	default:
	case VIDEO_TYPE_MPEG4:
		video_codec_id = 12; // AV_CODEC_ID_MPEG4
		break;
	case VIDEO_TYPE_AVC_MP:
		video_codec_id = 27; // AV_CODEC_ID_H264
		break;
	case VIDEO_TYPE_AVC_BL:
		video_codec_id = 27; // AV_CODEC_ID_H264
		max_b_frames = 0; // Apparently baseline H264 does not support B-Frames
		break;
	case VIDEO_TYPE_MJPEG:
		video_codec_id = 7; // AV_CODEC_ID_MJPEG
		break;
	case VIDEO_TYPE_M4HD:
		// TODO: no idea if these are H264 or MPEG4
		video_codec_id = 12; // AV_CODEC_ID_MPEG4
		break;
	}

	const bool wide = g_cfg.video.aspect_ratio == video_aspect::_16_9;
	bool hd = true;

	switch(g_cfg.video.resolution)
	{
	case video_resolution::_1080p:
	case video_resolution::_1080i:
	case video_resolution::_720p:
	case video_resolution::_1600x1080p:
	case video_resolution::_1440x1080p:
	case video_resolution::_1280x1080p:
	case video_resolution::_960x1080p:
		hd = true;
		break;
	case video_resolution::_480p:
	case video_resolution::_480i:
	case video_resolution::_576p:
	case video_resolution::_576i:
		hd = false;
		break;
	}

	switch (video_quality)
	{
	case VIDEO_QUALITY_0: // small
		input_format.width = wide ? 368 : 320;
		input_format.height = wide ? 208 : 240;
		break;
	case VIDEO_QUALITY_1: // middle
		input_format.width = wide ? 480 : 368;
		input_format.height = 272;
		break;
	case VIDEO_QUALITY_2: // large
		input_format.width = wide ? 640 : 480;
		input_format.height = 368;
		break;
	case VIDEO_QUALITY_3: // youtube
		input_format.width = 320;
		input_format.height = 240;
		break;
	case VIDEO_QUALITY_4:
	case VIDEO_QUALITY_5:
		// TODO:
		break;
	case VIDEO_QUALITY_6: // HD720
		input_format.width = hd ? 1280 : (wide ? 864 : 640);
		input_format.height = hd ? 720 : 480;
		break;
	case VIDEO_QUALITY_7:
	default:
		// TODO:
		input_format.width = 1280;
		input_format.height = 720;
		break;
	}

	output_format.av_pixel_format = AVPixelFormat::AV_PIX_FMT_YUV420P;
	output_format.width = input_format.width;
	output_format.height = input_format.height;
	output_format.pitch = input_format.width * 4; // unused

	switch (param.video_input)
	{
	case CELL_REC_PARAM_VIDEO_INPUT_ARGB_4_3:
	case CELL_REC_PARAM_VIDEO_INPUT_ARGB_16_9:
		input_format.av_pixel_format = AVPixelFormat::AV_PIX_FMT_ARGB;
		input_format.pitch = input_format.width * 4;
		break;
	case CELL_REC_PARAM_VIDEO_INPUT_RGBA_4_3:
	case CELL_REC_PARAM_VIDEO_INPUT_RGBA_16_9:
		input_format.av_pixel_format = AVPixelFormat::AV_PIX_FMT_RGBA;
		input_format.pitch = input_format.width * 4;
		break;
	case CELL_REC_PARAM_VIDEO_INPUT_YUV420PLANAR_16_9:
		input_format.av_pixel_format = AVPixelFormat::AV_PIX_FMT_YUV420P;
		input_format.pitch = input_format.width * 4; // TODO
		break;
	case CELL_REC_PARAM_VIDEO_INPUT_DISABLE:
	default:
		input_format.av_pixel_format = AVPixelFormat::AV_PIX_FMT_RGBA;
		input_format.width = 1280;
		input_format.height = 720;
		input_format.pitch = input_format.width * 4; // unused
		break;
	}

	cellRec.notice("set_video_params: video_format=0x%x, video_type=0x%x, video_quality=0x%x, video_bps=%d, video_codec_id=%d, wide=%d, hd=%d, input_format=%s, output_format=%s", video_format, video_type, video_quality, video_bps, video_codec_id, wide, hd, input_format.to_string(), output_format.to_string());
}

void rec_info::set_audio_params(s32 audio_format)
{
	// Get the codec
	switch (audio_format)
	{
	case CELL_REC_PARAM_AUDIO_FMT_AAC_96K:
	case CELL_REC_PARAM_AUDIO_FMT_AAC_128K:
	case CELL_REC_PARAM_AUDIO_FMT_AAC_64K:
		audio_codec_id = 86018; // AV_CODEC_ID_AAC
		break;
	case CELL_REC_PARAM_AUDIO_FMT_ULAW_384K:
	case CELL_REC_PARAM_AUDIO_FMT_ULAW_768K:
		// TODO: no idea what this is
		audio_codec_id = 65542; // AV_CODEC_ID_PCM_MULAW
		break;
	case CELL_REC_PARAM_AUDIO_FMT_PCM_384K:
	case CELL_REC_PARAM_AUDIO_FMT_PCM_768K:
	case CELL_REC_PARAM_AUDIO_FMT_PCM_1536K:
		audio_codec_id = 65556; // AV_CODEC_ID_PCM_F32BE
		//audio_codec_id = 65557; // AV_CODEC_ID_PCM_F32LE // TODO: maybe this one?
		break;
	default:
		audio_codec_id = 86018; // AV_CODEC_ID_AAC
		break;
	}

	// Get the sampling rate
	switch (audio_format)
	{
	case CELL_REC_PARAM_AUDIO_FMT_AAC_96K:
	case CELL_REC_PARAM_AUDIO_FMT_AAC_128K:
	case CELL_REC_PARAM_AUDIO_FMT_AAC_64K:
	case CELL_REC_PARAM_AUDIO_FMT_PCM_1536K:
		sample_rate = 48000;
		break;
	case CELL_REC_PARAM_AUDIO_FMT_ULAW_384K:
	case CELL_REC_PARAM_AUDIO_FMT_ULAW_768K:
		// TODO: no idea what this is
		sample_rate = 48000;
		break;
	case CELL_REC_PARAM_AUDIO_FMT_PCM_384K:
		sample_rate = 12000;
		break;
	case CELL_REC_PARAM_AUDIO_FMT_PCM_768K:
		sample_rate = 24000;
		break;
	default:
		sample_rate = 48000;
		break;
	}

	// Get the bitrate
	switch (audio_format)
	{
	case CELL_REC_PARAM_AUDIO_FMT_AAC_64K:
		audio_bps = 64000;
		break;
	case CELL_REC_PARAM_AUDIO_FMT_AAC_96K:
		audio_bps = 96000;
		break;
	case CELL_REC_PARAM_AUDIO_FMT_AAC_128K:
		audio_bps = 128000;
		break;
	case CELL_REC_PARAM_AUDIO_FMT_PCM_1536K:
		audio_bps = 1536000;
		break;
	case CELL_REC_PARAM_AUDIO_FMT_ULAW_384K:
	case CELL_REC_PARAM_AUDIO_FMT_PCM_384K:
		audio_bps = 384000;
		break;
	case CELL_REC_PARAM_AUDIO_FMT_ULAW_768K:
	case CELL_REC_PARAM_AUDIO_FMT_PCM_768K:
		audio_bps = 768000;
		break;
	default:
		audio_bps = 96000;
		break;
	}

	cellRec.notice("set_audio_params: audio_format=0x%x, audio_codec_id=%d, sample_rate=%d, audio_bps=%d", audio_format, audio_codec_id, sample_rate, audio_bps);
}

void rec_info::start_video_provider()
{
	const bool was_paused = paused.exchange(false);

	if (video_provider_thread && was_paused)
	{
		// Resume
		const u64 pause_time_end = get_system_time();
		ensure(pause_time_end > pause_time_start);
		pause_time_total += (pause_time_end - pause_time_start);

		if (param.use_internal_video() || param.use_internal_audio())
		{
			utils::video_provider& video_provider = g_fxo->get<utils::video_provider>();
			video_provider.set_pause_time_us(pause_time_total);
		}

		cellRec.notice("Resuming video provider.");
		return;
	}

	cellRec.notice("Starting video provider.");

	recording_time_start = get_system_time();
	pause_time_start = 0;
	pause_time_total = 0;

	if (param.use_internal_video() || param.use_internal_audio())
	{
		utils::video_provider& video_provider = g_fxo->get<utils::video_provider>();
		video_provider.set_pause_time_us(0);
	}

	video_provider_thread = std::make_unique<named_thread<std::function<void()>>>("cellRec video provider", [this]()
	{
		const bool use_internal_audio = param.use_internal_audio();
		const bool use_external_audio = param.use_external_audio();
		const bool use_external_video = !param.use_internal_video();
		const bool use_ring_buffer = param.ring_sec > 0;
		const usz frame_size = input_format.pitch * input_format.height;
		audio_block buffer_external{}; // for cellRec input
		audio_block buffer_internal{}; // for cellAudio input
		s64 last_video_pts = -1;
		s64 last_audio_pts = -1;

		cellRec.notice("video_provider_thread: use_ring_buffer=%d, video_ringbuffer_size=%d, audio_ringbuffer_size=%d, ring_sec=%d, frame_size=%d, use_internal_video=%d, use_external_audio=%d, use_internal_audio=%d", use_ring_buffer, video_ringbuffer.size(), audio_ringbuffer.size(), param.ring_sec, frame_size, encoder->use_internal_video, use_external_audio, encoder->use_internal_audio);

		while (thread_ctrl::state() != thread_state::aborting && encoder)
		{
			if (encoder->has_error)
			{
				cellRec.error("Encoder error. Sending error status.");

				sysutil_register_cb([this](ppu_thread& ppu) -> s32
				{
					if (cb)
					{
						cb(ppu, CELL_REC_STATUS_ERR, CELL_REC_ERROR_FILE_WRITE, cbUserData);
					}
					return CELL_OK;
				});

				return;
			}

			if (paused)
			{
				thread_ctrl::wait_for(10000);
				continue;
			}

			// We only care for new video frames or audio samples that can be properly encoded, so we check the timestamps and pts.
			const usz timestamp_ms = (get_system_time() - recording_time_start - pause_time_total) / 1000;

			/////////////////
			//    VIDEO    //
			/////////////////

			// TODO: wait for flip before adding a frame
			if (use_external_video)
			{
				// The video frames originate from cellRec instead of our render pipeline.
				if (const s64 pts = encoder->get_pts(timestamp_ms); pts > last_video_pts)
				{
					if (video_input_buffer)
					{
						if (use_ring_buffer)
						{
							// The video frames originate from cellRec and are stored in a ringbuffer.
							utils::video_sink::encoder_frame& frame_data = video_ringbuffer[next_video_ring_pos()];
							frame_data.pts = pts;
							frame_data.width = input_format.width;
							frame_data.height = input_format.height;
							frame_data.av_pixel_format = input_format.av_pixel_format;
							frame_data.data.resize(frame_size);
							std::memcpy(frame_data.data.data(), video_input_buffer.get_ptr(), frame_data.data.size());
							video_ring_frame_count++;
						}
						else
						{
							// The video frames originate from cellRec and are pushed to the encoder immediately.
							std::vector<u8> frame(frame_size);
							std::memcpy(frame.data(), video_input_buffer.get_ptr(), frame.size());
							encoder->add_frame(frame, input_format.pitch, input_format.width, input_format.height, input_format.av_pixel_format, timestamp_ms);
						}
					}

					last_video_pts = pts;
				}
			}
			else if (sink)
			{
				// The video frames originate from our render pipeline.
				utils::video_sink::encoder_frame frame = sink->get_frame();

				if (const s64 pts = encoder->get_pts(frame.timestamp_ms); pts > last_video_pts && !frame.data.empty())
				{
					if (use_ring_buffer)
					{
						// The video frames originate from our render pipeline and are stored in a ringbuffer.
						frame.pts = pts;
						video_ringbuffer[next_video_ring_pos()] = std::move(frame);
						video_ring_frame_count++;
					}
					else
					{
						// The video frames originate from our render pipeline and are directly encoded by the encoder.
						encoder->add_frame(frame.data, frame.pitch, frame.width, frame.height, frame.av_pixel_format, frame.timestamp_ms);
					}

					last_video_pts = pts;
				}
			}

			/////////////////
			//    AUDIO    //
			/////////////////

			const usz timestamp_us = get_system_time() - recording_time_start - pause_time_total;
			bool got_new_samples = false;

			if (use_external_audio)
			{
				if (const s64 pts = encoder->get_audio_pts(timestamp_us); pts > last_audio_pts)
				{
					if (audio_input_buffer)
					{
						// The audio samples originate from cellRec instead of our render pipeline.
						// TODO: This needs to be synchronized with the game somehow if possible.
						std::memcpy(buffer_external.block.data(), audio_input_buffer.get_ptr(), buffer_external.block.size());
						buffer_external.pts = pts;
						got_new_samples = true;
					}

					last_audio_pts = pts;
				}
			}

			if (sink && use_internal_audio)
			{
				// The audio samples originate from cellAudio and are stored in a ringbuffer.
				utils::video_sink::encoder_sample sample = sink->get_sample();

				if (!sample.data.empty() && sample.channels >= channels && sample.sample_count >= CELL_REC_AUDIO_BLOCK_SAMPLES)
				{
					s64 pts = encoder->get_audio_pts(sample.timestamp_us);

					// Each encoder_sample can have more than one block
					for (u32 i = 0; i < sample.sample_count; i += CELL_REC_AUDIO_BLOCK_SAMPLES)
					{
						if (pts > last_audio_pts)
						{
							const f32* src = reinterpret_cast<const f32*>(&sample.data[i * sample.channels * sizeof(f32)]);

							// Copy the new samples to the internal buffer if we need them for volume mixing below.
							// Otherwise copy them directly to the external buffer which is used for output later.
							audio_block& dst_buffer = got_new_samples ? buffer_internal : buffer_external;

							if (sample.channels > channels)
							{
								// Downmix channels
								AudioBackend::downmix(CELL_REC_AUDIO_BLOCK_SAMPLES * sample.channels, sample.channels, audio_channel_layout::stereo, src, reinterpret_cast<f32*>(dst_buffer.block.data()));
							}
							else
							{
								std::memcpy(dst_buffer.block.data(), src, audio_block::block_size);
							}

							// Mix external and internal audio with param.audio_input_mix_vol if we already got samples from cellRec.
							if (got_new_samples)
							{
								const float volume = std::clamp(param.audio_input_mix_vol / 100.0f, 0.0f, 1.0f);
								const f32* src = reinterpret_cast<const f32*>(buffer_internal.block.data());
								f32* dst = reinterpret_cast<f32*>(buffer_external.block.data());

								for (u32 sample = 0; sample < (CELL_REC_AUDIO_BLOCK_SAMPLES * channels); sample++)
								{
									*dst = std::clamp(*dst + (*src++ * volume), -1.0f, 1.0f);
									++dst;
								}
							}

							last_audio_pts = std::max(pts, last_audio_pts); // The cellAudio pts may be older than the pts from cellRec
							buffer_external.pts = last_audio_pts;
							got_new_samples = true;
						}

						// We only take the first sample for simplicity for now
						break;

						// Increase pts for each sample block
						//pts++;
					}
				}
			}

			if (got_new_samples)
			{
				if (use_ring_buffer)
				{
					// Copy new sample to ringbuffer
					audio_ringbuffer[next_audio_ring_pos()] = buffer_external;
					audio_ring_block_count++;
				}
				else
				{
					// Push new sample to encoder
					encoder->add_audio_samples(buffer_external.block.data(), CELL_REC_AUDIO_BLOCK_SAMPLES, channels, timestamp_us);
				}
			}

			// Update recording time
			recording_time_total = encoder->get_timestamp_ms(encoder->last_video_pts());

			thread_ctrl::wait_for(1);
		}
	});
}

void rec_info::pause_video_provider()
{
	cellRec.notice("Pausing video provider.");

	if (video_provider_thread)
	{
		paused = true;
		pause_time_start = get_system_time();
	}
}

void rec_info::stop_video_provider(bool flush)
{
	cellRec.notice("Stopping video provider.");

	// Join thread
	video_provider_thread.reset();

	// Flush the ringbuffer if necessary.
	// This should only happen if the video sink is not the encoder itself.
	// In this case the encoder should have been idle until now.
	if (flush && param.ring_sec > 0 && (!video_ringbuffer.empty() || !audio_ringbuffer.empty()))
	{
		cellRec.notice("Flushing video ringbuffer.");

		// Fill encoder with data from ringbuffer
		// TODO: ideally the encoder should do this on the fly and overwrite old frames in the file.
		ensure(encoder);
		encoder->encode();

		const usz frame_count = std::min(video_ringbuffer.size(), video_ring_frame_count);
		const usz video_start_offset = video_ring_frame_count < video_ringbuffer.size() ? 0 : video_ring_frame_count;
		const s64 video_start_pts = video_ringbuffer.empty() ? 0 : video_ringbuffer[video_start_offset % video_ringbuffer.size()].pts;

		const usz block_count = std::min(audio_ringbuffer.size(), audio_ring_block_count);
		const usz audio_start_offset = audio_ring_block_count < audio_ringbuffer.size() ? 0 : audio_ring_block_count;
		const s64 audio_start_pts = audio_ringbuffer.empty() ? 0 : audio_ringbuffer[audio_start_offset % audio_ringbuffer.size()].pts;

		// Try to add the frames and samples in proper order
		for (usz sync_timestamp_us = 0, frame = 0, block = 0; frame < frame_count || block < block_count; frame++)
		{
			// Add one frame
			if (frame < frame_count)
			{
				const usz pos = (video_start_offset + frame) % video_ringbuffer.size();
				utils::video_sink::encoder_frame& frame_data = video_ringbuffer[pos];
				const usz timestamp_ms = encoder->get_timestamp_ms(frame_data.pts - video_start_pts);
				encoder->add_frame(frame_data.data, frame_data.pitch, frame_data.width, frame_data.height, frame_data.av_pixel_format, timestamp_ms);

				// Increase sync timestamp
				sync_timestamp_us = timestamp_ms * 1000;
			}

			// Add all the samples that fit into the last frame
			for (usz i = block; i < block_count; i++)
			{
				const usz pos = (audio_start_offset + i) % audio_ringbuffer.size();
				const audio_block& sample_block = audio_ringbuffer[pos];
				const usz timestamp_us = encoder->get_audio_timestamp_us(sample_block.pts - audio_start_pts);

				// Stop adding new samples if the sync timestamp is exceeded, unless we already added all the frames.
				if (timestamp_us > sync_timestamp_us && frame < frame_count)
				{
					break;
				}

				encoder->add_audio_samples(sample_block.block.data(), CELL_REC_AUDIO_BLOCK_SAMPLES, channels, timestamp_us);
				block++;
			}
		}

		video_ringbuffer.clear();
		audio_ringbuffer.clear();
	}
}

bool create_path(std::string& out, std::string dir_name, const std::string& file_name)
{
	out.clear();

	if (dir_name.size() + file_name.size() > CELL_REC_MAX_PATH_LEN)
	{
		return false;
	}

	out = std::move(dir_name);

	if (!out.empty() && out.back() != '/')
	{
		out += '/';
	}

	out += file_name;

	return true;
}

u32 cellRecQueryMemSize(vm::cptr<CellRecParam> pParam); // Forward declaration

error_code cellRecOpen(vm::cptr<char> pDirName, vm::cptr<char> pFileName, vm::cptr<CellRecParam> pParam, u32 container, vm::ptr<CellRecCallback> cb, vm::ptr<void> cbUserData)
{
	cellRec.todo("cellRecOpen(pDirName=%s, pFileName=%s, pParam=*0x%x, container=0x%x, cb=*0x%x, cbUserData=*0x%x)", pDirName, pFileName, pParam, container, cb, cbUserData);

	auto& rec = g_fxo->get<rec_info>();

	if (rec.state != rec_state::closed)
	{
		return CELL_REC_ERROR_INVALID_STATE;
	}

	if (!pParam || !pDirName || !pFileName)
	{
		return CELL_REC_ERROR_INVALID_VALUE;
	}

	std::string options;

	for (s32 i = 0; i < pParam->numOfOpt; i++)
	{
		if (i > 0) options += ", ";
		fmt::append(options, "%d", pParam->pOpt[i].option);
	}

	cellRec.notice("cellRecOpen: pParam={ videoFmt=0x%x, audioFmt=0x%x, numOfOpt=0x%x, options=[ %s ] }", pParam->videoFmt, pParam->audioFmt, pParam->numOfOpt, options);

	const u32 mem_size = cellRecQueryMemSize(pParam);

	if (container == SYS_MEMORY_CONTAINER_ID_INVALID)
	{
		if (mem_size != 0)
		{
			return CELL_REC_ERROR_INVALID_VALUE;
		}
	}
	else
	{
		// NOTE: Most likely tries to allocate a complete container. But we use g_fxo instead.
		// TODO: how much data do we actually need ?
		rec.video_input_buffer = vm::cast(vm::alloc(mem_size, vm::main));
		rec.audio_input_buffer = vm::cast(vm::alloc(mem_size, vm::main));

		if (!rec.video_input_buffer || !rec.audio_input_buffer)
		{
			return CELL_REC_ERROR_OUT_OF_MEMORY;
		}
	}

	switch (pParam->videoFmt)
	{
	case CELL_REC_PARAM_VIDEO_FMT_MPEG4_SMALL_512K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_MPEG4_SMALL_768K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_MPEG4_MIDDLE_512K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_MPEG4_MIDDLE_768K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_512K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_768K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_1024K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_1536K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_MPEG4_LARGE_2048K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_AVC_MP_SMALL_512K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_AVC_MP_SMALL_768K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_AVC_MP_MIDDLE_512K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_AVC_MP_MIDDLE_768K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_AVC_MP_MIDDLE_1024K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_AVC_MP_MIDDLE_1536K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_AVC_BL_SMALL_512K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_AVC_BL_SMALL_768K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_AVC_BL_MIDDLE_512K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_AVC_BL_MIDDLE_768K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_AVC_BL_MIDDLE_1024K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_AVC_BL_MIDDLE_1536K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_MJPEG_SMALL_5000K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_MJPEG_MIDDLE_5000K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_MJPEG_LARGE_11000K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_MJPEG_HD720_11000K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_MJPEG_HD720_20000K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_MJPEG_HD720_25000K_30FPS:
	case 0x3791:
	case 0x37a1:
	case 0x3790:
	case 0x37a0:
	case CELL_REC_PARAM_VIDEO_FMT_M4HD_SMALL_768K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_M4HD_MIDDLE_768K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_M4HD_LARGE_1536K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_M4HD_LARGE_2048K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_M4HD_HD720_2048K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_M4HD_HD720_5000K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_M4HD_HD720_11000K_30FPS:
	case CELL_REC_PARAM_VIDEO_FMT_YOUTUBE:
		break;
	default:
		return CELL_REC_ERROR_INVALID_VALUE;
	}

	const s32 video_type = pParam->videoFmt & 0xf000;
	const s32 video_quality = pParam->videoFmt & 0xf00;

	switch (pParam->audioFmt)
	{
	case CELL_REC_PARAM_AUDIO_FMT_AAC_96K:
	case CELL_REC_PARAM_AUDIO_FMT_AAC_128K:
	case CELL_REC_PARAM_AUDIO_FMT_AAC_64K:
	{
		// Do not allow MJPEG or AVC_MP video format
		if (video_type == VIDEO_TYPE_AVC_MP || video_type == VIDEO_TYPE_MJPEG)
		{
			return CELL_REC_ERROR_INVALID_VALUE;
		}
		[[fallthrough]];
	}
	case CELL_REC_PARAM_AUDIO_FMT_ULAW_384K:
	case CELL_REC_PARAM_AUDIO_FMT_ULAW_768K:
	{
		// Do not allow AVC_MP video format
		if (video_type == VIDEO_TYPE_AVC_MP)
		{
			return CELL_REC_ERROR_INVALID_VALUE;
		}
		break;
	}
	case CELL_REC_PARAM_AUDIO_FMT_PCM_384K:
	case CELL_REC_PARAM_AUDIO_FMT_PCM_768K:
	case CELL_REC_PARAM_AUDIO_FMT_PCM_1536K:
	{
		// Only allow MJPEG video format
		if (video_type != VIDEO_TYPE_MJPEG)
		{
			return CELL_REC_ERROR_INVALID_VALUE;
		}
		break;
	}
	default:
		return CELL_REC_ERROR_INVALID_VALUE;
	}

	rec.param = {};

	u32 spurs_param = 0;
	bool abort_loop = false;

	for (s32 i = 0; i < pParam->numOfOpt; i++)
	{
		const CellRecOption& opt = pParam->pOpt[i];

		switch (opt.option)
		{
		case CELL_REC_OPTION_PPU_THREAD_PRIORITY:
		{
			if (opt.value.ppu_thread_priority > 0xbff)
			{
				return CELL_REC_ERROR_INVALID_VALUE;
			}
			rec.param.ppu_thread_priority = opt.value.ppu_thread_priority;
			break;
		}
		case CELL_REC_OPTION_SPU_THREAD_PRIORITY:
		{
			if (opt.value.spu_thread_priority - 0x10U > 0xef)
			{
				return CELL_REC_ERROR_INVALID_VALUE;
			}
			rec.param.spu_thread_priority = opt.value.spu_thread_priority;
			break;
		}
		case CELL_REC_OPTION_CAPTURE_PRIORITY:
		{
			rec.param.capture_priority = opt.value.capture_priority;
			break;
		}
		case CELL_REC_OPTION_USE_SYSTEM_SPU:
		{
			if (video_quality == VIDEO_QUALITY_6 || video_quality == VIDEO_QUALITY_7)
			{
				// TODO: Seems differ if video_quality is VIDEO_QUALITY_6 or VIDEO_QUALITY_7
			}
			rec.param.use_system_spu = opt.value.use_system_spu;
			break;
		}
		case CELL_REC_OPTION_FIT_TO_YOUTUBE:
		{
			rec.param.fit_to_youtube = opt.value.fit_to_youtube;
			break;
		}
		case CELL_REC_OPTION_XMB_BGM:
		{
			rec.param.xmb_bgm = opt.value.xmb_bgm;
			break;
		}
		case CELL_REC_OPTION_RING_SEC:
		{
			rec.param.ring_sec = opt.value.ring_sec;
			break;
		}
		case CELL_REC_OPTION_MPEG4_FAST_ENCODE:
		{
			rec.param.mpeg4_fast_encode = opt.value.mpeg4_fast_encode;
			break;
		}
		case CELL_REC_OPTION_VIDEO_INPUT:
		{
			const u32 v_input = (opt.value.video_input & 0xffU);
			if (v_input > CELL_REC_PARAM_VIDEO_INPUT_YUV420PLANAR_16_9)
			{
				return CELL_REC_ERROR_INVALID_VALUE;
			}
			rec.param.video_input = v_input;
			break;
		}
		case CELL_REC_OPTION_AUDIO_INPUT:
		{
			rec.param.audio_input = opt.value.audio_input;

			if (opt.value.audio_input == CELL_REC_PARAM_AUDIO_INPUT_DISABLE)
			{
				rec.param.audio_input_mix_vol = CELL_REC_PARAM_AUDIO_INPUT_MIX_VOL_MIN;
			}
			else
			{
				rec.param.audio_input_mix_vol = CELL_REC_PARAM_AUDIO_INPUT_MIX_VOL_MAX;
			}
			break;
		}
		case CELL_REC_OPTION_AUDIO_INPUT_MIX_VOL:
		{
			rec.param.audio_input_mix_vol = opt.value.audio_input_mix_vol;
			break;
		}
		case CELL_REC_OPTION_REDUCE_MEMSIZE:
		{
			rec.param.reduce_memsize = opt.value.reduce_memsize != CELL_REC_PARAM_REDUCE_MEMSIZE_DISABLE;
			break;
		}
		case CELL_REC_OPTION_SHOW_XMB:
		{
			rec.param.show_xmb = (opt.value.show_xmb != 0);
			break;
		}
		case CELL_REC_OPTION_METADATA_FILENAME:
		{
			if (opt.value.metadata_filename)
			{
				std::string path;
				if (!create_path(path, pDirName.get_ptr(), opt.value.metadata_filename.get_ptr()))
				{
					return CELL_REC_ERROR_INVALID_VALUE;
				}
				rec.param.metadata_filename = path;
			}
			else
			{
				abort_loop = true; // TODO: Apparently this doesn't return an error but goes to the callback section instead...
			}
			break;
		}
		case CELL_REC_OPTION_SPURS:
		{
			spurs_param = opt.value.pSpursParam.addr();

			if (!opt.value.pSpursParam || !opt.value.pSpursParam->pSpurs) // TODO: check both or only pSpursParam ?
			{
				return CELL_REC_ERROR_INVALID_VALUE;
			}

			if (opt.value.pSpursParam->spu_usage_rate < 1 || opt.value.pSpursParam->spu_usage_rate > 100)
			{
				return CELL_REC_ERROR_INVALID_VALUE;
			}

			rec.param.spurs_param.spu_usage_rate = opt.value.pSpursParam->spu_usage_rate;
			[[fallthrough]];
		}
		case 100:
		{
			if (spurs_param)
			{
				// TODO:
			}
			break;
		}
		default:
		{
			cellRec.warning("cellRecOpen: unknown option %d", opt.option);
			break;
		}
		}

		if (abort_loop)
		{
			break;
		}
	}

	// NOTE: The abort_loop variable I chose is actually a goto to the callback section at the end of the function.
	if (!abort_loop)
	{
		if (rec.param.video_input != CELL_REC_PARAM_VIDEO_INPUT_DISABLE)
		{
			if (video_type == VIDEO_TYPE_MJPEG || video_type == VIDEO_TYPE_M4HD)
			{
				if (rec.param.video_input != CELL_REC_PARAM_VIDEO_INPUT_YUV420PLANAR_16_9)
				{
					return CELL_REC_ERROR_INVALID_VALUE;
				}
			}
			else if (rec.param.video_input == CELL_REC_PARAM_VIDEO_INPUT_YUV420PLANAR_16_9)
			{
				return CELL_REC_ERROR_INVALID_VALUE;
			}
		}

		if (!create_path(rec.param.filename, pDirName.get_ptr(), pFileName.get_ptr()))
		{
			return CELL_REC_ERROR_INVALID_VALUE;
		}

		if (!rec.param.filename.starts_with("/dev_hdd0") && !rec.param.filename.starts_with("/dev_hdd1"))
		{
			return CELL_REC_ERROR_INVALID_VALUE;
		}

		//if (spurs_param)
		//{
		//	if (error_code error = cellSpurs_0xD9A9C592();
		//	{
		//		return error;
		//	}
		//}
	}

	if (!cb)
	{
		return CELL_REC_ERROR_INVALID_VALUE;
	}

	cellRec.notice("cellRecOpen: Using parameters: %s", rec.param.to_string());

	rec.cb = cb;
	rec.cbUserData = cbUserData;
	rec.video_ringbuffer.clear();
	rec.audio_ringbuffer.clear();
	rec.video_ring_pos = 0;
	rec.audio_ring_pos = 0;
	rec.video_ring_frame_count = 0;
	rec.audio_ring_block_count = 0;
	rec.recording_time_start = 0;
	rec.recording_time_total = 0;
	rec.pause_time_start = 0;
	rec.pause_time_total = 0;
	rec.paused = false;

	rec.set_video_params(pParam->videoFmt);
	rec.set_audio_params(pParam->audioFmt);

	if (rec.param.ring_sec > 0)
	{
		const usz audio_ring_buffer_size = static_cast<usz>(std::ceil((rec.param.ring_sec * rec.sample_rate) / static_cast<f32>(CELL_REC_AUDIO_BLOCK_SAMPLES)));
		const usz video_ring_buffer_size = rec.param.ring_sec * rec.fps;

		cellRec.notice("Preparing ringbuffer for %d seconds. video_ring_buffer_size=%d, audio_ring_buffer_size=%d, pitch=%d, width=%d, height=%d", rec.param.ring_sec, video_ring_buffer_size, audio_ring_buffer_size, rec.input_format.pitch, rec.input_format.width, rec.input_format.height);

		rec.audio_ringbuffer.resize(audio_ring_buffer_size);
		rec.video_ringbuffer.resize(video_ring_buffer_size);
	}

	if (rec.param.use_internal_audio() || rec.param.use_internal_video())
	{
		rec.sink = std::make_shared<rec_video_sink>();
		rec.sink->use_internal_audio = rec.param.use_internal_audio();
		rec.sink->use_internal_video = rec.param.use_internal_video();
		rec.sink->set_sample_rate(rec.sample_rate);
	}

	rec.encoder = std::make_shared<utils::video_encoder>();
	rec.encoder->use_internal_audio = false; // We use the other sink
	rec.encoder->use_internal_video = false; // We use the other sink
	rec.encoder->set_path(vfs::get(rec.param.filename));
	rec.encoder->set_framerate(rec.fps);
	rec.encoder->set_video_bitrate(rec.video_bps);
	rec.encoder->set_video_codec(rec.video_codec_id);
	rec.encoder->set_sample_rate(rec.sample_rate);
	rec.encoder->set_audio_channels(rec.channels);
	rec.encoder->set_audio_bitrate(rec.audio_bps);
	rec.encoder->set_audio_codec(rec.audio_codec_id);
	rec.encoder->set_output_format(rec.output_format);

	sysutil_register_cb([&rec](ppu_thread& ppu) -> s32
	{
		rec.state = rec_state::stopped;
		rec.cb(ppu, CELL_REC_STATUS_OPEN, CELL_OK, rec.cbUserData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellRecClose(s32 isDiscard)
{
	cellRec.todo("cellRecClose(isDiscard=0x%x)", isDiscard);

	auto& rec = g_fxo->get<rec_info>();

	if (rec.state == rec_state::closed)
	{
		return CELL_REC_ERROR_INVALID_STATE;
	}

	sysutil_register_cb([=, &rec](ppu_thread& ppu) -> s32
	{
		bool is_valid_range = true;

		if (isDiscard)
		{
			// No need to flush the encoder
			rec.stop_video_provider(false);
			rec.encoder->stop(false);

			if (rec.sink)
			{
				rec.sink->stop(true);
			}

			if (fs::is_file(rec.param.filename))
			{
				cellRec.warning("cellRecClose: removing discarded recording '%s'", rec.param.filename);

				if (!fs::remove_file(rec.param.filename))
				{
					cellRec.error("cellRecClose: failed to remove recording '%s'", rec.param.filename);
				}
			}
		}
		else
		{
			// Flush to make sure we encode all remaining frames
			rec.stop_video_provider(true);
			rec.encoder->stop(true);
			rec.recording_time_total = rec.encoder->get_timestamp_ms(rec.encoder->last_video_pts());

			if (rec.sink)
			{
				rec.sink->stop(true);
			}

			const s64 start_pts = rec.encoder->get_pts(rec.param.scene_metadata.start_time);
			const s64 end_pts = rec.encoder->get_pts(rec.param.scene_metadata.end_time);
			const s64 last_pts = rec.encoder->last_video_pts();

			is_valid_range = start_pts >= 0 && end_pts <= last_pts;
		}

		vm::dealloc(rec.video_input_buffer.addr(), vm::main);
		vm::dealloc(rec.audio_input_buffer.addr(), vm::main);

		// Release the video sink if it was used
		if (rec.param.use_internal_video() || rec.param.use_internal_audio())
		{
			const recording_mode old_mode = g_recording_mode.exchange(recording_mode::stopped);

			if (old_mode == recording_mode::rpcs3)
			{
				cellRec.error("cellRecClose: Unexpected recording mode %s found while stopping video capture.", old_mode);
			}

			utils::video_provider& video_provider = g_fxo->get<utils::video_provider>();

			if (!video_provider.set_video_sink(nullptr, recording_mode::cell))
			{
				cellRec.error("cellRecClose failed to release video sink");
			}
		}

		rec.param = {};
		rec.encoder.reset();
		rec.sink.reset();
		rec.audio_ringbuffer.clear();
		rec.video_ringbuffer.clear();
		rec.state = rec_state::closed;

		if (is_valid_range)
		{
			rec.cb(ppu, CELL_REC_STATUS_CLOSE, CELL_OK, rec.cbUserData);
		}
		else
		{
			rec.cb(ppu, CELL_REC_STATUS_ERR, CELL_REC_ERROR_FILE_NO_DATA, rec.cbUserData);
		}
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellRecStop()
{
	cellRec.todo("cellRecStop()");

	auto& rec = g_fxo->get<rec_info>();

	if (rec.state != rec_state::started)
	{
		return CELL_REC_ERROR_INVALID_STATE;
	}

	sysutil_register_cb([&rec](ppu_thread& ppu) -> s32
	{
		// cellRecStop actually just pauses the recording
		rec.pause_video_provider();

		if (rec.sink)
		{
			rec.sink->pause(true);
		}

		ensure(!!rec.encoder);
		rec.encoder->pause(true);

		rec.recording_time_total = rec.encoder->get_timestamp_ms(rec.encoder->last_video_pts());
		rec.state = rec_state::stopped;

		rec.cb(ppu, CELL_REC_STATUS_STOP, CELL_OK, rec.cbUserData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellRecStart()
{
	cellRec.todo("cellRecStart()");

	auto& rec = g_fxo->get<rec_info>();

	if (rec.state != rec_state::stopped)
	{
		return CELL_REC_ERROR_INVALID_STATE;
	}

	sysutil_register_cb([&rec](ppu_thread& ppu) -> s32
	{
		// Start/resume the recording
		ensure(!!rec.encoder);

		if (rec.param.ring_sec == 0)
		{
			rec.encoder->encode();
		}

		// Setup a video sink if it is needed
		if (rec.param.use_internal_video() || rec.param.use_internal_audio())
		{
			utils::video_provider& video_provider = g_fxo->get<utils::video_provider>();

			if (rec.sink && !video_provider.set_video_sink(rec.sink, recording_mode::cell))
			{
				cellRec.error("Failed to set video sink");
				rec.cb(ppu, CELL_REC_STATUS_ERR, CELL_REC_ERROR_FATAL, rec.cbUserData);
				return CELL_OK;
			}

			// Force rsx recording
			g_recording_mode = recording_mode::cell;
		}
		else
		{
			// Force stop rsx recording
			g_recording_mode = recording_mode::stopped;
		}

		rec.start_video_provider();

		if (rec.sink)
		{
			rec.sink->resume();
		}

		if (rec.encoder->has_error)
		{
			rec.cb(ppu, CELL_REC_STATUS_ERR, CELL_REC_ERROR_FILE_OPEN, rec.cbUserData);
		}
		else
		{
			rec.state = rec_state::started;
			rec.cb(ppu, CELL_REC_STATUS_START, CELL_OK, rec.cbUserData);
		}
		return CELL_OK;
	});

	return CELL_OK;
}

u32 cellRecQueryMemSize(vm::cptr<CellRecParam> pParam)
{
	cellRec.notice("cellRecQueryMemSize(pParam=*0x%x)", pParam);

	if (!pParam)
	{
		return 0x900000;
	}

	u32 video_size = 0x600000; // 6 MB
	u32 audio_size = 0x100000; // 1 MB
	u32 external_input_size = 0;
	u32 reduce_memsize = 0;

	s32 video_type = pParam->videoFmt & 0xf000;
	s32 video_quality = pParam->videoFmt & 0xf00;

	switch (video_type)
	{
	case VIDEO_TYPE_MPEG4:
	{
		switch (video_quality)
		{
		case VIDEO_QUALITY_0:
		case VIDEO_QUALITY_3:
			video_size = 0x600000; // 6 MB
			break;
		case VIDEO_QUALITY_1:
			video_size = 0x700000; // 7 MB
			break;
		case VIDEO_QUALITY_2:
			video_size = 0x800000; // 8 MB
			break;
		default:
			video_size = 0x900000; // 9 MB
			break;
		}

		audio_size = 0x100000; // 1 MB
		break;
	}
	case VIDEO_TYPE_AVC_BL:
	case VIDEO_TYPE_AVC_MP:
	{
		switch (video_quality)
		{
		case VIDEO_QUALITY_0:
		case VIDEO_QUALITY_3:
			video_size = 0x800000; // 8 MB
			break;
		default:
			video_size = 0x900000; // 9 MB
			break;
		}

		audio_size = 0x100000; // 1 MB
		break;
	}
	case VIDEO_TYPE_M4HD:
	{
		switch (video_quality)
		{
		case VIDEO_QUALITY_0:
		case VIDEO_QUALITY_1:
			video_size = 0x600000; // 6 MB
			break;
		case VIDEO_QUALITY_2:
			video_size = 0x700000; // 7 MB
			break;
		default:
			video_size = 0x1000000; // 16 MB
			break;
		}

		audio_size = 0x100000; // 1 MB
		break;
	}
	default:
	{
		switch (video_quality)
		{
		case VIDEO_QUALITY_0:
			video_size = 0x300000; // 3 MB
			break;
		case VIDEO_QUALITY_1:
		case VIDEO_QUALITY_2:
			video_size = 0x400000; // 4 MB
			break;
		case VIDEO_QUALITY_6:
			video_size = 0x700000; // 7 MB
			break;
		default:
			video_size = 0xd00000; // 14 MB
			break;
		}

		audio_size = 0; // 0 MB
		break;
	}
	}

	for (s32 i = 0; i < pParam->numOfOpt; i++)
	{
		const CellRecOption& opt = pParam->pOpt[i];

		switch (opt.option)
		{
		case CELL_REC_OPTION_REDUCE_MEMSIZE:
		{
			if (opt.value.reduce_memsize == CELL_REC_PARAM_REDUCE_MEMSIZE_DISABLE)
			{
				reduce_memsize = 0; // 0 MB
			}
			else
			{
				if (video_type == VIDEO_TYPE_M4HD && (video_quality == VIDEO_QUALITY_1 || video_quality == VIDEO_QUALITY_2))
				{
					reduce_memsize = 0x200000; // 2 MB
				}
				else
				{
					reduce_memsize = 0x300000; // 3 MB
				}
			}
			break;
		}
		case CELL_REC_OPTION_VIDEO_INPUT:
		case CELL_REC_OPTION_AUDIO_INPUT:
		{
			if (opt.value.audio_input != CELL_REC_PARAM_AUDIO_INPUT_DISABLE)
			{
				if (video_type == VIDEO_TYPE_MJPEG || (video_type == VIDEO_TYPE_M4HD && video_quality != VIDEO_QUALITY_6))
				{
					external_input_size = 0x100000; // 1MB
				}
			}
			break;
		}
		case CELL_REC_OPTION_AUDIO_INPUT_MIX_VOL:
		{
			// NOTE: Doesn't seem to check opt.value.audio_input
			if (video_type == VIDEO_TYPE_MJPEG || (video_type == VIDEO_TYPE_M4HD && video_quality != VIDEO_QUALITY_6))
			{
				external_input_size = 0x100000; // 1MB
			}
			break;
		}
		default:
			break;
		}
	}

	u32 size = video_size + audio_size + external_input_size;

	if (size > reduce_memsize)
	{
		size -= reduce_memsize;
	}
	else
	{
		size = 0;
	}

	return size;
}

void cellRecGetInfo(s32 info, vm::ptr<u64> pValue)
{
	cellRec.todo("cellRecGetInfo(info=0x%x, pValue=*0x%x)", info, pValue);

	if (!pValue)
	{
		return;
	}

	*pValue = 0;

	auto& rec = g_fxo->get<rec_info>();

	switch (info)
	{
	case CELL_REC_INFO_VIDEO_INPUT_ADDR:
	{
		*pValue = rec.video_input_buffer.addr();
		break;
	}
	case CELL_REC_INFO_VIDEO_INPUT_WIDTH:
	{
		*pValue = rec.input_format.width;
		break;
	}
	case CELL_REC_INFO_VIDEO_INPUT_PITCH:
	{
		*pValue = rec.input_format.pitch;
		break;
	}
	case CELL_REC_INFO_VIDEO_INPUT_HEIGHT:
	{
		*pValue = rec.input_format.height;
		break;
	}
	case CELL_REC_INFO_AUDIO_INPUT_ADDR:
	{
		*pValue = rec.audio_input_buffer.addr();
		break;
	}
	case CELL_REC_INFO_MOVIE_TIME_MSEC:
	{
		if (rec.state == rec_state::stopped)
		{
			*pValue = rec.recording_time_total;
		}
		break;
	}
	case CELL_REC_INFO_SPURS_SYSTEMWORKLOAD_ID:
	{
		if (rec.param.spurs_param.pSpurs)
		{
			// TODO
			//cellSpurs_0xE279681F();
		}
		*pValue = 0xffffffff;
		break;
	}
	default:
	{
		break;
	}
	}
}

error_code cellRecSetInfo(s32 setInfo, u64 value)
{
	cellRec.todo("cellRecSetInfo(setInfo=0x%x, value=0x%x)", setInfo, value);

	auto& rec = g_fxo->get<rec_info>();

	if (rec.state != rec_state::stopped)
	{
		return CELL_REC_ERROR_INVALID_STATE;
	}

	switch (setInfo)
	{
	case CELL_REC_SETINFO_MOVIE_START_TIME_MSEC:
	{
		// TODO: check if this is actually identical to scene metadata
		rec.param.scene_metadata.start_time = value;
		cellRec.notice("cellRecSetInfo: changed movie start time to %d", value);
		break;
	}
	case CELL_REC_SETINFO_MOVIE_END_TIME_MSEC:
	{
		// TODO: check if this is actually identical to scene metadata
		rec.param.scene_metadata.end_time = value;
		cellRec.notice("cellRecSetInfo: changed movie end time to %d", value);
		break;
	}
	case CELL_REC_SETINFO_MOVIE_META:
	{
		if (!value)
		{
			return CELL_REC_ERROR_INVALID_VALUE;
		}

		vm::ptr<CellRecMovieMetadata> movie_metadata = vm::cast(value);

		if (!movie_metadata || !movie_metadata->gameTitle || !movie_metadata->movieTitle ||
			strnlen(movie_metadata->gameTitle.get_ptr(), CELL_REC_MOVIE_META_GAME_TITLE_LEN) >= CELL_REC_MOVIE_META_GAME_TITLE_LEN ||
			strnlen(movie_metadata->movieTitle.get_ptr(), CELL_REC_MOVIE_META_MOVIE_TITLE_LEN) >= CELL_REC_MOVIE_META_MOVIE_TITLE_LEN)
		{
			return CELL_REC_ERROR_INVALID_VALUE;
		}

		if ((movie_metadata->description && strnlen(movie_metadata->description.get_ptr(), CELL_REC_MOVIE_META_DESCRIPTION_LEN) >= CELL_REC_MOVIE_META_DESCRIPTION_LEN) ||
			(movie_metadata->userdata && strnlen(movie_metadata->userdata.get_ptr(), CELL_REC_MOVIE_META_USERDATA_LEN) >= CELL_REC_MOVIE_META_USERDATA_LEN))
		{
			return CELL_REC_ERROR_INVALID_VALUE;
		}

		rec.param.movie_metadata = {};
		rec.param.movie_metadata.game_title  = std::string{movie_metadata->gameTitle.get_ptr()};
		rec.param.movie_metadata.movie_title = std::string{movie_metadata->movieTitle.get_ptr()};

		if (movie_metadata->description)
		{
			rec.param.movie_metadata.description = std::string{movie_metadata->description.get_ptr()};
		}

		if (movie_metadata->userdata)
		{
			rec.param.movie_metadata.userdata = std::string{movie_metadata->userdata.get_ptr()};
		}

		cellRec.notice("cellRecSetInfo: changed movie metadata to %s", rec.param.movie_metadata.to_string());
		break;
	}
	case CELL_REC_SETINFO_SCENE_META:
	{
		if (!value)
		{
			return CELL_REC_ERROR_INVALID_VALUE;
		}

		vm::ptr<CellRecSceneMetadata> scene_metadata = vm::cast(value);

		if (!scene_metadata || !scene_metadata->title ||
			scene_metadata->type > CELL_REC_SCENE_META_TYPE_CLIP_USER ||
			scene_metadata->tagNum > CELL_REC_SCENE_META_TAG_NUM ||
			strnlen(scene_metadata->title.get_ptr(), CELL_REC_SCENE_META_TITLE_LEN) >= CELL_REC_SCENE_META_TITLE_LEN)
		{
			return CELL_REC_ERROR_INVALID_VALUE;
		}

		for (u32 i = 0; i < scene_metadata->tagNum; i++)
		{
			if (!scene_metadata->tag[i] ||
				strnlen(scene_metadata->tag[i].get_ptr(), CELL_REC_SCENE_META_TAG_LEN) >= CELL_REC_SCENE_META_TAG_LEN)
			{
				return CELL_REC_ERROR_INVALID_VALUE;
			}
		}

		rec.param.scene_metadata = {};
		rec.param.scene_metadata.is_set = true;
		rec.param.scene_metadata.type = scene_metadata->type;
		rec.param.scene_metadata.start_time = scene_metadata->startTime;
		rec.param.scene_metadata.end_time = scene_metadata->endTime;
		rec.param.scene_metadata.title = std::string{scene_metadata->title.get_ptr()};
		rec.param.scene_metadata.tags.resize(scene_metadata->tagNum);

		for (u32 i = 0; i < scene_metadata->tagNum; i++)
		{
			if (scene_metadata->tag[i])
			{
				rec.param.scene_metadata.tags[i] = std::string{scene_metadata->tag[i].get_ptr()};
			}
		}

		cellRec.notice("cellRecSetInfo: changed scene metadata to %s", rec.param.scene_metadata.to_string());
		break;
	}
	default:
	{
		return CELL_REC_ERROR_INVALID_VALUE;
	}
	}

	return CELL_OK;
}


DECLARE(ppu_module_manager::cellRec)("cellRec", []()
{
	REG_FUNC(cellRec, cellRecOpen);
	REG_FUNC(cellRec, cellRecClose);
	REG_FUNC(cellRec, cellRecGetInfo);
	REG_FUNC(cellRec, cellRecStop);
	REG_FUNC(cellRec, cellRecStart);
	REG_FUNC(cellRec, cellRecQueryMemSize);
	REG_FUNC(cellRec, cellRecSetInfo);
});
