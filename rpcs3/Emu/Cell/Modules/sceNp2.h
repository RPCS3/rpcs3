#pragma once

#include "sceNp.h"

#include "Emu/Memory/vm_ptr.h"

// Error codes
enum SceNpMatching2Error : u32
{
	// NP Matching 2 Utility
	SCE_NP_MATCHING2_ERROR_OUT_OF_MEMORY               = 0x80022301,
	SCE_NP_MATCHING2_ERROR_ALREADY_INITIALIZED         = 0x80022302,
	SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED             = 0x80022303,
	SCE_NP_MATCHING2_ERROR_CONTEXT_MAX                 = 0x80022304,
	SCE_NP_MATCHING2_ERROR_CONTEXT_ALREADY_EXISTS      = 0x80022305,
	SCE_NP_MATCHING2_ERROR_CONTEXT_NOT_FOUND           = 0x80022306,
	SCE_NP_MATCHING2_ERROR_CONTEXT_ALREADY_STARTED     = 0x80022307,
	SCE_NP_MATCHING2_ERROR_CONTEXT_NOT_STARTED         = 0x80022308,
	SCE_NP_MATCHING2_ERROR_SERVER_NOT_FOUND            = 0x80022309,
	SCE_NP_MATCHING2_ERROR_INVALID_ARGUMENT            = 0x8002230a,
	SCE_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID          = 0x8002230b,
	SCE_NP_MATCHING2_ERROR_INVALID_SERVER_ID           = 0x8002230c,
	SCE_NP_MATCHING2_ERROR_INVALID_WORLD_ID            = 0x8002230d,
	SCE_NP_MATCHING2_ERROR_INVALID_LOBBY_ID            = 0x8002230e,
	SCE_NP_MATCHING2_ERROR_INVALID_ROOM_ID             = 0x8002230f,
	SCE_NP_MATCHING2_ERROR_INVALID_MEMBER_ID           = 0x80022310,
	SCE_NP_MATCHING2_ERROR_INVALID_ATTRIBUTE_ID        = 0x80022311,
	SCE_NP_MATCHING2_ERROR_INVALID_CASTTYPE            = 0x80022312,
	SCE_NP_MATCHING2_ERROR_INVALID_SORT_METHOD         = 0x80022313,
	SCE_NP_MATCHING2_ERROR_INVALID_MAX_SLOT            = 0x80022314,
	SCE_NP_MATCHING2_ERROR_INVALID_MATCHING_SPACE      = 0x80022316,
	SCE_NP_MATCHING2_ERROR_INVALID_BLOCK_KICK_FLAG     = 0x80022318,
	SCE_NP_MATCHING2_ERROR_INVALID_MESSAGE_TARGET      = 0x80022319,
	SCE_NP_MATCHING2_ERROR_RANGE_FILTER_MAX            = 0x8002231a,
	SCE_NP_MATCHING2_ERROR_INSUFFICIENT_BUFFER         = 0x8002231b,
	SCE_NP_MATCHING2_ERROR_DESTINATION_DISAPPEARED     = 0x8002231c,
	SCE_NP_MATCHING2_ERROR_REQUEST_TIMEOUT             = 0x8002231d,
	SCE_NP_MATCHING2_ERROR_INVALID_ALIGNMENT           = 0x8002231e,
	SCE_NP_MATCHING2_ERROR_REQUEST_CB_QUEUE_OVERFLOW   = 0x8002231f,
	SCE_NP_MATCHING2_ERROR_EVENT_CB_QUEUE_OVERFLOW     = 0x80022320,
	SCE_NP_MATCHING2_ERROR_MSG_CB_QUEUE_OVERFLOW       = 0x80022321,
	SCE_NP_MATCHING2_ERROR_CONNECTION_CLOSED_BY_SERVER = 0x80022322,
	SCE_NP_MATCHING2_ERROR_SSL_VERIFY_FAILED           = 0x80022323,
	SCE_NP_MATCHING2_ERROR_SSL_HANDSHAKE               = 0x80022324,
	SCE_NP_MATCHING2_ERROR_SSL_SEND                    = 0x80022325,
	SCE_NP_MATCHING2_ERROR_SSL_RECV                    = 0x80022326,
	SCE_NP_MATCHING2_ERROR_JOINED_SESSION_MAX          = 0x80022327,
	SCE_NP_MATCHING2_ERROR_ALREADY_JOINED              = 0x80022328,
	SCE_NP_MATCHING2_ERROR_INVALID_SESSION_TYPE        = 0x80022329,
	SCE_NP_MATCHING2_ERROR_CLAN_LOBBY_NOT_EXIST        = 0x8002232a,
	SCE_NP_MATCHING2_ERROR_NP_SIGNED_OUT               = 0x8002232b,
	SCE_NP_MATCHING2_ERROR_CONTEXT_UNAVAILABLE         = 0x8002232c,
	SCE_NP_MATCHING2_ERROR_SERVER_NOT_AVAILABLE        = 0x8002232d,
	SCE_NP_MATCHING2_ERROR_NOT_ALLOWED                 = 0x8002232e,
	SCE_NP_MATCHING2_ERROR_ABORTED                     = 0x8002232f,
	SCE_NP_MATCHING2_ERROR_REQUEST_NOT_FOUND           = 0x80022330,
	SCE_NP_MATCHING2_ERROR_SESSION_DESTROYED           = 0x80022331,
	SCE_NP_MATCHING2_ERROR_CONTEXT_STOPPED             = 0x80022332,
	SCE_NP_MATCHING2_ERROR_INVALID_REQUEST_PARAMETER   = 0x80022333,
	SCE_NP_MATCHING2_ERROR_NOT_NP_SIGN_IN              = 0x80022334,
	SCE_NP_MATCHING2_ERROR_ROOM_NOT_FOUND              = 0x80022335,
	SCE_NP_MATCHING2_ERROR_ROOM_MEMBER_NOT_FOUND       = 0x80022336,
	SCE_NP_MATCHING2_ERROR_LOBBY_NOT_FOUND             = 0x80022337,
	SCE_NP_MATCHING2_ERROR_LOBBY_MEMBER_NOT_FOUND      = 0x80022338,
	SCE_NP_MATCHING2_ERROR_EVENT_DATA_NOT_FOUND        = 0x80022339,
	SCE_NP_MATCHING2_ERROR_KEEPALIVE_TIMEOUT           = 0x8002233a,
	SCE_NP_MATCHING2_ERROR_TIMEOUT_TOO_SHORT           = 0x8002233b,
	SCE_NP_MATCHING2_ERROR_TIMEDOUT                    = 0x8002233c,
	SCE_NP_MATCHING2_ERROR_CREATE_HEAP                 = 0x8002233d,
	SCE_NP_MATCHING2_ERROR_INVALID_ATTRIBUTE_SIZE      = 0x8002233e,
	SCE_NP_MATCHING2_ERROR_CANNOT_ABORT                = 0x8002233f,

	SCE_NP_MATCHING2_RESOLVER_ERROR_NO_DNS_SERVER       = 0x800223a2,
	SCE_NP_MATCHING2_RESOLVER_ERROR_INVALID_PACKET      = 0x800223ad,
	SCE_NP_MATCHING2_RESOLVER_ERROR_TIMEOUT             = 0x800223b0,
	SCE_NP_MATCHING2_RESOLVER_ERROR_NO_RECORD           = 0x800223b1,
	SCE_NP_MATCHING2_RESOLVER_ERROR_RES_PACKET_FORMAT   = 0x800223b2,
	SCE_NP_MATCHING2_RESOLVER_ERROR_RES_SERVER_FAILURE  = 0x800223b3,
	SCE_NP_MATCHING2_RESOLVER_ERROR_NO_HOST             = 0x800223b4,
	SCE_NP_MATCHING2_RESOLVER_ERROR_RES_NOT_IMPLEMENTED = 0x800223b5,
	SCE_NP_MATCHING2_RESOLVER_ERROR_RES_SERVER_REFUSED  = 0x800223b6,
	SCE_NP_MATCHING2_RESOLVER_ERROR_RESP_TRUNCATED      = 0x800223bc,

	SCE_NP_MATCHING2_SERVER_ERROR_BAD_REQUEST               = 0x80022b01,
	SCE_NP_MATCHING2_SERVER_ERROR_SERVICE_UNAVAILABLE       = 0x80022b02,
	SCE_NP_MATCHING2_SERVER_ERROR_BUSY                      = 0x80022b03,
	SCE_NP_MATCHING2_SERVER_ERROR_END_OF_SERVICE            = 0x80022b04,
	SCE_NP_MATCHING2_SERVER_ERROR_INTERNAL_SERVER_ERROR     = 0x80022b05,
	SCE_NP_MATCHING2_SERVER_ERROR_PLAYER_BANNED             = 0x80022b06,
	SCE_NP_MATCHING2_SERVER_ERROR_FORBIDDEN                 = 0x80022b07,
	SCE_NP_MATCHING2_SERVER_ERROR_BLOCKED                   = 0x80022b08,
	SCE_NP_MATCHING2_SERVER_ERROR_UNSUPPORTED_NP_ENV        = 0x80022b09,
	SCE_NP_MATCHING2_SERVER_ERROR_INVALID_TICKET            = 0x80022b0a,
	SCE_NP_MATCHING2_SERVER_ERROR_INVALID_SIGNATURE         = 0x80022b0b,
	SCE_NP_MATCHING2_SERVER_ERROR_EXPIRED_TICKET            = 0x80022b0c,
	SCE_NP_MATCHING2_SERVER_ERROR_ENTITLEMENT_REQUIRED      = 0x80022b0d,
	SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_CONTEXT           = 0x80022b0e,
	SCE_NP_MATCHING2_SERVER_ERROR_CLOSED                    = 0x80022b0f,
	SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_TITLE             = 0x80022b10,
	SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_WORLD             = 0x80022b11,
	SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_LOBBY             = 0x80022b12,
	SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_ROOM              = 0x80022b13,
	SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_LOBBY_INSTANCE    = 0x80022b14,
	SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_ROOM_INSTANCE     = 0x80022b15,
	SCE_NP_MATCHING2_SERVER_ERROR_PASSWORD_MISMATCH         = 0x80022b17,
	SCE_NP_MATCHING2_SERVER_ERROR_LOBBY_FULL                = 0x80022b18,
	SCE_NP_MATCHING2_SERVER_ERROR_ROOM_FULL                 = 0x80022b19,
	SCE_NP_MATCHING2_SERVER_ERROR_GROUP_FULL                = 0x80022b1b,
	SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_USER              = 0x80022b1c,
	SCE_NP_MATCHING2_SERVER_ERROR_TITLE_PASSPHRASE_MISMATCH = 0x80022b1e,
	SCE_NP_MATCHING2_SERVER_ERROR_DUPLICATE_LOBBY           = 0x80022b25,
	SCE_NP_MATCHING2_SERVER_ERROR_DUPLICATE_ROOM            = 0x80022b26,
	SCE_NP_MATCHING2_SERVER_ERROR_NO_JOIN_GROUP_LABEL       = 0x80022b29,
	SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_GROUP             = 0x80022b2a,
	SCE_NP_MATCHING2_SERVER_ERROR_NO_PASSWORD               = 0x80022b2b,
	SCE_NP_MATCHING2_SERVER_ERROR_MAX_OVER_SLOT_GROUP       = 0x80022b2c,
	SCE_NP_MATCHING2_SERVER_ERROR_MAX_OVER_PASSWORD_MASK    = 0x80022b2d,
	SCE_NP_MATCHING2_SERVER_ERROR_DUPLICATE_GROUP_LABEL     = 0x80022b2e,
	SCE_NP_MATCHING2_SERVER_ERROR_REQUEST_OVERFLOW          = 0x80022b2f,
	SCE_NP_MATCHING2_SERVER_ERROR_ALREADY_JOINED            = 0x80022b30,
	SCE_NP_MATCHING2_SERVER_ERROR_NAT_TYPE_MISMATCH         = 0x80022b31,
	SCE_NP_MATCHING2_SERVER_ERROR_ROOM_INCONSISTENCY        = 0x80022b32,
	// SCE_NP_MATCHING2_NET_ERRNO_BASE   = 0x800224XX,
	// SCE_NP_MATCHING2_NET_H_ERRNO_BASE = 0x800225XX,
};

// Constants for matching functions and structures
enum
{
	SCE_NP_MATCHING2_ALLOWED_USER_MAX                                      = 100,
	SCE_NP_MATCHING2_BLOCKED_USER_MAX                                      = 100,
	SCE_NP_MATCHING2_CHAT_MSG_MAX_SIZE                                     = 1024,
	SCE_NP_MATCHING2_BIN_MSG_MAX_SIZE                                      = 1024,
	SCE_NP_MATCHING2_GROUP_LABEL_SIZE                                      = 8,
	SCE_NP_MATCHING2_INVITATION_OPTION_DATA_MAX_SIZE                       = 32,
	SCE_NP_MATCHING2_INVITATION_TARGET_SESSION_MAX                         = 2,
	SCE_NP_MATCHING2_LOBBY_MEMBER_DATA_INTERNAL_LIST_MAX                   = 256,
	SCE_NP_MATCHING2_LOBBY_MEMBER_DATA_INTERNAL_EXTENDED_DATA_LIST_MAX     = 64,
	SCE_NP_MATCHING2_LOBBY_BIN_ATTR_INTERNAL_NUM                           = 2,
	SCE_NP_MATCHING2_LOBBYMEMBER_BIN_ATTR_INTERNAL_NUM                     = 1,
	SCE_NP_MATCHING2_LOBBYMEMBER_BIN_ATTR_INTERNAL_MAX_SIZE                = 64,
	SCE_NP_MATCHING2_LOBBY_MAX_SLOT                                        = 256,
	SCE_NP_MATCHING2_PRESENCE_OPTION_DATA_SIZE                             = 16,
	SCE_NP_MATCHING2_RANGE_FILTER_START_INDEX_MIN                          = 1,
	SCE_NP_MATCHING2_RANGE_FILTER_MAX                                      = 20,
	SCE_NP_MATCHING2_ROOM_MAX_SLOT                                         = 64,
	SCE_NP_MATCHING2_ROOM_GROUP_ID_MAX                                     = 15,
	SCE_NP_MATCHING2_ROOM_BIN_ATTR_EXTERNAL_NUM                            = 2,
	SCE_NP_MATCHING2_ROOM_BIN_ATTR_EXTERNAL_MAX_SIZE                       = 256,
	SCE_NP_MATCHING2_ROOM_BIN_ATTR_INTERNAL_NUM                            = 2,
	SCE_NP_MATCHING2_ROOM_BIN_ATTR_INTERNAL_MAX_SIZE                       = 256,
	SCE_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_NUM                 = 8,
	SCE_NP_MATCHING2_ROOM_SEARCHABLE_BIN_ATTR_EXTERNAL_NUM                 = 1,
	SCE_NP_MATCHING2_ROOM_SEARCHABLE_BIN_ATTR_EXTERNAL_MAX_SIZE            = 64,
	SCE_NP_MATCHING2_ROOMMEMBER_BIN_ATTR_INTERNAL_NUM                      = 1,
	SCE_NP_MATCHING2_ROOMMEMBER_BIN_ATTR_INTERNAL_MAX_SIZE                 = 64,
	SCE_NP_MATCHING2_SESSION_PASSWORD_SIZE                                 = 8,
	SCE_NP_MATCHING2_USER_BIN_ATTR_NUM                                     = 1,
	SCE_NP_MATCHING2_USER_BIN_ATTR_MAX_SIZE                                = 128,
	SCE_NP_MATCHING2_GET_USER_INFO_LIST_NPID_NUM_MAX                       = 25,
	SCE_NP_MATCHING2_GET_ROOM_DATA_EXTERNAL_LIST_ROOM_NUM_MAX              = 20,
	SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_GetServerInfo                     = 4,
	SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_GetWorldInfoList                  = 3848,
	SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_GetRoomMemberDataExternalList     = 15624,
	SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_GetRoomDataExternalList           = 25768,
	SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_GetLobbyInfoList                  = 1296,
	SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_GetUserInfoList                   = 17604,
	SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_CreateJoinRoom                    = 25224,
	SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_JoinRoom                          = 25224,
	SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_SearchRoom                        = 25776,
	SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_SendRoomChatMessage               = 1,
	SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_GetRoomDataInternal               = 25224,
	SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_GetRoomMemberDataInternal         = 372,
	SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_JoinLobby                         = 1124,
	SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_SendLobbyChatMessage              = 1,
	SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_GetLobbyMemberDataInternal        = 672,
	SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_GetLobbyMemberDataInternalList    = 42760,
	SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_SignalingGetPingInfo              = 40,
	SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_RoomMemberUpdateInfo              = 396,
	SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_RoomUpdateInfo                    = 28,
	SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_RoomOwnerUpdateInfo               = 40,
	SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_RoomDataInternalUpdateInfo        = 26208,
	SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_RoomMemberDataInternalUpdateInfo  = 493,
	SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_SignalingOptParamUpdateInfo       = 8,
	SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_RoomMessageInfo                   = 1407,
	SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_LobbyMemberUpdateInfo             = 696,
	SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_LobbyUpdateInfo                   = 8,
	SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_LobbyMemberDataInternalUpdateInfo = 472,
	SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_LobbyMessageInfo                  = 1790,
	SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_LobbyInvitationInfo               = 870,
};

enum
{
	SCE_NP_MATCHING2_MAX_SIZE_RoomDataExternal                       = 1288,
	SCE_NP_MATCHING2_MAX_SIZE_RoomMemberDataInternal                 = 368,
	SCE_NP_MATCHING2_MAX_SIZE_LobbyMemberDataInternal                = 668,
	SCE_NP_MATCHING2_MAX_SIZE_LobbyMemberDataInternal_NoExtendedData = 80,
	SCE_NP_MATCHING2_MAX_SIZE_UserInfo                               = 704,
};

enum
{
	SCE_NP_MATCHING2_TITLE_PASSPHRASE_SIZE = 128
};

// Comparison operator specified as the search condition
enum
{
	SCE_NP_MATCHING2_OPERATOR_EQ = 1,
	SCE_NP_MATCHING2_OPERATOR_NE = 2,
	SCE_NP_MATCHING2_OPERATOR_LT = 3,
	SCE_NP_MATCHING2_OPERATOR_LE = 4,
	SCE_NP_MATCHING2_OPERATOR_GT = 5,
	SCE_NP_MATCHING2_OPERATOR_GE = 6,
};

// Message cast type
enum
{
	SCE_NP_MATCHING2_CASTTYPE_BROADCAST      = 1,
	SCE_NP_MATCHING2_CASTTYPE_UNICAST        = 2,
	SCE_NP_MATCHING2_CASTTYPE_MULTICAST      = 3,
	SCE_NP_MATCHING2_CASTTYPE_MULTICAST_TEAM = 4,
};

// Session type
enum
{
	SCE_NP_MATCHING2_SESSION_TYPE_LOBBY = 1,
	SCE_NP_MATCHING2_SESSION_TYPE_ROOM  = 2,
};

// Signaling type
enum
{
	SCE_NP_MATCHING2_SIGNALING_TYPE_NONE = 0,
	SCE_NP_MATCHING2_SIGNALING_TYPE_MESH = 1,
	SCE_NP_MATCHING2_SIGNALING_TYPE_STAR = 2,
};

enum
{
	SCE_NP_MATCHING2_SIGNALING_FLAG_MANUAL_MODE = 0x01
};

// Event cause
enum
{
	SCE_NP_MATCHING2_EVENT_CAUSE_LEAVE_ACTION       = 1,
	SCE_NP_MATCHING2_EVENT_CAUSE_KICKOUT_ACTION     = 2,
	SCE_NP_MATCHING2_EVENT_CAUSE_GRANT_OWNER_ACTION = 3,
	SCE_NP_MATCHING2_EVENT_CAUSE_SERVER_OPERATION   = 4,
	SCE_NP_MATCHING2_EVENT_CAUSE_MEMBER_DISAPPEARED = 5,
	SCE_NP_MATCHING2_EVENT_CAUSE_SERVER_INTERNAL    = 6,
	SCE_NP_MATCHING2_EVENT_CAUSE_CONNECTION_ERROR   = 7,
	SCE_NP_MATCHING2_EVENT_CAUSE_NP_SIGNED_OUT      = 8,
	SCE_NP_MATCHING2_EVENT_CAUSE_SYSTEM_ERROR       = 9,
	SCE_NP_MATCHING2_EVENT_CAUSE_CONTEXT_ERROR      = 10,
	SCE_NP_MATCHING2_EVENT_CAUSE_CONTEXT_ACTION     = 11,
};

// Server status
enum
{
	SCE_NP_MATCHING2_SERVER_STATUS_AVAILABLE   = 1,
	SCE_NP_MATCHING2_SERVER_STATUS_UNAVAILABLE = 2,
	SCE_NP_MATCHING2_SERVER_STATUS_BUSY        = 3,
	SCE_NP_MATCHING2_SERVER_STATUS_MAINTENANCE = 4,
};

// Member role
enum
{
	SCE_NP_MATCHING2_ROLE_MEMBER = 1,
	SCE_NP_MATCHING2_ROLE_OWNER  = 2,
};

// Status of kicked-out member with regards to rejoining
enum
{
	SCE_NP_MATCHING2_BLOCKKICKFLAG_OK = 0,
	SCE_NP_MATCHING2_BLOCKKICKFLAG_NG = 1,
};

// Sort method
enum
{
	SCE_NP_MATCHING2_SORT_METHOD_JOIN_DATE   = 0,
	SCE_NP_MATCHING2_SORT_METHOD_SLOT_NUMBER = 1,
};

// Context options (matching)
enum
{
	SCE_NP_MATCHING2_CONTEXT_OPTION_USE_ONLINENAME = 0x01,
	SCE_NP_MATCHING2_CONTEXT_OPTION_USE_AVATARURL  = 0x02,
};

// User information acquisition option
enum
{
	SCE_NP_MATCHING2_GET_USER_INFO_LIST_OPTION_WITH_ONLINENAME = 0x01,
	SCE_NP_MATCHING2_GET_USER_INFO_LIST_OPTION_WITH_AVATARURL  = 0x02,
};

// Room search options
enum
{
	SCE_NP_MATCHING2_SEARCH_ROOM_OPTION_WITH_NPID       = 0x01,
	SCE_NP_MATCHING2_SEARCH_ROOM_OPTION_WITH_ONLINENAME = 0x02,
	SCE_NP_MATCHING2_SEARCH_ROOM_OPTION_WITH_AVATARURL  = 0x04,
	SCE_NP_MATCHING2_SEARCH_ROOM_OPTION_NAT_TYPE_FILTER = 0x08,
	SCE_NP_MATCHING2_SEARCH_ROOM_OPTION_RANDOM          = 0x10,
};

// Send options
enum
{
	SCE_NP_MATCHING2_SEND_MSG_OPTION_WITH_NPID       = 0x01,
	SCE_NP_MATCHING2_SEND_MSG_OPTION_WITH_ONLINENAME = 0x02,
	SCE_NP_MATCHING2_SEND_MSG_OPTION_WITH_AVATARURL  = 0x04,
};

enum
{
	SCE_NP_MATCHING2_ROOM_ALLOWED_USER_MAX = 100,
	SCE_NP_MATCHING2_ROOM_BLOCKED_USER_MAX = 100,
};

// Flag-type lobby attribute
enum
{
	SCE_NP_MATCHING2_LOBBY_FLAG_ATTR_PERMANENT           = 0x80000000,
	SCE_NP_MATCHING2_LOBBY_FLAG_ATTR_CLAN                = 0x40000000,
	SCE_NP_MATCHING2_LOBBY_FLAG_ATTR_MEMBER_NOTIFICATION = 0x20000000,
};

// Attribute ID of lobby member internal binary attribute
enum
{
	SCE_NP_MATCHING2_LOBBYMEMBER_BIN_ATTR_INTERNAL_1_ID = 0x0039,
};

// Flag-type room attribute
enum
{
	SCE_NP_MATCHING2_ROOM_FLAG_ATTR_OWNER_AUTO_GRANT     = 0x80000000,
	SCE_NP_MATCHING2_ROOM_FLAG_ATTR_CLOSED               = 0x40000000,
	SCE_NP_MATCHING2_ROOM_FLAG_ATTR_FULL                 = 0x20000000,
	SCE_NP_MATCHING2_ROOM_FLAG_ATTR_HIDDEN               = 0x10000000,
	SCE_NP_MATCHING2_ROOM_FLAG_ATTR_NAT_TYPE_RESTRICTION = 0x04000000,
	SCE_NP_MATCHING2_ROOM_FLAG_ATTR_PROHIBITIVE_MODE     = 0x02000000,
};

// Flah-type room member attribute
enum
{
	SCE_NP_MATCHING2_LOBBYMEMBER_FLAG_ATTR_OWNER = 0x80000000,
	SCE_NP_MATCHING2_ROOMMEMBER_FLAG_ATTR_OWNER  = 0x80000000,
};

// ID of external room search integer attribute
enum
{
	SCE_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_1_ID = 0x004c,
	SCE_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_2_ID = 0x004d,
	SCE_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_3_ID = 0x004e,
	SCE_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_4_ID = 0x004f,
	SCE_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_5_ID = 0x0050,
	SCE_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_6_ID = 0x0051,
	SCE_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_7_ID = 0x0052,
	SCE_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_8_ID = 0x0053,
};

// ID of external room search binary attribute
enum
{
	SCE_NP_MATCHING2_ROOM_SEARCHABLE_BIN_ATTR_EXTERNAL_1_ID = 0x0054,
};

// ID of external room binary attribute
enum
{
	SCE_NP_MATCHING2_ROOM_BIN_ATTR_EXTERNAL_1_ID = 0x0055,
	SCE_NP_MATCHING2_ROOM_BIN_ATTR_EXTERNAL_2_ID = 0x0056,
};

// ID of internal lobby binary attribute
enum
{
	SCE_NP_MATCHING2_LOBBY_BIN_ATTR_INTERNAL_1_ID = 0x0037,
	SCE_NP_MATCHING2_LOBBY_BIN_ATTR_INTERNAL_2_ID = 0x0038,
};

// ID of internal room binary attribute
enum
{
	SCE_NP_MATCHING2_ROOM_BIN_ATTR_INTERNAL_1_ID = 0x0057,
	SCE_NP_MATCHING2_ROOM_BIN_ATTR_INTERNAL_2_ID = 0x0058,
};

// ID of internal room member binary attribute
enum
{
	SCE_NP_MATCHING2_ROOMMEMBER_BIN_ATTR_INTERNAL_1_ID = 0x0059,
};

// Attribute ID of user binary attribute
enum
{
	SCE_NP_MATCHING2_USER_BIN_ATTR_1_ID = 0x005f,
};

// Event of request functions
enum
{
	SCE_NP_MATCHING2_REQUEST_EVENT_GetServerInfo                  = 0x0001,
	SCE_NP_MATCHING2_REQUEST_EVENT_GetWorldInfoList               = 0x0002,
	SCE_NP_MATCHING2_REQUEST_EVENT_GetRoomMemberDataExternalList  = 0x0003,
	SCE_NP_MATCHING2_REQUEST_EVENT_SetRoomDataExternal            = 0x0004,
	SCE_NP_MATCHING2_REQUEST_EVENT_GetRoomDataExternalList        = 0x0005,
	SCE_NP_MATCHING2_REQUEST_EVENT_GetLobbyInfoList               = 0x0006,
	SCE_NP_MATCHING2_REQUEST_EVENT_SetUserInfo                    = 0x0007,
	SCE_NP_MATCHING2_REQUEST_EVENT_GetUserInfoList                = 0x0008,
	SCE_NP_MATCHING2_REQUEST_EVENT_CreateServerContext            = 0x0009,
	SCE_NP_MATCHING2_REQUEST_EVENT_DeleteServerContext            = 0x000a,
	SCE_NP_MATCHING2_REQUEST_EVENT_CreateJoinRoom                 = 0x0101,
	SCE_NP_MATCHING2_REQUEST_EVENT_JoinRoom                       = 0x0102,
	SCE_NP_MATCHING2_REQUEST_EVENT_LeaveRoom                      = 0x0103,
	SCE_NP_MATCHING2_REQUEST_EVENT_GrantRoomOwner                 = 0x0104,
	SCE_NP_MATCHING2_REQUEST_EVENT_KickoutRoomMember              = 0x0105,
	SCE_NP_MATCHING2_REQUEST_EVENT_SearchRoom                     = 0x0106,
	SCE_NP_MATCHING2_REQUEST_EVENT_SendRoomChatMessage            = 0x0107,
	SCE_NP_MATCHING2_REQUEST_EVENT_SendRoomMessage                = 0x0108,
	SCE_NP_MATCHING2_REQUEST_EVENT_SetRoomDataInternal            = 0x0109,
	SCE_NP_MATCHING2_REQUEST_EVENT_GetRoomDataInternal            = 0x010a,
	SCE_NP_MATCHING2_REQUEST_EVENT_SetRoomMemberDataInternal      = 0x010b,
	SCE_NP_MATCHING2_REQUEST_EVENT_GetRoomMemberDataInternal      = 0x010c,
	SCE_NP_MATCHING2_REQUEST_EVENT_SetSignalingOptParam           = 0x010d,
	SCE_NP_MATCHING2_REQUEST_EVENT_JoinLobby                      = 0x0201,
	SCE_NP_MATCHING2_REQUEST_EVENT_LeaveLobby                     = 0x0202,
	SCE_NP_MATCHING2_REQUEST_EVENT_SendLobbyChatMessage           = 0x0203,
	SCE_NP_MATCHING2_REQUEST_EVENT_SendLobbyInvitation            = 0x0204,
	SCE_NP_MATCHING2_REQUEST_EVENT_SetLobbyMemberDataInternal     = 0x0205,
	SCE_NP_MATCHING2_REQUEST_EVENT_GetLobbyMemberDataInternal     = 0x0206,
	SCE_NP_MATCHING2_REQUEST_EVENT_GetLobbyMemberDataInternalList = 0x0207,
	SCE_NP_MATCHING2_REQUEST_EVENT_SignalingGetPingInfo           = 0x0e01,
};

// Room event
enum
{
	SCE_NP_MATCHING2_ROOM_EVENT_MemberJoined                  = 0x1101,
	SCE_NP_MATCHING2_ROOM_EVENT_MemberLeft                    = 0x1102,
	SCE_NP_MATCHING2_ROOM_EVENT_Kickedout                     = 0x1103,
	SCE_NP_MATCHING2_ROOM_EVENT_RoomDestroyed                 = 0x1104,
	SCE_NP_MATCHING2_ROOM_EVENT_RoomOwnerChanged              = 0x1105,
	SCE_NP_MATCHING2_ROOM_EVENT_UpdatedRoomDataInternal       = 0x1106,
	SCE_NP_MATCHING2_ROOM_EVENT_UpdatedRoomMemberDataInternal = 0x1107,
	SCE_NP_MATCHING2_ROOM_EVENT_UpdatedSignalingOptParam      = 0x1108,
};

// Room message event
enum
{
	SCE_NP_MATCHING2_ROOM_MSG_EVENT_ChatMessage = 0x2101,
	SCE_NP_MATCHING2_ROOM_MSG_EVENT_Message     = 0x2102,
};

// Lobby event
enum
{
	SCE_NP_MATCHING2_LOBBY_EVENT_MemberJoined                   = 0x3201,
	SCE_NP_MATCHING2_LOBBY_EVENT_MemberLeft                     = 0x3202,
	SCE_NP_MATCHING2_LOBBY_EVENT_LobbyDestroyed                 = 0x3203,
	SCE_NP_MATCHING2_LOBBY_EVENT_UpdatedLobbyMemberDataInternal = 0x3204,
};

// Lobby message event
enum
{
	SCE_NP_MATCHING2_LOBBY_MSG_EVENT_ChatMessage = 0x4201,
	SCE_NP_MATCHING2_LOBBY_MSG_EVENT_Invitation  = 0x4202,
};

// Signaling event
enum
{
	SCE_NP_MATCHING2_SIGNALING_EVENT_Dead          = 0x5101,
	SCE_NP_MATCHING2_SIGNALING_EVENT_Established   = 0x5102,
	SCE_NP_MATCHING2_SIGNALING_EVENT_NetInfoResult = 0x5103,
};

// Context event
enum
{
	SCE_NP_MATCHING2_CONTEXT_EVENT_StartOver = 0x6f01,
	SCE_NP_MATCHING2_CONTEXT_EVENT_Start     = 0x6f02,
	SCE_NP_MATCHING2_CONTEXT_EVENT_Stop      = 0x6f03,
};

enum
{
	SCE_NP_MATCHING2_SIGNALING_NETINFO_NAT_STATUS_UNKNOWN = 0,
	SCE_NP_MATCHING2_SIGNALING_NETINFO_NAT_STATUS_TYPE    = 1,
	SCE_NP_MATCHING2_SIGNALING_NETINFO_NAT_STATUS_TYPE2   = 2,
	SCE_NP_MATCHING2_SIGNALING_NETINFO_NAT_STATUS_TYPE3   = 3,
};

typedef u16 SceNpMatching2ServerId;
typedef u32 SceNpMatching2WorldId;
typedef u16 SceNpMatching2WorldNumber;
typedef u64 SceNpMatching2LobbyId;
typedef u16 SceNpMatching2LobbyNumber;
typedef u16 SceNpMatching2LobbyMemberId;
typedef u64 SceNpMatching2RoomId;
typedef u16 SceNpMatching2RoomNumber;
typedef u16 SceNpMatching2RoomMemberId;
typedef u8 SceNpMatching2RoomGroupId;
typedef u8 SceNpMatching2TeamId;
typedef u16 SceNpMatching2ContextId;
typedef u32 SceNpMatching2RequestId;
typedef u16 SceNpMatching2AttributeId;
typedef u32 SceNpMatching2FlagAttr;
typedef u8 SceNpMatching2NatType;
typedef u8 SceNpMatching2Operator;
typedef u8 SceNpMatching2CastType;
typedef u8 SceNpMatching2SessionType;
typedef u8 SceNpMatching2SignalingType;
typedef u8 SceNpMatching2SignalingFlag;
typedef u8 SceNpMatching2EventCause;
typedef u8 SceNpMatching2ServerStatus;
typedef u8 SceNpMatching2Role;
typedef u8 SceNpMatching2BlockKickFlag;
typedef u64 SceNpMatching2RoomPasswordSlotMask;
typedef u64 SceNpMatching2RoomJoinedSlotMask;
typedef u16 SceNpMatching2Event;
typedef u32 SceNpMatching2EventKey;
typedef u32 SceNpMatching2SignalingRequestId;
typedef SceNpCommunicationPassphrase SceNpMatching2TitlePassphrase;

// Request callback function
using SceNpMatching2RequestCallback = void(SceNpMatching2ContextId ctxId, SceNpMatching2RequestId reqId, SceNpMatching2Event event, SceNpMatching2EventKey eventKey,
										   s32 errorCode, u32 dataSize, vm::ptr<void> arg);
using SceNpMatching2RoomEventCallback = void(SceNpMatching2ContextId ctxId, SceNpMatching2RoomId roomId, SceNpMatching2Event event, SceNpMatching2EventKey eventKey,
											 s32 errorCode, u32 dataSize, vm::ptr<void> arg);
using SceNpMatching2RoomMessageCallback = void(SceNpMatching2ContextId ctxId, SceNpMatching2RoomId roomId, SceNpMatching2RoomMemberId srcMemberId, SceNpMatching2Event event,
											   SceNpMatching2EventKey eventKey, s32 errorCode, u32 dataSize, vm::ptr<void> arg);
using SceNpMatching2LobbyEventCallback = void(SceNpMatching2ContextId ctxId, SceNpMatching2LobbyId lobbyId, SceNpMatching2Event event, SceNpMatching2EventKey eventKey,
											  s32 errorCode, u32 dataSize, vm::ptr<void> arg);
using SceNpMatching2LobbyMessageCallback = void(SceNpMatching2ContextId ctxId, SceNpMatching2LobbyId lobbyId, SceNpMatching2LobbyMemberId srcMemberId, SceNpMatching2Event event,
												SceNpMatching2EventKey eventKey, s32 errorCode, u32 dataSize, vm::ptr<void> arg);
using SceNpMatching2SignalingCallback = void(SceNpMatching2ContextId ctxId, SceNpMatching2RoomId roomId, SceNpMatching2RoomMemberId peerMemberId, SceNpMatching2Event event,
											 s32 errorCode, vm::ptr<void> arg);
using SceNpMatching2ContextCallback = void(SceNpMatching2ContextId ctxId, SceNpMatching2Event event, SceNpMatching2EventCause eventCause, s32 errorCode, vm::ptr<void> arg);

// Session password
struct SceNpMatching2SessionPassword
{
	u8 data[SCE_NP_MATCHING2_SESSION_PASSWORD_SIZE];
};

// Optional presence data
struct SceNpMatching2PresenceOptionData
{
	u8 data[SCE_NP_MATCHING2_PRESENCE_OPTION_DATA_SIZE];
	be_t<u32> length;
};

// Integer-type attribute
struct SceNpMatching2IntAttr
{
	be_t<SceNpMatching2AttributeId> id;
	u8 padding[2];
	be_t<u32> num;
};

// Binary-type attribute
struct SceNpMatching2BinAttr
{
	be_t<SceNpMatching2AttributeId> id;
	u8 padding[2];
	vm::bptr<u8> ptr;
	be_t<u32> size;
};

// Range filter
struct SceNpMatching2RangeFilter
{
	be_t<u32> startIndex;
	be_t<u32> max;
};

// Integer-type search condition
struct SceNpMatching2IntSearchFilter
{
	SceNpMatching2Operator searchOperator;
	u8 padding[3];
	SceNpMatching2IntAttr attr;
};

// Binary-type search condition
struct SceNpMatching2BinSearchFilter
{
	SceNpMatching2Operator searchOperator;
	u8 padding[3];
	SceNpMatching2BinAttr attr;
};

// Range of result
struct SceNpMatching2Range
{
	be_t<u32> startIndex;
	be_t<u32> total;
	be_t<u32> size;
};

// Session information about a session joined by the user
struct SceNpMatching2JoinedSessionInfo
{
	u8 sessionType;
	u8 padding1[1];
	be_t<SceNpMatching2ServerId> serverId;
	be_t<SceNpMatching2WorldId> worldId;
	be_t<SceNpMatching2LobbyId> lobbyId;
	be_t<SceNpMatching2RoomId> roomId;
	CellRtcTick joinDate;
};

// User information
struct SceNpMatching2UserInfo
{
	vm::bptr<SceNpMatching2UserInfo> next;
	SceNpUserInfo2 userInfo;
	vm::bptr<SceNpMatching2BinAttr> userBinAttr;
	be_t<u32> userBinAttrNum;
	SceNpMatching2JoinedSessionInfo joinedSessionInfo;
	be_t<u32> joinedSessionInfoNum;
};

// Server
struct SceNpMatching2Server
{
	be_t<SceNpMatching2ServerId> serverId;
	SceNpMatching2ServerStatus status;
	u8 padding[1];
};

// World
struct SceNpMatching2World
{
	be_t<SceNpMatching2WorldId> worldId;
	be_t<u32> numOfLobby;
	be_t<u32> maxNumOfTotalLobbyMember;
	be_t<u32> curNumOfTotalLobbyMember;
	be_t<u32> curNumOfRoom;
	be_t<u32> curNumOfTotalRoomMember;
	b8 withEntitlementId;
	SceNpEntitlementId entitlementId;
	u8 padding[3];
};

// Lobby member internal binary attribute
struct SceNpMatching2LobbyMemberBinAttrInternal
{
	CellRtcTick updateDate;
	SceNpMatching2BinAttr data;
	u8 padding[4];
};

// Lobby-internal lobby member information
struct SceNpMatching2LobbyMemberDataInternal
{
	vm::bptr<SceNpMatching2LobbyMemberDataInternal> next;
	SceNpUserInfo2 userInfo;
	CellRtcTick joinDate;
	be_t<SceNpMatching2LobbyMemberId> memberId;
	u8 padding[2];
	be_t<SceNpMatching2FlagAttr> flagAttr;
	vm::bptr<SceNpMatching2JoinedSessionInfo> joinedSessionInfo;
	be_t<u32> joinedSessionInfoNum;
	vm::bptr<SceNpMatching2LobbyMemberBinAttrInternal> lobbyMemberBinAttrInternal;
	be_t<u32> lobbyMemberBinAttrInternalNum; // Unsigned ints are be_t<u32> not uint, right?
};

// Lobby member ID list
struct SceNpMatching2LobbyMemberIdList
{
	be_t<SceNpMatching2LobbyMemberId> memberId;
	be_t<u32> memberIdNum;
	be_t<SceNpMatching2LobbyMemberId> me;
	u8 padding[6];
};

// Lobby-internal binary attribute
struct SceNpMatching2LobbyBinAttrInternal
{
	CellRtcTick updateDate;
	be_t<SceNpMatching2LobbyMemberId> updateMemberId;
	u8 padding[2];
	SceNpMatching2BinAttr data;
};

// Lobby-external lobby information
struct SceNpMatching2LobbyDataExternal
{
	vm::bptr<SceNpMatching2LobbyDataExternal> next;
	be_t<SceNpMatching2ServerId> serverId;
	u8 padding1[2];
	be_t<SceNpMatching2WorldId> worldId;
	u8 padding2[4];
	be_t<SceNpMatching2LobbyId	> lobbyId;
	be_t<u32> maxSlot;
	be_t<u32> curMemberNum;
	be_t<u32> flagAttr;
	vm::bptr<SceNpMatching2IntAttr> lobbySearchableIntAttrExternal;
	be_t<u32> lobbySearchableIntAttrExternalNum;
	vm::bptr<SceNpMatching2BinAttr> lobbySearchableBinAttrExternal;
	be_t<u32> lobbySearchableBinAttrExternalNum;
	vm::bptr<SceNpMatching2BinAttr> lobbyBinAttrExternal;
	be_t<u32> lobbyBinAttrExternalNum;
	u8 padding3[4];
};

// Lobby-internal lobby information
struct SceNpMatching2LobbyDataInternal
{
	be_t<SceNpMatching2ServerId> serverId;
	u8 padding1[2];
	be_t<SceNpMatching2WorldId> worldId;
	be_t<SceNpMatching2LobbyId> lobbyId;
	be_t<u32> maxSlot;
	SceNpMatching2LobbyMemberIdList memberIdList;
	be_t<SceNpMatching2FlagAttr> flagAttr;
	vm::bptr<SceNpMatching2LobbyBinAttrInternal> lobbyBinAttrInternal;
	be_t<u32> lobbyBinAttrInternalNum;
};

// Lobby message transmission destination
union SceNpMatching2LobbyMessageDestination
{
	be_t<SceNpMatching2LobbyMemberId> unicastTarget;

	struct multicastTarget
	{
		vm::bptr<SceNpMatching2LobbyMemberId> memberId;
		be_t<u32> memberIdNum;
	};
};

// Group label
struct SceNpMatching2GroupLabel
{
	u8 data[SCE_NP_MATCHING2_GROUP_LABEL_SIZE];
};

// Set groups in a room
struct SceNpMatching2RoomGroupConfig
{
	be_t<u32> slotNum;
	b8 withLabel;
	SceNpMatching2GroupLabel label;
	b8 withPassword;
	u8 padding[2];
};

// Set group password
struct SceNpMatching2RoomGroupPasswordConfig
{
	SceNpMatching2RoomGroupId groupId;
	b8 withPassword;
	u8 padding[1];
};

// Group (of slots in a room)
struct SceNpMatching2RoomGroup
{
	SceNpMatching2RoomGroupId groupId;
	b8 withPassword;
	b8 withLabel;
	u8 padding[1];
	SceNpMatching2GroupLabel label;
	be_t<u32> slotNum;
	be_t<u32> curGroupMemberNum;
};

// Internal room member binary attribute
struct SceNpMatching2RoomMemberBinAttrInternal
{
	CellRtcTick updateDate;
	SceNpMatching2BinAttr data;
	u8 padding[4];
};

// External room member data
struct SceNpMatching2RoomMemberDataExternal
{
	vm::bptr<SceNpMatching2RoomMemberDataExternal> next;
	SceNpUserInfo2 userInfo;
	CellRtcTick joinDate;
	SceNpMatching2Role role;
	u8 padding[7];
};

// Internal room member data
struct SceNpMatching2RoomMemberDataInternal
{
	vm::bptr<SceNpMatching2RoomMemberDataInternal> next;
	SceNpUserInfo2 userInfo;
	CellRtcTick joinDate;
	be_t<SceNpMatching2RoomMemberId> memberId;
	SceNpMatching2TeamId teamId;
	u8 padding1[1];
	vm::bptr<SceNpMatching2RoomGroup> roomGroup;
	SceNpMatching2NatType natType;
	u8 padding2[3];
	be_t<SceNpMatching2FlagAttr> flagAttr;
	vm::bptr<SceNpMatching2RoomMemberBinAttrInternal> roomMemberBinAttrInternal;
	be_t<u32> roomMemberBinAttrInternalNum;
};

// Internal room member data list
struct SceNpMatching2RoomMemberDataInternalList
{
	vm::bptr<SceNpMatching2RoomMemberDataInternal> members;
	be_t<u32> membersNum;
	vm::bptr<SceNpMatching2RoomMemberDataInternal> me;
	vm::bptr<SceNpMatching2RoomMemberDataInternal> owner;
};

// Internal room binary attribute
struct SceNpMatching2RoomBinAttrInternal
{
	CellRtcTick updateDate;
	be_t<SceNpMatching2RoomMemberId> updateMemberId;
	u8 padding[2];
	SceNpMatching2BinAttr data;
};

// External room data
struct SceNpMatching2RoomDataExternal
{
	vm::bptr<SceNpMatching2RoomDataExternal> next;
	be_t<SceNpMatching2ServerId> serverId;
	u8 padding1[2];
	be_t<SceNpMatching2WorldId> worldId;
	be_t<u16> publicSlotNum;
	be_t<u16> privateSlotNum;
	be_t<SceNpMatching2LobbyId> lobbyId;
	be_t<SceNpMatching2RoomId> roomId;
	be_t<u16> openPublicSlotNum;
	be_t<u16> maxSlot;
	be_t<u16> openPrivateSlotNum;
	be_t<u16> curMemberNum;
	be_t<SceNpMatching2RoomPasswordSlotMask> passwordSlotMask;
	vm::bptr<SceNpUserInfo2> owner;
	vm::bptr<SceNpMatching2RoomGroup> roomGroup;
	be_t<u32> roomGroupNum;
	be_t<u32> flagAttr;
	vm::bptr<SceNpMatching2IntAttr> roomSearchableIntAttrExternal;
	be_t<u32> roomSearchableIntAttrExternalNum;
	vm::bptr<SceNpMatching2BinAttr> roomSearchableBinAttrExternal;
	be_t<u32> roomSearchableBinAttrExternalNum;
	vm::bptr<SceNpMatching2BinAttr> roomBinAttrExternal;
	be_t<u32> roomBinAttrExternalNum;
};

// Internal room data
struct SceNpMatching2RoomDataInternal
{
	be_t<SceNpMatching2ServerId> serverId;
	u8 padding1[2];
	be_t<SceNpMatching2WorldId> worldId;
	be_t<SceNpMatching2LobbyId> lobbyId;
	be_t<SceNpMatching2RoomId> roomId;
	be_t<SceNpMatching2RoomPasswordSlotMask> passwordSlotMask;
	be_t<u32> maxSlot;
	SceNpMatching2RoomMemberDataInternalList memberList;
	vm::bptr<SceNpMatching2RoomGroup> roomGroup;
	be_t<u32> roomGroupNum;
	be_t<SceNpMatching2FlagAttr> flagAttr;
	vm::bptr<SceNpMatching2RoomBinAttrInternal> roomBinAttrInternal;
	be_t<u32> roomBinAttrInternalNum;
};

// Room message recipient
union SceNpMatching2RoomMessageDestination
{
	be_t<SceNpMatching2RoomMemberId> unicastTarget;

	struct multicastTarget
	{
		vm::bptr<SceNpMatching2RoomMemberId> memberId;
		be_t<u32> memberIdNum;
	} multicastTarget;

	SceNpMatching2TeamId multicastTargetTeamId;
};

// Invitation data
struct SceNpMatching2InvitationData
{
	vm::bptr<SceNpMatching2JoinedSessionInfo> targetSession;
	be_t<u32> targetSessionNum;
	vm::bptr<void> optData;
	be_t<u32> optDataLen;
};

// Signaling option parameter
struct SceNpMatching2SignalingOptParam
{
	SceNpMatching2SignalingType type;
	SceNpMatching2SignalingFlag flag;
	be_t<SceNpMatching2RoomMemberId> hubMemberId;
	u8 reserved2[4];
};

// Option parameters for requests
struct SceNpMatching2RequestOptParam
{
	vm::bptr<SceNpMatching2RequestCallback> cbFunc;
	vm::bptr<void> cbFuncArg;
	be_t<u32> timeout;
	be_t<u16> appReqId;
	u8 padding[2];
};

// Room slot information
struct SceNpMatching2RoomSlotInfo
{
	be_t<SceNpMatching2RoomId> roomId;
	be_t<SceNpMatching2RoomJoinedSlotMask> joinedSlotMask;
	be_t<SceNpMatching2RoomPasswordSlotMask> passwordSlotMask;
	be_t<u16> publicSlotNum;
	be_t<u16> privateSlotNum;
	be_t<u16> openPublicSlotNum;
	be_t<u16> openPrivateSlotNum;
};

// Server data request parameter
struct SceNpMatching2GetServerInfoRequest
{
	be_t<SceNpMatching2ServerId> serverId;
};

// Server data request response data
struct SceNpMatching2GetServerInfoResponse
{
	SceNpMatching2Server server;
};

// Request parameter for creating a server context
struct SceNpMatching2CreateServerContextRequest
{
	be_t<SceNpMatching2ServerId> serverId;
};

// Request parameter for deleting a server context
struct SceNpMatching2DeleteServerContextRequest
{
	be_t<SceNpMatching2ServerId> serverId;
};

// World data list request parameter
struct SceNpMatching2GetWorldInfoListRequest
{
	be_t<SceNpMatching2ServerId> serverId;
};

// World data list request response data
struct SceNpMatching2GetWorldInfoListResponse
{
	vm::bptr<SceNpMatching2World> world;
	be_t<u32> worldNum;
};

// User information setting request parameter
struct SceNpMatching2SetUserInfoRequest
{
	be_t<SceNpMatching2ServerId> serverId;
	u8 padding[2];
	vm::bptr<SceNpMatching2BinAttr> userBinAttr;
	be_t<u32> userBinAttrNum;
};

// User information list acquisition request parameter
struct SceNpMatching2GetUserInfoListRequest
{
	be_t<SceNpMatching2ServerId> serverId;
	u8 padding[2];
	vm::bptr<SceNpId> npId;
	be_t<u32> npIdNum;
	vm::bptr<SceNpMatching2AttributeId> attrId;
	be_t<u32> attrIdNum;
	be_t<s32> option; // int should be be_t<s32>, right?
};

// User information list acquisition response data
struct SceNpMatching2GetUserInfoListResponse
{
	vm::bptr<SceNpMatching2UserInfo> userInfo;
	be_t<u32> userInfoNum;
};

// External room member data list request parameter
struct SceNpMatching2GetRoomMemberDataExternalListRequest
{
	be_t<SceNpMatching2RoomId> roomId;
};

// External room member data list request response data
struct SceNpMatching2GetRoomMemberDataExternalListResponse
{
	vm::bptr<SceNpMatching2RoomMemberDataExternal> roomMemberDataExternal;
	be_t<u32> roomMemberDataExternalNum;
};

// External room data configuration request parameters
struct SceNpMatching2SetRoomDataExternalRequest
{
	be_t<SceNpMatching2RoomId> roomId;
	vm::bptr<SceNpMatching2IntAttr> roomSearchableIntAttrExternal;
	be_t<u32> roomSearchableIntAttrExternalNum;
	vm::bptr<SceNpMatching2BinAttr> roomSearchableBinAttrExternal;
	be_t<u32> roomSearchableBinAttrExternalNum;
	vm::bptr<SceNpMatching2BinAttr> roomBinAttrExternal;
	be_t<u32> roomBinAttrExternalNum;
};

// External room data list request parameters
struct SceNpMatching2GetRoomDataExternalListRequest
{
	vm::bptr<SceNpMatching2RoomId> roomId;
	be_t<u32> roomIdNum;
	vm::bcptr<SceNpMatching2AttributeId> attrId;
	be_t<u32> attrIdNum;
};

// External room data list request response data
struct SceNpMatching2GetRoomDataExternalListResponse
{
	vm::bptr<SceNpMatching2RoomDataExternal> roomDataExternal;
	be_t<u32> roomDataExternalNum;
};

// Create-and-join room request parameters
struct SceNpMatching2CreateJoinRoomRequest
{
	be_t<SceNpMatching2WorldId> worldId;
	u8 padding1[4];
	be_t<SceNpMatching2LobbyId> lobbyId;
	be_t<u32> maxSlot;
	be_t<u32> flagAttr;
	vm::bptr<SceNpMatching2BinAttr> roomBinAttrInternal;
	be_t<u32> roomBinAttrInternalNum;
	vm::bptr<SceNpMatching2IntAttr> roomSearchableIntAttrExternal;
	be_t<u32> roomSearchableIntAttrExternalNum;
	vm::bptr<SceNpMatching2BinAttr> roomSearchableBinAttrExternal;
	be_t<u32> roomSearchableBinAttrExternalNum;
	vm::bptr<SceNpMatching2BinAttr> roomBinAttrExternal;
	be_t<u32> roomBinAttrExternalNum;
	vm::bptr<SceNpMatching2SessionPassword> roomPassword;
	vm::bptr<SceNpMatching2RoomGroupConfig> groupConfig;
	be_t<u32> groupConfigNum;
	vm::bptr<SceNpMatching2RoomPasswordSlotMask> passwordSlotMask;
	vm::bptr<SceNpId> allowedUser;
	be_t<u32> allowedUserNum;
	vm::bptr<SceNpId> blockedUser;
	be_t<u32> blockedUserNum;
	vm::bptr<SceNpMatching2GroupLabel> joinRoomGroupLabel;
	vm::bptr<SceNpMatching2BinAttr> roomMemberBinAttrInternal;
	be_t<u32> roomMemberBinAttrInternalNum;
	SceNpMatching2TeamId teamId;
	u8 padding2[3];
	vm::bptr<SceNpMatching2SignalingOptParam> sigOptParam;
	u8 padding3[4];
};

// Create-and-join room request response data
struct SceNpMatching2CreateJoinRoomResponse
{
	vm::bptr<SceNpMatching2RoomDataInternal> roomDataInternal;
};

// Join room request parameters
struct SceNpMatching2JoinRoomRequest
{
	be_t<SceNpMatching2RoomId> roomId;
	vm::bptr<SceNpMatching2SessionPassword> roomPassword;
	vm::bptr<SceNpMatching2GroupLabel> joinRoomGroupLabel;
	vm::bptr<SceNpMatching2BinAttr> roomMemberBinAttrInternal;
	be_t<u32> roomMemberBinAttrInternalNum;
	SceNpMatching2PresenceOptionData optData;
	SceNpMatching2TeamId teamId;
	u8 padding[3];
};

// Join room request response data
struct SceNpMatching2JoinRoomResponse
{
	vm::bptr<SceNpMatching2RoomDataInternal> roomDataInternal;
};

// Leave room request parameters
struct SceNpMatching2LeaveRoomRequest
{
	be_t<SceNpMatching2RoomId> roomId;
	SceNpMatching2PresenceOptionData optData;
	u8 padding[4];
};

// Room ownership grant request parameters
struct SceNpMatching2GrantRoomOwnerRequest
{
	be_t<SceNpMatching2RoomId> roomId;
	be_t<SceNpMatching2RoomMemberId> newOwner;
	u8 padding[2];
	SceNpMatching2PresenceOptionData optData;
};

// Kickout request parameters
struct SceNpMatching2KickoutRoomMemberRequest
{
	be_t<SceNpMatching2RoomId> roomId;
	be_t<SceNpMatching2RoomMemberId> target;
	SceNpMatching2BlockKickFlag blockKickFlag;
	u8 padding[1];
	SceNpMatching2PresenceOptionData optData;
};

// Room search parameters
struct SceNpMatching2SearchRoomRequest
{
	be_t<s32> option;
	be_t<SceNpMatching2WorldId> worldId;
	be_t<SceNpMatching2LobbyId> lobbyId;
	SceNpMatching2RangeFilter rangeFilter;
	be_t<SceNpMatching2FlagAttr> flagFilter;
	be_t<SceNpMatching2FlagAttr> flagAttr;
	vm::bptr<SceNpMatching2IntSearchFilter> intFilter;
	be_t<u32> intFilterNum;
	vm::bptr<SceNpMatching2BinSearchFilter> binFilter;
	be_t<u32> binFilterNum;
	vm::bptr<SceNpMatching2AttributeId> attrId;
	be_t<u32> attrIdNum;
};

// Room search response data
struct SceNpMatching2SearchRoomResponse
{
	SceNpMatching2Range range;
	vm::bptr<SceNpMatching2RoomDataExternal> roomDataExternal;
};

// Room message send request parameters
struct SceNpMatching2SendRoomMessageRequest
{
	be_t<SceNpMatching2RoomId> roomId;
	SceNpMatching2CastType castType;
	u8 padding[3];
	SceNpMatching2RoomMessageDestination dst;
	vm::bcptr<void> msg;
	be_t<u32> msgLen;
	be_t<s32> option;
};

// Room chat message send request parameters
struct SceNpMatching2SendRoomChatMessageRequest
{
	be_t<SceNpMatching2RoomId> roomId;
	SceNpMatching2CastType castType;
	u8 padding[3];
	SceNpMatching2RoomMessageDestination dst;
	vm::bcptr<void> msg;
	be_t<u32> msgLen;
	be_t<s32> option;
};

// Room chat message send request response data
struct SceNpMatching2SendRoomChatMessageResponse
{
	b8 filtered;
};

// Internal room data configuration request parameters
struct SceNpMatching2SetRoomDataInternalRequest
{
	be_t<SceNpMatching2RoomId> roomId;
	be_t<SceNpMatching2FlagAttr> flagFilter;
	be_t<SceNpMatching2FlagAttr> flagAttr;
	vm::bptr<SceNpMatching2BinAttr> roomBinAttrInternal;
	be_t<u32> roomBinAttrInternalNum;
	vm::bptr<SceNpMatching2RoomGroupPasswordConfig> passwordConfig;
	be_t<u32> passwordConfigNum;
	vm::bptr<SceNpMatching2RoomPasswordSlotMask> passwordSlotMask;
	vm::bptr<SceNpMatching2RoomMemberId> ownerPrivilegeRank;
	be_t<u32> ownerPrivilegeRankNum;
	u8 padding[4];
};

// Internal room data request parameters
struct SceNpMatching2GetRoomDataInternalRequest
{
	be_t<SceNpMatching2RoomId> roomId;
	vm::bcptr<SceNpMatching2AttributeId> attrId;
	be_t<u32> attrIdNum;
};

// Internal room data request response data
struct SceNpMatching2GetRoomDataInternalResponse
{
	vm::bptr<SceNpMatching2RoomDataInternal> roomDataInternal;
};

// Internal room member data configuration request parameters
struct SceNpMatching2SetRoomMemberDataInternalRequest
{
	be_t<SceNpMatching2RoomId> roomId;
	be_t<SceNpMatching2RoomMemberId> memberId;
	SceNpMatching2TeamId teamId;
	u8 padding[5];
	be_t<SceNpMatching2FlagAttr> flagFilter;
	be_t<SceNpMatching2FlagAttr> flagAttr;
	vm::bptr<SceNpMatching2BinAttr> roomMemberBinAttrInternal;
	be_t<u32> roomMemberBinAttrInternalNum;
};

// Internal room member data request parameters
struct SceNpMatching2GetRoomMemberDataInternalRequest
{
	be_t<SceNpMatching2RoomId> roomId;
	be_t<SceNpMatching2RoomMemberId> memberId;
	u8 padding[6];
	vm::bcptr<SceNpMatching2AttributeId> attrId;
	be_t<u32> attrIdNum;
};

// Internal room member data request response data
struct SceNpMatching2GetRoomMemberDataInternalResponse
{
	vm::bptr<SceNpMatching2RoomMemberDataInternal> roomMemberDataInternal;
};

// Signaling option parameter setting request parameter
struct SceNpMatching2SetSignalingOptParamRequest
{
	be_t<SceNpMatching2RoomId> roomId;
	SceNpMatching2SignalingOptParam sigOptParam;
};

// Lobby information list acquisition request parameter
struct SceNpMatching2GetLobbyInfoListRequest
{
	be_t<SceNpMatching2WorldId> worldId;
	SceNpMatching2RangeFilter rangeFilter;
	vm::bptr<SceNpMatching2AttributeId> attrId;
	be_t<u32> attrIdNum;
};

// Lobby information list acquisition response data
struct SceNpMatching2GetLobbyInfoListResponse
{
	SceNpMatching2Range range;
	vm::bptr<SceNpMatching2LobbyDataExternal> lobbyDataExternal;
};

// Lobby joining request parameter
struct SceNpMatching2JoinLobbyRequest
{
	be_t<SceNpMatching2LobbyId> lobbyId;
	vm::bptr<SceNpMatching2JoinedSessionInfo> joinedSessionInfo;
	be_t<u32> joinedSessionInfoNum;
	vm::bptr<SceNpMatching2BinAttr> lobbyMemberBinAttrInternal;
	be_t<u32> lobbyMemberBinAttrInternalNum;
	SceNpMatching2PresenceOptionData optData;
	u8 padding[4];
};

// Lobby joining response data
struct SceNpMatching2JoinLobbyResponse
{
	vm::bptr<SceNpMatching2LobbyDataInternal> lobbyDataInternal;
};

// Lobby leaving request parameter
struct SceNpMatching2LeaveLobbyRequest
{
	be_t<SceNpMatching2LobbyId> lobbyId;
	SceNpMatching2PresenceOptionData optData;
	u8 padding[4];
};

// Lobby chat message sending request parameter
struct SceNpMatching2SendLobbyChatMessageRequest
{
	be_t<SceNpMatching2LobbyId> lobbyId;
	SceNpMatching2CastType castType;
	u8 padding[3];
	SceNpMatching2LobbyMessageDestination dst;
	vm::bcptr<void> msg;
	be_t<u32> msgLen;
	be_t<s32> option;
};

// Lobby chat message sending response data
struct SceNpMatching2SendLobbyChatMessageResponse
{
	b8 filtered;
};

// Lobby invitation message sending request parameter
struct SceNpMatching2SendLobbyInvitationRequest
{
	be_t<SceNpMatching2LobbyId> lobbyId;
	SceNpMatching2CastType castType;
	u8 padding[3];
	SceNpMatching2LobbyMessageDestination dst;
	SceNpMatching2InvitationData invitationData;
	be_t<s32> option;
};

// Lobby-internal lobby member information setting request parameter
struct SceNpMatching2SetLobbyMemberDataInternalRequest
{
	be_t<SceNpMatching2LobbyId> lobbyId;
	be_t<SceNpMatching2LobbyMemberId> memberId;
	u8 padding1[2];
	be_t<SceNpMatching2FlagAttr> flagFilter;
	be_t<SceNpMatching2FlagAttr> flagAttr;
	vm::bptr<SceNpMatching2JoinedSessionInfo> joinedSessionInfo;
	be_t<u32> joinedSessionInfoNum;
	vm::bptr<SceNpMatching2BinAttr> lobbyMemberBinAttrInternal;
	be_t<u32> lobbyMemberBinAttrInternalNum;
	u8 padding2[4];
};

// Lobby-internal lobby member information acquisition request parameter
struct SceNpMatching2GetLobbyMemberDataInternalRequest
{
	be_t<SceNpMatching2LobbyId> lobbyId;
	be_t<SceNpMatching2LobbyMemberId> memberId;
	u8 padding[6];
	vm::bcptr<SceNpMatching2AttributeId> attrId;
	be_t<u32> attrIdNum;
};

// Lobby-internal lobby member information acquisition response data
struct SceNpMatching2GetLobbyMemberDataInternalResponse
{
	vm::bptr<SceNpMatching2LobbyMemberDataInternal> lobbyMemberDataInternal;
};

// Request parameters for obtaining a list of lobby-internal lobby member information
struct SceNpMatching2GetLobbyMemberDataInternalListRequest
{
	be_t<SceNpMatching2LobbyId> lobbyId;
	vm::bptr<SceNpMatching2LobbyMemberId> memberId;
	be_t<u32> memberIdNum;
	vm::bcptr<SceNpMatching2AttributeId> attrId;
	be_t<u32> attrIdNum;
	b8 extendedData;
	u8 padding[7];
};

// Reponse data for obtaining a list of lobby-internal lobby member information
struct SceNpMatching2GetLobbyMemberDataInternalListResponse
{
	vm::bptr<SceNpMatching2LobbyMemberDataInternal> lobbyMemberDataInternal;
	be_t<u32> lobbyMemberDataInternalNum;
};

// Request parameters for obtaining Ping information
struct SceNpMatching2SignalingGetPingInfoRequest
{
	be_t<SceNpMatching2RoomId> roomId;
	u8 reserved[16];
};

// Response data for obtaining Ping information
struct SceNpMatching2SignalingGetPingInfoResponse
{
	be_t<SceNpMatching2ServerId> serverId;
	u8 padding1[2];
	be_t<SceNpMatching2WorldId> worldId;
	be_t<SceNpMatching2RoomId> roomId;
	be_t<u32> rtt;
	u8 reserved[20];
};

// Join request parameters for room in prohibitive mode
struct SceNpMatching2JoinProhibitiveRoomRequest
{
	SceNpMatching2JoinRoomRequest joinParam;
	vm::bptr<SceNpId> blockedUser;
	be_t<u32> blockedUserNum;
};

// Room member update information
struct SceNpMatching2RoomMemberUpdateInfo
{
	vm::bptr<SceNpMatching2RoomMemberDataInternal> roomMemberDataInternal;
	SceNpMatching2EventCause eventCause;
	u8 padding[3];
	SceNpMatching2PresenceOptionData optData;
};

// Room owner update information
struct SceNpMatching2RoomOwnerUpdateInfo
{
	be_t<SceNpMatching2RoomMemberId> prevOwner;
	be_t<SceNpMatching2RoomMemberId> newOwner;
	SceNpMatching2EventCause eventCause;
	u8 padding[3];
	vm::bptr<SceNpMatching2SessionPassword> roomPassword;
	SceNpMatching2PresenceOptionData optData;
};

// Room update information
struct SceNpMatching2RoomUpdateInfo
{
	SceNpMatching2EventCause eventCause;
	u8 padding[3];
	be_t<s32> errorCode;
	SceNpMatching2PresenceOptionData optData;
};

// Internal room data update information
struct SceNpMatching2RoomDataInternalUpdateInfo
{
	vm::bptr<SceNpMatching2RoomDataInternal> newRoomDataInternal;
	vm::bptr<SceNpMatching2FlagAttr> newFlagAttr;
	vm::bptr<SceNpMatching2FlagAttr> prevFlagAttr;
	vm::bptr<SceNpMatching2RoomPasswordSlotMask> newRoomPasswordSlotMask;
	vm::bptr<SceNpMatching2RoomPasswordSlotMask> prevRoomPasswordSlotMask;
	vm::bpptr<SceNpMatching2RoomGroup> newRoomGroup;
	be_t<u32> newRoomGroupNum;
	vm::bpptr<SceNpMatching2RoomBinAttrInternal> newRoomBinAttrInternal;
	be_t<u32> newRoomBinAttrInternalNum;
};

// Internal room member data update information
struct SceNpMatching2RoomMemberDataInternalUpdateInfo
{
	vm::bptr<SceNpMatching2RoomMemberDataInternal> newRoomMemberDataInternal;
	vm::bptr<SceNpMatching2FlagAttr> newFlagAttr;
	vm::bptr<SceNpMatching2FlagAttr> prevFlagAttr;
	vm::bptr<SceNpMatching2TeamId> newTeamId;
	vm::bpptr<SceNpMatching2RoomMemberBinAttrInternal> newRoomMemberBinAttrInternal;
	be_t<u32> newRoomMemberBinAttrInternalNum;
};

// Room message information
struct SceNpMatching2RoomMessageInfo
{
	b8 filtered;
	SceNpMatching2CastType castType;
	u8 padding[2];
	vm::bptr<SceNpMatching2RoomMessageDestination> dst;
	vm::bptr<SceNpUserInfo2> srcMember;
	vm::bptr<void> msg;
	be_t<u32> msgLen;
};

// Lobby member update information
struct SceNpMatching2LobbyMemberUpdateInfo
{
	vm::bptr<SceNpMatching2LobbyMemberDataInternal> lobbyMemberDataInternal;
	SceNpMatching2EventCause eventCause;
	u8 padding[3];
	SceNpMatching2PresenceOptionData optData;
};

// Lobby update information
struct SceNpMatching2LobbyUpdateInfo
{
	SceNpMatching2EventCause eventCause;
	u8 padding[3];
	be_t<s32> errorCode;
};

// Lobby-internal lobby member information update information
struct SceNpMatching2LobbyMemberDataInternalUpdateInfo
{
	be_t<SceNpMatching2LobbyMemberId> memberId;
	u8 padding[2];
	SceNpId npId;
	be_t<SceNpMatching2FlagAttr> flagFilter;
	be_t<SceNpMatching2FlagAttr> newFlagAttr;
	SceNpMatching2JoinedSessionInfo newJoinedSessionInfo;
	be_t<u32> newJoinedSessionInfoNum;
	vm::bptr<SceNpMatching2LobbyMemberBinAttrInternal> newLobbyMemberBinAttrInternal;
	be_t<u32> newLobbyMemberBinAttrInternalNum;
};

// Lobby message information
struct SceNpMatching2LobbyMessageInfo
{
	b8 filtered;
	SceNpMatching2CastType castType;
	u8 padding[2];
	vm::bptr<SceNpMatching2LobbyMessageDestination> dst;
	vm::bptr<SceNpUserInfo2> srcMember;
	vm::bcptr<void> msg;
	be_t<u32> msgLen;
};

// Lobby invitation message information
struct SceNpMatching2LobbyInvitationInfo
{
	SceNpMatching2CastType castType;
	u8 padding[3];
	vm::bptr<SceNpMatching2LobbyMessageDestination> dst;
	vm::bptr<SceNpUserInfo2> srcMember;
	SceNpMatching2InvitationData invitationData;
};

// Update information of the signaling option parameter
struct SceNpMatching2SignalingOptParamUpdateInfo
{
	SceNpMatching2SignalingOptParam newSignalingOptParam;
};

// Matching2 utility intilization parameters
struct SceNpMatching2UtilityInitParam
{
	be_t<u32> containerId;
	be_t<u32> requestCbQueueLen;
	be_t<u32> sessionEventCbQueueLen;
	be_t<u32> sessionMsgCbQueueLen;
	u8 reserved[16];
};

// Matching2 memory information
struct SceNpMatching2MemoryInfo
{
	be_t<u32> totalMemSize;
	be_t<u32> curMemUsage;
	be_t<u32> maxMemUsage;
	u8 reserved[12];
};

// Matching2 information on the event data queues in the system
struct SceNpMatching2CbQueueInfo
{
	be_t<u32> requestCbQueueLen;
	be_t<u32> curRequestCbQueueLen;
	be_t<u32> maxRequestCbQueueLen;
	be_t<u32> sessionEventCbQueueLen;
	be_t<u32> curSessionEventCbQueueLen;
	be_t<u32> maxSessionEventCbQueueLen;
	be_t<u32> sessionMsgCbQueueLen;
	be_t<u32> curSessionMsgCbQueueLen;
	be_t<u32> maxSessionMsgCbQueueLen;
	u8 reserved[12];
};

struct SceNpMatching2SignalingNetInfo
{
	be_t<u32> size;
	be_t<u32> localAddr;
	be_t<u32> mappedAddr;
	be_t<u32> natStatus;
};

// NP OAuth Errors
enum SceNpOauthError : u32
{
	SCE_NP_OAUTH_ERROR_UNKNOWN                                         = 0x80025f01,
	SCE_NP_OAUTH_ERROR_ALREADY_INITIALIZED                             = 0x80025f02,
	SCE_NP_OAUTH_ERROR_NOT_INITIALIZED                                 = 0x80025f03,
	SCE_NP_OAUTH_ERROR_INVALID_ARGUMENT                                = 0x80025f04,
	SCE_NP_OAUTH_ERROR_OUT_OF_MEMORY                                   = 0x80025f05,
	SCE_NP_OAUTH_ERROR_OUT_OF_BUFFER                                   = 0x80025f06,
	SCE_NP_OAUTH_ERROR_BAD_RESPONSE                                    = 0x80025f07,
	SCE_NP_OAUTH_ERROR_ABORTED                                         = 0x80025f08,
	SCE_NP_OAUTH_ERROR_SIGNED_OUT                                      = 0x80025f09,
	SCE_NP_OAUTH_ERROR_REQUEST_NOT_FOUND                               = 0x80025f0a,
	SCE_NP_OAUTH_ERROR_SSL_ERR_CN_CHECK                                = 0x80025f0b,
	SCE_NP_OAUTH_ERROR_SSL_ERR_UNKNOWN_CA                              = 0x80025f0c,
	SCE_NP_OAUTH_ERROR_SSL_ERR_NOT_AFTER_CHECK                         = 0x80025f0d,
	SCE_NP_OAUTH_ERROR_SSL_ERR_NOT_BEFORE_CHECK                        = 0x80025f0e,
	SCE_NP_OAUTH_ERROR_SSL_ERR_INVALID_CERT                            = 0x80025f0f,
	SCE_NP_OAUTH_ERROR_SSL_ERR_INTERNAL                                = 0x80025f10,
	SCE_NP_OAUTH_ERROR_REQUEST_MAX                                     = 0x80025f11,

	SCE_NP_OAUTH_SERVER_ERROR_BANNED_CONSOLE                           = 0x80025d14,
	SCE_NP_OAUTH_SERVER_ERROR_INVALID_LOGIN                            = 0x82e00014,
	SCE_NP_OAUTH_SERVER_ERROR_INACTIVE_ACCOUNT                         = 0x82e0001b,
	SCE_NP_OAUTH_SERVER_ERROR_SUSPENDED_ACCOUNT                        = 0x82e0001c,
	SCE_NP_OAUTH_SERVER_ERROR_SUSPENDED_DEVICE                         = 0x82e0001d,
	SCE_NP_OAUTH_SERVER_ERROR_PASSWORD_EXPIRED                         = 0x82e00064,
	SCE_NP_OAUTH_SERVER_ERROR_TOSUA_MUST_BE_RE_ACCEPTED                = 0x82e00067,
	SCE_NP_OAUTH_SERVER_ERROR_TOSUA_MUST_BE_RE_ACCEPTED_FOR_SUBACCOUNT = 0x82e01042,
	SCE_NP_OAUTH_SERVER_ERROR_BANNED_ACCOUNT                           = 0x82e01050,
	SCE_NP_OAUTH_SERVER_ERROR_SERVICE_END                              = 0x82e1019a,
	SCE_NP_OAUTH_SERVER_ERROR_SERVICE_UNAVAILABLE                      = 0x82e101f7,
};

typedef s32 SceNpAuthOAuthRequestId;

enum
{
	SCE_NP_AUTHORIZATION_CODE_MAX_LEN = 128,
	SCE_NP_CLIENT_ID_MAX_LEN          = 128,
};

struct SceNpClientId
{
	char id[SCE_NP_CLIENT_ID_MAX_LEN + 1];
	u8 padding[7];
};

struct SceNpAuthorizationCode
{
	char code[SCE_NP_AUTHORIZATION_CODE_MAX_LEN + 1];
	u8 padding[7];
};

struct SceNpAuthGetAuthorizationCodeParameter
{
	be_t<u32> size;
	vm::bcptr<SceNpClientId> pClientId;
	vm::bcptr<char> pScope;
};
