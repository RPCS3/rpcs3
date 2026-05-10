#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Utilities/LUrlParser.h"

#include "cellHttpUtil.h"

#ifdef _WIN32
#include <windows.h>
#include <codecvt>
#ifdef _MSC_VER
#pragma comment(lib, "Winhttp.lib")
#endif
#endif

LOG_CHANNEL(cellHttpUtil);

template <>
void fmt_class_string<CellHttpUtilError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_HTTP_UTIL_ERROR_NO_MEMORY);
			STR_CASE(CELL_HTTP_UTIL_ERROR_NO_BUFFER);
			STR_CASE(CELL_HTTP_UTIL_ERROR_NO_STRING);
			STR_CASE(CELL_HTTP_UTIL_ERROR_INSUFFICIENT);
			STR_CASE(CELL_HTTP_UTIL_ERROR_INVALID_URI);
			STR_CASE(CELL_HTTP_UTIL_ERROR_INVALID_HEADER);
			STR_CASE(CELL_HTTP_UTIL_ERROR_INVALID_REQUEST);
			STR_CASE(CELL_HTTP_UTIL_ERROR_INVALID_RESPONSE);
			STR_CASE(CELL_HTTP_UTIL_ERROR_INVALID_LENGTH);
			STR_CASE(CELL_HTTP_UTIL_ERROR_INVALID_CHARACTER);
		}

		return unknown;
	});
}

error_code cellHttpUtilParseUri(vm::ptr<CellHttpUri> uri, vm::cptr<char> str, vm::ptr<void> pool, u32 size, vm::ptr<u32> required)
{
	cellHttpUtil.trace("cellHttpUtilParseUri(uri=*0x%x, str=%s, pool=*0x%x, size=%d, required=*0x%x)", uri, str, pool, size, required);

	if (!str)
	{
		return CELL_HTTP_UTIL_ERROR_NO_STRING;
	}

	if (!pool || !uri)
	{
		if (!required)
		{
			return CELL_HTTP_UTIL_ERROR_NO_BUFFER;
		}
	}

	LUrlParser::clParseURL URL = LUrlParser::clParseURL::ParseURL(str.get_ptr());
	if ( URL.IsValid() )
	{
		std::string scheme = URL.m_Scheme;
		std::string host = URL.m_Host;
		std::string path = URL.m_Path;
		std::string username = URL.m_UserName;
		std::string password = URL.m_Password;

		u32 schemeOffset = 0;
		u32 hostOffset = ::size32(scheme) + 1;
		u32 pathOffset = hostOffset + ::size32(host) + 1;
		u32 usernameOffset = pathOffset + ::size32(path) + 1;
		u32 passwordOffset = usernameOffset + ::size32(username) + 1;
		u32 totalSize = passwordOffset + ::size32(password) + 1;

		//called twice, first to setup pool, then to populate.
		if (!uri)
		{
			*required = totalSize;
			return CELL_OK;
		}
		else
		{
			std::memcpy(vm::base(pool.addr() + schemeOffset), scheme.c_str(), scheme.length() + 1);
			std::memcpy(vm::base(pool.addr() + hostOffset), host.c_str(), host.length() + 1);
			std::memcpy(vm::base(pool.addr() + pathOffset), path.c_str(), path.length() + 1);
			std::memcpy(vm::base(pool.addr() + usernameOffset), username.c_str(), username.length() + 1);
			std::memcpy(vm::base(pool.addr() + passwordOffset), password.c_str(), password.length() + 1);

			uri->scheme.set(pool.addr() + schemeOffset);
			uri->hostname.set(pool.addr() + hostOffset);
			uri->path.set(pool.addr() + pathOffset);
			uri->username.set(pool.addr() + usernameOffset);
			uri->password.set(pool.addr() + passwordOffset);

			if (!URL.m_Port.empty())
			{
				int port = stoi(URL.m_Port);
				uri->port = port;
			}
			else
			{
				uri->port = 80;
			}
			return CELL_OK;
		}
	}
	else
	{
		std::string parseError;
		switch(URL.m_ErrorCode)
		{
			case LUrlParser::LUrlParserError_Ok:
				parseError = "No error, URL was parsed fine";
				break;
			case LUrlParser::LUrlParserError_Uninitialized:
				parseError = "Error, LUrlParser is uninitialized";
				break;
			case LUrlParser::LUrlParserError_NoUrlCharacter:
				parseError = "Error, the URL has invalid characters";
				break;
			case LUrlParser::LUrlParserError_InvalidSchemeName:
				parseError = "Error, the URL has an invalid scheme";
				break;
			case LUrlParser::LUrlParserError_NoDoubleSlash:
				parseError = "Error, the URL did not contain a double slash";
				break;
			case LUrlParser::LUrlParserError_NoAtSign:
				parseError = "Error, the URL did not contain an @ sign";
				break;
			case LUrlParser::LUrlParserError_UnexpectedEndOfLine:
				parseError = "Error, unexpectedly got the end of the line";
				break;
			case LUrlParser::LUrlParserError_NoSlash:
				parseError = "Error, URI didn't contain a slash";
				break;
			default:
				parseError = "Error, unknown error #" + std::to_string(static_cast<int>(URL.m_ErrorCode));
				break;
		}
		cellHttpUtil.error("%s, while parsing URI, %s.", parseError, str.get_ptr());
		return -1;
	}
}

error_code cellHttpUtilParseUriPath(vm::ptr<CellHttpUriPath> path, vm::cptr<char> str, vm::ptr<void> pool, u32 size, vm::ptr<u32> required)
{
	cellHttpUtil.todo("cellHttpUtilParseUriPath(path=*0x%x, str=%s, pool=*0x%x, size=%d, required=*0x%x)", path, str, pool, size, required);

	if (!str)
	{
		return CELL_HTTP_UTIL_ERROR_NO_STRING;
	}

	if (!pool || !path)
	{
		if (!required)
		{
			return CELL_HTTP_UTIL_ERROR_NO_BUFFER;
		}
	}

	return CELL_OK;
}

error_code cellHttpUtilParseProxy(vm::ptr<CellHttpUri> uri, vm::cptr<char> str, vm::ptr<void> pool, u32 size, vm::ptr<u32> required)
{
	cellHttpUtil.todo("cellHttpUtilParseProxy(uri=*0x%x, str=%s, pool=*0x%x, size=%d, required=*0x%x)", uri, str, pool, size, required);

	if (!str)
	{
		return CELL_HTTP_UTIL_ERROR_NO_STRING;
	}

	if (!pool || !uri)
	{
		if (!required)
		{
			return CELL_HTTP_UTIL_ERROR_NO_BUFFER;
		}
	}

	return CELL_OK;
}

error_code cellHttpUtilParseStatusLine(vm::ptr<CellHttpStatusLine> resp, vm::cptr<char> str, u32 len, vm::ptr<void> pool, u32 size, vm::ptr<u32> required, vm::ptr<u32> parsedLength)
{
	cellHttpUtil.todo("cellHttpUtilParseStatusLine(resp=*0x%x, str=%s, len=%d, pool=*0x%x, size=%d, required=*0x%x, parsedLength=*0x%x)", resp, str, len, pool, size, required, parsedLength);

	if (!str)
	{
		return CELL_HTTP_UTIL_ERROR_NO_STRING;
	}

	if (!pool || !resp)
	{
		if (!required)
		{
			return CELL_HTTP_UTIL_ERROR_NO_BUFFER;
		}
	}

	return CELL_OK;
}

error_code cellHttpUtilParseHeader(vm::ptr<CellHttpHeader> header, vm::cptr<char> str, u32 len, vm::ptr<void> pool, u32 size, vm::ptr<u32> required, vm::ptr<u32> parsedLength)
{
	cellHttpUtil.todo("cellHttpUtilParseHeader(header=*0x%x, str=%s, len=%d, pool=*0x%x, size=%d, required=*0x%x, parsedLength=*0x%x)", header, str, len, pool, size, required, parsedLength);

	if (!str)
	{
		return CELL_HTTP_UTIL_ERROR_NO_STRING;
	}

	if (!pool || !header)
	{
		if (!required)
		{
			return CELL_HTTP_UTIL_ERROR_NO_BUFFER;
		}
	}

	return CELL_OK;
}

error_code cellHttpUtilBuildRequestLine(vm::cptr<CellHttpRequestLine> req, vm::ptr<char> buf, u32 len, vm::ptr<u32> required)
{
	cellHttpUtil.notice("cellHttpUtilBuildRequestLine(req=*0x%x, buf=*0x%x, len=%d, required=*0x%x)", req, buf, len, required);

	if (!req || !req->method || !req->path || !req->protocol)
	{
		return CELL_HTTP_UTIL_ERROR_INVALID_REQUEST;
	}

	std::string path = fmt::format("%s", req->path);
	if (path.empty())
	{
		path += '/';
	}

	// TODO: are the numbers properly formatted ?
	const std::string result = fmt::format("%s %s %s/%d.%d\r\n", req->method, path, req->protocol, req->majorVersion, req->minorVersion);

	if (buf)
	{
		if (len < result.size())
		{
			return CELL_HTTP_UTIL_ERROR_INSUFFICIENT;
		}

		std::memcpy(buf.get_ptr(), result.c_str(), result.size());
	}

	if (required)
	{
		*required = ::narrow<u32>(result.size());
	}

	return CELL_OK;
}

error_code cellHttpUtilBuildHeader(vm::cptr<CellHttpHeader> header, vm::ptr<char> buf, u32 len, vm::ptr<u32> required)
{
	cellHttpUtil.notice("cellHttpUtilBuildHeader(header=*0x%x, buf=*0x%x, len=%d, required=*0x%x)", header, buf, len, required);

	if (!header || !header->name)
	{
		return CELL_HTTP_UTIL_ERROR_INVALID_HEADER;
	}

	const std::string result = fmt::format("%s: %s\r\n", header->name, header->value);

	if (buf)
	{
		if (len < result.size())
		{
			return CELL_HTTP_UTIL_ERROR_INSUFFICIENT;
		}

		std::memcpy(buf.get_ptr(), result.c_str(), result.size());
	}

	if (required)
	{
		*required = ::narrow<u32>(result.size());
	}

	return CELL_OK;
}

error_code cellHttpUtilBuildUri(vm::cptr<CellHttpUri> uri, vm::ptr<char> buf, u32 len, vm::ptr<u32> required, s32 flags)
{
	cellHttpUtil.todo("cellHttpUtilBuildUri(uri=*0x%x, buf=*0x%x, len=%d, required=*0x%x, flags=%d)", uri, buf, len, required, flags);

	if (!uri || !uri->hostname)
	{
		return CELL_HTTP_UTIL_ERROR_INVALID_URI;
	}

	std::string result;

	if (!(flags & CELL_HTTP_UTIL_URI_FLAG_NO_SCHEME))
	{
		if (uri->scheme && uri->scheme[0])
		{
			result = fmt::format("%s", uri->scheme);
		}
		else if (uri->port == 443u)
		{
			result = "https"; // TODO: confirm
		}
		else
		{
			result = "http"; // TODO: confirm
		}

		fmt::append(result, "://");
	}

	if (!(flags & CELL_HTTP_UTIL_URI_FLAG_NO_CREDENTIALS) && uri->username && uri->username[0])
	{
		fmt::append(result, "%s", uri->username);

		if (!(flags & CELL_HTTP_UTIL_URI_FLAG_NO_PASSWORD) && uri->password && uri->password[0])
		{
			fmt::append(result, ":%s", uri->password);
		}

		fmt::append(result, "@");
	}

	fmt::append(result, "%s", uri->hostname);

	if (true) // TODO: there seems to be a case where the port isn't added
	{
		fmt::append(result, ":%d", uri->port);
	}

	if (!(flags & CELL_HTTP_UTIL_URI_FLAG_NO_PATH) && uri->path && uri->path[0])
	{
		fmt::append(result, "%s", uri->path);
	}

	const u32 size_needed = ::narrow<u32>(result.size() + 1); // Including '\0'

	if (buf)
	{
		if (len < size_needed)
		{
			return CELL_HTTP_UTIL_ERROR_INSUFFICIENT;
		}

		std::memcpy(buf.get_ptr(), result.c_str(), size_needed);
	}

	if (required)
	{
		*required = size_needed;
	}

	return CELL_OK;
}

error_code cellHttpUtilCopyUri(vm::ptr<CellHttpUri> dest, vm::cptr<CellHttpUri> src, vm::ptr<void> pool, u32 poolSize, vm::ptr<u32> required)
{
	cellHttpUtil.todo("cellHttpUtilCopyUri(dest=*0x%x, src=*0x%x, pool=*0x%x, poolSize=%d, required=*0x%x)", dest, src, pool, poolSize, required);

	if (!src)
	{
		return CELL_HTTP_UTIL_ERROR_NO_BUFFER;
	}

	if (!pool || !dest)
	{
		if (!required)
		{
			return CELL_HTTP_UTIL_ERROR_NO_BUFFER;
		}
	}

	return CELL_OK;
}

error_code cellHttpUtilMergeUriPath(vm::ptr<CellHttpUri> uri, vm::cptr<CellHttpUri> src, vm::cptr<char> path, vm::ptr<void> pool, u32 poolSize, vm::ptr<u32> required)
{
	cellHttpUtil.todo("cellHttpUtilMergeUriPath(uri=*0x%x, src=*0x%x, path=%s, pool=*0x%x, poolSize=%d, required=*0x%x)", uri, src, path, pool, poolSize, required);

	if (!path)
	{
		return CELL_HTTP_UTIL_ERROR_NO_STRING;
	}

	if (!src)
	{
		return CELL_HTTP_UTIL_ERROR_NO_BUFFER;
	}

	if (!pool || !uri)
	{
		if (!required)
		{
			return CELL_HTTP_UTIL_ERROR_NO_BUFFER;
		}
	}

	return CELL_OK;
}

error_code cellHttpUtilSweepPath(vm::ptr<char> dst, vm::cptr<char> src, u32 srcSize)
{
	cellHttpUtil.todo("cellHttpUtilSweepPath(dst=*0x%x, src=%s, srcSize=%d)", dst, src, srcSize);

	if (!dst || !src)
	{
		return CELL_HTTP_UTIL_ERROR_NO_BUFFER;
	}

	if (!srcSize)
	{
		return CELL_OK;
	}

	u32 pos = 0;

	if (src[pos] != '/')
	{
		std::memcpy(dst.get_ptr(), src.get_ptr(), srcSize - 1);
		dst[srcSize - 1] = '\0';
		return CELL_OK;
	}

	// TODO

	return CELL_OK;
}

error_code cellHttpUtilCopyStatusLine(vm::ptr<CellHttpStatusLine> dest, vm::cptr<CellHttpStatusLine> src, vm::ptr<void> pool, u32 poolSize, vm::ptr<u32> required)
{
	cellHttpUtil.todo("cellHttpUtilCopyStatusLine(dest=*0x%x, src=*0x%x, pool=*0x%x, poolSize=%d, required=*0x%x)", dest, src, pool, poolSize, required);

	if (!src)
	{
		return CELL_HTTP_UTIL_ERROR_NO_BUFFER;
	}

	if (!pool || !dest)
	{
		if (!required)
		{
			return CELL_HTTP_UTIL_ERROR_NO_BUFFER;
		}
	}

	return CELL_OK;
}

error_code cellHttpUtilCopyHeader(vm::ptr<CellHttpHeader> dest, vm::cptr<CellHttpHeader> src, vm::ptr<void> pool, u32 poolSize, vm::ptr<u32> required)
{
	cellHttpUtil.todo("cellHttpUtilCopyHeader(dest=*0x%x, src=*0x%x, pool=*0x%x, poolSize=%d, required=*0x%x)", dest, src, pool, poolSize, required);

	if (!src)
	{
		return CELL_HTTP_UTIL_ERROR_NO_BUFFER;
	}

	if (!pool || !dest)
	{
		if (!required)
		{
			return CELL_HTTP_UTIL_ERROR_NO_BUFFER;
		}
	}

	return CELL_OK;
}

error_code cellHttpUtilAppendHeaderValue(vm::ptr<CellHttpHeader> dest, vm::cptr<CellHttpHeader> src, vm::cptr<char> value, vm::ptr<void> pool, u32 poolSize, vm::ptr<u32> required)
{
	cellHttpUtil.todo("cellHttpUtilAppendHeaderValue(dest=*0x%x, src=*0x%x, value=%s, pool=*0x%x, poolSize=%d, required=*0x%x)", dest, src, value, pool, poolSize, required);

	if (!src)
	{
		return CELL_HTTP_UTIL_ERROR_NO_BUFFER;
	}

	if (!pool || !dest)
	{
		if (!required)
		{
			return CELL_HTTP_UTIL_ERROR_NO_BUFFER;
		}
	}

	return CELL_OK;
}

error_code cellHttpUtilEscapeUri(vm::ptr<char> out, u32 outSize, vm::cptr<u8> in, u32 inSize, vm::ptr<u32> required)
{
	cellHttpUtil.todo("cellHttpUtilEscapeUri(out=*0x%x, outSize=%d, in=*0x%x, inSize=%d, required=*0x%x)", out, outSize, in, inSize, required);

	if (!in || !inSize)
	{
		return CELL_HTTP_UTIL_ERROR_NO_STRING;
	}

	if (!out && !required)
	{
		return CELL_HTTP_UTIL_ERROR_NO_BUFFER;
	}

	u32 size_needed = 0;
	u32 out_pos = 0;
	s32 rindex = 0;

	if (const u32 end = in.addr() + inSize; end && end >= in.addr())
	{
		rindex = inSize;
	}

	for (u32 pos = 0; rindex >= 0; rindex--, pos++)
	{
		const char c1 = in[pos];

		if (false) // DAT[c1] == '\x03') // TODO
		{
			size_needed += 3;

			if (out)
			{
				if (outSize < size_needed)
				{
					return CELL_HTTP_UTIL_ERROR_NO_MEMORY;
				}

				constexpr const char* chars = "0123456789ABCDEF";
				out[out_pos++] = '%'; // 0x25
				out[out_pos++] = chars[c1 >> 4];
				out[out_pos++] = chars[c1 & 0xf];
			}
		}
		else
		{
			size_needed++;

			if (out)
			{
				if (outSize < size_needed)
				{
					return CELL_HTTP_UTIL_ERROR_NO_MEMORY;
				}

				out[out_pos++] = c1;
			}
		}
	}

	size_needed++;

	if (out)
	{
		if (outSize < size_needed)
		{
			return CELL_HTTP_UTIL_ERROR_NO_MEMORY;
		}

		out[out_pos] = '\0';
	}

	if (required)
	{
		*required = size_needed;
	}

	return CELL_OK;
}

error_code cellHttpUtilUnescapeUri(vm::ptr<u8> out, u32 size, vm::cptr<char> in, vm::ptr<u32> required)
{
	cellHttpUtil.todo("cellHttpUtilUnescapeUri(out=*0x%x, size=%d, in=*0x%x, required=*0x%x)", out, size, in, required);

	if (!in)
	{
		return CELL_HTTP_UTIL_ERROR_NO_STRING;
	}

	if (!out && !required)
	{
		return CELL_HTTP_UTIL_ERROR_NO_BUFFER;
	}

	if (required)
	{
		*required = 0; // TODO
	}

	return CELL_OK;
}

error_code cellHttpUtilFormUrlEncode(vm::ptr<char> out, u32 outSize, vm::cptr<u8> in, u32 inSize, vm::ptr<u32> required)
{
	cellHttpUtil.todo("cellHttpUtilFormUrlEncode(out=*0x%x, outSize=%d, in=*0x%x, inSize=%d, required=*0x%x)", out, outSize, in, inSize, required);

	if (!in || !inSize)
	{
		return CELL_HTTP_UTIL_ERROR_NO_STRING;
	}

	if (!out && !required)
	{
		return CELL_HTTP_UTIL_ERROR_NO_BUFFER;
	}

	u32 size_needed = 0;
	u32 out_pos = 0;
	s32 rindex = 0;

	if (const u32 end = in.addr() + inSize; end && end >= in.addr())
	{
		rindex = inSize;
	}

	for (u32 pos = 0; rindex >= 0; rindex--, pos++)
	{
		const char c1 = in[pos];

		if (c1 == ' ')
		{
			size_needed++;

			if (out)
			{
				if (outSize < size_needed)
				{
					return CELL_HTTP_UTIL_ERROR_NO_MEMORY;
				}

				out[out_pos++] = '+';
			}
		}
		else if (false) // DAT[c1] == '\x03') // TODO
		{
			size_needed += 3;

			if (out)
			{
				if (outSize < size_needed)
				{
					return CELL_HTTP_UTIL_ERROR_NO_MEMORY;
				}

				constexpr const char* chars = "0123456789ABCDEF";
				out[out_pos++] = '%'; // 0x25
				out[out_pos++] = chars[c1 >> 4];
				out[out_pos++] = chars[c1 & 0xf];
			}
		}
		else
		{
			size_needed++;

			if (out)
			{
				if (outSize < size_needed)
				{
					return CELL_HTTP_UTIL_ERROR_NO_MEMORY;
				}

				out[out_pos++] = c1;
			}
		}
	}

	size_needed++;

	if (out)
	{
		if (outSize < size_needed)
		{
			return CELL_HTTP_UTIL_ERROR_NO_MEMORY;
		}

		out[out_pos++] = '\0';
	}

	if (required)
	{
		*required = size_needed;
	}

	return CELL_OK;
}

error_code cellHttpUtilFormUrlDecode(vm::ptr<u8> out, u32 size, vm::cptr<char> in, vm::ptr<u32> required)
{
	cellHttpUtil.todo("cellHttpUtilFormUrlDecode(out=*0x%x, size=%d, in=%s, required=*0x%x)", out, size, in, required);

	if (!in)
	{
		return CELL_HTTP_UTIL_ERROR_NO_STRING;
	}

	if (!out && !required)
	{
		return CELL_HTTP_UTIL_ERROR_NO_BUFFER;
	}

	u32 size_needed = 0;
	u32 out_pos = 0;

	for (s32 index = 0, pos = 0;; index++)
	{
		size_needed = index + 1;
		const char c1 = in[pos++];

		if (!c1)
		{
			break;
		}

		if (out && (size < size_needed))
		{
			return CELL_HTTP_UTIL_ERROR_NO_MEMORY;
		}

		if (c1 == '%')
		{
			const char c2 = in[pos++];
			const char c3 = in[pos++];

			if (!c2 || !c3)
			{
				return CELL_HTTP_UTIL_ERROR_INVALID_URI;
			}

			const auto check_char = [](b8 c)
			{
				const u32 utmp = static_cast<u32>(c);
				s32 stmp = utmp - 48;
				if (static_cast<u8>(c - 48) > 9)
				{
					stmp = utmp - 55;
					if (static_cast<u8>(c + 191) > 5)
					{
						stmp = -1;
						if (static_cast<u8>(c + 159) < 6)
						{
							stmp = utmp - 87;
						}
					}
				}
				return stmp;
			};

			const s32 tmp1 = check_char(c2);
			const s32 tmp2 = check_char(c3);

			if (tmp1 < 0 || tmp2 < 0)
			{
				return CELL_HTTP_UTIL_ERROR_INVALID_URI;
			}

			if (out)
			{
				out[out_pos++] = static_cast<char>((tmp1 & 0xffffffff) << 4) + static_cast<char>(tmp2);
			}
		}
		else
		{
			if (out)
			{
				out[out_pos++] = (c1 == '+' ? ' ' : c1);
			}
		}
	}

	if (out)
	{
		if (size < size_needed)
		{
			return CELL_HTTP_UTIL_ERROR_NO_MEMORY;
		}

		out[out_pos] = '\0';
	}

	if (required)
	{
		*required = size_needed;
	}

	return CELL_OK;
}

error_code cellHttpUtilBase64Encoder(vm::ptr<char> out, vm::cptr<void> input, u32 len)
{
	cellHttpUtil.todo("cellHttpUtilBase64Encoder(out=*0x%x, input=*0x%x, len=%d)", out, input, len);

	if (!input || !len)
	{
		return CELL_HTTP_UTIL_ERROR_NO_STRING;
	}

	if (!out)
	{
		return CELL_HTTP_UTIL_ERROR_NO_BUFFER;
	}

	return CELL_OK;
}

error_code cellHttpUtilBase64Decoder(vm::ptr<char> output, vm::cptr<void> in, u32 len)
{
	cellHttpUtil.todo("cellHttpUtilBase64Decoder(output=*0x%x, in=*0x%x, len=%d)", output, in, len);

	if (!in)
	{
		return CELL_HTTP_UTIL_ERROR_NO_STRING;
	}

	if ((len & 3) != 0)
	{
		return CELL_HTTP_UTIL_ERROR_INVALID_LENGTH;
	}

	if (!output)
	{
		return CELL_HTTP_UTIL_ERROR_NO_BUFFER;
	}

	return CELL_OK;
}

DECLARE(ppu_module_manager::cellHttpUtil)("cellHttpUtil", []()
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
