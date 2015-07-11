#pragma once

struct SceJpegOutputInfo
{
	le_t<s32> colorSpace;
	le_t<u16> imageWidth;
	le_t<u16> imageHeight;
	le_t<u32> outputBufferSize;
	le_t<u32> tempBufferSize;
	le_t<u32> coefBufferSize;

	struct _pitch_t
	{
		le_t<u32> x;
		le_t<u32> y;
	};

	_pitch_t pitch[4];
};

struct SceJpegSplitDecodeCtrl
{
	vm::lptr<u8> pStreamBuffer;
	le_t<u32> streamBufferSize;
	vm::lptr<u8> pWriteBuffer;
	le_t<u32> writeBufferSize;
	le_t<s32> isEndOfStream;
	le_t<s32> decodeMode;

	SceJpegOutputInfo outputInfo;

	vm::lptr<void> pOutputBuffer;
	vm::lptr<void> pCoefBuffer;

	le_t<u32> internalData[3];
};

extern psv_log_base sceJpeg;
