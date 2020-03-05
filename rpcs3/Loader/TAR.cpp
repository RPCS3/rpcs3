#include "stdafx.h"

#include "TAR.h"

#include <cmath>
#include <cstdlib>

LOG_CHANNEL(tar_log, "TAR");

tar_object::tar_object(const fs::file& file, size_t offset)
	: m_file(file)
	, initial_offset(static_cast<int>(offset))
{
	m_file.seek(initial_offset);
	largest_offset = initial_offset;
}

TARHeader tar_object::read_header(u64 offset)
{
	m_file.seek(offset);
	TARHeader header;
	m_file.read(header);
	return header;
}

int octalToDecimal(int octalNumber)
{
	int decimalNumber = 0, i = 0, rem;
	while (octalNumber != 0)
	{
		rem = octalNumber % 10;
		octalNumber /= 10;
		decimalNumber += rem * (1 << (i * 3));
		++i;
	}
	return decimalNumber;
}

std::vector<std::string> tar_object::get_filenames()
{
	std::vector<std::string> vec;
	get_file("");
	for (auto it = m_map.cbegin(); it != m_map.cend(); ++it)
	{
		vec.push_back(it->first);
	}
	return vec;
}

fs::file tar_object::get_file(std::string path)
{
	if (!m_file) return fs::file();

	auto it = m_map.find(path);
	if (it != m_map.end())
	{
		TARHeader header = read_header(it->second);
		int size = octalToDecimal(atoi(header.size));
		std::vector<u8> buf(size);
		m_file.read(buf, size);
		int offset = ((m_file.pos() - initial_offset + 512 - 1) & ~(512 - 1)) + initial_offset; // Always keep the offset aligned to 512 bytes + the initial offset.
		m_file.seek(offset);
		return fs::make_stream(std::move(buf));
	}
	else //continue scanning from last file entered
	{
		while (m_file.pos() < m_file.size())
		{
			TARHeader header = read_header(largest_offset);

			if (std::string(header.magic).find("ustar") != umax)
				m_map[header.name] = largest_offset;

			int size = octalToDecimal(atoi(header.size));
			if (path == header.name) { //path is equal, read file and advance offset to start of next block
				std::vector<u8> buf(size);
				m_file.read(buf, size);
				int offset = ((m_file.pos() - initial_offset + 512 - 1) & ~(512 - 1)) + initial_offset;
				m_file.seek(offset);
				largest_offset = offset;

				return fs::make_stream(std::move(buf));
			}
			else { // just advance offset to next block
				m_file.seek(size, fs::seek_mode::seek_cur);
				int offset = ((m_file.pos() - initial_offset + 512 - 1) & ~(512 - 1)) + initial_offset;
				m_file.seek(offset);
				largest_offset = offset;
			}
		}

		return fs::file();
	}
}

bool tar_object::extract(std::string path, std::string ignore)
{
	if (!m_file) return false;

	get_file(""); //Make sure we have scanned all files
	for (auto iter : m_map)
	{
		TARHeader header = read_header(iter.second);
		if (!header.name[0]) continue;

		std::string result = path + header.name;

		if (result.compare(path.size(), ignore.size(), ignore) == 0)
		{
			result.erase(path.size(), ignore.size());
		}

		switch (header.filetype)
		{
		case '0':
		{
			fs::file file(result, fs::rewrite);
			file.write(get_file(header.name).to_vector<u8>());
			break;
		}

		case '5':
		{
			fs::create_dir(result);
			break;
		}

		default:
			tar_log.error("TAR Loader: unknown file type: 0x%x", header.filetype);
			return false;
		}
	}
	return true;
}
