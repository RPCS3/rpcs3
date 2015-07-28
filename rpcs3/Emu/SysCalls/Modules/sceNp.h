#pragma once

namespace vm { using namespace ps3; }

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

using SceNpBasicEventHandler = s32(s32 event, s32 retCode, u32 reqId, vm::ptr<void> arg);

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
	char data[9];
	char term;
	u8 num;
	char dummy;
};

// OnlineId structure
struct SceNpOnlineId
{
	char data[16];
	char term;
	char dummy[3];
};

// NP ID structure
struct SceNpId
{
	SceNpOnlineId handle;
	u8 opt[8];
	u8 reserved[8];
};

// Online Name structure
struct SceNpOnlineName
{
	char data[48];
	char term;
	char padding[3];
};

// Avatar structure
struct SceNpAvatarUrl
{
	char data[127];
	char term;
};

// Avatar image structure
struct SceNpAvatarImage
{
	u8 data[SCE_NET_NP_AVATAR_IMAGE_MAX_SIZE];
	be_t<u32> size;
	u8 reserved[12];
};

// Self introduction structure
struct SceNpAboutMe
{
	char data[SCE_NET_NP_ABOUT_ME_MAX_LENGTH];
	char term;
};

// User information structure
struct SceNpUserInfo
{
	SceNpId userId;
	SceNpOnlineName name;
	SceNpAvatarUrl icon;
};

// User information structure
struct SceNpUserInfo2
{
	SceNpId npId;
	vm::bptr<SceNpOnlineName> onlineName;
	vm::bptr<SceNpAvatarUrl> avatarUrl;
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
	be_t<u32> size;
	SceNpOnlineId onlineId;
	SceNpId npId;
	SceNpOnlineName onlineName;
	SceNpAvatarUrl avatarUrl;
};

// Message attachment data
struct SceNpBasicAttachmentData
{
	be_t<u32> id; // SceNpBasicAttachmentDataId
	be_t<u32> size;
};

// Message extended attachment data
struct SceNpBasicExtendedAttachmentData
{
	be_t<u64> flags;
	be_t<u64> msgId;
	SceNpBasicAttachmentData data;
	be_t<u32> userAction;
	b8 markedAsUsed;
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
using SceNpManagerCallback = void(s32 event, s32 result, u32 arg_addr);

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
	u8 pad[4];
};

// Basic clan information to be used in raking
struct SceNpScoreClanBasicInfo
{
	s8 clanName[SCE_NP_CLANS_CLAN_NAME_MAX_LENGTH + 1];
	s8 clanTag[SCE_NP_CLANS_CLAN_TAG_MAX_LENGTH + 1];
	u8 reserved[10];
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
union SceNpSignalingConnectionInfo
{
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