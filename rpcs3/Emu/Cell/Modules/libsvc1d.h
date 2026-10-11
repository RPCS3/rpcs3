#pragma once

enum Svc1dError : u32
{
	SVC1D_ERROR_ARG = 0x80615341,
};

enum Svc1dProfileLevel : u32
{
	SVC1D_SP_LL = 1,
	SVC1D_SP_ML,
	SVC1D_MP_LL,
	SVC1D_MP_ML,
	SVC1D_MP_HL,
	SVC1D_AP_L0,
	SVC1D_AP_L1,
	SVC1D_AP_L2,
	SVC1D_AP_L3,
	SVC1D_AP_L4
};

struct Svc1dParams
{
	be_t<u32> unk1;
	be_t<u32> unk2;
	u32 unk[5];
	be_t<u32> unk3;
	be_t<u32> max_decoded_frame_width;
	be_t<u32> max_decoded_frame_height;
};

error_code svc1dGetMemorySize(ppu_thread& ppu, vm::ptr<u32> memSize, u32 profileLevel);
error_code svc1dGetMemorySize2(ppu_thread& ppu, vm::ptr<u32> memSize, u32 profileLevel, vm::cptr<Svc1dParams> params);
error_code svc1dGetVersionNumber(ppu_thread& ppu, vm::ptr<u32> version);
