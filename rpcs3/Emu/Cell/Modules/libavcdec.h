#pragma once

enum AvcDecError : u32
{
	AVC_DEC_ERROR_ARG = 0x80615201,
};

struct AvcDecParams
{
	u8 profile_level;
	b8 disable_deblocking_filter;
	u8 number_of_decoded_frame_buffer;
	be_t<u16> max_decoded_frame_width;
	be_t<u16> max_decoded_frame_height;
};

struct AvcDecAttr
{
	u32 unk;
	be_t<u32> mem_size;
	be_t<u32> mem_align;
};

error_code avcDecQueryMemory(ppu_thread& ppu, vm::cptr<AvcDecParams> params, vm::ptr<AvcDecAttr> attr);
void avcDecGetVersion(ppu_thread& ppu, vm::ptr<u32> version);
error_code avcDecQueryCharacteristics(ppu_thread& ppu, vm::ptr<u32> unk);
