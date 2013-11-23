#include "stdafx.h"
#include "PSF.h"

PSFLoader::PSFLoader(vfsStream& f) : psf_f(f)
{
}

bool PSFLoader::Load(bool show)
{
	if(!psf_f.IsOpened()) return false;

	m_show_log = show;

	if(!LoadHdr()) return false;
	if(!LoadKeyTable()) return false;
	if(!LoadValuesTable()) return false;

	if(show)
	{
		ConLog.SkipLn();
		for(uint i=0; i<m_table.GetCount(); ++i)
		{
			ConLog.Write("%s", m_table[i].mb_str());
		}
		ConLog.SkipLn();
	}

	return true;
}

bool PSFLoader::Close()
{
	return psf_f.Close();
}

bool PSFLoader::LoadHdr()
{
	psf_f.Read(&psfhdr, sizeof(PsfHeader));
	if(!psfhdr.CheckMagic()) return false;

	if(m_show_log) ConLog.Write("PSF version: %x", psfhdr.psf_version);

	return true;
}

bool PSFLoader::LoadKeyTable()
{
	psf_f.Seek(psfhdr.psf_offset_key_table);

	m_table.Clear();
	m_table.Add(wxEmptyString);

	while(!psf_f.Eof())
	{
		char c;
		psf_f.Read(&c, 1);
		if(c == 0)
		{
			psf_f.Read(&c, 1);
			if(c == 0) break;

			m_table.Add(wxEmptyString);
		}
		m_table[m_table.GetCount() - 1].Append(c);
	}

	if(m_table.GetCount() != psfhdr.psf_entries_num)
	{
		if(m_show_log) ConLog.Error("PSF error: Entries loaded with error! [%d - %d]", m_table.GetCount(), psfhdr.psf_entries_num);
		m_table.Clear();
		return false;
	}

	return true;
}

struct PsfHelper
{
	static wxString ReadString(vfsStream& f, const u32 size)
	{
		wxString ret = wxEmptyString;

		for(uint i=0; i<size && !f.Eof(); ++i)
		{
			ret += ReadChar(f);
		}

		return ret;
	}

	static wxString ReadString(vfsStream& f)
	{
		wxString ret = wxEmptyString;

		while(!f.Eof())
		{
			const char c = ReadChar(f);
			if(c == 0) break;
			ret += c;
		}
		
		return ret;
	}

	static char ReadChar(vfsStream& f)
	{
		char c;
		f.Read(&c, 1);
		return c;
	}

	static char ReadCharNN(vfsStream& f)
	{
		char c;
		while(!f.Eof())
		{
			f.Read(&c, 1);
			if(c != 0) break;
		}
			
		return c;
	}

	static void GoToNN(vfsStream& f)
	{
		while(!f.Eof())
		{
			char c;
			f.Read(&c, 1);
			if(c != 0)
			{
				f.Seek(f.Tell() - 1);
				break;
			}
		}
	}

	static wxString FixName(const wxString& name)
	{
		wxString ret = wxEmptyString;

		for(uint i=0; i<name.Length(); ++i)
		{
			switch((u8)name[i])
			{
				case 0xE2: case 0xA2: case 0x84: continue;
				default: ret += name[i]; break;
			};
		}

		return ret;
	}
};

bool PSFLoader::LoadValuesTable()
{
	psf_f.Seek(psfhdr.psf_offset_values_table);
	m_info.Reset();

	for(uint i=0;i<m_table.GetCount(); i++)
	{
		if(!m_table[i].Cmp("TITLE_ID"))
		{
			m_info.serial = PsfHelper::ReadString(psf_f);
			m_table[i].Append(wxString::Format(": %s", m_info.serial.mb_str()));
			PsfHelper::GoToNN(psf_f);
		}
		else if(!m_table[i](0, 5).Cmp("TITLE"))
		{
			m_info.name = PsfHelper::FixName(PsfHelper::ReadString(psf_f));
			m_table[i].Append(wxString::Format(": %s", m_info.name.mb_str()));
			PsfHelper::GoToNN(psf_f);
		}
		else if(!m_table[i].Cmp("APP_VER"))
		{
			m_info.app_ver = PsfHelper::ReadString(psf_f, sizeof(u64));
			m_table[i].Append(wxString::Format(": %s", m_info.app_ver.mb_str()));
		}
		else if(!m_table[i].Cmp("ATTRIBUTE"))
		{
			psf_f.Read(&m_info.attr, sizeof(m_info.attr));
			m_table[i].Append(wxString::Format(": 0x%x", m_info.attr));
		}
		else if(!m_table[i].Cmp("CATEGORY"))
		{
			m_info.category = PsfHelper::ReadString(psf_f, sizeof(u32));
			m_table[i].Append(wxString::Format(": %s", m_info.category.mb_str()));
		}
		else if(!m_table[i].Cmp("BOOTABLE"))
		{
			psf_f.Read(&m_info.bootable, sizeof(m_info.bootable));
			m_table[i].Append(wxString::Format(": %d", m_info.bootable));
		}
		else if(!m_table[i].Cmp("LICENSE"))
		{
			m_table[i].Append(wxString::Format(": %s", PsfHelper::ReadString(psf_f).mb_str()));
			psf_f.Seek(psf_f.Tell() + (sizeof(u64) * 7 * 2) - 1);
		}
		else if(!m_table[i](0, 14).Cmp("PARENTAL_LEVEL"))
		{
			u32 buf;
			psf_f.Read(&buf, sizeof(buf));
			if(!m_table[i].Cmp("PARENTAL_LEVEL"))
			{
				m_info.parental_lvl = buf;
			}
			m_table[i].Append(wxString::Format(": %d", buf));
		}
		else if(!m_table[i].Cmp("PS3_SYSTEM_VER"))
		{
			m_info.fw =  PsfHelper::ReadString(psf_f, sizeof(u64));
			m_table[i].Append(wxString::Format(": %s", m_info.fw.mb_str()));
		}
		else if(!m_table[i].Cmp("SOUND_FORMAT"))
		{
			m_info.sound_format = Read32(psf_f);
			m_table[i].Append(wxString::Format(": 0x%x", m_info.sound_format));
		}
		else if(!m_table[i].Cmp("RESOLUTION"))
		{
			m_info.resolution = Read32(psf_f);
			m_table[i].Append(wxString::Format(": 0x%x", m_info.resolution));
		}
		else
		{
			m_table[i].Append(wxString::Format(": %s", PsfHelper::ReadString(psf_f).mb_str()));
			PsfHelper::GoToNN(psf_f);
		}
	}

	if(m_info.serial.Length() == 9)
	{
		m_info.serial = m_info.serial(0, 4) + "-" + m_info.serial(4, 5);
	}

	return true;
}
