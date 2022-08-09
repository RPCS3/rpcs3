#pragma once

// Return Codes
enum CellSubDisplayError : u32
{
	CELL_SUBDISPLAY_ERROR_OUT_OF_MEMORY    = 0x80029851,
	CELL_SUBDISPLAY_ERROR_FATAL            = 0x80029852,
	CELL_SUBDISPLAY_ERROR_NOT_FOUND        = 0x80029853,
	CELL_SUBDISPLAY_ERROR_INVALID_VALUE    = 0x80029854,
	CELL_SUBDISPLAY_ERROR_NOT_INITIALIZED  = 0x80029855,
	CELL_SUBDISPLAY_ERROR_NOT_SUPPORTED    = 0x80029856,
	CELL_SUBDISPLAY_ERROR_SET_SAMPLE       = 0x80029860,
	CELL_SUBDISPLAY_ERROR_AUDIOOUT_IS_BUSY = 0x80029861,
	CELL_SUBDISPLAY_ERROR_ZERO_REGISTERED  = 0x80029813, // = CELL_REMOTEPLAY_ERROR_ZERO_REGISTERED
};

// Different constants
enum
{
	CELL_SUBDISPLAY_STATUS_JOIN                = 1,
	CELL_SUBDISPLAY_STATUS_LEAVE               = 2,
	CELL_SUBDISPLAY_STATUS_FATALERROR          = 3,

	CELL_SUBDISPLAY_VERSION_0001               = 1,
	CELL_SUBDISPLAY_VERSION_0002               = 2,
	CELL_SUBDISPLAY_VERSION_0003               = 3,

	CELL_SUBDISPLAY_MODE_REMOTEPLAY            = 1,

	CELL_SUBDISPLAY_VIDEO_FORMAT_A8R8G8B8      = 1,
	CELL_SUBDISPLAY_VIDEO_FORMAT_R8G8B8A8      = 2,
	CELL_SUBDISPLAY_VIDEO_FORMAT_YUV420        = 3,

	CELL_SUBDISPLAY_VIDEO_ASPECT_RATIO_16_9    = 0,
	CELL_SUBDISPLAY_VIDEO_ASPECT_RATIO_4_3     = 1,

	CELL_SUBDISPLAY_VIDEO_MODE_SETDATA         = 0,
	CELL_SUBDISPLAY_VIDEO_MODE_CAPTURE         = 1,

	CELL_SUBDISPLAY_AUDIO_MODE_SETDATA         = 0,
	CELL_SUBDISPLAY_AUDIO_MODE_CAPTURE         = 1,

	CELL_SUBDISPLAY_0001_MEMORY_CONTAINER_SIZE = 8 * 1024 * 1024,
	CELL_SUBDISPLAY_0002_MEMORY_CONTAINER_SIZE = 10 * 1024 * 1024,
	CELL_SUBDISPLAY_0003_MEMORY_CONTAINER_SIZE = 10 * 1024 * 1024,

	CELL_SUBDISPLAY_0003_WIDTH                 = 864,
	CELL_SUBDISPLAY_0003_PITCH                 = 864,
	CELL_SUBDISPLAY_0003_HEIGHT                = 480,

	CELL_SUBDISPLAY_NICKNAME_LEN               = 256,
	CELL_SUBDISPLAY_PSPID_LEN                  = 16,
};

// Touch
enum
{
	CELL_SUBDISPLAY_TOUCH_STATUS_NONE    = 0,
	CELL_SUBDISPLAY_TOUCH_STATUS_PRESS   = 1,
	CELL_SUBDISPLAY_TOUCH_STATUS_RELEASE = 2,
	CELL_SUBDISPLAY_TOUCH_STATUS_MOVE    = 3,
	CELL_SUBDISPLAY_TOUCH_STATUS_ABORT   = 4,

	CELL_SUBDISPLAY_TOUCH_MAX_TOUCH_INFO = 6,
};

struct CellSubDisplayVideoParam
{
	be_t<s32> format;
	be_t<s32> width;
	be_t<s32> height;
	be_t<s32> pitch;
	be_t<s32> aspectRatio;
	be_t<s32> videoMode;
};

struct CellSubDisplayAudioParam
{
	be_t<s32> ch;
	be_t<s32> audioMode;
};

struct CellSubDisplayParam
{
	be_t<s32> version;
	be_t<s32> mode;
	be_t<s32> nGroup;
	be_t<s32> nPeer;
	CellSubDisplayVideoParam videoParam;
	CellSubDisplayAudioParam audioParam;
};

struct CellSubDisplayPSPId
{
	char data[CELL_SUBDISPLAY_PSPID_LEN];
};

struct CellSubDisplayNickname
{
	char data[CELL_SUBDISPLAY_NICKNAME_LEN];
};

struct CellSubDisplayPeerInfo
{
	be_t<u64> sessionId;
	be_t<u32> portNo;
	CellSubDisplayPSPId pspId;
	CellSubDisplayNickname pspNickname;
};

struct CellSubDisplayTouchInfo
{
	u8 status;
	u8 force;
	be_t<u16> x;
	be_t<u16> y;
};

using CellSubDisplayHandler = void(s32 cbMsg, u64 cbParam, vm::ptr<void> userdata);
