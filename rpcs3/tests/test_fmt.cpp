#include <gtest/gtest.h>
#include "Utilities/StrUtil.h"

using namespace std::literals::string_literals;

namespace fmt
{
	TEST(StrUtil, Trim)
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

	TEST(StrUtil, TrimFront)
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

	TEST(StrUtil, TrimBack)
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

	TEST(StrUtil, ToUpperToLower)
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

	TEST(StrUtil, Truncate)
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

	TEST(StrUtil, ReplaceAll)
	{
		EXPECT_EQ(""s, fmt::replace_all("", "", ""));
		EXPECT_EQ(""s, fmt::replace_all("", "", " "));
		EXPECT_EQ(""s, fmt::replace_all("", "", "word"));
		EXPECT_EQ(""s, fmt::replace_all("", " ", ""));
		EXPECT_EQ(""s, fmt::replace_all("", "word", ""));
		EXPECT_EQ(""s, fmt::replace_all("", "word", "drow"));

		EXPECT_EQ("  "s, fmt::replace_all("  ", "", ""));
		EXPECT_EQ("  "s, fmt::replace_all("  ", "", " "));
		EXPECT_EQ("  "s, fmt::replace_all("  ", "", "word"));
		EXPECT_EQ(""s, fmt::replace_all("  ", " ", ""));
		EXPECT_EQ("  "s, fmt::replace_all("  ", "word", ""));
		EXPECT_EQ("  "s, fmt::replace_all("  ", "word", "drow"));

		EXPECT_EQ("word"s, fmt::replace_all("word", "", ""));
		EXPECT_EQ("word"s, fmt::replace_all("word", "", " "));
		EXPECT_EQ("word"s, fmt::replace_all("word", "", "word"));
		EXPECT_EQ("word"s, fmt::replace_all("word", " ", ""));
		EXPECT_EQ(""s, fmt::replace_all("word", "word", ""));
		EXPECT_EQ("drow"s, fmt::replace_all("word", "word", "drow"));

		EXPECT_EQ(" word "s, fmt::replace_all(" word ", "", ""));
		EXPECT_EQ(" word "s, fmt::replace_all(" word ", "", " "));
		EXPECT_EQ(" word "s, fmt::replace_all(" word ", "", "word"));
		EXPECT_EQ("word"s, fmt::replace_all(" word ", " ", ""));
		EXPECT_EQ("  "s, fmt::replace_all(" word ", "word", ""));
		EXPECT_EQ(" drow "s, fmt::replace_all(" word ", "word", "drow"));

		EXPECT_EQ("word word"s, fmt::replace_all("word word", "", ""));
		EXPECT_EQ("word word"s, fmt::replace_all("word word", "", " "));
		EXPECT_EQ("word word"s, fmt::replace_all("word word", "", "word"));
		EXPECT_EQ("wordword"s, fmt::replace_all("word word", " ", ""));
		EXPECT_EQ(" "s, fmt::replace_all("word word", "word", ""));
		EXPECT_EQ("drow drow"s, fmt::replace_all("word word", "word", "drow"));

		// Test count
		EXPECT_EQ("word word word"s, fmt::replace_all("word word word", "word", "drow", 0));
		EXPECT_EQ("drow word word"s, fmt::replace_all("word word word", "word", "drow", 1));
		EXPECT_EQ("drow drow word"s, fmt::replace_all("word word word", "word", "drow", 2));
		EXPECT_EQ("drow drow drow"s, fmt::replace_all("word word word", "word", "drow", 3));
		EXPECT_EQ("drow drow drow"s, fmt::replace_all("word word word", "word", "drow", umax));
		EXPECT_EQ("drow drow drow"s, fmt::replace_all("word word word", "word", "drow", -1));
	}

	TEST(StrUtil, Split)
	{
		using vec = std::vector<std::string>;

		EXPECT_EQ(vec{""}, fmt::split("", {}, false));
		EXPECT_EQ(vec{""}, fmt::split("", {""}, false));
		EXPECT_EQ(vec{""}, fmt::split("", {" "}, false));
		EXPECT_EQ(vec{""}, fmt::split("", {"a"}, false));
		EXPECT_EQ(vec{""}, fmt::split("", {"a "}, false));
		EXPECT_EQ(vec{""}, fmt::split("", {"a b"}, false));
		EXPECT_EQ(vec{""}, fmt::split("", {"a", " "}, false));
		EXPECT_EQ(vec{""}, fmt::split("", {"a", " ", "b"}, false));

		EXPECT_EQ(vec{" "}, fmt::split(" ", {}, false));
		EXPECT_EQ(vec{" "}, fmt::split(" ", {""}, false));
		EXPECT_EQ(vec{""}, fmt::split(" ", {" "}, false));
		EXPECT_EQ(vec{" "}, fmt::split(" ", {"a"}, false));
		EXPECT_EQ(vec{" "}, fmt::split(" ", {"a "}, false));
		EXPECT_EQ(vec{" "}, fmt::split(" ", {"a b"}, false));
		EXPECT_EQ(vec{""}, fmt::split(" ", {"a", " "}, false));
		EXPECT_EQ(vec{""}, fmt::split(" ", {"a", " ", "b"}, false));

		EXPECT_EQ(vec{"  "}, fmt::split("  ", {}, false));
		EXPECT_EQ(vec{"  "}, fmt::split("  ", {""}, false));
		EXPECT_EQ(vec({"", ""}), fmt::split("  ", {" "}, false));
		EXPECT_EQ(vec{"  "}, fmt::split("  ", {"a"}, false));
		EXPECT_EQ(vec{"  "}, fmt::split("  ", {"a "}, false));
		EXPECT_EQ(vec{"  "}, fmt::split("  ", {"a b"}, false));
		EXPECT_EQ(vec({"", ""}), fmt::split("  ", {"a", " "}, false));
		EXPECT_EQ(vec({"", ""}), fmt::split("  ", {"a", " ", "b"}, false));

		EXPECT_EQ(vec{"a"}, fmt::split("a", {}, false));
		EXPECT_EQ(vec{"a"}, fmt::split("a", {""}, false));
		EXPECT_EQ(vec{"a"}, fmt::split("a", {" "}, false));
		EXPECT_EQ(vec{""}, fmt::split("a", {"a"}, false));
		EXPECT_EQ(vec{"a"}, fmt::split("a", {"a "}, false));
		EXPECT_EQ(vec{"a"}, fmt::split("a", {"a b"}, false));
		EXPECT_EQ(vec{""}, fmt::split("a", {"a", " "}, false));
		EXPECT_EQ(vec{""}, fmt::split("a", {"a", " ", "b"}, false));

		EXPECT_EQ(vec{"aa"}, fmt::split("aa", {}, false));
		EXPECT_EQ(vec{"aa"}, fmt::split("aa", {""}, false));
		EXPECT_EQ(vec{"aa"}, fmt::split("aa", {" "}, false));
		EXPECT_EQ(vec({"", ""}), fmt::split("aa", {"a"}, false));
		EXPECT_EQ(vec{"aa"}, fmt::split("aa", {"a "}, false));
		EXPECT_EQ(vec{"aa"}, fmt::split("aa", {"a b"}, false));
		EXPECT_EQ(vec({"", ""}), fmt::split("aa", {"a", " "}, false));
		EXPECT_EQ(vec({"", ""}), fmt::split("aa", {"a", " ", "b"}, false));

		EXPECT_EQ(vec{"a b"}, fmt::split("a b", {}, false));
		EXPECT_EQ(vec{"a b"}, fmt::split("a b", {""}, false));
		EXPECT_EQ(vec({"a", "b"}), fmt::split("a b", {" "}, false));
		EXPECT_EQ(vec({"", " b"}), fmt::split("a b", {"a"}, false));
		EXPECT_EQ(vec({"", "b"}), fmt::split("a b", {"a "}, false));
		EXPECT_EQ(vec{""}, fmt::split("a b", {"a b"}, false));
		EXPECT_EQ(vec({"", "", "b"}), fmt::split("a b", {"a", " "}, false));
		EXPECT_EQ(vec({"", "", ""}), fmt::split("a b", {"a", " ", "b"}, false));

		EXPECT_EQ(vec{"a b c c b a"}, fmt::split("a b c c b a", {}, false));
		EXPECT_EQ(vec{"a b c c b a"}, fmt::split("a b c c b a", {""}, false));
		EXPECT_EQ(vec({"a", "b", "c", "c", "b", "a"}), fmt::split("a b c c b a", {" "}, false));
		EXPECT_EQ(vec({"", " b c c b "}), fmt::split("a b c c b a", {"a"}, false));
		EXPECT_EQ(vec({"", "b c c b a"}), fmt::split("a b c c b a", {"a "}, false));
		EXPECT_EQ(vec({"", " c c b a"}), fmt::split("a b c c b a", {"a b"}, false));
		EXPECT_EQ(vec({"", "", "b", "c", "c", "b", ""}), fmt::split("a b c c b a", {"a", " "}, false));
		EXPECT_EQ(vec({"", "", "", "", "c", "c", "", "", ""}), fmt::split("a b c c b a", {"a", " ", "b"}, false));

		EXPECT_EQ(vec{" This is a test! "}, fmt::split(" This is a test! ", {}, false));
		EXPECT_EQ(vec{" This is a test! "}, fmt::split(" This is a test! ", {""}, false));
		EXPECT_EQ(vec({"", "This", "is", "a", "test!"}), fmt::split(" This is a test! ", {" "}, false));
		EXPECT_EQ(vec({" This is ", " test! "}), fmt::split(" This is a test! ", {"a"}, false));
		EXPECT_EQ(vec({" This is ", "test! "}), fmt::split(" This is a test! ", {"a "}, false));
		EXPECT_EQ(vec{" This is a test! "}, fmt::split(" This is a test! ", {"a b"}, false));
		EXPECT_EQ(vec({"", "This", "is", "", "", "test!"}), fmt::split(" This is a test! ", {"a", " "}, false));
		EXPECT_EQ(vec({"", "This", "is", "", "", "test!"}), fmt::split(" This is a test! ", {"a", " ", "b"}, false));

		EXPECT_EQ(vec{}, fmt::split("", {}, true));
		EXPECT_EQ(vec{}, fmt::split("", {""}, true));
		EXPECT_EQ(vec{}, fmt::split("", {" "}, true));
		EXPECT_EQ(vec{}, fmt::split("", {"a"}, true));
		EXPECT_EQ(vec{}, fmt::split("", {"a "}, true));
		EXPECT_EQ(vec{}, fmt::split("", {"a b"}, true));
		EXPECT_EQ(vec{}, fmt::split("", {"a", " "}, true));
		EXPECT_EQ(vec{}, fmt::split("", {"a", " ", "b"}, true));

		EXPECT_EQ(vec{" "}, fmt::split(" ", {}, true));
		EXPECT_EQ(vec{" "}, fmt::split(" ", {""}, true));
		EXPECT_EQ(vec{}, fmt::split(" ", {" "}, true));
		EXPECT_EQ(vec{" "}, fmt::split(" ", {"a"}, true));
		EXPECT_EQ(vec{" "}, fmt::split(" ", {"a "}, true));
		EXPECT_EQ(vec{" "}, fmt::split(" ", {"a b"}, true));
		EXPECT_EQ(vec{}, fmt::split(" ", {"a", " "}, true));
		EXPECT_EQ(vec{}, fmt::split(" ", {"a", " ", "b"}, true));

		EXPECT_EQ(vec{"  "}, fmt::split("  ", {}, true));
		EXPECT_EQ(vec{"  "}, fmt::split("  ", {""}, true));
		EXPECT_EQ(vec{}, fmt::split("  ", {" "}, true));
		EXPECT_EQ(vec{"  "}, fmt::split("  ", {"a"}, true));
		EXPECT_EQ(vec{"  "}, fmt::split("  ", {"a "}, true));
		EXPECT_EQ(vec{"  "}, fmt::split("  ", {"a b"}, true));
		EXPECT_EQ(vec{}, fmt::split("  ", {"a", " "}, true));
		EXPECT_EQ(vec{}, fmt::split("  ", {"a", " ", "b"}, true));

		EXPECT_EQ(vec{"a"}, fmt::split("a", {}, true));
		EXPECT_EQ(vec{"a"}, fmt::split("a", {""}, true));
		EXPECT_EQ(vec{"a"}, fmt::split("a", {" "}, true));
		EXPECT_EQ(vec{}, fmt::split("a", {"a"}, true));
		EXPECT_EQ(vec{"a"}, fmt::split("a", {"a "}, true));
		EXPECT_EQ(vec{"a"}, fmt::split("a", {"a b"}, true));
		EXPECT_EQ(vec{}, fmt::split("a", {"a", " "}, true));
		EXPECT_EQ(vec{}, fmt::split("a", {"a", " ", "b"}, true));

		EXPECT_EQ(vec{"aa"}, fmt::split("aa", {}, true));
		EXPECT_EQ(vec{"aa"}, fmt::split("aa", {""}, true));
		EXPECT_EQ(vec{"aa"}, fmt::split("aa", {" "}, true));
		EXPECT_EQ(vec{}, fmt::split("aa", {"a"}, true));
		EXPECT_EQ(vec{"aa"}, fmt::split("aa", {"a "}, true));
		EXPECT_EQ(vec{"aa"}, fmt::split("aa", {"a b"}, true));
		EXPECT_EQ(vec{}, fmt::split("aa", {"a", " "}, true));
		EXPECT_EQ(vec{}, fmt::split("aa", {"a", " ", "b"}, true));

		EXPECT_EQ(vec{"a b"}, fmt::split("a b", {}, true));
		EXPECT_EQ(vec{"a b"}, fmt::split("a b", {""}, true));
		EXPECT_EQ(vec({"a", "b"}), fmt::split("a b", {" "}, true));
		EXPECT_EQ(vec{" b"}, fmt::split("a b", {"a"}, true));
		EXPECT_EQ(vec{"b"}, fmt::split("a b", {"a "}, true));
		EXPECT_EQ(vec{}, fmt::split("a b", {"a b"}, true));
		EXPECT_EQ(vec{"b"}, fmt::split("a b", {"a", " "}, true));
		EXPECT_EQ(vec{}, fmt::split("a b", {"a", " ", "b"}, true));

		EXPECT_EQ(vec{"a b c c b a"}, fmt::split("a b c c b a", {}, true));
		EXPECT_EQ(vec{"a b c c b a"}, fmt::split("a b c c b a", {""}, true));
		EXPECT_EQ(vec({"a", "b", "c", "c", "b", "a"}), fmt::split("a b c c b a", {" "}, true));
		EXPECT_EQ(vec{" b c c b "}, fmt::split("a b c c b a", {"a"}, true));
		EXPECT_EQ(vec{"b c c b a"}, fmt::split("a b c c b a", {"a "}, true));
		EXPECT_EQ(vec{" c c b a"}, fmt::split("a b c c b a", {"a b"}, true));
		EXPECT_EQ(vec({"b", "c", "c", "b"}), fmt::split("a b c c b a", {"a", " "}, true));
		EXPECT_EQ(vec({"c", "c"}), fmt::split("a b c c b a", {"a", " ", "b"}, true));

		EXPECT_EQ(vec{" This is a test! "}, fmt::split(" This is a test! ", {}, true));
		EXPECT_EQ(vec{" This is a test! "}, fmt::split(" This is a test! ", {""}, true));
		EXPECT_EQ(vec({"This", "is", "a", "test!"}), fmt::split(" This is a test! ", {" "}, true));
		EXPECT_EQ(vec({" This is ", " test! "}), fmt::split(" This is a test! ", {"a"}, true));
		EXPECT_EQ(vec({" This is ", "test! "}), fmt::split(" This is a test! ", {"a "}, true));
		EXPECT_EQ(vec{" This is a test! "}, fmt::split(" This is a test! ", {"a b"}, true));
		EXPECT_EQ(vec({"This", "is", "test!"}), fmt::split(" This is a test! ", {"a", " "}, true));
		EXPECT_EQ(vec({"This", "is", "test!"}), fmt::split(" This is a test! ", {"a", " ", "b"}, true));
	}

	TEST(StrUtil, Merge)
	{
		using vec = std::vector<std::string>;
		using lst = std::initializer_list<std::vector<std::string>>;

		// Vector of strings
		EXPECT_EQ(""s, fmt::merge(vec{}, ""));
		EXPECT_EQ(""s, fmt::merge(vec{}, " "));
		EXPECT_EQ(""s, fmt::merge(vec{}, "-"));
		EXPECT_EQ(""s, fmt::merge(vec{}, " *-* "));

		EXPECT_EQ(""s, fmt::merge(vec{""}, ""));
		EXPECT_EQ(""s, fmt::merge(vec{""}, " "));
		EXPECT_EQ(""s, fmt::merge(vec{""}, "-"));
		EXPECT_EQ(""s, fmt::merge(vec{""}, " *-* "));

		EXPECT_EQ("a"s, fmt::merge(vec{"a"}, ""));
		EXPECT_EQ("a"s, fmt::merge(vec{"a"}, " "));
		EXPECT_EQ("a"s, fmt::merge(vec{"a"}, "-"));
		EXPECT_EQ("a"s, fmt::merge(vec{"a"}, " *-* "));

		EXPECT_EQ("ab"s, fmt::merge(vec{"a", "b"}, ""));
		EXPECT_EQ("a b"s, fmt::merge(vec{"a", "b"}, " "));
		EXPECT_EQ("a-b"s, fmt::merge(vec{"a", "b"}, "-"));
		EXPECT_EQ("a *-* b"s, fmt::merge(vec{"a", "b"}, " *-* "));

		EXPECT_EQ("abc"s, fmt::merge(vec{"a", "b", "c"}, ""));
		EXPECT_EQ("a b c"s, fmt::merge(vec{"a", "b", "c"}, " "));
		EXPECT_EQ("a-b-c"s, fmt::merge(vec{"a", "b", "c"}, "-"));
		EXPECT_EQ("a *-* b *-* c"s, fmt::merge(vec{"a", "b", "c"}, " *-* "));

		// Initializer list of vector of strings
		EXPECT_EQ(""s, fmt::merge(lst{}, ""));
		EXPECT_EQ(""s, fmt::merge(lst{}, " "));
		EXPECT_EQ(""s, fmt::merge(lst{}, "-"));
		EXPECT_EQ(""s, fmt::merge(lst{}, " *-* "));

		EXPECT_EQ(""s, fmt::merge(lst{vec{}}, ""));
		EXPECT_EQ(""s, fmt::merge(lst{vec{}}, " "));
		EXPECT_EQ(""s, fmt::merge(lst{vec{}}, "-"));
		EXPECT_EQ(""s, fmt::merge(lst{vec{}}, " *-* "));

		EXPECT_EQ("a"s, fmt::merge(lst{vec{"a"}}, ""));
		EXPECT_EQ("a"s, fmt::merge(lst{vec{"a"}}, " "));
		EXPECT_EQ("a"s, fmt::merge(lst{vec{"a"}}, "-"));
		EXPECT_EQ("a"s, fmt::merge(lst{vec{"a"}}, " *-* "));

		EXPECT_EQ("ab"s, fmt::merge(lst{vec{"a", "b"}}, ""));
		EXPECT_EQ("a b"s, fmt::merge(lst{vec{"a", "b"}}, " "));
		EXPECT_EQ("a-b"s, fmt::merge(lst{vec{"a", "b"}}, "-"));
		EXPECT_EQ("a *-* b"s, fmt::merge(lst{vec{"a", "b"}}, " *-* "));

		EXPECT_EQ("abc"s, fmt::merge(lst{vec{"a", "b", "c"}}, ""));
		EXPECT_EQ("a b c"s, fmt::merge(lst{vec{"a", "b", "c"}}, " "));
		EXPECT_EQ("a-b-c"s, fmt::merge(lst{vec{"a", "b", "c"}}, "-"));
		EXPECT_EQ("a *-* b *-* c"s, fmt::merge(lst{vec{"a", "b", "c"}}, " *-* "));

		EXPECT_EQ("ab"s, fmt::merge(lst{vec{"a"}, vec{"b"}}, ""));
		EXPECT_EQ("a b"s, fmt::merge(lst{vec{"a"}, vec{"b"}}, " "));
		EXPECT_EQ("a-b"s, fmt::merge(lst{vec{"a"}, vec{"b"}}, "-"));
		EXPECT_EQ("a *-* b"s, fmt::merge(lst{vec{"a"}, vec{"b"}}, " *-* "));

		EXPECT_EQ("abc"s, fmt::merge(lst{vec{"a"}, vec{"b"}, vec{"c"}}, ""));
		EXPECT_EQ("a b c"s, fmt::merge(lst{vec{"a"}, vec{"b"}, vec{"c"}}, " "));
		EXPECT_EQ("a-b-c"s, fmt::merge(lst{vec{"a"}, vec{"b"}, vec{"c"}}, "-"));
		EXPECT_EQ("a *-* b *-* c"s, fmt::merge(lst{vec{"a"}, vec{"b"}, vec{"c"}}, " *-* "));

		EXPECT_EQ("a1b2"s, fmt::merge(lst{vec{"a", "1"}, vec{"b", "2"}}, ""));
		EXPECT_EQ("a 1 b 2"s, fmt::merge(lst{vec{"a", "1"}, vec{"b", "2"}}, " "));
		EXPECT_EQ("a-1-b-2"s, fmt::merge(lst{vec{"a", "1"}, vec{"b", "2"}}, "-"));
		EXPECT_EQ("a *-* 1 *-* b *-* 2"s, fmt::merge(lst{vec{"a", "1"}, vec{"b", "2"}}, " *-* "));
	}

	TEST(StrUtil, GetFileExtension)
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

	TEST(StrUtil, StrcpyTrunc)
	{
		char dst[13];
		std::memset(dst, 'A', sizeof(dst));
		strcpy_trunc(dst, "");
		EXPECT_TRUE(std::all_of(dst, dst + sizeof(dst), [](char c){ return c == '\0'; }));

		std::memset(dst, 'A', sizeof(dst));
		strcpy_trunc(dst, "Hello");
		EXPECT_EQ('\0', dst[5]);
		EXPECT_EQ(std::string(dst), "Hello");
		EXPECT_TRUE(std::all_of(dst + 5, dst + sizeof(dst), [](char c){ return c == '\0'; }));

		std::memset(dst, 'A', sizeof(dst));
		strcpy_trunc(dst, "Hello World!");
		EXPECT_EQ('\0', dst[12]);
		EXPECT_EQ(std::string(dst), "Hello World!");

		std::memset(dst, 'A', sizeof(dst));
		strcpy_trunc(dst, "Hello World! Here I am!");
		EXPECT_EQ('\0', dst[12]);
		EXPECT_EQ(std::string(dst), "Hello World!");
	}
}
