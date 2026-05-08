#include <gtest/gtest.h>
#include "Emu/NP/np_helpers.h"

namespace np
{
	TEST(NpHelpers, ParsesCanonicalEtherAddress)
	{
		const auto ether = string_to_ether_addr("02:00:00:00:00:01");

		ASSERT_TRUE(ether);
		EXPECT_EQ((std::array<u8, 6>{0x02, 0x00, 0x00, 0x00, 0x00, 0x01}), *ether);
		EXPECT_TRUE(is_valid_ether_addr(*ether));
		EXPECT_EQ("02:00:00:00:00:01", ether_to_string(*ether));
	}

	TEST(NpHelpers, RejectsInvalidEtherAddresses)
	{
		EXPECT_FALSE(string_to_ether_addr("00:00:00:00:00:00"));
		EXPECT_FALSE(string_to_ether_addr("ff:ff:ff:ff:ff:ff"));
		EXPECT_FALSE(string_to_ether_addr("01:00:00:00:00:00"));
		EXPECT_FALSE(string_to_ether_addr("03:00:00:00:00:00"));
		EXPECT_FALSE(string_to_ether_addr("02:00:00:00:00"));
		EXPECT_FALSE(string_to_ether_addr("02:00:00:00:00:001"));
		EXPECT_FALSE(string_to_ether_addr("2:00:00:00:00:01"));
		EXPECT_FALSE(string_to_ether_addr("02-00-00-00-00-01"));
		EXPECT_FALSE(string_to_ether_addr("02:00:00:00:00:0g"));
		EXPECT_FALSE(string_to_ether_addr(" 02:00:00:00:00:01"));
	}

	TEST(NpHelpers, GeneratedEtherAddressIsValidDeterministicAndLocallyAdministered)
	{
		const u128 psid = (u128{0x0123456789abcdefull} << 64) | u128{0xfedcba9876543210ull};

		const auto ether1 = generate_emulated_ether_addr(psid, 1);
		const auto ether2 = generate_emulated_ether_addr(psid, 1);

		EXPECT_EQ(ether1, ether2);
		EXPECT_TRUE(is_valid_ether_addr(ether1));
		EXPECT_EQ(0x00, ether1[0] & 0x01); // Unicast.
		EXPECT_EQ(0x02, ether1[0] & 0x02); // Locally administered.
	}

	TEST(NpHelpers, GeneratedEtherAddressChangesWithSeedInputs)
	{
		const u128 psid1 = (u128{0x0123456789abcdefull} << 64) | u128{0xfedcba9876543210ull};
		const u128 psid2 = (u128{0x1123456789abcdefull} << 64) | u128{0xfedcba9876543210ull};

		EXPECT_NE(generate_emulated_ether_addr(psid1, 1), generate_emulated_ether_addr(psid1, 2));
		EXPECT_NE(generate_emulated_ether_addr(psid1, 1), generate_emulated_ether_addr(psid2, 1));
	}
}
