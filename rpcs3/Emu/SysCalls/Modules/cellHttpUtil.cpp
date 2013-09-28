#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellHttpUtil_init();
Module cellHttpUtil(0x0002, cellHttpUtil_init);

int cellHttpUtilParseUri()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

int cellHttpUtilParseUriPath()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

int cellHttpUtilParseProxy()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

int cellHttpUtilParseStatusLine()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

int cellHttpUtilParseHeader()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

int cellHttpUtilBuildRequestLine()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

int cellHttpUtilBuildHeader()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

int cellHttpUtilBuildUri()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

int cellHttpUtilCopyUri()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

int cellHttpUtilMergeUriPath()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

int cellHttpUtilSweepPath()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

int cellHttpUtilCopyStatusLine()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

int cellHttpUtilCopyHeader()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

int cellHttpUtilAppendHeaderValue()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

int cellHttpUtilEscapeUri()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

int cellHttpUtilUnescapeUri()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

int cellHttpUtilFormUrlEncode()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

int cellHttpUtilFormUrlDecode()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

int cellHttpUtilBase64Encoder()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

int cellHttpUtilBase64Decoder()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

void cellHttpUtil_init()
{
	cellHttpUtil.AddFunc(0x32faaf58, cellHttpUtilParseUri);
	cellHttpUtil.AddFunc(0x8bb608e4, cellHttpUtilParseUriPath);
	cellHttpUtil.AddFunc(0xa3457869, cellHttpUtilParseProxy);
	cellHttpUtil.AddFunc(0x2bcbced4, cellHttpUtilParseStatusLine);
	cellHttpUtil.AddFunc(0xe1fb0ebd, cellHttpUtilParseHeader);

	cellHttpUtil.AddFunc(0x1c6e4dbb, cellHttpUtilBuildRequestLine);
	cellHttpUtil.AddFunc(0x04accebf, cellHttpUtilBuildHeader);
	cellHttpUtil.AddFunc(0x6f0f7667, cellHttpUtilBuildUri);

	cellHttpUtil.AddFunc(0xf05df789, cellHttpUtilCopyUri);
	cellHttpUtil.AddFunc(0x8ea23deb, cellHttpUtilMergeUriPath);
	cellHttpUtil.AddFunc(0xaabeb869, cellHttpUtilSweepPath);
	cellHttpUtil.AddFunc(0x50ea75bc, cellHttpUtilCopyStatusLine);
	cellHttpUtil.AddFunc(0x97f9fbe5, cellHttpUtilCopyHeader);
	cellHttpUtil.AddFunc(0x37bb53a2, cellHttpUtilAppendHeaderValue);

	cellHttpUtil.AddFunc(0x9003b1f2, cellHttpUtilEscapeUri);
	cellHttpUtil.AddFunc(0x2763fd66, cellHttpUtilUnescapeUri);
	cellHttpUtil.AddFunc(0x44d756d6, cellHttpUtilFormUrlEncode);
	cellHttpUtil.AddFunc(0x8e6c5bb9, cellHttpUtilFormUrlDecode);
	cellHttpUtil.AddFunc(0x83faa354, cellHttpUtilBase64Encoder);
	cellHttpUtil.AddFunc(0x8e52ee08, cellHttpUtilBase64Decoder);
}