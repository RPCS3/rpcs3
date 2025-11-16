#include "stdafx.h"
#include "overlay_utils.h"

#include <unordered_map>

LOG_CHANNEL(overlays);

static auto s_ascii_lowering_map = []()
{
	std::unordered_map<u32, u8> _map;

	// Fullwidth block (FF00-FF5E)
	for (u32 u = 0xFF01, c = 0x21; u <= 0xFF5E; ++u, ++c)
	{
		_map[u] = u8(c);
	}

	// Em and En space variations (General Punctuation)
	for (u32 u = 0x2000; u <= 0x200A; ++u)
	{
		_map[u] = u8(' ');
	}

	// Misc space variations
	_map[0x202F] = u8(0xA0); // narrow NBSP
	_map[0x205F] = u8(' ');  // medium mathematical space
	_map[0x3164] = u8(' ');  // hangul filler

	// Ideographic (CJK punctuation)
	_map[0x3000] = u8(' ');  // space
	_map[0x3001] = u8(',');  // comma
	_map[0x3002] = u8('.');  // fullstop
	_map[0x3003] = u8('"');  // ditto
	_map[0x3007] = u8('0');  // wide zero
	_map[0x3008] = u8('<');  // left angle brace
	_map[0x3009] = u8('>');  // right angle brace
	_map[0x300A] = u8(0xAB); // double left angle brace
	_map[0x300B] = u8(0xBB); // double right angle brace
	_map[0x300C] = u8('[');  // the following are all slight variations on the angular brace
	_map[0x300D] = u8(']');
	_map[0x300E] = u8('[');
	_map[0x300F] = u8(']');
	_map[0x3010] = u8('[');
	_map[0x3011] = u8(']');
	_map[0x3014] = u8('[');
	_map[0x3015] = u8(']');
	_map[0x3016] = u8('[');
	_map[0x3017] = u8(']');
	_map[0x3018] = u8('[');
	_map[0x3019] = u8(']');
	_map[0x301A] = u8('[');
	_map[0x301B] = u8(']');
	_map[0x301C] = u8('~');  // wave dash (inverted tilde)
	_map[0x301D] = u8('"');  // reverse double prime quotation
	_map[0x301E] = u8('"');  // double prime quotation
	_map[0x301F] = u8('"');  // low double prime quotation
	_map[0x3031] = u8('<');  // vertical kana repeat mark

	return _map;
}();

template<typename F>
void process_multibyte(const std::string& s, F&& func)
{
	const usz end = s.length();
	for (usz index = 0; index < end; ++index)
	{
		const u8 code = static_cast<u8>(s[index]);

		if (!code)
		{
			break;
		}

		if (code <= 0x7F)
		{
			std::invoke(func, code);
			continue;
		}

		const u32 extra_bytes = (code <= 0xDF) ? 1u : (code <= 0xEF) ? 2u : 3u;
		if ((index + extra_bytes) > end)
		{
			// Malformed string, abort
			overlays.error("Failed to decode supossedly malformed utf8 string '%s'", s);
			break;
		}

		u32 u_code = 0;
		switch (extra_bytes)
		{
		case 1:
			// 11 bits, 6 + 5
			u_code = (u32(code & 0x1F) << 6) | u32(s[index + 1] & 0x3F);
			break;
		case 2:
			// 16 bits, 6 + 6 + 4
			u_code = (u32(code & 0xF) << 12) | (u32(s[index + 1] & 0x3F) << 6) | u32(s[index + 2] & 0x3F);
			break;
		case 3:
			// 21 bits, 6 + 6 + 6 + 3
			u_code = (u32(code & 0x7) << 18) | (u32(s[index + 1] & 0x3F) << 12) | (u32(s[index + 2] & 0x3F) << 6) | u32(s[index + 3] & 0x3F);
			break;
		default:
			fmt::throw_exception("Unreachable");
		}

		index += extra_bytes;
		std::invoke(func, u_code);
	}
}

std::string utf8_to_ascii8(const std::string& utf8_string)
{
	std::string out;
	out.reserve(utf8_string.length());

	process_multibyte(utf8_string, [&out](u32 code)
	{
		if (code <= 0x7F)
		{
			out.push_back(static_cast<u8>(code));
		}
		else if (auto replace = s_ascii_lowering_map.find(code);
			replace == s_ascii_lowering_map.end())
		{
			out.push_back('#');
		}
		else
		{
			out.push_back(replace->second);
		}
	});

	return out;
}

std::string utf16_to_ascii8(const std::u16string& utf16_string)
{
	// Strip extended codes, map to '#' instead (placeholder)
	std::string out;
	out.reserve(utf16_string.length());

	for (const auto& code : utf16_string)
	{
		if (!code)
			break;

		out.push_back(code > 0xFF ? '#' : static_cast<char>(code));
	}

	return out;
}

std::u16string ascii8_to_utf16(const std::string& ascii_string)
{
	std::u16string out;
	out.reserve(ascii_string.length());

	for (const auto& code : ascii_string)
	{
		if (!code)
			break;

		out.push_back(static_cast<char16_t>(code));
	}

	return out;
}

std::u32string utf8_to_u32string(const std::string& utf8_string)
{
	std::u32string result;
	result.reserve(utf8_string.size());

	process_multibyte(utf8_string, [&result](u32 code)
	{
		result.push_back(static_cast<char32_t>(code));
	});

	return result;
}

std::u16string u32string_to_utf16(const std::u32string& utf32_string)
{
	std::u16string result;
	result.reserve(utf32_string.size());

	for (const auto& code : utf32_string)
	{
		result.push_back(static_cast<char16_t>(code));
	}

	return result;
}

std::u32string utf16_to_u32string(const std::u16string& utf16_string)
{
	std::u32string result;
	result.reserve(utf16_string.size());

	for (const auto& code : utf16_string)
	{
		result.push_back(static_cast<char32_t>(code));
	}

	return result;
}
