#pragma once
#include "Loader.h"

struct PSFHeader
{
	u32 psf_magic;
	u32 psf_version;
	u32 psf_offset_key_table;
	u32 psf_offset_data_table;
	u32 psf_entries_num;

	bool CheckMagic() const { return psf_magic == *(u32*)"\0PSF"; }
};

struct PSFDefTbl
{
	u16 psf_key_table_offset;
	u16 psf_param_fmt;
	u32 psf_param_len;
	u32 psf_param_max_len;
	u32 psf_data_tbl_offset;
};

struct PSFEntry
{
	char name[128];
	u16 fmt;
	char param[4096];

	const char* FormatString() const
	{
		switch(fmt)
		{
		default:
		case 0x0400:
		case 0x0402:
			return (const char*)param;
		case 0x0404:
			return (const char*)wxString::Format("0x%x", FormatInteger()).mb_str();
		}
	}

	u32 FormatInteger() const
	{
		return *(u32*)param;
	}
};

class PSFLoader
{
	vfsStream& psf_f;

	PSFHeader m_header;
	std::vector<PSFDefTbl> m_psfindxs;
	std::vector<PSFEntry> m_entries;
	bool m_show_log;

	bool LoadHeader();
	bool LoadKeyTable();
	bool LoadDataTable();

public:
	PSFLoader(vfsStream& f);
	virtual bool Load(bool show = true);
	virtual bool Close();

	PSFEntry* SearchEntry(const std::string& key);
	const char* GetString(const std::string& key);
	u32 GetInteger(const std::string& key);
};