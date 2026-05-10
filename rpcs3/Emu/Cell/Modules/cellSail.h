#pragma once

#include "Emu/Memory/vm_ptr.h"
#include "cellVpost.h"

// Error Codes
enum CellSailError : u32
{
	CELL_SAIL_ERROR_INVALID_ARG        = 0x80610701,
	CELL_SAIL_ERROR_INVALID_STATE      = 0x80610702,
	CELL_SAIL_ERROR_UNSUPPORTED_STREAM = 0x80610703,
	CELL_SAIL_ERROR_INDEX_OUT_OF_RANGE = 0x80610704,
	CELL_SAIL_ERROR_EMPTY              = 0x80610705,
	CELL_SAIL_ERROR_FULLED             = 0x80610706,
	CELL_SAIL_ERROR_USING              = 0x80610707,
	CELL_SAIL_ERROR_NOT_AVAILABLE      = 0x80610708,
	CELL_SAIL_ERROR_CANCEL             = 0x80610709,
	CELL_SAIL_ERROR_MEMORY             = 0x806107F0,
	CELL_SAIL_ERROR_INVALID_FD         = 0x806107F1,
	CELL_SAIL_ERROR_FATAL              = 0x806107FF,
};

// Call types
enum
{
	CELL_SAIL_PLAYER_CALL_NONE                  = 0,
	CELL_SAIL_PLAYER_CALL_BOOT                  = 1,
	CELL_SAIL_PLAYER_CALL_OPEN_STREAM           = 2,
	CELL_SAIL_PLAYER_CALL_CLOSE_STREAM          = 3,
	CELL_SAIL_PLAYER_CALL_OPEN_ES_AUDIO         = 4,
	CELL_SAIL_PLAYER_CALL_OPEN_ES_VIDEO         = 5,
	CELL_SAIL_PLAYER_CALL_OPEN_ES_USER          = 6,
	CELL_SAIL_PLAYER_CALL_CLOSE_ES_AUDIO        = 7,
	CELL_SAIL_PLAYER_CALL_CLOSE_ES_VIDEO        = 8,
	CELL_SAIL_PLAYER_CALL_CLOSE_ES_USER         = 9,
	CELL_SAIL_PLAYER_CALL_START                 = 10,
	CELL_SAIL_PLAYER_CALL_STOP                  = 11,
	CELL_SAIL_PLAYER_CALL_NEXT                  = 12,
	CELL_SAIL_PLAYER_CALL_REOPEN_ES_AUDIO       = 13,
	CELL_SAIL_PLAYER_CALL_REOPEN_ES_VIDEO       = 14,
	CELL_SAIL_PLAYER_CALL_REOPEN_ES_USER        = 15,

	_CELL_SAIL_PLAYER_CALL_TYPE_NUM_OF_ELEMENTS = 16, // Never used?
};

// State types
enum
{
	CELL_SAIL_PLAYER_STATE_INITIALIZED           = 0,
	CELL_SAIL_PLAYER_STATE_BOOT_TRANSITION       = 1,
	CELL_SAIL_PLAYER_STATE_CLOSED                = 2,
	CELL_SAIL_PLAYER_STATE_OPEN_TRANSITION       = 3,
	CELL_SAIL_PLAYER_STATE_OPENED                = 4,
	CELL_SAIL_PLAYER_STATE_START_TRANSITION      = 5,
	CELL_SAIL_PLAYER_STATE_RUNNING               = 6,
	CELL_SAIL_PLAYER_STATE_STOP_TRANSITION       = 7,
	CELL_SAIL_PLAYER_STATE_CLOSE_TRANSITION      = 8,
	CELL_SAIL_PLAYER_STATE_LOST                  = 9,
	_CELL_SAIL_PLAYER_STATE_TYPE_NUM_OF_ELEMENTS = 10, // Never used?
};

// Preset types
enum
{
	CELL_SAIL_PLAYER_PRESET_AV_SYNC             = 0, // Deprecated, same as 59_94HZ
	CELL_SAIL_PLAYER_PRESET_AS_IS               = 1,
	CELL_SAIL_PLAYER_PRESET_AV_SYNC_59_94HZ     = 2,
	CELL_SAIL_PLAYER_PRESET_AV_SYNC_29_97HZ     = 3,
	CELL_SAIL_PLAYER_PRESET_AV_SYNC_50HZ        = 4,
	CELL_SAIL_PLAYER_PRESET_AV_SYNC_25HZ        = 5,
	CELL_SAIL_PLAYER_PRESET_AV_SYNC_AUTO_DETECT = 6,
};

// Event types
enum
{
	CELL_SAIL_EVENT_EMPTY                 = 0, // NEVER USED
	CELL_SAIL_EVENT_ERROR_OCCURED         = 1,
	CELL_SAIL_EVENT_PLAYER_CALL_COMPLETED = 2,
	CELL_SAIL_EVENT_PLAYER_STATE_CHANGED  = 3,
	CELL_SAIL_EVENT_STREAM_OPENED         = 4,
	CELL_SAIL_EVENT_STREAM_CLOSED         = 5,
	CELL_SAIL_EVENT_SESSION_STARTED       = 6,
	CELL_SAIL_EVENT_PAUSE_STATE_CHANGED   = 7,
	CELL_SAIL_EVENT_SOURCE_EOS            = 8,
	CELL_SAIL_EVENT_ES_OPENED             = 9,
	CELL_SAIL_EVENT_ES_CLOSED             = 10,
	CELL_SAIL_EVENT_MEDIA_STATE_CHANGED   = 11,

	_CELL_SAIL_EVENT_TYPE_NUM_OF_ELEMENTS = 12, // DO NOT USE
};

// Parameter types
enum
{
	CELL_SAIL_PARAMETER_ENABLE_VPOST                          = 0,

	// Player
	CELL_SAIL_PARAMETER_CONTROL_QUEUE_DEPTH                   = 1,
	CELL_SAIL_PARAMETER_CONTROL_PPU_THREAD_PRIORITY           = 2,

	// SPURS
	CELL_SAIL_PARAMETER_SPURS_NUM_OF_SPUS                     = 3,
	CELL_SAIL_PARAMETER_SPURS_SPU_THREAD_PRIORITY             = 4,
	CELL_SAIL_PARAMETER_SPURS_PPU_THREAD_PRIORITY             = 5,
	CELL_SAIL_PARAMETER_SPURS_EXIT_IF_NO_WORK                 = 6,

	// Source
	CELL_SAIL_PARAMETER_IO_PPU_THREAD_PRIORITY                = 7,

	// Dmux
	CELL_SAIL_PARAMETER_DMUX_PPU_THREAD_PRIORITY              = 8,
	CELL_SAIL_PARAMETER_DMUX_SPU_THREAD_PRIORITY              = 9, // Deprecated
	CELL_SAIL_PARAMETER_DMUX_NUM_OF_SPUS                      = 10,
	CELL_SAIL_PARAMETER_DMUX_SPURS_TASK_PRIORITIES            = 11,

	// Adec
	CELL_SAIL_PARAMETER_ADEC_PPU_THREAD_PRIORITY              = 12,
	CELL_SAIL_PARAMETER_ADEC_SPU_THREAD_PRIORITY              = 13, // Deprecated
	CELL_SAIL_PARAMETER_ADEC_NUM_OF_SPUS                      = 14,
	CELL_SAIL_PARAMETER_ADEC_SPURS_TASK_PRIORITIES            = 15,

	// Vdec
	CELL_SAIL_PARAMETER_VDEC_PPU_THREAD_PRIORITY              = 16,
	CELL_SAIL_PARAMETER_VDEC_SPU_THREAD_PRIORITY              = 17, // Deprecated
	CELL_SAIL_PARAMETER_VDEC_M2V_NUM_OF_SPUS                  = 18,
	CELL_SAIL_PARAMETER_VDEC_AVC_NUM_OF_SPUS                  = 19,
	CELL_SAIL_PARAMETER_VDEC_SPURS_TASK_PRIORITIES            = 20,

	// Vpost */
	CELL_SAIL_PARAMETER_VPOST_PPU_THREAD_PRIORITY             = 21, // Deprecated
	CELL_SAIL_PARAMETER_VPOST_SPU_THREAD_PRIORITY             = 22, // Deprecated
	CELL_SAIL_PARAMETER_VPOST_NUM_OF_SPUS                     = 23,
	CELL_SAIL_PARAMETER_VPOST_SPURS_TASK_PRIORITIES           = 24,

	// Graphics Adapter
	CELL_SAIL_PARAMETER_GRAPHICS_ADAPTER_BUFFER_RELEASE_DELAY = 25,

	// AV Sync
	CELL_SAIL_PARAMETER_AV_SYNC_ES_AUDIO                      = 26,
	CELL_SAIL_PARAMETER_AV_SYNC_ES_VIDEO                      = 27,
	CELL_SAIL_PARAMETER_AV_SYNC_ES_USER                       = 28, // Not available

	// Control
	CELL_SAIL_PARAMETER_CONTROL_PPU_THREAD_STACK_SIZE         = 29,
	CELL_SAIL_PARAMETER_RESERVED0_                            = 30, // Should be never used
	CELL_SAIL_PARAMETER_RESERVED1                             = 31, // Should be never used

	// Apost
	CELL_SAIL_PARAMETER_ENABLE_APOST_SRC                      = 32,

	// File I/O Interface
	CELL_SAIL_PARAMETER_FS                                    = 33,
	CELL_SAIL_PARAMETER_IO_PPU_THREAD_STACK_SIZE              = 34,
	CELL_SAIL_PARAMETER_VIDEO_PERFORMANCE_POLICY              = 35,
	_CELL_SAIL_PARAMETER_TYPE_NUM_OF_ELEMENTS                 = 36, // Should be never used
	CELL_SAIL_PARAMETER_SOURCE_PPU_THREAD_PRIORITY            = CELL_SAIL_PARAMETER_IO_PPU_THREAD_PRIORITY,
	CELL_SAIL_PARAMETER_DMUX_SPURS_TASK_PRIORITY              = CELL_SAIL_PARAMETER_DMUX_SPURS_TASK_PRIORITIES, // Deprecated
	CELL_SAIL_PARAMETER_VDEC_SPURS_TASK_PRIORITY              = CELL_SAIL_PARAMETER_VDEC_SPURS_TASK_PRIORITIES, // Deprecated
	CELL_SAIL_PARAMETER_ADEC_SPURS_TASK_PRIORITY              = CELL_SAIL_PARAMETER_ADEC_SPURS_TASK_PRIORITIES, // Deprecated
	CELL_SAIL_PARAMETER_VPOST_SPURS_TASK_PRIORITY             = CELL_SAIL_PARAMETER_VPOST_SPURS_TASK_PRIORITIES, // Deprecated
};

// Media states
enum
{
	CELL_SAIL_MEDIA_STATE_FINE = 0,
	CELL_SAIL_MEDIA_STATE_BAD  = 1,
	CELL_SAIL_MEDIA_STATE_LOST = 2,
};

// Stream Types
enum
{
	CELL_SAIL_STREAM_PAMF = 0,
	CELL_SAIL_STREAM_MP4  = 1,
	CELL_SAIL_STREAM_AVI  = 2,

	CELL_SAIL_STREAM_UNSPECIFIED = -1,
};

// Sync Types
enum
{
	CELL_SAIL_SYNC_MODE_REPEAT = 1 << 0,
	CELL_SAIL_SYNC_MODE_SKIP   = 1 << 1,
};

// Flags
enum {
	CELL_SAIL_AVISF_DISABLED         = 0x00000001,
	CELL_SAIL_AVIF_HASINDEX          = 0x00000010,
	CELL_SAIL_AVIF_MUSTUSEINDEX      = 0x00000020,
	CELL_SAIL_AVIF_ISINTERLEAVED     = 0x00000100,
	CELL_SAIL_AVIF_WASCAPTUREFILE    = 0x00010000,
	CELL_SAIL_AVISF_VIDEO_PALCHANGES = 0x00010000,
	CELL_SAIL_AVIF_COPYRIGHTED       = 0x00020000,

	CELL_SAIL_AVIF_TRUSTCKTYPE       = 0x00000800, // Open-DML only
};

// Wave types
enum
{
	CELL_SAIL_WAVE_FORMAT_PCM = 0x0001,
	CELL_SAIL_WAVE_FORMAT_MPEG = 0x0050,
	CELL_SAIL_WAVE_FORMAT_MPEGLAYER3 = 0x0055,
	CELL_SAIL_WAVE_FORMAT_AC3 = 0x2000,
	CELL_SAIL_WAVE_FORMAT_UNSPECIFIED = 0xFFFF,
};

// MPEG Layers
enum
{
	CELL_SAIL_ACM_MPEG_LAYER1 = 0x0001,
	CELL_SAIL_ACM_MPEG_LAYER2 = 0x0002,
	CELL_SAIL_ACM_MPEG_LAYER3 = 0x0004,
};

// MPEG Modes
enum
{
	CELL_SAIL_ACM_MPEG_STEREO        = 0x0001,
	CELL_SAIL_ACM_MPEG_JOINTSTEREO   = 0x0002,
	CELL_SAIL_ACM_MPEG_DUALCHANNEL   = 0x0004,
	CELL_SAIL_ACM_MPEG_SINGLECHANNEL = 0x0008,
};

// MPEG Flags
enum
{
	CELL_SAIL_ACM_MPEG_PRIVATEBIT    = 0x0001,
	CELL_SAIL_ACM_MPEG_COPYRIGHT     = 0x0002,
	CELL_SAIL_ACM_MPEG_ORIGINALHOME  = 0x0004,
	CELL_SAIL_ACM_MPEG_PROTECTIONBIT = 0x0008,
	CELL_SAIL_ACM_MPEG_ID_MPEG1      = 0x0010,
};

// MPEG Layer 3 Flags
enum
{
	CELL_SAIL_MPEGLAYER3_ID_UNKNOWN           = 0,
	CELL_SAIL_MPEGLAYER3_ID_MPEG              = 1,
	CELL_SAIL_MPEGLAYER3_ID_CONSTANTFRAMESIZE = 2,
	CELL_SAIL_MPEGLAYER3_FLAG_PADDING_ISO     = 0x00000000,
	CELL_SAIL_MPEGLAYER3_FLAG_PADDING_ON      = 0x00000001,
	CELL_SAIL_MPEGLAYER3_FLAG_PADDING_OFF     = 0x00000002,
};

// ES Types
enum
{
	CELL_SAIL_ES_AUDIO = 0,
	CELL_SAIL_ES_VIDEO = 1,
	CELL_SAIL_ES_USER  = 2,
};

// Audio Coding Types
enum
{
	CELL_SAIL_AUDIO_CODING_UNSPECIFIED  = -1,
	CELL_SAIL_AUDIO_CODING_LPCM_FLOAT32 = 1,
};

enum
{
	CELL_SAIL_AUDIO_CHNUM_UNSPECIFIED      = -1,
	CELL_SAIL_AUDIO_CH_NUM_UNSPECIFIED     = -1,
	CELL_SAIL_AUDIO_AUSAMPLE_UNSPECIFIED   = -1,
	CELL_SAIL_AUDIO_SAMPLE_NUM_UNSPECIFIED = -1,
};

enum
{
	CELL_SAIL_AUDIO_FS_32000HZ     = 32000,
	CELL_SAIL_AUDIO_FS_44100HZ     = 44100,
	CELL_SAIL_AUDIO_FS_48000HZ     = 48000,

	CELL_SAIL_AUDIO_FS_96000HZ     = 96000,
	CELL_SAIL_AUDIO_FS_88200HZ     = 88200,
	CELL_SAIL_AUDIO_FS_64000HZ     = 64000,
	//CELL_SAIL_AUDIO_FS_48000HZ   = 48000,
	//CELL_SAIL_AUDIO_FS_44100HZ   = 44100,
	//CELL_SAIL_AUDIO_FS_32000HZ   = 32000,
	CELL_SAIL_AUDIO_FS_24000HZ     = 24000,
	CELL_SAIL_AUDIO_FS_22050HZ     = 22050,
	CELL_SAIL_AUDIO_FS_16000HZ     = 16000,
	CELL_SAIL_AUDIO_FS_12000HZ     = 12000,
	CELL_SAIL_AUDIO_FS_11025HZ     = 11025,
	CELL_SAIL_AUDIO_FS_8000HZ      = 8000,
	CELL_SAIL_AUDIO_FS_7350HZ      = 7350,

	CELL_SAIL_AUDIO_FS_192000HZ    = 192000,
	//CELL_SAIL_AUDIO_FS_11024HZ   = 11025,
	CELL_SAIL_AUDIO_FS_UNSPECIFIED = -1,

};

enum
{
	CELL_SAIL_AUDIO_CH_LAYOUT_UNDEFINED    = 0,

	// monoral
	CELL_SAIL_AUDIO_CH_LAYOUT_1CH          = 1,

	// 1. Front Left
	// 2. Front Right
	CELL_SAIL_AUDIO_CH_LAYOUT_2CH_LR       = 2,

	// 1. Front Left
	// 2. Front Center
	// 3. Front Right
	// for m4aac ac3
	CELL_SAIL_AUDIO_CH_LAYOUT_3CH_LCR      = 3,

	// 1. Front Left
	// 2. Front Center
	// 3. Surround
	// for m4aac ac3
	CELL_SAIL_AUDIO_CH_LAYOUT_3CH_LRc      = 4,

	// 1. Front Left
	// 2. Front Center
	// 3. Front Right
	// 4. Surround
	// for m4aac ac3
	CELL_SAIL_AUDIO_CH_LAYOUT_4CH_LCRc     = 5,

	// 1. Front Left
	// 2. Front Right
	// 3. Surround Left
	// 4. Surround Right
	// for m4aac
	CELL_SAIL_AUDIO_CH_LAYOUT_4CH_LRlr     = 6,

	// 1. Front Left
	// 2. Front Center
	// 3. Front Right
	// 4. Surround Left
	// 5. Surround Right
	// for m4aac
	CELL_SAIL_AUDIO_CH_LAYOUT_5CH_LCRlr    = 7,

	// 1. Front Left
	// 2. Front Center
	// 3. Front Right
	// 4. Surround Left
	// 5. Surround Right
	// 6. LFE
	// for lpcm ac3 m4aac
	CELL_SAIL_AUDIO_CH_LAYOUT_6CH_LCRlrE   = 8,

	// 1. Front Left
	// 2. Front Center
	// 3. Front Right
	// 4. Back Left
	// 5. Back Right
	// 6. LFE
	// for at3plus
	CELL_SAIL_AUDIO_CH_LAYOUT_6CH_LCRxyE   = 9,

	// 1. Front Left
	// 2. Front Center
	// 3. Front Right
	// 4. Back Left
	// 5. Back Right
	// 6. Back Center
	// 7. LFE
	// (for at3plus)
	CELL_SAIL_AUDIO_CH_LAYOUT_7CH_LCRxycE  = 10,

	// 1. Front Left
	// 2. Front Center
	// 3. Front Right
	// 4. LFE
	// 5. Surround Left
	// 6. Surround Right
	// 7. Back Left  (Left-Extend)
	// 8. Back Right (Right-Extend)
	// for lpcm at3plus
	CELL_SAIL_AUDIO_CH_LAYOUT_8CH_LRCElrxy = 11,

	CELL_SAIL_AUDIO_CH_LAYOUT_2CH_DUAL     = 12,
	CELL_SAIL_AUDIO_CH_LAYOUT_UNSPECIFIED  = -1,
};

// Video Codings
enum
{
	CELL_SAIL_VIDEO_CODING_UNSPECIFIED                 = -1,
	CELL_SAIL_VIDEO_CODING_ARGB_INTERLEAVED            = 0,
	CELL_SAIL_VIDEO_CODING_RGBA_INTERLEAVED            = 1,
	CELL_SAIL_VIDEO_CODING_YUV422_U_Y0_V_Y1            = 2,
	CELL_SAIL_VIDEO_CODING_YUV420_PLANAR               = 3,

	// Suported by cellCamera
	CELL_SAIL_VIDEO_CODING_YUV422_Y0_U_Y1_V            = 4,
	CELL_SAIL_VIDEO_CODING_YUV422_V_Y1_U_Y0            = 9,
	CELL_SAIL_VIDEO_CODING_YUV422_Y1_V_Y0_U            = 10,
	CELL_SAIL_VIDEO_CODING_JPEG                        = 11,
	CELL_SAIL_VIDEO_CODING_RAW8_BAYER_BGGR             = 12,
	_CELL_SAIL_VIDEO_CODING_TYPE_NUM_OF_ELEMENTS       = 13,
	CELL_SAIL_VIDEO_CODING_UYVY422_INTERLEAVED         = 2,
	CELL_SAIL_VIDEO_CODING_YUYV422_INTERLEAVED         = 4,
	CELL_SAIL_VIDEO_CODING_VYUY422_REVERSE_INTERLEAVED = 9,
	CELL_SAIL_VIDEO_CODING_RAW8_BAYER_GRBG             = 12,
};

// Video Color Types
enum
{
	CELL_SAIL_VIDEO_COLOR_MATRIX_UNSPECIFIED           = -1,
	CELL_SAIL_VIDEO_COLOR_MATRIX_BT601                 = 0,
	CELL_SAIL_VIDEO_COLOR_MATRIX_BT709                 = 1,
	_CELL_SAIL_VIDEO_COLOR_MATRIX_TYPE_NUM_OF_ELEMENTS = 2,
};

// Video Scan Types
enum
{
	CELL_SAIL_VIDEO_SCAN_UNSPECIFIED           = -1,
	CELL_SAIL_VIDEO_SCAN_PROGRESSIVE           = 0,
	CELL_SAIL_VIDEO_SCAN_INTERLACE             = 1,
	_CELL_SAIL_VIDEO_SCAN_TYPE_NUM_OF_ELEMENTS = 2,
};

// Framerates
enum
{
	CELL_SAIL_VIDEO_FRAME_RATE_UNSPECIFIED           = -1,
	CELL_SAIL_VIDEO_FRAME_RATE_24000_1001HZ          = 0,
	CELL_SAIL_VIDEO_FRAME_RATE_24HZ                  = 1,
	CELL_SAIL_VIDEO_FRAME_RATE_25HZ                  = 2,
	CELL_SAIL_VIDEO_FRAME_RATE_30000_1001HZ          = 3,
	CELL_SAIL_VIDEO_FRAME_RATE_30HZ                  = 4,
	CELL_SAIL_VIDEO_FRAME_RATE_50HZ                  = 5,
	CELL_SAIL_VIDEO_FRAME_RATE_60000_1001HZ          = 6,
	CELL_SAIL_VIDEO_FRAME_RATE_60HZ                  = 7,
	_CELL_SAIL_VIDEO_FRAME_RATE_TYPE_NUM_OF_ELEMENTS = 8,
};

// Aspect Ratios
enum
{
	CELL_SAIL_VIDEO_ASPECT_RATIO_UNSPECIFIED = -1,
	CELL_SAIL_VIDEO_ASPECT_RATIO_1_1         = 1, // 1920x1080 1280x720
	CELL_SAIL_VIDEO_ASPECT_RATIO_12_11       = 2, // 720x576 normal
	CELL_SAIL_VIDEO_ASPECT_RATIO_10_11       = 3, // 720x480 normal
	CELL_SAIL_VIDEO_ASPECT_RATIO_16_11       = 4, // 720x576 wide
	CELL_SAIL_VIDEO_ASPECT_RATIO_40_33       = 5, // 720x480 wide
	CELL_SAIL_VIDEO_ASPECT_RATIO_4_3         = 14, // 1440x1080
};

enum
{
	CELL_SAIL_VIDEO_WIDTH_UNSPECIFIED          = -1,
	CELL_SAIL_VIDEO_HEIGHT_UNSPECIFIED         = -1,
	CELL_SAIL_VIDEO_PITCH_UNSPECIFIED          = -1,
	CELL_SAIL_VIDEO_BITS_PER_COLOR_UNSPECIFIED = -1,
	CELL_SAIL_VIDEO_ALPHA_UNSPECIFIED          = -1,
};

// Color Ranges
enum
{
	CELL_SAIL_VIDEO_COLOR_RANGE_UNSPECIFIED = -1,
	CELL_SAIL_VIDEO_COLOR_RANGE_LIMITED     = 1,
	CELL_SAIL_VIDEO_COLOR_RANGE_FULL        = 0,
};

enum
{
	CELL_SAIL_START_NOT_SPECIFIED   = 0,
	CELL_SAIL_START_NORMAL          = 1 << 0, //1
	CELL_SAIL_START_TIME_SCALE      = 1 << 2, //4
	CELL_SAIL_START_EP_SKIP         = 1 << 4, //16
	CELL_SAIL_START_EP_SKIP_REVERSE = 1 << 5, //32
	CELL_SAIL_START_FRAME_STEP      = 1 << 6, //64
};

// Seek Types
enum
{
	CELL_SAIL_SEEK_NOT_SPECIFIED          = 0,
	CELL_SAIL_SEEK_ABSOLUTE_BYTE_POSITION = 1 << 0, // For PAMF
	CELL_SAIL_SEEK_RELATIVE_BYTE_POSITION = 1 << 1, // Not implemented
	CELL_SAIL_SEEK_ABSOLUTE_TIME_POSITION = 1 << 4, // MP4, AVI
	CELL_SAIL_SEEK_CURRENT_POSITION       = 1 << 6,
	CELL_SAIL_SEEK_MP4_SCALE_AND_TIME     = 1 << 4, // For MP4, obsolete
};

// Terminus Types
enum
{
	CELL_SAIL_TERMINUS_NOT_SPECIFIED          = 0,
	CELL_SAIL_TERMINUS_EOS                    = 1 << 0,
	CELL_SAIL_TERMINUS_ABSOLUTE_BYTE_POSITION = 1 << 1, // For PAMF
	CELL_SAIL_TERMINUS_RELATIVE_BYTE_POSITION = 1 << 2, // Mot implemented
	CELL_SAIL_TERMINUS_ABSOLUTE_TIME_POSITION = 1 << 5, // For MP4, AVI
	CELL_SAIL_TERMINUS_MP4_SCALE_AND_TIME     = 1 << 5, // For MP4, obsolete
	CELL_SAIL_TERMINUS_MP4_SCALE_ANT_TIME     = 1 << 5, // For MP4, here because of a typo
};

// Start Flag Types
enum
{
	CELL_SAIL_START_FLAG_NOT_SPECIFIED   = 0,
	CELL_SAIL_START_FLAG_UNFLUSH         = 1 << 0,
	CELL_SAIL_START_FLAG_PAUSE_BEGIN     = 1 << 1,
	CELL_SAIL_START_FLAG_PAUSE_END       = 1 << 2,
	CELL_SAIL_START_FLAG_COMPLETE_STREAM = 1 << 3,
	CELL_SAIL_START_FLAG_STICKY          = 1 << 4,
	CELL_SAIL_START_FLAG_PAUSE           = 1 << 1, // Obsolete
};

enum
{
	_CELL_SAIL_SYNC_SHIFT_NUM      = 8,

	// Buffering
	CELL_SAIL_SYNC_UNDERFLOW       = 1,
	//                             = 2, Reserved

	// Sync Status
	CELL_SAIL_SYNC_ON_TIME         = 1 << 2,
	CELL_SAIL_SYNC_MAYBE_ON_TIME   = 2 << 2,
	CELL_SAIL_SYNC_EARLY           = 3 << 2,
	CELL_SAIL_SYNC_LATE            = 4 << 2,
	CELL_SAIL_SYNC_NO_SYNC         = 5 << 2,
	CELL_SAIL_SYNC_NO_PTS          = 6 << 2,
	CELL_SAIL_SYNC_NOT_READY       = 7 << 2,
	CELL_SAIL_SYNC_DISABLED        = 8 << 2,
	CELL_SAIL_SYNC_PAUSED          = 9 << 2,
	CELL_SAIL_SYNC_DISABLED_PAUSED = 10 << 2,
	CELL_SAIL_SYNC_MUTED           = 11 << 2,
	CELL_SAIL_SYNC_DONE            = 12 << 2,
	//                             = 13 << 2, Reserved
	//                             = 14 << 2, Reserved
	//                             = 15 << 2, Reserved

	//CELL_SAIL_SYNC_FIRST_FRAME   = 64,
	//CELL_SAIL_SYNC_LAST_FRAME    = 128,


	// Frame Status
	CELL_SAIL_SYNC_NO_FRAME        = 0,
	CELL_SAIL_SYNC_REPEATED        = 1 << _CELL_SAIL_SYNC_SHIFT_NUM,
	CELL_SAIL_SYNC_NEXT            = 2 << _CELL_SAIL_SYNC_SHIFT_NUM,
	CELL_SAIL_SYNC_SKIPPED_ONE     = 3 << _CELL_SAIL_SYNC_SHIFT_NUM,
};

enum
{
	CELL_SAIL_EVENT_RECORDER_CALL_COMPLETED = 2,
	CELL_SAIL_EVENT_RECORDER_STATE_CHANGED  = 3,
};


enum
{
	CELL_SAIL_VIDEO_FRAME_RATE_100HZ         = 8,
	CELL_SAIL_VIDEO_FRAME_RATE_120000_1001HZ = 9,
	CELL_SAIL_VIDEO_FRAME_RATE_120HZ         = 10,
};

enum
{
	CELL_SAIL_GRAPHICS_ADAPTER_FIELD_TOP       = 0,
	CELL_SAIL_GRAPHICS_ADAPTER_FIELD_BOTTOM    = 1,
	CELL_SAIL_GRAPHICS_ADAPTER_FIELD_DONT_CARE = 2,
};

enum
{
	CELL_SAIL_SOURCE_SEEK_ABSOLUTE_BYTE_POSITION = 1 << 0,
};

enum
{
	CELL_SAIL_SOURCE_CAPABILITY_NONE                        = 0,
	CELL_SAIL_SOURCE_CAPABILITY_SEEK_ABSOLUTE_BYTE_POSITION = 1 << 0,
	CELL_SAIL_SOURCE_CAPABILITY_PAUSE                       = 1 << 4,
	CELL_SAIL_SOURCE_CAPABILITY_GAPLESS                     = 1 << 5,
	CELL_SAIL_SOURCE_CAPABILITY_EOS                         = 1 << 6,
	CELL_SAIL_SOURCE_CAPABILITY_SEEK_ABSOLUTE_TIME_POSITION = 1 << 7,
};

struct CellSailAudioFormat
{
	s8 coding;
	s8 chNum;
	be_t<s16> sampleNum;
	be_t<s32> fs;
	be_t<s32> chLayout;
	be_t<s32> reserved0; // Specify both -1
	be_t<s64> reserved1;
};

struct CellSailAudioFrameInfo
{
	be_t<u32> pPcm;
	be_t<s32> status;
	be_t<u64> pts;
	be_t<u64> reserved; // Specify 0
};

struct CellSailVideoFormat
{
	s8 coding;
	s8 scan;
	s8 bitsPerColor;
	s8 frameRate;
	be_t<s16> width;
	be_t<s16> height;
	be_t<s32> pitch;
	be_t<s32> alpha;
	s8 colorMatrix;
	s8 aspectRatio;
	s8 colorRange;
	s8 reserved1; // Specify all three -1
	be_t<s32> reserved2;
	be_t<s64> reserved3;
};

struct CellSailVideoFrameInfo
{
	be_t<u32> pPic;
	be_t<s32> status;
	be_t<u64> pts;
	be_t<u64> reserved; // Specify both 0
	be_t<u16> interval;
	u8 structure;
	s8 repeatNum;
	u8 reserved2[4];
};

struct CellSailSourceBufferItem
{
	u8 pBuf;
	be_t<u32> size;
	be_t<u32> sessionId;
	be_t<u32> reserved; // Specify 0
};

struct CellSailSourceStartCommand
{
	be_t<u64> startFlags;
	be_t<s64> startArg;
	be_t<s64> lengthArg;
	be_t<u64> optionalArg0;
	be_t<u64> optionalArg1;
};

struct CellSailSourceStreamingProfile
{
	be_t<u32> reserved0; // Specify 0
	be_t<u32> numItems;
	be_t<u32> maxBitrate;
	be_t<u32> reserved1; // Specify 0
	be_t<u64> duration;
	be_t<u64> streamSize;
};

union CellSailEvent
{
	struct
	{
		be_t<u32> major;
		be_t<u32> minor;
	}
	u32x2;

	be_t<u64> value;
};

template <typename T>
struct ppu_gpr_cast_impl;

template <>
struct ppu_gpr_cast_impl<CellSailEvent>
{
	static inline u64 to(const CellSailEvent& event)
	{
		return event.value;
	}
};

using CellSailMemAllocatorFuncAlloc = vm::ptr<void>(vm::ptr<void> pArg, u32 boundary, u32 size);
using CellSailMemAllocatorFuncFree = void(vm::ptr<void> pArg, u32 boundary, vm::ptr<void> pMemory);

using CellSailSoundAdapterFuncMakeup = s32(vm::ptr<void> pArg);
using CellSailSoundAdapterFuncCleanup = s32(vm::ptr<void> pArg);
using CellSailSoundAdapterFuncFormatChanged = void(vm::ptr<void> pArg, vm::ptr<CellSailAudioFormat> pFormat, u32 sessionId);

using CellSailGraphicsAdapterFuncMakeup = s32(vm::ptr<void> pArg);
using CellSailGraphicsAdapterFuncCleanup = s32(vm::ptr<void> pArg);
using CellSailGraphicsAdapterFuncFormatChanged = void(vm::ptr<void> pArg, vm::ptr<CellSailVideoFormat> pFormat, u32 sessionId);
using CellSailGraphicsAdapterFuncAllocFrame = s32(vm::ptr<void> pArg, u32 size, s32 num, vm::pptr<u8> ppFrame);
using CellSailGraphicsAdapterFuncFreeFrame = s32(vm::ptr<void> pArg, s32 num, vm::pptr<u8> ppFrame);

using CellSailSourceFuncMakeup = s32(vm::ptr<void> pArg, vm::cptr<char> pProtocolNames);
using CellSailSourceFuncCleanup = s32(vm::ptr<void> pArg);
using CellSailSourceFuncOpen = void(vm::ptr<void> pArg, s32 streamType, vm::ptr<void> pMediaInfo, vm::cptr<char> pUri, vm::ptr<CellSailSourceStreamingProfile> pProfile);
using CellSailSourceFuncClose = void(vm::ptr<void> pArg);
using CellSailSourceFuncStart = void(vm::ptr<void> pArg, vm::ptr<CellSailSourceStartCommand> pCommand, u32 sessionId);
using CellSailSourceFuncStop = void(vm::ptr<void> pArg);
using CellSailSourceFuncCancel = void(vm::ptr<void> pArg);
using CellSailSourceFuncCheckout = s32(vm::ptr<void> pArg, vm::pptr<CellSailSourceBufferItem> ppItem);
using CellSailSourceFuncCheckin = s32(vm::ptr<void> pArg, vm::ptr<CellSailSourceBufferItem> pItem);
using CellSailSourceFuncClear = s32(vm::ptr<void> pArg);
using CellSailSourceFuncRead = s32(vm::ptr<void> pArg, s32 streamType, vm::ptr<void> pMediaInfo, vm::cptr<char> pUri, u64 offset, vm::ptr<u8> pBuf, u32 size, vm::ptr<u64> pTotalSize);
using CellSailSourceFuncReadSync = s32(vm::ptr<void> pArg, s32 streamType, vm::ptr<void> pMediaInfo, vm::cptr<char> pUri, u64 offset, vm::ptr<u8> pBuf, u32 size, vm::ptr<u64> pTotalSize);
using CellSailSourceFuncGetCapabilities = s32(vm::ptr<void> pArg, s32 streamType, vm::ptr<void> pMediaInfo, vm::cptr<char> pUri, vm::ptr<u64> pCapabilities);
using CellSailSourceFuncInquireCapability = s32(vm::ptr<void> pArg, s32 streamType, vm::ptr<void> pMediaInfo, vm::cptr<char> pUri, vm::ptr<CellSailSourceStartCommand> pCommand);
using CellSailSourceCheckFuncError = void(vm::ptr<void> pArg, vm::cptr<char> pMsg, s32 line);

using CellSailFsFuncOpen = s32(vm::cptr<char> pPath, s32 flag, vm::ptr<s32> pFd, vm::ptr<void> pArg, u64 size);
using CellSailFsFuncOpenSecond = s32(vm::cptr<char> pPath, s32 flag, s32 fd, vm::ptr<void> pArg, u64 size);
using CellSailFsFuncClose = s32(s32 fd);
using CellSailFsFuncFstat = s32(s32 fd, vm::ptr<struct CellFstat> pStat);
using CellSailFsFuncRead = s32(s32 fd, vm::ptr<void> pBuf, u64 numBytes, vm::ptr<u64> pNumRead);
using CellSailFsFuncLseek = s32(s32 fd, s64 offset, s32 whence, vm::ptr<u64> pPosition);
using CellSailFsFuncCancel = s32(s32 fd);

using CellSailRendererAudioFuncMakeup = s32(vm::ptr<void> pArg);
using CellSailRendererAudioFuncCleanup = s32(vm::ptr<void> pArg);
using CellSailRendererAudioFuncOpen = void(vm::ptr<void> pArg, vm::ptr<CellSailAudioFormat> pInfo, u32 frameNum);
using CellSailRendererAudioFuncClose = void(vm::ptr<void> pArg);
using CellSailRendererAudioFuncStart = void(vm::ptr<void> pArg, b8 buffering);
using CellSailRendererAudioFuncStop = void(vm::ptr<void> pArg, b8 flush);
using CellSailRendererAudioFuncCancel = void(vm::ptr<void> pArg);
using CellSailRendererAudioFuncCheckout = s32(vm::ptr<void> pArg, vm::pptr<CellSailAudioFrameInfo> ppInfo);
using CellSailRendererAudioFuncCheckin = s32(vm::ptr<void> pArg, vm::ptr<CellSailAudioFrameInfo> pInfo);

using CellSailRendererVideoFuncMakeup = s32(vm::ptr<void> pArg);
using CellSailRendererVideoFuncCleanup = s32(vm::ptr<void> pArg);
using CellSailRendererVideoFuncOpen = void(vm::ptr<void> pArg, vm::ptr<CellSailVideoFormat> pInfo, u32 frameNum, u32 minFrameNum);
using CellSailRendererVideoFuncClose = void(vm::ptr<void> pArg);
using CellSailRendererVideoFuncStart = void(vm::ptr<void> pArg, b8 buffering);
using CellSailRendererVideoFuncStop = void(vm::ptr<void> pArg, b8 flush, b8 keepRendering);
using CellSailRendererVideoFuncCancel = void(vm::ptr<void> pArg);
using CellSailRendererVideoFuncCheckout = s32(vm::ptr<void> pArg, vm::pptr<CellSailVideoFrameInfo> ppInfo);
using CellSailRendererVideoFuncCheckin = s32(vm::ptr<void> pArg, vm::ptr<CellSailVideoFrameInfo> pInfo);

using CellSailPlayerFuncNotified = void(vm::ptr<void> pArg, CellSailEvent event, u64 arg0, u64 arg1);

struct CellSailMemAllocatorFuncs
{
	vm::ptr<CellSailMemAllocatorFuncAlloc> pAlloc;
	vm::ptr<CellSailMemAllocatorFuncFree> pFree;
};

struct CellSailMemAllocator
{
	vm::bptr<CellSailMemAllocatorFuncs> callbacks;
	be_t<u32> pArg;
};

struct CellSailFuture
{
	be_t<u32> mutex_id;
	be_t<u32> cond_id;
	volatile be_t<u32> flags;
	be_t<s32> result;
	be_t<u64> userParam;
};

struct CellSailSoundAdapterFuncs
{
	vm::bptr<CellSailSoundAdapterFuncMakeup> pMakeup;
	vm::bptr<CellSailSoundAdapterFuncCleanup> pCleanup;
	vm::bptr<CellSailSoundAdapterFuncFormatChanged> pFormatChanged;
};

struct CellSailSoundFrameInfo
{
	be_t<u32> pBuffer;
	be_t<u32> sessionId;
	be_t<u32> tag;
	be_t<s32> status;
	be_t<u64> pts;
};

struct CellSailSoundAdapter
{
	bool initialized;
	bool registered;
	vm::ptr<CellSailSoundAdapterFuncMakeup> pMakeup;
	vm::ptr<CellSailSoundAdapterFuncCleanup> pCleanup;
	vm::ptr<CellSailSoundAdapterFuncFormatChanged> pFormatChanged;
	vm::ptr<void> arg;
	be_t<u32> index;
	CellSailAudioFormat format;
};

CHECK_MAX_SIZE(CellSailSoundAdapter, 0x100);

struct CellSailGraphicsAdapterFuncs
{
	vm::bptr<CellSailGraphicsAdapterFuncMakeup> pMakeup;
	vm::bptr<CellSailGraphicsAdapterFuncCleanup> pCleanup;
	vm::bptr<CellSailGraphicsAdapterFuncFormatChanged> pFormatChanged;
	vm::bptr<CellSailGraphicsAdapterFuncAllocFrame> pAlloc;
	vm::bptr<CellSailGraphicsAdapterFuncFreeFrame> pFree;
};

struct CellSailGraphicsFrameInfo
{
	be_t<u32> pBuffer;
	be_t<u32> sessionId;
	be_t<u32> tag;
	be_t<s32> status;
	be_t<u64> pts;
};

struct CellSailGraphicsAdapter
{
	bool initialized;
	bool registered;
	vm::ptr<CellSailGraphicsAdapterFuncMakeup> pMakeup;
	vm::ptr<CellSailGraphicsAdapterFuncCleanup> pCleanup;
	vm::ptr<CellSailGraphicsAdapterFuncFormatChanged> pFormatChanged;
	vm::ptr<CellSailGraphicsAdapterFuncAllocFrame> pAlloc;
	vm::ptr<CellSailGraphicsAdapterFuncFreeFrame> pFree;
	vm::ptr<void> arg;
	CellSailVideoFormat format;
	be_t<s32> index;
};

CHECK_MAX_SIZE(CellSailGraphicsAdapter, 0x100);

struct CellSailAuInfo
{
	be_t<u32> pAu;
	be_t<u32> size;
	be_t<s32> status;
	be_t<u32> sessionId;
	be_t<u64> pts;
	be_t<u64> dts;
	be_t<u64> reserved; // Specify 0
};

struct CellSailAuReceiver
{
	be_t<u64> internalData[64];
};

struct CellSailRendererAudioFuncs
{
	vm::bptr<CellSailRendererAudioFuncMakeup> pMakeup;
	vm::bptr<CellSailRendererAudioFuncCleanup> pCleanup;
	vm::bptr<CellSailRendererAudioFuncOpen> pOpen;
	vm::bptr<CellSailRendererAudioFuncClose> pClose;
	vm::bptr<CellSailRendererAudioFuncStart> pStart;
	vm::bptr<CellSailRendererAudioFuncStop> pStop;
	vm::bptr<CellSailRendererAudioFuncCancel> pCancel;
	vm::bptr<CellSailRendererAudioFuncCheckout> pCheckout;
	vm::bptr<CellSailRendererAudioFuncCheckin> pCheckin;
};

struct CellSailRendererAudioAttribute
{
	be_t<u32> thisSize;
	CellSailAudioFormat pPreferredFormat;
};

struct CellSailRendererAudio
{
	be_t<u64> internalData[32];
};

struct CellSailRendererVideoFuncs
{
	vm::bptr<CellSailRendererVideoFuncMakeup> pMakeup;
	vm::bptr<CellSailRendererVideoFuncCleanup> pCleanup;
	vm::bptr<CellSailRendererVideoFuncOpen> pOpen;
	vm::bptr<CellSailRendererVideoFuncClose> pClose;
	vm::bptr<CellSailRendererVideoFuncStart> pStart;
	vm::bptr<CellSailRendererVideoFuncStop> pStop;
	vm::bptr<CellSailRendererVideoFuncCancel> pCancel;
	vm::bptr<CellSailRendererVideoFuncCheckout> pCheckout;
	vm::bptr<CellSailRendererVideoFuncCheckin> pCheckin;
};

struct CellSailRendererVideoAttribute
{
	be_t<u32> thisSize;
	CellSailVideoFormat *pPreferredFormat;
};

struct CellSailRendererVideo
{
	be_t<u64> internalData[32];
};

struct CellSailSourceFuncs
{
	vm::bptr<CellSailSourceFuncMakeup> pMakeup;
	vm::bptr<CellSailSourceFuncCleanup> pCleanup;
	vm::bptr<CellSailSourceFuncOpen> pOpen;
	vm::bptr<CellSailSourceFuncClose> pClose;
	vm::bptr<CellSailSourceFuncStart> pStart;
	vm::bptr<CellSailSourceFuncStop> pStop;
	vm::bptr<CellSailSourceFuncCancel> pCancel;
	vm::bptr<CellSailSourceFuncCheckout> pCheckout;
	vm::bptr<CellSailSourceFuncCheckin> pCheckin;
	vm::bptr<CellSailSourceFuncClear> pClear;
	vm::bptr<CellSailSourceFuncRead> pRead;
	vm::bptr<CellSailSourceFuncReadSync> pReadSync;
	vm::bptr<CellSailSourceFuncGetCapabilities> pGetCapabilities;
	vm::bptr<CellSailSourceFuncInquireCapability> pInquireCapability;
};

struct CellSailSource
{
	be_t<u64> internalData[20];
};

struct CellSailSourceCheckStream
{
	be_t<s32> streamType;
	be_t<u32> pMediaInfo;
	s8 pUri;
};

struct CellSailSourceCheckResource
{
	CellSailSourceCheckStream ok;
	CellSailSourceCheckStream readError;
	CellSailSourceCheckStream openError;
	CellSailSourceCheckStream startError;
	CellSailSourceCheckStream runningError;
};

template <template <class T, usz A = alignof(T)> class be_t = ::be_t>
struct CellSailMp4DateTime
{
	be_t<u16, 2> second;
	be_t<u16> minute;
	be_t<u16> hour;
	be_t<u16> day;
	be_t<u16> month;
	be_t<u16> year;
	be_t<u16> reserved0;
	be_t<u16> reserved1;
};

struct CellSailMp4Movie
{
	be_t<u64> internalData[16];
};

template <template <class T, usz A = alignof(T)> class be_t = ::be_t>
struct CellSailMp4MovieInfo
{
	CellSailMp4DateTime<be_t> creationDateTime;
	CellSailMp4DateTime<be_t> modificationDateTime;
	be_t<u32> trackCount;
	be_t<u32> movieTimeScale;
	be_t<u32> movieDuration;
	be_t<u32> reserved[16];
};

struct CellSailMp4Track
{
	be_t<u64> internalData[6];
};

struct CellSailMp4TrackInfo
{
	b8 isTrackEnabled;
	u8 reserved0[3];
	be_t<u32> trackId;
	be_t<u64> trackDuration;
	be_t<s16> layer;
	be_t<s16> alternateGroup;
	be_t<u16> reserved1[2];
	be_t<u32> trackWidth;
	be_t<u32> trackHeight;
	be_t<u16> language;
	be_t<u16> reserved2;
	be_t<u16> mediaType;
	be_t<u32> reserved3[3];
};

struct CellSailAviMovie
{
	be_t<u64> internalData[16];
};

struct CellSailAviMovieInfo
{
	be_t<u32> maxBytesPerSec;
	be_t<u32> flags;
	be_t<u32> reserved0;
	be_t<u32> streams;
	be_t<u32> suggestedBufferSize;
	be_t<u32> width;
	be_t<u32> height;
	be_t<u32> scale;
	be_t<u32> rate;
	be_t<u32> length;
	be_t<u32> reserved1;
	be_t<u32> reserved2;
};

struct CellSailAviMainHeader
{
	be_t<u32> microSecPerFrame;
	be_t<u32> maxBytesPerSec;
	be_t<u32> paddingGranularity;
	be_t<u32> flags;
	be_t<u32> totalFrames;
	be_t<u32> initialFrames;
	be_t<u32> streams;
	be_t<u32> suggestedBufferSize;
	be_t<u32> width;
	be_t<u32> height;
	be_t<u32> reserved[4];
};

struct CellSailAviExtendedHeader
{
	be_t<u32> totalFrames;
};

struct CellSailAviStream
{
	be_t<u64> internalData[2];
};

struct CellSailAviMediaType
{
	be_t<u32> fccType;
	be_t<u32> fccHandler;
	union u {
		struct audio {
			be_t<u16> formatTag;
			be_t<u16> reserved; // Specify 0
			union u {
				struct mpeg {
					be_t<u16> headLayer; // Specify 0
					be_t<u16> reserved; // Specify 0
				};
			};
		};
		struct video {
			be_t<u32> compression;
			be_t<u32> reserved;  // Specify 0
		};
	};
};

struct CellSailAviStreamHeader
{
	be_t<u32> fccType;
	be_t<u32> fccHandler;
	be_t<u32> flags;
	be_t<u16> priority;
	be_t<u32> initialFrames;
	be_t<u32> scale;
	be_t<u32> rate;
	be_t<u32> start;
	be_t<u32> length;
	be_t<u32> suggestedBufferSize;
	be_t<u32> quality;
	be_t<u32> sampleSize;
	struct frame {
		be_t<u16> left;
		be_t<u16> top;
		be_t<u16> right;
		be_t<u16> bottom;
	};
};

struct CellSailBitmapInfoHeader
{
	be_t<u32> size;
	be_t<s32> width;
	be_t<s32> height;
	be_t<u16> planes;
	be_t<u16> bitCount;
	be_t<u32> compression;
	be_t<u32> sizeImage;
	be_t<s32> xPelsPerMeter;
	be_t<s32> yPelsPerMeter;
	be_t<u32> clrUsed;
	be_t<u32> clrImportant;
};

struct CellSailWaveFormatEx
{
	be_t<u16> formatTag;
	be_t<u16> channels;
	be_t<u32> samplesPerSec;
	be_t<u32> avgBytesPerSec;
	be_t<u16> blockAlign;
	be_t<u16> bitsPerSample;
	be_t<u16> cbSize;
};

struct CellSailMpeg1WaveFormat
{
	CellSailWaveFormatEx wfx;
	be_t<u16> headLayer;
	be_t<u32> headBitrate;
	be_t<u16> headMode;
	be_t<u16> headModeExt;
	be_t<u16> headEmphasis;
	be_t<u16> headFlags;
	be_t<u32> PTSLow;
	be_t<u32> PTSHigh;
};

struct CellSailMpegLayer3WaveFormat
{
	CellSailWaveFormatEx wfx;
	be_t<u16> ID;
	be_t<u32> flags;
	be_t<u16> blockSize;
	be_t<u16> framesPerBlock;
	be_t<u16> codecDelay;
};

struct CellSailDescriptor
{
	b8 autoSelection;
	b8 registered;
	be_t<s32> streamType;
	be_t<u32> buffer;
	be_t<u32> sp_;
};

CHECK_MAX_SIZE(CellSailDescriptor, 0x100);

struct CellSailStartCommand
{
	be_t<u32> startType;
	be_t<u32> seekType;
	be_t<u32> terminusType;
	be_t<u32> flags;
	be_t<u32> startArg;
	be_t<u32> reserved;
	be_t<u64> seekArg;
	be_t<u64> terminusArg;
};

struct CellSailFsReadFuncs
{
	CellSailFsFuncOpen pOpen;
	CellSailFsFuncOpenSecond pOpenSecond;
	CellSailFsFuncClose pClose;
	CellSailFsFuncFstat pFstat;
	CellSailFsFuncRead pRead;
	CellSailFsFuncLseek pLseek;
	CellSailFsFuncCancel pCancel;
	be_t<u32> reserved[2]; // Specify 0
};

struct CellSailFsRead
{
	be_t<u32> capability;
	CellSailFsReadFuncs funcs;
};

struct CellSailPlayerAttribute
{
	be_t<s32> preset;
	be_t<u32> maxAudioStreamNum;
	be_t<u32> maxVideoStreamNum;
	be_t<u32> maxUserStreamNum;
	be_t<u32> queueDepth;
	be_t<u32> reserved0; // All three specify 0
	be_t<u32> reserved1;
	be_t<u32> reserved2;
};

struct CellSailPlayerResource
{
	be_t<u32> pSpurs;
	be_t<u32> reserved0; // All three specify 0
	be_t<u32> reserved1;
	be_t<u32> reserved2;
};

struct CellSailPlayer
{
	CellSailMemAllocator allocator;
	vm::ptr<CellSailPlayerFuncNotified> callback;
	vm::ptr<void> callbackArg;
	CellSailPlayerAttribute attribute;
	CellSailPlayerResource resource;
	vm::ptr<CellSailStartCommand> playbackCommand;
	s32 repeatMode;
	s32 descriptors;
	vm::ptr<CellSailDescriptor> registeredDescriptors[2];
	bool paused;
	bool booted;
	vm::ptr<CellSailSoundAdapter> sAdapter;
	vm::ptr<CellSailGraphicsAdapter> gAdapter;

	// Attributes
	be_t<s32> control_ppu_thread_priority;
	be_t<s32> control_ppu_thread_stack_size;
	be_t<s32> spurs_num_of_spus;
	be_t<s32> spurs_spu_thread_priority;
	be_t<s32> spurs_ppu_thread_priority;
	b8        spurs_exit_if_no_work;
	be_t<s32> io_ppu_thread_priority;
	be_t<s32> io_ppu_thread_stack_size;
	be_t<s32> dmux_ppu_thread_priority;
	be_t<s32> dmux_num_of_spus;
	be_t<u64> dmux_spurs_task_priorities;
	be_t<s32> adec_ppu_thread_priority;
	be_t<s32> adec_num_of_spus;
	be_t<u64> adec_spurs_task_priorities;
	b8 enable_apost_src;
	be_t<s32> vdec_ppu_thread_priority;
	be_t<s32> vdec_m2v_num_of_spus;
	be_t<s32> vdec_avc_num_of_spus;
	be_t<u64> vdec_spurs_task_priorities;
	vm::ptr<CellVpostCtrlParam> enable_vpost;
	be_t<s32> vpost_ppu_thread_priority;
	be_t<s32> vpost_num_of_spus;
	be_t<u64> vpost_spurs_task_priorities;
	be_t<s32> graphics_adapter_buffer_release_delay;
	be_t<s32> video_performance_policy;
	b8        av_sync_es_audio;
	b8        av_sync_es_video;
	CellSailFsRead fs;
};

CHECK_MAX_SIZE(CellSailPlayer, 0x100);

inline static const char* ParameterCodeToName(s32 code)
{
	switch (code)
	{
	case CELL_SAIL_PARAMETER_ENABLE_VPOST:                          return "ENABLE_VPOST";
	case CELL_SAIL_PARAMETER_CONTROL_QUEUE_DEPTH:                   return "CONTROL_QUEUE_DEPTH";
	case CELL_SAIL_PARAMETER_CONTROL_PPU_THREAD_PRIORITY:           return "CONTROL_PPU_THREAD_PRIORITY";
	case CELL_SAIL_PARAMETER_SPURS_NUM_OF_SPUS:                     return "SPURS_NUM_OF_SPUS";
	case CELL_SAIL_PARAMETER_SPURS_SPU_THREAD_PRIORITY:             return "SPURS_SPU_THREAD_PRIORITY";
	case CELL_SAIL_PARAMETER_SPURS_PPU_THREAD_PRIORITY:             return "SPURS_PPU_THREAD_PRIORITY";
	case CELL_SAIL_PARAMETER_SPURS_EXIT_IF_NO_WORK:                 return "SPURS_EXIT_IF_NO_WORK";
	case CELL_SAIL_PARAMETER_IO_PPU_THREAD_PRIORITY:                return "IO_PPU_THREAD_PRIORITY";
	case CELL_SAIL_PARAMETER_DMUX_PPU_THREAD_PRIORITY:              return "DMUX_PPU_THREAD_PRIORITY";
	case CELL_SAIL_PARAMETER_DMUX_SPU_THREAD_PRIORITY:              return "DMUX_SPU_THREAD_PRIORITY";
	case CELL_SAIL_PARAMETER_DMUX_NUM_OF_SPUS:                      return "DMUX_NUM_OF_SPUS";
	case CELL_SAIL_PARAMETER_DMUX_SPURS_TASK_PRIORITIES:            return "DMUX_SPURS_TASK_PRIORITIES";
	case CELL_SAIL_PARAMETER_ADEC_PPU_THREAD_PRIORITY:              return "ADEC_PPU_THREAD_PRIORITY";
	case CELL_SAIL_PARAMETER_ADEC_SPU_THREAD_PRIORITY:              return "ADEC_SPU_THREAD_PRIORITY";
	case CELL_SAIL_PARAMETER_ADEC_NUM_OF_SPUS:                      return "ADEC_NUM_OF_SPUS";
	case CELL_SAIL_PARAMETER_ADEC_SPURS_TASK_PRIORITIES:            return "ADEC_SPURS_TASK_PRIORITIES";
	case CELL_SAIL_PARAMETER_VDEC_PPU_THREAD_PRIORITY:              return "VDEC_PPU_THREAD_PRIORITY";
	case CELL_SAIL_PARAMETER_VDEC_SPU_THREAD_PRIORITY:              return "VDEC_SPU_THREAD_PRIORITY";
	case CELL_SAIL_PARAMETER_VDEC_M2V_NUM_OF_SPUS:                  return "VDEC_M2V_NUM_OF_SPUS";
	case CELL_SAIL_PARAMETER_VDEC_AVC_NUM_OF_SPUS:                  return "VDEC_AVC_NUM_OF_SPUS";
	case CELL_SAIL_PARAMETER_VDEC_SPURS_TASK_PRIORITIES:            return "VDEC_SPURS_TASK_PRIORITIES";
	case CELL_SAIL_PARAMETER_VPOST_PPU_THREAD_PRIORITY:             return "VPOST_PPU_THREAD_PRIORITY";
	case CELL_SAIL_PARAMETER_VPOST_SPU_THREAD_PRIORITY:             return "VPOST_SPU_THREAD_PRIORITY";
	case CELL_SAIL_PARAMETER_VPOST_NUM_OF_SPUS:                     return "VPOST_NUM_OF_SPUS";
	case CELL_SAIL_PARAMETER_VPOST_SPURS_TASK_PRIORITIES:           return "VPOST_SPURS_TASK_PRIORITIES";
	case CELL_SAIL_PARAMETER_GRAPHICS_ADAPTER_BUFFER_RELEASE_DELAY: return "GRAPHICS_ADAPTER_BUFFER_RELEASE_DELAY";
	case CELL_SAIL_PARAMETER_AV_SYNC_ES_AUDIO:                      return "AV_SYNC_ES_AUDIO";
	case CELL_SAIL_PARAMETER_AV_SYNC_ES_VIDEO:                      return "AV_SYNC_ES_VIDEO";
	case CELL_SAIL_PARAMETER_AV_SYNC_ES_USER:                       return "AV_SYNC_ES_USER";
	case CELL_SAIL_PARAMETER_CONTROL_PPU_THREAD_STACK_SIZE:         return "CONTROL_PPU_THREAD_STACK_SIZE";
	case CELL_SAIL_PARAMETER_RESERVED0_:                            return "RESERVED0_";
	case CELL_SAIL_PARAMETER_RESERVED1:                             return "RESERVED1";
	case CELL_SAIL_PARAMETER_ENABLE_APOST_SRC:                      return "ENABLE_APOST_SRC";
	case CELL_SAIL_PARAMETER_FS:                                    return "FS";
	case CELL_SAIL_PARAMETER_IO_PPU_THREAD_STACK_SIZE:              return "IO_PPU_THREAD_STACK_SIZE";
	case CELL_SAIL_PARAMETER_VIDEO_PERFORMANCE_POLICY:              return "VIDEO_PERFORMANCE_POLICY";
	case _CELL_SAIL_PARAMETER_TYPE_NUM_OF_ELEMENTS:                 return "TYPE_NUM_OF_ELEMENTS";
	default: return "???";
	}
}
