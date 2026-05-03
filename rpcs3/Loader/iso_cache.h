#pragma once

#include "Loader/PSF.h"
#include "Utilities/File.h"
#include "util/types.hpp"

#include <string>
#include <unordered_set>
#include <vector>

// Cached metadata extracted from an ISO during game list scanning.
struct iso_metadata_cache_entry
{
	s64 mtime = 0;
	std::vector<u8> psf_data{};
	std::string icon_path{};
	std::vector<u8> icon_data{};
	std::string movie_path{};
	std::string audio_path{};
};

namespace iso_cache
{
	// Returns false if no valid cache entry exists or mtime has changed.
	bool load(const std::string& iso_path, const std::string& cache_key, iso_metadata_cache_entry& out_entry);

	// Persists a populated cache entry to disk.
	void save(const std::string& iso_path, const std::string& cache_key, const iso_metadata_cache_entry& entry);

	// Remove cache entries for ISOs that are no longer in the scanned set.
	void cleanup(const std::unordered_set<std::string>& valid_iso_paths);
}
