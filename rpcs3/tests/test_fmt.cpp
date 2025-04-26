#include <gtest/gtest.h>
#include "Utilities/StrUtil.h"

namespace fmt
{
	TEST(StrUtil, to_upper_to_lower)
	{
		const std::string lowercase = "abcdefghijklmnopqrstuvwxyzäüöß0123456789 .,-<#+";
		const std::string uppercase = "ABCDEFGHIJKLMNOPQRSTUVWXYZäüöß0123456789 .,-<#+";
		const std::string to_lower_res = fmt::to_lower(uppercase);
		const std::string to_upper_res = fmt::to_upper(lowercase);

		EXPECT_EQ(std::string(), fmt::to_lower(""));
		EXPECT_EQ(lowercase, fmt::to_lower(lowercase));
		EXPECT_EQ(lowercase, to_lower_res);

		EXPECT_EQ(std::string(), fmt::to_upper(""));
		EXPECT_EQ(uppercase, fmt::to_upper(uppercase));
		EXPECT_EQ(uppercase, to_upper_res);
	}
}
