#pragma once

struct TRPHeader
{
	be_t<u32> trp_magic;
	be_t<u32> trp_version;
	be_t<u64> trp_file_size;
	be_t<u32> trp_files_count;
	be_t<u32> trp_element_size;
	be_t<u32> trp_dev_flag;
	unsigned char sha1[20];
	unsigned char padding[16];
};

struct TRPEntry
{
	char name[32];
	be_t<u64> offset;
	be_t<u64> size;
	be_t<u32> unknown;
	char padding[12];
};

class TRPLoader final
{
	const fs::file& trp_f;
	TRPHeader m_header{};
	std::vector<TRPEntry> m_entries{};

public:
	TRPLoader(const fs::file& f);

	bool Install(std::string_view dest, bool show = false);
	bool LoadHeader(bool show = false);
	u64 GetRequiredSpace() const;

	bool ContainsEntry(std::string_view filename);
	void RemoveEntry(std::string_view filename);
	void RenameEntry(std::string_view oldname, std::string_view newname);
};
