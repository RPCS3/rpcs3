#include "stdafx.h"
#include "TROPUSR.h"

#include "wx/xml/xml.h"

TROPUSRLoader::TROPUSRLoader()
{
	m_file = NULL;
	memset(&m_header, 0, sizeof(m_header));
}

TROPUSRLoader::~TROPUSRLoader()
{
	Close();
}

bool TROPUSRLoader::Load(const std::string& filepath, const std::string& configpath)
{
	if (m_file)
		Close();

	// TODO: This seems to be always true... A bug in ExistsFile() ?
	if (!Emu.GetVFS().ExistsFile(filepath))
		Generate(filepath, configpath);

	m_file = Emu.GetVFS().OpenFile(filepath, vfsRead);
	LoadHeader();
	LoadTableHeaders();
	LoadTables();

	Close();
	return true;
}

bool TROPUSRLoader::LoadHeader()
{
	if(!m_file->IsOpened())
		return false;

	m_file->Seek(0);
	if (m_file->Read(&m_header, sizeof(TROPUSRHeader)) != sizeof(TROPUSRHeader))
		return false;
	return true;
}

bool TROPUSRLoader::LoadTableHeaders()
{
	if(!m_file->IsOpened())
		return false;

	m_file->Seek(0x30);
	m_tableHeaders.clear();
	m_tableHeaders.resize(m_header.tables_count);
	for (TROPUSRTableHeader& tableHeader : m_tableHeaders) {
		if (m_file->Read(&tableHeader, sizeof(TROPUSRTableHeader)) != sizeof(TROPUSRTableHeader))
			return false;
	}
	return true;
}

bool TROPUSRLoader::LoadTables()
{
	if(!m_file->IsOpened())
		return false;

	for (const TROPUSRTableHeader& tableHeader : m_tableHeaders)
	{
		m_file->Seek(tableHeader.offset);

		if (tableHeader.type == 4)
		{
			m_table4.clear();
			m_table4.resize(tableHeader.entries_count);
			for (auto& entry : m_table4) {
				if (m_file->Read(&entry, sizeof(TROPUSREntry4)) != sizeof(TROPUSREntry4))
					return false;
			}
		}

		if (tableHeader.type == 6)
		{
			m_table6.clear();
			m_table6.resize(tableHeader.entries_count);
			for (auto& entry : m_table6) {
				if (m_file->Read(&entry, sizeof(TROPUSREntry6)) != sizeof(TROPUSREntry6))
					return false;
			}
		}

		// TODO: Other tables
	}

	return true;
}

// TODO: TROPUSRLoader::Save deletes the TROPUSR and creates it again. This is probably very slow.
bool TROPUSRLoader::Save(const std::string& filepath)
{
	if (m_file)
		Close();

	if (!Emu.GetVFS().ExistsFile(filepath))
		Emu.GetVFS().CreateFile(filepath);

	m_file = Emu.GetVFS().OpenFile(filepath, vfsWrite);
	m_file->Write(&m_header, sizeof(TROPUSRHeader));

	for (const TROPUSRTableHeader& tableHeader : m_tableHeaders)
		m_file->Write(&tableHeader, sizeof(TROPUSRTableHeader));

	for (const auto& entry : m_table4)
		m_file->Write(&entry, sizeof(TROPUSREntry4));
	for (const auto& entry : m_table6)
		m_file->Write(&entry, sizeof(TROPUSREntry6));

	m_file->Close();
	return true;
}

bool TROPUSRLoader::Generate(const std::string& filepath, const std::string& configpath)
{
	std::string path;
	wxXmlDocument doc;
	Emu.GetVFS().GetDevice(configpath.c_str(), path);
	doc.Load(fmt::FromUTF8(path));

	m_table4.clear();
	m_table6.clear();
	for (wxXmlNode *n = doc.GetRoot()->GetChildren(); n; n = n->GetNext())
	{
		if (n->GetName() == "trophy")
		{
			u32 trophy_id = atoi(n->GetAttribute("id").mb_str());
			u32 trophy_grade;
			switch (((const char *)n->GetAttribute("ttype").mb_str())[0])
			{
			case 'B': trophy_grade = 4; break;
			case 'S': trophy_grade = 3; break;
			case 'G': trophy_grade = 2; break;
			case 'P': trophy_grade = 1; break;
			default: trophy_grade = 0;
			}

			TROPUSREntry4 entry4 = {4, sizeof(TROPUSREntry4)-0x10, m_table4.size(), 0, trophy_id, trophy_grade, 0xFFFFFFFF};
			TROPUSREntry6 entry6 = {6, sizeof(TROPUSREntry6)-0x10, m_table6.size(), 0, trophy_id, 0, 0, 0, 0, 0};

			m_table4.push_back(entry4);
			m_table6.push_back(entry6);
		}
	}

	u64 offset = sizeof(TROPUSRHeader) + 2 * sizeof(TROPUSRTableHeader);
	TROPUSRTableHeader table4header = {4, sizeof(TROPUSREntry4)-0x10, 1, m_table4.size(), offset, 0};
	offset += m_table4.size() * sizeof(TROPUSREntry4);
	TROPUSRTableHeader table6header = {6, sizeof(TROPUSREntry6)-0x10, 1, m_table6.size(), offset, 0};
	offset += m_table6.size() * sizeof(TROPUSREntry6);

	m_tableHeaders.clear();
	m_tableHeaders.push_back(table4header);
	m_tableHeaders.push_back(table6header);

	m_header.magic = 0x818F54AD;
	m_header.unk1 = 0x00010000;
	m_header.tables_count = m_tableHeaders.size();
	m_header.unk2 = 0;

	Save(filepath);
	return true;
}

u32 TROPUSRLoader::GetTrophiesCount()
{
	return m_table6.size();
}

u32 TROPUSRLoader::GetTrophyUnlockState(u32 id)
{
	if (id >=  m_table6.size())
		ConLog.Warning("TROPUSRLoader::GetUnlockState: Invalid id=%d", id);

	return m_table6[id].trophy_state; // Let's assume the trophies are stored ordered
}

u32 TROPUSRLoader::GetTrophyTimestamp(u32 id)
{
	if (id >=  m_table6.size())
		ConLog.Warning("TROPUSRLoader::GetTrophyTimestamp: Invalid id=%d", id);

	// TODO: What timestamp does sceNpTrophyGetTrophyInfo want, timestamp1 or timestamp2? 
	return m_table6[id].timestamp2; // Let's assume the trophies are stored ordered
}

bool TROPUSRLoader::UnlockTrophy(u32 id, u64 timestamp1, u64 timestamp2)
{
	if (id >=  m_table6.size())
		return false;

	m_table6[id].trophy_state = 1;
	m_table6[id].timestamp1 = timestamp1;
	m_table6[id].timestamp2 = timestamp2;
	return true;
}

bool TROPUSRLoader::Close()
{
	if (m_file && m_file->Close())
	{
		safe_delete(m_file);
		return true;
	}
	return false;
}
