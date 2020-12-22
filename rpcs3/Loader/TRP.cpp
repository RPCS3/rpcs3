#include "stdafx.h"
#include "Emu/VFS.h"
#include "TRP.h"
#include "Crypto/sha1.h"
#include "Utilities/StrUtil.h"

LOG_CHANNEL(trp_log, "Trophy");

TRPLoader::TRPLoader(const fs::file& f)
	: trp_f(f)
{
}

bool TRPLoader::Install(const std::string& dest, bool show)
{
	if (!trp_f)
	{
		fs::g_tls_error = fs::error::noent;
		return false;
	}

	fs::g_tls_error = {};

	const std::string& local_path = vfs::get(dest);

	const auto temp = fmt::format(u8"%s.＄temp＄%u", local_path, utils::get_unique_tsc());

	if (!fs::create_dir(temp))
	{
		return false;
	}

	// Save TROPUSR.DAT
	fs::copy_file(local_path + "/TROPUSR.DAT", temp + "/TROPUSR.DAT", false);

	std::vector<char> buffer(65536);

	bool success = true;
	for (const TRPEntry& entry : m_entries)
	{
		trp_f.seek(entry.offset);
		buffer.resize(entry.size);
		if (!trp_f.read(buffer))
		{
			trp_log.error("Failed to read TRPEntry at: offset=0x%x, size=0x%x", entry.offset, entry.size);
			continue; // ???
		}

		// Create the file in the temporary directory
		success = fs::write_file(temp + '/' + vfs::escape(entry.name), fs::create + fs::excl, buffer);
		if (!success)
		{
			break;
		}
	}

	if (success)
	{
		success = fs::remove_all(local_path) || !fs::is_dir(local_path);

		if (success)
		{
			// Atomically create trophy data (overwrite existing data)
			success = fs::rename(temp, local_path, false);
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
		trp_log.notice("TRP version: 0x%x", m_header.trp_version);
	}

	if (m_header.trp_version >= 2)
	{
		unsigned char hash[20];
		std::vector<unsigned char> file_contents(m_header.trp_file_size);

		trp_f.seek(0);
		if (!trp_f.read(file_contents))
		{
			trp_log.notice("Failed verifying checksum");
		}
		else
		{
			memset(&(reinterpret_cast<TRPHeader*>(file_contents.data()))->sha1, 0, 20);
			sha1(reinterpret_cast<const unsigned char*>(file_contents.data()), m_header.trp_file_size, hash);

			if (memcmp(hash, m_header.sha1, 20) != 0)
			{
				trp_log.error("Invalid checksum of TROPHY.TRP file");
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
			trp_log.notice("TRP entry #%d: %s", m_entries[i].name);
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
	for (TRPEntry& entry : m_entries)
	{
		if (!strcmp(entry.name, oldname))
		{
			strcpy_trunc(entry.name, std::string_view(newname));
		}
	}
}
