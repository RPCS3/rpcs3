#pragma once

enum MvcDecError : u32
{
	MVC_DEC_ERROR_ARG = 0x80615901,
};

struct AvcDecAttr;
struct AvcDecParams;

error_code mvcDecQueryMemory(ppu_thread& ppu, vm::cptr<AvcDecParams> params, vm::ptr<AvcDecAttr> attr);
void mvcDecGetVersion(ppu_thread& ppu, vm::ptr<u32> version);
error_code mvcDecQueryCharacteristics(ppu_thread& ppu, vm::ptr<u32> unk);
