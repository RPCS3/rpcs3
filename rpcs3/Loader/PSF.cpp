#include "stdafx.h"
#include "PSF.h"

PSFLoader::PSFLoader(vfsStream& f) : psf_f(f)
{
}

PSFEntry* PSFLoader::SearchEntry(const std::string& key)
{
	for(auto& entry : m_entries)
	{
		if(entry.name == key)
			return &entry;
	}

	return nullptr;
}

bool PSFLoader::Load(bool show)
{
	if(!psf_f.IsOpened()) return false;

	m_show_log = show;

	if(!LoadHeader()) return false;
	if(!LoadKeyTable()) return false;
	if(!LoadDataTable()) return false;

	return true;
}

bool PSFLoader::Close()
{
	return psf_f.Close();
}

bool PSFLoader::LoadHeader()
{
	if(psf_f.Read(&m_header, sizeof(PSFHeader)) != sizeof(PSFHeader))
		return false;

	if(!m_header.CheckMagic())
		return false;

	if(m_show_log) ConLog.Write("PSF version: %x", m_header.psf_version);

	m_psfindxs.clear();
	m_entries.clear();
	m_psfindxs.resize(m_header.psf_entries_num);
	m_entries.resize(m_header.psf_entries_num);

	for(u32 i=0; i<m_header.psf_entries_num; ++i)
	{
		if(psf_f.Read(&m_psfindxs[i], sizeof(PSFDefTbl)) != sizeof(PSFDefTbl))
			return false;

		m_entries[i].fmt = m_psfindxs[i].psf_param_fmt;
	}

	return true;
}

bool PSFLoader::LoadKeyTable()
{
	for(u32 i=0; i<m_header.psf_entries_num; ++i)
	{
		psf_f.Seek(m_header.psf_offset_key_table + m_psfindxs[i].psf_key_table_offset);

		int c_pos = 0;

		while(!psf_f.Eof())
		{
			char c;
			psf_f.Read(&c, 1);
			m_entries[i].name[c_pos++] = c;

			if(c_pos >= sizeof(m_entries[i].name) || c == '\0')
				break;
		}
	}

	return true;
}

bool PSFLoader::LoadDataTable()
{
	for(u32 i=0; i<m_header.psf_entries_num; ++i)
	{
		psf_f.Seek(m_header.psf_offset_data_table + m_psfindxs[i].psf_data_tbl_offset);
		psf_f.Read(m_entries[i].param, m_psfindxs[i].psf_param_len);
		memset(m_entries[i].param + m_psfindxs[i].psf_param_len, 0, m_psfindxs[i].psf_param_max_len - m_psfindxs[i].psf_param_len);
	}

	return true;
}

std::string PSFLoader::GetString(const std::string& key)
{
	if(PSFEntry* entry = SearchEntry(key))
		return entry->FormatString();
	else
		return "";
}

u32 PSFLoader::GetInteger(const std::string& key)
{
	if(PSFEntry* entry = SearchEntry(key))
		return entry->FormatInteger();
	else
		return 0;
}
