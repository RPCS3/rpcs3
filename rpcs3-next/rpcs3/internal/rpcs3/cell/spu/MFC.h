#pragma once

const char* get_mfc_cmd_name(u32 cmd);

enum : u32
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
};

// Atomic Status Update
enum : u32
{
	MFC_PUTLLC_SUCCESS = 0,
	MFC_PUTLLC_FAILURE = 1, // reservation was lost
	MFC_PUTLLUC_SUCCESS = 2,
	MFC_GETLLAR_SUCCESS = 4,
};

// MFC Write Tag Status Update Request Channel (ch23) operations
enum : u32
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

struct spu_mfc_arg_t
{
	union
	{
		u64 ea;

		struct
		{
			u32 eal;
			u32 eah;
		};
	};

	u32 lsa;

	union
	{
		struct
		{
			u16 tag;
			u16 size;
		};

		u32 size_tag;
	};
};
