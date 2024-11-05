#pragma once

#include "util/types.hpp"

#include <map>

struct TARHeader
{
	char name[100];
	char dontcare[24];
	char size[12];
	char mtime[12];
	char chksum[8];
	char filetype;
	char linkname[100];
	char magic[6];
	char dontcare2[82];
	char prefix[155];
	char padding[12]; // atime for RPCS3

	ENABLE_BITWISE_SERIALIZATION;
};

namespace fs
{
	class file;
	struct dir_entry;
}

namespace utils
{
	struct serial;
}

class tar_object
{
	const fs::file* m_file;
	utils::serial* m_ar;
	const usz m_ar_tar_start;

	usz largest_offset = 0; // We store the largest offset so we can continue to scan from there.
	std::map<std::string, std::pair<u64, TARHeader>> m_map{}; // Maps path to offset of file data and its header

	TARHeader read_header(u64 offset) const;

public:
	tar_object(const fs::file& file);
	tar_object(utils::serial& ar);

	std::vector<std::string> get_filenames();

	std::unique_ptr<utils::serial> get_file(const std::string& path, std::string* new_file_path = nullptr);

	using process_func = std::function<bool(const fs::file&, std::string&, utils::serial&)>;

	// Extract all files in archive to destination (as VFS if is_vfs is true)
	// Allow to optionally specify explicit mount point (which may be directory meant for extraction)
	bool extract(const std::string& prefix_path = {}, bool is_vfs = false);

	static void save_directory(const std::string& src_dir, utils::serial& ar, const process_func& func = {}, std::vector<fs::dir_entry>&& = std::vector<fs::dir_entry>{}, bool has_evaluated_results = false, usz src_dir_pos = umax);
};

bool extract_tar(const std::string& file_path, const std::string& dir_path, fs::file file = {});
