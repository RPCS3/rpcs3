#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceNet.h"
#include "sceNpCommon.h"

extern psv_log_base sceNpMatching;

struct SceNpMatching2MemoryInfo
{
	u32 totalMemSize;
	u32 curMemUsage;
	u32 maxMemUsage;
	u8 reserved[12];
};

typedef u16 SceNpMatching2ServerId;
typedef u32 SceNpMatching2WorldId;
typedef u16 SceNpMatching2WorldNumber;
typedef u64 SceNpMatching2LobbyId;
typedef u16 SceNpMatching2LobbyNumber;
typedef u64 SceNpMatching2RoomId;
typedef u16 SceNpMatching2RoomNumber;
typedef u16 SceNpMatching2ContextId;
typedef u32 SceNpMatching2RequestId;
typedef u32 SceNpMatching2SignalingRequestId;
typedef u8 SceNpMatching2NatType;
typedef u8 SceNpMatching2Operator;
typedef u8 SceNpMatching2CastType;

struct SceNpMatching2SessionPassword
{
	u8 data[8];
};

typedef u8 SceNpMatching2SessionType;
typedef u8 SceNpMatching2EventCause;

struct SceNpMatching2PresenceOptionData
{
	u8 data[16];
	u32 len;
};

typedef u16 SceNpMatching2AttributeId;
typedef u32 SceNpMatching2FlagAttr;

struct SceNpMatching2IntAttr
{
	SceNpMatching2AttributeId id;
	u8 padding[2];
	u32 num;
};


struct SceNpMatching2BinAttr
{
	SceNpMatching2AttributeId id;
	u8 padding[2];
	vm::psv::ptr<void> ptr;
	u32 size;
};


struct SceNpMatching2RangeFilter
{
	u32 startIndex;
	u32 max;
};


struct SceNpMatching2IntSearchFilter
{
	SceNpMatching2Operator searchOperator;
	u8 padding[3];
	SceNpMatching2IntAttr attr;
};


struct SceNpMatching2BinSearchFilter
{
	SceNpMatching2Operator searchOperator;
	u8 padding[3];
	SceNpMatching2BinAttr attr;
};


struct SceNpMatching2Range
{
	u32 startIndex;
	u32 total;
	u32 resultCount;
};


struct SceNpMatching2JoinedSessionInfo
{
	SceNpMatching2SessionType sessionType;
	u8 padding1[1];
	SceNpMatching2ServerId serverId;
	SceNpMatching2WorldId worldId;
	SceNpMatching2LobbyId lobbyId;
	SceNpMatching2RoomId roomId;
	u64 joinDate;
};


struct SceNpMatching2UserInfo
{
	vm::psv::ptr<SceNpMatching2UserInfo> next;
	SceNpId npId;
	vm::psv::ptr<SceNpMatching2BinAttr> userBinAttr;
	u32 userBinAttrNum;
	vm::psv::ptr<SceNpMatching2JoinedSessionInfo> joinedSessionInfo;
	u32 joinedSessionInfoNum;
};

typedef u8 SceNpMatching2ServerStatus;

struct SceNpMatching2Server
{
	SceNpMatching2ServerId serverId;
	SceNpMatching2ServerStatus status;
	u8 padding[1];
};


struct SceNpMatching2World
{
	vm::psv::ptr<SceNpMatching2World> next;
	SceNpMatching2WorldId worldId;
	u32 numOfLobby;
	u32 maxNumOfTotalLobbyMember;
	u32 curNumOfTotalLobbyMember;
	u32 curNumOfRoom;
	u32 curNumOfTotalRoomMember;
	bool withEntitlementId;
	SceNpEntitlementId entitlementId;
	u8 padding[3];
};

typedef u16 SceNpMatching2LobbyMemberId;

struct SceNpMatching2LobbyMemberBinAttrInternal
{
	u64 updateDate;
	SceNpMatching2BinAttr data;
	u8 padding[4];
};


struct SceNpMatching2LobbyMemberDataInternal
{
	vm::psv::ptr<SceNpMatching2LobbyMemberDataInternal> next;
	SceNpId npId;

	u64 joinDate;
	SceNpMatching2LobbyMemberId memberId;
	u8 padding[2];

	SceNpMatching2FlagAttr flagAttr;

	vm::psv::ptr<SceNpMatching2JoinedSessionInfo> joinedSessionInfo;
	u32 joinedSessionInfoNum;
	vm::psv::ptr<SceNpMatching2LobbyMemberBinAttrInternal> lobbyMemberBinAttrInternal;
	u32 lobbyMemberBinAttrInternalNum;
};


struct SceNpMatching2LobbyMemberIdList
{
	vm::psv::ptr<SceNpMatching2LobbyMemberId> memberId;
	u32 memberIdNum;
	SceNpMatching2LobbyMemberId me;
	u8 padding[6];
};


struct SceNpMatching2LobbyBinAttrInternal
{
	u64 updateDate;
	SceNpMatching2LobbyMemberId updateMemberId;
	u8 padding[2];
	SceNpMatching2BinAttr data;
};


struct SceNpMatching2LobbyDataExternal
{
	vm::psv::ptr<SceNpMatching2LobbyDataExternal> next;
	SceNpMatching2ServerId serverId;
	u8 padding1[2];
	SceNpMatching2WorldId worldId;
	u8 padding2[4];
	SceNpMatching2LobbyId lobbyId;
	u32 maxSlot;
	u32 curMemberNum;
	SceNpMatching2FlagAttr flagAttr;
	vm::psv::ptr<SceNpMatching2IntAttr> lobbySearchableIntAttrExternal;
	u32 lobbySearchableIntAttrExternalNum;
	vm::psv::ptr<SceNpMatching2BinAttr> lobbySearchableBinAttrExternal;
	u32 lobbySearchableBinAttrExternalNum;
	vm::psv::ptr<SceNpMatching2BinAttr> lobbyBinAttrExternal;
	u32 lobbyBinAttrExternalNum;
	u8 padding3[4];
};


struct SceNpMatching2LobbyDataInternal
{
	SceNpMatching2ServerId serverId;
	u8 padding1[2];
	SceNpMatching2WorldId worldId;
	SceNpMatching2LobbyId lobbyId;

	u32 maxSlot;
	SceNpMatching2LobbyMemberIdList memberIdList;
	SceNpMatching2FlagAttr flagAttr;
	vm::psv::ptr<SceNpMatching2LobbyBinAttrInternal> lobbyBinAttrInternal;
	u32 lobbyBinAttrInternalNum;
};


union SceNpMatching2LobbyMessageDestination
{
	SceNpMatching2LobbyMemberId unicastTarget;
	struct
	{
		vm::psv::ptr<SceNpMatching2LobbyMemberId> memberId;
		u32 memberIdNum;

	} multicastTarget;
};

typedef u8 SceNpMatching2RoomGroupId;
typedef u16 SceNpMatching2RoomMemberId;
typedef u8 SceNpMatching2TeamId;
typedef u8 SceNpMatching2Role;
typedef u8 SceNpMatching2BlockKickFlag;

struct SceNpMatching2GroupLabel
{
	u8 data[8];
};

typedef u64 SceNpMatching2RoomPasswordSlotMask;
typedef u64 SceNpMatching2RoomJoinedSlotMask;

struct SceNpMatching2RoomGroupConfig
{
	u32 slotNum;
	bool withLabel;
	SceNpMatching2GroupLabel label;
	bool withPassword;
	u8 padding[2];
};


struct SceNpMatching2RoomGroupPasswordConfig
{
	SceNpMatching2RoomGroupId groupId;
	bool withPassword;
	u8 padding[1];
};


struct SceNpMatching2RoomMemberBinAttrInternal
{
	u64 updateDate;
	SceNpMatching2BinAttr data;
	u8 padding[4];
};


struct SceNpMatching2RoomGroup
{
	SceNpMatching2RoomGroupId groupId;
	bool withPassword;
	bool withLabel;
	u8 padding[1];
	SceNpMatching2GroupLabel label;
	u32 slotNum;
	u32 curGroupMemberNum;
};


struct SceNpMatching2RoomMemberDataExternal
{
	struct vm::psv::ptr<SceNpMatching2RoomMemberDataExternal> next;
	SceNpId npId;
	u64 joinDate;
	SceNpMatching2Role role;
	u8 padding[7];
};


struct SceNpMatching2RoomMemberDataInternal
{
	struct vm::psv::ptr<SceNpMatching2RoomMemberDataInternal> next;
	SceNpId npId;

	u64 joinDate;
	SceNpMatching2RoomMemberId memberId;
	SceNpMatching2TeamId teamId;
	u8 padding1[1];

	vm::psv::ptr<SceNpMatching2RoomGroup> roomGroup;

	SceNpMatching2NatType natType;
	u8 padding2[3];
	SceNpMatching2FlagAttr flagAttr;
	vm::psv::ptr<SceNpMatching2RoomMemberBinAttrInternal> roomMemberBinAttrInternal;
	u32 roomMemberBinAttrInternalNum;
};


struct SceNpMatching2RoomMemberDataInternalList
{
	vm::psv::ptr<SceNpMatching2RoomMemberDataInternal> members;
	u32 membersNum;
	vm::psv::ptr<SceNpMatching2RoomMemberDataInternal> me;
	vm::psv::ptr<SceNpMatching2RoomMemberDataInternal> owner;
};


struct SceNpMatching2RoomBinAttrInternal
{
	u64 updateDate;
	SceNpMatching2RoomMemberId updateMemberId;
	u8 padding[2];
	SceNpMatching2BinAttr data;
};


struct SceNpMatching2RoomDataExternal
{
	vm::psv::ptr<SceNpMatching2RoomDataExternal> next;

	u16 maxSlot;
	u16 curMemberNum;

	SceNpMatching2ServerId serverId;
	u8 padding[2];
	SceNpMatching2WorldId worldId;
	SceNpMatching2LobbyId lobbyId;
	SceNpMatching2RoomId roomId;

	SceNpMatching2RoomPasswordSlotMask passwordSlotMask;
	SceNpMatching2RoomJoinedSlotMask joinedSlotMask;
	u16 publicSlotNum;
	u16 privateSlotNum;
	u16 openPublicSlotNum;
	u16 openPrivateSlotNum;

	vm::psv::ptr<SceNpId> owner;
	SceNpMatching2FlagAttr flagAttr;

	vm::psv::ptr<SceNpMatching2RoomGroup> roomGroup;
	u32 roomGroupNum;
	vm::psv::ptr<SceNpMatching2IntAttr> roomSearchableIntAttrExternal;
	u32 roomSearchableIntAttrExternalNum;
	vm::psv::ptr<SceNpMatching2BinAttr> roomSearchableBinAttrExternal;
	u32 roomSearchableBinAttrExternalNum;
	vm::psv::ptr<SceNpMatching2BinAttr> roomBinAttrExternal;
	u32 roomBinAttrExternalNum;
};


struct SceNpMatching2RoomDataInternal
{
	u16 maxSlot;

	SceNpMatching2ServerId serverId;
	SceNpMatching2WorldId worldId;
	SceNpMatching2LobbyId lobbyId;
	SceNpMatching2RoomId roomId;

	SceNpMatching2RoomPasswordSlotMask passwordSlotMask;
	SceNpMatching2RoomJoinedSlotMask joinedSlotMask;
	u16 publicSlotNum;
	u16 privateSlotNum;
	u16 openPublicSlotNum;
	u16 openPrivateSlotNum;

	SceNpMatching2RoomMemberDataInternalList memberList;

	vm::psv::ptr<SceNpMatching2RoomGroup> roomGroup;
	u32 roomGroupNum;

	SceNpMatching2FlagAttr flagAttr;
	u8 padding[4];
	vm::psv::ptr<SceNpMatching2RoomBinAttrInternal> roomBinAttrInternal;
	u32 roomBinAttrInternalNum;
};


union SceNpMatching2RoomMessageDestination
{
	SceNpMatching2RoomMemberId unicastTarget;
	struct
	{
		vm::psv::ptr<SceNpMatching2RoomMemberId> memberId;
		u32 memberIdNum;

	} multicastTarget;
	SceNpMatching2TeamId multicastTargetTeamId;
};


struct SceNpMatching2InvitationData
{
	vm::psv::ptr<SceNpMatching2JoinedSessionInfo> targetSession;
	u32 targetSessionNum;
	vm::psv::ptr<void> optData;
	u32 optDataLen;
};

typedef u16 SceNpMatching2Event;

typedef vm::psv::ptr<void(
	SceNpMatching2ContextId ctxId,
	SceNpMatching2RequestId reqId,
	SceNpMatching2Event event,
	s32 errorCode,
	vm::psv::ptr<const void> data,
	vm::psv::ptr<void> arg
	)> SceNpMatching2RequestCallback;

typedef vm::psv::ptr<void(
	SceNpMatching2ContextId ctxId,
	SceNpMatching2LobbyId lobbyId,
	SceNpMatching2Event event,
	s32 errorCode,
	vm::psv::ptr<const void> data,
	vm::psv::ptr<void> arg
	)> SceNpMatching2LobbyEventCallback;

typedef vm::psv::ptr<void(
	SceNpMatching2ContextId ctxId,
	SceNpMatching2RoomId roomId,
	SceNpMatching2Event event,
	s32 errorCode,
	vm::psv::ptr<const void> data,
	vm::psv::ptr<void> arg
	)> SceNpMatching2RoomEventCallback;

typedef vm::psv::ptr<void(
	SceNpMatching2ContextId ctxId,
	SceNpMatching2LobbyId lobbyId,
	SceNpMatching2LobbyMemberId srcMemberId,
	SceNpMatching2Event event,
	s32 errorCode,
	vm::psv::ptr<const void> data,
	vm::psv::ptr<void> arg
	)> SceNpMatching2LobbyMessageCallback;

typedef vm::psv::ptr<void(
	SceNpMatching2ContextId ctxId,
	SceNpMatching2RoomId roomId,
	SceNpMatching2RoomMemberId srcMemberId,
	SceNpMatching2Event event,
	s32 errorCode,
	vm::psv::ptr<const void> data,
	vm::psv::ptr<void> arg
	)> SceNpMatching2RoomMessageCallback;

typedef vm::psv::ptr<void(SceNpMatching2ContextId ctxId,
	SceNpMatching2RoomId roomId,
	SceNpMatching2RoomMemberId peerMemberId,
	SceNpMatching2Event event,
	s32 errorCode,
	vm::psv::ptr<void> arg
	)> SceNpMatching2SignalingCallback;

typedef vm::psv::ptr<void(SceNpMatching2ContextId ctxId,
	SceNpMatching2Event event,
	SceNpMatching2EventCause eventCause,
	s32 errorCode,
	vm::psv::ptr<void> arg
	)> SceNpMatching2ContextCallback;


struct SceNpMatching2RequestOptParam
{
	SceNpMatching2RequestCallback cbFunc;
	vm::psv::ptr<void> cbFuncArg;
	u32 timeout;
	u16 appReqId;
	u8 padding[2];
};


struct SceNpMatching2GetWorldInfoListRequest
{
	SceNpMatching2ServerId serverId;
};



struct SceNpMatching2GetWorldInfoListResponse
{
	vm::psv::ptr<SceNpMatching2World> world;
	u32 worldNum;
};

struct SceNpMatching2SetUserInfoRequest
{
	SceNpMatching2ServerId serverId;
	u8 padding[2];
	vm::psv::ptr<SceNpMatching2BinAttr> userBinAttr;
	u32 userBinAttrNum;
};


struct SceNpMatching2GetUserInfoListRequest
{
	SceNpMatching2ServerId serverId;
	u8 padding[2];
	vm::psv::ptr<SceNpId> npId;
	u32 npIdNum;
	vm::psv::ptr<SceNpMatching2AttributeId> attrId;
	u32 attrIdNum;
	s32 option;
};



struct SceNpMatching2GetUserInfoListResponse
{
	vm::psv::ptr<SceNpMatching2UserInfo> userInfo;
	u32 userInfoNum;
};


struct SceNpMatching2GetRoomMemberDataExternalListRequest
{
	SceNpMatching2RoomId roomId;
};



struct SceNpMatching2GetRoomMemberDataExternalListResponse
{
	vm::psv::ptr<SceNpMatching2RoomMemberDataExternal> roomMemberDataExternal;
	u32 roomMemberDataExternalNum;
};


struct SceNpMatching2SetRoomDataExternalRequest
{
	SceNpMatching2RoomId roomId;
	vm::psv::ptr<SceNpMatching2IntAttr> roomSearchableIntAttrExternal;
	u32 roomSearchableIntAttrExternalNum;
	vm::psv::ptr<SceNpMatching2BinAttr> roomSearchableBinAttrExternal;
	u32 roomSearchableBinAttrExternalNum;
	vm::psv::ptr<SceNpMatching2BinAttr> roomBinAttrExternal;
	u32 roomBinAttrExternalNum;
};


struct SceNpMatching2GetRoomDataExternalListRequest
{
	vm::psv::ptr<SceNpMatching2RoomId> roomId;
	u32 roomIdNum;
	vm::psv::ptr<const SceNpMatching2AttributeId> attrId;
	u32 attrIdNum;
};



struct SceNpMatching2GetRoomDataExternalListResponse
{
	vm::psv::ptr<SceNpMatching2RoomDataExternal> roomDataExternal;
	u32 roomDataExternalNum;
};

typedef u8 SceNpMatching2SignalingType;
typedef u8 SceNpMatching2SignalingFlag;

struct SceNpMatching2SignalingOptParam
{
	SceNpMatching2SignalingType type;
	SceNpMatching2SignalingFlag flag;
	SceNpMatching2RoomMemberId hubMemberId;
	u8 reserved2[4];
};



struct SceNpMatching2CreateJoinRoomRequest
{
	SceNpMatching2WorldId worldId;
	u8 padding1[4];
	SceNpMatching2LobbyId lobbyId;

	u32 maxSlot;
	SceNpMatching2FlagAttr flagAttr;
	vm::psv::ptr<SceNpMatching2BinAttr> roomBinAttrInternal;
	u32 roomBinAttrInternalNum;
	vm::psv::ptr<SceNpMatching2IntAttr> roomSearchableIntAttrExternal;
	u32 roomSearchableIntAttrExternalNum;
	vm::psv::ptr<SceNpMatching2BinAttr> roomSearchableBinAttrExternal;
	u32 roomSearchableBinAttrExternalNum;
	vm::psv::ptr<SceNpMatching2BinAttr> roomBinAttrExternal;
	u32 roomBinAttrExternalNum;
	vm::psv::ptr<SceNpMatching2SessionPassword> roomPassword;
	vm::psv::ptr<SceNpMatching2RoomGroupConfig> groupConfig;
	u32 groupConfigNum;
	vm::psv::ptr<SceNpMatching2RoomPasswordSlotMask> passwordSlotMask;
	vm::psv::ptr<SceNpId> allowedUser;
	u32 allowedUserNum;
	vm::psv::ptr<SceNpId> blockedUser;
	u32 blockedUserNum;

	vm::psv::ptr<SceNpMatching2GroupLabel> joinRoomGroupLabel;
	vm::psv::ptr<SceNpMatching2BinAttr> roomMemberBinAttrInternal;
	u32 roomMemberBinAttrInternalNum;
	SceNpMatching2TeamId teamId;
	u8 padding2[3];

	vm::psv::ptr<SceNpMatching2SignalingOptParam> sigOptParam;
	u8 padding3[4];
};



struct SceNpMatching2CreateJoinRoomResponse
{
	vm::psv::ptr<SceNpMatching2RoomDataInternal> roomDataInternal;
};


struct SceNpMatching2JoinRoomRequest
{
	SceNpMatching2RoomId roomId;
	vm::psv::ptr<SceNpMatching2SessionPassword> roomPassword;
	vm::psv::ptr<SceNpMatching2GroupLabel> joinRoomGroupLabel;
	vm::psv::ptr<SceNpMatching2BinAttr> roomMemberBinAttrInternal;
	u32 roomMemberBinAttrInternalNum;
	SceNpMatching2PresenceOptionData optData;
	SceNpMatching2TeamId teamId;
	u8 padding[3];
	vm::psv::ptr<SceNpId> blockedUser;
	u32 blockedUserNum;
};



struct SceNpMatching2JoinRoomResponse
{
	vm::psv::ptr<SceNpMatching2RoomDataInternal> roomDataInternal;
};


struct SceNpMatching2LeaveRoomRequest
{
	SceNpMatching2RoomId roomId;
	SceNpMatching2PresenceOptionData optData;
	u8 padding[4];
};


struct SceNpMatching2GrantRoomOwnerRequest
{
	SceNpMatching2RoomId roomId;
	SceNpMatching2RoomMemberId newOwner;
	u8 padding[2];
	SceNpMatching2PresenceOptionData optData;
};


struct SceNpMatching2KickoutRoomMemberRequest
{
	SceNpMatching2RoomId roomId;
	SceNpMatching2RoomMemberId target;
	SceNpMatching2BlockKickFlag blockKickFlag;
	u8 padding[1];
	SceNpMatching2PresenceOptionData optData;
};


struct SceNpMatching2SearchRoomRequest
{
	s32 option;
	SceNpMatching2WorldId worldId;
	SceNpMatching2LobbyId lobbyId;
	SceNpMatching2RangeFilter rangeFilter;
	SceNpMatching2FlagAttr flagFilter;
	SceNpMatching2FlagAttr flagAttr;
	vm::psv::ptr<SceNpMatching2IntSearchFilter> intFilter;
	u32 intFilterNum;
	vm::psv::ptr<SceNpMatching2BinSearchFilter> binFilter;
	u32 binFilterNum;
	vm::psv::ptr<SceNpMatching2AttributeId> attrId;
	u32 attrIdNum;
};



struct SceNpMatching2SearchRoomResponse
{
	SceNpMatching2Range range;
	vm::psv::ptr<SceNpMatching2RoomDataExternal> roomDataExternal;
};


struct SceNpMatching2SendRoomMessageRequest
{
	SceNpMatching2RoomId roomId;
	SceNpMatching2CastType castType;
	u8 padding[3];
	SceNpMatching2RoomMessageDestination dst;
	vm::psv::ptr<const void> msg;
	u32 msgLen;
	s32 option;
};


struct SceNpMatching2SendRoomChatMessageRequest
{
	SceNpMatching2RoomId roomId;
	SceNpMatching2CastType castType;
	u8 padding[3];
	SceNpMatching2RoomMessageDestination dst;
	vm::psv::ptr<const void> msg;
	u32 msgLen;
	s32 option;
};



struct SceNpMatching2SendRoomChatMessageResponse
{
	bool filtered;
};


struct SceNpMatching2SetRoomDataInternalRequest
{
	SceNpMatching2RoomId roomId;
	SceNpMatching2FlagAttr flagFilter;
	SceNpMatching2FlagAttr flagAttr;
	vm::psv::ptr<SceNpMatching2BinAttr> roomBinAttrInternal;
	u32 roomBinAttrInternalNum;
	vm::psv::ptr<SceNpMatching2RoomGroupPasswordConfig> passwordConfig;
	u32 passwordConfigNum;
	vm::psv::ptr<SceNpMatching2RoomPasswordSlotMask> passwordSlotMask;
	vm::psv::ptr<SceNpMatching2RoomMemberId> ownerPrivilegeRank;
	u32 ownerPrivilegeRankNum;
	u8 padding[4];
};


struct SceNpMatching2GetRoomDataInternalRequest
{
	SceNpMatching2RoomId roomId;
	vm::psv::ptr<const SceNpMatching2AttributeId> attrId;
	u32 attrIdNum;
};



struct SceNpMatching2GetRoomDataInternalResponse
{
	vm::psv::ptr<SceNpMatching2RoomDataInternal> roomDataInternal;
};


struct SceNpMatching2SetRoomMemberDataInternalRequest
{
	SceNpMatching2RoomId roomId;
	SceNpMatching2RoomMemberId memberId;
	SceNpMatching2TeamId teamId;
	u8 padding[5];
	SceNpMatching2FlagAttr flagFilter;
	SceNpMatching2FlagAttr flagAttr;
	vm::psv::ptr<SceNpMatching2BinAttr> roomMemberBinAttrInternal;
	u32 roomMemberBinAttrInternalNum;
};


struct SceNpMatching2GetRoomMemberDataInternalRequest
{
	SceNpMatching2RoomId roomId;
	SceNpMatching2RoomMemberId memberId;
	u8 padding[6];
	vm::psv::ptr<const SceNpMatching2AttributeId> attrId;
	u32 attrIdNum;
};



struct SceNpMatching2GetRoomMemberDataInternalResponse
{
	vm::psv::ptr<SceNpMatching2RoomMemberDataInternal> roomMemberDataInternal;
};


struct SceNpMatching2SetSignalingOptParamRequest
{
	SceNpMatching2RoomId roomId;
	SceNpMatching2SignalingOptParam sigOptParam;
};


struct SceNpMatching2GetLobbyInfoListRequest
{
	SceNpMatching2WorldId worldId;
	SceNpMatching2RangeFilter rangeFilter;
	vm::psv::ptr<SceNpMatching2AttributeId> attrId;
	u32 attrIdNum;
};



struct SceNpMatching2GetLobbyInfoListResponse
{
	SceNpMatching2Range range;
	vm::psv::ptr<SceNpMatching2LobbyDataExternal> lobbyDataExternal;
};


struct SceNpMatching2JoinLobbyRequest
{
	SceNpMatching2LobbyId lobbyId;
	vm::psv::ptr<SceNpMatching2JoinedSessionInfo> joinedSessionInfo;
	u32 joinedSessionInfoNum;
	vm::psv::ptr<SceNpMatching2BinAttr> lobbyMemberBinAttrInternal;
	u32 lobbyMemberBinAttrInternalNum;
	SceNpMatching2PresenceOptionData optData;
	u8 padding[4];
};



struct SceNpMatching2JoinLobbyResponse
{
	vm::psv::ptr<SceNpMatching2LobbyDataInternal> lobbyDataInternal;
};


struct SceNpMatching2LeaveLobbyRequest
{
	SceNpMatching2LobbyId lobbyId;
	SceNpMatching2PresenceOptionData optData;
	u8 padding[4];
};


struct SceNpMatching2SetLobbyMemberDataInternalRequest
{
	SceNpMatching2LobbyId lobbyId;
	SceNpMatching2LobbyMemberId memberId;
	u8 padding1[2];
	SceNpMatching2FlagAttr flagFilter;
	SceNpMatching2FlagAttr flagAttr;
	vm::psv::ptr<SceNpMatching2JoinedSessionInfo> joinedSessionInfo;
	u32 joinedSessionInfoNum;
	vm::psv::ptr<SceNpMatching2BinAttr> lobbyMemberBinAttrInternal;
	u32 lobbyMemberBinAttrInternalNum;
	u8 padding2[4];
};


struct SceNpMatching2GetLobbyMemberDataInternalRequest
{
	SceNpMatching2LobbyId lobbyId;
	SceNpMatching2LobbyMemberId memberId;
	u8 padding[6];
	vm::psv::ptr<const SceNpMatching2AttributeId> attrId;
	u32 attrIdNum;
};



struct SceNpMatching2GetLobbyMemberDataInternalResponse
{
	vm::psv::ptr<SceNpMatching2LobbyMemberDataInternal> lobbyMemberDataInternal;
};



struct SceNpMatching2GetLobbyMemberDataInternalListRequest
{
	SceNpMatching2LobbyId lobbyId;
	vm::psv::ptr<SceNpMatching2LobbyMemberId> memberId;
	u32 memberIdNum;
	vm::psv::ptr<const SceNpMatching2AttributeId> attrId;
	u32 attrIdNum;
	bool extendedData;
	u8 padding[7];
};



struct SceNpMatching2GetLobbyMemberDataInternalListResponse
{
	vm::psv::ptr<SceNpMatching2LobbyMemberDataInternal> lobbyMemberDataInternal;
	u32 lobbyMemberDataInternalNum;
};


struct SceNpMatching2SendLobbyChatMessageRequest
{
	SceNpMatching2LobbyId lobbyId;
	SceNpMatching2CastType castType;
	u8 padding[3];
	SceNpMatching2LobbyMessageDestination dst;
	vm::psv::ptr<const void> msg;
	u32 msgLen;
	s32 option;
};



struct SceNpMatching2SendLobbyChatMessageResponse
{
	bool filtered;
};


struct SceNpMatching2SendLobbyInvitationRequest
{
	SceNpMatching2LobbyId lobbyId;
	SceNpMatching2CastType castType;
	u8 padding[3];
	SceNpMatching2LobbyMessageDestination dst;
	SceNpMatching2InvitationData invitationData;
	s32 option;
};


struct SceNpMatching2RoomMemberUpdateInfo
{
	vm::psv::ptr<SceNpMatching2RoomMemberDataInternal> roomMemberDataInternal;
	SceNpMatching2EventCause eventCause;
	u8 padding[3];
	SceNpMatching2PresenceOptionData optData;
};


struct SceNpMatching2RoomOwnerUpdateInfo
{
	SceNpMatching2RoomMemberId prevOwner;
	SceNpMatching2RoomMemberId newOwner;
	SceNpMatching2EventCause eventCause;
	u8 padding[3];
	vm::psv::ptr<SceNpMatching2SessionPassword> roomPassword;
	SceNpMatching2PresenceOptionData optData;
};


struct SceNpMatching2RoomUpdateInfo
{
	SceNpMatching2EventCause eventCause;
	u8 padding[3];
	s32 errorCode;
	SceNpMatching2PresenceOptionData optData;
};


struct SceNpMatching2RoomDataInternalUpdateInfo
{
	vm::psv::ptr<SceNpMatching2RoomDataInternal> newRoomDataInternal;
	vm::psv::ptr<SceNpMatching2FlagAttr> newFlagAttr;
	vm::psv::ptr<SceNpMatching2FlagAttr> prevFlagAttr;
	vm::psv::ptr<SceNpMatching2RoomPasswordSlotMask> newRoomPasswordSlotMask;
	vm::psv::ptr<SceNpMatching2RoomPasswordSlotMask> prevRoomPasswordSlotMask;
	vm::psv::ptr<SceNpMatching2RoomGroup> *newRoomGroup;
	u32 newRoomGroupNum;
	vm::psv::ptr<SceNpMatching2RoomBinAttrInternal> *newRoomBinAttrInternal;
	u32 newRoomBinAttrInternalNum;
};


struct SceNpMatching2RoomMemberDataInternalUpdateInfo
{
	vm::psv::ptr<SceNpMatching2RoomMemberDataInternal> newRoomMemberDataInternal;
	vm::psv::ptr<SceNpMatching2FlagAttr> newFlagAttr;
	vm::psv::ptr<SceNpMatching2FlagAttr> prevFlagAttr;
	vm::psv::ptr<SceNpMatching2TeamId> newTeamId;
	vm::psv::ptr<SceNpMatching2RoomMemberBinAttrInternal> *newRoomMemberBinAttrInternal;
	u32 newRoomMemberBinAttrInternalNum;
};


struct SceNpMatching2SignalingOptParamUpdateInfo
{
	SceNpMatching2SignalingOptParam newSignalingOptParam;
};


struct SceNpMatching2RoomMessageInfo
{
	bool filtered;
	SceNpMatching2CastType castType;
	u8 padding[2];
	vm::psv::ptr<SceNpMatching2RoomMessageDestination> dst;
	vm::psv::ptr<SceNpId> srcMember;
	vm::psv::ptr<const void> msg;
	u32 msgLen;
};


struct SceNpMatching2LobbyMemberUpdateInfo
{
	vm::psv::ptr<SceNpMatching2LobbyMemberDataInternal> lobbyMemberDataInternal;
	SceNpMatching2EventCause eventCause;
	u8 padding[3];
	SceNpMatching2PresenceOptionData optData;
};


struct SceNpMatching2LobbyUpdateInfo
{
	SceNpMatching2EventCause eventCause;
	u8 padding[3];
	s32 errorCode;
};


struct SceNpMatching2LobbyMemberDataInternalUpdateInfo
{
	SceNpMatching2LobbyMemberId memberId;
	u8 padding[2];
	SceNpId npId;
	SceNpMatching2FlagAttr flagFilter;
	SceNpMatching2FlagAttr newFlagAttr;
	vm::psv::ptr<SceNpMatching2JoinedSessionInfo> newJoinedSessionInfo;
	u32 newJoinedSessionInfoNum;
	vm::psv::ptr<SceNpMatching2LobbyMemberBinAttrInternal> newLobbyMemberBinAttrInternal;
	u32 newLobbyMemberBinAttrInternalNum;
};


struct SceNpMatching2LobbyMessageInfo
{
	bool filtered;
	SceNpMatching2CastType castType;
	u8 padding[2];
	vm::psv::ptr<SceNpMatching2LobbyMessageDestination> dst;
	vm::psv::ptr<SceNpId> srcMember;
	vm::psv::ptr<const void> msg;
	u32 msgLen;
};


struct SceNpMatching2LobbyInvitationInfo
{
	SceNpMatching2CastType castType;
	u8 padding[3];
	vm::psv::ptr<SceNpMatching2LobbyMessageDestination> dst;
	vm::psv::ptr<SceNpId> srcMember;
	SceNpMatching2InvitationData invitationData;
};

union SceNpMatching2SignalingConnectionInfo
{
	u32 rtt;
	u32 bandwidth;
	SceNpId npId;
	struct
	{
		SceNetInAddr addr;
		u16 port;
		u8 padding[2];

	} address;
	u32 packetLoss;
};

struct SceNpMatching2SignalingNetInfo
{
	u32 size;
	SceNetInAddr localAddr;
	SceNetInAddr mappedAddr;
	s32 natStatus;
};

// Functions

s32 sceNpMatching2Init(
	const u32 poolSize,
	const s32 threadPriority,
	const s32 cpuAffinityMask,
	const u32 threadStackSize)
{
	throw __FUNCTION__;
}

s32 sceNpMatching2Term()
{
	throw __FUNCTION__;
}

s32 sceNpMatching2CreateContext(
	vm::psv::ptr<const SceNpId> npId,
	vm::psv::ptr<const SceNpCommunicationId> commId,
	vm::psv::ptr<const SceNpCommunicationPassphrase> passPhrase,
	vm::psv::ptr<SceNpMatching2ContextId> ctxId)
{
	throw __FUNCTION__;
}

s32 sceNpMatching2DestroyContext(const SceNpMatching2ContextId ctxId)
{
	throw __FUNCTION__;
}

s32 sceNpMatching2ContextStart(const SceNpMatching2ContextId ctxId, const u64 timeout)
{
	throw __FUNCTION__;
}

s32 sceNpMatching2AbortContextStart(const SceNpMatching2ContextId ctxId)
{
	throw __FUNCTION__;
}

s32 sceNpMatching2ContextStop(const SceNpMatching2ContextId ctxId)
{
	throw __FUNCTION__;
}

s32 sceNpMatching2SetDefaultRequestOptParam(const SceNpMatching2ContextId ctxId, vm::psv::ptr<const SceNpMatching2RequestOptParam> optParam)
{
	throw __FUNCTION__;
}

s32 sceNpMatching2RegisterRoomEventCallback(const SceNpMatching2ContextId ctxId, SceNpMatching2RoomEventCallback cbFunc, vm::psv::ptr<void> cbFuncArg)
{
	throw __FUNCTION__;
}

s32 sceNpMatching2RegisterRoomMessageCallback(const SceNpMatching2ContextId ctxId, SceNpMatching2RoomMessageCallback cbFunc, vm::psv::ptr<void> cbFuncArg)
{
	throw __FUNCTION__;
}

s32 sceNpMatching2RegisterSignalingCallback(const SceNpMatching2ContextId ctxId, SceNpMatching2SignalingCallback cbFunc, vm::psv::ptr<void> cbFuncArg)
{
	throw __FUNCTION__;
}

s32 sceNpMatching2RegisterContextCallback(SceNpMatching2ContextCallback cbFunc, vm::psv::ptr<void> cbFuncArg)
{
	throw __FUNCTION__;
}

s32 sceNpMatching2AbortRequest(const SceNpMatching2ContextId ctxId, const SceNpMatching2RequestId reqId)
{
	throw __FUNCTION__;
}

s32 sceNpMatching2GetMemoryInfo(vm::psv::ptr<SceNpMatching2MemoryInfo> memInfo)
{
	throw __FUNCTION__;
}

s32 sceNpMatching2GetServerLocal(const SceNpMatching2ContextId ctxId, vm::psv::ptr<SceNpMatching2Server> server)
{
	throw __FUNCTION__;
}

s32 sceNpMatching2GetWorldInfoList(
	const SceNpMatching2ContextId ctxId,
	vm::psv::ptr<const SceNpMatching2GetWorldInfoListRequest> reqParam,
	vm::psv::ptr<const SceNpMatching2RequestOptParam> optParam,
	vm::psv::ptr<SceNpMatching2RequestId> assignedReqId)
{
	throw __FUNCTION__;
}

s32 sceNpMatching2CreateJoinRoom(
	const SceNpMatching2ContextId ctxId,
	vm::psv::ptr<const SceNpMatching2CreateJoinRoomRequest> reqParam,
	vm::psv::ptr<const SceNpMatching2RequestOptParam> optParam,
	vm::psv::ptr<SceNpMatching2RequestId> assignedReqId)
{
	throw __FUNCTION__;
}

s32 sceNpMatching2SearchRoom(
	const SceNpMatching2ContextId ctxId,
	vm::psv::ptr<const SceNpMatching2SearchRoomRequest> reqParam,
	vm::psv::ptr<const SceNpMatching2RequestOptParam> optParam,
	vm::psv::ptr<SceNpMatching2RequestId> assignedReqId)
{
	throw __FUNCTION__;
}

s32 sceNpMatching2JoinRoom(
	const SceNpMatching2ContextId ctxId,
	vm::psv::ptr<const SceNpMatching2JoinRoomRequest> reqParam,
	vm::psv::ptr<const SceNpMatching2RequestOptParam> optParam,
	vm::psv::ptr<SceNpMatching2RequestId> assignedReqId)
{
	throw __FUNCTION__;
}

s32 sceNpMatching2LeaveRoom(
	const SceNpMatching2ContextId ctxId,
	vm::psv::ptr<const SceNpMatching2LeaveRoomRequest> reqParam,
	vm::psv::ptr<const SceNpMatching2RequestOptParam> optParam,
	vm::psv::ptr<SceNpMatching2RequestId> assignedReqId)
{
	throw __FUNCTION__;
}

s32 sceNpMatching2GetSignalingOptParamLocal(
	const SceNpMatching2ContextId ctxId,
	const SceNpMatching2RoomId roomId,
	vm::psv::ptr<SceNpMatching2SignalingOptParam> signalingOptParam)
{
	throw __FUNCTION__;
}

s32 sceNpMatching2SetRoomDataExternal(
	const SceNpMatching2ContextId ctxId,
	vm::psv::ptr<const SceNpMatching2SetRoomDataExternalRequest> reqParam,
	vm::psv::ptr<const SceNpMatching2RequestOptParam> optParam,
	vm::psv::ptr<SceNpMatching2RequestId> assignedReqId)
{
	throw __FUNCTION__;
}

s32 sceNpMatching2KickoutRoomMember(
	const SceNpMatching2ContextId ctxId,
	vm::psv::ptr<const SceNpMatching2KickoutRoomMemberRequest> reqParam,
	vm::psv::ptr<const SceNpMatching2RequestOptParam> optParam,
	vm::psv::ptr<SceNpMatching2RequestId> assignedReqId)
{
	throw __FUNCTION__;
}

s32 sceNpMatching2SendRoomChatMessage(
	const SceNpMatching2ContextId ctxId,
	vm::psv::ptr<const SceNpMatching2SendRoomChatMessageRequest> reqParam,
	vm::psv::ptr<const SceNpMatching2RequestOptParam> optParam,
	vm::psv::ptr<SceNpMatching2RequestId> assignedReqId)
{
	throw __FUNCTION__;
}

s32 sceNpMatching2SendRoomMessage(
	const SceNpMatching2ContextId ctxId,
	vm::psv::ptr<const SceNpMatching2SendRoomMessageRequest> reqParam,
	vm::psv::ptr<const SceNpMatching2RequestOptParam> optParam,
	vm::psv::ptr<SceNpMatching2RequestId> assignedReqId)
{
	throw __FUNCTION__;
}

s32 sceNpMatching2SignalingGetConnectionStatus(
	SceNpMatching2ContextId ctxId,
	SceNpMatching2RoomId roomId,
	SceNpMatching2RoomMemberId memberId,
	vm::psv::ptr<s32> connStatus,
	vm::psv::ptr<SceNetInAddr> peerAddr,
	vm::psv::ptr<u16> peerPort)
{
	throw __FUNCTION__;
}

s32 sceNpMatching2SignalingGetConnectionInfo(
	SceNpMatching2ContextId ctxId,
	SceNpMatching2RoomId roomId,
	SceNpMatching2RoomMemberId memberId,
	s32 code,
	vm::psv::ptr<SceNpMatching2SignalingConnectionInfo> info)
{
	throw __FUNCTION__;
}

s32 sceNpMatching2SignalingGetLocalNetInfo(vm::psv::ptr<SceNpMatching2SignalingNetInfo> netinfo)
{
	throw __FUNCTION__;
}

s32 sceNpMatching2SignalingGetPeerNetInfo(
	SceNpMatching2ContextId ctxId,
	SceNpMatching2RoomId roomId,
	SceNpMatching2RoomMemberId memberId,
	vm::psv::ptr<SceNpMatching2SignalingRequestId> reqId)
{
	throw __FUNCTION__;
}

s32 sceNpMatching2SignalingCancelPeerNetInfo(
	SceNpMatching2ContextId ctxId,
	SceNpMatching2SignalingRequestId reqId)
{
	throw __FUNCTION__;
}

s32 sceNpMatching2SignalingGetPeerNetInfoResult(
	SceNpMatching2ContextId ctxId,
	SceNpMatching2SignalingRequestId reqId,
	vm::psv::ptr<SceNpMatching2SignalingNetInfo> netinfo)
{
	throw __FUNCTION__;
}

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceNpMatching, #name, name)

psv_log_base sceNpMatching("SceNpMatching2", []()
{
	sceNpMatching.on_load = nullptr;
	sceNpMatching.on_unload = nullptr;
	sceNpMatching.on_stop = nullptr;

	REG_FUNC(0xEBB1FE74, sceNpMatching2Init);
	REG_FUNC(0x0124641C, sceNpMatching2Term);
	REG_FUNC(0xADF578E1, sceNpMatching2CreateContext);
	REG_FUNC(0x368AA759, sceNpMatching2DestroyContext);
	REG_FUNC(0xBB2E7559, sceNpMatching2ContextStart);
	REG_FUNC(0xF2847E3B, sceNpMatching2AbortContextStart);
	REG_FUNC(0x506454DE, sceNpMatching2ContextStop);
	REG_FUNC(0xF3A43C50, sceNpMatching2SetDefaultRequestOptParam);
	REG_FUNC(0xF486991B, sceNpMatching2RegisterRoomEventCallback);
	REG_FUNC(0xFA51949B, sceNpMatching2RegisterRoomMessageCallback);
	REG_FUNC(0xF9E35566, sceNpMatching2RegisterContextCallback);
	REG_FUNC(0x74EB6CE9, sceNpMatching2AbortRequest);
	REG_FUNC(0x7BD39E50, sceNpMatching2GetMemoryInfo);
	REG_FUNC(0x65C0FEED, sceNpMatching2GetServerLocal);
	REG_FUNC(0xC086B560, sceNpMatching2GetWorldInfoList);
	REG_FUNC(0x818A9499, sceNpMatching2CreateJoinRoom);
	REG_FUNC(0xD48BAF13, sceNpMatching2SearchRoom);
	REG_FUNC(0x33F7D5AE, sceNpMatching2JoinRoom);
	REG_FUNC(0xC8B0C9EE, sceNpMatching2LeaveRoom);
	REG_FUNC(0x495D2B46, sceNpMatching2GetSignalingOptParamLocal);
	REG_FUNC(0xE0BE0510, sceNpMatching2SendRoomChatMessage);
	REG_FUNC(0x7B908D99, sceNpMatching2SendRoomMessage);
	REG_FUNC(0x4E4C55BD, sceNpMatching2SignalingGetConnectionStatus);
	REG_FUNC(0x20598618, sceNpMatching2SignalingGetConnectionInfo);
	REG_FUNC(0x79310806, sceNpMatching2SignalingGetLocalNetInfo);
	REG_FUNC(0xF0CB1DD3, sceNpMatching2SignalingGetPeerNetInfo);
	REG_FUNC(0xADCD102C, sceNpMatching2SignalingCancelPeerNetInfo);
	REG_FUNC(0xFDC7B2C9, sceNpMatching2SignalingGetPeerNetInfoResult);
	REG_FUNC(0x1C60BC5B, sceNpMatching2RegisterSignalingCallback);
	REG_FUNC(0x8F88AC7E, sceNpMatching2SetRoomDataExternal);
	REG_FUNC(0xA8021394, sceNpMatching2KickoutRoomMember);
});
