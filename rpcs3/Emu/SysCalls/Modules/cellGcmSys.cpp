#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "Emu/GS/GCM.h"

void cellGcmSys_init();
Module cellGcmSys(0x0010, cellGcmSys_init);

enum
{
	CELL_GCM_ERROR_FAILURE				= 0x802100ff,
	CELL_GCM_ERROR_INVALID_ENUM			= 0x80210002,
	CELL_GCM_ERROR_INVALID_VALUE		= 0x80210003,
	CELL_GCM_ERROR_INVALID_ALIGNMENT	= 0x80210004,
};

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

int cellGcmMapMainMemory(u32 address, u32 size, mem32_t offset)
{
	cellGcmSys.Warning("cellGcmMapMainMemory(address=0x%x,size=0x%x,offset_addr=0x%x)", address, size, offset.GetAddr());
	if(!offset.IsGood()) return CELL_EFAULT;

	Emu.GetGSManager().GetRender().m_main_mem_addr = Emu.GetGSManager().GetRender().m_ioAddress;

	offset = address - Emu.GetGSManager().GetRender().m_main_mem_addr;
	Emu.GetGSManager().GetRender().m_main_mem_info.AddCpy(MemInfo(address, size));

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

	u32 ctx_begin = ioAddress/* + 0x1000*/;
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

	auto& render = Emu.GetGSManager().GetRender();
	render.m_ctxt_addr = context_addr;
	render.m_gcm_buffers_addr = Memory.Alloc(sizeof(gcmBuffer) * 8, sizeof(gcmBuffer));
	render.m_zculls_addr = Memory.Alloc(sizeof(CellGcmZcullInfo) * 8, sizeof(CellGcmZcullInfo));
	render.m_tiles_addr = Memory.Alloc(sizeof(CellGcmTileInfo) * 15, sizeof(CellGcmTileInfo));
	render.m_gcm_buffers_count = 0;
	render.m_gcm_current_buffer = 0;
	render.m_main_mem_info.Clear();
	render.m_main_mem_addr = 0;
	render.Init(ctx_begin, ctx_size, gcm_info.control_addr, local_addr);

	return CELL_OK;
}

int cellGcmGetConfiguration(mem_ptr_t<CellGcmConfig> config)
{
	cellGcmSys.Log("cellGcmGetConfiguration(config_addr=0x%x)", config.GetAddr());

	if(!config.IsGood()) return CELL_EFAULT;

	*config = current_config;

	return CELL_OK;
}

int cellGcmAddressToOffset(u32 address, mem32_t offset)
{
	cellGcmSys.Log("cellGcmAddressToOffset(address=0x%x,offset_addr=0x%x)", address, offset.GetAddr());
	if(!offset.IsGood()) return CELL_EFAULT;
	
	if(!map_offset_addr)
	{
		map_offset_addr = Memory.Alloc(4*50, 4);
	}

	u32 sa;
	bool is_main_mem = false;
	const auto& main_mem_info = Emu.GetGSManager().GetRender().m_main_mem_info;
	for(u32 i=0; i<Emu.GetGSManager().GetRender().m_main_mem_info.GetCount(); ++i)
	{
		//main memory
		if(address >= main_mem_info[i].addr && address < main_mem_info[i].addr + main_mem_info[i].size)
		{
			is_main_mem = true;
			break;
		}
	}

	if(is_main_mem)
	{
		//main
		sa = Emu.GetGSManager().GetRender().m_main_mem_addr;
		//ConLog.Warning("cellGcmAddressToOffset: main memory: offset = 0x%x - 0x%x = 0x%x", address, sa, address - sa);
	}
	else if(Memory.RSXFBMem.IsMyAddress(address))
	{
		//local
		sa = Emu.GetGSManager().GetRender().m_local_mem_addr;
		//ConLog.Warning("cellGcmAddressToOffset: local memory: offset = 0x%x - 0x%x = 0x%x", address, sa, address - sa);
	}
	else
	{
		//io
		sa = Emu.GetGSManager().GetRender().m_ioAddress;
		//ConLog.Warning("cellGcmAddressToOffset: io memory: offset = 0x%x - 0x%x = 0x%x", address, sa, address - sa);
	}

	offset = address - sa;
	//Memory.Write16(map_offset_addr + map_offset_pos + 0, ea);
	//Memory.Write16(map_offset_addr + map_offset_pos + 2, offset);
	//map_offset_pos += 4;

	return CELL_OK;
}

int cellGcmSetDisplayBuffer(u32 id, u32 offset, u32 pitch, u32 width, u32 height)
{
	cellGcmSys.Log("cellGcmSetDisplayBuffer(id=0x%x,offset=0x%x,pitch=%d,width=%d,height=%d)",
		id, offset, width ? pitch/width : pitch, width, height);
	if(id > 7) return CELL_EINVAL;

	gcmBuffer* buffers = (gcmBuffer*)Memory.GetMemFromAddr(Emu.GetGSManager().GetRender().m_gcm_buffers_addr);

	buffers[id].offset = re(offset);
	buffers[id].pitch = re(pitch);
	buffers[id].width = re(width);
	buffers[id].height = re(height);

	if(id + 1 > Emu.GetGSManager().GetRender().m_gcm_buffers_count)
	{
		Emu.GetGSManager().GetRender().m_gcm_buffers_count = id + 1;
	}

	return CELL_OK;
}

u32 cellGcmGetLabelAddress(u8 index)
{
	cellGcmSys.Log("cellGcmGetLabelAddress(index=%d)", index);
	return Memory.RSXCMDMem.GetStartAddr() + 0x10 * index;
}

u32 cellGcmGetControlRegister()
{
	cellGcmSys.Log("cellGcmGetControlRegister()");

	return gcm_info.control_addr;
}

int cellGcmSetPrepareFlip(mem_ptr_t<CellGcmContextData> ctxt, u32 id)
{
	cellGcmSys.Log("cellGcmSetPrepareFlip(ctx=0x%x, id=0x%x)", ctxt.GetAddr(), id);

	if(id >= 8)
	{
		return CELL_GCM_ERROR_FAILURE;
	}

	GSLockCurrent gslock(GS_LOCK_WAIT_FLUSH);

	u32 current = re(ctxt->current);
	u32 end = re(ctxt->end);

	if(current + 8 >= end)
	{
		ConLog.Warning("bad flip!");
		cellGcmCallback(ctxt.GetAddr(), current + 8 - end);
	}

	current = re(ctxt->current);
	Memory.Write32(current, 0x3fead | (1 << 18));
	Memory.Write32(current + 4, id);
	re(ctxt->current, current + 8);

	if(ctxt.GetAddr() == gcm_info.context_addr)
	{
		CellGcmControl& ctrl = (CellGcmControl&)Memory[gcm_info.control_addr];
		re(ctrl.put, re(ctrl.put) + 8);
	}

	return id;
}

int cellGcmSetFlipCommand(u32 ctx, u32 id)
{
	return cellGcmSetPrepareFlip(ctx, id);
}

int cellGcmSetZcull(u8 index, u32 offset, u32 width, u32 height, u32 cullStart, u32 zFormat, u32 aaFormat, u32 zCullDir, u32 zCullFormat, u32 sFunc, u32 sRef, u32 sMask)
{
	cellGcmSys.Warning("TODO: cellGcmSetZcull(index=%d, offset=0x%x, width=%d, height=%d, cullStart=0x%x, zFormat=0x%x, aaFormat=0x%x, zCullDir=0x%x, zCullFormat=0x%x, sFunc=0x%x, sRef=0x%x, sMask=0x%x)",
		index, offset, width, height, cullStart, zFormat, aaFormat, zCullDir, zCullFormat, sFunc, sRef, sMask);

	return CELL_OK;
}

int cellGcmBindZcull(u8 index)
{
	cellGcmSys.Warning("TODO: cellGcmBindZcull(index=%d)", index);

	return CELL_OK;
}

u32 cellGcmGetZcullInfo()
{
	cellGcmSys.Warning("cellGcmGetZcullInfo()");
	return Emu.GetGSManager().GetRender().m_zculls_addr;
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

u32 cellGcmSetUserHandler(u32 handler)
{
	cellGcmSys.Warning("cellGcmSetUserHandler(handler=0x%x)", handler);
	return handler;
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

int cellGcmMapEaIoAddress(const u32 ea, const u32 io, const u32 size)
{
	cellGcmSys.Warning("cellGcmMapEaIoAddress(ea=0x%x, io=0x%x, size=0x%x)", ea, io, size);
	Memory.Map(io, ea, size);
	Emu.GetGSManager().GetRender().m_report_main_addr = ea;
	return CELL_OK;
}

int cellGcmUnbindZcull(u8 index)
{
	cellGcmSys.Warning("cellGcmUnbindZcull(index=%d)", index);
	if(index >= 8)
		return CELL_EINVAL;

	return CELL_OK;
}

u64 cellGcmGetTimeStamp(u32 index)
{
	cellGcmSys.Log("cellGcmGetTimeStamp(index=%d)", index);
	return Memory.Read64(Memory.RSXFBMem.GetStartAddr() + index * 0x10);
}

int cellGcmSetFlipHandler(u32 handler_addr)
{
	cellGcmSys.Warning("cellGcmSetFlipHandler(handler_addr=%d)", handler_addr);
	if(handler_addr != 0 && !Memory.IsGoodAddr(handler_addr))
	{
		return CELL_EFAULT;
	}

	Emu.GetGSManager().GetRender().m_flip_handler.SetAddr(handler_addr);
	return CELL_OK;
}

s64 cellGcmFunc15()
{
	cellGcmSys.Error("cellGcmFunc15()");
	return 0;
}

int cellGcmSetFlipCommandWithWaitLabel(u32 ctx, u32 id, u32 label_index, u32 label_value)
{
	int res = cellGcmSetPrepareFlip(ctx, id);
	Memory.Write32(Memory.RSXCMDMem.GetStartAddr() + 0x10 * label_index, label_value);
	return res == CELL_GCM_ERROR_FAILURE ? CELL_GCM_ERROR_FAILURE : CELL_OK;
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

void cellGcmSetDefaultCommandBuffer()
{
	cellGcmSys.Warning("cellGcmSetDefaultCommandBuffer()");
	Memory.Write32(Emu.GetGSManager().GetRender().m_ctxt_addr, gcm_info.context_addr);
}

void cellGcmSetFlipStatus()
{
	cellGcmSys.Warning("cellGcmSetFlipStatus()");
	
	Emu.GetGSManager().GetRender().m_flip_status = 0;
}

int cellGcmSetTileInfo(u8 index, u8 location, u32 offset, u32 size, u32 pitch, u8 comp, u16 base, u8 bank)
{
	cellGcmSys.Warning("cellGcmSetTileInfo(index=%d, location=%d, offset=%d, size=%d, pitch=%d, comp=%d, base=%d, bank=%d)",
		index, location, offset, size, pitch, comp, base, bank);

	if(index >= RSXThread::m_tiles_count || base >= 800 || bank >= 4)
	{
		return CELL_GCM_ERROR_INVALID_VALUE;
	}

	if(offset & 0xffff || size & 0xffff || pitch & 0xf)
	{
		return CELL_GCM_ERROR_INVALID_ALIGNMENT;
	}

	if(location >= 2 || (comp != 0 && (comp < 7 || comp > 12)))
	{
		return CELL_GCM_ERROR_INVALID_ENUM;
	}

	if(comp)
	{
		cellGcmSys.Error("cellGcmSetTileInfo: bad comp! (%d)", comp);
	}

	auto& tile = Emu.GetGSManager().GetRender().m_tiles[index];
	tile.m_location = location;
	tile.m_offset = offset;
	tile.m_size = size;
	tile.m_pitch = pitch;
	tile.m_comp = comp;
	tile.m_base = base;
	tile.m_bank = bank;
	Memory.WriteData(Emu.GetGSManager().GetRender().m_tiles_addr + sizeof(CellGcmTileInfo) * index, tile.Pack());

	return CELL_OK;
}

int cellGcmBindTile(u8 index)
{
	cellGcmSys.Warning("cellGcmBindTile(index=%d)", index);

	if(index >= RSXThread::m_tiles_count)
	{
		return CELL_GCM_ERROR_INVALID_VALUE;
	}

	auto& tile = Emu.GetGSManager().GetRender().m_tiles[index];
	tile.m_binded = true;

	return CELL_OK;
}

int cellGcmUnbindTile(u8 index)
{
	cellGcmSys.Warning("cellGcmUnbindTile(index=%d)", index);

	if(index >= RSXThread::m_tiles_count)
	{
		return CELL_GCM_ERROR_INVALID_VALUE;
	}

	auto& tile = Emu.GetGSManager().GetRender().m_tiles[index];
	tile.m_binded = false;

	return CELL_OK;
}

u32 cellGcmGetTileInfo()
{
	cellGcmSys.Warning("cellGcmGetTileInfo()");
	return Emu.GetGSManager().GetRender().m_tiles_addr;
}

int cellGcmInitDefaultFifoMode(int mode)
{
	cellGcmSys.Warning("cellGcmInitDefaultFifoMode(mode=%d)", mode);
	return CELL_OK;
}

u32 cellGcmGetReportDataAddressLocation(u8 location, u32 index)
{
	ConLog.Warning("cellGcmGetReportDataAddressLocation(location=%d, index=%d)", location, index);
	return Emu.GetGSManager().GetRender().m_report_main_addr;
}

int cellGcmSetDebugOutputLevel (int level)
{
	cellGcmSys.Warning("cellGcmSetDebugOutputLevel(level=%d)", level);
	
	switch (level)
	{
		case CELL_GCM_DEBUG_LEVEL0:
		case CELL_GCM_DEBUG_LEVEL1:
		case CELL_GCM_DEBUG_LEVEL2:
			Emu.GetGSManager().GetRender().m_debug_level = level;
		break;

		default: return CELL_EINVAL;
	}
}

int cellGcmSetSecondVFrequency (u32 freq)
{
	cellGcmSys.Warning("cellGcmSetSecondVFrequency(level=%d)", freq);
	
	switch (freq)
	{
		case CELL_GCM_DISPLAY_FREQUENCY_59_94HZ:
		case CELL_GCM_DISPLAY_FREQUENCY_SCANOUT:
		case CELL_GCM_DISPLAY_FREQUENCY_DISABLE:
			Emu.GetGSManager().GetRender().m_frequency_mode = freq;
		break;

		default: return CELL_EINVAL;
	}
}

void cellGcmSys_init()
{
	cellGcmSys.AddFunc(0x055bd74d, cellGcmGetTiledPitchSize);
	cellGcmSys.AddFunc(0x06edea9e, cellGcmSetUserHandler);
	cellGcmSys.AddFunc(0x15bae46b, cellGcmInit);
	cellGcmSys.AddFunc(0x21397818, cellGcmSetFlipCommand);
	cellGcmSys.AddFunc(0x21ac3697, cellGcmAddressToOffset);
	cellGcmSys.AddFunc(0x3a33c1fd, cellGcmFunc15);
	cellGcmSys.AddFunc(0x4ae8d215, cellGcmSetFlipMode);
	cellGcmSys.AddFunc(0x63441cb4, cellGcmMapEaIoAddress);
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
	cellGcmSys.AddFunc(0xd34a420d, cellGcmSetZcull);
	cellGcmSys.AddFunc(0xd9a0a879, cellGcmGetZcullInfo);
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
	cellGcmSys.AddFunc(0xbc982946, cellGcmSetDefaultCommandBuffer);
	cellGcmSys.AddFunc(0xa47c09ff, cellGcmSetFlipStatus);
	cellGcmSys.AddFunc(0xbd100dbc, cellGcmSetTileInfo);
	cellGcmSys.AddFunc(0x4524cccd, cellGcmBindTile);
	cellGcmSys.AddFunc(0x0b4b62d5, cellGcmSetPrepareFlip);
	cellGcmSys.AddFunc(0x657571f7, cellGcmGetTileInfo);
	cellGcmSys.AddFunc(0xcaabd992, cellGcmInitDefaultFifoMode);
	cellGcmSys.AddFunc(0x8572bce2, cellGcmGetReportDataAddressLocation);
	cellGcmSys.AddFunc(0x51c9d62b, cellGcmSetDebugOutputLevel);
	cellGcmSys.AddFunc(0x4d7ce993, cellGcmSetSecondVFrequency);
}
