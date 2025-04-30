#pragma once

#include <cstring>
#include <string>
#include <vector>
#include <functional>
#include <string_view>

#include "util/types.hpp"

std::wstring utf8_to_wchar(std::string_view src);
std::string wchar_to_utf8(std::wstring_view src);
std::string utf16_to_utf8(std::u16string_view src);
std::u16string utf8_to_utf16(std::string_view src);

// Copy null-terminated string from a std::string or a char array to a char array with truncation
template <typename D, typename T>
inline void strcpy_trunc(D&& dst, const T& src)
{
	const usz count = std::size(src) >= std::size(dst) ? std::max<usz>(std::size(dst), 1) - 1 : std::size(src);
	std::memcpy(std::data(dst), std::data(src), count);
	std::memset(std::data(dst) + count, 0, std::size(dst) - count);
}

// Convert string to signed integer
bool try_to_int64(s64* out, std::string_view value, s64 min, s64 max);

// Convert string to unsigned integer
bool try_to_uint64(u64* out, std::string_view value, u64 min, u64 max);

// Convert string to float
bool try_to_float(f64* out, std::string_view value, f64 min, f64 max);

// Convert float to string locale independent
bool try_to_string(std::string* out, const f64& value);

// Get the file extension of a file path ("png", "jpg", etc.)
std::string get_file_extension(const std::string& file_path);

namespace fmt
{
	// Replaces all occurrences of 'from' with 'to' until 'count' substrings were replaced.
	std::string replace_all(std::string_view src, std::string_view from, std::string_view to, usz count = umax);

	template <usz list_size>
	std::string replace_all(std::string src, const std::pair<std::string_view, std::string> (&list)[list_size])
	{
		if constexpr (list_size == 0)
			return src;

		for (usz pos = 0; pos < src.length(); ++pos)
		{
			for (usz i = 0; i < list_size; ++i)
			{
				const usz comp_length = list[i].first.length();

				if (src.length() - pos < comp_length)
				{
					continue;
				}

				if (src.substr(pos, comp_length) == list[i].first)
				{
					src.erase(pos, comp_length);
					src.insert(pos, list[i].second.data(), list[i].second.length());
					pos += list[i].second.length() - 1;
					break;
				}
			}
		}

		return src;
	}

	template <usz list_size>
	std::string replace_all(std::string src, const std::pair<std::string_view, std::function<std::string()>> (&list)[list_size])
	{
		if constexpr (list_size == 0)
			return src;

		for (usz pos = 0; pos < src.length(); ++pos)
		{
			for (usz i = 0; i < list_size; ++i)
			{
				const usz comp_length = list[i].first.length();

				if (src.length() - pos < comp_length)
				{
					continue;
				}

				if (src.substr(pos, comp_length) == list[i].first)
				{
					src.erase(pos, comp_length);
					auto replacement = list[i].second();
					src.insert(pos, replacement);
					pos += replacement.length() - 1;
					break;
				}
			}
		}

		return src;
	}

	static inline
	std::string replace_all(std::string src, const std::vector<std::pair<std::string, std::string>>& list)
	{
		if (list.empty())
			return src;

		for (usz pos = 0; pos < src.length(); ++pos)
		{
			for (usz i = 0; i < list.size(); ++i)
			{
				const usz comp_length = list[i].first.length();

				if (src.length() - pos < comp_length)
				{
					continue;
				}

				if (src.substr(pos, comp_length) == list[i].first)
				{
					src.erase(pos, comp_length);
					src.insert(pos, list[i].second);
					pos += list[i].second.length() - 1;
					break;
				}
			}
		}

		return src;
	}

	// Splits the string into a vector of strings using the separators. The vector may contain empty strings unless is_skip_empty is true.
	std::vector<std::string> split(std::string_view source, std::initializer_list<std::string_view> separators, bool is_skip_empty = true);

	// Removes all preceding and trailing characters specified by 'values' from 'source'.
	std::string trim(const std::string& source, std::string_view values = " \t");

	// Removes all preceding characters specified by 'values' from 'source'.
	std::string trim_front(const std::string& source, std::string_view values = " \t");

	// Removes all trailing characters specified by 'values' from 'source'.
	void trim_back(std::string& source, std::string_view values = " \t");

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

		for (const auto& v : sources)
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

	// Returns the string transformed to uppercase
	std::string to_upper(std::string_view string);

	// Returns the string transformed to lowercase
	std::string to_lower(std::string_view string);

	// Returns the string shortened to length
	std::string truncate(std::string_view src, usz length);

	struct buf_to_hexstring
	{
		buf_to_hexstring(const u8* buf, usz len, usz line_length = 16, bool with_prefix = false)
			: buf(buf), len(len), line_length(line_length), with_prefix(with_prefix) {}

		const u8* buf;
		usz len;
		usz line_length;
		bool with_prefix;
	};

	struct string_hash
	{
		using hash_type = std::hash<std::string_view>;
		using is_transparent = void;

		std::size_t operator()(const char* str) const
		{
			return hash_type{}(str);
		}
		std::size_t operator()(std::string_view str) const
		{
			return hash_type{}(str);
		}
		std::size_t operator()(std::string const& str) const
		{
			return hash_type{}(str);
		}
	};
}
