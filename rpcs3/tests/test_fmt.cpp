#include <gtest/gtest.h>
#include "Utilities/StrUtil.h"

using namespace std::literals::string_literals;

namespace fmt
{
	TEST(StrUtil, test_trim)
	{
		EXPECT_EQ(""s, fmt::trim("", ""));
		EXPECT_EQ(""s, fmt::trim("", " "));
		EXPECT_EQ(""s, fmt::trim("", "a "));
		EXPECT_EQ(" "s, fmt::trim(" ", ""));
		EXPECT_EQ(""s, fmt::trim(" ", " "));
		EXPECT_EQ("a"s, fmt::trim("a ", " "));
		EXPECT_EQ("a"s, fmt::trim(" a", " "));
		EXPECT_EQ("a a"s, fmt::trim("a a", " "));
		EXPECT_EQ("a a"s, fmt::trim("a a ", " "));
		EXPECT_EQ("a a"s, fmt::trim(" a a", " "));
		EXPECT_EQ("a a"s, fmt::trim(" a a ", " "));
		EXPECT_EQ("a a"s, fmt::trim("a a    ", " "));
		EXPECT_EQ("a a"s, fmt::trim("    a a    ", " "));
		EXPECT_EQ("a a"s, fmt::trim("    a a", " "));
		EXPECT_EQ(""s, fmt::trim("    a a    ", " a"));
		EXPECT_EQ("b"s, fmt::trim("    aba    ", " a"));
	}

	TEST(StrUtil, test_trim_front)
	{
		EXPECT_EQ(""s, fmt::trim_front("", ""));
		EXPECT_EQ(""s, fmt::trim_front("", " "));
		EXPECT_EQ(""s, fmt::trim_front("", "a "));
		EXPECT_EQ(" "s, fmt::trim_front(" ", ""));
		EXPECT_EQ(""s, fmt::trim_front(" ", " "));
		EXPECT_EQ("a "s, fmt::trim_front("a ", " "));
		EXPECT_EQ("a"s, fmt::trim_front(" a", " "));
		EXPECT_EQ("a a"s, fmt::trim_front("a a", " "));
		EXPECT_EQ("a a "s, fmt::trim_front("a a ", " "));
		EXPECT_EQ("a a"s, fmt::trim_front(" a a", " "));
		EXPECT_EQ("a a "s, fmt::trim_front(" a a ", " "));
		EXPECT_EQ("a a    "s, fmt::trim_front("a a    ", " "));
		EXPECT_EQ("a a    "s, fmt::trim_front("    a a    ", " "));
		EXPECT_EQ("a a"s, fmt::trim_front("    a a", " "));
		EXPECT_EQ(""s, fmt::trim_front("    a a    ", " a"));
		EXPECT_EQ("ba    "s, fmt::trim_front("    aba    ", " a"));
	}

	TEST(StrUtil, test_trim_back)
	{
		std::string str;
		fmt::trim_back(str, "");
		EXPECT_EQ(""s, str);

		str = {};
		fmt::trim_back(str, " ");
		EXPECT_EQ(""s, str);

		str = {};
		fmt::trim_back(str, "a ");
		EXPECT_EQ(""s, str);

		str = " ";
		fmt::trim_back(str, "");
		EXPECT_EQ(" "s, str);

		str = " ";
		fmt::trim_back(str, " ");
		EXPECT_EQ(""s, str);

		str = "a ";
		fmt::trim_back(str, " ");
		EXPECT_EQ("a"s, str);

		str = " a";
		fmt::trim_back(str, " ");
		EXPECT_EQ(" a"s, str);

		str = "a a";
		fmt::trim_back(str, " ");
		EXPECT_EQ("a a"s, str);

		str = "a a ";
		fmt::trim_back(str, " ");
		EXPECT_EQ("a a"s, str);

		str = " a a";
		fmt::trim_back(str, " ");
		EXPECT_EQ(" a a"s, str);

		str = " a a ";
		fmt::trim_back(str, " ");
		EXPECT_EQ(" a a"s, str);

		str = "a a    ";
		fmt::trim_back(str, " ");
		EXPECT_EQ("a a"s, str);

		str = "    a a    ";
		fmt::trim_back(str, " ");
		EXPECT_EQ("    a a"s, str);

		str = "    a a";
		fmt::trim_back(str, " ");
		EXPECT_EQ("    a a"s, str);

		str = "    a a    ";
		fmt::trim_back(str, " a");
		EXPECT_EQ(""s, str);

		str = "    aba    ";
		fmt::trim_back(str, " a");
		EXPECT_EQ("    ab"s, str);
	}

	TEST(StrUtil, test_to_upper_to_lower)
	{
		const std::string lowercase = "abcdefghijklmnopqrstuvwxyzäüöß0123456789 .,-<#+";
		const std::string uppercase = "ABCDEFGHIJKLMNOPQRSTUVWXYZäüöß0123456789 .,-<#+";
		const std::string to_lower_res = fmt::to_lower(uppercase);
		const std::string to_upper_res = fmt::to_upper(lowercase);

		EXPECT_EQ(""s, fmt::to_lower(""));
		EXPECT_EQ(lowercase, fmt::to_lower(lowercase));
		EXPECT_EQ(lowercase, to_lower_res);

		EXPECT_EQ(""s, fmt::to_upper(""));
		EXPECT_EQ(uppercase, fmt::to_upper(uppercase));
		EXPECT_EQ(uppercase, to_upper_res);
	}

	TEST(StrUtil, test_truncate)
	{
		const std::string str = "abcdefghijklmnopqrstuvwxyzäüöß0123456789 .,-<#+";

		EXPECT_EQ(""s, fmt::truncate("", -1));
		EXPECT_EQ(""s, fmt::truncate("", 0));
		EXPECT_EQ(""s, fmt::truncate("", 1));

		EXPECT_EQ(""s, fmt::truncate(str, 0));
		EXPECT_EQ("a"s, fmt::truncate(str, 1));
		EXPECT_EQ("abcdefghijklmnopqrstuvwxyzäüöß0123456789 .,-<#"s, fmt::truncate(str, str.size() - 1));

		EXPECT_EQ(str, fmt::truncate(str, -1));
		EXPECT_EQ(str, fmt::truncate(str, str.size()));
		EXPECT_EQ(str, fmt::truncate(str, str.size() + 1));
	}

	TEST(StrUtil, test_get_file_extension)
	{
		EXPECT_EQ(""s, get_file_extension(""));
		EXPECT_EQ(""s, get_file_extension("."));
		EXPECT_EQ(""s, get_file_extension(".."));
		EXPECT_EQ(""s, get_file_extension("a."));
		EXPECT_EQ(""s, get_file_extension("a.."));
		EXPECT_EQ(""s, get_file_extension(".a."));
		EXPECT_EQ("a"s, get_file_extension(".a"));
		EXPECT_EQ("a"s, get_file_extension("..a"));
		EXPECT_EQ(""s, get_file_extension("abc"));
		EXPECT_EQ(""s, get_file_extension("abc."));
		EXPECT_EQ(""s, get_file_extension("abc.."));
		EXPECT_EQ(""s, get_file_extension(".abc."));
		EXPECT_EQ("abc"s, get_file_extension(".abc"));
		EXPECT_EQ("abc"s, get_file_extension("qwe.abc"));
		EXPECT_EQ("abc"s, get_file_extension("qwe..abc"));
		EXPECT_EQ(""s, get_file_extension("qwe..abc.."));
		EXPECT_EQ("txt"s, get_file_extension("qwe..abc..txt"));
		EXPECT_EQ(""s, get_file_extension("my_file/asd"));
		EXPECT_EQ("txt"s, get_file_extension("my_file/asd.txt"));
	}
}
