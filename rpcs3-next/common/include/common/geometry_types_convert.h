#pragma once
#include "convert.h"
#include "geometry_types.h"
#include "fmt.h"

namespace common
{
	template<typename Type, int Dimension>
	struct convert::to_impl_t<vector<Dimension, Type>, std::string>
	{
		static vector<Dimension, Type> func(const std::string& value)
		{
			vector<Dimension, Type> result;
			const std::vector<std::string> data = fmt::split(value, { "x" });

			for (std::size_t i = 0; i < data.size(); ++i)
			{
				result[i] = convert::to<Type>(data[i]);
			}

			return result;
		}
	};

	template<typename Type, int Dimension>
	struct convert::to_impl_t<std::string, vector<Dimension, Type>>
	{
		static std::string func(const vector<Dimension, Type>& value)
		{
			std::string result = convert::to<std::string>(value[0]);

			for (int i = 1; i < Dimension; ++i)
			{
				result += "x" + value[i];
			}

			return result;
		}
	};

	template<typename Type, int Dimension>
	struct convert::to_impl_t<size<Dimension, Type>, std::string>
	{
		static size<Dimension, Type> func(const std::string& value)
		{
			return convert::to<vector<Dimension, Type>>(value);
		}
	};

	template<typename Type, int Dimension>
	struct convert::to_impl_t<std::string, size<Dimension, Type>>
	{
		static std::string func(const size<Dimension, Type>& value)
		{
			return convert::to<std::string>((const vector<Dimension, Type>&)value);
		}
	};

	template<typename Type, int Dimension>
	struct convert::to_impl_t<point<Dimension, Type>, std::string>
	{
		static point<Dimension, Type> func(const std::string& value)
		{
			return convert::to<vector<Dimension, Type>>(value);
		}
	};

	template<typename Type, int Dimension>
	struct convert::to_impl_t<std::string, point<Dimension, Type>>
	{
		static std::string func(const point<Dimension, Type>& value)
		{
			return convert::to<std::string>((const vector<Dimension, Type>&)value);
		}
	};
}
