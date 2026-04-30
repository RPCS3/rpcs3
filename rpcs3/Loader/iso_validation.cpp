#include "stdafx.h"

#include "iso_validation.h"
#include "ISO.h"

#include "Emu/system_utils.hpp"
#include "Utilities/File.h"
#include "Utilities/rXml.h"
#include "Crypto/md5.h"
#include "Crypto/utils.h"

LOG_CHANNEL(iso_log, "ISO");

iso_integrity_status iso_file_validation::check_integrity(const std::string& hash, std::string* game_name)
{
	//
	// Check for Redump db
	//

	const std::string db_path = rpcs3::utils::get_redump_db_path();
	fs::file db_file(db_path);

	// If no db file exists
	if (!db_file)
	{
		// An empty hash is used to simply test the presence (without any logging) of the Redump db
		if (!hash.empty())
		{
			iso_log.error("check_integrity: Failed to open file: %s", db_path);
		}

		return iso_integrity_status::ERROR_OPENING_DB;
	}

	if (hash.empty())
	{
		return iso_integrity_status::NO_MATCH;
	}

	rXmlDocument db;

	if (!db.Read(db_file.to_string()))
	{
		iso_log.error("check_integrity: Failed to process file: %s", db_path);
		return iso_integrity_status::ERROR_PARSING_DB;
	}

	std::shared_ptr<rXmlNode> db_base = db.GetRoot();

	if (!db_base)
	{
		iso_log.error("check_integrity: Failed to get 'root' node on file: %s", db_path);
		return iso_integrity_status::ERROR_PARSING_DB;
	}

	if (db_base = db_base->GetChild(std::string_view("datafile")); !db_base)
	{
		iso_log.error("check_integrity: Failed to get 'datafile' node on file: %s", db_path);
		return iso_integrity_status::ERROR_PARSING_DB;
	}

	//
	// Check for a match on Redump db
	//

	for (auto node = db_base->GetChildren(); node; node = node->GetNext())
	{
		if (node->GetName() == "game")
		{
			if (const auto child = node->GetChild(std::string_view("rom")))
			{
				// If a match is found, fill in "game_desc" (if requested) and return FOUND_MATCH
				if (hash == child->GetAttribute(std::string_view("md5")))
				{
					if (game_name)
					{
						*game_name = node->GetAttribute(std::string_view("name"));
					}

					return iso_integrity_status::FOUND_MATCH;
				}
			}
		}
	}

	// No match found
	return iso_integrity_status::NO_MATCH;
}

bool iso_file_validation::init_hash(const std::string& path)
{
	fs::file iso_file(path);

	// If no ISO file exists
	if (!iso_file)
	{
		iso_log.error("init_hash: Failed to open file: %s", path);
		m_status = iso_hash_status::ABORTED;
		return false;
	}

	m_path = path;
	m_size = iso_file.size();
	m_bytes_read = 0;
	m_status = iso_hash_status::INITIALIZED;
	return true;
}

iso_hash_status iso_file_validation::calculate_hash(std::string& hash)
{
	if (m_status != iso_hash_status::INITIALIZED)
	{
		iso_log.error("calculate_hash: MD5 hash calculation already performed: %s", m_path);
		m_status = iso_hash_status::ABORTED;
		return m_status;
	}

	fs::file iso_file(m_path);

	// If no ISO file exists
	if (!iso_file)
	{
		iso_log.error("calculate_hash: Failed to open file: %s", m_path);
		m_status = iso_hash_status::ABORTED;
		return m_status;
	}

	constexpr u64 block_size = ISO_SECTOR_SIZE * 2;
	std::array<u8, block_size> buf;
	u64 bytes_read;
	mbedtls_md5_context md5_ctx;
	unsigned char md5_hash[16];

	mbedtls_md5_starts_ret(&md5_ctx);

	do
	{
		bytes_read = iso_file.read(buf.data(), block_size);
		mbedtls_md5_update_ret(&md5_ctx, buf.data(), bytes_read);

		m_bytes_read += bytes_read;
	} while (bytes_read == block_size && m_status != iso_hash_status::ABORTED);

	if (m_status == iso_hash_status::ABORTED)
	{
		iso_log.warning("calculate_hash: MD5 hash calculation aborted by user: %s", m_path);
		return m_status;
	}

	if (mbedtls_md5_finish_ret(&md5_ctx, md5_hash) != 0)
	{
		iso_log.error("calculate_hash: Failed to calculate MD5 hash on file: %s", m_path);
		m_status = iso_hash_status::ABORTED;
		return m_status;
	}

	// Convert the MD5 hash to hex string
	bytes_to_hex(hash, md5_hash, 16);

	m_status = iso_hash_status::COMPLETED;
	return m_status;
}
