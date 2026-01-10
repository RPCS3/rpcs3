#pragma once

#include "Loader/PSF.h"

#include "Utilities/File.h"
#include "util/types.hpp"

bool is_file_iso(const std::string& path);
bool is_file_iso(const fs::file& path);

void load_iso(const std::string& path);
void unload_iso();

struct iso_extent_info
{
	u64 start;
	u64 size;
};

struct iso_fs_metadata
{
	std::string name;
	s64 time;
	bool is_directory;
	bool has_multiple_extents;
	std::vector<iso_extent_info> extents;

	u64 size() const;
};

struct iso_fs_node
{
	iso_fs_metadata metadata;
	std::vector<std::unique_ptr<iso_fs_node>> children;
};

class iso_file : public fs::file_base
{
private:
	fs::file m_file;
	iso_fs_metadata m_meta;
	u64 m_pos = 0;

	std::pair<u64, iso_extent_info> get_extent_pos(u64 pos) const;
	u64 file_offset(u64 pos) const;
	u64 local_extent_remaining(u64 pos) const;
	u64 local_extent_size(u64 pos) const;

public:
	iso_file(fs::file&& iso_handle, const iso_fs_node& node);

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

// represents the .iso file itself.
class iso_archive
{
private:
	std::string m_path;
	iso_fs_node m_root;
	fs::file m_file;

public:
	iso_archive(const std::string& path);

	iso_fs_node* retrieve(const std::string& path);
	bool exists(const std::string& path);
	bool is_file(const std::string& path);

	iso_file open(const std::string& path);

	psf::registry open_psf(const std::string& path);
};

class iso_device : public fs::device_base
{
private:
	iso_archive m_archive;
	std::string iso_path;

public:
	inline static std::string virtual_device_name = "/vfsv0_virtual_iso_overlay_fs_dev";

	iso_device(const std::string& iso_path, const std::string& device_name = virtual_device_name)
		: m_archive(iso_path), iso_path(iso_path)
	{
		fs_prefix = device_name;
	}
	~iso_device() override = default;

	const std::string& get_loaded_iso() const { return iso_path; }

	bool stat(const std::string& path, fs::stat_t& info) override;
	bool statfs(const std::string& path, fs::device_stat& info) override;

	std::unique_ptr<fs::file_base> open(const std::string& path, bs_t<fs::open_mode> mode) override;
	std::unique_ptr<fs::dir_base> open_dir(const std::string& path) override;
};
