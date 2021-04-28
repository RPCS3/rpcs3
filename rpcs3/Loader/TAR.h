#pragma once

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
	char padding[12];
};

class tar_object
{
	const fs::file& m_file;

	usz largest_offset = 0; // We store the largest offset so we can continue to scan from there.
	std::map<std::string, std::pair<u64, TARHeader>> m_map{}; // Maps path to offset of file data and its header

	TARHeader read_header(u64 offset) const;

public:
	tar_object(const fs::file& file);

	std::vector<std::string> get_filenames();

	fs::file get_file(const std::string& path);

	// Extract all files in archive to destination as VFS
	// Allow to optionally specify explicit mount point (which may be directory meant for extraction)
	bool extract(std::string vfs_mp = {});
};

bool extract_tar(const std::string& file_path, const std::string& dir_path);
