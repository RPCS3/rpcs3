#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceVideodec;

struct SceVideodecQueryInitInfoHwAvcdec
{
	u32 size;
	u32 horizontal;
	u32 vertical;
	u32 numOfRefFrames;
	u32 numOfStreams;
};

union SceVideodecQueryInitInfo
{
	u8 reserved[32];
	SceVideodecQueryInitInfoHwAvcdec hwAvc;
};

struct SceVideodecTimeStamp
{
	u32 upper;
	u32 lower;
};

struct SceAvcdecQueryDecoderInfo
{
	u32 horizontal;
	u32 vertical;
	u32 numOfRefFrames;

};

struct SceAvcdecDecoderInfo
{
	u32 frameMemSize;

};

struct SceAvcdecBuf
{
	vm::psv::ptr<void> pBuf;
	u32 size;
};

struct SceAvcdecCtrl
{
	u32 handle;
	SceAvcdecBuf frameBuf;
};

struct SceAvcdecAu
{
	SceVideodecTimeStamp pts;
	SceVideodecTimeStamp dts;
	SceAvcdecBuf es;
};

struct SceAvcdecInfo
{
	u32 numUnitsInTick;
	u32 timeScale;
	u8 fixedFrameRateFlag;

	u8 aspectRatioIdc;
	u16 sarWidth;
	u16 sarHeight;

	u8 colourPrimaries;
	u8 transferCharacteristics;
	u8 matrixCoefficients;

	u8 videoFullRangeFlag;

	u8 picStruct[2];
	u8 ctType;

	u8 padding[3];
};

struct SceAvcdecFrameOptionRGBA
{
	u8 alpha;
	u8 cscCoefficient;
	u8 reserved[14];
};

union SceAvcdecFrameOption
{
	u8 reserved[16];
	SceAvcdecFrameOptionRGBA rgba;
};


struct SceAvcdecFrame
{
	u32 pixelType;
	u32 framePitch;
	u32 frameWidth;
	u32 frameHeight;

	u32 horizontalSize;
	u32 verticalSize;

	u32 frameCropLeftOffset;
	u32 frameCropRightOffset;
	u32 frameCropTopOffset;
	u32 frameCropBottomOffset;

	SceAvcdecFrameOption opt;

	vm::psv::ptr<void> pPicture[2];
};


struct SceAvcdecPicture
{
	u32 size;
	SceAvcdecFrame frame;
	SceAvcdecInfo info;
};

struct SceAvcdecArrayPicture
{
	u32 numOfOutput;
	u32 numOfElm;
	vm::psv::ptr<vm::psv::ptr<SceAvcdecPicture>> pPicture;
};


s32 sceVideodecInitLibrary(u32 codecType, vm::psv::ptr<const SceVideodecQueryInitInfo> pInitInfo)
{
	throw __FUNCTION__;
}

s32 sceVideodecTermLibrary(u32 codecType)
{
	throw __FUNCTION__;
}

s32 sceAvcdecQueryDecoderMemSize(u32 codecType, vm::psv::ptr<const SceAvcdecQueryDecoderInfo> pDecoderInfo, vm::psv::ptr<SceAvcdecDecoderInfo> pMemInfo)
{
	throw __FUNCTION__;
}

s32 sceAvcdecCreateDecoder(u32 codecType, vm::psv::ptr<SceAvcdecCtrl> pCtrl, vm::psv::ptr<const SceAvcdecQueryDecoderInfo> pDecoderInfo)
{
	throw __FUNCTION__;
}

s32 sceAvcdecDeleteDecoder(vm::psv::ptr<SceAvcdecCtrl> pCtrl)
{
	throw __FUNCTION__;
}

s32 sceAvcdecDecodeAvailableSize(vm::psv::ptr<SceAvcdecCtrl> pCtrl)
{
	throw __FUNCTION__;
}

s32 sceAvcdecDecode(vm::psv::ptr<SceAvcdecCtrl> pCtrl, vm::psv::ptr<const SceAvcdecAu> pAu, vm::psv::ptr<SceAvcdecArrayPicture> pArrayPicture)
{
	throw __FUNCTION__;
}

s32 sceAvcdecDecodeStop(vm::psv::ptr<SceAvcdecCtrl> pCtrl, vm::psv::ptr<SceAvcdecArrayPicture> pArrayPicture)
{
	throw __FUNCTION__;
}

s32 sceAvcdecDecodeFlush(vm::psv::ptr<SceAvcdecCtrl> pCtrl)
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func<(func_ptr)name>(nid, &sceVideodec, #name, name)

psv_log_base sceVideodec("SceVideodec", []()
{
	sceVideodec.on_load = nullptr;
	sceVideodec.on_unload = nullptr;
	sceVideodec.on_stop = nullptr;

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
