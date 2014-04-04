#pragma once

#include "Utilities/SQueue.h"

// Error Codes
enum
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


	CELL_ADEC_ERROR_ATX_OFFSET = 0x80612200,
	CELL_ADEC_ERROR_ATX_NONE                       = 0x80612200,
	CELL_ADEC_ERROR_ATX_OK                         = 0x80612200,
	CELL_ADEC_ERROR_ATX_BUSY                       = 0x80612264,
	CELL_ADEC_ERROR_ATX_EMPTY                      = 0x80612265,
	CELL_ADEC_ERROR_ATX_ATSHDR                     = 0x80612266,
	CELL_ADEC_ERROR_ATX_NON_FATAL                  = 0x80612281,
	CELL_ADEC_ERROR_ATX_NOT_IMPLE                  = 0x80612282,
	CELL_ADEC_ERROR_ATX_PACK_CE_OVERFLOW           = 0x80612283,
	CELL_ADEC_ERROR_ATX_ILLEGAL_NPROCQUS           = 0x80612284,
	CELL_ADEC_ERROR_ATX_FATAL                      = 0x8061228c,
	CELL_ADEC_ERROR_ATX_ENC_OVERFLOW               = 0x8061228d,
	CELL_ADEC_ERROR_ATX_PACK_CE_UNDERFLOW          = 0x8061228e,
	CELL_ADEC_ERROR_ATX_SYNTAX_IDCT                = 0x8061228f,
	CELL_ADEC_ERROR_ATX_SYNTAX_GAINADJ             = 0x80612290,
	CELL_ADEC_ERROR_ATX_SYNTAX_IDSF                = 0x80612291,
	CELL_ADEC_ERROR_ATX_SYNTAX_SPECTRA             = 0x80612292,
	CELL_ADEC_ERROR_ATX_SYNTAX_IDWL	               = 0x80612293,
	CELL_ADEC_ERROR_ATX_SYNTAX_GHWAVE              = 0x80612294,
	CELL_ADEC_ERROR_ATX_SYNTAX_SHEADER             = 0x80612295,
	CELL_ADEC_ERROR_ATX_SYNTAX_IDWL_A              = 0x80612296,
	CELL_ADEC_ERROR_ATX_SYNTAX_IDWL_B              = 0x80612297,
	CELL_ADEC_ERROR_ATX_SYNTAX_IDWL_C              = 0x80612298,
	CELL_ADEC_ERROR_ATX_SYNTAX_IDWL_D              = 0x80612299,
	CELL_ADEC_ERROR_ATX_SYNTAX_IDWL_E              = 0x8061229a,
	CELL_ADEC_ERROR_ATX_SYNTAX_IDSF_A              = 0x8061229b,
	CELL_ADEC_ERROR_ATX_SYNTAX_IDSF_B              = 0x8061229c,
	CELL_ADEC_ERROR_ATX_SYNTAX_IDSF_C              = 0x8061229d,
	CELL_ADEC_ERROR_ATX_SYNTAX_IDSF_D              = 0x8061229e,
	CELL_ADEC_ERROR_ATX_SYNTAX_IDCT_A              = 0x8061229f,
	CELL_ADEC_ERROR_ATX_SYNTAX_GC_NGC              = 0x806122a0,
	CELL_ADEC_ERROR_ATX_SYNTAX_GC_IDLEV_A          = 0x806122a1,
	CELL_ADEC_ERROR_ATX_SYNTAX_GC_IDLOC_A          = 0x806122a2,
	CELL_ADEC_ERROR_ATX_SYNTAX_GC_IDLEV_B          = 0x806122a3,
	CELL_ADEC_ERROR_ATX_SYNTAX_GC_IDLOC_B          = 0x806122a4,
	CELL_ADEC_ERROR_ATX_SYNTAX_SN_NWVS             = 0x806122a5,
	CELL_ADEC_ERROR_ATX_FATAL_HANDLE               = 0x806122aa,
	CELL_ADEC_ERROR_ATX_ASSERT_SAMPLING_FREQ       = 0x806122ab,
	CELL_ADEC_ERROR_ATX_ASSERT_CH_CONFIG_INDEX     = 0x806122ac,
	CELL_ADEC_ERROR_ATX_ASSERT_NBYTES              = 0x806122ad,
	CELL_ADEC_ERROR_ATX_ASSERT_BLOCK_NUM           = 0x806122ae,
	CELL_ADEC_ERROR_ATX_ASSERT_BLOCK_ID            = 0x806122af,
	CELL_ADEC_ERROR_ATX_ASSERT_CHANNELS            = 0x806122b0,
	CELL_ADEC_ERROR_ATX_UNINIT_BLOCK_SPECIFIED     = 0x806122b1,
	CELL_ADEC_ERROR_ATX_POSCFG_PRESENT             = 0x806122b2,
	CELL_ADEC_ERROR_ATX_BUFFER_OVERFLOW            = 0x806122b3,
	CELL_ADEC_ERROR_ATX_ILL_BLK_TYPE_ID            = 0x806122b4,
	CELL_ADEC_ERROR_ATX_UNPACK_CHANNEL_BLK_FAILED  = 0x806122b5,
	CELL_ADEC_ERROR_ATX_ILL_BLK_ID_USED_1          = 0x806122b6,
	CELL_ADEC_ERROR_ATX_ILL_BLK_ID_USED_2          = 0x806122b7,
	CELL_ADEC_ERROR_ATX_ILLEGAL_ENC_SETTING        = 0x806122b8,
	CELL_ADEC_ERROR_ATX_ILLEGAL_DEC_SETTING        = 0x806122b9,
	CELL_ADEC_ERROR_ATX_ASSERT_NSAMPLES            = 0x806122ba,

	CELL_ADEC_ERROR_ATX_ILL_SYNCWORD               = 0x806122bb,
	CELL_ADEC_ERROR_ATX_ILL_SAMPLING_FREQ          = 0x806122bc,
	CELL_ADEC_ERROR_ATX_ILL_CH_CONFIG_INDEX        = 0x806122bd,
	CELL_ADEC_ERROR_ATX_RAW_DATA_FRAME_SIZE_OVER   = 0x806122be,
	CELL_ADEC_ERROR_ATX_SYNTAX_ENHANCE_LENGTH_OVER = 0x806122bf,
	CELL_ADEC_ERROR_ATX_SPU_INTERNAL_FAIL          = 0x806122c8,


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
enum AudioCodecType
{
	CELL_ADEC_TYPE_RESERVED1,
	CELL_ADEC_TYPE_LPCM_PAMF,
	CELL_ADEC_TYPE_AC3,
	CELL_ADEC_TYPE_ATRACX,
	CELL_ADEC_TYPE_MP3,
	CELL_ADEC_TYPE_ATRAC3,
	CELL_ADEC_TYPE_MPEG_L2,
	CELL_ADEC_TYPE_RESERVED5,
	CELL_ADEC_TYPE_RESERVED6,
	CELL_ADEC_TYPE_RESERVED7,
	CELL_ADEC_TYPE_RESERVED8,
	CELL_ADEC_TYPE_CELP,
	CELL_ADEC_TYPE_RESERVED10,
	CELL_ADEC_TYPE_ATRACX_2CH,
	CELL_ADEC_TYPE_ATRACX_6CH,
	CELL_ADEC_TYPE_ATRACX_8CH,
	CELL_ADEC_TYPE_M4AAC,
	CELL_ADEC_TYPE_RESERVED12,
	CELL_ADEC_TYPE_RESERVED13,
	CELL_ADEC_TYPE_RESERVED14,
	CELL_ADEC_TYPE_RESERVED15,
	CELL_ADEC_TYPE_RESERVED16,
	CELL_ADEC_TYPE_RESERVED17,
	CELL_ADEC_TYPE_RESERVED18,
	CELL_ADEC_TYPE_RESERVED19,
	CELL_ADEC_TYPE_CELP8,
	CELL_ADEC_TYPE_RESERVED20,
	CELL_ADEC_TYPE_RESERVED21,
	CELL_ADEC_TYPE_RESERVED22,
	CELL_ADEC_TYPE_RESERVED23,
	CELL_ADEC_TYPE_RESERVED24,
	CELL_ADEC_TYPE_RESERVED25,
};

// Output Channel Number
enum CellAdecChannel
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
enum CellAdecSampleRate
{
	CELL_ADEC_FS_RESERVED1 = 0,
	CELL_ADEC_FS_48kHz = 1,
	CELL_ADEC_FS_16kHz = 2,
	CELL_ADEC_FS_8kHz = 5,
};

enum CellAdecBitLength
{
	CELL_ADEC_BIT_LENGTH_RESERVED1,
	CELL_ADEC_BIT_LENGTH_16,
	CELL_ADEC_BIT_LENGTH_20,
	CELL_ADEC_BIT_LENGTH_24,
};

struct CellAdecType
{
	be_t<AudioCodecType> audioCodecType;
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
	u8 priority[8];
	be_t<u32> maxContention;
};

// Callback Messages
enum CellAdecMsgType
{
	CELL_ADEC_MSG_TYPE_AUDONE,
	CELL_ADEC_MSG_TYPE_PCMOUT,
	CELL_ADEC_MSG_TYPE_ERROR,
	CELL_ADEC_MSG_TYPE_SEQDONE,
};

typedef mem_func_ptr_t<int (*)(u32 handle, CellAdecMsgType msgType, int msgData, u32 cbArg)> CellAdecCbMsg;

struct CellAdecCb
{
	be_t<u32> cbFunc;
	be_t<u32> cbArg;
};

typedef CellCodecTimeStamp CellAdecTimeStamp;

// AU Info
struct CellAdecAuInfo
{
	be_t<u32> startAddr;
	be_t<u32> size;
	CellCodecTimeStamp pts;
	be_t<u64> userData;
};

// BSI Info
struct CellAdecPcmAttr
{
	be_t<u32> bsiInfo_addr;
};

struct CellAdecPcmItem
{
	be_t<u32> pcmHandle;
	be_t<u32> status;
	be_t<u32> startAddr;
	be_t<u32> size;
	CellAdecPcmAttr	pcmAttr;
	CellAdecAuInfo auInfo;
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

// CELP Excitation Mode
enum CELP_ExcitationMode
{
	CELL_ADEC_CELP_EXCITATION_MODE_RPE = 1,
};

// CELP RPE Configuration
enum CELP_RPEConfig
{
	CELL_ADEC_CELP_RPE_CONFIG_0,
	CELL_ADEC_CELP_RPE_CONFIG_1,
	CELL_ADEC_CELP_RPE_CONFIG_2,
	CELL_ADEC_CELP_RPE_CONFIG_3,
};

// CELP Word Size
enum CELP_WordSize
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
enum CELP8_ExcitationMode
{
	CELL_ADEC_CELP8_EXCITATION_MODE_MPE = 0,
};

// CELP8 MPE Configuration
enum CELP8_MPEConfig
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
enum CELP8_WordSize
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

enum MPEG4AAC_ConfigType
{
	ADIFHeader = 0,
	ADTSHeader = 1,
	RawDataBlockOnly = 2,
};

enum MPEG4AAC_SamplingFreq
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
	be_t<MPEG4AAC_ConfigType> configNumber;

	union {
		struct mp { struct mp2
		{
			be_t<u32> adifProgramNumber; // 0
		} adifConfig; };

		struct mp3 { struct mp4
		{
			be_t<MPEG4AAC_SamplingFreq> samplingFreqIndex;
			be_t<u32> profile; // LC profile (1)
		} rawDataBlockConfig; };
	} configInfo;

	be_t<u32> enableDownmix; // enable downmix to 2.0 (if (enableDownmix))
};

// MPEG4 AAC BSI
struct CellAdecM4AacInfo
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
	be_t<MPEG4AAC_ConfigType> configNumber;
	
	be_t<u32> pad1; // TODO: check alignment

	union {
		struct mp5 {
			struct m6
			{
				be_t<u32> copyrightIdPresent;
				char copyrightId[9];
				be_t<u32> originalCopy;
				be_t<u32> home;
				be_t<u32> bitstreamType;
				be_t<u32> bitrate;
				be_t<u32> numberOfProgramConfigElements;
				be_t<u32> bufferFullness;
			} adif;
		};

		struct mp7 {
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
			} adts;
		};
	} bsi;

	be_t<u32> pad2; // TODO: check alignment

	struct mp9
	{
		be_t<u32> matrixMixdownPresent;
		be_t<u32> mixdownIndex;
		be_t<u32> pseudoSurroundEnable;
	} matrixMixdown;

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
	be_t<ATRAC3_JointType> isJoint;
	be_t<s32> nbytes; // byte count of single AU (???)
	be_t<ATRAC3_WordSize> bw_pcm; // bit length of output PCM sample
};

struct CellAdecAtrac3Info
{
	be_t<s32> nch;
	be_t<ATRAC3_JointType> isJoint;
	be_t<s32> nbytes;
};

enum ATRACX_WordSize : s32
{
	CELL_ADEC_ATRACX_WORD_SZ_16BIT = 0x02,
	CELL_ADEC_ATRACX_WORD_SZ_24BIT = 0x03,
	CELL_ADEC_ATRACX_WORD_SZ_32BIT = 0x04,
	CELL_ADEC_ATRACX_WORD_SZ_FLOAT = 0x84,
};

enum ATRACX_ATSHeaderInclude : u8
{
	CELL_ADEC_ATRACX_ATS_HDR_NOTINC = 0,
	CELL_ADEC_ATRACX_ATS_HDR_INC = 1,
};

enum ATRACX_DownmixFlag : u8
{
	ATRACX_DOWNMIX_OFF = 0,
	ATRACX_DOWNMIX_ON = 1,
};

struct CellAdecParamAtracX
{
	be_t<s32> sampling_freq;
	be_t<s32> ch_config_idx;
	be_t<s32> nch_out;
	be_t<s32> nbytes;
	u8 extra_config_data[4]; // downmix coefficients
	be_t<ATRACX_WordSize> bw_pcm;
	ATRACX_DownmixFlag downmix_flag;
	ATRACX_ATSHeaderInclude au_includes_ats_hdr_flg;
};

struct CellAdecAtracXInfo
{
	be_t<u32> samplingFreq; // [Hz]
	be_t<u32> channelConfigIndex;
	be_t<u32> nbytes;
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
	be_t<MP3_WordSize> bw_pcm;
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

enum M2BC_SampleFrequency
{
	CELL_ADEC_BSI_M2BC_SAMPLE_FREQUENCY_44 = 0,
	CELL_ADEC_BSI_M2BC_SAMPLE_FREQUENCY_48 = 1,
	CELL_ADEC_BSI_M2BC_SAMPLE_FREQUENCY_32 = 2,
};

enum M2BC_ErrorProtection
{
	CELL_ADEC_BSI_M2BC_ERROR_PROTECTION_NONE = 0,
	CELL_ADEC_BSI_M2BC_ERROR_PROTECTION_EXIST = 1,
};

enum M2BC_BitrateIndex
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

enum M2BC_StereoMode
{
	CELL_ADEC_BSI_M2BC_STEREO_MODE_STERO = 0,
	CELL_ADEC_BSI_M2BC_STEREO_MODE_JOINTSTERO = 1,
	CELL_ADEC_BSI_M2BC_STEREO_MODE_DUAL = 2,
	CELL_ADEC_BSI_M2BC_STEREO_MODE_MONO = 3,
};

enum M2BC_StereoModeEx
{
	CELL_ADEC_BSI_M2BC_STEREO_EXMODE_0 = 0,
	CELL_ADEC_BSI_M2BC_STEREO_EXMODE_1 = 1,
	CELL_ADEC_BSI_M2BC_STEREO_EXMODE_2 = 2,
	CELL_ADEC_BSI_M2BC_STEREO_EXMODE_3 = 3,
};

enum M2BC_Emphasis
{
	CELL_ADEC_BSI_M2BC_EMPHASIS_NONE = 0,
	CELL_ADEC_BSI_M2BC_EMPHASIS_50_15 = 1,
	CELL_ADEC_BSI_M2BC_EMPHASIS_CCITT = 3,
};

enum M2BC_CopyrightBit
{
	CELL_ADEC_BSI_M2BC_COPYRIGHT_NONE = 0,
	CELL_ADEC_BSI_M2BC_COPYRIGHT_ON = 1,
};

enum M2BC_OriginalBit
{
	CELL_ADEC_BSI_M2BC_ORIGINAL_COPY = 0,
	CELL_ADEC_BSI_M2BC_ORIGINAL_ORIGINAL = 1,
};

enum M2BC_SurroundMode
{
	CELL_ADEC_BSI_M2BC_SURROUND_NONE = 0,
	CELL_ADEC_BSI_M2BC_SURROUND_MONO = 1,
	CELL_ADEC_BSI_M2BC_SURROUND_STEREO = 2,
	CELL_ADEC_BSI_M2BC_SURROUND_SECOND = 3,
};

enum M2BC_CenterMode
{
	CELL_ADEC_BSI_M2BC_CENTER_NONE = 0,
	CELL_ADEC_BSI_M2BC_CENTER_EXIST = 1,
	CELL_ADEC_BSI_M2BC_CENTER_PHANTOM = 3,
};

enum M2BC_LFE
{
	CELL_ADEC_BSI_M2BC_LFE_NONE = 0,
	CELL_ADEC_BSI_M2BC_LFE_EXIST = 1,
};

enum M2BC_AudioMixMode
{
	CELL_ADEC_BSI_M2BC_AUDIOMIX_LARGE = 0,
	CELL_ADEC_BSI_M2BC_AUDIOMIX_SMALLE = 1,
};

enum M2BC_MCExtension
{
	CELL_ADEC_BSI_M2BC_MCEXTENSION_2CH = 0,
	CELL_ADEC_BSI_M2BC_MCEXTENSION_5CH = 1,
	CELL_ADEC_BSI_M2BC_MCEXTENSION_7CH = 2,
};

enum M2BC_ChannelConfig
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

/* Audio Decoder Thread Classes */

enum AdecJobType : u32
{
	adecStartSeq,
	adecEndSeq,
	adecDecodeAu,
	adecClose,
};

struct AdecTask
{
	AdecJobType type;
	union
	{
		struct
		{
			u32 auInfo_addr;
			u32 addr;
			u32 size;
			u64 pts;
			u64 userdata;
		} au;
	};

	AdecTask(AdecJobType type)
		: type(type)
	{
	}

	AdecTask()
	{
	}
};

struct AdecFrame
{
	AVFrame* data;
	u64 pts;
	u64 userdata;
	u32 auAddr;
	u32 auSize;
	u32 size;
};

int adecRead(void* opaque, u8* buf, int buf_size);

struct OMAHeader // OMA Header
{
	u32 magic; // 0x01334145
	u16 size; // 96 << 8
	u16 unk0; // 0xffff
	u64 unk1; // 0x00500f0100000000ULL
	u64 unk2; // 0xcef5000000000400ULL
	u64 unk3; // 0x1c458024329192d2ULL
	u8 codecId; // 1 for ATRAC3P
	u8 reserved0; // 0
	u8 code1;
	u8 code2;
	u32 reserved1; // 0
	u64 reserved[7]; // 0

	OMAHeader(u8 id, u8 code1, u8 code2)
		: magic(0x01334145)
		, size(96 << 8)
		, unk0(0xffff)
		, unk1(0x00500f0100000000ULL)
		, unk2(0xcef5000000000400ULL)
		, unk3(0x1c458024329192d2ULL)
		, codecId(id)
		, reserved0(0)
		, code1(code1)
		, code2(code2)
		, reserved1(0)
	{
		memset(reserved, 0, sizeof(reserved));
	}
};

static_assert(sizeof(OMAHeader) == 96, "Wrong OMAHeader size");

class AudioDecoder
{
public:
	SQueue<AdecTask> job;
	u32 id;
	volatile bool is_running;
	volatile bool is_finished;
	bool just_started;

	AVCodecContext* ctx;
	AVFormatContext* fmt;
	u8* io_buf;

	struct AudioReader
	{
		u32 addr;
		u32 size;
		bool init;
		u8* rem;
		u32 rem_size;

		AudioReader()
			: rem(nullptr)
			, rem_size(0)
		{
		}

		~AudioReader()
		{
			if (rem) free(rem);
			rem = nullptr;
		}
	} reader;

	SQueue<AdecFrame> frames;

	const AudioCodecType type;
	const u32 memAddr;
	const u32 memSize;
	const u32 cbFunc;
	const u32 cbArg;
	u32 memBias;

	AdecTask task;
	u64 last_pts, first_pts;

	CPUThread* adecCb;

	AudioDecoder(AudioCodecType type, u32 addr, u32 size, u32 func, u32 arg)
		: type(type)
		, memAddr(addr)
		, memSize(size)
		, memBias(0)
		, cbFunc(func)
		, cbArg(arg)
		, adecCb(nullptr)
		, is_running(false)
		, is_finished(false)
		, just_started(false)
		, ctx(nullptr)
		, fmt(nullptr)
	{
		AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_ATRAC3P);
		if (!codec)
		{
			ConLog.Error("AudioDecoder(): avcodec_find_decoder(ATRAC3P) failed");
			Emu.Pause();
			return;
		}
		fmt = avformat_alloc_context();
		if (!fmt)
		{
			ConLog.Error("AudioDecoder(): avformat_alloc_context failed");
			Emu.Pause();
			return;
		}
		io_buf = (u8*)av_malloc(4096);
		fmt->pb = avio_alloc_context(io_buf, 4096, 0, this, adecRead, NULL, NULL);
		if (!fmt->pb)
		{
			ConLog.Error("AudioDecoder(): avio_alloc_context failed");
			Emu.Pause();
			return;
		}
	}

	~AudioDecoder()
	{
		if (ctx)
		{
			for (u32 i = frames.GetCount() - 1; ~i; i--)
			{
				AdecFrame& af = frames.Peek(i);
				av_frame_unref(af.data);
				av_frame_free(&af.data);
			}
			avcodec_close(ctx);
			avformat_close_input(&fmt);
		}
		if (fmt)
		{
			if (io_buf)
			{
				av_free(io_buf);
			}
			if (fmt->pb) av_free(fmt->pb);
			avformat_free_context(fmt);
		}
	}
};
