#pragma once

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
	be_t<u32> segs_addr;
	be_t<int> nsegs;
};
