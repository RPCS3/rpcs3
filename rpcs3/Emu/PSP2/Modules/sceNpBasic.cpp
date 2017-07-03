#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceNpBasic.h"

logs::channel sceNpBasic("sceNpBasic");

s32 sceNpBasicInit(vm::ptr<void> opt)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpBasicTerm(ARMv7Thread&)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpBasicRegisterHandler(vm::cptr<SceNpBasicEventHandlers> handlers, vm::cptr<SceNpCommunicationId> context, vm::ptr<void> userdata)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpBasicUnregisterHandler(ARMv7Thread&)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpBasicCheckCallback()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpBasicGetFriendOnlineStatus(vm::cptr<SceNpId> friendId, vm::ptr<s32> status)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpBasicGetGamePresenceOfFriend(vm::cptr<SceNpId> friendId, vm::ptr<SceNpBasicGamePresence> presence)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpBasicGetFriendListEntryCount(vm::ptr<u32> count)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpBasicGetFriendListEntries(u32 startIndex, vm::ptr<SceNpId> entries, u32 numEntries, vm::ptr<u32> retrieved)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpBasicGetBlockListEntryCount(vm::ptr<u32> count)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpBasicGetBlockListEntries(u32 startIndex, vm::ptr<SceNpId> entries, u32 numEntries, vm::ptr<u32> retrieved)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpBasicCheckIfPlayerIsBlocked(vm::cptr<SceNpId> player, vm::ptr<u8> playerIsBlocked)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpBasicSetInGamePresence(vm::cptr<SceNpBasicInGamePresence> presence)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpBasicUnsetInGamePresence()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpBasicSendInGameDataMessage(vm::cptr<SceNpId> to, vm::cptr<SceNpBasicInGameDataMessage> message)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpBasicRecordPlaySessionLog(vm::cptr<SceNpId> withWhom, vm::cptr<SceNpBasicPlaySessionLogDescription> description)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpBasicGetPlaySessionLogSize(SceNpBasicPlaySessionLogType type, vm::ptr<u32> size)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpBasicGetPlaySessionLog(SceNpBasicPlaySessionLogType type, u32 index, vm::ptr<SceNpBasicPlaySessionLog> log)
{
	fmt::throw_exception("Unimplemented" HERE);
}

#define REG_FUNC(nid, name) REG_FNID(SceNpBasic, nid, name)

DECLARE(arm_module_manager::SceNpBasic)("SceNpBasic", []()
{
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
