#include "StrFmt.h"
#include "BEType.h"
#include "StrUtil.h"

#include <cassert>
#include <array>
#include <memory>
#include <algorithm>

std::string v128::to_hex() const
{
	return fmt::format("%016llx%016llx", _u64[1], _u64[0]);
}

std::string v128::to_xyzw() const
{
	return fmt::format("x: %g y: %g z: %g w: %g", _f[3], _f[2], _f[1], _f[0]);
}

std::string fmt::unsafe_vformat(const char* fmt, va_list _args) noexcept
{
	// Fixed stack buffer for the first attempt
	std::array<char, 4096> fixed_buf;

	// Possibly dynamically allocated buffer for the second attempt
	std::unique_ptr<char[]> buf;

	// Pointer to the current buffer
	char* buf_addr = fixed_buf.data();

	for (std::size_t buf_size = fixed_buf.size();;)
	{
		va_list args;
		va_copy(args, _args);

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
#endif
		const std::size_t len = std::vsnprintf(buf_addr, buf_size, fmt, args);
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
		va_end(args);

		assert(len <= INT_MAX);

		if (len < buf_size)
		{
			return{ buf_addr, len };
		}

		buf.reset(buf_addr = new char[buf_size = len + 1]);
	}
}

std::string fmt::unsafe_format(const char* fmt...) noexcept
{
	va_list args;
	va_start(args, fmt);
	auto result = unsafe_vformat(fmt, args);
	va_end(args);

	return result;
}

fmt::exception_base::exception_base(const char* fmt...)
	: std::runtime_error((va_start(m_args, fmt), unsafe_vformat(fmt, m_args)))
{
	va_end(m_args);
}

std::string fmt::replace_first(const std::string& src, const std::string& from, const std::string& to)
{
	auto pos = src.find(from);

	if (pos == std::string::npos)
	{
		return src;
	}

	return (pos ? src.substr(0, pos) + to : to) + std::string(src.c_str() + pos + from.length());
}

std::string fmt::replace_all(const std::string &src, const std::string& from, const std::string& to)
{
	std::string target = src;
	for (auto pos = target.find(from); pos != std::string::npos; pos = target.find(from, pos + 1))
	{
		target = (pos ? target.substr(0, pos) + to : to) + std::string(target.c_str() + pos + from.length());
		pos += to.length();
	}

	return target;
}

std::vector<std::string> fmt::split(const std::string& source, std::initializer_list<std::string> separators, bool is_skip_empty)
{
	std::vector<std::string> result;

	size_t cursor_begin = 0;

	for (size_t cursor_end = 0; cursor_end < source.length(); ++cursor_end)
	{
		for (auto &separator : separators)
		{
			if (strncmp(source.c_str() + cursor_end, separator.c_str(), separator.length()) == 0)
			{
				std::string candidate = source.substr(cursor_begin, cursor_end - cursor_begin);
				if (!is_skip_empty || !candidate.empty())
					result.push_back(candidate);

				cursor_begin = cursor_end + separator.length();
				cursor_end = cursor_begin - 1;
				break;
			}
		}
	}

	if (cursor_begin != source.length())
	{
		result.push_back(source.substr(cursor_begin));
	}

	return std::move(result);
}

std::string fmt::trim(const std::string& source, const std::string& values)
{
	std::size_t begin = source.find_first_not_of(values);

	if (begin == source.npos)
		return{};

	return source.substr(begin, source.find_last_not_of(values) + 1);
}

std::string fmt::to_upper(const std::string& string)
{
	std::string result;
	result.resize(string.size());
	std::transform(string.begin(), string.end(), result.begin(), ::toupper);
	return result;
}

bool fmt::match(const std::string &source, const std::string &mask)
{
	std::size_t source_position = 0, mask_position = 0;

	for (; source_position < source.size() && mask_position < mask.size(); ++mask_position, ++source_position)
	{
		switch (mask[mask_position])
		{
		case '?': break;

		case '*':
			for (std::size_t test_source_position = source_position; test_source_position < source.size(); ++test_source_position)
			{
				if (match(source.substr(test_source_position), mask.substr(mask_position + 1)))
				{
					return true;
				}
			}
			return false;

		default:
			if (source[source_position] != mask[mask_position])
			{
				return false;
			}

			break;
		}
	}

	if (source_position != source.size())
		return false;

	if (mask_position != mask.size())
		return false;

	return true;
}
