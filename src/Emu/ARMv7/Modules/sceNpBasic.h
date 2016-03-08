#pragma once

#include "sceNpCommon.h"

enum SceNpBasicFriendListEventType : s32
{
	SCE_NP_BASIC_FRIEND_LIST_EVENT_TYPE_SYNC = 1,
	SCE_NP_BASIC_FRIEND_LIST_EVENT_TYPE_SYNC_DONE = 2,
	SCE_NP_BASIC_FRIEND_LIST_EVENT_TYPE_ADDED = 3,
	SCE_NP_BASIC_FRIEND_LIST_EVENT_TYPE_DELETED = 4
};

using SceNpBasicFriendListEventHandler = void(SceNpBasicFriendListEventType eventType, vm::cptr<SceNpId> friendId, vm::ptr<void> userdata);

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

using SceNpBasicFriendOnlineStatusEventHandler = void(SceNpBasicFriendOnlineStatusEventType eventType, vm::cptr<SceNpId> friendId, SceNpBasicFriendOnlineStatus status, vm::ptr<void> userdata);

enum SceNpBasicBlockListEventType : s32
{
	SCE_NP_BASIC_BLOCK_LIST_EVENT_TYPE_SYNC = 1,
	SCE_NP_BASIC_BLOCK_LIST_EVENT_TYPE_SYNC_DONE = 2,
	SCE_NP_BASIC_BLOCK_LIST_EVENT_TYPE_ADDED = 3,
	SCE_NP_BASIC_BLOCK_LIST_EVENT_TYPE_DELETED = 4
};

using SceNpBasicBlockListEventHandler = void(SceNpBasicBlockListEventType eventType, vm::cptr<SceNpId> playerId, vm::ptr<void> userdata);

enum SceNpBasicFriendGamePresenceEventType : s32
{
	SCE_NP_BASIC_FRIEND_GAME_PRESENCE_EVENT_TYPE_SYNC = 1,
	SCE_NP_BASIC_FRIEND_GAME_PRESENCE_EVENT_TYPE_SYNC_DONE = 2,
	SCE_NP_BASIC_FRIEND_GAME_PRESENCE_EVENT_TYPE_UPDATED = 3
};

enum SceNpBasicInGamePresenceType : s32
{
	SCE_NP_BASIC_IN_GAME_PRESENCE_TYPE_UNKNOWN = -1,
	SCE_NP_BASIC_IN_GAME_PRESENCE_TYPE_NONE = 0,
	SCE_NP_BASIC_IN_GAME_PRESENCE_TYPE_DEFAULT = 1,
	SCE_NP_BASIC_IN_GAME_PRESENCE_TYPE_JOINABLE = 2,
	SCE_NP_BASIC_IN_GAME_PRESENCE_TYPE_MAX = 3
};

struct SceNpBasicInGamePresence
{
	le_t<u32> sdkVersion;
	le_t<s32> type; // SceNpBasicInGamePresenceType
	char status[192];
	u8 data[128];
	le_t<u32> dataSize;
};

struct SceNpBasicGamePresence
{
	le_t<u32> size;
	char title[128];
	SceNpBasicInGamePresence inGamePresence;
};

using SceNpBasicFriendGamePresenceEventHandler = void(SceNpBasicFriendGamePresenceEventType eventtype, vm::cptr<SceNpId> friendId, vm::cptr<SceNpBasicGamePresence> presence, vm::ptr<void> userdata);

struct SceNpBasicInGameDataMessage
{
	u8 data[128];
	le_t<u32> dataSize;
};

using SceNpBasicInGameDataMessageEventHandler = void(vm::cptr<SceNpId> from, vm::cptr<SceNpBasicInGameDataMessage> message, vm::ptr<void> userdata);

struct SceNpBasicEventHandlers
{
	le_t<u32> sdkVersion;
	vm::lptr<SceNpBasicFriendListEventHandler> friendListEventHandler;
	vm::lptr<SceNpBasicFriendOnlineStatusEventHandler> friendOnlineStatusEventHandler;
	vm::lptr<SceNpBasicBlockListEventHandler> blockListEventHandler;
	vm::lptr<SceNpBasicFriendGamePresenceEventHandler> friendGamePresenceEventHandler;
	vm::lptr<SceNpBasicInGameDataMessageEventHandler> inGameDataMessageEventHandler;
};

struct SceNpBasicPlaySessionLogDescription
{
	char text[512];
};

struct SceNpBasicPlaySessionLog
{
	le_t<u64> date;
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

extern psv_log_base sceNpBasic;
