#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

extern Module cellHttpUtil;

s32 cellHttpUtilParseUri()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

s32 cellHttpUtilParseUriPath()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

s32 cellHttpUtilParseProxy()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

s32 cellHttpUtilParseStatusLine()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

s32 cellHttpUtilParseHeader()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

s32 cellHttpUtilBuildRequestLine()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

s32 cellHttpUtilBuildHeader()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

s32 cellHttpUtilBuildUri()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

s32 cellHttpUtilCopyUri()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

s32 cellHttpUtilMergeUriPath()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

s32 cellHttpUtilSweepPath()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

s32 cellHttpUtilCopyStatusLine()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

s32 cellHttpUtilCopyHeader()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

s32 cellHttpUtilAppendHeaderValue()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

s32 cellHttpUtilEscapeUri()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

s32 cellHttpUtilUnescapeUri()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

s32 cellHttpUtilFormUrlEncode()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

s32 cellHttpUtilFormUrlDecode()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

s32 cellHttpUtilBase64Encoder()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

s32 cellHttpUtilBase64Decoder()
{
	UNIMPLEMENTED_FUNC(cellHttpUtil);
	return CELL_OK;
}

Module cellHttpUtil("cellHttpUtil", []()
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
});
