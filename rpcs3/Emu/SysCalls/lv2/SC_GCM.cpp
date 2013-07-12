#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/GS/GCM.h"

extern Module cellGcmSys;

CellGcmConfig current_config;
CellGcmContextData current_context;
gcmInfo gcm_info;
struct gcm_offset
{
	u16 ea;
	u16 offset;
};

u32 map_offset_addr = 0;
u32 map_offset_pos = 0;

int cellGcmMapMainMemory(u32 address, u32 size, u32 offset_addr)
{
	cellGcmSys.Warning("cellGcmMapMainMemory(address=0x%x,size=0x%x,offset_addr=0x%x)", address, size, offset_addr);
	if(!Memory.IsGoodAddr(offset_addr, sizeof(u32))) return CELL_EFAULT;
	Memory.Write32(offset_addr, address & 0xffff);

	return CELL_OK;
}

int cellGcmInit(u32 context_addr, u32 cmdSize, u32 ioSize, u32 ioAddress)
{
	cellGcmSys.Log("cellGcmInit(context_addr=0x%x,cmdSize=0x%x,ioSize=0x%x,ioAddress=0x%x)", context_addr, cmdSize, ioSize, ioAddress);

	const u32 local_size = 0xf900000; //TODO
	const u32 local_addr = Memory.RSXFBMem.GetStartAddr();

	map_offset_addr = 0;
	map_offset_pos = 0;
	current_config.ioSize = re32(ioSize);
	current_config.ioAddress = re32(ioAddress);
	current_config.localSize = re32(local_size);
	current_config.localAddress = re32(local_addr);
	current_config.memoryFrequency = re32(650000000);
	current_config.coreFrequency = re32(500000000);

	Memory.RSXFBMem.Alloc(local_size);
	Memory.RSXCMDMem.Alloc(cmdSize);

	u32 ctx_begin = ioAddress + 0x1000;
	u32 ctx_size = 0x6ffc;
	current_context.begin = re(ctx_begin);
	current_context.end = re(ctx_begin + ctx_size);
	current_context.current = current_context.begin;
	current_context.callback = re32(Emu.GetRSXCallback() - 4);

	gcm_info.context_addr = Memory.MainMem.Alloc(0x1000);
	gcm_info.control_addr = gcm_info.context_addr + 0x40;

	Memory.WriteData(gcm_info.context_addr, current_context);
	Memory.Write32(context_addr, gcm_info.context_addr);

	CellGcmControl& ctrl = (CellGcmControl&)Memory[gcm_info.control_addr];
	ctrl.put = 0;
	ctrl.get = 0;
	ctrl.ref = -1;

	Emu.GetGSManager().GetRender().Init(ctx_begin, ctx_size, gcm_info.control_addr, local_addr);

	return CELL_OK;
}

int cellGcmCallback(u32 context_addr, u32 count)
{
	GSLockCurrent gslock(GS_LOCK_WAIT_FLUSH);

	CellGcmContextData& ctx = (CellGcmContextData&)Memory[context_addr];
	CellGcmControl& ctrl = (CellGcmControl&)Memory[gcm_info.control_addr];

	const s32 res = re(ctx.current) - re(ctx.begin) - re(ctrl.put);

	if(res > 0) memcpy(&Memory[re(ctx.begin)], &Memory[re(ctx.current) - res], res);

	ctx.current = re(re(ctx.begin) + res);

	ctrl.put = re(res);
	ctrl.get = 0;
	
	return CELL_OK;
}

int cellGcmGetConfiguration(u32 config_addr)
{
	cellGcmSys.Log("cellGcmGetConfiguration(config_addr=0x%x)", config_addr);

	if(!Memory.IsGoodAddr(config_addr, sizeof(CellGcmConfig))) return CELL_EFAULT;

	Memory.WriteData(config_addr, current_config);
	return CELL_OK;
}

int cellGcmAddressToOffset(u32 address, u32 offset_addr)
{
	cellGcmSys.Log("cellGcmAddressToOffset(address=0x%x,offset_addr=0x%x)", address, offset_addr);
	if(!Memory.IsGoodAddr(offset_addr, sizeof(u32))) return CELL_EFAULT;
	
	if(!map_offset_addr)
	{
		map_offset_addr = Memory.Alloc(4*50, 4);
	}

	u32 sa = Memory.RSXFBMem.IsInMyRange(address) ? Memory.RSXFBMem.GetStartAddr() : re(current_context.begin);
	u16 ea = (sa + address - sa) >> 16;
	u32 offset = address - sa;
	//Memory.Write16(map_offset_addr + map_offset_pos + 0, ea);
	//Memory.Write16(map_offset_addr + map_offset_pos + 2, offset);
	//map_offset_pos += 4;

	Memory.Write32(offset_addr, offset);

	return CELL_OK;
}

int cellGcmSetDisplayBuffer(u32 id, u32 offset, u32 pitch, u32 width, u32 height)
{
	cellGcmSys.Warning("cellGcmSetDisplayBuffer(id=0x%x,offset=0x%x,pitch=%d,width=%d,height=%d)",
		id, offset, width ? pitch/width : pitch, width, height);
	if(id > 1) return CELL_EINVAL;

	gcmBuffers[id].offset = offset;
	gcmBuffers[id].pitch = pitch;
	gcmBuffers[id].width = width;
	gcmBuffers[id].height = height;
	gcmBuffers[id].update = true;

	return CELL_OK;
}

u32 cellGcmGetLabelAddress(u8 index)
{
	cellGcmSys.Error("cellGcmGetLabelAddress(index=%d)", index);
	return Memory.RSXCMDMem.GetStartAddr() + 0x10 * index;
}

u32 cellGcmGetControlRegister()
{
	cellGcmSys.Log("cellGcmGetControlRegister()");

	return gcm_info.control_addr;
}

int cellGcmFlush(u32 ctx, u32 id)
{
	cellGcmSys.Log("cellGcmFlush(ctx=0x%x, id=0x%x)", ctx, id);
	if(id > 1) return CELL_EINVAL;

	Emu.GetGSManager().GetRender().Draw();

	return CELL_OK;
}

void cellGcmSetTile(u32 index, u32 location, u32 offset, u32 size, u32 pitch, u32 comp, u32 base, u32 bank)
{
	cellGcmSys.Warning("cellGcmSetTile(index=%d, location=%d, offset=0x%x, size=0x%x, pitch=0x%x, comp=0x%x, base=0x%x, bank=0x%x)",
		index, location, offset, size, pitch, comp, base, bank);

	//return CELL_OK;
}

int cellGcmBindTile(u32 index)
{
	cellGcmSys.Warning("TODO: cellGcmBindTile(index=%d)", index);
	
	return CELL_OK;
}

int cellGcmBindZcull(u32 index, u32 offset, u32 width, u32 height, u32 cullStart, u32 zFormat, u32 aaFormat, u32 zCullDir, u32 zCullFormat, u32 sFunc, u32 sRef, u32 sMask)
{
	cellGcmSys.Warning("TODO: cellGcmBindZcull(index=%d, offset=0x%x, width=%d, height=%d, cullStart=0x%x, zFormat=0x%x, aaFormat=0x%x, zCullDir=0x%x, zCullFormat=0x%x, sFunc=0x%x, sRef=0x%x, sMask=0x%x)", index, offset, width, height, cullStart, zFormat, aaFormat, zCullDir, zCullFormat, sFunc, sRef, sMask);
	
	return CELL_OK;
}

int cellGcmGetFlipStatus()
{
	return Emu.GetGSManager().GetRender().m_flip_status;
}

int cellGcmResetFlipStatus()
{
	Emu.GetGSManager().GetRender().m_flip_status = 1;

	return CELL_OK;
}

int cellGcmSetFlipMode(u32 mode)
{
	cellGcmSys.Warning("cellGcmSetFlipMode(mode=%d)", mode);

	switch(mode)
	{
	case CELL_GCM_DISPLAY_HSYNC:
	case CELL_GCM_DISPLAY_VSYNC:
	case CELL_GCM_DISPLAY_HSYNC_WITH_NOISE:
		Emu.GetGSManager().GetRender().m_flip_mode = mode;
	break;

	default:
		return CELL_EINVAL;
	}

	return CELL_OK;
}

u32 cellGcmGetTiledPitchSize(u32 size)
{
	//TODO
	cellGcmSys.Warning("cellGcmGetTiledPitchSize(size=%d)", size);

	return size;
}

u32 cellGcmGetDefaultCommandWordSize()
{
	cellGcmSys.Warning("cellGcmGetDefaultCommandWordSize()");
	return 0x400;
}

u32 cellGcmGetDefaultSegmentWordSize()
{
	cellGcmSys.Warning("cellGcmGetDefaultSegmentWordSize()");
	return 0x100;
}

int cellGcmSetDefaultFifoSize(u32 bufferSize, u32 segmentSize)
{
	cellGcmSys.Warning("cellGcmSetDefaultFifoSize(bufferSize=0x%x, segmentSize=0x%x)", bufferSize, segmentSize);
	return CELL_OK;
}
