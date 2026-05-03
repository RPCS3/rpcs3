#pragma once

#include "PSF.h"

#include "Utilities/File.h"
#include "util/types.hpp"
#include "Crypto/aes.h"

bool is_iso_file(const std::string& path, u64* size = nullptr, bool* is_raw_device = nullptr);

void load_iso(const std::string& path);
void unload_iso();

constexpr u64 ISO_SECTOR_SIZE = 2048;

/*
- Hijacked the "iso_archive::iso_archive" method to test if the ".iso" file is encrypted and sets a flag.
  The flag is set according to the first matching encryption type found following the order below:
  - Redump: ".dkey" or ".key" (as alternative) file, with the same name of the ".iso" file,
            exists in the same folder of the ".iso" file
  - 3k3y:   3k3y watermark exists at offset 0xF70
  If the flag is set then the "iso_file::read" method will decrypt the data on the fly

- Supported ISO encryption type:
  - Decrypted (.iso)
  - 3k3y (decrypted / encrypted) (.iso)
  - Redump (encrypted) (.iso + .dkey / .key)

- Unsupported ISO encryption type:
  - Encrypted split ISO files
*/

// Struct to store ISO region information (storing addresses instead of LBA since we need to compare
// the address anyway, so would have to multiply or divide every read if storing LBA)
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

// Enum returned by checking type
enum class iso_type_status
{
	NOT_ISO,
	REDUMP_ISO,
	ERROR_OPENING_KEY,
	ERROR_PROCESSING_KEY
};

class iso_archive;

// ISO file decryption class
class iso_file_decryption
{
private:
	aes_context m_aes_dec;
	iso_encryption_type m_enc_type = iso_encryption_type::NONE;
	std::vector<iso_region_info> m_region_info;

	static iso_type_status get_key(const std::string& key_path, aes_context* aes_ctx = nullptr);
	static iso_type_status retrieve_key(iso_archive& archive, std::string& key_path, aes_context& aes_ctx);

public:
	static iso_type_status check_type(const std::string& path, std::string& key_path, aes_context* aes_ctx = nullptr);

	iso_encryption_type get_enc_type() const { return m_enc_type; }

	bool init(const std::string& path, iso_archive* archive = nullptr);
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
protected:
	fs::file m_file;
	iso_fs_metadata m_meta;
	bool m_raw_device = false;
	u64 m_pos = 0;

	std::pair<u64, iso_extent_info> get_extent_pos(u64 pos) const;
	u64 local_extent_remaining(u64 pos) const;
	u64 local_extent_size(u64 pos) const;
	u64 file_offset(u64 pos) const;

public:
	iso_file(const std::string& path, bs_t<fs::open_mode> mode);
	iso_file(const std::string& path, bs_t<fs::open_mode> mode, const iso_fs_node& node);

	explicit operator bool() const { return m_file.operator bool(); }

	fs::stat_t get_stat() override;
	bool trunc(u64 length) override;
	u64 read(void* buffer, u64 size) override;
	u64 read_at(u64 offset, void* buffer, u64 size) override;
	u64 write(const void* buffer, u64 size) override;
	u64 seek(s64 offset, fs::seek_mode whence) override;
	u64 size() override;

	void release() override;

	friend class iso_file_decryption;
};

class iso_file_encrypted : public iso_file
{
private:
	std::shared_ptr<iso_file_decryption> m_dec;

public:
	iso_file_encrypted(const std::string& path, bs_t<fs::open_mode> mode, const iso_fs_node& node, std::shared_ptr<iso_file_decryption> dec);

	u64 read_at(u64 offset, void* buffer, u64 size) override;
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
	iso_fs_node m_root {};
	std::shared_ptr<iso_file_decryption> m_dec;

public:
	iso_archive(const std::string& path);

	const std::string& path() const { return m_path; }
	const iso_fs_node& root() const { return m_root; }
	iso_fs_node* retrieve(const std::string& path);
	bool exists(const std::string& path);
	bool is_file(const std::string& path);

	std::unique_ptr<fs::file_base> get_iso_file(const std::string& path, bs_t<fs::open_mode> mode, const iso_fs_node& node);
	std::unique_ptr<fs::file_base> open(const std::string& path);
	psf::registry open_psf(const std::string& path);

	friend class iso_file;
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
