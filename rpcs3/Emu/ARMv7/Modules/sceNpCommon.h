#pragma once

enum SceNpServiceState : s32
{
	SCE_NP_SERVICE_STATE_UNKNOWN = 0,
	SCE_NP_SERVICE_STATE_SIGNED_OUT,
	SCE_NP_SERVICE_STATE_SIGNED_IN,
	SCE_NP_SERVICE_STATE_ONLINE
};

struct SceNpCommunicationId
{
	char data[9];
	char term;
	u8 num;
	char dummy;
};

struct SceNpCommunicationPassphrase
{
	u8 data[128];
};

struct SceNpCommunicationSignature
{
	u8 data[160];
};

struct SceNpCommunicationConfig
{
	vm::lcptr<SceNpCommunicationId> commId;
	vm::lcptr<SceNpCommunicationPassphrase> commPassphrase;
	vm::lcptr<SceNpCommunicationSignature> commSignature;
};

struct SceNpCountryCode
{
	char data[2];
	char term;
	char padding[1];
};

struct SceNpOnlineId
{
	char data[16];
	char term;
	char dummy[3];
};

struct SceNpId
{
	SceNpOnlineId handle;
	u8 opt[8];
	u8 reserved[8];
};

struct SceNpAvatarUrl
{
	char data[127];
	char term;
};

struct SceNpUserInformation
{
	SceNpId userId;
	SceNpAvatarUrl icon;
	u8 reserved[52];
};

struct SceNpMyLanguages
{
	le_t<s32> language1;
	le_t<s32> language2;
	le_t<s32> language3;
	u8 padding[4];
};

struct SceNpAvatarImage
{
	u8 data[200 * 1024];
	le_t<u32> size;
	u8 reserved[12];
};

enum SceNpAvatarSizeType : s32
{
	SCE_NP_AVATAR_SIZE_LARGE,
	SCE_NP_AVATAR_SIZE_MIDDLE,
	SCE_NP_AVATAR_SIZE_SMALL
};

struct SceNpAboutMe
{
	char data[64];
};

struct SceNpDate
{
	le_t<u16> year;
	u8 month;
	u8 day;
};

union SceNpTicketParam
{
	le_t<s32> _s32;
	le_t<s64> _s64;
	le_t<u32> _u32;
	le_t<u64> _u64;
	SceNpDate date;
	u8 data[256];
};

struct SceNpTicketVersion
{
	le_t<u16> major;
	le_t<u16> minor;
};

using SceNpAuthCallback = s32(s32 id, s32 result, vm::ptr<void> arg);

struct SceNpAuthRequestParameter
{
	le_t<u32> size;
	SceNpTicketVersion version;
	vm::lcptr<char> serviceId;
	vm::lcptr<void> cookie;
	le_t<u32> cookieSize;
	vm::lcptr<char> entitlementId;
	le_t<u32> consumedCount;
	vm::lptr<SceNpAuthCallback> ticketCb;
	vm::lptr<void> cbArg;
};

struct SceNpEntitlementId
{
	u8 data[32];
};

struct SceNpEntitlement
{
	SceNpEntitlementId id;
	le_t<u64> createdDate;
	le_t<u64> expireDate;
	le_t<u32> type;
	le_t<s32> remainingCount;
	le_t<u32> consumedCount;
	char padding[4];
};

extern psv_log_base sceNpCommon;
