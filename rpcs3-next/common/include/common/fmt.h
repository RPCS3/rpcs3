#pragma once

#include "basic_types.h"
#include "platform.h"

#include <functional>
#include <string>
#include <memory>
#include <vector>
#include <array>

namespace common
{
	namespace fmt
	{
		//struct empty_t{};

		//extern const std::string placeholder;

		template <typename T>
		std::string after_last(const std::string& source, T searchstr)
		{
			size_t search_pos = source.rfind(searchstr);
			search_pos = search_pos == std::string::npos ? 0 : search_pos;
			return source.substr(search_pos);
		}

		template <typename T>
		std::string before_last(const std::string& source, T searchstr)
		{
			size_t search_pos = source.rfind(searchstr);
			search_pos = search_pos == std::string::npos ? 0 : search_pos;
			return source.substr(0, search_pos);
		}

		template <typename T>
		std::string after_first(const std::string& source, T searchstr)
		{
			size_t search_pos = source.find(searchstr);
			search_pos = search_pos == std::string::npos ? 0 : search_pos;
			return source.substr(search_pos);
		}

		template <typename T>
		std::string before_first(const std::string& source, T searchstr)
		{
			size_t search_pos = source.find(searchstr);
			search_pos = search_pos == std::string::npos ? 0 : search_pos;
			return source.substr(0, search_pos);
		}

		// write `fmt` from `pos` to the first occurence of `fmt::placeholder` to
		// the stream `os`. Then write `arg` to to the stream. If there's no
		// `fmt::placeholder` after `pos` everything in `fmt` after pos is written
		// to `os`. Then `arg` is written to `os` after appending a space character
		//template<typename T>
		//empty_t write(const std::string &fmt, std::ostream &os, std::string::size_type &pos, T &&arg)
		//{
		//	std::string::size_type ins = fmt.find(placeholder, pos);

		//	if (ins == std::string::npos)
		//	{
		//		os.write(fmt.data() + pos, fmt.size() - pos);
		//		os << ' ' << arg;

		//		pos = fmt.size();
		//	}
		//	else
		//	{
		//		os.write(fmt.data() + pos, ins - pos);
		//		os << arg;

		//		pos = ins + placeholder.size();
		//	}
		//	return{};
		//}

		// typesafe version of a sprintf-like function. Returns the printed to
		// string. To mark positions where the arguments are supposed to be
		// inserted use `fmt::placeholder`. If there's not enough placeholders
		// the rest of the arguments are appended at the end, seperated by spaces
		//template<typename  ... Args>
		//std::string SFormat(const std::string &fmt, Args&& ... parameters)
		//{
		//	std::ostringstream os;
		//	std::string::size_type pos = 0;
		//	std::initializer_list<empty_t> { write(fmt, os, pos, parameters)... };

		//	if (!fmt.empty())
		//	{
		//		os.write(fmt.data() + pos, fmt.size() - pos);
		//	}

		//	std::string result = os.str();
		//	return result;
		//}

		std::string replace_first(const std::string& src, const std::string& from, const std::string& to);
		std::string replace_all(const std::string &src, const std::string& from, const std::string& to);

		template<size_t list_size>
		std::string replace_all(std::string src, const std::pair<std::string, std::string>(&list)[list_size])
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

		template<size_t list_size>
		std::string replace_all(std::string src, const std::pair<std::string, std::function<std::string()>>(&list)[list_size])
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

		//std::string to_hex(u64 value, u64 count = 1);
		//std::string to_udec(u64 value);
		//std::string to_sdec(s64 value);

		template<typename T, bool is_enum = std::is_enum<T>::value> struct unveil
		{
			using result_type = T;

			force_inline static result_type get_value(const T& arg)
			{
				return arg;
			}
		};

		template<> struct unveil<char*, false>
		{
			using result_type = const char*;

			force_inline static result_type get_value(const char* arg)
			{
				return arg;
			}
		};

		template<std::size_t N> struct unveil<const char[N], false>
		{
			using result_type = const char*;

			force_inline static result_type get_value(const char(&arg)[N])
			{
				return arg;
			}
		};

		template<> struct unveil<std::string, false>
		{
			using result_type = const char*;

			force_inline static result_type get_value(const std::string& arg)
			{
				return arg.c_str();
			}
		};

		template<typename T> struct unveil<T, true>
		{
			using result_type = std::underlying_type_t<T>;

			force_inline static result_type get_value(const T& arg)
			{
				return static_cast<result_type>(arg);
			}
		};
		/*
		template<typename T, bool Se> struct unveil<se_t<T, Se>, false>
		{
			using result_type = typename unveil<T>::result_type;

			force_inline static result_type get_value(const se_t<T, Se>& arg)
			{
				return unveil<T>::get_value(arg);
			}
		};
		*/

		template<typename T>
		force_inline typename unveil<T>::result_type do_unveil(const T& arg)
		{
			return unveil<T>::get_value(arg);
		}

		// Formatting function with special functionality:
		//
		// std::string is forced to .c_str()
		// be_t<> is forced to .value() (fmt::do_unveil reverts byte order automatically)
		//
		// External specializations for fmt::do_unveil (can be found in another headers):
		// vm::ptr, vm::bptr, ... (fmt::do_unveil) (vm_ptr.h) (with appropriate address type, using .addr() can be avoided)
		// vm::ref, vm::bref, ... (fmt::do_unveil) (vm_ref.h)
		//
		template<typename... Args> safe_buffers std::string format(const char* fmt, Args... args)
		{
			// fixed stack buffer for the first attempt
			std::array<char, 4096> fixed_buf;

			// possibly dynamically allocated buffer for the second attempt
			std::unique_ptr<char[]> buf;

			// pointer to the current buffer
			char* buf_addr = fixed_buf.data();

			for (std::size_t buf_size = fixed_buf.size();;)
			{
#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
#endif
				const std::size_t len = std::snprintf(buf_addr, buf_size, fmt, do_unveil(args)...);
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
				if (len > INT_MAX)
				{
					throw std::runtime_error("std::snprintf() failed");
				}

				if (len < buf_size)
				{
					return{ buf_addr, len };
				}

				buf.reset(buf_addr = new char[buf_size = len + 1]);
			}
		}

		std::vector<std::string> split(const std::string& source, std::initializer_list<std::string> separators, bool is_skip_empty = true);
		std::string trim(const std::string& source, const std::string& values = " \t");

		template<typename T>
		std::string merge(const T& source, const std::string& separator)
		{
			if (!source.size())
			{
				return{};
			}

			std::string result;

			auto it = source.begin();
			auto end = source.end();
			for (--end; it != end; ++it)
			{
				result += *it + separator;
			}

			return result + source.back();
		}

		template<typename T>
		std::string merge(std::initializer_list<T> sources, const std::string& separator)
		{
			if (!sources.size())
			{
				return{};
			}

			std::string result;
			bool first = true;

			for (auto &v : sources)
			{
				if (first)
				{
					result = fmt::merge(v, separator);
					first = false;
				}
				else
				{
					result += separator + fmt::merge(v, separator);
				}
			}

			return result;
		}

		std::string tolower(std::string source);
		std::string toupper(std::string source);
		std::string escape(std::string source);
	}
}