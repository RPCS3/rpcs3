#pragma once

#include "sceNet.h"
#include "sceNpCommon.h"

struct SceNpMatching2MemoryInfo
{
	le_t<u32> totalMemSize;
	le_t<u32> curMemUsage;
	le_t<u32> maxMemUsage;
	u8 reserved[12];
};

struct SceNpMatching2SessionPassword
{
	u8 data[8];
};

struct SceNpMatching2PresenceOptionData
{
	u8 data[16];
	le_t<u32> len;
};

struct SceNpMatching2IntAttr
{
	le_t<u16> id;
	u8 padding[2];
	le_t<u32> num;
};


struct SceNpMatching2BinAttr
{
	le_t<u16> id;
	u8 padding[2];
	vm::lptr<void> ptr;
	le_t<u32> size;
};


struct SceNpMatching2RangeFilter
{
	le_t<u32> startIndex;
	le_t<u32> max;
};


struct SceNpMatching2IntSearchFilter
{
	u8 searchOperator;
	u8 padding[3];
	SceNpMatching2IntAttr attr;
};


struct SceNpMatching2BinSearchFilter
{
	u8 searchOperator;
	u8 padding[3];
	SceNpMatching2BinAttr attr;
};


struct SceNpMatching2Range
{
	le_t<u32> startIndex;
	le_t<u32> total;
	le_t<u32> resultCount;
};


struct SceNpMatching2JoinedSessionInfo
{
	u8 sessionType;
	u8 padding1[1];
	le_t<u16> serverId;
	le_t<u32> worldId;
	le_t<u64> lobbyId;
	le_t<u64> roomId;
	le_t<u64> joinDate;
};


struct SceNpMatching2UserInfo
{
	vm::lptr<SceNpMatching2UserInfo> next;
	SceNpId npId;
	vm::lptr<SceNpMatching2BinAttr> userBinAttr;
	le_t<u32> userBinAttrNum;
	vm::lptr<SceNpMatching2JoinedSessionInfo> joinedSessionInfo;
	le_t<u32> joinedSessionInfoNum;
};

struct SceNpMatching2Server
{
	le_t<u16> serverId;
	u8 status;
	u8 padding[1];
};


struct SceNpMatching2World
{
	vm::lptr<SceNpMatching2World> next;
	le_t<u32> worldId;
	le_t<u32> numOfLobby;
	le_t<u32> maxNumOfTotalLobbyMember;
	le_t<u32> curNumOfTotalLobbyMember;
	le_t<u32> curNumOfRoom;
	le_t<u32> curNumOfTotalRoomMember;
	b8 withEntitlementId;
	SceNpEntitlementId entitlementId;
	u8 padding[3];
};

struct SceNpMatching2LobbyMemberBinAttrInternal
{
	le_t<u64> updateDate;
	SceNpMatching2BinAttr data;
	u8 padding[4];
};


struct SceNpMatching2LobbyMemberDataInternal
{
	vm::lptr<SceNpMatching2LobbyMemberDataInternal> next;
	SceNpId npId;

	le_t<u64> joinDate;
	le_t<u16> memberId;
	u8 padding[2];

	le_t<u32> flagAttr;

	vm::lptr<SceNpMatching2JoinedSessionInfo> joinedSessionInfo;
	le_t<u32> joinedSessionInfoNum;
	vm::lptr<SceNpMatching2LobbyMemberBinAttrInternal> lobbyMemberBinAttrInternal;
	le_t<u32> lobbyMemberBinAttrInternalNum;
};


struct SceNpMatching2LobbyMemberIdList
{
	vm::lptr<u16> memberId;
	le_t<u32> memberIdNum;
	le_t<u16> me;
	u8 padding[6];
};


struct SceNpMatching2LobbyBinAttrInternal
{
	le_t<u64> updateDate;
	le_t<u16> updateMemberId;
	u8 padding[2];
	SceNpMatching2BinAttr data;
};


struct SceNpMatching2LobbyDataExternal
{
	vm::lptr<SceNpMatching2LobbyDataExternal> next;
	le_t<u16> serverId;
	u8 padding1[2];
	le_t<u32> worldId;
	u8 padding2[4];
	le_t<u64> lobbyId;
	le_t<u32> maxSlot;
	le_t<u32> curMemberNum;
	le_t<u32> flagAttr;
	vm::lptr<SceNpMatching2IntAttr> lobbySearchableIntAttrExternal;
	le_t<u32> lobbySearchableIntAttrExternalNum;
	vm::lptr<SceNpMatching2BinAttr> lobbySearchableBinAttrExternal;
	le_t<u32> lobbySearchableBinAttrExternalNum;
	vm::lptr<SceNpMatching2BinAttr> lobbyBinAttrExternal;
	le_t<u32> lobbyBinAttrExternalNum;
	u8 padding3[4];
};


struct SceNpMatching2LobbyDataInternal
{
	le_t<u16> serverId;
	u8 padding1[2];
	le_t<u32> worldId;
	le_t<u64> lobbyId;

	le_t<u32> maxSlot;
	SceNpMatching2LobbyMemberIdList memberIdList;
	le_t<u32> flagAttr;
	vm::lptr<SceNpMatching2LobbyBinAttrInternal> lobbyBinAttrInternal;
	le_t<u32> lobbyBinAttrInternalNum;
};


union SceNpMatching2LobbyMessageDestination
{
	le_t<u16> unicastTarget;

	struct
	{
		vm::lptr<u16> memberId;
		le_t<u32> memberIdNum;
	}
	multicastTarget;
};

struct SceNpMatching2GroupLabel
{
	u8 data[8];
};

struct SceNpMatching2RoomGroupConfig
{
	le_t<u32> slotNum;
	b8 withLabel;
	SceNpMatching2GroupLabel label;
	b8 withPassword;
	u8 padding[2];
};


struct SceNpMatching2RoomGroupPasswordConfig
{
	u8 groupId;
	b8 withPassword;
	u8 padding[1];
};


struct SceNpMatching2RoomMemberBinAttrInternal
{
	le_t<u64> updateDate;
	SceNpMatching2BinAttr data;
	u8 padding[4];
};


struct SceNpMatching2RoomGroup
{
	u8 groupId;
	b8 withPassword;
	b8 withLabel;
	u8 padding[1];
	SceNpMatching2GroupLabel label;
	le_t<u32> slotNum;
	le_t<u32> curGroupMemberNum;
};


struct SceNpMatching2RoomMemberDataExternal
{
	vm::lptr<SceNpMatching2RoomMemberDataExternal> next;
	SceNpId npId;
	le_t<u64> joinDate;
	u8 role;
	u8 padding[7];
};


struct SceNpMatching2RoomMemberDataInternal
{
	vm::lptr<SceNpMatching2RoomMemberDataInternal> next;
	SceNpId npId;

	le_t<u64> joinDate;
	le_t<u16> memberId;
	u8 teamId;
	u8 padding1[1];

	vm::lptr<SceNpMatching2RoomGroup> roomGroup;

	u8 natType;
	u8 padding2[3];
	le_t<u32> flagAttr;
	vm::lptr<SceNpMatching2RoomMemberBinAttrInternal> roomMemberBinAttrInternal;
	le_t<u32> roomMemberBinAttrInternalNum;
};


struct SceNpMatching2RoomMemberDataInternalList
{
	vm::lptr<SceNpMatching2RoomMemberDataInternal> members;
	le_t<u32> membersNum;
	vm::lptr<SceNpMatching2RoomMemberDataInternal> me;
	vm::lptr<SceNpMatching2RoomMemberDataInternal> owner;
};


struct SceNpMatching2RoomBinAttrInternal
{
	le_t<u64> updateDate;
	le_t<u16> updateMemberId;
	u8 padding[2];
	SceNpMatching2BinAttr data;
};


struct SceNpMatching2RoomDataExternal
{
	vm::lptr<SceNpMatching2RoomDataExternal> next;

	le_t<u16> maxSlot;
	le_t<u16> curMemberNum;

	le_t<u16> serverId;
	u8 padding[2];
	le_t<u32> worldId;
	le_t<u64> lobbyId;
	le_t<u64> roomId;

	le_t<u64> passwordSlotMask;
	le_t<u64> joinedSlotMask;
	le_t<u16> publicSlotNum;
	le_t<u16> privateSlotNum;
	le_t<u16> openPublicSlotNum;
	le_t<u16> openPrivateSlotNum;

	vm::lptr<SceNpId> owner;
	le_t<u32> flagAttr;

	vm::lptr<SceNpMatching2RoomGroup> roomGroup;
	le_t<u32> roomGroupNum;
	vm::lptr<SceNpMatching2IntAttr> roomSearchableIntAttrExternal;
	le_t<u32> roomSearchableIntAttrExternalNum;
	vm::lptr<SceNpMatching2BinAttr> roomSearchableBinAttrExternal;
	le_t<u32> roomSearchableBinAttrExternalNum;
	vm::lptr<SceNpMatching2BinAttr> roomBinAttrExternal;
	le_t<u32> roomBinAttrExternalNum;
};


struct SceNpMatching2RoomDataInternal
{
	le_t<u16> maxSlot;

	le_t<u16> serverId;
	le_t<u32> worldId;
	le_t<u64> lobbyId;
	le_t<u64> roomId;

	le_t<u64> passwordSlotMask;
	le_t<u64> joinedSlotMask;
	le_t<u16> publicSlotNum;
	le_t<u16> privateSlotNum;
	le_t<u16> openPublicSlotNum;
	le_t<u16> openPrivateSlotNum;

	SceNpMatching2RoomMemberDataInternalList memberList;

	vm::lptr<SceNpMatching2RoomGroup> roomGroup;
	le_t<u32> roomGroupNum;

	le_t<u32> flagAttr;
	u8 padding[4];
	vm::lptr<SceNpMatching2RoomBinAttrInternal> roomBinAttrInternal;
	le_t<u32> roomBinAttrInternalNum;
};


union SceNpMatching2RoomMessageDestination
{
	le_t<u16> unicastTarget;

	struct
	{
		vm::lptr<u16> memberId;
		le_t<u32> memberIdNum;
	}
	multicastTarget;

	u8 multicastTargetTeamId;
};


struct SceNpMatching2InvitationData
{
	vm::lptr<SceNpMatching2JoinedSessionInfo> targetSession;
	le_t<u32> targetSessionNum;
	vm::lptr<void> optData;
	le_t<u32> optDataLen;
};

using SceNpMatching2RequestCallback = func_def<void(u16 ctxId, u32 reqId, u16 event, s32 errorCode, vm::cptr<void> data, vm::ptr<void> arg)>;
using SceNpMatching2LobbyEventCallback = func_def<void(u16 ctxId, u64 lobbyId, u16 event, s32 errorCode, vm::cptr<void> data, vm::ptr<void> arg)>;
using SceNpMatching2RoomEventCallback = func_def<void(u16 ctxId, u64 roomId, u16 event, s32 errorCode, vm::cptr<void> data, vm::ptr<void> arg)>;
using SceNpMatching2LobbyMessageCallback = func_def<void(u16 ctxId, u64 lobbyId, u16 srcMemberId, u16 event, s32 errorCode, vm::cptr<void> data, vm::ptr<void> arg)>;
using SceNpMatching2RoomMessageCallback = func_def<void(u16 ctxId, u64 roomId, u16 srcMemberId, u16 event, s32 errorCode, vm::cptr<void> data, vm::ptr<void> arg)>;
using SceNpMatching2SignalingCallback = func_def<void(u16 ctxId, u64 roomId, u16 peerMemberId, u16 event, s32 errorCode, vm::ptr<void> arg)>;
using SceNpMatching2ContextCallback = func_def<void(u16 ctxId, u16 event, u8 eventCause, s32 errorCode, vm::ptr<void> arg)>;

struct SceNpMatching2RequestOptParam
{
	vm::lptr<SceNpMatching2RequestCallback> cbFunc;
	vm::lptr<void> cbFuncArg;
	le_t<u32> timeout;
	le_t<u16> appReqId;
	u8 padding[2];
};


struct SceNpMatching2GetWorldInfoListRequest
{
	le_t<u16> serverId;
};



struct SceNpMatching2GetWorldInfoListResponse
{
	vm::lptr<SceNpMatching2World> world;
	le_t<u32> worldNum;
};

struct SceNpMatching2SetUserInfoRequest
{
	le_t<u16> serverId;
	u8 padding[2];
	vm::lptr<SceNpMatching2BinAttr> userBinAttr;
	le_t<u32> userBinAttrNum;
};


struct SceNpMatching2GetUserInfoListRequest
{
	le_t<u16> serverId;
	u8 padding[2];
	vm::lptr<SceNpId> npId;
	le_t<u32> npIdNum;
	vm::lptr<u16> attrId;
	le_t<u32> attrIdNum;
	le_t<s32> option;
};



struct SceNpMatching2GetUserInfoListResponse
{
	vm::lptr<SceNpMatching2UserInfo> userInfo;
	le_t<u32> userInfoNum;
};


struct SceNpMatching2GetRoomMemberDataExternalListRequest
{
	le_t<u64> roomId;
};



struct SceNpMatching2GetRoomMemberDataExternalListResponse
{
	vm::lptr<SceNpMatching2RoomMemberDataExternal> roomMemberDataExternal;
	le_t<u32> roomMemberDataExternalNum;
};


struct SceNpMatching2SetRoomDataExternalRequest
{
	le_t<u64> roomId;
	vm::lptr<SceNpMatching2IntAttr> roomSearchableIntAttrExternal;
	le_t<u32> roomSearchableIntAttrExternalNum;
	vm::lptr<SceNpMatching2BinAttr> roomSearchableBinAttrExternal;
	le_t<u32> roomSearchableBinAttrExternalNum;
	vm::lptr<SceNpMatching2BinAttr> roomBinAttrExternal;
	le_t<u32> roomBinAttrExternalNum;
};


struct SceNpMatching2GetRoomDataExternalListRequest
{
	vm::lptr<u64> roomId;
	le_t<u32> roomIdNum;
	vm::lcptr<u16> attrId;
	le_t<u32> attrIdNum;
};



struct SceNpMatching2GetRoomDataExternalListResponse
{
	vm::lptr<SceNpMatching2RoomDataExternal> roomDataExternal;
	le_t<u32> roomDataExternalNum;
};

struct SceNpMatching2SignalingOptParam
{
	u8 type;
	u8 flag;
	le_t<u16> hubMemberId;
	u8 reserved2[4];
};



struct SceNpMatching2CreateJoinRoomRequest
{
	le_t<u32> worldId;
	u8 padding1[4];
	le_t<u64> lobbyId;

	le_t<u32> maxSlot;
	le_t<u32> flagAttr;
	vm::lptr<SceNpMatching2BinAttr> roomBinAttrInternal;
	le_t<u32> roomBinAttrInternalNum;
	vm::lptr<SceNpMatching2IntAttr> roomSearchableIntAttrExternal;
	le_t<u32> roomSearchableIntAttrExternalNum;
	vm::lptr<SceNpMatching2BinAttr> roomSearchableBinAttrExternal;
	le_t<u32> roomSearchableBinAttrExternalNum;
	vm::lptr<SceNpMatching2BinAttr> roomBinAttrExternal;
	le_t<u32> roomBinAttrExternalNum;
	vm::lptr<SceNpMatching2SessionPassword> roomPassword;
	vm::lptr<SceNpMatching2RoomGroupConfig> groupConfig;
	le_t<u32> groupConfigNum;
	vm::lptr<u64> passwordSlotMask;
	vm::lptr<SceNpId> allowedUser;
	le_t<u32> allowedUserNum;
	vm::lptr<SceNpId> blockedUser;
	le_t<u32> blockedUserNum;

	vm::lptr<SceNpMatching2GroupLabel> joinRoomGroupLabel;
	vm::lptr<SceNpMatching2BinAttr> roomMemberBinAttrInternal;
	le_t<u32> roomMemberBinAttrInternalNum;
	u8 teamId;
	u8 padding2[3];

	vm::lptr<SceNpMatching2SignalingOptParam> sigOptParam;
	u8 padding3[4];
};



struct SceNpMatching2CreateJoinRoomResponse
{
	vm::lptr<SceNpMatching2RoomDataInternal> roomDataInternal;
};


struct SceNpMatching2JoinRoomRequest
{
	le_t<u64> roomId;
	vm::lptr<SceNpMatching2SessionPassword> roomPassword;
	vm::lptr<SceNpMatching2GroupLabel> joinRoomGroupLabel;
	vm::lptr<SceNpMatching2BinAttr> roomMemberBinAttrInternal;
	le_t<u32> roomMemberBinAttrInternalNum;
	SceNpMatching2PresenceOptionData optData;
	u8 teamId;
	u8 padding[3];
	vm::lptr<SceNpId> blockedUser;
	le_t<u32> blockedUserNum;
};



struct SceNpMatching2JoinRoomResponse
{
	vm::lptr<SceNpMatching2RoomDataInternal> roomDataInternal;
};


struct SceNpMatching2LeaveRoomRequest
{
	le_t<u64> roomId;
	SceNpMatching2PresenceOptionData optData;
	u8 padding[4];
};


struct SceNpMatching2GrantRoomOwnerRequest
{
	le_t<u64> roomId;
	le_t<u16> newOwner;
	u8 padding[2];
	SceNpMatching2PresenceOptionData optData;
};


struct SceNpMatching2KickoutRoomMemberRequest
{
	le_t<u64> roomId;
	le_t<u16> target;
	u8 blockKickFlag;
	u8 padding[1];
	SceNpMatching2PresenceOptionData optData;
};


struct SceNpMatching2SearchRoomRequest
{
	le_t<s32> option;
	le_t<u32> worldId;
	le_t<u64> lobbyId;
	SceNpMatching2RangeFilter rangeFilter;
	le_t<u32> flagFilter;
	le_t<u32> flagAttr;
	vm::lptr<SceNpMatching2IntSearchFilter> intFilter;
	le_t<u32> intFilterNum;
	vm::lptr<SceNpMatching2BinSearchFilter> binFilter;
	le_t<u32> binFilterNum;
	vm::lptr<u16> attrId;
	le_t<u32> attrIdNum;
};



struct SceNpMatching2SearchRoomResponse
{
	SceNpMatching2Range range;
	vm::lptr<SceNpMatching2RoomDataExternal> roomDataExternal;
};


struct SceNpMatching2SendRoomMessageRequest
{
	le_t<u64> roomId;
	u8 castType;
	u8 padding[3];
	SceNpMatching2RoomMessageDestination dst;
	vm::lcptr<void> msg;
	le_t<u32> msgLen;
	le_t<s32> option;
};


struct SceNpMatching2SendRoomChatMessageRequest
{
	le_t<u64> roomId;
	u8 castType;
	u8 padding[3];
	SceNpMatching2RoomMessageDestination dst;
	vm::lcptr<void> msg;
	le_t<u32> msgLen;
	le_t<s32> option;
};



struct SceNpMatching2SendRoomChatMessageResponse
{
	b8 filtered;
};


struct SceNpMatching2SetRoomDataInternalRequest
{
	le_t<u64> roomId;
	le_t<u32> flagFilter;
	le_t<u32> flagAttr;
	vm::lptr<SceNpMatching2BinAttr> roomBinAttrInternal;
	le_t<u32> roomBinAttrInternalNum;
	vm::lptr<SceNpMatching2RoomGroupPasswordConfig> passwordConfig;
	le_t<u32> passwordConfigNum;
	vm::lptr<u64> passwordSlotMask;
	vm::lptr<u16> ownerPrivilegeRank;
	le_t<u32> ownerPrivilegeRankNum;
	u8 padding[4];
};


struct SceNpMatching2GetRoomDataInternalRequest
{
	le_t<u64> roomId;
	vm::lcptr<u16> attrId;
	le_t<u32> attrIdNum;
};



struct SceNpMatching2GetRoomDataInternalResponse
{
	vm::lptr<SceNpMatching2RoomDataInternal> roomDataInternal;
};


struct SceNpMatching2SetRoomMemberDataInternalRequest
{
	le_t<u64> roomId;
	le_t<u16> memberId;
	u8 teamId;
	u8 padding[5];
	le_t<u32> flagFilter;
	le_t<u32> flagAttr;
	vm::lptr<SceNpMatching2BinAttr> roomMemberBinAttrInternal;
	le_t<u32> roomMemberBinAttrInternalNum;
};


struct SceNpMatching2GetRoomMemberDataInternalRequest
{
	le_t<u64> roomId;
	le_t<u16> memberId;
	u8 padding[6];
	vm::lcptr<u16> attrId;
	le_t<u32> attrIdNum;
};



struct SceNpMatching2GetRoomMemberDataInternalResponse
{
	vm::lptr<SceNpMatching2RoomMemberDataInternal> roomMemberDataInternal;
};


struct SceNpMatching2SetSignalingOptParamRequest
{
	le_t<u64> roomId;
	SceNpMatching2SignalingOptParam sigOptParam;
};


struct SceNpMatching2GetLobbyInfoListRequest
{
	le_t<u32> worldId;
	SceNpMatching2RangeFilter rangeFilter;
	vm::lptr<u16> attrId;
	le_t<u32> attrIdNum;
};



struct SceNpMatching2GetLobbyInfoListResponse
{
	SceNpMatching2Range range;
	vm::lptr<SceNpMatching2LobbyDataExternal> lobbyDataExternal;
};


struct SceNpMatching2JoinLobbyRequest
{
	le_t<u64> lobbyId;
	vm::lptr<SceNpMatching2JoinedSessionInfo> joinedSessionInfo;
	le_t<u32> joinedSessionInfoNum;
	vm::lptr<SceNpMatching2BinAttr> lobbyMemberBinAttrInternal;
	le_t<u32> lobbyMemberBinAttrInternalNum;
	SceNpMatching2PresenceOptionData optData;
	u8 padding[4];
};



struct SceNpMatching2JoinLobbyResponse
{
	vm::lptr<SceNpMatching2LobbyDataInternal> lobbyDataInternal;
};


struct SceNpMatching2LeaveLobbyRequest
{
	le_t<u64> lobbyId;
	SceNpMatching2PresenceOptionData optData;
	u8 padding[4];
};


struct SceNpMatching2SetLobbyMemberDataInternalRequest
{
	le_t<u64> lobbyId;
	le_t<u16> memberId;
	u8 padding1[2];
	le_t<u32> flagFilter;
	le_t<u32> flagAttr;
	vm::lptr<SceNpMatching2JoinedSessionInfo> joinedSessionInfo;
	le_t<u32> joinedSessionInfoNum;
	vm::lptr<SceNpMatching2BinAttr> lobbyMemberBinAttrInternal;
	le_t<u32> lobbyMemberBinAttrInternalNum;
	u8 padding2[4];
};


struct SceNpMatching2GetLobbyMemberDataInternalRequest
{
	le_t<u64> lobbyId;
	le_t<u16> memberId;
	u8 padding[6];
	vm::lcptr<u16> attrId;
	le_t<u32> attrIdNum;
};



struct SceNpMatching2GetLobbyMemberDataInternalResponse
{
	vm::lptr<SceNpMatching2LobbyMemberDataInternal> lobbyMemberDataInternal;
};



struct SceNpMatching2GetLobbyMemberDataInternalListRequest
{
	le_t<u64> lobbyId;
	vm::lptr<u16> memberId;
	le_t<u32> memberIdNum;
	vm::lcptr<u16> attrId;
	le_t<u32> attrIdNum;
	b8 extendedData;
	u8 padding[7];
};



struct SceNpMatching2GetLobbyMemberDataInternalListResponse
{
	vm::lptr<SceNpMatching2LobbyMemberDataInternal> lobbyMemberDataInternal;
	le_t<u32> lobbyMemberDataInternalNum;
};


struct SceNpMatching2SendLobbyChatMessageRequest
{
	le_t<u64> lobbyId;
	u8 castType;
	u8 padding[3];
	SceNpMatching2LobbyMessageDestination dst;
	vm::lcptr<void> msg;
	le_t<u32> msgLen;
	le_t<s32> option;
};



struct SceNpMatching2SendLobbyChatMessageResponse
{
	b8 filtered;
};


struct SceNpMatching2SendLobbyInvitationRequest
{
	le_t<u64> lobbyId;
	u8 castType;
	u8 padding[3];
	SceNpMatching2LobbyMessageDestination dst;
	SceNpMatching2InvitationData invitationData;
	le_t<s32> option;
};


struct SceNpMatching2RoomMemberUpdateInfo
{
	vm::lptr<SceNpMatching2RoomMemberDataInternal> roomMemberDataInternal;
	u8 eventCause;
	u8 padding[3];
	SceNpMatching2PresenceOptionData optData;
};


struct SceNpMatching2RoomOwnerUpdateInfo
{
	le_t<u16> prevOwner;
	le_t<u16> newOwner;
	u8 eventCause;
	u8 padding[3];
	vm::lptr<SceNpMatching2SessionPassword> roomPassword;
	SceNpMatching2PresenceOptionData optData;
};


struct SceNpMatching2RoomUpdateInfo
{
	u8 eventCause;
	u8 padding[3];
	le_t<s32> errorCode;
	SceNpMatching2PresenceOptionData optData;
};


struct SceNpMatching2RoomDataInternalUpdateInfo
{
	vm::lptr<SceNpMatching2RoomDataInternal> newRoomDataInternal;
	vm::lptr<u32> newFlagAttr;
	vm::lptr<u32> prevFlagAttr;
	vm::lptr<u64> newRoomPasswordSlotMask;
	vm::lptr<u64> prevRoomPasswordSlotMask;
	vm::lptr<SceNpMatching2RoomGroup> *newRoomGroup;
	le_t<u32> newRoomGroupNum;
	vm::lptr<SceNpMatching2RoomBinAttrInternal> *newRoomBinAttrInternal;
	le_t<u32> newRoomBinAttrInternalNum;
};


struct SceNpMatching2RoomMemberDataInternalUpdateInfo
{
	vm::lptr<SceNpMatching2RoomMemberDataInternal> newRoomMemberDataInternal;
	vm::lptr<u32> newFlagAttr;
	vm::lptr<u32> prevFlagAttr;
	vm::lptr<u8> newTeamId;
	vm::lptr<SceNpMatching2RoomMemberBinAttrInternal> *newRoomMemberBinAttrInternal;
	le_t<u32> newRoomMemberBinAttrInternalNum;
};


struct SceNpMatching2SignalingOptParamUpdateInfo
{
	SceNpMatching2SignalingOptParam newSignalingOptParam;
};


struct SceNpMatching2RoomMessageInfo
{
	b8 filtered;
	u8 castType;
	u8 padding[2];
	vm::lptr<SceNpMatching2RoomMessageDestination> dst;
	vm::lptr<SceNpId> srcMember;
	vm::lcptr<void> msg;
	le_t<u32> msgLen;
};


struct SceNpMatching2LobbyMemberUpdateInfo
{
	vm::lptr<SceNpMatching2LobbyMemberDataInternal> lobbyMemberDataInternal;
	u8 eventCause;
	u8 padding[3];
	SceNpMatching2PresenceOptionData optData;
};


struct SceNpMatching2LobbyUpdateInfo
{
	u8 eventCause;
	u8 padding[3];
	le_t<s32> errorCode;
};


struct SceNpMatching2LobbyMemberDataInternalUpdateInfo
{
	le_t<u16> memberId;
	u8 padding[2];
	SceNpId npId;
	le_t<u32> flagFilter;
	le_t<u32> newFlagAttr;
	vm::lptr<SceNpMatching2JoinedSessionInfo> newJoinedSessionInfo;
	le_t<u32> newJoinedSessionInfoNum;
	vm::lptr<SceNpMatching2LobbyMemberBinAttrInternal> newLobbyMemberBinAttrInternal;
	le_t<u32> newLobbyMemberBinAttrInternalNum;
};


struct SceNpMatching2LobbyMessageInfo
{
	b8 filtered;
	u8 castType;
	u8 padding[2];
	vm::lptr<SceNpMatching2LobbyMessageDestination> dst;
	vm::lptr<SceNpId> srcMember;
	vm::lcptr<void> msg;
	le_t<u32> msgLen;
};


struct SceNpMatching2LobbyInvitationInfo
{
	u8 castType;
	u8 padding[3];
	vm::lptr<SceNpMatching2LobbyMessageDestination> dst;
	vm::lptr<SceNpId> srcMember;
	SceNpMatching2InvitationData invitationData;
};

union SceNpMatching2SignalingConnectionInfo
{
	le_t<u32> rtt;
	le_t<u32> bandwidth;
	SceNpId npId;

	struct
	{
		SceNetInAddr addr;
		le_t<u16> port;
		u8 padding[2];
	}
	address;

	le_t<u32> packetLoss;
};

struct SceNpMatching2SignalingNetInfo
{
	le_t<u32> size;
	SceNetInAddr localAddr;
	SceNetInAddr mappedAddr;
	le_t<s32> natStatus;
};

extern psv_log_base sceNpMatching;
