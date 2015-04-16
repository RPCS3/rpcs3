#pragma once

struct vfsStream;

struct PSFHeader
{
	u32 magic;
	u32 version;
	u32 off_key_table;
	u32 off_data_table;
	u32 entries_num;
};

struct PSFDefTable
{
	u16 key_off;
	u16 param_fmt;
	u32 param_len;
	u32 param_max;
	u32 data_off;
};

enum : u16
{
	PSF_PARAM_UNK = 0x0004,
	PSF_PARAM_STR = 0x0204,
	PSF_PARAM_INT = 0x0404,
};

struct PSFEntry
{
	u16 fmt;
	std::string name;

	s32 vint;
	std::string vstr;
};

class PSFLoader
{
	std::vector<PSFEntry> m_entries;

public:
	PSFLoader(vfsStream& stream)
	{
		Load(stream);
	}

	virtual ~PSFLoader() = default;

	bool Load(vfsStream& stream);

	bool Save(vfsStream& stream);

	void Clear();

	operator bool() const
	{
		return !m_entries.empty();
	}

	const PSFEntry* SearchEntry(const std::string& key) const;

	PSFEntry& PSFLoader::AddEntry(const std::string& key, u16 type);

	std::string GetString(const std::string& key, std::string def = "") const;

	s32 GetInteger(const std::string& key, s32 def = 0) const;

	void SetString(const std::string& key, std::string value);

	void SetInteger(const std::string& key, s32 value);
};
