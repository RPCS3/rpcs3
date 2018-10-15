#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

#include "cellMic.h"
#include <Emu/IdManager.h>
#include <Emu/Cell/lv2/sys_event.h>

LOG_CHANNEL(cellMic);

void mic_context::operator()()
{
	while (fxm::check<mic_thread>() == this && !Emu.IsStopped())
	{
		thread_ctrl::wait_for(1000);

		if (Emu.IsPaused())
			continue;

		if (!micOpened || !micStarted)
			continue;

		auto micQueue = lv2_event_queue::find(eventQueueKey);
		if (!micQueue)
			continue;

		micQueue->send(0, CELL_MIC_DATA, 0, 0);
	}
}

/// Initialization/Shutdown Functions

s32 cellMicInit()
{
	cellMic.notice("cellMicInit()");

	const auto micThread = fxm::make<mic_thread>("Mic Thread");
	if (!micThread)
		return CELL_MIC_ERROR_ALREADY_INIT;

	return CELL_OK;
}

s32 cellMicEnd()
{
	cellMic.notice("cellMicEnd()");

	const auto micThread = fxm::withdraw<mic_thread>();
	if (!micThread)
		return CELL_MIC_ERROR_NOT_INIT;

	// Join
	micThread->operator()();
	return CELL_OK;
}

/// Open/Close Microphone Functions

s32 cellMicOpen(u32 deviceNumber, u32 sampleRate)
{
	cellMic.todo("cellMicOpen(deviceNumber=%um sampleRate=%u)", deviceNumber, sampleRate);
	const auto micThread = fxm::get<mic_thread>();
	if (!micThread)
		return CELL_MIC_ERROR_NOT_INIT;

	if (micThread->micOpened)
		return CELL_MIC_ERROR_ALREADY_OPEN;

	micThread->DspFrequency = sampleRate;
	micThread->micOpened    = true;
	return CELL_OK;
}

s32 cellMicOpenRaw(u32 deviceNumber, u32 sampleRate, u32 maxChannels)
{
	cellMic.todo("cellMicOpenRaw(deviceNumber=%d, sampleRate=%d, maxChannels=%d)", deviceNumber, sampleRate, maxChannels);
	const auto micThread = fxm::get<mic_thread>();
	if (!micThread)
		return CELL_MIC_ERROR_NOT_INIT;

	if (micThread->micOpened)
		return CELL_MIC_ERROR_ALREADY_OPEN;

	micThread->rawFrequency = sampleRate;
	micThread->micOpened    = true;
	return CELL_OK;
}

s32 cellMicOpenEx(u32 deviceNumber, u32 rawSampleRate, u32 rawChannel, u32 DSPSampleRate, u32 bufferSizeMS, u8 signalType)
{
	cellMic.todo("cellMicOpenEx(deviceNumber=%d, rawSampleRate=%d, rawChannel=%d, DSPSampleRate=%d, bufferSizeMS=%d, signalType=0x%x)", deviceNumber, rawSampleRate, rawChannel, DSPSampleRate,
	    bufferSizeMS, signalType);
	const auto micThread = fxm::get<mic_thread>();
	if (!micThread)
		return CELL_MIC_ERROR_NOT_INIT;

	if (micThread->micOpened)
		return CELL_MIC_ERROR_ALREADY_OPEN;

	micThread->rawFrequency = rawSampleRate;
	micThread->DspFrequency = DSPSampleRate;
	micThread->micOpened    = true;
	return CELL_OK;
}

u8 cellMicIsOpen(u32 deviceNumber)
{
	cellMic.warning("cellMicIsOpen(deviceNumber=%d)", deviceNumber);
	const auto micThread = fxm::get<mic_thread>();
	if (!micThread)
		return false;

	return micThread->micOpened;
}

s32 cellMicIsAttached(u32 deviceNumber)
{
	cellMic.warning("cellMicIsAttached(deviceNumber=%d)", deviceNumber);
	return 1;
}

s32 cellMicClose(u32 deviceNumber)
{
	cellMic.warning("cellMicClose(deviceNumber=%d)", deviceNumber);
	const auto micThread = fxm::get<mic_thread>();
	if (!micThread)
		return CELL_MIC_ERROR_NOT_INIT;

	micThread->micOpened = false;

	return CELL_OK;
}

/// Starting/Stopping Microphone Functions

s32 cellMicStart(u32 deviceNumber)
{
	cellMic.warning("cellMicStart(deviceNumber=%d)", deviceNumber);
	const auto micThread = fxm::get<mic_thread>();
	if (!micThread)
		return CELL_MIC_ERROR_NOT_INIT;

	if (!micThread->micOpened)
		return CELL_MIC_ERROR_NOT_OPEN;

	micThread->micStarted = true;

	return CELL_OK;
}

s32 cellMicStartEx(u32 deviceNumber, u32 flags)
{
	cellMic.todo("cellMicStartEx(deviceNumber=%d, flags=%d)", deviceNumber, flags);
	const auto micThread = fxm::get<mic_thread>();
	if (!micThread)
		return CELL_MIC_ERROR_NOT_INIT;

	if (!micThread->micOpened)
		return CELL_MIC_ERROR_NOT_OPEN;

	micThread->micStarted = true;

	return CELL_OK;
}

s32 cellMicStop(u32 deviceNumber)
{
	cellMic.todo("cellMicStop(deviceNumber=%d)", deviceNumber);
	const auto micThread = fxm::get<mic_thread>();
	if (!micThread)
		return CELL_MIC_ERROR_NOT_INIT;

	if (!micThread->micOpened)
		return CELL_MIC_ERROR_NOT_OPEN;

	micThread->micStarted = false;

	return CELL_OK;
}

/// Microphone Attributes/States Functions

s32 cellMicGetDeviceAttr(u32 deviceNumber, vm::ptr<void> deviceAttributes, vm::ptr<u32> arg1, vm::ptr<u32> arg2)
{
	cellMic.todo("cellMicGetDeviceAttr(deviceNumber=%d, deviceAttribute=*0x%x, arg1=*0x%x, arg2=*0x%x)", deviceNumber, deviceAttributes, arg1, arg2);
	return CELL_OK;
}

s32 cellMicSetDeviceAttr()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicGetSignalAttr()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicSetSignalAttr()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicGetSignalState(u32 deviceNumber, CellMicSignalState signalState, vm::ptr<void> value)
{
	cellMic.todo("cellMicGetSignalState(deviceNumber=%d, signalSate=%d, value=0x%x)", deviceNumber, (int)signalState, value);
	const auto micThread = fxm::get<mic_thread>();
	if (!micThread)
		return CELL_MIC_ERROR_NOT_INIT;

	be_t<u32>* ival = (be_t<u32>*)value.get_ptr();
	be_t<f32>* fval = (be_t<f32>*)value.get_ptr();

	switch (signalState)
	{
	case CELL_MIC_SIGSTATE_LOCTALK:
		*ival = 9; // Someone is probably talking
		break;
	case CELL_MIC_SIGSTATE_FARTALK:
		// TODO
		break;
	case CELL_MIC_SIGSTATE_NSR:
		// TODO
		break;
	case CELL_MIC_SIGSTATE_AGC:
		// TODO
		break;
	case CELL_MIC_SIGSTATE_MICENG:
		*fval = 40.0f; // 40 decibels
		break;
	case CELL_MIC_SIGSTATE_SPKENG:
		// TODO
		break;
	}

	return CELL_OK;
}

s32 cellMicGetFormatRaw(u32 deviceNumber, vm::ptr<CellMicInputFormat> format)
{
	cellMic.warning("cellMicGetFormatRaw(deviceNumber=%d, format=0x%x)", deviceNumber, format);
	const auto micThread = fxm::get<mic_thread>();
	if (!micThread)
		return CELL_MIC_ERROR_NOT_INIT;

	format->channelNum    = 4;
	format->subframeSize  = 2;
	format->bitResolution = micThread->bitResolution;
	format->dataType      = 1;
	format->sampleRate    = micThread->rawFrequency;

	return CELL_OK;
}

s32 cellMicGetFormatAux(u32 deviceNumber, vm::ptr<CellMicInputFormat> format)
{
	cellMic.warning("cellMicGetFormatAux(deviceNumber=%d, format=0x%x)", deviceNumber, format);
	const auto micThread = fxm::get<mic_thread>();
	if (!micThread)
		return CELL_MIC_ERROR_NOT_INIT;

	format->channelNum    = 4;
	format->subframeSize  = 2;
	format->bitResolution = micThread->bitResolution;
	format->dataType      = 1;
	format->sampleRate    = micThread->AuxFrequency;

	return CELL_OK;
}

s32 cellMicGetFormatDsp(u32 deviceNumber, vm::ptr<CellMicInputFormat> format)
{
	cellMic.warning("cellMicGetFormatDsp(deviceNumber=%d, format=0x%x)", deviceNumber, format);
	const auto micThread = fxm::get<mic_thread>();
	if (!micThread)
		return CELL_MIC_ERROR_NOT_INIT;

	format->channelNum    = 4;
	format->subframeSize  = 2;
	format->bitResolution = micThread->bitResolution;
	format->dataType      = 1;
	format->sampleRate    = micThread->DspFrequency;

	return CELL_OK;
}

/// Event Queue Functions

s32 cellMicSetNotifyEventQueue(u64 key)
{
	cellMic.warning("cellMicSetNotifyEventQueue(key=0x%llx)", key);
	const auto micThread = fxm::get<mic_thread>();
	if (!micThread)
		return CELL_MIC_ERROR_NOT_INIT;

	// default mic queue size = 4
	auto micQueue = lv2_event_queue::find(key);
	micQueue->send(0, CELL_MIC_ATTACH, 0, 0);
	micThread->eventQueueKey = key;

	return CELL_OK;
}

s32 cellMicSetNotifyEventQueue2(u64 key, u64 source)
{
	// TODO: Actually do things with the source variable
	cellMic.todo("cellMicSetNotifyEventQueue2(key=0x%llx, source=0x%llx", key, source);
	const auto micThread = fxm::get<mic_thread>();
	if (!micThread)
		return CELL_MIC_ERROR_NOT_INIT;

	// default mic queue size = 4
	auto micQueue = lv2_event_queue::find(key);
	micQueue->send(0, CELL_MIC_ATTACH, 0, 0);
	micThread->eventQueueKey = key;

	return CELL_OK;
}

s32 cellMicRemoveNotifyEventQueue(u64 key)
{
	cellMic.warning("cellMicRemoveNotifyEventQueue(key=0x%llx)", key);
	const auto micThread = fxm::get<mic_thread>();
	if (!micThread)
		return CELL_MIC_ERROR_NOT_INIT;

	micThread->eventQueueKey = 0;

	return CELL_OK;
}

/// Reading Functions

s32 cellMicRead(u32 deviceNumber, vm::ptr<void> data, u32 maxBytes)
{
	cellMic.warning("cellMicRead(deviceNumber=%d, data=0x%x, maxBytes=0x%x)", deviceNumber, data, maxBytes);
	const auto micThread = fxm::get<mic_thread>();
	if (!micThread)
		return CELL_MIC_ERROR_NOT_INIT;

	const s32 size = std::min<s32>(maxBytes, bufferSize);

	return size;
}

s32 cellMicReadRaw(u32 deviceNumber, vm::ptr<void> data, int maxBytes)
{
	cellMic.warning("cellMicReadRaw(deviceNumber=%d, data=0x%x, maxBytes=%d)", deviceNumber, data, maxBytes);
	const auto micThread = fxm::get<mic_thread>();
	if (!micThread)
		return CELL_MIC_ERROR_NOT_INIT;

	const s32 size = std::min<s32>(maxBytes, bufferSize);

	return size;
}

s32 cellMicReadAux()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicReadDsp()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

/// Unimplemented Functions

s32 cellMicReset()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicGetDeviceGUID()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicGetType()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicGetStatus()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicStopEx()
{
	fmt::throw_exception("Unexpected function" HERE);
}

s32 cellMicSysShareClose()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicGetFormat()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicSetMultiMicNotifyEventQueue()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicGetFormatEx()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicSysShareStop()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicSysShareOpen()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicCommand()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicSysShareStart()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicSysShareInit()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicSysShareEnd()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicGetDeviceIdentifier()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellMic)("cellMic", []()
{
	REG_FUNC(cellMic, cellMicInit);
	REG_FUNC(cellMic, cellMicEnd);
	REG_FUNC(cellMic, cellMicOpen);
	REG_FUNC(cellMic, cellMicClose);
	REG_FUNC(cellMic, cellMicGetDeviceGUID);
	REG_FUNC(cellMic, cellMicGetType);
	REG_FUNC(cellMic, cellMicIsAttached);
	REG_FUNC(cellMic, cellMicIsOpen);
	REG_FUNC(cellMic, cellMicGetDeviceAttr);
	REG_FUNC(cellMic, cellMicSetDeviceAttr);
	REG_FUNC(cellMic, cellMicGetSignalAttr);
	REG_FUNC(cellMic, cellMicSetSignalAttr);
	REG_FUNC(cellMic, cellMicGetSignalState);
	REG_FUNC(cellMic, cellMicStart);
	REG_FUNC(cellMic, cellMicRead);
	REG_FUNC(cellMic, cellMicStop);
	REG_FUNC(cellMic, cellMicReset);
	REG_FUNC(cellMic, cellMicSetNotifyEventQueue);
	REG_FUNC(cellMic, cellMicSetNotifyEventQueue2);
	REG_FUNC(cellMic, cellMicRemoveNotifyEventQueue);
	REG_FUNC(cellMic, cellMicOpenEx);
	REG_FUNC(cellMic, cellMicStartEx);
	REG_FUNC(cellMic, cellMicGetFormatRaw);
	REG_FUNC(cellMic, cellMicGetFormatAux);
	REG_FUNC(cellMic, cellMicGetFormatDsp);
	REG_FUNC(cellMic, cellMicOpenRaw);
	REG_FUNC(cellMic, cellMicReadRaw);
	REG_FUNC(cellMic, cellMicReadAux);
	REG_FUNC(cellMic, cellMicReadDsp);
	REG_FUNC(cellMic, cellMicGetStatus);
	REG_FUNC(cellMic, cellMicStopEx); // this function shouldn't exist
	REG_FUNC(cellMic, cellMicSysShareClose);
	REG_FUNC(cellMic, cellMicGetFormat);
	REG_FUNC(cellMic, cellMicSetMultiMicNotifyEventQueue);
	REG_FUNC(cellMic, cellMicGetFormatEx);
	REG_FUNC(cellMic, cellMicSysShareStop);
	REG_FUNC(cellMic, cellMicSysShareOpen);
	REG_FUNC(cellMic, cellMicCommand);
	REG_FUNC(cellMic, cellMicSysShareStart);
	REG_FUNC(cellMic, cellMicSysShareInit);
	REG_FUNC(cellMic, cellMicSysShareEnd);
	REG_FUNC(cellMic, cellMicGetDeviceIdentifier);
});
