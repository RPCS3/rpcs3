#pragma once

#include "cellPamf.h" // CellCodecTimeStamp
#include "../lv2/sys_mutex.h"
#include "../lv2/sys_cond.h"

// Error Codes
enum CellAdecError : u32
{
	CELL_ADEC_ERROR_FATAL   = 0x80610001,
	CELL_ADEC_ERROR_SEQ     = 0x80610002,
	CELL_ADEC_ERROR_ARG     = 0x80610003,
	CELL_ADEC_ERROR_BUSY    = 0x80610004,
	CELL_ADEC_ERROR_EMPTY   = 0x80610005,

	CELL_ADEC_ERROR_CELP_BUSY         = 0x80612e01,
	CELL_ADEC_ERROR_CELP_EMPTY        = 0x80612e02,
	CELL_ADEC_ERROR_CELP_ARG          = 0x80612e03,
	CELL_ADEC_ERROR_CELP_SEQ          = 0x80612e04,
	CELL_ADEC_ERROR_CELP_CORE_FATAL   = 0x80612e81,
	CELL_ADEC_ERROR_CELP_CORE_ARG     = 0x80612e82,
	CELL_ADEC_ERROR_CELP_CORE_SEQ     = 0x80612e83,

	CELL_ADEC_ERROR_CELP8_BUSY         = 0x80612ea1,
	CELL_ADEC_ERROR_CELP8_EMPTY        = 0x80612ea2,
	CELL_ADEC_ERROR_CELP8_ARG          = 0x80612ea3,
	CELL_ADEC_ERROR_CELP8_SEQ          = 0x80612ea4,
	CELL_ADEC_ERROR_CELP8_CORE_FATAL   = 0x80612eb1,
	CELL_ADEC_ERROR_CELP8_CORE_ARG     = 0x80612eb2,
	CELL_ADEC_ERROR_CELP8_CORE_SEQ     = 0x80612eb3,

	CELL_ADEC_ERROR_M4AAC_FATAL                                   = 0x80612401,
	CELL_ADEC_ERROR_M4AAC_SEQ                                     = 0x80612402,
	CELL_ADEC_ERROR_M4AAC_ARG                                     = 0x80612403,
	CELL_ADEC_ERROR_M4AAC_BUSY                                    = 0x80612404,
	CELL_ADEC_ERROR_M4AAC_EMPTY                                   = 0x80612405,
	CELL_ADEC_ERROR_M4AAC_BUFFER_OVERFLOW                         = 0x80612406,
	CELL_ADEC_ERROR_M4AAC_END_OF_BITSTREAM                        = 0x80612407,

	/* Core */
	CELL_ADEC_ERROR_M4AAC_CH_CONFIG_INCONSISTENCY                 = 0x80612410,
	CELL_ADEC_ERROR_M4AAC_NO_CH_DEFAULT_POS                       = 0x80612411,
	CELL_ADEC_ERROR_M4AAC_INVALID_CH_POS                          = 0x80612412,
	CELL_ADEC_ERROR_M4AAC_UNANTICIPATED_COUPLING_CH               = 0x80612413,
	CELL_ADEC_ERROR_M4AAC_INVALID_LAYER_ID                        = 0x80612414,
	CELL_ADEC_ERROR_M4AAC_ADTS_SYNCWORD_ERROR                     = 0x80612415,
	CELL_ADEC_ERROR_M4AAC_INVALID_ADTS_ID                         = 0x80612416,
	CELL_ADEC_ERROR_M4AAC_CH_CHANGED                              = 0x80612417,
	CELL_ADEC_ERROR_M4AAC_SAMPLING_FREQ_CHANGED                   = 0x80612418,
	CELL_ADEC_ERROR_M4AAC_WRONG_SBR_CH                            = 0x80612419,
	CELL_ADEC_ERROR_M4AAC_WRONG_SCALE_FACTOR                      = 0x8061241a,
	CELL_ADEC_ERROR_M4AAC_INVALID_BOOKS                           = 0x8061241b,
	CELL_ADEC_ERROR_M4AAC_INVALID_SECTION_DATA                    = 0x8061241c,
	CELL_ADEC_ERROR_M4AAC_PULSE_IS_NOT_LONG                       = 0x8061241d,
	CELL_ADEC_ERROR_M4AAC_GC_IS_NOT_SUPPORTED                     = 0x8061241e,
	CELL_ADEC_ERROR_M4AAC_INVALID_ELEMENT_ID                      = 0x8061241f,
	CELL_ADEC_ERROR_M4AAC_NO_CH_CONFIG                            = 0x80612420,
	CELL_ADEC_ERROR_M4AAC_UNEXPECTED_OVERLAP_CRC                  = 0x80612421,
	CELL_ADEC_ERROR_M4AAC_CRC_BUFFER_EXCEEDED                     = 0x80612422,
	CELL_ADEC_ERROR_M4AAC_INVALID_CRC                             = 0x80612423,
	CELL_ADEC_ERROR_M4AAC_BAD_WINDOW_CODE                         = 0x80612424,
	CELL_ADEC_ERROR_M4AAC_INVALID_ADIF_HEADER_ID                  = 0x80612425,
	CELL_ADEC_ERROR_M4AAC_NOT_SUPPORTED_PROFILE                   = 0x80612426,
	CELL_ADEC_ERROR_M4AAC_PROG_NUMBER_NOT_FOUND                   = 0x80612427,
	CELL_ADEC_ERROR_M4AAC_INVALID_SAMP_RATE_INDEX                 = 0x80612428,
	CELL_ADEC_ERROR_M4AAC_UNANTICIPATED_CH_CONFIG                 = 0x80612429,
	CELL_ADEC_ERROR_M4AAC_PULSE_OVERFLOWED                        = 0x8061242a,
	CELL_ADEC_ERROR_M4AAC_CAN_NOT_UNPACK_INDEX                    = 0x8061242b,
	CELL_ADEC_ERROR_M4AAC_DEINTERLEAVE_FAILED                     = 0x8061242c,
	CELL_ADEC_ERROR_M4AAC_CALC_BAND_OFFSET_FAILED                 = 0x8061242d,
	CELL_ADEC_ERROR_M4AAC_GET_SCALE_FACTOR_FAILED                 = 0x8061242e,
	CELL_ADEC_ERROR_M4AAC_GET_CC_GAIN_FAILED                      = 0x8061242f,
	CELL_ADEC_ERROR_M4AAC_MIX_COUPLING_CH_FAILED                  = 0x80612430,
	CELL_ADEC_ERROR_M4AAC_GROUP_IS_INVALID                        = 0x80612431,
	CELL_ADEC_ERROR_M4AAC_PREDICT_FAILED                          = 0x80612432,
	CELL_ADEC_ERROR_M4AAC_INVALID_PREDICT_RESET_PATTERN           = 0x80612433,
	CELL_ADEC_ERROR_M4AAC_INVALID_TNS_FRAME_INFO                  = 0x80612434,
	CELL_ADEC_ERROR_M4AAC_GET_MASK_FAILED                         = 0x80612435,
	CELL_ADEC_ERROR_M4AAC_GET_GROUP_FAILED                        = 0x80612436,
	CELL_ADEC_ERROR_M4AAC_GET_LPFLAG_FAILED                       = 0x80612437,
	CELL_ADEC_ERROR_M4AAC_INVERSE_QUANTIZATION_FAILED             = 0x80612438,
	CELL_ADEC_ERROR_M4AAC_GET_CB_MAP_FAILED                       = 0x80612439,
	CELL_ADEC_ERROR_M4AAC_GET_PULSE_FAILED                        = 0x8061243a,
	CELL_ADEC_ERROR_M4AAC_MONO_MIXDOWN_ELEMENT_IS_NOT_SUPPORTED   = 0x8061243b,
	CELL_ADEC_ERROR_M4AAC_STEREO_MIXDOWN_ELEMENT_IS_NOT_SUPPORTED = 0x8061243c,

	CELL_ADEC_ERROR_M4AAC_SBR_CH_OVERFLOW                         = 0x80612480,
	CELL_ADEC_ERROR_M4AAC_SBR_NOSYNCH                             = 0x80612481,
	CELL_ADEC_ERROR_M4AAC_SBR_ILLEGAL_PROGRAM                     = 0x80612482,
	CELL_ADEC_ERROR_M4AAC_SBR_ILLEGAL_TAG                         = 0x80612483,
	CELL_ADEC_ERROR_M4AAC_SBR_ILLEGAL_CHN_CONFIG                  = 0x80612484,
	CELL_ADEC_ERROR_M4AAC_SBR_ILLEGAL_SECTION                     = 0x80612485,
	CELL_ADEC_ERROR_M4AAC_SBR_ILLEGAL_SCFACTORS                   = 0x80612486,
	CELL_ADEC_ERROR_M4AAC_SBR_ILLEGAL_PULSE_DATA                  = 0x80612487,
	CELL_ADEC_ERROR_M4AAC_SBR_MAIN_PROFILE_NOT_IMPLEMENTED        = 0x80612488,
	CELL_ADEC_ERROR_M4AAC_SBR_GC_NOT_IMPLEMENTED                  = 0x80612489,
	CELL_ADEC_ERROR_M4AAC_SBR_ILLEGAL_PLUS_ELE_ID                 = 0x8061248a,
	CELL_ADEC_ERROR_M4AAC_SBR_CREATE_ERROR                        = 0x8061248b,
	CELL_ADEC_ERROR_M4AAC_SBR_NOT_INITIALIZED                     = 0x8061248c,
	CELL_ADEC_ERROR_M4AAC_SBR_INVALID_ENVELOPE                    = 0x8061248d,


	CELL_ADEC_ERROR_AC3_BUSY = 0x80612500,
	CELL_ADEC_ERROR_AC3_EMPTY = 0x80612501,
	CELL_ADEC_ERROR_AC3_PARAM = 0x80612502,
	CELL_ADEC_ERROR_AC3_FRAME = 0x80612503,


	CELL_ADEC_ERROR_AT3_OFFSET = 0x80612100,
	CELL_ADEC_ERROR_AT3_OK     = 0x80612100,
	CELL_ADEC_ERROR_AT3_BUSY   = 0x80612164,
	CELL_ADEC_ERROR_AT3_EMPTY  = 0x80612165,
	CELL_ADEC_ERROR_AT3_ERROR  = 0x80612180,


	CELL_ADEC_ERROR_LPCM_FATAL = 0x80612001,
	CELL_ADEC_ERROR_LPCM_SEQ   = 0x80612002,
	CELL_ADEC_ERROR_LPCM_ARG   = 0x80612003,
	CELL_ADEC_ERROR_LPCM_BUSY  = 0x80612004,
	CELL_ADEC_ERROR_LPCM_EMPTY = 0x80612005,


	CELL_ADEC_ERROR_MP3_OFFSET = 0x80612700U,
	CELL_ADEC_ERROR_MP3_OK                  = 0x80612700,
	CELL_ADEC_ERROR_MP3_BUSY                = 0x80612764,
	CELL_ADEC_ERROR_MP3_EMPTY               = 0x80612765,
	CELL_ADEC_ERROR_MP3_ERROR               = 0x80612781,
	CELL_ADEC_ERROR_MP3_LOST_SYNC           = 0x80612782,
	CELL_ADEC_ERROR_MP3_NOT_L3              = 0x80612783,
	CELL_ADEC_ERROR_MP3_BAD_BITRATE         = 0x80612784,
	CELL_ADEC_ERROR_MP3_BAD_SFREQ           = 0x80612785,
	CELL_ADEC_ERROR_MP3_BAD_EMPHASIS        = 0x80612786,
	CELL_ADEC_ERROR_MP3_BAD_BLKTYPE         = 0x80612787,
	CELL_ADEC_ERROR_MP3_BAD_VERSION         = 0x8061278c,
	CELL_ADEC_ERROR_MP3_BAD_MODE            = 0x8061278d,
	CELL_ADEC_ERROR_MP3_BAD_MODE_EXT        = 0x8061278e,
	CELL_ADEC_ERROR_MP3_HUFFMAN_NUM         = 0x80612796,
	CELL_ADEC_ERROR_MP3_HUFFMAN_CASE_ID     = 0x80612797,
	CELL_ADEC_ERROR_MP3_SCALEFAC_COMPRESS   = 0x80612798,
	CELL_ADEC_ERROR_MP3_HGETBIT             = 0x80612799,
	CELL_ADEC_ERROR_MP3_FLOATING_EXCEPTION  = 0x8061279a,
	CELL_ADEC_ERROR_MP3_ARRAY_OVERFLOW      = 0x8061279b,
	CELL_ADEC_ERROR_MP3_STEREO_PROCESSING   = 0x8061279c,
	CELL_ADEC_ERROR_MP3_JS_BOUND            = 0x8061279d,
	CELL_ADEC_ERROR_MP3_PCMOUT              = 0x8061279e,


	CELL_ADEC_ERROR_M2BC_FATAL          = 0x80612b01,
	CELL_ADEC_ERROR_M2BC_SEQ            = 0x80612b02,
	CELL_ADEC_ERROR_M2BC_ARG            = 0x80612b03,
	CELL_ADEC_ERROR_M2BC_BUSY           = 0x80612b04,
	CELL_ADEC_ERROR_M2BC_EMPTY          = 0x80612b05,

	CELL_ADEC_ERROR_M2BC_SYNCF          = 0x80612b11,
	CELL_ADEC_ERROR_M2BC_LAYER          = 0x80612b12,
	CELL_ADEC_ERROR_M2BC_BITRATE        = 0x80612b13,
	CELL_ADEC_ERROR_M2BC_SAMPLEFREQ     = 0x80612b14,
	CELL_ADEC_ERROR_M2BC_VERSION        = 0x80612b15,
	CELL_ADEC_ERROR_M2BC_MODE_EXT       = 0x80612b16,
	CELL_ADEC_ERROR_M2BC_UNSUPPORT      = 0x80612b17,

	CELL_ADEC_ERROR_M2BC_OPENBS_EX      = 0x80612b21,
	CELL_ADEC_ERROR_M2BC_SYNCF_EX       = 0x80612b22,
	CELL_ADEC_ERROR_M2BC_CRCGET_EX      = 0x80612b23,
	CELL_ADEC_ERROR_M2BC_CRC_EX         = 0x80612b24,

	CELL_ADEC_ERROR_M2BC_CRCGET         = 0x80612b31,
	CELL_ADEC_ERROR_M2BC_CRC            = 0x80612b32,
	CELL_ADEC_ERROR_M2BC_BITALLOC       = 0x80612b33,
	CELL_ADEC_ERROR_M2BC_SCALE          = 0x80612b34,
	CELL_ADEC_ERROR_M2BC_SAMPLE         = 0x80612b35,
	CELL_ADEC_ERROR_M2BC_OPENBS         = 0x80612b36,

	CELL_ADEC_ERROR_M2BC_MC_CRCGET      = 0x80612b41,
	CELL_ADEC_ERROR_M2BC_MC_CRC         = 0x80612b42,
	CELL_ADEC_ERROR_M2BC_MC_BITALLOC    = 0x80612b43,
	CELL_ADEC_ERROR_M2BC_MC_SCALE       = 0x80612b44,
	CELL_ADEC_ERROR_M2BC_MC_SAMPLE      = 0x80612b45,
	CELL_ADEC_ERROR_M2BC_MC_HEADER      = 0x80612b46,
	CELL_ADEC_ERROR_M2BC_MC_STATUS      = 0x80612b47,

	CELL_ADEC_ERROR_M2BC_AG_CCRCGET     = 0x80612b51,
	CELL_ADEC_ERROR_M2BC_AG_CRC         = 0x80612b52,
	CELL_ADEC_ERROR_M2BC_AG_BITALLOC    = 0x80612b53,
	CELL_ADEC_ERROR_M2BC_AG_SCALE       = 0x80612b54,
	CELL_ADEC_ERROR_M2BC_AG_SAMPLE      = 0x80612b55,
	CELL_ADEC_ERROR_M2BC_AG_STATUS      = 0x80612b57,
};

// Audio Codec Type
enum AudioCodecType : s32
{
	CELL_ADEC_TYPE_INVALID1,
	CELL_ADEC_TYPE_LPCM_PAMF,
	CELL_ADEC_TYPE_AC3,
	CELL_ADEC_TYPE_ATRACX,
	CELL_ADEC_TYPE_MP3,
	CELL_ADEC_TYPE_ATRAC3,
	CELL_ADEC_TYPE_MPEG_L2,
	CELL_ADEC_TYPE_M2AAC,
	CELL_ADEC_TYPE_EAC3,
	CELL_ADEC_TYPE_TRUEHD,
	CELL_ADEC_TYPE_DTS, // Removed in firmware 4.00, integrated into DTSHD
	CELL_ADEC_TYPE_CELP,
	CELL_ADEC_TYPE_LPCM_BLURAY,
	CELL_ADEC_TYPE_ATRACX_2CH,
	CELL_ADEC_TYPE_ATRACX_6CH,
	CELL_ADEC_TYPE_ATRACX_8CH,
	CELL_ADEC_TYPE_M4AAC,
	CELL_ADEC_TYPE_LPCM_DVD,
	CELL_ADEC_TYPE_WMA,
	CELL_ADEC_TYPE_DTSLBR,
	CELL_ADEC_TYPE_M4AAC_2CH,
	CELL_ADEC_TYPE_DTSHD,
	CELL_ADEC_TYPE_MPEG_L1,
	CELL_ADEC_TYPE_MP3S,
	CELL_ADEC_TYPE_M4AAC_2CH_MOD,
	CELL_ADEC_TYPE_CELP8,
	CELL_ADEC_TYPE_INVALID2,
	CELL_ADEC_TYPE_INVALID3,
	CELL_ADEC_TYPE_RESERVED22, // Either WMA Pro or WMA Lossless, was never released
	CELL_ADEC_TYPE_RESERVED23, // Either WMA Pro or WMA Lossless, was never released
	CELL_ADEC_TYPE_DTSHDCORE, // Removed in firmware 4.00, integrated into DTSHD
	CELL_ADEC_TYPE_ATRAC3MULTI,
};

// Output Channel Number
enum CellAdecChannel : s32
{
	CELL_ADEC_CH_RESERVED1,
	CELL_ADEC_CH_MONO,
	CELL_ADEC_CH_RESERVED2,
	CELL_ADEC_CH_STEREO,
	CELL_ADEC_CH_3_0,
	CELL_ADEC_CH_2_1,
	CELL_ADEC_CH_3_1,
	CELL_ADEC_CH_2_2,
	CELL_ADEC_CH_3_2,
	CELL_ADEC_CH_3_2_LFE,
	CELL_ADEC_CH_3_4,
	CELL_ADEC_CH_3_4_LFE,
	CELL_ADEC_CH_RESERVED3,
};

// Sampling Rate
enum CellAdecSampleRate : s32
{
	CELL_ADEC_FS_RESERVED1,
	CELL_ADEC_FS_48kHz,
	CELL_ADEC_FS_16kHz,
	CELL_ADEC_FS_96kHz,
	CELL_ADEC_FS_192kHz,
	CELL_ADEC_FS_8kHz,
};

enum CellAdecBitLength : u32
{
	CELL_ADEC_BIT_LENGTH_RESERVED1,
	CELL_ADEC_BIT_LENGTH_16,
	CELL_ADEC_BIT_LENGTH_20,
	CELL_ADEC_BIT_LENGTH_24,
};

struct CellAdecType
{
	be_t<s32> audioCodecType; // AudioCodecType
};

struct CellAdecAttr
{
	be_t<u32> workMemSize;
	be_t<u32> adecVerUpper;
	be_t<u32> adecVerLower;
};

struct CellAdecResource
{
	be_t<u32> totalMemSize;
	be_t<u32> startAddr;
	be_t<u32> ppuThreadPriority;
	be_t<u32> spuThreadPriority;
	be_t<u32> ppuThreadStackSize;
};

struct CellAdecResourceEx
{
	be_t<u32> totalMemSize;
	be_t<u32> startAddr;
	be_t<u32> ppuThreadPriority;
	be_t<u32> ppuThreadStackSize;
	be_t<u32> spurs_addr;
	be_t<u64, 1> priority;
	be_t<u32> maxContention;
};

struct CellAdecResourceSpurs
{
	be_t<u32> spurs_addr; // CellSpurs*
	be_t<u64, 1> priority;
	be_t<u32> maxContention;
};

// Callback Messages
enum CellAdecMsgType : s32
{
	CELL_ADEC_MSG_TYPE_AUDONE,
	CELL_ADEC_MSG_TYPE_PCMOUT,
	CELL_ADEC_MSG_TYPE_ERROR,
	CELL_ADEC_MSG_TYPE_SEQDONE,
};

using CellAdecCbMsg = s32(vm::ptr<void> handle, CellAdecMsgType msgType, s32 msgData, vm::ptr<void> cbArg);

// Used for internal callbacks as well
template <typename F>
struct AdecCb
{
	vm::bptr<F> cbFunc;
	vm::bptr<void> cbArg;
};

using CellAdecCb = AdecCb<CellAdecCbMsg>;

// AU Info
struct CellAdecAuInfo
{
	vm::bcptr<u8> startAddr;
	be_t<u32> size;
	CellCodecTimeStamp pts;
	be_t<u64> userData;
};

// BSI Info
struct CellAdecPcmAttr
{
	vm::bptr<void> bsiInfo;
};

struct CellAdecPcmItem
{
	be_t<s32> pcmHandle;
	be_t<u32> status;
	vm::bcptr<void> startAddr;
	be_t<u32> size;
	CellAdecPcmAttr	pcmAttr;
	CellAdecAuInfo auInfo;
};

// Controls how much is added to the presentation time stamp of the previous frame if the game didn't set a pts itself in CellAdecAuInfo when calling cellAdecDecodeAu()
enum AdecCorrectPtsValueType : s8
{
	ADEC_CORRECT_PTS_VALUE_TYPE_UNSPECIFIED = -1,

	// Adds a fixed amount
	ADEC_CORRECT_PTS_VALUE_TYPE_LPCM_HDMV = 0,
	ADEC_CORRECT_PTS_VALUE_TYPE_LPCM_DVD = 1, // Unused for some reason, the DVD player probably takes care of timestamps itself
	ADEC_CORRECT_PTS_VALUE_TYPE_ATRACX_48000Hz = 2,
	ADEC_CORRECT_PTS_VALUE_TYPE_ATRACX_44100Hz = 3,
	ADEC_CORRECT_PTS_VALUE_TYPE_ATRACX_32000Hz = 4,
	ADEC_CORRECT_PTS_VALUE_TYPE_AC3 = 5,
	ADEC_CORRECT_PTS_VALUE_TYPE_ATRAC3 = 6,
	ADEC_CORRECT_PTS_VALUE_TYPE_MP3_48000Hz = 7,
	ADEC_CORRECT_PTS_VALUE_TYPE_MP3_44100Hz = 8,
	ADEC_CORRECT_PTS_VALUE_TYPE_MP3_32000Hz = 9,
	ADEC_CORRECT_PTS_VALUE_TYPE_ATRAC3MULTI = 39,

	// Calls a decoder function (_SceAdecCorrectPtsValue_codec())
	ADEC_CORRECT_PTS_VALUE_TYPE_EAC3 = 17,
	ADEC_CORRECT_PTS_VALUE_TYPE_DTSHD = 21,
	ADEC_CORRECT_PTS_VALUE_TYPE_CELP = 24,
	ADEC_CORRECT_PTS_VALUE_TYPE_M2AAC = 25,
	ADEC_CORRECT_PTS_VALUE_TYPE_MPEG_L2 = 26,
	ADEC_CORRECT_PTS_VALUE_TYPE_TRUEHD = 27,
	ADEC_CORRECT_PTS_VALUE_TYPE_DTS = 28,
	ADEC_CORRECT_PTS_VALUE_TYPE_M4AAC = 29,
	ADEC_CORRECT_PTS_VALUE_TYPE_WMA = 30,
	ADEC_CORRECT_PTS_VALUE_TYPE_DTSLBR = 31,
	ADEC_CORRECT_PTS_VALUE_TYPE_MPEG_L1 = 32,
	ADEC_CORRECT_PTS_VALUE_TYPE_MP3S = 33,
	ADEC_CORRECT_PTS_VALUE_TYPE_CELP8 = 34,
	ADEC_CORRECT_PTS_VALUE_TYPE_WMAPRO = 35,
	ADEC_CORRECT_PTS_VALUE_TYPE_WMALSL = 36,
	ADEC_CORRECT_PTS_VALUE_TYPE_DTSHDCORE_UNK1 = 37,
	ADEC_CORRECT_PTS_VALUE_TYPE_DTSHDCORE_UNK2 = 38,
};

// Internal callbacks
using AdecNotifyAuDone = error_code(s32 pcmHandle, vm::ptr<void> cbArg);
using AdecNotifyPcmOut = error_code(s32 pcmHandle, vm::ptr<void> pcmAddr, u32 pcmSize, vm::ptr<void> cbArg, vm::cpptr<void> bsiInfo, AdecCorrectPtsValueType correctPtsValueType, s32 errorCode);
using AdecNotifyError = error_code(s32 errorCode, vm::ptr<void> cbArg);
using AdecNotifySeqDone = error_code(vm::ptr<void> cbArg);

// Decoder functions
using CellAdecCoreOpGetMemSize = error_code(vm::ptr<CellAdecAttr> attr);
using CellAdecCoreOpOpen = error_code(vm::ptr<void> coreHandle, vm::ptr<AdecNotifyAuDone> cbFuncAuDone, vm::ptr<void> cbArgAuDone, vm::ptr<AdecNotifyPcmOut> cbFuncPcmOut, vm::ptr<void> cbArgPcmOut,
	vm::ptr<AdecNotifyError> cbFuncError, vm::ptr<void> cbArgError, vm::ptr<AdecNotifySeqDone> cbFuncSeqDone, vm::ptr<void> cbArgSeqDone, vm::cptr<CellAdecResource> res);
using CellAdecCoreOpClose = error_code(vm::ptr<void> coreHandle);
using CellAdecCoreOpStartSeq = error_code(vm::ptr<void> coreHandle, vm::cptr<void> param);
using CellAdecCoreOpEndSeq = error_code(vm::ptr<void> coreHandle);
using CellAdecCoreOpDecodeAu = error_code(vm::ptr<void> coreHandle, s32 pcmHandle, vm::cptr<CellAdecAuInfo> auInfo);
using CellAdecCoreOpGetVersion = void(vm::ptr<be_t<u32, 1>> version);
using CellAdecCoreOpRealign = error_code(vm::ptr<void> coreHandle, vm::ptr<void> outBuffer, vm::cptr<void> pcmStartAddr);
using CellAdecCoreOpReleasePcm = error_code(vm::ptr<void> coreHandle, s32 pcmHandle, vm::cptr<void> outBuffer);
using CellAdecCoreOpGetPcmHandleNum = s32();
using CellAdecCoreOpGetBsiInfoSize = u32();
using CellAdecCoreOpOpenExt = error_code(vm::ptr<void> coreHandle, vm::ptr<AdecNotifyAuDone> cbFuncAuDone, vm::ptr<void> cbArgAuDone, vm::ptr<AdecNotifyPcmOut> cbFuncPcmOut, vm::ptr<void> cbArgPcmOut,
	vm::ptr<AdecNotifyError> cbFuncError, vm::ptr<void> cbArgError, vm::ptr<AdecNotifySeqDone> cbFuncSeqDone, vm::ptr<void> cbArgSeqDone, vm::cptr<CellAdecResource> res, vm::cptr<CellAdecResourceSpurs> spursRes);

// Decoders export a pointer to this struct
struct CellAdecCoreOps
{
	vm::bptr<CellAdecCoreOpGetMemSize> getMemSize;
	vm::bptr<CellAdecCoreOpOpen> open;
	vm::bptr<CellAdecCoreOpClose> close;
	vm::bptr<CellAdecCoreOpStartSeq> startSeq;
	vm::bptr<CellAdecCoreOpEndSeq> endSeq;
	vm::bptr<CellAdecCoreOpDecodeAu> decodeAu;
	vm::bptr<CellAdecCoreOpGetVersion> getVersion;
	vm::bptr<CellAdecCoreOpRealign> realign;
	vm::bptr<CellAdecCoreOpReleasePcm> releasePcm;
	vm::bptr<CellAdecCoreOpGetPcmHandleNum> getPcmHandleNum;
	vm::bptr<CellAdecCoreOpGetBsiInfoSize> getBsiInfoSize;
	vm::bptr<CellAdecCoreOpOpenExt> openExt;
};

// Used by several decoders as command queue
template <typename T>
struct AdecCmdQueue
{
	T elements[4];

	be_t<s32> front = 0;
	be_t<s32> back = 0;
	be_t<s32> size = 0;

	template <bool is_peek = false>
	void pop(T& cmd)
	{
		// LLE returns uninitialized stack memory if the queue is empty
		cmd = elements[front];

		if constexpr (!is_peek)
		{
			elements[front].pcm_handle = 0xff;
			front = (front + 1) & 3;
			size--;
		}
	}

	void emplace(auto&&... args)
	{
		new (&elements[back]) T(std::forward<decltype(args)>(args)...);

		back = (back + 1) & 3;
		size++;
	}

	void peek(T& cmd) const { return pop<true>(cmd); }
	bool empty() const { return size == 0; }
	bool full() const { return size >= 4; }
};

struct AdecFrame
{
	b8 in_use; // True after issuing a decode command until the frame is consumed

	be_t<s32> this_index; // Set when initialized in cellAdecOpen(), unused afterward

	// Set when the corresponding callback is received, unused afterward
	b8 au_done;
	b8 unk1;
	b8 pcm_out;
	b8 unk2;

	CellAdecAuInfo au_info;
	CellAdecPcmItem pcm_item;

	u32 reserved1;
	u32 reserved2;

	// Frames that are ready to be consumed form a linked list. However, this list is not used (AdecOutputQueue is used instead)
	be_t<s32> next; // Index of the next frame that can be consumed
	be_t<s32> prev; // Index of the previous frame that can be consumed
};

CHECK_SIZE(AdecFrame, 0x68);

class AdecOutputQueue
{
	struct entry
	{
		be_t<s32> this_index; // Unused
		be_t<s32> state; // 0xff = empty, 0x10 = filled
		vm::bptr<CellAdecPcmItem> pcm_item;
		be_t<s32> pcm_handle;
	}
	entries[4];

	be_t<s32> front;
	be_t<s32> back;
	be_t<s32> size;

	be_t<u32> mutex; // sys_mutex_t
	be_t<u32> cond;  // sys_cond_t, unused

public:
	void init(ppu_thread& ppu, vm::ptr<AdecOutputQueue> _this)
	{
		this->front = 0;
		this->back = 0;
		this->size = 0;

		const vm::var<sys_mutex_attribute_t> mutex_attr = {{ SYS_SYNC_PRIORITY, SYS_SYNC_NOT_RECURSIVE, SYS_SYNC_NOT_PROCESS_SHARED, SYS_SYNC_NOT_ADAPTIVE, 0, 0, 0, { "_adem07"_u64 } }};
		ensure(sys_mutex_create(ppu, _this.ptr(&AdecOutputQueue::mutex), mutex_attr) == CELL_OK); // Error code isn't checked on LLE

		const vm::var<sys_cond_attribute_t> cond_attr = {{ SYS_SYNC_NOT_PROCESS_SHARED, 0, 0, { "_adec05"_u64 } }};
		ensure(sys_cond_create(ppu, _this.ptr(&AdecOutputQueue::cond), mutex, cond_attr) == CELL_OK); // Error code isn't checked on LLE

		for (s32 i = 0; i < 4; i++)
		{
			entries[i] = { i, 0xff, vm::null, -1 };
		}
	}

	error_code finalize(ppu_thread& ppu) const
	{
		if (error_code ret = sys_cond_destroy(ppu, cond); ret != CELL_OK)
		{
			return ret;
		}

		if (error_code ret = sys_mutex_destroy(ppu, mutex); ret != CELL_OK)
		{
			return ret;
		}

		return CELL_OK;
	}

	error_code push(ppu_thread& ppu, vm::ptr<CellAdecPcmItem> pcm_item, s32 pcm_handle)
	{
		ensure(sys_mutex_lock(ppu, mutex, 0) == CELL_OK); // Error code isn't checked on LLE

		if (entries[back].state != 0xff)
		{
			ensure(sys_mutex_unlock(ppu, mutex) == CELL_OK); // Error code isn't checked on LLE
			return true; // LLE returns the result of the comparison above
		}

		entries[back].state = 0x10;
		entries[back].pcm_item = pcm_item;
		entries[back].pcm_handle = pcm_handle;

		back = (back + 1) & 3;
		size++;

		ensure(sys_mutex_unlock(ppu, mutex) == CELL_OK); // Error code isn't checked on LLE
		return CELL_OK;
	}

	const entry* pop(ppu_thread& ppu)
	{
		ensure(sys_mutex_lock(ppu, mutex, 0) == CELL_OK); // Error code isn't checked on LLE

		if (ppu.state & cpu_flag::again) // Savestate was created while waiting on the mutex
		{
			return {};
		}

		if (entries[front].state == 0xff)
		{
			ensure(sys_mutex_unlock(ppu, mutex) == CELL_OK); // Error code isn't checked on LLE
			return nullptr;
		}

		const entry* const ret = &entries[front];

		entries[front].state = 0xff;
		entries[front].pcm_handle = -1;

		front = (front + 1) & 3;
		size--;

		ensure(sys_mutex_unlock(ppu, mutex) == CELL_OK); // Error code isn't checked on LLE
		return ret;
	}

	const entry& peek(ppu_thread& ppu) const
	{
		ensure(sys_mutex_lock(ppu, mutex, 0) == CELL_OK); // Error code isn't checked on LLE
		const entry& ret = entries[front];
		ensure(sys_mutex_unlock(ppu, mutex) == CELL_OK); // Error code isn't checked on LLE
		return ret;
	}
};

CHECK_SIZE(AdecOutputQueue, 0x54);

enum class AdecSequenceState : u32
{
	dormant = 0x100,
	ready = 0x200,
	closed = 0xa00,
};

struct AdecContext // CellAdecHandle = AdecContext*
{
	vm::bptr<AdecContext> _this;
	be_t<u32> this_size; // Size of this struct + AdecFrames + bitstream info structs

	u32 unk; // Unused

	be_t<AdecSequenceState> sequence_state;

	CellAdecType type;
	CellAdecResource res;
	CellAdecCb callback;

	vm::bptr<void> core_handle;
	vm::bcptr<CellAdecCoreOps> core_ops;

	CellCodecTimeStamp previous_pts;

	be_t<s32> frames_num;
	u32 reserved1;
	be_t<s32> frames_head;      // Index of the oldest frame that can be consumed
	be_t<s32> frames_tail;      // Index of the most recent frame that can be consumed
	vm::bptr<AdecFrame> frames; // Array of AdecFrames, number of elements is return value of CellAdecCoreOps::getPcmHandleNum

	be_t<u32> bitstream_info_size;

	sys_mutex_attribute_t mutex_attribute;
	be_t<u32> mutex; // sys_mutex_t

	AdecOutputQueue pcm_queue;      // Output queue for cellAdecGetPcm()
	AdecOutputQueue pcm_item_queue; // Output queue for cellAdecGetPcmItem()

	u8 reserved2[1028];

	[[nodiscard]] error_code get_new_pcm_handle(vm::ptr<CellAdecAuInfo> au_info) const;
	error_code verify_pcm_handle(s32 pcm_handle) const;
	vm::ptr<CellAdecAuInfo> get_au_info(s32 pcm_handle) const;
	void set_state(s32 pcm_handle, u32 state) const;
	error_code get_pcm_item(s32 pcm_handle, vm::ptr<CellAdecPcmItem>& pcm_item) const;
	error_code set_pcm_item(s32 pcm_handle, vm::ptr<void> pcm_addr, u32 pcm_size, vm::cpptr<void> bitstream_info) const;
	error_code link_frame(ppu_thread& ppu, s32 pcm_handle);
	error_code unlink_frame(ppu_thread& ppu, s32 pcm_handle);
	void reset_frame(s32 pcm_handle) const;
	error_code correct_pts_value(ppu_thread& ppu, s32 pcm_handle, s8 correct_pts_type);
};

static_assert(std::is_standard_layout_v<AdecContext> && std::is_trivial_v<AdecContext>);
CHECK_SIZE_ALIGN(AdecContext, 0x530, 8);


enum : u32
{
	CELL_ADEC_LPCM_DVD_CH_RESERVED1,
	CELL_ADEC_LPCM_DVD_CH_MONO,
	CELL_ADEC_LPCM_DVD_CH_RESERVED2,
	CELL_ADEC_LPCM_DVD_CH_STEREO,
	CELL_ADEC_LPCM_DVD_CH_UNK1, // Either 3 front or 2 front + 1 surround
	CELL_ADEC_LPCM_DVD_CH_UNK2, // Either 3 front + 1 surround or 2 front + 2 surround
	CELL_ADEC_LPCM_DVD_CH_3_2,
	CELL_ADEC_LPCM_DVD_CH_3_2_LFE,
	CELL_ADEC_LPCM_DVD_CH_3_4,
	CELL_ADEC_LPCM_DVD_CH_3_4_LFE,
};

struct CellAdecParamLpcm
{
	be_t<u32> channelNumber;
	be_t<u32> sampleRate;
	be_t<u32> sizeOfWord;
	be_t<u32> audioPayloadSize;
};

// LPCM BSI
struct CellAdecLpcmInfo
{
	be_t<u32> channelNumber;
	be_t<u32> sampleRate;
	be_t<u32> outputDataSize;
};

// HLE exclusive, for savestates
enum class lpcm_dec_state : u8
{
	waiting_for_cmd_mutex_lock,
	waiting_for_cmd_cond_wait,
	waiting_for_output_mutex_lock,
	waiting_for_output_cond_wait,
	queue_mutex_lock,
	executing_cmd
};

class LpcmDecSemaphore
{
	be_t<u32> value;
	be_t<u32> mutex; // sys_mutex_t
	be_t<u32> cond;  // sys_cond_t

public:
	error_code init(ppu_thread& ppu, vm::ptr<LpcmDecSemaphore> _this, u32 initial_value)
	{
		value = initial_value;

		const vm::var<sys_mutex_attribute_t> mutex_attr{{ SYS_SYNC_PRIORITY, SYS_SYNC_NOT_RECURSIVE, SYS_SYNC_NOT_PROCESS_SHARED, SYS_SYNC_NOT_ADAPTIVE, 0, 0, 0, { "_adem01"_u64 } }};
		const vm::var<sys_cond_attribute_t> cond_attr{{ SYS_SYNC_NOT_PROCESS_SHARED, 0, 0, { "_adec01"_u64 } }};

		if (error_code ret = sys_mutex_create(ppu, _this.ptr(&LpcmDecSemaphore::mutex), mutex_attr); ret != CELL_OK)
		{
			return ret;
		}

		return sys_cond_create(ppu, _this.ptr(&LpcmDecSemaphore::cond), mutex, cond_attr);
	}

	error_code finalize(ppu_thread& ppu) const
	{
		if (error_code ret = sys_cond_destroy(ppu, cond); ret != CELL_OK)
		{
			return ret;
		}

		return sys_mutex_destroy(ppu, mutex);
	}

	error_code release(ppu_thread& ppu)
	{
		if (error_code ret = sys_mutex_lock(ppu, mutex, 0); ret != CELL_OK)
		{
			return ret;
		}

		value++;

		if (error_code ret = sys_cond_signal(ppu, cond); ret != CELL_OK)
		{
			return ret; // LLE doesn't unlock the mutex
		}

		return sys_mutex_unlock(ppu, mutex);
	}

	error_code acquire(ppu_thread& ppu, lpcm_dec_state& savestate)
	{
		if (savestate == lpcm_dec_state::waiting_for_cmd_cond_wait)
		{
			goto cond_wait;
		}

		savestate = lpcm_dec_state::waiting_for_cmd_mutex_lock;

		if (error_code ret = sys_mutex_lock(ppu, mutex, 0); ret != CELL_OK)
		{
			return ret;
		}

		if (ppu.state & cpu_flag::again)
		{
			return {};
		}

		if (value == 0u)
		{
			savestate = lpcm_dec_state::waiting_for_cmd_cond_wait;
			cond_wait:

			if (error_code ret = sys_cond_wait(ppu, cond, 0); ret != CELL_OK)
			{
				return ret; // LLE doesn't unlock the mutex
			}

			if (ppu.state & cpu_flag::again)
			{
				return {};
			}
		}

		value--;

		return sys_mutex_unlock(ppu, mutex);
	}
};

CHECK_SIZE(LpcmDecSemaphore, 0xc);

enum class LpcmDecCmdType : u32
{
	start_seq,
	end_seq,
	decode_au,
	close
};

struct LpcmDecCmd
{
	be_t<s32> pcm_handle;
	vm::bcptr<void> au_start_addr;
	be_t<u32> au_size;
	u32 reserved1[2];
	CellAdecParamLpcm lpcm_param;
	be_t<LpcmDecCmdType> type;
	u32 reserved2;

	LpcmDecCmd() = default; // cellAdecOpen()

	LpcmDecCmd(LpcmDecCmdType&& type) // End sequence
		: type(type)
	{
	}

	LpcmDecCmd(LpcmDecCmdType&& type, const CellAdecParamLpcm& lpcm_param) // Start sequence
		: lpcm_param(lpcm_param), type(type)
	{
	}

	LpcmDecCmd(LpcmDecCmdType&& type, const s32& pcm_handle, const CellAdecAuInfo& au_info) // Decode au
		: pcm_handle(pcm_handle), au_start_addr(au_info.startAddr), au_size(au_info.size), type(type)
	{
	}
};

CHECK_SIZE(LpcmDecCmd, 0x2c);

struct LpcmDecContext
{
	AdecCmdQueue<LpcmDecCmd> cmd_queue;

	be_t<u64> thread_id; // sys_ppu_thread_t

	be_t<u32> queue_size_mutex; // sys_mutex_t
	be_t<u32> queue_size_cond;  // sys_cond_t, unused
	be_t<u32> unk_mutex;        // sys_mutex_t, unused
	be_t<u32> unk_cond;         // sys_cond_t, unused

	be_t<u32> run_thread;

	AdecCb<AdecNotifyAuDone> notify_au_done;
	AdecCb<AdecNotifyPcmOut> notify_pcm_out;
	AdecCb<AdecNotifyError> notify_error;
	AdecCb<AdecNotifySeqDone> notify_seq_done;

	be_t<u32> output_locked;
	vm::bptr<f32> output;

	vm::bptr<CellAdecParamLpcm> lpcm_param;

	vm::bcptr<void> spurs_cmd_data;

	// HLE exclusive
	lpcm_dec_state savestate;
	u64 cmd_counter; // For debugging

	u8 reserved1[24]; // 36 bytes on LLE

	be_t<u32> output_mutex;    // sys_mutex_t
	be_t<u32> output_consumed; // sys_cond_t

	LpcmDecSemaphore cmd_available;
	LpcmDecSemaphore reserved2; // Unused

	be_t<u32> queue_mutex; // sys_mutex_t

	be_t<u32> error_occurred;

	u8 spurs_stuff[32];

	be_t<u32> spurs_queue_pop_mutex;
	be_t<u32> spurs_queue_push_mutex;

	be_t<u32> using_existing_spurs_instance;

	be_t<u32> dvd_packing;

	be_t<u32> output_size;

	LpcmDecCmd cmd; // HLE exclusive, name of Spurs taskset (32 bytes) + CellSpursTaskLsPattern on LLE

	u8 more_spurs_stuff[10]; // 52 bytes on LLE

	void exec(ppu_thread& ppu);

	template <LpcmDecCmdType type>
	error_code send_command(ppu_thread& ppu, auto&&... args);

	inline error_code release_output(ppu_thread& ppu);
};

static_assert(std::is_standard_layout_v<LpcmDecContext>);
CHECK_SIZE_ALIGN(LpcmDecContext, 0x1c8, 8);

constexpr s32 LPCM_DEC_OUTPUT_BUFFER_SIZE = 0x40000;

// CELP Excitation Mode
enum CELP_ExcitationMode : s32
{
	CELL_ADEC_CELP_EXCITATION_MODE_RPE = 1,
};

// CELP RPE Configuration
enum CELP_RPEConfig : s32
{
	CELL_ADEC_CELP_RPE_CONFIG_0,
	CELL_ADEC_CELP_RPE_CONFIG_1,
	CELL_ADEC_CELP_RPE_CONFIG_2,
	CELL_ADEC_CELP_RPE_CONFIG_3,
};

// CELP Word Size
enum CELP_WordSize : s32
{
	CELL_ADEC_CELP_WORD_SZ_INT16_LE,
	CELL_ADEC_CELP_WORD_SZ_FLOAT,
};

// CELP Parameters
struct CellAdecParamCelp
{
	be_t<u32> excitationMode;
	be_t<u32> sampleRate; // CELL_ADEC_FS_16kHz
	be_t<u32> configuration;
	be_t<u32> wordSize;
};

// CELP BSI (same as CellAdecParamCelp ???)
struct CellAdecCelpInfo
{
	be_t<u32> excitationMode;
	be_t<u32> sampleRate; // CELL_ADEC_FS_16kHz
	be_t<u32> configuration;
	be_t<u32> wordSize;
};

// CELP8 Excitation Mode
enum CELP8_ExcitationMode : s32
{
	CELL_ADEC_CELP8_EXCITATION_MODE_MPE = 0,
};

// CELP8 MPE Configuration
enum CELP8_MPEConfig : s32
{
	CELL_ADEC_CELP8_MPE_CONFIG_0  = 0,
	CELL_ADEC_CELP8_MPE_CONFIG_2  = 2,
	CELL_ADEC_CELP8_MPE_CONFIG_6  = 6,
	CELL_ADEC_CELP8_MPE_CONFIG_9  = 9,
	CELL_ADEC_CELP8_MPE_CONFIG_12 = 12,
	CELL_ADEC_CELP8_MPE_CONFIG_15 = 15,
	CELL_ADEC_CELP8_MPE_CONFIG_18 = 18,
	CELL_ADEC_CELP8_MPE_CONFIG_21 = 21,
	CELL_ADEC_CELP8_MPE_CONFIG_24 = 24,
	CELL_ADEC_CELP8_MPE_CONFIG_26 = 26,
};

// CELP8 Word Size
enum CELP8_WordSize : s32
{
	CELL_ADEC_CELP8_WORD_SZ_FLOAT,
};

// CELP8 Parameters
struct CellAdecParamCelp8
{
	be_t<u32> excitationMode;
	be_t<u32> sampleRate; // CELL_ADEC_FS_8kHz
	be_t<u32> configuration;
	be_t<u32> wordSize;
};

// CELP8 BSI
struct CellAdecCelp8Info
{
	be_t<u32> excitationMode;
	be_t<u32> sampleRate; // CELL_ADEC_FS_8kHz
	be_t<u32> configuration;
	be_t<u32> wordSize;
};

enum MPEG4AAC_ConfigType : s32
{
	ADIFHeader = 0,
	ADTSHeader = 1,
	RawDataBlockOnly = 2,
};

enum MPEG4AAC_SamplingFreq : s32
{
	SF_96000 = 0,
	SF_88200 = 1,
	SF_64000 = 2,
	SF_48000 = 3,
	SF_44100 = 4,
	SF_32000 = 5,
	SF_24000 = 6,
	SF_22050 = 7,
	SF_16000 = 8,
	SF_12000 = 9,
	SF_11025 = 10,
	SF_8000 = 11,
};

// MPEG4 AAC Parameters
struct CellAdecParamM4Aac
{
	be_t<s32> configNumber; // MPEG4AAC_ConfigType

	union
	{
		struct
		{
			be_t<u32> programNumber;
		}
		adifConfig;

		struct
		{
			be_t<s32> samplingFreqIndex; // MPEG4AAC_SamplingFreq
			be_t<u32> profile; // LC profile (1)
		}
		rawDataBlockConfig;
	}
	configInfo;

	be_t<u32> enableDownmix; // enable downmix to 2.0 (if (enableDownmix))
};

// MPEG4 AAC BSI
struct alignas(16) CellAdecM4AacInfo
{
	be_t<u32> samplingFreq; // [Hz]
	be_t<u32> numberOfChannels;
	be_t<u32> numberOfFrontChannels;
	be_t<u32> numberOfFrontMonoChannels;
	be_t<u32> numberOfSideChannels;
	be_t<u32> numberOfBackChannels;
	be_t<u32> numberOfLfeChannels;
	be_t<u32> enableSBR;
	be_t<u32> SBRUpsamplingFactor;
	be_t<u32> isBsiValid;
	be_t<s32> configNumber; // MPEG4AAC_ConfigType

	union
	{
		struct
		{
			be_t<u32> copyrightIdPresent;
			char copyrightId[9];
			be_t<u32> originalCopy;
			be_t<u32> home;
			be_t<u32> bitstreamType;
			be_t<u32> bitrate;
			be_t<u32> numberOfProgramConfigElements;
			be_t<u32> bufferFullness;
		}
		adif;

		struct mp8
		{
			be_t<u32> id;
			be_t<u32> layer;
			be_t<u32> protectionAbsent;
			be_t<u32> profile;
			be_t<u32> samplingFreqIndex;
			be_t<u32> privateBit;
			be_t<u32> channelConfiguration;
			be_t<u32> originalCopy;
			be_t<u32> home;
			be_t<u32> copyrightIdBit;
			be_t<u32> copyrightIdStart;
			be_t<u32> frameLength;
			be_t<u32> bufferFullness;
			be_t<u32> numberOfRawDataBlocks;
			be_t<u32> crcCheck;
		}
		adts;
	}
	bsi;

	struct
	{
		be_t<u32> matrixMixdownPresent;
		be_t<u32> mixdownIndex;
		be_t<u32> pseudoSurroundEnable;
	}
	matrixMixdown;

	be_t<u32> reserved;
};

enum AC3_WordSize : u8
{
	CELL_ADEC_AC3_WORD_SZ_INT16 = 0,
	CELL_ADEC_AC3_WORD_SZ_FLOAT = 1,
};

enum AC3_OutputMode : u8
{
	CELL_ADEC_AC3_OUTPUT_MODE_RESERVED = 0,
	CELL_ADEC_AC3_OUTPUT_MODE_1_0 = 1,
	CELL_ADEC_AC3_OUTPUT_MODE_2_0 = 2,
	CELL_ADEC_AC3_OUTPUT_MODE_3_0 = 3,
	CELL_ADEC_AC3_OUTPUT_MODE_2_1 = 4,
	CELL_ADEC_AC3_OUTPUT_MODE_3_1 = 5,
	CELL_ADEC_AC3_OUTPUT_MODE_2_2 = 6,
	CELL_ADEC_AC3_OUTPUT_MODE_3_2 = 7,
};

enum AC3_LFE : u8
{
	CELL_ADEC_AC3_LFE_NOT_PRESENT = 0,
	CELL_ADEC_AC3_LFE_PRESENT = 1,
};

enum AC3_CompressionMode : u8
{
	CELL_ADEC_AC3_COMPRESSION_MODE_CUSTOM_ANALOG = 0,
	CELL_ADEC_AC3_COMPRESSION_MODE_CUSTOM_DIGITAL = 1,
	CELL_ADEC_AC3_COMPRESSION_MODE_LINE_OUT = 2,
	CELL_ADEC_AC3_COMPRESSION_MODE_RF_REMOD = 3,
};

enum AC3_StereoMode : u8
{
	CELL_ADEC_AC3_STEREO_MODE_AUTO_DETECT = 0,
	CELL_ADEC_AC3_STEREO_MODE_DOLBY_SURROUND_COMPATIBLE = 1,
	CELL_ADEC_AC3_STEREO_MODE_STEREO = 2,
};

enum AC3_DualmonoMode : u8
{
	CELL_ADEC_AC3_DUALMONO_MODE_STEREO = 0,
	CELL_ADEC_AC3_DUALMONO_MODE_LEFT_MONO = 1,
	CELL_ADEC_AC3_DUALMONO_MODE_RIGHT_MONO = 2,
	CELL_ADEC_AC3_DUALMONO_MODE_MIXED_MONO = 3,
};

enum AC3_KaraokeCapableMode : u8
{
	CELL_ADEC_AC3_KARAOKE_CAPABLE_MODE_NO_VOCAL = 0,
	CELL_ADEC_AC3_KARAOKE_CAPABLE_MODE_LEFT_VOCAL = 1,
	CELL_ADEC_AC3_KARAOKE_CAPABLE_MODE_RIGHT_VOCAL = 2,
	CELL_ADEC_AC3_KARAOKE_CAPABLE_MODE_BOTH_VOCAL = 3,
};

enum AC3_InputChannel : u8
{
	CELL_ADEC_AC3_INPUT_CHANNEL_L = 0,
	CELL_ADEC_AC3_INPUT_CHANNEL_C = 1,
	CELL_ADEC_AC3_INPUT_CHANNEL_R = 2,
	CELL_ADEC_AC3_INPUT_CHANNEL_l = 3,
	CELL_ADEC_AC3_INPUT_CHANNEL_r = 4,
	CELL_ADEC_AC3_INPUT_CHANNEL_s = 5,
};

struct CellAdecParamAc3
{
	AC3_WordSize wordSize;
	AC3_OutputMode outputMode;
	AC3_LFE outLfeOn;

	be_t<float> drcCutScaleFactor;
	be_t<float> drcBoostScaleFactor;

	AC3_CompressionMode compressionMode;
	AC3_InputChannel numberOfChannels;
	AC3_StereoMode stereoMode;
	AC3_DualmonoMode dualmonoMode;
	AC3_KaraokeCapableMode karaokeCapableMode;

	be_t<float> pcmScaleFactor;

	be_t<s32> channelPointer0;
	be_t<s32> channelPointer1;
	be_t<s32> channelPointer2;
	be_t<s32> channelPointer3;
	be_t<s32> channelPointer4;
	be_t<s32> channelPointer5;
};

struct CellAdecBsiAc3
{
	be_t<u32> codecType;
	be_t<u32> versionInfo;
	be_t<u32> totalCallCarryRun;
	be_t<u32> totalCallNum;
	be_t<u32> bitrateValue;
	be_t<u32> pcmSize;
	be_t<u32> esSizeBit;
	be_t<s32> errorCode;
	u8 libInfoFlag;
	u8 validity;
	u8 channelValue;
	u8 fsIndex;
	u8 outputQuantization;
	u8 outputChannel;
	u8 monoFlag;
	be_t<u32> bsi[2];
	be_t<u16> frmSizeCod;
	u8 acmod;
	u8 lfeOn;
	u8 karaokeMode;
	u8 cmixlev;
	u8 surmixlev;
	u8 dsurmod;
	u8 copyright;
	u8 original;
	u8 bsmod;
	u8 bsid;
	u8 xbsi1e;
	u8 dmixmod;
	u8 xbsi2e;
	u8 dsurexmod;
	u8 dheadphonmod;
	u8 adconvtyp;
	u8 crcErrorFlag;
	u8 execDmixType;
};

enum ATRAC3_WordSize : s32
{
	CELL_ADEC_ATRAC3_WORD_SZ_16BIT = 0x02,
	CELL_ADEC_ATRAC3_WORD_SZ_24BIT = 0x03,
	CELL_ADEC_ATRAC3_WORD_SZ_32BIT = 0x04,
	CELL_ADEC_ATRAC3_WORD_SZ_FLOAT = 0x84,
};

enum ATRAC3_JointType : s32
{
	ATRAC3_DUAL_STEREO = 0,
	ATRAC3_JOINT_STEREO = 1,
};

struct CellAdecParamAtrac3
{
	be_t<s32> nch; // channel count
	be_t<s32> isJoint; // ATRAC3_JointType
	be_t<s32> nbytes; // byte count of single AU (???)
	be_t<s32> bw_pcm; // ATRAC3_WordSize, bit length of output PCM sample
};

struct CellAdecAtrac3Info
{
	be_t<s32> nch;
	be_t<s32> isJoint; // ATRAC3_JointType
	be_t<s32> nbytes;
};

enum MP3_WordSize : s32
{
	CELL_ADEC_MP3_WORD_SZ_16BIT = 3,
	CELL_ADEC_MP3_WORD_SZ_FLOAT = 4,
};

enum MP3_ChannelMode : u8
{
	MP3_STEREO = 0,
	MP3_JOINT_STEREO = 1,
	MP3_DUAL = 2,
	MP3_MONO = 3,
};

enum MP3_CRCMode : u8
{
	MP3_CRC = 0,
	MP3_NO_CRC = 1,
};

struct CellAdecParamMP3
{
	be_t<s32> bw_pcm; // MP3_WordSize
};

struct CellAdecMP3Info
{
	be_t<u32> ui_header;
	be_t<u32> ui_main_data_begin;
	be_t<u32> ui_main_data_remain_size;
	be_t<u32> ui_main_data_now_size;
	MP3_CRCMode uc_crc;
	MP3_ChannelMode uc_mode;
	u8 uc_mode_extension;
	u8 uc_copyright;
	u8 uc_original;
	u8 uc_emphasis;
	u8 uc_crc_error_flag;
	be_t<s32> i_error_code;
};

enum M2BC_SampleFrequency : s32
{
	CELL_ADEC_BSI_M2BC_SAMPLE_FREQUENCY_44 = 0,
	CELL_ADEC_BSI_M2BC_SAMPLE_FREQUENCY_48 = 1,
	CELL_ADEC_BSI_M2BC_SAMPLE_FREQUENCY_32 = 2,
};

enum M2BC_ErrorProtection : s32
{
	CELL_ADEC_BSI_M2BC_ERROR_PROTECTION_NONE = 0,
	CELL_ADEC_BSI_M2BC_ERROR_PROTECTION_EXIST = 1,
};

enum M2BC_BitrateIndex : s32
{
	CELL_ADEC_BSI_M2BC_BITRATE_32 = 1,
	CELL_ADEC_BSI_M2BC_BITRATE_48 = 2,
	CELL_ADEC_BSI_M2BC_BITRATE_56 = 3,
	CELL_ADEC_BSI_M2BC_BITRATE_64 = 4,
	CELL_ADEC_BSI_M2BC_BITRATE_80 = 5,
	CELL_ADEC_BSI_M2BC_BITRATE_96 = 6,
	CELL_ADEC_BSI_M2BC_BITRATE_112 = 7,
	CELL_ADEC_BSI_M2BC_BITRATE_128 = 8,
	CELL_ADEC_BSI_M2BC_BITRATE_160 = 9,
	CELL_ADEC_BSI_M2BC_BITRATE_192 = 10,
	CELL_ADEC_BSI_M2BC_BITRATE_224 = 11,
	CELL_ADEC_BSI_M2BC_BITRATE_256 = 12,
	CELL_ADEC_BSI_M2BC_BITRATE_320 = 13,
	CELL_ADEC_BSI_M2BC_BITRATE_384 = 14,
};

enum M2BC_StereoMode : s32
{
	CELL_ADEC_BSI_M2BC_STEREO_MODE_STERO = 0,
	CELL_ADEC_BSI_M2BC_STEREO_MODE_JOINTSTERO = 1,
	CELL_ADEC_BSI_M2BC_STEREO_MODE_DUAL = 2,
	CELL_ADEC_BSI_M2BC_STEREO_MODE_MONO = 3,
};

enum M2BC_StereoModeEx : s32
{
	CELL_ADEC_BSI_M2BC_STEREO_EXMODE_0 = 0,
	CELL_ADEC_BSI_M2BC_STEREO_EXMODE_1 = 1,
	CELL_ADEC_BSI_M2BC_STEREO_EXMODE_2 = 2,
	CELL_ADEC_BSI_M2BC_STEREO_EXMODE_3 = 3,
};

enum M2BC_Emphasis : s32
{
	CELL_ADEC_BSI_M2BC_EMPHASIS_NONE = 0,
	CELL_ADEC_BSI_M2BC_EMPHASIS_50_15 = 1,
	CELL_ADEC_BSI_M2BC_EMPHASIS_CCITT = 3,
};

enum M2BC_CopyrightBit : s32
{
	CELL_ADEC_BSI_M2BC_COPYRIGHT_NONE = 0,
	CELL_ADEC_BSI_M2BC_COPYRIGHT_ON = 1,
};

enum M2BC_OriginalBit : s32
{
	CELL_ADEC_BSI_M2BC_ORIGINAL_COPY = 0,
	CELL_ADEC_BSI_M2BC_ORIGINAL_ORIGINAL = 1,
};

enum M2BC_SurroundMode : s32
{
	CELL_ADEC_BSI_M2BC_SURROUND_NONE = 0,
	CELL_ADEC_BSI_M2BC_SURROUND_MONO = 1,
	CELL_ADEC_BSI_M2BC_SURROUND_STEREO = 2,
	CELL_ADEC_BSI_M2BC_SURROUND_SECOND = 3,
};

enum M2BC_CenterMode : s32
{
	CELL_ADEC_BSI_M2BC_CENTER_NONE = 0,
	CELL_ADEC_BSI_M2BC_CENTER_EXIST = 1,
	CELL_ADEC_BSI_M2BC_CENTER_PHANTOM = 3,
};

enum M2BC_LFE : s32
{
	CELL_ADEC_BSI_M2BC_LFE_NONE = 0,
	CELL_ADEC_BSI_M2BC_LFE_EXIST = 1,
};

enum M2BC_AudioMixMode : s32
{
	CELL_ADEC_BSI_M2BC_AUDIOMIX_LARGE = 0,
	CELL_ADEC_BSI_M2BC_AUDIOMIX_SMALLE = 1,
};

enum M2BC_MCExtension : s32
{
	CELL_ADEC_BSI_M2BC_MCEXTENSION_2CH = 0,
	CELL_ADEC_BSI_M2BC_MCEXTENSION_5CH = 1,
	CELL_ADEC_BSI_M2BC_MCEXTENSION_7CH = 2,
};

enum M2BC_ChannelConfig : s32
{
	CELL_ADEC_BSI_M2BC_CH_CONFIG_MONO = 0,
	CELL_ADEC_BSI_M2BC_CH_CONFIG_DUAL = 1,
	CELL_ADEC_BSI_M2BC_CH_CONFIG_L_R = 2,
	CELL_ADEC_BSI_M2BC_CH_CONFIG_L_R_S = 3,
	CELL_ADEC_BSI_M2BC_CH_CONFIG_L_R_C = 4,
	CELL_ADEC_BSI_M2BC_CH_CONFIG_L_R_LS_RS = 5,
	CELL_ADEC_BSI_M2BC_CH_CONFIG_L_R_C_S = 6,
	CELL_ADEC_BSI_M2BC_CH_CONFIG_L_R_C_LS_RS = 7,
	CELL_ADEC_BSI_M2BC_CH_CONFIG_LL_RR_CC_LS_RS_LC_RC = 8,
	CELL_ADEC_BSI_M2BC_CH_CONFIG_MONO_SECONDSTEREO = 9,
	CELL_ADEC_BSI_M2BC_CH_CONFIG_L_R_SECONDSTEREO = 10,
	CELL_ADEC_BSI_M2BC_CH_CONFIG_L_R_C_SECONDSTEREO = 11,
};

struct CellAdecParamMpmc
{
	be_t<u32> channelNumber;
	be_t<u32> downmix;
	be_t<u32> lfeUpSample;
};

struct CellAdecMpmcInfo
{
	be_t<u32> channelNumber;
	be_t<u32> sampleFreq;
	be_t<u32> errorPprotection;
	be_t<u32> bitrateIndex;
	be_t<u32> stereoMode;
	be_t<u32> stereoModeEextention;
	be_t<u32> emphasis;
	be_t<u32> copyright;
	be_t<u32> original;
	be_t<u32> surroundMode;
	be_t<u32> centerMode;
	be_t<u32> audioMmix;
	be_t<u32> outputFramSize;
	be_t<u32> multiCodecMode;
	be_t<u32> lfePresent;
	be_t<u32> channelCoufiguration;
};
