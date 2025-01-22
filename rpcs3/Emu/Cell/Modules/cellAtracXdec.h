#pragma once

#ifdef _MSC_VER
#pragma warning(push, 0)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
extern "C"
{
#include "libavcodec/avcodec.h"
}

constexpr int averror_eof = AVERROR_EOF; // Workaround for old-style-cast error
#ifdef _MSC_VER
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif

#include "cellPamf.h"
#include "cellAdec.h"

enum CellAtracXdecError : u32
{
	CELL_ADEC_ERROR_ATX_OFFSET                     = 0x80612200,
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
};

enum : u32
{
	CELL_ADEC_ATRACX_WORD_SZ_16BIT = 0x02,
	CELL_ADEC_ATRACX_WORD_SZ_24BIT = 0x03,
	CELL_ADEC_ATRACX_WORD_SZ_32BIT = 0x04,
	CELL_ADEC_ATRACX_WORD_SZ_FLOAT = 0x84,
};

enum : u8
{
	CELL_ADEC_ATRACX_ATS_HDR_NOTINC = 0,
	CELL_ADEC_ATRACX_ATS_HDR_INC = 1,
};

enum : u8
{
	ATRACX_DOWNMIX_OFF = 0,
	ATRACX_DOWNMIX_ON = 1,
};

struct CellAdecParamAtracX
{
	be_t<u32> sampling_freq;
	be_t<u32> ch_config_idx;
	be_t<u32> nch_out;
	be_t<u32> nbytes;
	std::array<u8, 4> extra_config_data;
	be_t<u32> bw_pcm;
	u8 downmix_flag;
	u8 au_includes_ats_hdr_flg;
};

struct CellAdecAtracXInfo
{
	be_t<u32> samplingFreq;
	be_t<u32> channelConfigIndex;
	be_t<u32> nbytes;
};

CHECK_SIZE(CellAdecAtracXInfo, 12);

struct AtracXdecAtsHeader
{
	be_t<u16> sync_word; // 0x0fd0
	be_t<u16> params;    // 3 bits: sample rate, 3 bits: channel config, 10 bits: (nbytes / 8) - 1
	u8 extra_config_data[4];
};

CHECK_SIZE(AtracXdecAtsHeader, 8);

enum class AtracXdecCmdType : u32
{
	invalid,
	start_seq,
	decode_au,
	end_seq,
	close,
};

struct AtracXdecCmd
{
	be_t<AtracXdecCmdType> type;
	be_t<s32> pcm_handle;
	vm::bcptr<u8> au_start_addr;
	be_t<u32> au_size;
	vm::bptr<void> pcm_start_addr; // Unused
	be_t<u32> pcm_size;            // Unused
	CellAdecParamAtracX atracx_param;

	AtracXdecCmd() = default; // cellAdecOpen()

	AtracXdecCmd(AtracXdecCmdType&& type) // cellAdecEndSeq(), cellAdecClose()
		: type(type)
	{
	}

	AtracXdecCmd(AtracXdecCmdType&& type, const CellAdecParamAtracX& atracx_param) // cellAdecStartSeq()
		: type(type), atracx_param(atracx_param)
	{
	}

	AtracXdecCmd(AtracXdecCmdType&& type, const s32& pcm_handle, const CellAdecAuInfo& au_info) // cellAdecDecodeAu()
		: type(type), pcm_handle(pcm_handle), au_start_addr(au_info.startAddr), au_size(au_info.size)
	{
	}
};

CHECK_SIZE(AtracXdecCmd, 0x34);

struct AtracXdecDecoder
{
	be_t<u32> sampling_freq;
	be_t<u32> ch_config_idx;
	be_t<u32> nch_in;
	be_t<u32> nch_blocks;
	be_t<u32> nbytes;
	be_t<u32> nch_out;
	be_t<u32> bw_pcm;
	be_t<u32> nbytes_128_aligned;
	be_t<u32> status;
	be_t<u32> pcm_output_size;

	const vm::bptr<u8> work_mem;

	// HLE exclusive
	b8 config_is_set = false; // For savestates
	const AVCodec* codec;
	AVCodecContext* ctx;
	AVPacket* packet;
	AVFrame* frame;

	u8 spurs_stuff[84]; // 120 bytes on LLE, pointers to CellSpurs, CellSpursTaskset, etc.

	be_t<u32> spurs_task_id;  // CellSpursTaskId

	AtracXdecDecoder(vm::ptr<u8> work_mem) : work_mem(work_mem) {}

	void alloc_avcodec();
	void free_avcodec();
	void init_avcodec();

	error_code set_config_info(u32 sampling_freq, u32 ch_config_idx, u32 nbytes);
	error_code init_decode(u32 bw_pcm, u32 nch_out);
	error_code parse_ats_header(vm::cptr<u8> au_start_addr);
};

CHECK_SIZE(AtracXdecDecoder, 0xa8);

// HLE exclusive, for savestates
enum class atracxdec_state : u8
{
	initial,
	waiting_for_cmd,
	checking_run_thread_1,
	executing_cmd,
	waiting_for_output,
	checking_run_thread_2,
	decoding
};

struct AtracXdecContext
{
	be_t<u64> thread_id; // sys_ppu_thread_t

	be_t<u32> queue_mutex;     // sys_mutex_t
	be_t<u32> queue_not_empty; // sys_cond_t
	AdecCmdQueue<AtracXdecCmd> cmd_queue;

	be_t<u32> output_mutex;    // sys_mutex_t
	be_t<u32> output_consumed; // sys_cond_t
	be_t<u32> output_locked = false;

	be_t<u32> run_thread_mutex; // sys_mutex_t
	be_t<u32> run_thread_cond;  // sys_cond_t
	be_t<u32> run_thread = true;

	const AdecCb<AdecNotifyAuDone> notify_au_done;
	const AdecCb<AdecNotifyPcmOut> notify_pcm_out;
	const AdecCb<AdecNotifyError> notify_error;
	const AdecCb<AdecNotifySeqDone> notify_seq_done;

	const vm::bptr<u8> work_mem;

	// HLE exclusive
	u64 cmd_counter = 0;         // For debugging
	AtracXdecCmd cmd;            // For savestates; if savestate was created while processing a decode command, we need to save the current command
	atracxdec_state savestate{}; // For savestates
	b8 skip_next_frame;          // Needed to emulate behavior of LLE SPU program, it doesn't output the first frame after a sequence reset or error

	u8 spurs_stuff[58]; // 120 bytes on LLE, pointers to CellSpurs, CellSpursTaskset, etc.

	CellAdecParamAtracX atracx_param;

	u8 reserved;
	b8 first_decode;

	AtracXdecDecoder decoder;

	AtracXdecContext(vm::ptr<AdecNotifyAuDone> notifyAuDone, vm::ptr<void> notifyAuDoneArg, vm::ptr<AdecNotifyPcmOut> notifyPcmOut, vm::ptr<void> notifyPcmOutArg,
		vm::ptr<AdecNotifyError> notifyError, vm::ptr<void> notifyErrorArg, vm::ptr<AdecNotifySeqDone> notifySeqDone, vm::ptr<void> notifySeqDoneArg, vm::bptr<u8> work_mem)
		: notify_au_done{ notifyAuDone, notifyAuDoneArg }
		, notify_pcm_out{ notifyPcmOut, notifyPcmOutArg }
		, notify_error{ notifyError, notifyErrorArg }
		, notify_seq_done{ notifySeqDone, notifySeqDoneArg }
		, work_mem(work_mem)
		, decoder(work_mem)
	{
	}

	void exec(ppu_thread& ppu);

	template <AtracXdecCmdType type>
	error_code send_command(ppu_thread& ppu, auto&&... args);
};

static_assert(std::is_standard_layout_v<AtracXdecContext>);
CHECK_SIZE_ALIGN(AtracXdecContext, 0x268, 8);

constexpr u32 ATXDEC_SPURS_STRUCTS_SIZE = 0x1cf00; // CellSpurs, CellSpursTaskset, context, etc.
constexpr u16 ATXDEC_SAMPLES_PER_FRAME = 0x800;
constexpr u16 ATXDEC_MAX_FRAME_LENGTH = 0x2000;
constexpr std::array<u8, 8> ATXDEC_NCH_BLOCKS_MAP = { 0, 1, 1, 2, 3, 4, 5, 5 };

// Expected output channel order
//  - for 1 to 7 channels: Front Left, Center, Front Right, Rear Left, Rear Right, Rear Center, LFE
//  - for 8 channels:      Front Left, Front Right, Center, LFE, Rear Left, Rear Right, Side Left, Side Right
// FFmpeg output
//  - ver <= 5.1.2: Front Left, Front Right, Center, Rear Left, Rear Right, Rear Center, Side Left, Side Right, LFE
//  - ver >= 5.1.3: Front Left, Front Right, Center, LFE, Rear Left, Rear Right, Rear Center, Side Left, Side Right
constexpr u8 ATXDEC_AVCODEC_CH_MAP[7][8] =
{
	{ 0 },
	{ 0, 1 },
	{ 0, 2, 1 },
	{ 0, 2, 1, 3 },
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(59, 51, 101)
	{ 0, 2, 1, 3, 4, 5 },
	{ 0, 2, 1, 3, 4, 5, 6 },
	{ 0, 1, 2, 4, 5, 6, 7, 3 }
#else
	{ 0, 2, 1, 4, 5, 3 },
	{ 0, 2, 1, 4, 5, 6, 3 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 }
#endif
};
