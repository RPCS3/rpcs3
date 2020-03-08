#include "StrFmt.h"
#include "BEType.h"
#include "StrUtil.h"
#include "cfmt.h"
#include "util/logs.hpp"

#include <algorithm>
#include <string_view>
#include "Thread.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <errno.h>
#endif

template <>
void fmt_class_string<std::pair<const fmt_type_info*, u64>>::format(std::string& out, u64 arg)
{
	// Dynamic format arg
	const auto& pair = get_object(arg);

	if (pair.first)
	{
		pair.first->fmt_string(out, pair.second);
	}
}

template <>
void fmt_class_string<fmt::base57>::format(std::string& out, u64 arg)
{
	const auto& _arg = get_object(arg);

	if (_arg.data && _arg.size)
	{
		// Precomputed tail sizes if input data is not multiple of 8
		static constexpr u8 s_tail[8] = {0, 2, 3, 5, 6, 7, 9, 10};

		// Get full output size
		const std::size_t out_size = _arg.size / 8 * 11 + s_tail[_arg.size % 8];

		out.resize(out.size() + out_size);

		const auto ptr = &out.front() + (out.size() - out_size);

		// Each 8 bytes of input data produce 11 bytes of base57 output
		for (std::size_t i = 0, p = 0; i < _arg.size; i += 8, p += 11)
		{
			// Load up to 8 bytes
			be_t<u64> be_value;

			if (_arg.size - i < sizeof(be_value))
			{
				std::memset(&be_value, 0, sizeof(be_value));
				std::memcpy(&be_value, _arg.data + i, _arg.size - i);
			}
			else
			{
				std::memcpy(&be_value, _arg.data + i, sizeof(be_value));
			}

			u64 value = be_value;

			for (int j = 10; j >= 0; j--)
			{
				if (p + j < out_size)
				{
					ptr[p + j] = "0123456789ACEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz"[value % 57];
				}

				value /= 57;
			}
		}
	}
}

void fmt_class_string<const void*>::format(std::string& out, u64 arg)
{
	fmt::append(out, "%p", arg);
}

void fmt_class_string<const char*>::format(std::string& out, u64 arg)
{
	if (arg)
	{
		out += reinterpret_cast<const char*>(arg);
	}
	else
	{
		out += "(NULLSTR)";
	}
}

template <>
void fmt_class_string<std::string>::format(std::string& out, u64 arg)
{
	out += get_object(arg);
}

template <>
void fmt_class_string<std::string_view>::format(std::string& out, u64 arg)
{
	out += get_object(arg);
}

template <>
void fmt_class_string<std::vector<char>>::format(std::string& out, u64 arg)
{
	const std::vector<char>& obj = get_object(arg);
	out.append(obj.cbegin(), obj.cend());
}

template <>
void fmt_class_string<char>::format(std::string& out, u64 arg)
{
	fmt::append(out, "%#hhx", static_cast<char>(arg));
}

template <>
void fmt_class_string<uchar>::format(std::string& out, u64 arg)
{
	fmt::append(out, "%#hhx", static_cast<uchar>(arg));
}

template <>
void fmt_class_string<schar>::format(std::string& out, u64 arg)
{
	fmt::append(out, "%#hhx", static_cast<schar>(arg));
}

template <>
void fmt_class_string<short>::format(std::string& out, u64 arg)
{
	fmt::append(out, "%#hx", static_cast<short>(arg));
}

template <>
void fmt_class_string<ushort>::format(std::string& out, u64 arg)
{
	fmt::append(out, "%#hx", static_cast<ushort>(arg));
}

template <>
void fmt_class_string<int>::format(std::string& out, u64 arg)
{
	fmt::append(out, "%#x", static_cast<int>(arg));
}

template <>
void fmt_class_string<uint>::format(std::string& out, u64 arg)
{
	fmt::append(out, "%#x", static_cast<uint>(arg));
}

template <>
void fmt_class_string<long>::format(std::string& out, u64 arg)
{
	fmt::append(out, "%#lx", static_cast<long>(arg));
}

template <>
void fmt_class_string<ulong>::format(std::string& out, u64 arg)
{
	fmt::append(out, "%#lx", static_cast<ulong>(arg));
}

template <>
void fmt_class_string<llong>::format(std::string& out, u64 arg)
{
	fmt::append(out, "%#llx", static_cast<llong>(arg));
}

template <>
void fmt_class_string<ullong>::format(std::string& out, u64 arg)
{
	fmt::append(out, "%#llx", static_cast<ullong>(arg));
}

template <>
void fmt_class_string<float>::format(std::string& out, u64 arg)
{
	fmt::append(out, "%gf", static_cast<float>(std::bit_cast<f64>(arg)));
}

template <>
void fmt_class_string<double>::format(std::string& out, u64 arg)
{
	fmt::append(out, "%g", std::bit_cast<f64>(arg));
}

template <>
void fmt_class_string<bool>::format(std::string& out, u64 arg)
{
	out += arg ? "true" : "false";
}

template <>
void fmt_class_string<v128>::format(std::string& out, u64 arg)
{
	const v128& vec = get_object(arg);
	fmt::append(out, "0x%016llx%016llx", vec._u64[1], vec._u64[0]);
}

namespace fmt
{
	void raw_error(const char* msg)
	{
		thread_ctrl::emergency_exit(msg);
	}

	void raw_verify_error(const char* msg, const fmt_type_info* sup, u64 arg)
	{
		std::string out{"Verification failed"};

		// Print error code (may be irrelevant)
#ifdef _WIN32
		if (DWORD error = GetLastError())
		{
			fmt::append(out, " (e=%#x)", error);
		}
#else
		if (int error = errno)
		{
			fmt::append(out, " (e=%d)", error);
		}
#endif

		if (sup)
		{
			out += " (";
			sup->fmt_string(out, arg); // Print value
			out += ")";
		}

		if (msg)
		{
			out += ": ";
			out += msg;
		}

		thread_ctrl::emergency_exit(out);
	}

	void raw_narrow_error(const char* msg, const fmt_type_info* sup, u64 arg)
	{
		std::string out{"Narrow error"};

		if (sup)
		{
			out += " (";
			sup->fmt_string(out, arg); // Print value
			out += ")";
		}

		if (msg)
		{
			out += ": ";
			out += msg;
		}

		thread_ctrl::emergency_exit(out);
	}

	void raw_throw_exception(const char* fmt, const fmt_type_info* sup, const u64* args)
	{
		std::string out;
		raw_append(out, fmt, sup, args);
		thread_ctrl::emergency_exit(out);
	}

	struct cfmt_src;
}

// Temporary implementation
struct fmt::cfmt_src
{
	const fmt_type_info* sup;
	const u64* args;

	bool test(std::size_t index) const
	{
		if (!sup[index].fmt_string)
		{
			return false;
		}

		return true;
	}

	template <typename T>
	T get(std::size_t index) const
	{
		return *reinterpret_cast<const T*>(reinterpret_cast<const u8*>(args + index));
	}

	void skip(std::size_t extra)
	{
		sup += extra + 1;
		args += extra + 1;
	}

	std::size_t fmt_string(std::string& out, std::size_t extra) const
	{
		const std::size_t start = out.size();
		sup[extra].fmt_string(out, args[extra]);
		return out.size() - start;
	}

	// Returns type size (0 if unknown, pointer, unsigned, assumed max)
	std::size_t type(std::size_t extra) const
	{
// Hack: use known function pointers to determine type
#define TYPE(type) \
	if (sup[extra].fmt_string == &fmt_class_string<type>::format) return sizeof(type);

		TYPE(int);
		TYPE(llong);
		TYPE(schar);
		TYPE(short);
		if (std::is_signed<char>::value) TYPE(char);
		TYPE(long);

#undef TYPE

		return 0;
	}

	static constexpr std::size_t size_char  = 1;
	static constexpr std::size_t size_short = 2;
	static constexpr std::size_t size_int   = 0;
	static constexpr std::size_t size_long  = sizeof(ulong);
	static constexpr std::size_t size_llong = sizeof(ullong);
	static constexpr std::size_t size_size  = sizeof(std::size_t);
	static constexpr std::size_t size_max   = sizeof(std::uintmax_t);
	static constexpr std::size_t size_diff  = sizeof(std::ptrdiff_t);
};

void fmt::raw_append(std::string& out, const char* fmt, const fmt_type_info* sup, const u64* args) noexcept
{
	cfmt_append(out, fmt, cfmt_src{sup, args});
}

std::string fmt::replace_first(const std::string& src, const std::string& from, const std::string& to)
{
	auto pos = src.find(from);

	if (pos == umax)
	{
		return src;
	}

	return (pos ? src.substr(0, pos) + to : to) + std::string(src.c_str() + pos + from.length());
}

std::string fmt::replace_all(const std::string& src, const std::string& from, const std::string& to)
{
	std::string target = src;
	for (auto pos = target.find(from); pos != umax; pos = target.find(from, pos + 1))
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
		for (auto& separator : separators)
		{
			if (strncmp(source.c_str() + cursor_end, separator.c_str(), separator.length()) == 0)
			{
				std::string candidate = source.substr(cursor_begin, cursor_end - cursor_begin);
				if (!is_skip_empty || !candidate.empty())
					result.push_back(candidate);

				cursor_begin = cursor_end + separator.length();
				cursor_end   = cursor_begin - 1;
				break;
			}
		}
	}

	if (cursor_begin != source.length())
	{
		result.push_back(source.substr(cursor_begin));
	}

	return result;
}

std::string fmt::trim(const std::string& source, const std::string& values)
{
	std::size_t begin = source.find_first_not_of(values);

	if (begin == source.npos)
		return {};

	return source.substr(begin, source.find_last_not_of(values) + 1);
}

std::string fmt::to_upper(const std::string& string)
{
	std::string result;
	result.resize(string.size());
	std::transform(string.begin(), string.end(), result.begin(), ::toupper);
	return result;
}

std::string fmt::to_lower(const std::string& string)
{
	std::string result;
	result.resize(string.size());
	std::transform(string.begin(), string.end(), result.begin(), ::tolower);
	return result;
}

bool fmt::match(const std::string& source, const std::string& mask)
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
