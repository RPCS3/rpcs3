#include "iso_test_fixture.h"

TEST_F(iso_test, MissingMemberReturnsEmptyHandle)
{
	iso_archive archive(m_path);
	ASSERT_TRUE(archive.is_valid());
	EXPECT_FALSE(archive.open("MISSING.BIN"));
	EXPECT_EQ(fs::g_tls_error, fs::error::noent);
	EXPECT_FALSE(archive.open("TEST.BIN/MISSING"));
	EXPECT_FALSE(archive.open(""));

	fs::file member(archive.open("TEST.BIN"));
	ASSERT_TRUE(member);
	std::array<char, 3> data{};
	EXPECT_EQ(member.read(data.data(), data.size()), data.size());
	EXPECT_EQ(std::string(data.data(), data.size()), "abc");
}

TEST_F(iso_test, InvalidArchiveReturnsEmptyHandle)
{
	iso_archive archive(m_path + ".missing");
	ASSERT_FALSE(archive.is_valid());
	EXPECT_FALSE(archive.open("TEST.BIN"));
}
