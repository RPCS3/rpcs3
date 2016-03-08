#pragma once

struct SceVideodecQueryInitInfoHwAvcdec
{
	le_t<u32> size;
	le_t<u32> horizontal;
	le_t<u32> vertical;
	le_t<u32> numOfRefFrames;
	le_t<u32> numOfStreams;
};

union SceVideodecQueryInitInfo
{
	u8 reserved[32];
	SceVideodecQueryInitInfoHwAvcdec hwAvc;
};

struct SceVideodecTimeStamp
{
	le_t<u32> upper;
	le_t<u32> lower;
};

struct SceAvcdecQueryDecoderInfo
{
	le_t<u32> horizontal;
	le_t<u32> vertical;
	le_t<u32> numOfRefFrames;
};

struct SceAvcdecDecoderInfo
{
	le_t<u32> frameMemSize;
};

struct SceAvcdecBuf
{
	vm::lptr<void> pBuf;
	le_t<u32> size;
};

struct SceAvcdecCtrl
{
	le_t<u32> handle;
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
	le_t<u32> numUnitsInTick;
	le_t<u32> timeScale;
	u8 fixedFrameRateFlag;

	u8 aspectRatioIdc;
	le_t<u16> sarWidth;
	le_t<u16> sarHeight;

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
	le_t<u32> pixelType;
	le_t<u32> framePitch;
	le_t<u32> frameWidth;
	le_t<u32> frameHeight;

	le_t<u32> horizontalSize;
	le_t<u32> verticalSize;

	le_t<u32> frameCropLeftOffset;
	le_t<u32> frameCropRightOffset;
	le_t<u32> frameCropTopOffset;
	le_t<u32> frameCropBottomOffset;

	SceAvcdecFrameOption opt;

	vm::lptr<void> pPicture[2];
};


struct SceAvcdecPicture
{
	le_t<u32> size;
	SceAvcdecFrame frame;
	SceAvcdecInfo info;
};

struct SceAvcdecArrayPicture
{
	le_t<u32> numOfOutput;
	le_t<u32> numOfElm;
	vm::lpptr<SceAvcdecPicture> pPicture;
};

extern psv_log_base sceVideodec;
