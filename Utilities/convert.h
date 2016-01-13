#pragma once
#include <string>
#include "types.h"

namespace convert
{
	template<typename ReturnType, typename FromType>
	struct to_impl_t;

	template<typename Type>
	struct to_impl_t<Type, Type>
	{
		static Type func(const Type& value)
		{
			return value;
		}
	};

	template<>
	struct to_impl_t<std::string, bool>
	{
		static std::string func(bool value)
		{
			return value ? "true" : "false";
		}
	};

	template<>
	struct to_impl_t<bool, std::string>
	{
		static bool func(const std::string& value)
		{
			return value == "true" ? true : value == "false" ? false : throw std::invalid_argument(__FUNCTION__);
		}
	};

	template<>
	struct to_impl_t<std::string, signed char>
	{
		static std::string func(signed char value)
		{
			return std::to_string(value);
		}
	};

	template<>
	struct to_impl_t<std::string, unsigned char>
	{
		static std::string func(unsigned char value)
		{
			return std::to_string(value);
		}
	};


	template<>
	struct to_impl_t<std::string, short>
	{
		static std::string func(short value)
		{
			return std::to_string(value);
		}
	};

	template<>
	struct to_impl_t<std::string, unsigned short>
	{
		static std::string func(unsigned short value)
		{
			return std::to_string(value);
		}
	};

	template<>
	struct to_impl_t<std::string, int>
	{
		static std::string func(int value)
		{
			return std::to_string(value);
		}
	};

	template<>
	struct to_impl_t<std::string, unsigned int>
	{
		static std::string func(unsigned int value)
		{
			return std::to_string(value);
		}
	};

	template<>
	struct to_impl_t<std::string, long>
	{
		static std::string func(long value)
		{
			return std::to_string(value);
		}
	};

	template<>
	struct to_impl_t<std::string, unsigned long>
	{
		static std::string func(unsigned long value)
		{
			return std::to_string(value);
		}
	};

	template<>
	struct to_impl_t<std::string, long long>
	{
		static std::string func(long long value)
		{
			return std::to_string(value);
		}
	};

	template<>
	struct to_impl_t<std::string, unsigned long long>
	{
		static std::string func(unsigned long long value)
		{
			return std::to_string(value);
		}
	};

	template<>
	struct to_impl_t<std::string, float>
	{
		static std::string func(float value)
		{
			return std::to_string(value);
		}
	};

	template<>
	struct to_impl_t<std::string, double>
	{
		static std::string func(double value)
		{
			return std::to_string(value);
		}
	};

	template<>
	struct to_impl_t<std::string, long double>
	{
		static std::string func(long double value)
		{
			return std::to_string(value);
		}
	};

	template<>
	struct to_impl_t<std::string, size2i>
	{
		static std::string func(size2i value)
		{
			return std::to_string(value.width) + "x" + std::to_string(value.height);
		}
	};

	template<>
	struct to_impl_t<std::string, position2i>
	{
		static std::string func(position2i value)
		{
			return std::to_string(value.x) + ":" + std::to_string(value.y);
		}
	};

	template<>
	struct to_impl_t<int, std::string>
	{
		static int func(const std::string& value)
		{
			return std::stoi(value);
		}
	};

	template<>
	struct to_impl_t<unsigned int, std::string>
	{
		static unsigned int func(const std::string& value)
		{
			return (unsigned long)std::stoul(value);
		}
	};

	template<>
	struct to_impl_t<long, std::string>
	{
		static long func(const std::string& value)
		{
			return std::stol(value);
		}
	};

	template<>
	struct to_impl_t<unsigned long, std::string>
	{
		static unsigned long func(const std::string& value)
		{
			return std::stoul(value);
		}
	};

	template<>
	struct to_impl_t<long long, std::string>
	{
		static long long func(const std::string& value)
		{
			return std::stoll(value);
		}
	};

	template<>
	struct to_impl_t<unsigned long long, std::string>
	{
		static unsigned long long func(const std::string& value)
		{
			return std::stoull(value);
		}
	};

	template<>
	struct to_impl_t<float, std::string>
	{
		static float func(const std::string& value)
		{
			return std::stof(value);
		}
	};

	template<>
	struct to_impl_t<double, std::string>
	{
		static double func(const std::string& value)
		{
			return std::stod(value);
		}
	};

	template<>
	struct to_impl_t<long double, std::string>
	{
		static long double func(const std::string& value)
		{
			return std::stold(value);
		}
	};

	template<>
	struct to_impl_t<size2i, std::string>
	{
		static size2i func(const std::string& value)
		{
			const auto& data = fmt::split(value, { "x" });
			return { std::stoi(data[0]), std::stoi(data[1]) };
		}
	};

	template<>
	struct to_impl_t<position2i, std::string>
	{
		static position2i func(const std::string& value)
		{
			const auto& data = fmt::split(value, { ":" });
			return { std::stoi(data[0]), std::stoi(data[1]) };
		}
	};
	
	template<typename ReturnType, typename FromType>
	ReturnType to(FromType value)
	{
		return to_impl_t<std::remove_all_extents_t<ReturnType>, std::remove_all_extents_t<FromType>>::func(value);
	}
}
