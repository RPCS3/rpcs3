#pragma once
#include "convert.h"
#include "geometry_types.h"
#include "fmt.h"

namespace common
{
	template<typename Type, int dimension>
	struct to_impl_t<vector<Type, dimension>, std::string>
	{
		static vector<Type, dimension> func(const std::string& value)
		{
			vector<Type, dimension> result;
			const std::string& data = fmt::split(value, { "x" });

			for (std::size_t i = 0; i < data.size(); ++i)
			{
				result[i] = convert::to<Type>(data[i]);
			}

			return result;
		}
	};

	template<>
	struct to_impl_t<vector<Type, dimension>, std::string>
	{
		static vector<Type, dimension> func(const std::string& value)
		{
			vector<Type, dimension> result;
			const std::string& data = fmt::split(value, { "x" });

			for (std::size_t i = 0; i < data.size(); ++i)
			{
				result[i] = convert::to<Type>(data[i]);
			}

			return result;
		}
	};
}
