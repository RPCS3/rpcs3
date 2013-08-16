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
	cellGcmSys.AddFunc(0x055bd74d, cellGcmGetTiledPitchSize);
	cellGcmSys.AddFunc(0x15bae46b, cellGcmInit);
	cellGcmSys.AddFunc(0x21397818, cellGcmFlush);
	cellGcmSys.AddFunc(0x21ac3697, cellGcmAddressToOffset);
	cellGcmSys.AddFunc(0x3a33c1fd, cellGcmFunc15);
	cellGcmSys.AddFunc(0x4ae8d215, cellGcmSetFlipMode);
	cellGcmSys.AddFunc(0x5e2ee0f0, cellGcmGetDefaultCommandWordSize);
	cellGcmSys.AddFunc(0x72a577ce, cellGcmGetFlipStatus);
	cellGcmSys.AddFunc(0x8cdf8c70, cellGcmGetDefaultSegmentWordSize);
	cellGcmSys.AddFunc(0x9ba451e4, cellGcmSetDefaultFifoSize);
	cellGcmSys.AddFunc(0xa53d12ae, cellGcmSetDisplayBuffer);
	cellGcmSys.AddFunc(0xa547adde, cellGcmGetControlRegister);
	cellGcmSys.AddFunc(0xb2e761d4, cellGcmResetFlipStatus);
	cellGcmSys.AddFunc(0xd8f88e1a, cellGcmSetFlipCommandWithWaitLabel);
	cellGcmSys.AddFunc(0xe315a0b2, cellGcmGetConfiguration);
	cellGcmSys.AddFunc(0x9dc04436, cellGcmBindZcull);
	cellGcmSys.AddFunc(0x5a41c10f, cellGcmGetTimeStamp);
	cellGcmSys.AddFunc(0xd9b7653e, cellGcmUnbindTile);
	cellGcmSys.AddFunc(0xa75640e8, cellGcmUnbindZcull);
	cellGcmSys.AddFunc(0xa41ef7e8, cellGcmSetFlipHandler);
}
