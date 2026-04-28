#include "stdafx.h"

#include "ISO.h"
#include "Emu/VFS.h"
#include "Emu/system_utils.hpp"
#include "Crypto/utils.h"

#include <codecvt>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <stack>
#include <cstdlib>

LOG_CHANNEL(sys_log, "SYS");
LOG_CHANNEL(iso_log, "ISO");

struct iso_sector
{
	u64 lba_address;
	u64 offset;
	u64 size;
	u64 address_aligned;
	u64 offset_aligned;
	u64 size_aligned;
};

static void* get_aligned_buf()
{
	static thread_local struct aligned_buf
	{
		void* buf;

		aligned_buf() noexcept
		{
			// IMPORTANT NOTE: It must be aligned (probably enough on multiple of 4) to support raw device, otherwise any read from file will fail
#if defined(_WIN32)
			buf = _aligned_malloc(ISO_SECTOR_SIZE, ISO_SECTOR_SIZE * 2);
#else
			buf = std::aligned_alloc(ISO_SECTOR_SIZE * 2, ISO_SECTOR_SIZE);
#endif
		}

		~aligned_buf() noexcept
		{
#if defined(_WIN32)
			_aligned_free(buf);
#else
			std::free(buf);
#endif
		}
	} s_aligned_buf {};

	return s_aligned_buf.buf;
}

static bool is_iso_file(const fs::file& file, u64* size = nullptr)
{
	if (!file || file.size() < 32768ULL + 6)
	{
		return false;
	}

	char magic[5];

	file.read_at(32768ULL + 1, magic, 5);

	const bool ret = magic[0] == 'C' && magic[1] == 'D' && magic[2] == '0' && magic[3] == '0' && magic[4] == '1';

	if (size && ret)
	{
		*size = file.size();
	}

	return ret;
}

bool is_iso_file(const std::string& path, u64* size, bool* is_raw_device)
{
	if (path.empty())
	{
		return false;
	}

	std::string new_path = path;

	// "new_path" is updated with the raw device path in case "path" points to a BD drive
	const bool raw_device = fs::get_optical_raw_device(path, &new_path);

	if (!raw_device && fs::is_dir(path))
	{
		return false;
	}

	if (is_raw_device)
	{
		*is_raw_device = raw_device;
	}

	fs::file file(std::make_unique<iso_file>(new_path, fs::read));

	return is_iso_file(file, size);
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
static bool decrypt_data(aes_context& aes, u64 offset, const unsigned char* buffer, unsigned char* out_buffer, u64 size)
{
	// The following preliminary checks are good to be provided.
	// Commented out to gain a bit of performance, just because we know the caller is providing values in the expected range

	//if (size == 0)
	//{
	//	return false;
	//}

	//if ((size % 16) != 0)
	//{
	//	iso_log.error("decrypt_data: Requested ciphertext blocks' size must be a multiple of 16 (%llu)", size);
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
		std::memcpy(iv.data(), buffer, 16);
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
	if (aes_crypt_cbc(&aes, AES_DECRYPT, cur_size, iv.data(), &buffer[cur_offset], &out_buffer[cur_offset]) != 0)
	{
		iso_log.error("decrypt_data: Error decrypting data on first sector read");
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

		if (aes_crypt_cbc(&aes, AES_DECRYPT, ISO_SECTOR_SIZE, iv.data(), &buffer[cur_offset], &out_buffer[cur_offset]) != 0)
		{
			iso_log.error("decrypt_data: Error decrypting data on inner sector(s) read");
			return false;
		}

		cur_offset += ISO_SECTOR_SIZE;
	}

	reset_iv(iv, ++cur_sector_lba); // Next sector's IV

	// Partial (or even full) last sector
	if (aes_crypt_cbc(&aes, AES_DECRYPT, size - cur_offset, iv.data(), &buffer[cur_offset], &out_buffer[cur_offset]) != 0)
	{
		iso_log.error("decrypt_data: Error decrypting data on last sector read");
		return false;
	}

	return true;
}

iso_type_status iso_file_decryption::get_key(const std::string& key_path, aes_context* aes_ctx)
{
	fs::file key_file(key_path);

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
			std::memcpy(key.data(), key_str, sizeof(key));
		}
		else
		{
			hex_to_bytes(key.data(), std::string_view(key_str, key_len), static_cast<unsigned int>(key_len));
		}

		aes_context aes_dec;

		// If "aes_ctx" not requested
		if (!aes_ctx)
		{
			aes_ctx = &aes_dec;
		}

		// Create the decryption context. If the context is successfully created, fill in "aes_ctx"
		// (if requested) and return REDUMP_ISO
		if (aes_setkey_dec(aes_ctx, key.data(), 128) == 0)
		{
			return iso_type_status::REDUMP_ISO;
		}
	}

	return iso_type_status::ERROR_PROCESSING_KEY;
}

iso_type_status iso_file_decryption::retrieve_key(iso_archive& archive, std::string& key_path, aes_context& aes_ctx)
{
	//
	// Find the first existing file in the archive present on the list of well known encrypted files to use for testing a matching key
	//

	const std::map<std::string, std::string> dec_magics {
		{"PS3_GAME/LICDIR/LIC.DAT", "PS3LICDA"},
		{"PS3_GAME/USRDIR/EBOOT.BIN", "SCE"}
	};

	iso_fs_node* node = nullptr;
	std::string magic_value;

	for (const auto& magic : dec_magics)
	{
		if (node = archive.retrieve(magic.first))
		{
			magic_value = magic.second;
			break;
		}
	}

	if (!node)
	{
		return iso_type_status::ERROR_OPENING_KEY;
	}

	//
	// Read the first encrypted sector to use for testing a matching key
	//

	std::array<u8, ISO_SECTOR_SIZE> enc_sec;
	std::array<u8, ISO_SECTOR_SIZE> dec_sec;
	iso_file iso_file(archive.path(), fs::read, *node);

	if (!iso_file || iso_file.read(enc_sec.data(), ISO_SECTOR_SIZE) != ISO_SECTOR_SIZE)
	{
		return iso_type_status::NOT_ISO;
	}

	//
	// Scan all the key files present in the redump keys folder, decrypt the read sector and test for a match with file's magic value
	//

	std::vector<fs::dir_entry> entries;

	for (auto&& dir_entry : fs::dir(rpcs3::utils::get_redump_key_dir()))
	{
		// Prefetch entries, it is unsafe to keep fs::dir for a long time or for many operations
		entries.emplace_back(std::move(dir_entry));
	}

	for (auto path_it = entries.begin(); path_it != entries.end(); path_it++)
	{
		const auto dir_entry = std::move(*path_it);

		if (dir_entry.name == "." || dir_entry.name == ".." || dir_entry.is_directory)
		{
			continue;
		}

		key_path = rpcs3::utils::get_redump_key_dir() + dir_entry.name;

		// If no valid key is present on the file
		if (get_key(key_path, &aes_ctx) != iso_type_status::REDUMP_ISO)
		{
			continue;
		}

		// If the decryption fails
		if (!decrypt_data(aes_ctx, iso_file.file_offset(0), enc_sec.data(), dec_sec.data(), ISO_SECTOR_SIZE))
		{
			continue;
		}

		// If the decrypted data match the magic value
		if (std::memcmp(magic_value.data(), dec_sec.data(), magic_value.size()) == 0)
		{
			return iso_type_status::REDUMP_ISO;
		}
	}

	return iso_type_status::ERROR_OPENING_KEY;
}

iso_type_status iso_file_decryption::check_type(const std::string& path, std::string& key_path, aes_context* aes_ctx)
{
	if (!is_iso_file(path))
	{
		return iso_type_status::NOT_ISO;
	}

	// Remove file extension from file path
	const usz ext_pos = path.rfind('.');
	const std::string name_path = ext_pos == umax ? path : path.substr(0, ext_pos);

	// Detect file name (with no parent folder and no file extension)
	const usz name_pos = name_path.rfind('/');
	const std::string name = name_pos == umax ? name_path : name_path.substr(name_pos);

	const std::array<std::string, 4> key_paths {
		name_path + ".dkey",
		name_path + ".key",
		rpcs3::utils::get_redump_key_dir() + name + ".dkey",
		rpcs3::utils::get_redump_key_dir() + name + ".key"
	};

	for (const std::string& path : key_paths)
	{
		if (fs::is_file(path))
		{
			key_path = path;
			return get_key(key_path, aes_ctx);
		}
	}

	return iso_type_status::ERROR_OPENING_KEY;
}

bool iso_file_decryption::init(const std::string& path, iso_archive* archive)
{
	// Reset attributes first
	m_enc_type = iso_encryption_type::NONE;
	m_region_info.clear();

	//
	// Store the ISO region information (needed by both the "Redump" type (only on "decrypt()" method) and "3k3y" type)
	//

	fs::file iso_file(std::make_unique<iso_file>(path, fs::read));

	if (!is_iso_file(iso_file))
	{
		iso_log.error("init: Failed to recognize ISO file: %s", path);
		return false;
	}

	// Reset the file position after it was changed by is_iso_file()
	iso_file.seek(0);

	std::array<u8, ISO_SECTOR_SIZE * 2> sec0_sec1;

	if (iso_file.size() < sec0_sec1.size())
	{
		iso_log.error("init: Found only %llu sector(s) (minimum required is 2): %s", iso_file.size(), path);
		return false;
	}

	if (iso_file.read(sec0_sec1.data(), sec0_sec1.size()) != sec0_sec1.size())
	{
		iso_log.error("init: Failed to read file: %s", path);
		return false;
	}

	// NOTE:
	//
	// Following checks and assigned values are based on PS3 ISO specification.
	// E.g. all even regions (0, 2, 4 etc.) are always unencrypted while the odd ones are encrypted

	const u32 region_count = char_arr_BE_to_uint(sec0_sec1.data());

	// Ensure the region count is a proper value
	if (region_count < 1 || region_count > 127) // It's non-PS3ISO
	{
		iso_log.error("init: Failed to read region information (region_count=%lu): %s", region_count, path);
		return false;
	}

	m_region_info.resize(region_count * 2 - 1);

	for (size_t i = 0; i < m_region_info.size(); i++)
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

	iso_type_status status;
	std::string key_path;

	// If raw device and requested by the caller ("archive" provided), scan the redump keys folder and retrieve
	// (if present) the first key that allows decrypting a sector of the ISO file
	if (fs::is_optical_raw_device(path) && archive)
	{
		status = retrieve_key(*archive, key_path, m_aes_dec);
	}
	else
	{
		// Try to detect the Redump type. If so, the decryption context is set into "m_aes_dec"
		status = check_type(path, key_path, &m_aes_dec);
	}

	switch (status)
	{
	case iso_type_status::NOT_ISO:
		iso_log.warning("init: Failed to recognize ISO file: %s", path);
		break;
	case iso_type_status::REDUMP_ISO:
		iso_log.warning("init: Found matching key file: %s", key_path);
		m_enc_type = iso_encryption_type::REDUMP; // SET ENCRYPTION TYPE: REDUMP
		break;
	case iso_type_status::ERROR_OPENING_KEY:
		iso_log.warning("init: Failed to open, or missing, key file: %s", key_path);
		break;
	case iso_type_status::ERROR_PROCESSING_KEY:
		iso_log.error("init: Failed to process key file: %s", key_path);
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

		if (std::memcmp(&k3k3y_enc_watermark[0], &sec0_sec1[0xF70], sizeof(k3k3y_enc_watermark)) == 0)
		{
			// Grab D1 from the 3k3y sector
			unsigned char key[16];

			std::memcpy(key, &sec0_sec1[0xF80], 0x10);

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
				iso_log.error("init: Failed to set encryption type to ENC_3K3Y: %s", path);
			}
		}
		else if (std::memcmp(&k3k3y_dec_watermark[0], &sec0_sec1[0xF70], sizeof(k3k3y_dec_watermark)) == 0)
		{
			m_enc_type = iso_encryption_type::DEC_3K3Y; // SET ENCRYPTION TYPE: DEC_3K3Y
		}
	}

	switch (m_enc_type)
	{
	case iso_encryption_type::REDUMP:
		iso_log.warning("init: Set 'enc type': REDUMP, 'reg count': %u: %s", m_region_info.size(), path);
		break;
	case iso_encryption_type::ENC_3K3Y:
		iso_log.warning("init: Set 'enc type': ENC_3K3Y, 'reg count': %u: %s", m_region_info.size(), path);
		break;
	case iso_encryption_type::DEC_3K3Y:
		iso_log.warning("init: Set 'enc type': DEC_3K3Y, 'reg count': %u: %s", m_region_info.size(), path);
		break;
	case iso_encryption_type::NONE: // If encryption type was not set for any reason
		iso_log.warning("init: Set 'enc type': NONE, 'reg count': %u: %s", m_region_info.size(), path);
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
			decrypt_data(m_aes_dec, offset, reinterpret_cast<unsigned char*>(buffer), reinterpret_cast<unsigned char*>(buffer), size);

			return true;
		}
	}

	iso_log.error("decrypt: %s: LBA request wasn't in the 'm_region_info' for an encrypted ISO? - RP: 0x%lx, RC: 0x%lx, LR: (0x%016lx - 0x%016lx)",
		name,
		offset,
		static_cast<unsigned long int>(m_region_info.size()),
		static_cast<unsigned long int>(!m_region_info.empty() ? m_region_info.back().region_first_addr : 0),
		static_cast<unsigned long int>(!m_region_info.empty() ? m_region_info.back().region_last_addr : 0));

	return true;
}

iso_file_encrypted::iso_file_encrypted(const std::string& path, bs_t<fs::open_mode> mode, const iso_fs_node& node, std::shared_ptr<iso_file_decryption> dec)
	: iso_file(path, mode, node), m_dec(dec)
{
}

u64 iso_file_encrypted::read_at(u64 offset, void* buffer, u64 size)
{
	// IMPORTANT NOTES:
	// - For a raw device, we must use a support buffer aligned (probably enough on multiple of 4), otherwise any read from file will fail.
	//   For that reason, we don't use directly "buffer" (not guaranteeing any alignment)
	// - "iso_file_decryption::decrypt()" method requires that offset and size are multiple of 16 bytes (ciphertext block's size)
	//   and that a previous ciphertext block (used as IV) is read in case offset is not a multiple of ISO_SECTOR_SIZE
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

	const u64 max_size = std::min(size, local_extent_remaining(offset));

	if (max_size == 0)
	{
		return 0;
	}

	const u64 archive_first_offset = file_offset(offset);
	const u64 archive_last_offset = archive_first_offset + max_size - 1;
	iso_sector first_sec, last_sec;
	void* aligned_buf = get_aligned_buf(); // thread-safe buffer

	first_sec.lba_address = (archive_first_offset / ISO_SECTOR_SIZE) * ISO_SECTOR_SIZE;
	first_sec.offset = archive_first_offset % ISO_SECTOR_SIZE;
	first_sec.size = first_sec.offset + max_size <= ISO_SECTOR_SIZE ? max_size : ISO_SECTOR_SIZE - first_sec.offset;

	last_sec.lba_address = last_sec.address_aligned = (archive_last_offset / ISO_SECTOR_SIZE) * ISO_SECTOR_SIZE;
	// last_sec.offset = last_sec.offset_aligned = 0; // Always 0 so no need to set and use those attributes
	last_sec.size = (archive_last_offset % ISO_SECTOR_SIZE) + 1;

	//
	// First sector
	//

	u64 offset_aligned_first_out = 0;

	if (!m_raw_device)
	{
		const u64 offset_aligned = first_sec.offset & ~0xF;
		offset_aligned_first_out = (first_sec.offset + first_sec.size) & ~0xF;

		first_sec.offset_aligned = offset_aligned != 0 ? offset_aligned - 16 : 0; // Eventually include the previous block (used as IV)
		first_sec.size_aligned = offset_aligned_first_out != (first_sec.offset + first_sec.size) ?
			offset_aligned_first_out + 16 - first_sec.offset_aligned :
			offset_aligned_first_out - first_sec.offset_aligned;
		first_sec.address_aligned = first_sec.lba_address + first_sec.offset_aligned;
	}
	else
	{
		first_sec.offset_aligned = 0;
		first_sec.size_aligned = ISO_SECTOR_SIZE;
		first_sec.address_aligned = first_sec.lba_address;
	}

	u64 total_read = m_file.read_at(first_sec.address_aligned, &reinterpret_cast<u8*>(aligned_buf)[first_sec.offset_aligned], first_sec.size_aligned);

	m_dec->decrypt(first_sec.address_aligned, &reinterpret_cast<u8*>(aligned_buf)[first_sec.offset_aligned], first_sec.size_aligned, m_meta.name);
	std::memcpy(buffer, &reinterpret_cast<u8*>(aligned_buf)[first_sec.offset], first_sec.size);

	const u64 sector_count = (last_sec.lba_address - first_sec.lba_address) / ISO_SECTOR_SIZE + 1;

	if (sector_count < 2) // If no more sector(s)
	{
		if (total_read != first_sec.size_aligned)
		{
			iso_log.error("read_at: %s: Error reading from file (%llu/%llu)", m_meta.name, total_read, first_sec.size_aligned);
			return 0;
		}

		return max_size;
	}

	//
	// Inner sector(s), if any
	//

	if (sector_count > 2) // If inner sector(s) are present
	{
		if (!m_raw_device)
		{
			const u64 inner_sector_size = (sector_count - 2) * ISO_SECTOR_SIZE;

			total_read += m_file.read_at(first_sec.lba_address + ISO_SECTOR_SIZE, &reinterpret_cast<u8*>(buffer)[first_sec.size], inner_sector_size);

			m_dec->decrypt(first_sec.lba_address + ISO_SECTOR_SIZE, &reinterpret_cast<u8*>(buffer)[first_sec.size], inner_sector_size, m_meta.name);
		}
		else
		{
			u64 inner_sector_offset = 0;

			for (u64 i = 0; i < sector_count - 2; i++, inner_sector_offset += ISO_SECTOR_SIZE)
			{
				total_read += m_file.read_at(first_sec.lba_address + ISO_SECTOR_SIZE + inner_sector_offset, aligned_buf, ISO_SECTOR_SIZE);

				m_dec->decrypt(first_sec.lba_address + ISO_SECTOR_SIZE + inner_sector_offset, aligned_buf, ISO_SECTOR_SIZE, m_meta.name);
				std::memcpy(&reinterpret_cast<u8*>(buffer)[first_sec.size + inner_sector_offset], aligned_buf, ISO_SECTOR_SIZE);
			}
		}
	}

	//
	// Last sector
	//

	if (!m_raw_device)
	{
		offset_aligned_first_out = last_sec.size & ~0xF;

		last_sec.size_aligned = offset_aligned_first_out != last_sec.size ? offset_aligned_first_out + 16 : offset_aligned_first_out;
	}
	else
	{
		last_sec.size_aligned = ISO_SECTOR_SIZE;
	}

	total_read += m_file.read_at(last_sec.address_aligned, aligned_buf, last_sec.size_aligned);

	m_dec->decrypt(last_sec.address_aligned, aligned_buf, last_sec.size_aligned, m_meta.name);
	std::memcpy(&reinterpret_cast<u8*>(buffer)[max_size - last_sec.size], aligned_buf, last_sec.size);

	//
	// As last, check for an unlikely reading error (decoding also failed due to use of partially initialized buffer)
	//

	if (total_read != first_sec.size_aligned + last_sec.size_aligned + (sector_count - 2) * ISO_SECTOR_SIZE)
	{
		iso_log.error("read_at: %s: Error reading from file (%llu/%llu)", m_meta.name,
			total_read, ISO_SECTOR_SIZE + ISO_SECTOR_SIZE + (sector_count - 2) * ISO_SECTOR_SIZE);

		return 0;
	}

	return max_size;
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

		for (usz i = 0; i < utf16.size(); i++, raw++)
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

	// "m_path" is updated with the raw device path in case "path" points to a BD drive
	fs::get_optical_raw_device(path, &m_path);

	fs::file iso_file(std::make_unique<iso_file>(m_path, fs::read));

	if (!iso_file || !is_iso_file(iso_file))
	{
		// Not ISO... TODO: throw something?
		iso_log.error("iso_archive: Failed to recognize ISO file: %s", path);
		return;
	}

	// Reset the file position after it was changed by is_iso_file()
	iso_file.seek(0);

	u8 descriptor_type = -2;
	bool use_ucs2_decoding = false;

	do
	{
		const auto descriptor_start = iso_file.pos();

		descriptor_type = iso_file.read<u8>();

		// 1 = primary vol descriptor, 2 = joliet SVD
		if (descriptor_type == 1 || descriptor_type == 2)
		{
			use_ucs2_decoding = descriptor_type == 2;

			// Skip the rest of descriptor's data
			iso_file.seek(155, fs::seek_cur);

			const auto node = iso_read_directory_entry(iso_file, use_ucs2_decoding);

			if (node)
			{
				m_root = iso_fs_node
				{
					.metadata = node.value()
				};
			}
		}

		iso_file.seek(descriptor_start + ISO_SECTOR_SIZE);
	}
	while (descriptor_type != 255);

	iso_form_hierarchy(iso_file, m_root, use_ucs2_decoding);

	// Only when the archive object is fully set, we can finally initialize the decryption object needing the archive object
	m_dec = std::make_shared<iso_file_decryption>();

	if (!m_dec->init(m_path, this))
	{
		// TODO: throw something?
		return;
	}
}

iso_fs_node* iso_archive::retrieve(const std::string& passed_path)
{
	if (passed_path.empty())
	{
		return nullptr;
	}

	const std::string path = std::filesystem::path(passed_path).string();
	const std::string_view path_sv = path;

	usz start = 0;
	usz end = path_sv.find_first_of(fs::delim);

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

std::unique_ptr<fs::file_base> iso_archive::get_iso_file(const std::string& path, bs_t<fs::open_mode> mode, const iso_fs_node& node)
{
	if (m_dec->get_enc_type() == iso_encryption_type::NONE)
	{
		return std::make_unique<iso_file>(path, mode, node);
	}

	return std::make_unique<iso_file_encrypted>(path, mode, node, m_dec);
}

std::unique_ptr<fs::file_base> iso_archive::open(const std::string& path)
{
	return get_iso_file(m_path, fs::read, *ensure(retrieve(path)));
}

psf::registry iso_archive::open_psf(const std::string& path)
{
	const auto node = retrieve(path);

	if (!node)
	{
		return psf::registry();
	}

	const fs::file psf_file(get_iso_file(m_path, fs::read, *node));

	return psf::load_object(psf_file, path);
}

iso_file::iso_file(const std::string& path, bs_t<fs::open_mode> mode)
{
	m_file = fs::file(path, mode);

	if (!m_file)
	{
		// Should never happen... TODO: throw something?
		iso_log.error("iso_file: Failed to open file: %s", path);
		return;
	}

	m_meta.name = path;
	m_meta.extents.push_back({0, m_file.size()});

	m_file.seek(m_meta.extents[0].start * ISO_SECTOR_SIZE);

	m_raw_device = fs::is_optical_raw_device(path);
}

iso_file::iso_file(const std::string& path, bs_t<fs::open_mode> mode, const iso_fs_node& node)
	: m_meta(node.metadata)
{
	m_file = fs::file(path, mode);

	if (!m_file)
	{
		// Should never happen... TODO: throw something?
		iso_log.error("iso_file: Failed to open file: %s", path);
		return;
	}

	m_file.seek(m_meta.extents[0].start * ISO_SECTOR_SIZE);

	m_raw_device = fs::is_optical_raw_device(path);
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

	// If it's not a raw device
	if (!m_raw_device)
	{
		u64 total_read = m_file.read_at(archive_first_offset, buffer, max_size);

		if (total_read != max_size)
		{
			iso_log.error("read_at: %s: Error reading from file (%llu/%llu)", m_meta.name, total_read, max_size);
			return 0;
		}

		return max_size;
	}

	// If it's a raw device

	// IMPORTANT NOTE:
	//
	// For a raw device, we must use a support buffer aligned (probably enough on multiple of 4), otherwise any read from file will fail.
	// For that reason, we don't use directly "buffer" (not guaranteeing any alignment)
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
	//                                                '           '                                   '           '
	//                                                | first sec |           inner sec(s)            | last sec  |

	const u64 archive_last_offset = archive_first_offset + max_size - 1;
	iso_sector first_sec, last_sec;
	void* aligned_buf = get_aligned_buf(); // thread-safe buffer

	first_sec.lba_address = (archive_first_offset / ISO_SECTOR_SIZE) * ISO_SECTOR_SIZE;
	first_sec.offset = archive_first_offset % ISO_SECTOR_SIZE;
	first_sec.size = first_sec.offset + max_size <= ISO_SECTOR_SIZE ? max_size : ISO_SECTOR_SIZE - first_sec.offset;

	last_sec.lba_address = last_sec.address_aligned = (archive_last_offset / ISO_SECTOR_SIZE) * ISO_SECTOR_SIZE;
	// last_sec.offset = last_sec.offset_aligned = 0; // Always 0 so no need to set and use those attributes
	last_sec.size = (archive_last_offset % ISO_SECTOR_SIZE) + 1;

	//
	// First sector
	//

	u64 total_read = m_file.read_at(first_sec.lba_address, aligned_buf, ISO_SECTOR_SIZE);

	std::memcpy(buffer, &reinterpret_cast<u8*>(aligned_buf)[first_sec.offset], first_sec.size);

	const u64 sector_count = (last_sec.lba_address - first_sec.lba_address) / ISO_SECTOR_SIZE + 1;

	if (sector_count < 2) // If no more sector(s)
	{
		if (total_read != ISO_SECTOR_SIZE)
		{
			iso_log.error("read_at: %s: Error reading from file (%llu/%llu)", m_meta.name, total_read, ISO_SECTOR_SIZE);
			return 0;
		}

		return max_size;
	}

	//
	// Inner sector(s), if any
	//

	if (sector_count > 2) // If inner sector(s) are present
	{
		u64 sector_offset = 0;

		for (u64 i = 0; i < sector_count - 2; i++, sector_offset += ISO_SECTOR_SIZE)
		{
			total_read += m_file.read_at(first_sec.lba_address + ISO_SECTOR_SIZE + sector_offset, aligned_buf, ISO_SECTOR_SIZE);

			std::memcpy(&reinterpret_cast<u8*>(buffer)[first_sec.size + sector_offset], aligned_buf, ISO_SECTOR_SIZE);
		}
	}

	//
	// Last sector
	//

	total_read += m_file.read_at(last_sec.address_aligned, aligned_buf, ISO_SECTOR_SIZE);

	std::memcpy(&reinterpret_cast<u8*>(buffer)[max_size - last_sec.size], aligned_buf, last_sec.size);

	//
	// As last, check for an unlikely reading error
	//

	if (total_read != ISO_SECTOR_SIZE + ISO_SECTOR_SIZE + (sector_count - 2) * ISO_SECTOR_SIZE)
	{
		iso_log.error("read_at: %s: Error reading from file (%llu/%llu)", m_meta.name,
			total_read, ISO_SECTOR_SIZE + ISO_SECTOR_SIZE + (sector_count - 2) * ISO_SECTOR_SIZE);

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
	const s64 new_pos =
		whence == fs::seek_set ? offset :
		whence == fs::seek_cur ? offset + m_pos :
		whence == fs::seek_end ? offset + size() : -1;

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

	return m_archive.get_iso_file(m_archive.path(), mode, *node);
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
