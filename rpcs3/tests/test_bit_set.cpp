#include <gtest/gtest.h>

#include "util/types.hpp"

namespace utils
{
	TEST(BitSetTest, DefaultConstructedIsEmpty)
	{
		bit_set<8> bits;

		EXPECT_TRUE(bits.none());
		EXPECT_FALSE(bits.any());
		EXPECT_FALSE(bits.all());
		EXPECT_EQ(bits.count(), 0u);
		EXPECT_EQ(bits.size(), 8u);
		EXPECT_EQ(bits.to_ulong(), 0u);
		EXPECT_EQ(bits.to_ullong(), 0u);
	}

	TEST(BitSetTest, ConstructFromValue)
	{
		bit_set<8> bits(0b10101010);

		EXPECT_TRUE(bits.test(1));
		EXPECT_TRUE(bits.test(3));
		EXPECT_TRUE(bits.test(5));
		EXPECT_TRUE(bits.test(7));

		EXPECT_FALSE(bits.test(0));
		EXPECT_FALSE(bits.test(2));
		EXPECT_FALSE(bits.test(4));
		EXPECT_FALSE(bits.test(6));

		EXPECT_EQ(bits.count(), 4u);
	}

	TEST(BitSetTest, SetAndResetBit)
	{
		bit_set<16> bits;

		bits.set(3);

		EXPECT_TRUE(bits.test(3));
		EXPECT_EQ(bits.count(), 1u);

		bits.reset(3);

		EXPECT_FALSE(bits.test(3));
		EXPECT_EQ(bits.count(), 0u);
	}

	TEST(BitSetTest, SetWithExplicitValue)
	{
		bit_set<8> bits;

		bits.set(2, true);
		EXPECT_TRUE(bits.test(2));

		bits.set(2, false);
		EXPECT_FALSE(bits.test(2));
	}

	TEST(BitSetTest, ResetAllBits)
	{
		bit_set<8> bits(0xFF);

		EXPECT_TRUE(bits.all());

		bits.reset();

		EXPECT_TRUE(bits.none());
		EXPECT_EQ(bits.count(), 0u);
	}

	TEST(BitSetTest, OperatorBracketUsesTest)
	{
		bit_set<8> bits;

		bits.set(5);

		EXPECT_TRUE(bits[5]);
		EXPECT_FALSE(bits[4]);
	}

	TEST(BitSetTest, BitwiseAnd)
	{
		bit_set<8> a(0b11110000);
		bit_set<8> b(0b10101010);

		a &= b;

		EXPECT_EQ(a.to_ulong(), 0b10100000u);
	}

	TEST(BitSetTest, BitwiseOr)
	{
		bit_set<8> a(0b11110000);
		bit_set<8> b(0b00001111);

		a |= b;

		EXPECT_EQ(a.to_ulong(), 0b11111111u);
	}

	TEST(BitSetTest, BitwiseXor)
	{
		bit_set<8> a(0b11110000);
		bit_set<8> b(0b10101010);

		a ^= b;

		EXPECT_EQ(a.to_ulong(), 0b01011010u);
	}

	TEST(BitSetTest, LeftShift)
	{
		bit_set<8> bits(0b00001111);

		bits <<= 2;

		EXPECT_EQ(bits.to_ulong(), 0b00111100u);
	}

	TEST(BitSetTest, RightShift)
	{
		bit_set<8> bits(0b11110000);

		bits >>= 2;

		EXPECT_EQ(bits.to_ulong(), 0b00111100u);
	}

	TEST(BitSetTest, LeftShiftOperator)
	{
		bit_set<8> bits(0b00001111);

		const auto shifted = bits << 1;

		EXPECT_EQ(shifted.to_ulong(), 0b00011110u);

		// Original unchanged
		EXPECT_EQ(bits.to_ulong(), 0b00001111u);
	}

	TEST(BitSetTest, RightShiftOperator)
	{
		bit_set<8> bits(0b11110000);

		const auto shifted = bits >> 1;

		EXPECT_EQ(shifted.to_ulong(), 0b01111000u);

		EXPECT_EQ(bits.to_ulong(), 0b11110000u);
	}

	TEST(BitSetTest, AnyNoneAll)
	{
		bit_set<4> bits;

		EXPECT_TRUE(bits.none());
		EXPECT_FALSE(bits.any());
		EXPECT_FALSE(bits.all());

		bits.set(0);
		EXPECT_TRUE(bits.any());
		EXPECT_FALSE(bits.none());
		EXPECT_FALSE(bits.all());

		bits.set(1).set(2).set(3);
		EXPECT_TRUE(bits.all());
	}

	TEST(BitSetTest, UnsafeAccess)
	{
		bit_set<8> bits;

		bits.set_unsafe(6);

		EXPECT_TRUE(bits.test_unsafe(6));

		bits.reset_unsafe(6);

		EXPECT_FALSE(bits.test_unsafe(6));
	}

	TEST(BitSetTest, ToULong)
	{
		bit_set<32> bits(12345);

		EXPECT_EQ(bits.to_ulong(), 12345ul);
	}

	TEST(BitSetTest, ToULLong)
	{
		bit_set<64> bits(123456789ull);

		EXPECT_EQ(bits.to_ullong(), 123456789ull);
	}
}
