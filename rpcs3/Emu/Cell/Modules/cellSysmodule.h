#pragma once

// Error codes
enum CellSysmoduleError
{
	CELL_SYSMODULE_ERROR_DUPLICATED           = 0x80012001,
	CELL_SYSMODULE_ERROR_UNKNOWN              = 0x80012002,
	CELL_SYSMODULE_ERROR_UNLOADED             = 0x80012003,
	CELL_SYSMODULE_ERROR_INVALID_MEMCONTAINER = 0x80012004,
	CELL_SYSMODULE_ERROR_FATAL                = 0x800120ff,
};

enum CellSysmoduleModuleID : u16
{
	CELL_SYSMODULE_NET                      = 0x00,
	CELL_SYSMODULE_HTTP                     = 0x01,
	CELL_SYSMODULE_HTTP_UTIL                = 0x02,
	CELL_SYSMODULE_SSL                      = 0x03,
	CELL_SYSMODULE_HTTPS                    = 0x04,
	CELL_SYSMODULE_VDEC                     = 0x05,
	CELL_SYSMODULE_ADEC                     = 0x06,
	CELL_SYSMODULE_DMUX                     = 0x07,
	CELL_SYSMODULE_VPOST                    = 0x08,
	CELL_SYSMODULE_RTC                      = 0x09,
	CELL_SYSMODULE_SPURS                    = 0x0a,
	CELL_SYSMODULE_OVIS                     = 0x0b,
	CELL_SYSMODULE_SHEAP                    = 0x0c,
	CELL_SYSMODULE_SYNC                     = 0x0d,
	CELL_SYSMODULE_FS                       = 0x0e,
	CELL_SYSMODULE_JPGDEC                   = 0x0f,
	CELL_SYSMODULE_GCM_SYS                  = 0x10,
	CELL_SYSMODULE_AUDIO                    = 0x11,
	CELL_SYSMODULE_PAMF                     = 0x12,
	CELL_SYSMODULE_ATRAC3PLUS               = 0x13,
	CELL_SYSMODULE_NETCTL                   = 0x14,
	CELL_SYSMODULE_SYSUTIL                  = 0x15,
	CELL_SYSMODULE_SYSUTIL_NP               = 0x16,
	CELL_SYSMODULE_IO                       = 0x17,
	CELL_SYSMODULE_PNGDEC                   = 0x18,
	CELL_SYSMODULE_FONT                     = 0x19,
	CELL_SYSMODULE_FONTFT                   = 0x1a,
	CELL_SYSMODULE_FREETYPE                 = 0x1b,
	CELL_SYSMODULE_USBD                     = 0x1c,
	CELL_SYSMODULE_SAIL                     = 0x1d,
	CELL_SYSMODULE_L10N                     = 0x1e,
	CELL_SYSMODULE_RESC                     = 0x1f,
	CELL_SYSMODULE_DAISY                    = 0x20,
	CELL_SYSMODULE_KEY2CHAR                 = 0x21,
	CELL_SYSMODULE_MIC                      = 0x22,
	CELL_SYSMODULE_CAMERA                   = 0x23,
	CELL_SYSMODULE_VDEC_MPEG2               = 0x24,
	CELL_SYSMODULE_VDEC_AVC                 = 0x25,
	CELL_SYSMODULE_ADEC_LPCM                = 0x26,
	CELL_SYSMODULE_ADEC_AC3                 = 0x27,
	CELL_SYSMODULE_ADEC_ATX                 = 0x28,
	CELL_SYSMODULE_ADEC_AT3                 = 0x29,
	CELL_SYSMODULE_DMUX_PAMF                = 0x2a,
	CELL_SYSMODULE_VDEC_AL                  = 0x2b,
	CELL_SYSMODULE_ADEC_AL                  = 0x2c,
	CELL_SYSMODULE_DMUX_AL                  = 0x2d,
	CELL_SYSMODULE_LV2DBG                   = 0x2e,
	// 0x2f
	CELL_SYSMODULE_USBPSPCM                 = 0x30,
	CELL_SYSMODULE_AVCONF_EXT               = 0x31,
	CELL_SYSMODULE_SYSUTIL_USERINFO         = 0x32,
	CELL_SYSMODULE_SYSUTIL_SAVEDATA         = 0x33,
	CELL_SYSMODULE_SUBDISPLAY               = 0x34,
	CELL_SYSMODULE_SYSUTIL_REC              = 0x35,
	CELL_SYSMODULE_VIDEO_EXPORT             = 0x36,
	CELL_SYSMODULE_SYSUTIL_GAME_EXEC        = 0x37,
	CELL_SYSMODULE_SYSUTIL_NP2              = 0x38,
	CELL_SYSMODULE_SYSUTIL_AP               = 0x39,
	CELL_SYSMODULE_SYSUTIL_NP_CLANS         = 0x3a,
	CELL_SYSMODULE_SYSUTIL_OSK_EXT          = 0x3b,
	CELL_SYSMODULE_VDEC_DIVX                = 0x3c,
	CELL_SYSMODULE_JPGENC                   = 0x3d,
	CELL_SYSMODULE_SYSUTIL_GAME             = 0x3e,
	CELL_SYSMODULE_BGDL                     = 0x3f,
	CELL_SYSMODULE_FREETYPE_TT              = 0x40,
	CELL_SYSMODULE_SYSUTIL_VIDEO_UPLOAD     = 0x41,
	CELL_SYSMODULE_SYSUTIL_SYSCONF_EXT      = 0x42,
	CELL_SYSMODULE_FIBER                    = 0x43,
	CELL_SYSMODULE_SYSUTIL_NP_COMMERCE2     = 0x44,
	CELL_SYSMODULE_SYSUTIL_NP_TUS           = 0x45,
	CELL_SYSMODULE_VOICE                    = 0x46,
	CELL_SYSMODULE_ADEC_CELP8               = 0x47,
	CELL_SYSMODULE_CELP8ENC                 = 0x48,
	CELL_SYSMODULE_SYSUTIL_LICENSEAREA      = 0x49,
	CELL_SYSMODULE_SYSUTIL_MUSIC2           = 0x4a,
	// 0x4b
	// 0x4c
	// 0x4d
	CELL_SYSMODULE_SYSUTIL_SCREENSHOT       = 0x4e,
	CELL_SYSMODULE_SYSUTIL_MUSIC_DECODE     = 0x4f,
	CELL_SYSMODULE_SPURS_JQ                 = 0x50,
	// 0x51
	CELL_SYSMODULE_PNGENC                   = 0x52,
	CELL_SYSMODULE_SYSUTIL_MUSIC_DECODE2    = 0x53,
	// 0x54
	CELL_SYSMODULE_SYNC2                    = 0x55,
	CELL_SYSMODULE_SYSUTIL_NP_UTIL          = 0x56,
	CELL_SYSMODULE_RUDP                     = 0x57,
	// 0x58 = Invalid
	CELL_SYSMODULE_SYSUTIL_NP_SNS           = 0x59,
	CELL_SYSMODULE_GEM                      = 0x5a,
	CELL_SYSMODULE_SYSUTIL_CROSS_CONTROLLER = 0x5c,

	// Internal modules
	CELL_SYSMODULE_CELPENC                  = 0xf00a,
	CELL_SYSMODULE_GIFDEC                   = 0xf010,
	CELL_SYSMODULE_ADEC_CELP                = 0xf019,
	CELL_SYSMODULE_ADEC_M2BC                = 0xf01b,
	CELL_SYSMODULE_ADEC_M4AAC               = 0xf01d,
	CELL_SYSMODULE_ADEC_MP3                 = 0xf01e,
	CELL_SYSMODULE_IMEJP                    = 0xf023,
	CELL_SYSMODULE_SYSUTIL_MUSIC            = 0xf028,
	CELL_SYSMODULE_PHOTO_EXPORT             = 0xf029,
	CELL_SYSMODULE_PRINT                    = 0xf02a,
	CELL_SYSMODULE_PHOTO_IMPORT             = 0xf02b,
	CELL_SYSMODULE_MUSIC_EXPORT             = 0xf02c,
	CELL_SYSMODULE_PHOTO_DECODE             = 0xf02e,
	CELL_SYSMODULE_SYSUTIL_SEARCH           = 0xf02f,
	CELL_SYSMODULE_SYSUTIL_AVCHAT2          = 0xf030,
	CELL_SYSMODULE_SAIL_REC                 = 0xf034,
	CELL_SYSMODULE_SYSUTIL_NP_TROPHY        = 0xf035,
	CELL_SYSMODULE_LIBATRAC3MULTI           = 0xf054,

	CELL_SYSMODULE_INVALID                  = 0xffff
};

struct SysmoduleUnk
{
	be_t<s32> prx_id;
	be_t<s32> unk;
};

CHECK_SIZE_ALIGN(SysmoduleUnk, 8, 4);

error_code cellSysmoduleLoadModule(ppu_thread& ppu, u16 id);
error_code cellSysmoduleLoadModuleInternal(ppu_thread& ppu, u16 id);
error_code cellSysmoduleUnloadModule(ppu_thread& ppu, u16 id);
error_code cellSysmoduleUnloadModuleInternal(ppu_thread& ppu, u16 id);
error_code cellSysmoduleIsLoadedEx(ppu_thread& ppu, u16 id);
error_code cellSysmoduleSetDebugmode(u32 debug_mode);
