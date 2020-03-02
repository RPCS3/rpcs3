#include "stdafx.h"
#include "restore_new.h"
#include "Utilities/rXml.h"
#include "define_new_memleakdetect.h"
#include "Emu/VFS.h"
#include "TROPUSR.h"

LOG_CHANNEL(trp_log, "Trophy");

bool TROPUSRLoader::Load(const std::string& filepath, const std::string& configpath)
{
	const std::string& path = vfs::get(filepath);

	if (!m_file.open(path, fs::read))
	{
		if (!Generate(filepath, configpath))
		{
			return false;
		}
	}

	if (!LoadHeader() || !LoadTableHeaders() || !LoadTables())
	{
		return false;
	}

	m_file.release();
	return true;
}

bool TROPUSRLoader::LoadHeader()
{
	if (!m_file)
	{
		return false;
	}

	m_file.seek(0);

	if (!m_file.read(m_header))
	{
		return false;
	}

	return true;
}

bool TROPUSRLoader::LoadTableHeaders()
{
	if (!m_file)
	{
		return false;
	}

	m_file.seek(0x30);
	m_tableHeaders.clear();
	m_tableHeaders.resize(m_header.tables_count);

	for (TROPUSRTableHeader& tableHeader : m_tableHeaders)
	{
		if (!m_file.read(tableHeader))
			return false;
	}

	return true;
}

bool TROPUSRLoader::LoadTables()
{
	if (!m_file)
	{
		return false;
	}

	for (const TROPUSRTableHeader& tableHeader : m_tableHeaders)
	{
		m_file.seek(tableHeader.offset);

		if (tableHeader.type == 4u)
		{
			m_table4.clear();
			m_table4.resize(tableHeader.entries_count);

			for (auto& entry : m_table4)
			{
				if (!m_file.read(entry))
					return false;
			}
		}

		if (tableHeader.type == 6u)
		{
			m_table6.clear();
			m_table6.resize(tableHeader.entries_count);

			for (auto& entry : m_table6)
			{
				if (!m_file.read(entry))
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
	if (!m_file.open(vfs::get(filepath), fs::rewrite))
	{
		return false;
	}

	m_file.write(m_header);

	for (const TROPUSRTableHeader& tableHeader : m_tableHeaders)
	{
		m_file.write(tableHeader);
	}

	for (const auto& entry : m_table4)
	{
		m_file.write(entry);
	}

	for (const auto& entry : m_table6)
	{
		m_file.write(entry);
	}

	m_file.release();
	return true;
}

bool TROPUSRLoader::Generate(const std::string& filepath, const std::string& configpath)
{
	fs::file config(vfs::get(configpath));

	if (!config)
	{
		return false;
	}

	rXmlDocument doc;
	doc.Read(config.to_string());

	m_table4.clear();
	m_table6.clear();

	auto trophy_base = doc.GetRoot();
	if (trophy_base->GetChildren()->GetName() == "trophyconf")
	{
		trophy_base = trophy_base->GetChildren();
	}

	for (std::shared_ptr<rXmlNode> n = trophy_base->GetChildren(); n; n = n->GetNext())
	{
		if (n->GetName() == "trophy")
		{
			const u32 trophy_id = std::atoi(n->GetAttribute("id").c_str());
			const u32 trophy_pid = std::atoi(n->GetAttribute("pid").c_str());

			u32 trophy_grade;
			switch (n->GetAttribute("ttype")[0])
			{
			case 'B': trophy_grade = trophy_grade::bronze; break;
			case 'S': trophy_grade = trophy_grade::silver; break;
			case 'G': trophy_grade = trophy_grade::gold; break;
			case 'P': trophy_grade = trophy_grade::platinum; break;
			default: trophy_grade = trophy_grade::unknown; break;
			}

			TROPUSREntry4 entry4 = { 4, u32{sizeof(TROPUSREntry4)} - 0x10, ::size32(m_table4), 0, trophy_id, trophy_grade, trophy_pid };
			TROPUSREntry6 entry6 = { 6, u32{sizeof(TROPUSREntry6)} - 0x10, ::size32(m_table6), 0, trophy_id };

			m_table4.push_back(entry4);
			m_table6.push_back(entry6);
		}
	}

	u64 offset = sizeof(TROPUSRHeader) + 2 * sizeof(TROPUSRTableHeader);
	TROPUSRTableHeader table4header = { 4, u32{sizeof(TROPUSREntry4)} - 0x10, 1, ::size32(m_table4), offset };
	offset += m_table4.size() * sizeof(TROPUSREntry4);
	TROPUSRTableHeader table6header = { 6, u32{sizeof(TROPUSREntry6)} - 0x10, 1, ::size32(m_table6), offset };
	offset += m_table6.size() * sizeof(TROPUSREntry6);

	m_tableHeaders.clear();
	m_tableHeaders.push_back(table4header);
	m_tableHeaders.push_back(table6header);

	m_header.magic = 0x818F54AD;
	m_header.unk1 = 0x00010000;
	m_header.tables_count = ::size32(m_tableHeaders);
	m_header.unk2 = 0;

	Save(filepath);

	return true;
}

u32 TROPUSRLoader::GetTrophiesCount()
{
	return ::size32(m_table6);
}

u32 TROPUSRLoader::GetUnlockedTrophiesCount()
{
	u32 count = 0;
	for (const auto& trophy : m_table6)
	{
		if (trophy.trophy_state)
		{
			count++;
		}
	}
	return count;
}

u32 TROPUSRLoader::GetUnlockedPlatinumID(u32 trophy_id, const std::string& config_path)
{
	constexpr u32 invalid_trophy_id = -1; // SCE_NP_TROPHY_INVALID_TROPHY_ID;

	if (trophy_id >= m_table6.size() || trophy_id >= m_table4.size())
	{
		trp_log.warning("TROPUSRLoader::GetUnlockedPlatinumID: Invalid id=%d", trophy_id);
		return invalid_trophy_id;
	}

	if (m_table6.size() != m_table4.size())
	{
		trp_log.warning("TROPUSRLoader::GetUnlockedPlatinumID: Table size mismatch: %d vs. %d", m_table6.size(), m_table4.size());
		return invalid_trophy_id;
	}

	// We need to read the trophy info from file here and update it for backwards compatibility.
	// TROPUSRLoader::Generate will currently not be called on existing trophy data which might lack the pid.
	fs::file config(config_path);

	if (!config)
	{
		return invalid_trophy_id;
	}

	rXmlDocument doc;
	doc.Read(config.to_string());

	auto trophy_base = doc.GetRoot();
	if (trophy_base->GetChildren()->GetName() == "trophyconf")
	{
		trophy_base = trophy_base->GetChildren();
	}

	const size_t trophy_count = m_table4.size();

	for (std::shared_ptr<rXmlNode> n = trophy_base->GetChildren(); n; n = n->GetNext())
	{
		if (n->GetName() == "trophy")
		{
			const u32 trophy_id = std::atoi(n->GetAttribute("id").c_str());
			const u32 trophy_pid = std::atoi(n->GetAttribute("pid").c_str());

			// We currently assume that trophies are ordered
			if (trophy_id < trophy_count && m_table4[trophy_id].trophy_id == trophy_id)
			{
				// Update the pid for backwards compatibility
				m_table4[trophy_id].trophy_pid = trophy_pid;
			}
		}
	}

	// Get this trophy's platinum link id
	const u32 pid = m_table4[trophy_id].trophy_pid;

	// The platinum trophy has to have a valid id and must still be locked
	if (pid == invalid_trophy_id || GetTrophyUnlockState(pid)) // the first check is redundant but I'll keep it to prevent regressions
	{
		return invalid_trophy_id;
	}

	// The platinum trophy stays locked if any relevant trophy is still locked
	for (size_t i = 0; i < trophy_count; i++)
	{
		if (m_table4[i].trophy_pid == pid && !m_table6[i].trophy_state)
		{
			return invalid_trophy_id;
		}
	}

	// All relevant trophies for this platinum link id were unlocked
	return pid;
}

u32 TROPUSRLoader::GetTrophyGrade(u32 id)
{
	if (id >= m_table4.size())
	{
		trp_log.warning("TROPUSRLoader::GetTrophyGrade: Invalid id=%d", id);
		return trophy_grade::unknown;
	}

	return m_table4[id].trophy_grade; // Let's assume the trophies are stored ordered
}

u32 TROPUSRLoader::GetTrophyUnlockState(u32 id)
{
	if (id >= m_table6.size())
	{
		trp_log.warning("TROPUSRLoader::GetTrophyUnlockState: Invalid id=%d", id);
		return 0;
	}

	return m_table6[id].trophy_state; // Let's assume the trophies are stored ordered
}

u64 TROPUSRLoader::GetTrophyTimestamp(u32 id)
{
	if (id >= m_table6.size())
	{
		trp_log.warning("TROPUSRLoader::GetTrophyTimestamp: Invalid id=%d", id);
		return 0;
	}

	// TODO: What timestamp does sceNpTrophyGetTrophyInfo want, timestamp1 or timestamp2?
	return m_table6[id].timestamp2; // Let's assume the trophies are stored ordered
}

bool TROPUSRLoader::UnlockTrophy(u32 id, u64 timestamp1, u64 timestamp2)
{
	if (id >= m_table6.size())
	{
		trp_log.warning("TROPUSRLoader::UnlockTrophy: Invalid id=%d", id);
		return false;
	}

	m_table6[id].trophy_state = 1;
	m_table6[id].timestamp1 = timestamp1;
	m_table6[id].timestamp2 = timestamp2;

	return true;
}
