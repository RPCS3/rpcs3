#pragma once

class wxString;

#if defined(_MSC_VER)
#define snprintf _snprintf
#endif

namespace fmt
{
	struct empty_t{};

	extern const std::string placeholder;

	template <typename T>
	std::string AfterLast(const std::string& source, T searchstr)
	{
		size_t search_pos = source.rfind(searchstr);
		search_pos = search_pos == std::string::npos ? 0 : search_pos;
		return source.substr(search_pos);
	}

	template <typename T>
	std::string BeforeLast(const std::string& source, T searchstr)
	{
		size_t search_pos = source.rfind(searchstr);
		search_pos = search_pos == std::string::npos ? 0 : search_pos;
		return source.substr(0, search_pos);
	}

	template <typename T>
	std::string AfterFirst(const std::string& source, T searchstr)
	{
		size_t search_pos = source.find(searchstr);
		search_pos = search_pos == std::string::npos ? 0 : search_pos;
		return source.substr(search_pos);
	}

	template <typename T>
	std::string BeforeFirst(const std::string& source, T searchstr)
	{
		size_t search_pos = source.find(searchstr);
		search_pos = search_pos == std::string::npos ? 0 : search_pos;
		return source.substr(0, search_pos);
	}

	// write `fmt` from `pos` to the first occurence of `fmt::placeholder` to
	// the stream `os`. Then write `arg` to to the stream. If there's no
	// `fmt::placeholder` after `pos` everything in `fmt` after pos is written
	// to `os`. Then `arg` is written to `os` after appending a space character
	template<typename T>
	empty_t write(const std::string &fmt, std::ostream &os, std::string::size_type &pos, T &&arg)
	{
		std::string::size_type ins = fmt.find(placeholder, pos);

		if (ins == std::string::npos)
		{
			os.write(fmt.data() + pos, fmt.size() - pos);
			os << ' ' << arg;

			pos = fmt.size();
		}
		else
		{
			os.write(fmt.data() + pos, ins - pos);
			os << arg;

			pos = ins + placeholder.size();
		}
		return{};
	}

	// typesafe version of a sprintf-like function. Returns the printed to
	// string. To mark positions where the arguments are supposed to be
	// inserted use `fmt::placeholder`. If there's not enough placeholders
	// the rest of the arguments are appended at the end, seperated by spaces
	template<typename  ... Args>
	std::string SFormat(const std::string &fmt, Args&& ... parameters)
	{
		std::ostringstream os;
		std::string::size_type pos = 0;
		std::initializer_list<empty_t> { write(fmt, os, pos, parameters)... };

		if (!fmt.empty())
		{
			os.write(fmt.data() + pos, fmt.size() - pos);
		}

		std::string result = os.str();
		return result;
	}

	//small wrapper used to deal with bitfields
	template<typename T>
	T by_value(T x) { return x; }

	//wrapper to deal with advance sprintf formating options with automatic length finding
	template<typename  ... Args>
	std::string Format(const char* fmt, Args ... parameters)
	{
		size_t length = 256;
		std::string str;

		for (;;)
		{
			std::vector<char> buffptr(length);
#if !defined(_MSC_VER)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
			size_t printlen = snprintf(buffptr.data(), length, fmt, std::forward<Args>(parameters)...);
#pragma clang diagnostic pop
#else
			size_t printlen = _snprintf_s(buffptr.data(), length, length - 1, fmt, std::forward<Args>(parameters)...);
#endif
			if (printlen < length)
			{
				str = std::string(buffptr.data(), printlen);
				break;
			}
			length *= 2;
		}
		return str;
	}

	std::string replace_first(const std::string& src, const std::string& from, const std::string& to);
	std::string replace_all(std::string src, const std::string& from, const std::string& to);

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
					src = (pos ? src.substr(0, pos) + list[i].second : list[i].second) + std::string(src.c_str() + pos + comp_length);
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
					src = (pos ? src.substr(0, pos) + list[i].second() : list[i].second()) + std::string(src.c_str() + pos + comp_length);
					pos += list[i].second().length() - 1;
					break;
				}
			}
		}

		return src;
	}

	namespace detail
	{
		static std::string to_hex(u64 value, size_t count = 1)
		{
			assert(count - 1 < 16);
			count = std::max<u64>(count, 16 - cntlz64(value) / 4);

			char res[16] = {};

			for (size_t i = count - 1; ~i; i--, value /= 16)
			{
				res[i] = "0123456789abcdef"[value % 16];
			}

			return std::string(res, count);
		}

		static size_t get_fmt_start(const char* fmt, size_t len)
		{
			for (size_t i = 0; i < len; i++)
			{
				if (fmt[i] == '%')
				{
					return i;
				}
			}

			return len;
		}

		static size_t get_fmt_len(const char* fmt, size_t len)
		{
			assert(len >= 2 && fmt[0] == '%');

			size_t res = 2;

			if (fmt[1] == '.' || fmt[1] == '0')
			{
				assert(len >= 4 && fmt[2] - '1' < 9);
				res += 2;
				fmt += 2;
				len -= 2;

				if (fmt[1] == '1')
				{
					assert(len >= 3 && fmt[2] - '0' < 7);
					res++;
					fmt++;
					len--;
				}
			}

			if (fmt[1] == 'l')
			{
				assert(len >= 3);
				res++;
				fmt++;
				len--;
			}

			if (fmt[1] == 'l')
			{
				assert(len >= 3);
				res++;
				fmt++;
				len--;
			}

			return res;
		}

		static size_t get_fmt_precision(const char* fmt, size_t len)
		{
			assert(len >= 2);

			if (fmt[1] == '.' || fmt[1] == '0')
			{
				assert(len >= 4 && fmt[2] - '1' < 9);

				if (fmt[2] == '1')
				{
					assert(len >= 5 && fmt[3] - '0' < 7);
					return 10 + fmt[3] - '0';
				}

				return fmt[2] - '0';
			}

			return 1;
		}

		template<typename T, bool is_enum = std::is_enum<T>::value>
		struct get_fmt
		{
			static_assert(is_enum, "Unsupported fmt::format argument");
			typedef typename std::underlying_type<T>::type underlying_type;

			static std::string text(const char* fmt, size_t len, const T& arg)
			{
				return get_fmt<underlying_type>::text(fmt, len, (underlying_type)arg);
			}
		};

		template<typename T, typename T2>
		struct get_fmt<be_t<T, T2>, false>
		{
			static std::string text(const char* fmt, size_t len, const be_t<T, T2>& arg)
			{
				return get_fmt<T>::text(fmt, len, arg.value());
			}
		};

		template<>
		struct get_fmt<u8>
		{
			static std::string text(const char* fmt, size_t len, u8 arg)
			{
				if (fmt[len - 1] == 'x')
				{
					return to_hex(arg, get_fmt_precision(fmt, len));
				}
				else if (fmt[len - 1] == 'd')
				{
					return std::to_string((u32)arg);
				}
				else
				{
					throw "Invalid formatting (u8): " + std::string(fmt, len);
				}

				return{};
			}
		};

		template<>
		struct get_fmt<u16>
		{
			static std::string text(const char* fmt, size_t len, u16 arg)
			{
				if (fmt[len - 1] == 'x')
				{
					return to_hex(arg, get_fmt_precision(fmt, len));
				}
				else if (fmt[len - 1] == 'd')
				{
					return std::to_string((u32)arg);
				}
				else
				{
					throw "Invalid formatting (u16): " + std::string(fmt, len);
				}

				return{};
			}
		};

		template<>
		struct get_fmt<u32>
		{
			static std::string text(const char* fmt, size_t len, u32 arg)
			{
				if (fmt[len - 1] == 'x')
				{
					return to_hex(arg, get_fmt_precision(fmt, len));
				}
				else if (fmt[len - 1] == 'd')
				{
					return std::to_string(arg);
				}
				else
				{
					throw "Invalid formatting (u32): " + std::string(fmt, len);
				}

				return{};
			}
		};

		template<>
		struct get_fmt<u64>
		{
			static std::string text(const char* fmt, size_t len, u64 arg)
			{
				if (fmt[len - 1] == 'x')
				{
					return to_hex(arg, get_fmt_precision(fmt, len));
				}
				else if (fmt[len - 1] == 'd')
				{
					return std::to_string(arg);
				}
				else
				{
					throw "Invalid formatting (u64): " + std::string(fmt, len);
				}

				return{};
			}
		};

		template<>
		struct get_fmt<s8>
		{
			static std::string text(const char* fmt, size_t len, s8 arg)
			{
				if (fmt[len - 1] == 'x')
				{
					return to_hex((u8)arg, get_fmt_precision(fmt, len));
				}
				else if (fmt[len - 1] == 'd')
				{
					return std::to_string((s32)arg);
				}
				else
				{
					throw "Invalid formatting (s8): " + std::string(fmt, len);
				}

				return{};
			}
		};

		template<>
		struct get_fmt<s16>
		{
			static std::string text(const char* fmt, size_t len, s16 arg)
			{
				if (fmt[len - 1] == 'x')
				{
					return to_hex((u16)arg, get_fmt_precision(fmt, len));
				}
				else if (fmt[len - 1] == 'd')
				{
					return std::to_string((s32)arg);
				}
				else
				{
					throw "Invalid formatting (s16): " + std::string(fmt, len);
				}

				return{};
			}
		};

		template<>
		struct get_fmt<s32>
		{
			static std::string text(const char* fmt, size_t len, s32 arg)
			{
				if (fmt[len - 1] == 'x')
				{
					return to_hex((u32)arg, get_fmt_precision(fmt, len));
				}
				else if (fmt[len - 1] == 'd')
				{
					return std::to_string(arg);
				}
				else
				{
					throw "Invalid formatting (s32): " + std::string(fmt, len);
				}

				return{};
			}
		};

		template<>
		struct get_fmt<s64>
		{
			static std::string text(const char* fmt, size_t len, s64 arg)
			{
				if (fmt[len - 1] == 'x')
				{
					return to_hex((u64)arg, get_fmt_precision(fmt, len));
				}
				else if (fmt[len - 1] == 'd')
				{
					return std::to_string(arg);
				}
				else
				{
					throw "Invalid formatting (s64): " + std::string(fmt, len);
				}

				return{};
			}
		};

		template<>
		struct get_fmt<float>
		{
			static std::string text(const char* fmt, size_t len, float arg)
			{
				if (fmt[len - 1] == 'x')
				{
					return to_hex((u32&)arg, get_fmt_precision(fmt, len));
				}
				else if (fmt[len - 1] == 'f')
				{
					return std::to_string(arg);
				}
				else
				{
					throw "Invalid formatting (float): " + std::string(fmt, len);
				}

				return{};
			}
		};

		template<>
		struct get_fmt<double>
		{
			static std::string text(const char* fmt, size_t len, double arg)
			{
				if (fmt[len - 1] == 'x')
				{
					return to_hex((u64&)arg, get_fmt_precision(fmt, len));
				}
				else if (fmt[len - 1] == 'f')
				{
					return std::to_string(arg);
				}
				else
				{
					throw "Invalid formatting (double): " + std::string(fmt, len);
				}

				return{};
			}
		};

		template<>
		struct get_fmt<bool>
		{
			static std::string text(const char* fmt, size_t len, bool arg)
			{
				if (fmt[len - 1] == 'x')
				{
					return to_hex(arg, get_fmt_precision(fmt, len));
				}
				else if (fmt[len - 1] == 'd')
				{
					return arg ? "1" : "0";
				}
				else if (fmt[len - 1] == 's')
				{
					return arg ? "true" : "false";
				}
				else
				{
					throw "Invalid formatting (bool): " + std::string(fmt, len);
				}

				return{};
			}
		};

		template<>
		struct get_fmt<char*>
		{
			static std::string text(const char* fmt, size_t len, char* arg)
			{
				if (fmt[len - 1] == 's')
				{
					return arg;
				}
				else
				{
					throw "Invalid formatting (char*): " + std::string(fmt, len);
				}

				return{};
			}
		};

		template<>
		struct get_fmt<const char*>
		{
			static std::string text(const char* fmt, size_t len, const char* arg)
			{
				if (fmt[len - 1] == 's')
				{
					return arg;
				}
				else
				{
					throw "Invalid formatting (const char*): " + std::string(fmt, len);
				}

				return{};
			}
		};

		//template<size_t size>
		//struct get_fmt<char[size], false>
		//{
		//	static std::string text(const char* fmt, size_t len, const char(&arg)[size])
		//	{
		//		if (fmt[len - 1] == 's')
		//		{
		//			return std::string(arg, size);
		//		}
		//		else
		//		{
		//			throw "Invalid formatting (char[size]): " + std::string(fmt, len);
		//		}

		//		return{};
		//	}
		//};

		//template<size_t size>
		//struct get_fmt<const char[size], false>
		//{
		//	static std::string text(const char* fmt, size_t len, const char(&arg)[size])
		//	{
		//		if (fmt[len - 1] == 's')
		//		{
		//			return std::string(arg, size);
		//		}
		//		else
		//		{
		//			throw "Invalid formatting (const char[size]): " + std::string(fmt, len);
		//		}

		//		return{};
		//	}
		//};

		template<>
		struct get_fmt<std::string>
		{
			static std::string text(const char* fmt, size_t len, const std::string& arg)
			{
				if (fmt[len - 1] == 's')
				{
					return arg;
				}
				else
				{
					throw "Invalid formatting (std::string): " + std::string(fmt, len);
				}

				return{};
			}
		};

		static std::string format(const char* fmt, size_t len)
		{
			const size_t fmt_start = get_fmt_start(fmt, len);
			assert(fmt_start == len);

			return std::string(fmt, len);
		}

		template<typename T, typename... Args>
		static std::string format(const char* fmt, size_t len, const T& arg, Args... args)
		{
			const size_t fmt_start = get_fmt_start(fmt, len);
			const size_t fmt_len = get_fmt_len(fmt + fmt_start, len - fmt_start);
			const size_t fmt_end = fmt_start + fmt_len;

			return std::string(fmt, fmt_start) + get_fmt<T>::text(fmt + fmt_start, fmt_len, arg) + format(fmt + fmt_end, len - fmt_end, args...);
		}
	};

	// formatting function with very limited functionality (compared to printf-like formatting) and be_t<> support
	/*
	Supported types:

	u8, s8 (%x, %d)
	u16, s16 (%x, %d)
	u32, s32 (%x, %d)
	u64, s64 (%x, %d)
	float (%x, %f)
	double (%x, %f)
	bool (%x, %d, %s)
	char*, const char*, std::string (%s)
	be_t<> of any appropriate type in this list
	enum of any appropriate type in this list
	
	Supported formatting:
	%d - decimal; only basic std::to_string() functionality
	%x - hexadecimal; %.8x - hexadecimal with the precision (from .2 to .16)
	%s - string; generates "true" or "false" for bool
	%f - floating point; only basic std::to_string() functionality

	Other features are not supported.
	*/
	template<typename... Args>
	__forceinline static std::string format(const char* fmt, Args... args)
	{
		return detail::format(fmt, strlen(fmt), args...);
	}

	//convert a wxString to a std::string encoded in utf8
	//CAUTION, only use this to interface with wxWidgets classes
	std::string ToUTF8(const wxString& right);

	//convert a std::string encoded in utf8 to a wxString
	//CAUTION, only use this to interface with wxWidgets classes
	wxString FromUTF8(const std::string& right);

	//TODO: remove this after every snippet that uses it is gone
	//WARNING: not fully compatible with CmpNoCase from wxString
	int CmpNoCase(const std::string& a, const std::string& b);

	//TODO: remove this after every snippet that uses it is gone
	//WARNING: not fully compatible with Replace from wxString
	void Replace(std::string &str, const std::string &searchterm, const std::string& replaceterm);

	std::vector<std::string> rSplit(const std::string& source, const std::string& delim);

	std::vector<std::string> split(const std::string& source, std::initializer_list<std::string> separators, bool is_skip_empty = true);
	std::string merge(std::vector<std::string> source, const std::string& separator);
	std::string merge(std::initializer_list<std::vector<std::string>> sources, const std::string& separator);
	std::string tolower(std::string source);
}
