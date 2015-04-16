#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/FS/vfsStream.h"
#include "PSF.h"

bool PSFLoader::Load(vfsStream& stream)
{
	Clear();

	// load header
	if (!stream.SRead(m_header))
	{
		return false;
	}

	// check magic
	if (m_header.magic != *(u32*)"\0PSF")
	{
		LOG_ERROR(LOADER, "PSFLoader::Load() failed: unknown magic (0x%x)", m_header.magic);
		return false;
	}

	// check version
	if (m_header.version != 0x101)
	{
		LOG_ERROR(LOADER, "PSFLoader::Load() failed: unknown version (0x%x)", m_header.version);
		return false;
	}

	// load indices
	std::vector<PSFDefTable> indices;

	indices.resize(m_header.entries_num);

	if (!stream.SRead(indices[0], sizeof(PSFDefTable) * m_header.entries_num))
	{
		return false;
	}

	// load key table
	if (m_header.off_key_table > m_header.off_data_table)
	{
		LOG_ERROR(LOADER, "PSFLoader::Load() failed: off_key_table=0x%x, off_data_table=0x%x", m_header.off_key_table, m_header.off_data_table);
		return false;
	}

	const u32 key_table_size = m_header.off_data_table - m_header.off_key_table;

	std::unique_ptr<char> keys(new char[key_table_size + 1]);

	stream.Seek(m_header.off_key_table);

	if (stream.Read(keys.get(), key_table_size) != key_table_size)
	{
		return false;
	}

	keys.get()[key_table_size] = 0;

	// fill entries
	m_entries.resize(m_header.entries_num);

	for (u32 i = 0; i < m_header.entries_num; ++i)
	{
		m_entries[i].fmt = indices[i].param_fmt;

		if (indices[i].key_off >= key_table_size)
		{
			return false;
		}

		m_entries[i].name = keys.get() + indices[i].key_off;

		// load data
		stream.Seek(m_header.off_data_table + indices[i].data_off);

		if (indices[i].param_fmt == PSF_PARAM_INT && indices[i].param_len == 4 && indices[i].param_max == 4)
		{
			// load int data

			if (!stream.SRead(m_entries[i].vint))
			{
				return false;
			}
		}
		else if (indices[i].param_fmt == PSF_PARAM_STR && indices[i].param_max >= indices[i].param_len)
		{
			// load str data

			const u32 size = indices[i].param_len;

			std::unique_ptr<char> str(new char[size + 1]);

			if (stream.Read(str.get(), size) != size)
			{
				return false;
			}

			str.get()[size] = 0;

			m_entries[i].vstr = str.get();
		}
		else
		{
			LOG_ERROR(LOADER, "PSFLoader::Load() failed: (i=%d) fmt=0x%x, len=0x%x, max=0x%x", i, indices[i].param_fmt, indices[i].param_len, indices[i].param_max);
			return false;
		}
	}

	return (m_loaded = true);
}

bool PSFLoader::Save(vfsStream& stream)
{
	// TODO: Construct m_header

	m_loaded = true;

	// TODO: Save data

	return true;
}

void PSFLoader::Clear()
{
	m_loaded = false;
	m_header = {};
	m_entries.clear();
}

const PSFEntry* PSFLoader::SearchEntry(const std::string& key) const
{
	for (auto& entry : m_entries)
	{
		if (key == entry.name)
		{
			return &entry;
		}
	}

	return nullptr;
}

PSFEntry& PSFLoader::AddEntry(const std::string& key, u16 fmt)
{
	for (auto& entry : m_entries)
	{
		if (key == entry.name)
		{
			entry.fmt = fmt;
			return entry;
		}
	}

	PSFEntry new_entry = {};
	new_entry.fmt = fmt;
	new_entry.name = key;
	m_entries.push_back(new_entry);

	return m_entries.back();
}

std::string PSFLoader::GetString(const std::string& key, std::string def) const
{
	if (const auto entry = SearchEntry(key))
	{
		if (entry->fmt == PSF_PARAM_STR)
		{
			return entry->vstr;
		}
	}

	return def;
}

s32 PSFLoader::GetInteger(const std::string& key, s32 def) const
{
	if (const auto entry = SearchEntry(key))
	{
		if (entry->fmt == PSF_PARAM_INT)
		{
			return entry->vint;
		}
	}
	
	return def;
}

void PSFLoader::SetString(const std::string& key, std::string value)
{
	auto& entry = AddEntry(key, PSF_PARAM_STR);

	entry.vstr = value;
}

void PSFLoader::SetInteger(const std::string& key, s32 value)
{
	auto& entry = AddEntry(key, PSF_PARAM_INT);

	entry.vint = value;
}
