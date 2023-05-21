#pragma once

#include "Emu/Memory/vm_ptr.h"

// Return codes
enum sceNpSnsError : u32
{
	SCE_NP_SNS_ERROR_UNKNOWN                         = 0x80024501,
	SCE_NP_SNS_ERROR_NOT_SIGN_IN                     = 0x80024502,
	SCE_NP_SNS_ERROR_INVALID_ARGUMENT                = 0x80024503,
	SCE_NP_SNS_ERROR_OUT_OF_MEMORY                   = 0x80024504,
	SCE_NP_SNS_ERROR_SHUTDOWN                        = 0x80024505,
	SCE_NP_SNS_ERROR_BUSY                            = 0x80024506,
	SCE_NP_SNS_FB_ERROR_ALREADY_INITIALIZED          = 0x80024511,
	SCE_NP_SNS_FB_ERROR_NOT_INITIALIZED              = 0x80024512,
	SCE_NP_SNS_FB_ERROR_EXCEEDS_MAX                  = 0x80024513,
	SCE_NP_SNS_FB_ERROR_UNKNOWN_HANDLE               = 0x80024514,
	SCE_NP_SNS_FB_ERROR_ABORTED                      = 0x80024515,
	SCE_NP_SNS_FB_ERROR_ALREADY_ABORTED              = 0x80024516,
	SCE_NP_SNS_FB_ERROR_CONFIG_DISABLED              = 0x80024517,
	SCE_NP_SNS_FB_ERROR_FBSERVER_ERROR_RESPONSE      = 0x80024518,
	SCE_NP_SNS_FB_ERROR_THROTTLE_CLOSED              = 0x80024519,
	SCE_NP_SNS_FB_ERROR_OPERATION_INTERVAL_VIOLATION = 0x8002451a,
	SCE_NP_SNS_FB_ERROR_UNLOADED_THROTTLE            = 0x8002451b,
	SCE_NP_SNS_FB_ERROR_ACCESS_NOT_ALLOWED           = 0x8002451c,
};

// Access token param options
enum
{
	SCE_NP_SNS_FB_ACCESS_TOKEN_PARAM_OPTIONS_SILENT = 0x00000001
};

// Constants for SNS functions
enum
{
	SCE_NP_SNS_FB_INVALID_HANDLE               = 0,
	SCE_NP_SNS_FB_HANDLE_SLOT_MAX              = 4,
	SCE_NP_SNS_FB_PERMISSIONS_LENGTH_MAX       = 255,
	SCE_NP_SNS_FB_ACCESS_TOKEN_LENGTH_MAX      = 255,
	SCE_NP_SNS_FB_LONG_ACCESS_TOKEN_LENGTH_MAX = 4096
};

struct sns_fb_handle_t
{
	static const u32 id_base  = 1;
	static const u32 id_step  = 1;
	static const u32 id_count = SCE_NP_SNS_FB_HANDLE_SLOT_MAX + 1;
	static const u32 invalid  = SCE_NP_SNS_FB_INVALID_HANDLE;

	SAVESTATE_INIT_POS(20);
	sns_fb_handle_t() = default;
	sns_fb_handle_t(utils::serial&){}
	void save(utils::serial&){}
};

// Initialization parameters for functionalities coordinated with Facebook
struct SceNpSnsFbInitParams
{
	vm::bptr<void> pool;
	be_t<u32> poolSize;
};

struct SceNpSnsFbAccessTokenParam
{
	be_t<u64> fb_app_id;
	char permissions[SCE_NP_SNS_FB_PERMISSIONS_LENGTH_MAX + 1];
	be_t<u32> options;
};

struct SceNpSnsFbAccessTokenResult
{
	be_t<u64> expiration;
	char access_token[SCE_NP_SNS_FB_ACCESS_TOKEN_LENGTH_MAX + 1];
};

struct SceNpSnsFbLongAccessTokenResult
{
	be_t<u64> expiration;
	char access_token[SCE_NP_SNS_FB_LONG_ACCESS_TOKEN_LENGTH_MAX + 1];
};

// fxm objects

struct sce_np_sns_manager
{
	atomic_t<bool> is_initialized = false;
};
