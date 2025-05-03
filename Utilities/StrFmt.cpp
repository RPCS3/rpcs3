#include "StrFmt.h"
#include "StrUtil.h"
#include "cfmt.h"
#include "util/endian.hpp"
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

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
std::string wchar_to_utf8(std::wstring_view src)
{
#ifdef _WIN32
	std::string utf8_string;
	const int size = ::narrow<int>(src.size());
	const auto tmp_size = WideCharToMultiByte(CP_UTF8, 0, src.data(), size, nullptr, 0, nullptr, nullptr);
	utf8_string.resize(tmp_size);
	WideCharToMultiByte(CP_UTF8, 0, src.data(), size, utf8_string.data(), tmp_size, nullptr, nullptr);
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
	const int size = ::narrow<int>(src.size());
	const auto tmp_size = MultiByteToWideChar(CP_UTF8, 0, src.data(), size, nullptr, 0);
	wchar_string.resize(tmp_size);
	MultiByteToWideChar(CP_UTF8, 0, src.data(), size, wchar_string.data(), tmp_size);
	return wchar_string;
#else
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter{};
	return converter.from_bytes(src.data());
#endif
}
#ifdef _MSC_VER
#pragma warning(pop)
#elif defined(__clang__)
#pragma clang diagnostic pop
#else
#pragma GCC diagnostic pop
#endif

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

fmt::base57_result fmt::base57_result::from_string(std::string_view str)
{
	fmt::base57_result result(str.size() / 11 * 8 + (str.size() % 11 ? 8 : 0));

	// Each 11 chars of input produces 8 bytes of byte output
	for (usz i = 0, p = 0; i < result.size; i += 8, p += 11)
	{
		// Load up to 8 bytes
		const std::string_view be_value = str.substr(p);
		be_t<u64> value = 0;

		for (u64 j = 10, multiplier = 0; j != umax; j--)
		{
			if (multiplier == 0)
			{
				multiplier = 1;
			}
			else
			{
				// Do it first to avoid overflow
				multiplier *= 57;
			}

			if (j < be_value.size())
			{
				auto to_val = [](u8 c) -> u64
				{
					if (std::isdigit(c))
					{
						return c - '0';
					}

					if (std::isupper(c))
					{
						// Omitted characters
						if (c == 'B' || c == 'D' || c == 'I' || c == 'O')
						{
							return umax;
						}

						if (c > 'O')
						{
							c -= 4;
						}
						else if (c > 'I')
						{
							c -= 3;
						}
						else if (c > 'D')
						{
							c -= 2;
						}
						else if (c > 'B')
						{
							c--;
						}

						return c - 'A' + 10;
					}

					if (std::islower(c))
					{
						// Omitted characters
						if (c == 'l')
						{
							return umax;
						}

						if (c > 'l')
						{
							c--;
						}

						return c - 'a' + 10 + 22;
					}

					return umax;
				};

				const u64 res = to_val(be_value[j]);

				if (res == umax)
				{
					// Invalid input character
					result = {};
					break;
				}

				if (u64{umax} / multiplier < res)
				{
					// Overflow
					result = {};
					break;
				}

				const u64 addend = res * multiplier;

				if (~value < addend)
				{
					// Overflow
					result = {};
					break;
				}

				value += addend;
			}
		}

		if (!result.size)
		{
			break;
		}

		if (result.size - i < sizeof(value))
		{
			std::memcpy(result.memory.get() + i, &value, result.size - i);
		}
		else
		{
			std::memcpy(result.memory.get() + i, &value, sizeof(value));
		}
	}

	return result;
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
void fmt_class_string<std::source_location>::format(std::string& out, u64 arg)
{
	const std::source_location& loc = get_object(arg);

	auto is_valid = [](auto num)
	{
		return num && num != umax;
	};

	const bool has_line = is_valid(loc.line());

	if (has_line && is_valid(loc.column()))
	{
		fmt::append(out, "\n(in file %s:%u[:%u]", loc.file_name(), loc.line(), loc.column());
	}
	else if (has_line)
	{
		fmt::append(out, "\n(in file %s:%u", loc.file_name(), loc.line());
	}
	// Let's not care about such useless corner cases
	// else if (is_valid(loc.column())
	else
	{
		fmt::append(out, "\n(in file %s", loc.file_name());
	}

	if (std::string_view full_func{loc.function_name() ? loc.function_name() : ""}; !full_func.empty())
	{
		// Remove useless disambiguators
		std::string func = fmt::replace_all(std::string(full_func), {
			{"struct ", ""},
			{"class ", ""},
			{"enum ", ""},
			{"typename ", ""},
#ifdef _MSC_VER
			{"__cdecl ", ""},
#endif
			{"unsigned long long", "ullong"},
			//{"unsigned long", "ulong"}, // ullong
			{"unsigned int", "uint"},
			{"unsigned short", "ushort"},
			{"unsigned char", "uchar"}});

		// Remove function argument signature for long names
		for (usz index = func.find_first_of('('); index != umax && func.size() >= 100u; index = func.find_first_of('(', index))
		{
			// Operator() function
			if (func.compare(0, 3, "()("sv) == 0 || func.compare(0, 3, "() "sv))
			{
				if (usz not_space = func.find_first_not_of(' ', index + 2); not_space != umax && func[not_space] == '(')
				{
					index += 2;
					continue;
				}
			}

			func = func.substr(0, index) + "()";
			break;
		}

		fmt::append(out, ", in function '%s')", func);
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
	[[noreturn]] void raw_verify_error(std::source_location loc, const char8_t* msg, usz object)
	{
		std::string out;
		fmt::append(out, "%s (object: 0x%x)%s", msg ? msg : u8"Verification failed", object, loc);
		thread_ctrl::emergency_exit(out);
	}

	[[noreturn]] void raw_range_error(std::source_location loc, std::string_view index, usz container_size)
	{
		std::string out;

		if (container_size != umax)
		{
			fmt::append(out, "Range check failed (index: %s, container_size: %u)%s", index, container_size, loc);
		}
		else
		{
			fmt::append(out, "Range check failed (index: %s)%s", index, loc);
		}

		thread_ctrl::emergency_exit(out);
	}

	[[noreturn]] void raw_range_error(std::source_location loc, usz index, usz container_size)
	{
		std::string out;

		if (container_size != umax)
		{
			fmt::append(out, "Range check failed (index: %u, container_size: %u)%s", index, container_size, loc);
		}
		else
		{
			fmt::append(out, "Range check failed (index: %u)%s", index, loc);
		}

		thread_ctrl::emergency_exit(out);
	}

	[[noreturn]] void raw_throw_exception(std::source_location loc, const char* fmt, const fmt_type_info* sup, const u64* args)
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
		if constexpr (std::is_signed_v<char>) TYPE(char);
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
	if (src.empty())
		return {};

	if (from.empty() || count == 0)
		return std::string(src);

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
	const usz begin = source.find_first_not_of(values);

	if (begin == source.npos)
		return {};

	const usz end = source.find_last_not_of(values);

	if (end == source.npos)
		return source.substr(begin);

	return source.substr(begin, end + 1 - begin);
}

std::string fmt::trim_front(const std::string& source, std::string_view values)
{
	const usz begin = source.find_first_not_of(values);

	if (begin == source.npos)
		return {};

	return source.substr(begin);
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

std::string fmt::truncate(std::string_view src, usz length)
{
	return std::string(src.begin(), src.begin() + std::min(src.size(), length));
}

std::string get_file_extension(const std::string& file_path)
{
	if (usz dotpos = file_path.find_last_of('.'); dotpos != std::string::npos && dotpos + 1 < file_path.size())
	{
		return file_path.substr(dotpos + 1);
	}

	return {};
}
