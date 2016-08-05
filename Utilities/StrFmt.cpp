#include "StrFmt.h"
#include "BEType.h"
#include "StrUtil.h"
#include "Macro.h"
#include "cfmt.h"

#include <algorithm>

void fmt_class_string<const void*>::format(std::string& out, u64 arg)
{
	fmt::append(out, "%p", reinterpret_cast<const void*>(static_cast<std::uintptr_t>(arg)));
}

template<>
void fmt_class_string<std::nullptr_t>::format(std::string& out, u64 arg)
{
	fmt::append(out, "%p", reinterpret_cast<const void*>(static_cast<std::uintptr_t>(arg)));
}

void fmt_class_string<const char*>::format(std::string& out, u64 arg)
{
	out += reinterpret_cast<const char*>(static_cast<std::uintptr_t>(arg));
}

template<>
void fmt_class_string<std::string>::format(std::string& out, u64 arg)
{
	out += get_object(arg).c_str(); // TODO?
}

template<>
void fmt_class_string<std::vector<char>>::format(std::string& out, u64 arg)
{
	const std::vector<char>& obj = get_object(arg);
	out.append(obj.cbegin(), obj.cend());
}

template<>
void fmt_class_string<char>::format(std::string& out, u64 arg)
{
	fmt::append(out, "0x%hhx", static_cast<char>(arg));
}

template<>
void fmt_class_string<uchar>::format(std::string& out, u64 arg)
{
	fmt::append(out, "0x%hhx", static_cast<uchar>(arg));
}

template<>
void fmt_class_string<schar>::format(std::string& out, u64 arg)
{
	fmt::append(out, "0x%hhx", static_cast<schar>(arg));
}

template<>
void fmt_class_string<short>::format(std::string& out, u64 arg)
{
	fmt::append(out, "0x%hx", static_cast<short>(arg));
}

template<>
void fmt_class_string<ushort>::format(std::string& out, u64 arg)
{
	fmt::append(out, "0x%hx", static_cast<ushort>(arg));
}

template<>
void fmt_class_string<int>::format(std::string& out, u64 arg)
{
	fmt::append(out, "0x%x", static_cast<int>(arg));
}

template<>
void fmt_class_string<uint>::format(std::string& out, u64 arg)
{
	fmt::append(out, "0x%x", static_cast<uint>(arg));
}

template<>
void fmt_class_string<long>::format(std::string& out, u64 arg)
{
	fmt::append(out, "0x%lx", static_cast<long>(arg));
}

template<>
void fmt_class_string<ulong>::format(std::string& out, u64 arg)
{
	fmt::append(out, "0x%lx", static_cast<ulong>(arg));
}

template<>
void fmt_class_string<llong>::format(std::string& out, u64 arg)
{
	fmt::append(out, "0x%llx", static_cast<llong>(arg));
}

template<>
void fmt_class_string<ullong>::format(std::string& out, u64 arg)
{
	fmt::append(out, "0x%llx", static_cast<ullong>(arg));
}

template<>
void fmt_class_string<float>::format(std::string& out, u64 arg)
{
	fmt::append(out, "%gf", static_cast<float>(reinterpret_cast<f64&>(arg)));
}

template<>
void fmt_class_string<double>::format(std::string& out, u64 arg)
{
	fmt::append(out, "%g", reinterpret_cast<f64&>(arg));
}

template<>
void fmt_class_string<bool>::format(std::string& out, u64 arg)
{
	out += arg ? "true" : "false"; // TODO?
}

template<>
void fmt_class_string<v128>::format(std::string& out, u64 arg)
{
	const v128& vec = get_object(arg);
	fmt::append(out, "0x%016llx%016llx", vec._u64[1], vec._u64[0]);
}

namespace fmt
{
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

	template<typename T>
	T get(std::size_t index) const
	{
		return reinterpret_cast<const T&>(args[index]);
	}

	void skip(std::size_t extra)
	{
		++sup += extra;
		++args += extra;
	}

	std::size_t fmt_string(std::string& out, std::size_t extra) const
	{
		const std::size_t start = out.size();
		sup[extra].fmt_string(out, args[extra]);
		return out.size() - start;
	}

	// Returns type size (0 if unknown, pointer, assumed max)
	std::size_t type(std::size_t extra) const
	{
		// Hack: use known function pointers to determine type
#define TYPE(type)\
		if (sup[extra].fmt_string == &fmt_class_string<type>::format) return sizeof(type);

		TYPE(char);
		TYPE(schar);
		TYPE(uchar);
		TYPE(short);
		TYPE(ushort);
		TYPE(int);
		TYPE(uint);
		TYPE(long);
		TYPE(ulong);
		TYPE(llong);
		TYPE(ullong);

#undef TYPE

		return 0;
	}
};

void fmt::raw_append(std::string& out, const char* fmt, const fmt_type_info* sup, const u64* args) noexcept
{
	cfmt_append(out, fmt, cfmt_src{sup, args});
}

char* fmt::alloc_format(const char* fmt, const fmt_type_info* sup, const u64* args) noexcept
{
	std::string str;
	raw_append(str, fmt, sup, args);
	return static_cast<char*>(std::memcpy(std::malloc(str.size() + 1), str.data(), str.size() + 1));
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
