#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceNpCommon;

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceNpCommon, #name, name)

psv_log_base sceNpCommon("SceNpCommon", []()
{
	sceNpCommon.on_load = nullptr;
	sceNpCommon.on_unload = nullptr;
	sceNpCommon.on_stop = nullptr;

	//REG_FUNC(0x441D8B4E, sceNpAuthInit);
	//REG_FUNC(0x6093B689, sceNpAuthTerm);
	//REG_FUNC(0xED42079F, sceNpAuthCreateStartRequest);
	//REG_FUNC(0x14FC18AF, sceNpAuthDestroyRequest);
	//REG_FUNC(0xE2582575, sceNpAuthAbortRequest);
	//REG_FUNC(0x59608D1C, sceNpAuthGetTicket);
	//REG_FUNC(0xC1E23E01, sceNpAuthGetTicketParam);
	//REG_FUNC(0x3377CD37, sceNpAuthGetEntitlementIdList);
	//REG_FUNC(0xF93842F0, sceNpAuthGetEntitlementById);
	//REG_FUNC(0xFB8D82E5, sceNpCmpNpId);
	//REG_FUNC(0x6BC8150A, sceNpCmpNpIdInOrder);
});
