#include "iso_test_fixture.h"

namespace
{
	class counted_file final : public fs::file_base
	{
		fs::file m_file;
		u64& m_reads;

	public:
		counted_file(fs::file file, u64& reads)
			: m_file(std::move(file)), m_reads(reads)
		{
		}

		bool trunc(u64 length) override
		{
			return m_file.trunc(length);
		}

		u64 read(void* buffer, u64 size) override
		{
			++m_reads;
			return m_file.read(buffer, size);
		}

		u64 read_at(u64 offset, void* buffer, u64 size) override
		{
			++m_reads;
			return m_file.read_at(offset, buffer, size);
		}

		u64 write(const void* buffer, u64 size) override
		{
			return m_file.write(buffer, size);
		}

		u64 seek(s64 offset, fs::seek_mode whence) override
		{
			return m_file.seek(offset, whence);
		}

		u64 size() override
		{
			return m_file.size();
		}
	};

	template <typename Base>
	class observed_iso_file final : public Base
	{
	public:
		template <typename... Args>
		observed_iso_file(u64& reads, Args&&... args)
			: Base(std::forward<Args>(args)...)
		{
			this->m_file = fs::file(std::make_unique<counted_file>(std::move(this->m_file), reads));
		}
	};
}

TEST_F(iso_test, ExtentEofDoesNotReadBackingFile)
{
	for (bool encrypted_path : {false, true})
	{
		for (bool multiple_extents : {false, true})
		{
			SCOPED_TRACE(testing::Message() << "encrypted path=" << encrypted_path << ", multiple extents=" << multiple_extents);
			iso_fs_node node;
			node.metadata.extents = {{20, 3}};
			if (multiple_extents)
			{
				node.metadata.extents.insert(node.metadata.extents.begin(), {22, ISO_SECTOR_SIZE});
			}
			u64 reads = 0;
			std::unique_ptr<iso_file> member;
			if (encrypted_path)
			{
				member = std::make_unique<observed_iso_file<iso_file_encrypted>>(reads, m_path, fs::read, node, std::make_shared<iso_file_decryption>());
			}
			else
			{
				member = std::make_unique<observed_iso_file<iso_file>>(reads, m_path, fs::read, node);
			}
			for (u64 offset : {member->size(), member->size() + 1, member->size() + ISO_SECTOR_SIZE, u64{umax}})
			{
				SCOPED_TRACE(offset);
				std::array<u8, 8> buffer;
				buffer.fill(0x5a);
				const auto expected = buffer;
				const u64 before = reads;
				EXPECT_EQ(member->read_at(offset, buffer.data(), buffer.size()), 0);
				EXPECT_EQ(reads, before);
				EXPECT_EQ(buffer, expected);
			}
		}
	}
}

TEST_F(iso_test, ReadsAcrossExtentsStopAtLogicalEof)
{
	iso_fs_node node;
	node.metadata.extents = {{22, ISO_SECTOR_SIZE}, {20, 3}};
	iso_file member(m_path, fs::read, node);
	std::array<char, 8> buffer{};
	EXPECT_EQ(member.read_at(ISO_SECTOR_SIZE - 1, buffer.data(), buffer.size()), 4);
	EXPECT_EQ(buffer[0], '\0');
	EXPECT_EQ(std::string(buffer.data() + 1, 3), "abc");
	EXPECT_EQ(buffer[4], '\0');
	EXPECT_EQ(member.seek(member.size() + 1, fs::seek_set), member.size() + 1);
	EXPECT_EQ(member.read(buffer.data(), buffer.size()), 0);
}

TEST_F(iso_test, EmptyMemberDoesNotReadBackingFile)
{
	iso_fs_node node;
	node.metadata.extents = {{20, 0}};
	u64 reads = 0;
	observed_iso_file<iso_file> member(reads, m_path, fs::read, node);
	char byte = 'x';
	EXPECT_EQ(member.read_at(0, &byte, 1), 0);
	EXPECT_EQ(member.read_at(1, &byte, 1), 0);
	EXPECT_EQ(member.read_at(0, &byte, 0), 0);
	EXPECT_EQ(reads, 0);
	EXPECT_EQ(byte, 'x');
}
