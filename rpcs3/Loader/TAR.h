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

	int initial_offset;
	int largest_offset; //we store the largest offset so we can continue to scan from there.
	std::map<std::string, u64> m_map; //maps path to offset of header of that file, so we only need to scan the entire file once.

	TARHeader read_header(u64 offset);

public:
	tar_object(const fs::file& file, usz offset = 0);

	std::vector<std::string> get_filenames();

	fs::file get_file(std::string path);

	bool extract(std::string path, std::string ignore = ""); // extract all files in archive to path
};
