/*
Copyright (C) 2016 Ivan
This file may be modified and distributed under the terms of the MIT license:

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

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
