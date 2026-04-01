#include "stdafx.h"

#include "iso_cache.h"
#include "Loader/PSF.h"
#include "util/yaml.hpp"
#include "util/fnv_hash.hpp"
#include "Utilities/File.h"

#include <fmt/core.h>

LOG_CHANNEL(iso_cache_log, "ISOCache");

namespace
{
	std::string get_cache_dir()
	{
		const std::string dir = fs::get_config_dir() + "iso_cache/";
		fs::create_path(dir);
		return dir;
	}

	// FNV-64 hash of the ISO path used as the cache filename stem.
	std::string get_cache_stem(const std::string& iso_path)
	{
		usz hash = rpcs3::fnv_seed;
		for (const char c : iso_path)
		{
			hash ^= static_cast<u8>(c);
			hash *= rpcs3::fnv_prime;
		}
		return fmt::format("{:016x}", hash);
	}
}

namespace iso_cache
{
	bool load(const std::string& iso_path, iso_metadata_cache_entry& out_entry)
	{
		fs::stat_t iso_stat{};
		if (!fs::get_stat(iso_path, iso_stat))
		{
			return false;
		}

		const std::string stem     = get_cache_stem(iso_path);
		const std::string dir      = get_cache_dir();
		const std::string yml_path = dir + stem + ".yml";
		const std::string sfo_path = dir + stem + ".sfo";
		const std::string png_path = dir + stem + ".png";

		const fs::file yml_file(yml_path);
		if (!yml_file)
		{
			return false;
		}

		auto [node, error] = yaml_load(yml_file.to_string());
		if (!error.empty())
		{
			iso_cache_log.warning("Failed to parse cache YAML for '%s': %s", iso_path, error);
			return false;
		}

		// Reject stale entries.
		const s64 cached_mtime = node["mtime"].as<s64>(0);
		if (cached_mtime != iso_stat.mtime)
		{
			return false;
		}

		const fs::file sfo_file(sfo_path);
		if (!sfo_file)
		{
			return false;
		}

		out_entry.mtime      = cached_mtime;
		out_entry.psf_data   = sfo_file.to_vector<u8>();
		out_entry.icon_path  = node["icon_path"].as<std::string>("");
		out_entry.movie_path = node["movie_path"].as<std::string>("");
		out_entry.audio_path = node["audio_path"].as<std::string>("");

		// Icon bytes are optional — game may have no icon.
		if (const fs::file png_file(png_path); png_file)
		{
			out_entry.icon_data = png_file.to_vector<u8>();
		}

		return true;
	}

	void save(const std::string& iso_path, const iso_metadata_cache_entry& entry)
	{
		const std::string stem     = get_cache_stem(iso_path);
		const std::string dir      = get_cache_dir();
		const std::string yml_path = dir + stem + ".yml";
		const std::string sfo_path = dir + stem + ".sfo";
		const std::string png_path = dir + stem + ".png";

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "mtime"      << YAML::Value << entry.mtime;
		out << YAML::Key << "icon_path"  << YAML::Value << entry.icon_path;
		out << YAML::Key << "movie_path" << YAML::Value << entry.movie_path;
		out << YAML::Key << "audio_path" << YAML::Value << entry.audio_path;
		out << YAML::EndMap;

		if (fs::file yml_file(yml_path, fs::rewrite); yml_file)
		{
			yml_file.write(std::string_view(out.c_str(), out.size()));
		}
		else
		{
			iso_cache_log.warning("Failed to write cache YAML for '%s'", iso_path);
		}

		if (!entry.psf_data.empty())
		{
			if (fs::file sfo_file(sfo_path, fs::rewrite); sfo_file)
			{
				sfo_file.write(entry.psf_data);
			}
		}

		if (!entry.icon_data.empty())
		{
			if (fs::file png_file(png_path, fs::rewrite); png_file)
			{
				png_file.write(entry.icon_data);
			}
		}
	}
}