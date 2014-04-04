#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

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
	cellSailRec.AddFunc(0xe14cae97, cellSailProfileSetEsAudioParameter);
	cellSailRec.AddFunc(0x1422a425, cellSailProfileSetEsVideoParameter);
	cellSailRec.AddFunc(0xe8d86c43, cellSailProfileSetStreamParameter);

	cellSailRec.AddFunc(0xb3d30b0d, cellSailVideoConverterCanProcess);
	cellSailRec.AddFunc(0x855da8c6, cellSailVideoConverterProcess);
	cellSailRec.AddFunc(0xe16de678, cellSailVideoConverterCanGetResult);
	cellSailRec.AddFunc(0xe15679fe, cellSailVideoConverterGetResult);
	//cellSailRec.AddFunc(, CellSailVideoConverterFuncProcessDone);

	cellSailRec.AddFunc(0xbd591197, cellSailFeederAudioInitialize);
	cellSailRec.AddFunc(0x899d1587, cellSailFeederAudioFinalize);
	cellSailRec.AddFunc(0xc2e2f30d, cellSailFeederAudioNotifyCallCompleted);
	cellSailRec.AddFunc(0x3c775cea, cellSailFeederAudioNotifyFrameOut);
	cellSailRec.AddFunc(0x999c0dc5, cellSailFeederAudioNotifySessionEnd);
	cellSailRec.AddFunc(0xaf310ae6, cellSailFeederAudioNotifySessionError);

	cellSailRec.AddFunc(0x57415dd3, cellSailFeederVideoInitialize);
	cellSailRec.AddFunc(0x81bfeae8, cellSailFeederVideoFinalize);
	cellSailRec.AddFunc(0xd84daeb9, cellSailFeederVideoNotifyCallCompleted);
	cellSailRec.AddFunc(0xe5e0572a, cellSailFeederVideoNotifyFrameOut);
	cellSailRec.AddFunc(0xbff6e8d3, cellSailFeederVideoNotifySessionEnd);
	cellSailRec.AddFunc(0x86cae679, cellSailFeederVideoNotifySessionError);

	cellSailRec.AddFunc(0x7a52bf69, cellSailRecorderInitialize);
	cellSailRec.AddFunc(0xf57d74e3, cellSailRecorderFinalize);
	cellSailRec.AddFunc(0x3deae857, cellSailRecorderSetFeederAudio);
	cellSailRec.AddFunc(0x4fec43a9, cellSailRecorderSetFeederVideo);
	cellSailRec.AddFunc(0x0a3ea2a9, cellSailRecorderSetParameter);
	cellSailRec.AddFunc(0xff20157b, cellSailRecorderGetParameter);
	//cellSailRec.AddFunc(, cellSailRecorderSubscribeEvent);
	//cellSailRec.AddFunc(, cellSailRecorderUnsubscribeEvent);
	//cellSailRec.AddFunc(, cellSailRecorderReplaceEventHandler);
	cellSailRec.AddFunc(0xc4617ddc, cellSailRecorderBoot);
	cellSailRec.AddFunc(0x50affdc1, cellSailRecorderCreateProfile);
	cellSailRec.AddFunc(0x376c3926, cellSailRecorderDestroyProfile);
	cellSailRec.AddFunc(0x49476a3d, cellSailRecorderCreateVideoConverter);
	cellSailRec.AddFunc(0x455c4709, cellSailRecorderDestroyVideoConverter);
	cellSailRec.AddFunc(0x10c81457, cellSailRecorderOpenStream);
	cellSailRec.AddFunc(0xe3f56f62, cellSailRecorderCloseStream);
	cellSailRec.AddFunc(0x4830faf8, cellSailRecorderStart);
	cellSailRec.AddFunc(0x18ecc741, cellSailRecorderStop);
	cellSailRec.AddFunc(0xd37fb694, cellSailRecorderCancel);

	cellSailRec.AddFunc(0x37aad85f, cellSailRecorderDumpImage);
}