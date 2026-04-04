#pragma once

#include "Loader/PSF.h"

#include "Utilities/File.h"
#include "util/types.hpp"
#include "Crypto/aes.h"

bool is_file_iso(const std::string& path);
bool is_file_iso(const fs::file& path);

void load_iso(const std::string& path);
void unload_iso();

/*
- Hijacked the "iso_archive::iso_archive" method to test if the ".iso" file is encrypted and sets a flag.
  The flag is set according to the first matching encryption type found in the following order:
  - Redump type: ".dkey" file, with the same name of the ".iso" file, exists in the same folder of the ".iso" file
  - 3k3y type:   3k3y watermark exists at 0xF70
  If the flag is set then the "iso_file::read" method will decrypt the data on the fly

- Supported ISO encryption type:
  - Decrypted (.iso)
  - 3k3y (decrypted / encrypted) (.iso)
  - Redump (encrypted) (.iso & .dkey)

- Unsupported ISO encryption type:
  - Encrypted split ISO files
*/

// Struct to store ISO region information (storing addrs instead of lba since we need to compare the addr anyway,
// so would have to multiply or divide every read if storing lba)
struct iso_region_info
{
	bool encrypted = false;
	u64 region_first_addr = 0;
	u64 region_last_addr = 0;
};

// Enum to decide ISO encryption type
enum class iso_encryption_type
{
	NONE,
	DEC_3K3Y,
	ENC_3K3Y,
	REDUMP
};

// ISO file decryption class
class iso_file_decryption
{
private:
	aes_context m_aes_dec;
	iso_encryption_type m_enc_type = iso_encryption_type::NONE;
	std::vector<iso_region_info> m_region_info;

	void reset();

public:
	iso_encryption_type get_enc_type() const { return m_enc_type; }

	bool init(const std::string& path);
	bool decrypt(u64 offset, void* buffer, u64 size, const std::string& name);
};

struct iso_extent_info
{
	u64 start = 0;
	u64 size = 0;
};

struct iso_fs_metadata
{
	std::string name;
	s64 time = 0;
	bool is_directory = false;
	bool has_multiple_extents = false;
	std::vector<iso_extent_info> extents;

	u64 size() const;
};

struct iso_fs_node
{
	iso_fs_metadata metadata {};
	std::vector<std::unique_ptr<iso_fs_node>> children;
};

class iso_file : public fs::file_base
{
private:
	fs::file m_file;
	std::shared_ptr<iso_file_decryption> m_dec;
	iso_fs_metadata m_meta;
	u64 m_pos = 0;

	std::pair<u64, iso_extent_info> get_extent_pos(u64 pos) const;
	u64 local_extent_remaining(u64 pos) const;
	u64 local_extent_size(u64 pos) const;
	u64 file_offset(u64 pos) const;

public:
	iso_file(fs::file&& iso_handle, std::shared_ptr<iso_file_decryption> iso_dec, const iso_fs_node& node);

	fs::stat_t get_stat() override;
	bool trunc(u64 length) override;
	u64 read(void* buffer, u64 size) override;
	u64 read_at(u64 offset, void* buffer, u64 size) override;
	u64 write(const void* buffer, u64 size) override;
	u64 seek(s64 offset, fs::seek_mode whence) override;
	u64 size() override;

	void release() override;
};

class iso_dir : public fs::dir_base
{
private:
	const iso_fs_node& m_node;
	u64 m_pos = 0;

public:
	iso_dir(const iso_fs_node& node)
		: m_node(node)
	{}

	bool read(fs::dir_entry&) override;
	void rewind() override;
};

// Represents the .iso file itself
class iso_archive
{
private:
	std::string m_path;
	fs::file m_file;
	std::shared_ptr<iso_file_decryption> m_dec;
	iso_fs_node m_root {};

public:
	iso_archive(const std::string& path);

	const std::string& path() const { return m_path; }
	const std::shared_ptr<iso_file_decryption> get_dec() { return m_dec; }

	iso_fs_node* retrieve(const std::string& path);
	bool exists(const std::string& path);
	bool is_file(const std::string& path);

	iso_file open(const std::string& path);
	psf::registry open_psf(const std::string& path);
};

class iso_device : public fs::device_base
{
private:
	std::string m_path;
	iso_archive m_archive;

public:
	inline static std::string virtual_device_name = "/vfsv0_virtual_iso_overlay_fs_dev";

	iso_device(const std::string& iso_path, const std::string& device_name = virtual_device_name)
		: m_path(iso_path), m_archive(iso_path)
	{
		fs_prefix = device_name;
	}

	~iso_device() override = default;

	const std::string& get_loaded_iso() const { return m_path; }

	bool stat(const std::string& path, fs::stat_t& info) override;
	bool statfs(const std::string& path, fs::device_stat& info) override;

	std::unique_ptr<fs::file_base> open(const std::string& path, bs_t<fs::open_mode> mode) override;
	std::unique_ptr<fs::dir_base> open_dir(const std::string& path) override;
};
