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
	vm::psv::ptr<const SceNpCommunicationId> commId;
	vm::psv::ptr<const SceNpCommunicationPassphrase> commPassphrase;
	vm::psv::ptr<const SceNpCommunicationSignature> commSignature;
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
	s32 language1;
	s32 language2;
	s32 language3;
	u8 padding[4];
};

struct SceNpAvatarImage
{
	u8 data[200 * 1024];
	u32 size;
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

typedef s32 SceNpAuthRequestId;
typedef u64 SceNpTime;

struct SceNpDate
{
	u16 year;
	u8 month;
	u8 day;
};

union SceNpTicketParam
{
	s32 _s32;
	s64 _s64;
	u32 _u32;
	u64 _u64;
	SceNpDate date;
	u8 data[256];
};

struct SceNpTicketVersion
{
	u16 major;
	u16 minor;
};

typedef vm::psv::ptr<s32(SceNpAuthRequestId id, s32 result, vm::psv::ptr<void> arg)> SceNpAuthCallback;

struct SceNpAuthRequestParameter
{
	u32 size;
	SceNpTicketVersion version;
	vm::psv::ptr<const char> serviceId;
	vm::psv::ptr<const void> cookie;
	u32 cookieSize;
	vm::psv::ptr<const char> entitlementId;
	u32 consumedCount;
	SceNpAuthCallback ticketCb;
	vm::psv::ptr<void> cbArg;
};

struct SceNpEntitlementId
{
	u8 data[32];
};

struct SceNpEntitlement
{
	SceNpEntitlementId id;
	SceNpTime createdDate;
	SceNpTime expireDate;
	u32 type;
	s32 remainingCount;
	u32 consumedCount;
	char padding[4];
};

extern psv_log_base sceNpCommon;
