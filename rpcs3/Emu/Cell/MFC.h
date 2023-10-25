#pragma once

#include "util/types.hpp"

enum MFC : u8
{
	MFC_PUT_CMD      = 0x20, MFC_PUTB_CMD     = 0x21, MFC_PUTF_CMD     = 0x22,
	MFC_PUTS_CMD     = 0x28, MFC_PUTBS_CMD    = 0x29, MFC_PUTFS_CMD    = 0x2a,
	MFC_PUTR_CMD     = 0x30, MFC_PUTRB_CMD    = 0x31, MFC_PUTRF_CMD    = 0x32,
	MFC_GET_CMD      = 0x40, MFC_GETB_CMD     = 0x41, MFC_GETF_CMD     = 0x42,
	MFC_GETS_CMD     = 0x48, MFC_GETBS_CMD    = 0x49, MFC_GETFS_CMD    = 0x4a,
	MFC_PUTL_CMD     = 0x24, MFC_PUTLB_CMD    = 0x25, MFC_PUTLF_CMD    = 0x26,
	MFC_PUTRL_CMD    = 0x34, MFC_PUTRLB_CMD   = 0x35, MFC_PUTRLF_CMD   = 0x36,
	MFC_GETL_CMD     = 0x44, MFC_GETLB_CMD    = 0x45, MFC_GETLF_CMD    = 0x46,
	MFC_GETLLAR_CMD  = 0xD0,
	MFC_PUTLLC_CMD   = 0xB4,
	MFC_PUTLLUC_CMD  = 0xB0,
	MFC_PUTQLLUC_CMD = 0xB8,

	MFC_SNDSIG_CMD   = 0xA0, MFC_SNDSIGB_CMD  = 0xA1, MFC_SNDSIGF_CMD  = 0xA2,
	MFC_BARRIER_CMD  = 0xC0,
	MFC_EIEIO_CMD    = 0xC8,
	MFC_SYNC_CMD     = 0xCC,

	MFC_BARRIER_MASK = 0x01,
	MFC_FENCE_MASK   = 0x02,
	MFC_LIST_MASK    = 0x04,
	MFC_START_MASK   = 0x08,
	MFC_RESULT_MASK  = 0x10, // ???

	MFC_SDCRT_CMD   = 0x80,
	MFC_SDCRTST_CMD = 0x81,
	MFC_SDCRZ_CMD   = 0x89,
	MFC_SDCRS_CMD   = 0x8D,
	MFC_SDCRF_CMD   = 0x8F,
};

// Atomic Status Update
enum mfc_atomic_status : u32
{
	MFC_PUTLLC_SUCCESS = 0,
	MFC_PUTLLC_FAILURE = 1, // reservation was lost
	MFC_PUTLLUC_SUCCESS = 2,
	MFC_GETLLAR_SUCCESS = 4,
};

// MFC Write Tag Status Update Request Channel (ch23) operations
enum mfc_tag_update : u32
{
	MFC_TAG_UPDATE_IMMEDIATE = 0,
	MFC_TAG_UPDATE_ANY       = 1,
	MFC_TAG_UPDATE_ALL       = 2,
};

enum : u32
{
	MFC_PPU_DMA_CMD_ENQUEUE_SUCCESSFUL      = 0x00,
	MFC_PPU_DMA_CMD_SEQUENCE_ERROR          = 0x01,
	MFC_PPU_DMA_QUEUE_FULL                  = 0x02,
};

enum : u32
{
	MFC_PROXY_COMMAND_QUEUE_EMPTY_FLAG      = 0x80000000,
};

enum : u32
{
	MFC_PPU_MAX_QUEUE_SPACE                 = 0x08,
	MFC_SPU_MAX_QUEUE_SPACE                 = 0x10,
};

enum : u32
{
	MFC_DMA_TAG_STATUS_UPDATE_EVENT    = 0x00000001,
	MFC_DMA_TAG_CMD_STALL_NOTIFY_EVENT = 0x00000002,
	MFC_DMA_QUEUE_VACANCY_EVENT        = 0x00000008,
	MFC_SPU_MAILBOX_WRITTEN_EVENT      = 0x00000010,
	MFC_DECREMENTER_EVENT              = 0x00000020,
	MFC_PU_INT_MAILBOX_AVAIL_EVENT     = 0x00000040,
	MFC_PU_MAILBOX_AVAIL_EVENT         = 0x00000080,
	MFC_SIGNAL_2_EVENT                 = 0x00000100,
	MFC_SIGNAL_1_EVENT                 = 0x00000200,
	MFC_LLR_LOST_EVENT                 = 0x00000400,
	MFC_PRIV_ATTN_EVENT                = 0x00000800,
	MFC_MULTISOURCE_SYNC_EVENT         = 0x00001000,
};

struct alignas(16) spu_mfc_cmd
{
	ENABLE_BITWISE_SERIALIZATION;

	MFC cmd;
	u8 tag;
	u16 size;
	u32 lsa;
	u32 eal;
	u32 eah;
};

enum class spu_block_hash : u64;

struct mfc_cmd_dump
{
	spu_mfc_cmd cmd;
	u64 block_hash;
	alignas(16) u8 data[128];
};
