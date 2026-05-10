#pragma once

#include "Emu/Memory/vm_ptr.h"

struct CellVideoUploadOption
{
	be_t<s32> type;
	be_t<u64> value;
};

struct CellVideoUploadParam
{
	be_t<s32> siteID;
	vm::bcptr<char> pFilePath;
	union
	{
		struct
		{
			vm::bcptr<char> pClientId;
			vm::bcptr<char> pDeveloperKey;
			vm::bcptr<char> pTitle_UTF8;
			vm::bcptr<char> pDescription_UTF8;
			vm::bcptr<char> pKeyword_1_UTF8;
			vm::bcptr<char> pKeyword_2_UTF8;
			vm::bcptr<char> pKeyword_3_UTF8;
			u8 isPrivate;
			u8 rating;
		} youtube;
	} u;
	be_t<s32> numOfOption;
	vm::bptr<CellVideoUploadOption> pOption;
};

using CellVideoUploadCallback = void(s32 status, s32 errorCode, vm::cptr<char> pResultURL, vm::ptr<void> userdata);

enum
{
	CELL_VIDEO_UPLOAD_MAX_FILE_PATH_LEN             = 1023,
	CELL_VIDEO_UPLOAD_MAX_YOUTUBE_CLIENT_ID_LEN     = 64,
	CELL_VIDEO_UPLOAD_MAX_YOUTUBE_DEVELOPER_KEY_LEN = 128,
	CELL_VIDEO_UPLOAD_MAX_YOUTUBE_TITLE_LEN         = 61,
	CELL_VIDEO_UPLOAD_MAX_YOUTUBE_DESCRIPTION_LEN   = 1024,
	CELL_VIDEO_UPLOAD_MAX_YOUTUBE_KEYWORD_LEN       = 25
};

// Return Codes
enum CellVideoUploadError : u32
{
	CELL_VIDEO_UPLOAD_ERROR_CANCEL              = 0x8002d000,
	CELL_VIDEO_UPLOAD_ERROR_NETWORK             = 0x8002d001,
	CELL_VIDEO_UPLOAD_ERROR_SERVICE_STOP        = 0x8002d002,
	CELL_VIDEO_UPLOAD_ERROR_SERVICE_BUSY        = 0x8002d003,
	CELL_VIDEO_UPLOAD_ERROR_SERVICE_UNAVAILABLE = 0x8002d004,
	CELL_VIDEO_UPLOAD_ERROR_SERVICE_QUOTA       = 0x8002d005,
	CELL_VIDEO_UPLOAD_ERROR_ACCOUNT_STOP        = 0x8002d006,
	CELL_VIDEO_UPLOAD_ERROR_OUT_OF_MEMORY       = 0x8002d020,
	CELL_VIDEO_UPLOAD_ERROR_FATAL               = 0x8002d021,
	CELL_VIDEO_UPLOAD_ERROR_INVALID_VALUE       = 0x8002d022,
	CELL_VIDEO_UPLOAD_ERROR_FILE_OPEN           = 0x8002d023,
	CELL_VIDEO_UPLOAD_ERROR_INVALID_STATE       = 0x8002d024
};

enum
{
	CELL_VIDEO_UPLOAD_STATUS_INITIALIZED = 1,
	CELL_VIDEO_UPLOAD_STATUS_FINALIZED = 2
};
