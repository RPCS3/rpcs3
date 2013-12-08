#pragma once
#include "Loader.h"

struct PsfHeader
{
	u32 psf_magic;
	u32 psf_version;
	u32 psf_offset_key_table;
	u32 psf_offset_data_table;
	u32 psf_entries_num;

	bool CheckMagic() const { return psf_magic == *(u32*)"\0PSF"; }
};

struct PsfDefTbl
{
	u16 psf_key_table_offset;
	u16 psf_param_fmt;
	u32 psf_param_len;
	u32 psf_param_max_len;
	u32 psf_data_tbl_offset;
};

struct PsfEntry
{
	char name[128];
	u16 fmt;
	char param[4096];

	std::string Format() const
	{
		switch(fmt)
		{
		default:
		case 0x0400:
		case 0x0402:
			return FormatString();

		case 0x0404:
			return wxString::Format("0x%x", FormatInteger()).c_str();
		}
	}

	u32 FormatInteger() const
	{
		return *(u32*)param;
	}

	char* FormatString() const
	{
		return (char*)param;
	}
};

class PSFLoader
{
	vfsStream& psf_f;
	bool m_show_log;

public:
	PSFLoader(vfsStream& f);

	Array<PsfEntry> m_entries;

	PsfEntry* SearchEntry(const std::string& key);

	//wxArrayString m_table;
	GameInfo m_info;
	PsfHeader psfhdr;
	Array<PsfDefTbl> m_psfindxs;
	virtual bool Load(bool show = true);
	virtual bool Close();

private:
	bool LoadHdr();
	bool LoadKeyTable();
	bool LoadDataTable();
};