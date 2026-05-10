#pragma once

enum Divx311DecError : u32
{
	DIVX311_DEC_ERROR_ARG = 0x80615502,
};

struct DivxDecParams;

error_code divx311DecQueryAttr(ppu_thread& ppu, vm::cptr<DivxDecParams> params, vm::ptr<u32> memSize, vm::ptr<u32> version);
