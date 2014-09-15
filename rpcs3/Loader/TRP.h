#pragma once
#include "Loader.h"

struct vfsStream;

struct TRPHeader
{
	be_t<u32> trp_magic;
	be_t<u32> trp_version;
	be_t<u64> trp_file_size;
	be_t<u32> trp_files_count;
	be_t<u32> trp_element_size;
	be_t<u32> trp_unknown;
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

class TRPLoader
{
	vfsStream& trp_f;
	TRPHeader m_header;
	std::vector<TRPEntry> m_entries;

public:
	TRPLoader(vfsStream& f);
	~TRPLoader();
	virtual bool Install(std::string dest, bool show = false);
	virtual bool LoadHeader(bool show = false);

	virtual bool ContainsEntry(const char *filename);
	virtual void RemoveEntry(const char *filename);
	virtual void RenameEntry(const char *oldname, const char *newname);

	virtual bool Close();
};