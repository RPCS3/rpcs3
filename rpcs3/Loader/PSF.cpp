#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/FS/vfsStream.h"
#include "PSF.h"

bool PSFLoader::Load(vfsStream& stream)
{
	Close();

	// Load Header
	if (stream.Read(&m_header, sizeof(PSFHeader)) != sizeof(PSFHeader) || !m_header.CheckMagic())
	{
		return false;
	}

	m_psfindxs.resize(m_header.psf_entries_num);
	m_entries.resize(m_header.psf_entries_num);

	// Load Indices
	for (u32 i = 0; i < m_header.psf_entries_num; ++i)
	{
		if (!stream.Read(m_psfindxs[i]))
		{
			return false;
		}

		m_entries[i].fmt = m_psfindxs[i].psf_param_fmt;
	}

	// Load Key Table
	for (u32 i = 0; i < m_header.psf_entries_num; ++i)
	{
		stream.Seek(m_header.psf_offset_key_table + m_psfindxs[i].psf_key_table_offset);

		int c_pos = 0;

		while (c_pos < sizeof(m_entries[i].name) - 1)
		{
			char c;

			if (!stream.Read(c) || !c)
			{
				break;
			}

			m_entries[i].name[c_pos++] = c;
		}

		m_entries[i].name[c_pos] = 0;
	}

	// Load Data Table
	for (u32 i = 0; i < m_header.psf_entries_num; ++i)
	{
		stream.Seek(m_header.psf_offset_data_table + m_psfindxs[i].psf_data_tbl_offset);
		stream.Read(m_entries[i].param, m_psfindxs[i].psf_param_len);
		memset(m_entries[i].param + m_psfindxs[i].psf_param_len, 0, m_psfindxs[i].psf_param_max_len - m_psfindxs[i].psf_param_len);
	}

	return (m_loaded = true);
}

void PSFLoader::Close()
{
	m_loaded = false;
	m_header = {};
	m_psfindxs.clear();
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

std::string PSFLoader::GetString(const std::string& key, std::string def) const
{
	if (const auto entry = SearchEntry(key))
	{
		return entry->FormatString();
	}
	else
	{
		return def;
	}
}

u32 PSFLoader::GetInteger(const std::string& key, u32 def) const
{
	if (const auto entry = SearchEntry(key))
	{
		return entry->FormatInteger();
	}
	else
	{
		return def;
	}
}
