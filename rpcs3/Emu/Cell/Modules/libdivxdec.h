#pragma once

enum DivxDecError : u32
{
	DIVX_DEC_ERROR_ARG = 0x80615502,
};

struct DivxDecParams
{
	be_t<u32> profile_level;
	be_t<u32> max_decoded_frame_width;
	be_t<u32> max_decoded_frame_height;
	be_t<u32> number_of_decoded_frame_buffer;
};

error_code divxDecQueryAttr(ppu_thread& ppu, vm::cptr<DivxDecParams> params, vm::ptr<u32> memSize, vm::ptr<u32> version);
