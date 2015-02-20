#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "cellMic.h"

extern Module cellMic;

struct cellMicInternal
{
	bool m_bCellMicInitialized;;

	cellMicInternal()
		: m_bCellMicInitialized(false)
	{
	}
};

cellMicInternal CellMicInstance;

int cellMicInit()
{
	cellMic.Warning("cellMicInit()");

	if (CellMicInstance.m_bCellMicInitialized)
		return CELL_MICIN_ERROR_ALREADY_INIT;

	CellMicInstance.m_bCellMicInitialized = true;

	return CELL_OK;
}

int cellMicEnd()
{
	cellMic.Warning("cellMicEnd()");

	if (!CellMicInstance.m_bCellMicInitialized)
		return CELL_MICIN_ERROR_NOT_INIT;

	CellMicInstance.m_bCellMicInitialized = false;

	return CELL_OK;
}

int cellMicOpen()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicClose()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicGetDeviceGUID()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicGetType()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicIsAttached()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicIsOpen()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicGetDeviceAttr()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicSetDeviceAttr()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicGetSignalAttr()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicSetSignalAttr()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicGetSignalState()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicStart()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicRead()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicStop()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicReset()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicSetNotifyEventQueue()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicSetNotifyEventQueue2()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicRemoveNotifyEventQueue()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicOpenEx()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicStartEx()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicGetFormatRaw()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicGetFormatAux()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicGetFormatDsp()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicOpenRaw()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicReadRaw()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicReadAux()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicReadDsp()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicGetStatus()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicStopEx()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicSysShareClose()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicGetFormat()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicSetMultiMicNotifyEventQueue()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicGetFormatEx()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicSysShareStop()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicSysShareOpen()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicCommand()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicSysShareStart()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicSysShareInit()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicSysShareEnd()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicGetDeviceIdentifier()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

void cellMic_unload()
{
	CellMicInstance.m_bCellMicInitialized = false;
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
	REG_FUNC(cellMic, cellMicStopEx);
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
