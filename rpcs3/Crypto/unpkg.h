#pragma once

#include "Loader/PSF.h"
#include "util/endian.hpp"
#include "util/types.hpp"
#include "Utilities/File.h"
#include <sstream>
#include <iomanip>
#include <span>
#include <deque>

// Constants
enum : u32
{
	PKG_HEADER_SIZE  = 0xC0, // sizeof(pkg_header) + sizeof(pkg_unk_checksum)
	PKG_HEADER_SIZE2 = 0x280,
	PKG_MAX_FILENAME_SIZE = 256,
};

enum : u16
{
	PKG_RELEASE_TYPE_RELEASE      = 0x8000,
	PKG_RELEASE_TYPE_DEBUG        = 0x0000,

	PKG_PLATFORM_TYPE_PS3         = 0x0001,
	PKG_PLATFORM_TYPE_PSP_PSVITA  = 0x0002,
};

enum : u32
{
	PKG_FILE_ENTRY_NPDRM          = 1,
	PKG_FILE_ENTRY_NPDRMEDAT      = 2,
	PKG_FILE_ENTRY_REGULAR        = 3,
	PKG_FILE_ENTRY_FOLDER         = 4,
	PKG_FILE_ENTRY_UNK0           = 5,
	PKG_FILE_ENTRY_UNK1           = 6,
	PKG_FILE_ENTRY_SDAT           = 9,

	PKG_FILE_ENTRY_OVERWRITE      = 0x80000000,
	PKG_FILE_ENTRY_PSP            = 0x10000000,

	PKG_FILE_ENTRY_KNOWN_BITS     = 0xff | PKG_FILE_ENTRY_PSP | PKG_FILE_ENTRY_OVERWRITE,
};

enum : u32
{
	PKG_CONTENT_TYPE_UNKNOWN_1      = 0x01, // ?
	PKG_CONTENT_TYPE_UNKNOWN_2      = 0x02, // ?
	PKG_CONTENT_TYPE_UNKNOWN_3      = 0x03, // ?
	PKG_CONTENT_TYPE_GAME_DATA      = 0x04, // GameData (also patches)
	PKG_CONTENT_TYPE_GAME_EXEC      = 0x05, // GameExec
	PKG_CONTENT_TYPE_PS1_EMU        = 0x06, // PS1emu
	PKG_CONTENT_TYPE_PC_ENGINE      = 0x07, // PSP & PCEngine
	PKG_CONTENT_TYPE_UNKNOWN_4      = 0x08, // ?
	PKG_CONTENT_TYPE_THEME          = 0x09, // Theme
	PKG_CONTENT_TYPE_WIDGET         = 0x0A, // Widget
	PKG_CONTENT_TYPE_LICENSE        = 0x0B, // License
	PKG_CONTENT_TYPE_VSH_MODULE     = 0x0C, // VSHModule
	PKG_CONTENT_TYPE_PSN_AVATAR     = 0x0D, // PSN Avatar
	PKG_CONTENT_TYPE_PSP_GO         = 0x0E, // PSPgo
	PKG_CONTENT_TYPE_MINIS          = 0x0F, // Minis
	PKG_CONTENT_TYPE_NEOGEO         = 0x10, // NEOGEO
	PKG_CONTENT_TYPE_VMC            = 0x11, // VMC
	PKG_CONTENT_TYPE_PS2_CLASSIC    = 0x12, // ?PS2Classic? Seen on PS2 classic
	PKG_CONTENT_TYPE_UNKNOWN_5      = 0x13, // ?
	PKG_CONTENT_TYPE_PSP_REMASTERED = 0x14, // ?
	PKG_CONTENT_TYPE_PSP2_GD        = 0x15, // PSVita Game Data
	PKG_CONTENT_TYPE_PSP2_AC        = 0x16, // PSVita Additional Content
	PKG_CONTENT_TYPE_PSP2_LA        = 0x17, // PSVita LiveArea
	PKG_CONTENT_TYPE_PSM_1          = 0x18, // PSVita PSM ?
	PKG_CONTENT_TYPE_WT             = 0x19, // Web TV ?
	PKG_CONTENT_TYPE_UNKNOWN_6      = 0x1A, // ?
	PKG_CONTENT_TYPE_UNKNOWN_7      = 0x1B, // ?
	PKG_CONTENT_TYPE_UNKNOWN_8      = 0x1C, // ?
	PKG_CONTENT_TYPE_PSM_2          = 0x1D, // PSVita PSM ?
	PKG_CONTENT_TYPE_UNKNOWN_9      = 0x1E, // ?
	PKG_CONTENT_TYPE_PSP2_THEME     = 0x1F, // PSVita Theme
};

// Structs
struct PKGHeader
{
	le_t<u32> pkg_magic;    // Magic (0x7f504b47) (" PKG")
	be_t<u16> pkg_type;     // Release type (Retail:0x8000, Debug:0x0000)
	be_t<u16> pkg_platform; // Platform type (PS3:0x0001, PSP:0x0002)
	be_t<u32> meta_offset;  // Metadata offset. Usually 0xC0 for PS3, usually 0x280 for PSP and PSVita
	be_t<u32> meta_count;   // Metadata item count
	be_t<u32> meta_size;    // Metadata size.
	be_t<u32> file_count;   // Number of files
	be_t<u64> pkg_size;     // PKG size in bytes
	be_t<u64> data_offset;  // Encrypted data offset
	be_t<u64> data_size;    // Encrypted data size in bytes
	char title_id[48];      // Title ID
	be_t<u64> qa_digest[2]; // This should be the hash of "files + attribs"
	be_t<u128> klicensee;   // Nonce
	// + some stuff
};

// Extended header in PSP and PSVita packages
struct PKGExtHeader
{
	le_t<u32> magic;                            // 0x7F657874 (" ext")
	be_t<u32> unknown_1;                        // Maybe version. always 1
	be_t<u32> ext_hdr_size;                     // Extended header size. ex: 0x40
	be_t<u32> ext_data_size;                    // ex: 0x180
	be_t<u32> main_and_ext_headers_hmac_offset; // ex: 0x100
	be_t<u32> metadata_header_hmac_offset;      // ex: 0x360, 0x390, 0x490
	be_t<u64> tail_offset;                      // tail size seams to be always 0x1A0
	be_t<u32> padding1;
	be_t<u32> pkg_key_id;                       // Id of the AES key used for decryption. PSP = 0x1, PSVita = 0xC0000002, PSM = 0xC0000004
	be_t<u32> full_header_hmac_offset;          // ex: none (old pkg): 0, 0x930
	u8 padding2[20];
};

struct PKGEntry
{
	be_t<u32> name_offset;  // File name offset
	be_t<u32> name_size;    // File name size
	be_t<u64> file_offset;  // File offset
	be_t<u64> file_size;    // File size
	be_t<u32> type;         // File type
	be_t<u32> pad;          // Padding (zeros)
};

// https://www.psdevwiki.com/ps3/PKG_files#PKG_Metadata
struct PKGMetaData
{
private:
	static std::string to_hex_string(u8 buf[], usz size)
	{
		std::stringstream sstream;
		for (usz i = 0; i < size; i++)
		{
			sstream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(buf[i]);
		}
		return sstream.str();
	}
	static std::string to_hex_string(u8 buf[], usz size, usz dotpos)
	{
		std::string result = to_hex_string(buf, size);
		if (result.size() > dotpos)
		{
			result.insert(dotpos, 1, '.');
		}
		return result;
	}
public:
	be_t<u32> drm_type{ 0 };
	be_t<u32> content_type{ 0 };
	be_t<u32> package_type{ 0 };
	be_t<u64> package_size{ 0 };
	u8 qa_digest[24]{ 0 };

	be_t<u64> unk_0x9{ 0 };
	be_t<u64> unk_0xB{ 0 };

	struct package_revision
	{
		struct package_revision_data
		{
			u8 make_package_npdrm_ver[2]{ 0 };
			u8 version[2]{ 0 };
		} data{};

		std::string make_package_npdrm_ver;
		std::string version;

		void interpret_data()
		{
			make_package_npdrm_ver = to_hex_string(data.make_package_npdrm_ver, sizeof(data.make_package_npdrm_ver));
			version = to_hex_string(data.version, sizeof(data.version), 2);
		}
		std::string to_string() const
		{
			return fmt::format("make package npdrm version: %s, version: %s", make_package_npdrm_ver, version);
		}
	} package_revision;

	struct software_revision
	{
		struct software_revision_data
		{
			u8 unk[1]{ 0 };
			u8 firmware_version[3]{ 0 };
			u8 version[2]{ 0 };
			u8 app_version[2]{ 0 };
		} data{};

		std::string unk; // maybe hardware id
		std::string firmware_version;
		std::string version;
		std::string app_version;

		void interpret_data()
		{
			unk = to_hex_string(data.unk, sizeof(data.unk));
			firmware_version = to_hex_string(data.firmware_version, sizeof(data.firmware_version), 2);
			version = to_hex_string(data.version, sizeof(data.version), 2);
			app_version = to_hex_string(data.app_version, sizeof(data.app_version), 2);
		}
		std::string to_string() const
		{
			return fmt::format("unk: %s, firmware version: %s, version: %s, app version: %s",
				unk, firmware_version, version, app_version);
		}
	} software_revision;

	std::string title_id;
	std::string install_dir;

	// PSVita stuff

	struct vita_item_info // size is 0x28 (40)
	{
		be_t<u32> offset{ 0 };
		be_t<u32> size{ 0 };
		u8 sha256[32]{ 0 };

		std::string to_string() const
		{
			return fmt::format("offset: 0x%x, size: 0x%x, sha256: 0x%x", offset, size, sha256);
		}
	} item_info;

	struct vita_sfo_info // size is 0x38 (56)
	{
		be_t<u32> param_offset{ 0 };
		be_t<u16> param_size{ 0 };
		be_t<u32> unk_1{ 0 };           // seen values: 0x00000001-0x00000018, 0x0000001b-0x0000001c
		be_t<u32> psp2_system_ver{ 0 }; // BCD encoded
		u8 unk_2[8]{ 0 };
		u8 param_digest[32]{ 0 };       // SHA256 of param_data. Called ParamDigest: This is sha256 digest of param.sfo.

		std::string to_string() const
		{
			return fmt::format("param_offset: 0x%x, param_size: 0x%x, unk_1: 0x%x, psp2_system_ver: 0x%x, unk_2: 0x%x, param_digest: 0x%x",
				param_offset, param_size, unk_1, psp2_system_ver, unk_2, param_digest);
		}
	} sfo_info;

	struct vita_unknown_data_info // size is 0x48 (72)
	{
		be_t<u32> unknown_data_offset{ 0 };
		be_t<u16> unknown_data_size{ 0 };   // ex: 0x320
		u8 unk[32]{ 0 };
		u8 unknown_data_sha256[32]{ 0 };

		std::string to_string() const
		{
			return fmt::format("unknown_data_offset: 0x%x, unknown_data_size: 0x%x, unk: 0x%x, unknown_data_sha256: 0x%x",
				unknown_data_offset, unknown_data_size, unk, unknown_data_sha256);
		}
	} unknown_data_info;

	struct vita_entirety_info // size is 0x38 (56)
	{
		be_t<u32> entirety_data_offset{ 0 }; // located just before SFO
		be_t<u32> entirety_data_size{ 0 };   // ex: 0xA0, C0, 0x100, 0x120, 0x160
		be_t<u16> flags{ 0 };                // ex: EE 00, FE 10, FE 78, FE F8, FF 10, FF 90, FF D0, flags indicating which digests it embeds
		be_t<u16> unk_1{ 0 };                // always 00 00
		be_t<u32> unk_2{ 0 };                // ex: 1, 0
		u8 unk_3[8]{ 0 };
		u8 entirety_digest[32]{ 0 };

		std::string to_string() const
		{
			return fmt::format("entirety_data_offset: 0x%x, entirety_data_size: 0x%x, flags: 0x%x, unk_1: 0x%x, unk_2: 0x%x, unk_3: 0x%x, entirety_digest: 0x%x",
				entirety_data_offset, entirety_data_size, flags, unk_1, unk_2, unk_3, entirety_digest);
		}
	} entirety_info;

	struct vita_version_info // size is 0x28 (40)
	{
		be_t<u32> publishing_tools_version{ 0 };
		be_t<u32> psf_builder_version{ 0 };
		u8 padding[32]{ 0 };

		std::string to_string() const
		{
			return fmt::format("publishing_tools_version: 0x%x, psf_builder_version: 0x%x, padding: 0x%x",
				publishing_tools_version, psf_builder_version, padding);
		}
	} version_info;

	struct vita_self_info // size is 0x38 (56)
	{
		be_t<u32> self_info_offset{ 0 }; // offset to the first self_info_data_element
		be_t<u32> self_info_size{ 0 };   // usually 0x10 or 0x20
		u8 unk[16]{ 0 };
		u8 self_sha256[32]{ 0 };

		std::string to_string() const
		{
			return fmt::format("self_info_offset: 0x%x, self_info_size: 0x%x, unk: 0x%x, self_sha256: 0x%x",
				self_info_offset, self_info_size, unk, self_sha256);
		}
	} self_info;
};

struct package_install_result
{
	enum class error_type
	{
		no_error,
		app_version,
		other
	} error = error_type::no_error;
	struct version
	{
		std::string expected;
		std::string found;
	} version;
};

class package_reader
{
	struct install_entry
	{
		typename std::map<std::string, install_entry*>::value_type* weak_reference{};
		std::string name;

		u64 file_offset{};
		u64 file_size{};
		u32 type{};
		u32 pad{};

		// Check if the entry is the same one registered in entries to install
		bool is_dominating() const
		{
			return weak_reference->second == this;
		}
	};

public:
	package_reader(const std::string& path, fs::file file = {});
	~package_reader();

	enum result
	{
		not_started,
		started,
		success,
		aborted,
		aborted_dirty,
		error,
		error_dirty
	};

	bool is_valid() const { return m_is_valid; }
	package_install_result check_target_app_version() const;
	static package_install_result extract_data(std::deque<package_reader>& readers, std::deque<std::string>& bootable_paths);
	const psf::registry& get_psf() const { return m_psf; }
	result get_result() const { return m_result; };

	int get_progress(int maximum = 100) const;

	void abort_extract();

	fs::file &file()
	{
		return m_file;
	}

private:
	bool read_header();
	bool read_metadata();
	bool read_param_sfo();
	bool set_decryption_key();
	bool read_entries(std::vector<PKGEntry>& entries);
	void archive_seek(s64 new_offset, const fs::seek_mode damode = fs::seek_set);
	u64 archive_read(void* data_ptr, u64 num_bytes);
	bool set_install_path();
	bool fill_data(std::map<std::string, install_entry*>& all_install_entries);
	std::span<const char> archive_read_block(u64 offset, void* data_ptr, u64 num_bytes);
	usz decrypt(u64 offset, u64 size, const uchar* key, void* local_buf);
	void extract_worker();

	std::deque<install_entry> m_install_entries;
	std::string m_install_path;
	atomic_t<bool> m_aborted = false;
	atomic_t<usz> m_num_failures = 0;
	atomic_t<usz> m_entry_indexer = 0;
	atomic_t<usz> m_written_bytes = 0;
	bool m_was_null = false;

	static constexpr usz BUF_SIZE = 8192 * 1024; // 8 MB
	static constexpr usz BUF_PADDING = 32;

	bool m_is_valid = false;
	result m_result = result::not_started;

	std::string m_path{};
	std::string m_install_dir{};
	fs::file m_file{};
	std::array<uchar, 16> m_dec_key{};

	PKGHeader m_header{};
	PKGMetaData m_metadata{};
	psf::registry m_psf{};

	// Expose bootable file installed (if installed such)
	std::string m_bootable_file_path;
};
