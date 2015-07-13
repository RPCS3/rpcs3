#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceNpCommon.h"

extern psv_log_base sceNpBasic;

enum SceNpBasicFriendListEventType : s32
{
	SCE_NP_BASIC_FRIEND_LIST_EVENT_TYPE_SYNC = 1,
	SCE_NP_BASIC_FRIEND_LIST_EVENT_TYPE_SYNC_DONE = 2,
	SCE_NP_BASIC_FRIEND_LIST_EVENT_TYPE_ADDED = 3,
	SCE_NP_BASIC_FRIEND_LIST_EVENT_TYPE_DELETED = 4
};

typedef vm::psv::ptr<void(SceNpBasicFriendListEventType eventType, vm::psv::ptr<const SceNpId> friendId, vm::psv::ptr<void> userdata)> SceNpBasicFriendListEventHandler;

enum SceNpBasicFriendOnlineStatusEventType : s32
{
	SCE_NP_BASIC_FRIEND_ONLINE_STATUS_EVENT_TYPE_SYNC = 1,
	SCE_NP_BASIC_FRIEND_ONLINE_STATUS_EVENT_TYPE_SYNC_DONE = 2,
	SCE_NP_BASIC_FRIEND_ONLINE_STATUS_EVENT_TYPE_UPDATED = 3
};

enum SceNpBasicFriendOnlineStatus : s32
{
	SCE_NP_BASIC_FRIEND_ONLINE_STATUS_UNKNOWN = 0,
	SCE_NP_BASIC_FRIEND_ONLINE_STATUS_OFFLINE = 1,
	SCE_NP_BASIC_FRIEND_ONLINE_STATUS_STANDBY = 2,
	SCE_NP_BASIC_FRIEND_ONLINE_STATUS_ONLINE_OUT_OF_CONTEXT = 3,
	SCE_NP_BASIC_FRIEND_ONLINE_STATUS_ONLINE_IN_CONTEXT = 4
};

typedef vm::psv::ptr<void(SceNpBasicFriendOnlineStatusEventType eventType, vm::psv::ptr<const SceNpId> friendId, SceNpBasicFriendOnlineStatus status, vm::psv::ptr<void> userdata)> SceNpBasicFriendOnlineStatusEventHandler;

enum SceNpBasicBlockListEventType : s32
{
	SCE_NP_BASIC_BLOCK_LIST_EVENT_TYPE_SYNC = 1,
	SCE_NP_BASIC_BLOCK_LIST_EVENT_TYPE_SYNC_DONE = 2,
	SCE_NP_BASIC_BLOCK_LIST_EVENT_TYPE_ADDED = 3,
	SCE_NP_BASIC_BLOCK_LIST_EVENT_TYPE_DELETED = 4
};

typedef vm::psv::ptr<void(SceNpBasicBlockListEventType eventType, vm::psv::ptr<const SceNpId> playerId, vm::psv::ptr<void> userdata)> SceNpBasicBlockListEventHandler;

enum SceNpBasicFriendGamePresenceEventType : s32
{
	SCE_NP_BASIC_FRIEND_GAME_PRESENCE_EVENT_TYPE_SYNC = 1,
	SCE_NP_BASIC_FRIEND_GAME_PRESENCE_EVENT_TYPE_SYNC_DONE = 2,
	SCE_NP_BASIC_FRIEND_GAME_PRESENCE_EVENT_TYPE_UPDATED = 3
};

enum SceNpBasicInGamePresenceType
{
	SCE_NP_BASIC_IN_GAME_PRESENCE_TYPE_UNKNOWN = -1,
	SCE_NP_BASIC_IN_GAME_PRESENCE_TYPE_NONE = 0,
	SCE_NP_BASIC_IN_GAME_PRESENCE_TYPE_DEFAULT = 1,
	SCE_NP_BASIC_IN_GAME_PRESENCE_TYPE_JOINABLE = 2,
	SCE_NP_BASIC_IN_GAME_PRESENCE_TYPE_MAX = 3
};

struct SceNpBasicInGamePresence
{
	u32 sdkVersion;
	SceNpBasicInGamePresenceType type;
	char status[192];
	u8 data[128];
	u32 dataSize;
};

struct SceNpBasicGamePresence
{
	u32 size;
	char title[128];
	SceNpBasicInGamePresence inGamePresence;
};

typedef vm::psv::ptr<void(SceNpBasicFriendGamePresenceEventType eventtype, vm::psv::ptr<const SceNpId> friendId, vm::psv::ptr<const SceNpBasicGamePresence> presence, vm::psv::ptr<void> userdata)> SceNpBasicFriendGamePresenceEventHandler;

struct SceNpBasicInGameDataMessage
{
	u8 data[128];
	u32 dataSize;
};

typedef vm::psv::ptr<void(vm::psv::ptr<const SceNpId> from, vm::psv::ptr<const SceNpBasicInGameDataMessage> message, vm::psv::ptr<void> userdata)> SceNpBasicInGameDataMessageEventHandler;

struct SceNpBasicEventHandlers
{
	u32 sdkVersion;
	SceNpBasicFriendListEventHandler friendListEventHandler;
	SceNpBasicFriendOnlineStatusEventHandler friendOnlineStatusEventHandler;
	SceNpBasicBlockListEventHandler blockListEventHandler;
	SceNpBasicFriendGamePresenceEventHandler friendGamePresenceEventHandler;
	SceNpBasicInGameDataMessageEventHandler inGameDataMessageEventHandler;
};

struct SceNpBasicPlaySessionLogDescription
{
	char text[512];
};

struct SceNpBasicPlaySessionLog
{
	u64 date;
	SceNpId withWhom;
	SceNpCommunicationId commId;
	char title[128];
	SceNpBasicPlaySessionLogDescription description;
};

enum SceNpBasicPlaySessionLogType : s32
{
	SCE_NP_BASIC_PLAY_SESSION_LOG_TYPE_INVALID = -1,
	SCE_NP_BASIC_PLAY_SESSION_LOG_TYPE_ALL = 0,
	SCE_NP_BASIC_PLAY_SESSION_LOG_TYPE_BY_NP_COMM_ID = 1,
	SCE_NP_BASIC_PLAY_SESSION_LOG_TYPE_MAX = 2
};

s32 sceNpBasicInit(vm::psv::ptr<void> opt)
{
	throw __FUNCTION__;
}

s32 sceNpBasicTerm(ARMv7Context&)
{
	throw __FUNCTION__;
}

s32 sceNpBasicRegisterHandler(vm::psv::ptr<const SceNpBasicEventHandlers> handlers, vm::psv::ptr<const SceNpCommunicationId> context, vm::psv::ptr<void> userdata)
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

s32 sceNpBasicGetFriendOnlineStatus(vm::psv::ptr<const SceNpId> friendId, vm::psv::ptr<SceNpBasicFriendOnlineStatus> status)
{
	throw __FUNCTION__;
}

s32 sceNpBasicGetGamePresenceOfFriend(vm::psv::ptr<const SceNpId> friendId, vm::psv::ptr<SceNpBasicGamePresence> presence)
{
	throw __FUNCTION__;
}

s32 sceNpBasicGetFriendListEntryCount(vm::psv::ptr<u32> count)
{
	throw __FUNCTION__;
}

s32 sceNpBasicGetFriendListEntries(u32 startIndex, vm::psv::ptr<SceNpId> entries, u32 numEntries, vm::psv::ptr<u32> retrieved)
{
	throw __FUNCTION__;
}

s32 sceNpBasicGetBlockListEntryCount(vm::psv::ptr<u32> count)
{
	throw __FUNCTION__;
}

s32 sceNpBasicGetBlockListEntries(u32 startIndex, vm::psv::ptr<SceNpId> entries, u32 numEntries, vm::psv::ptr<u32> retrieved)
{
	throw __FUNCTION__;
}

s32 sceNpBasicCheckIfPlayerIsBlocked(vm::psv::ptr<const SceNpId> player, vm::psv::ptr<u8> playerIsBlocked)
{
	throw __FUNCTION__;
}

s32 sceNpBasicSetInGamePresence(vm::psv::ptr<const SceNpBasicInGamePresence> presence)
{
	throw __FUNCTION__;
}

s32 sceNpBasicUnsetInGamePresence()
{
	throw __FUNCTION__;
}

s32 sceNpBasicSendInGameDataMessage(vm::psv::ptr<const SceNpId> to, vm::psv::ptr<const SceNpBasicInGameDataMessage> message)
{
	throw __FUNCTION__;
}

s32 sceNpBasicRecordPlaySessionLog(vm::psv::ptr<const SceNpId> withWhom, vm::psv::ptr<const SceNpBasicPlaySessionLogDescription> description)
{
	throw __FUNCTION__;
}

s32 sceNpBasicGetPlaySessionLogSize(SceNpBasicPlaySessionLogType type, vm::psv::ptr<u32> size)
{
	throw __FUNCTION__;
}

s32 sceNpBasicGetPlaySessionLog(SceNpBasicPlaySessionLogType type, u32 index, vm::psv::ptr<SceNpBasicPlaySessionLog> log)
{
	throw __FUNCTION__;
}

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceNpBasic, #name, name)

psv_log_base sceNpBasic("SceNpBasic", []()
{
	sceNpBasic.on_load = nullptr;
	sceNpBasic.on_unload = nullptr;
	sceNpBasic.on_stop = nullptr;

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
