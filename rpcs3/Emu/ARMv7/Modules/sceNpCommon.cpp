#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceNpCommon.h"

s32 sceNpAuthInit()
{
	throw __FUNCTION__;
}

s32 sceNpAuthTerm()
{
	throw __FUNCTION__;
}

s32 sceNpAuthCreateStartRequest(vm::psv::ptr<const SceNpAuthRequestParameter> param)
{
	throw __FUNCTION__;
}

s32 sceNpAuthDestroyRequest(SceNpAuthRequestId id)
{
	throw __FUNCTION__;
}

s32 sceNpAuthAbortRequest(SceNpAuthRequestId id)
{
	throw __FUNCTION__;
}

s32 sceNpAuthGetTicket(SceNpAuthRequestId id, vm::psv::ptr<void> buf, u32 len)
{
	throw __FUNCTION__;
}

s32 sceNpAuthGetTicketParam(vm::psv::ptr<const u8> ticket, u32 ticketSize, s32 paramId, vm::psv::ptr<SceNpTicketParam> param)
{
	throw __FUNCTION__;
}

s32 sceNpAuthGetEntitlementIdList(vm::psv::ptr<const u8> ticket, u32 ticketSize, vm::psv::ptr<SceNpEntitlementId> entIdList, u32 entIdListNum)
{
	throw __FUNCTION__;
}

s32 sceNpAuthGetEntitlementById(vm::psv::ptr<const u8> ticket, u32 ticketSize, vm::psv::ptr<const char> entId, vm::psv::ptr<SceNpEntitlement> ent)
{
	throw __FUNCTION__;
}

s32 sceNpCmpNpId(vm::psv::ptr<const SceNpId> npid1, vm::psv::ptr<const SceNpId> npid2)
{
	throw __FUNCTION__;
}

s32 sceNpCmpNpIdInOrder(vm::psv::ptr<const SceNpId> npid1, vm::psv::ptr<const SceNpId> npid2, vm::psv::ptr<s32> order)
{
	throw __FUNCTION__;
}

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceNpCommon, #name, name)

psv_log_base sceNpCommon("SceNpCommon", []()
{
	sceNpCommon.on_load = nullptr;
	sceNpCommon.on_unload = nullptr;
	sceNpCommon.on_stop = nullptr;

	REG_FUNC(0x441D8B4E, sceNpAuthInit);
	REG_FUNC(0x6093B689, sceNpAuthTerm);
	REG_FUNC(0xED42079F, sceNpAuthCreateStartRequest);
	REG_FUNC(0x14FC18AF, sceNpAuthDestroyRequest);
	REG_FUNC(0xE2582575, sceNpAuthAbortRequest);
	REG_FUNC(0x59608D1C, sceNpAuthGetTicket);
	REG_FUNC(0xC1E23E01, sceNpAuthGetTicketParam);
	REG_FUNC(0x3377CD37, sceNpAuthGetEntitlementIdList);
	REG_FUNC(0xF93842F0, sceNpAuthGetEntitlementById);
	REG_FUNC(0xFB8D82E5, sceNpCmpNpId);
	REG_FUNC(0x6BC8150A, sceNpCmpNpIdInOrder);
});
