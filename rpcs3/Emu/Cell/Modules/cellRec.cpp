#include "stdafx.h"
#include "Emu/Cell/lv2/sys_memory.h"
#include "Emu/Cell/timers.hpp"
#include "Emu/Cell/PPUModule.h"
#include "Emu/IdManager.h"
#include "cellRec.h"
#include "cellSysutil.h"

LOG_CHANNEL(cellRec);

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
	VIDEO_QUALITY_3 = 0x300,
	VIDEO_QUALITY_4 = 0x400,
	VIDEO_QUALITY_5 = 0x500,
	VIDEO_QUALITY_6 = 0x600,
	VIDEO_QUALITY_7 = 0x700,
};

enum class rec_state : u32
{
	closed = 0x2710,
	open   = 0x2711,
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
	u32 ring_sec = 10; // TODO
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
	} movie_metadata{};
	struct
	{
		bool is_set = false;
		u32 type;
		u64 start_time;
		u64 end_time;
		std::string title;
		std::vector<std::string> tags;
	} scene_metadata{};
};

struct rec_info
{
	vm::ptr<CellRecCallback> cb{};
	vm::ptr<void> cbUserData{};
	atomic_t<rec_state> state = rec_state::closed;
	rec_param param{};

	vm::bptr<u8> video_input_buffer{};
	vm::bptr<u8> audio_input_buffer{};

	u32 pitch = 4 * 1280; // TODO: remember to handle the pitch for CELL_REC_PARAM_VIDEO_INPUT_YUV420PLANAR_16_9
	u32 width = 1280;
	u32 height = 720;

	u64 recording_time_start = 0; // TODO: proper time measurements
	u64 recording_time_end = 0;   // TODO: proper time measurements
	u64 recording_time_total = 0; // TODO: proper time measurements

	shared_mutex mutex;
};

bool create_path(std::string& out, std::string dir_name, std::string file_name)
{
	out.clear();

	if (dir_name.size() + file_name.size() > CELL_REC_MAX_PATH_LEN)
	{
		return false;
	}

	out = dir_name;

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
			if (opt.value.audio_input == CELL_REC_PARAM_AUDIO_INPUT_DISABLE)
			{
				rec.param.audio_input_mix_vol = 0;
			}
			else
			{
				rec.param.audio_input_mix_vol = 100;
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

	rec.cb = cb;
	rec.cbUserData = cbUserData;

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		cb(ppu, CELL_REC_STATUS_OPEN, CELL_OK, cbUserData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellRecClose(s32 isDiscard)
{
	cellRec.todo("cellRecClose(isDiscard=0x%x)", isDiscard);

	auto& rec = g_fxo->get<rec_info>();

	if (rec.state != rec_state::open)
	{
		return CELL_REC_ERROR_INVALID_STATE;
	}

	sysutil_register_cb([=, &rec](ppu_thread& ppu) -> s32
	{
		if (isDiscard)
		{
			// TODO: remove recording
		}
		else
		{
			// TODO: save recording
		}

		vm::dealloc(rec.video_input_buffer.addr(), vm::main);
		vm::dealloc(rec.audio_input_buffer.addr(), vm::main);

		rec.param = {};

		// TODO: Sets status CELL_REC_ERROR_FILE_NO_DATA if the start time AND end time are out of scope

		// Sets status CELL_REC_STATUS_ERR on error
		rec.cb(ppu, CELL_REC_STATUS_CLOSE, CELL_OK, rec.cbUserData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellRecStop()
{
	cellRec.todo("cellRecStop()");

	auto& rec = g_fxo->get<rec_info>();

	if (rec.state != rec_state::open)
	{
		return CELL_REC_ERROR_INVALID_STATE;
	}

	sysutil_register_cb([=, &rec](ppu_thread& ppu) -> s32
	{
		// TODO: stop recording
		rec.recording_time_end = get_system_time();
		rec.recording_time_total += (rec.recording_time_end - rec.recording_time_start);

		// Sets status CELL_REC_STATUS_ERR on error
		rec.cb(ppu, CELL_REC_STATUS_STOP, CELL_OK, rec.cbUserData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellRecStart()
{
	cellRec.todo("cellRecStart()");

	auto& rec = g_fxo->get<rec_info>();

	if (rec.state != rec_state::open)
	{
		return CELL_REC_ERROR_INVALID_STATE;
	}

	sysutil_register_cb([=, &rec](ppu_thread& ppu) -> s32
	{
		// TODO: start recording
		rec.recording_time_start = get_system_time();
		rec.recording_time_end = rec.recording_time_start;

		// Sets status CELL_REC_STATUS_ERR on error
		rec.cb(ppu, CELL_REC_STATUS_START, CELL_OK, rec.cbUserData);
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
		*pValue = rec.width;
		break;
	}
	case CELL_REC_INFO_VIDEO_INPUT_PITCH:
	{
		*pValue = rec.pitch;
		break;
	}
	case CELL_REC_INFO_VIDEO_INPUT_HEIGHT:
	{
		*pValue = rec.height;
		break;
	}
	case CELL_REC_INFO_AUDIO_INPUT_ADDR:
	{
		*pValue = rec.audio_input_buffer.addr();
		break;
	}
	case CELL_REC_INFO_MOVIE_TIME_MSEC:
	{
		if (rec.state == rec_state::open)
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

	if (rec.state != rec_state::open)
	{
		return CELL_REC_ERROR_INVALID_STATE;
	}

	switch (setInfo)
	{
	case CELL_REC_SETINFO_MOVIE_START_TIME_MSEC:
	{
		rec.param.scene_metadata.start_time = value;
		break;
	}
	case CELL_REC_SETINFO_MOVIE_END_TIME_MSEC:
	{
		rec.param.scene_metadata.end_time = value;
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

		break;
	}
	case CELL_REC_SETINFO_SCEME_META:
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
		
		for (usz i = 0; i < scene_metadata->tagNum; i++)
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

		for (usz i = 0; i < scene_metadata->tagNum; i++)
		{
			if (scene_metadata->tag[i])
			{
				rec.param.scene_metadata.tags[i] = std::string{scene_metadata->tag[i].get_ptr()};
			}
		}

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
