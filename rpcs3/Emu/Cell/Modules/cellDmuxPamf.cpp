#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/IdManager.h"

#include "cellDmux.h"
#include "cellDmuxPamf.h"


vm::gvar<CellDmuxCoreOps> g_cell_dmux_core_ops_pamf;
vm::gvar<CellDmuxCoreOps> g_cell_dmux_core_ops_raw_es;

LOG_CHANNEL(cellDmuxPamf)

error_code _CellDmuxCoreOpQueryAttr(vm::cptr<CellDmuxPamfSpecificInfo> pamfSpecificInfo, vm::ptr<CellDmuxPamfAttr> pamfAttr)
{
	cellDmuxPamf.todo("_CellDmuxCoreOpQueryAttr(pamfSpecificInfo=*0x%x, pamfAttr=*0x%x)", pamfSpecificInfo, pamfAttr);

	return CELL_OK;
}

error_code _CellDmuxCoreOpOpen(vm::cptr<CellDmuxPamfSpecificInfo> pamfSpecificInfo, vm::cptr<CellDmuxResource> demuxerResource, vm::cptr<CellDmuxResourceSpurs> demuxerResourceSpurs, vm::cptr<DmuxCb<DmuxNotifyDemuxDone>> notifyDemuxDone,
	vm::cptr<DmuxCb<DmuxNotifyProgEndCode>> notifyProgEndCode, vm::cptr<DmuxCb<DmuxNotifyFatalErr>> notifyFatalErr, vm::pptr<void> handle)
{
	cellDmuxPamf.todo("_CellDmuxCoreOpOpen(pamfSpecificInfo=*0x%x, demuxerResource=*0x%x, demuxerResourceSpurs=*0x%x, notifyDemuxDone=*0x%x, notifyProgEndCode=*0x%x, notifyFatalErr=*0x%x, handle=**0x%x)",
		pamfSpecificInfo, demuxerResource, demuxerResourceSpurs, notifyDemuxDone, notifyProgEndCode, notifyFatalErr, handle);

	return CELL_OK;
}

error_code _CellDmuxCoreOpClose(vm::ptr<void> handle)
{
	cellDmuxPamf.todo("_CellDmuxCoreOpClose(handle=*0x%x)", handle);

	return CELL_OK;
}

error_code _CellDmuxCoreOpResetStream(vm::ptr<void> handle)
{
	cellDmuxPamf.todo("_CellDmuxCoreOpResetStream(handle=*0x%x)", handle);

	return CELL_OK;
}

error_code _CellDmuxCoreOpCreateThread(vm::ptr<void> handle)
{
	cellDmuxPamf.todo("_CellDmuxCoreOpCreateThread(handle=*0x%x)", handle);

	return CELL_OK;
}

error_code _CellDmuxCoreOpJoinThread(vm::ptr<void> handle)
{
	cellDmuxPamf.todo("_CellDmuxCoreOpJoinThread(handle=*0x%x)", handle);

	return CELL_OK;
}

template <bool raw_es>
error_code _CellDmuxCoreOpSetStream(vm::ptr<void> handle, vm::cptr<void> streamAddress, u32 streamSize, b8 discontinuity, u64 userData)
{
	cellDmuxPamf.todo("_CellDmuxCoreOpSetStream<raw_es=%d>(handle=*0x%x, streamAddress=*0x%x, streamSize=0x%x, discontinuity=%d, userData=0x%llx)", raw_es, handle, streamAddress, streamSize, +discontinuity, userData);

	return CELL_OK;
}

error_code _CellDmuxCoreOpFreeMemory(vm::ptr<void> esHandle, vm::ptr<void> memAddr, u32 memSize)
{
	cellDmuxPamf.todo("_CellDmuxCoreOpFreeMemory(esHandle=*0x%x, memAddr=*0x%x, memSize=0x%x)", esHandle, memAddr, memSize);

	return CELL_OK;
}

template <bool raw_es>
error_code _CellDmuxCoreOpQueryEsAttr(vm::cptr<void> esFilterId, vm::cptr<void> esSpecificInfo, vm::ptr<CellDmuxPamfEsAttr> attr)
{
	cellDmuxPamf.todo("_CellDmuxCoreOpQueryEsAttr<raw_es=%d>(esFilterId=*0x%x, esSpecificInfo=*0x%x, attr=*0x%x)", raw_es, esFilterId, esSpecificInfo, attr);

	return CELL_OK;
}

template <bool raw_es>
error_code _CellDmuxCoreOpEnableEs(vm::ptr<void> handle, vm::cptr<void> esFilterId, vm::cptr<CellDmuxEsResource> esResource, vm::cptr<DmuxCb<DmuxEsNotifyAuFound>> notifyAuFound,
	vm::cptr<DmuxCb<DmuxEsNotifyFlushDone>> notifyFlushDone, vm::cptr<void> esSpecificInfo, vm::pptr<void> esHandle)
{
	cellDmuxPamf.todo("_CellDmuxCoreOpEnableEs<raw_es=%d>(handle=*0x%x, esFilterId=*0x%x, esResource=*0x%x, notifyAuFound=*0x%x, notifyFlushDone=*0x%x, esSpecificInfo=*0x%x, esHandle)",
		raw_es, handle, esFilterId, esResource, notifyAuFound, notifyFlushDone, esSpecificInfo, esHandle);

	return CELL_OK;
}

error_code _CellDmuxCoreOpDisableEs(vm::ptr<void> esHandle)
{
	cellDmuxPamf.todo("_CellDmuxCoreOpDisableEs(esHandle=*0x%x)", esHandle);

	return CELL_OK;
}

error_code _CellDmuxCoreOpFlushEs(vm::ptr<void> esHandle)
{
	cellDmuxPamf.todo("_CellDmuxCoreOpFlushEs(esHandle=*0x%x)", esHandle);

	return CELL_OK;
}

error_code _CellDmuxCoreOpResetEs(vm::ptr<void> esHandle)
{
	cellDmuxPamf.todo("_CellDmuxCoreOpResetEs(esHandle=*0x%x)", esHandle);

	return CELL_OK;
}

error_code _CellDmuxCoreOpResetStreamAndWaitDone(vm::ptr<void> handle)
{
	cellDmuxPamf.todo("_CellDmuxCoreOpResetStreamAndWaitDone(handle=*0x%x)", handle);

	return CELL_OK;
}

static void init_gvar(const vm::gvar<CellDmuxCoreOps>& var)
{
	var->queryAttr.set(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(_CellDmuxCoreOpQueryAttr)));
	var->open.set(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(_CellDmuxCoreOpOpen)));
	var->close.set(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(_CellDmuxCoreOpClose)));
	var->resetStream.set(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(_CellDmuxCoreOpResetStream)));
	var->createThread.set(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(_CellDmuxCoreOpCreateThread)));
	var->joinThread.set(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(_CellDmuxCoreOpJoinThread)));
	var->freeMemory.set(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(_CellDmuxCoreOpFreeMemory)));
	var->disableEs.set(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(_CellDmuxCoreOpDisableEs)));
	var->flushEs.set(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(_CellDmuxCoreOpFlushEs)));
	var->resetEs.set(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(_CellDmuxCoreOpResetEs)));
	var->resetStreamAndWaitDone.set(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(_CellDmuxCoreOpResetStreamAndWaitDone)));
}

DECLARE(ppu_module_manager::cellDmuxPamf)("cellDmuxPamf", []
{
	REG_VNID(cellDmuxPamf, 0x28b2b7b2, g_cell_dmux_core_ops_pamf).init = []
	{
		g_cell_dmux_core_ops_pamf->setStream.set(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(_CellDmuxCoreOpSetStream<false>)));
		g_cell_dmux_core_ops_pamf->queryEsAttr.set(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(_CellDmuxCoreOpQueryEsAttr<false>)));
		g_cell_dmux_core_ops_pamf->enableEs.set(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(_CellDmuxCoreOpEnableEs<false>)));
		init_gvar(g_cell_dmux_core_ops_pamf);
	};

	REG_VNID(cellDmuxPamf, 0x9728a0e9, g_cell_dmux_core_ops_raw_es).init = []
	{
		g_cell_dmux_core_ops_raw_es->setStream.set(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(_CellDmuxCoreOpSetStream<true>)));
		g_cell_dmux_core_ops_raw_es->queryEsAttr.set(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(_CellDmuxCoreOpQueryEsAttr<true>)));
		g_cell_dmux_core_ops_raw_es->enableEs.set(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(_CellDmuxCoreOpEnableEs<true>)));
		init_gvar(g_cell_dmux_core_ops_raw_es);
	};

	REG_HIDDEN_FUNC(_CellDmuxCoreOpQueryAttr);
	REG_HIDDEN_FUNC(_CellDmuxCoreOpOpen);
	REG_HIDDEN_FUNC(_CellDmuxCoreOpClose);
	REG_HIDDEN_FUNC(_CellDmuxCoreOpResetStream);
	REG_HIDDEN_FUNC(_CellDmuxCoreOpCreateThread);
	REG_HIDDEN_FUNC(_CellDmuxCoreOpJoinThread);
	REG_HIDDEN_FUNC(_CellDmuxCoreOpSetStream<false>);
	REG_HIDDEN_FUNC(_CellDmuxCoreOpSetStream<true>);
	REG_HIDDEN_FUNC(_CellDmuxCoreOpFreeMemory);
	REG_HIDDEN_FUNC(_CellDmuxCoreOpQueryEsAttr<false>);
	REG_HIDDEN_FUNC(_CellDmuxCoreOpQueryEsAttr<true>);
	REG_HIDDEN_FUNC(_CellDmuxCoreOpEnableEs<false>);
	REG_HIDDEN_FUNC(_CellDmuxCoreOpEnableEs<true>);
	REG_HIDDEN_FUNC(_CellDmuxCoreOpDisableEs);
	REG_HIDDEN_FUNC(_CellDmuxCoreOpFlushEs);
	REG_HIDDEN_FUNC(_CellDmuxCoreOpResetEs);
	REG_HIDDEN_FUNC(_CellDmuxCoreOpResetStreamAndWaitDone);
});
