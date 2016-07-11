#pragma once

namespace hash
{
	std::string SHA1(const std::string& str);

	// Get a unique path hash in SHA-1 format with the original filename appended.
	// len defines the length from the end to the beginning of the path which should be hashed.
	// A length of zero means hash the whole file path.
	std::string GetPathHashWithFilename(const std::string& file_path, std::size_t len = 0);
}
