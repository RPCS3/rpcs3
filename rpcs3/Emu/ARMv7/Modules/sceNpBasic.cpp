#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceNpBasic;

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceNpBasic, #name, name)

psv_log_base sceNpBasic("SceNpBasic", []()
{
	sceNpBasic.on_load = nullptr;
	sceNpBasic.on_unload = nullptr;
	sceNpBasic.on_stop = nullptr;

	//REG_FUNC(0xEFB91A99, sceNpBasicInit);
	//REG_FUNC(0x389BCB3B, sceNpBasicTerm);
	//REG_FUNC(0x26E6E048, sceNpBasicRegisterHandler);
	//REG_FUNC(0x050AE072, sceNpBasicUnregisterHandler);
	//REG_FUNC(0x20146AEC, sceNpBasicCheckCallback);
	//REG_FUNC(0x5183A4B5, sceNpBasicGetFriendOnlineStatus);
	//REG_FUNC(0xEF8A91BC, sceNpBasicGetGamePresenceOfFriend);
	//REG_FUNC(0xDF41F308, sceNpBasicGetFriendListEntryCount);
	//REG_FUNC(0xFF07E787, sceNpBasicGetFriendListEntries);
	//REG_FUNC(0x407E1E6F, sceNpBasicGetBlockListEntryCount);
	//REG_FUNC(0x1211AE8E, sceNpBasicGetBlockListEntries);
	//REG_FUNC(0xF51545D8, sceNpBasicCheckIfPlayerIsBlocked);
	//REG_FUNC(0x51D75562, sceNpBasicSetInGamePresence);
	//REG_FUNC(0xD20C2370, sceNpBasicUnsetInGamePresence);
	//REG_FUNC(0x7A5020A5, sceNpBasicSendInGameDataMessage);
	//REG_FUNC(0x3B0A7F47, sceNpBasicRecordPlaySessionLog);
	//REG_FUNC(0xFB0F7FDF, sceNpBasicGetPlaySessionLogSize);
	//REG_FUNC(0x364531A8, sceNpBasicGetPlaySessionLog);
});
