#include "stdafx.h"

#include "ISO.h"
#include "Emu/VFS.h"
#include "EMU/system_utils.hpp"
#include "Crypto/utils.h"
#include "Crypto/md5.h"
#include "Utilities/rXml.h"

#include <codecvt>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <stack>

LOG_CHANNEL(sys_log, "SYS");

constexpr u64 ISO_SECTOR_SIZE = 2048;

struct iso_sector
{
	u64 lba_address;
	u64 offset;
	u64 size;
	u64 address_aligned;
	u64 offset_aligned;
	u64 size_aligned;
	std::array<u8, ISO_SECTOR_SIZE> buf;
};

bool is_file_iso(const std::string& path)
{
	if (path.empty() || fs::is_dir(path))
	{
		return false;
	}

	return is_file_iso(fs::file(path));
}

bool is_file_iso(const fs::file& file)
{
	if (!file || file.size() < 32768 + 6)
	{
		return false;
	}

	file.seek(32768);

	char magic[5];
	file.read_at(32768 + 1, magic, 5);

	return magic[0] == 'C' && magic[1] == 'D' && magic[2] == '0' && magic[3] == '0' && magic[4] == '1';
}

// Convert 4 bytes in big-endian format to an unsigned integer
static u32 char_arr_BE_to_uint(const u8* arr)
{
	return arr[0] << 24 | arr[1] << 16 | arr[2] << 8 | arr[3];
}

// Reset the iv to a particular LBA
static void reset_iv(std::array<u8, 16>& iv, u32 lba)
{
	memset(iv.data(), 0, 12);

	iv[12] = (lba & 0xFF000000) >> 24;
	iv[13] = (lba & 0x00FF0000) >> 16;
	iv[14] = (lba & 0x0000FF00) >> 8;
	iv[15] = (lba & 0x000000FF) >> 0;
}

// Main function that will decrypt the sector(s)
static bool decrypt_data(aes_context& aes, u64 offset, unsigned char* buffer, u64 size)
{
	// The following preliminary checks are good to be provided.
	// Commented out to gain a bit of performance, just because we know the caller is providing values in the expected range

	//if (size == 0)
	//{
	//	return false;
	//}

	//if ((size % 16) != 0)
	//{
	//	sys_log.error("decrypt_data(): Requested ciphertext blocks' size must be a multiple of 16 (%ull)", size);
	//	return;
	//}

	u32 cur_sector_lba = static_cast<u32>(offset / ISO_SECTOR_SIZE); // First sector's LBA
	const u32 sector_count = static_cast<u32>((offset + size - 1) / ISO_SECTOR_SIZE) - cur_sector_lba + 1;
	const u64 sector_offset = offset % ISO_SECTOR_SIZE;

	std::array<u8, 16> iv;
	u64 cur_offset;
	u64 cur_size;

	// If the offset is not at the beginning of a sector, the first 16 bytes in the buffer
	// represents the IV for decrypting the next data in the buffer.
	// Otherwise, the IV is based on sector's LBA
	if (sector_offset != 0)
	{
		memcpy(iv.data(), buffer, 16);
		cur_offset = 16;
	}
	else
	{
		reset_iv(iv, cur_sector_lba);
		cur_offset = 0;
	}

	cur_size = sector_offset + size <= ISO_SECTOR_SIZE ? size : ISO_SECTOR_SIZE - sector_offset;
	cur_size -= cur_offset;

	// Partial (or even full) first sector
	if (aes_crypt_cbc(&aes, AES_DECRYPT, cur_size, iv.data(), &buffer[cur_offset], &buffer[cur_offset]) != 0)
	{
		sys_log.error("decrypt_data(): Error decrypting data on first sector read");
		return false;
	}

	if (sector_count < 2) // If no more sector(s)
	{
		return true;
	}

	cur_offset += cur_size;

	const u32 inner_sector_count = sector_count > 2 ? sector_count - 2 : 0; // Remove first and last sector

	// Inner sector(s), if any
	for (u32 i = 0; i < inner_sector_count; i++)
	{
		reset_iv(iv, ++cur_sector_lba); // Next sector's IV

		if (aes_crypt_cbc(&aes, AES_DECRYPT, ISO_SECTOR_SIZE, iv.data(), &buffer[cur_offset], &buffer[cur_offset]) != 0)
		{
			sys_log.error("decrypt_data(): Error decrypting data on inner sector(s) read");
			return false;
		}

		cur_offset += ISO_SECTOR_SIZE;
	}

	reset_iv(iv, ++cur_sector_lba); // Next sector's IV

	// Partial (or even full) last sector
	if (aes_crypt_cbc(&aes, AES_DECRYPT, size - cur_offset, iv.data(), &buffer[cur_offset], &buffer[cur_offset]) != 0)
	{
		sys_log.error("decrypt_data(): Error decrypting data on last sector read");
		return false;
	}

	return true;
}

void iso_file_decryption::reset()
{
	m_enc_type = iso_encryption_type::NONE;
	m_region_info.clear();
}

iso_type_status iso_file_decryption::check_type(const std::string& path, std::string& key_path, aes_context* aes_ctx)
{
	if (!is_file_iso(path))
	{
		return iso_type_status::NOT_ISO;
	}

	// Remove file extension from file path
	const usz ext_pos = path.rfind('.');
	std::string name_path = ext_pos == umax ? path : path.substr(0, ext_pos);

	// Detect file name (with no parent folder and no file extension)
	const usz name_pos = name_path.rfind('/');
	std::string name = name_pos == umax ? name_path : name_path.substr(name_pos);

	// Open ".dkey" file
	key_path = name_path + ".dkey";

	fs::file key_file(key_path);

	// If no ".dkey" file exists, try with ".key"
	if (!key_file)
	{
		key_path = name_path + ".key";
		key_file = fs::file(key_path);
	}

	// If no ".dkey" and ".key" file exists, try on default ISO keys folder
	if (!key_file)
	{
		key_path = rpcs3::utils::get_redump_dir() + name + ".dkey";
		key_file = fs::file(key_path);
	}

	// If no ".dkey" file exists, try with ".key"
	if (!key_file)
	{
		key_path = rpcs3::utils::get_redump_dir() + name + ".key";
		key_file = fs::file(key_path);
	}

	// If no ".dkey" and ".key" file exists
	if (!key_file)
	{
		return iso_type_status::ERROR_OPENING_KEY;
	}

	char key_str[32];
	std::array<u8, 16> key {};

	const u64 key_len = key_file.read(key_str, sizeof(key_str));

	if (key_len == sizeof(key_str) || key_len == sizeof(key))
	{
		// If the key read from the key file is 16 bytes long instead of 32, consider the file as
		// binary (".key") and so not needing any further conversion from hex string to bytes
		if (key_len == sizeof(key))
		{
			memcpy(key.data(), key_str, sizeof(key));
		}
		else
		{
			hex_to_bytes(key.data(), std::string_view(key_str, key_len), static_cast<unsigned int>(key_len));
		}

		aes_context aes_dec;

		if (aes_ctx == nullptr)
		{
			aes_ctx = &aes_dec;
		}

		// Create the decryption context. If the context is successfully created, fill in "aes_ctx"
		// (if not passed as nullptr) and return Redump ISO status
		if (aes_setkey_dec(aes_ctx, key.data(), 128) == 0)
		{
			return iso_type_status::REDUMP_ISO;
		}
	}

	return iso_type_status::ERROR_PROCESSING_KEY;
}

bool iso_file_decryption::calculate_md5_hash(const std::string& path, std::string& hash)
{
	fs::file iso_file(path);

	// If no ISO file exists
	if (!iso_file)
	{
		sys_log.error("calculate_md5_hash(): Failed to open file: %s", path);
		return false;
	}

	constexpr u64 block_size = ISO_SECTOR_SIZE;
	std::array<u8, block_size> buf;
	u64 bytes_read;
	mbedtls_md5_context md5_ctx;
	unsigned char md5_hash[16];

	mbedtls_md5_starts_ret(&md5_ctx);

	do
	{
		bytes_read = iso_file.read(buf.data(), block_size);
		mbedtls_md5_update_ret(&md5_ctx, buf.data(), bytes_read);
	} while (bytes_read == block_size);

	if (mbedtls_md5_finish_ret(&md5_ctx, md5_hash) != 0)
	{
		sys_log.error("calculate_md5_hash(): Failed to calculate MD5 hash on file: %s", path);
		return false;
	}

	// Convert the MD5 hash to hex string
	bytes_to_hex(hash, md5_hash, 16);
	return true;
}

iso_integrity_status iso_file_decryption::check_integrity(const std::string& path, const std::string& hash, std::string* game_name)
{
	//
	// Check for Redump db
	//

	const usz ext_pos = path.rfind('.');
	std::string db_path;

	// If no file extension is provided, set "db_path" appending ".dat" to "path".
	// Otherwise, replace the extension (e.g. ".iso") with ".dat"
	db_path = ext_pos == umax ? path + ".dat" : path.substr(0, ext_pos) + ".dat";

	fs::file db_file(db_path);

	// If no ".dat" file exists, try with default "redump.dat" file
	if (!db_file)
	{
		db_path = rpcs3::utils::get_redump_db_path();
		db_file = fs::file(db_path);
	}

	// If no db file exists
	if (!db_file)
	{
		// An empty hash is used to simply test the presence (without any logging) of the Redump db
		if (hash != "")
		{
			sys_log.error("check_integrity(): Failed to open file: %s", db_path);
		}

		return iso_integrity_status::ERROR_OPENING_DB;
	}

	if (hash == "")
	{
		return iso_integrity_status::NO_MATCH;
	}

	rXmlDocument db;

	if (!db.Read(db_file.to_string()))
	{
		sys_log.error("check_integrity(): Failed to process file: %s", db_path);
		return iso_integrity_status::ERROR_PARSING_DB;
	}

	std::shared_ptr<rXmlNode> db_base = db.GetRoot();

	if (!db_base)
	{
		sys_log.error("check_integrity(): Failed to get 'root' node on file: %s", db_path);
		return iso_integrity_status::ERROR_PARSING_DB;
	}

	if (const auto& node = db_base->GetChildren(); node && node->GetName() == "datafile")
	{
		db_base = node;
	}
	else
	{
		sys_log.error("check_integrity(): Failed to get 'datafile' node on file: %s", db_path);
		return iso_integrity_status::ERROR_PARSING_DB;
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
				if (child->GetName() == "rom")
				{
					// If a match is found, Fill in "game_desc" and exit
					if (hash == child->GetAttribute(std::string_view("md5")))
					{
						if (game_name != nullptr)
						{
							*game_name = node->GetAttribute(std::string_view("name"));
						}

						return iso_integrity_status::FOUND_MATCH;
					}
				}
			}
		}
	}

	// No match found
	return iso_integrity_status::NO_MATCH;
}

bool iso_file_decryption::init(const std::string& path)
{
	reset();

	if (!is_file_iso(path))
	{
		return false;
	}

	std::array<u8, ISO_SECTOR_SIZE * 2> sec0_sec1 {};

	//
	// Store the ISO region information (needed by both the "Redump" type (only on "decrypt()" method) and "3k3y" type)
	//

	fs::file iso_file(path);

	if (!iso_file)
	{
		sys_log.error("init(): Failed to open file: %s", path);
		return false;
	}

	if (iso_file.size() < sec0_sec1.size())
	{
		sys_log.error("init(): Found only %ull sector(s) (minimum required is 2): %s", iso_file.size(), path);
		return false;
	}

	if (iso_file.read(sec0_sec1.data(), sec0_sec1.size()) != sec0_sec1.size())
	{
		sys_log.error("init(): Failed to read file: %s", path);
		return false;
	}

	// NOTE:
	//
	// Following checks and assigned values are based on PS3 ISO specification.
	// E.g. all even regions (0, 2, 4 etc.) are always unencrypted while the odd ones are encrypted

	size_t region_count = char_arr_BE_to_uint(sec0_sec1.data());

	// Ensure the region count is a proper value
	if (region_count < 1 || region_count > 31) // It's non-PS3ISO
	{
		sys_log.error("init(): Failed to read region information: %s", path);
		return false;
	}

	m_region_info.resize(region_count * 2 - 1);

	for (size_t i = 0; i < m_region_info.size(); ++i)
	{
		// Store the region information in address format
		m_region_info[i].encrypted = (i % 2 == 1);
		m_region_info[i].region_first_addr = (i == 0 ? 0ULL : m_region_info[i - 1].region_last_addr + 1ULL);
		m_region_info[i].region_last_addr = (static_cast<u64>(char_arr_BE_to_uint(sec0_sec1.data() + 12 + (i * 4)))
			- (i % 2 == 1 ? 1ULL : 0ULL)) * ISO_SECTOR_SIZE + ISO_SECTOR_SIZE - 1ULL;
	}

	//
	// Check for Redump type
	//

	std::string key_path;

	// Try to detect the Redump type. If so, the decryption context is set into "m_aes_dec"
	switch (check_type(path, key_path, &m_aes_dec))
	{
	case iso_type_status::NOT_ISO:
		sys_log.warning("init(): Failed to recognize ISO file: %s", path);
		break;
	case iso_type_status::REDUMP_ISO:
		m_enc_type = iso_encryption_type::REDUMP; // SET ENCRYPTION TYPE: REDUMP
		break;
	case iso_type_status::ERROR_OPENING_KEY:
		sys_log.warning("init(): Failed to open, or missing, key file: %s", key_path);
		break;
	case iso_type_status::ERROR_PROCESSING_KEY:
		sys_log.error("init(): Failed to process key file: %s", key_path);
		break;
	default:
		break;
	}

	//
	// Check for 3k3y type
	//

	// If encryption type is still set to NONE
	if (m_enc_type == iso_encryption_type::NONE)
	{
		// The 3k3y watermarks located at offset 0xF70: (D|E)ncrypted 3K BLD
		static const unsigned char k3k3y_enc_watermark[16] =
			{0x45, 0x6E, 0x63, 0x72, 0x79, 0x70, 0x74, 0x65, 0x64, 0x20, 0x33, 0x4B, 0x20, 0x42, 0x4C, 0x44};
		static const unsigned char k3k3y_dec_watermark[16] =
			{0x44, 0x6E, 0x63, 0x72, 0x79, 0x70, 0x74, 0x65, 0x64, 0x20, 0x33, 0x4B, 0x20, 0x42, 0x4C, 0x44};

		if (memcmp(&k3k3y_enc_watermark[0], &sec0_sec1[0xF70], sizeof(k3k3y_enc_watermark)) == 0)
		{
			// Grab D1 from the 3k3y sector
			unsigned char key[16];

			memcpy(key, &sec0_sec1[0xF80], 0x10);

			// Convert D1 to KEY and generate the "m_aes_dec" context
			unsigned char key_d1[] = {0x38, 11, 0xcf, 11, 0x53, 0x45, 0x5b, 60, 120, 0x17, 0xab, 0x4f, 0xa3, 0xba, 0x90, 0xed};
			unsigned char iv_d1[] = {0x69, 0x47, 0x47, 0x72, 0xaf, 0x6f, 0xda, 0xb3, 0x42, 0x74, 0x3a, 0xef, 170, 0x18, 0x62, 0x87};

			aes_context aes_d1;

			if (aes_setkey_enc(&aes_d1, key_d1, 128) == 0)
			{
				if (aes_crypt_cbc(&aes_d1, AES_ENCRYPT, 16, &iv_d1[0], key, key) == 0)
				{
					if (aes_setkey_dec(&m_aes_dec, key, 128) == 0)
					{
						m_enc_type = iso_encryption_type::ENC_3K3Y; // SET ENCRYPTION TYPE: ENC_3K3Y
					}
				}
			}

			if (m_enc_type == iso_encryption_type::NONE) // If encryption type was not set to ENC_3K3Y for any reason
			{
				sys_log.error("init(): Failed to set encryption type to ENC_3K3Y: %s", path);
			}
		}
		else if (memcmp(&k3k3y_dec_watermark[0], &sec0_sec1[0xF70], sizeof(k3k3y_dec_watermark)) == 0)
		{
			m_enc_type = iso_encryption_type::DEC_3K3Y; // SET ENCRYPTION TYPE: DEC_3K3Y
		}
	}

	switch (m_enc_type)
	{
	case iso_encryption_type::REDUMP:
		sys_log.warning("init(): Set 'enc type': REDUMP, 'reg count': %u: %s", m_region_info.size(), path);
		break;
	case iso_encryption_type::ENC_3K3Y:
		sys_log.warning("init(): Set 'enc type': ENC_3K3Y, 'reg count': %u: %s", m_region_info.size(), path);
		break;
	case iso_encryption_type::DEC_3K3Y:
		sys_log.warning("init(): Set 'enc type': DEC_3K3Y, 'reg count': %u: %s", m_region_info.size(), path);
		break;
	case iso_encryption_type::NONE: // If encryption type was not set for any reason
		sys_log.warning("init(): Set 'enc type': NONE, 'reg count': %u: %s", m_region_info.size(), path);
		break;
	}

	return true;
}

bool iso_file_decryption::decrypt(u64 offset, void* buffer, u64 size, const std::string& name)
{
	// If it's a non-encrypted type, nothing more to do
	if (m_enc_type == iso_encryption_type::NONE)
	{
		return true;
	}

	// If it's a 3k3y ISO and data at offset 0xF70 is being requested, we should null it out
	if (m_enc_type == iso_encryption_type::DEC_3K3Y || m_enc_type == iso_encryption_type::ENC_3K3Y)
	{
		if (offset + size >= 0xF70ULL && offset <= 0x1070ULL)
		{
			// Zero out the 0xF70 - 0x1070 overlap
			unsigned char* buf = reinterpret_cast<unsigned char*>(buffer);
			unsigned char* buf_overlap_start = offset < 0xF70ULL ? buf + 0xF70ULL - offset : buf;

			memset(buf_overlap_start, 0x00, offset + size < 0x1070ULL ? size - (buf_overlap_start - buf) : 0x100ULL - (buf_overlap_start - buf));
		}

		// If it's a decrypted ISO then return, otherwise go on to the decryption logic
		if (m_enc_type == iso_encryption_type::DEC_3K3Y)
		{
			return true;
		}
	}

	// If it's an encrypted type, check if the request lies in an encrypted range
	for (const iso_region_info& info : m_region_info)
	{
		if (offset >= info.region_first_addr && offset <= info.region_last_addr)
		{
			// We found the region, decrypt if needed
			if (!info.encrypted)
			{
				return true;
			}

			// Decrypt the region before sending it back
			decrypt_data(m_aes_dec, offset, reinterpret_cast<unsigned char*>(buffer), size);

			return true;
		}
	}

	sys_log.error("decrypt(): %s: LBA request wasn't in the 'm_region_info' for an encrypted ISO? - RP: 0x%lx, RC: 0x%lx, LR: (0x%016lx - 0x%016lx)",
		name,
		offset,
		static_cast<unsigned long int>(m_region_info.size()),
		static_cast<unsigned long int>(!m_region_info.empty() ? m_region_info.back().region_first_addr : 0),
		static_cast<unsigned long int>(!m_region_info.empty() ? m_region_info.back().region_last_addr : 0));

	return true;
}

template<typename T>
inline T retrieve_endian_int(const u8* buf)
{
	T out {};

	if constexpr (std::endian::little == std::endian::native)
	{
		// First half = little-endian copy
		std::memcpy(&out, buf, sizeof(T));
	}
	else
	{
		// Second half = big-endian copy
		std::memcpy(&out, buf + sizeof(T), sizeof(T));
	}

	return out;
}

// Assumed that directory entry is at file head
static std::optional<iso_fs_metadata> iso_read_directory_entry(fs::file& entry, bool names_in_ucs2 = false)
{
	const auto start_pos = entry.pos();
	const u8 entry_length = entry.read<u8>();

	if (entry_length == 0)
	{
		return std::nullopt;
	}

	// Batch this set of file reads. This reduces overall time spent in iso_read_directory_entry by ~41%
#pragma pack(push, 1)
	struct iso_entry_header
	{
		//u8 entry_length; // Handled separately
		u8 extended_attribute_length;
		u8 start_sector[8];
		u8 file_size[8];
		u8 year;
		u8 month;
		u8 day;
		u8 hour;
		u8 minute;
		u8 second;
		u8 timezone_value;
		u8 flags;
		u8 file_unit_size;
		u8 interleave;
		u8 volume_sequence_number[4];
		u8 file_name_length;
		//u8 file_name[file_name_length]; // Handled separately
	};
#pragma pack(pop)
	static_assert(sizeof(iso_entry_header) == 32);

	const iso_entry_header header = entry.read<iso_entry_header>();

	const u32 start_sector = retrieve_endian_int<u32>(header.start_sector);
	const u32 file_size = retrieve_endian_int<u32>(header.file_size);

	std::tm file_date = {};

	file_date.tm_year = header.year;
	file_date.tm_mon = header.month - 1;
	file_date.tm_mday = header.day;
	file_date.tm_hour = header.hour;
	file_date.tm_min = header.minute;
	file_date.tm_sec = header.second;

	const s16 timezone_value = header.timezone_value;
	const s16 timezone_offset = (timezone_value - 50) * 15 * 60;

	const std::time_t date_time = std::mktime(&file_date) + timezone_offset;

	// 2nd flag bit indicates whether a given fs node is a directory
	const bool is_directory = header.flags & 0b00000010;
	const bool has_more_extents = header.flags & 0b10000000;

	std::string file_name;

	entry.read(file_name, header.file_name_length);

	if (header.file_name_length == 1 && file_name[0] == 0)
	{
		file_name = ".";
	}
	else if (file_name == "\1")
	{
		file_name = "..";
	}
	else if (names_in_ucs2) // For strings in joliet descriptor
	{
		// Characters are stored in big endian format
		const u16* raw = reinterpret_cast<const u16*>(file_name.data());
		std::u16string utf16;

		utf16.resize(header.file_name_length / 2);

		for (size_t i = 0; i < utf16.size(); ++i, raw++)
		{
			utf16[i] = *reinterpret_cast<const be_t<u16>*>(raw);
		}

		file_name = utf16_to_utf8(utf16);
	}

	if (file_name.ends_with(";1"))
	{
		file_name.erase(file_name.end() - 2, file_name.end());
	}

	if (header.file_name_length > 1 && file_name.ends_with("."))
	{
		file_name.pop_back();
	}

	// Skip the rest of the entry
	entry.seek(entry_length + start_pos);

	return iso_fs_metadata
	{
		.name = std::move(file_name),
		.time = date_time,
		.is_directory = is_directory,
		.has_multiple_extents = has_more_extents,
		.extents =
		{
			iso_extent_info
			{
				.start = start_sector,
				.size = file_size
			}
		}
	};
}

static void iso_form_hierarchy(fs::file& file, iso_fs_node& node, bool use_ucs2_decoding = false, const std::string& parent_path = "")
{
	if (!node.metadata.is_directory)
	{
		return;
	}

	std::vector<usz> multi_extent_node_indices;

	// Assuming the directory spans a single extent
	const auto& directory_extent = node.metadata.extents[0];
	const u64 end_pos = (directory_extent.start * ISO_SECTOR_SIZE) + directory_extent.size;

	file.seek(directory_extent.start * ISO_SECTOR_SIZE);

	while (file.pos() < end_pos)
	{
		auto entry = iso_read_directory_entry(file, use_ucs2_decoding);

		if (!entry)
		{
			const u64 new_sector = (file.pos() / ISO_SECTOR_SIZE) + 1;

			file.seek(new_sector * ISO_SECTOR_SIZE);
			continue;
		}

		bool extent_added = false;

		// Find previous extent and merge into it, otherwise we push this node's index
		for (usz index : multi_extent_node_indices)
		{
			auto& selected_node = ::at32(node.children, index);

			if (selected_node->metadata.name == entry->name)
			{
				// Merge into selected_node
				selected_node->metadata.extents.push_back(entry->extents[0]);

				extent_added = true;
				break;
			}
		}

		if (extent_added)
		{
			continue;
		}

		if (entry->has_multiple_extents)
		{
			// Haven't pushed entry to node.children yet so node.children::size() == entry_index
			multi_extent_node_indices.push_back(node.children.size());
		}

		node.children.push_back(std::make_unique<iso_fs_node>(iso_fs_node{
			.metadata = std::move(*entry)
		}));
	}

	for (auto& child_node : node.children)
	{
		if (child_node->metadata.name != "." && child_node->metadata.name != "..")
		{
			iso_form_hierarchy(file, *child_node, use_ucs2_decoding, parent_path + "/" + node.metadata.name);
		}
	}
}

u64 iso_fs_metadata::size() const
{
	u64 total_size = 0;

	for (const auto& extent : extents)
	{
		total_size += extent.size;
	}

	return total_size;
}

iso_archive::iso_archive(const std::string& path)
{
	m_path = path;
	m_file = fs::file(path);
	m_dec = std::make_shared<iso_file_decryption>();

	if (!m_dec->init(path))
	{
		// Not ISO... TODO: throw something??
		return;
	}

	u8 descriptor_type = -2;
	bool use_ucs2_decoding = false;

	do
	{
		const auto descriptor_start = m_file.pos();

		descriptor_type = m_file.read<u8>();

		// 1 = primary vol descriptor, 2 = joliet SVD
		if (descriptor_type == 1 || descriptor_type == 2)
		{
			use_ucs2_decoding = descriptor_type == 2;

			// Skip the rest of descriptor's data
			m_file.seek(155, fs::seek_cur);

			const auto node = iso_read_directory_entry(m_file, use_ucs2_decoding);

			if (node)
			{
				m_root = iso_fs_node
				{
					.metadata = node.value()
				};
			}
		}

		m_file.seek(descriptor_start + ISO_SECTOR_SIZE);
	}
	while (descriptor_type != 255);

	iso_form_hierarchy(m_file, m_root, use_ucs2_decoding);
}

iso_fs_node* iso_archive::retrieve(const std::string& passed_path)
{
	if (passed_path.empty())
	{
		return nullptr;
	}

	const std::string path = std::filesystem::path(passed_path).string();
	const std::string_view path_sv = path;

	size_t start = 0;
	size_t end = path_sv.find_first_of(fs::delim);

	std::stack<iso_fs_node*> search_stack;

	search_stack.push(&m_root);

	do
	{
		if (search_stack.empty())
		{
			return nullptr;
		}

		const auto* top_entry = search_stack.top();

		if (end == umax)
		{
			end = path.size();
		}

		const std::string_view path_component = path_sv.substr(start, end - start);

		bool found = false;

		if (path_component == ".")
		{
			found = true;
		}
		else if (path_component == "..")
		{
			search_stack.pop();

			found = true;
		}
		else
		{
			for (const auto& entry : top_entry->children)
			{
				if (entry->metadata.name == path_component)
				{
					search_stack.push(entry.get());

					found = true;
					break;
				}
			}
		}

		if (!found)
		{
			return nullptr;
		}

		start = end + 1;
		end = path_sv.find_first_of(fs::delim, start);
	}
	while (start < path.size());

	if (search_stack.empty())
	{
		return nullptr;
	}

	return search_stack.top();
}

bool iso_archive::exists(const std::string& path)
{
	return retrieve(path) != nullptr;
}

bool iso_archive::is_file(const std::string& path)
{
	const auto file_node = retrieve(path);

	if (!file_node)
	{
		return false;
	}

	return !file_node->metadata.is_directory;
}

iso_file iso_archive::open(const std::string& path)
{
	return iso_file(fs::file(m_path), m_dec, *ensure(retrieve(path)));
}

psf::registry iso_archive::open_psf(const std::string& path)
{
	auto* archive_file = retrieve(path);

	if (!archive_file)
	{
		return psf::registry();
	}

	const fs::file psf_file(std::make_unique<iso_file>(fs::file(m_path), m_dec, *archive_file));

	return psf::load_object(psf_file, path);
}

iso_file::iso_file(fs::file&& iso_handle, std::shared_ptr<iso_file_decryption> iso_dec, const iso_fs_node& node)
	: m_file(std::move(iso_handle)), m_dec(iso_dec), m_meta(node.metadata)
{
	m_file.seek(node.metadata.extents[0].start * ISO_SECTOR_SIZE);
}

fs::stat_t iso_file::get_stat()
{
	return fs::stat_t
	{
		.is_directory = false,
		.is_symlink = false,
		.is_writable = false,
		.size = size(),
		.atime = m_meta.time,
		.mtime = m_meta.time,
		.ctime = m_meta.time
	};
}

bool iso_file::trunc(u64 /*length*/)
{
	fs::g_tls_error = fs::error::readonly;
	return false;
}

std::pair<u64, iso_extent_info> iso_file::get_extent_pos(u64 pos) const
{
	ensure(!m_meta.extents.empty());

	auto it = m_meta.extents.begin();

	while (pos >= it->size && it != m_meta.extents.end() - 1)
	{
		pos -= it->size;

		it++;
	}

	return {pos, *it};
}

u64 iso_file::local_extent_remaining(u64 pos) const
{
	const auto [local_pos, extent] = get_extent_pos(pos);

	return extent.size - local_pos;
}

u64 iso_file::local_extent_size(u64 pos) const
{
	return get_extent_pos(pos).second.size;
}

// Assumed valid and in bounds
u64 iso_file::file_offset(u64 pos) const
{
	const auto [local_pos, extent] = get_extent_pos(pos);

	return (extent.start * ISO_SECTOR_SIZE) + local_pos;
}

u64 iso_file::read(void* buffer, u64 size)
{
	const auto r = read_at(m_pos, buffer, size);

	m_pos += r;
	return r;
}

u64 iso_file::read_at(u64 offset, void* buffer, u64 size)
{
	const u64 max_size = std::min(size, local_extent_remaining(offset));

	if (max_size == 0)
	{
		return 0;
	}

	const u64 archive_first_offset = file_offset(offset);
	const u64 total_size = this->size();
	u64 total_read;

	// If it's a non-encrypted type
	if (m_dec->get_enc_type() == iso_encryption_type::NONE)
	{
		total_read = m_file.read_at(archive_first_offset, buffer, max_size);

		if (size > total_read && (offset + total_read) < total_size)
		{
			total_read += read_at(offset + total_read, reinterpret_cast<u8*>(buffer) + total_read, size - total_read);
		}

		return total_read;
	}

	// If it's an encrypted type

	// IMPORTANT NOTE:
	//
	// "iso_file_decryption::decrypt()" method requires that offset and size are multiple of 16 bytes
	// (ciphertext block's size) and that a previous ciphertext block (used as IV) is read in case
	// offset is not a multiple of ISO_SECTOR_SIZE
	//
	//                                                ----------------------------------------------------------------------
	//                           file on ISO archive: |     '                                           '                  |
	//                                                ----------------------------------------------------------------------
	//                                                      '                                           '
	//                                                      ---------------------------------------------
	//                                              buffer: |                                           |
	//                                                      ---------------------------------------------
	//                                                      '     '                                   ' '
	//                        -------------------------------------------------------------------------------------------------------------------------------------
	//           ISO archive: | sec 0     | sec 1     |xxxxx######'###########'###########'###########'##xxxxxxxxx|           | ...       | sec n-1   | sec n     |
	//                        -------------------------------------------------------------------------------------------------------------------------------------
	// 16 Bytes x block read: |   |   |   |   |   |   |   '#######'###########'###########'###########'###|   |   |   |   |   |   |   |   |   |   |   |   |   |   |
	//                                                '           '                                   '           '
	//                                                | first sec |           inner sec(s)            | last sec  |

	const u64 archive_last_offset = archive_first_offset + max_size - 1;
	iso_sector first_sec, last_sec;
	u64 offset_aligned;
	u64 offset_aligned_first_out;

	first_sec.lba_address = (archive_first_offset / ISO_SECTOR_SIZE) * ISO_SECTOR_SIZE;
	first_sec.offset = archive_first_offset % ISO_SECTOR_SIZE;
	first_sec.size = first_sec.offset + max_size <= ISO_SECTOR_SIZE ? max_size : ISO_SECTOR_SIZE - first_sec.offset;

	last_sec.lba_address = last_sec.address_aligned = (archive_last_offset / ISO_SECTOR_SIZE) * ISO_SECTOR_SIZE;
	//last_sec.offset = last_sec.offset_aligned = 0; // Always 0 so no need to set and use those attributes
	last_sec.size = (archive_last_offset % ISO_SECTOR_SIZE) + 1;

	//
	// First sector
	//

	offset_aligned = first_sec.offset & ~0xF;
	offset_aligned_first_out = (first_sec.offset + first_sec.size) & ~0xF;

	first_sec.offset_aligned = offset_aligned != 0 ? offset_aligned - 16 : 0; // Eventually include the previous block (used as IV)
	first_sec.size_aligned = offset_aligned_first_out != (first_sec.offset + first_sec.size) ?
		offset_aligned_first_out + 16 - first_sec.offset_aligned :
		offset_aligned_first_out - first_sec.offset_aligned;
	first_sec.address_aligned = first_sec.lba_address + first_sec.offset_aligned;

	total_read = m_file.read_at(first_sec.address_aligned, &first_sec.buf.data()[first_sec.offset_aligned], first_sec.size_aligned);

	m_dec->decrypt(first_sec.address_aligned, &first_sec.buf.data()[first_sec.offset_aligned], first_sec.size_aligned, m_meta.name);
	memcpy(buffer, &first_sec.buf.data()[first_sec.offset], first_sec.size);

	u64 sector_count = (last_sec.lba_address - first_sec.lba_address) / ISO_SECTOR_SIZE + 1;

	if (sector_count < 2) // If no more sector(s)
	{
		if (total_read != first_sec.size_aligned)
		{
			sys_log.error("read_at(): %s: Error reading from file", m_meta.name);

			seek(m_pos, fs::seek_set);
			return 0;
		}

		return max_size;
	}

	//
	// Inner sector(s), if any
	//

	u64 expected_inner_sector_read = 0;

	if (sector_count > 2) // If inner sector(s) are present
	{
		u64 inner_sector_size = expected_inner_sector_read = (sector_count - 2) * ISO_SECTOR_SIZE;

		total_read += m_file.read_at(first_sec.lba_address + ISO_SECTOR_SIZE, &reinterpret_cast<u8*>(buffer)[first_sec.size], inner_sector_size);

		m_dec->decrypt(first_sec.lba_address + ISO_SECTOR_SIZE, &reinterpret_cast<u8*>(buffer)[first_sec.size], inner_sector_size, m_meta.name);
	}

	//
	// Last sector
	//

	offset_aligned_first_out = last_sec.size & ~0xF;
	last_sec.size_aligned = offset_aligned_first_out != last_sec.size ?	offset_aligned_first_out + 16 :	offset_aligned_first_out;

	total_read += m_file.read_at(last_sec.address_aligned, last_sec.buf.data(), last_sec.size_aligned);

	m_dec->decrypt(last_sec.address_aligned, last_sec.buf.data(), last_sec.size_aligned, m_meta.name);
	memcpy(&reinterpret_cast<u8*>(buffer)[max_size - last_sec.size], last_sec.buf.data(), last_sec.size);

	//
	// As last, check for an unlikely reading error (decoding also failed due to use of partially initialized buffer)
	//

	if (total_read != first_sec.size_aligned + last_sec.size_aligned + expected_inner_sector_read)
	{
		sys_log.error("read_at(): %s: Error reading from file", m_meta.name);

		seek(m_pos, fs::seek_set);
		return 0;
	}

	return max_size;
}

u64 iso_file::write(const void* /*buffer*/, u64 /*size*/)
{
	fs::g_tls_error = fs::error::readonly;
	return 0;
}

u64 iso_file::seek(s64 offset, fs::seek_mode whence)
{
	const s64 total_size = size();
	const s64 new_pos =
		whence == fs::seek_set ? offset :
		whence == fs::seek_cur ? offset + m_pos :
		whence == fs::seek_end ? offset + total_size : -1;

	if (new_pos < 0)
	{
		fs::g_tls_error = fs::error::inval;
		return -1;
	}

	if (m_file.seek(file_offset(new_pos)) == umax)
	{
		return umax;
	}

	m_pos = new_pos;
	return m_pos;
}

u64 iso_file::size()
{
	u64 extent_sizes = 0;

	for (const auto& extent : m_meta.extents)
	{
		extent_sizes += extent.size;
	}

	return extent_sizes;
}

void iso_file::release()
{
	m_file.release();
}

bool iso_dir::read(fs::dir_entry& entry)
{
	if (m_pos < m_node.children.size())
	{
		const auto& selected = m_node.children[m_pos].get()->metadata;

		entry.name = selected.name;
		entry.atime = selected.time;
		entry.mtime = selected.time;
		entry.ctime = selected.time;
		entry.is_directory = selected.is_directory;
		entry.is_symlink = false;
		entry.is_writable = false;
		entry.size = selected.size();

		m_pos++;
		return true;
	}

	return false;
}

void iso_dir::rewind()
{
	m_pos = 0;
}

bool iso_device::stat(const std::string& path, fs::stat_t& info)
{
	const auto relative_path = std::filesystem::relative(std::filesystem::path(path), std::filesystem::path(fs_prefix)).string();

	const auto node = m_archive.retrieve(relative_path);

	if (!node)
	{
		fs::g_tls_error = fs::error::noent;
		return false;
	}

	const auto& meta = node->metadata;

	info = fs::stat_t
	{
		.is_directory = meta.is_directory,
		.is_symlink = false,
		.is_writable = false,
		.size = meta.size(),
		.atime = meta.time,
		.mtime = meta.time,
		.ctime = meta.time
	};

	return true;
}

bool iso_device::statfs(const std::string& path, fs::device_stat& info)
{
	const auto relative_path = std::filesystem::relative(std::filesystem::path(path), std::filesystem::path(fs_prefix)).string();

	const auto node = m_archive.retrieve(relative_path);

	if (!node)
	{
		fs::g_tls_error = fs::error::noent;
		return false;
	}

	const u64 size = node->metadata.size();

	info = fs::device_stat
	{
		.block_size = size,
		.total_size = size,
		.total_free = 0,
		.avail_free = 0
	};

	return true;
}

std::unique_ptr<fs::file_base> iso_device::open(const std::string& path, bs_t<fs::open_mode> mode)
{
	const auto relative_path = std::filesystem::relative(std::filesystem::path(path), std::filesystem::path(fs_prefix)).string();

	const auto node = m_archive.retrieve(relative_path);

	if (!node)
	{
		fs::g_tls_error = fs::error::noent;
		return nullptr;
	}

	if (node->metadata.is_directory)
	{
		fs::g_tls_error = fs::error::isdir;
		return nullptr;
	}

	return std::make_unique<iso_file>(fs::file(m_path, mode), m_archive.get_dec(), *node);
}

std::unique_ptr<fs::dir_base> iso_device::open_dir(const std::string& path)
{
	const auto relative_path = std::filesystem::relative(std::filesystem::path(path), std::filesystem::path(fs_prefix)).string();

	const auto node = m_archive.retrieve(relative_path);

	if (!node)
	{
		fs::g_tls_error = fs::error::noent;
		return nullptr;
	}

	if (!node->metadata.is_directory)
	{
		// fs::dir::open -> ::readdir should return ENOTDIR when path is pointing to a file instead of a folder.
		fs::g_tls_error = fs::error::notdir;
		return nullptr;
	}

	return std::make_unique<iso_dir>(*node);
}

void load_iso(const std::string& path)
{
	sys_log.notice("Loading ISO '%s'", path);

	fs::set_virtual_device("iso_overlay_fs_dev", stx::make_shared<iso_device>(path));

	vfs::mount("/dev_bdvd/"sv, iso_device::virtual_device_name + "/");
}

void unload_iso()
{
	sys_log.notice("Unloading ISO");

	fs::set_virtual_device("iso_overlay_fs_dev", stx::shared_ptr<iso_device>());
}
