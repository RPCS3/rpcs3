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

Module *cellGcmSys = nullptr;

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
	cellGcmSys->Log("cellGcmGetLabelAddress(index=%d)", index);
	return (u32)Memory.RSXCMDMem.GetStartAddr() + 0x10 * index;
}

vm::ptr<CellGcmReportData> cellGcmGetReportDataAddressLocation(u32 index, u32 location)
{
	cellGcmSys->Warning("cellGcmGetReportDataAddressLocation(index=%d, location=%d)", index, location);

	if (location == CELL_GCM_LOCATION_LOCAL) {
		if (index >= 2048) {
			cellGcmSys->Error("cellGcmGetReportDataAddressLocation: Wrong local index (%d)", index);
			return vm::ptr<CellGcmReportData>::make(0);
		}
		return vm::ptr<CellGcmReportData>::make((u32)Memory.RSXFBMem.GetStartAddr() + index * 0x10);
	}

	if (location == CELL_GCM_LOCATION_MAIN) {
		if (index >= 1024*1024) {
			cellGcmSys->Error("cellGcmGetReportDataAddressLocation: Wrong main index (%d)", index);
			return vm::ptr<CellGcmReportData>::make(0);
		}
		// TODO: It seems m_report_main_addr is not initialized
		return vm::ptr<CellGcmReportData>::make(Emu.GetGSManager().GetRender().m_report_main_addr + index * 0x10);
	}

	cellGcmSys->Error("cellGcmGetReportDataAddressLocation: Wrong location (%d)", location);
	return vm::ptr<CellGcmReportData>::make(0);
}

u64 cellGcmGetTimeStamp(u32 index)
{
	cellGcmSys->Log("cellGcmGetTimeStamp(index=%d)", index);

	if (index >= 2048) {
		cellGcmSys->Error("cellGcmGetTimeStamp: Wrong local index (%d)", index);
		return 0;
	}
	return vm::read64(Memory.RSXFBMem.GetStartAddr() + index * 0x10);
}

int cellGcmGetCurrentField()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

u32 cellGcmGetNotifyDataAddress(u32 index)
{
	cellGcmSys->Warning("cellGcmGetNotifyDataAddress(index=%d)", index);

	// Get address of 'IO table' and 'EA table'
	vm::var<CellGcmOffsetTable> table;
	cellGcmGetOffsetTable(table);

	// If entry not in use, return NULL
	u16 entry = table->eaAddress[241];
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
	cellGcmSys->Warning("cellGcmGetReport(type=%d, index=%d)", type, index);

	if (index >= 2048) {
		cellGcmSys->Error("cellGcmGetReport: Wrong local index (%d)", index);
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
	cellGcmSys->Warning("cellGcmGetReportDataAddress(index=%d)",  index);

	if (index >= 2048) {
		cellGcmSys->Error("cellGcmGetReportDataAddress: Wrong local index (%d)", index);
		return 0;
	}
	return (u32)Memory.RSXFBMem.GetStartAddr() + index * 0x10;
}

u32 cellGcmGetReportDataLocation(u32 index, u32 location)
{
	cellGcmSys->Warning("cellGcmGetReportDataLocation(index=%d, location=%d)", index, location);

	vm::ptr<CellGcmReportData> report = cellGcmGetReportDataAddressLocation(index, location);
	return report->value;
}

u64 cellGcmGetTimeStampLocation(u32 index, u32 location)
{
	cellGcmSys->Warning("cellGcmGetTimeStampLocation(index=%d, location=%d)", index, location);

	if (location == CELL_GCM_LOCATION_LOCAL) {
		if (index >= 2048) {
			cellGcmSys->Error("cellGcmGetTimeStampLocation: Wrong local index (%d)", index);
			return 0;
		}
		return vm::read64(Memory.RSXFBMem.GetStartAddr() + index * 0x10);
	}

	if (location == CELL_GCM_LOCATION_MAIN) {
		if (index >= 1024 * 1024) {
			cellGcmSys->Error("cellGcmGetTimeStampLocation: Wrong main index (%d)", index);
			return 0;
		}
		// TODO: It seems m_report_main_addr is not initialized
		return vm::read64(Emu.GetGSManager().GetRender().m_report_main_addr + index * 0x10);
	}

	cellGcmSys->Error("cellGcmGetTimeStampLocation: Wrong location (%d)", location);
	return 0;
}

//----------------------------------------------------------------------------
// Command Buffer Control
//----------------------------------------------------------------------------

u32 cellGcmGetControlRegister()
{
	cellGcmSys->Log("cellGcmGetControlRegister()");

	return gcm_info.control_addr;
}

u32 cellGcmGetDefaultCommandWordSize()
{
	cellGcmSys->Log("cellGcmGetDefaultCommandWordSize()");
	return 0x400;
}

u32 cellGcmGetDefaultSegmentWordSize()
{
	cellGcmSys->Log("cellGcmGetDefaultSegmentWordSize()");
	return 0x100;
}

int cellGcmInitDefaultFifoMode(s32 mode)
{
	cellGcmSys->Warning("cellGcmInitDefaultFifoMode(mode=%d)", mode);
	return CELL_OK;
}

int cellGcmSetDefaultFifoSize(u32 bufferSize, u32 segmentSize)
{
	cellGcmSys->Warning("cellGcmSetDefaultFifoSize(bufferSize=0x%x, segmentSize=0x%x)", bufferSize, segmentSize);
	return CELL_OK;
}

//----------------------------------------------------------------------------
// Hardware Resource Management
//----------------------------------------------------------------------------

int cellGcmBindTile(u8 index)
{
	cellGcmSys->Warning("cellGcmBindTile(index=%d)", index);

	if (index >= RSXThread::m_tiles_count)
	{
		cellGcmSys->Error("cellGcmBindTile : CELL_GCM_ERROR_INVALID_VALUE");
		return CELL_GCM_ERROR_INVALID_VALUE;
	}

	auto& tile = Emu.GetGSManager().GetRender().m_tiles[index];
	tile.m_binded = true;

	return CELL_OK;
}

int cellGcmBindZcull(u8 index)
{
	cellGcmSys->Warning("cellGcmBindZcull(index=%d)", index);

	if (index >= RSXThread::m_zculls_count)
	{
		cellGcmSys->Error("cellGcmBindZcull : CELL_GCM_ERROR_INVALID_VALUE");
		return CELL_GCM_ERROR_INVALID_VALUE;
	}

	auto& zcull = Emu.GetGSManager().GetRender().m_zculls[index];
	zcull.m_binded = true;

	return CELL_OK;
}

int cellGcmGetConfiguration(vm::ptr<CellGcmConfig> config)
{
	cellGcmSys->Log("cellGcmGetConfiguration(config_addr=0x%x)", config.addr());

	*config = current_config;

	return CELL_OK;
}

int cellGcmGetFlipStatus()
{
	int status = Emu.GetGSManager().GetRender().m_flip_status;

	cellGcmSys->Log("cellGcmGetFlipStatus() -> %d", status);

	return status;
}

u32 cellGcmGetTiledPitchSize(u32 size)
{
	cellGcmSys->Log("cellGcmGetTiledPitchSize(size=%d)", size);

	for (size_t i=0; i < sizeof(tiled_pitches) / sizeof(tiled_pitches[0]) - 1; i++) {
		if (tiled_pitches[i] < size && size <= tiled_pitches[i+1]) {
			return tiled_pitches[i+1];
		}
	}
	return 0;
}

void _cellGcmFunc1()
{
	cellGcmSys->Todo("_cellGcmFunc1()");
	return;
}

void _cellGcmFunc15(vm::ptr<CellGcmContextData> context)
{
	cellGcmSys->Todo("_cellGcmFunc15(context_addr=0x%x)", context.addr());
	return;
}

// Called by cellGcmInit
s32 _cellGcmInitBody(vm::ptr<CellGcmContextData> context, u32 cmdSize, u32 ioSize, u32 ioAddress)
{
	cellGcmSys->Warning("_cellGcmInitBody(context_addr=0x%x, cmdSize=0x%x, ioSize=0x%x, ioAddress=0x%x)", context.addr(), cmdSize, ioSize, ioAddress);

	if(!cellGcmSys->IsLoaded())
		cellGcmSys->Load();

	if(!local_size && !local_addr)
	{
		local_size = 0xf900000; // TODO: Get sdk_version in _cellGcmFunc15 and pass it to gcmGetLocalMemorySize
		local_addr = (u32)Memory.RSXFBMem.GetStartAddr();
		Memory.RSXFBMem.AllocAlign(local_size);
	}

	cellGcmSys->Warning("*** local memory(addr=0x%x, size=0x%x)", local_addr, local_size);

	InitOffsetTable();
	if (system_mode == CELL_GCM_SYSTEM_MODE_IOMAP_512MB)
	{
		cellGcmSys->Warning("cellGcmInit(): 512MB io address space used");
		Memory.RSXIOMem.SetRange(0, 0x20000000 /*512MB*/);
	}
	else
	{
		cellGcmSys->Warning("cellGcmInit(): 256MB io address space used");
		Memory.RSXIOMem.SetRange(0, 0x10000000 /*256MB*/);
	}

	if(cellGcmMapEaIoAddress(ioAddress, 0, ioSize) != CELL_OK)
	{
		cellGcmSys->Error("cellGcmInit : CELL_GCM_ERROR_FAILURE");
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
	current_context.end = ctx_begin + ctx_size - 4;
	current_context.current = current_context.begin;
	current_context.callback.set(be_t<u32>::make(Emu.GetRSXCallback() - 4));

	gcm_info.context_addr = (u32)Memory.MainMem.AllocAlign(0x1000);
	gcm_info.control_addr = gcm_info.context_addr + 0x40;

	vm::get_ref<CellGcmContextData>(gcm_info.context_addr) = current_context;
	vm::write32(context.addr(), gcm_info.context_addr);

	auto& ctrl = vm::get_ref<CellGcmControl>(gcm_info.control_addr);
	ctrl.put.write_relaxed(be_t<u32>::make(0));
	ctrl.get.write_relaxed(be_t<u32>::make(0));
	ctrl.ref.write_relaxed(be_t<u32>::make(-1));

	auto& render = Emu.GetGSManager().GetRender();
	render.m_ctxt_addr = context.addr();
	render.m_gcm_buffers_addr = (u32)Memory.Alloc(sizeof(CellGcmDisplayInfo) * 8, sizeof(CellGcmDisplayInfo));
	render.m_zculls_addr = (u32)Memory.Alloc(sizeof(CellGcmZcullInfo) * 8, sizeof(CellGcmZcullInfo));
	render.m_tiles_addr = (u32)Memory.Alloc(sizeof(CellGcmTileInfo) * 15, sizeof(CellGcmTileInfo));
	render.m_gcm_buffers_count = 0;
	render.m_gcm_current_buffer = 0;
	render.m_main_mem_addr = 0;
	render.Init(ctx_begin, ctx_size, gcm_info.control_addr, local_addr);

	return CELL_OK;
}

int cellGcmResetFlipStatus()
{
	cellGcmSys->Log("cellGcmResetFlipStatus()");

	Emu.GetGSManager().GetRender().m_flip_status = CELL_GCM_DISPLAY_FLIP_STATUS_WAITING;

	return CELL_OK;
}

int cellGcmSetDebugOutputLevel(int level)
{
	cellGcmSys->Warning("cellGcmSetDebugOutputLevel(level=%d)", level);

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
	cellGcmSys->Log("cellGcmSetDisplayBuffer(id=0x%x,offset=0x%x,pitch=%d,width=%d,height=%d)", id, offset, width ? pitch / width : pitch, width, height);

	if (id > 7) {
		cellGcmSys->Error("cellGcmSetDisplayBuffer : CELL_EINVAL");
		return CELL_EINVAL;
	}

	auto buffers = vm::get_ptr<CellGcmDisplayInfo>(Emu.GetGSManager().GetRender().m_gcm_buffers_addr);

	buffers[id].offset = offset;
	buffers[id].pitch = pitch;
	buffers[id].width = width;
	buffers[id].height = height;

	if (id + 1 > Emu.GetGSManager().GetRender().m_gcm_buffers_count) {
		Emu.GetGSManager().GetRender().m_gcm_buffers_count = id + 1;
	}

	return CELL_OK;
}

int cellGcmSetFlip(vm::ptr<CellGcmContextData> ctxt, u32 id)
{
	cellGcmSys->Log("cellGcmSetFlip(ctx=0x%x, id=0x%x)", ctxt.addr(), id);

	int res = cellGcmSetPrepareFlip(ctxt, id);
	return res < 0 ? CELL_GCM_ERROR_FAILURE : CELL_OK;
}

void cellGcmSetFlipHandler(vm::ptr<void(*)(const u32)> handler)
{
	cellGcmSys->Warning("cellGcmSetFlipHandler(handler_addr=%d)", handler.addr());

	Emu.GetGSManager().GetRender().m_flip_handler = handler;
}

int cellGcmSetFlipMode(u32 mode)
{
	cellGcmSys->Warning("cellGcmSetFlipMode(mode=%d)", mode);

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
	cellGcmSys->Warning("cellGcmSetFlipStatus()");

	Emu.GetGSManager().GetRender().m_flip_status = 0;
}

s32 cellGcmSetPrepareFlip(vm::ptr<CellGcmContextData> ctxt, u32 id)
{
	cellGcmSys->Log("cellGcmSetPrepareFlip(ctx=0x%x, id=0x%x)", ctxt.addr(), id);

	if(id > 7)
	{
		cellGcmSys->Error("cellGcmSetPrepareFlip : CELL_GCM_ERROR_FAILURE");
		return CELL_GCM_ERROR_FAILURE;
	}

	GSLockCurrent gslock(GS_LOCK_WAIT_FLUSH);

	u32 current = ctxt->current;

	if (current + 8 == ctxt->begin)
	{
		cellGcmSys->Error("cellGcmSetPrepareFlip : queue is full");
		return CELL_GCM_ERROR_FAILURE;
	}

	if (current + 8 >= ctxt->end)
	{
		cellGcmSys->Error("Bad flip!");
		if (s32 res = ctxt->callback(ctxt, 8 /* ??? */))
		{
			cellGcmSys->Error("cellGcmSetPrepareFlip : callback failed (0x%08x)", res);
			return res;
		}
	}

	current = ctxt->current;
	vm::write32(current, 0x3fead | (1 << 18));
	vm::write32(current + 4, id);
	ctxt->current += 8;

	if(ctxt.addr() == gcm_info.context_addr)
	{
		auto& ctrl = vm::get_ref<CellGcmControl>(gcm_info.control_addr);
		ctrl.put.atomic_op([](be_t<u32>& value)
		{
			value += 8;
		});
	}

	return id;
}

int cellGcmSetSecondVFrequency(u32 freq)
{
	cellGcmSys->Warning("cellGcmSetSecondVFrequency(level=%d)", freq);

	switch (freq)
	{
	case CELL_GCM_DISPLAY_FREQUENCY_59_94HZ:
		cellGcmSys->Todo("Unimplemented display frequency: 59.94Hz");
	case CELL_GCM_DISPLAY_FREQUENCY_SCANOUT:
		cellGcmSys->Todo("Unimplemented display frequency: Scanout");
	case CELL_GCM_DISPLAY_FREQUENCY_DISABLE:
		Emu.GetGSManager().GetRender().m_frequency_mode = freq;
		break;

	default: return CELL_EINVAL;
	}

	return CELL_OK;
}

int cellGcmSetTileInfo(u8 index, u8 location, u32 offset, u32 size, u32 pitch, u8 comp, u16 base, u8 bank)
{
	cellGcmSys->Warning("cellGcmSetTileInfo(index=%d, location=%d, offset=%d, size=%d, pitch=%d, comp=%d, base=%d, bank=%d)",
		index, location, offset, size, pitch, comp, base, bank);

	if (index >= RSXThread::m_tiles_count || base >= 800 || bank >= 4)
	{
		cellGcmSys->Error("cellGcmSetTileInfo : CELL_GCM_ERROR_INVALID_VALUE");
		return CELL_GCM_ERROR_INVALID_VALUE;
	}

	if (offset & 0xffff || size & 0xffff || pitch & 0xf)
	{
		cellGcmSys->Error("cellGcmSetTileInfo : CELL_GCM_ERROR_INVALID_ALIGNMENT");
		return CELL_GCM_ERROR_INVALID_ALIGNMENT;
	}

	if (location >= 2 || (comp != 0 && (comp < 7 || comp > 12)))
	{
		cellGcmSys->Error("cellGcmSetTileInfo : CELL_GCM_ERROR_INVALID_ALIGNMENT");
		return CELL_GCM_ERROR_INVALID_ENUM;
	}

	if (comp)
	{
		cellGcmSys->Error("cellGcmSetTileInfo: bad compression mode! (%d)", comp);
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

void cellGcmSetUserHandler(vm::ptr<void(*)(const u32)> handler)
{
	cellGcmSys->Warning("cellGcmSetUserHandler(handler_addr=0x%x)", handler.addr());

	Emu.GetGSManager().GetRender().m_user_handler = handler;
}

void cellGcmSetVBlankHandler(vm::ptr<void(*)(const u32)> handler)
{
	cellGcmSys->Warning("cellGcmSetVBlankHandler(handler_addr=0x%x)", handler.addr());

	Emu.GetGSManager().GetRender().m_vblank_handler = handler;
}

int cellGcmSetWaitFlip(vm::ptr<CellGcmContextData> ctxt)
{
	cellGcmSys->Log("cellGcmSetWaitFlip(ctx=0x%x)", ctxt.addr());

	GSLockCurrent lock(GS_LOCK_WAIT_FLIP);
	return CELL_OK;
}

int cellGcmSetZcull(u8 index, u32 offset, u32 width, u32 height, u32 cullStart, u32 zFormat, u32 aaFormat, u32 zCullDir, u32 zCullFormat, u32 sFunc, u32 sRef, u32 sMask)
{
	cellGcmSys->Todo("cellGcmSetZcull(index=%d, offset=0x%x, width=%d, height=%d, cullStart=0x%x, zFormat=0x%x, aaFormat=0x%x, zCullDir=0x%x, zCullFormat=0x%x, sFunc=0x%x, sRef=0x%x, sMask=0x%x)",
		index, offset, width, height, cullStart, zFormat, aaFormat, zCullDir, zCullFormat, sFunc, sRef, sMask);

	if (index >= RSXThread::m_zculls_count)
	{
		cellGcmSys->Error("cellGcmSetZcull : CELL_GCM_ERROR_INVALID_VALUE");
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

int cellGcmUnbindTile(u8 index)
{
	cellGcmSys->Warning("cellGcmUnbindTile(index=%d)", index);

	if (index >= RSXThread::m_tiles_count)
	{
		cellGcmSys->Error("cellGcmUnbindTile : CELL_GCM_ERROR_INVALID_VALUE");
		return CELL_GCM_ERROR_INVALID_VALUE;
	}

	auto& tile = Emu.GetGSManager().GetRender().m_tiles[index];
	tile.m_binded = false;

	return CELL_OK;
}

int cellGcmUnbindZcull(u8 index)
{
	cellGcmSys->Warning("cellGcmUnbindZcull(index=%d)", index);

	if (index >= 8)
	{
		cellGcmSys->Error("cellGcmUnbindZcull : CELL_EINVAL");
		return CELL_EINVAL;
	}

	auto& zcull = Emu.GetGSManager().GetRender().m_zculls[index];
	zcull.m_binded = false;

	return CELL_OK;
}

u32 cellGcmGetTileInfo()
{
	cellGcmSys->Warning("cellGcmGetTileInfo()");
	return Emu.GetGSManager().GetRender().m_tiles_addr;
}

u32 cellGcmGetZcullInfo()
{
	cellGcmSys->Warning("cellGcmGetZcullInfo()");
	return Emu.GetGSManager().GetRender().m_zculls_addr;
}

u32 cellGcmGetDisplayInfo()
{
	cellGcmSys->Warning("cellGcmGetDisplayInfo() = 0x%x", Emu.GetGSManager().GetRender().m_gcm_buffers_addr);
	return Emu.GetGSManager().GetRender().m_gcm_buffers_addr;
}

int cellGcmGetCurrentDisplayBufferId(u32 id_addr)
{
	cellGcmSys->Warning("cellGcmGetCurrentDisplayBufferId(id_addr=0x%x)", id_addr);

	vm::write32(id_addr, Emu.GetGSManager().GetRender().m_gcm_current_buffer);

	return CELL_OK;
}

int cellGcmSetInvalidateTile()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

int cellGcmDumpGraphicsError()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

int cellGcmGetDisplayBufferByFlipIndex()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

u64 cellGcmGetLastFlipTime()
{
	cellGcmSys->Log("cellGcmGetLastFlipTime()");

	return Emu.GetGSManager().GetRender().m_last_flip_time;
}

int cellGcmGetLastSecondVTime()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

u64 cellGcmGetVBlankCount()
{
	cellGcmSys->Log("cellGcmGetVBlankCount()");

	return Emu.GetGSManager().GetRender().m_vblank_count;
}

int cellGcmInitSystemMode(u64 mode)
{
	cellGcmSys->Log("cellGcmInitSystemMode(mode=0x%x)", mode);

	system_mode = mode;

	return CELL_OK;
}

int cellGcmSetFlipImmediate(u8 id)
{
	cellGcmSys->Todo("cellGcmSetFlipImmediate(fid=0x%x)", id);

	if (id > 7)
	{
		cellGcmSys->Error("cellGcmSetFlipImmediate : CELL_GCM_ERROR_FAILURE");
		return CELL_GCM_ERROR_FAILURE;
	}

	cellGcmSetFlipMode(id);

	return CELL_OK;
}

int cellGcmSetGraphicsHandler()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

int cellGcmSetQueueHandler()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

int cellGcmSetSecondVHandler()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

int cellGcmSetVBlankFrequency()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

int cellGcmSortRemapEaIoAddress()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

//----------------------------------------------------------------------------
// Memory Mapping
//----------------------------------------------------------------------------
s32 cellGcmAddressToOffset(u64 address, vm::ptr<be_t<u32>> offset)
{
	cellGcmSys->Log("cellGcmAddressToOffset(address=0x%x,offset_addr=0x%x)", address, offset.addr());

	// Address not on main memory or local memory
	if (!address || address >= 0xD0000000) {
		cellGcmSys->Error("cellGcmAddressToOffset(address=0x%x,offset_addr=0x%x)", address, offset.addr());
		return CELL_GCM_ERROR_FAILURE;
	}

	u32 result;

	// Address in local memory
	if (Memory.RSXFBMem.IsInMyRange(address)) {
		result = (u32)(address - Memory.RSXFBMem.GetStartAddr());
	}
	// Address in main memory else check 
	else
	{
		u16 upper12Bits = offsetTable.ioAddress[address >> 20];

		// If the address is mapped in IO
		if (upper12Bits != 0xFFFF) {
			result = ((u64)upper12Bits << 20) | (address & 0xFFFFF);
		}
		else {
			return CELL_GCM_ERROR_FAILURE;
		}
	}

	*offset = result;
	return CELL_OK;
}

u32 cellGcmGetMaxIoMapSize()
{
	cellGcmSys->Log("cellGcmGetMaxIoMapSize()");

	return (u32)(Memory.RSXIOMem.GetEndAddr() - Memory.RSXIOMem.GetReservedAmount());
}

void cellGcmGetOffsetTable(vm::ptr<CellGcmOffsetTable> table)
{
	cellGcmSys->Log("cellGcmGetOffsetTable(table_addr=0x%x)", table.addr());

	table->ioAddress = offsetTable.ioAddress;
	table->eaAddress = offsetTable.eaAddress;
}

s32 cellGcmIoOffsetToAddress(u32 ioOffset, u64 address)
{
	cellGcmSys->Log("cellGcmIoOffsetToAddress(ioOffset=0x%x, address=0x%llx)", ioOffset, address);

	u64 realAddr;

	if (!Memory.RSXIOMem.getRealAddr(ioOffset, realAddr)) 
		return CELL_GCM_ERROR_FAILURE;

	vm::write64(address, realAddr);

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
		cellGcmSys->Error("cellGcmMapEaIoAddress : CELL_GCM_ERROR_FAILURE");
		return CELL_GCM_ERROR_FAILURE;
	}

	return CELL_OK;
}

s32 cellGcmMapEaIoAddress(u32 ea, u32 io, u32 size)
{
	cellGcmSys->Warning("cellGcmMapEaIoAddress(ea=0x%x, io=0x%x, size=0x%x)", ea, io, size);

	return gcmMapEaIoAddress(ea, io, size, false);
}

s32 cellGcmMapEaIoAddressWithFlags(u32 ea, u32 io, u32 size, u32 flags)
{
	cellGcmSys->Warning("cellGcmMapEaIoAddressWithFlags(ea=0x%x, io=0x%x, size=0x%x, flags=0x%x)", ea, io, size, flags);

	assert(flags == 2 /*CELL_GCM_IOMAP_FLAG_STRICT_ORDERING*/);

	return gcmMapEaIoAddress(ea, io, size, true);
}

s32 cellGcmMapLocalMemory(u64 address, u64 size)
{
	cellGcmSys->Warning("cellGcmMapLocalMemory(address=0x%llx, size=0x%llx)", address, size);

	if (!local_size && !local_addr)
	{
		local_size = 0xf900000; //TODO
		local_addr = (u32)Memory.RSXFBMem.GetStartAddr();
		Memory.RSXFBMem.AllocAlign(local_size);
		vm::write32(address, local_addr);
		vm::write32(size, local_size);
	}
	else
	{
		cellGcmSys->Error("RSX local memory already mapped");
		return CELL_GCM_ERROR_FAILURE;
	}

	return CELL_OK;
}

s32 cellGcmMapMainMemory(u32 ea, u32 size, vm::ptr<u32> offset)
{
	cellGcmSys->Warning("cellGcmMapMainMemory(ea=0x%x,size=0x%x,offset_addr=0x%x)", ea, size, offset.addr());

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
		cellGcmSys->Error("cellGcmMapMainMemory : CELL_GCM_ERROR_NO_IO_PAGE_TABLE");
		return CELL_GCM_ERROR_NO_IO_PAGE_TABLE;
	}

	Emu.GetGSManager().GetRender().m_main_mem_addr = Emu.GetGSManager().GetRender().m_ioAddress;

	return CELL_OK;
}

s32 cellGcmReserveIoMapSize(u32 size)
{
	cellGcmSys->Log("cellGcmReserveIoMapSize(size=0x%x)", size);

	if (size & 0xFFFFF)
	{
		cellGcmSys->Error("cellGcmReserveIoMapSize : CELL_GCM_ERROR_INVALID_ALIGNMENT");
		return CELL_GCM_ERROR_INVALID_ALIGNMENT;
	}

	if (size > cellGcmGetMaxIoMapSize())
	{
		cellGcmSys->Error("cellGcmReserveIoMapSize : CELL_GCM_ERROR_INVALID_VALUE");
		return CELL_GCM_ERROR_INVALID_VALUE;
	}

	Memory.RSXIOMem.Reserve(size);
	return CELL_OK;
}

s32 cellGcmUnmapEaIoAddress(u64 ea)
{
	cellGcmSys->Log("cellGcmUnmapEaIoAddress(ea=0x%llx)", ea);

	u32 size;
	if (Memory.RSXIOMem.UnmapRealAddress(ea, size))
	{
		u64 io;
		ea = ea >> 20;
		io = offsetTable.ioAddress[ea];

		for (u32 i = 0; i<size; i++)
		{
			offsetTable.ioAddress[ea + i] = 0xFFFF;
			offsetTable.eaAddress[io + i] = 0xFFFF;
		}
	}
	else
	{
		cellGcmSys->Error("cellGcmUnmapEaIoAddress : CELL_GCM_ERROR_FAILURE");
		return CELL_GCM_ERROR_FAILURE;
	}

	return CELL_OK;
}

s32 cellGcmUnmapIoAddress(u64 io)
{
	cellGcmSys->Log("cellGcmUnmapIoAddress(io=0x%llx)", io);

	u32 size;
	if (Memory.RSXIOMem.UnmapAddress(io, size))
	{
		u64 ea;
		io = io >> 20;
		ea = offsetTable.eaAddress[io];

		for (u32 i = 0; i<size; i++)
		{
			offsetTable.ioAddress[ea + i] = 0xFFFF;
			offsetTable.eaAddress[io + i] = 0xFFFF;
		}
	}
	else
	{
		cellGcmSys->Error("cellGcmUnmapIoAddress : CELL_GCM_ERROR_FAILURE");
		return CELL_GCM_ERROR_FAILURE;
	}

	return CELL_OK;
}

s32 cellGcmUnreserveIoMapSize(u32 size)
{
	cellGcmSys->Log("cellGcmUnreserveIoMapSize(size=0x%x)", size);

	if (size & 0xFFFFF)
	{
		cellGcmSys->Error("cellGcmReserveIoMapSize : CELL_GCM_ERROR_INVALID_ALIGNMENT");
		return CELL_GCM_ERROR_INVALID_ALIGNMENT;
	}

	if (size > Memory.RSXIOMem.GetReservedAmount())
	{
		cellGcmSys->Error("cellGcmReserveIoMapSize : CELL_GCM_ERROR_INVALID_VALUE");
		return CELL_GCM_ERROR_INVALID_VALUE;
	}

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
	cellGcmSys->Warning("cellGcmSetDefaultCommandBuffer()");
	vm::write32(Emu.GetGSManager().GetRender().m_ctxt_addr, gcm_info.context_addr);
}

//------------------------------------------------------------------------
// Other
//------------------------------------------------------------------------

int cellGcmSetFlipCommand(vm::ptr<CellGcmContextData> ctx, u32 id)
{
	cellGcmSys->Log("cellGcmSetFlipCommand(ctx_addr=0x%x, id=0x%x)", ctx.addr(), id);

	return cellGcmSetPrepareFlip(ctx, id);
}

int cellGcmSetFlipCommandWithWaitLabel(vm::ptr<CellGcmContextData> ctx, u32 id, u32 label_index, u32 label_value)
{
	cellGcmSys->Log("cellGcmSetFlipCommandWithWaitLabel(ctx_addr=0x%x, id=0x%x, label_index=0x%x, label_value=0x%x)",
		ctx.addr(), id, label_index, label_value);

	int res = cellGcmSetPrepareFlip(ctx, id);
	vm::write32(Memory.RSXCMDMem.GetStartAddr() + 0x10 * label_index, label_value);
	return res < 0 ? CELL_GCM_ERROR_FAILURE : CELL_OK;
}

int cellGcmSetTile(u8 index, u8 location, u32 offset, u32 size, u32 pitch, u8 comp, u16 base, u8 bank)
{
	cellGcmSys->Warning("cellGcmSetTile(index=%d, location=%d, offset=%d, size=%d, pitch=%d, comp=%d, base=%d, bank=%d)",
		index, location, offset, size, pitch, comp, base, bank);

	// Copied form cellGcmSetTileInfo
	if(index >= RSXThread::m_tiles_count || base >= 800 || bank >= 4)
	{
		cellGcmSys->Error("cellGcmSetTile : CELL_GCM_ERROR_INVALID_VALUE");
		return CELL_GCM_ERROR_INVALID_VALUE;
	}

	if(offset & 0xffff || size & 0xffff || pitch & 0xf)
	{
		cellGcmSys->Error("cellGcmSetTile : CELL_GCM_ERROR_INVALID_ALIGNMENT");
		return CELL_GCM_ERROR_INVALID_ALIGNMENT;
	}

	if(location >= 2 || (comp != 0 && (comp < 7 || comp > 12)))
	{
		cellGcmSys->Error("cellGcmSetTile : CELL_GCM_ERROR_INVALID_ENUM");
		return CELL_GCM_ERROR_INVALID_ENUM;
	}

	if(comp)
	{
		cellGcmSys->Error("cellGcmSetTile: bad compression mode! (%d)", comp);
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
	cellGcmSys->Log("cellGcmCallback(context_addr=0x%x, count=0x%x)", context.addr(), count);

	GSLockCurrent gslock(GS_LOCK_WAIT_FLUSH);

	if (1)
	{
		auto& ctrl = vm::get_ref<CellGcmControl>(gcm_info.control_addr);
		be_t<u32> res = be_t<u32>::make(context->current - context->begin - ctrl.put.read_relaxed());
		memmove(vm::get_ptr<void>(context->begin), vm::get_ptr<void>(context->current - res), res);

		context->current = context->begin + res;
		ctrl.put.write_relaxed(res);
		ctrl.get.write_relaxed(be_t<u32>::make(0));

		return CELL_OK;
	}

	//auto& ctrl = vm::get_ref<CellGcmControl>(gcm_info.control_addr);

	// preparations for changing the place (for optimized FIFO mode)
	//auto cmd = vm::ptr<u32>::make(context->current.ToLE());
	//cmd[0] = 0x41D6C;
	//cmd[1] = 0x20;
	//cmd[2] = 0x41D74;
	//cmd[3] = 0; // some incrementing by module value
	//context->current += 0x10;

	if (1)
	{
		const u32 address = context->begin;
		const u32 upper = offsetTable.ioAddress[address >> 20]; // 12 bits
		assert(upper != 0xFFFF);
		const u32 offset = (upper << 20) | (address & 0xFFFFF);
		vm::write32(context->current, CELL_GCM_METHOD_FLAG_JUMP | offset); // set JUMP cmd

		auto& ctrl = vm::get_ref<CellGcmControl>(gcm_info.control_addr);
		ctrl.put.write_relaxed(be_t<u32>::make(offset));
	}
	else
	{
		vm::write32(context->current, CELL_GCM_METHOD_FLAG_JUMP | CELL_GCM_METHOD_FLAG_NON_INCREMENT | (0));
	}
	
	context->current = context->begin; // rewind to the beginning
	// TODO: something is missing
	return CELL_OK;
}

//----------------------------------------------------------------------------

void cellGcmSys_init(Module *pxThis)
{
	cellGcmSys = pxThis;

	// Data Retrieval
	cellGcmSys->AddFunc(0xc8f3bd09, cellGcmGetCurrentField);
	cellGcmSys->AddFunc(0xf80196c1, cellGcmGetLabelAddress);
	cellGcmSys->AddFunc(0x21cee035, cellGcmGetNotifyDataAddress);
	cellGcmSys->AddFunc(0x661fe266, _cellGcmFunc12);
	cellGcmSys->AddFunc(0x99d397ac, cellGcmGetReport);
	cellGcmSys->AddFunc(0x9a0159af, cellGcmGetReportDataAddress);
	cellGcmSys->AddFunc(0x8572bce2, cellGcmGetReportDataAddressLocation);
	cellGcmSys->AddFunc(0xa6b180ac, cellGcmGetReportDataLocation);
	cellGcmSys->AddFunc(0x5a41c10f, cellGcmGetTimeStamp);
	cellGcmSys->AddFunc(0x2ad4951b, cellGcmGetTimeStampLocation);

	// Command Buffer Control
	cellGcmSys->AddFunc(0xa547adde, cellGcmGetControlRegister);
	cellGcmSys->AddFunc(0x5e2ee0f0, cellGcmGetDefaultCommandWordSize);
	cellGcmSys->AddFunc(0x8cdf8c70, cellGcmGetDefaultSegmentWordSize);
	cellGcmSys->AddFunc(0xcaabd992, cellGcmInitDefaultFifoMode);
	cellGcmSys->AddFunc(0x9ba451e4, cellGcmSetDefaultFifoSize);
	//cellGcmSys->AddFunc(, cellGcmReserveMethodSize);
	//cellGcmSys->AddFunc(, cellGcmResetDefaultCommandBuffer);
	//cellGcmSys->AddFunc(, cellGcmSetupContextData);
	//cellGcmSys->AddFunc(, cellGcmCallbackForSnc);
	//cellGcmSys->AddFunc(, cellGcmFinish);
	//cellGcmSys->AddFunc(, cellGcmFlush);

	// Hardware Resource Management
	cellGcmSys->AddFunc(0x4524cccd, cellGcmBindTile);
	cellGcmSys->AddFunc(0x9dc04436, cellGcmBindZcull);
	cellGcmSys->AddFunc(0x1f61b3ff, cellGcmDumpGraphicsError);
	cellGcmSys->AddFunc(0xe315a0b2, cellGcmGetConfiguration);
	cellGcmSys->AddFunc(0x371674cf, cellGcmGetDisplayBufferByFlipIndex);
	cellGcmSys->AddFunc(0x72a577ce, cellGcmGetFlipStatus);
	cellGcmSys->AddFunc(0x63387071, cellGcmGetLastFlipTime);
	cellGcmSys->AddFunc(0x23ae55a3, cellGcmGetLastSecondVTime);
	cellGcmSys->AddFunc(0x055bd74d, cellGcmGetTiledPitchSize);
	cellGcmSys->AddFunc(0x723bbc7e, cellGcmGetVBlankCount);
	cellGcmSys->AddFunc(0x5f909b17, _cellGcmFunc1);
	cellGcmSys->AddFunc(0x3a33c1fd, _cellGcmFunc15);
	cellGcmSys->AddFunc(0x15bae46b, _cellGcmInitBody);
	cellGcmSys->AddFunc(0xfce9e764, cellGcmInitSystemMode);
	cellGcmSys->AddFunc(0xb2e761d4, cellGcmResetFlipStatus);
	cellGcmSys->AddFunc(0x51c9d62b, cellGcmSetDebugOutputLevel);
	cellGcmSys->AddFunc(0xa53d12ae, cellGcmSetDisplayBuffer);
	cellGcmSys->AddFunc(0xdc09357e, cellGcmSetFlip);
	cellGcmSys->AddFunc(0xa41ef7e8, cellGcmSetFlipHandler);
	cellGcmSys->AddFunc(0xacee8542, cellGcmSetFlipImmediate);
	cellGcmSys->AddFunc(0x4ae8d215, cellGcmSetFlipMode);
	cellGcmSys->AddFunc(0xa47c09ff, cellGcmSetFlipStatus);
	cellGcmSys->AddFunc(0xd01b570d, cellGcmSetGraphicsHandler);
	cellGcmSys->AddFunc(0x0b4b62d5, cellGcmSetPrepareFlip);
	cellGcmSys->AddFunc(0x0a862772, cellGcmSetQueueHandler);
	cellGcmSys->AddFunc(0x4d7ce993, cellGcmSetSecondVFrequency);
	cellGcmSys->AddFunc(0xdc494430, cellGcmSetSecondVHandler);
	cellGcmSys->AddFunc(0xbd100dbc, cellGcmSetTileInfo);
	cellGcmSys->AddFunc(0x06edea9e, cellGcmSetUserHandler);
	cellGcmSys->AddFunc(0xffe0160e, cellGcmSetVBlankFrequency);
	cellGcmSys->AddFunc(0xa91b0402, cellGcmSetVBlankHandler);
	cellGcmSys->AddFunc(0x983fb9aa, cellGcmSetWaitFlip);
	cellGcmSys->AddFunc(0xd34a420d, cellGcmSetZcull);
	cellGcmSys->AddFunc(0x25b40ab4, cellGcmSortRemapEaIoAddress);
	cellGcmSys->AddFunc(0xd9b7653e, cellGcmUnbindTile);
	cellGcmSys->AddFunc(0xa75640e8, cellGcmUnbindZcull);
	cellGcmSys->AddFunc(0x657571f7, cellGcmGetTileInfo);
	cellGcmSys->AddFunc(0xd9a0a879, cellGcmGetZcullInfo);
	cellGcmSys->AddFunc(0x0e6b0dae, cellGcmGetDisplayInfo);
	cellGcmSys->AddFunc(0x93806525, cellGcmGetCurrentDisplayBufferId);
	cellGcmSys->AddFunc(0xbd6d60d9, cellGcmSetInvalidateTile);
	//cellGcmSys->AddFunc(, cellGcmSetFlipWithWaitLabel);

	// Memory Mapping
	cellGcmSys->AddFunc(0x21ac3697, cellGcmAddressToOffset);
	cellGcmSys->AddFunc(0xfb81c03e, cellGcmGetMaxIoMapSize);
	cellGcmSys->AddFunc(0x2922aed0, cellGcmGetOffsetTable);
	cellGcmSys->AddFunc(0x2a6fba9c, cellGcmIoOffsetToAddress);
	cellGcmSys->AddFunc(0x63441cb4, cellGcmMapEaIoAddress);
	cellGcmSys->AddFunc(0x626e8518, cellGcmMapEaIoAddressWithFlags);
	cellGcmSys->AddFunc(0xdb769b32, cellGcmMapLocalMemory);
	cellGcmSys->AddFunc(0xa114ec67, cellGcmMapMainMemory);
	cellGcmSys->AddFunc(0xa7ede268, cellGcmReserveIoMapSize);
	cellGcmSys->AddFunc(0xefd00f54, cellGcmUnmapEaIoAddress);
	cellGcmSys->AddFunc(0xdb23e867, cellGcmUnmapIoAddress);
	cellGcmSys->AddFunc(0x3b9bd5bd, cellGcmUnreserveIoMapSize);

	// Cursor
	cellGcmSys->AddFunc(0x107bf3a1, cellGcmInitCursor);
	cellGcmSys->AddFunc(0xc47d0812, cellGcmSetCursorEnable);
	cellGcmSys->AddFunc(0x69c6cc82, cellGcmSetCursorDisable);
	cellGcmSys->AddFunc(0xf9bfdc72, cellGcmSetCursorImageOffset);
	cellGcmSys->AddFunc(0x1a0de550, cellGcmSetCursorPosition);
	cellGcmSys->AddFunc(0xbd2fa0a7, cellGcmUpdateCursor);

	// Functions for Maintaining Compatibility
	cellGcmSys->AddFunc(0xbc982946, cellGcmSetDefaultCommandBuffer);
	//cellGcmSys->AddFunc(, cellGcmGetCurrentBuffer);
	//cellGcmSys->AddFunc(, cellGcmSetCurrentBuffer);
	//cellGcmSys->AddFunc(, cellGcmSetDefaultCommandBufferAndSegmentWordSize);
	//cellGcmSys->AddFunc(, cellGcmSetUserCallback);

	// Other
	cellGcmSys->AddFunc(0x21397818, cellGcmSetFlipCommand);
	cellGcmSys->AddFunc(0xd8f88e1a, cellGcmSetFlipCommandWithWaitLabel);
	cellGcmSys->AddFunc(0xd0b1d189, cellGcmSetTile);
}

void cellGcmSys_load()
{
	current_config.ioAddress = 0;
	current_config.localAddress = 0;
	local_size = 0;
	local_addr = 0;
}

void cellGcmSys_unload()
{
}
