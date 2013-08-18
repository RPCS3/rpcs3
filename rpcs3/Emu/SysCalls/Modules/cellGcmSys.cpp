#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "Emu/GS/GCM.h"

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

int cellGcmInitCursor()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

int cellGcmSetCursorPosition(s32 x, s32 y)
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

int cellGcmSetCursorDisable()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

int cellGcmSetVBlankHandler()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

int cellGcmUpdateCursor()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

int cellGcmSetCursorEnable()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

int cellGcmSetCursorImageOffset(u32 offset)
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

u32 cellGcmGetDisplayInfo()
{
	cellGcmSys.Warning("cellGcmGetDisplayInfo() = 0x%x", Emu.GetGSManager().GetRender().m_gcm_buffers_addr);
	return Emu.GetGSManager().GetRender().m_gcm_buffers_addr;
}

int cellGcmGetCurrentDisplayBufferId(u32 id_addr)
{
	cellGcmSys.Warning("cellGcmGetCurrentDisplayBufferId(id_addr=0x%x)", id_addr);

	if(!Memory.IsGoodAddr(id_addr))
	{
		return CELL_EFAULT;
	}

	Memory.Write32(id_addr, Emu.GetGSManager().GetRender().m_gcm_current_buffer);

	return CELL_OK;
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
	cellGcmSys.AddFunc(0xa114ec67, cellGcmMapMainMemory);
	cellGcmSys.AddFunc(0xf80196c1, cellGcmGetLabelAddress);
	cellGcmSys.AddFunc(0x107bf3a1, cellGcmInitCursor);
	cellGcmSys.AddFunc(0x1a0de550, cellGcmSetCursorPosition);
	cellGcmSys.AddFunc(0x69c6cc82, cellGcmSetCursorDisable);
	cellGcmSys.AddFunc(0xa91b0402, cellGcmSetVBlankHandler);
	cellGcmSys.AddFunc(0xbd2fa0a7, cellGcmUpdateCursor);
	cellGcmSys.AddFunc(0xc47d0812, cellGcmSetCursorEnable);
	cellGcmSys.AddFunc(0xf9bfdc72, cellGcmSetCursorImageOffset);
	cellGcmSys.AddFunc(0x0e6b0dae, cellGcmGetDisplayInfo);
	cellGcmSys.AddFunc(0x93806525, cellGcmGetCurrentDisplayBufferId);
}
