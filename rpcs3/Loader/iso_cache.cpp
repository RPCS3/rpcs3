#include "stdafx.h"

#include "iso_cache.h"
#include "Loader/PSF.h"
#include "util/yaml.hpp"
#include "util/fnv_hash.hpp"
#include "Utilities/File.h"

#include <unordered_set>

LOG_CHANNEL(iso_cache_log, "ISOCache");

namespace
{
	std::string get_cache_dir()
	{
		const std::string dir = fs::get_cache_dir() + "cache/iso_cache/";
		fs::create_path(dir);
		return dir;
	}

	// FNV-64 hash of the given key used as the cache filename stem.
	std::string get_cache_stem(std::string_view key)
	{
		usz hash = rpcs3::fnv_seed;
		for (const char c : key)
		{
			hash ^= static_cast<u8>(c);
			hash *= rpcs3::fnv_prime;
		}
		return fmt::format("%016llx", hash);
	}

	// Separate stem for the per-ISO subdir index entry.
	std::string get_index_stem(const std::string& iso_path)
	{
		return get_cache_stem(iso_path + "//index");
	}
}

namespace iso_cache
{
	bool load(const std::string& iso_path, const std::string& cache_key, iso_metadata_cache_entry& out_entry)
	{
		fs::stat_t iso_stat{};
		if (!fs::get_stat(iso_path, iso_stat) || iso_stat.is_directory)
		{
			return false;
		}

		const std::string stem     = get_cache_stem(cache_key);
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

	void save(const std::string& iso_path, const std::string& cache_key, const iso_metadata_cache_entry& entry)
	{
		const std::string stem     = get_cache_stem(cache_key);
		const std::string dir      = get_cache_dir();
		const std::string yml_path = dir + stem + ".yml";
		const std::string sfo_path = dir + stem + ".sfo";
		const std::string png_path = dir + stem + ".png";

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "mtime"      << YAML::Value << static_cast<long long>(entry.mtime);
		out << YAML::Key << "icon_path"  << YAML::Value << entry.icon_path;
		out << YAML::Key << "movie_path" << YAML::Value << entry.movie_path;
		out << YAML::Key << "audio_path" << YAML::Value << entry.audio_path;
		out << YAML::EndMap;

		if (fs::pending_file yml_file(yml_path); yml_file.file)
		{
			yml_file.file.write(out.c_str(), out.size());
			yml_file.commit();
		}
		else
		{
			iso_cache_log.warning("Failed to write cache YAML for '%s'", iso_path);
		}

		if (!entry.psf_data.empty())
		{
			if (fs::pending_file sfo_file(sfo_path); sfo_file.file)
			{
				sfo_file.file.write(entry.psf_data);
				sfo_file.commit();
			}
		}

		if (!entry.icon_data.empty())
		{
			if (fs::pending_file png_file(png_path); png_file.file)
			{
				png_file.file.write(entry.icon_data);
				png_file.commit();
			}
		}
	}

	bool load_index(const std::string& iso_path, std::vector<std::string>& out_subdirs)
	{
		fs::stat_t iso_stat{};
		if (!fs::get_stat(iso_path, iso_stat) || iso_stat.is_directory)
		{
			return false;
		}

		const std::string dir      = get_cache_dir();
		const std::string yml_path = dir + get_index_stem(iso_path) + ".yml";

		const fs::file yml_file(yml_path);
		if (!yml_file)
		{
			return false;
		}

		const auto [node, error] = yaml_load(yml_file.to_string());
		if (!error.empty())
		{
			iso_cache_log.warning("Failed to parse index YAML for '%s': %s", iso_path, error);
			return false;
		}

		const s64 cached_mtime = node["mtime"].as<s64>(0);
		if (cached_mtime != iso_stat.mtime)
		{
			return false;
		}

		const YAML::Node subdirs_node = node["subdirs"];
		if (!subdirs_node || !subdirs_node.IsSequence())
		{
			return false;
		}

		for (const auto& entry : subdirs_node)
		{
			std::string name = entry.as<std::string>("");
			if (!name.empty())
			{
				out_subdirs.push_back(std::move(name));
			}
		}

		return !out_subdirs.empty();
	}

	void save_index(const std::string& iso_path, const std::vector<std::string>& subdirs)
	{
		fs::stat_t iso_stat{};
		if (!fs::get_stat(iso_path, iso_stat))
		{
			return;
		}
		const std::string dir      = get_cache_dir();
		const std::string yml_path = dir + get_index_stem(iso_path) + ".yml";

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "mtime"   << YAML::Value << static_cast<long long>(iso_stat.mtime);
		out << YAML::Key << "subdirs" << YAML::Value << YAML::BeginSeq;
		for (const std::string& s : subdirs)
		{
			out << s;
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;

		if (fs::pending_file yml_file(yml_path); yml_file.file)
		{
			yml_file.file.write(out.c_str(), out.size());
			yml_file.commit();
		}
		else
		{
			iso_cache_log.warning("Failed to write index YAML for '%s'", iso_path);
		}
	}

	void cleanup(const std::unordered_set<std::string>& valid_iso_paths)
	{
		const std::string dir = get_cache_dir();

		// Build a set of stems that should exist, including index entries.
		std::unordered_set<std::string> valid_stems;
		for (const std::string& path : valid_iso_paths)
		{
			valid_stems.insert(get_cache_stem(path));
			valid_stems.insert(get_index_stem(path));
		}

		// Delete any cache files whose stem is not in the valid set.
		fs::dir cache_dir(dir);
		fs::dir_entry entry{};
		while (cache_dir.read(entry))
		{
			if (entry.name == "." || entry.name == "..") continue;

			const std::string stem = entry.name.substr(0, entry.name.find_last_of('.'));
			if (valid_stems.find(stem) == valid_stems.end())
			{
				fs::remove_file(dir + entry.name);
			}
		}
	}
}
