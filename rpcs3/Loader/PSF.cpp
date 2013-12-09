#include "stdafx.h"
#include "PSF.h"

PSFLoader::PSFLoader(vfsStream& f) : psf_f(f)
{
}

PsfEntry* PSFLoader::SearchEntry(const std::string& key)
{
	for(uint i=0; i<m_entries.GetCount(); ++i)
	{
		if(m_entries[i].name == key)
			return &m_entries[i];
	}

	return nullptr;
}

bool PSFLoader::Load(bool show)
{
	if(!psf_f.IsOpened()) return false;

	m_show_log = show;

	if(!LoadHdr()) return false;
	if(!LoadKeyTable()) return false;
	if(!LoadDataTable()) return false;

	return true;
}

bool PSFLoader::Close()
{
	return psf_f.Close();
}

bool PSFLoader::LoadHdr()
{
	if(psf_f.Read(&psfhdr, sizeof(PsfHeader)) != sizeof(PsfHeader))
		return false;

	if(!psfhdr.CheckMagic()) return false;

	if(m_show_log) ConLog.Write("PSF version: %x", psfhdr.psf_version);

	m_psfindxs.Clear();
	m_entries.Clear();
	m_psfindxs.SetCount(psfhdr.psf_entries_num);
	m_entries.SetCount(psfhdr.psf_entries_num);

	for(u32 i=0; i<psfhdr.psf_entries_num; ++i)
	{
		if(psf_f.Read(&m_psfindxs[i], sizeof(PsfDefTbl)) != sizeof(PsfDefTbl))
			return false;

		m_entries[i].fmt = m_psfindxs[i].psf_param_fmt;
	}

	return true;
}

bool PSFLoader::LoadKeyTable()
{
	for(u32 i=0; i<psfhdr.psf_entries_num; ++i)
	{
		psf_f.Seek(psfhdr.psf_offset_key_table + m_psfindxs[i].psf_key_table_offset);

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
	for(u32 i=0; i<psfhdr.psf_entries_num; ++i)
	{
		psf_f.Seek(psfhdr.psf_offset_data_table + m_psfindxs[i].psf_data_tbl_offset);
		psf_f.Read(m_entries[i].param, m_psfindxs[i].psf_param_len);
		memset(m_entries[i].param + m_psfindxs[i].psf_param_len, 0, m_psfindxs[i].psf_param_max_len - m_psfindxs[i].psf_param_len);
	}

	m_info.Reset();

	if(PsfEntry* entry = SearchEntry("TITLE_ID"))		m_info.serial = entry->Format();
	if(PsfEntry* entry = SearchEntry("TITLE"))			m_info.name = entry->Format();
	if(PsfEntry* entry = SearchEntry("APP_VER"))		m_info.app_ver = entry->Format();
	if(PsfEntry* entry = SearchEntry("CATEGORY"))		m_info.category = entry->Format();
	if(PsfEntry* entry = SearchEntry("PS3_SYSTEM_VER"))	m_info.fw = entry->Format();
	if(PsfEntry* entry = SearchEntry("SOUND_FORMAT"))	m_info.sound_format = entry->FormatInteger();
	if(PsfEntry* entry = SearchEntry("RESOLUTION"))		m_info.resolution = entry->FormatInteger();
	if(PsfEntry* entry = SearchEntry("PARENTAL_LEVEL"))	m_info.parental_lvl = entry->FormatInteger();


	if(m_info.serial.Length() == 9)
	{
		m_info.serial = m_info.serial(0, 4) + "-" + m_info.serial(4, 5);
	}

	return true;
}
