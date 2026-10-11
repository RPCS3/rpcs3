#include "stdafx.h"

#include "content_validation.h"
#include "ISO.h"

#include "Emu/system_utils.hpp"
#include "Utilities/File.h"
#include "Utilities/rXml.h"
#include "Crypto/md5.h"
#include "Crypto/utils.h"

LOG_CHANNEL(sys_log, "VALIDATION");

content_integrity_status content_validation::check_integrity(content_file_type file_type, std::string_view hash, std::string* game_name)
{
	//
	// Check for Redump db
	//

	std::string db_path;

	switch (file_type)
	{
	case content_file_type::ISO:
		db_path = rpcs3::utils::get_redump_db_path();
		break;
	case content_file_type::PSN_CONTENT:
		db_path = rpcs3::utils::get_psn_content_db_path();
		break;
	case content_file_type::PSN_DLC:
		db_path = rpcs3::utils::get_psn_dlc_db_path();
		break;
	case content_file_type::PSN_UPDATE:
		db_path = rpcs3::utils::get_psn_update_db_path();
		break;
	}

	fs::file db_file(db_path);

	// If no db file exists
	if (!db_file)
	{
		// An empty hash is used to simply test the presence (without any logging) of the Redump db
		if (!hash.empty())
		{
			sys_log.error("check_integrity: Failed to open file: %s", db_path);
		}

		return content_integrity_status::ERROR_OPENING_DB;
	}

	if (hash.empty())
	{
		return content_integrity_status::NO_MATCH;
	}

	rXmlDocument db;

	if (!db.Read(db_file.to_string()))
	{
		sys_log.error("check_integrity: Failed to process file: %s", db_path);
		return content_integrity_status::ERROR_PARSING_DB;
	}

	// Close the file and work with the data loaded into the "db" document
	db_file.close();

	std::shared_ptr<rXmlNode> db_base = db.GetRoot();

	if (!db_base)
	{
		sys_log.error("check_integrity: Failed to get 'root' node on file: %s", db_path);
		return content_integrity_status::ERROR_PARSING_DB;
	}

	if (db_base = db_base->GetChild(std::string_view("datafile")); !db_base)
	{
		sys_log.error("check_integrity: Failed to get 'datafile' node on file: %s", db_path);
		return content_integrity_status::ERROR_PARSING_DB;
	}

	//
	// Check for a match on Redump db
	//

	for (auto node = db_base->GetChildren(); node; node = node->GetNext())
	{
		if (node->GetName() == "game")
		{
			for (auto child = node->GetChildren(); child; child = child->GetNext())
			{
				// If a match is found, fill in "game_desc" (if requested) and return FOUND_MATCH
				if (child->GetName() == "rom" && hash == child->GetAttribute(std::string_view("md5")))
				{
					if (game_name)
					{
						*game_name = node->GetAttribute(std::string_view("name"));
					}

					return content_integrity_status::FOUND_MATCH;
				}
			}
		}
	}

	// No match found
	return content_integrity_status::NO_MATCH;
}

bool content_validation::init_hash(const std::string& path)
{
	std::string new_path = path;

	fs::get_optical_raw_device(path, &new_path);

	iso_file file(new_path);

	// If no file exists
	if (!file)
	{
		sys_log.error("init_hash: Failed to open file: %s", new_path);
		m_status = content_hash_status::ABORTED;
		return false;
	}

	m_path = new_path;
	m_name = new_path.find_last_of(fs::delim) != umax ? new_path.substr(new_path.find_last_of(fs::delim) + 1) : new_path;
	m_size = file.size();
	m_bytes_read = 0;
	m_status = content_hash_status::INITIALIZED;
	return true;
}

content_hash_status content_validation::calculate_hash(std::string& hash)
{
	if (m_status != content_hash_status::INITIALIZED)
	{
		sys_log.error("calculate_hash: MD5 hash calculation already performed: %s", m_path);
		m_status = content_hash_status::ABORTED;
		return m_status;
	}

	auto file = std::make_unique<iso_file>(m_path);

	// If no file exists.
	// NOTE: it has to be asked here, while the ISO file is still at hand: wrapped into a "fs::file" it would
	//       answer as a valid one and hand out the hash of nothing at all
	if (!*file)
	{
		sys_log.error("calculate_hash: Failed to open file: %s", m_path);
		m_status = content_hash_status::ABORTED;
		return m_status;
	}

	// The very hashing a content check goes through, over the whole file and with no tail to pad
	std::vector<u8> buffer;

	if (!hash_file(fs::file(std::move(file)), 0, buffer, hash))
	{
		if (m_status == content_hash_status::ABORTED)
		{
			sys_log.warning("calculate_hash: MD5 hash calculation aborted by user: %s", m_path);
			return m_status;
		}

		m_status = content_hash_status::ABORTED;
		return m_status;
	}

	m_status = content_hash_status::COMPLETED;
	return m_status;
}

// Read block of the hashing below. A megabyte a time is what makes reading a whole disc worth it: on an
// encrypted image every read is split into sectors and decrypted, so the fewer of them the better
static constexpr u64 s_hash_block_size = 0x100000;

bool content_validation::hash_file(const fs::file& file, u64 padded_size, std::vector<u8>& buffer, std::string& hash)
{
	if (!file)
	{
		sys_log.error("hash_file: Failed to open file: %s", m_name);
		return false;
	}

	// Never bigger than what this file has to give, and never shrunk: a check over many files pays for the
	// biggest of them once, while one over a handful of small ones does not pay for a megabyte it cannot use
	if (const u64 wanted = std::min<u64>(s_hash_block_size, std::max<u64>({file.size(), padded_size, 1}));
		buffer.size() < wanted)
	{
		buffer.resize(wanted);
	}

	const u64 block_size = buffer.size();
	u8* const buf = buffer.data();
	mbedtls_md5_context md5_ctx;
	unsigned char md5_hash[16];
	u64 size = 0;

	mbedtls_md5_starts_ret(&md5_ctx);

	// Only a read returning nothing at all ends the file: a short one is not to be trusted as the end of it,
	// since a file held by an ISO image is read back extent by extent
	while (m_status != content_hash_status::ABORTED)
	{
		const u64 read = file.read(buf, block_size);

		if (!read)
		{
			break;
		}

		mbedtls_md5_update_ret(&md5_ctx, buf, read);

		size += read;
		m_bytes_read += read;
	}

	// Content that stopped coming before its end leaves the hash of a part of it, which passes for the hash of
	// something else entirely: an unreadable sector of a disc has to fail the check, not answer it wrongly
	if (m_status != content_hash_status::ABORTED && size != file.size())
	{
		sys_log.error("hash_file: Failed to read the whole content: %s (%d/%d)", m_name, size, file.size());
		return false;
	}

	// A file shorter than the one on the disc can only match once its missing tail is hashed as zeros: that is how
	// a "PS3UPDAT.PUP" downloaded apart is turned back into the 256 MB one the disc holds
	if (padded_size > size)
	{
		std::memset(buf, 0, block_size);

		for (u64 left = padded_size - size; left && m_status != content_hash_status::ABORTED;)
		{
			const u64 chunk = std::min<u64>(left, block_size);

			mbedtls_md5_update_ret(&md5_ctx, buf, chunk);

			left -= chunk;
			m_bytes_read += chunk;
		}
	}

	if (m_status == content_hash_status::ABORTED)
	{
		return false;
	}

	if (mbedtls_md5_finish_ret(&md5_ctx, md5_hash) != 0)
	{
		sys_log.error("hash_file: Failed to calculate MD5 hash on file: %s", m_name);
		return false;
	}

	bytes_to_hex(hash, md5_hash, 16);
	return true;
}
