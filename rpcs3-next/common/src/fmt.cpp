
#include "fmt.h"
#include <algorithm>
#include <cstring>

namespace common
{
	namespace fmt
	{
		/*
		std::string v128::to_hex() const
		{
			return fmt::format("%016llx%016llx", _u64[1], _u64[0]);
		}

		std::string v128::to_xyzw() const
		{
			return fmt::format("x: %g y: %g z: %g w: %g", _f[3], _f[2], _f[1], _f[0]);
		}
		

		std::string fmt::to_hex(u64 value, u64 count)
		{
			if (count - 1 >= 16)
			{
				throw EXCEPTION("Invalid count: 0x%llx", count);
			}

			count = std::max<u64>(count, 16 - cntlz64(value) / 4);

			char res[16] = {};

			for (size_t i = count - 1; ~i; i--, value /= 16)
			{
				res[i] = "0123456789abcdef"[value % 16];
			}

			return std::string(res, count);
		}

		std::string fmt::to_udec(u64 value)
		{
			char res[20] = {};
			size_t first = sizeof(res);

			if (!value)
			{
				res[--first] = '0';
			}

			for (; value; value /= 10)
			{
				res[--first] = '0' + (value % 10);
			}

			return std::string(&res[first], sizeof(res) - first);
		}

		std::string fmt::to_sdec(s64 svalue)
		{
			const bool sign = svalue < 0;
			u64 value = sign ? -svalue : svalue;

			char res[20] = {};
			size_t first = sizeof(res);

			if (!value)
			{
				res[--first] = '0';
			}

			for (; value; value /= 10)
			{
				res[--first] = '0' + (value % 10);
			}

			if (sign)
			{
				res[--first] = '-';
			}

			return std::string(&res[first], sizeof(res) - first);
		}
		*/
		//extern const std::string fmt::placeholder = "???";

		std::string replace_first(const std::string& src, const std::string& from, const std::string& to)
		{
			auto pos = src.find(from);

			if (pos == std::string::npos)
			{
				return src;
			}

			return (pos ? src.substr(0, pos) + to : to) + std::string(src.c_str() + pos + from.length());
		}

		std::string replace_all(const std::string &src, const std::string& from, const std::string& to)
		{
			std::string target = src;
			for (auto pos = target.find(from); pos != std::string::npos; pos = target.find(from, pos + 1))
			{
				target = (pos ? target.substr(0, pos) + to : to) + std::string(target.c_str() + pos + from.length());
				pos += to.length();
			}

			return target;
		}

		std::vector<std::string> split(const std::string& source, std::initializer_list<std::string> separators, bool is_skip_empty)
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

			return result;
		}

		std::string trim(const std::string& source, const std::string& values)
		{
			std::size_t begin = source.find_first_not_of(values);

			if (begin == source.npos)
				return{};

			return source.substr(begin, source.find_last_not_of(values) + 1);
		}

		std::string tolower(std::string source)
		{
			std::transform(source.begin(), source.end(), source.begin(), ::tolower);

			return source;
		}

		std::string toupper(std::string source)
		{
			std::transform(source.begin(), source.end(), source.begin(), ::toupper);

			return source;
		}

		std::string escape(std::string source)
		{
			const std::pair<std::string, std::string> escape_list[] =
			{
				{ "\\", "\\\\" },
				{ "\a", "\\a" },
				{ "\b", "\\b" },
				{ "\f", "\\f" },
				{ "\n", "\\n\n" },
				{ "\r", "\\r" },
				{ "\t", "\\t" },
				{ "\v", "\\v" },
			};

			source = replace_all(source, escape_list);

			for (char c = 0; c < 32; c++)
			{
				if (c != '\n') source = replace_all(source, std::string(1, c), format("\\x%02X", c));
			}

			return source;
		}
	}
}