#pragma once

struct vfsStream;

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

	std::string FormatString() const
	{
		switch(fmt)
		{
		default:
		case 0x0400:
		case 0x0402:
			return std::string(param);
		case 0x0404:
			return fmt::Format("0x%x", FormatInteger());
		}
	}

	u32 FormatInteger() const
	{
		return *(u32*)param;
	}
};

class PSFLoader
{
	bool m_loaded = false;
	PSFHeader m_header = {};
	std::vector<PSFDefTbl> m_psfindxs;
	std::vector<PSFEntry> m_entries;

public:
	PSFLoader(vfsStream& stream)
	{
		Load(stream);
	}

	virtual ~PSFLoader() = default;

	bool Load(vfsStream& stream);

	void Close();

	operator bool() const
	{
		return m_loaded;
	}

	const PSFEntry* SearchEntry(const std::string& key) const;
	std::string GetString(const std::string& key, std::string def = "") const;
	u32 GetInteger(const std::string& key, u32 def = 0) const;
};
