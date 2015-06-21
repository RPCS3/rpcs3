#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceNpBasic.h"

s32 sceNpBasicInit(vm::ptr<void> opt)
{
	throw __FUNCTION__;
}

s32 sceNpBasicTerm(ARMv7Context&)
{
	throw __FUNCTION__;
}

s32 sceNpBasicRegisterHandler(vm::cptr<SceNpBasicEventHandlers> handlers, vm::cptr<SceNpCommunicationId> context, vm::ptr<void> userdata)
{
	throw __FUNCTION__;
}

s32 sceNpBasicUnregisterHandler(ARMv7Context&)
{
	throw __FUNCTION__;
}

s32 sceNpBasicCheckCallback()
{
	throw __FUNCTION__;
}

s32 sceNpBasicGetFriendOnlineStatus(vm::cptr<SceNpId> friendId, vm::ptr<SceNpBasicFriendOnlineStatus> status)
{
	throw __FUNCTION__;
}

s32 sceNpBasicGetGamePresenceOfFriend(vm::cptr<SceNpId> friendId, vm::ptr<SceNpBasicGamePresence> presence)
{
	throw __FUNCTION__;
}

s32 sceNpBasicGetFriendListEntryCount(vm::ptr<u32> count)
{
	throw __FUNCTION__;
}

s32 sceNpBasicGetFriendListEntries(u32 startIndex, vm::ptr<SceNpId> entries, u32 numEntries, vm::ptr<u32> retrieved)
{
	throw __FUNCTION__;
}

s32 sceNpBasicGetBlockListEntryCount(vm::ptr<u32> count)
{
	throw __FUNCTION__;
}

s32 sceNpBasicGetBlockListEntries(u32 startIndex, vm::ptr<SceNpId> entries, u32 numEntries, vm::ptr<u32> retrieved)
{
	throw __FUNCTION__;
}

s32 sceNpBasicCheckIfPlayerIsBlocked(vm::cptr<SceNpId> player, vm::ptr<u8> playerIsBlocked)
{
	throw __FUNCTION__;
}

s32 sceNpBasicSetInGamePresence(vm::cptr<SceNpBasicInGamePresence> presence)
{
	throw __FUNCTION__;
}

s32 sceNpBasicUnsetInGamePresence()
{
	throw __FUNCTION__;
}

s32 sceNpBasicSendInGameDataMessage(vm::cptr<SceNpId> to, vm::cptr<SceNpBasicInGameDataMessage> message)
{
	throw __FUNCTION__;
}

s32 sceNpBasicRecordPlaySessionLog(vm::cptr<SceNpId> withWhom, vm::cptr<SceNpBasicPlaySessionLogDescription> description)
{
	throw __FUNCTION__;
}

s32 sceNpBasicGetPlaySessionLogSize(SceNpBasicPlaySessionLogType type, vm::ptr<u32> size)
{
	throw __FUNCTION__;
}

s32 sceNpBasicGetPlaySessionLog(SceNpBasicPlaySessionLogType type, u32 index, vm::ptr<SceNpBasicPlaySessionLog> log)
{
	throw __FUNCTION__;
}

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceNpBasic, #name, name)

psv_log_base sceNpBasic("SceNpBasic", []()
{
	sceNpBasic.on_load = nullptr;
	sceNpBasic.on_unload = nullptr;
	sceNpBasic.on_stop = nullptr;
	sceNpBasic.on_error = nullptr;

	REG_FUNC(0xEFB91A99, sceNpBasicInit);
	REG_FUNC(0x389BCB3B, sceNpBasicTerm);
	REG_FUNC(0x26E6E048, sceNpBasicRegisterHandler);
	REG_FUNC(0x050AE072, sceNpBasicUnregisterHandler);
	REG_FUNC(0x20146AEC, sceNpBasicCheckCallback);
	REG_FUNC(0x5183A4B5, sceNpBasicGetFriendOnlineStatus);
	REG_FUNC(0xEF8A91BC, sceNpBasicGetGamePresenceOfFriend);
	REG_FUNC(0xDF41F308, sceNpBasicGetFriendListEntryCount);
	REG_FUNC(0xFF07E787, sceNpBasicGetFriendListEntries);
	REG_FUNC(0x407E1E6F, sceNpBasicGetBlockListEntryCount);
	REG_FUNC(0x1211AE8E, sceNpBasicGetBlockListEntries);
	REG_FUNC(0xF51545D8, sceNpBasicCheckIfPlayerIsBlocked);
	REG_FUNC(0x51D75562, sceNpBasicSetInGamePresence);
	REG_FUNC(0xD20C2370, sceNpBasicUnsetInGamePresence);
	REG_FUNC(0x7A5020A5, sceNpBasicSendInGameDataMessage);
	REG_FUNC(0x3B0A7F47, sceNpBasicRecordPlaySessionLog);
	REG_FUNC(0xFB0F7FDF, sceNpBasicGetPlaySessionLogSize);
	REG_FUNC(0x364531A8, sceNpBasicGetPlaySessionLog);
});
