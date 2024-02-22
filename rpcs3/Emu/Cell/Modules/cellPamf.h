#pragma once

#include "Emu/Memory/vm_ptr.h"

// Error Codes
enum CellPamfError : u32
{
	CELL_PAMF_ERROR_STREAM_NOT_FOUND    = 0x80610501,
	CELL_PAMF_ERROR_INVALID_PAMF        = 0x80610502,
	CELL_PAMF_ERROR_INVALID_ARG         = 0x80610503,
	CELL_PAMF_ERROR_UNKNOWN_TYPE        = 0x80610504,
	CELL_PAMF_ERROR_UNSUPPORTED_VERSION = 0x80610505,
	CELL_PAMF_ERROR_UNKNOWN_STREAM      = 0x80610506,
	CELL_PAMF_ERROR_EP_NOT_FOUND        = 0x80610507,
	CELL_PAMF_ERROR_NOT_AVAILABLE       = 0x80610508,
};

// PamfReaderInitialize Attribute Flags
enum
{
	CELL_PAMF_ATTRIBUTE_VERIFY_ON = 1,
	CELL_PAMF_ATTRIBUTE_MINIMUM_HEADER = 2,
};

enum CellPamfStreamType
{
	CELL_PAMF_STREAM_TYPE_AVC             = 0,
	CELL_PAMF_STREAM_TYPE_M2V             = 1,
	CELL_PAMF_STREAM_TYPE_ATRAC3PLUS      = 2,
	CELL_PAMF_STREAM_TYPE_PAMF_LPCM       = 3,
	CELL_PAMF_STREAM_TYPE_AC3             = 4,
	CELL_PAMF_STREAM_TYPE_USER_DATA       = 5,
	CELL_PAMF_STREAM_TYPE_PSMF_AVC        = 6,
	CELL_PAMF_STREAM_TYPE_PSMF_ATRAC3PLUS = 7,
	CELL_PAMF_STREAM_TYPE_PSMF_LPCM       = 8,
	CELL_PAMF_STREAM_TYPE_PSMF_USER_DATA  = 9,
	CELL_PAMF_STREAM_TYPE_VIDEO           = 20,
	CELL_PAMF_STREAM_TYPE_AUDIO           = 21,
	CELL_PAMF_STREAM_TYPE_UNK             = 22,
};

enum PamfStreamCodingType : u8
{
	PAMF_STREAM_CODING_TYPE_M2V        = 0x02,
	PAMF_STREAM_CODING_TYPE_AVC        = 0x1b,
	PAMF_STREAM_CODING_TYPE_PAMF_LPCM  = 0x80,
	PAMF_STREAM_CODING_TYPE_AC3        = 0x81,
	PAMF_STREAM_CODING_TYPE_ATRAC3PLUS = 0xdc,
	PAMF_STREAM_CODING_TYPE_USER_DATA  = 0xdd,
	PAMF_STREAM_CODING_TYPE_PSMF       = 0xff,
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

constexpr u32 CODEC_TS_INVALID = umax;

// Entry point information
struct CellPamfEp
{
	be_t<u32> indexN;
	be_t<u32> nThRefPictureOffset;
	CellCodecTimeStamp pts;
	be_t<u64> rpnOffset;
};

struct CellPamfEpUnk // Speculative name, only used in two undocumented functions
{
	CellPamfEp ep;
	be_t<u64> nextRpnOffset;
};

CHECK_SIZE(CellPamfEpUnk, 0x20);

// Entry point iterator
struct CellPamfEpIterator
{
	b8 isPamf;
	be_t<u32> index;
	be_t<u32> num;
	vm::bcptr<void> pCur;
};

struct CellCodecEsFilterId
{
	be_t<u32> filterIdMajor;
	be_t<u32> filterIdMinor;
	be_t<u32> supplementalInfo1;
	be_t<u32> supplementalInfo2;
};

// AVC (MPEG4 AVC Video) Specific Information
struct CellPamfAvcInfo
{
	u8 profileIdc;
	u8 levelIdc;
	u8 frameMbsOnlyFlag;
	u8 videoSignalInfoFlag;
	u8 frameRateInfo;
	u8 aspectRatioIdc;
	be_t<u16> sarWidth;
	be_t<u16> sarHeight;
	be_t<u16> horizontalSize;
	be_t<u16> verticalSize;
	be_t<u16> frameCropLeftOffset;
	be_t<u16> frameCropRightOffset;
	be_t<u16> frameCropTopOffset;
	be_t<u16> frameCropBottomOffset;
	u8 videoFormat;
	u8 videoFullRangeFlag;
	u8 colourPrimaries;
	u8 transferCharacteristics;
	u8 matrixCoefficients;
	u8 entropyCodingModeFlag;
	u8 deblockingFilterFlag;
	u8 minNumSlicePerPictureIdc;
	u8 nfwIdc;
	u8 maxMeanBitrate;
};

CHECK_SIZE(CellPamfAvcInfo, 0x20);

// M2V (MPEG2 Video) Specific Information
struct CellPamfM2vInfo
{
	u8 profileAndLevelIndication;
	u8 progressiveSequence;
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

CHECK_SIZE(CellPamfM2vInfo, 0x18);

// ATRAC3+ Audio Specific Information
struct CellPamfAtrac3plusInfo
{
	be_t<u32> samplingFrequency;
	u8 numberOfChannels;
};

CHECK_SIZE(CellPamfAtrac3plusInfo, 8);

// AC3 Audio Specific Information
struct CellPamfAc3Info
{
	be_t<u32> samplingFrequency;
	u8 numberOfChannels;
};

CHECK_SIZE(CellPamfAc3Info, 8);

// LPCM Audio Specific Information
struct CellPamfLpcmInfo
{
	be_t<u32> samplingFrequency;
	u8 numberOfChannels;
	be_t<u16> bitsPerSample;
};

CHECK_SIZE(CellPamfLpcmInfo, 8);

// PAMF file structs, everything here is not aligned (LLE uses exclusively u8 pointers)

struct PamfStreamHeader
{
	u8 stream_coding_type;

	u8 reserved[3];

	u8 stream_id;
	u8 private_stream_id; // for streams multiplexed as private data streams (stream_id == 0xbd)

	be_t<u16, 1> p_std_buffer; // 2 bits: unused ??? "00", 1 bit: P_STD_buffer_scale, 13 bits: P_STD_buffer_size

	be_t<u32, 1> ep_offset; // offset of EP section in header
	be_t<u32, 1> ep_num; // count of EPs

	union
	{
		u8 data[32]; // specific info

		// AVC specific information
		struct
		{
			u8 profileIdc;
			u8 levelIdc;
			u8 x2; // contains frameMbsOnlyFlag, videoSignalInfoFlag, frameRateInfo
			u8 aspectRatioIdc;
			be_t<u16, 1> sarWidth;
			be_t<u16, 1> sarHeight;
			u8 reserved1;
			u8 horizontalSize; // divided by 16
			u8 reserved2;
			u8 verticalSize; // divided by 16
			be_t<u16, 1> frameCropLeftOffset;
			be_t<u16, 1> frameCropRightOffset;
			be_t<u16, 1> frameCropTopOffset;
			be_t<u16, 1> frameCropBottomOffset;
			u8 x14; // contains videoFormat and videoFullRangeFlag
			u8 colourPrimaries;
			u8 transferCharacteristics;
			u8 matrixCoefficients;
			u8 x18; // contains entropyCodingModeFlag, deblockingFilterFlag, minNumSlicePerPictureIdc, nfwIdc
			u8 maxMeanBitrate;
		}
		AVC;

		// M2V specific information
		struct
		{
			s8 x0; // contains profileAndLevelIndication
			u8 x1; // not used
			u8 x2; // contains progressiveSequence, videoSignalInfoFlag, frameRateInfo
			u8 aspectRatioIdc;
			be_t<u16, 1> sarWidth;
			be_t<u16, 1> sarHeight;
			u8 reserved1;
			u8 horizontalSize; // in units of 16 pixels
			u8 reserved2;
			u8 verticalSize; // in units of 16 pixels
			be_t<u16, 1> horizontalSizeValue;
			be_t<u16, 1> verticalSizeValue;
			be_t<u32, 1> x10; // not used
			u8 x14; // contains videoFormat and videoFullRangeFlag
			u8 colourPrimaries;
			u8 transferCharacteristics;
			u8 matrixCoefficients;
		}
		M2V;

		// Audio specific information
		struct
		{
			be_t<u16, 1> unknown; // 0
			u8 channels; // number of channels (1, 2, 6, 8)
			u8 freq; // 1 (always 48000)
			u8 bps; // LPCM only
		}
		audio;
	};
};

CHECK_SIZE_ALIGN(PamfStreamHeader, 48, 1);

struct PamfGroup
{
	be_t<u32, 1> size; // doesn't include this field

	u8 reserved;

	u8 stream_num; // same value as in PamfSequenceInfo
	PamfStreamHeader streams;
};

CHECK_SIZE_ALIGN(PamfGroup, 6 + sizeof(PamfStreamHeader), 1);

struct PamfGroupingPeriod
{
	be_t<u32, 1> size; // doesn't include this field

	be_t<u16, 1> start_pts_high; // always 0, greatest valid pts is UINT32_MAX
	be_t<u32, 1> start_pts_low;  // same value as in PamfSequenceInfo, since there is only one PamfGroupingPeriod
	be_t<u16, 1> end_pts_high;   // unused due to bug
	be_t<u32, 1> end_pts_low;    // same value as in PamfSequenceInfo, since there is only one PamfGroupingPeriod

	u8 reserved;

	u8 group_num; // always 1
	PamfGroup groups;
};

CHECK_SIZE_ALIGN(PamfGroupingPeriod, 0x12 + sizeof(PamfGroup), 1);

struct PamfSequenceInfo
{
	be_t<u32, 1> size; // doesn't include this field

	be_t<u16, 1> reserved1;

	be_t<u16, 1> start_pts_high; // always 0, greatest valid pts is UINT32_MAX
	be_t<u32, 1> start_pts_low;  // Presentation Time Stamp (start)
	be_t<u16, 1> end_pts_high;   // always 0, greatest valid pts is UINT32_MAX
	be_t<u32, 1> end_pts_low;    // Presentation Time Stamp (end)

	be_t<u32, 1> mux_rate_bound;  // multiplex bitrate in units of 50 bytes per second
	be_t<u32, 1> std_delay_bound; // buffer delay in units of 1/90000 seconds

	be_t<u32, 1> total_stream_num; // across all groups; since there is always only one group, this is equal stream_count in PamfGroup

	u8 reserved2;

	u8 grouping_period_num; // always 1
	PamfGroupingPeriod grouping_periods;
};

CHECK_SIZE_ALIGN(PamfSequenceInfo, 0x20 + sizeof(PamfGroupingPeriod), 1);

struct PamfHeader
{
	be_t<u32, 1> magic;       // "PAMF"
	be_t<u32, 1> version;     // "0040" or "0041"
	be_t<u32, 1> header_size; // in units of 2048 bytes
	be_t<u32, 1> data_size;   // in units of 2048 bytes

	be_t<u32, 1> psmf_marks_offset; // always 0
	be_t<u32, 1> psmf_marks_size;   // always 0
	be_t<u32, 1> unk_offset;        // always 0
	be_t<u32, 1> unk_size;          // always 0

	u8 reserved[0x30];

	PamfSequenceInfo seq_info;
};

CHECK_SIZE_ALIGN(PamfHeader, 0x50 + sizeof(PamfSequenceInfo), 1);

struct PamfEpHeader
{
	be_t<u16, 1> value0;    // 2 bits: indexN, 1 bit: unused, 13 bits: nThRefPictureOffset in units of 2048 bytes
	be_t<u16, 1> pts_high;  // always 0, greatest valid pts is UINT32_MAX
	be_t<u32, 1> pts_low;
	be_t<u32, 1> rpnOffset; // in units of 2048 bytes
};

CHECK_SIZE_ALIGN(PamfEpHeader, 12, 1);

// PSMF specific

struct PsmfStreamHeader
{
	u8 stream_id;
	u8 private_stream_id; // for streams multiplexed as private data streams (stream_id == 0xbd)

	be_t<u16, 1> p_std_buffer; // 2 bits: unused ??? "00", 1 bit: P_STD_buffer_scale, 13 bits: P_STD_buffer_size

	be_t<u32, 1> ep_offset; // offset of EP section in header
	be_t<u32, 1> ep_num; // count of EPs

	union
	{
		// Video specific information
		struct
		{
			u8 horizontalSize; // in units of 16 pixels
			u8 verticalSize;   // in units of 16 pixels
		}
		video;

		// Audio specific information
		struct
		{
			be_t<u16, 1> unknown;    // 0
			u8 channelConfiguration; // 1 = mono, 2 = stereo
			u8 samplingFrequency;    // 2 = 44.1kHz
		}
		audio;
	};
};

CHECK_SIZE_ALIGN(PsmfStreamHeader, 0x10, 1);

struct PsmfGroup
{
	be_t<u32, 1> size; // doesn't include this field

	u8 reserved;

	u8 stream_num; // same value as in PsmfSequenceInfo
	PsmfStreamHeader streams;
};

CHECK_SIZE_ALIGN(PsmfGroup, 6 + sizeof(PsmfStreamHeader), 1);

struct PsmfGroupingPeriod
{
	be_t<u32, 1> size; // doesn't include this field

	be_t<u16, 1> start_pts_high; // always 0, greatest valid pts is UINT32_MAX
	be_t<u32, 1> start_pts_low;  // same value as in PsmfSequenceInfo, since there is only one PsmfGroupingPeriod
	be_t<u16, 1> end_pts_high;   // unused due to bug
	be_t<u32, 1> end_pts_low;    // same value as in PsmfSequenceInfo, since there is only one PsmfGroupingPeriod

	u8 reserved;

	u8 group_num; // always 1
	PsmfGroup groups;
};

CHECK_SIZE_ALIGN(PsmfGroupingPeriod, 0x12 + sizeof(PsmfGroup), 1);

struct PsmfSequenceInfo
{
	be_t<u32, 1> size; // doesn't include this field

	be_t<u16, 1> start_pts_high; // always 0, greatest valid pts is UINT32_MAX
	be_t<u32, 1> start_pts_low;  // Presentation Time Stamp (start)
	be_t<u16, 1> end_pts_high;   // always 0, greatest valid pts is UINT32_MAX
	be_t<u32, 1> end_pts_low;    // Presentation Time Stamp (end)

	be_t<u32, 1> mux_rate_bound;  // multiplex bitrate in units of 50 bytes per second
	be_t<u32, 1> std_delay_bound; // buffer delay in units of 1/90000 seconds

	u8 total_stream_num; // across all groups; since there is always only one group, this is equal stream_count in PsmfGroup

	u8 grouping_period_num; // always 1
	PsmfGroupingPeriod grouping_periods;
};

CHECK_SIZE_ALIGN(PsmfSequenceInfo, 0x1a + sizeof(PsmfGroupingPeriod), 1);

struct PsmfHeader
{
	be_t<u32, 1> magic;       // "PSMF"
	be_t<u32, 1> version;     // "0012", "0013", "0014" or "0015"
	be_t<u32, 1> header_size; // not scaled, unlike PAMF
	be_t<u32, 1> data_size;   // not scaled, unlike PAMF

	be_t<u32, 1> psmf_marks_offset;
	be_t<u32, 1> psmf_marks_size;
	be_t<u32, 1> unk[2];

	u8 reserved[0x30];

	PsmfSequenceInfo seq_info;
};

CHECK_SIZE_ALIGN(PsmfHeader, 0x50 + sizeof(PsmfSequenceInfo), 1);

struct PsmfEpHeader
{
	be_t<u16, 1> value0;    // 2 bits: indexN, 2 bits: unused, 11 bits: nThRefPictureOffset in units of 1024 bytes, 1 bit: pts_high
	be_t<u32, 1> pts_low;
	be_t<u32, 1> rpnOffset; // in units of 2048 bytes
};

CHECK_SIZE_ALIGN(PsmfEpHeader, 10, 1);

struct CellPamfReader
{
	be_t<u64> headerSize;
	be_t<u64> dataSize;
	be_t<u32> attribute;
	be_t<u16> isPsmf;
	be_t<u16> version;
	be_t<u32> currentGroupingPeriodIndex;
	be_t<u32> currentGroupIndex;
	be_t<u32> currentStreamIndex;

	union
	{
		struct
		{
			vm::bcptr<PamfHeader> header;
			vm::bcptr<PamfSequenceInfo> sequenceInfo;
			vm::bcptr<PamfGroupingPeriod> currentGroupingPeriod;
			vm::bcptr<PamfGroup> currentGroup;
			vm::bcptr<PamfStreamHeader> currentStream;
		}
		pamf;

		struct
		{
			vm::bcptr<PsmfHeader> header;
			vm::bcptr<PsmfSequenceInfo> sequenceInfo;
			vm::bcptr<PsmfGroupingPeriod> currentGroupingPeriod;
			vm::bcptr<PsmfGroup> currentGroup;
			vm::bcptr<PsmfStreamHeader> currentStream;
		}
		psmf;
	};

	u32 reserved[18];
};

CHECK_SIZE(CellPamfReader, 128);

error_code cellPamfReaderInitialize(vm::ptr<CellPamfReader> pSelf, vm::cptr<PamfHeader> pAddr, u64 fileSize, u32 attribute);

#include <mutex>
#include <condition_variable>

extern const std::function<bool()> SQUEUE_ALWAYS_EXIT;
extern const std::function<bool()> SQUEUE_NEVER_EXIT;

bool squeue_test_exit();

// TODO: eliminate this boolshit
template<typename T, u32 sq_size = 256>
class squeue_t
{
	struct squeue_sync_var_t
	{
		struct
		{
			u32 position : 31;
			u32 pop_lock : 1;
		};
		struct
		{
			u32 count : 31;
			u32 push_lock : 1;
		};
	};

	atomic_t<squeue_sync_var_t> m_sync;

	mutable std::mutex m_rcv_mutex;
	mutable std::mutex m_wcv_mutex;
	mutable std::condition_variable m_rcv;
	mutable std::condition_variable m_wcv;

	T m_data[sq_size];

	enum squeue_sync_var_result : u32
	{
		SQSVR_OK = 0,
		SQSVR_LOCKED = 1,
		SQSVR_FAILED = 2,
	};

public:
	squeue_t()
		: m_sync(squeue_sync_var_t{})
	{
	}

	static u32 get_max_size()
	{
		return sq_size;
	}

	bool is_full() const
	{
		return m_sync.load().count == sq_size;
	}

	bool push(const T& data, const std::function<bool()>& test_exit)
	{
		u32 pos = 0;

		while (u32 res = m_sync.atomic_op([&pos](squeue_sync_var_t& sync) -> u32
		{
			ensure(sync.count <= sq_size);
			ensure(sync.position < sq_size);

			if (sync.push_lock)
			{
				return SQSVR_LOCKED;
			}
			if (sync.count == sq_size)
			{
				return SQSVR_FAILED;
			}

			sync.push_lock = 1;
			pos = sync.position + sync.count;
			return SQSVR_OK;
		}))
		{
			if (res == SQSVR_FAILED && (test_exit() || squeue_test_exit()))
			{
				return false;
			}

			std::unique_lock<std::mutex> wcv_lock(m_wcv_mutex);
			m_wcv.wait_for(wcv_lock, std::chrono::milliseconds(1));
		}

		m_data[pos >= sq_size ? pos - sq_size : pos] = data;

		m_sync.atomic_op([](squeue_sync_var_t& sync)
		{
			ensure(sync.count <= sq_size);
			ensure(sync.position < sq_size);
			ensure(!!sync.push_lock);
			sync.push_lock = 0;
			sync.count++;
		});

		m_rcv.notify_one();
		m_wcv.notify_one();
		return true;
	}

	bool push(const T& data, const volatile bool* do_exit)
	{
		return push(data, [do_exit]() { return do_exit && *do_exit; });
	}

	bool push(const T& data)
	{
		return push(data, SQUEUE_NEVER_EXIT);
	}

	bool try_push(const T& data)
	{
		return push(data, SQUEUE_ALWAYS_EXIT);
	}

	bool pop(T& data, const std::function<bool()>& test_exit)
	{
		u32 pos = 0;

		while (u32 res = m_sync.atomic_op([&pos](squeue_sync_var_t& sync) -> u32
		{
			ensure(sync.count <= sq_size);
			ensure(sync.position < sq_size);

			if (!sync.count)
			{
				return SQSVR_FAILED;
			}
			if (sync.pop_lock)
			{
				return SQSVR_LOCKED;
			}

			sync.pop_lock = 1;
			pos = sync.position;
			return SQSVR_OK;
		}))
		{
			if (res == SQSVR_FAILED && (test_exit() || squeue_test_exit()))
			{
				return false;
			}

			std::unique_lock<std::mutex> rcv_lock(m_rcv_mutex);
			m_rcv.wait_for(rcv_lock, std::chrono::milliseconds(1));
		}

		data = m_data[pos];

		m_sync.atomic_op([](squeue_sync_var_t& sync)
		{
			ensure(sync.count <= sq_size);
			ensure(sync.position < sq_size);
			ensure(!!sync.pop_lock);
			sync.pop_lock = 0;
			sync.position++;
			sync.count--;
			if (sync.position == sq_size)
			{
				sync.position = 0;
			}
		});

		m_rcv.notify_one();
		m_wcv.notify_one();
		return true;
	}

	bool pop(T& data, const volatile bool* do_exit)
	{
		return pop(data, [do_exit]() { return do_exit && *do_exit; });
	}

	bool pop(T& data)
	{
		return pop(data, SQUEUE_NEVER_EXIT);
	}

	bool try_pop(T& data)
	{
		return pop(data, SQUEUE_ALWAYS_EXIT);
	}

	bool peek(T& data, u32 start_pos, const std::function<bool()>& test_exit)
	{
		ensure(start_pos < sq_size);
		u32 pos = 0;

		while (u32 res = m_sync.atomic_op([&pos, start_pos](squeue_sync_var_t& sync) -> u32
		{
			ensure(sync.count <= sq_size);
			ensure(sync.position < sq_size);

			if (sync.count <= start_pos)
			{
				return SQSVR_FAILED;
			}
			if (sync.pop_lock)
			{
				return SQSVR_LOCKED;
			}

			sync.pop_lock = 1;
			pos = sync.position + start_pos;
			return SQSVR_OK;
		}))
		{
			if (res == SQSVR_FAILED && (test_exit() || squeue_test_exit()))
			{
				return false;
			}

			std::unique_lock<std::mutex> rcv_lock(m_rcv_mutex);
			m_rcv.wait_for(rcv_lock, std::chrono::milliseconds(1));
		}

		data = m_data[pos >= sq_size ? pos - sq_size : pos];

		m_sync.atomic_op([](squeue_sync_var_t& sync)
		{
			ensure(sync.count <= sq_size);
			ensure(sync.position < sq_size);
			ensure(!!sync.pop_lock);
			sync.pop_lock = 0;
		});

		m_rcv.notify_one();
		return true;
	}

	bool peek(T& data, u32 start_pos, const volatile bool* do_exit)
	{
		return peek(data, start_pos, [do_exit]() { return do_exit && *do_exit; });
	}

	bool peek(T& data, u32 start_pos = 0)
	{
		return peek(data, start_pos, SQUEUE_NEVER_EXIT);
	}

	bool try_peek(T& data, u32 start_pos = 0)
	{
		return peek(data, start_pos, SQUEUE_ALWAYS_EXIT);
	}

	class squeue_data_t
	{
		T* const m_data;
		const u32 m_pos;
		const u32 m_count;

		squeue_data_t(T* data, u32 pos, u32 count)
			: m_data(data)
			, m_pos(pos)
			, m_count(count)
		{
		}

	public:
		T& operator [] (u32 index)
		{
			ensure(index < m_count);
			index += m_pos;
			index = index < sq_size ? index : index - sq_size;
			return m_data[index];
		}
	};

	void process(void(*proc)(squeue_data_t data))
	{
		u32 pos, count;

		while (m_sync.atomic_op([&pos, &count](squeue_sync_var_t& sync) -> u32
		{
			ensure(sync.count <= sq_size);
			ensure(sync.position < sq_size);

			if (sync.pop_lock || sync.push_lock)
			{
				return SQSVR_LOCKED;
			}

			pos = sync.position;
			count = sync.count;
			sync.pop_lock = 1;
			sync.push_lock = 1;
			return SQSVR_OK;
		}))
		{
			std::unique_lock<std::mutex> rcv_lock(m_rcv_mutex);
			m_rcv.wait_for(rcv_lock, std::chrono::milliseconds(1));
		}

		proc(squeue_data_t(m_data, pos, count));

		m_sync.atomic_op([](squeue_sync_var_t& sync)
		{
			ensure(sync.count <= sq_size);
			ensure(sync.position < sq_size);
			ensure(!!sync.pop_lock);
			ensure(!!sync.push_lock);
			sync.pop_lock = 0;
			sync.push_lock = 0;
		});

		m_wcv.notify_one();
		m_rcv.notify_one();
	}

	void clear()
	{
		while (m_sync.atomic_op([](squeue_sync_var_t& sync) -> u32
		{
			ensure(sync.count <= sq_size);
			ensure(sync.position < sq_size);

			if (sync.pop_lock || sync.push_lock)
			{
				return SQSVR_LOCKED;
			}

			sync.pop_lock = 1;
			sync.push_lock = 1;
			return SQSVR_OK;
		}))
		{
			std::unique_lock<std::mutex> rcv_lock(m_rcv_mutex);
			m_rcv.wait_for(rcv_lock, std::chrono::milliseconds(1));
		}

		m_sync.exchange({});
		m_wcv.notify_one();
		m_rcv.notify_one();
	}
};
