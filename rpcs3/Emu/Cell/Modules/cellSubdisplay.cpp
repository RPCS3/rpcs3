#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "cellSubdisplay.h"

logs::channel cellSubdisplay("cellSubdisplay");

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
		STR_CASE(CELL_SUBDISPLAY_ERROR_SET_SAMPLE);
		STR_CASE(CELL_SUBDISPLAY_ERROR_AUDIOOUT_IS_BUSY);
		STR_CASE(CELL_SUBDISPLAY_ERROR_ZERO_REGISTERED);
		}

		return unknown;
	});
}

error_code cellSubDisplayInit(vm::ptr<CellSubDisplayParam> pParam, vm::ptr<CellSubDisplayHandler> func, vm::ptr<void> userdata, u32 container)
{
	cellSubdisplay.todo("cellSubDisplayInit(pParam=*0x%x, func=*0x%x, userdata=*0x%x, container=0x%x)", pParam, func, userdata, container);
	return CELL_OK;
}

error_code cellSubDisplayEnd()
{
	cellSubdisplay.todo("cellSubDisplayEnd()");
	return CELL_OK;
}

error_code cellSubDisplayGetRequiredMemory(vm::ptr<CellSubDisplayParam> pParam)
{
	cellSubdisplay.warning("cellSubDisplayGetRequiredMemory(pParam=*0x%x)", pParam);

	if (pParam->version == CELL_SUBDISPLAY_VERSION_0002)
	{
		return not_an_error(CELL_SUBDISPLAY_0002_MEMORY_CONTAINER_SIZE);
	}
	else
	{
		return not_an_error(CELL_SUBDISPLAY_0001_MEMORY_CONTAINER_SIZE);
	}
}

error_code cellSubDisplayStart()
{
	cellSubdisplay.todo("cellSubDisplayStart()");
	return CELL_OK;
}

error_code cellSubDisplayStop()
{
	cellSubdisplay.todo("cellSubDisplayStop()");
	return CELL_OK;
}

error_code cellSubDisplayGetVideoBuffer(s32 groupId, vm::pptr<void> ppVideoBuf, vm::ptr<u32> pSize)
{
	cellSubdisplay.todo("cellSubDisplayGetVideoBuffer(groupId=%d, ppVideoBuf=**0x%x, pSize=*0x%x)", groupId, ppVideoBuf, pSize);
	return CELL_OK;
}

error_code cellSubDisplayAudioOutBlocking(s32 groupId, vm::ptr<void> pvData, s32 samples)
{
	cellSubdisplay.todo("cellSubDisplayAudioOutBlocking(groupId=%d, pvData=*0x%x, samples=%d)", groupId, pvData, samples);

	if (samples % 1024)
	{
		return CELL_SUBDISPLAY_ERROR_SET_SAMPLE;
	}

	return CELL_OK;
}

error_code cellSubDisplayAudioOutNonBlocking(s32 groupId, vm::ptr<void> pvData, s32 samples)
{
	cellSubdisplay.todo("cellSubDisplayAudioOutNonBlocking(groupId=%d, pvData=*0x%x, samples=%d)", groupId, pvData, samples);

	if (samples % 1024)
	{
		return CELL_SUBDISPLAY_ERROR_SET_SAMPLE;
	}

	return CELL_OK;
}

error_code cellSubDisplayGetPeerNum(s32 groupId)
{
	cellSubdisplay.todo("cellSubDisplayGetPeerNum(groupId=%d)", groupId);
	return CELL_OK;
}

error_code cellSubDisplayGetPeerList(s32 groupId, vm::ptr<CellSubDisplayPeerInfo> pInfo, vm::ptr<s32> pNum)
{
	cellSubdisplay.todo("cellSubDisplayGetPeerList(groupId=%d, pInfo=*0x%x, pNum=*0x%x)", groupId, pInfo, pNum);

	*pNum = 0;

	return CELL_OK;
}

DECLARE(ppu_module_manager::cellSubdisplay)("cellSubdisplay", []()
{
	// Initialization / Termination Functions
	REG_FUNC(cellSubdisplay, cellSubDisplayInit);
	REG_FUNC(cellSubdisplay, cellSubDisplayEnd);
	REG_FUNC(cellSubdisplay, cellSubDisplayGetRequiredMemory);
	REG_FUNC(cellSubdisplay, cellSubDisplayStart);
	REG_FUNC(cellSubdisplay, cellSubDisplayStop);

	// Data Setting Functions
	REG_FUNC(cellSubdisplay, cellSubDisplayGetVideoBuffer);
	REG_FUNC(cellSubdisplay, cellSubDisplayAudioOutBlocking);
	REG_FUNC(cellSubdisplay, cellSubDisplayAudioOutNonBlocking);

	// Peer Status Acquisition Functions
	REG_FUNC(cellSubdisplay, cellSubDisplayGetPeerNum);
	REG_FUNC(cellSubdisplay, cellSubDisplayGetPeerList);
});
