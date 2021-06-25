#pragma once

#include <cstring>
#include <string>
#include <vector>
#include <functional>
#include <string_view>

#ifdef _WIN32
std::string wchar_to_utf8(wchar_t *src);
std::string wchar_path_to_ansi_path(const std::wstring& src);
std::string utf8_path_to_ansi_path(const std::string& src);
#endif

// Copy null-terminated string from a std::string or a char array to a char array with truncation
template <typename D, typename T>
inline void strcpy_trunc(D& dst, const T& src)
{
	const usz count = std::size(src) >= std::size(dst) ? std::size(dst) - 1 : std::size(src);
	std::memcpy(std::data(dst), std::data(src), count);
	std::memset(std::data(dst) + count, 0, std::size(dst) - count);
}

// Convert string to signed integer
bool try_to_int64(s64* out, const std::string& value, s64 min, s64 max);

// Convert string to unsigned integer
bool try_to_uint64(u64* out, const std::string& value, u64 min, u64 max);

namespace fmt
{
	std::string replace_all(std::string_view src, std::string_view from, std::string_view to, usz count = -1);

	template <usz list_size>
	std::string replace_all(std::string src, const std::pair<std::string, std::string> (&list)[list_size])
	{
		for (usz pos = 0; pos < src.length(); ++pos)
		{
			for (usz i = 0; i < list_size; ++i)
			{
				const usz comp_length = list[i].first.length();

				if (src.length() - pos < comp_length)
					continue;

				if (src.substr(pos, comp_length) == list[i].first)
				{
					src = (pos ? src.substr(0, pos) + list[i].second : list[i].second) + src.substr(pos + comp_length);
					pos += list[i].second.length() - 1;
					break;
				}
			}
		}

		return src;
	}

	template <usz list_size>
	std::string replace_all(std::string src, const std::pair<std::string, std::function<std::string()>> (&list)[list_size])
	{
		for (usz pos = 0; pos < src.length(); ++pos)
		{
			for (usz i = 0; i < list_size; ++i)
			{
				const usz comp_length = list[i].first.length();

				if (src.length() - pos < comp_length)
					continue;

				if (src.substr(pos, comp_length) == list[i].first)
				{
					src = (pos ? src.substr(0, pos) + list[i].second() : list[i].second()) + src.substr(pos + comp_length);
					pos += list[i].second().length() - 1;
					break;
				}
			}
		}

		return src;
	}

	std::vector<std::string> split(std::string_view source, std::initializer_list<std::string_view> separators, bool is_skip_empty = true);
	std::string trim(const std::string& source, const std::string& values = " \t");

	template <typename T>
	std::string merge(const T& source, const std::string& separator)
	{
		if (source.empty())
		{
			return {};
		}

		std::string result;

		auto it  = source.begin();
		auto end = source.end();
		for (--end; it != end; ++it)
		{
			result += std::string{*it} + separator;
		}

		return result + std::string{source.back()};
	}

	template <typename T>
	std::string merge(std::initializer_list<T> sources, const std::string& separator)
	{
		if (!sources.size())
		{
			return {};
		}

		std::string result;
		bool first = true;

		for (auto& v : sources)
		{
			if (first)
			{
				result = fmt::merge(v, separator);
				first  = false;
			}
			else
			{
				result += separator + fmt::merge(v, separator);
			}
		}

		return result;
	}

	std::string to_upper(const std::string& string);
	std::string to_lower(const std::string& string);

	bool match(const std::string& source, const std::string& mask);
}
