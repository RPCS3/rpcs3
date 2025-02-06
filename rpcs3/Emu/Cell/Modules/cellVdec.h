#pragma once

// Error Codes
enum CellVdecError : u32
{
	CELL_VDEC_ERROR_ARG   = 0x80610101,
	CELL_VDEC_ERROR_SEQ   = 0x80610102,
	CELL_VDEC_ERROR_BUSY  = 0x80610103,
	CELL_VDEC_ERROR_EMPTY = 0x80610104,
	CELL_VDEC_ERROR_AU    = 0x80610105,
	CELL_VDEC_ERROR_PIC   = 0x80610106,
	CELL_VDEC_ERROR_FATAL = 0x80610180,
};

enum CellVdecCodecType : s32
{
	CELL_VDEC_CODEC_TYPE_MPEG2 = 0,
	CELL_VDEC_CODEC_TYPE_AVC   = 1,
	CELL_VDEC_CODEC_TYPE_DIVX  = 5,
	CELL_VDEC_CODEC_TYPE_MAX
};

// Callback Messages
enum CellVdecMsgType : s32
{
	CELL_VDEC_MSG_TYPE_AUDONE  = 0, // decoding finished
	CELL_VDEC_MSG_TYPE_PICOUT  = 1, // picture done
	CELL_VDEC_MSG_TYPE_SEQDONE = 2, // finishing done
	CELL_VDEC_MSG_TYPE_ERROR   = 3, // fatal error
};

// Decoder Operation Mode
enum CellVdecDecodeMode : s32
{
	CELL_VDEC_DEC_MODE_NORMAL,
	CELL_VDEC_DEC_MODE_B_SKIP,
	CELL_VDEC_DEC_MODE_PB_SKIP,
};

// Output Picture Format Type
enum CellVdecPicFormatType : u32
{
	CELL_VDEC_PICFMT_ARGB32_ILV,
	CELL_VDEC_PICFMT_RGBA32_ILV,
	CELL_VDEC_PICFMT_UYVY422_ILV,
	CELL_VDEC_PICFMT_YUV420_PLANAR,
};

// Output Color Matrix Coef
enum CellVdecColorMatrixType : u32
{
	CELL_VDEC_COLOR_MATRIX_TYPE_BT601,
	CELL_VDEC_COLOR_MATRIX_TYPE_BT709,
};

enum CellVdecPicAttr : s32
{
	CELL_VDEC_PICITEM_ATTR_NORMAL,
	CELL_VDEC_PICITEM_ATTR_SKIPPED,
};

// Universal Frame Rate Code
enum CellVdecFrameRate : s32
{
	CELL_VDEC_FRC_24000DIV1001 = 0x80,
	CELL_VDEC_FRC_24           = 0x81,
	CELL_VDEC_FRC_25           = 0x82,
	CELL_VDEC_FRC_30000DIV1001 = 0x83,
	CELL_VDEC_FRC_30           = 0x84,
	CELL_VDEC_FRC_50           = 0x85,
	CELL_VDEC_FRC_60000DIV1001 = 0x86,
	CELL_VDEC_FRC_60           = 0x87,
};

enum
{
	CELL_CODEC_PTS_INVALID = 0xffffffff,
	CELL_CODEC_DTS_INVALID = 0xffffffff,
};

// Codec Type Information
struct CellVdecType
{
	be_t<s32> codecType; // CellVdecCodecType
	be_t<u32> profileLevel;
};

// Extended Codec Type Information
struct CellVdecTypeEx
{
	be_t<s32> codecType; // CellVdecCodecType
	be_t<u32> profileLevel;
	be_t<u32> codecSpecificInfo_addr;
};

// Library Attributes
struct CellVdecAttr
{
	be_t<u32> memSize; // required memory
	u8 cmdDepth; // command queue depth
	be_t<u32> decoderVerUpper;
	be_t<u32> decoderVerLower;
};

// Configurable Information
struct CellVdecResource
{
	be_t<u32> memAddr;
	be_t<u32> memSize;
	be_t<s32> ppuThreadPriority;
	be_t<u32> ppuThreadStackSize;
	be_t<s32> spuThreadPriority;
	be_t<u32> numOfSpus;
};

// SPURS Information
struct CellVdecResourceSpurs
{
	be_t<u32> spursAddr;
	u8 tasksetPriority[8];
	be_t<u32> tasksetMaxContention;
};

// Extended Configurable Information
struct CellVdecResourceEx
{
	be_t<u32> memAddr;
	be_t<u32> memSize;
	be_t<s32> ppuThreadPriority;
	be_t<u32> ppuThreadStackSize;
	be_t<s32> spuThreadPriority;
	be_t<u32> numOfSpus;
	be_t<u32> spursResource_addr;
};

struct CellVdecResourceExt // speculative
{
	be_t<u32> memAddr;
	be_t<u32> memSize;
	be_t<s32> ppuThreadPriority;
	be_t<u32> ppuThreadStackSize;
	u8 unk[12];
	be_t<u32> numOfSpus;
};

// Access Unit Information
struct CellVdecAuInfo
{
	be_t<u32> startAddr;
	be_t<u32> size;
	CellCodecTimeStamp pts;
	CellCodecTimeStamp dts;
	be_t<u64> userData;
	be_t<u64> codecSpecificData;
};

// Access Unit Information
struct CellVdecAuInfoEx2 // Speculative name
{
	be_t<u32> startAddr;
	be_t<u32> unk1; // Speculative
	be_t<u32> size;
	be_t<u32> unk2; // Speculative
	CellCodecTimeStamp pts;
	CellCodecTimeStamp dts;
	be_t<u64> userData;
	be_t<u64> codecSpecificData;
};

// Output Picture Information
struct CellVdecPicItem
{
	be_t<s32> codecType; // CellVdecCodecType
	be_t<u32> startAddr;
	be_t<u32> size;
	u8 auNum;
	CellCodecTimeStamp auPts[2];
	CellCodecTimeStamp auDts[2];
	be_t<u64> auUserData[2];
	be_t<s32> status;
	be_t<s32> attr; // CellVdecPicAttr
	be_t<u32> picInfo_addr;
};

// Output Picture Format
struct CellVdecPicFormat
{
	be_t<u32> formatType; // CellVdecPicFormatType
	be_t<u32> colorMatrixType; // CellVdecColorMatrixType
	u8 alpha;
};

struct CellVdecPicFormat2
{
	be_t<u32> formatType; // CellVdecPicFormatType
	be_t<u32> colorMatrixType; // CellVdecColorMatrixType
	be_t<u32> unk0;
	u8 alpha;
	be_t<u32> unk1;
};

using CellVdecCbMsg = u32(u32 handle, CellVdecMsgType msgType, s32 msgData, u32 cbArg);

// Callback Function Information
struct CellVdecCb
{
	vm::bptr<CellVdecCbMsg> cbFunc;
	be_t<u32> cbArg;
};

// Max CC Data Length
enum
{
	CELL_VDEC_AVC_CCD_MAX = 128,
};

enum AVC_level : u8
{
	CELL_VDEC_AVC_LEVEL_1P0	= 10,
	CELL_VDEC_AVC_LEVEL_1P1	= 11,
	CELL_VDEC_AVC_LEVEL_1P2	= 12,
	CELL_VDEC_AVC_LEVEL_1P3	= 13,
	CELL_VDEC_AVC_LEVEL_2P0	= 20,
	CELL_VDEC_AVC_LEVEL_2P1	= 21,
	CELL_VDEC_AVC_LEVEL_2P2	= 22,
	CELL_VDEC_AVC_LEVEL_3P0	= 30,
	CELL_VDEC_AVC_LEVEL_3P1	= 31,
	CELL_VDEC_AVC_LEVEL_3P2	= 32,
	CELL_VDEC_AVC_LEVEL_4P0	= 40,
	CELL_VDEC_AVC_LEVEL_4P1	= 41,
	CELL_VDEC_AVC_LEVEL_4P2	= 42,
};

struct CellVdecAvcSpecificInfo
{
	be_t<u32> thisSize;
	be_t<u16> maxDecodedFrameWidth;
	be_t<u16> maxDecodedFrameHeight;
	b8 disableDeblockingFilter;
	u8 numberOfDecodedFrameBuffer;
};

enum AVC_video_format : u8
{
	CELL_VDEC_AVC_VF_COMPONENT                = 0x00,
	CELL_VDEC_AVC_VF_PAL                      = 0x01,
	CELL_VDEC_AVC_VF_NTSC                     = 0x02,
	CELL_VDEC_AVC_VF_SECAM                    = 0x03,
	CELL_VDEC_AVC_VF_MAC                      = 0x04,
	CELL_VDEC_AVC_VF_UNSPECIFIED              = 0x05,
};

enum AVC_colour_primaries : u8
{
	CELL_VDEC_AVC_CP_ITU_R_BT_709_5           = 0x01,
	CELL_VDEC_AVC_CP_UNSPECIFIED              = 0x02,
	CELL_VDEC_AVC_CP_ITU_R_BT_470_6_SYS_M     = 0x04,
	CELL_VDEC_AVC_CP_ITU_R_BT_470_6_SYS_BG    = 0x05,
	CELL_VDEC_AVC_CP_SMPTE_170_M              = 0x06,
	CELL_VDEC_AVC_CP_SMPTE_240_M              = 0x07,
	CELL_VDEC_AVC_CP_GENERIC_FILM             = 0x08,
};

enum AVC_transfer_characteristics : u8
{
	CELL_VDEC_AVC_TC_ITU_R_BT_709_5           = 0x01,
	CELL_VDEC_AVC_TC_UNSPECIFIED              = 0x02,
	CELL_VDEC_AVC_TC_ITU_R_BT_470_6_SYS_M     = 0x04,
	CELL_VDEC_AVC_TC_ITU_R_BT_470_6_SYS_BG    = 0x05,
	CELL_VDEC_AVC_TC_SMPTE_170_M              = 0x06,
	CELL_VDEC_AVC_TC_SMPTE_240_M              = 0x07,
	CELL_VDEC_AVC_TC_LINEAR                   = 0x08,
	CELL_VDEC_AVC_TC_LOG_100_1                = 0x09,
	CELL_VDEC_AVC_TC_LOG_316_1                = 0x0a,
};

enum AVC_matrix_coefficients : u8
{
	CELL_VDEC_AVC_MXC_GBR                     = 0x00,
	CELL_VDEC_AVC_MXC_ITU_R_BT_709_5          = 0x01,
	CELL_VDEC_AVC_MXC_UNSPECIFIED             = 0x02,
	CELL_VDEC_AVC_MXC_FCC                     = 0x04,
	CELL_VDEC_AVC_MXC_ITU_R_BT_470_6_SYS_BG   = 0x05,
	CELL_VDEC_AVC_MXC_SMPTE_170_M             = 0x06,
	CELL_VDEC_AVC_MXC_SMPTE_240_M             = 0x07,
	CELL_VDEC_AVC_MXC_YCGCO                   = 0x08,
};

enum AVC_FrameRateCode : u8
{
	CELL_VDEC_AVC_FRC_24000DIV1001            = 0x00,
	CELL_VDEC_AVC_FRC_24                      = 0x01,
	CELL_VDEC_AVC_FRC_25                      = 0x02,
	CELL_VDEC_AVC_FRC_30000DIV1001            = 0x03,
	CELL_VDEC_AVC_FRC_30                      = 0x04,
	CELL_VDEC_AVC_FRC_50                      = 0x05,
	CELL_VDEC_AVC_FRC_60000DIV1001            = 0x06,
	CELL_VDEC_AVC_FRC_60                      = 0x07,
};

enum AVC_NulUnitPresentFlags : u16
{
	CELL_VDEC_AVC_FLG_SPS                     = 0x0001,
	CELL_VDEC_AVC_FLG_PPS                     = 0x0002,
	CELL_VDEC_AVC_FLG_AUD                     = 0x0004,
	CELL_VDEC_AVC_FLG_EO_SEQ                  = 0x0008,
	CELL_VDEC_AVC_FLG_EO_STREAM               = 0x0100,
	CELL_VDEC_AVC_FLG_FILLER_DATA             = 0x0200,
	CELL_VDEC_AVC_FLG_PIC_TIMING_SEI          = 0x0400,
	CELL_VDEC_AVC_FLG_BUFF_PERIOD_SEI         = 0x0800,
	CELL_VDEC_AVC_FLG_USER_DATA_UNREG_SEI     = 0x1000,
};

enum AVC_aspect_ratio_idc : u8
{
	CELL_VDEC_AVC_ARI_SAR_UNSPECIFIED         = 0x00,
	CELL_VDEC_AVC_ARI_SAR_1_1                 = 0x01,
	CELL_VDEC_AVC_ARI_SAR_12_11               = 0x02,
	CELL_VDEC_AVC_ARI_SAR_10_11               = 0x03,
	CELL_VDEC_AVC_ARI_SAR_16_11               = 0x04,
	CELL_VDEC_AVC_ARI_SAR_40_33               = 0x05,
	CELL_VDEC_AVC_ARI_SAR_24_11               = 0x06,
	CELL_VDEC_AVC_ARI_SAR_20_11               = 0x07,
	CELL_VDEC_AVC_ARI_SAR_32_11               = 0x08,
	CELL_VDEC_AVC_ARI_SAR_80_33               = 0x09,
	CELL_VDEC_AVC_ARI_SAR_18_11               = 0x0a,
	CELL_VDEC_AVC_ARI_SAR_15_11               = 0x0b,
	CELL_VDEC_AVC_ARI_SAR_64_33               = 0x0c,
	CELL_VDEC_AVC_ARI_SAR_160_99              = 0x0d,
	CELL_VDEC_AVC_ARI_SAR_4_3                 = 0x0e,
	CELL_VDEC_AVC_ARI_SAR_3_2                 = 0x0f,
	CELL_VDEC_AVC_ARI_SAR_2_1                 = 0x10,
	CELL_VDEC_AVC_ARI_SAR_EXTENDED_SAR        = 0xff,
};

enum AVC_PictureType : u8
{
	CELL_VDEC_AVC_PCT_I                       = 0x00,
	CELL_VDEC_AVC_PCT_P                       = 0x01,
	CELL_VDEC_AVC_PCT_B                       = 0x02,
	CELL_VDEC_AVC_PCT_UNKNOWN                 = 0x03,
};

enum AVC_pic_struct : u8
{
	CELL_VDEC_AVC_PSTR_FRAME                  = 0x00,
	CELL_VDEC_AVC_PSTR_FIELD_TOP              = 0x01,
	CELL_VDEC_AVC_PSTR_FIELD_BTM              = 0x02,
	CELL_VDEC_AVC_PSTR_FIELD_TOP_BTM          = 0x03,
	CELL_VDEC_AVC_PSTR_FIELD_BTM_TOP          = 0x04,
	CELL_VDEC_AVC_PSTR_FIELD_TOP_BTM_TOP      = 0x05,
	CELL_VDEC_AVC_PSTR_FIELD_BTM_TOP_BTM      = 0x06,
	CELL_VDEC_AVC_PSTR_FRAME_DOUBLING         = 0x07,
	CELL_VDEC_AVC_PSTR_FRAME_TRIPLING         = 0x08,
};

struct CellVdecAvcInfo
{
	be_t<u16> horizontalSize;
	be_t<u16> verticalSize;
	AVC_PictureType pictureType[2];
	b8 idrPictureFlag;
	AVC_aspect_ratio_idc aspect_ratio_idc;
	be_t<u16> sar_height;
	be_t<u16> sar_width;
	AVC_pic_struct pic_struct;
	be_t<s16> picOrderCount[2];
	b8 vui_parameters_present_flag;
	b8 frame_mbs_only_flag;
	b8 video_signal_type_present_flag;
	AVC_video_format video_format;
	b8 video_full_range_flag;
	b8 colour_description_present_flag;
	AVC_colour_primaries colour_primaries;
	AVC_transfer_characteristics transfer_characteristics;
	AVC_matrix_coefficients matrix_coefficients;
	b8 timing_info_present_flag;
	AVC_FrameRateCode frameRateCode; // ???
	b8 fixed_frame_rate_flag;
	b8 low_delay_hrd_flag;
	b8 entropy_coding_mode_flag;
	be_t<u16> nalUnitPresentFlags;
	u8 ccDataLength[2];
	u8 ccData[2][CELL_VDEC_AVC_CCD_MAX];
	be_t<u64> reserved[2];
};

// DIVX Profile
enum DIVX_level : u8
{
	CELL_VDEC_DIVX_QMOBILE                   = 10,
	CELL_VDEC_DIVX_MOBILE                    = 11,
	CELL_VDEC_DIVX_HOME_THEATER              = 12,
	CELL_VDEC_DIVX_HD_720                    = 13,
	CELL_VDEC_DIVX_HD_1080                   = 14,
};

struct CellVdecDivxSpecificInfo
{
	be_t<u32> thisSize;
	be_t<u16> maxDecodedFrameWidth;
	be_t<u16> maxDecodedFrameHeight;
};

struct CellVdecDivxSpecificInfo2
{
	be_t<u32> thisSize;
	be_t<u16> maxDecodedFrameWidth;
	be_t<u16> maxDecodedFrameHeight;
	be_t<u16> numberOfDecodedFrameBuffer;
};

enum DIVX_frameRateCode : u16
{
	CELL_VDEC_DIVX_FRC_UNDEFINED             = 0x00,
	CELL_VDEC_DIVX_FRC_24000DIV1001          = 0x01,
	CELL_VDEC_DIVX_FRC_24                    = 0x02,
	CELL_VDEC_DIVX_FRC_25                    = 0x03,
	CELL_VDEC_DIVX_FRC_30000DIV1001          = 0x04,
	CELL_VDEC_DIVX_FRC_30                    = 0x05,
	CELL_VDEC_DIVX_FRC_50                    = 0x06,
	CELL_VDEC_DIVX_FRC_60000DIV1001          = 0x07,
	CELL_VDEC_DIVX_FRC_60                    = 0x08,
};

enum DIVX_pixelAspectRatio : u8
{
	CELL_VDEC_DIVX_ARI_PAR_1_1               = 0x1,
	CELL_VDEC_DIVX_ARI_PAR_12_11             = 0x2,
	CELL_VDEC_DIVX_ARI_PAR_10_11             = 0x3,
	CELL_VDEC_DIVX_ARI_PAR_16_11             = 0x4,
	CELL_VDEC_DIVX_ARI_PAR_40_33             = 0x5,
	CELL_VDEC_DIVX_ARI_PAR_EXTENDED_PAR      = 0xF,
};

enum DIVX_pictureType : u8
{
	CELL_VDEC_DIVX_VCT_I                     = 0x0,
	CELL_VDEC_DIVX_VCT_P                     = 0x1,
	CELL_VDEC_DIVX_VCT_B                     = 0x2,
};

enum DIVX_pictureStruct : u8
{
	CELL_VDEC_DIVX_PSTR_FRAME                = 0x0,
	CELL_VDEC_DIVX_PSTR_TOP_BTM              = 0x1,
	CELL_VDEC_DIVX_PSTR_BTM_TOP              = 0x2,
};

enum DIVX_colourPrimaries : u8
{
	CELL_VDEC_DIVX_CP_ITU_R_BT_709           = 0x01,
	CELL_VDEC_DIVX_CP_UNSPECIFIED            = 0x02,
	CELL_VDEC_DIVX_CP_ITU_R_BT_470_SYS_M     = 0x04,
	CELL_VDEC_DIVX_CP_ITU_R_BT_470_SYS_BG    = 0x05,
	CELL_VDEC_DIVX_CP_SMPTE_170_M            = 0x06,
	CELL_VDEC_DIVX_CP_SMPTE_240_M            = 0x07,
	CELL_VDEC_DIVX_CP_GENERIC_FILM           = 0x08,
};

enum DIVX_transferCharacteristics : u8
{
	CELL_VDEC_DIVX_TC_ITU_R_BT_709           = 0x01,
	CELL_VDEC_DIVX_TC_UNSPECIFIED            = 0x02,
	CELL_VDEC_DIVX_TC_ITU_R_BT_470_SYS_M     = 0x04,
	CELL_VDEC_DIVX_TC_ITU_R_BT_470_SYS_BG    = 0x05,
	CELL_VDEC_DIVX_TC_SMPTE_170_M            = 0x06,
	CELL_VDEC_DIVX_TC_SMPTE_240_M            = 0x07,
	CELL_VDEC_DIVX_TC_LINEAR                 = 0x08,
	CELL_VDEC_DIVX_TC_LOG_100_1              = 0x09,
	CELL_VDEC_DIVX_TC_LOG_316_1              = 0x0a,
};

enum DIVX_matrixCoefficients : u8
{
	CELL_VDEC_DIVX_MXC_ITU_R_BT_709          = 0x01,
	CELL_VDEC_DIVX_MXC_UNSPECIFIED           = 0x02,
	CELL_VDEC_DIVX_MXC_FCC                   = 0x04,
	CELL_VDEC_DIVX_MXC_ITU_R_BT_470_SYS_BG   = 0x05,
	CELL_VDEC_DIVX_MXC_SMPTE_170_M           = 0x06,
	CELL_VDEC_DIVX_MXC_SMPTE_240_M           = 0x07,
	CELL_VDEC_DIVX_MXC_YCGCO                 = 0x08,
};

struct CellVdecDivxInfo
{
	DIVX_pictureType pictureType;
	be_t<u16> horizontalSize;
	be_t<u16> verticalSize;
	DIVX_pixelAspectRatio pixelAspectRatio;
	u8 parWidth;
	u8 parHeight;
	b8 colourDescription;
	DIVX_colourPrimaries colourPrimaries;
	DIVX_transferCharacteristics transferCharacteristics;
	DIVX_matrixCoefficients matrixCoefficients;
	DIVX_pictureStruct pictureStruct;
	be_t<u16> frameRateCode; // DIVX_frameRateCode
};

enum MPEG2_level
{
	CELL_VDEC_MPEG2_MP_LL,
	CELL_VDEC_MPEG2_MP_ML,
	CELL_VDEC_MPEG2_MP_H14,
	CELL_VDEC_MPEG2_MP_HL,
};

struct CellVdecMpeg2SpecificInfo
{
	be_t<u32> thisSize;
	be_t<u16> maxDecodedFrameWidth;
	be_t<u16> maxDecodedFrameHeight;
};

enum MPEG2_headerFlags : u32
{
	CELL_VDEC_MPEG2_FLG_SEQ_HDR       = 0x00000001,
	CELL_VDEC_MPEG2_FLG_SEQ_EXT       = 0x00000002,
	CELL_VDEC_MPEG2_FLG_SEQ_DSP_EXT   = 0x00000004,
	CELL_VDEC_MPEG2_FLG_SEQ_USR_DAT   = 0x00000008,
	CELL_VDEC_MPEG2_FLG_SEQ_END       = 0x00000010,
	CELL_VDEC_MPEG2_FLG_GOP_HDR       = 0x00000020,
	CELL_VDEC_MPEG2_FLG_GOP_USR_DAT   = 0x00000040,
	CELL_VDEC_MPEG2_FLG_PIC_HDR_1     = 0x00000100,
	CELL_VDEC_MPEG2_FLG_PIC_EXT_1     = 0x00000200,
	CELL_VDEC_MPEG2_FLG_PIC_DSP_EXT_1 = 0x00000400,
	CELL_VDEC_MPEG2_FLG_PIC_USR_DAT_1 = 0x00000800,
	CELL_VDEC_MPEG2_FLG_PIC_HDR_2     = 0x00001000,
	CELL_VDEC_MPEG2_FLG_PIC_EXT_2     = 0x00002000,
	CELL_VDEC_MPEG2_FLG_PIC_DSP_EXT_2 = 0x00004000,
	CELL_VDEC_MPEG2_FLG_PIC_USR_DAT_2 = 0x00008000,
};

enum MPEG2_aspectRatio : u8
{
	CELL_VDEC_MPEG2_ARI_SAR_1_1                 = 0x01,
	CELL_VDEC_MPEG2_ARI_DAR_4_3                 = 0x02,
	CELL_VDEC_MPEG2_ARI_DAR_16_9                = 0x03,
	CELL_VDEC_MPEG2_ARI_DAR_2P21_1              = 0x04,
};

enum MPEG1_aspectRatio : u8
{
	CELL_VDEC_MPEG1_ARI_SAR_1P0                 = 0x01,
	CELL_VDEC_MPEG1_ARI_SAR_0P6735              = 0x02,
	CELL_VDEC_MPEG1_ARI_SAR_0P7031              = 0x03,
	CELL_VDEC_MPEG1_ARI_SAR_0P7615              = 0x04,
	CELL_VDEC_MPEG1_ARI_SAR_0P8055              = 0x05,
	CELL_VDEC_MPEG1_ARI_SAR_0P8437              = 0x06,
	CELL_VDEC_MPEG1_ARI_SAR_0P8935              = 0x07,
	CELL_VDEC_MPEG1_ARI_SAR_0P9157              = 0x08,
	CELL_VDEC_MPEG1_ARI_SAR_0P9815              = 0x09,
	CELL_VDEC_MPEG1_ARI_SAR_1P0255              = 0x0a,
	CELL_VDEC_MPEG1_ARI_SAR_1P0695              = 0x0b,
	CELL_VDEC_MPEG1_ARI_SAR_1P0950              = 0x0c,
	CELL_VDEC_MPEG1_ARI_SAR_1P1575              = 0x0d,
	CELL_VDEC_MPEG1_ARI_SAR_1P2015              = 0x0e,
};

enum MPEG2_frameRate : u8
{
	CELL_VDEC_MPEG2_FRC_FORBIDDEN               = 0x00,
	CELL_VDEC_MPEG2_FRC_24000DIV1001            = 0x01,
	CELL_VDEC_MPEG2_FRC_24                      = 0x02,
	CELL_VDEC_MPEG2_FRC_25                      = 0x03,
	CELL_VDEC_MPEG2_FRC_30000DIV1001            = 0x04,
	CELL_VDEC_MPEG2_FRC_30                      = 0x05,
	CELL_VDEC_MPEG2_FRC_50                      = 0x06,
	CELL_VDEC_MPEG2_FRC_60000DIV1001            = 0x07,
	CELL_VDEC_MPEG2_FRC_60                      = 0x08,
};

enum MPEG2_videoFormat : u8
{
	CELL_VDEC_MPEG2_VF_COMPONENT                = 0x00,
	CELL_VDEC_MPEG2_VF_PAL                      = 0x01,
	CELL_VDEC_MPEG2_VF_NTSC                     = 0x02,
	CELL_VDEC_MPEG2_VF_SECAM                    = 0x03,
	CELL_VDEC_MPEG2_VF_MAC                      = 0x04,
	CELL_VDEC_MPEG2_VF_UNSPECIFIED              = 0x05,
};

enum MPEG2_colourPrimaries : u8
{
	CELL_VDEC_MPEG2_CP_FORBIDDEN                = 0x00,
	CELL_VDEC_MPEG2_CP_ITU_R_BT_709             = 0x01,
	CELL_VDEC_MPEG2_CP_UNSPECIFIED              = 0x02,
	CELL_VDEC_MPEG2_CP_ITU_R_BT_470_2_SYS_M     = 0x04,
	CELL_VDEC_MPEG2_CP_ITU_R_BT_470_2_SYS_BG    = 0x05,
	CELL_VDEC_MPEG2_CP_SMPTE_170_M              = 0x06,
	CELL_VDEC_MPEG2_CP_SMPTE_240_M              = 0x07,
};

enum MPEG2_transferCharacteristics : u8
{
	CELL_VDEC_MPEG2_TC_FORBIDDEN                = 0x00,
	CELL_VDEC_MPEG2_TC_ITU_R_BT_709             = 0x01,
	CELL_VDEC_MPEG2_TC_UNSPECIFIED              = 0x02,
	CELL_VDEC_MPEG2_TC_ITU_R_BT_470_2_SYS_M     = 0x04,
	CELL_VDEC_MPEG2_TC_ITU_R_BT_470_2_SYS_BG    = 0x05,
	CELL_VDEC_MPEG2_TC_SMPTE_170_M              = 0x06,
	CELL_VDEC_MPEG2_TC_SMPTE_240_M              = 0x07,
	CELL_VDEC_MPEG2_TC_LINEAR                   = 0x08,
	CELL_VDEC_MPEG2_TC_LOG_100_1                = 0x09,
	CELL_VDEC_MPEG2_TC_LOG_316_1                = 0x0a,
};

enum MPEG2_matrixCoefficients : u8
{
	CELL_VDEC_MPEG2_MXC_FORBIDDEN               = 0x00,
	CELL_VDEC_MPEG2_MXC_ITU_R_BT_709            = 0x01,
	CELL_VDEC_MPEG2_MXC_UNSPECIFIED             = 0x02,
	CELL_VDEC_MPEG2_MXC_FCC                     = 0x04,
	CELL_VDEC_MPEG2_MXC_ITU_R_BT_470_2_SYS_BG   = 0x05,
	CELL_VDEC_MPEG2_MXC_SMPTE_170_M             = 0x06,
	CELL_VDEC_MPEG2_MXC_SMPTE_240_M             = 0x07,
};

enum MPEG2_pictureCodingType : u8
{
	CELL_VDEC_MPEG2_PCT_FORBIDDEN               = 0x00,
	CELL_VDEC_MPEG2_PCT_I                       = 0x01,
	CELL_VDEC_MPEG2_PCT_P                       = 0x02,
	CELL_VDEC_MPEG2_PCT_B                       = 0x03,
	CELL_VDEC_MPEG2_PCT_D                       = 0x04,
};

enum MPEG2_pictureStructure : u8
{
	CELL_VDEC_MPEG2_PSTR_TOP_FIELD              = 0x01,
	CELL_VDEC_MPEG2_PSTR_BOTTOM_FIELD           = 0x02,
	CELL_VDEC_MPEG2_PSTR_FRAME                  = 0x03,
};

struct CellVdecMpeg2Info
{
	be_t<u16> horizontal_size;
	be_t<u16> vertical_size;
	union
	{
		MPEG2_aspectRatio aspect_ratio_information;
		MPEG1_aspectRatio aspect_ratio_information1;
	};
	MPEG2_frameRate frame_rate_code;
	b8 progressive_sequence;
	b8 low_delay;
	MPEG2_videoFormat video_format;
	b8 colour_description;
	MPEG2_colourPrimaries colour_primaries;
	MPEG2_transferCharacteristics transfer_characteristics;
	MPEG2_matrixCoefficients matrix_coefficients;
	be_t<u16> temporal_reference[2];
	MPEG2_pictureCodingType picture_coding_type[2];
	MPEG2_pictureStructure picture_structure[2];
	b8 top_field_first;
	b8 repeat_first_field;
	b8 progressive_frame;
	be_t<u32> time_code;
	b8 closed_gop;
	b8 broken_link;
	be_t<u16> vbv_delay[2];
	be_t<u16> display_horizontal_size;
	be_t<u16> display_vertical_size;
	u8  number_of_frame_centre_offsets[2];
	be_t<u16> frame_centre_horizontal_offset[2][3];
	be_t<u16> frame_centre_vertical_offset[2][3];
	be_t<u32> headerPresentFlags; // MPEG2_headerFlags
	be_t<u32> headerRetentionFlags; // MPEG2_headerFlags
	b8 mpeg1Flag;
	u8 ccDataLength[2];
	u8 ccData[2][128];
	be_t<u64> reserved[2];
};
