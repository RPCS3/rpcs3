#include "stdafx.h"

#include "ISO.h"
#include "Emu/VFS.h"
#include "Crypto/utils.h"

#include <codecvt>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <stack>

LOG_CHANNEL(sys_log, "SYS");

constexpr u64 ISO_SECTOR_SIZE = 2048;

struct iso_sector
{
	u64 archive_first_addr = 0;
	u64 offset = 0;
	u64 size = 0;
	std::array<u8, ISO_SECTOR_SIZE> buf {};
};

bool is_file_iso(const std::string& path)
{
	if (path.empty()) return false;
	if (fs::is_dir(path)) return false;

	return is_file_iso(fs::file(path));
}

bool is_file_iso(const fs::file& file)
{
	if (!file) return false;
	if (file.size() < 32768 + 6) return false;

	file.seek(32768);

	char magic[5];
	file.read_at(32768 + 1, magic, 5);

	return magic[0] == 'C' && magic[1] == 'D'
		&& magic[2] == '0' && magic[3] == '0'
		&& magic[4] == '1';
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

// Main function that will decrypt the sector(s) (needs to be a multiple of ISO_SECTOR_SIZE)
static void decrypt_data(aes_context& aes, unsigned char* data, u32 sector_count, u32 start_lba)
{
	// Micro optimization, only ever used by "decrypt_data()"
	std::array<u8, 16> iv;

	for (u32 i = 0; i < sector_count; ++i)
	{
		reset_iv(iv, start_lba + i);

		if (aes_crypt_cbc(&aes, AES_DECRYPT, ISO_SECTOR_SIZE, iv.data(), &data[ISO_SECTOR_SIZE * i], &data[ISO_SECTOR_SIZE * i]) != 0)
		{
			sys_log.error("decrypt_data(): Error decrypting data (decrypt_data() > aes_crypt_cbc())");
			return;
		}
	}
}

void iso_file_decryption::reset()
{
	m_enc_type = iso_encryption_type::NONE;
	m_region_info.clear();
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

	const usz ext_pos = path.rfind('.');
	std::string key_path;

	// If no file extension is provided, set "key_path" appending ".dkey" to "path".
	// Otherwise, replace the extension (e.g. ".iso") with ".dkey"
	key_path = ext_pos == umax ? path + ".dkey" : path.substr(0, ext_pos) + ".dkey";

	fs::file key_file(key_path);

	// If no ".dkey" file exists, try with ".key"
	if (!key_file)
	{
		key_path = ext_pos == umax ? path + ".key" : path.substr(0, ext_pos) + ".key";
		key_file = fs::file(key_path);
	}

	// Check if "key_path" exists and create the "m_aes_dec" context if so
	if (key_file)
	{
		char key_str[32];
		unsigned char key[16];

		const u64 key_len = key_file.read(key_str, sizeof(key_str));

		if (key_len == sizeof(key_str) || key_len == sizeof(key))
		{
			// If the key read from the key file is 16 bytes long instead of 32, consider the file as
			// binary (".key") and so not needing any further conversion from hex string to bytes
			if (key_len == sizeof(key))
			{
				memcpy(key, key_str, sizeof(key));
			}
			else
			{
				hex_to_bytes(key, std::string_view(key_str, key_len), key_len);
			}

			if (aes_setkey_dec(&m_aes_dec, key, 128) == 0)
			{
				m_enc_type = iso_encryption_type::REDUMP; // SET ENCRYPTION TYPE: REDUMP
			}
		}

		if (m_enc_type == iso_encryption_type::NONE) // If encryption type was not set to REDUMP for any reason
		{
			sys_log.error("init(): Failed to process key file: %s", key_path);
		}
	}
	else
	{
		sys_log.warning("init(): Failed to open, or missing, key file: %s", key_path);
	}

	//
	// Check for 3k3y type
	//

	// If encryption type is still set to none
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
			decrypt_data(m_aes_dec, reinterpret_cast<unsigned char*>(buffer),
				static_cast<u32>(size / ISO_SECTOR_SIZE), static_cast<u32>(offset / ISO_SECTOR_SIZE));

			// TODO: Sanity check.
			//       We are assuming that reads always start at the beginning of a sector and that all reads will be
			//       multiples of ISO_SECTOR_SIZE
			//
			// NOTE: Both are easy fixes, but, code can be more simple + efficient if these two conditions are true
			//       (which they look to be from initial testing)

			/*if (size % ISO_SECTOR_SIZE != 0 || offset % ISO_SECTOR_SIZE != 0)
			{
				sys_log.error("decrypt(): %s: Encryption assumptions were not met, code needs to be updated, your game is probably about to crash - offset: 0x%lx, size: 0x%lx",
					name,
					static_cast<unsigned long int>(offset),
					static_cast<unsigned long int>(size);
			}*/

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
inline T read_both_endian_int(fs::file& file)
{
	T out;

	if (std::endian::little == std::endian::native)
	{
		out = file.read<T>();
		file.seek(sizeof(T), fs::seek_cur);
	}
	else
	{
		file.seek(sizeof(T), fs::seek_cur);
		out = file.read<T>();
	}

	return out;
}

// Assumed that directory entry is at file head
static std::optional<iso_fs_metadata> iso_read_directory_entry(fs::file& file, bool names_in_ucs2 = false)
{
	const auto start_pos = file.pos();
	const u8 entry_length = file.read<u8>();

	if (entry_length == 0) return std::nullopt;

	file.seek(1, fs::seek_cur);
	const u32 start_sector = read_both_endian_int<u32>(file);
	const u32 file_size = read_both_endian_int<u32>(file);

	std::tm file_date = {};
	file_date.tm_year = file.read<u8>();
	file_date.tm_mon = file.read<u8>() - 1;
	file_date.tm_mday = file.read<u8>();
	file_date.tm_hour = file.read<u8>();
	file_date.tm_min = file.read<u8>();
	file_date.tm_sec = file.read<u8>();
	const s16 timezone_value = file.read<u8>();
	const s16 timezone_offset = (timezone_value - 50) * 15 * 60;

	const std::time_t date_time = std::mktime(&file_date) + timezone_offset;

	const u8 flags = file.read<u8>();

	// 2nd flag bit indicates whether a given fs node is a directory
	const bool is_directory = flags & 0b00000010;
	const bool has_more_extents = flags & 0b10000000;

	file.seek(6, fs::seek_cur);

	const u8 file_name_length = file.read<u8>();

	std::string file_name;
	file.read(file_name, file_name_length);

	if (file_name_length == 1 && file_name[0] == 0)
	{
		file_name = ".";
	}
	else if (file_name == "\1")
	{
		file_name = "..";
	}
	else if (names_in_ucs2) // for strings in joliet descriptor
	{
		// characters are stored in big endian format.
		std::u16string utf16;
		utf16.resize(file_name_length / 2);

		const u16* raw = reinterpret_cast<const u16*>(file_name.data());
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

	if (file_name_length > 1 && file_name.ends_with("."))
	{
		file_name.pop_back();
	}

	// skip the rest of the entry.
	file.seek(entry_length + start_pos);

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
	if (passed_path.empty()) return nullptr;

	const std::string path = std::filesystem::path(passed_path).string();
	const std::string_view path_sv = path;

	size_t start = 0;
	size_t end = path_sv.find_first_of(fs::delim);

	std::stack<iso_fs_node*> search_stack;
	search_stack.push(&m_root);

	do
	{
		if (search_stack.empty()) return nullptr;
		const auto* top_entry = search_stack.top();

		if (end == umax)
		{
			end = path.size();
		}

		const std::string_view path_component = path_sv.substr(start, end-start);

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

		if (!found) return nullptr;

		start = end + 1;
		end = path_sv.find_first_of(fs::delim, start);
	}
	while (start < path.size());

	if (search_stack.empty()) return nullptr;

	return search_stack.top();
}

bool iso_archive::exists(const std::string& path)
{
	return retrieve(path) != nullptr;
}

bool iso_archive::is_file(const std::string& path)
{
	const auto file_node = retrieve(path);
	if (!file_node) return false;

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

	// If it's a non-encrypted type
	if (m_dec->get_enc_type() == iso_encryption_type::NONE)
	{
		u64 total_read = m_file.read_at(archive_first_offset, buffer, max_size);

		if (size > total_read && (offset + total_read) < total_size)
		{
			total_read += read_at(offset + total_read, reinterpret_cast<u8*>(buffer) + total_read, size - total_read);
		}

		return total_read;
	}

	// If it's an encrypted type

	// IMPORTANT NOTE:
	//
	// For performance reasons, current "iso_file_decryption::decrypt()" method requires that reads always
	// start at the beginning of a sector and that all reads will be multiples of ISO_SECTOR_SIZE.
	// Both are easy fixes, but, code can be more simple + efficient if these two conditions are true
	// (which they look to be from initial testing)
	//
	//
	// TODO:
	//
	// The local buffers storing the first and last sector of the reads could be replaced by caches.
	// That will allow to avoid to read and decrypt again the entire sector on a further call to "read_at()" method
	// if trying to read the previously not requested bytes (marked with "x" in the picture below)
	//
	//                                -------------------------------------------------------------------
	//           file on ISO archive: |     '                                     '                     |
	//                                -------------------------------------------------------------------
	//                                      '                                     '
	//                                      ---------------------------------------
	//                              buffer: |                                     |
	//                                      ---------------------------------------
	//                                      '     '                             ' '
	//              ---------------------------------------------------------------------------------------------------------------
	// ISO archive: | sec 0   | sec 1   |xxx'#####|#########|#########|#########|#'xxxxxxx|         | ...     | sec n-1 | sec n   |
	//              ---------------------------------------------------------------------------------------------------------------
	//                                  '         '                             '         '
	//                                  |first sec|                             |last sec |

	const u64 archive_last_offset = archive_first_offset + max_size - 1;
	iso_sector first_sec, last_sec;

	first_sec.archive_first_addr = (archive_first_offset / ISO_SECTOR_SIZE) * ISO_SECTOR_SIZE;
	first_sec.offset = archive_first_offset % ISO_SECTOR_SIZE;
	first_sec.size = archive_last_offset < (first_sec.archive_first_addr + ISO_SECTOR_SIZE) ? max_size : ISO_SECTOR_SIZE - first_sec.offset;
	last_sec.archive_first_addr = (archive_last_offset / ISO_SECTOR_SIZE) * ISO_SECTOR_SIZE;
	last_sec.offset = 0;
	last_sec.size = (archive_last_offset % ISO_SECTOR_SIZE) + 1;

	u64 sec_count = (last_sec.archive_first_addr - first_sec.archive_first_addr) / ISO_SECTOR_SIZE + 1;
	u64 sec_inner_size = (sec_count - 2) * ISO_SECTOR_SIZE;

	if (m_file.read_at(first_sec.archive_first_addr, first_sec.buf.data(), ISO_SECTOR_SIZE) != ISO_SECTOR_SIZE)
	{
		sys_log.error("read_at(): %s: Error reading from file", m_meta.name);

		seek(m_pos, fs::seek_set);
		return 0;
	}

	// Decrypt read buffer (if needed) and copy to destination buffer
	m_dec->decrypt(first_sec.archive_first_addr, first_sec.buf.data(), ISO_SECTOR_SIZE, m_meta.name);
	memcpy(buffer, first_sec.buf.data() + first_sec.offset, first_sec.size);

	// If the sector was already read, decrypted and copied to destination buffer, nothing more to do
	if (sec_count < 2)
	{
		return max_size;
	}

	if (m_file.read_at(last_sec.archive_first_addr, last_sec.buf.data(), ISO_SECTOR_SIZE) != ISO_SECTOR_SIZE)
	{
		sys_log.error("read_at(): %s: Error reading from file", m_meta.name);

		seek(m_pos, fs::seek_set);
		return 0;
	}

	// Decrypt read buffer (if needed) and copy to destination buffer
	m_dec->decrypt(last_sec.archive_first_addr, last_sec.buf.data(), ISO_SECTOR_SIZE, m_meta.name);
	memcpy(reinterpret_cast<u8*>(buffer) + max_size - last_sec.size, last_sec.buf.data(), last_sec.size);

	// If the sector was already read, decrypted and copied to destination buffer, nothing more to do
	if (sec_count < 3)
	{
		return max_size;
	}

	if (m_file.read_at(first_sec.archive_first_addr + ISO_SECTOR_SIZE, reinterpret_cast<u8*>(buffer) + first_sec.size, sec_inner_size) != sec_inner_size)
	{
		sys_log.error("read_at(): %s: Error reading from file", m_meta.name);

		seek(m_pos, fs::seek_set);
		return 0;
	}

	// Decrypt read buffer (if needed) and copy to destination buffer
	m_dec->decrypt(first_sec.archive_first_addr + ISO_SECTOR_SIZE, reinterpret_cast<u8*>(buffer) + first_sec.size, sec_inner_size, m_meta.name);

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

	const u64 result = m_file.seek(file_offset(m_pos));
	if (result == umax) return umax;

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

	const auto& meta = node->metadata;
	const u64 size = meta.size();

	info = fs::device_stat
	{
		.block_size = size,
		.total_size = size,
		.total_free = 0,
		.avail_free = 0
	};

	return false;
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

void iso_dir::rewind()
{
	m_pos = 0;
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
