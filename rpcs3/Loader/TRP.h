#pragma once
#include "Loader.h"

struct TRPHeader
{
	u32 trp_magic;
	u32 trp_version;
	u64 trp_file_size;
	u32 trp_files_count;
	u32 trp_element_size;
	u32 trp_unknown;
	unsigned char sha1[20];
	unsigned char padding[16];

	bool CheckMagic() const {
		return trp_magic == 0xDCA23D00;
	}
};

struct TRPEntry
{
	char name[20];
	u64 offset;
	u64 size;
	u32 unknown;
	char padding[12];
};

class TRPLoader
{
	vfsStream& trp_f;
	TRPHeader m_header;
	std::vector<TRPEntry> m_entries;

public:
	TRPLoader(vfsStream& f);
	virtual bool Install(std::string dest, bool show = false);
	virtual bool LoadHeader(bool show = false);
	virtual bool Close();
};