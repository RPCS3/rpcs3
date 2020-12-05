#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

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

error_code cellSubDisplayInit(vm::ptr<CellSubDisplayParam> pParam, vm::ptr<CellSubDisplayHandler> func, vm::ptr<void> userdata, u32 container)
{
	cellSubDisplay.todo("cellSubDisplayInit(pParam=*0x%x, func=*0x%x, userdata=*0x%x, container=0x%x)", pParam, func, userdata, container);
	return CELL_SUBDISPLAY_ERROR_ZERO_REGISTERED;
}

error_code cellSubDisplayEnd()
{
	cellSubDisplay.todo("cellSubDisplayEnd()");
	return CELL_OK;
}

error_code cellSubDisplayGetRequiredMemory(vm::ptr<CellSubDisplayParam> pParam)
{
	cellSubDisplay.warning("cellSubDisplayGetRequiredMemory(pParam=*0x%x)", pParam);

	switch (pParam->version)
	{
		case CELL_SUBDISPLAY_VERSION_0001: return not_an_error(CELL_SUBDISPLAY_0001_MEMORY_CONTAINER_SIZE);
		case CELL_SUBDISPLAY_VERSION_0002: return not_an_error(CELL_SUBDISPLAY_0002_MEMORY_CONTAINER_SIZE);
		case CELL_SUBDISPLAY_VERSION_0003: return not_an_error(CELL_SUBDISPLAY_0003_MEMORY_CONTAINER_SIZE);
	}

	return CELL_SUBDISPLAY_ERROR_INVALID_VALUE;
}

error_code cellSubDisplayStart()
{
	cellSubDisplay.todo("cellSubDisplayStart()");
	return CELL_OK;
}

error_code cellSubDisplayStop()
{
	cellSubDisplay.todo("cellSubDisplayStop()");
	return CELL_OK;
}

error_code cellSubDisplayGetVideoBuffer(s32 groupId, vm::pptr<void> ppVideoBuf, vm::ptr<u32> pSize)
{
	cellSubDisplay.todo("cellSubDisplayGetVideoBuffer(groupId=%d, ppVideoBuf=**0x%x, pSize=*0x%x)", groupId, ppVideoBuf, pSize);
	return CELL_OK;
}

error_code cellSubDisplayAudioOutBlocking(s32 groupId, vm::ptr<void> pvData, s32 samples)
{
	cellSubDisplay.todo("cellSubDisplayAudioOutBlocking(groupId=%d, pvData=*0x%x, samples=%d)", groupId, pvData, samples);

	if (samples % 1024)
	{
		return CELL_SUBDISPLAY_ERROR_SET_SAMPLE;
	}

	return CELL_OK;
}

error_code cellSubDisplayAudioOutNonBlocking(s32 groupId, vm::ptr<void> pvData, s32 samples)
{
	cellSubDisplay.todo("cellSubDisplayAudioOutNonBlocking(groupId=%d, pvData=*0x%x, samples=%d)", groupId, pvData, samples);

	if (samples % 1024)
	{
		return CELL_SUBDISPLAY_ERROR_SET_SAMPLE;
	}

	return CELL_OK;
}

error_code cellSubDisplayGetPeerNum(s32 groupId)
{
	cellSubDisplay.todo("cellSubDisplayGetPeerNum(groupId=%d)", groupId);
	return CELL_OK;
}

error_code cellSubDisplayGetPeerList(s32 groupId, vm::ptr<CellSubDisplayPeerInfo> pInfo, vm::ptr<s32> pNum)
{
	cellSubDisplay.todo("cellSubDisplayGetPeerList(groupId=%d, pInfo=*0x%x, pNum=*0x%x)", groupId, pInfo, pNum);

	*pNum = 0;

	return CELL_OK;
}

error_code cellSubDisplayGetTouchInfo(s32 groupId, vm::ptr<CellSubDisplayTouchInfo> pTouchInfo, vm::ptr<s32> pNumTouchInfo)
{
	cellSubDisplay.todo("cellSubDisplayGetTouchInfo(groupId=%d, pTouchInfo=*0x%x, pNumTouchInfo=*0x%x)", groupId, pTouchInfo, pNumTouchInfo);
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
