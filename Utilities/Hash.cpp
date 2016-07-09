#include "stdafx.h"
#include "Hash.h"
#include <sstream>
#include <iomanip>

#include "../rpcs3/Crypto/sha1.h"
#include <iostream>
namespace hash
{

std::string SHA1(const std::string& str)
{
	int len = str.size();

	unsigned char hash[20];

	std::unique_ptr<unsigned char[]> input(new unsigned char[len+1]);
	::memcpy(input.get(), str.c_str(), len);

	sha1(input.get(), len, hash);

	std::stringstream res;
	res << std::hex;
	for (int i = 0; i < 20; ++i)
	{
		res << ((hash[i] & 0x000000F0) >> 4)
		    << (hash[i] & 0x0000000F);
	}

	return res.str();
}

std::string GetPathHashWithFilename(const std::string& file_path, std::size_t len)
{
	std::size_t path_end_pos = file_path.find_last_of('/');

	// no path detected, hash filename itself instead
	if (path_end_pos == std::string::npos)
	{
		std::size_t str_size = file_path.size();

		if (len != 0)
		{
			if (str_size < len)
			{
				len = str_size;
			}
		}

		return SHA1(file_path.substr(str_size - len)) + '-' + file_path;
	}

	// filename found, extract path for hashing
	else
	{
		std::string path = file_path.substr(0, path_end_pos);
		std::string file;

		if (path_end_pos < file_path.size())
		{
			file = file_path.substr(path_end_pos + 1);
		}

		else
		{
			file = file_path.substr(path_end_pos);
		}

		std::size_t str_size = path.size();

		// only path was given, set filename to unknown
		if (file.empty())
		{
			file = "UNKNOWN";
		}

		if (len != 0)
		{
			if (str_size < len)
			{
				len = str_size;
			}
		}

		return SHA1(path.substr(str_size - len)) + '-' + file;
	}
}

}
