#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceNetCtl;

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceNetCtl, #name, name)

psv_log_base sceNetCtl("SceNetCtl", []()
{
	sceNetCtl.on_load = nullptr;
	sceNetCtl.on_unload = nullptr;
	sceNetCtl.on_stop = nullptr;

	//REG_FUNC(0x495CA1DB, sceNetCtlInit);
	//REG_FUNC(0xCD188648, sceNetCtlTerm);
	//REG_FUNC(0xDFFC3ED4, sceNetCtlCheckCallback);
	//REG_FUNC(0x6B20EC02, sceNetCtlInetGetResult);
	//REG_FUNC(0x7AE0ED19, sceNetCtlAdhocGetResult);
	//REG_FUNC(0xB26D07F3, sceNetCtlInetGetInfo);
	//REG_FUNC(0x6D26AC68, sceNetCtlInetGetState);
	//REG_FUNC(0xEAEE6185, sceNetCtlInetRegisterCallback);
	//REG_FUNC(0xD0C3BF3F, sceNetCtlInetUnregisterCallback);
	//REG_FUNC(0x4DDD6149, sceNetCtlGetNatInfo);
	//REG_FUNC(0x0961A561, sceNetCtlAdhocGetState);
	//REG_FUNC(0xFFA9D594, sceNetCtlAdhocRegisterCallback);
	//REG_FUNC(0xA4471E10, sceNetCtlAdhocUnregisterCallback);
	//REG_FUNC(0xED43B79A, sceNetCtlAdhocDisconnect);
	//REG_FUNC(0x77586C59, sceNetCtlAdhocGetPeerList);
	//REG_FUNC(0x7118C99D, sceNetCtlAdhocGetInAddr);
});
