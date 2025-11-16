#include <gtest/gtest.h>

#include "util/types.hpp"
#include "util/pair.hpp"

struct some_struct
{
	u64 v {};
	char s[12] = "Hello World";

	bool operator == (const some_struct& r) const
	{
		return v == r.v && std::memcmp(s, r.s, sizeof(s)) == 0;
	}
};

TEST(Utils, Pair)
{
	some_struct s {};
	s.v = 1234;

	utils::pair<int, some_struct> p;
	EXPECT_EQ(sizeof(p), 32);
	EXPECT_EQ(p.first, 0);
	EXPECT_EQ(p.second, some_struct{});

	p = { 666, s };
	EXPECT_EQ(p.first, 666);
	EXPECT_EQ(p.second, s);

	const utils::pair<int, some_struct> p1 = p;
	EXPECT_EQ(p.first, 666);
	EXPECT_EQ(p.second, s);
	EXPECT_EQ(p1.first, 666);
	EXPECT_EQ(p1.second, s);

	utils::pair<int, some_struct> p2 = p1;
	EXPECT_EQ(p1.first, 666);
	EXPECT_EQ(p1.second, s);
	EXPECT_EQ(p2.first, 666);
	EXPECT_EQ(p2.second, s);

	utils::pair<int, some_struct> p3 = std::move(p);
	EXPECT_EQ(p3.first, 666);
	EXPECT_EQ(p3.second, s);
}
