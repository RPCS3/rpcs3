#pragma once
#include "cellRtc.h"

// Error Codes
enum
{
	// NP Manager Utility
	SCE_NP_ERROR_NOT_INITIALIZED            = 0x8002aa01,
	SCE_NP_ERROR_ALREADY_INITIALIZED        = 0x8002aa02,
	SCE_NP_ERROR_INVALID_ARGUMENT           = 0x8002aa03,
	SCE_NP_ERROR_OUT_OF_MEMORY              = 0x8002aa04,
	SCE_NP_ERROR_ID_NO_SPACE                = 0x8002aa05,
	SCE_NP_ERROR_ID_NOT_FOUND               = 0x8002aa06,
	SCE_NP_ERROR_SESSION_RUNNING            = 0x8002aa07,
	SCE_NP_ERROR_LOGINID_ALREADY_EXISTS     = 0x8002aa08,
	SCE_NP_ERROR_INVALID_TICKET_SIZE        = 0x8002aa09,
	SCE_NP_ERROR_INVALID_STATE              = 0x8002aa0a,
	SCE_NP_ERROR_ABORTED                    = 0x8002aa0b,
	SCE_NP_ERROR_OFFLINE                    = 0x8002aa0c,
	SCE_NP_ERROR_VARIANT_ACCOUNT_ID         = 0x8002aa0d,
	SCE_NP_ERROR_GET_CLOCK                  = 0x8002aa0e,
	SCE_NP_ERROR_INSUFFICIENT_BUFFER        = 0x8002aa0f,
	SCE_NP_ERROR_EXPIRED_TICKET             = 0x8002aa10,
	SCE_NP_ERROR_TICKET_PARAM_NOT_FOUND     = 0x8002aa11,
	SCE_NP_ERROR_UNSUPPORTED_TICKET_VERSION = 0x8002aa12,
	SCE_NP_ERROR_TICKET_STATUS_CODE_INVALID = 0x8002aa13,
	SCE_NP_ERROR_INVALID_TICKET_VERSION     = 0x8002aa14,
	SCE_NP_ERROR_ALREADY_USED               = 0x8002aa15,
	SCE_NP_ERROR_DIFFERENT_USER             = 0x8002aa16,
	SCE_NP_ERROR_ALREADY_DONE               = 0x8002aa17,
	// NP Basic Utility
	SCE_NP_BASIC_ERROR_ALREADY_INITIALIZED      = 0x8002a661,
	SCE_NP_BASIC_ERROR_NOT_INITIALIZED          = 0x8002a662,
	SCE_NP_BASIC_ERROR_NOT_SUPPORTED            = 0x8002a663,
	SCE_NP_BASIC_ERROR_OUT_OF_MEMORY            = 0x8002a664,
	SCE_NP_BASIC_ERROR_INVALID_ARGUMENT         = 0x8002a665,
	SCE_NP_BASIC_ERROR_BAD_ID                   = 0x8002a666,
	SCE_NP_BASIC_ERROR_IDS_DIFFER               = 0x8002a667,
	SCE_NP_BASIC_ERROR_PARSER_FAILED            = 0x8002a668,
	SCE_NP_BASIC_ERROR_TIMEOUT                  = 0x8002a669,
	SCE_NP_BASIC_ERROR_NO_EVENT                 = 0x8002a66a,
	SCE_NP_BASIC_ERROR_EXCEEDS_MAX              = 0x8002a66b,
	SCE_NP_BASIC_ERROR_INSUFFICIENT             = 0x8002a66c,
	SCE_NP_BASIC_ERROR_NOT_REGISTERED           = 0x8002a66d,
	SCE_NP_BASIC_ERROR_DATA_LOST                = 0x8002a66e,
	SCE_NP_BASIC_ERROR_BUSY                     = 0x8002a66f,
	SCE_NP_BASIC_ERROR_STATUS                   = 0x8002a670,
	SCE_NP_BASIC_ERROR_CANCEL                   = 0x8002a671,
	SCE_NP_BASIC_ERROR_INVALID_MEMORY_CONTAINER = 0x8002a672,
	SCE_NP_BASIC_ERROR_INVALID_DATA_ID          = 0x8002a673,
	SCE_NP_BASIC_ERROR_BROKEN_DATA              = 0x8002a674,
	SCE_NP_BASIC_ERROR_BLOCKLIST_ADD_FAILED     = 0x8002a675,
	SCE_NP_BASIC_ERROR_BLOCKLIST_IS_FULL        = 0x8002a676,
	SCE_NP_BASIC_ERROR_SEND_FAILED              = 0x8002a677,
	SCE_NP_BASIC_ERROR_NOT_CONNECTED            = 0x8002a678,
	SCE_NP_BASIC_ERROR_INSUFFICIENT_DISK_SPACE  = 0x8002a679,
	SCE_NP_BASIC_ERROR_INTERNAL_FAILURE         = 0x8002a67a,
	SCE_NP_BASIC_ERROR_DOES_NOT_EXIST           = 0x8002a67b,
	SCE_NP_BASIC_ERROR_INVALID                  = 0x8002a67c,
	SCE_NP_BASIC_ERROR_UNKNOWN                  = 0x8002a6bf,
	SCE_NP_EXT_ERROR_CONTEXT_DOES_NOT_EXIST     = 0x8002a6a1,
	SCE_NP_EXT_ERROR_CONTEXT_ALREADY_EXISTS     = 0x8002a6a2,
	SCE_NP_EXT_ERROR_NO_CONTEXT                 = 0x8002a6a3,
	SCE_NP_EXT_ERROR_NO_ORIGIN                  = 0x8002a6a4,
	// NP Community Utility
	SCE_NP_COMMUNITY_ERROR_ALREADY_INITIALIZED          = 0x8002a101,
	SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED              = 0x8002a102,
	SCE_NP_COMMUNITY_ERROR_OUT_OF_MEMORY                = 0x8002a103,
	SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT             = 0x8002a104,
	SCE_NP_COMMUNITY_ERROR_NO_TITLE_SET                 = 0x8002a105,
	SCE_NP_COMMUNITY_ERROR_NO_LOGIN                     = 0x8002a106,
	SCE_NP_COMMUNITY_ERROR_TOO_MANY_OBJECTS             = 0x8002a107,
	SCE_NP_COMMUNITY_ERROR_TRANSACTION_STILL_REFERENCED = 0x8002a108,
	SCE_NP_COMMUNITY_ERROR_ABORTED                      = 0x8002a109,
	SCE_NP_COMMUNITY_ERROR_NO_RESOURCE                  = 0x8002a10a,
	SCE_NP_COMMUNITY_ERROR_BAD_RESPONSE                 = 0x8002a10b,
	SCE_NP_COMMUNITY_ERROR_BODY_TOO_LARGE               = 0x8002a10c,
	SCE_NP_COMMUNITY_ERROR_HTTP_SERVER                  = 0x8002a10d,
	SCE_NP_COMMUNITY_ERROR_INVALID_SIGNATURE            = 0x8002a10e,
	SCE_NP_COMMUNITY_ERROR_TIMEOUT                      = 0x8002a10f,
	SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT        = 0x8002a1a1,
	SCE_NP_COMMUNITY_ERROR_UNKNOWN_TYPE                 = 0x8002a1a2,
	SCE_NP_COMMUNITY_ERROR_INVALID_ID                   = 0x8002a1a3,
	SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID            = 0x8002a1a4,
	SCE_NP_COMMUNITY_ERROR_INVALID_TICKET               = 0x8002a1a5,
	SCE_NP_COMMUNITY_ERROR_CLIENT_HANDLE_ALREADY_EXISTS = 0x8002a1a6,
	SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_BUFFER          = 0x8002a1a7,
	SCE_NP_COMMUNITY_ERROR_INVALID_TYPE                 = 0x8002a1a8,
	SCE_NP_COMMUNITY_ERROR_TRANSACTION_ALREADY_END      = 0x8002a1a9,
	SCE_NP_COMMUNITY_ERROR_TRANSACTION_BEFORE_END       = 0x8002a1aa,
	SCE_NP_COMMUNITY_ERROR_BUSY_BY_ANOTEHR_TRANSACTION  = 0x8002a1ab,
	SCE_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT            = 0x8002a1ac,
	SCE_NP_COMMUNITY_ERROR_TOO_MANY_NPID                = 0x8002a1ad,
	SCE_NP_COMMUNITY_ERROR_TOO_LARGE_RANGE              = 0x8002a1ae,
	SCE_NP_COMMUNITY_ERROR_INVALID_PARTITION            = 0x8002a1af,
	SCE_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID              = 0x8002a1b1,
};

typedef int(*SceNpBasicEventHandler)(s32 event, s32 retCode, u32 reqId, vm::ptr<void> arg);

// NP Manager Utility statuses
enum
{
	SCE_NP_MANAGER_STATUS_OFFLINE         = -1,
	SCE_NP_MANAGER_STATUS_GETTING_TICKET  = 0,
	SCE_NP_MANAGER_STATUS_GETTING_PROFILE = 1,
	SCE_NP_MANAGER_STATUS_LOGGING_IN      = 2,
	SCE_NP_MANAGER_STATUS_ONLINE          = 3,
};

// Event types
enum
{
	SCE_NP_BASIC_EVENT_UNKNOWN                               = -1,
	SCE_NP_BASIC_EVENT_OFFLINE                               = 0,
	SCE_NP_BASIC_EVENT_PRESENCE                              = 1,
	SCE_NP_BASIC_EVENT_MESSAGE                               = 2,
	SCE_NP_BASIC_EVENT_ADD_FRIEND_RESULT                     = 3,
	SCE_NP_BASIC_EVENT_INCOMING_ATTACHMENT                   = 4,
	SCE_NP_BASIC_EVENT_INCOMING_INVITATION                   = 5,
	SCE_NP_BASIC_EVENT_END_OF_INITIAL_PRESENCE               = 6,
	SCE_NP_BASIC_EVENT_SEND_ATTACHMENT_RESULT                = 7,
	SCE_NP_BASIC_EVENT_RECV_ATTACHMENT_RESULT                = 8,
	SCE_NP_BASIC_EVENT_OUT_OF_CONTEXT                        = 9,
	SCE_NP_BASIC_EVENT_FRIEND_REMOVED                        = 10,
	SCE_NP_BASIC_EVENT_ADD_BLOCKLIST_RESULT                  = 11,
	SCE_NP_BASIC_EVENT_SEND_MESSAGE_RESULT                   = 12,
	SCE_NP_BASIC_EVENT_SEND_INVITATION_RESULT                = 13,
	SCE_NP_BASIC_EVENT_RECV_INVITATION_RESULT                = 14,
	SCE_NP_BASIC_EVENT_MESSAGE_MARKED_AS_USED_RESULT         = 15,
	SCE_NP_BASIC_EVENT_INCOMING_CUSTOM_INVITATION            = 16,
	SCE_NP_BASIC_EVENT_INCOMING_CLAN_MESSAGE                 = 17,
	SCE_NP_BASIC_EVENT_ADD_PLAYERS_HISTORY_RESULT            = 18,
	SCE_NP_BASIC_EVENT_SEND_CUSTOM_DATA_RESULT               = 19,
	SCE_NP_BASIC_EVENT_RECV_CUSTOM_DATA_RESULT               = 20,
	SCE_NP_BASIC_EVENT_INCOMING_CUSTOM_DATA_MESSAGE          = 21,
	SCE_NP_BASIC_EVENT_SEND_URL_ATTACHMENT_RESULT            = 22,
	SCE_NP_BASIC_EVENT_INCOMING_BOOTABLE_INVITATION          = 23,
	SCE_NP_BASIC_EVENT_BLOCKLIST_UPDATE                      = 24,
	SCE_NP_BASIC_EVENT_INCOMING_BOOTABLE_CUSTOM_DATA_MESSAGE = 25,
};

// IDs for attachment data objects
enum
{
	SCE_NP_BASIC_INVALID_ATTACHMENT_DATA_ID = 0,
	SCE_NP_BASIC_SELECTED_INVITATION_DATA   = 1,
	SCE_NP_BASIC_SELECTED_MESSAGE_DATA      = 2,
};

// Actions made in system GUI
enum
{
	SCE_NP_BASIC_MESSAGE_ACTION_UNKNOWN = 0,
	SCE_NP_BASIC_MESSAGE_ACTION_USE     = 1,
	SCE_NP_BASIC_MESSAGE_ACTION_ACCEPT  = 2,
	SCE_NP_BASIC_MESSAGE_ACTION_DENY    = 3,
};

// Main types of messages
enum
{
	SCE_NP_BASIC_MESSAGE_MAIN_TYPE_DATA_ATTACHMENT = 0,
	SCE_NP_BASIC_MESSAGE_MAIN_TYPE_GENERAL         = 1,
	SCE_NP_BASIC_MESSAGE_MAIN_TYPE_ADD_FRIEND      = 2,
	SCE_NP_BASIC_MESSAGE_MAIN_TYPE_INVITE          = 3,
	SCE_NP_BASIC_MESSAGE_MAIN_TYPE_CUSTOM_DATA     = 4,
	SCE_NP_BASIC_MESSAGE_MAIN_TYPE_URL_ATTACHMENT  = 5,
};

// Sub types of messages
enum
{
	SCE_NP_BASIC_MESSAGE_DATA_ATTACHMENT_SUBTYPE_ACTION_USE = 0,
	SCE_NP_BASIC_MESSAGE_GENERAL_SUBTYPE_NONE               = 0,
	SCE_NP_BASIC_MESSAGE_ADD_FRIEND_SUBTYPE_NONE            = 0,
	SCE_NP_BASIC_MESSAGE_INVITE_SUBTYPE_ACTION_ACCEPT_DENY  = 0,
	SCE_NP_BASIC_MESSAGE_CUSTOM_DATA_SUBTYPE_ACTION_USE     = 0,
	SCE_NP_BASIC_MESSAGE_URL_ATTACHMENT_SUBTYPE_ACTION_USE  = 0,
	SCE_NP_BASIC_MESSAGE_INVITE_SUBTYPE_ACTION_ACCEPT       = 1,
};

// Applicable features of messages
enum
{
	SCE_NP_BASIC_MESSAGE_FEATURES_MULTI_RECEIPIENTS = 0x00000001,
	SCE_NP_BASIC_MESSAGE_FEATURES_BOOTABLE          = 0x00000002,
	SCE_NP_BASIC_MESSAGE_FEATURES_ASSUME_SEND       = 0x00000004,
};

// Types of messages
enum
{
	SCE_NP_BASIC_MESSAGE_INFO_TYPE_MESSAGE_ATTACHMENT           = 0,
	SCE_NP_BASIC_MESSAGE_INFO_TYPE_MATCHING_INVITATION          = 1,
	SCE_NP_BASIC_MESSAGE_INFO_TYPE_CLAN_MESSAGE                 = 3,
	SCE_NP_BASIC_MESSAGE_INFO_TYPE_CUSTOM_DATA_MESSAGE          = 4,
	SCE_NP_BASIC_MESSAGE_INFO_TYPE_ANY_UNREAD_MESSAGE           = 5,
	SCE_NP_BASIC_MESSAGE_INFO_TYPE_BOOTABLE_INVITATION          = 6,
	SCE_NP_BASIC_MESSAGE_INFO_TYPE_BOOTABLE_CUSTOM_DATA_MESSAGE = 7,
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
	SCE_NP_MATCHING2_ROOMMEMBER_FLAG_ATTR_OWNER = 0x80000000,
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
	SCE_NP_MATCHING2_SIGNALING_EVENT_Dead        = 0x5101,
	SCE_NP_MATCHING2_SIGNALING_EVENT_Established = 0x5102,
};

// Context event
enum
{
	SCE_NP_MATCHING2_CONTEXT_EVENT_StartOver = 0x6f01,
	SCE_NP_MATCHING2_CONTEXT_EVENT_Start     = 0x6f02,
	SCE_NP_MATCHING2_CONTEXT_EVENT_Stop      = 0x6f03,
};

// Context options (signaling)
enum
{
	SCE_NP_SIGNALING_CTX_OPT_BANDWIDTH_PROBE_DISABLE = 0,
	SCE_NP_SIGNALING_CTX_OPT_BANDWIDTH_PROBE_ENABLE  = 1,
	SCE_NP_SIGNALING_CTX_OPT_BANDWIDTH_PROBE         = 1,
};

// Event types (including extended ones)
enum
{
	SCE_NP_SIGNALING_EVENT_DEAD                 = 0,
	SCE_NP_SIGNALING_EVENT_ESTABLISHED          = 1,
	SCE_NP_SIGNALING_EVENT_NETINFO_ERROR        = 2,
	SCE_NP_SIGNALING_EVENT_NETINFO_RESULT       = 3,
	SCE_NP_SIGNALING_EVENT_EXT_PEER_ACTIVATED   = 10,
	SCE_NP_SIGNALING_EVENT_EXT_PEER_DEACTIVATED = 11,
	SCE_NP_SIGNALING_EVENT_EXT_MUTUAL_ACTIVATED = 12,
};

// Connection states
enum
{
	SCE_NP_SIGNALING_CONN_STATUS_INACTIVE = 0,
	SCE_NP_SIGNALING_CONN_STATUS_PENDING  = 1,
	SCE_NP_SIGNALING_CONN_STATUS_ACTIVE   = 2,
};

// Connection information to obtain
enum
{
	SCE_NP_SIGNALING_CONN_INFO_RTT            = 1,
	SCE_NP_SIGNALING_CONN_INFO_BANDWIDTH      = 2,
	SCE_NP_SIGNALING_CONN_INFO_PEER_NPID      = 3,
	SCE_NP_SIGNALING_CONN_INFO_PEER_ADDRESS   = 4,
	SCE_NP_SIGNALING_CONN_INFO_MAPPED_ADDRESS = 5,
	SCE_NP_SIGNALING_CONN_INFO_PACKET_LOSS    = 6,
};

// NAT status type
enum
{
	SCE_NP_SIGNALING_NETINFO_NAT_STATUS_UNKNOWN = 0,
	SCE_NP_SIGNALING_NETINFO_NAT_STATUS_TYPE1   = 1,
	SCE_NP_SIGNALING_NETINFO_NAT_STATUS_TYPE2   = 2,
	SCE_NP_SIGNALING_NETINFO_NAT_STATUS_TYPE3   = 3,
};

// UPnP status
enum
{
	SCE_NP_SIGNALING_NETINFO_UPNP_STATUS_UNKNOWN = 0,
	SCE_NP_SIGNALING_NETINFO_UPNP_STATUS_INVALID = 1,
	SCE_NP_SIGNALING_NETINFO_UPNP_STATUS_VALID   = 2,
};

// NP port status
enum
{
	SCE_NP_SIGNALING_NETINFO_NPPORT_STATUS_CLOSED = 0,
	SCE_NP_SIGNALING_NETINFO_NPPORT_STATUS_OPEN   = 1,
};

// Constants for common NP functions and structures
enum
{
	SCE_NET_NP_AVATAR_IMAGE_MAX_SIZE         = 204800,
	SCE_NET_NP_AVATAR_IMAGE_MAX_SIZE_LARGE   = 204800,
	SCE_NET_NP_AVATAR_IMAGE_MAX_SIZE_MIDDLE  = 102400,
	SCE_NET_NP_AVATAR_IMAGE_MAX_SIZE_SMALL   = 10240,
	SCE_NET_NP_AVATAR_URL_MAX_LENGTH         = 127,
	SCE_NET_NP_ONLINEID_MIN_LENGTH           = 3,
	SCE_NET_NP_ONLINEID_MAX_LENGTH           = 16,
	SCE_NET_NP_ONLINENAME_MAX_LENGTH         = 48,
	SCE_NET_NP_ABOUT_ME_MAX_LENGTH           = 63,
	SCE_NP_FRIEND_MAX_NUM                    = 100,
	SCE_NET_NP_COMMUNICATION_PASSPHRASE_SIZE = 128,
	SCE_NP_COMMUNICATION_SIGNATURE_SIZE      = 160,
	SCE_NP_COMMUNICATION_PASSPHRASE_SIZE     = SCE_NET_NP_COMMUNICATION_PASSPHRASE_SIZE,
};

// Constants for basic NP functions and structures
enum
{
	SCE_NP_BASIC_MAX_MESSAGE_SIZE                       = 512,
	SCE_NP_BASIC_MAX_PRESENCE_SIZE                      = 128,
	SCE_NP_BASIC_MAX_MESSAGE_ATTACHMENT_SIZE            = 1048576,
	SCE_NP_BASIC_SUBJECT_CHARACTER_MAX                  = 18,
	SCE_NP_BASIC_BODY_CHARACTER_MAX                     = 512,
	SCE_NP_BASIC_DESCRIPTION_CHARACTER_MAX              = 341,
	SCE_NP_BASIC_SEND_MESSAGE_MAX_RECIPIENTS            = 12,
	SCE_NP_BASIC_PRESENCE_TITLE_SIZE_MAX                = 128,
	SCE_NP_BASIC_PRESENCE_STATUS_SIZE_MAX               = 64,
	SCE_NP_BASIC_PRESENCE_STATUS_CHARACTER_MAX          = 21,
	SCE_NP_BASIC_PRESENCE_EXTENDED_STATUS_SIZE_MAX      = 192,
	SCE_NP_BASIC_PRESENCE_EXTENDED_STATUS_CHARACTER_MAX = 63,
	SCE_NP_BASIC_PRESENCE_COMMENT_SIZE_MAX              = 64,
	SCE_NP_BASIC_MAX_INVITATION_DATA_SIZE               = 1024,
	SCE_NP_BASIC_MAX_URL_ATTACHMENT_SIZE                = 2048,
	SCE_NP_BASIC_PLAYER_HISTORY_MAX_PLAYERS             = 8,
};

// Common constants of sceNpClans
enum
{
	SCE_NP_CLANS_CLAN_NAME_MAX_LENGTH        = 64,
	SCE_NP_CLANS_CLAN_TAG_MAX_LENGTH         = 8,
	SCE_NP_CLANS_CLAN_DESCRIPTION_MAX_LENGTH = 255,
};

// Constants for custom menu functions and structures
enum
{
	SCE_NP_CUSTOM_MENU_ACTION_CHARACTER_MAX = 21,
	SCE_NP_CUSTOM_MENU_ACTION_ITEMS_MAX = 7,
	SCE_NP_CUSTOM_MENU_ACTION_ITEMS_TOTAL_MAX = 16,
	SCE_NP_CUSTOM_MENU_EXCEPTION_ITEMS_MAX = 256,
};

// Constants for manager functions and structures
enum
{
	SCE_NP_COOKIE_MAX_SIZE       = 1024,
	SCE_NP_TICKET_MAX_SIZE       = 65536,
	SCE_NP_TICKET_PARAM_DATA_LEN = 256,
	SCE_NP_ENTITLEMENT_ID_SIZE   = 32,
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
	SCE_NP_MATCHING2_ROOM_SEARCHABLE_BIN_ATTR_EXTERNAL_MAX_SIZE            = 64,
	SCE_NP_MATCHING2_ROOMMEMBER_BIN_ATTR_INTERNAL_NUM                      = 1,
	SCE_NP_MATCHING2_ROOMMEMBER_BIN_ATTR_INTERNAL_MAX_SIZE                 = 64,
	SCE_NP_MATCHING2_SESSION_PASSWORD_SIZE                                 = 8,
	SCE_NP_MATCHING2_USER_BIN_ATTR_NUM                                     = 1,
	SCE_NP_MATCHING2_USER_BIN_ATTR_MAX_SIZE                                = 128,
	SCE_NP_MATCHING2_GET_USER_INFO_LIST_NPID_NUM_MAX                       = 25,
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

// Constants for ranking (score) functions and structures
enum
{
	SCE_NP_SCORE_COMMENT_MAXLEN          = 63,
	SCE_NP_SCORE_CENSOR_COMMENT_MAXLEN   = 255,
	SCE_NP_SCORE_SANITIZE_COMMENT_MAXLEN = 255,
	SCE_NP_SCORE_GAMEINFO_SIZE           = 64,
	SCE_NP_SCORE_MAX_CTX_NUM             = 32,
	SCE_NP_SCORE_MAX_RANGE_NUM_PER_TRANS = 100,
	SCE_NP_SCORE_MAX_NPID_NUM_PER_TRANS  = 101,
	SCE_NP_SCORE_MAX_CLAN_NUM_PER_TRANS  = 101,
};

// Constants for signaling functions and structures
enum
{
	SCE_NP_SIGNALING_CTX_MAX = 8,
};

// NP communication ID structure
struct SceNpCommunicationId
{
	s8 data[9];
	s8 term;
	u8 num;
	//s8 dummy;
};

// OnlineId structure
struct SceNpOnlineId
{
	s8 data[16];
	s8 term;
	//s8 dummy[3];
};

// NP ID structure
struct SceNpId
{
	SceNpOnlineId handle;
	//u8 opt[8];
	//u8 reserved[8];
};

// Online Name structure
struct SceNpOnlineName
{
	s8 data[48];
	s8 term;
	s8 padding[3];
};

// Avatar structure
struct SceNpAvatarUrl
{
	s8 data[127];
	s8 term;
};

// Avatar image structure
struct SceNpAvatarImage
{
	u8 data[SCE_NET_NP_AVATAR_IMAGE_MAX_SIZE];
	be_t<u32> size;
	//u8 reserved[12];
};

// Self introduction structure
struct SceNpAboutMe
{
	s8 data[SCE_NET_NP_ABOUT_ME_MAX_LENGTH];
	s8 term;
};

// User information structure
struct SceNpUserInfo
{
	SceNpId userId;
	SceNpOnlineName name;
	SceNpAvatarUrl icon;
};

// User information structure (pointer version)
struct SceNpUserInfo2
{
	SceNpId npId;
	SceNpOnlineName onlineName;
	SceNpAvatarUrl avatarUrl;
};

// Often used languages structure
struct SceNpMyLanguages
{
	be_t<s32> language1;
	be_t<s32> language2;
	be_t<s32> language3;
	u8 padding[4];
};

// NP communication passphrase
struct SceNpCommunicationPassphrase
{
	u8 data[SCE_NP_COMMUNICATION_PASSPHRASE_SIZE];
};

// NP communication signature
struct SceNpCommunicationSignature
{
	u8 data[SCE_NP_COMMUNICATION_SIGNATURE_SIZE];
};

// NP cache information structure
struct SceNpManagerCacheParam
{
	SceNpOnlineId onlineId;
	SceNpId npId;
	SceNpOnlineName onlineName;
	SceNpAvatarUrl avatarUrl;
};

// Message attachment data
struct SceNpBasicAttachmentData
{
	be_t<u32> id;
	be_t<u32> size;
};

// Message extended attachment data
struct SceNpBasicExtendedAttachmentData
{
	be_t<u64> flags;
	be_t<u64> msgId;
	SceNpBasicAttachmentData data;
	be_t<u32> userAction;
	bool markedAsUsed;
	//be_t<u8> reserved[3];
};

// Message structure
struct SceNpBasicMessageDetails
{
	be_t<u64> msgId;
	be_t<u16> mainType;
	be_t<u16> subType;
	be_t<u32> msgFeatures;
	const SceNpId npids;
	be_t<u32> count;
	const s8 subject;
	const s8 body;
	const be_t<u32> data;
	be_t<u32> size;
};

// Presence details of an user
struct SceNpBasicPresenceDetails
{
	s8 title[SCE_NP_BASIC_PRESENCE_TITLE_SIZE_MAX];
	s8 status[SCE_NP_BASIC_PRESENCE_STATUS_SIZE_MAX];
	s8 comment[SCE_NP_BASIC_PRESENCE_COMMENT_SIZE_MAX];
	u8 data[SCE_NP_BASIC_MAX_PRESENCE_SIZE];
	be_t<u32> size;
	be_t<s32> state;
};

// Extended presence details of an user
struct SceNpBasicPresenceDetails2
{
	be_t<u32> struct_size;
	be_t<s32> state;
	s8 title[SCE_NP_BASIC_PRESENCE_TITLE_SIZE_MAX];
	s8 status[SCE_NP_BASIC_PRESENCE_EXTENDED_STATUS_SIZE_MAX];
	s8 comment[SCE_NP_BASIC_PRESENCE_COMMENT_SIZE_MAX];
	u8 data[SCE_NP_BASIC_MAX_PRESENCE_SIZE];
	be_t<u32> size;
};

// Country/region code
struct SceNpCountryCode
{
	s8 data[2];
	s8 term;
	s8 padding[1];
};

// Date information
struct SceNpDate
{
	be_t<u16> year;
	u8 month;
	u8 day;
};

// Entitlement ID (fixed-length)
struct SceNpEntitlementId
{
	u8 data[SCE_NP_ENTITLEMENT_ID_SIZE]; // Unsigned char? What is the right type...?
};

// Callback for getting the connection status
typedef void(SceNpManagerCallback)(s32 event, s32 result, u32 arg_addr);

// Request callback function
typedef void(*SceNpMatching2RequestCallback)(u16 ctxId, u32 reqId, u16 event,
	                                         u32 eventKey, s32 errorCode, u32 dataSize, u32 *arg
	);

// NOTE: Use SceNpCommunicationPassphrase instead
// Np communication passphrase
//SceNpCommunicationPassphrase SceNpMatching2TitlePassphrase;

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
	be_t<u16> id;
	u8 padding[2];
	be_t<u32> num;
};

// Binary-type attribute
struct SceNpMatching2BinAttr
{
	be_t<u16> id;
	u8 padding[2];
	be_t<u32> ptr;
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
	u8 searchOperator;
	u8 padding[3];
	SceNpMatching2IntAttr attr;
};

// Binary-type search condition
struct SceNpMatching2BinSearchFilter
{
	u8 searchOperator;
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
	be_t<u16> serverId;
	be_t<u32> worldId;
	be_t<u64> lobbyId;
	be_t<u64> roomId;
	CellRtcTick joinDate;
};

// User information
struct SceNpMatching2UserInfo
{
	SceNpMatching2UserInfo *next;
	SceNpUserInfo2 userInfo;
	SceNpMatching2BinAttr *userBinAttr;
	be_t<u32> userBinAttrNum;
	SceNpMatching2JoinedSessionInfo joinedSessionInfo;
	be_t<u32> joinedSessionInfoNum;
};

// Server
struct SceNpMatching2Server
{
	be_t<u16> serverId;
	u8 status;
	u8 padding[1];
};

// World
struct SceNpMatching2World
{
	be_t<u32> worldId;
	be_t<u32> numOfLobby;
	be_t<u32> maxNumOfTotalLobbyMember;
	be_t<u32> curNumOfTotalLobbyMember;
	be_t<u32> curNumOfRoom;
	be_t<u32> curNumOfTotalRoomMember;
	bool withEntitlementId;
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
	SceNpMatching2LobbyMemberDataInternal *next;
	SceNpUserInfo2 userInfo;
	CellRtcTick joinDate;
	be_t<u16> memberId;
	u8 padding[2];
	be_t<u32> flagAttr;
	SceNpMatching2JoinedSessionInfo joinedSessionInfo;
	be_t<u32> joinedSessionInfoNum;
	SceNpMatching2LobbyMemberBinAttrInternal lobbyMemberBinAttrInternal;
	be_t<u32> lobbyMemberBinAttrInternalNum; // Unsigned ints are be_t<u32> not uint, right?
};

// Lobby member ID list
struct SceNpMatching2LobbyMemberIdList
{
	be_t<u16> memberId;
	be_t<u32> memberIdNum;
	be_t<u16> me;
	u8 padding[6];
};

// Lobby-internal binary attribute
struct SceNpMatching2LobbyBinAttrInternal
{
	CellRtcTick updateDate;
	be_t<u16> updateMemberId;
	u8 padding[2];
	SceNpMatching2BinAttr data;
};

// Lobby-external lobby information
struct SceNpMatching2LobbyDataExternal
{
	SceNpMatching2LobbyDataExternal *next;
	be_t<u16> serverId;
	u8 padding1[2];
	be_t<u32> worldId;
	u8 padding2[4];
	be_t<u64> lobbyId;
	be_t<u32> maxSlot;
	be_t<u32> curMemberNum;
	be_t<u32> flagAttr;
	SceNpMatching2IntAttr lobbySearchableIntAttrExternal;
	be_t<u32> lobbySearchableIntAttrExternalNum;
	SceNpMatching2BinAttr lobbySearchableBinAttrExternal;
	be_t<u32> lobbySearchableBinAttrExternalNum;
	SceNpMatching2BinAttr lobbyBinAttrExternal;
	be_t<u32> lobbyBinAttrExternalNum;
	u8 padding3[4];
};

// Lobby-internal lobby information
struct SceNpMatching2LobbyDataInternal
{
	be_t<u16> serverId;
	u8 padding1[2];
	be_t<u32> worldId;
	be_t<u64> lobbyId;
	be_t<u32> maxSlot;
	SceNpMatching2LobbyMemberIdList memberIdList;
	be_t<u32> flagAttr;
	SceNpMatching2LobbyBinAttrInternal lobbyBinAttrInternal;
	be_t<u32> lobbyBinAttrInternalNum;
};

// Lobby message transmission destination
union SceNpMatching2LobbyMessageDestination
{
	be_t<u16> unicastTarget;
	struct multicastTarget {
		be_t<u16> *memberId;
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
	bool withLabel;
	SceNpMatching2GroupLabel label;
	bool withPassword;
	u8 padding[2];
};

// Set group password
struct SceNpMatching2RoomGroupPasswordConfig
{
	u8 groupId;
	bool withPassword;
	u8 padding[1];
};

// Group (of slots in a room)
struct SceNpMatching2RoomGroup
{
	u8 groupId;
	bool withPassword;
	bool withLabel;
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
	SceNpMatching2RoomMemberDataExternal *next;
	SceNpUserInfo2 userInfo;
	CellRtcTick joinDate;
	u8 role;
	u8 padding[7];
};

// Internal room member data
struct SceNpMatching2RoomMemberDataInternal
{
	SceNpMatching2RoomMemberDataInternal *next;
	SceNpUserInfo2 userInfo;
	CellRtcTick joinDate;
	be_t<u16> memberId;
	u8 teamId;
	u8 padding1[1];
	SceNpMatching2RoomGroup roomGroup;
	u8 natType;
	u8 padding2[3];
	be_t<u32> flagAttr;
	SceNpMatching2RoomMemberBinAttrInternal roomMemberBinAttrInternal;
	be_t<u32> roomMemberBinAttrInternalNum;
};

// Internal room member data list
struct SceNpMatching2RoomMemberDataInternalList
{
	SceNpMatching2RoomMemberDataInternal members;
	be_t<u32> membersNum;
	SceNpMatching2RoomMemberDataInternal me;
	SceNpMatching2RoomMemberDataInternal owner;
};

// Internal room binary attribute
struct SceNpMatching2RoomBinAttrInternal
{
	CellRtcTick updateDate;
	be_t<u16> updateMemberId;
	u8 padding[2];
	SceNpMatching2BinAttr data;
};

// External room data
struct SceNpMatching2RoomDataExternal
{
	SceNpMatching2RoomDataExternal *next;
	be_t<u16> serverId;
	u8 padding1[2];
	be_t<u32> worldId;
	be_t<u16> publicSlotNum;
	be_t<u16> privateSlotNum;
	be_t<u64> lobbyId;
	be_t<u64> roomId;
	be_t<u16> openPublicSlotNum;
	be_t<u16> maxSlot;
	be_t<u16> openPrivateSlotNum;
	be_t<u16> curMemberNum;
	be_t<u64> passwordSlotMask;
	SceNpUserInfo2 owner;
	SceNpMatching2RoomGroup roomGroup;
	be_t<u32> roomGroupNum;
	be_t<u32> flagAttr;
	SceNpMatching2IntAttr roomSearchableIntAttrExternal;
	be_t<u32> roomSearchableIntAttrExternalNum;
	SceNpMatching2BinAttr roomSearchableBinAttrExternal;
	be_t<u32> roomSearchableBinAttrExternalNum;
	SceNpMatching2BinAttr roomBinAttrExternal;
	be_t<u32> roomBinAttrExternalNum;
};

// Internal room data
struct SceNpMatching2RoomDataInternal
{
	be_t<u16> serverId;
	u8 padding1[2];
	be_t<u32> worldId;
	be_t<u64> lobbyId;
	be_t<u64> roomId;
	be_t<u64> passwordSlotMask;
	be_t<u32> maxSlot;
	SceNpMatching2RoomMemberDataInternalList memberList;
	SceNpMatching2RoomGroup *roomGroup;
	be_t<u32> roomGroupNum;
	be_t<u32> flagAttr;
	SceNpMatching2RoomBinAttrInternal *roomBinAttrInternal;
	be_t<u32> roomBinAttrInternalNum;
};

// Room message recipient
union SceNpMatching2RoomMessageDestination
{
	be_t<u16> unicastTarget;
	struct multicastTarget {
		be_t<u16> memberId;
		be_t<u32> memberIdNum;
	};
	u8 multicastTargetTeamId;
};

// Invitation data
struct SceNpMatching2InvitationData
{
	SceNpMatching2JoinedSessionInfo targetSession;
	be_t<u32> targetSessionNum;
	be_t<u32> optData;
	be_t<u32> optDataLen;
};

// Signaling option parameter
struct SceNpMatching2SignalingOptParam
{
	u8 type;
	u8 reserved1[1];
	be_t<u16> hubMemberId;
	//u8 reserved2[4];
};

// Option parameters for requests
struct SceNpMatching2RequestOptParam
{
	SceNpMatching2RequestCallback cbFunc;
	be_t<u32> *cbFuncArg;
	be_t<u32> timeout;
	be_t<u16> appReqId;
	u8 padding[2];
};

// Room slot information
struct SceNpMatching2RoomSlotInfo
{
	be_t<u64> roomId;
	be_t<u64> joinedSlotMask;
	be_t<u64> passwordSlotMask;
	be_t<u16> publicSlotNum;
	be_t<u16> privateSlotNum;
	be_t<u16> openPublicSlotNum;
	be_t<u16> openPrivateSlotNum;
};

// Server data request parameter
struct SceNpMatching2GetServerInfoRequest
{
	be_t<u16> serverId;
};

// Server data request response data
struct SceNpMatching2GetServerInfoResponse
{
	SceNpMatching2Server server;
};

// Request parameter for creating a server context
struct SceNpMatching2CreateServerContextRequest
{
	be_t<u16> serverId;
};

// Request parameter for deleting a server context
struct SceNpMatching2DeleteServerContextRequest
{
	be_t<u16> serverId;
};

// World data list request parameter
struct SceNpMatching2GetWorldInfoListRequest
{
	be_t<u16> serverId;
};

// World data list request response data
struct SceNpMatching2GetWorldInfoListResponse
{
	SceNpMatching2World world;
	be_t<u32> worldNum;
};

// User information setting request parameter
struct SceNpMatching2SetUserInfoRequest
{
	be_t<u16> serverId;
	u8 padding[2];
	SceNpMatching2BinAttr *userBinAttr;
	be_t<u32> userBinAttrNum;
};

// User information list acquisition request parameter
struct SceNpMatching2GetUserInfoListRequest
{
	be_t<u16> serverId;
	u8 padding[2];
	SceNpId npId;
	be_t<u32> npIdNum;
	be_t<u16> attrId;
	be_t<u32> attrIdNum;
	be_t<s32> option; // int should be be_t<s32>, right?
};

// User information list acquisition response data
struct SceNpMatching2GetUserInfoListResponse
{
	SceNpMatching2UserInfo userInfo;
	be_t<u32> userInfoNum;
};

// External room member data list request parameter
struct SceNpMatching2GetRoomMemberDataExternalListRequest
{
	be_t<u64> roomId;
};

// External room member data list request response data
struct SceNpMatching2GetRoomMemberDataExternalListResponse
{
	SceNpMatching2RoomMemberDataExternal roomMemberDataExternal;
	be_t<u32> roomMemberDataExternalNum;
};

// External room data configuration request parameters
struct SceNpMatching2SetRoomDataExternalRequest
{
	be_t<u64> roomId;
	SceNpMatching2IntAttr roomSearchableIntAttrExternal;
	be_t<u32> roomSearchableIntAttrExternalNum;
	SceNpMatching2BinAttr roomSearchableBinAttrExternal;
	be_t<u32> roomSearchableBinAttrExternalNum;
	SceNpMatching2BinAttr roomBinAttrExternal;
	be_t<u32> roomBinAttrExternalNum;
};

// External room data list request parameters
struct SceNpMatching2GetRoomDataExternalListRequest
{
	be_t<u64> roomId;
	be_t<u32> roomIdNum;
	be_t<u16> attrId;
	be_t<u32> attrIdNum;
};

// External room data list request response data
struct SceNpMatching2GetRoomDataExternalListResponse
{
	SceNpMatching2RoomDataExternal roomDataExternal;
	be_t<u32> roomDataExternalNum;
};

// Create-and-join room request parameters
struct SceNpMatching2CreateJoinRoomRequest
{
	be_t<u32> worldId;
	u8 padding1[4];
	be_t<u64> lobbyId;
	be_t<u32> maxSlot;
	be_t<u32> flagAttr;
	SceNpMatching2BinAttr roomBinAttrInternal;
	be_t<u32> roomBinAttrInternalNum;
	SceNpMatching2IntAttr roomSearchableIntAttrExternal;
	be_t<u32> roomSearchableIntAttrExternalNum;
	SceNpMatching2BinAttr roomSearchableBinAttrExternal;
	be_t<u32> roomSearchableBinAttrExternalNum;
	SceNpMatching2BinAttr roomBinAttrExternal;
	be_t<u32> roomBinAttrExternalNum;
	SceNpMatching2SessionPassword roomPassword;
	SceNpMatching2RoomGroupConfig groupConfig;
	be_t<u32> groupConfigNum;
	be_t<u64> passwordSlotMask;
	SceNpId allowedUser;
	be_t<u32> allowedUserNum;
	SceNpId blockedUser;
	be_t<u32> blockedUserNum;
	SceNpMatching2GroupLabel joinRoomGroupLabel;
	SceNpMatching2BinAttr roomMemberBinAttrInternal;
	be_t<u32> roomMemberBinAttrInternalNum;
	u8 teamId;
	u8 padding2[3];
	SceNpMatching2SignalingOptParam sigOptParam;
	u8 padding3[4];
};

// Create-and-join room request response data
struct SceNpMatching2CreateJoinRoomResponse
{
	SceNpMatching2RoomDataInternal roomDataInternal;
};

// Join room request parameters
struct SceNpMatching2JoinRoomRequest
{
	be_t<u64> roomId;
	SceNpMatching2SessionPassword roomPassword;
	SceNpMatching2GroupLabel joinRoomGroupLabel;
	SceNpMatching2BinAttr roomMemberBinAttrInternal;
	be_t<u32> roomMemberBinAttrInternalNum;
	SceNpMatching2PresenceOptionData optData;
	u8 teamId;
	u8 padding[3];
};

// Join room request response data
struct SceNpMatching2JoinRoomResponse
{
	SceNpMatching2RoomDataInternal roomDataInternal;
};

// Leave room request parameters
struct SceNpMatching2LeaveRoomRequest
{
	be_t<u64> roomId;
	SceNpMatching2PresenceOptionData optData;
	u8 padding[4];
};

// Room ownership grant request parameters
struct SceNpMatching2GrantRoomOwnerRequest
{
	be_t<u64> roomId;
	be_t<u16> newOwner;
	u8 padding[2];
	SceNpMatching2PresenceOptionData optData;
};

// Kickout request parameters
struct SceNpMatching2KickoutRoomMemberRequest
{
	be_t<u64> roomId;
	be_t<u16> target;
	u8 blockKickFlag;
	u8 padding[1];
	SceNpMatching2PresenceOptionData optData;
};

// Room search parameters
struct SceNpMatching2SearchRoomRequest
{
	be_t<s32> option;
	be_t<u32> worldId;
	be_t<u64> lobbyId;
	SceNpMatching2RangeFilter rangeFilter;
	be_t<u32> flagFilter;
	be_t<u32> flagAttr;
	SceNpMatching2IntSearchFilter intFilter;
	be_t<u32> intFilterNum;
	SceNpMatching2BinSearchFilter binFilter;
	be_t<u32> binFilterNum;
	be_t<u16> attrId;
	be_t<u32> attrIdNum;
};

// Room search response data
struct SceNpMatching2SearchRoomResponse
{
	SceNpMatching2Range range;
	SceNpMatching2RoomDataExternal roomDataExternal;
};

// Room message send request parameters
struct SceNpMatching2SendRoomMessageRequest
{
	be_t<u64> roomId;
	u8 castType;
	u8 padding[3];
	SceNpMatching2RoomMessageDestination dst;
	be_t<u32> msg; // const void = be_t<u32>, right?
	be_t<u32> msgLen;
	be_t<s32> option; // int = be_t<s32>, right?
};

// Room chat message send request parameters
struct SceNpMatching2SendRoomChatMessageRequest
{
	be_t<u64> roomId;
	u8 castType;
	u8 padding[3];
	SceNpMatching2RoomMessageDestination dst;
	be_t<u32> msg;
	be_t<u32> msgLen;
	be_t<s32> option;
};

// Room chat message send request response data
struct SceNpMatching2SendRoomChatMessageResponse
{
	bool filtered;
};

// Internal room data configuration request parameters
struct SceNpMatching2SetRoomDataInternalRequest
{
	be_t<u64> roomId;
	be_t<u32> flagFilter;
	be_t<u32> flagAttr;
	SceNpMatching2BinAttr roomBinAttrInternal;
	be_t<u32> roomBinAttrInternalNum;
	SceNpMatching2RoomGroupPasswordConfig passwordConfig;
	be_t<u32> passwordConfigNum;
	be_t<u64> passwordSlotMask;
	be_t<u16> ownerPrivilegeRank;
	be_t<u32> ownerPrivilegeRankNum;
	u8 padding[4];
};

// Internal room data request parameters
struct SceNpMatching2GetRoomDataInternalRequest
{
	be_t<u64> roomId;
	be_t<u16> attrId;
	be_t<u32> attrIdNum;
};

// Internal room data request response data
struct SceNpMatching2GetRoomDataInternalResponse
{
	SceNpMatching2RoomDataInternal roomDataInternal;
};

// Internal room member data configuration request parameters
struct SceNpMatching2SetRoomMemberDataInternalRequest
{
	be_t<u64> roomId;
	be_t<u16> memberId;
	u8 teamId;
	u8 padding[5];
	be_t<u32> flagFilter;
	be_t<u32> flagAttr;
	SceNpMatching2BinAttr roomMemberBinAttrInternal;
	be_t<u32> roomMemberBinAttrInternalNum;
};

// Internal room member data request parameters
struct SceNpMatching2GetRoomMemberDataInternalRequest
{
	be_t<u64> roomId;
	be_t<u16> memberId;
	u8 padding[6];
	be_t<u16> attrId;
	be_t<u32> attrIdNum;
};

// Internal room member data request response data
struct SceNpMatching2GetRoomMemberDataInternalResponse
{
	SceNpMatching2RoomMemberDataInternal roomMemberDataInternal;
};

// Signaling option parameter setting request parameter
struct SceNpMatching2SetSignalingOptParamRequest
{
	be_t<u64> roomId;
	SceNpMatching2SignalingOptParam sigOptParam;
};

// Lobby information list acquisition request parameter
struct SceNpMatching2GetLobbyInfoListRequest
{
	be_t<u32> worldId;
	SceNpMatching2RangeFilter rangeFilter;
	be_t<u16> attrId;
	be_t<u32> attrIdNum;
};

// Lobby information list acquisition response data
struct SceNpMatching2GetLobbyInfoListResponse
{
	SceNpMatching2Range range;
	SceNpMatching2LobbyDataExternal lobbyDataExternal;
};

// Lobby joining request parameter
struct SceNpMatching2JoinLobbyRequest
{
	be_t<u64> lobbyId;
	SceNpMatching2JoinedSessionInfo joinedSessionInfo;
	be_t<u32> joinedSessionInfoNum;
	SceNpMatching2BinAttr lobbyMemberBinAttrInternal;
	be_t<u32> lobbyMemberBinAttrInternalNum;
	SceNpMatching2PresenceOptionData optData;
	u8 padding[4];
};

// Lobby joining response data
struct SceNpMatching2JoinLobbyResponse
{
	SceNpMatching2LobbyDataInternal lobbyDataInternal;
};

// Lobby leaving request parameter
struct SceNpMatching2LeaveLobbyRequest
{
	be_t<u64> lobbyId;
	SceNpMatching2PresenceOptionData optData;
	u8 padding[4];
};

// Lobby chat message sending request parameter
struct SceNpMatching2SendLobbyChatMessageRequest
{
	be_t<u64> lobbyId;
	u8 castType;
	u8 padding[3];
	SceNpMatching2LobbyMessageDestination dst;
	be_t<u32> msg;
	be_t<u32> msgLen;
	be_t<s32> option;
};

// Lobby chat message sending response data
struct SceNpMatching2SendLobbyChatMessageResponse
{
	bool filtered;
};

// Lobby invitation message sending request parameter
struct SceNpMatching2SendLobbyInvitationRequest
{
	be_t<u64> lobbyId;
	u8 castType;
	u8 padding[3];
	SceNpMatching2LobbyMessageDestination dst;
	SceNpMatching2InvitationData invitationData;
	be_t<s32> option;
};

// Lobby-internal lobby member information setting request parameter
struct SceNpMatching2SetLobbyMemberDataInternalRequest
{
	be_t<u64> lobbyId;
	be_t<u16> memberId;
	u8 padding1[2];
	be_t<u32> flagFilter;
	be_t<u32> flagAttr;
	SceNpMatching2JoinedSessionInfo *joinedSessionInfo;
	be_t<u32> joinedSessionInfoNum;
	SceNpMatching2BinAttr lobbyMemberBinAttrInternal;
	be_t<u32> lobbyMemberBinAttrInternalNum;
	u8 padding2[4];
};

// Lobby-internal lobby member information acquisition request parameter
struct SceNpMatching2GetLobbyMemberDataInternalRequest
{
	be_t<u64> lobbyId;
	be_t<u16> memberId;
	u8 padding[6];
	be_t<u16> attrId;
	be_t<u32> attrIdNum;
};

// Lobby-internal lobby member information acquisition response data
struct SceNpMatching2GetLobbyMemberDataInternalResponse
{
	SceNpMatching2LobbyMemberDataInternal lobbyMemberDataInternal;
};

// Request parameters for obtaining a list of lobby-internal lobby member information
struct SceNpMatching2GetLobbyMemberDataInternalListRequest
{
	be_t<u64> lobbyId;
	be_t<u16> memberId;
	be_t<u32> memberIdNum;
	be_t<u16> attrId;
	be_t<u32> attrIdNum;
	bool extendedData;
	u8 padding[7];
};

// Reponse data for obtaining a list of lobby-internal lobby member information
struct SceNpMatching2GetLobbyMemberDataInternalListResponse
{
	SceNpMatching2LobbyMemberDataInternal lobbyMemberDataInternal;
	be_t<u32> lobbyMemberDataInternalNum;
};

// Request parameters for obtaining Ping information
struct SceNpMatching2SignalingGetPingInfoRequest
{
	be_t<u64> roomId;
	//u8 reserved[16];
};

// Response data for obtaining Ping information
struct SceNpMatching2SignalingGetPingInfoResponse
{
	be_t<u16> serverId;
	u8 padding1[2];
	be_t<u32> worldId;
	be_t<u64> roomId;
	be_t<u32> rtt;
	//u8 reserved[20];
};

// Join request parameters for room in prohibitive mode
struct SceNpMatching2JoinProhibitiveRoomRequest
{
	SceNpMatching2JoinRoomRequest joinParam;
	SceNpId blockedUser;
	be_t<u32> blockedUserNum;
};

// Room member update information
struct SceNpMatching2RoomMemberUpdateInfo
{
	SceNpMatching2RoomMemberDataInternal roomMemberDataInternal;
	u8 eventCause;
	u8 padding[3];
	SceNpMatching2PresenceOptionData optData;
};

// Room owner update information
struct SceNpMatching2RoomOwnerUpdateInfo
{
	be_t<u16> prevOwner;
	be_t<u16> newOwner;
	u8 eventCause;
	u8 padding[3];
	SceNpMatching2SessionPassword roomPassword;
	SceNpMatching2PresenceOptionData optData;
};

// Room update information
struct SceNpMatching2RoomUpdateInfo
{
	u8 eventCause;
	u8 padding[3];
	be_t<s32> errorCode;
	SceNpMatching2PresenceOptionData optData;
};

// Internal room data update information
struct SceNpMatching2RoomDataInternalUpdateInfo
{
	SceNpMatching2RoomDataInternal newRoomDataInternal;
	be_t<u32> newFlagAttr;
	be_t<u32> prevFlagAttr;
	be_t<u64> newRoomPasswordSlotMask;
	be_t<u64> prevRoomPasswordSlotMask;
	SceNpMatching2RoomGroup newRoomGroup;
	be_t<u32> newRoomGroupNum;
	SceNpMatching2RoomBinAttrInternal newRoomBinAttrInternal;
	be_t<u32> newRoomBinAttrInternalNum;
};

// Internal room member data update information
struct SceNpMatching2RoomMemberDataInternalUpdateInfo
{
	SceNpMatching2RoomMemberDataInternal newRoomMemberDataInternal;
	be_t<u16> newFlagAttr;
	be_t<u16> prevFlagAttr;
	u8 newTeamId;
	SceNpMatching2RoomMemberBinAttrInternal newRoomMemberBinAttrInternal;
	be_t<u32> newRoomMemberBinAttrInternalNum;
};

// Room message information
struct SceNpMatching2RoomMessageInfo
{
	bool filtered;
	u8 castType;
	u8 padding[2];
	SceNpMatching2RoomMessageDestination dst;
	SceNpUserInfo2 srcMember;
	be_t<u32> msg;
	be_t<u32> msgLen;
};

// Lobby member update information
struct SceNpMatching2LobbyMemberUpdateInfo
{
	SceNpMatching2LobbyMemberDataInternal *lobbyMemberDataInternal;
	u8 eventCause;
	u8 padding[3];
	SceNpMatching2PresenceOptionData optData;
};

// Lobby update information
struct SceNpMatching2LobbyUpdateInfo
{
	u8 eventCause;
	u8 padding[3];
	be_t<s32> errorCode;
};

// Lobby-internal lobby member information update information
struct SceNpMatching2LobbyMemberDataInternalUpdateInfo
{
	be_t<u16> memberId;
	u8 padding[2];
	SceNpId npId;
	be_t<u16> flagFilter;
	be_t<u16> newFlagAttr;
	SceNpMatching2JoinedSessionInfo newJoinedSessionInfo;
	be_t<u32> newJoinedSessionInfoNum;
	SceNpMatching2LobbyMemberBinAttrInternal newLobbyMemberBinAttrInternal;
	be_t<u32> newLobbyMemberBinAttrInternalNum;
};

// Lobby message information
struct SceNpMatching2LobbyMessageInfo
{
	bool filtered;
	u8 castType;
	u8 padding[2];
	SceNpMatching2LobbyMessageDestination dst;
	SceNpUserInfo2 srcMember;
	be_t<u32> msg;
	be_t<u32> msgLen;
};

// Lobby invitation message information
struct SceNpMatching2LobbyInvitationInfo
{
	u8 castType;
	u8 padding[3];
	SceNpMatching2LobbyMessageDestination dst;
	SceNpUserInfo2 srcMember;
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
	be_t<u32> sessionEventCbQueueLen;;
	be_t<u32> sessionMsgCbQueueLen;;
	//u8 reserved[16];
};

// Matching2 memory information
struct SceNpMatching2MemoryInfo
{
	be_t<u32> totalMemSize;
	be_t<u32> curMemUsage;;
	be_t<u32> maxMemUsage;;
	//u8 reserved[12];
};

// Matching2 information on the event data queues in the system
struct SceNpMatching2CbQueueInfo
{
	be_t<u32> requestCbQueueLen;
	be_t<u32> curRequestCbQueueLen;;
	be_t<u32> maxRequestCbQueueLen;;
	be_t<u32> sessionEventCbQueueLen;;
	be_t<u32> curSessionEventCbQueueLen;;
	be_t<u32> maxSessionEventCbQueueLen;;
	be_t<u32> sessionMsgCbQueueLen;;
	be_t<u32> curSessionMsgCbQueueLen;;
	be_t<u32> maxSessionMsgCbQueueLen;;
	//u8 reserved[12];
};

// Score data unique to the application
struct SceNpScoreGameInfo
{
	u8 nativeData[SCE_NP_SCORE_GAMEINFO_SIZE];
};

// Ranking comment structure
struct SceNpScoreComment
{
	s8 data[SCE_NP_SCORE_COMMENT_MAXLEN];
	s8 term[1];
};

// Ranking information structure
struct SceNpScoreRankData
{
	SceNpId npId;
	SceNpOnlineName onlineName;
	be_t<s32> pcId;
	be_t<u32> serialRank;
	be_t<u32> rank;
	be_t<u32> highestRank;
	be_t<s64> scoreValue;
	be_t<s32> hasGameData;
	u8 pad0[4];
	CellRtcTick recordDate;
};

// Ranking information of a player or a clan member
struct SceNpScorePlayerRankData
{
	be_t<s32> hasData;
	u8 pad0[4];
	SceNpScoreRankData rankData;
};

// Scoreboard information
struct SceNpScoreBoardInfo
{
	be_t<u32> rankLimit;
	be_t<u32> updateMode;
	be_t<u32> sortMode;
	be_t<u32> uploadNumLimit;
	be_t<u32> uploadSizeLimit;
};

// NOTE: Use SceNpCommunicationPassphrase instead
// Authentication information per NP Communication ID for score ranking
// SceNpCommunicationPassphrase SceNpScorePassphrase;

// NP ID structure with player character ID
struct SceNpScoreNpIdPcId
{
	SceNpId npId;
	be_t<s32> pcId;
	//u8 pad[4];
};

// Basic clan information to be used in raking
struct SceNpScoreClanBasicInfo
{
	s8 clanName[SCE_NP_CLANS_CLAN_NAME_MAX_LENGTH + 1];
	s8 clanTag[SCE_NP_CLANS_CLAN_TAG_MAX_LENGTH + 1];
	//u8 reserved[10];
};

// Clan member information handled in ranking
struct SceNpScoreClansMemberDescription
{
	s8 description[SCE_NP_CLANS_CLAN_DESCRIPTION_MAX_LENGTH + 1];
};

// Clan ranking information
struct SceNpScoreClanRankData
{
	be_t<u32> clanId;
	SceNpScoreClanBasicInfo clanInfo;
	be_t<u32> regularMemberCount;
	be_t<u32> recordMemberCount;
	be_t<u32> serialRank;
	be_t<u32> rank;
	be_t<s64> scoreValue;
	CellRtcTick recordDate;
	SceNpId npId;
	SceNpOnlineName onlineName;
	u8 reserved[32];
};

// Clan ranking information to be obtained for a specified clan ID
struct SceNpScoreClanIdRankData
{
	be_t<s32> hasData;
	u8 pad0[4];
	SceNpScoreClanRankData rankData;
};

// Union for connection information
union SceNpSignalingConnectionInfo {
	be_t<u32> rtt;
	be_t<u32> bandwidth;
	SceNpId npId;
	struct address {
		be_t<u32> addr; // in_addr
		//in_port_t port; // TODO: Implement this?
	};
	be_t<u32> packet_loss;
};

// Network information structure
struct SceNpSignalingNetInfo
{
	be_t<u32> size;
	be_t<u32> local_addr; // in_addr
	be_t<u32> mapped_addr; // in_addr
	be_t<s32> nat_status;
	be_t<s32> upnp_status;
	be_t<s32> npport_status;
	be_t<u16> npport;
};

// NP signaling callback function
typedef void(*SceNpSignalingHandler)(u32 ctx_id, u32 subject_id, s32 event, s32 error_code, u32 arg_addr);