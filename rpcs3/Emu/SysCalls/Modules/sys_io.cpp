#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void sys_io_init();
Module sys_io(0x0017, sys_io_init);

void sys_io_init()
{
	sys_io.AddFunc(0x1cf98800, cellPadInit);
	sys_io.AddFunc(0x4d9b75d5, cellPadEnd);
	sys_io.AddFunc(0x0d5f2c14, cellPadClearBuf);
	sys_io.AddFunc(0x8b72cda1, cellPadGetData);
	sys_io.AddFunc(0x6bc09c61, cellPadGetDataExtra);
	sys_io.AddFunc(0xf65544ee, cellPadSetActDirect);
	sys_io.AddFunc(0x3aaad464, cellPadGetInfo);
	sys_io.AddFunc(0xa703a51d, cellPadGetInfo2);
	sys_io.AddFunc(0x578e3c98, cellPadSetPortSetting);
	sys_io.AddFunc(0x0e2dfaad, cellPadInfoPressMode);
	sys_io.AddFunc(0x78200559, cellPadInfoSensorMode);
	sys_io.AddFunc(0xf83f8182, cellPadSetPressMode);
	sys_io.AddFunc(0xbe5be3ba, cellPadSetSensorMode);

	sys_io.AddFunc(0x433f6ec0, cellKbInit);
	sys_io.AddFunc(0xbfce3285, cellKbEnd);
	sys_io.AddFunc(0x2073b7f6, cellKbClearBuf);
	sys_io.AddFunc(0x4ab1fa77, cellKbCnvRawCode);
	sys_io.AddFunc(0x2f1774d5, cellKbGetInfo);
	sys_io.AddFunc(0xff0a21b7, cellKbRead);
	sys_io.AddFunc(0xa5f85e4d, cellKbSetCodeType);
	sys_io.AddFunc(0x3f72c56e, cellKbSetLEDStatus);
	sys_io.AddFunc(0xdeefdfa7, cellKbSetReadMode);
	sys_io.AddFunc(0x1f71ecbe, cellKbGetConfiguration);

	sys_io.AddFunc(0xc9030138, cellMouseInit);
	sys_io.AddFunc(0x3ef66b95, cellMouseClearBuf);
	sys_io.AddFunc(0xe10183ce, cellMouseEnd);
	sys_io.AddFunc(0x5baf30fb, cellMouseGetInfo);
	sys_io.AddFunc(0x4d0b3b1f, cellMouseInfoTabletMode);
	sys_io.AddFunc(0x3138e632, cellMouseGetData);
	sys_io.AddFunc(0x6bd131f0, cellMouseGetDataList);
	sys_io.AddFunc(0x2d16da4f, cellMouseSetTabletMode);
	sys_io.AddFunc(0x21a62e9b, cellMouseGetTabletDataList);
	sys_io.AddFunc(0xa328cc35, cellMouseGetRawData);
}
