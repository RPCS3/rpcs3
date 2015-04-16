#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/FS/vfsStream.h"
#include "PSF.h"

bool PSFLoader::Load(vfsStream& stream)
{
	PSFHeader header;

	// load header
	if (!stream.SRead(header))
	{
		return false;
	}

	// check magic
	if (header.magic != *(u32*)"\0PSF")
	{
		LOG_ERROR(LOADER, "PSFLoader::Load() failed: unknown magic (0x%x)", header.magic);
		return false;
	}

	// check version
	if (header.version != 0x101)
	{
		LOG_ERROR(LOADER, "PSFLoader::Load() failed: unknown version (0x%x)", header.version);
		return false;
	}

	// load indices
	std::vector<PSFDefTable> indices;

	indices.resize(header.entries_num);

	if (!stream.SRead(indices[0], sizeof(PSFDefTable) * header.entries_num))
	{
		return false;
	}

	// load key table
	if (header.off_key_table > header.off_data_table)
	{
		LOG_ERROR(LOADER, "PSFLoader::Load() failed: off_key_table=0x%x, off_data_table=0x%x", header.off_key_table, header.off_data_table);
		return false;
	}

	const u32 key_table_size = header.off_data_table - header.off_key_table;

	std::unique_ptr<char> keys(new char[key_table_size + 1]);

	stream.Seek(header.off_key_table);

	if (stream.Read(keys.get(), key_table_size) != key_table_size)
	{
		return false;
	}

	keys.get()[key_table_size] = 0;

	// load entries
	std::vector<PSFEntry> entries;

	entries.resize(header.entries_num);

	for (u32 i = 0; i < header.entries_num; ++i)
	{
		entries[i].fmt = indices[i].param_fmt;

		if (indices[i].key_off >= key_table_size)
		{
			return false;
		}

		entries[i].name = keys.get() + indices[i].key_off;

		// load data
		stream.Seek(header.off_data_table + indices[i].data_off);

		if (indices[i].param_fmt == PSF_PARAM_INT && indices[i].param_len == 4 && indices[i].param_max == 4)
		{
			// load int data

			if (!stream.SRead(entries[i].vint))
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

			entries[i].vstr = str.get();
		}
		else
		{
			LOG_ERROR(LOADER, "PSFLoader::Load() failed: (i=%d) fmt=0x%x, len=0x%x, max=0x%x", i, indices[i].param_fmt, indices[i].param_len, indices[i].param_max);
			return false;
		}
	}

	// reset data
	m_entries = std::move(entries);

	return true;
}

bool PSFLoader::Save(vfsStream& stream)
{
	std::vector<PSFDefTable> indices;

	indices.resize(m_entries.size());

	// generate header
	PSFHeader header;
	header.magic = *(u32*)"\0PSF";
	header.version = 0x101;
	header.entries_num = static_cast<u32>(m_entries.size());
	header.off_key_table = sizeof(PSFHeader) + sizeof(PSFDefTable) * header.entries_num;
	header.off_data_table = header.off_key_table + [&]() -> u32
	{
		// calculate key table length and generate indices

		u32 key_offset = 0;
		u32 data_offset = 0;

		for (u32 i = 0; i < m_entries.size(); i++)
		{
			indices[i].key_off = key_offset;
			indices[i].data_off = data_offset;
			indices[i].param_fmt = m_entries[i].fmt;

			key_offset += static_cast<u32>(m_entries[i].name.size()) + 1; // key size

			switch (m_entries[i].fmt) // calculate data size
			{
			case PSF_PARAM_STR:
			{
				data_offset += (indices[i].param_len = indices[i].param_max = static_cast<u32>(m_entries[i].vstr.size()) + 1);
				break;
			}
			case PSF_PARAM_INT:
			{
				data_offset += (indices[i].param_len = indices[i].param_max = 4);
				break;
			}
			default:
			{
				data_offset += (indices[i].param_len = indices[i].param_max = 0);
				LOG_ERROR(LOADER, "PSFLoader::Save(): (i=%d) unknown entry format (0x%x, key='%s')", i, m_entries[i].fmt, m_entries[i].name);
			}
			}
		}

		return key_offset;
	}();

	// save header
	if (!stream.SWrite(header))
	{
		return false;
	}

	// save indices
	if (!stream.SWrite(indices[0], sizeof(PSFDefTable) * m_entries.size()))
	{
		return false;
	}

	// save key table
	for (const auto& entry : m_entries)
	{
		if (!stream.SWrite(entry.name[0], entry.name.size() + 1))
		{
			return false;
		}
	}

	// save data
	for (const auto& entry : m_entries)
	{
		switch (entry.fmt)
		{
		case PSF_PARAM_STR:
		{
			if (!stream.SWrite(entry.vstr[0], entry.vstr.size() + 1))
			{
				return false;
			}
			break;
		}
		case PSF_PARAM_INT:
		{
			if (!stream.SWrite(entry.vint))
			{
				return false;
			}
			break;
		}
		}
	}

	return true;
}

void PSFLoader::Clear()
{
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
	AddEntry(key, PSF_PARAM_STR).vstr = value;
}

void PSFLoader::SetInteger(const std::string& key, s32 value)
{
	AddEntry(key, PSF_PARAM_INT).vint = value;
}
