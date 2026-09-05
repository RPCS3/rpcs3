#include "stdafx.h"

#include "content_validation.h"
#include "IRD.h"
#include "ISO.h"
#include "PSF.h"

#include "Emu/system_utils.hpp"
#include "Utilities/File.h"
#include "Utilities/StrUtil.h"
#include "Utilities/rXml.h"
#include "Crypto/md5.h"
#include "Crypto/utils.h"

#include <zlib.h>

#include <map>

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

	iso_file file(m_path);

	// If no file exists
	if (!file)
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
		bytes_read = file.read(buf.data(), block_size);
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

//
// Disc content validation against an IRD file
//

// Path of the license file every PS3 disc holds. A JB folder missing it can still be valid, since its content
// is fully determined by the id of the game (see "check_content")
static const std::string s_lic_dat_path = "/PS3_GAME/LICDIR/LIC.DAT";

// Rebuilds the "LIC.DAT" file of a game the very same way the dumping tools do, so that a folder missing it (or
// holding a stripped one) can still be validated against the disc without writing anything to the drive
static std::vector<u8> generate_lic_dat(const std::string& game_id)
{
	static constexpr std::array<u8, 32> lic_header =
	{
		'P', 'S', '3', 'L', 'I', 'C', 'D', 'A',
		0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x08, 0x00,
		0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01
	};

	std::vector<u8> lic(0x10000);

	std::memcpy(lic.data(), lic_header.data(), lic_header.size());

	// The id of the game is stored on the second sector, right after a "1" marking the sector as used
	lic[0x800] = 0x01;
	std::memcpy(lic.data() + 0x801, game_id.data(), std::min<usz>(game_id.size(), 0x10));

	// CRC32 of the first 2304 bytes (i.e. up to the end of the id), stored big endian right after the header
	const be_t<u32> crc = ::crc32(::crc32(0, nullptr, 0), lic.data(), 2304);
	std::memcpy(lic.data() + lic_header.size(), &crc, sizeof(crc));

	return lic;
}

// MD5 hash (lowercase hex) of a buffer, i.e. of a file rebuilt in memory rather than read from the drive
static std::string md5_of_buffer(const std::vector<u8>& buffer)
{
	mbedtls_md5_context md5_ctx;
	unsigned char md5_hash[16];
	std::string hash;

	mbedtls_md5_starts_ret(&md5_ctx);
	mbedtls_md5_update_ret(&md5_ctx, buffer.data(), buffer.size());

	if (mbedtls_md5_finish_ret(&md5_ctx, md5_hash) != 0)
	{
		return {};
	}

	bytes_to_hex(hash, md5_hash, 16);
	return hash;
}

// A file of the game as it lies on the drive, either inside a JB folder or inside an ISO image
struct disc_content_file
{
	std::string path;                  // Path relative to the root of the game, with the case it has on the drive
	u64 size = 0;
	const iso_fs_node* node = nullptr; // Set only for a file held by an ISO image
	bool paired = false;               // Whether a file of the disc was matched with this one
};

// The file system of a PS3 disc is case insensitive, so every listing below is keyed by the uppercase path: the
// case a ripping tool used must not turn a file into a missing one
using disc_content_list = std::map<std::string, disc_content_file>;

// Lists every file the JB folder holds
static void list_folder_files(const std::string& root, const std::string& parent_path, disc_content_list& files)
{
	for (const fs::dir_entry& entry : fs::dir(root + parent_path))
	{
		if (entry.name == "." || entry.name == "..")
		{
			continue;
		}

		const std::string path = parent_path + "/" + entry.name;

		if (entry.is_directory)
		{
			list_folder_files(root, path, files);
			continue;
		}

		files.emplace(fmt::to_upper(path), disc_content_file{path, entry.size});
	}
}

// Lists every file the ISO image holds, keeping the node of each one so that it can be read back without walking
// the file system of the image again
static void list_iso_files(const iso_fs_node& node, const std::string& parent_path, disc_content_list& files)
{
	for (const auto& child : node.children)
	{
		const iso_fs_metadata& meta = child->metadata;

		if (meta.name == "." || meta.name == "..")
		{
			continue;
		}

		const std::string path = parent_path + "/" + meta.name;

		if (meta.is_directory)
		{
			list_iso_files(*child, path, files);
			continue;
		}

		files.emplace(fmt::to_upper(path), disc_content_file{path, meta.size(), child.get()});
	}
}

// Finds the root of the JB folder (i.e. the directory the disc starts at, the one holding "PS3_GAME") out of the
// path of a game, which can point either at the root itself or at the game directory inside it, and reports the
// name of that game directory ("PS3_GAME", or a "PS3_GMxx" one for a disc holding more than one game)
static bool resolve_jb_folder(std::string& root, std::string& game_dir)
{
	while (!root.empty() && (root.back() == '/' || root.back() == '\\'))
	{
		root.pop_back();
	}

	if (root.empty())
	{
		return false;
	}

	// The path points at the game directory of the disc: the root is the parent one
	if (fs::is_file(root + "/PARAM.SFO"))
	{
		game_dir = root.substr(root.find_last_of("/\\") + 1);
		root = fs::get_parent_dir(root);

		return !root.empty();
	}

	// The path points at the root of the disc: look for the game directory inside it
	for (const fs::dir_entry& entry : fs::dir(root))
	{
		if (!entry.is_directory || (entry.name != "PS3_GAME" && !rpcs3::utils::is_ps3_gm_dir_name(entry.name)))
		{
			continue;
		}

		if (fs::is_file(root + "/" + entry.name + "/PARAM.SFO"))
		{
			game_dir = entry.name;
			return true;
		}
	}

	return false;
}

// Same as above for an ISO image, whose root is the image itself: only the game directory has to be found
static std::string resolve_iso_game_dir(const disc_content_list& files)
{
	static const std::string sfo_suffix = "/PARAM.SFO";

	for (const auto& [key, file] : files)
	{
		if (!key.ends_with(sfo_suffix))
		{
			continue;
		}

		// Only the "PARAM.SFO" of a game directory ("/PS3_GAME/PARAM.SFO") is the one of the disc: the ones deeper
		// down the tree belong to the packages the disc carries.
		// NOTE: the name is returned with the case it has inside the image, since that is the only one the file
		//       system of the image answers to
		const std::string dir = file.path.substr(1, file.path.size() - sfo_suffix.size() - 1);

		if (dir.find('/') != umax)
		{
			continue;
		}

		if (const std::string upper_dir = fmt::to_upper(dir); upper_dir == "PS3_GAME" || rpcs3::utils::is_ps3_gm_dir_name(upper_dir))
		{
			return dir;
		}
	}

	return {};
}

// Tells whether the files of an ISO image can be read back at all. An image whose key is missing still lists its
// files just fine (the file system of a PS3 disc lies in the clear, and so does its "PARAM.SFO" more often than
// not), but hands out nothing but noise for whatever sits in an encrypted region: without this, the check would
// read the whole image only to call every single one of its files wrong.
// The files below are the very ones the key detection of "iso_file_decryption" relies on, so their magic is the
// same yardstick used there
static bool iso_content_is_readable(iso_archive& archive, const disc_content_list& content, const std::string& game_dir)
{
	static const std::array<std::pair<std::string, std::string>, 2> known_magics
	{{
		{"/USRDIR/EBOOT.BIN", "SCE"},
		{"/LICDIR/LIC.DAT", "PS3LICDA"}
	}};

	const std::string upper_game_dir = "/" + fmt::to_upper(game_dir);

	for (const auto& [suffix, magic] : known_magics)
	{
		const auto found = content.find(upper_game_dir + suffix);

		if (found == content.cend() || !found->second.node || found->second.size < magic.size())
		{
			continue;
		}

		const fs::file file(archive.get_iso_file(archive.path(), fs::read, *found->second.node));
		std::string head(magic.size(), '\0');

		if (!file || file.read(head.data(), head.size()) != head.size())
		{
			return false;
		}

		return head == magic;
	}

	// Neither file is there to tell: whatever the image holds, it is not for this check to refuse it
	return true;
}

// Read block of the hashing below. A megabyte a time is what makes reading a whole disc worth it: on an
// encrypted image every read is split into sectors and decrypted, so the fewer of them the better
static constexpr u64 s_hash_block_size = 0x100000;

bool content_validation::hash_content_file(const fs::file& file, u64 disc_size, std::string& hash)
{
	if (!file)
	{
		sys_log.error("hash_content_file: Failed to open file: %s", m_name);
		return false;
	}

	// Kept between files: a buffer of this size is zeroed out on construction, and doing that once per file of a
	// disc costs more than the reads it serves
	if (m_hash_buffer.size() != s_hash_block_size)
	{
		m_hash_buffer.resize(s_hash_block_size);
	}

	u8* const buf = m_hash_buffer.data();
	mbedtls_md5_context md5_ctx;
	unsigned char md5_hash[16];
	u64 size = 0;

	mbedtls_md5_starts_ret(&md5_ctx);

	// Only a read returning nothing at all ends the file: a short one is not to be trusted as the end of it,
	// since a file held by an ISO image is read back extent by extent
	while (m_status != content_hash_status::ABORTED)
	{
		const u64 bytes_read = file.read(buf, s_hash_block_size);

		if (!bytes_read)
		{
			break;
		}

		mbedtls_md5_update_ret(&md5_ctx, buf, bytes_read);

		size += bytes_read;
		m_bytes_read += bytes_read;
	}

	// A file shorter than the one on the disc can only match once its missing tail is hashed as zeros: that is how
	// a "PS3UPDAT.PUP" downloaded apart is turned back into the 256 MB one the disc holds
	if (disc_size > size)
	{
		std::memset(buf, 0, s_hash_block_size);

		for (u64 left = disc_size - size; left && m_status != content_hash_status::ABORTED;)
		{
			const u64 chunk = std::min<u64>(left, s_hash_block_size);

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
		sys_log.error("hash_content_file: Failed to calculate MD5 hash on file: %s", m_name);
		return false;
	}

	bytes_to_hex(hash, md5_hash, 16);
	return true;
}

disc_check_status content_validation::check_content(const std::string& game_path, const std::string& ird_path, disc_check_report& report)
{
	report = {};

	// Set right away, so that the progress bar reports something sensible while the IRD file is being parsed and
	// the content is being listed (and so that a cancellation asked for meanwhile is not lost)
	m_path = game_path;
	m_name = game_path;
	m_size = 0;
	m_bytes_read = 0;
	m_status = content_hash_status::INITIALIZED;

	std::string root = game_path;
	std::string game_dir;
	std::unique_ptr<iso_archive> archive;
	disc_content_list content;

	report.is_iso = is_iso_file(game_path);

	if (report.is_iso)
	{
		// The whole file system of the image is walked once here: every file is then read back through the node it
		// is listed with, and an encrypted image (3k3y, Redump) is decrypted on the fly while doing so
		archive = std::make_unique<iso_archive>(game_path);

		if (!archive->is_valid())
		{
			sys_log.error("check_content: Failed to open ISO file: %s", game_path);
			report.status = disc_check_status::ERROR_OPENING_ISO;
			return report.status;
		}

		list_iso_files(archive->root(), "", content);
		game_dir = resolve_iso_game_dir(content);
	}
	else
	{
		// The IRD describes the disc from its root, so the check needs the directory the disc starts at: the game
		// list points at it only when it holds a "PS3_DISC.SFB" file, otherwise it points at the game directory
		if (resolve_jb_folder(root, game_dir))
		{
			list_folder_files(root, "", content);
		}
	}

	if (game_dir.empty() || content.empty())
	{
		sys_log.error("check_content: No 'PS3_GAME/PARAM.SFO' file found: %s", game_path);
		report.status = disc_check_status::ERROR_NOT_A_PS3_GAME;
		return report.status;
	}

	sys_log.notice("check_content: Checking the %s '%s' against the IRD file '%s'", report.is_iso ? "ISO file" : "JB folder", game_path, ird_path);

	// The IRD comes first since, besides the hashes, it carries the key of the disc: an encrypted image whose key
	// file is missing is read back through that one
	const ird_file ird(ird_path);

	if (!ird.is_valid())
	{
		sys_log.error("check_content: Failed to parse IRD file: %s", ird_path);
		report.status = disc_check_status::ERROR_PARSING_IRD;
		return report.status;
	}

	// Caught before anything is hashed: reading an encrypted image no key was found for would take as long as
	// reading the whole disc, only to report every single one of its files as invalid
	if (report.is_iso && !iso_content_is_readable(*archive, content, game_dir))
	{
		if (!archive->set_disc_key(ird.get_disc_key()) || !iso_content_is_readable(*archive, content, game_dir))
		{
			sys_log.error("check_content: The ISO file is encrypted and neither a key file nor the key stored in the "
				"IRD file can read it back: %s", game_path);
			report.status = disc_check_status::ERROR_ISO_ENCRYPTED;
			return report.status;
		}

		report.decrypted_with_ird_key = true;
		sys_log.success("check_content: The ISO file is decrypted through the key stored in the IRD file: %s", game_path);
	}

	// The game is recognized the very same way the dumping tools do: through the "PARAM.SFO" of its game directory
	const std::string sfo_path = game_dir + "/PARAM.SFO";
	const psf::registry sfo = report.is_iso ? archive->open_psf(sfo_path) : psf::load_object(root + "/" + sfo_path);

	report.game_id = std::string(psf::get_string(sfo, "TITLE_ID", ""));
	report.game_title = std::string(psf::get_string(sfo, "TITLE", ""));

	if (report.game_id.empty())
	{
		sys_log.error("check_content: No 'TITLE_ID' found on '%s': %s", sfo_path, game_path);
		report.status = disc_check_status::ERROR_NOT_A_PS3_GAME;
		return report.status;
	}

	report.ird_crc_valid = ird.is_crc_valid();
	report.ird_game_id = ird.get_game_id();
	report.ird_game_name = ird.get_game_name();
	report.ird_game_version = ird.get_game_version();
	report.ird_app_version = ird.get_app_version();
	report.ird_update_version = ird.get_update_version();

	// The IRD of another game still tells which files are wrong, so the check goes on: the mismatch is only
	// reported, exactly like the dumping tools do (they just give up rebuilding the files they could fix)
	report.serial_mismatch = report.game_id != report.ird_game_id;

	if (report.serial_mismatch)
	{
		sys_log.warning("check_content: The IRD file belongs to '%s' while the game is '%s'", report.ird_game_id, report.game_id);
	}

	// An ISO image is meant to hold the disc as it is, so nothing of it is ever rebuilt: only a folder, which a
	// ripping tool regularly strips, is given the benefit of the doubt
	const bool allow_rebuild = !report.is_iso && !report.serial_mismatch;

	// Pair each file of the disc with the one of the game before hashing anything, so that the progress can be
	// reported over the whole amount of data to read
	std::vector<std::pair<const ird_file_entry*, disc_content_file*>> work_list;
	const ird_file_entry* lic_dat_file = nullptr;
	u64 total_size = 0;

	work_list.reserve(ird.get_files().size());

	for (const ird_file_entry& disc_file : ird.get_files())
	{
		const std::string key = fmt::to_upper(disc_file.path);

		// Spotted here, where the uppercase path is at hand: the loop below then tells the license file apart by
		// its very address, without a string comparison per file
		if (key == s_lic_dat_path)
		{
			lic_dat_file = &disc_file;
		}

		auto found = content.find(key);

		// Ripping tools drop the trailing dot of a name the file system of the host cannot store, so a file only
		// differing by it is still the file the disc holds
		if (found == content.end())
		{
			found = content.find(key + ".");
		}

		// Whatever the game holds answers for a single file of the disc only
		if (found == content.end() || found->second.paired)
		{
			work_list.emplace_back(&disc_file, nullptr);
			continue;
		}

		found->second.paired = true;

		// Only the files that are going to be read weigh on the progress: a size of its own already tells a file
		// apart from the one of the disc, unless a shorter one can still be padded with zeros up to it
		if (!disc_file.md5.empty() &&
			(found->second.size == disc_file.size || (allow_rebuild && found->second.size < disc_file.size)))
		{
			total_size += disc_file.size;
		}

		work_list.emplace_back(&disc_file, &found->second);
	}

	if (m_status == content_hash_status::ABORTED)
	{
		report.status = disc_check_status::ABORTED;
		return report.status;
	}

	m_path = root;
	m_size = total_size;

	// Rebuilt once, not once per file: what a rebuilt file hashes to depends on nothing but the game itself
	const std::string lic_dat_md5 = allow_rebuild ? md5_of_buffer(generate_lic_dat(report.game_id)) : std::string{};
	const std::string empty_file_md5 = allow_rebuild ? md5_of_buffer({}) : std::string{};

	usz file_index = 0;

	for (const auto& [disc_file, content_file] : work_list)
	{
		disc_file_status status = disc_file_status::MISSING;

		file_index++;

		if (content_file)
		{
			// The count goes into the name since that is all the progress dialog shows: without it, hashing every
			// file of a disc looks exactly like hashing the whole image
			m_name = fmt::format("(%d/%d) %s", file_index, work_list.size(), content_file->path);

			// Reading a file of a different size would only confirm what that size already tells: the MD5 of the
			// disc can never come out of it. Only a shorter one that is going to be padded is still worth reading.
			// An entry the IRD lists but holds no hash for has nothing to be compared against either
			if (disc_file->md5.empty() ||
				(content_file->size != disc_file->size && !(allow_rebuild && content_file->size < disc_file->size)))
			{
				status = disc_file_status::MISMATCH;
			}
			else
			{
				std::string hash;

				const fs::file file = content_file->node ?
					fs::file(archive->get_iso_file(archive->path(), fs::read, *content_file->node)) :
					fs::file(root + content_file->path);

				if (!hash_content_file(file, allow_rebuild ? disc_file->size : 0, hash))
				{
					if (m_status == content_hash_status::ABORTED)
					{
						report.status = disc_check_status::ABORTED;
						return report.status;
					}

					status = disc_file_status::MISMATCH;
				}
				else if (hash != disc_file->md5)
				{
					status = disc_file_status::MISMATCH;
				}
				else
				{
					status = content_file->size < disc_file->size ? disc_file_status::MATCH_REBUILT : disc_file_status::MATCH;
				}
			}
		}

		// Some of the files a rip regularly leaves out need nothing but the game itself to be rebuilt: the license
		// file is fully determined by its id, and an empty file has no content at all. They are rebuilt in memory
		// (nothing is ever written to the game) only to tell whether it would still work
		if (allow_rebuild && status != disc_file_status::MATCH && status != disc_file_status::MATCH_REBUILT)
		{
			if ((disc_file == lic_dat_file && lic_dat_md5 == disc_file->md5) ||
				(disc_file->size == 0 && empty_file_md5 == disc_file->md5))
			{
				status = disc_file_status::MATCH_REBUILT;
			}
		}

		switch (status)
		{
		case disc_file_status::MATCH:
			report.matched++;
			continue; // A file matching the disc is not worth reporting
		case disc_file_status::MATCH_REBUILT:
			report.matched++;
			report.rebuilt++;
			break;
		case disc_file_status::MISMATCH:
			report.mismatched++;
			break;
		case disc_file_status::MISSING:
			report.missing++;
			break;
		case disc_file_status::NOT_REQUIRED:
			break;
		}

		report.entries.push_back(disc_file_entry{disc_file->path, disc_file->size, status});
	}

	// Whatever went unpaired is not part of the disc (updates installed over a rip, leftovers of the ripping tool,
	// files added to a rebuilt image, etc.): it does not make the game invalid, so it is only listed
	for (const auto& [key, content_file] : content)
	{
		if (content_file.paired)
		{
			continue;
		}

		report.not_required++;
		report.entries.push_back(disc_file_entry{content_file.path, content_file.size, disc_file_status::NOT_REQUIRED});
	}

	m_status = content_hash_status::COMPLETED;
	report.status = (report.mismatched || report.missing) ? disc_check_status::FAILED : disc_check_status::PASSED;

	sys_log.notice("check_content: '%s' checked against '%s': %d valid | %d invalid | %d missing | %d not required",
		game_path, ird_path, report.matched, report.mismatched, report.missing, report.not_required);

	return report.status;
}
