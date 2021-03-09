#include "StrFmt.h"
#include "StrUtil.h"
#include "cfmt.h"
#include "util/endian.hpp"
#include "util/logs.hpp"
#include "util/v128.hpp"

#include <algorithm>
#include <string_view>
#include "Thread.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <errno.h>
#endif

#ifdef _WIN32
std::string wchar_to_utf8(wchar_t *src)
{
	std::string utf8_string;
	const auto tmp_size = WideCharToMultiByte(CP_UTF8, 0, src, -1, nullptr, 0, nullptr, nullptr);
	utf8_string.resize(tmp_size);
	WideCharToMultiByte(CP_UTF8, 0, src, -1, utf8_string.data(), tmp_size, nullptr, nullptr);
	return utf8_string;
}

std::string wchar_path_to_ansi_path(const std::wstring& src)
{
	std::wstring buf_short;
	std::string buf_final;

	// Get the short path from the wide char path(short path should only contain ansi characters)
	auto tmp_size = GetShortPathNameW(src.data(), nullptr, 0);
	buf_short.resize(tmp_size);
	GetShortPathNameW(src.data(), buf_short.data(), tmp_size);

	// Convert wide char to ansi
	tmp_size = WideCharToMultiByte(CP_ACP, 0, buf_short.data(), -1, nullptr, 0, nullptr, nullptr);
	buf_final.resize(tmp_size);
	WideCharToMultiByte(CP_ACP, 0, buf_short.data(), -1, buf_final.data(), tmp_size, nullptr, nullptr);

	return buf_final;
}

std::string utf8_path_to_ansi_path(const std::string& src)
{
	std::wstring buf_wide;

	// Converts the utf-8 path to wide char
	const auto tmp_size = MultiByteToWideChar(CP_UTF8, 0, src.c_str(), -1, nullptr, 0);
	buf_wide.resize(tmp_size);
	MultiByteToWideChar(CP_UTF8, 0, src.c_str(), -1, buf_wide.data(), tmp_size);

	return wchar_path_to_ansi_path(buf_wide);
}
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
		const usz out_size = _arg.size / 8 * 11 + s_tail[_arg.size % 8];

		out.resize(out.size() + out_size);

		const auto ptr = &out.front() + (out.size() - out_size);

		// Each 8 bytes of input data produce 11 bytes of base57 output
		for (usz i = 0, p = 0; i < _arg.size; i += 8, p += 11)
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
void fmt_class_string<b8>::format(std::string& out, u64 arg)
{
	out += get_object(arg) ? "true" : "false";
}

template <>
void fmt_class_string<v128>::format(std::string& out, u64 arg)
{
	const v128& vec = get_object(arg);
	fmt::append(out, "0x%016llx%016llx", vec._u64[1], vec._u64[0]);
}

template <>
void fmt_class_string<u128>::format(std::string& out, u64 arg)
{
	// TODO: it should be supported as full-fledged integral type (with %u, %d, etc, fmt)
	const u128& num = get_object(arg);
#ifdef _MSC_VER
	fmt::append(out, "0x%016llx%016llx", num.hi, num.lo);
#else
	fmt::append(out, "0x%016llx%016llx", static_cast<u64>(num >> 64), static_cast<u64>(num));
#endif
}

template <>
void fmt_class_string<src_loc>::format(std::string& out, u64 arg)
{
	const src_loc& loc = get_object(arg);

	if (loc.col != umax)
	{
		fmt::append(out, "\n(in file %s:%u[:%u]", loc.file, loc.line, loc.col);
	}
	else
	{
		fmt::append(out, "\n(in file %s:%u", loc.file, loc.line);
	}

	if (loc.func && *loc.func)
	{
		fmt::append(out, ", in function %s)", loc.func);
	}
	else
	{
		out += ')';
	}

	// Print error code (may be irrelevant)
#ifdef _WIN32
	if (DWORD error = GetLastError())
	{
		fmt::append(out, " (e=0x%08x[%u])", error, error);
	}
#else
	if (int error = errno)
	{
		fmt::append(out, " (errno=%d)", error);
	}
#endif
}

namespace fmt
{
	[[noreturn]] void raw_verify_error(const src_loc& loc)
	{
		std::string out{"Verification failed"};
		fmt::append(out, "%s", loc);
		thread_ctrl::emergency_exit(out);
	}

	[[noreturn]] void raw_narrow_error(const src_loc& loc)
	{
		std::string out{"Narrowing error"};
		fmt::append(out, "%s", loc);
		thread_ctrl::emergency_exit(out);
	}

	[[noreturn]] void raw_throw_exception(const src_loc& loc, const char* fmt, const fmt_type_info* sup, const u64* args)
	{
		std::string out;
		raw_append(out, fmt, sup, args);
		fmt::append(out, "%s", loc);
		thread_ctrl::emergency_exit(out);
	}

	struct cfmt_src;
}

// Temporary implementation
struct fmt::cfmt_src
{
	const fmt_type_info* sup;
	const u64* args;

	bool test(usz index) const
	{
		if (!sup[index].fmt_string)
		{
			return false;
		}

		return true;
	}

	template <typename T>
	T get(usz index) const
	{
		T res{};
		std::memcpy(&res, reinterpret_cast<const u8*>(args + index), sizeof(res));
		return res;
	}

	void skip(usz extra)
	{
		sup += extra + 1;
		args += extra + 1;
	}

	usz fmt_string(std::string& out, usz extra) const
	{
		const usz start = out.size();
		sup[extra].fmt_string(out, args[extra]);
		return out.size() - start;
	}

	// Returns type size (0 if unknown, pointer, unsigned, assumed max)
	usz type(usz extra) const
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

	static constexpr usz size_char  = 1;
	static constexpr usz size_short = 2;
	static constexpr usz size_int   = 0;
	static constexpr usz size_long  = sizeof(ulong);
	static constexpr usz size_llong = sizeof(ullong);
	static constexpr usz size_size  = sizeof(usz);
	static constexpr usz size_max   = sizeof(std::uintmax_t);
	static constexpr usz size_diff  = sizeof(std::ptrdiff_t);
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

std::vector<std::string> fmt::split(std::string_view source, std::initializer_list<std::string_view> separators, bool is_skip_empty)
{
	std::vector<std::string> result;

	for (usz index = 0; index < source.size();)
	{
		usz pos = -1;
		usz sep_size = 0;

		for (auto& separator : separators)
		{
			if (usz pos0 = source.find(separator, index); pos0 < pos)
			{
				pos = pos0;
				sep_size = separator.size();
			}
		}

		if (!sep_size)
		{
			result.emplace_back(&source[index], source.size() - index);
			return result;
		}

		std::string_view piece = {&source[index], pos - index};

		index = pos + sep_size;

		if (piece.empty() && is_skip_empty)
		{
			continue;
		}

		result.emplace_back(std::string(piece));
	}

	if (result.empty() && !is_skip_empty)
	{
		result.emplace_back();
	}

	return result;
}

std::string fmt::trim(const std::string& source, const std::string& values)
{
	usz begin = source.find_first_not_of(values);

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
	usz source_position = 0, mask_position = 0;

	for (; source_position < source.size() && mask_position < mask.size(); ++mask_position, ++source_position)
	{
		switch (mask[mask_position])
		{
		case '?': break;

		case '*':
			for (usz test_source_position = source_position; test_source_position < source.size(); ++test_source_position)
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
