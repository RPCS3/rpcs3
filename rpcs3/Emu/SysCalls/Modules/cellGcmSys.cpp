#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/CB_FUNC.h"
#include "sysPrxForUser.h"

//#include "Emu/RSX/GCM.h"
#include "Emu/RSX/GSManager.h"
//#include "Emu/SysCalls/lv2/sys_process.h"
#include "cellGcmSys.h"

extern Module cellGcmSys;

const u32 tiled_pitches[] = {
	0x00000000, 0x00000200, 0x00000300, 0x00000400,
	0x00000500, 0x00000600, 0x00000700, 0x00000800,
	0x00000A00, 0x00000C00, 0x00000D00, 0x00000E00,
	0x00001000, 0x00001400, 0x00001800, 0x00001A00,
	0x00001C00, 0x00002000, 0x00002800, 0x00003000,
	0x00003400, 0x00003800, 0x00004000, 0x00005000,
	0x00006000, 0x00006800, 0x00007000, 0x00008000,
	0x0000A000, 0x0000C000, 0x0000D000, 0x0000E000,
	0x00010000
};

u32 local_size = 0;
u32 local_addr = 0;
u64 system_mode = 0;

CellGcmConfig current_config;
CellGcmContextData current_context;
gcmInfo gcm_info;

u32 map_offset_addr = 0;
u32 map_offset_pos = 0;

// Auxiliary functions

/*
 * Get usable local memory size for a specific game SDK version
 * Example: For 0x00446000 (FW 4.46) we get a localSize of 0x0F900000 (249MB)
 */ 
u32 gcmGetLocalMemorySize(u32 sdk_version)
{
	if (sdk_version >= 0x00220000) {
		return 0x0F900000; // 249MB
	}
	if (sdk_version >= 0x00200000) {
		return 0x0F200000; // 242MB
	}
	if (sdk_version >= 0x00190000) {
		return 0x0EA00000; // 234MB
	}
	if (sdk_version >= 0x00180000) {
		return 0x0E800000; // 232MB
	}
	return 0x0E000000; // 224MB
}

CellGcmOffsetTable offsetTable;

void InitOffsetTable()
{
	offsetTable.ioAddress.set(be_t<u32>::make((u32)Memory.Alloc(3072 * sizeof(u16), 1)));
	offsetTable.eaAddress.set(be_t<u32>::make((u32)Memory.Alloc(512 * sizeof(u16), 1)));

	memset(offsetTable.ioAddress.get_ptr(), 0xFF, 3072 * sizeof(u16));
	memset(offsetTable.eaAddress.get_ptr(), 0xFF, 512 * sizeof(u16));
}

//----------------------------------------------------------------------------
// Data Retrieval
//----------------------------------------------------------------------------

u32 cellGcmGetLabelAddress(u8 index)
{
	cellGcmSys.Log("cellGcmGetLabelAddress(index=%d)", index);
	return gcm_info.label_addr + 0x10 * index;
}

vm::ptr<CellGcmReportData> cellGcmGetReportDataAddressLocation(u32 index, u32 location)
{
	cellGcmSys.Warning("cellGcmGetReportDataAddressLocation(index=%d, location=%d)", index, location);

	if (location == CELL_GCM_LOCATION_LOCAL) {
		if (index >= 2048) {
			cellGcmSys.Error("cellGcmGetReportDataAddressLocation: Wrong local index (%d)", index);
			return vm::ptr<CellGcmReportData>::make(0);
		}
		return vm::ptr<CellGcmReportData>::make((u32)Memory.RSXFBMem.GetStartAddr() + index * 0x10);
	}

	if (location == CELL_GCM_LOCATION_MAIN) {
		if (index >= 1024 * 1024) {
			cellGcmSys.Error("cellGcmGetReportDataAddressLocation: Wrong main index (%d)", index);
			return vm::ptr<CellGcmReportData>::make(0);
		}
		// TODO: It seems m_report_main_addr is not initialized
		return vm::ptr<CellGcmReportData>::make(Emu.GetGSManager().GetRender().m_report_main_addr + index * 0x10);
	}

	cellGcmSys.Error("cellGcmGetReportDataAddressLocation: Wrong location (%d)", location);
	return vm::ptr<CellGcmReportData>::make(0);
}

u64 cellGcmGetTimeStamp(u32 index)
{
	cellGcmSys.Log("cellGcmGetTimeStamp(index=%d)", index);

	if (index >= 2048) {
		cellGcmSys.Error("cellGcmGetTimeStamp: Wrong local index (%d)", index);
		return 0;
	}
	return vm::read64(Memory.RSXFBMem.GetStartAddr() + index * 0x10);
}

s32 cellGcmGetCurrentField()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

u32 cellGcmGetNotifyDataAddress(u32 index)
{
	cellGcmSys.Warning("cellGcmGetNotifyDataAddress(index=%d)", index);

	// If entry not in use, return NULL
	u16 entry = offsetTable.eaAddress[241];
	if (entry == 0xFFFF) {
		return 0;
	}

	return (entry << 20) + (index * 0x20);
}

/*
 *  Get base address of local report data area
 */
vm::ptr<CellGcmReportData> _cellGcmFunc12()
{
	return vm::ptr<CellGcmReportData>::make(Memory.RSXFBMem.GetStartAddr()); // TODO
}

u32 cellGcmGetReport(u32 type, u32 index)
{
	cellGcmSys.Warning("cellGcmGetReport(type=%d, index=%d)", type, index);

	if (index >= 2048) {
		cellGcmSys.Error("cellGcmGetReport: Wrong local index (%d)", index);
		return -1;
	}

	if (type < 1 || type > 5) {
		return -1;
	}

	vm::ptr<CellGcmReportData> local_reports = _cellGcmFunc12();
	return local_reports[index].value;
}

u32 cellGcmGetReportDataAddress(u32 index)
{
	cellGcmSys.Warning("cellGcmGetReportDataAddress(index=%d)",  index);

	if (index >= 2048) {
		cellGcmSys.Error("cellGcmGetReportDataAddress: Wrong local index (%d)", index);
		return 0;
	}
	return (u32)Memory.RSXFBMem.GetStartAddr() + index * 0x10;
}

u32 cellGcmGetReportDataLocation(u32 index, u32 location)
{
	cellGcmSys.Warning("cellGcmGetReportDataLocation(index=%d, location=%d)", index, location);

	vm::ptr<CellGcmReportData> report = cellGcmGetReportDataAddressLocation(index, location);
	return report->value;
}

u64 cellGcmGetTimeStampLocation(u32 index, u32 location)
{
	cellGcmSys.Warning("cellGcmGetTimeStampLocation(index=%d, location=%d)", index, location);

	if (location == CELL_GCM_LOCATION_LOCAL) {
		if (index >= 2048) {
			cellGcmSys.Error("cellGcmGetTimeStampLocation: Wrong local index (%d)", index);
			return 0;
		}
		return vm::read64(Memory.RSXFBMem.GetStartAddr() + index * 0x10);
	}

	if (location == CELL_GCM_LOCATION_MAIN) {
		if (index >= 1024 * 1024) {
			cellGcmSys.Error("cellGcmGetTimeStampLocation: Wrong main index (%d)", index);
			return 0;
		}
		// TODO: It seems m_report_main_addr is not initialized
		return vm::read64(Emu.GetGSManager().GetRender().m_report_main_addr + index * 0x10);
	}

	cellGcmSys.Error("cellGcmGetTimeStampLocation: Wrong location (%d)", location);
	return 0;
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
	cellGcmSys.Log("cellGcmGetDefaultCommandWordSize()");
	return 0x400;
}

u32 cellGcmGetDefaultSegmentWordSize()
{
	cellGcmSys.Log("cellGcmGetDefaultSegmentWordSize()");
	return 0x100;
}

s32 cellGcmInitDefaultFifoMode(s32 mode)
{
	cellGcmSys.Warning("cellGcmInitDefaultFifoMode(mode=%d)", mode);
	return CELL_OK;
}

s32 cellGcmSetDefaultFifoSize(u32 bufferSize, u32 segmentSize)
{
	cellGcmSys.Warning("cellGcmSetDefaultFifoSize(bufferSize=0x%x, segmentSize=0x%x)", bufferSize, segmentSize);
	return CELL_OK;
}

//----------------------------------------------------------------------------
// Hardware Resource Management
//----------------------------------------------------------------------------

s32 cellGcmBindTile(u8 index)
{
	cellGcmSys.Warning("cellGcmBindTile(index=%d)", index);

	if (index >= RSXThread::m_tiles_count)
	{
		cellGcmSys.Error("cellGcmBindTile : CELL_GCM_ERROR_INVALID_VALUE");
		return CELL_GCM_ERROR_INVALID_VALUE;
	}

	auto& tile = Emu.GetGSManager().GetRender().m_tiles[index];
	tile.m_binded = true;

	return CELL_OK;
}

s32 cellGcmBindZcull(u8 index)
{
	cellGcmSys.Warning("cellGcmBindZcull(index=%d)", index);

	if (index >= RSXThread::m_zculls_count)
	{
		cellGcmSys.Error("cellGcmBindZcull : CELL_GCM_ERROR_INVALID_VALUE");
		return CELL_GCM_ERROR_INVALID_VALUE;
	}

	auto& zcull = Emu.GetGSManager().GetRender().m_zculls[index];
	zcull.m_binded = true;

	return CELL_OK;
}

s32 cellGcmGetConfiguration(vm::ptr<CellGcmConfig> config)
{
	cellGcmSys.Log("cellGcmGetConfiguration(config_addr=0x%x)", config.addr());

	*config = current_config;

	return CELL_OK;
}

s32 cellGcmGetFlipStatus()
{
	s32 status = Emu.GetGSManager().GetRender().m_flip_status;

	cellGcmSys.Log("cellGcmGetFlipStatus() -> %d", status);

	return status;
}

u32 cellGcmGetTiledPitchSize(u32 size)
{
	cellGcmSys.Log("cellGcmGetTiledPitchSize(size=%d)", size);

	for (size_t i=0; i < sizeof(tiled_pitches) / sizeof(tiled_pitches[0]) - 1; i++) {
		if (tiled_pitches[i] < size && size <= tiled_pitches[i+1]) {
			return tiled_pitches[i+1];
		}
	}
	return 0;
}

void _cellGcmFunc1()
{
	cellGcmSys.Todo("_cellGcmFunc1()");
	return;
}

void _cellGcmFunc15(vm::ptr<CellGcmContextData> context)
{
	cellGcmSys.Todo("_cellGcmFunc15(context_addr=0x%x)", context.addr());
	return;
}

// Called by cellGcmInit
s32 _cellGcmInitBody(vm::ptr<CellGcmContextData> context, u32 cmdSize, u32 ioSize, u32 ioAddress)
{
	cellGcmSys.Warning("_cellGcmInitBody(context_addr=0x%x, cmdSize=0x%x, ioSize=0x%x, ioAddress=0x%x)", context.addr(), cmdSize, ioSize, ioAddress);

	if(!local_size && !local_addr)
	{
		local_size = 0xf900000; // TODO: Get sdk_version in _cellGcmFunc15 and pass it to gcmGetLocalMemorySize
		local_addr = (u32)Memory.RSXFBMem.GetStartAddr();
		Memory.RSXFBMem.AllocAlign(local_size);
	}

	cellGcmSys.Warning("*** local memory(addr=0x%x, size=0x%x)", local_addr, local_size);

	InitOffsetTable();
	if (system_mode == CELL_GCM_SYSTEM_MODE_IOMAP_512MB)
	{
		cellGcmSys.Warning("cellGcmInit(): 512MB io address space used");
		Memory.RSXIOMem.SetRange(0, 0x20000000 /*512MB*/);
	}
	else
	{
		cellGcmSys.Warning("cellGcmInit(): 256MB io address space used");
		Memory.RSXIOMem.SetRange(0, 0x10000000 /*256MB*/);
	}

	if (gcmMapEaIoAddress(ioAddress, 0, ioSize, false) != CELL_OK)
	{
		cellGcmSys.Error("cellGcmInit : CELL_GCM_ERROR_FAILURE");
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

	u32 ctx_begin = ioAddress/* + 0x1000*/;
	u32 ctx_size = 0x6ffc;
	current_context.begin = ctx_begin;
	current_context.end = ctx_begin + ctx_size - 4;
	current_context.current = current_context.begin;
	current_context.callback.set(be_t<u32>::make(Emu.GetRSXCallback() - 4));

	gcm_info.context_addr = Memory.MainMem.AllocAlign(0x1000);
	gcm_info.control_addr = gcm_info.context_addr + 0x40;

	gcm_info.label_addr = Memory.MainMem.AllocAlign(0x1000); // ???

	vm::get_ref<CellGcmContextData>(gcm_info.context_addr) = current_context;
	vm::write32(context.addr(), gcm_info.context_addr);

	auto& ctrl = vm::get_ref<CellGcmControl>(gcm_info.control_addr);
	ctrl.put.write_relaxed(be_t<u32>::make(0));
	ctrl.get.write_relaxed(be_t<u32>::make(0));
	ctrl.ref.write_relaxed(be_t<u32>::make(-1));

	auto& render = Emu.GetGSManager().GetRender();
	render.m_ctxt_addr = context.addr();
	render.m_gcm_buffers.set(Memory.Alloc(sizeof(CellGcmDisplayInfo) * 8, sizeof(CellGcmDisplayInfo)));
	render.m_zculls_addr = (u32)Memory.Alloc(sizeof(CellGcmZcullInfo) * 8, sizeof(CellGcmZcullInfo));
	render.m_tiles_addr = (u32)Memory.Alloc(sizeof(CellGcmTileInfo) * 15, sizeof(CellGcmTileInfo));
	render.m_gcm_buffers_count = 0;
	render.m_gcm_current_buffer = 0;
	render.m_main_mem_addr = 0;
	render.m_label_addr = gcm_info.label_addr;
	render.Init(ctx_begin, ctx_size, gcm_info.control_addr, local_addr);

	return CELL_OK;
}

s32 cellGcmResetFlipStatus()
{
	cellGcmSys.Log("cellGcmResetFlipStatus()");

	Emu.GetGSManager().GetRender().m_flip_status = CELL_GCM_DISPLAY_FLIP_STATUS_WAITING;

	return CELL_OK;
}

s32 cellGcmSetDebugOutputLevel(s32 level)
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

s32 cellGcmSetDisplayBuffer(u32 id, u32 offset, u32 pitch, u32 width, u32 height)
{
	cellGcmSys.Log("cellGcmSetDisplayBuffer(id=0x%x,offset=0x%x,pitch=%d,width=%d,height=%d)", id, offset, width ? pitch / width : pitch, width, height);

	if (id > 7) {
		cellGcmSys.Error("cellGcmSetDisplayBuffer : CELL_EINVAL");
		return CELL_EINVAL;
	}

	auto buffers = Emu.GetGSManager().GetRender().m_gcm_buffers;

	buffers[id].offset = offset;
	buffers[id].pitch = pitch;
	buffers[id].width = width;
	buffers[id].height = height;

	if (id + 1 > Emu.GetGSManager().GetRender().m_gcm_buffers_count) {
		Emu.GetGSManager().GetRender().m_gcm_buffers_count = id + 1;
	}

	return CELL_OK;
}

void cellGcmSetFlipHandler(vm::ptr<void(u32)> handler)
{
	cellGcmSys.Warning("cellGcmSetFlipHandler(handler_addr=%d)", handler.addr());

	Emu.GetGSManager().GetRender().m_flip_handler = handler;
}

s32 cellGcmSetFlipMode(u32 mode)
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

s32 cellGcmSetPrepareFlip(vm::ptr<CellGcmContextData> ctxt, u32 id)
{
	cellGcmSys.Log("cellGcmSetPrepareFlip(ctx=0x%x, id=0x%x)", ctxt.addr(), id);

	if(id > 7)
	{
		cellGcmSys.Error("cellGcmSetPrepareFlip : CELL_GCM_ERROR_FAILURE");
		return CELL_GCM_ERROR_FAILURE;
	}

	u32 current = ctxt->current;

	if (current + 8 == ctxt->begin) //???
	{
		cellGcmSys.Error("cellGcmSetPrepareFlip : queue is full");
		return CELL_GCM_ERROR_FAILURE;
	}

	if (current + 8 >= ctxt->end)
	{
		cellGcmSys.Error("Bad flip!");
		if (s32 res = ctxt->callback(ctxt, 8 /* ??? */))
		{
			cellGcmSys.Error("cellGcmSetPrepareFlip : callback failed (0x%08x)", res);
			return res;
		}
	}

	auto pointer = vm::ptr<u32>::make(ctxt->current);
	ctxt->current += make_rsx_command(pointer, GCM_FLIP_COMMAND, id) * 4;

	return id;
}

s32 cellGcmSetFlip(vm::ptr<CellGcmContextData> ctxt, u32 id)
{
	cellGcmSys.Log("cellGcmSetFlip(ctx=0x%x, id=0x%x)", ctxt.addr(), id);

	s32 res = cellGcmSetPrepareFlip(ctxt, id);
	return res < 0 ? CELL_GCM_ERROR_FAILURE : CELL_OK;
}

s32 cellGcmSetSecondVFrequency(u32 freq)
{
	cellGcmSys.Warning("cellGcmSetSecondVFrequency(level=%d)", freq);

	switch (freq)
	{
	case CELL_GCM_DISPLAY_FREQUENCY_59_94HZ:
		Emu.GetGSManager().GetRender().m_frequency_mode = freq; Emu.GetGSManager().GetRender().m_fps_limit = 59.94; break;
	case CELL_GCM_DISPLAY_FREQUENCY_SCANOUT:
		Emu.GetGSManager().GetRender().m_frequency_mode = freq; cellGcmSys.Todo("Unimplemented display frequency: Scanout"); break;
	case CELL_GCM_DISPLAY_FREQUENCY_DISABLE:
		Emu.GetGSManager().GetRender().m_frequency_mode = freq; cellGcmSys.Todo("Unimplemented display frequency: Disabled"); break;
	default: cellGcmSys.Error("Improper display frequency specified!"); return CELL_OK;
	}

	return CELL_OK;
}

s32 cellGcmSetTileInfo(u8 index, u8 location, u32 offset, u32 size, u32 pitch, u8 comp, u16 base, u8 bank)
{
	cellGcmSys.Warning("cellGcmSetTileInfo(index=%d, location=%d, offset=%d, size=%d, pitch=%d, comp=%d, base=%d, bank=%d)",
		index, location, offset, size, pitch, comp, base, bank);

	if (index >= RSXThread::m_tiles_count || base >= 800 || bank >= 4)
	{
		cellGcmSys.Error("cellGcmSetTileInfo : CELL_GCM_ERROR_INVALID_VALUE");
		return CELL_GCM_ERROR_INVALID_VALUE;
	}

	if (offset & 0xffff || size & 0xffff || pitch & 0xf)
	{
		cellGcmSys.Error("cellGcmSetTileInfo : CELL_GCM_ERROR_INVALID_ALIGNMENT");
		return CELL_GCM_ERROR_INVALID_ALIGNMENT;
	}

	if (location >= 2 || (comp != 0 && (comp < 7 || comp > 12)))
	{
		cellGcmSys.Error("cellGcmSetTileInfo : CELL_GCM_ERROR_INVALID_ALIGNMENT");
		return CELL_GCM_ERROR_INVALID_ENUM;
	}

	if (comp)
	{
		cellGcmSys.Error("cellGcmSetTileInfo: bad compression mode! (%d)", comp);
	}

	auto& tile = Emu.GetGSManager().GetRender().m_tiles[index];
	tile.m_location = location;
	tile.m_offset = offset;
	tile.m_size = size;
	tile.m_pitch = pitch;
	tile.m_comp = comp;
	tile.m_base = base;
	tile.m_bank = bank;

	vm::get_ptr<CellGcmTileInfo>(Emu.GetGSManager().GetRender().m_tiles_addr)[index] = tile.Pack();
	return CELL_OK;
}

void cellGcmSetUserHandler(vm::ptr<void(u32)> handler)
{
	cellGcmSys.Warning("cellGcmSetUserHandler(handler_addr=0x%x)", handler.addr());

	Emu.GetGSManager().GetRender().m_user_handler = handler;
}

void cellGcmSetVBlankHandler(vm::ptr<void(u32)> handler)
{
	cellGcmSys.Warning("cellGcmSetVBlankHandler(handler_addr=0x%x)", handler.addr());

	Emu.GetGSManager().GetRender().m_vblank_handler = handler;
}

s32 cellGcmSetWaitFlip(vm::ptr<CellGcmContextData> ctxt)
{
	cellGcmSys.Log("cellGcmSetWaitFlip(ctx=0x%x)", ctxt.addr());

	GSLockCurrent lock(GS_LOCK_WAIT_FLIP);
	return CELL_OK;
}

s32 cellGcmSetZcull(u8 index, u32 offset, u32 width, u32 height, u32 cullStart, u32 zFormat, u32 aaFormat, u32 zCullDir, u32 zCullFormat, u32 sFunc, u32 sRef, u32 sMask)
{
	cellGcmSys.Todo("cellGcmSetZcull(index=%d, offset=0x%x, width=%d, height=%d, cullStart=0x%x, zFormat=0x%x, aaFormat=0x%x, zCullDir=0x%x, zCullFormat=0x%x, sFunc=0x%x, sRef=0x%x, sMask=0x%x)",
		index, offset, width, height, cullStart, zFormat, aaFormat, zCullDir, zCullFormat, sFunc, sRef, sMask);

	if (index >= RSXThread::m_zculls_count)
	{
		cellGcmSys.Error("cellGcmSetZcull : CELL_GCM_ERROR_INVALID_VALUE");
		return CELL_GCM_ERROR_INVALID_VALUE;
	}

	auto& zcull = Emu.GetGSManager().GetRender().m_zculls[index];
	zcull.m_offset = offset;
	zcull.m_width = width; 
	zcull.m_height = height;
	zcull.m_cullStart = cullStart;
	zcull.m_zFormat = zFormat;
	zcull.m_aaFormat = aaFormat;
	zcull.m_zcullDir = zCullDir;
	zcull.m_zcullFormat = zCullFormat;
	zcull.m_sFunc = sFunc;
	zcull.m_sRef = sRef;
	zcull.m_sMask = sMask;

	vm::get_ptr<CellGcmZcullInfo>(Emu.GetGSManager().GetRender().m_zculls_addr)[index] = zcull.Pack();
	return CELL_OK;
}

s32 cellGcmUnbindTile(u8 index)
{
	cellGcmSys.Warning("cellGcmUnbindTile(index=%d)", index);

	if (index >= RSXThread::m_tiles_count)
	{
		cellGcmSys.Error("cellGcmUnbindTile : CELL_GCM_ERROR_INVALID_VALUE");
		return CELL_GCM_ERROR_INVALID_VALUE;
	}

	auto& tile = Emu.GetGSManager().GetRender().m_tiles[index];
	tile.m_binded = false;

	return CELL_OK;
}

s32 cellGcmUnbindZcull(u8 index)
{
	cellGcmSys.Warning("cellGcmUnbindZcull(index=%d)", index);

	if (index >= 8)
	{
		cellGcmSys.Error("cellGcmUnbindZcull : CELL_EINVAL");
		return CELL_EINVAL;
	}

	auto& zcull = Emu.GetGSManager().GetRender().m_zculls[index];
	zcull.m_binded = false;

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
	cellGcmSys.Warning("cellGcmGetDisplayInfo() = 0x%x", Emu.GetGSManager().GetRender().m_gcm_buffers);
	return Emu.GetGSManager().GetRender().m_gcm_buffers.addr();
}

s32 cellGcmGetCurrentDisplayBufferId(u32 id_addr)
{
	cellGcmSys.Warning("cellGcmGetCurrentDisplayBufferId(id_addr=0x%x)", id_addr);

	vm::write32(id_addr, Emu.GetGSManager().GetRender().m_gcm_current_buffer);

	return CELL_OK;
}

s32 cellGcmSetInvalidateTile()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

s32 cellGcmDumpGraphicsError()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

s32 cellGcmGetDisplayBufferByFlipIndex()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

u64 cellGcmGetLastFlipTime()
{
	cellGcmSys.Log("cellGcmGetLastFlipTime()");

	return Emu.GetGSManager().GetRender().m_last_flip_time;
}

s32 cellGcmGetLastSecondVTime()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

u64 cellGcmGetVBlankCount()
{
	cellGcmSys.Log("cellGcmGetVBlankCount()");

	return Emu.GetGSManager().GetRender().m_vblank_count;
}

s32 cellGcmInitSystemMode(u64 mode)
{
	cellGcmSys.Log("cellGcmInitSystemMode(mode=0x%x)", mode);

	system_mode = mode;

	return CELL_OK;
}

s32 cellGcmSetFlipImmediate(u8 id)
{
	cellGcmSys.Todo("cellGcmSetFlipImmediate(fid=0x%x)", id);

	if (id > 7)
	{
		cellGcmSys.Error("cellGcmSetFlipImmediate : CELL_GCM_ERROR_FAILURE");
		return CELL_GCM_ERROR_FAILURE;
	}

	cellGcmSetFlipMode(id);

	return CELL_OK;
}

s32 cellGcmSetGraphicsHandler()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

s32 cellGcmSetQueueHandler()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

s32 cellGcmSetSecondVHandler()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

s32 cellGcmSetVBlankFrequency()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

s32 cellGcmSortRemapEaIoAddress()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

//----------------------------------------------------------------------------
// Memory Mapping
//----------------------------------------------------------------------------
s32 cellGcmAddressToOffset(u32 address, vm::ptr<u32> offset)
{
	cellGcmSys.Log("cellGcmAddressToOffset(address=0x%x, &offset=0x%x)", address, offset.addr());

	// Address not on main memory or local memory
	if (address >= 0xD0000000)
	{
		return CELL_GCM_ERROR_FAILURE;
	}

	u32 result;

	// Address in local memory
	if (Memory.RSXFBMem.IsInMyRange(address))
	{
		result = address - Memory.RSXFBMem.GetStartAddr();
	}
	// Address in main memory else check 
	else
	{
		const u32 upper12Bits = offsetTable.ioAddress[address >> 20];

		// If the address is mapped in IO
		if (upper12Bits != 0xFFFF)
		{
			result = (upper12Bits << 20) | (address & 0xFFFFF);
		}
		else
		{
			return CELL_GCM_ERROR_FAILURE;
		}
	}

	*offset = result;
	return CELL_OK;
}

u32 cellGcmGetMaxIoMapSize()
{
	cellGcmSys.Log("cellGcmGetMaxIoMapSize()");

	return (u32)(Memory.RSXIOMem.GetEndAddr() - Memory.RSXIOMem.GetReservedAmount());
}

void cellGcmGetOffsetTable(vm::ptr<CellGcmOffsetTable> table)
{
	cellGcmSys.Log("cellGcmGetOffsetTable(table_addr=0x%x)", table.addr());

	table->ioAddress = offsetTable.ioAddress;
	table->eaAddress = offsetTable.eaAddress;
}

s32 cellGcmIoOffsetToAddress(u32 ioOffset, vm::ptr<u32> address)
{
	cellGcmSys.Log("cellGcmIoOffsetToAddress(ioOffset=0x%x, address=0x%x)", ioOffset, address);

	u32 realAddr;

	if (!Memory.RSXIOMem.getRealAddr(ioOffset, realAddr)) 
		return CELL_GCM_ERROR_FAILURE;

	*address = realAddr;

	return CELL_OK;
}

s32 gcmMapEaIoAddress(u32 ea, u32 io, u32 size, bool is_strict)
{
	if ((ea & 0xFFFFF) || (io & 0xFFFFF) || (size & 0xFFFFF)) return CELL_GCM_ERROR_FAILURE;

	// Check if the mapping was successfull
	if (Memory.RSXIOMem.Map(ea, size, io))
	{
		// Fill the offset table
		for (u32 i = 0; i<(size >> 20); i++)
		{
			offsetTable.ioAddress[(ea >> 20) + i] = (io >> 20) + i;
			offsetTable.eaAddress[(io >> 20) + i] = (ea >> 20) + i;
			Emu.GetGSManager().GetRender().m_strict_ordering[(io >> 20) + i] = is_strict;
		}
	}
	else
	{
		cellGcmSys.Error("cellGcmMapEaIoAddress : CELL_GCM_ERROR_FAILURE");
		return CELL_GCM_ERROR_FAILURE;
	}

	return CELL_OK;
}

s32 cellGcmMapEaIoAddress(u32 ea, u32 io, u32 size)
{
	cellGcmSys.Warning("cellGcmMapEaIoAddress(ea=0x%x, io=0x%x, size=0x%x)", ea, io, size);

	return gcmMapEaIoAddress(ea, io, size, false);
}

s32 cellGcmMapEaIoAddressWithFlags(u32 ea, u32 io, u32 size, u32 flags)
{
	cellGcmSys.Warning("cellGcmMapEaIoAddressWithFlags(ea=0x%x, io=0x%x, size=0x%x, flags=0x%x)", ea, io, size, flags);

	assert(flags == 2 /*CELL_GCM_IOMAP_FLAG_STRICT_ORDERING*/);

	return gcmMapEaIoAddress(ea, io, size, true);
}

s32 cellGcmMapLocalMemory(vm::ptr<u32> address, vm::ptr<u32> size)
{
	cellGcmSys.Warning("cellGcmMapLocalMemory(address=*0x%x, size=*0x%x)", address, size);

	if (!local_addr && !local_size && Memory.RSXFBMem.AllocFixed(local_addr = Memory.RSXFBMem.GetStartAddr(), local_size = 0xf900000 /* TODO */))
	{
		*address = local_addr;
		*size = local_size;
	}
	else
	{
		cellGcmSys.Error("RSX local memory already mapped");
		return CELL_GCM_ERROR_FAILURE;
	}

	return CELL_OK;
}

s32 cellGcmMapMainMemory(u32 ea, u32 size, vm::ptr<u32> offset)
{
	cellGcmSys.Warning("cellGcmMapMainMemory(ea=0x%x,size=0x%x,offset_addr=0x%x)", ea, size, offset.addr());

	if ((ea & 0xFFFFF) || (size & 0xFFFFF)) return CELL_GCM_ERROR_FAILURE;

	u32 io = Memory.RSXIOMem.Map(ea, size);

	//check if the mapping was successfull
	if (Memory.RSXIOMem.RealAddr(io) == ea)
	{
		//fill the offset table
		for (u32 i = 0; i<(size >> 20); i++)
		{
			offsetTable.ioAddress[(ea >> 20) + i] = (u16)((io >> 20) + i);
			offsetTable.eaAddress[(io >> 20) + i] = (u16)((ea >> 20) + i);
			Emu.GetGSManager().GetRender().m_strict_ordering[(io >> 20) + i] = false;
		}

		*offset = io;
	}
	else
	{
		cellGcmSys.Error("cellGcmMapMainMemory : CELL_GCM_ERROR_NO_IO_PAGE_TABLE");
		return CELL_GCM_ERROR_NO_IO_PAGE_TABLE;
	}

	Emu.GetGSManager().GetRender().m_main_mem_addr = Emu.GetGSManager().GetRender().m_ioAddress;

	return CELL_OK;
}

s32 cellGcmReserveIoMapSize(u32 size)
{
	cellGcmSys.Log("cellGcmReserveIoMapSize(size=0x%x)", size);

	if (size & 0xFFFFF)
	{
		cellGcmSys.Error("cellGcmReserveIoMapSize : CELL_GCM_ERROR_INVALID_ALIGNMENT");
		return CELL_GCM_ERROR_INVALID_ALIGNMENT;
	}

	if (size > cellGcmGetMaxIoMapSize())
	{
		cellGcmSys.Error("cellGcmReserveIoMapSize : CELL_GCM_ERROR_INVALID_VALUE");
		return CELL_GCM_ERROR_INVALID_VALUE;
	}

	Memory.RSXIOMem.Reserve(size);
	return CELL_OK;
}

s32 cellGcmUnmapEaIoAddress(u32 ea)
{
	cellGcmSys.Log("cellGcmUnmapEaIoAddress(ea=0x%x)", ea);

	u32 size;
	if (Memory.RSXIOMem.UnmapRealAddress(ea, size))
	{
		const u32 io = offsetTable.ioAddress[ea >>= 20];

		for (u32 i = 0; i < size >> 20; i++)
		{
			offsetTable.ioAddress[ea + i] = 0xFFFF;
			offsetTable.eaAddress[io + i] = 0xFFFF;
		}
	}
	else
	{
		cellGcmSys.Error("cellGcmUnmapEaIoAddress(ea=0x%x): UnmapRealAddress() failed");
		return CELL_GCM_ERROR_FAILURE;
	}

	return CELL_OK;
}

s32 cellGcmUnmapIoAddress(u32 io)
{
	cellGcmSys.Log("cellGcmUnmapIoAddress(io=0x%x)", io);

	u32 size;
	if (Memory.RSXIOMem.UnmapAddress(io, size))
	{
		const u32 ea = offsetTable.eaAddress[io >>= 20];

		for (u32 i = 0; i < size >> 20; i++)
		{
			offsetTable.ioAddress[ea + i] = 0xFFFF;
			offsetTable.eaAddress[io + i] = 0xFFFF;
		}
	}
	else
	{
		cellGcmSys.Error("cellGcmUnmapIoAddress(io=0x%x): UnmapAddress() failed");
		return CELL_GCM_ERROR_FAILURE;
	}

	return CELL_OK;
}

s32 cellGcmUnreserveIoMapSize(u32 size)
{
	cellGcmSys.Log("cellGcmUnreserveIoMapSize(size=0x%x)", size);

	if (size & 0xFFFFF)
	{
		cellGcmSys.Error("cellGcmReserveIoMapSize : CELL_GCM_ERROR_INVALID_ALIGNMENT");
		return CELL_GCM_ERROR_INVALID_ALIGNMENT;
	}

	if (size > Memory.RSXIOMem.GetReservedAmount())
	{
		cellGcmSys.Error("cellGcmReserveIoMapSize : CELL_GCM_ERROR_INVALID_VALUE");
		return CELL_GCM_ERROR_INVALID_VALUE;
	}

	Memory.RSXIOMem.Unreserve(size);
	return CELL_OK;
}

//----------------------------------------------------------------------------
// Cursor
//----------------------------------------------------------------------------

s32 cellGcmInitCursor()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

s32 cellGcmSetCursorPosition(s32 x, s32 y)
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

s32 cellGcmSetCursorDisable()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

s32 cellGcmUpdateCursor()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

s32 cellGcmSetCursorEnable()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

s32 cellGcmSetCursorImageOffset(u32 offset)
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
	vm::write32(Emu.GetGSManager().GetRender().m_ctxt_addr, gcm_info.context_addr);
}

//------------------------------------------------------------------------
// Other
//------------------------------------------------------------------------

s32 _cellGcmSetFlipCommand(vm::ptr<CellGcmContextData> ctx, u32 id)
{
	cellGcmSys.Log("cellGcmSetFlipCommand(ctx_addr=0x%x, id=0x%x)", ctx.addr(), id);

	return cellGcmSetPrepareFlip(ctx, id);
}

s32 _cellGcmSetFlipCommandWithWaitLabel(vm::ptr<CellGcmContextData> ctx, u32 id, u32 label_index, u32 label_value)
{
	cellGcmSys.Log("cellGcmSetFlipCommandWithWaitLabel(ctx_addr=0x%x, id=0x%x, label_index=0x%x, label_value=0x%x)",
		ctx.addr(), id, label_index, label_value);

	s32 res = cellGcmSetPrepareFlip(ctx, id);
	vm::write32(gcm_info.label_addr + 0x10 * label_index, label_value);
	return res < 0 ? CELL_GCM_ERROR_FAILURE : CELL_OK;
}

s32 cellGcmSetTile(u8 index, u8 location, u32 offset, u32 size, u32 pitch, u8 comp, u16 base, u8 bank)
{
	cellGcmSys.Warning("cellGcmSetTile(index=%d, location=%d, offset=%d, size=%d, pitch=%d, comp=%d, base=%d, bank=%d)",
		index, location, offset, size, pitch, comp, base, bank);

	// Copied form cellGcmSetTileInfo
	if(index >= RSXThread::m_tiles_count || base >= 800 || bank >= 4)
	{
		cellGcmSys.Error("cellGcmSetTile : CELL_GCM_ERROR_INVALID_VALUE");
		return CELL_GCM_ERROR_INVALID_VALUE;
	}

	if(offset & 0xffff || size & 0xffff || pitch & 0xf)
	{
		cellGcmSys.Error("cellGcmSetTile : CELL_GCM_ERROR_INVALID_ALIGNMENT");
		return CELL_GCM_ERROR_INVALID_ALIGNMENT;
	}

	if(location >= 2 || (comp != 0 && (comp < 7 || comp > 12)))
	{
		cellGcmSys.Error("cellGcmSetTile : CELL_GCM_ERROR_INVALID_ENUM");
		return CELL_GCM_ERROR_INVALID_ENUM;
	}

	if(comp)
	{
		cellGcmSys.Error("cellGcmSetTile: bad compression mode! (%d)", comp);
	}

	auto& tile = Emu.GetGSManager().GetRender().m_tiles[index];
	tile.m_location = location;
	tile.m_offset = offset;
	tile.m_size = size;
	tile.m_pitch = pitch;
	tile.m_comp = comp;
	tile.m_base = base;
	tile.m_bank = bank;

	vm::get_ptr<CellGcmTileInfo>(Emu.GetGSManager().GetRender().m_tiles_addr)[index] = tile.Pack();
	return CELL_OK;
}

//----------------------------------------------------------------------------

// TODO: This function was originally located in lv2/SC_GCM and appears in RPCS3 as a lv2 syscall with id 1023,
//       which according to lv2 dumps isn't the case. So, is this a proper place for this function?

s32 cellGcmCallback(vm::ptr<CellGcmContextData> context, u32 count)
{
	cellGcmSys.Log("cellGcmCallback(context_addr=0x%x, count=0x%x)", context.addr(), count);

	//GSLockCurrent gslock(GS_LOCK_WAIT_FLUSH);

	auto cmd = vm::ptr<u32>::make(context->current);
	/*
	size_t offset;
	offset  = make_rsx_command(cmd, NV4097_SET_SEMAPHORE_OFFSET, 0x20);
	offset += make_rsx_command(cmd, NV4097_TEXTURE_READ_SEMAPHORE_RELEASE, 0); //incrementing by module value
	context->current += offset * sizeof(u32);
	*/
	make_rsx_jump(cmd, 0);
	context->current = context->begin; // rewind to the beginning

	return CELL_OK;
}

//----------------------------------------------------------------------------

Module cellGcmSys("cellGcmSys", []()
{
	current_config.ioAddress = 0;
	current_config.localAddress = 0;
	local_size = 0;
	local_addr = 0;

	// Data Retrieval
	REG_FUNC(cellGcmSys, cellGcmGetCurrentField);
	REG_FUNC(cellGcmSys, cellGcmGetLabelAddress);
	REG_FUNC(cellGcmSys, cellGcmGetNotifyDataAddress);
	REG_FUNC(cellGcmSys, _cellGcmFunc12);
	REG_FUNC(cellGcmSys, cellGcmGetReport);
	REG_FUNC(cellGcmSys, cellGcmGetReportDataAddress);
	REG_FUNC(cellGcmSys, cellGcmGetReportDataAddressLocation);
	REG_FUNC(cellGcmSys, cellGcmGetReportDataLocation);
	REG_FUNC(cellGcmSys, cellGcmGetTimeStamp);
	REG_FUNC(cellGcmSys, cellGcmGetTimeStampLocation);

	// Command Buffer Control
	REG_FUNC(cellGcmSys, cellGcmGetControlRegister);
	REG_FUNC(cellGcmSys, cellGcmGetDefaultCommandWordSize);
	REG_FUNC(cellGcmSys, cellGcmGetDefaultSegmentWordSize);
	REG_FUNC(cellGcmSys, cellGcmInitDefaultFifoMode);
	REG_FUNC(cellGcmSys, cellGcmSetDefaultFifoSize);
	//cellGcmSys.AddFunc(, cellGcmReserveMethodSize);
	//cellGcmSys.AddFunc(, cellGcmResetDefaultCommandBuffer);
	//cellGcmSys.AddFunc(, cellGcmSetupContextData);
	//cellGcmSys.AddFunc(, cellGcmCallbackForSnc);
	//cellGcmSys.AddFunc(, cellGcmFinish);
	//cellGcmSys.AddFunc(, cellGcmFlush);

	// Hardware Resource Management
	REG_FUNC(cellGcmSys, cellGcmBindTile);
	REG_FUNC(cellGcmSys, cellGcmBindZcull);
	REG_FUNC(cellGcmSys, cellGcmDumpGraphicsError);
	REG_FUNC(cellGcmSys, cellGcmGetConfiguration);
	REG_FUNC(cellGcmSys, cellGcmGetDisplayBufferByFlipIndex);
	REG_FUNC(cellGcmSys, cellGcmGetFlipStatus);
	REG_FUNC(cellGcmSys, cellGcmGetLastFlipTime);
	REG_FUNC(cellGcmSys, cellGcmGetLastSecondVTime);
	REG_FUNC(cellGcmSys, cellGcmGetTiledPitchSize);
	REG_FUNC(cellGcmSys, cellGcmGetVBlankCount);
	REG_FUNC(cellGcmSys, _cellGcmFunc1);
	REG_FUNC(cellGcmSys, _cellGcmFunc15);
	REG_FUNC(cellGcmSys, _cellGcmInitBody);
	REG_FUNC(cellGcmSys, cellGcmInitSystemMode);
	REG_FUNC(cellGcmSys, cellGcmResetFlipStatus);
	REG_FUNC(cellGcmSys, cellGcmSetDebugOutputLevel);
	REG_FUNC(cellGcmSys, cellGcmSetDisplayBuffer);
	REG_FUNC(cellGcmSys, cellGcmSetFlip);
	REG_FUNC(cellGcmSys, cellGcmSetFlipHandler);
	REG_FUNC(cellGcmSys, cellGcmSetFlipImmediate);
	REG_FUNC(cellGcmSys, cellGcmSetFlipMode);
	REG_FUNC(cellGcmSys, cellGcmSetFlipStatus);
	REG_FUNC(cellGcmSys, cellGcmSetGraphicsHandler);
	REG_FUNC(cellGcmSys, cellGcmSetPrepareFlip);
	REG_FUNC(cellGcmSys, cellGcmSetQueueHandler);
	REG_FUNC(cellGcmSys, cellGcmSetSecondVFrequency);
	REG_FUNC(cellGcmSys, cellGcmSetSecondVHandler);
	REG_FUNC(cellGcmSys, cellGcmSetTileInfo);
	REG_FUNC(cellGcmSys, cellGcmSetUserHandler);
	REG_FUNC(cellGcmSys, cellGcmSetVBlankFrequency);
	REG_FUNC(cellGcmSys, cellGcmSetVBlankHandler);
	REG_FUNC(cellGcmSys, cellGcmSetWaitFlip);
	REG_FUNC(cellGcmSys, cellGcmSetZcull);
	REG_FUNC(cellGcmSys, cellGcmSortRemapEaIoAddress);
	REG_FUNC(cellGcmSys, cellGcmUnbindTile);
	REG_FUNC(cellGcmSys, cellGcmUnbindZcull);
	REG_FUNC(cellGcmSys, cellGcmGetTileInfo);
	REG_FUNC(cellGcmSys, cellGcmGetZcullInfo);
	REG_FUNC(cellGcmSys, cellGcmGetDisplayInfo);
	REG_FUNC(cellGcmSys, cellGcmGetCurrentDisplayBufferId);
	REG_FUNC(cellGcmSys, cellGcmSetInvalidateTile);
	//cellGcmSys.AddFunc(, cellGcmSetFlipWithWaitLabel);

	// Memory Mapping
	REG_FUNC(cellGcmSys, cellGcmAddressToOffset);
	REG_FUNC(cellGcmSys, cellGcmGetMaxIoMapSize);
	REG_FUNC(cellGcmSys, cellGcmGetOffsetTable);
	REG_FUNC(cellGcmSys, cellGcmIoOffsetToAddress);
	REG_FUNC(cellGcmSys, cellGcmMapEaIoAddress);
	REG_FUNC(cellGcmSys, cellGcmMapEaIoAddressWithFlags);
	REG_FUNC(cellGcmSys, cellGcmMapLocalMemory);
	REG_FUNC(cellGcmSys, cellGcmMapMainMemory);
	REG_FUNC(cellGcmSys, cellGcmReserveIoMapSize);
	REG_FUNC(cellGcmSys, cellGcmUnmapEaIoAddress);
	REG_FUNC(cellGcmSys, cellGcmUnmapIoAddress);
	REG_FUNC(cellGcmSys, cellGcmUnreserveIoMapSize);

	// Cursor
	REG_FUNC(cellGcmSys, cellGcmInitCursor);
	REG_FUNC(cellGcmSys, cellGcmSetCursorEnable);
	REG_FUNC(cellGcmSys, cellGcmSetCursorDisable);
	REG_FUNC(cellGcmSys, cellGcmSetCursorImageOffset);
	REG_FUNC(cellGcmSys, cellGcmSetCursorPosition);
	REG_FUNC(cellGcmSys, cellGcmUpdateCursor);

	// Functions for Maintaining Compatibility
	REG_FUNC(cellGcmSys, cellGcmSetDefaultCommandBuffer);
	//cellGcmSys.AddFunc(, cellGcmGetCurrentBuffer);
	//cellGcmSys.AddFunc(, cellGcmSetCurrentBuffer);
	//cellGcmSys.AddFunc(, cellGcmSetDefaultCommandBufferAndSegmentWordSize);
	//cellGcmSys.AddFunc(, cellGcmSetUserCallback);

	// Other
	REG_FUNC(cellGcmSys, _cellGcmSetFlipCommand);
	REG_FUNC(cellGcmSys, _cellGcmSetFlipCommandWithWaitLabel);
	REG_FUNC(cellGcmSys, cellGcmSetTile);
});
