#include "StrFmt.h"
#include "StrUtil.h"
#include "cfmt.h"
#include "util/endian.hpp"
#include "util/logs.hpp"
#include "util/v128.hpp"

#include <locale>
#include <codecvt>
#include <algorithm>
#include <string_view>
#include "Thread.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <errno.h>
#endif

std::string wchar_to_utf8(std::wstring_view src)
{
#ifdef _WIN32
	std::string utf8_string;
	const auto tmp_size = WideCharToMultiByte(CP_UTF8, 0, src.data(), src.size(), nullptr, 0, nullptr, nullptr);
	utf8_string.resize(tmp_size);
	WideCharToMultiByte(CP_UTF8, 0, src.data(), src.size(), utf8_string.data(), tmp_size, nullptr, nullptr);
	return utf8_string;
#else
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter{};
	return converter.to_bytes(src.data());
#endif
}

std::string utf16_to_utf8(std::u16string_view src)
{
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter{};
	return converter.to_bytes(src.data());
}

std::u16string utf8_to_utf16(std::string_view src)
{
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter{};
	return converter.from_bytes(src.data());
}

std::wstring utf8_to_wchar(std::string_view src)
{
#ifdef _WIN32
	std::wstring wchar_string;
	const auto tmp_size = MultiByteToWideChar(CP_UTF8, 0, src.data(), src.size(), nullptr, 0);
	wchar_string.resize(tmp_size);
	MultiByteToWideChar(CP_UTF8, 0, src.data(), src.size(), wchar_string.data(), tmp_size);
	return wchar_string;
#else
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter{};
	return converter.from_bytes(src.data());
#endif
}

#ifdef _WIN32
std::string fmt::win_error_to_string(unsigned long error, void* module_handle)
{
	std::string message;
	LPWSTR message_buffer = nullptr;
	if (FormatMessageW((module_handle ? FORMAT_MESSAGE_FROM_HMODULE : FORMAT_MESSAGE_FROM_SYSTEM) | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
			module_handle, error, 0, reinterpret_cast<LPWSTR>(&message_buffer), 0, nullptr))
	{
		message = fmt::format("%s (0x%x)", fmt::trim(wchar_to_utf8(message_buffer), " \t\n\r\f\v"), error);
	}
	else
	{
		message = fmt::format("0x%x", error);
	}

	if (message_buffer)
	{
		LocalFree(message_buffer);
	}

	return message;
}

std::string fmt::win_error_to_string(const fmt::win_error& error)
{
	return fmt::win_error_to_string(error.error, error.module_handle);
}

template <>
void fmt_class_string<fmt::win_error>::format(std::string& out, u64 arg)
{
	fmt::append(out, "%s", fmt::win_error_to_string(get_object(arg)));
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

void fmt_class_string<const wchar_t*>::format(std::string& out, u64 arg)
{
	out += wchar_to_utf8(reinterpret_cast<const wchar_t*>(arg));
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
void fmt_class_string<std::u8string>::format(std::string& out, u64 arg)
{
	const std::u8string& obj = get_object(arg);
	out.append(obj.cbegin(), obj.cend());
}

template <>
void fmt_class_string<std::u8string_view>::format(std::string& out, u64 arg)
{
	const std::u8string_view& obj = get_object(arg);
	out.append(obj.cbegin(), obj.cend());
}

template <>
void fmt_class_string<std::vector<char8_t>>::format(std::string& out, u64 arg)
{
	const std::vector<char8_t>& obj = get_object(arg);
	out.append(obj.cbegin(), obj.cend());
}

template <>
void fmt_class_string<fmt::buf_to_hexstring>::format(std::string& out, u64 arg)
{
	const auto& _arg = get_object(arg);
	const std::vector<u8> buf(_arg.buf, _arg.buf + _arg.len);
	out.reserve(out.size() + (buf.size() * 3));
	static constexpr char hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
	const bool use_linebreak = _arg.line_length > 0;

	for (usz index = 0; index < buf.size(); index++)
	{
		if (index > 0)
		{
			if (use_linebreak && (index % _arg.line_length) == 0)
				out += '\n';
			else
				out += ' ';
		}

		if (_arg.with_prefix)
			out += "0x";

		out += hex[buf[index] >> 4];
		out += hex[buf[index] & 15];
	}
}

void format_byte_array(std::string& out, const uchar* data, usz size)
{
	if (!size)
	{
		out += "{ EMPTY }";
		return;
	}

	out += "{ ";

	for (usz i = 0;; i++)
	{
		if (i == size - 1)
		{
			fmt::append(out, "%02X", data[i]);
			break;
		}

		if ((i % 4) == 3)
		{
			// Place a comma each 4 bytes for ease of byte placement finding
			fmt::append(out, "%02X, ", data[i]);
			continue;
		}

		fmt::append(out, "%02X ", data[i]);
	}

	out += " }";
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

	if (!num)
	{
		out += '0';
		return;
	}

#ifdef _MSC_VER
	fmt::append(out, "0x%016llx%016llx", num.hi, num.lo);
#else
	fmt::append(out, "0x%016llx%016llx", static_cast<u64>(num >> 64), static_cast<u64>(num));
#endif
}

template <>
void fmt_class_string<s128>::format(std::string& out, u64 arg)
{
	return fmt_class_string<u128>::format(out, arg);
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
		fmt::append(out, " (error=%s)", error, fmt::win_error_to_string(error));
	}
#else
	if (int error = errno)
	{
		fmt::append(out, " (errno=%d=%s)", error, strerror(errno));
	}
#endif
}

namespace fmt
{
	[[noreturn]] void raw_verify_error(const src_loc& loc, const char8_t* msg)
	{
		std::string out;
		fmt::append(out, "%s%s", msg ? msg : u8"Verification failed", loc);
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
		TYPE(s128);

#undef TYPE
		if (sup[extra].fmt_string == &fmt_class_string<u128>::format)
			return -1;

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

std::string fmt::replace_all(std::string_view src, std::string_view from, std::string_view to, usz count)
{
	std::string target;
	target.reserve(src.size() + to.size());

	for (usz i = 0, replaced = 0; i < src.size();)
	{
		const usz pos = src.find(from, i);

		if (pos == umax || replaced++ >= count)
		{
			// No match or too many encountered, append the rest of the string as is
			target.append(src.substr(i));
			break;
		}

		// Append source until the matched string position
		target.append(src.substr(i, pos - i));

		// Replace string
		target.append(to);
		i = pos + from.size();
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

std::string fmt::trim(const std::string& source, std::string_view values)
{
	usz begin = source.find_first_not_of(values);

	if (begin == source.npos)
		return {};

	return source.substr(begin, source.find_last_not_of(values) + 1);
}

void fmt::trim_back(std::string& source, std::string_view values)
{
	const usz index = source.find_last_not_of(values);
	source.resize(index + 1);
}

std::string fmt::to_upper(std::string_view string)
{
	std::string result;
	result.resize(string.size());
	std::transform(string.begin(), string.end(), result.begin(), ::toupper);
	return result;
}

std::string fmt::to_lower(std::string_view string)
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

std::string get_file_extension(const std::string& file_path)
{
	if (usz dotpos = file_path.find_last_of('.'); dotpos != std::string::npos && dotpos + 1 < file_path.size())
	{
		return file_path.substr(dotpos + 1);
	}

	return {};
}
