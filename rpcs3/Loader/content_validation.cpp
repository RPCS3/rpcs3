#include "stdafx.h"

#include "content_validation.h"

#include "Emu/system_utils.hpp"
#include "Utilities/File.h"
#include "Utilities/rXml.h"
#include "Crypto/md5.h"
#include "Crypto/utils.h"

LOG_CHANNEL(sys_log, "VALIDATION");

content_integrity_status content_validation::check_integrity(content_file_type file_type, const std::string& hash, std::string* game_name)
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
	default: // Let the following opening attempt fail and log the error message
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
	fs::file iso_file(path);

	// If no ISO file exists
	if (!iso_file)
	{
		sys_log.error("init_hash: Failed to open file: %s", path);
		m_status = content_hash_status::ABORTED;
		return false;
	}

	m_path = path;
	m_size = iso_file.size();
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

	fs::file iso_file(m_path);

	// If no ISO file exists
	if (!iso_file)
	{
		sys_log.error("calculate_hash: Failed to open file: %s", m_path);
		m_status = content_hash_status::ABORTED;
		return m_status;
	}

	constexpr u64 block_size = 4096;
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
	} while (bytes_read == block_size && m_status != content_hash_status::ABORTED);

	if (m_status == content_hash_status::ABORTED)
	{
		sys_log.warning("calculate_hash: MD5 hash calculation aborted by user: %s", m_path);
		return m_status;
	}

	if (mbedtls_md5_finish_ret(&md5_ctx, md5_hash) != 0)
	{
		sys_log.error("calculate_hash: Failed to calculate MD5 hash on file: %s", m_path);
		m_status = content_hash_status::ABORTED;
		return m_status;
	}

	// Convert the MD5 hash to hex string
	bytes_to_hex(hash, md5_hash, 16);

	m_status = content_hash_status::COMPLETED;
	return m_status;
}
