#include <gtest/gtest.h>

#include "util/tuple.hpp"

struct some_struct
{
	u64 v {};
	char s[12] = "Hello World";

	bool operator == (const some_struct& r) const
	{
		return v == r.v && std::memcmp(s, r.s, sizeof(s)) == 0;
	}
};

TEST(Utils, Tuple)
{
	some_struct s {};
	s.v = 1234;

	utils::tuple t0 = {};
	EXPECT_EQ(t0.size(), 0);

	utils::tuple<int> t;
	EXPECT_EQ(sizeof(t), sizeof(int));
	EXPECT_TRUE((std::is_same_v<decltype(t.get<0>()), int&>));
	EXPECT_EQ(t.size(), 1);
	EXPECT_EQ(t.get<0>(), 0);

	utils::tuple<int> t1 = 2;
	EXPECT_EQ(sizeof(t1), sizeof(int));
	EXPECT_TRUE((std::is_same_v<decltype(t1.get<0>()), int&>));
	EXPECT_EQ(t1.size(), 1);
	EXPECT_EQ(t1.get<0>(), 2);
	t1 = {};
	EXPECT_EQ(t1.size(), 1);
	EXPECT_EQ(t1.get<0>(), 0);

	utils::tuple<int, some_struct> t2 = { 2, s };
	EXPECT_EQ(sizeof(t2), 32);
	EXPECT_EQ(t2.size(), 2);
	EXPECT_TRUE((std::is_same_v<decltype(t2.get<0>()), int&>));
	EXPECT_TRUE((std::is_same_v<decltype(t2.get<1>()), some_struct&>));
	EXPECT_EQ(t2.get<0>(), 2);
	EXPECT_EQ(t2.get<1>(), s);
	t2 = {};
	EXPECT_EQ(t2.size(), 2);
	EXPECT_EQ(t2.get<0>(), 0);
	EXPECT_EQ(t2.get<1>(), some_struct{});

	t2.get<0>() = 666;
	t2.get<1>() = s;
	EXPECT_EQ(t2.get<0>(), 666);
	EXPECT_EQ(t2.get<1>(), s);

	utils::tuple<int, some_struct, double> t3 = { 2, s, 1234.0 };
	EXPECT_EQ(sizeof(t3), 40);
	EXPECT_EQ(t3.size(), 3);
	EXPECT_TRUE((std::is_same_v<decltype(t3.get<0>()), int&>));
	EXPECT_TRUE((std::is_same_v<decltype(t3.get<1>()), some_struct&>));
	EXPECT_TRUE((std::is_same_v<decltype(t3.get<2>()), double&>));
	EXPECT_EQ(t3.get<0>(), 2);
	EXPECT_EQ(t3.get<1>(), s);
	EXPECT_EQ(t3.get<2>(), 1234.0);
	t3 = {};
	EXPECT_EQ(t3.size(), 3);
	EXPECT_EQ(t3.get<0>(), 0);
	EXPECT_EQ(t3.get<1>(), some_struct{});
	EXPECT_EQ(t3.get<2>(), 0.0);

	t3.get<0>() = 666;
	t3.get<1>() = s;
	t3.get<2>() = 7.0;
	EXPECT_EQ(t3.get<0>(), 666);
	EXPECT_EQ(t3.get<1>(), s);
	EXPECT_EQ(t3.get<2>(), 7.0);

	// const
	const utils::tuple<int, some_struct> tc = { 2, s };
	EXPECT_EQ(tc.size(), 2);
	EXPECT_TRUE((std::is_same_v<decltype(tc.get<0>()), const int&>));
	EXPECT_TRUE((std::is_same_v<decltype(tc.get<1>()), const some_struct&>));
	EXPECT_EQ(tc.get<0>(), 2);
	EXPECT_EQ(tc.get<1>(), s);

	// assignment
	const utils::tuple<int, some_struct> ta1 = { 2, s };
	utils::tuple<int, some_struct> ta = ta1;
	EXPECT_EQ(ta.size(), 2);
	EXPECT_TRUE((std::is_same_v<decltype(ta.get<0>()), int&>));
	EXPECT_TRUE((std::is_same_v<decltype(ta.get<1>()), some_struct&>));
	EXPECT_EQ(ta.get<0>(), 2);
	EXPECT_EQ(ta.get<1>(), s);

	utils::tuple<int, some_struct> ta2 = { 2, s };
	ta = ta2;
	EXPECT_EQ(ta.size(), 2);
	EXPECT_TRUE((std::is_same_v<decltype(ta.get<0>()), int&>));
	EXPECT_TRUE((std::is_same_v<decltype(ta.get<1>()), some_struct&>));
	EXPECT_EQ(ta.get<0>(), 2);
	EXPECT_EQ(ta.get<1>(), s);
	EXPECT_EQ(ta2.size(), 2);
	EXPECT_TRUE((std::is_same_v<decltype(ta2.get<0>()), int&>));
	EXPECT_TRUE((std::is_same_v<decltype(ta2.get<1>()), some_struct&>));
	EXPECT_EQ(ta2.get<0>(), 2);
	EXPECT_EQ(ta2.get<1>(), s);

	ta = std::move(ta2);
	EXPECT_EQ(ta.size(), 2);
	EXPECT_TRUE((std::is_same_v<decltype(ta.get<0>()), int&>));
	EXPECT_TRUE((std::is_same_v<decltype(ta.get<1>()), some_struct&>));
	EXPECT_EQ(ta.get<0>(), 2);
	EXPECT_EQ(ta.get<1>(), s);
}
