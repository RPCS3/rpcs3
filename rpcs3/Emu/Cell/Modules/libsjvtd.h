#pragma once

enum SjvtdError : u32
{
	SJVTD_ERROR_ARG = 0x80615841,
};

enum SjvtdProfileLevel : u8
{
	SJVTD_LEVEL_1P0 = 1,
	SJVTD_LEVEL_1P1,
	SJVTD_LEVEL_1P2,
	SJVTD_LEVEL_1P3,
	SJVTD_LEVEL_2P0,
	SJVTD_LEVEL_2P1,
	SJVTD_LEVEL_2P2,
	SJVTD_LEVEL_3P0,
	SJVTD_LEVEL_3P1,
	SJVTD_LEVEL_3P2,
	SJVTD_LEVEL_4P0,
	SJVTD_LEVEL_4P1,
	SJVTD_LEVEL_4P2
};

struct SjvtdParams
{
	be_t<u32> unk1;
	be_t<u32> unk2;
	be_t<u32> unk3;
	be_t<s32> max_decoded_frame_width;
	be_t<s32> max_decoded_frame_height;
	be_t<u32> enable_deblocking_filter;
	be_t<u32> unk;
	be_t<u32> number_of_decoded_frame_buffer;
};

error_code sjvtdGetMemorySize(ppu_thread& ppu, vm::ptr<u32> memSize, u32 profileLevel);
error_code sjvtdGetMemorySize2(ppu_thread& ppu, vm::ptr<u32> memSize, u32 profileLevel, vm::cptr<SjvtdParams> params);
error_code sjvtdGetVersionNumber(ppu_thread& ppu, vm::ptr<u32> version);
