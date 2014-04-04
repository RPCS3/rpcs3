#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellMic_init();
Module cellMic(0x0022, cellMic_init);

// Error Codes
enum
{
	CELL_MICIN_ERROR_ALREADY_INIT       = 0x80140101,
	CELL_MICIN_ERROR_SYSTEM             = 0x80140102,
	CELL_MICIN_ERROR_NOT_INIT           = 0x80140103,
	CELL_MICIN_ERROR_PARAM              = 0x80140104,
	CELL_MICIN_ERROR_PORT_FULL          = 0x80140105,
	CELL_MICIN_ERROR_ALREADY_OPEN       = 0x80140106,
	CELL_MICIN_ERROR_NOT_OPEN           = 0x80140107,
	CELL_MICIN_ERROR_NOT_RUN            = 0x80140108,
	CELL_MICIN_ERROR_TRANS_EVENT        = 0x80140109,
	CELL_MICIN_ERROR_OPEN               = 0x8014010a,
	CELL_MICIN_ERROR_SHAREDMEMORY       = 0x8014010b,
	CELL_MICIN_ERROR_MUTEX              = 0x8014010c,
	CELL_MICIN_ERROR_EVENT_QUEUE        = 0x8014010d,
	CELL_MICIN_ERROR_DEVICE_NOT_FOUND   = 0x8014010e,
	CELL_MICIN_ERROR_SYSTEM_NOT_FOUND   = 0x8014010e,
	CELL_MICIN_ERROR_FATAL              = 0x8014010f,
	CELL_MICIN_ERROR_DEVICE_NOT_SUPPORT = 0x80140110,
};

int cellMicInit()
{
	UNIMPLEMENTED_FUNC(cellMic);
	return CELL_OK;
}

int cellMicEnd()
{
	UNIMPLEMENTED_FUNC(cellMic);
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

void cellMic_init()
{
	cellMic.AddFunc(0x8325e02d, cellMicInit);
	cellMic.AddFunc(0xc6328caa, cellMicEnd);
	cellMic.AddFunc(0xdd1b59f0, cellMicOpen);
	cellMic.AddFunc(0x8d229f8e, cellMicClose);

	cellMic.AddFunc(0x017024a8, cellMicGetDeviceGUID);
	cellMic.AddFunc(0xa52d2ae4, cellMicGetType);
	cellMic.AddFunc(0x1b42101b, cellMicIsAttached);
	cellMic.AddFunc(0x186cb1fb, cellMicIsOpen);
	cellMic.AddFunc(0x6a024aa0, cellMicGetDeviceAttr);
	cellMic.AddFunc(0xb2c16321, cellMicSetDeviceAttr);
	cellMic.AddFunc(0xac5ba03a, cellMicGetSignalAttr);
	cellMic.AddFunc(0x323deb41, cellMicSetSignalAttr);
	cellMic.AddFunc(0xb30780eb, cellMicGetSignalState);

	cellMic.AddFunc(0xdd724314, cellMicStart);
	cellMic.AddFunc(0x07e1b12c, cellMicRead);
	cellMic.AddFunc(0xfcfaf246, cellMicStop);
	cellMic.AddFunc(0x6bc46aab, cellMicReset);

	cellMic.AddFunc(0x7903400e, cellMicSetNotifyEventQueue);
	cellMic.AddFunc(0x6cc7ae00, cellMicSetNotifyEventQueue2);
	cellMic.AddFunc(0x65336418, cellMicRemoveNotifyEventQueue);

	cellMic.AddFunc(0x05709bbf, cellMicOpenEx);
	cellMic.AddFunc(0xddd19a89, cellMicStartEx);
	cellMic.AddFunc(0x4e0b69ee, cellMicGetFormatRaw);
	cellMic.AddFunc(0xfda12276, cellMicGetFormatAux);
	cellMic.AddFunc(0x87a08d29, cellMicGetFormatDsp);
	cellMic.AddFunc(0xa42ac07a, cellMicOpenRaw);
	cellMic.AddFunc(0x72165a7f, cellMicReadRaw);
	cellMic.AddFunc(0x3acc118e, cellMicReadAux);
	cellMic.AddFunc(0xc414faa5, cellMicReadDsp);

	cellMic.AddFunc(0x25c5723f, cellMicGetStatus);
	cellMic.AddFunc(0xe839380f, cellMicStopEx);
	cellMic.AddFunc(0x3ace58f3, cellMicSysShareClose);
	cellMic.AddFunc(0x48108a23, cellMicGetFormat);
	cellMic.AddFunc(0x891c6291, cellMicSetMultiMicNotifyEventQueue);
	cellMic.AddFunc(0xad049ecf, cellMicGetFormatEx);
	cellMic.AddFunc(0xbdfd51e2, cellMicSysShareStop);
	cellMic.AddFunc(0xc3610dbd, cellMicSysShareOpen);
	cellMic.AddFunc(0xc461563c, cellMicCommand);
	cellMic.AddFunc(0xcac7e7d7, cellMicSysShareStart);
	cellMic.AddFunc(0xd127cd3e, cellMicSysShareInit);
	cellMic.AddFunc(0xf82bbf7c, cellMicSysShareEnd);
	cellMic.AddFunc(0xfdbbe469, cellMicGetDeviceIdentifier);
}