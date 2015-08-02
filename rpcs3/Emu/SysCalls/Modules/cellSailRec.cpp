#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

extern Module cellSailRec;

// Error Codes
enum
{
	CELL_SAIL_ERROR_INVALID_ARG        = 0x80610701,
	CELL_SAIL_ERROR_INVALID_STATE      = 0x80610702,
	CELL_SAIL_ERROR_UNSUPPORTED_STREAM = 0x80610703,
	CELL_SAIL_ERROR_INDEX_OUT_OF_RANGE = 0x80610704,
	CELL_SAIL_ERROR_EMPTY              = 0x80610705,
	CELL_SAIL_ERROR_FULLED             = 0x80610706,
	CELL_SAIL_ERROR_USING              = 0x80610707,
	CELL_SAIL_ERROR_NOT_AVAILABLE      = 0x80610708,
	CELL_SAIL_ERROR_CANCEL             = 0x80610709,
	CELL_SAIL_ERROR_MEMORY             = 0x806107F0,
	CELL_SAIL_ERROR_INVALID_FD         = 0x806107F1,
	CELL_SAIL_ERROR_FATAL              = 0x806107FF,
};

s32 cellSailProfileSetEsAudioParameter()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailProfileSetEsVideoParameter()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailProfileSetStreamParameter()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailVideoConverterCanProcess()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailVideoConverterProcess()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailVideoConverterCanGetResult()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailVideoConverterGetResult()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailFeederAudioInitialize()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailFeederAudioFinalize()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailFeederAudioNotifyCallCompleted()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailFeederAudioNotifyFrameOut()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailFeederAudioNotifySessionEnd()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailFeederAudioNotifySessionError()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailFeederVideoInitialize()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailFeederVideoFinalize()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailFeederVideoNotifyCallCompleted()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailFeederVideoNotifyFrameOut()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailFeederVideoNotifySessionEnd()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailFeederVideoNotifySessionError()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailRecorderInitialize()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailRecorderFinalize()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailRecorderSetFeederAudio()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailRecorderSetFeederVideo()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailRecorderSetParameter()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailRecorderGetParameter()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailRecorderSubscribeEvent()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailRecorderUnsubscribeEvent()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailRecorderReplaceEventHandler()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailRecorderBoot()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailRecorderCreateProfile()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailRecorderDestroyProfile()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailRecorderCreateVideoConverter()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailRecorderDestroyVideoConverter()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailRecorderOpenStream()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailRecorderCloseStream()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailRecorderStart()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailRecorderStop()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailRecorderCancel()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

s32 cellSailRecorderDumpImage()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

Module cellSailRec("cellSailRec", []()
{
	REG_FUNC(cellSailRec, cellSailProfileSetEsAudioParameter);
	REG_FUNC(cellSailRec, cellSailProfileSetEsVideoParameter);
	REG_FUNC(cellSailRec, cellSailProfileSetStreamParameter);

	REG_FUNC(cellSailRec, cellSailVideoConverterCanProcess);
	REG_FUNC(cellSailRec, cellSailVideoConverterProcess);
	REG_FUNC(cellSailRec, cellSailVideoConverterCanGetResult);
	REG_FUNC(cellSailRec, cellSailVideoConverterGetResult);

	REG_FUNC(cellSailRec, cellSailFeederAudioInitialize);
	REG_FUNC(cellSailRec, cellSailFeederAudioFinalize);
	REG_FUNC(cellSailRec, cellSailFeederAudioNotifyCallCompleted);
	REG_FUNC(cellSailRec, cellSailFeederAudioNotifyFrameOut);
	REG_FUNC(cellSailRec, cellSailFeederAudioNotifySessionEnd);
	REG_FUNC(cellSailRec, cellSailFeederAudioNotifySessionError);

	REG_FUNC(cellSailRec, cellSailFeederVideoInitialize);
	REG_FUNC(cellSailRec, cellSailFeederVideoFinalize);
	REG_FUNC(cellSailRec, cellSailFeederVideoNotifyCallCompleted);
	REG_FUNC(cellSailRec, cellSailFeederVideoNotifyFrameOut);
	REG_FUNC(cellSailRec, cellSailFeederVideoNotifySessionEnd);
	REG_FUNC(cellSailRec, cellSailFeederVideoNotifySessionError);

	REG_FUNC(cellSailRec, cellSailRecorderInitialize);
	REG_FUNC(cellSailRec, cellSailRecorderFinalize);
	REG_FUNC(cellSailRec, cellSailRecorderSetFeederAudio);
	REG_FUNC(cellSailRec, cellSailRecorderSetFeederVideo);
	REG_FUNC(cellSailRec, cellSailRecorderSetParameter);
	REG_FUNC(cellSailRec, cellSailRecorderGetParameter);
	REG_FUNC(cellSailRec, cellSailRecorderSubscribeEvent);
	REG_FUNC(cellSailRec, cellSailRecorderUnsubscribeEvent);
	REG_FUNC(cellSailRec, cellSailRecorderReplaceEventHandler);
	REG_FUNC(cellSailRec, cellSailRecorderBoot);
	REG_FUNC(cellSailRec, cellSailRecorderCreateProfile);
	REG_FUNC(cellSailRec, cellSailRecorderDestroyProfile);
	REG_FUNC(cellSailRec, cellSailRecorderCreateVideoConverter);
	REG_FUNC(cellSailRec, cellSailRecorderDestroyVideoConverter);
	REG_FUNC(cellSailRec, cellSailRecorderOpenStream);
	REG_FUNC(cellSailRec, cellSailRecorderCloseStream);
	REG_FUNC(cellSailRec, cellSailRecorderStart);
	REG_FUNC(cellSailRec, cellSailRecorderStop);
	REG_FUNC(cellSailRec, cellSailRecorderCancel);

	REG_FUNC(cellSailRec, cellSailRecorderDumpImage);
});
