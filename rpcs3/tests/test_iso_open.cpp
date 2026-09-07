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

TEST_F(iso_test, MembersRetainSourceAfterArchiveDestruction)
{
	fs::file first;
	fs::file second;
	{
		iso_archive archive(m_path);
		ASSERT_TRUE(archive.is_valid());
		first = fs::file(archive.open("TEST.BIN"));
		second = fs::file(archive.open("TEST.BIN"));
	}
	ASSERT_TRUE(first);
	ASSERT_TRUE(second);
	EXPECT_EQ(first.seek(2), 2);
	char byte = 0;
	EXPECT_EQ(second.read(&byte, 1), 1);
	EXPECT_EQ(byte, 'a');
	EXPECT_EQ(first.read(&byte, 1), 1);
	EXPECT_EQ(byte, 'c');
	first.close();
	EXPECT_EQ(second.read(&byte, 1), 1);
	EXPECT_EQ(byte, 'b');
}

#ifndef _WIN32
TEST_F(iso_test, PathReplacementKeepsMountedSource)
{
	iso_archive archive(m_path);
	ASSERT_TRUE(archive.is_valid());
	std::filesystem::rename(m_path, m_path + ".old");
	m_image[20 * ISO_SECTOR_SIZE] = 'X';
	{
		std::ofstream replacement(m_path, std::ios::binary);
		replacement.write(reinterpret_cast<const char*>(m_image.data()), m_image.size());
		ASSERT_TRUE(replacement.good());
	}
	fs::file original(archive.open("TEST.BIN"));
	iso_archive new_archive(m_path);
	fs::file replacement(new_archive.open("TEST.BIN"));
	ASSERT_TRUE(original);
	ASSERT_TRUE(replacement);
	char byte = 0;
	EXPECT_EQ(original.read(&byte, 1), 1);
	EXPECT_EQ(byte, 'a');
	EXPECT_EQ(replacement.read(&byte, 1), 1);
	EXPECT_EQ(byte, 'X');
}
#endif

TEST_F(iso_test, MemberSeekingNeverMovesSourceCursor)
{
	auto source = std::make_shared<fs::file>(m_path);
	ASSERT_TRUE(*source);
	ASSERT_EQ(source->seek(123), 123);
	iso_file member(source, false, { .extents = {{20, 3}} });
	EXPECT_EQ(member.seek(1, fs::seek_set), 1);
	EXPECT_EQ(member.seek(10, fs::seek_cur), 11);
	EXPECT_EQ(member.seek(-1, fs::seek_end), 2);
	EXPECT_EQ(member.seek(-4, fs::seek_end), umax);
	EXPECT_EQ(source->pos(), 123);
	char byte = 0;
	EXPECT_EQ(member.read(&byte, 1), 1);
	EXPECT_EQ(byte, 'c');
	member.release();
	EXPECT_TRUE(*source);
	EXPECT_EQ(source->pos(), 123);
}
