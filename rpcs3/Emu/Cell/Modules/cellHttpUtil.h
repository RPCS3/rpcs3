#pragma once

namespace vm { using namespace ps3; }

// libHttp_Util: 0x80711001 - 0x807110ff

// Error Codes
enum
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
	vm::cptr<char> scheme;
	vm::cptr<char> hostname;
	vm::cptr<char> username;
	vm::cptr<char> password;
	vm::cptr<char> path;
	u32 port;
	u8 reserved[4];
};

struct CellHttpUriPath
{
	vm::cptr<char> path;
	vm::cptr<char> query;
	vm::cptr<char> fragment;
};

struct CellHttpRequestLine
{
	vm::cptr<char> method;
	vm::cptr<char> path;
	vm::cptr<char> protocol;
	u32 majorVersion;
	u32 minorVersion;
};

struct CellHttpStatusLine
{
	vm::cptr<char> protocol;
	u32 majorVersion;
	u32 minorVersion;
	vm::cptr<char> reasonPhrase;
	s32 statusCode;
	u8 reserved[4];
};

struct CellHttpHeader
{
	vm::cptr<char> name;
	vm::cptr<char> value;
};
