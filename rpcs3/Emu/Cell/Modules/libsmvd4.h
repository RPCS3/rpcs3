#pragma once

enum Smvd4Error : u32
{
	SMVD4_ERROR_ARG = 0x80615141,
};

enum Smvd4ProfileLevel : u32
{
	SMVD4_SP_L1 = 1,
	SMVD4_SP_L2,
	SMVD4_SP_L3,
	SMVD4_SP_D1_NTSC,
	SMVD4_SP_VGA,
	SMVD4_SP_D1_PAL
};

struct Smvd4Params
{
	be_t<u32> unk1;
	be_t<u32> unk2;
	u32 unk[5];
	be_t<u32> unk3;
	be_t<u32> max_decoded_frame_width;
	be_t<u32> max_decoded_frame_height;
};

error_code smvd4GetMemorySize(ppu_thread& ppu, vm::ptr<u32> memSize, u32 profileLevel);
error_code smvd4GetMemorySize2(ppu_thread& ppu, vm::ptr<u32> memSize, u32 profileLevel, vm::cptr<Smvd4Params> params);
error_code smvd4GetVersionNumber(ppu_thread& ppu, vm::ptr<u32> version);
