#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceVideodec.h"

logs::channel sceVideodec("sceVideodec");

s32 sceVideodecInitLibrary(u32 codecType, vm::cptr<SceVideodecQueryInitInfo> pInitInfo)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceVideodecTermLibrary(u32 codecType)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAvcdecQueryDecoderMemSize(u32 codecType, vm::cptr<SceAvcdecQueryDecoderInfo> pDecoderInfo, vm::ptr<SceAvcdecDecoderInfo> pMemInfo)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAvcdecCreateDecoder(u32 codecType, vm::ptr<SceAvcdecCtrl> pCtrl, vm::cptr<SceAvcdecQueryDecoderInfo> pDecoderInfo)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAvcdecDeleteDecoder(vm::ptr<SceAvcdecCtrl> pCtrl)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAvcdecDecodeAvailableSize(vm::ptr<SceAvcdecCtrl> pCtrl)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAvcdecDecode(vm::ptr<SceAvcdecCtrl> pCtrl, vm::cptr<SceAvcdecAu> pAu, vm::ptr<SceAvcdecArrayPicture> pArrayPicture)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAvcdecDecodeStop(vm::ptr<SceAvcdecCtrl> pCtrl, vm::ptr<SceAvcdecArrayPicture> pArrayPicture)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAvcdecDecodeFlush(vm::ptr<SceAvcdecCtrl> pCtrl)
{
	fmt::throw_exception("Unimplemented" HERE);
}


#define REG_FUNC(nid, name) REG_FNID(SceVideodecUser, nid, name)

DECLARE(arm_module_manager::SceVideodec)("SceVideodecUser", []()
{
	REG_FUNC(0xF1AF65A3, sceVideodecInitLibrary);
	REG_FUNC(0x3A5F4924, sceVideodecTermLibrary);
	REG_FUNC(0x97E95EDB, sceAvcdecQueryDecoderMemSize);
	REG_FUNC(0xE82BB69B, sceAvcdecCreateDecoder);
	REG_FUNC(0x8A0E359E, sceAvcdecDeleteDecoder);
	REG_FUNC(0x441673E3, sceAvcdecDecodeAvailableSize);
	REG_FUNC(0xD6190A06, sceAvcdecDecode);
	REG_FUNC(0x9648D853, sceAvcdecDecodeStop);
	REG_FUNC(0x25F31020, sceAvcdecDecodeFlush);
	//REG_FUNC(0xB2A428DB, sceAvcdecCsc);
	//REG_FUNC(0x6C68A38F, sceAvcdecDecodeNalAu);
	//REG_FUNC(0xC67C1A80, sceM4vdecQueryDecoderMemSize);
	//REG_FUNC(0x17C6AC9E, sceM4vdecCreateDecoder);
	//REG_FUNC(0x0EB2E4E7, sceM4vdecDeleteDecoder);
	//REG_FUNC(0xA8CF1942, sceM4vdecDecodeAvailableSize);
	//REG_FUNC(0x624664DB, sceM4vdecDecode);
	//REG_FUNC(0x87CFD23B, sceM4vdecDecodeStop);
	//REG_FUNC(0x7C460D75, sceM4vdecDecodeFlush);
	//REG_FUNC(0xB4BC325B, sceM4vdecCsc);
});
