#pragma once

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <functional>

// Copy null-terminated string from std::string to char array with truncation
template <std::size_t N>
inline void strcpy_trunc(char (&dst)[N], const std::string& src)
{
	const std::size_t count = src.size() >= N ? N - 1 : src.size();
	std::memcpy(dst, src.c_str(), count);
	dst[count] = '\0';
}

// Copy null-terminated string from char array to another char array with truncation
template <std::size_t N, std::size_t N2>
inline void strcpy_trunc(char (&dst)[N], const char (&src)[N2])
{
	const std::size_t count = N2 >= N ? N - 1 : N2;
	std::memcpy(dst, src, count);
	dst[count] = '\0';
}

template <std::size_t N>
inline bool ends_with(const std::string& src, const char (&end)[N])
{
	return src.size() >= N - 1 && src.compare(src.size() - (N - 1), N - 1, end, N - 1) == 0;
}

namespace fmt
{
	std::string replace_first(const std::string& src, const std::string& from, const std::string& to);
	std::string replace_all(const std::string& src, const std::string& from, const std::string& to);

	template <size_t list_size>
	std::string replace_all(std::string src, const std::pair<std::string, std::string> (&list)[list_size])
	{
		for (size_t pos = 0; pos < src.length(); ++pos)
		{
			for (size_t i = 0; i < list_size; ++i)
			{
				const size_t comp_length = list[i].first.length();

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

	template <size_t list_size>
	std::string replace_all(std::string src, const std::pair<std::string, std::function<std::string()>> (&list)[list_size])
	{
		for (size_t pos = 0; pos < src.length(); ++pos)
		{
			for (size_t i = 0; i < list_size; ++i)
			{
				const size_t comp_length = list[i].first.length();

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

	std::vector<std::string> split(const std::string& source, std::initializer_list<std::string> separators, bool is_skip_empty = true);
	std::string trim(const std::string& source, const std::string& values = " \t");

	template <typename T>
	std::string merge(const T& source, const std::string& separator)
	{
		if (!source.size())
		{
			return {};
		}

		std::string result;

		auto it  = source.begin();
		auto end = source.end();
		for (--end; it != end; ++it)
		{
			result += *it + separator;
		}

		return result + source.back();
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
