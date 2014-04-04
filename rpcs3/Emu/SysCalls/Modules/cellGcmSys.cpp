#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "Emu/GS/GCM.h"

void cellGcmSys_init();
void cellGcmSys_load();
void cellGcmSys_unload();
Module cellGcmSys(0x0010, cellGcmSys_init, cellGcmSys_load, cellGcmSys_unload);

u32 local_size = 0;
u32 local_addr = NULL;

enum
{
	CELL_GCM_ERROR_FAILURE           = 0x802100ff,
	CELL_GCM_ERROR_NO_IO_PAGE_TABLE  = 0x80210001,
	CELL_GCM_ERROR_INVALID_ENUM      = 0x80210002,
	CELL_GCM_ERROR_INVALID_VALUE     = 0x80210003,
	CELL_GCM_ERROR_INVALID_ALIGNMENT = 0x80210004,
	CELL_GCM_ERROR_ADDRESS_OVERWRAP  = 0x80210005
};

// Function declaration 
int cellGcmSetPrepareFlip(mem_ptr_t<CellGcmContextData> ctxt, u32 id);

//----------------------------------------------------------------------------
// Memory Mapping
//----------------------------------------------------------------------------

struct gcm_offset
{
	u64 io;
	u64 ea;
};

void InitOffsetTable();
int32_t cellGcmAddressToOffset(u64 address, mem32_t offset);
uint32_t cellGcmGetMaxIoMapSize();
void cellGcmGetOffsetTable(mem_ptr_t<gcm_offset> table);
int32_t cellGcmIoOffsetToAddress(u32 ioOffset, u64 address);
int32_t cellGcmMapEaIoAddress(const u32 ea, const u32 io, const u32 size);
int32_t cellGcmMapEaIoAddressWithFlags(const u32 ea, const u32 io, const u32 size, const u32 flags);
int32_t cellGcmMapMainMemory(u64 ea, u32 size, mem32_t offset);
int32_t cellGcmReserveIoMapSize(const u32 size);
int32_t cellGcmUnmapEaIoAddress(u64 ea);
int32_t cellGcmUnmapIoAddress(u64 io);
int32_t cellGcmUnreserveIoMapSize(u32 size);

//----------------------------------------------------------------------------

CellGcmConfig current_config;
CellGcmContextData current_context;
gcmInfo gcm_info;

u32 map_offset_addr = 0;
u32 map_offset_pos = 0;

//----------------------------------------------------------------------------
// Data Retrieval
//----------------------------------------------------------------------------

u32 cellGcmGetLabelAddress(u8 index)
{
	cellGcmSys.Log("cellGcmGetLabelAddress(index=%d)", index);
	return Memory.RSXCMDMem.GetStartAddr() + 0x10 * index;
}

u32 cellGcmGetReportDataAddressLocation(u8 location, u32 index)
{
	ConLog.Warning("cellGcmGetReportDataAddressLocation(location=%d, index=%d)", location, index);
	return Emu.GetGSManager().GetRender().m_report_main_addr;
}

u64 cellGcmGetTimeStamp(u32 index)
{
	cellGcmSys.Log("cellGcmGetTimeStamp(index=%d)", index);
	return Memory.Read64(Memory.RSXFBMem.GetStartAddr() + index * 0x10);
}

//----------------------------------------------------------------------------
// Command Buffer Control
//----------------------------------------------------------------------------

u32 cellGcmGetControlRegister()
{
	cellGcmSys.Log("cellGcmGetControlRegister()");

	return gcm_info.control_addr;
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

int cellGcmInitDefaultFifoMode(s32 mode)
{
	cellGcmSys.Warning("cellGcmInitDefaultFifoMode(mode=%d)", mode);
	return CELL_OK;
}

int cellGcmSetDefaultFifoSize(u32 bufferSize, u32 segmentSize)
{
	cellGcmSys.Warning("cellGcmSetDefaultFifoSize(bufferSize=0x%x, segmentSize=0x%x)", bufferSize, segmentSize);
	return CELL_OK;
}

//----------------------------------------------------------------------------
// Hardware Resource Management
//----------------------------------------------------------------------------

int cellGcmBindTile(u8 index)
{
	cellGcmSys.Warning("cellGcmBindTile(index=%d)", index);

	if (index >= RSXThread::m_tiles_count)
	{
		return CELL_GCM_ERROR_INVALID_VALUE;
	}

	auto& tile = Emu.GetGSManager().GetRender().m_tiles[index];
	tile.m_binded = true;

	return CELL_OK;
}

int cellGcmBindZcull(u8 index)
{
	cellGcmSys.Warning("TODO: cellGcmBindZcull(index=%d)", index);

	return CELL_OK;
}

int cellGcmGetConfiguration(mem_ptr_t<CellGcmConfig> config)
{
	cellGcmSys.Log("cellGcmGetConfiguration(config_addr=0x%x)", config.GetAddr());

	if (!config.IsGood()) return CELL_EFAULT;

	*config = current_config;

	return CELL_OK;
}

int cellGcmGetFlipStatus()
{
	return Emu.GetGSManager().GetRender().m_flip_status;
}

u32 cellGcmGetTiledPitchSize(u32 size)
{
	//TODO
	cellGcmSys.Warning("cellGcmGetTiledPitchSize(size=%d)", size);

	return size;
}

int cellGcmInit(u32 context_addr, u32 cmdSize, u32 ioSize, u32 ioAddress)
{
	cellGcmSys.Warning("cellGcmInit(context_addr=0x%x,cmdSize=0x%x,ioSize=0x%x,ioAddress=0x%x)", context_addr, cmdSize, ioSize, ioAddress);

	if(!cellGcmSys.IsLoaded())
		cellGcmSys.Load();

	if(!local_size && !local_addr)
	{
		local_size = 0xf900000; //TODO
		local_addr = Memory.RSXFBMem.GetStartAddr();
		Memory.RSXFBMem.AllocAlign(local_size);
	}

	cellGcmSys.Warning("*** local memory(addr=0x%x, size=0x%x)", local_addr, local_size);

	InitOffsetTable();
	Memory.MemoryBlocks.push_back(Memory.RSXIOMem.SetRange(0x50000000, 0x10000000/*256MB*/));//TODO: implement allocateAdressSpace in memoryBase
	if(cellGcmMapEaIoAddress(ioAddress, 0, ioSize) != CELL_OK)
	{
		Memory.MemoryBlocks.pop_back();
		return CELL_GCM_ERROR_FAILURE;
	}

	map_offset_addr = 0;
	map_offset_pos = 0;
	current_config.ioSize = ioSize;
	current_config.ioAddress = ioAddress;
	current_config.localSize = local_size;
	current_config.localAddress = local_addr;
	current_config.memoryFrequency = 650000000;
	current_config.coreFrequency = 500000000;

	Memory.RSXCMDMem.AllocAlign(cmdSize);

	u32 ctx_begin = ioAddress/* + 0x1000*/;
	u32 ctx_size = 0x6ffc;
	current_context.begin = ctx_begin;
	current_context.end = ctx_begin + ctx_size;
	current_context.current = current_context.begin;
	current_context.callback = Emu.GetRSXCallback() - 4;

	gcm_info.context_addr = Memory.MainMem.AllocAlign(0x1000);
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
	render.m_main_mem_addr = 0;
	render.Init(ctx_begin, ctx_size, gcm_info.control_addr, local_addr);

	return CELL_OK;
}

int cellGcmResetFlipStatus()
{
	Emu.GetGSManager().GetRender().m_flip_status = 1;

	return CELL_OK;
}

int cellGcmSetDebugOutputLevel(int level)
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

	return CELL_OK;
}

int cellGcmSetDisplayBuffer(u32 id, u32 offset, u32 pitch, u32 width, u32 height)
{
	cellGcmSys.Warning("cellGcmSetDisplayBuffer(id=0x%x,offset=0x%x,pitch=%d,width=%d,height=%d)",
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

int cellGcmSetFlip(mem_ptr_t<CellGcmContextData> ctxt, u32 id)
{
	cellGcmSys.Log("cellGcmSetFlip(ctx=0x%x, id=0x%x)", ctxt.GetAddr(), id);

	int res = cellGcmSetPrepareFlip(ctxt, id);
	return res < 0 ? CELL_GCM_ERROR_FAILURE : CELL_OK;
}

int cellGcmSetFlipHandler(u32 handler_addr)
{
	cellGcmSys.Warning("cellGcmSetFlipHandler(handler_addr=%d)", handler_addr);
	if (handler_addr != 0 && !Memory.IsGoodAddr(handler_addr))
	{
		return CELL_EFAULT;
	}

	Emu.GetGSManager().GetRender().m_flip_handler.SetAddr(handler_addr);
	return CELL_OK;
}

int cellGcmSetFlipMode(u32 mode)
{
	cellGcmSys.Warning("cellGcmSetFlipMode(mode=%d)", mode);

	switch (mode)
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

void cellGcmSetFlipStatus()
{
	cellGcmSys.Warning("cellGcmSetFlipStatus()");

	Emu.GetGSManager().GetRender().m_flip_status = 0;
}

int cellGcmSetPrepareFlip(mem_ptr_t<CellGcmContextData> ctxt, u32 id)
{
	cellGcmSys.Log("cellGcmSetPrepareFlip(ctx=0x%x, id=0x%x)", ctxt.GetAddr(), id);

	if(id >= 8)
	{
		return CELL_GCM_ERROR_FAILURE;
	}

	GSLockCurrent gslock(GS_LOCK_WAIT_FLUSH); // could stall on exit

	u32 current = ctxt->current;
	u32 end = ctxt->end;

	if(current + 8 >= end)
	{
		ConLog.Warning("bad flip!");
		//cellGcmCallback(ctxt.GetAddr(), current + 8 - end);
 		//copied:
 
 		CellGcmControl& ctrl = (CellGcmControl&)Memory[gcm_info.control_addr];
 
 		const s32 res = ctxt->current - ctxt->begin - ctrl.put;
 
 		if(res > 0) Memory.Copy(ctxt->begin, ctxt->current - res, res);
 		ctxt->current = ctxt->begin + res;
 		ctrl.put = res;
 		ctrl.get = 0;
	}

	current = ctxt->current;
	Memory.Write32(current, 0x3fead | (1 << 18));
	Memory.Write32(current + 4, id);
	ctxt->current += 8;

	if(ctxt.GetAddr() == gcm_info.context_addr)
	{
		CellGcmControl& ctrl = (CellGcmControl&)Memory[gcm_info.control_addr];
		ctrl.put += 8;
	}

	return id;
}

int cellGcmSetSecondVFrequency(u32 freq)
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

	return CELL_OK;
}

int cellGcmSetTileInfo(u8 index, u8 location, u32 offset, u32 size, u32 pitch, u8 comp, u16 base, u8 bank)
{
	cellGcmSys.Warning("cellGcmSetTileInfo(index=%d, location=%d, offset=%d, size=%d, pitch=%d, comp=%d, base=%d, bank=%d)",
		index, location, offset, size, pitch, comp, base, bank);

	if (index >= RSXThread::m_tiles_count || base >= 800 || bank >= 4)
	{
		return CELL_GCM_ERROR_INVALID_VALUE;
	}

	if (offset & 0xffff || size & 0xffff || pitch & 0xf)
	{
		return CELL_GCM_ERROR_INVALID_ALIGNMENT;
	}

	if (location >= 2 || (comp != 0 && (comp < 7 || comp > 12)))
	{
		return CELL_GCM_ERROR_INVALID_ENUM;
	}

	if (comp)
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
	Memory.WriteData(Emu.GetGSManager().GetRender().m_tiles_addr + sizeof(CellGcmTileInfo)* index, tile.Pack());

	return CELL_OK;
}

u32 cellGcmSetUserHandler(u32 handler)
{
	cellGcmSys.Warning("cellGcmSetUserHandler(handler=0x%x)", handler);
	return handler;
}

int cellGcmSetVBlankHandler()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

int cellGcmSetWaitFlip(mem_ptr_t<CellGcmContextData> ctxt)
{
	cellGcmSys.Log("cellGcmSetWaitFlip(ctx=0x%x)", ctxt.GetAddr());

	GSLockCurrent lock(GS_LOCK_WAIT_FLIP);
	return CELL_OK;
}

int cellGcmSetZcull(u8 index, u32 offset, u32 width, u32 height, u32 cullStart, u32 zFormat, u32 aaFormat, u32 zCullDir, u32 zCullFormat, u32 sFunc, u32 sRef, u32 sMask)
{
	cellGcmSys.Warning("TODO: cellGcmSetZcull(index=%d, offset=0x%x, width=%d, height=%d, cullStart=0x%x, zFormat=0x%x, aaFormat=0x%x, zCullDir=0x%x, zCullFormat=0x%x, sFunc=0x%x, sRef=0x%x, sMask=0x%x)",
		index, offset, width, height, cullStart, zFormat, aaFormat, zCullDir, zCullFormat, sFunc, sRef, sMask);

	return CELL_OK;
}

int cellGcmUnbindTile(u8 index)
{
	cellGcmSys.Warning("cellGcmUnbindTile(index=%d)", index);

	if (index >= RSXThread::m_tiles_count)
	{
		return CELL_GCM_ERROR_INVALID_VALUE;
	}

	auto& tile = Emu.GetGSManager().GetRender().m_tiles[index];
	tile.m_binded = false;

	return CELL_OK;
}

int cellGcmUnbindZcull(u8 index)
{
	cellGcmSys.Warning("cellGcmUnbindZcull(index=%d)", index);
	if (index >= 8)
		return CELL_EINVAL;

	return CELL_OK;
}

u32 cellGcmGetTileInfo()
{
	cellGcmSys.Warning("cellGcmGetTileInfo()");
	return Emu.GetGSManager().GetRender().m_tiles_addr;
}

u32 cellGcmGetZcullInfo()
{
	cellGcmSys.Warning("cellGcmGetZcullInfo()");
	return Emu.GetGSManager().GetRender().m_zculls_addr;
}

u32 cellGcmGetDisplayInfo()
{
	cellGcmSys.Warning("cellGcmGetDisplayInfo() = 0x%x", Emu.GetGSManager().GetRender().m_gcm_buffers_addr);
	return Emu.GetGSManager().GetRender().m_gcm_buffers_addr;
}

int cellGcmGetCurrentDisplayBufferId(u32 id_addr)
{
	cellGcmSys.Warning("cellGcmGetCurrentDisplayBufferId(id_addr=0x%x)", id_addr);

	if (!Memory.IsGoodAddr(id_addr))
	{
		return CELL_EFAULT;
	}

	Memory.Write32(id_addr, Emu.GetGSManager().GetRender().m_gcm_current_buffer);

	return CELL_OK;
}

//----------------------------------------------------------------------------
// Memory Mapping
//----------------------------------------------------------------------------

gcm_offset offsetTable = { 0, 0 };

void InitOffsetTable()
{
	offsetTable.io = Memory.Alloc(3072 * sizeof(u16), 1);
	for (int i = 0; i<3072; i++)
	{
		Memory.Write16(offsetTable.io + sizeof(u16)*i, 0xFFFF);
	}

	offsetTable.ea = Memory.Alloc(256 * sizeof(u16), 1);//TODO: check flags
	for (int i = 0; i<256; i++)
	{
		Memory.Write16(offsetTable.ea + sizeof(u16)*i, 0xFFFF);
	}
}

int32_t cellGcmAddressToOffset(u64 address, mem32_t offset)
{
	cellGcmSys.Log("cellGcmAddressToOffset(address=0x%x,offset_addr=0x%x)", address, offset.GetAddr());

	if (address >= 0xD0000000/*not on main memory or local*/)
		return CELL_GCM_ERROR_FAILURE;

	u32 result;

	// If address is in range of local memory
	if (Memory.RSXFBMem.IsInMyRange(address))
	{
		result = address - Memory.RSXFBMem.GetStartAddr();
	}
	// else check if the adress (main memory) is mapped in IO
	else
	{
		u16 upper12Bits = Memory.Read16(offsetTable.io + sizeof(u16)*(address >> 20));

		if (upper12Bits != 0xFFFF)
		{
			result = (((u64)upper12Bits << 20) | (address & (0xFFFFF)));
		}
		// address is not mapped in IO
		else
		{
			return CELL_GCM_ERROR_FAILURE;
		}
	}

	offset = result;
	return CELL_OK;
}

uint32_t cellGcmGetMaxIoMapSize()
{
	return Memory.RSXIOMem.GetEndAddr() - Memory.RSXIOMem.GetStartAddr() - Memory.RSXIOMem.GetResevedAmount();
}

void cellGcmGetOffsetTable(mem_ptr_t<gcm_offset> table)
{
	table->io = re(offsetTable.io);
	table->ea = re(offsetTable.ea);
}

int32_t cellGcmIoOffsetToAddress(u32 ioOffset, u64 address)
{
	u64 realAddr;

	realAddr = Memory.RSXIOMem.getRealAddr(Memory.RSXIOMem.GetStartAddr() + ioOffset);

	if (!realAddr)
		return CELL_GCM_ERROR_FAILURE;

	Memory.Write64(address, realAddr);

	return CELL_OK;
}

int32_t cellGcmMapEaIoAddress(const u32 ea, const u32 io, const u32 size)
{
	cellGcmSys.Warning("cellGcmMapEaIoAddress(ea=0x%x, io=0x%x, size=0x%x)", ea, io, size);

	if ((ea & 0xFFFFF) || (io & 0xFFFFF) || (size & 0xFFFFF)) return CELL_GCM_ERROR_FAILURE;

	//check if the mapping was successfull
	if (Memory.RSXIOMem.Map(ea, size, Memory.RSXIOMem.GetStartAddr() + io))
	{
		//fill the offset table
		for (u32 i = 0; i<(size >> 20); i++)
		{
			Memory.Write16(offsetTable.io + ((ea >> 20) + i)*sizeof(u16), (io >> 20) + i);
			Memory.Write16(offsetTable.ea + ((io >> 20) + i)*sizeof(u16), (ea >> 20) + i);
		}
	}
	else
	{
		return CELL_GCM_ERROR_FAILURE;
	}

	return CELL_OK;
}

int32_t cellGcmMapEaIoAddressWithFlags(const u32 ea, const u32 io, const u32 size, const u32 flags)
{
	cellGcmSys.Warning("cellGcmMapEaIoAddressWithFlags(ea=0x%x, io=0x%x, size=0x%x, flags=0x%x)", ea, io, size, flags);
	return cellGcmMapEaIoAddress(ea, io, size); // TODO: strict ordering
}

int32_t cellGcmMapLocalMemory(u64 address, u64 size)
{
	if (!local_size && !local_addr)
	{
		local_size = 0xf900000; //TODO
		local_addr = Memory.RSXFBMem.GetStartAddr();
		Memory.RSXFBMem.AllocAlign(local_size);
		Memory.Write32(address, local_addr);
		Memory.Write32(size, local_size);
	}
	else
	{
		cellGcmSys.Error("RSX local memory already mapped");
		return CELL_GCM_ERROR_FAILURE;
	}

	return CELL_OK;
}

int32_t cellGcmMapMainMemory(u64 ea, u32 size, mem32_t offset)
{
	cellGcmSys.Warning("cellGcmMapMainMemory(ea=0x%x,size=0x%x,offset_addr=0x%x)", ea, size, offset.GetAddr());

	u64 io;

	if ((ea & 0xFFFFF) || (size & 0xFFFFF)) return CELL_GCM_ERROR_FAILURE;

	//check if the mapping was successfull
	if (io = Memory.RSXIOMem.Map(ea, size, 0))
	{
		// convert to offset
		io = io - Memory.RSXIOMem.GetStartAddr();

		//fill the offset table
		for (u32 i = 0; i<(size >> 20); i++)
		{
			Memory.Write16(offsetTable.io + ((ea >> 20) + i)*sizeof(u16), (io >> 20) + i);
			Memory.Write16(offsetTable.ea + ((io >> 20) + i)*sizeof(u16), (ea >> 20) + i);
		}

		offset = io;
	}
	else
	{
		return CELL_GCM_ERROR_NO_IO_PAGE_TABLE;
	}

	Emu.GetGSManager().GetRender().m_main_mem_addr = Emu.GetGSManager().GetRender().m_ioAddress;

	return CELL_OK;
}

int32_t cellGcmReserveIoMapSize(const u32 size)
{
	if (size & 0xFFFFF)
		return CELL_GCM_ERROR_INVALID_ALIGNMENT;

	if (size > cellGcmGetMaxIoMapSize())
		return CELL_GCM_ERROR_INVALID_VALUE;

	Memory.RSXIOMem.Reserve(size);
	return CELL_OK;
}

int32_t cellGcmUnmapEaIoAddress(u64 ea)
{
	u32 size;
	if (size = Memory.RSXIOMem.UnmapRealAddress(ea))
	{
		u64 io;
		ea = ea >> 20;
		io = Memory.Read16(offsetTable.io + (ea*sizeof(u16)));

		for (u32 i = 0; i<size; i++)
		{
			Memory.Write16(offsetTable.io + ((ea + i)*sizeof(u16)), 0xFFFF);
			Memory.Write16(offsetTable.ea + ((io + i)*sizeof(u16)), 0xFFFF);
		}
	}
	else
	{
		return CELL_GCM_ERROR_FAILURE;
	}

	return CELL_OK;
}

int32_t cellGcmUnmapIoAddress(u64 io)
{
	u32 size;
	if (size = Memory.RSXIOMem.UnmapAddress(io))
	{
		u64 ea;
		io = io >> 20;
		ea = Memory.Read16(offsetTable.ea + (io*sizeof(u16)));

		for (u32 i = 0; i<size; i++)
		{
			Memory.Write16(offsetTable.io + ((ea + i)*sizeof(u16)), 0xFFFF);
			Memory.Write16(offsetTable.ea + ((io + i)*sizeof(u16)), 0xFFFF);
		}
	}
	else
	{
		return CELL_GCM_ERROR_FAILURE;
	}

	return CELL_OK;
}

int32_t cellGcmUnreserveIoMapSize(u32 size)
{
	if (size & 0xFFFFF)
		return CELL_GCM_ERROR_INVALID_ALIGNMENT;

	if (size > Memory.RSXIOMem.GetResevedAmount())
		return CELL_GCM_ERROR_INVALID_VALUE;

	Memory.RSXIOMem.Unreserve(size);
	return CELL_OK;
}

//----------------------------------------------------------------------------
// Cursor
//----------------------------------------------------------------------------

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

//------------------------------------------------------------------------
// Functions for Maintaining Compatibility
//------------------------------------------------------------------------

void cellGcmSetDefaultCommandBuffer()
{
	cellGcmSys.Warning("cellGcmSetDefaultCommandBuffer()");
	Memory.Write32(Emu.GetGSManager().GetRender().m_ctxt_addr, gcm_info.context_addr);
}

//------------------------------------------------------------------------
// Other
//------------------------------------------------------------------------

int cellGcmSetFlipCommand(u32 ctx, u32 id)
{
	return cellGcmSetPrepareFlip(ctx, id);
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
	return res < 0 ? CELL_GCM_ERROR_FAILURE : CELL_OK;
}

int cellGcmSetTile(u8 index, u8 location, u32 offset, u32 size, u32 pitch, u8 comp, u16 base, u8 bank)
{
	cellGcmSys.Warning("cellGcmSetTile(index=%d, location=%d, offset=%d, size=%d, pitch=%d, comp=%d, base=%d, bank=%d)",
		index, location, offset, size, pitch, comp, base, bank);

	// Copied form cellGcmSetTileInfo
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
		cellGcmSys.Error("cellGcmSetTile: bad comp! (%d)", comp);
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

//----------------------------------------------------------------------------

void cellGcmSys_init()
{
	// Data Retrieval
	//cellGcmSys.AddFunc(0xc8f3bd09, cellGcmGetCurrentField);
	cellGcmSys.AddFunc(0xf80196c1, cellGcmGetLabelAddress);
	//cellGcmSys.AddFunc(0x21cee035, cellGcmGetNotifyDataAddress);
	//cellGcmSys.AddFunc(0x99d397ac, cellGcmGetReport);
	//cellGcmSys.AddFunc(0x9a0159af, cellGcmGetReportDataAddress);
	cellGcmSys.AddFunc(0x8572bce2, cellGcmGetReportDataAddressLocation);
	//cellGcmSys.AddFunc(0xa6b180ac, cellGcmGetReportDataLocation);
	cellGcmSys.AddFunc(0x5a41c10f, cellGcmGetTimeStamp);
	//cellGcmSys.AddFunc(0x2ad4951b, cellGcmGetTimeStampLocation);

	// Command Buffer Control
	//cellGcmSys.AddFunc(, cellGcmCallbackForSnc);
	//cellGcmSys.AddFunc(, cellGcmFinish);
	//cellGcmSys.AddFunc(, cellGcmFlush);
	cellGcmSys.AddFunc(0xa547adde, cellGcmGetControlRegister);
	cellGcmSys.AddFunc(0x5e2ee0f0, cellGcmGetDefaultCommandWordSize);
	cellGcmSys.AddFunc(0x8cdf8c70, cellGcmGetDefaultSegmentWordSize);
	cellGcmSys.AddFunc(0xcaabd992, cellGcmInitDefaultFifoMode);
	//cellGcmSys.AddFunc(, cellGcmReserveMethodSize);
	//cellGcmSys.AddFunc(, cellGcmResetDefaultCommandBuffer);
	cellGcmSys.AddFunc(0x9ba451e4, cellGcmSetDefaultFifoSize);
	//cellGcmSys.AddFunc(, cellGcmSetupContextData);

	// Hardware Resource Management
	cellGcmSys.AddFunc(0x4524cccd, cellGcmBindTile);
	cellGcmSys.AddFunc(0x9dc04436, cellGcmBindZcull);
	//cellGcmSys.AddFunc(0x1f61b3ff, cellGcmDumpGraphicsError);
	cellGcmSys.AddFunc(0xe315a0b2, cellGcmGetConfiguration);
	//cellGcmSys.AddFunc(0x371674cf, cellGcmGetDisplayBufferByFlipIndex);
	cellGcmSys.AddFunc(0x72a577ce, cellGcmGetFlipStatus);
	//cellGcmSys.AddFunc(0x63387071, cellGcmgetLastFlipTime);
	//cellGcmSys.AddFunc(0x23ae55a3, cellGcmGetLastSecondVTime);
	cellGcmSys.AddFunc(0x055bd74d, cellGcmGetTiledPitchSize);
	//cellGcmSys.AddFunc(0x723bbc7e, cellGcmGetVBlankCount);
	cellGcmSys.AddFunc(0x15bae46b, cellGcmInit);
	//cellGcmSys.AddFunc(0xfce9e764, cellGcmInitSystemMode);
	cellGcmSys.AddFunc(0xb2e761d4, cellGcmResetFlipStatus);
	cellGcmSys.AddFunc(0x51c9d62b, cellGcmSetDebugOutputLevel);
	cellGcmSys.AddFunc(0xa53d12ae, cellGcmSetDisplayBuffer);
	cellGcmSys.AddFunc(0xdc09357e, cellGcmSetFlip);
	cellGcmSys.AddFunc(0xa41ef7e8, cellGcmSetFlipHandler);
	//cellGcmSys.AddFunc(0xacee8542, cellGcmSetFlipImmediate);
	cellGcmSys.AddFunc(0x4ae8d215, cellGcmSetFlipMode);
	cellGcmSys.AddFunc(0xa47c09ff, cellGcmSetFlipStatus);
	//cellGcmSys.AddFunc(, cellGcmSetFlipWithWaitLabel);
	//cellGcmSys.AddFunc(0xd01b570d, cellGcmSetGraphicsHandler);
	cellGcmSys.AddFunc(0x0b4b62d5, cellGcmSetPrepareFlip);
	//cellGcmSys.AddFunc(0x0a862772, cellGcmSetQueueHandler);
	cellGcmSys.AddFunc(0x4d7ce993, cellGcmSetSecondVFrequency);
	//cellGcmSys.AddFunc(0xdc494430, cellGcmSetSecondVHandler);
	cellGcmSys.AddFunc(0xbd100dbc, cellGcmSetTileInfo);
	cellGcmSys.AddFunc(0x06edea9e, cellGcmSetUserHandler);
	//cellGcmSys.AddFunc(0xffe0160e, cellGcmSetVBlankFrequency);
	cellGcmSys.AddFunc(0xa91b0402, cellGcmSetVBlankHandler);
	cellGcmSys.AddFunc(0x983fb9aa, cellGcmSetWaitFlip);
	cellGcmSys.AddFunc(0xd34a420d, cellGcmSetZcull);
	//cellGcmSys.AddFunc(0x25b40ab4, cellGcmSortRemapEaIoAddress);
	cellGcmSys.AddFunc(0xd9b7653e, cellGcmUnbindTile);
	cellGcmSys.AddFunc(0xa75640e8, cellGcmUnbindZcull);
	cellGcmSys.AddFunc(0x657571f7, cellGcmGetTileInfo);
	cellGcmSys.AddFunc(0xd9a0a879, cellGcmGetZcullInfo);
	cellGcmSys.AddFunc(0x0e6b0dae, cellGcmGetDisplayInfo);
	cellGcmSys.AddFunc(0x93806525, cellGcmGetCurrentDisplayBufferId);

	// Memory Mapping
	cellGcmSys.AddFunc(0x21ac3697, cellGcmAddressToOffset);
	cellGcmSys.AddFunc(0xfb81c03e, cellGcmGetMaxIoMapSize);
	cellGcmSys.AddFunc(0x2922aed0, cellGcmGetOffsetTable);
	cellGcmSys.AddFunc(0x2a6fba9c, cellGcmIoOffsetToAddress);
	cellGcmSys.AddFunc(0x63441cb4, cellGcmMapEaIoAddress);
	cellGcmSys.AddFunc(0x626e8518, cellGcmMapEaIoAddressWithFlags);
	cellGcmSys.AddFunc(0xdb769b32, cellGcmMapLocalMemory);
	cellGcmSys.AddFunc(0xa114ec67, cellGcmMapMainMemory);
	cellGcmSys.AddFunc(0xa7ede268, cellGcmReserveIoMapSize);
	cellGcmSys.AddFunc(0xefd00f54, cellGcmUnmapEaIoAddress);
	cellGcmSys.AddFunc(0xdb23e867, cellGcmUnmapIoAddress);
	cellGcmSys.AddFunc(0x3b9bd5bd, cellGcmUnreserveIoMapSize);

	// Cursor
	cellGcmSys.AddFunc(0x107bf3a1, cellGcmInitCursor);
	cellGcmSys.AddFunc(0xc47d0812, cellGcmSetCursorEnable);
	cellGcmSys.AddFunc(0x69c6cc82, cellGcmSetCursorDisable);
	cellGcmSys.AddFunc(0xf9bfdc72, cellGcmSetCursorImageOffset);
	cellGcmSys.AddFunc(0x1a0de550, cellGcmSetCursorPosition);
	cellGcmSys.AddFunc(0xbd2fa0a7, cellGcmUpdateCursor);

	// Functions for Maintaining Compatibility
	//cellGcmSys.AddFunc(, cellGcmGetCurrentBuffer);
	//cellGcmSys.AddFunc(, cellGcmSetCurrentBuffer);
	cellGcmSys.AddFunc(0xbc982946, cellGcmSetDefaultCommandBuffer);
	//cellGcmSys.AddFunc(, cellGcmSetDefaultCommandBufferAndSegmentWordSize);
	//cellGcmSys.AddFunc(, cellGcmSetUserCallback);

	// Other
	cellGcmSys.AddFunc(0x21397818, cellGcmSetFlipCommand);
	cellGcmSys.AddFunc(0x3a33c1fd, cellGcmFunc15);
	cellGcmSys.AddFunc(0xd8f88e1a, cellGcmSetFlipCommandWithWaitLabel);
	cellGcmSys.AddFunc(0xd0b1d189, cellGcmSetTile);
}

void cellGcmSys_load()
{
	current_config.ioAddress = NULL;
	current_config.localAddress = NULL;
	local_size = 0;
	local_addr = NULL;
}

void cellGcmSys_unload()
{
}
