#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellGcmSys_init();
Module cellGcmSys(0x0010, cellGcmSys_init);

s64 cellGcmFunc15()
{
	cellGcmSys.Error("cellGcmFunc15()");
	return 0;
}

s64 cellGcmSetFlipCommandWithWaitLabel()
{
	cellGcmSys.Error("cellGcmSetFlipCommandWithWaitLabel()");
	return 0;
}

void cellGcmSys_init()
{
	cellGcmSys.AddFunc(0x055bd74d, bind_func(cellGcmGetTiledPitchSize));
	cellGcmSys.AddFunc(0x15bae46b, bind_func(cellGcmInit));
	cellGcmSys.AddFunc(0x21397818, bind_func(cellGcmFlush));
	cellGcmSys.AddFunc(0x21ac3697, bind_func(cellGcmAddressToOffset));
	cellGcmSys.AddFunc(0x3a33c1fd, bind_func(cellGcmFunc15));
	cellGcmSys.AddFunc(0x4ae8d215, bind_func(cellGcmSetFlipMode));
	cellGcmSys.AddFunc(0x5e2ee0f0, bind_func(cellGcmGetDefaultCommandWordSize));
	cellGcmSys.AddFunc(0x72a577ce, bind_func(cellGcmGetFlipStatus));
	cellGcmSys.AddFunc(0x8cdf8c70, bind_func(cellGcmGetDefaultSegmentWordSize));
	cellGcmSys.AddFunc(0x9ba451e4, bind_func(cellGcmSetDefaultFifoSize));
	cellGcmSys.AddFunc(0xa53d12ae, bind_func(cellGcmSetDisplayBuffer));
	cellGcmSys.AddFunc(0xa547adde, bind_func(cellGcmGetControlRegister));
	cellGcmSys.AddFunc(0xb2e761d4, bind_func(cellGcmResetFlipStatus));
	cellGcmSys.AddFunc(0xd8f88e1a, bind_func(cellGcmSetFlipCommandWithWaitLabel));
	cellGcmSys.AddFunc(0xe315a0b2, bind_func(cellGcmGetConfiguration));
	cellGcmSys.AddFunc(0x9dc04436, bind_func(cellGcmBindZcull));
}
