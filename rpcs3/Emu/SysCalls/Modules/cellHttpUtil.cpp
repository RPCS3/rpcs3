#include "stdafx.h"
#if 0

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
	REG_FUNC(cellHttpUtil, cellHttpUtilParseUri);
	REG_FUNC(cellHttpUtil, cellHttpUtilParseUriPath);
	REG_FUNC(cellHttpUtil, cellHttpUtilParseProxy);
	REG_FUNC(cellHttpUtil, cellHttpUtilParseStatusLine);
	REG_FUNC(cellHttpUtil, cellHttpUtilParseHeader);

	REG_FUNC(cellHttpUtil, cellHttpUtilBuildRequestLine);
	REG_FUNC(cellHttpUtil, cellHttpUtilBuildHeader);
	REG_FUNC(cellHttpUtil, cellHttpUtilBuildUri);

	REG_FUNC(cellHttpUtil, cellHttpUtilCopyUri);
	REG_FUNC(cellHttpUtil, cellHttpUtilMergeUriPath);
	REG_FUNC(cellHttpUtil, cellHttpUtilSweepPath);
	REG_FUNC(cellHttpUtil, cellHttpUtilCopyStatusLine);
	REG_FUNC(cellHttpUtil, cellHttpUtilCopyHeader);
	REG_FUNC(cellHttpUtil, cellHttpUtilAppendHeaderValue);

	REG_FUNC(cellHttpUtil, cellHttpUtilEscapeUri);
	REG_FUNC(cellHttpUtil, cellHttpUtilUnescapeUri);
	REG_FUNC(cellHttpUtil, cellHttpUtilFormUrlEncode);
	REG_FUNC(cellHttpUtil, cellHttpUtilFormUrlDecode);
	REG_FUNC(cellHttpUtil, cellHttpUtilBase64Encoder);
	REG_FUNC(cellHttpUtil, cellHttpUtilBase64Decoder);
}
#endif
