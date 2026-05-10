#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/IdManager.h"

#include "cellSubDisplay.h"

LOG_CHANNEL(cellSubDisplay);

template<>
void fmt_class_string<CellSubDisplayError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
		STR_CASE(CELL_SUBDISPLAY_ERROR_OUT_OF_MEMORY);
		STR_CASE(CELL_SUBDISPLAY_ERROR_FATAL);
		STR_CASE(CELL_SUBDISPLAY_ERROR_NOT_FOUND);
		STR_CASE(CELL_SUBDISPLAY_ERROR_INVALID_VALUE);
		STR_CASE(CELL_SUBDISPLAY_ERROR_NOT_INITIALIZED);
		STR_CASE(CELL_SUBDISPLAY_ERROR_NOT_SUPPORTED);
		STR_CASE(CELL_SUBDISPLAY_ERROR_SET_SAMPLE);
		STR_CASE(CELL_SUBDISPLAY_ERROR_AUDIOOUT_IS_BUSY);
		STR_CASE(CELL_SUBDISPLAY_ERROR_ZERO_REGISTERED);
		}

		return unknown;
	});
}

enum class sub_display_status : u32
{
	uninitialized = 0,
	stopped = 1,
	started = 2
};

struct sub_display_manager
{
	shared_mutex mutex;
	sub_display_status status = sub_display_status::uninitialized;
	CellSubDisplayParam param{};
	vm::ptr<CellSubDisplayHandler> func = vm::null;
	vm::ptr<void> userdata = vm::null;

	// Video data
	vm::ptr<void> video_buffer = vm::null;
	u32 buf_size = 0;

	// Audio data
	bool audio_is_busy = false;

	// Touch data
	std::array<CellSubDisplayTouchInfo, CELL_SUBDISPLAY_TOUCH_MAX_TOUCH_INFO> touch_info{};
};


error_code check_param(CellSubDisplayParam* param)
{
	if (!param ||
		param->version < CELL_SUBDISPLAY_VERSION_0001 ||
		param->version > CELL_SUBDISPLAY_VERSION_0003 ||
		param->mode != CELL_SUBDISPLAY_MODE_REMOTEPLAY ||
		param->nGroup != 1 ||
		param->nPeer != 1 ||
		param->audioParam.ch != 2 ||
		param->audioParam.audioMode < CELL_SUBDISPLAY_AUDIO_MODE_SETDATA ||
		param->audioParam.audioMode > CELL_SUBDISPLAY_AUDIO_MODE_CAPTURE)
	{
		return CELL_SUBDISPLAY_ERROR_INVALID_VALUE;
	}

	switch (param->videoParam.format)
	{
	case CELL_SUBDISPLAY_VIDEO_FORMAT_A8R8G8B8:
	case CELL_SUBDISPLAY_VIDEO_FORMAT_R8G8B8A8:
	{
		if (param->version == CELL_SUBDISPLAY_VERSION_0003 ||
			param->videoParam.width != 480 ||
			param->videoParam.height != 272 ||
			(param->videoParam.pitch != 1920 && param->videoParam.pitch != 2048) ||
			param->videoParam.aspectRatio < CELL_SUBDISPLAY_VIDEO_ASPECT_RATIO_16_9 ||
			param->videoParam.aspectRatio > CELL_SUBDISPLAY_VIDEO_ASPECT_RATIO_4_3 ||
			param->videoParam.videoMode < CELL_SUBDISPLAY_VIDEO_MODE_SETDATA ||
			param->videoParam.videoMode > CELL_SUBDISPLAY_VIDEO_MODE_CAPTURE)
		{
			return CELL_SUBDISPLAY_ERROR_INVALID_VALUE;
		}

		break;
	}
	case CELL_SUBDISPLAY_VIDEO_FORMAT_YUV420:
	{
		if (param->version != CELL_SUBDISPLAY_VERSION_0003 ||
			param->videoParam.width != CELL_SUBDISPLAY_0003_WIDTH ||
			param->videoParam.height != CELL_SUBDISPLAY_0003_HEIGHT ||
			param->videoParam.pitch != CELL_SUBDISPLAY_0003_PITCH ||
			param->videoParam.aspectRatio != CELL_SUBDISPLAY_VIDEO_ASPECT_RATIO_16_9 ||
			param->videoParam.videoMode != CELL_SUBDISPLAY_VIDEO_MODE_SETDATA)
		{
			return CELL_SUBDISPLAY_ERROR_INVALID_VALUE;
		}

		break;
	}
	default:
	{
		return CELL_SUBDISPLAY_ERROR_INVALID_VALUE;
	}
	}

	return CELL_OK;
}

error_code cellSubDisplayInit(vm::ptr<CellSubDisplayParam> pParam, vm::ptr<CellSubDisplayHandler> func, vm::ptr<void> userdata, u32 container)
{
	cellSubDisplay.todo("cellSubDisplayInit(pParam=*0x%x, func=*0x%x, userdata=*0x%x, container=0x%x)", pParam, func, userdata, container);

	auto& manager = g_fxo->get<sub_display_manager>();
	std::lock_guard lock(manager.mutex);

	if (manager.func)
	{
		return CELL_SUBDISPLAY_ERROR_NOT_INITIALIZED;
	}

	if (error_code error = check_param(pParam.get_ptr()))
	{
		return error;
	}

	if (!func)
	{
		return CELL_SUBDISPLAY_ERROR_INVALID_VALUE;
	}

	manager.param = *pParam;
	manager.func = func;
	manager.userdata = userdata;

	if (true) // TODO
	{
		return CELL_SUBDISPLAY_ERROR_ZERO_REGISTERED;
	}

	return CELL_OK;
}

error_code cellSubDisplayEnd()
{
	cellSubDisplay.todo("cellSubDisplayEnd()");

	auto& manager = g_fxo->get<sub_display_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.func)
	{
		return CELL_SUBDISPLAY_ERROR_NOT_INITIALIZED;
	}

	if (manager.status == sub_display_status::started)
	{
		// TODO:
		// cellSubDisplayStop();
	}

	manager.param = {};
	manager.func = vm::null;
	manager.userdata = vm::null;
	manager.status = sub_display_status::uninitialized;

	return CELL_OK;
}

error_code cellSubDisplayGetRequiredMemory(vm::ptr<CellSubDisplayParam> pParam)
{
	cellSubDisplay.warning("cellSubDisplayGetRequiredMemory(pParam=*0x%x)", pParam);

	if (error_code error = check_param(pParam.get_ptr()))
	{
		return error;
	}

	switch (pParam->version)
	{
		case CELL_SUBDISPLAY_VERSION_0001: return not_an_error(CELL_SUBDISPLAY_0001_MEMORY_CONTAINER_SIZE);
		case CELL_SUBDISPLAY_VERSION_0002: return not_an_error(CELL_SUBDISPLAY_0002_MEMORY_CONTAINER_SIZE);
		case CELL_SUBDISPLAY_VERSION_0003: return not_an_error(CELL_SUBDISPLAY_0003_MEMORY_CONTAINER_SIZE);
		default: break;
	}

	return CELL_SUBDISPLAY_ERROR_INVALID_VALUE;
}

error_code cellSubDisplayStart()
{
	cellSubDisplay.todo("cellSubDisplayStart()");

	auto& manager = g_fxo->get<sub_display_manager>();
	std::lock_guard lock(manager.mutex);

	if (manager.status == sub_display_status::uninitialized)
	{
		return CELL_SUBDISPLAY_ERROR_NOT_INITIALIZED;
	}

	manager.status = sub_display_status::started;

	// TODO

	return CELL_OK;
}

error_code cellSubDisplayStop()
{
	cellSubDisplay.todo("cellSubDisplayStop()");

	auto& manager = g_fxo->get<sub_display_manager>();
	std::lock_guard lock(manager.mutex);

	if (manager.status == sub_display_status::uninitialized)
	{
		return CELL_SUBDISPLAY_ERROR_NOT_INITIALIZED;
	}

	manager.status = sub_display_status::stopped;

	// TODO

	return CELL_OK;
}

error_code cellSubDisplayGetVideoBuffer(s32 groupId, vm::pptr<void> ppVideoBuf, vm::ptr<u32> pSize)
{
	cellSubDisplay.todo("cellSubDisplayGetVideoBuffer(groupId=%d, ppVideoBuf=**0x%x, pSize=*0x%x)", groupId, ppVideoBuf, pSize);

	auto& manager = g_fxo->get<sub_display_manager>();
	std::lock_guard lock(manager.mutex);

	if (manager.status == sub_display_status::uninitialized)
	{
		return CELL_SUBDISPLAY_ERROR_NOT_INITIALIZED;
	}

	if (groupId != 0 || !ppVideoBuf || !pSize)
	{
		return CELL_SUBDISPLAY_ERROR_INVALID_VALUE;
	}

	*pSize = manager.buf_size;
	*ppVideoBuf = manager.video_buffer;

	return CELL_OK;
}

error_code cellSubDisplayAudioOutBlocking(s32 groupId, vm::ptr<void> pvData, s32 samples)
{
	cellSubDisplay.todo("cellSubDisplayAudioOutBlocking(groupId=%d, pvData=*0x%x, samples=%d)", groupId, pvData, samples);

	auto& manager = g_fxo->get<sub_display_manager>();
	std::lock_guard lock(manager.mutex);

	if (manager.status == sub_display_status::uninitialized)
	{
		return CELL_SUBDISPLAY_ERROR_NOT_INITIALIZED;
	}

	if (groupId != 0 || samples < 0)
	{
		return CELL_SUBDISPLAY_ERROR_INVALID_VALUE;
	}

	if (samples % 1024)
	{
		return CELL_SUBDISPLAY_ERROR_SET_SAMPLE;
	}

	// TODO

	return CELL_OK;
}

error_code cellSubDisplayAudioOutNonBlocking(s32 groupId, vm::ptr<void> pvData, s32 samples)
{
	cellSubDisplay.todo("cellSubDisplayAudioOutNonBlocking(groupId=%d, pvData=*0x%x, samples=%d)", groupId, pvData, samples);

	auto& manager = g_fxo->get<sub_display_manager>();
	std::lock_guard lock(manager.mutex);

	if (manager.status == sub_display_status::uninitialized)
	{
		return CELL_SUBDISPLAY_ERROR_NOT_INITIALIZED;
	}

	if (groupId != 0 || samples < 0)
	{
		return CELL_SUBDISPLAY_ERROR_INVALID_VALUE;
	}

	if (samples % 1024)
	{
		return CELL_SUBDISPLAY_ERROR_SET_SAMPLE;
	}

	if (manager.audio_is_busy)
	{
		return CELL_SUBDISPLAY_ERROR_AUDIOOUT_IS_BUSY;
	}

	// TODO: fetch audio async
	// manager.audio_is_busy = true;

	return CELL_OK;
}

error_code cellSubDisplayGetPeerNum(s32 groupId)
{
	cellSubDisplay.todo("cellSubDisplayGetPeerNum(groupId=%d)", groupId);

	auto& manager = g_fxo->get<sub_display_manager>();
	std::lock_guard lock(manager.mutex);

	if (manager.status == sub_display_status::uninitialized)
	{
		return CELL_SUBDISPLAY_ERROR_NOT_INITIALIZED;
	}

	if (groupId != 0)
	{
		return CELL_SUBDISPLAY_ERROR_INVALID_VALUE;
	}

	s32 peer_num = 0; // TODO

	return not_an_error(peer_num);
}

error_code cellSubDisplayGetPeerList(s32 groupId, vm::ptr<CellSubDisplayPeerInfo> pInfo, vm::ptr<s32> pNum)
{
	cellSubDisplay.todo("cellSubDisplayGetPeerList(groupId=%d, pInfo=*0x%x, pNum=*0x%x)", groupId, pInfo, pNum);

	auto& manager = g_fxo->get<sub_display_manager>();
	std::lock_guard lock(manager.mutex);

	if (manager.status == sub_display_status::uninitialized)
	{
		return CELL_SUBDISPLAY_ERROR_NOT_INITIALIZED;
	}

	if (groupId != 0)
	{
		return CELL_OK;
	}

	if (!pInfo || !pNum || *pNum < 1)
	{
		return CELL_SUBDISPLAY_ERROR_INVALID_VALUE;
	}

	*pNum = 0;

	// TODO

	return CELL_OK;
}

error_code cellSubDisplayGetTouchInfo(s32 groupId, vm::ptr<CellSubDisplayTouchInfo> pTouchInfo, vm::ptr<s32> pNumTouchInfo)
{
	cellSubDisplay.todo("cellSubDisplayGetTouchInfo(groupId=%d, pTouchInfo=*0x%x, pNumTouchInfo=*0x%x)", groupId, pTouchInfo, pNumTouchInfo);

	if (groupId != 0 || !pNumTouchInfo || !pTouchInfo)
	{
		return CELL_SUBDISPLAY_ERROR_INVALID_VALUE;
	}

	auto& manager = g_fxo->get<sub_display_manager>();
	std::lock_guard lock(manager.mutex);

	if (manager.param.version != CELL_SUBDISPLAY_VERSION_0003)
	{
		return CELL_SUBDISPLAY_ERROR_NOT_SUPPORTED;
	}

	if (*pNumTouchInfo > CELL_SUBDISPLAY_TOUCH_MAX_TOUCH_INFO)
	{
		*pNumTouchInfo = CELL_SUBDISPLAY_TOUCH_MAX_TOUCH_INFO;
	}

	std::memcpy(pTouchInfo.get_ptr(), manager.touch_info.data(), *pNumTouchInfo * sizeof(CellSubDisplayTouchInfo));

	return CELL_OK;
}

DECLARE(ppu_module_manager::cellSubDisplay)("cellSubDisplay", []()
{
	// Initialization / Termination Functions
	REG_FUNC(cellSubDisplay, cellSubDisplayInit);
	REG_FUNC(cellSubDisplay, cellSubDisplayEnd);
	REG_FUNC(cellSubDisplay, cellSubDisplayGetRequiredMemory);
	REG_FUNC(cellSubDisplay, cellSubDisplayStart);
	REG_FUNC(cellSubDisplay, cellSubDisplayStop);

	// Data Setting Functions
	REG_FUNC(cellSubDisplay, cellSubDisplayGetVideoBuffer);
	REG_FUNC(cellSubDisplay, cellSubDisplayAudioOutBlocking);
	REG_FUNC(cellSubDisplay, cellSubDisplayAudioOutNonBlocking);

	// Peer Status Acquisition Functions
	REG_FUNC(cellSubDisplay, cellSubDisplayGetPeerNum);
	REG_FUNC(cellSubDisplay, cellSubDisplayGetPeerList);

	//
	REG_FUNC(cellSubDisplay, cellSubDisplayGetTouchInfo);
});
