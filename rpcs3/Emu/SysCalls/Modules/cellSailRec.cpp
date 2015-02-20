#include "stdafx.h"
#if 0

void cellSailRec_init();
Module cellSailRec(0xf034, cellSailRec_init);

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

int cellSailProfileSetEsAudioParameter()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailProfileSetEsVideoParameter()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailProfileSetStreamParameter()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailVideoConverterCanProcess()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailVideoConverterProcess()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailVideoConverterCanGetResult()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailVideoConverterGetResult()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailFeederAudioInitialize()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailFeederAudioFinalize()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailFeederAudioNotifyCallCompleted()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailFeederAudioNotifyFrameOut()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailFeederAudioNotifySessionEnd()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailFeederAudioNotifySessionError()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailFeederVideoInitialize()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailFeederVideoFinalize()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailFeederVideoNotifyCallCompleted()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailFeederVideoNotifyFrameOut()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailFeederVideoNotifySessionEnd()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailFeederVideoNotifySessionError()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailRecorderInitialize()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailRecorderFinalize()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailRecorderSetFeederAudio()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailRecorderSetFeederVideo()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailRecorderSetParameter()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailRecorderGetParameter()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailRecorderBoot()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailRecorderCreateProfile()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailRecorderDestroyProfile()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailRecorderCreateVideoConverter()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailRecorderDestroyVideoConverter()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailRecorderOpenStream()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailRecorderCloseStream()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailRecorderStart()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailRecorderStop()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailRecorderCancel()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

int cellSailRecorderDumpImage()
{
	UNIMPLEMENTED_FUNC(cellSailRec);
	return CELL_OK;
}

void cellSailRec_init()
{
	REG_FUNC(cellSailRec, cellSailProfileSetEsAudioParameter);
	REG_FUNC(cellSailRec, cellSailProfileSetEsVideoParameter);
	REG_FUNC(cellSailRec, cellSailProfileSetStreamParameter);

	REG_FUNC(cellSailRec, cellSailVideoConverterCanProcess);
	REG_FUNC(cellSailRec, cellSailVideoConverterProcess);
	REG_FUNC(cellSailRec, cellSailVideoConverterCanGetResult);
	REG_FUNC(cellSailRec, cellSailVideoConverterGetResult);
	//cellSailRec.AddFunc(, CellSailVideoConverterFuncProcessDone);

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
	//cellSailRec.AddFunc(, cellSailRecorderSubscribeEvent);
	//cellSailRec.AddFunc(, cellSailRecorderUnsubscribeEvent);
	//cellSailRec.AddFunc(, cellSailRecorderReplaceEventHandler);
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
}
#endif
