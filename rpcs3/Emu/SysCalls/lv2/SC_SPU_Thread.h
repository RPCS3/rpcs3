#pragma once

u32 LoadSpuImage(vfsStream& stream, u32& spu_ep);

enum
{
	SYS_SPU_THREAD_GROUP_JOIN_GROUP_EXIT = 0x0001,
	SYS_SPU_THREAD_GROUP_JOIN_ALL_THREADS_EXIT = 0x0002,
	SYS_SPU_THREAD_GROUP_JOIN_TERMINATED = 0x0004
};

enum
{
	SYS_SPU_SEGMENT_TYPE_COPY = 0x0001,
	SYS_SPU_SEGMENT_TYPE_FILL = 0x0002,
	SYS_SPU_SEGMENT_TYPE_INFO = 0x0004,
};

struct sys_spu_thread_group_attribute
{
	be_t<u32> name_len;
	be_t<u32> name_addr;
	be_t<int> type;
	struct{be_t<u32> ct;} option;
};

struct sys_spu_thread_attribute
{
	be_t<u32> name_addr;
	be_t<u32> name_len;
	be_t<u32> option;
};

struct sys_spu_thread_argument
{
	be_t<u64> arg1;
	be_t<u64> arg2;
	be_t<u64> arg3;
	be_t<u64> arg4;
};

struct sys_spu_image
{
	be_t<u32> type;
	be_t<u32> entry_point; 
	be_t<u32> segs_addr; //temporarily used as offset of LS image after elf loading
	be_t<int> nsegs;
};

struct sys_spu_segment
{
	be_t<int> type;
	be_t<u32> ls_start;
	be_t<int> size;
	be_t<u64> src;
};
