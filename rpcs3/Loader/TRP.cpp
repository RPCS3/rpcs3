#include "stdafx.h"
#include "Emu/VFS.h"
#include "TRP.h"
#include "Crypto/sha1.h"
#include "Utilities/StrUtil.h"

LOG_CHANNEL(trp_log, "Trophy");

TRPLoader::TRPLoader(const fs::file& f)
	: m_file(f)
{
}

bool TRPLoader::Install(std::string_view dest, bool /*show*/)
{
	if (!m_file)
	{
		fs::g_tls_error = fs::error::noent;
		return false;
	}

	fs::g_tls_error = {};

	const std::string local_path = vfs::get(dest);

	const std::string temp = fmt::format(u8"%s.＄temp＄%u", local_path, utils::get_unique_tsc());

	if (!fs::create_dir(temp))
	{
		trp_log.error("Failed to create temp dir: '%s' (error=%s)", temp, fs::g_tls_error);
		return false;
	}

	// Save TROPUSR.DAT
	if (!fs::copy_file(local_path + "/TROPUSR.DAT", temp + "/TROPUSR.DAT", false))
	{
		trp_log.error("Failed to copy TROPUSR.DAT from '%s' to '%s' (error=%s)", local_path, temp, fs::g_tls_error);
	}

	std::vector<char> buffer(65536);

	bool success = true;
	for (const TRPEntry& entry : m_entries)
	{
		m_file.seek(entry.offset);

		if (!m_file.read(buffer, entry.size))
		{
			trp_log.error("Failed to read TRPEntry at: offset=0x%x, size=0x%x", entry.offset, entry.size);
			continue; // ???
		}

		// Create the file in the temporary directory
		const std::string filename = temp + '/' + vfs::escape(entry.name);
		success = fs::write_file<true>(filename, fs::create + fs::excl, buffer);
		if (!success)
		{
			trp_log.error("Failed to write file '%s' (error=%s)", filename, fs::g_tls_error);
			break;
		}
	}

	if (success)
	{
		success = fs::remove_all(local_path, true, true);

		if (success)
		{
			// Atomically create trophy data (overwrite existing data)
			success = fs::rename(temp, local_path, false);
			if (!success)
			{
				trp_log.error("Failed to move directory '%s' to '%s' (error=%s)", temp, local_path, fs::g_tls_error);
			}
		}
		else
		{
			trp_log.error("Failed to remove directory '%s' (error=%s)", local_path, fs::g_tls_error);
		}
	}

	if (!success)
	{
		// Remove temporary directory manually on failure (removed automatically on success)
		auto old_error = fs::g_tls_error;
		fs::remove_all(temp);
		fs::g_tls_error = old_error;
	}

	return success;
}

bool TRPLoader::LoadHeader(bool show)
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

	if (m_header.trp_magic != 0xDCA24D00)
	{
		return false;
	}

	if (show)
	{
		trp_log.notice("TRP version: 0x%x", m_header.trp_version);
	}

	if (m_header.trp_version >= 2)
	{
		if (m_header.trp_file_size < sizeof(TRPHeader))
		{
			trp_log.error("Trophy file size too small (trp_file_size=%d, expected >= %d)", m_header.trp_file_size, sizeof(TRPHeader));
			return false;
		}

		std::vector<u8> file_contents;

		m_file.seek(0);
		if (!m_file.read(file_contents, m_header.trp_file_size))
		{
			trp_log.notice("Failed verifying checksum");
		}
		else
		{
			ensure(file_contents.size() >= sizeof(TRPHeader));
			ensure(file_contents.size() == m_header.trp_file_size);

			std::memset(&(reinterpret_cast<TRPHeader*>(file_contents.data()))->sha1, 0, 20);

			unsigned char hash[20];
			sha1(file_contents.data(), m_header.trp_file_size, hash);

			if (std::memcmp(hash, m_header.sha1, 20) != 0)
			{
				trp_log.error("Invalid checksum of TROPHY.TRP file");
				return false;
			}
		}

		m_file.seek(sizeof(m_header));
	}

	m_entries.clear();

	if (!m_file.read(m_entries, m_header.trp_files_count))
	{
		return false;
	}

	if (show)
	{
		for (const auto& entry : m_entries)
		{
			trp_log.notice("TRP entry #%u: %s", &entry - m_entries.data(), entry.name);
		}
	}

	return true;
}

u64 TRPLoader::GetRequiredSpace() const
{
	const u64 file_size = m_header.trp_file_size;
	const u64 file_element_size = u64{1} * m_header.trp_files_count * m_header.trp_element_size;

	return file_size - sizeof(m_header) - file_element_size;
}

bool TRPLoader::ContainsEntry(std::string_view filename)
{
	if (filename.size() >= sizeof(TRPEntry::name))
	{
		return false;
	}

	for (const TRPEntry& entry : m_entries)
	{
		if (entry.name == filename)
		{
			return true;
		}
	}
	return false;
}

void TRPLoader::RemoveEntry(std::string_view filename)
{
	if (filename.size() >= sizeof(TRPEntry::name))
	{
		return;
	}

	std::vector<TRPEntry>::iterator i = m_entries.begin();
	while (i != m_entries.end())
	{
		if (i->name == filename)
		{
			i = m_entries.erase(i);
		}
		else
		{
			i++;
		}
	}
}

void TRPLoader::RenameEntry(std::string_view oldname, std::string_view newname)
{
	if (oldname.size() >= sizeof(TRPEntry::name) || newname.size() >= sizeof(TRPEntry::name))
	{
		return;
	}

	for (TRPEntry& entry : m_entries)
	{
		if (entry.name == oldname)
		{
			strcpy_trunc(entry.name, newname);
		}
	}
}
