#pragma once

#include "Utilities/File.h"
#include "util/types.hpp"

#include <functional>
#include <memory>
#include <string>

class ZArchiveReader;

enum class zar_disc_layout
{
	iso,
	jb,
};

struct zar_dir_entry
{
	std::string name;
	u32 node = 0xffffffffu;
	bool is_file = false;
	bool is_directory = false;
	u64 size = 0;
};

class zar_disc_container
{
public:
	static std::shared_ptr<zar_disc_container> open(const std::string& path, std::string* error = nullptr);
	static bool has_zar_extension(const std::string& path);

	const std::string& source_path() const { return m_source_path; }
	zar_disc_layout layout() const { return m_layout; }
	bool is_iso_layout() const { return m_layout == zar_disc_layout::iso; }
	bool is_jb_layout() const { return m_layout == zar_disc_layout::jb; }
	const std::string& iso_name() const { return m_iso_name; }
	const std::string& key_name() const { return m_key_name; }
	s64 source_mtime() const { return m_source_mtime; }

	fs::file open_iso() const;
	fs::file open_key() const;
	fs::file open_file(u32 node, const std::string& display_name) const;
	fs::file open_file(const std::string& path) const;
	std::unique_ptr<fs::dir_base> open_dir(const std::string& path) const;
	bool stat(const std::string& path, fs::stat_t& info) const;
	u64 iso_size() const;
	u64 file_size(u32 node) const;

	u32 lookup(const std::string& path, bool allow_file = true, bool allow_directory = true) const;
	bool is_file(u32 node) const;
	bool is_directory(u32 node) const;
	u32 dir_entry_count(u32 node) const;
	bool dir_entry(u32 node, u32 index, zar_dir_entry& entry) const;

private:
	zar_disc_container(std::string source_path, std::shared_ptr<ZArchiveReader> reader, zar_disc_layout layout,
		u32 iso_node, u32 key_node, std::string iso_name, std::string key_name, s64 source_mtime);

	std::string m_source_path;
	std::shared_ptr<ZArchiveReader> m_reader;
	zar_disc_layout m_layout = zar_disc_layout::iso;
	u32 m_iso_node = 0xffffffffu;
	u32 m_key_node = 0xffffffffu;
	std::string m_iso_name;
	std::string m_key_name;
	s64 m_source_mtime = 0;
};


enum class zar_compression_status
{
	success,
	cancelled,
	invalid_source,
	invalid_key,
	output_exists,
	io_error,
	archive_error,
};

struct zar_compression_result
{
	zar_compression_status status = zar_compression_status::archive_error;
	std::string error;
	std::string embedded_key;
	u64 input_size = 0;
	u64 output_size = 0;

	explicit operator bool() const { return status == zar_compression_status::success; }
};

// Return false from the callback to cancel compression
using zar_compression_progress_cb = std::function<bool(u64 processed, u64 total, const std::string& current_file)>;

// Creates disc ZArchive from either a JB folder or a ISO image.
// ISO archives use disc.iso and, when a matching key is found, disc.dkey/disc.key
zar_compression_result compress_ps3_disc_to_zar(const std::string& source_path, const std::string& output_path,
	const zar_compression_progress_cb& progress = {});
