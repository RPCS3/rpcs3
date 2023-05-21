#pragma once

#include "Emu/Memory/vm_ptr.h"

// libHttp_Util: 0x80711001 - 0x807110ff

// Error Codes
enum CellHttpUtilError : u32
{
	CELL_HTTP_UTIL_ERROR_NO_MEMORY         = 0x80711001,
	CELL_HTTP_UTIL_ERROR_NO_BUFFER         = 0x80711002,
	CELL_HTTP_UTIL_ERROR_NO_STRING         = 0x80711003,
	CELL_HTTP_UTIL_ERROR_INSUFFICIENT      = 0x80711004,
	CELL_HTTP_UTIL_ERROR_INVALID_URI       = 0x80711005,
	CELL_HTTP_UTIL_ERROR_INVALID_HEADER    = 0x80711006,
	CELL_HTTP_UTIL_ERROR_INVALID_REQUEST   = 0x80711007,
	CELL_HTTP_UTIL_ERROR_INVALID_RESPONSE  = 0x80711008,
	CELL_HTTP_UTIL_ERROR_INVALID_LENGTH    = 0x80711009,
	CELL_HTTP_UTIL_ERROR_INVALID_CHARACTER = 0x8071100a,
};

enum
{
	CELL_HTTP_UTIL_URI_FLAG_FULL_URI       = 0x00000000,
	CELL_HTTP_UTIL_URI_FLAG_NO_SCHEME      = 0x00000001,
	CELL_HTTP_UTIL_URI_FLAG_NO_CREDENTIALS = 0x00000002,
	CELL_HTTP_UTIL_URI_FLAG_NO_PASSWORD    = 0x00000004,
	CELL_HTTP_UTIL_URI_FLAG_NO_PATH        = 0x00000008
};

struct CellHttpUri
{
	vm::bcptr<char> scheme;
	vm::bcptr<char> hostname;
	vm::bcptr<char> username;
	vm::bcptr<char> password;
	vm::bcptr<char> path;
	be_t<u32> port;
	u8 reserved[4];
};

struct CellHttpUriPath
{
	vm::bcptr<char> path;
	vm::bcptr<char> query;
	vm::bcptr<char> fragment;
};

struct CellHttpRequestLine
{
	vm::bcptr<char> method;
	vm::bcptr<char> path;
	vm::bcptr<char> protocol;
	be_t<u32> majorVersion;
	be_t<u32> minorVersion;
};

struct CellHttpStatusLine
{
	vm::bcptr<char> protocol;
	be_t<u32> majorVersion;
	be_t<u32> minorVersion;
	vm::bcptr<char> reasonPhrase;
	be_t<s32> statusCode;
	u8 reserved[4];
};

struct CellHttpHeader
{
	vm::bcptr<char> name;
	vm::bcptr<char> value;
};

error_code cellHttpUtilCopyUri(vm::ptr<CellHttpUri> dest, vm::cptr<CellHttpUri> src, vm::ptr<void> pool, u32 poolSize, vm::ptr<u32> required);
