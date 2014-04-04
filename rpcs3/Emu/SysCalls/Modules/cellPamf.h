#pragma once

// Error Codes
enum
{
	CELL_PAMF_ERROR_STREAM_NOT_FOUND    = 0x80610501,
	CELL_PAMF_ERROR_INVALID_PAMF        = 0x80610502,
	CELL_PAMF_ERROR_INVALID_ARG         = 0x80610503,
	CELL_PAMF_ERROR_UNKNOWN_TYPE        = 0x80610504,
	CELL_PAMF_ERROR_UNSUPPORTED_VERSION = 0x80610505,
	CELL_PAMF_ERROR_UNKNOWN_STREAM      = 0x80610506,
	CELL_PAMF_ERROR_EP_NOT_FOUND        = 0x80610507,
};

// PamfReaderInitialize Attribute Flags
enum
{
	CELL_PAMF_ATTRIBUTE_VERIFY_ON = 1,
	CELL_PAMF_ATTRIBUTE_MINIMUM_HEADER = 2,
};

enum CellPamfStreamType
{
	CELL_PAMF_STREAM_TYPE_AVC        = 0,
	CELL_PAMF_STREAM_TYPE_M2V        = 1,
	CELL_PAMF_STREAM_TYPE_ATRAC3PLUS = 2,
	CELL_PAMF_STREAM_TYPE_PAMF_LPCM  = 3,
	CELL_PAMF_STREAM_TYPE_AC3        = 4,
	CELL_PAMF_STREAM_TYPE_USER_DATA  = 5,
	CELL_PAMF_STREAM_TYPE_VIDEO      = 20,
	CELL_PAMF_STREAM_TYPE_AUDIO      = 21,
};

enum
{
	CELL_PAMF_FS_48kHz = 1,
};

enum 
{
	CELL_PAMF_BIT_LENGTH_16 = 1,
	CELL_PAMF_BIT_LENGTH_24 = 3,
};

enum
{
	CELL_PAMF_AVC_PROFILE_MAIN = 77,
	CELL_PAMF_AVC_PROFILE_HIGH = 100,
};

enum
{
	CELL_PAMF_AVC_LEVEL_2P1 = 21,
	CELL_PAMF_AVC_LEVEL_3P0 = 30,
	CELL_PAMF_AVC_LEVEL_3P1 = 31,
	CELL_PAMF_AVC_LEVEL_3P2 = 32,
	CELL_PAMF_AVC_LEVEL_4P1 = 41,
	CELL_PAMF_AVC_LEVEL_4P2 = 42,
};

enum 
{
	CELL_PAMF_AVC_FRC_24000DIV1001 = 0,
	CELL_PAMF_AVC_FRC_24           = 1,
	CELL_PAMF_AVC_FRC_25           = 2,
	CELL_PAMF_AVC_FRC_30000DIV1001 = 3,
	CELL_PAMF_AVC_FRC_30           = 4,
	CELL_PAMF_AVC_FRC_50           = 5,
	CELL_PAMF_AVC_FRC_60000DIV1001 = 6,
};

enum
{
	CELL_PAMF_M2V_MP_ML   = 1,
	CELL_PAMF_M2V_MP_H14  = 2,
	CELL_PAMF_M2V_MP_HL   = 3,
	CELL_PAMF_M2V_UNKNOWN = 255,
};

enum
{
	CELL_PAMF_M2V_FRC_24000DIV1001 = 1,
	CELL_PAMF_M2V_FRC_24           = 2,
	CELL_PAMF_M2V_FRC_25           = 3,
	CELL_PAMF_M2V_FRC_30000DIV1001 = 4,
	CELL_PAMF_M2V_FRC_30           = 5,
	CELL_PAMF_M2V_FRC_50           = 6,
	CELL_PAMF_M2V_FRC_60000DIV1001 = 7,
};

enum
{
	CELL_PAMF_ASPECT_RATIO_1_1   = 1,
	CELL_PAMF_ASPECT_RATIO_12_11 = 2,
	CELL_PAMF_ASPECT_RATIO_10_11 = 3,
	CELL_PAMF_ASPECT_RATIO_16_11 = 4,
	CELL_PAMF_ASPECT_RATIO_40_33 = 5,
	CELL_PAMF_ASPECT_RATIO_4_3   = 14,
};

enum
{
	CELL_PAMF_COLOUR_PRIMARIES_ITR_R_BT_709        = 1,
	CELL_PAMF_COLOUR_PRIMARIES_UNSPECIFIED         = 2,
	CELL_PAMF_COLOUR_PRIMARIES_ITU_R_BT_470_SYS_M  = 4,
	CELL_PAMF_COLOUR_PRIMARIES_ITU_R_BT_470_SYS_BG = 5,
	CELL_PAMF_COLOUR_PRIMARIES_SMPTE_170_M         = 6,
	CELL_PAMF_COLOUR_PRIMARIES_SMPTE_240_M         = 7,
	CELL_PAMF_COLOUR_PRIMARIES_GENERIC_FILM        = 8,
};

enum
{
	CELL_PAMF_TRANSFER_CHARACTERISTICS_ITU_R_BT_709        = 1,
	CELL_PAMF_TRANSFER_CHARACTERISTICS_UNSPECIFIED         = 2,
	CELL_PAMF_TRANSFER_CHARACTERISTICS_ITU_R_BT_470_SYS_M  = 4,
	CELL_PAMF_TRANSFER_CHARACTERISTICS_ITU_R_BT_470_SYS_BG = 5,
	CELL_PAMF_TRANSFER_CHARACTERISTICS_SMPTE_170_M         = 6,
	CELL_PAMF_TRANSFER_CHARACTERISTICS_SMPTE_240_M         = 7,
	CELL_PAMF_TRANSFER_CHARACTERISTICS_LINEAR              = 8,
	CELL_PAMF_TRANSFER_CHARACTERISTICS_LOG_100_1           = 9,
	CELL_PAMF_TRANSFER_CHARACTERISTICS_LOG_316_1           = 10,
};

enum
{
	CELL_PAMF_MATRIX_GBR                 = 0,
	CELL_PAMF_MATRIX_ITU_R_BT_709        = 1,
	CELL_PAMF_MATRIX_UNSPECIFIED         = 2,
	CELL_PAMF_MATRIX_FCC                 = 4,
	CELL_PAMF_MATRIX_ITU_R_BT_470_SYS_BG = 5,
	CELL_PAMF_MATRIX_SMPTE_170_M         = 6,
	CELL_PAMF_MATRIX_SMPTE_240_M         = 7,
	CELL_PAMF_MATRIX_YCGCO               = 8,
};

// Timestamp information (time in increments of 90 kHz)
struct CellCodecTimeStamp
{
	be_t<u32> upper;
	be_t<u32> lower;
};

// Entry point information
struct CellPamfEp
{
	be_t<u32> indexN;
	be_t<u32> nThRefPictureOffset;
	CellCodecTimeStamp pts;
	be_t<u64> rpnOffset;
};

// Entry point iterator
struct CellPamfEpIterator
{
	bool isPamf;
	be_t<u32> index;
	be_t<u32> num;
	be_t<u32> pCur_addr;
};

struct CellCodecEsFilterId
{
	be_t<u32> filterIdMajor;
	be_t<u32> filterIdMinor;
	be_t<u32> supplementalInfo1;
	be_t<u32> supplementalInfo2;
};

// AVC (MPEG4 AVC Video) Specific Information
struct CellPamfAvcInfo {
	u8 profileIdc;
	u8 levelIdc;
	u8 frameMbsOnlyFlag;
	u8 videoSignalInfoFlag;
	u8 frameRateInfo;
	u8 aspectRatioIdc;
	be_t<u16> sarWidth; //reserved
	be_t<u16> sarHeight; //reserved
	be_t<u16> horizontalSize;
	be_t<u16> verticalSize;
	be_t<u16> frameCropLeftOffset; //reserved
	be_t<u16> frameCropRightOffset; //reserved
	be_t<u16> frameCropTopOffset; //reserved
	be_t<u16> frameCropBottomOffset; //!!!!!
	u8 videoFormat; //reserved
	u8 videoFullRangeFlag;
	u8 colourPrimaries;
	u8 transferCharacteristics;
	u8 matrixCoefficients;
	u8 entropyCodingModeFlag; //reserved
	u8 deblockingFilterFlag;
	u8 minNumSlicePerPictureIdc; //reserved
	u8 nfwIdc; //reserved
	u8 maxMeanBitrate; //reserved
};

// M2V (MPEG2 Video) Specific Information
struct CellPamfM2vInfo {
	u8 profileAndLevelIndication;
	bool progressiveSequence;
	u8 videoSignalInfoFlag;
	u8 frameRateInfo;
	u8 aspectRatioIdc;
	be_t<u16> sarWidth;
	be_t<u16> sarHeight;
	be_t<u16> horizontalSize;
	be_t<u16> verticalSize;
	be_t<u16> horizontalSizeValue;
	be_t<u16> verticalSizeValue;
	u8 videoFormat;
	u8 videoFullRangeFlag;
	u8 colourPrimaries;
	u8 transferCharacteristics;
	u8 matrixCoefficients;
};

// LPCM Audio Specific Information
struct CellPamfLpcmInfo {
	be_t<u32> samplingFrequency;
	u8 numberOfChannels;
	be_t<u16> bitsPerSample;
};

// ATRAC3+ Audio Specific Information
struct CellPamfAtrac3plusInfo {
	be_t<u32> samplingFrequency;
	u8 numberOfChannels;
};

// AC3 Audio Specific Information
struct CellPamfAc3Info {
	be_t<u32> samplingFrequency;
	u8 numberOfChannels;
};

#pragma pack(push, 1) //file data

struct PamfStreamHeader_AVC { //AVC specific information
	u8 profileIdc;
	u8 levelIdc;
	u8 unk0;
	u8 unk1; //1
	u32 unk2; //0
	be_t<u16> horizontalSize; //divided by 16
	be_t<u16> verticalSize; //divided by 16
	u32 unk3; //0
	u32 unk4; //0
	u8 unk5; //0xA0
	u8 unk6; //1
	u8 unk7; //1
	u8 unk8; //1
	u8 unk9; //0xB0
	u8 unk10;
	u16 unk11; //0
	u32 unk12; //0
};

struct PamfStreamHeader_M2V { //M2V specific information
	u8 unknown[32]; //no information yet
};

struct PamfStreamHeader_Audio { //Audio specific information
	u16 unknown; //== 0
	u8 channels; //number of channels (1, 2, 6 or 8)
	u8 freq; //== 1 (always 48000)
	u8 bps; //(LPCM only, 0x40 for 16 bit, ???? for 24)
	u8 reserved[27]; //probably nothing
};

struct PamfStreamHeader //48 bytes
{
	//TODO: look for correct beginning of stream header
	u8 type; //0x1B for video (AVC), 0xDC ATRAC3+, 0x80 LPCM, 0xDD userdata
	u8 unknown[3]; //0
	//TODO: examine stream_ch encoding
	u8 stream_id;
	u8 private_stream_id;
	u8 unknown1; //?????
	u8 unknown2; //?????
	//Entry Point Info
	be_t<u32> ep_offset; //offset of EP section in header
	be_t<u32> ep_num; //count of EPs
	//Specific Info
	u8 data[32];
};

struct PamfHeader
{
	u32 magic; //"PAMF"
	u32 version; //"0041" (is it const?)
	be_t<u32> data_offset; //== 2048 >> 11, PAMF headers seem to be always 2048 bytes in size
	be_t<u32> data_size; //== ((fileSize - 2048) >> 11)
	u64 reserved[8];
	be_t<u32> table_size; //== size of mapping-table
	u16 reserved1;
	be_t<u16> start_pts_high;
	be_t<u32> start_pts_low; //Presentation Time Stamp (start)
	be_t<u16> end_pts_high;
	be_t<u32> end_pts_low; //Presentation Time Stamp (end)
	be_t<u32> mux_rate_max; //== 0x01D470 (400 bps per unit, == 48000000 bps)
	be_t<u32> mux_rate_min; //== 0x0107AC (?????)
	u16 reserved2; // ?????
	u8 reserved3;
	u8 stream_count; //total stream count (reduced to 1 byte)
	be_t<u16> unk1; //== 1 (?????)
	be_t<u32> table_data_size; //== table_size - 0x20 == 0x14 + (0x30 * total_stream_num) (?????)
	//TODO: check relative offset of stream structs (could be from 0x0c to 0x14, currently 0x14)
	be_t<u16> start_pts_high2; //????? (probably same values)
	be_t<u32> start_pts_low2; //?????
	be_t<u16> end_pts_high2; //?????
	be_t<u32> end_pts_low2; //?????
	be_t<u32> unk2; //== 0x10000 (?????)
	be_t<u16> unk3; // ?????
	be_t<u16> unk4; // == stream_count
	//==========================
	PamfStreamHeader stream_headers[256];
};

struct PamfEpHeader { //12 bytes
	be_t<u16> value0; //mixed indexN (probably left 2 bits) and nThRefPictureOffset
	be_t<u16> pts_high;
	be_t<u32> pts_low;
	be_t<u32> rpnOffset;
};

#pragma pack(pop)

struct CellPamfReader
{
	//this struct can be used in any way, if it is not accessed directly by virtual CPU
	//be_t<u64> internalData[16];
	u32 pAddr;
	int stream;
	u64 fileSize;
	u32 internalData[28];
};