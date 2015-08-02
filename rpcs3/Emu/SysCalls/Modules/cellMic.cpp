#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "cellMic.h"

extern Module cellMic;

s32 cellMicInit()
{
	cellMic.Warning("cellMicInit()");

	return CELL_OK;
}

s32 cellMicEnd()
{
	cellMic.Warning("cellMicEnd()");

	return CELL_OK;
}

s32 cellMicOpen()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicClose()
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

s32 cellMicIsAttached()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicIsOpen()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicGetDeviceAttr()
{
	UNIMPLEMENTED_FUNC(cellMic);
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

s32 cellMicGetSignalState()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicStart()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicRead()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicStop()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicReset()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicSetNotifyEventQueue()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicSetNotifyEventQueue2()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicRemoveNotifyEventQueue()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicOpenEx()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicStartEx()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicGetFormatRaw()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicGetFormatAux()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicGetFormatDsp()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicOpenRaw()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicReadRaw()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
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

s32 cellMicGetStatus()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

s32 cellMicStopEx()
{
	throw EXCEPTION("Unexpected function");
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

Module cellMic("cellMic", []()
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
