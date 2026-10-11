#include <gtest/gtest.h>

#include "util/cctype.hpp"

namespace utils
{
	TEST(CctypeTest, Test_check_arg)
	{
		for (int i = std::numeric_limits<char>::min(); i <= std::numeric_limits<char>::max(); i++)
		{
			EXPECT_TRUE(utils::check_cctype_arg<char>(static_cast<char>(i)));
			EXPECT_TRUE(utils::check_cctype_arg<s8>(static_cast<s8>(i)));
		}

		for (u32 i = 0; i <= std::numeric_limits<u16>::max(); i++)
		{
			EXPECT_EQ(i < 256, utils::check_cctype_arg<u16>(static_cast<u16>(i)));
		}
	}

	TEST(CctypeTest, Test_isalnum)
	{
		for (int i = std::numeric_limits<char>::min(); i <= std::numeric_limits<char>::max(); i++)
		{
			EXPECT_EQ(::isalnum(static_cast<unsigned char>(i)) != 0, utils::isalnum<char>(static_cast<char>(i)));
			EXPECT_EQ(::isalnum(static_cast<unsigned char>(i)) != 0, utils::isalnum<s8>(static_cast<s8>(i)));
		}

		for (u32 i = 0; i <= std::numeric_limits<u16>::max(); i++)
		{
			if (i < 256)
			{
				EXPECT_EQ(::isalnum(static_cast<unsigned char>(i)) != 0, utils::isalnum<u16>(static_cast<u16>(i)));
			}
			else
			{
				EXPECT_FALSE(utils::isalnum<u16>(static_cast<u16>(i)));
			}
		}
	}

	TEST(CctypeTest, Test_isalpha)
	{
		for (int i = std::numeric_limits<char>::min(); i <= std::numeric_limits<char>::max(); i++)
		{
			EXPECT_EQ(::isalpha(static_cast<unsigned char>(i)) != 0, utils::isalpha<char>(static_cast<char>(i)));
			EXPECT_EQ(::isalpha(static_cast<unsigned char>(i)) != 0, utils::isalpha<s8>(static_cast<s8>(i)));
		}

		for (u32 i = 0; i <= std::numeric_limits<u16>::max(); i++)
		{
			if (i < 256)
			{
				EXPECT_EQ(::isalpha(static_cast<unsigned char>(i)) != 0, utils::isalpha<u16>(static_cast<u16>(i)));
			}
			else
			{
				EXPECT_FALSE(utils::isalpha<u16>(static_cast<u16>(i)));
			}
		}
	}

	TEST(CctypeTest, Test_iscntrl)
	{
		for (int i = std::numeric_limits<char>::min(); i <= std::numeric_limits<char>::max(); i++)
		{
			EXPECT_EQ(::iscntrl(static_cast<unsigned char>(i)) != 0, utils::iscntrl<char>(static_cast<char>(i)));
			EXPECT_EQ(::iscntrl(static_cast<unsigned char>(i)) != 0, utils::iscntrl<s8>(static_cast<s8>(i)));
		}

		for (u32 i = 0; i <= std::numeric_limits<u16>::max(); i++)
		{
			if (i < 256)
			{
				EXPECT_EQ(::iscntrl(static_cast<unsigned char>(i)) != 0, utils::iscntrl<u16>(static_cast<u16>(i)));
			}
			else
			{
				EXPECT_FALSE(utils::iscntrl<u16>(static_cast<u16>(i)));
			}
		}
	}

	TEST(CctypeTest, Test_isdigit)
	{
		for (int i = std::numeric_limits<char>::min(); i <= std::numeric_limits<char>::max(); i++)
		{
			EXPECT_EQ(::isdigit(static_cast<unsigned char>(i)) != 0, utils::isdigit<char>(static_cast<char>(i)));
			EXPECT_EQ(::isdigit(static_cast<unsigned char>(i)) != 0, utils::isdigit<s8>(static_cast<s8>(i)));
		}

		for (u32 i = 0; i <= std::numeric_limits<u16>::max(); i++)
		{
			if (i < 256)
			{
				EXPECT_EQ(::isdigit(static_cast<unsigned char>(i)) != 0, utils::isdigit<u16>(static_cast<u16>(i)));
			}
			else
			{
				EXPECT_FALSE(utils::isdigit<u16>(static_cast<u16>(i)));
			}
		}
	}

	TEST(CctypeTest, Test_isgraph)
	{
		for (int i = std::numeric_limits<char>::min(); i <= std::numeric_limits<char>::max(); i++)
		{
			EXPECT_EQ(::isgraph(static_cast<unsigned char>(i)) != 0, utils::isgraph<char>(static_cast<char>(i)));
			EXPECT_EQ(::isgraph(static_cast<unsigned char>(i)) != 0, utils::isgraph<s8>(static_cast<s8>(i)));
		}

		for (u32 i = 0; i <= std::numeric_limits<u16>::max(); i++)
		{
			if (i < 256)
			{
				EXPECT_EQ(::isgraph(static_cast<unsigned char>(i)) != 0, utils::isgraph<u16>(static_cast<u16>(i)));
			}
			else
			{
				EXPECT_FALSE(utils::isgraph<u16>(static_cast<u16>(i)));
			}
		}
	}

	TEST(CctypeTest, Test_islower)
	{
		for (int i = std::numeric_limits<char>::min(); i <= std::numeric_limits<char>::max(); i++)
		{
			EXPECT_EQ(::islower(static_cast<unsigned char>(i)) != 0, utils::islower<char>(static_cast<char>(i)));
			EXPECT_EQ(::islower(static_cast<unsigned char>(i)) != 0, utils::islower<s8>(static_cast<s8>(i)));
		}

		for (u32 i = 0; i <= std::numeric_limits<u16>::max(); i++)
		{
			if (i < 256)
			{
				EXPECT_EQ(::islower(static_cast<unsigned char>(i)) != 0, utils::islower<u16>(static_cast<u16>(i)));
			}
			else
			{
				EXPECT_FALSE(utils::islower<u16>(static_cast<u16>(i)));
			}
		}
	}

	TEST(CctypeTest, Test_isupper)
	{
		for (int i = std::numeric_limits<char>::min(); i <= std::numeric_limits<char>::max(); i++)
		{
			EXPECT_EQ(::isupper(static_cast<unsigned char>(i)) != 0, utils::isupper<char>(static_cast<char>(i)));
			EXPECT_EQ(::isupper(static_cast<unsigned char>(i)) != 0, utils::isupper<s8>(static_cast<s8>(i)));
		}

		for (u32 i = 0; i <= std::numeric_limits<u16>::max(); i++)
		{
			if (i < 256)
			{
				EXPECT_EQ(::isupper(static_cast<unsigned char>(i)) != 0, utils::isupper<u16>(static_cast<u16>(i)));
			}
			else
			{
				EXPECT_FALSE(utils::isupper<u16>(static_cast<u16>(i)));
			}
		}
	}

	TEST(CctypeTest, Test_isprint)
	{
		for (int i = std::numeric_limits<char>::min(); i <= std::numeric_limits<char>::max(); i++)
		{
			EXPECT_EQ(::isprint(static_cast<unsigned char>(i)) != 0, utils::isprint<char>(static_cast<char>(i)));
			EXPECT_EQ(::isprint(static_cast<unsigned char>(i)) != 0, utils::isprint<s8>(static_cast<s8>(i)));
		}

		for (u32 i = 0; i <= std::numeric_limits<u16>::max(); i++)
		{
			if (i < 256)
			{
				EXPECT_EQ(::isprint(static_cast<unsigned char>(i)) != 0, utils::isprint<u16>(static_cast<u16>(i)));
			}
			else
			{
				EXPECT_FALSE(utils::isprint<u16>(static_cast<u16>(i)));
			}
		}
	}

	TEST(CctypeTest, Test_ispunct)
	{
		for (int i = std::numeric_limits<char>::min(); i <= std::numeric_limits<char>::max(); i++)
		{
			EXPECT_EQ(::ispunct(static_cast<unsigned char>(i)) != 0, utils::ispunct<char>(static_cast<char>(i)));
			EXPECT_EQ(::ispunct(static_cast<unsigned char>(i)) != 0, utils::ispunct<s8>(static_cast<s8>(i)));
		}

		for (u32 i = 0; i <= std::numeric_limits<u16>::max(); i++)
		{
			if (i < 256)
			{
				EXPECT_EQ(::ispunct(static_cast<unsigned char>(i)) != 0, utils::ispunct<u16>(static_cast<u16>(i)));
			}
			else
			{
				EXPECT_FALSE(utils::ispunct<u16>(static_cast<u16>(i)));
			}
		}
	}

	TEST(CctypeTest, Test_isspace)
	{
		for (int i = std::numeric_limits<char>::min(); i <= std::numeric_limits<char>::max(); i++)
		{
			EXPECT_EQ(::isspace(static_cast<unsigned char>(i)) != 0, utils::isspace<char>(static_cast<char>(i)));
			EXPECT_EQ(::isspace(static_cast<unsigned char>(i)) != 0, utils::isspace<s8>(static_cast<s8>(i)));
		}

		for (u32 i = 0; i <= std::numeric_limits<u16>::max(); i++)
		{
			if (i < 256)
			{
				EXPECT_EQ(::isspace(static_cast<unsigned char>(i)) != 0, utils::isspace<u16>(static_cast<u16>(i)));
			}
			else
			{
				EXPECT_FALSE(utils::isspace<u16>(static_cast<u16>(i)));
			}
		}
	}

	TEST(CctypeTest, Test_isblank)
	{
		for (int i = std::numeric_limits<char>::min(); i <= std::numeric_limits<char>::max(); i++)
		{
			EXPECT_EQ(::isblank(static_cast<unsigned char>(i)) != 0, utils::isblank<char>(static_cast<char>(i)));
			EXPECT_EQ(::isblank(static_cast<unsigned char>(i)) != 0, utils::isblank<s8>(static_cast<s8>(i)));
		}

		for (u32 i = 0; i <= std::numeric_limits<u16>::max(); i++)
		{
			if (i < 256)
			{
				EXPECT_EQ(::isblank(static_cast<unsigned char>(i)) != 0, utils::isblank<u16>(static_cast<u16>(i)));
			}
			else
			{
				EXPECT_FALSE(utils::isblank<u16>(static_cast<u16>(i)));
			}
		}
	}

	TEST(CctypeTest, Test_isxdigit)
	{
		for (int i = std::numeric_limits<char>::min(); i <= std::numeric_limits<char>::max(); i++)
		{
			EXPECT_EQ(::isxdigit(static_cast<unsigned char>(i)) != 0, utils::isxdigit<char>(static_cast<char>(i)));
			EXPECT_EQ(::isxdigit(static_cast<unsigned char>(i)) != 0, utils::isxdigit<s8>(static_cast<s8>(i)));
		}

		for (u32 i = 0; i <= std::numeric_limits<u16>::max(); i++)
		{
			if (i < 256)
			{
				EXPECT_EQ(::isxdigit(static_cast<unsigned char>(i)) != 0, utils::isxdigit<u16>(static_cast<u16>(i)));
			}
			else
			{
				EXPECT_FALSE(utils::isxdigit<u16>(static_cast<u16>(i)));
			}
		}
	}

	TEST(CctypeTest, Test_tolower)
	{
		for (int i = std::numeric_limits<char>::min(); i <= std::numeric_limits<char>::max(); i++)
		{
			EXPECT_EQ(::tolower(static_cast<unsigned char>(i)), utils::tolower<char>(static_cast<char>(i)));
			EXPECT_EQ(::tolower(static_cast<unsigned char>(i)), utils::tolower<s8>(static_cast<s8>(i)));
		}

		for (u32 i = 0; i <= std::numeric_limits<u16>::max(); i++)
		{
			if (i < 256)
			{
				EXPECT_EQ(::tolower(static_cast<unsigned char>(i)), utils::tolower<u16>(static_cast<u16>(i)));
			}
			else
			{
				EXPECT_EQ(EOF, utils::tolower<u16>(static_cast<u16>(i)));
			}
		}
	}

	TEST(CctypeTest, Test_toupper)
	{
		for (int i = std::numeric_limits<char>::min(); i <= std::numeric_limits<char>::max(); i++)
		{
			EXPECT_EQ(::toupper(static_cast<unsigned char>(i)), utils::toupper<char>(static_cast<char>(i)));
			EXPECT_EQ(::toupper(static_cast<unsigned char>(i)), utils::toupper<s8>(static_cast<s8>(i)));
		}

		for (u32 i = 0; i <= std::numeric_limits<u16>::max(); i++)
		{
			if (i < 256)
			{
				EXPECT_EQ(::toupper(static_cast<unsigned char>(i)), utils::toupper<u16>(static_cast<u16>(i)));
			}
			else
			{
				EXPECT_EQ(EOF, utils::toupper<u16>(static_cast<u16>(i)));
			}
		}
	}
}
