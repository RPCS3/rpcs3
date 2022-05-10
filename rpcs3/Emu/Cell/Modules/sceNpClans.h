#pragma once

#include "sceNp.h"

// Return codes
enum SceNpClansError : u32
{
	SCE_NP_CLANS_ERROR_ALREADY_INITIALIZED                  = 0x80022701,
	SCE_NP_CLANS_ERROR_NOT_INITIALIZED                      = 0x80022702,
	SCE_NP_CLANS_ERROR_NOT_SUPPORTED                        = 0x80022703,
	SCE_NP_CLANS_ERROR_OUT_OF_MEMORY                        = 0x80022704,
	SCE_NP_CLANS_ERROR_INVALID_ARGUMENT                     = 0x80022705,
	SCE_NP_CLANS_ERROR_EXCEEDS_MAX                          = 0x80022706,
	SCE_NP_CLANS_ERROR_BAD_RESPONSE                         = 0x80022707,
	SCE_NP_CLANS_ERROR_BAD_DATA                             = 0x80022708,
	SCE_NP_CLANS_ERROR_BAD_REQUEST                          = 0x80022709,
	SCE_NP_CLANS_ERROR_INVALID_SIGNATURE                    = 0x8002270a,
	SCE_NP_CLANS_ERROR_INSUFFICIENT                         = 0x8002270b,
	SCE_NP_CLANS_ERROR_INTERNAL_BUFFER                      = 0x8002270c,
	SCE_NP_CLANS_ERROR_SERVER_MAINTENANCE                   = 0x8002270d,
	SCE_NP_CLANS_ERROR_SERVER_END_OF_SERVICE                = 0x8002270e,
	SCE_NP_CLANS_ERROR_SERVER_BEFORE_START_OF_SERVICE       = 0x8002270f,
	SCE_NP_CLANS_ERROR_ABORTED                              = 0x80022710,
	SCE_NP_CLANS_ERROR_SERVICE_UNAVAILABLE                  = 0x80022711,
	SCE_NP_CLANS_SERVER_ERROR_BAD_REQUEST                   = 0x80022801,
	SCE_NP_CLANS_SERVER_ERROR_INVALID_TICKET                = 0x80022802,
	SCE_NP_CLANS_SERVER_ERROR_INVALID_SIGNATURE             = 0x80022803,
	SCE_NP_CLANS_SERVER_ERROR_TICKET_EXPIRED                = 0x80022804,
	SCE_NP_CLANS_SERVER_ERROR_INVALID_NPID                  = 0x80022805,
	SCE_NP_CLANS_SERVER_ERROR_FORBIDDEN                     = 0x80022806,
	SCE_NP_CLANS_SERVER_ERROR_INTERNAL_SERVER_ERROR         = 0x80022807,
	SCE_NP_CLANS_SERVER_ERROR_BANNED                        = 0x8002280a,
	SCE_NP_CLANS_SERVER_ERROR_BLACKLISTED                   = 0x80022811,
	SCE_NP_CLANS_SERVER_ERROR_INVALID_ENVIRONMENT           = 0x8002281d,
	SCE_NP_CLANS_SERVER_ERROR_NO_SUCH_CLAN_SERVICE          = 0x8002282f,
	SCE_NP_CLANS_SERVER_ERROR_NO_SUCH_CLAN                  = 0x80022830,
	SCE_NP_CLANS_SERVER_ERROR_NO_SUCH_CLAN_MEMBER           = 0x80022831,
	SCE_NP_CLANS_SERVER_ERROR_BEFORE_HOURS                  = 0x80022832,
	SCE_NP_CLANS_SERVER_ERROR_CLOSED_SERVICE                = 0x80022833,
	SCE_NP_CLANS_SERVER_ERROR_PERMISSION_DENIED             = 0x80022834,
	SCE_NP_CLANS_SERVER_ERROR_CLAN_LIMIT_REACHED            = 0x80022835,
	SCE_NP_CLANS_SERVER_ERROR_CLAN_LEADER_LIMIT_REACHED     = 0x80022836,
	SCE_NP_CLANS_SERVER_ERROR_CLAN_MEMBER_LIMIT_REACHED     = 0x80022837,
	SCE_NP_CLANS_SERVER_ERROR_CLAN_JOINED_LIMIT_REACHED     = 0x80022838,
	SCE_NP_CLANS_SERVER_ERROR_MEMBER_STATUS_INVALID         = 0x80022839,
	SCE_NP_CLANS_SERVER_ERROR_DUPLICATED_CLAN_NAME          = 0x8002283a,
	SCE_NP_CLANS_SERVER_ERROR_CLAN_LEADER_CANNOT_LEAVE      = 0x8002283b,
	SCE_NP_CLANS_SERVER_ERROR_INVALID_ROLE_PRIORITY         = 0x8002283c,
	SCE_NP_CLANS_SERVER_ERROR_ANNOUNCEMENT_LIMIT_REACHED    = 0x8002283d,
	SCE_NP_CLANS_SERVER_ERROR_CLAN_CONFIG_MASTER_NOT_FOUND  = 0x8002283e,
	SCE_NP_CLANS_SERVER_ERROR_DUPLICATED_CLAN_TAG           = 0x8002283f,
	SCE_NP_CLANS_SERVER_ERROR_EXCEEDS_CREATE_CLAN_FREQUENCY = 0x80022840,
	SCE_NP_CLANS_SERVER_ERROR_CLAN_PASSPHRASE_INCORRECT     = 0x80022841,
	SCE_NP_CLANS_SERVER_ERROR_CANNOT_RECORD_BLACKLIST_ENTRY = 0x80022842,
	SCE_NP_CLANS_SERVER_ERROR_NO_SUCH_CLAN_ANNOUNCEMENT     = 0x80022843,
	SCE_NP_CLANS_SERVER_ERROR_VULGAR_WORDS_POSTED           = 0x80022844,
	SCE_NP_CLANS_SERVER_ERROR_BLACKLIST_LIMIT_REACHED       = 0x80022845,
	SCE_NP_CLANS_SERVER_ERROR_NO_SUCH_BLACKLIST_ENTRY       = 0x80022846,
	SCE_NP_CLANS_SERVER_ERROR_INVALID_NP_MESSAGE_FORMAT     = 0x8002284b,
	SCE_NP_CLANS_SERVER_ERROR_FAILED_TO_SEND_NP_MESSAGE     = 0x8002284c,
};

// Clan roles
enum
{
	SCE_NP_CLANS_ROLE_UNKNOWN    = 0,
	SCE_NP_CLANS_ROLE_NON_MEMBER = 1,
	SCE_NP_CLANS_ROLE_MEMBER     = 2,
	SCE_NP_CLANS_ROLE_SUB_LEADER = 3,
	SCE_NP_CLANS_ROLE_LEADER     = 4,
};

// Clan member status
enum
{
	SCE_NP_CLANS_MEMBER_STATUS_UNKNOWN = 0,
	SCE_NP_CLANS_MEMBER_STATUS_NORMAL  = 1,
	SCE_NP_CLANS_MEMBER_STATUS_INVITED = 2,
	SCE_NP_CLANS_MEMBER_STATUS_PENDING = 3,
};

// Clan search operators
enum
{
	SCE_NP_CLANS_SEARCH_OPERATOR_EQUAL_TO                 = 0,
	SCE_NP_CLANS_SEARCH_OPERATOR_NOT_EQUAL_TO             = 1,
	SCE_NP_CLANS_SEARCH_OPERATOR_GREATER_THAN             = 2,
	SCE_NP_CLANS_SEARCH_OPERATOR_GREATER_THAN_OR_EQUAL_TO = 3,
	SCE_NP_CLANS_SEARCH_OPERATOR_LESS_THAN                = 4,
	SCE_NP_CLANS_SEARCH_OPERATOR_LESS_THAN_OR_EQUAL_TO    = 5,
	SCE_NP_CLANS_SEARCH_OPERATOR_SIMILAR_TO               = 6,
};

// Constants for clan functions and structures
enum
{
	SCE_NP_CLANS_ANNOUNCEMENT_MESSAGE_BODY_MAX_LENGTH = 1536,
	SCE_NP_CLANS_CLAN_BINARY_ATTRIBUTE1_MAX_SIZE      = 190,
	SCE_NP_CLANS_CLAN_BINARY_DATA_MAX_SIZE            = 10240,
	SCE_NP_CLANS_MEMBER_BINARY_ATTRIBUTE1_MAX_SIZE    = 16,
	SCE_NP_CLANS_MEMBER_DESCRIPTION_MAX_LENGTH        = 255,
	SCE_NP_CLANS_MEMBER_BINARY_DATA_MAX_SIZE          = 1024,
	SCE_NP_CLANS_MESSAGE_BODY_MAX_LENGTH              = 1536,
	SCE_NP_CLANS_MESSAGE_SUBJECT_MAX_LENGTH           = 54,
	SCE_NP_CLANS_MESSAGE_BODY_CHARACTER_MAX           = 512,
	SCE_NP_CLANS_MESSAGE_SUBJECT_CHARACTER_MAX        = 18,
	SCE_NP_CLANS_MESSAGE_BINARY_DATA_MAX_SIZE         = 1024,
	SCE_NP_CLANS_PAGING_REQUEST_START_POSITION_MAX    = 1000000,
	SCE_NP_CLANS_PAGING_REQUEST_PAGE_MAX              = 100,
};

enum
{
	SCE_NP_CLANS_FIELDS_SEARCHABLE_ATTR_INT_ATTR1       = 0x00000001,
	SCE_NP_CLANS_FIELDS_SEARCHABLE_ATTR_INT_ATTR2       = 0x00000002,
	SCE_NP_CLANS_FIELDS_SEARCHABLE_ATTR_INT_ATTR3       = 0x00000004,
	SCE_NP_CLANS_FIELDS_SEARCHABLE_ATTR_BIN_ATTR1       = 0x00000008,
	SCE_NP_CLANS_FIELDS_SEARCHABLE_PROFILE_TAG          = 0x00000010,
	SCE_NP_CLANS_FIELDS_SEARCHABLE_PROFILE_NUM_MEMBERS  = 0x00000020,
	SCE_NP_CLANS_FIELDS_UPDATABLE_CLAN_INFO_DESCR       = 0x00000040,
	SCE_NP_CLANS_FIELDS_UPDATABLE_CLAN_INFO_BIN_DATA1   = 0x00000080,
	SCE_NP_CLANS_FIELDS_UPDATABLE_MEMBER_INFO_DESCR     = 0x00000100,
	SCE_NP_CLANS_FIELDS_UPDATABLE_MEMBER_INFO_BIN_ATTR1 = 0x00000200,
	SCE_NP_CLANS_FIELDS_UPDATABLE_MEMBER_INFO_BIN_DATA1 = 0x00000400,
	SCE_NP_CLANS_FIELDS_UPDATABLE_MEMBER_INFO_ALLOW_MSG = 0x00000800,
};

enum
{
	SCE_NP_CLANS_MESSAGE_OPTIONS_NONE   = 0x00000000,
	SCE_NP_CLANS_MESSAGE_OPTIONS_CENSOR = 0x00000001,
};

enum
{
	SCE_NP_CLANS_INVALID_ID             = 0,
	SCE_NP_CLANS_INVALID_REQUEST_HANDLE = 0,
};

// Request handle for clan API
using SceNpClansRequestHandle = vm::ptr<struct SceNpClansRequest>;

// Paging request structure
struct SceNpClansPagingRequest
{
	be_t<u32> startPos;
	be_t<u32> max;
};

// Paging result structure
struct SceNpClansPagingResult
{
	be_t<u32> count;
	be_t<u32> total;
};

// Basic clan information
struct SceNpClansClanBasicInfo
{
	be_t<u32> clanId;
	be_t<u32> numMembers;
	s8 name[SCE_NP_CLANS_CLAN_NAME_MAX_LENGTH + 1];
	s8 tag[SCE_NP_CLANS_CLAN_TAG_MAX_LENGTH + 1];
	u8 reserved[2];
};

// Clan entry structure
struct SceNpClansEntry
{
	SceNpClansClanBasicInfo info;
	be_t<u32> role;
	be_t<s32> status;
	b8 allowMsg;
	u8 reserved[3];
};

// Clan search attribute structure
struct SceNpClansSearchableAttr
{
	be_t<u32> fields;
	be_t<u32> intAttr1;
	be_t<u32> intAttr2;
	be_t<u32> intAttr3;
	u8 binAttr1[SCE_NP_CLANS_CLAN_BINARY_ATTRIBUTE1_MAX_SIZE];
	u8 reserved[2];
};

// Clan search profile structure
struct SceNpClansSearchableProfile
{
	SceNpClansSearchableAttr attr;
	be_t<u32> fields;
	be_t<u32> numMembers;
	be_t<s32> tagSearchOp;
	be_t<s32> numMemberSearchOp;
	be_t<s32> intAttr1SearchOp;
	be_t<s32> intAttr2SearchOp;
	be_t<s32> intAttr3SearchOp;
	be_t<s32> binAttr1SearchOp;
	s8 tag[SCE_NP_CLANS_CLAN_TAG_MAX_LENGTH + 1];
	u8 reserved[3];
};

// Clan search name structure
struct SceNpClansSearchableName
{
	be_t<s32> nameSearchOp;
	s8 name[SCE_NP_CLANS_CLAN_NAME_MAX_LENGTH + 1];
	u8 reserved[3];
};

// Updatable clan information structure
struct SceNpClansUpdatableClanInfo
{
	be_t<u32> fields;
	s8 description[SCE_NP_CLANS_CLAN_DESCRIPTION_MAX_LENGTH + 1];
	SceNpClansSearchableAttr attr;
	u8 binData1;
	be_t<u32> binData1Size;
};

// Clan information structure
struct SceNpClansClanInfo
{
	CellRtcTick dateCreated;
	SceNpClansClanBasicInfo info;
	SceNpClansUpdatableClanInfo updatable;
};

// Updatable member information structure
struct SceNpClansUpdatableMemberInfo
{
	be_t<u32> fields;
	u8 binData1;
	be_t<u32> binData1Size;
	u8 binAttr1[SCE_NP_CLANS_CLAN_BINARY_ATTRIBUTE1_MAX_SIZE + 1];
	s8 description[SCE_NP_CLANS_MEMBER_DESCRIPTION_MAX_LENGTH + 1];
	b8 allowMsg;
	u8 reserved[3];
};

// Member entry structure
struct SceNpClansMemberEntry
{
	SceNpId npid;
	be_t<u32> role;
	be_t<s32> status;
	SceNpClansUpdatableMemberInfo updatable;
};

// Clan message structure
struct SceNpClansMessage
{
	char subject[SCE_NP_CLANS_MESSAGE_SUBJECT_MAX_LENGTH + 1];
	char body[SCE_NP_CLANS_MESSAGE_BODY_MAX_LENGTH + 1];
	be_t<u32> options;
};

// Clan message data structure
struct SceNpClansMessageData
{
	u8 binData1;
	be_t<u32> binData1Size;
};

// Clan message entry structure
struct SceNpClansMessageEntry
{
	CellRtcTick postDate;
	be_t<u32> mId;
	SceNpClansMessage message;
	SceNpClansMessageData data;
	SceNpId npid;
	u8 reserved[4];
};

// Blacklist entry structure
struct SceNpClansBlacklistEntry
{
	SceNpId entry;
	SceNpId registeredBy;
};

// fxm objects

struct sce_np_clans_manager
{
	atomic_t<bool> is_initialized = false;
};
