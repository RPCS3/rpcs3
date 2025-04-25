#include "stdafx.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"

#include "Emu/Memory/vm.h"
#include "Emu/Cell/lv2/sys_ppu_thread.h"
#include "Emu/Cell/lv2/sys_rsx.h"
#include "Emu/RSX/RSXThread.h"

#include "cellGcmSys.h"
#include "sysPrxForUser.h"

#include "util/asm.hpp"

LOG_CHANNEL(cellGcmSys);

template<>
void fmt_class_string<CellGcmError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
		STR_CASE(CELL_GCM_ERROR_FAILURE);
		STR_CASE(CELL_GCM_ERROR_NO_IO_PAGE_TABLE);
		STR_CASE(CELL_GCM_ERROR_INVALID_ENUM);
		STR_CASE(CELL_GCM_ERROR_INVALID_VALUE);
		STR_CASE(CELL_GCM_ERROR_INVALID_ALIGNMENT);
		STR_CASE(CELL_GCM_ERROR_ADDRESS_OVERWRAP);
		}

		return unknown;
	});
}

namespace rsx
{
	u32 make_command(vm::bptr<u32>& dst, u32 start_register, std::initializer_list<any32> values)
	{
		*dst++ = start_register << 2 | static_cast<u32>(values.size()) << 18;

		for (const any32& cmd : values)
		{
			*dst++ = cmd.as<u32>();
		}

		return u32{sizeof(u32)} * (static_cast<u32>(values.size()) + 1);
	}

	u32 make_jump(vm::bptr<u32>& dst, u32 offset)
	{
		*dst++ = RSX_METHOD_OLD_JUMP_CMD | offset;

		return sizeof(u32);
	}
}

extern s32 cellGcmCallback(ppu_thread& ppu, vm::ptr<CellGcmContextData> context, u32 count);

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

// Auxiliary functions

/*
 * Get usable local memory size for a specific game SDK version
 * Example: For 0x00446000 (FW 4.46) we get a localSize of 0x0F900000 (249MB)
 */
u32 gcmGetLocalMemorySize(u32 sdk_version)
{
	if (sdk_version >= 0x00220000)
	{
		return 0x0F900000; // 249MB
	}
	if (sdk_version >= 0x00200000)
	{
		return 0x0F200000; // 242MB
	}
	if (sdk_version >= 0x00190000)
	{
		return 0x0EA00000; // 234MB
	}
	if (sdk_version >= 0x00180000)
	{
		return 0x0E800000; // 232MB
	}
	return 0x0E000000; // 224MB
}

error_code gcmMapEaIoAddress(ppu_thread& ppu, u32 ea, u32 io, u32 size, bool is_strict);

u32 gcmIoOffsetToAddress(u32 ioOffset)
{
	const u32 upper12Bits = g_fxo->get<gcm_config>().offsetTable.eaAddress[ioOffset >> 20];

	if (upper12Bits > 0xBFF)
	{
		return 0;
	}

	return (upper12Bits << 20) | (ioOffset & 0xFFFFF);
}

void InitOffsetTable()
{
	auto& cfg = g_fxo->get<gcm_config>();

	const u32 addr = vm::alloc((3072 + 512) * sizeof(u16), vm::main);

	cfg.offsetTable.ioAddress.set(addr);
	cfg.offsetTable.eaAddress.set(addr + (3072 * sizeof(u16)));

	std::memset(vm::base(addr), 0xFF, (3072 + 512) * sizeof(u16));
}

//----------------------------------------------------------------------------
// Data Retrieval
//----------------------------------------------------------------------------

u32 cellGcmGetLabelAddress(u8 index)
{
	cellGcmSys.trace("cellGcmGetLabelAddress(index=%d)", index);
	return rsx::get_current_renderer()->label_addr + 0x10 * index;
}

vm::ptr<CellGcmReportData> cellGcmGetReportDataAddressLocation(u32 index, u32 location)
{
	cellGcmSys.warning("cellGcmGetReportDataAddressLocation(index=%d, location=%d)", index, location);

	if (location == CELL_GCM_LOCATION_MAIN)
	{
		if (index >= 1024 * 1024)
		{
			cellGcmSys.error("cellGcmGetReportDataAddressLocation: Wrong main index (%d)", index);
		}

		return vm::cast(gcmIoOffsetToAddress(0x0e000000 + index * 0x10));
	}

	// Anything else is Local

	if (index >= 2048)
	{
		cellGcmSys.error("cellGcmGetReportDataAddressLocation: Wrong local index (%d)", index);
	}

	return vm::cast(rsx::get_current_renderer()->label_addr + ::offset32(&RsxReports::report) + index * 0x10);
}

u64 cellGcmGetTimeStamp(u32 index)
{
	cellGcmSys.trace("cellGcmGetTimeStamp(index=%d)", index);

	if (index >= 2048)
	{
		cellGcmSys.error("cellGcmGetTimeStamp: Wrong local index (%d)", index);
	}

	const u32 address = rsx::get_current_renderer()->label_addr + ::offset32(&RsxReports::report) + index * 0x10;
	return *vm::get_super_ptr<u64>(address);
}

u32 cellGcmGetCurrentField()
{
	cellGcmSys.todo("cellGcmGetCurrentField()");
	return 0;
}

u32 cellGcmGetNotifyDataAddress(u32 index)
{
	cellGcmSys.warning("cellGcmGetNotifyDataAddress(index=%d)", index);

	// If entry not in use, return NULL
	u16 entry = g_fxo->get<gcm_config>().offsetTable.eaAddress[241];
	if (entry == 0xFFFF) {
		return 0;
	}

	return (entry << 20) + (index * 0x40);
}

/*
 *  Get base address of local report data area
 */
vm::ptr<CellGcmReportData> _cellGcmFunc12()
{
	return vm::ptr<CellGcmReportData>::make(rsx::get_current_renderer()->label_addr + ::offset32(&RsxReports::report)); // TODO
}

u32 cellGcmGetReport(u32 type, u32 index)
{
	cellGcmSys.warning("cellGcmGetReport(type=%d, index=%d)", type, index);

	if (index >= 2048)
	{
		cellGcmSys.error("cellGcmGetReport: Wrong local index (%d)", index);
	}

	if (type < 1 || type > 5) {
		return -1;
	}

	vm::ptr<CellGcmReportData> local_reports = _cellGcmFunc12();
	return local_reports[index].value;
}

u32 cellGcmGetReportDataAddress(u32 index)
{
	cellGcmSys.warning("cellGcmGetReportDataAddress(index=%d)", index);

	if (index >= 2048)
	{
		cellGcmSys.error("cellGcmGetReportDataAddress: Wrong local index (%d)", index);
	}

	return rsx::get_current_renderer()->label_addr + ::offset32(&RsxReports::report) + index * 0x10;
}

u32 cellGcmGetReportDataLocation(u32 index, u32 location)
{
	cellGcmSys.warning("cellGcmGetReportDataLocation(index=%d, location=%d)", index, location);

	vm::ptr<CellGcmReportData> report = cellGcmGetReportDataAddressLocation(index, location);
	return report->value;
}

u64 cellGcmGetTimeStampLocation(u32 index, u32 location)
{
	cellGcmSys.warning("cellGcmGetTimeStampLocation(index=%d, location=%d)", index, location);

	// NOTE: No error checkings
	return cellGcmGetReportDataAddressLocation(index, location)->timer;
}

//----------------------------------------------------------------------------
// Command Buffer Control
//----------------------------------------------------------------------------

u32 cellGcmGetControlRegister()
{
	cellGcmSys.trace("cellGcmGetControlRegister()");
	return g_fxo->get<gcm_config>().gcm_info.control_addr;
}

u32 cellGcmGetDefaultCommandWordSize()
{
	cellGcmSys.trace("cellGcmGetDefaultCommandWordSize()");
	return g_fxo->get<gcm_config>().gcm_info.command_size;
}

u32 cellGcmGetDefaultSegmentWordSize()
{
	cellGcmSys.trace("cellGcmGetDefaultSegmentWordSize()");
	return g_fxo->get<gcm_config>().gcm_info.segment_size;
}

error_code cellGcmInitDefaultFifoMode(s32 mode)
{
	cellGcmSys.warning("cellGcmInitDefaultFifoMode(mode=%d)", mode);
	return CELL_OK;
}

error_code cellGcmSetDefaultFifoSize(u32 bufferSize, u32 segmentSize)
{
	cellGcmSys.warning("cellGcmSetDefaultFifoSize(bufferSize=0x%x, segmentSize=0x%x)", bufferSize, segmentSize);
	return CELL_OK;
}

//----------------------------------------------------------------------------
// Hardware Resource Management
//----------------------------------------------------------------------------

error_code cellGcmBindTile(u8 index)
{
	cellGcmSys.warning("cellGcmBindTile(index=%d)", index);

	if (index >= rsx::limits::tiles_count)
	{
		return CELL_GCM_ERROR_INVALID_VALUE;
	}

	rsx::get_current_renderer()->tiles[index].bound = true;

	return CELL_OK;
}

error_code cellGcmBindZcull(u8 index, u32 offset, u32 width, u32 height, u32 cullStart, u32 zFormat, u32 aaFormat, u32 zCullDir, u32 zCullFormat, u32 sFunc, u32 sRef, u32 sMask)
{
	cellGcmSys.warning("cellGcmBindZcull(index=%d, offset=0x%x, width=%d, height=%d, cullStart=0x%x, zFormat=0x%x, aaFormat=0x%x, zCullDir=0x%x, zCullFormat=0x%x, sFunc=0x%x, sRef=0x%x, sMask=0x%x)",
		index, offset, width, height, cullStart, zFormat, aaFormat, zCullDir, zCullFormat, sFunc, sRef, sMask);

	auto& gcm_cfg = g_fxo->get<gcm_config>();

	GcmZcullInfo zcull{};
	zcull.offset = offset;
	zcull.width = width;
	zcull.height = height;
	zcull.cullStart = cullStart;
	zcull.zFormat = zFormat;
	zcull.aaFormat = aaFormat;
	zcull.zcullDir = zCullDir;
	zcull.zcullFormat = zCullFormat;
	zcull.sFunc = sFunc;
	zcull.sRef = sRef;
	zcull.sMask = sMask;
	zcull.bound = true;

	const auto gcm_zcull = zcull.pack();

	std::lock_guard lock(gcm_cfg.gcmio_mutex);

	if (auto err = sys_rsx_context_attribute(0x5555'5555, 0x301, index, u64{gcm_zcull.region} << 32 | gcm_zcull.size, u64{gcm_zcull.start} << 32 | gcm_zcull.offset, u64{gcm_zcull.status0} << 32 | gcm_zcull.status1))
	{
		return err;
	}

	vm::_ptr<CellGcmZcullInfo>(gcm_cfg.zculls_addr)[index] = gcm_zcull;
	return CELL_OK;
}

void cellGcmGetConfiguration(vm::ptr<CellGcmConfig> config)
{
	cellGcmSys.trace("cellGcmGetConfiguration(config=*0x%x)", config);
	*config = g_fxo->get<gcm_config>().current_config;
}

u32 cellGcmGetFlipStatus()
{
	u32 status = rsx::get_current_renderer()->flip_status;

	cellGcmSys.trace("cellGcmGetFlipStatus() -> %d", status);

	return status;
}

error_code cellGcmGetFlipStatus2()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

u32 cellGcmGetTiledPitchSize(u32 size)
{
	cellGcmSys.trace("cellGcmGetTiledPitchSize(size=%d)", size);

	for (usz i = 0; i < std::size(tiled_pitches) - 1; i++) {
		if (tiled_pitches[i] < size && size <= tiled_pitches[i + 1]) {
			return tiled_pitches[i + 1];
		}
	}
	return 0;
}

void _cellGcmFunc1()
{
	cellGcmSys.todo("_cellGcmFunc1()");
	return;
}

void _cellGcmFunc15(vm::ptr<CellGcmContextData> context)
{
	cellGcmSys.todo("_cellGcmFunc15(context=*0x%x)", context);
	return;
}

u32 g_defaultCommandBufferBegin, g_defaultCommandBufferFragmentCount;

// Called by cellGcmInit
error_code _cellGcmInitBody(ppu_thread& ppu, vm::pptr<CellGcmContextData> context, u32 cmdSize, u32 ioSize, u32 ioAddress)
{
	cellGcmSys.warning("_cellGcmInitBody(context=**0x%x, cmdSize=0x%x, ioSize=0x%x, ioAddress=0x%x)", context, cmdSize, ioSize, ioAddress);

	auto& gcm_cfg = g_fxo->get<gcm_config>();
	std::lock_guard lock(gcm_cfg.gcmio_mutex);

	gcm_cfg.current_config.ioAddress = 0;
	gcm_cfg.current_config.localAddress = 0;
	gcm_cfg.local_size = 0;
	gcm_cfg.local_addr = 0;

	//if (!gcm_cfg.local_size && !gcm_cfg.local_addr)
	{
		gcm_cfg.local_size = 0xf900000; // TODO: Get sdk_version in _cellGcmFunc15 and pass it to gcmGetLocalMemorySize
		gcm_cfg.local_addr = rsx::constants::local_mem_base;
		vm::falloc(gcm_cfg.local_addr, gcm_cfg.local_size, vm::video);
	}

	cellGcmSys.warning("*** local memory(addr=0x%x, size=0x%x)", gcm_cfg.local_addr, gcm_cfg.local_size);

	InitOffsetTable();

	const auto render = rsx::get_current_renderer();
	if (gcm_cfg.system_mode == CELL_GCM_SYSTEM_MODE_IOMAP_512MB)
	{
		cellGcmSys.warning("cellGcmInit(): 512MB io address space used");
		render->main_mem_size = 0x20000000;
	}
	else
	{
		cellGcmSys.warning("cellGcmInit(): 256MB io address space used");
		render->main_mem_size = 0x10000000;
	}

	render->isHLE = true;
	render->local_mem_size = gcm_cfg.local_size;

	ensure(sys_rsx_device_map(ppu, vm::var<u64>{}, vm::null, 0x8) == CELL_OK);
	ensure(sys_rsx_context_allocate(ppu, vm::var<u32>{}, vm::var<u64>{}, vm::var<u64>{}, vm::var<u64>{}, 0, gcm_cfg.system_mode) == CELL_OK);

	if (gcmMapEaIoAddress(ppu, ioAddress, 0, ioSize, false) != CELL_OK)
	{
		return CELL_GCM_ERROR_FAILURE;
	}

	gcm_cfg.current_config.ioSize = ioSize;
	gcm_cfg.current_config.ioAddress = ioAddress;
	gcm_cfg.current_config.localSize =  gcm_cfg.local_size;
	gcm_cfg.current_config.localAddress =  gcm_cfg.local_addr;
	gcm_cfg.current_config.memoryFrequency = 650000000;
	gcm_cfg.current_config.coreFrequency = 500000000;

	const u32 rsx_ctxaddr = render->device_addr;
	ensure(rsx_ctxaddr);

	g_defaultCommandBufferBegin = ioAddress;
	g_defaultCommandBufferFragmentCount = cmdSize / (32 * 1024);

	gcm_cfg.gcm_info.context_addr = rsx_ctxaddr;
	gcm_cfg.gcm_info.control_addr = render->dma_address;
	gcm_cfg.current_context.begin.set(g_defaultCommandBufferBegin + 4096); // 4 kb reserved at the beginning
	gcm_cfg.current_context.end.set(g_defaultCommandBufferBegin + 32 * 1024 - 4); // 4b at the end for jump
	gcm_cfg.current_context.current = gcm_cfg.current_context.begin;
	gcm_cfg.current_context.callback.set(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(cellGcmCallback)));

	gcm_cfg.ctxt_addr = context.addr();
	gcm_cfg.gcm_buffers.set(vm::alloc(sizeof(CellGcmDisplayInfo) * 8, vm::main));
	gcm_cfg.zculls_addr = vm::alloc(sizeof(CellGcmZcullInfo) * 8, vm::main);
	gcm_cfg.tiles_addr = vm::alloc(sizeof(CellGcmTileInfo) * 15, vm::main);

	vm::write<CellGcmContextData>(gcm_cfg.gcm_info.context_addr, gcm_cfg.current_context);
	context->set(gcm_cfg.gcm_info.context_addr);

	// 0x40 is to offset CellGcmControl from RsxDmaControl
	gcm_cfg.gcm_info.control_addr += 0x40;

	vm::var<u64> _tid;
	vm::var<char[]> _name = vm::make_str("_gcm_intr_thread");
	ppu_execute<&sys_ppu_thread_create>(ppu, +_tid, 0x10000, 0, 1, 0x4000, SYS_PPU_THREAD_CREATE_INTERRUPT, +_name);
	render->intr_thread = idm::get_unlocked<named_thread<ppu_thread>>(static_cast<u32>(*_tid));
	render->intr_thread->state -= cpu_flag::stop;
	thread_ctrl::notify(*render->intr_thread);

	return CELL_OK;
}

void cellGcmResetFlipStatus()
{
	cellGcmSys.trace("cellGcmResetFlipStatus()");

	rsx::get_current_renderer()->flip_status = CELL_GCM_DISPLAY_FLIP_STATUS_WAITING;
}

error_code cellGcmResetFlipStatus2()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

void cellGcmSetDebugOutputLevel(s32 level)
{
	cellGcmSys.warning("cellGcmSetDebugOutputLevel(level=%d)", level);

	switch (level)
	{
	case CELL_GCM_DEBUG_LEVEL0:
	case CELL_GCM_DEBUG_LEVEL1:
	case CELL_GCM_DEBUG_LEVEL2:
		rsx::get_current_renderer()->debug_level = level;
		break;

	default:
		break;
	}
}

error_code cellGcmSetDisplayBuffer(u8 id, u32 offset, u32 pitch, u32 width, u32 height)
{
	cellGcmSys.trace("cellGcmSetDisplayBuffer(id=0x%x, offset=0x%x, pitch=%d, width=%d, height=%d)", id, offset, width ? pitch / width : pitch, width, height);

	auto& gcm_cfg = g_fxo->get<gcm_config>();

	if (id > 7)
	{
		return CELL_GCM_ERROR_FAILURE;
	}

	const auto render = rsx::get_current_renderer();

	auto buffers = render->display_buffers;

	buffers[id].offset = offset;
	buffers[id].pitch = pitch;
	buffers[id].width = width;
	buffers[id].height = height;

	gcm_cfg.gcm_buffers[id].offset = offset;
	gcm_cfg.gcm_buffers[id].pitch = pitch;
	gcm_cfg.gcm_buffers[id].width = width;
	gcm_cfg.gcm_buffers[id].height = height;

	if (id + 1u > render->display_buffers_count)
	{
		render->display_buffers_count = id + 1;
	}

	return CELL_OK;
}

void cellGcmSetFlipHandler(vm::ptr<void(u32)> handler)
{
	cellGcmSys.warning("cellGcmSetFlipHandler(handler=*0x%x)", handler);

	if (const auto rsx = rsx::get_current_renderer(); rsx->is_initialized)
	{
		rsx->flip_handler = handler;
	}
}

error_code cellGcmSetFlipHandler2()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

void cellGcmSetFlipMode(u32 mode)
{
	cellGcmSys.warning("cellGcmSetFlipMode(mode=%d)", mode);

	rsx::get_current_renderer()->requested_vsync.store(mode == CELL_GCM_DISPLAY_VSYNC);
}

error_code cellGcmSetFlipMode2()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

void cellGcmSetFlipStatus()
{
	cellGcmSys.warning("cellGcmSetFlipStatus()");

	rsx::get_current_renderer()->flip_status = CELL_GCM_DISPLAY_FLIP_STATUS_DONE;
}

error_code cellGcmSetFlipStatus2()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

template <bool old_api = false, typename ret_type = std::conditional_t<old_api, s32, error_code>>
ret_type gcmSetPrepareFlip(ppu_thread& ppu, vm::ptr<CellGcmContextData> ctxt, u32 id)
{
	auto& gcm_cfg = g_fxo->get<gcm_config>();

	if (id > 7)
	{
		return CELL_GCM_ERROR_FAILURE;
	}

	if (!old_api && ctxt->current + 2 >= ctxt->end)
	{
		if (s32 res = ctxt->callback(ppu, ctxt, 8 /* ??? */))
		{
			cellGcmSys.error("cellGcmSetPrepareFlip: callback failed (0x%08x)", res);
			return static_cast<ret_type>(not_an_error(res));
		}
	}

	const u32 cmd_size = rsx::make_command(ctxt->current, GCM_FLIP_COMMAND, { id });

	if (!old_api && ctxt.addr() == gcm_cfg.gcm_info.context_addr)
	{
		vm::_ptr<CellGcmControl>(gcm_cfg.gcm_info.control_addr)->put += cmd_size;
	}

	return static_cast<ret_type>(not_an_error(id));
}

error_code cellGcmSetPrepareFlip(ppu_thread& ppu, vm::ptr<CellGcmContextData> ctxt, u32 id)
{
	cellGcmSys.trace("cellGcmSetPrepareFlip(ctxt=*0x%x, id=0x%x)", ctxt, id);

	return gcmSetPrepareFlip(ppu, ctxt, id);
}

error_code cellGcmSetFlip(ppu_thread& ppu, vm::ptr<CellGcmContextData> ctxt, u32 id)
{
	cellGcmSys.trace("cellGcmSetFlip(ctxt=*0x%x, id=0x%x)", ctxt, id);

	if (auto res = gcmSetPrepareFlip(ppu, ctxt, id); res < 0)
	{
		return CELL_GCM_ERROR_FAILURE;
	}

	return CELL_OK;
}

void cellGcmSetSecondVFrequency(u32 freq)
{
	cellGcmSys.warning("cellGcmSetSecondVFrequency(level=%d)", freq);

	switch (freq)
	{
	case CELL_GCM_DISPLAY_FREQUENCY_59_94HZ:
		break;
	case CELL_GCM_DISPLAY_FREQUENCY_SCANOUT:
		cellGcmSys.todo("Unimplemented display frequency: Scanout");
		break;
	case CELL_GCM_DISPLAY_FREQUENCY_DISABLE:
		cellGcmSys.todo("Unimplemented display frequency: Disabled");
		break;
	default:
		cellGcmSys.error("Improper display frequency specified!");
		break;
	}
}

error_code cellGcmSetTileInfo(u8 index, u8 location, u32 offset, u32 size, u32 pitch, u8 comp, u16 base, u8 bank)
{
	cellGcmSys.warning("cellGcmSetTileInfo(index=%d, location=%d, offset=%d, size=%d, pitch=%d, comp=%d, base=%d, bank=%d)",
		index, location, offset, size, pitch, comp, base, bank);

	auto& gcm_cfg = g_fxo->get<gcm_config>();

	if (index >= rsx::limits::tiles_count || base >= 2048 || bank >= 4)
	{
		return CELL_GCM_ERROR_INVALID_VALUE;
	}

	if (offset & 0xffff || size & 0xffff || pitch & 0xff)
	{
		return CELL_GCM_ERROR_INVALID_ALIGNMENT;
	}

	if (location >= 2 || (comp != 0 && (comp < 7 || comp > 12)))
	{
		return CELL_GCM_ERROR_INVALID_ENUM;
	}

	if (comp)
	{
		cellGcmSys.error("cellGcmSetTileInfo: bad compression mode! (%d)", comp);
	}

	const auto render = rsx::get_current_renderer();

	auto& tile = render->tiles[index];
	tile.location = location;
	tile.offset = offset;
	tile.size = size;
	tile.pitch = pitch;
	tile.comp = comp;
	tile.base = base;
	tile.bank = bank;

	vm::_ptr<CellGcmTileInfo>(gcm_cfg.tiles_addr)[index] = tile.pack();
	return CELL_OK;
}

void cellGcmSetUserHandler(vm::ptr<void(u32)> handler)
{
	cellGcmSys.warning("cellGcmSetUserHandler(handler=*0x%x)", handler);

	if (const auto rsx = rsx::get_current_renderer(); rsx->is_initialized)
	{
		rsx->user_handler = handler;
	}
}

void cellGcmSetUserCommand(ppu_thread& ppu, vm::ptr<CellGcmContextData> ctxt, u32 cause)
{
	cellGcmSys.trace("cellGcmSetUserCommand(ctxt=*0x%x, cause=0x%x)", ctxt, cause);

	if (ctxt->current + 2 >= ctxt->end)
	{
		if (s32 res = ctxt->callback(ppu, ctxt, 8 /* ??? */))
		{
			cellGcmSys.error("cellGcmSetUserCommand(): callback failed (0x%08x)", res);
			return;
		}
	}

	rsx::make_command(ctxt->current, GCM_SET_USER_COMMAND, { cause });
}

void cellGcmSetVBlankHandler(vm::ptr<void(u32)> handler)
{
	cellGcmSys.warning("cellGcmSetVBlankHandler(handler=*0x%x)", handler);

	if (const auto rsx = rsx::get_current_renderer(); rsx->is_initialized)
	{
		rsx->vblank_handler = handler;
	}
}

void cellGcmSetWaitFlip(ppu_thread& ppu, vm::ptr<CellGcmContextData> ctxt)
{
	cellGcmSys.trace("cellGcmSetWaitFlip(ctxt=*0x%x)", ctxt);

	if (ctxt->current + 2 >= ctxt->end)
	{
		if (s32 res = ctxt->callback(ppu, ctxt, 8 /* ??? */))
		{
			cellGcmSys.error("cellGcmSetWaitFlip(): callback failed (0x%08x)", res);
			return;
		}
	}

	rsx::make_command(ctxt->current, NV406E_SEMAPHORE_OFFSET, { 0x10u, 0 });
}

void cellGcmSetWaitFlipUnsafe(vm::ptr<CellGcmContextData> ctxt)
{
	cellGcmSys.trace("cellGcmSetWaitFlipUnsafe(ctxt=*0x%x)", ctxt);

	rsx::make_command(ctxt->current, NV406E_SEMAPHORE_OFFSET, { 0x10u, 0 });
}

void cellGcmSetZcull(u8 index, u32 offset, u32 width, u32 height, u32 cullStart, u32 zFormat, u32 aaFormat, u32 zCullDir, u32 zCullFormat, u32 sFunc, u32 sRef, u32 sMask)
{
	cellGcmSys.warning("cellGcmSetZcull(index=%d, offset=0x%x, width=%d, height=%d, cullStart=0x%x, zFormat=0x%x, aaFormat=0x%x, zCullDir=0x%x, zCullFormat=0x%x, sFunc=0x%x, sRef=0x%x, sMask=0x%x)",
		index, offset, width, height, cullStart, zFormat, aaFormat, zCullDir, zCullFormat, sFunc, sRef, sMask);

	auto& gcm_cfg = g_fxo->get<gcm_config>();

	GcmZcullInfo zcull{};
	zcull.offset = offset;
	zcull.width = width;
	zcull.height = height;
	zcull.cullStart = cullStart;
	zcull.zFormat = zFormat;
	zcull.aaFormat = aaFormat;
	zcull.zcullDir = zCullDir;
	zcull.zcullFormat = zCullFormat;
	zcull.sFunc = sFunc;
	zcull.sRef = sRef;
	zcull.sMask = sMask;
	zcull.bound = true;

	const auto gcm_zcull = zcull.pack();

	// The second difference between BindZcull and this function (second is no return value) is that this function is not thread-safe
	// But take care anyway
	std::lock_guard lock(gcm_cfg.gcmio_mutex);

	if (!sys_rsx_context_attribute(0x5555'5555, 0x301, index, u64{gcm_zcull.region} << 32 | gcm_zcull.size, u64{gcm_zcull.start} << 32 | gcm_zcull.offset, u64{gcm_zcull.status0} << 32 | gcm_zcull.status1))
	{
		vm::_ptr<CellGcmZcullInfo>(gcm_cfg.zculls_addr)[index] = gcm_zcull;
	}
}

error_code cellGcmUnbindTile(u8 index)
{
	cellGcmSys.warning("cellGcmUnbindTile(index=%d)", index);

	if (index >= rsx::limits::tiles_count)
	{
		return CELL_GCM_ERROR_INVALID_VALUE;
	}

	rsx::get_current_renderer()->tiles[index].bound = false;

	return CELL_OK;
}

error_code cellGcmUnbindZcull(u8 index)
{
	cellGcmSys.warning("cellGcmUnbindZcull(index=%d)", index);

	if (index >= 8)
	{
		return CELL_GCM_ERROR_INVALID_VALUE;
	}

	rsx::get_current_renderer()->zculls[index].bound = false;

	return CELL_OK;
}

u32 cellGcmGetTileInfo()
{
	cellGcmSys.warning("cellGcmGetTileInfo()");
	return g_fxo->get<gcm_config>().tiles_addr;
}

u32 cellGcmGetZcullInfo()
{
	cellGcmSys.warning("cellGcmGetZcullInfo()");
	return g_fxo->get<gcm_config>().zculls_addr;
}

u32 cellGcmGetDisplayInfo()
{
	cellGcmSys.warning("cellGcmGetDisplayInfo()");
	return g_fxo->get<gcm_config>().gcm_buffers.addr();
}

error_code cellGcmGetCurrentDisplayBufferId(vm::ptr<u8> id)
{
	cellGcmSys.warning("cellGcmGetCurrentDisplayBufferId(id=*0x%x)", id);

	*id = ::narrow<u8>(rsx::get_current_renderer()->current_display_buffer);

	return CELL_OK;
}

void cellGcmSetInvalidateTile(u8 index)
{
	cellGcmSys.todo("cellGcmSetInvalidateTile(index=%d)", index);
}

error_code cellGcmTerminate()
{
	// The firmware just return CELL_OK as well
	return CELL_OK;
}

error_code cellGcmDumpGraphicsError()
{
	cellGcmSys.todo("cellGcmDumpGraphicsError()");
	return CELL_OK;
}

s32 cellGcmGetDisplayBufferByFlipIndex(u32 qid)
{
	cellGcmSys.todo("cellGcmGetDisplayBufferByFlipIndex(qid=%d)", qid);
	return -1; // Invalid id, todo
}

u64 cellGcmGetLastFlipTime()
{
	cellGcmSys.trace("cellGcmGetLastFlipTime()");

	return rsx::get_current_renderer()->last_guest_flip_timestamp;
}

error_code cellGcmGetLastFlipTime2()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

u64 cellGcmGetLastSecondVTime()
{
	cellGcmSys.todo("cellGcmGetLastSecondVTime()");
	return CELL_OK;
}

u64 cellGcmGetVBlankCount()
{
	cellGcmSys.trace("cellGcmGetVBlankCount()");

	return rsx::get_current_renderer()->vblank_count;
}

error_code cellGcmGetVBlankCount2()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

error_code cellGcmSysGetLastVBlankTime()
{
	cellGcmSys.todo("cellGcmSysGetLastVBlankTime()");
	return CELL_OK;
}

error_code cellGcmInitSystemMode(u64 mode)
{
	cellGcmSys.trace("cellGcmInitSystemMode(mode=0x%x)", mode);

	g_fxo->get<gcm_config>().system_mode = mode;

	return CELL_OK;
}

error_code cellGcmSetFlipImmediate(u8 id)
{
	cellGcmSys.todo("cellGcmSetFlipImmediate(id=0x%x)", id);

	if (id > 7)
	{
		return CELL_GCM_ERROR_FAILURE;
	}

	cellGcmSetFlipMode(id);

	return CELL_OK;
}

error_code cellGcmSetFlipImmediate2()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

void cellGcmSetGraphicsHandler(vm::ptr<void(u32)> handler)
{
	cellGcmSys.todo("cellGcmSetGraphicsHandler(handler=*0x%x)", handler);
}

void cellGcmSetQueueHandler(vm::ptr<void(u32)> handler)
{
	cellGcmSys.warning("cellGcmSetQueueHandler(handler=*0x%x)", handler);

	if (const auto rsx = rsx::get_current_renderer(); rsx->is_initialized)
	{
		rsx->queue_handler = handler;
	}
}

error_code cellGcmSetSecondVHandler(vm::ptr<void(u32)> handler)
{
	cellGcmSys.todo("cellGcmSetSecondVHandler(handler=0x%x)", handler);
	return CELL_OK;
}

void cellGcmSetVBlankFrequency(u32 freq)
{
	cellGcmSys.todo("cellGcmSetVBlankFrequency(freq=%d)", freq);
}

error_code cellGcmSortRemapEaIoAddress()
{
	cellGcmSys.todo("cellGcmSortRemapEaIoAddress()");
	return CELL_OK;
}

//----------------------------------------------------------------------------
// Memory Mapping
//----------------------------------------------------------------------------
error_code cellGcmAddressToOffset(u32 address, vm::ptr<u32> offset)
{
	cellGcmSys.trace("cellGcmAddressToOffset(address=0x%x, offset=*0x%x)", address, offset);

	auto& gcm_cfg = g_fxo->get<gcm_config>();

	u32 result;

	// Test if address is within local memory
	if (const u32 offs = address - gcm_cfg.local_addr; offs < gcm_cfg.local_size)
	{
		result = offs;
	}
	// Address in main memory else check
	else
	{
		const u32 upper12Bits = gcm_cfg.offsetTable.ioAddress[address >> 20];

		// If the address is mapped in IO
		if (upper12Bits << 20 < rsx::get_current_renderer()->main_mem_size)
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
	cellGcmSys.trace("cellGcmGetMaxIoMapSize()");

	return rsx::get_current_renderer()->main_mem_size - g_fxo->get<gcm_config>().reserved_size;
}

void cellGcmGetOffsetTable(vm::ptr<CellGcmOffsetTable> table)
{
	cellGcmSys.trace("cellGcmGetOffsetTable(table=*0x%x)", table);

	auto& gcm_cfg = g_fxo->get<gcm_config>();

	table->ioAddress = gcm_cfg.offsetTable.ioAddress;
	table->eaAddress = gcm_cfg.offsetTable.eaAddress;
}

error_code cellGcmIoOffsetToAddress(u32 ioOffset, vm::ptr<u32> address)
{
	cellGcmSys.trace("cellGcmIoOffsetToAddress(ioOffset=0x%x, address=*0x%x)", ioOffset, address);

	const u32 addr = gcmIoOffsetToAddress(ioOffset);

	if (!addr)
	{
		return CELL_GCM_ERROR_FAILURE;
	}

	*address = addr;

	return CELL_OK;
}

error_code gcmMapEaIoAddress(ppu_thread& ppu, u32 ea, u32 io, u32 size, bool is_strict)
{
	if (!size || (ea & 0xFFFFF) || (io & 0xFFFFF) || (size & 0xFFFFF))
	{
		return CELL_GCM_ERROR_FAILURE;
	}

	if (auto error = sys_rsx_context_iomap(ppu, 0x55555555, io, ea, size, 0xe000000000000800ull | (u64{is_strict} << 60)))
	{
		return error;
	}

	// Assume lock is acquired
	auto& gcm_cfg = g_fxo->get<gcm_config>();
	ea >>= 20, io >>= 20, size >>= 20;

	// Fill the offset table
	for (u32 i = 0; i < size; i++)
	{
		gcm_cfg.offsetTable.ioAddress[ea + i] = io + i;
		gcm_cfg.offsetTable.eaAddress[io + i] = ea + i;
	}

	gcm_cfg.IoMapTable[ea] = size;
	return CELL_OK;
}

error_code cellGcmMapEaIoAddress(ppu_thread& ppu, u32 ea, u32 io, u32 size)
{
	cellGcmSys.warning("cellGcmMapEaIoAddress(ea=0x%x, io=0x%x, size=0x%x)", ea, io, size);

	auto& gcm_cfg = g_fxo->get<gcm_config>();
	std::lock_guard lock(gcm_cfg.gcmio_mutex);

	return gcmMapEaIoAddress(ppu, ea, io, size, false);
}

error_code cellGcmMapEaIoAddressWithFlags(ppu_thread& ppu, u32 ea, u32 io, u32 size, u32 flags)
{
	cellGcmSys.warning("cellGcmMapEaIoAddressWithFlags(ea=0x%x, io=0x%x, size=0x%x, flags=0x%x)", ea, io, size, flags);

	ensure(flags == CELL_GCM_IOMAP_FLAG_STRICT_ORDERING);

	auto& gcm_cfg = g_fxo->get<gcm_config>();
	std::lock_guard lock(gcm_cfg.gcmio_mutex);

	return gcmMapEaIoAddress(ppu, ea, io, size, true);
}

error_code cellGcmMapLocalMemory(vm::ptr<u32> address, vm::ptr<u32> size)
{
	cellGcmSys.warning("cellGcmMapLocalMemory(address=*0x%x, size=*0x%x)", address, size);

	auto& gcm_cfg = g_fxo->get<gcm_config>();
	std::lock_guard lock(gcm_cfg.gcmio_mutex);

	if (!gcm_cfg.local_addr && !gcm_cfg.local_size && vm::falloc(gcm_cfg.local_addr = rsx::constants::local_mem_base, gcm_cfg.local_size = 0xf900000 /* TODO */, vm::video))
	{
		*address = gcm_cfg.local_addr;
		*size = gcm_cfg.local_size;
		return CELL_OK;
	}

	return CELL_GCM_ERROR_FAILURE;
}

error_code cellGcmMapMainMemory(ppu_thread& ppu, u32 ea, u32 size, vm::ptr<u32> offset)
{
	cellGcmSys.warning("cellGcmMapMainMemory(ea=0x%x, size=0x%x, offset=*0x%x)", ea, size, offset);

	if (!size || (ea & 0xFFFFF) || (size & 0xFFFFF)) return CELL_GCM_ERROR_FAILURE;

	auto& gcm_cfg = g_fxo->get<gcm_config>();
	std::lock_guard lock(gcm_cfg.gcmio_mutex);

	// Use the offset table to find the next free io address
	for (u32 io = 0, end = (rsx::get_current_renderer()->main_mem_size - gcm_cfg.reserved_size) >> 20, unmap_count = 1; io < end; unmap_count++)
	{
		if (gcm_cfg.offsetTable.eaAddress[io + unmap_count - 1] > 0xBFF)
		{
			if (unmap_count >= (size >> 20))
			{
				io <<= 20;

				if (auto error = gcmMapEaIoAddress(ppu, ea, io, size, false))
				{
					return error;
				}

				*offset = io;
				return CELL_OK;
			}
		}
		else
		{
			io += unmap_count;
			unmap_count = 0;
		}
	}

	return CELL_GCM_ERROR_NO_IO_PAGE_TABLE;
}

error_code cellGcmReserveIoMapSize(u32 size)
{
	cellGcmSys.trace("cellGcmReserveIoMapSize(size=0x%x)", size);

	if (size & 0xFFFFF)
	{
		return CELL_GCM_ERROR_INVALID_ALIGNMENT;
	}

	auto& gcm_cfg = g_fxo->get<gcm_config>();
	std::lock_guard lock(gcm_cfg.gcmio_mutex);

	if (size > cellGcmGetMaxIoMapSize())
	{
		return CELL_GCM_ERROR_INVALID_VALUE;
	}

	gcm_cfg.reserved_size += size;
	return CELL_OK;
}

error_code GcmUnmapIoAddress(ppu_thread& ppu, gcm_config& gcm_cfg, u32 io)
{
	if (u32 ea = gcm_cfg.offsetTable.eaAddress[io >>= 20], size = gcm_cfg.IoMapTable[ea]; size)
	{
		if (auto error = sys_rsx_context_iounmap(ppu, 0x55555555, io << 20, size << 20))
		{
			return error;
		}

		for (u32 i = 0; i < size; i++)
		{
			gcm_cfg.offsetTable.ioAddress[ea + i] = 0xFFFF;
			gcm_cfg.offsetTable.eaAddress[io + i] = 0xFFFF;
		}

		gcm_cfg.IoMapTable[ea] = 0;
		return CELL_OK;
	}

	return CELL_GCM_ERROR_FAILURE;
}

error_code cellGcmUnmapEaIoAddress(ppu_thread& ppu, u32 ea)
{
	cellGcmSys.warning("cellGcmUnmapEaIoAddress(ea=0x%x)", ea);

	// Ignores lower bits
	ea >>= 20;

	if (ea > 0xBFF)
	{
		return CELL_GCM_ERROR_FAILURE;
	}

	auto& gcm_cfg = g_fxo->get<gcm_config>();
	std::lock_guard lock(gcm_cfg.gcmio_mutex);

	if (const u32 io = gcm_cfg.offsetTable.ioAddress[ea] << 20;
		io < rsx::get_current_renderer()->main_mem_size)
	{
		return GcmUnmapIoAddress(ppu, gcm_cfg, io);
	}

	return CELL_GCM_ERROR_FAILURE;
}

error_code cellGcmUnmapIoAddress(ppu_thread& ppu, u32 io)
{
	cellGcmSys.warning("cellGcmUnmapIoAddress(io=0x%x)", io);

	auto& gcm_cfg = g_fxo->get<gcm_config>();
	std::lock_guard lock(gcm_cfg.gcmio_mutex);

	return GcmUnmapIoAddress(ppu, gcm_cfg, io);
}

error_code cellGcmUnreserveIoMapSize(u32 size)
{
	cellGcmSys.trace("cellGcmUnreserveIoMapSize(size=0x%x)", size);

	if (size & 0xFFFFF)
	{
		return CELL_GCM_ERROR_INVALID_ALIGNMENT;
	}

	auto& gcm_cfg = g_fxo->get<gcm_config>();
	std::lock_guard lock(gcm_cfg.gcmio_mutex);

	if (size > gcm_cfg.reserved_size)
	{
		return CELL_GCM_ERROR_INVALID_VALUE;
	}

	gcm_cfg.reserved_size -= size;
	return CELL_OK;
}

//----------------------------------------------------------------------------
// Cursor Functions
//----------------------------------------------------------------------------

error_code cellGcmInitCursor()
{
	cellGcmSys.todo("cellGcmInitCursor()");
	return CELL_OK;
}

error_code cellGcmSetCursorPosition(s32 x, s32 y)
{
	cellGcmSys.todo("cellGcmSetCursorPosition(x=%d, y=%d)", x, y);
	return CELL_OK;
}

error_code cellGcmSetCursorDisable()
{
	cellGcmSys.todo("cellGcmSetCursorDisable()");
	return CELL_OK;
}

error_code cellGcmUpdateCursor()
{
	cellGcmSys.todo("cellGcmUpdateCursor()");
	return CELL_OK;
}

error_code cellGcmSetCursorEnable()
{
	cellGcmSys.todo("cellGcmSetCursorEnable()");
	return CELL_OK;
}

error_code cellGcmSetCursorImageOffset(u32 offset)
{
	cellGcmSys.todo("cellGcmSetCursorImageOffset(offset=0x%x)", offset);
	return CELL_OK;
}

//------------------------------------------------------------------------
// Functions for Maintaining Compatibility
//------------------------------------------------------------------------

void cellGcmSetDefaultCommandBuffer()
{
	cellGcmSys.warning("cellGcmSetDefaultCommandBuffer()");

	auto& gcm_cfg = g_fxo->get<gcm_config>();

	vm::write32(gcm_cfg.ctxt_addr, gcm_cfg.gcm_info.context_addr);
}

error_code cellGcmSetDefaultCommandBufferAndSegmentWordSize(u32 bufferSize, u32 segmentSize)
{
	cellGcmSys.warning("cellGcmSetDefaultCommandBufferAndSegmentWordSize(bufferSize=0x%x, segmentSize=0x%x)", bufferSize, segmentSize);

	auto& gcm_cfg = g_fxo->get<gcm_config>();

	const auto& put = vm::_ref<CellGcmControl>(gcm_cfg.gcm_info.control_addr).put;
	const auto& get = vm::_ref<CellGcmControl>(gcm_cfg.gcm_info.control_addr).get;

	if (put != 0x1000 || get != 0x1000 || bufferSize < segmentSize * 2 || segmentSize >= 0x80000000)
	{
		return CELL_GCM_ERROR_FAILURE;
	}

	gcm_cfg.gcm_info.command_size = bufferSize;
	gcm_cfg.gcm_info.segment_size = segmentSize;

	return CELL_OK;
}

//------------------------------------------------------------------------
// Other
//------------------------------------------------------------------------

void _cellGcmSetFlipCommand(ppu_thread& ppu, vm::ptr<CellGcmContextData> ctx, u32 id)
{
	cellGcmSys.trace("cellGcmSetFlipCommand(ctx=*0x%x, id=0x%x)", ctx, id);

	if (auto error = gcmSetPrepareFlip<true>(ppu, ctx, id); error < 0)
	{
		// TODO: On actual fw this function doesn't have error checks at all
		cellGcmSys.error("cellGcmSetFlipCommand(): gcmSetPrepareFlip failed with %s", CellGcmError{error + 0u});
	}
}

error_code _cellGcmSetFlipCommand2()
{
	UNIMPLEMENTED_FUNC(cellGcmSys);
	return CELL_OK;
}

void _cellGcmSetFlipCommandWithWaitLabel(ppu_thread& ppu, vm::ptr<CellGcmContextData> ctx, u32 id, u32 label_index, u32 label_value)
{
	cellGcmSys.warning("cellGcmSetFlipCommandWithWaitLabel(ctx=*0x%x, id=0x%x, label_index=0x%x, label_value=0x%x)", ctx, id, label_index, label_value);

	rsx::make_command(ctx->current, NV406E_SEMAPHORE_OFFSET, { label_index * 0x10, label_value });

	if (auto error = gcmSetPrepareFlip<true>(ppu, ctx, id); error < 0)
	{
		// TODO: On actual fw this function doesn't have error checks at all
		cellGcmSys.error("cellGcmSetFlipCommandWithWaitLabel(): gcmSetPrepareFlip failed with %s", CellGcmError{error + 0u});
	}
}

error_code cellGcmSetTile(u8 index, u8 location, u32 offset, u32 size, u32 pitch, u8 comp, u16 base, u8 bank)
{
	cellGcmSys.warning("cellGcmSetTile(index=%d, location=%d, offset=%d, size=%d, pitch=%d, comp=%d, base=%d, bank=%d)",
		index, location, offset, size, pitch, comp, base, bank);

	auto& gcm_cfg = g_fxo->get<gcm_config>();

	// Copied form cellGcmSetTileInfo
	if (index >= rsx::limits::tiles_count || base >= 2048 || bank >= 4)
	{
		return CELL_GCM_ERROR_INVALID_VALUE;
	}

	if (offset & 0xffff || size & 0xffff || pitch & 0xff)
	{
		return CELL_GCM_ERROR_INVALID_ALIGNMENT;
	}

	if (location >= 2 || (comp != 0 && (comp < 7 || comp > 12)))
	{
		return CELL_GCM_ERROR_INVALID_ENUM;
	}

	if (comp)
	{
		cellGcmSys.error("cellGcmSetTile: bad compression mode! (%d)", comp);
	}

	const auto render = rsx::get_current_renderer();

	auto& tile = render->tiles[index];
	tile.location = location;
	tile.offset = offset;
	tile.size = size;
	tile.pitch = pitch;
	tile.comp = comp;
	tile.base = base;
	tile.bank = bank;
	tile.bound = (pitch > 0);

	vm::_ptr<CellGcmTileInfo>(gcm_cfg.tiles_addr)[index] = tile.pack();
	return CELL_OK;
}

error_code _cellGcmFunc2()
{
	cellGcmSys.todo("_cellGcmFunc2()");
	return CELL_OK;
}

error_code _cellGcmFunc3()
{
	cellGcmSys.todo("_cellGcmFunc3()");
	return CELL_OK;
}

error_code _cellGcmFunc4()
{
	cellGcmSys.todo("_cellGcmFunc4()");
	return CELL_OK;
}

error_code _cellGcmFunc13()
{
	cellGcmSys.todo("_cellGcmFunc13()");
	return CELL_OK;
}

error_code _cellGcmFunc38()
{
	cellGcmSys.todo("_cellGcmFunc38()");
	return CELL_OK;
}

error_code cellGcmGpadGetStatus(vm::ptr<u32> status)
{
	cellGcmSys.todo("cellGcmGpadGetStatus(status=*0x%x)", status);
	return CELL_OK;
}

error_code cellGcmGpadNotifyCaptureSurface(vm::ptr<CellGcmSurface> surface)
{
	cellGcmSys.todo("cellGcmGpadNotifyCaptureSurface(surface=*0x%x)", surface);
	return CELL_OK;
}

error_code cellGcmGpadCaptureSnapshot(u32 num)
{
	cellGcmSys.todo("cellGcmGpadCaptureSnapshot(num=%d)", num);
	return CELL_OK;
}


//----------------------------------------------------------------------------


/**
 * Using current to determine what is the next useable command buffer.
 * Caller may wait for RSX not to use the command buffer.
 */
static std::pair<u32, u32> getNextCommandBufferBeginEnd(u32 current)
{
	u32 currentRange = (current - g_defaultCommandBufferBegin) / (32 * 1024);
	if (currentRange >= g_defaultCommandBufferFragmentCount - 1)
		return std::make_pair(g_defaultCommandBufferBegin + 4096, g_defaultCommandBufferBegin + 32 * 1024 - 4);
	return std::make_pair(g_defaultCommandBufferBegin + (currentRange + 1) * 32 * 1024,
		g_defaultCommandBufferBegin + (currentRange + 2) * 32 * 1024 - 4);
}

static u32 getOffsetFromAddress(u32 address)
{
	const u32 upper = g_fxo->get<gcm_config>().offsetTable.ioAddress[address >> 20]; // 12 bits
	ensure(upper != 0xFFFF);
	return (upper << 20) | (address & 0xFFFFF);
}

/**
 * Returns true if getPos is a valid position in command buffer and not between
 * bufferBegin and bufferEnd which are absolute memory address
 */
static bool isInCommandBufferExcept(u32 getPos, u32 bufferBegin, u32 bufferEnd)
{
	// Is outside of default command buffer :
	// It's in a call/return statement
	// Conservatively return false here
	if (getPos < getOffsetFromAddress(g_defaultCommandBufferBegin + 4096) &&
		getPos > getOffsetFromAddress(g_defaultCommandBufferBegin + g_defaultCommandBufferFragmentCount * 32 * 1024))
		return false;
	if (getPos >= getOffsetFromAddress(bufferBegin) &&
		getPos <= getOffsetFromAddress(bufferEnd))
		return false;
	return true;
}

s32 cellGcmCallback(ppu_thread& ppu, vm::ptr<CellGcmContextData> context, u32 count)
{
	cellGcmSys.trace("cellGcmCallback(context=*0x%x, count=0x%x)", context, count);

	auto& gcm_cfg = g_fxo->get<gcm_config>();

	auto& ctrl = *vm::_ptr<CellGcmControl>(gcm_cfg.gcm_info.control_addr);

	// Flush command buffer (ie allow RSX to read up to context->current)
	ctrl.put.exchange(getOffsetFromAddress(context->current.addr()));

	std::pair<u32, u32> newCommandBuffer = getNextCommandBufferBeginEnd(context->current.addr());
	u32 offset = getOffsetFromAddress(newCommandBuffer.first);
	// Write jump instruction
	*context->current = RSX_METHOD_OLD_JUMP_CMD | offset;
	// Update current command buffer
	context->begin.set(newCommandBuffer.first);
	context->current.set(newCommandBuffer.first);
	context->end.set(newCommandBuffer.second);

	// Wait for rsx to "release" the new command buffer
	while (true)
	{
		u32 getPos = ctrl.get.load();
		if (isInCommandBufferExcept(getPos, newCommandBuffer.first, newCommandBuffer.second))
			break;

		if (ppu.test_stopped())
		{
			return 0;
		}

		busy_wait();
	}

	return CELL_OK;
}

//----------------------------------------------------------------------------

DECLARE(ppu_module_manager::cellGcmSys)("cellGcmSys", []()
{
	// Data Retrieval
	REG_FUNC(cellGcmSys, cellGcmGetCurrentField);
	REG_FUNC(cellGcmSys, cellGcmGetLabelAddress);
	REG_FUNC(cellGcmSys, cellGcmGetNotifyDataAddress);
	REG_FUNC(cellGcmSys, _cellGcmFunc12);
	REG_FUNC(cellGcmSys, cellGcmGetReport);
	REG_FUNC(cellGcmSys, cellGcmGetReportDataAddress);
	REG_FUNC(cellGcmSys, cellGcmGetReportDataAddressLocation);
	REG_FUNC(cellGcmSys, cellGcmGetReportDataLocation);
	REG_FUNC(cellGcmSys, cellGcmGetTimeStamp).flag(MFF_FORCED_HLE);     // HLE-ing this allows for optimizations around reports
	REG_FUNC(cellGcmSys, cellGcmGetTimeStampLocation);

	// Command Buffer Control
	REG_FUNC(cellGcmSys, cellGcmGetControlRegister);
	REG_FUNC(cellGcmSys, cellGcmGetDefaultCommandWordSize);
	REG_FUNC(cellGcmSys, cellGcmGetDefaultSegmentWordSize);
	REG_FUNC(cellGcmSys, cellGcmInitDefaultFifoMode);
	REG_FUNC(cellGcmSys, cellGcmSetDefaultFifoSize);

	// Hardware Resource Management
	REG_FUNC(cellGcmSys, cellGcmBindTile);
	REG_FUNC(cellGcmSys, cellGcmBindZcull);
	REG_FUNC(cellGcmSys, cellGcmDumpGraphicsError);
	REG_FUNC(cellGcmSys, cellGcmGetConfiguration);
	REG_FUNC(cellGcmSys, cellGcmGetDisplayBufferByFlipIndex);
	REG_FUNC(cellGcmSys, cellGcmGetFlipStatus);
	REG_FUNC(cellGcmSys, cellGcmGetFlipStatus2);
	REG_FUNC(cellGcmSys, cellGcmGetLastFlipTime);
	REG_FUNC(cellGcmSys, cellGcmGetLastFlipTime2);
	REG_FUNC(cellGcmSys, cellGcmGetLastSecondVTime);
	REG_FUNC(cellGcmSys, cellGcmGetTiledPitchSize);
	REG_FUNC(cellGcmSys, cellGcmGetVBlankCount);
	REG_FUNC(cellGcmSys, cellGcmGetVBlankCount2);
	REG_FUNC(cellGcmSys, cellGcmSysGetLastVBlankTime);
	REG_FUNC(cellGcmSys, _cellGcmFunc1);
	REG_FUNC(cellGcmSys, _cellGcmFunc15);
	REG_FUNC(cellGcmSys, _cellGcmInitBody);
	REG_FUNC(cellGcmSys, cellGcmInitSystemMode);
	REG_FUNC(cellGcmSys, cellGcmResetFlipStatus);
	REG_FUNC(cellGcmSys, cellGcmResetFlipStatus2);
	REG_FUNC(cellGcmSys, cellGcmSetDebugOutputLevel);
	REG_FUNC(cellGcmSys, cellGcmSetDisplayBuffer);
	REG_FUNC(cellGcmSys, cellGcmSetFlip); //
	REG_FUNC(cellGcmSys, cellGcmSetFlipHandler);
	REG_FUNC(cellGcmSys, cellGcmSetFlipHandler2);
	REG_FUNC(cellGcmSys, cellGcmSetFlipImmediate);
	REG_FUNC(cellGcmSys, cellGcmSetFlipImmediate2);
	REG_FUNC(cellGcmSys, cellGcmSetFlipMode);
	REG_FUNC(cellGcmSys, cellGcmSetFlipMode2);
	REG_FUNC(cellGcmSys, cellGcmSetFlipStatus);
	REG_FUNC(cellGcmSys, cellGcmSetFlipStatus2);
	REG_FUNC(cellGcmSys, cellGcmSetGraphicsHandler);
	REG_FUNC(cellGcmSys, cellGcmSetPrepareFlip);
	REG_FUNC(cellGcmSys, cellGcmSetQueueHandler);
	REG_FUNC(cellGcmSys, cellGcmSetSecondVFrequency);
	REG_FUNC(cellGcmSys, cellGcmSetSecondVHandler);
	REG_FUNC(cellGcmSys, cellGcmSetTileInfo);
	REG_FUNC(cellGcmSys, cellGcmSetUserHandler);
	REG_FUNC(cellGcmSys, cellGcmSetUserCommand); //
	REG_FUNC(cellGcmSys, cellGcmSetVBlankFrequency);
	REG_FUNC(cellGcmSys, cellGcmSetVBlankHandler);
	REG_FUNC(cellGcmSys, cellGcmSetWaitFlip); //
	REG_FUNC(cellGcmSys, cellGcmSetWaitFlipUnsafe); //
	REG_FUNC(cellGcmSys, cellGcmSetZcull);
	REG_FUNC(cellGcmSys, cellGcmSortRemapEaIoAddress);
	REG_FUNC(cellGcmSys, cellGcmUnbindTile);
	REG_FUNC(cellGcmSys, cellGcmUnbindZcull);
	REG_FUNC(cellGcmSys, cellGcmGetTileInfo);
	REG_FUNC(cellGcmSys, cellGcmGetZcullInfo);
	REG_FUNC(cellGcmSys, cellGcmGetDisplayInfo);
	REG_FUNC(cellGcmSys, cellGcmGetCurrentDisplayBufferId);
	REG_FUNC(cellGcmSys, cellGcmSetInvalidateTile);
	REG_FUNC(cellGcmSys, cellGcmTerminate);

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
	REG_FUNC(cellGcmSys, cellGcmSetDefaultCommandBufferAndSegmentWordSize);

	// Other
	REG_FUNC(cellGcmSys, _cellGcmSetFlipCommand);
	REG_FUNC(cellGcmSys, _cellGcmSetFlipCommand2);
	REG_FUNC(cellGcmSys, _cellGcmSetFlipCommandWithWaitLabel);
	REG_FUNC(cellGcmSys, cellGcmSetTile);
	REG_FUNC(cellGcmSys, _cellGcmFunc2);
	REG_FUNC(cellGcmSys, _cellGcmFunc3);
	REG_FUNC(cellGcmSys, _cellGcmFunc4);
	REG_FUNC(cellGcmSys, _cellGcmFunc13);
	REG_FUNC(cellGcmSys, _cellGcmFunc38);

	// GPAD
	REG_FUNC(cellGcmSys, cellGcmGpadGetStatus);
	REG_FUNC(cellGcmSys, cellGcmGpadNotifyCaptureSurface);
	REG_FUNC(cellGcmSys, cellGcmGpadCaptureSnapshot);

	// Special
	REG_HIDDEN_FUNC(cellGcmCallback);
});
