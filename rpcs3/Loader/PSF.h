#pragma once

struct vfsStream;

enum class psf_entry_format : u16
{
	string_not_null_term = 0x0004,
	string = 0x0204,
	integer = 0x0404,
};

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
	psf_entry_format param_fmt;
	u32 param_len;
	u32 param_max;
	u32 data_off;
};

struct PSFEntry
{
	psf_entry_format fmt;
	std::string name;

	s32 vint;
	std::string vstr;
};

class PSFLoader
{
	std::vector<PSFEntry> m_entries;

public:
	PSFLoader() = default;

	PSFLoader(vfsStream& stream)
	{
		Load(stream);
	}

	virtual ~PSFLoader() = default;

	bool Load(vfsStream& stream);
	bool Save(vfsStream& stream) const;
	void Clear();

	explicit operator bool() const
	{
		return !m_entries.empty();
	}

	const PSFEntry* SearchEntry(const std::string& key) const;
	PSFEntry& AddEntry(const std::string& key, psf_entry_format format);
	std::string GetString(const std::string& key, std::string def = "") const;
	s32 GetInteger(const std::string& key, s32 def = 0) const;
	void SetString(const std::string& key, std::string value);
	void SetInteger(const std::string& key, s32 value);

	std::vector<PSFEntry>& entries()
	{
		return m_entries;
	}

	const std::vector<PSFEntry>& entries() const
	{
		return m_entries;
	}

	std::vector<PSFEntry>::iterator begin()
	{
		return m_entries.begin();
	}

	std::vector<PSFEntry>::iterator end()
	{
		return m_entries.end();
	}
};
