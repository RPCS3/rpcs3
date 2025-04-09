#pragma once

#include "util/types.hpp"
#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/ErrorCodes.h"
#include <mutex>
#include <vector>
#include <mutex>

// Error codes
enum SceNpTrophyError : u32
{
	SCE_NP_TROPHY_ERROR_ALREADY_INITIALIZED          = 0x80022901,
	SCE_NP_TROPHY_ERROR_NOT_INITIALIZED              = 0x80022902,
	SCE_NP_TROPHY_ERROR_NOT_SUPPORTED                = 0x80022903,
	SCE_NP_TROPHY_ERROR_CONTEXT_NOT_REGISTERED       = 0x80022904,
	SCE_NP_TROPHY_ERROR_OUT_OF_MEMORY                = 0x80022905,
	SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT             = 0x80022906,
	SCE_NP_TROPHY_ERROR_EXCEEDS_MAX                  = 0x80022907,
	SCE_NP_TROPHY_ERROR_INSUFFICIENT                 = 0x80022909,
	SCE_NP_TROPHY_ERROR_UNKNOWN_CONTEXT              = 0x8002290a,
	SCE_NP_TROPHY_ERROR_INVALID_FORMAT               = 0x8002290b,
	SCE_NP_TROPHY_ERROR_BAD_RESPONSE                 = 0x8002290c,
	SCE_NP_TROPHY_ERROR_INVALID_GRADE                = 0x8002290d,
	SCE_NP_TROPHY_ERROR_INVALID_CONTEXT              = 0x8002290e,
	SCE_NP_TROPHY_ERROR_PROCESSING_ABORTED           = 0x8002290f,
	SCE_NP_TROPHY_ERROR_ABORT                        = 0x80022910,
	SCE_NP_TROPHY_ERROR_UNKNOWN_HANDLE               = 0x80022911,
	SCE_NP_TROPHY_ERROR_LOCKED                       = 0x80022912,
	SCE_NP_TROPHY_ERROR_HIDDEN                       = 0x80022913,
	SCE_NP_TROPHY_ERROR_CANNOT_UNLOCK_PLATINUM       = 0x80022914,
	SCE_NP_TROPHY_ERROR_ALREADY_UNLOCKED             = 0x80022915,
	SCE_NP_TROPHY_ERROR_INVALID_TYPE                 = 0x80022916,
	SCE_NP_TROPHY_ERROR_INVALID_HANDLE               = 0x80022917,
	SCE_NP_TROPHY_ERROR_INVALID_NP_COMM_ID           = 0x80022918,
	SCE_NP_TROPHY_ERROR_UNKNOWN_NP_COMM_ID           = 0x80022919,
	SCE_NP_TROPHY_ERROR_DISC_IO                      = 0x8002291a,
	SCE_NP_TROPHY_ERROR_CONF_DOES_NOT_EXIST          = 0x8002291b,
	SCE_NP_TROPHY_ERROR_UNSUPPORTED_FORMAT           = 0x8002291c,
	SCE_NP_TROPHY_ERROR_ALREADY_INSTALLED            = 0x8002291d,
	SCE_NP_TROPHY_ERROR_BROKEN_DATA                  = 0x8002291e,
	SCE_NP_TROPHY_ERROR_VERIFICATION_FAILURE         = 0x8002291f,
	SCE_NP_TROPHY_ERROR_INVALID_TROPHY_ID            = 0x80022920,
	SCE_NP_TROPHY_ERROR_UNKNOWN_TROPHY_ID            = 0x80022921,
	SCE_NP_TROPHY_ERROR_UNKNOWN_TITLE                = 0x80022922,
	SCE_NP_TROPHY_ERROR_UNKNOWN_FILE                 = 0x80022923,
	SCE_NP_TROPHY_ERROR_DISC_NOT_MOUNTED             = 0x80022924,
	SCE_NP_TROPHY_ERROR_SHUTDOWN                     = 0x80022925,
	SCE_NP_TROPHY_ERROR_TITLE_ICON_NOT_FOUND         = 0x80022926,
	SCE_NP_TROPHY_ERROR_TROPHY_ICON_NOT_FOUND        = 0x80022927,
	SCE_NP_TROPHY_ERROR_INSUFFICIENT_DISK_SPACE      = 0x80022928,
	SCE_NP_TROPHY_ERROR_ILLEGAL_UPDATE               = 0x8002292a,
	SCE_NP_TROPHY_ERROR_SAVEDATA_USER_DOES_NOT_MATCH = 0x8002292b,
	SCE_NP_TROPHY_ERROR_TROPHY_ID_DOES_NOT_EXIST     = 0x8002292c,
	SCE_NP_TROPHY_ERROR_SERVICE_UNAVAILABLE          = 0x8002292d,
	SCE_NP_TROPHY_ERROR_UNKNOWN                      = 0x800229ff,
};

enum
{
	SCE_NP_TROPHY_TITLE_MAX_SIZE       = 128,
	SCE_NP_TROPHY_GAME_DESCR_MAX_SIZE  = 1024,
	SCE_NP_TROPHY_NAME_MAX_SIZE        = 128,
	SCE_NP_TROPHY_DESCR_MAX_SIZE       = 1024,

	SCE_NP_TROPHY_FLAG_SETSIZE         = 128,
	SCE_NP_TROPHY_FLAG_BITS_SHIFT      = 5,

	SCE_NP_TROPHY_INVALID_CONTEXT      = 0,
	SCE_NP_TROPHY_INVALID_HANDLE       = 0,
};

enum : u32
{
	SCE_NP_TROPHY_INVALID_TROPHY_ID    = 0xffffffff,
};

enum SceNpTrophyGrade
{
	SCE_NP_TROPHY_GRADE_UNKNOWN        = 0,
	SCE_NP_TROPHY_GRADE_PLATINUM       = 1,
	SCE_NP_TROPHY_GRADE_GOLD           = 2,
	SCE_NP_TROPHY_GRADE_SILVER         = 3,
	SCE_NP_TROPHY_GRADE_BRONZE         = 4,
};

enum
{
	SCE_NP_TROPHY_OPTIONS_CREATE_CONTEXT_READ_ONLY = 1,

	SCE_NP_TROPHY_OPTIONS_REGISTER_CONTEXT_SHOW_ERROR_EXIT = 1
};

struct SceNpTrophyGameDetails
{
	be_t<u32> numTrophies;
	be_t<u32> numPlatinum;
	be_t<u32> numGold;
	be_t<u32> numSilver;
	be_t<u32> numBronze;
	char title[SCE_NP_TROPHY_TITLE_MAX_SIZE];
	char description[SCE_NP_TROPHY_GAME_DESCR_MAX_SIZE];
	u8 reserved[4];
};

struct SceNpTrophyGameData
{
	be_t<u32> unlockedTrophies;
	be_t<u32> unlockedPlatinum;
	be_t<u32> unlockedGold;
	be_t<u32> unlockedSilver;
	be_t<u32> unlockedBronze;
};

struct SceNpTrophyDetails
{
	be_t<s32> trophyId;     // SceNpTrophyId
	be_t<u32> trophyGrade;  // SceNpTrophyGrade
	char name[SCE_NP_TROPHY_NAME_MAX_SIZE];
	char description[SCE_NP_TROPHY_DESCR_MAX_SIZE];
	b8 hidden;
	u8 reserved[3];
};

struct SceNpTrophyData
{
	be_t<u64> timestamp;    // CellRtcTick
	be_t<s32> trophyId;     // SceNpTrophyId
	b8 unlocked;
	u8 reserved[3];
};

struct SceNpTrophyFlagArray
{
	be_t<u32> flag_bits[SCE_NP_TROPHY_FLAG_SETSIZE >> SCE_NP_TROPHY_FLAG_BITS_SHIFT];
};

enum SceNpTrophyStatus : u32
{
	SCE_NP_TROPHY_STATUS_UNKNOWN             = 0,
	SCE_NP_TROPHY_STATUS_NOT_INSTALLED       = 1,
	SCE_NP_TROPHY_STATUS_DATA_CORRUPT        = 2,
	SCE_NP_TROPHY_STATUS_INSTALLED           = 3,
	SCE_NP_TROPHY_STATUS_REQUIRES_UPDATE     = 4,

	SCE_NP_TROPHY_STATUS_PROCESSING_SETUP    = 5,
	SCE_NP_TROPHY_STATUS_PROCESSING_PROGRESS = 6,
	SCE_NP_TROPHY_STATUS_PROCESSING_FINALIZE = 7,

	SCE_NP_TROPHY_STATUS_PROCESSING_COMPLETE = 8,

	SCE_NP_TROPHY_STATUS_CHANGES_DETECTED    = 9,
};

enum : u32
{
	NP_TROPHY_COMM_SIGN_MAGIC = 0xB9DDE13B,
};

using SceNpTrophyStatusCallback = s32(u32 context, u32 status, s32 completed, s32 total, vm::ptr<void> arg);

// Forward declare this function since trophyunlock needs it
error_code sceNpTrophyGetTrophyInfo(u32 context, u32 handle, s32 trophyId, vm::ptr<SceNpTrophyDetails> details, vm::ptr<SceNpTrophyData> data);

class TrophyNotificationBase
{
public:
	virtual ~TrophyNotificationBase();

	virtual s32 ShowTrophyNotification(const SceNpTrophyDetails& trophy, const std::vector<uchar>& trophyIconBfr) = 0;
};

struct current_trophy_name
{
	std::mutex mtx;
	std::string name;
};
