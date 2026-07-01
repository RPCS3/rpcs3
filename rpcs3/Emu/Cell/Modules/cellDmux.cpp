#include "stdafx.h"
#include "Emu/Cell/lv2/sys_mutex.h"
#include "Emu/Cell/lv2/sys_sync.h"
#include "Emu/Cell/lv2/sys_timer.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/savestate_utils.hpp"
#include "util/asm.hpp"

#include "cellPamf.h"
#include "cellDmux.h"

LOG_CHANNEL(cellDmux);

template <>
void fmt_class_string<CellDmuxError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](CellDmuxError value)
	{
		switch (value)
		{
			STR_CASE(CELL_DMUX_ERROR_ARG);
			STR_CASE(CELL_DMUX_ERROR_SEQ);
			STR_CASE(CELL_DMUX_ERROR_BUSY);
			STR_CASE(CELL_DMUX_ERROR_EMPTY);
			STR_CASE(CELL_DMUX_ERROR_FATAL);
		}

		return unknown;
	});
}

static error_code get_error(u32 internal_error)
{
	switch (internal_error)
	{
	case 0: return CELL_OK;
	case 1: return CELL_DMUX_ERROR_FATAL;
	case 2: // Error values two to five are all converted to CELL_DMUX_ERROR_ARG.
	case 3:
	case 4:
	case 5: return CELL_DMUX_ERROR_ARG;
	default: return CELL_DMUX_ERROR_FATAL;
	}
}

static inline std::span<vm::bptr<DmuxEsContext>> get_es_handles(vm::ptr<DmuxContext> handle)
{
	return { vm::pptr<DmuxEsContext>::make(handle.addr() + sizeof(DmuxContext)).get_ptr(), static_cast<usz>(handle->enabled_es_num) };
}

static inline vm::ptr<DmuxAuQueueElement> get_au_queue_elements(vm::ptr<DmuxEsContext> es_handle)
{
	return vm::ptr<DmuxAuQueueElement>::make(es_handle.addr() + sizeof(DmuxEsContext));
}

static inline vm::cptr<CellDmuxCoreOps> get_core_ops()
{
	return vm::cptr<CellDmuxCoreOps>::make(*ppu_module_manager::cellDmuxPamf.variables.find(0x28b2b7b2)->second.export_addr);
}

template <auto Syscall>
static auto lv2_syscall(ppu_thread& ppu, auto&&... args)
{
	const auto ret = Syscall(ppu, std::forward<decltype(args)>(args)...);

	if (ppu.test_stopped())
	{
		ppu.state += cpu_flag::again;
	}

	return ret;
}

// Callbacks for cellDmuxPamf

static error_code notify_demux_done(ppu_thread& ppu, vm::ptr<void> core_handle, u32 error, vm::ptr<DmuxContext> handle)
{
	// Blocking savestate creation due to ppu_thread::fast_call()
	const std::unique_lock savestate_lock{ g_fxo->get<hle_locks_t>(), std::try_to_lock };

	if (!savestate_lock)
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellDmux.trace("notify_demux_done(core_handle=*0x%x, error=%d, handle=*0x%x)", core_handle, error, handle);

	ensure(!!handle); // Not checked on LLE

	ensure(lv2_syscall<sys_mutex_lock>(ppu, handle->_dx_mhd, 0) == CELL_OK); // Failing this check on LLE would result in it dereferencing an invalid pointer.
	handle->dmux_state = DMUX_STOPPED;
	ensure(lv2_syscall<sys_mutex_unlock>(ppu, handle->_dx_mhd) == CELL_OK); // Failing this check on LLE would result in it dereferencing an invalid pointer.

	if (handle->_this)
	{
		const vm::var<CellDmuxMsg> msg{{ .msgType = CELL_DMUX_MSG_TYPE_DEMUX_DONE, .supplementalInfo = handle->user_data }};
		handle->dmux_cb.cbFunc(ppu, handle, msg, handle->dmux_cb.cbArg);
	}

	return CELL_OK;
}

static error_code notify_fatal_err(ppu_thread& ppu, vm::ptr<void> core_handle, u32 error, vm::ptr<DmuxContext> handle)
{
	// Blocking savestate creation due to ppu_thread::fast_call()
	const std::unique_lock savestate_lock{ g_fxo->get<hle_locks_t>(), std::try_to_lock };

	if (!savestate_lock)
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellDmux.error("notify_fatal_err(core_handle=*0x%x, error=%d, handle=*0x%x)", core_handle, error, handle);

	ensure(!!handle); // Not checked on LLE

	const vm::var<CellDmuxMsg> msg{{ .msgType = CELL_DMUX_MSG_TYPE_FATAL_ERR, .supplementalInfo = static_cast<u32>(get_error(error)) }};
	return handle->dmux_cb.cbFunc(ppu, handle, msg, handle->dmux_cb.cbArg);
}

static error_code notify_prog_end_code(ppu_thread& ppu, vm::ptr<void> core_handle, vm::ptr<DmuxContext> handle)
{
	// Blocking savestate creation due to ppu_thread::fast_call()
	const std::unique_lock savestate_lock{ g_fxo->get<hle_locks_t>(), std::try_to_lock };

	if (!savestate_lock)
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellDmux.notice("notify_prog_end_code(core_handle=*0x%x, handle=*0x%x)", core_handle, handle);

	ensure(!!handle); // Not checked on LLE

	if (handle->_this)
	{
		const vm::var<CellDmuxMsg> msg{{ .msgType = CELL_DMUX_MSG_TYPE_PROG_END_CODE, .supplementalInfo = handle->user_data }};
		handle->dmux_cb.cbFunc(ppu, handle, msg, handle->dmux_cb.cbArg);
	}

	return CELL_OK;
}

static error_code notify_es_au_found(ppu_thread& ppu, vm::ptr<void> core_es_handle, vm::cptr<DmuxAuInfo> au_info, vm::ptr<DmuxEsContext> es_handle)
{
	// Blocking savestate creation due to ppu_thread::fast_call()
	const std::unique_lock savestate_lock{ g_fxo->get<hle_locks_t>(), std::try_to_lock };

	if (!savestate_lock)
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellDmux.trace("notify_es_au_found(core_es_handle=*0x%x, au_info=*0x%x, es_handle=*0x%x)", core_es_handle, au_info, es_handle);

	ensure(!!au_info && !!es_handle); // Not checked on LLE

	const auto fatal_err = [&](be_t<u32> es_is_enabled, error_code ret)
	{
		if (es_is_enabled)
		{
			const vm::var<CellDmuxMsg> demuxerMsg{{ .msgType = CELL_DMUX_MSG_TYPE_FATAL_ERR, .supplementalInfo = static_cast<u64>(ret) }};
			es_handle->dmux_handle->dmux_cb.cbFunc(ppu, es_handle->dmux_handle, demuxerMsg, es_handle->dmux_handle->dmux_cb.cbArg);
		}
	};

	// This is frequently checked in here because the elementary stream could get disabled at any time by a different thread via cellDmuxDisableEs() or cellDmuxClose().
	if (!es_handle->is_enabled)
	{
		return CELL_OK;
	}

	if (const error_code ret = lv2_syscall<sys_mutex_lock>(ppu, es_handle->_dx_mes, 0); ret != CELL_OK)
	{
		fatal_err(es_handle->is_enabled, ret);
		return 1;
	}

	// Check if the access unit queue is full. One slot is reserved for the access unit produced by flushing the stream, so that flushing always succeeds.
	if (!es_handle->is_enabled || es_handle->au_queue.allocated_size >= es_handle->au_queue.max_size - !es_handle->flush_started)
	{
		if (const error_code ret = lv2_syscall<sys_mutex_unlock>(ppu, es_handle->_dx_mes); ret != CELL_OK)
		{
			fatal_err(es_handle->is_enabled, ret);
			return 1;
		}

		return !es_handle->is_enabled ? CELL_OK : not_an_error(1); // Disable error reporting if the queue is full. This is expected to happen frequently.
	}

	DmuxAuInfo& _au_info = get_au_queue_elements(es_handle)[es_handle->au_queue.back].au_info;

	if (const error_code ret = lv2_syscall<sys_mutex_unlock>(ppu, es_handle->_dx_mes); ret != CELL_OK)
	{
		fatal_err(es_handle->is_enabled, ret);
		return 1;
	}

	_au_info.info = au_info->info;
	std::memcpy(_au_info.specific_info.get_ptr(), au_info->specific_info.get_ptr(), au_info->specific_info_size);

	if (!es_handle->is_enabled)
	{
		return CELL_OK;
	}

	if (const error_code ret = lv2_syscall<sys_mutex_lock>(ppu, es_handle->_dx_mes, 0); ret != CELL_OK)
	{
		fatal_err(es_handle->is_enabled, ret);
		return CELL_OK; // LLE returns CELL_OK
	}

	if (!es_handle->is_enabled)
	{
		if (const error_code ret = lv2_syscall<sys_mutex_unlock>(ppu, es_handle->_dx_mes); ret != CELL_OK)
		{
			fatal_err(es_handle->is_enabled, ret);
		}

		return CELL_OK;
	}

	es_handle->au_queue.back = (es_handle->au_queue.back + 1) % es_handle->au_queue.max_size;
	es_handle->au_queue.allocated_size++;
	es_handle->au_queue.size++;

	if (const error_code ret = lv2_syscall<sys_mutex_unlock>(ppu, es_handle->_dx_mes); ret != CELL_OK)
	{
		fatal_err(es_handle->is_enabled, ret);
		return CELL_OK; // LLE returns CELL_OK
	}

	if (!es_handle->is_enabled)
	{
		return CELL_OK;
	}

	const vm::var<CellDmuxEsMsg> es_msg{{ .msgType = CELL_DMUX_ES_MSG_TYPE_AU_FOUND, .supplementalInfo = es_handle->dmux_handle->user_data }};
	es_handle->es_cb.cbFunc(ppu, es_handle->dmux_handle, es_handle, es_msg, es_handle->es_cb.cbArg);

	return CELL_OK;
}

static error_code notify_es_flush_done(ppu_thread& ppu, vm::ptr<void> core_es_handle, vm::ptr<DmuxEsContext> es_handle)
{
	// Blocking savestate creation due to ppu_thread::fast_call()
	const std::unique_lock savestate_lock{ g_fxo->get<hle_locks_t>(), std::try_to_lock };

	if (!savestate_lock)
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellDmux.notice("dmuxEsNotifyFlushDone(unk=*0x%x, es_handle=*0x%x)", core_es_handle, es_handle);

	ensure(!!es_handle); // Not checked on LLE

	if (!es_handle->dmux_handle->_this || !es_handle->is_enabled)
	{
		return CELL_OK;
	}

	es_handle->flush_started = false;

	const vm::var<CellDmuxEsMsg> es_msg{{ .msgType = CELL_DMUX_ES_MSG_TYPE_FLUSH_DONE, .supplementalInfo = es_handle->dmux_handle->user_data }};
	es_handle->es_cb.cbFunc(ppu, es_handle->dmux_handle, es_handle, es_msg, es_handle->es_cb.cbArg);

	return CELL_OK;
}


static error_code query_attr(ppu_thread& ppu, vm::ptr<CellDmuxAttr> demuxerAttr, vm::cptr<void> streamSpecificInfo)
{
	// Blocking savestate creation due to ppu_thread::fast_call()
	const std::unique_lock savestate_lock{ g_fxo->get<hle_locks_t>(), std::try_to_lock };

	if (!savestate_lock)
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	const vm::var<CellDmuxPamfAttr> pamf_attr;

	if (const error_code ret = get_error(get_core_ops()->queryAttr(ppu, streamSpecificInfo, pamf_attr)); ret != CELL_OK)
	{
		return ret;
	}

	demuxerAttr->memSize = utils::align<u32, u32>(sizeof(DmuxContext) + (pamf_attr->maxEnabledEsNum * sizeof(vm::addr_t)) + sizeof(DmuxEsContext), alignof(DmuxContext))
		+ pamf_attr->memSize + 0xf;
	demuxerAttr->demuxerVerUpper = 0x260000;
	demuxerAttr->demuxerVerLower = pamf_attr->version;

	return CELL_OK;
}

error_code cellDmuxQueryAttr(ppu_thread& ppu, vm::cptr<CellDmuxType> demuxerType, vm::ptr<CellDmuxAttr> demuxerAttr)
{
	cellDmux.notice("cellDmuxQueryAttr(demuxerType=*0x%x, demuxerAttr=*0x%x)", demuxerType, demuxerAttr);

	if (!demuxerType || !demuxerAttr || demuxerType->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	return query_attr(ppu, demuxerAttr, vm::null);
}

error_code cellDmuxQueryAttr2(ppu_thread& ppu, vm::cptr<CellDmuxType2> demuxerType2, vm::ptr<CellDmuxAttr> demuxerAttr)
{
	cellDmux.notice("cellDmuxQueryAttr2(demuxerType2=*0x%x, demuxerAttr=*0x%x)", demuxerType2, demuxerAttr);

	if (!demuxerType2 || !demuxerAttr || demuxerType2->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	return query_attr(ppu, demuxerAttr, demuxerType2->streamSpecificInfo);
}

static error_code open(ppu_thread& ppu, vm::cptr<CellDmuxType> demuxerType, vm::cptr<CellDmuxResource> demuxerResource, vm::cptr<CellDmuxResourceEx> demuxerResourceEx,
	vm::cptr<CellDmuxCb> demuxerCb, vm::cptr<void> streamSpecificInfo, vm::pptr<DmuxContext> demuxerHandle)
{
	// Blocking savestate creation due to ppu_thread::fast_call()
	const std::unique_lock savestate_lock{ g_fxo->get<hle_locks_t>(), std::try_to_lock };

	if (!savestate_lock)
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	const vm::var<CellDmuxType2> type{{ .streamType = demuxerType->streamType, .streamSpecificInfo = streamSpecificInfo }};
	const vm::var<CellDmuxAttr> attr;

	if (const error_code ret = cellDmuxQueryAttr2(ppu, type, attr); ret != CELL_OK)
	{
		return ret;
	}

	if (attr->memSize > demuxerResource->memSize)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	const vm::var<CellDmuxPamfAttr> core_attr;

	if (const error_code ret = get_error(get_core_ops()->queryAttr(ppu, streamSpecificInfo, core_attr)); ret != CELL_OK)
	{
		return ret;
	}

	const auto handle = vm::ptr<DmuxContext>::make(utils::align<u32, u32>(demuxerResource->memAddr.addr(), alignof(DmuxContext)));
	const u32 es_handles_size = core_attr->maxEnabledEsNum * sizeof(vm::addr_t);
	const auto core_mem_addr = vm::ptr<void>::make(utils::align<u32>(handle.addr() + sizeof(DmuxContext) + es_handles_size, 0x10));

	const vm::var<CellDmuxResource> core_resource =
	{{
		.memAddr = core_mem_addr,
		.memSize = demuxerResource->memSize - (core_mem_addr.addr() - demuxerResource->memAddr.addr()),
		.ppuThreadPriority = demuxerResource->ppuThreadPriority,
		.ppuThreadStackSize = demuxerResource->ppuThreadStackSize,
		.spuThreadPriority = demuxerResource->spuThreadPriority,
		.numOfSpus = demuxerResource->numOfSpus
	}};

	const vm::var<CellDmuxResourceSpurs> res_spurs;

	if (demuxerResourceEx)
	{
		res_spurs->spurs = demuxerResourceEx->spurs;
		res_spurs->priority = demuxerResourceEx->priority;
		res_spurs->maxContention = demuxerResourceEx->maxContention;
	}

	const auto demux_done_func = vm::bptr<DmuxNotifyDemuxDone>::make(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(notify_demux_done)));
	const auto prog_end_code_func = vm::bptr<DmuxNotifyProgEndCode>::make(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(notify_prog_end_code)));
	const auto fatal_err_func = vm::bptr<DmuxNotifyFatalErr>::make(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(notify_fatal_err)));
	const vm::var<DmuxCb<DmuxNotifyDemuxDone>> cb_demux_done{{ .cbFunc = demux_done_func, .cbArg = handle }};
	const vm::var<DmuxCb<DmuxNotifyProgEndCode>> cb_prog_end_code{{ .cbFunc = prog_end_code_func, .cbArg = handle }};
	const vm::var<DmuxCb<DmuxNotifyFatalErr>> cb_fatal_err{{ .cbFunc = fatal_err_func, .cbArg = handle }};

	const vm::var<vm::bptr<void>> core_handle;

	if (const error_code ret = get_error(get_core_ops()->open(ppu, streamSpecificInfo, core_resource, demuxerResourceEx ? +res_spurs : vm::null,
		cb_demux_done, cb_prog_end_code, cb_fatal_err, core_handle));
		ret != CELL_OK)
	{
		return ret;
	}

	handle->_this = handle;
	handle->_this_size = sizeof(DmuxContext) + es_handles_size;
	handle->version = core_attr->version;
	handle->dmux_state = DMUX_STOPPED;
	handle->dmux_type = *demuxerType;
	handle->dmux_cb = *demuxerCb;
	handle->stream_is_set = false;
	handle->core_handle = *core_handle;
	handle->version_ = core_attr->version;
	handle->user_data = 0;
	handle->max_enabled_es_num = core_attr->maxEnabledEsNum;
	handle->enabled_es_num = 0;

	const vm::var<sys_mutex_attribute_t> mutex_attr =
	{{
		.protocol = SYS_SYNC_PRIORITY,
		.recursive = SYS_SYNC_NOT_RECURSIVE,
		.pshared = SYS_SYNC_NOT_PROCESS_SHARED,
		.adaptive = SYS_SYNC_NOT_ADAPTIVE,
		.name_u64 = "_dx_mhd"_u64
	}};

	if (const error_code ret = lv2_syscall<sys_mutex_create>(ppu, handle.ptr(&DmuxContext::_dx_mhd), mutex_attr); ret != CELL_OK)
	{
		return ret;
	}

	*demuxerHandle = handle;

	return CELL_OK;
}

error_code cellDmuxOpen(ppu_thread& ppu, vm::cptr<CellDmuxType> demuxerType, vm::cptr<CellDmuxResource> demuxerResource, vm::cptr<CellDmuxCb> demuxerCb, vm::pptr<DmuxContext> demuxerHandle)
{
	cellDmux.notice("cellDmuxOpen(demuxerType=*0x%x, demuxerResource=*0x%x, demuxerCb=*0x%x, handle=*0x%x)", demuxerType, demuxerResource, demuxerCb, demuxerHandle);

	if (!demuxerType || demuxerType->streamType != CELL_DMUX_STREAM_TYPE_PAMF
		|| !demuxerResource || !demuxerResource->memAddr || demuxerResource->memSize == umax || demuxerResource->ppuThreadStackSize == umax
		|| !demuxerCb || !demuxerCb->cbFunc
		|| !demuxerHandle)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	return open(ppu, demuxerType, demuxerResource, vm::null, demuxerCb, vm::null, demuxerHandle);
}

error_code cellDmuxOpenEx(ppu_thread& ppu, vm::cptr<CellDmuxType> demuxerType, vm::cptr<CellDmuxResourceEx> demuxerResourceEx, vm::cptr<CellDmuxCb> demuxerCb, vm::pptr<DmuxContext> demuxerHandle)
{
	cellDmux.notice("cellDmuxOpenEx(demuxerType=*0x%x, demuxerResourceEx=*0x%x, demuxerCb=*0x%x, demuxerHandle=*0x%x)", demuxerType, demuxerResourceEx, demuxerCb, demuxerHandle);

	if (!demuxerType || demuxerType->streamType != CELL_DMUX_STREAM_TYPE_PAMF
		|| !demuxerResourceEx || !demuxerResourceEx->memAddr || demuxerResourceEx->memSize == umax || demuxerResourceEx->ppuThreadStackSize == umax
		|| !demuxerResourceEx->spurs || demuxerResourceEx->maxContention == 0u
		|| (demuxerResourceEx->priority & 0xf0f0f0f0f0f0f0f0ull) != 0u // Each byte in priority must be less than 0x10
		|| !demuxerCb
		|| !demuxerHandle)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	const vm::var<CellDmuxResource> resource
	{{
		.memAddr = demuxerResourceEx->memAddr,
		.memSize = demuxerResourceEx->memSize,
		.ppuThreadPriority = demuxerResourceEx->ppuThreadPriority,
		.ppuThreadStackSize = demuxerResourceEx->ppuThreadStackSize,
		.spuThreadPriority = 0xfa,
		.numOfSpus =  1
	}};

	return open(ppu, demuxerType, resource, demuxerResourceEx, demuxerCb, vm::null, demuxerHandle);
}

error_code cellDmuxOpenExt(ppu_thread& ppu, vm::cptr<CellDmuxType> demuxerType, vm::cptr<CellDmuxResourceEx> demuxerResourceEx, vm::cptr<CellDmuxCb> demuxerCb, vm::pptr<DmuxContext> demuxerHandle)
{
	cellDmux.notice("cellDmuxOpenExt(demuxerType=*0x%x, demuxerResourceEx=*0x%x, demuxerCb=*0x%x, demuxerHandle=*0x%x)", demuxerType, demuxerResourceEx, demuxerCb, demuxerHandle);

	return cellDmuxOpenEx(ppu, demuxerType, demuxerResourceEx, demuxerCb, demuxerHandle);
}

error_code cellDmuxOpen2(ppu_thread& ppu, vm::cptr<CellDmuxType2> demuxerType2, vm::cptr<CellDmuxResource2> demuxerResource2, vm::cptr<CellDmuxCb> demuxerCb, vm::pptr<DmuxContext> demuxerHandle)
{
	cellDmux.notice("cellDmuxOpen2(demuxerType2=*0x%x, demuxerResource2=*0x%x, demuxerCb=*0x%x, demuxerHandle=*0x%x)", demuxerType2, demuxerResource2, demuxerCb, demuxerHandle);

	if (!demuxerType2 || demuxerType2->streamType != CELL_DMUX_STREAM_TYPE_PAMF
		|| !demuxerResource2
		|| !demuxerCb || !demuxerCb->cbFunc
		|| !demuxerHandle)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	const vm::var<CellDmuxType> type{{ .streamType = CELL_DMUX_STREAM_TYPE_PAMF }};

	if (demuxerResource2->isResourceEx)
	{
		if (!demuxerResource2->resourceEx.memAddr || demuxerResource2->resourceEx.memSize == umax || demuxerResource2->resourceEx.ppuThreadStackSize == umax
			|| !demuxerResource2->resourceEx.spurs || demuxerResource2->resourceEx.maxContention == 0u
			|| (demuxerResource2->resourceEx.priority & 0xf0f0f0f0f0f0f0f0ull) != 0u) // Each byte in priority must be less than 0x10
		{
			return CELL_DMUX_ERROR_ARG;
		}

		const vm::var<CellDmuxResource> resource
		{{
			.memAddr = demuxerResource2->resourceEx.memAddr,
			.memSize = demuxerResource2->resourceEx.memSize,
			.ppuThreadPriority = demuxerResource2->resourceEx.ppuThreadPriority,
			.ppuThreadStackSize = demuxerResource2->resourceEx.ppuThreadStackSize,
			.spuThreadPriority = 0xfa,
			.numOfSpus = 1
		}};

		return open(ppu, type, resource, demuxerResource2.ptr(&CellDmuxResource2::resourceEx), demuxerCb, demuxerType2->streamSpecificInfo, demuxerHandle);
	}

	if (!demuxerResource2->resource.memAddr || demuxerResource2->resource.memSize == umax || demuxerResource2->resource.ppuThreadStackSize == umax)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	return open(ppu, type, demuxerResource2.ptr(&CellDmuxResource2::resource), vm::null, demuxerCb, demuxerType2->streamSpecificInfo, demuxerHandle);
}

static error_code disable_es(ppu_thread& ppu, DmuxEsContext& esHandle)
{
	if (const error_code ret = lv2_syscall<sys_mutex_lock>(ppu, esHandle._dx_mes, 0); ret != CELL_OK)
	{
		return ret;
	}

	const error_code core_ret = get_core_ops()->disableEs(ppu, esHandle.core_es_handle);

	esHandle.is_enabled = false;

	if (const error_code ret = lv2_syscall<sys_mutex_unlock>(ppu, esHandle._dx_mes); ret != CELL_OK)
	{
		return ret;
	}

	error_code ret;
	while ((ret = lv2_syscall<sys_mutex_destroy>(ppu, esHandle._dx_mes)) == static_cast<s32>(CELL_EBUSY))
	{
		lv2_syscall<sys_timer_usleep>(ppu, 200);
	}

	if (ret != CELL_OK)
	{
		return ret;
	}

	esHandle._this = vm::null;

	return get_error(core_ret);
}

error_code cellDmuxClose(ppu_thread& ppu, vm::ptr<DmuxContext> demuxerHandle)
{
	// Blocking savestate creation due to ppu_thread::fast_call()
	const std::unique_lock savestate_lock{ g_fxo->get<hle_locks_t>(), std::try_to_lock };

	if (!savestate_lock)
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellDmux.notice("cellDmuxClose(demuxerHandle=*0x%x)", demuxerHandle);

	if (!demuxerHandle || !demuxerHandle->_this || demuxerHandle->dmux_type.streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	demuxerHandle->_this = vm::null;

	if (const error_code ret = lv2_syscall<sys_mutex_lock>(ppu, demuxerHandle->_dx_mhd, 0); ret != CELL_OK)
	{
		demuxerHandle->_this = demuxerHandle;
		return ret;
	}

	for (const vm::ptr<DmuxEsContext> es_handle : get_es_handles(demuxerHandle))
	{
		if (const error_code ret = disable_es(ppu, *es_handle); ret != CELL_OK)
		{
			ensure(lv2_syscall<sys_mutex_unlock>(ppu, demuxerHandle->_dx_mhd) == CELL_OK); // Not checked on LLE
			demuxerHandle->_this = demuxerHandle;
			return ret;
		}

		es_handle->dmux_handle = vm::null;
		demuxerHandle->enabled_es_num--;
	}

	error_code ret = lv2_syscall<sys_mutex_unlock>(ppu, demuxerHandle->_dx_mhd);
	ret = ret ? ret : get_error(get_core_ops()->close(ppu, demuxerHandle->core_handle));
	ret = ret ? ret : lv2_syscall<sys_mutex_destroy>(ppu, demuxerHandle->_dx_mhd);

	if (ret != CELL_OK)
	{
		demuxerHandle->_this = demuxerHandle;
	}

	return ret;
}

error_code cellDmuxSetStream(ppu_thread& ppu, vm::ptr<DmuxContext> demuxerHandle, vm::cptr<void> streamAddress, u32 streamSize, b8 discontinuity, u64 userData)
{
	// Blocking savestate creation due to ppu_thread::fast_call()
	const std::unique_lock savestate_lock{ g_fxo->get<hle_locks_t>(), std::try_to_lock };

	if (!savestate_lock)
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellDmux.trace("cellDmuxSetStream(demuxerHandle=*0x%x, streamAddress=*0x%x, streamSize=0x%x, discontinuity=%d, userData=0x%llx)",
		demuxerHandle, streamAddress, streamSize, +discontinuity, userData);

	if (!demuxerHandle || !demuxerHandle->_this || streamSize == 0 || streamSize == umax || demuxerHandle->dmux_type.streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	if (!(demuxerHandle->dmux_state & DMUX_STOPPED))
	{
		return CELL_DMUX_ERROR_BUSY;
	}

	if (const error_code ret = lv2_syscall<sys_mutex_lock>(ppu, demuxerHandle->_dx_mhd, 0); ret != CELL_OK)
	{
		return ret;
	}

	if (const error_code ret = get_error(get_core_ops()->setStream(ppu, demuxerHandle->core_handle, streamAddress, streamSize, discontinuity, userData));
		ret != CELL_OK)
	{
		const error_code mutex_unlock_ret = lv2_syscall<sys_mutex_unlock>(ppu, demuxerHandle->_dx_mhd);
		return mutex_unlock_ret ? mutex_unlock_ret : ret;
	}

	demuxerHandle->stream_is_set = true;
	demuxerHandle->dmux_state = DMUX_RUNNING;
	demuxerHandle->user_data = userData;

	return lv2_syscall<sys_mutex_unlock>(ppu, demuxerHandle->_dx_mhd);
}

error_code cellDmuxResetStream(ppu_thread& ppu, vm::ptr<DmuxContext> demuxerHandle)
{
	// Blocking savestate creation due to ppu_thread::fast_call()
	const std::unique_lock savestate_lock{ g_fxo->get<hle_locks_t>(), std::try_to_lock };

	if (!savestate_lock)
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellDmux.notice("cellDmuxResetStream(demuxerHandle=*0x%x)", demuxerHandle);

	if (!demuxerHandle || !demuxerHandle->_this || demuxerHandle->dmux_type.streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	if (const error_code ret = lv2_syscall<sys_mutex_lock>(ppu, demuxerHandle->_dx_mhd, 0); ret != CELL_OK)
	{
		return ret;
	}

	const u32 dmux_status = demuxerHandle->dmux_state;

	if (const error_code ret = lv2_syscall<sys_mutex_unlock>(ppu, demuxerHandle->_dx_mhd); ret != CELL_OK)
	{
		return ret;
	}

	if (!(dmux_status & DMUX_RUNNING) || !demuxerHandle->stream_is_set)
	{
		return CELL_DMUX_ERROR_SEQ;
	}

	if (const error_code ret = get_error(get_core_ops()->resetStream(ppu, demuxerHandle->core_handle)); ret != CELL_OK)
	{
		return ret;
	}

	demuxerHandle->stream_is_set = false;

	return CELL_OK;
}

error_code cellDmuxResetStreamAndWaitDone(ppu_thread& ppu, vm::ptr<DmuxContext> demuxerHandle)
{
	// Blocking savestate creation due to ppu_thread::fast_call()
	const std::unique_lock savestate_lock{ g_fxo->get<hle_locks_t>(), std::try_to_lock };

	if (!savestate_lock)
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellDmux.notice("cellDmuxResetStreamAndWaitDone(demuxerHandle=*0x%x)", demuxerHandle);

	if (!demuxerHandle || !demuxerHandle->_this || demuxerHandle->dmux_type.streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	if (const error_code ret = get_error(get_core_ops()->resetStreamAndWaitDone(ppu, demuxerHandle->core_handle)); ret != CELL_OK)
	{
		return ret;
	}

	// LLE doesn't set DmuxContext::stream_is_set to false

	return CELL_OK;
}

error_code cellDmuxQueryEsAttr(ppu_thread& ppu, vm::cptr<CellDmuxType> demuxerType, vm::cptr<CellCodecEsFilterId> esFilterId, vm::cptr<void> esSpecificInfo, vm::ptr<CellDmuxEsAttr> esAttr)
{
	// Blocking savestate creation due to ppu_thread::fast_call()
	const std::unique_lock savestate_lock{ g_fxo->get<hle_locks_t>(), std::try_to_lock };

	if (!savestate_lock)
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellDmux.notice("cellDmuxQueryEsAttr(demuxerType=*0x%x, esFilterId=*0x%x, esSpecificInfo=*0x%x, esAttr=*0x%x)", demuxerType, esFilterId, esSpecificInfo, esAttr);

	if (!demuxerType || demuxerType->streamType != CELL_DMUX_STREAM_TYPE_PAMF || !esFilterId || !esAttr)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	const vm::var<CellDmuxPamfEsAttr> core_es_attr;

	if (const error_code ret = get_error(get_core_ops()->queryEsAttr(ppu, vm::make_var<CellCodecEsFilterId>(*esFilterId), esSpecificInfo, core_es_attr));
		ret != CELL_OK)
	{
		return ret;
	}

	esAttr->memSize = utils::align<u32, u32>(sizeof(DmuxEsContext) + ((core_es_attr->auQueueMaxSize + 1) * (core_es_attr->specificInfoSize + sizeof(DmuxAuQueueElement))), alignof(DmuxEsContext))
		+ core_es_attr->memSize + 0xf;

	return CELL_OK;
}

error_code cellDmuxQueryEsAttr2(ppu_thread& ppu, vm::cptr<CellDmuxType2> demuxerType2, vm::cptr<CellCodecEsFilterId> esFilterId, vm::cptr<void> esSpecificInfo, vm::ptr<CellDmuxEsAttr> esAttr)
{
	cellDmux.notice("cellDmuxQueryEsAttr2(demuxerType2=*0x%x, esFilterId=*0x%x, esSpecificInfo=*0x%x, esAttr=*0x%x)", demuxerType2, esFilterId, esSpecificInfo, esAttr);

	ensure(!!demuxerType2); // Not checked on LLE

	const vm::var<CellDmuxType> demuxerType{{ .streamType = demuxerType2->streamType }};

	return cellDmuxQueryEsAttr(ppu, demuxerType, esFilterId, esSpecificInfo, esAttr);
}

error_code cellDmuxEnableEs(ppu_thread& ppu, vm::ptr<DmuxContext> demuxerHandle, vm::cptr<CellCodecEsFilterId> esFilterId, vm::cptr<CellDmuxEsResource> esResourceInfo,
	vm::cptr<CellDmuxEsCb> esCb, vm::cptr<void> esSpecificInfo, vm::pptr<DmuxEsContext> esHandle)
{
	// Blocking savestate creation due to ppu_thread::fast_call()
	const std::unique_lock savestate_lock{ g_fxo->get<hle_locks_t>(), std::try_to_lock };

	if (!savestate_lock)
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellDmux.notice("cellDmuxEnableEs(demuxerHandle=*0x%x, esFilterId=*0x%x, esResourceInfo=*0x%x, esCb=*0x%x, esSpecificInfo=*0x%x, esHandle=**0x%x)",
		demuxerHandle, esFilterId, esResourceInfo, esCb, esSpecificInfo, esHandle);

	if (!demuxerHandle || !demuxerHandle->_this || demuxerHandle->dmux_type.streamType != CELL_DMUX_STREAM_TYPE_PAMF
		|| !esFilterId
		|| !esResourceInfo || !esResourceInfo->memAddr || esResourceInfo->memSize == umax
		|| !esCb || !esCb->cbFunc
		|| !esHandle)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	if (const error_code ret = lv2_syscall<sys_mutex_lock>(ppu, demuxerHandle->_dx_mhd, 0); ret != CELL_OK)
	{
		return ret;
	}

	if (demuxerHandle->enabled_es_num >= demuxerHandle->max_enabled_es_num)
	{
		const error_code mutex_unlock_ret = lv2_syscall<sys_mutex_unlock>(ppu, demuxerHandle->_dx_mhd);
		return mutex_unlock_ret ? mutex_unlock_ret : CELL_DMUX_ERROR_ARG;
	}

	const vm::var<CellDmuxEsAttr> es_attr;

	if (const error_code ret = cellDmuxQueryEsAttr(ppu, demuxerHandle.ptr(&DmuxContext::dmux_type), esFilterId, esSpecificInfo, es_attr); ret != CELL_OK)
	{
		const error_code mutex_unlock_ret = lv2_syscall<sys_mutex_unlock>(ppu, demuxerHandle->_dx_mhd);
		return mutex_unlock_ret ? mutex_unlock_ret : ret;
	}

	if (es_attr->memSize > esResourceInfo->memSize)
	{
		const error_code mutex_unlock_ret = lv2_syscall<sys_mutex_unlock>(ppu, demuxerHandle->_dx_mhd);
		return mutex_unlock_ret ? mutex_unlock_ret : CELL_DMUX_ERROR_ARG;
	}

	const vm::var<CellCodecEsFilterId> es_filter_id{ *esFilterId };
	const vm::var<CellDmuxPamfEsAttr> core_es_attr;

	if (const error_code ret = get_error(get_core_ops()->queryEsAttr(ppu, es_filter_id, esSpecificInfo, core_es_attr)); ret != CELL_OK)
	{
		const error_code mutex_unlock_ret = lv2_syscall<sys_mutex_unlock>(ppu, demuxerHandle->_dx_mhd);
		return mutex_unlock_ret ? mutex_unlock_ret : ret;
	}

	core_es_attr->auQueueMaxSize++; // One extra slot for the access unit produced by flushing the stream, so that flushing always succeeds

	const auto es_handle = vm::ptr<DmuxEsContext>::make(utils::align<u32, u32>(esResourceInfo->memAddr.addr(), alignof(DmuxEsContext)));
	const u32 au_queue_elements_size = core_es_attr->auQueueMaxSize * (core_es_attr->specificInfoSize + sizeof(DmuxAuQueueElement));
	const auto core_mem_addr = vm::bptr<void>::make(utils::align<u32>(es_handle.addr() + sizeof(DmuxEsContext) + au_queue_elements_size, 0x10));

	const vm::var<CellDmuxEsResource> core_es_resource
	{{
		.memAddr = core_mem_addr,
		.memSize = esResourceInfo->memSize - (core_mem_addr.addr() - esResourceInfo->memAddr.addr())
	}};

	const vm::var<sys_mutex_attribute_t> mutex_attr =
	{{
		.protocol = SYS_SYNC_PRIORITY,
		.recursive = SYS_SYNC_NOT_RECURSIVE,
		.pshared = SYS_SYNC_NOT_PROCESS_SHARED,
		.adaptive = SYS_SYNC_NOT_ADAPTIVE,
		.name_u64 = "_dx_mes"_u64
	}};

	if (const error_code ret = lv2_syscall<sys_mutex_create>(ppu, es_handle.ptr(&DmuxEsContext::_dx_mes), mutex_attr); ret != CELL_OK)
	{
		ensure(lv2_syscall<sys_mutex_unlock>(ppu, demuxerHandle->_dx_mhd) == CELL_OK); // Not checked on LLE
		return ret;
	}

	if (const error_code ret = lv2_syscall<sys_mutex_lock>(ppu, es_handle->_dx_mes, 0); ret != CELL_OK)
	{
		ensure(lv2_syscall<sys_mutex_destroy>(ppu, es_handle->_dx_mes) == CELL_OK);    // Not checked on LLE
		ensure(lv2_syscall<sys_mutex_unlock>(ppu, demuxerHandle->_dx_mhd) == CELL_OK); // Not checked on LLE
		return ret;
	}

	const auto au_found_func = vm::bptr<DmuxEsNotifyAuFound>::make(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(notify_es_au_found)));
	const auto flush_done_func = vm::bptr<DmuxEsNotifyFlushDone>::make(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(notify_es_flush_done)));
	const vm::var<DmuxCb<DmuxEsNotifyAuFound>> cb_au_found{{ .cbFunc = au_found_func, .cbArg = es_handle }};
	const vm::var<DmuxCb<DmuxEsNotifyFlushDone>> cb_flush_done{{ .cbFunc = flush_done_func, .cbArg = es_handle }};

	const vm::var<vm::bptr<void>> core_es_handle;

	if (const error_code ret = get_error(get_core_ops()->enableEs(ppu, demuxerHandle->core_handle, es_filter_id, core_es_resource, cb_au_found, cb_flush_done,
		esSpecificInfo, core_es_handle));
		ret != CELL_OK)
	{
		const error_code mutex_unlock_ret = lv2_syscall<sys_mutex_unlock>(ppu, es_handle->_dx_mes);
		const error_code mutex_destroy_ret = lv2_syscall<sys_mutex_destroy>(ppu, es_handle->_dx_mes);

		if (mutex_unlock_ret != CELL_OK)
		{
			ensure(lv2_syscall<sys_mutex_unlock>(ppu, demuxerHandle->_dx_mhd) == CELL_OK); // Not checked on LLE
			return mutex_unlock_ret;
		}

		if (mutex_destroy_ret != CELL_OK)
		{
			ensure(lv2_syscall<sys_mutex_unlock>(ppu, demuxerHandle->_dx_mhd) == CELL_OK); // Not checked on LLE
			return mutex_destroy_ret;
		}

		const error_code mutex_unlock_ret2 = lv2_syscall<sys_mutex_unlock>(ppu, demuxerHandle->_dx_mhd);
		return mutex_unlock_ret2 ? mutex_unlock_ret2 : ret;
	}

	es_handle->is_enabled = true;
	es_handle->error_mem_size = 0;
	es_handle->error_count = 0;
	// es_handle->error_mem_addr is not initialized on LLE
	es_handle->_this = es_handle;
	es_handle->_this_size = sizeof(DmuxEsContext) + au_queue_elements_size;
	es_handle->_this_index = demuxerHandle->enabled_es_num;
	es_handle->dmux_handle = demuxerHandle;
	es_handle->es_cb = *esCb;
	es_handle->core_es_handle = *core_es_handle;
	es_handle->flush_started = bf_t<be_t<s32>, 0, 1>{};
	es_handle->au_queue.max_size = core_es_attr->auQueueMaxSize;
	es_handle->au_queue.allocated_size = 0;
	es_handle->au_queue.size = 0;
	es_handle->au_queue.front = 0;
	es_handle->au_queue.back = 0;
	es_handle->au_queue.allocated_back = 0;

	const vm::ptr<DmuxAuQueueElement> au_queue_elements = get_au_queue_elements(es_handle);

	for (u32 i = 0; i < core_es_attr->auQueueMaxSize; i++)
	{
		au_queue_elements[i].index = i;
		au_queue_elements[i].unk = 0;
		au_queue_elements[i].au_info.info.auAddr = vm::null;
		au_queue_elements[i].au_info.info.auMaxSize = 0;
		au_queue_elements[i].au_info.specific_info.set(au_queue_elements.addr() + (core_es_attr->auQueueMaxSize * static_cast<u32>(sizeof(DmuxAuQueueElement))) + (i * core_es_attr->specificInfoSize));
		au_queue_elements[i].au_info.specific_info_size = core_es_attr->specificInfoSize;
	}

	demuxerHandle->enabled_es_num++;
	*get_es_handles(demuxerHandle).rbegin() = es_handle;
	*esHandle = es_handle;

	if (const error_code ret = lv2_syscall<sys_mutex_unlock>(ppu, es_handle->_dx_mes); ret != CELL_OK)
	{
		ensure(lv2_syscall<sys_mutex_destroy>(ppu, es_handle->_dx_mes) == CELL_OK);    // Not checked on LLE
		ensure(lv2_syscall<sys_mutex_unlock>(ppu, demuxerHandle->_dx_mhd) == CELL_OK); // Not checked on LLE
		return ret;
	}

	return lv2_syscall<sys_mutex_unlock>(ppu, demuxerHandle->_dx_mhd);
}

error_code cellDmuxDisableEs(ppu_thread& ppu, vm::ptr<DmuxEsContext> esHandle)
{
	// Blocking savestate creation due to ppu_thread::fast_call()
	const std::unique_lock savestate_lock{ g_fxo->get<hle_locks_t>(), std::try_to_lock };

	if (!savestate_lock)
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellDmux.notice("cellDmuxDisableEs(esHandle=*0x%x)", esHandle);

	if (!esHandle || !esHandle->_this || !esHandle->dmux_handle || esHandle->dmux_handle->dmux_type.streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	if (const error_code ret = lv2_syscall<sys_mutex_lock>(ppu, esHandle->dmux_handle->_dx_mhd, 0); ret != CELL_OK)
	{
		return ret;
	}

	if (const error_code ret = disable_es(ppu, *esHandle); ret != CELL_OK)
	{
		ensure(lv2_syscall<sys_mutex_unlock>(ppu, esHandle->dmux_handle->_dx_mhd) == CELL_OK); // Not checked on LLE
		return ret;
	}

	const std::span<vm::bptr<DmuxEsContext>> es_handles = get_es_handles(esHandle->dmux_handle);

	std::shift_left(std::ranges::find(es_handles, static_cast<vm::bptr<DmuxEsContext>>(esHandle)), es_handles.end(), 1);

	esHandle->dmux_handle->enabled_es_num--;
	*es_handles.rbegin() = vm::null;

	return lv2_syscall<sys_mutex_unlock>(ppu, esHandle->dmux_handle->_dx_mhd);
}

error_code cellDmuxResetEs(ppu_thread& ppu, vm::ptr<DmuxEsContext> esHandle)
{
	// Blocking savestate creation due to ppu_thread::fast_call()
	const std::unique_lock savestate_lock{ g_fxo->get<hle_locks_t>(), std::try_to_lock };

	if (!savestate_lock)
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellDmux.notice("cellDmuxResetEs(esHandle=*0x%x)", esHandle);

	if (!esHandle || !esHandle->_this || !esHandle->dmux_handle || esHandle->dmux_handle->dmux_type.streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	if (const error_code ret = lv2_syscall<sys_mutex_lock>(ppu, esHandle->dmux_handle->_dx_mhd, 0); ret != CELL_OK)
	{
		return ret;
	}

	const u32 dmux_status = esHandle->dmux_handle->dmux_state;

	if (const error_code ret = lv2_syscall<sys_mutex_unlock>(ppu, esHandle->dmux_handle->_dx_mhd); ret != CELL_OK)
	{
		return ret;
	}

	if (!(dmux_status & DMUX_STOPPED))
	{
		return CELL_DMUX_ERROR_SEQ;
	}

	if (const error_code ret = lv2_syscall<sys_mutex_lock>(ppu, esHandle->_dx_mes, 0); ret != CELL_OK)
	{
		return ret;
	}

	if (const error_code ret = get_error(get_core_ops()->resetEs(ppu, esHandle->core_es_handle)); ret != CELL_OK)
	{
		const error_code mutex_unlock_ret = lv2_syscall<sys_mutex_unlock>(ppu, esHandle->_dx_mes);
		return mutex_unlock_ret ? mutex_unlock_ret : ret;
	}

	const auto au_queue_elements = get_au_queue_elements(esHandle);

	for (s32 i = 0; i < esHandle->au_queue.max_size; i++)
	{
		au_queue_elements[i].index = i;
		au_queue_elements[i].unk = 0;
		au_queue_elements[i].au_info.info.auAddr = vm::null;
		au_queue_elements[i].au_info.info.auMaxSize = 0;
	}

	esHandle->error_mem_size = 0;
	esHandle->error_count = 0;
	esHandle->au_queue.allocated_size = 0;
	esHandle->au_queue.size = 0;
	esHandle->au_queue.front = 0;
	esHandle->au_queue.back = 0;
	esHandle->au_queue.allocated_back = 0;

	return lv2_syscall<sys_mutex_unlock>(ppu, esHandle->_dx_mes);
}

template <bool is_peek>
static error_code pop_au(ppu_thread& ppu, vm::ptr<DmuxEsContext> esHandle, vm::cpptr<CellDmuxAuInfo> auInfo, vm::cpptr<void> auSpecificInfo)
{
	if (!esHandle || !esHandle->_this || !esHandle->dmux_handle || esHandle->dmux_handle->dmux_type.streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	if (const error_code ret = lv2_syscall<sys_mutex_lock>(ppu, esHandle->_dx_mes, 0); ret != CELL_OK)
	{
		return ret;
	}

	if (ppu.state & cpu_flag::again)
	{
		return {};
	}

	if (esHandle->au_queue.size <= 0)
	{
		const error_code mutex_unlock_ret = lv2_syscall<sys_mutex_unlock>(ppu, esHandle->_dx_mes);
		return mutex_unlock_ret ? mutex_unlock_ret : CELL_DMUX_ERROR_EMPTY;
	}

	const vm::ptr<DmuxAuInfo> au_info = (get_au_queue_elements(esHandle) + esHandle->au_queue.front).ptr(&DmuxAuQueueElement::au_info);

	if (auInfo)
	{
		*auInfo = au_info.ptr(&DmuxAuInfo::info);
	}

	if (auSpecificInfo)
	{
		*auSpecificInfo = au_info->specific_info;
	}

	if constexpr (!is_peek)
	{
		esHandle->au_queue.front = (esHandle->au_queue.front + 1) % esHandle->au_queue.max_size;
		esHandle->au_queue.size--;
	}

	return lv2_syscall<sys_mutex_unlock>(ppu, esHandle->_dx_mes);
}

error_code cellDmuxGetAu(ppu_thread& ppu, vm::ptr<DmuxEsContext> esHandle, vm::cpptr<CellDmuxAuInfo> auInfo, vm::cpptr<void> auSpecificInfo)
{
	cellDmux.trace("cellDmuxGetAu(esHandle=*0x%x, auInfo=**0x%x, auSpecificInfo=**0x%x)", esHandle, auInfo, auSpecificInfo);

	return pop_au<false>(ppu, esHandle, auInfo, auSpecificInfo);
}

error_code cellDmuxPeekAu(ppu_thread& ppu, vm::ptr<DmuxEsContext> esHandle, vm::cpptr<CellDmuxAuInfo> auInfo, vm::cpptr<void> auSpecificInfo)
{
	cellDmux.trace("cellDmuxPeekAu(esHandle=*0x%x, auInfo=**0x%x, auSpecificInfo=**0x%x)", esHandle, auInfo, auSpecificInfo);

	return pop_au<true>(ppu, esHandle, auInfo, auSpecificInfo);
}

error_code cellDmuxGetAuEx(ppu_thread& ppu, vm::ptr<DmuxEsContext> esHandle, vm::cpptr<CellDmuxAuInfoEx> auInfoEx, vm::cpptr<void> auSpecificInfo)
{
	cellDmux.trace("cellDmuxGetAuEx(esHandle=*0x%x, auInfoEx=**0x%x, auSpecificInfo=**0x%x)", esHandle, auInfoEx, auSpecificInfo);

	return pop_au<false>(ppu, esHandle, auInfoEx, auSpecificInfo);
}

error_code cellDmuxPeekAuEx(ppu_thread& ppu, vm::ptr<DmuxEsContext> esHandle, vm::cpptr<CellDmuxAuInfoEx> auInfoEx, vm::cpptr<void> auSpecificInfo)
{
	cellDmux.trace("cellDmuxPeekAuEx(esHandle=*0x%x, auInfoEx=**0x%x, auSpecificInfo=**0x%x)", esHandle, auInfoEx, auSpecificInfo);

	return pop_au<true>(ppu, esHandle, auInfoEx, auSpecificInfo);
}

error_code cellDmuxReleaseAu(ppu_thread& ppu, vm::ptr<DmuxEsContext> esHandle)
{
	// Blocking savestate creation due to ppu_thread::fast_call()
	const std::unique_lock savestate_lock{ g_fxo->get<hle_locks_t>(), std::try_to_lock };

	if (!savestate_lock)
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellDmux.trace("cellDmuxReleaseAu(esHandle=*0x%x)", esHandle);

	if (!esHandle || !esHandle->_this || !esHandle->dmux_handle || esHandle->dmux_handle->dmux_type.streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	if (const error_code ret = lv2_syscall<sys_mutex_lock>(ppu, esHandle->_dx_mes, 0); ret != CELL_OK)
	{
		return ret;
	}

	vm::bptr<void> mem_addr;
	u32 mem_size;

	if (esHandle->au_queue.allocated_size < 1)
	{
		if (esHandle->error_count == 0u)
		{
			const error_code mutex_unlock_ret = lv2_syscall<sys_mutex_unlock>(ppu, esHandle->_dx_mes);
			return mutex_unlock_ret ? mutex_unlock_ret : CELL_DMUX_ERROR_SEQ;
		}

		mem_addr = esHandle->error_mem_addr;
		mem_size = esHandle->error_mem_size;
	}
	else
	{
		const DmuxAuInfo& au_info = get_au_queue_elements(esHandle)[esHandle->au_queue.allocated_back].au_info;

		mem_size =
		esHandle->error_mem_size += au_info.info.auSize;

		if (esHandle->error_count == 0u)
		{
			mem_addr = au_info.info.auAddr;
		}
		else
		{
			mem_addr = esHandle->error_mem_addr;
		}

		esHandle->au_queue.allocated_back = (esHandle->au_queue.allocated_back + 1) % esHandle->au_queue.max_size;
		esHandle->au_queue.allocated_size--;

		if (esHandle->au_queue.allocated_size < esHandle->au_queue.size)
		{
			esHandle->au_queue.front = (esHandle->au_queue.front + 1) % esHandle->au_queue.max_size;
			esHandle->au_queue.size--;
		}
	}

	if (const error_code ret = get_error(get_core_ops()->releaseAu(ppu, esHandle->core_es_handle, mem_addr, mem_size)); ret != CELL_OK)
	{
		if (esHandle->error_count == 0u)
		{
			esHandle->error_mem_addr = mem_addr;
		}

		esHandle->error_count++;

		const error_code mutex_unlock_ret = lv2_syscall<sys_mutex_unlock>(ppu, esHandle->_dx_mes);
		return mutex_unlock_ret ? mutex_unlock_ret : ret;
	}

	esHandle->error_count = 0;
	esHandle->error_mem_size = 0;

	return lv2_syscall<sys_mutex_unlock>(ppu, esHandle->_dx_mes);
}

error_code cellDmuxFlushEs(ppu_thread& ppu, vm::ptr<DmuxEsContext> esHandle)
{
	// Blocking savestate creation due to ppu_thread::fast_call()
	const std::unique_lock savestate_lock{ g_fxo->get<hle_locks_t>(), std::try_to_lock };

	if (!savestate_lock)
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellDmux.notice("cellDmuxFlushEs(esHandle=*0x%x)", esHandle);

	if (!esHandle || !esHandle->_this || !esHandle->dmux_handle || esHandle->dmux_handle->dmux_type.streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	if (const error_code ret = lv2_syscall<sys_mutex_lock>(ppu, esHandle->dmux_handle->_dx_mhd, 0); ret != CELL_OK)
	{
		return ret;
	}

	const u32 dmux_state = esHandle->dmux_handle->dmux_state;

	if (const error_code ret = lv2_syscall<sys_mutex_unlock>(ppu, esHandle->dmux_handle->_dx_mhd); ret != CELL_OK)
	{
		return ret;
	}

	if (!(dmux_state & DMUX_STOPPED))
	{
		return CELL_DMUX_ERROR_SEQ;
	}

	esHandle->flush_started = true;

	if (const error_code ret = get_error(get_core_ops()->flushEs(ppu, esHandle->core_es_handle)); ret != CELL_OK)
	{
		esHandle->flush_started = false;
		return ret;
	}

	return CELL_OK;
}

DECLARE(ppu_module_manager::cellDmux)("cellDmux", []()
{
	REG_FUNC(cellDmux, cellDmuxQueryAttr);
	REG_FUNC(cellDmux, cellDmuxQueryAttr2);
	REG_FUNC(cellDmux, cellDmuxOpen);
	REG_FUNC(cellDmux, cellDmuxOpenEx);
	REG_FUNC(cellDmux, cellDmuxOpenExt); // 0xe075fabc
	REG_FUNC(cellDmux, cellDmuxOpen2);
	REG_FUNC(cellDmux, cellDmuxClose);
	REG_FUNC(cellDmux, cellDmuxSetStream);
	REG_FUNC(cellDmux, cellDmuxResetStream);
	REG_FUNC(cellDmux, cellDmuxResetStreamAndWaitDone);
	REG_FUNC(cellDmux, cellDmuxQueryEsAttr);
	REG_FUNC(cellDmux, cellDmuxQueryEsAttr2);
	REG_FUNC(cellDmux, cellDmuxEnableEs);
	REG_FUNC(cellDmux, cellDmuxDisableEs);
	REG_FUNC(cellDmux, cellDmuxResetEs);
	REG_FUNC(cellDmux, cellDmuxGetAu);
	REG_FUNC(cellDmux, cellDmuxPeekAu);
	REG_FUNC(cellDmux, cellDmuxGetAuEx);
	REG_FUNC(cellDmux, cellDmuxPeekAuEx);
	REG_FUNC(cellDmux, cellDmuxReleaseAu);
	REG_FUNC(cellDmux, cellDmuxFlushEs);

	REG_HIDDEN_FUNC(notify_demux_done);
	REG_HIDDEN_FUNC(notify_fatal_err);
	REG_HIDDEN_FUNC(notify_prog_end_code);

	REG_HIDDEN_FUNC(notify_es_au_found);
	REG_HIDDEN_FUNC(notify_es_flush_done);
});
