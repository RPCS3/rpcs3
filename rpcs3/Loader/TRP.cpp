#include "stdafx.h"
#include "Emu/System.h"
#include "TRP.h"
#include "Crypto/sha1.h"
#include "Utilities/StrUtil.h"

TRPLoader::TRPLoader(const fs::file& f)
	: trp_f(f)
	, m_header{ 0 }
{
}

bool TRPLoader::Install(const std::string& dest, bool show)
{
	if (!trp_f)
	{
		return false;
	}

	const std::string& local_path = vfs::get(GetBaseTrophyPath() + dest);

	if (!fs::create_dir(local_path) && fs::g_tls_error != fs::error::exist)
	{
		return false;
	}

	std::vector<char> buffer(65536);

	for (const TRPEntry& entry : m_entries)
	{
		trp_f.seek(entry.offset);
		buffer.resize(entry.size);
		if (!trp_f.read(buffer)) continue; // ???
		fs::file(local_path + '/' + entry.name, fs::rewrite).write(buffer);
	}

	return true;
}

bool TRPLoader::IsInstalled(const std::string& dest)
{
	// Just check if the directory exists
	// TODO: Ideally we'd check the files/entries too
	const std::string& local_path = vfs::get(GetBaseTrophyPath() + dest);
	return fs::is_dir(local_path);
}

bool TRPLoader::LoadHeader(bool show)
{
	if (!trp_f)
	{
		return false;
	}

	trp_f.seek(0);

	if (!trp_f.read(m_header))
	{
		return false;
	}

	if (m_header.trp_magic != 0xDCA24D00)
	{
		return false;
	}

	if (show)
	{
		LOG_NOTICE(LOADER, "TRP version: 0x%x", m_header.trp_version);
	}

	if (m_header.trp_version >= 2)
	{
		unsigned char hash[20];
		std::vector<unsigned char> file_contents(m_header.trp_file_size);

		trp_f.seek(0);
		if (!trp_f.read(file_contents))
		{
			LOG_NOTICE(LOADER, "Failed verifying checksum");
		}
		else
		{
			memset(&(reinterpret_cast<TRPHeader*>(file_contents.data()))->sha1, 0, 20);
			sha1(reinterpret_cast<const unsigned char*>(file_contents.data()), m_header.trp_file_size, hash);

			if (memcmp(hash, m_header.sha1, 20) != 0)
			{
				LOG_ERROR(LOADER, "Invalid checksum of TROPHY.TRP file");
				return false;
			}
		}

		trp_f.seek(sizeof(m_header));
	}

	m_entries.clear();
	m_entries.resize(m_header.trp_files_count);

	for (u32 i = 0; i < m_header.trp_files_count; i++)
	{
		if (!trp_f.read(m_entries[i]))
		{
			return false;
		}

		if (show)
		{
			LOG_NOTICE(LOADER, "TRP entry #%d: %s", m_entries[i].name);
		}
	}

	return true;
}

bool TRPLoader::ContainsEntry(const char *filename)
{
	for (const TRPEntry& entry : m_entries)
	{
		if (!strcmp(entry.name, filename))
		{
			return true;
		}
	}
	return false;
}

void TRPLoader::RemoveEntry(const char *filename)
{
	std::vector<TRPEntry>::iterator i = m_entries.begin();
	while (i != m_entries.end())
	{
		if (!strcmp(i->name, filename))
		{
			i = m_entries.erase(i);
		}
		else
		{
			i++;
		}
	}
}

void TRPLoader::RenameEntry(const char *oldname, const char *newname)
{
	for (const TRPEntry& entry : m_entries)
	{
		if (!strcmp(entry.name, oldname))
		{
			memcpy((void*)entry.name, newname, 32);
		}
	}
}

bool TRPLoader::TrimEntries()
{
	// Rename or discard certain entries based on the files found
	const size_t kTargetBufferLength = 31;
	char target[kTargetBufferLength + 1];
	target[kTargetBufferLength] = 0;
	strcpy_trunc(target, fmt::format("TROP_%02d.SFM", static_cast<s32>(g_cfg.sys.language)));

	if (ContainsEntry(target))
	{
		RemoveEntry("TROPCONF.SFM");
		RemoveEntry("TROP.SFM");
		RenameEntry(target, "TROPCONF.SFM");
	}
	else if (ContainsEntry("TROP.SFM"))
	{
		RemoveEntry("TROPCONF.SFM");
		RenameEntry("TROP.SFM", "TROPCONF.SFM");
	}
	else if (!ContainsEntry("TROPCONF.SFM"))
		return false;

	// Discard unnecessary TROP_XX.SFM files
	for (s32 i = 0; i <= 18; i++)
	{
		strcpy_trunc(target, fmt::format("TROP_%02d.SFM", i));
		if (i != g_cfg.sys.language)
		{
			RemoveEntry(target);
		}
	}
	return true;
}

u64 TRPLoader::GetFileSize()
{
	if (!m_header.trp_file_size)
	{
		LoadHeader();
	}

	const auto file_size = m_header.trp_file_size;
	const auto header_size = sizeof(m_header);
	const auto file_element_size = m_header.trp_files_count * m_header.trp_element_size;

	return file_size - header_size - file_element_size;
}

const std::string& TRPLoader::GetBaseTrophyPath()
{
	// TODO: Get the path of the current user
	static const std::string& base_trophy_path = "/dev_hdd0/home/00000001/trophy/";
	return base_trophy_path;
}
