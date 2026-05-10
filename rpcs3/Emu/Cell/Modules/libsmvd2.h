#pragma once

enum Smvd2Error : u32
{
	SMVD2_ERROR_ARG = 0x80615041,
};

enum Smvd2ProfileLevel : u32
{
	SMVD2_MP_LL = 1,
	SMVD2_MP_ML,
	SMVD2_MP_H14,
	SMVD2_MP_HL
};

struct Smvd2Params
{
	be_t<u32> unk1;
	be_t<u32> unk2;
	be_t<u32> unk3;
	be_t<u32> max_decoded_frame_width;
	be_t<u32> max_decoded_frame_height;
};

error_code smvd2GetMemorySize(ppu_thread& ppu, vm::ptr<u32> memSize, u32 profileLevel);
error_code smvd2GetMemorySize2(ppu_thread& ppu, vm::ptr<u32> memSize, u32 profileLevel, vm::cptr<Smvd2Params> params);
error_code smvd2GetVersionNumber(ppu_thread& ppu, vm::ptr<u32> version);
