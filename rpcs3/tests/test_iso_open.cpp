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

TEST_F(iso_test, TruncatedMetadataRejectsArchive)
{
	for (usz length : {16 * ISO_SECTOR_SIZE + 157, 18 * ISO_SECTOR_SIZE, 18 * ISO_SECTOR_SIZE + 10, 18 * ISO_SECTOR_SIZE + 33})
	{
		SCOPED_TRACE(length);
		{
			std::ofstream out(m_path, std::ios::binary);
			out.write(reinterpret_cast<const char*>(m_image.data()), length);
			ASSERT_TRUE(out.good());
		}
		iso_archive archive(m_path);
		EXPECT_FALSE(archive.is_valid());
	}
}

TEST_F(iso_test, InvalidDirectoryRecordRejectsArchive)
{
	const auto original = m_image;
	for (bool invalid_length : {false, true})
	{
		SCOPED_TRACE(invalid_length);
		m_image = original;
		if (invalid_length)
		{
			m_image[18 * ISO_SECTOR_SIZE] = 1;
		}
		else
		{
			m_image[18 * ISO_SECTOR_SIZE + 32] = 255;
		}
		{
			std::ofstream out(m_path, std::ios::binary);
			out.write(reinterpret_cast<const char*>(m_image.data()), m_image.size());
			ASSERT_TRUE(out.good());
		}
		iso_archive archive(m_path);
		EXPECT_FALSE(archive.is_valid());
	}
}
