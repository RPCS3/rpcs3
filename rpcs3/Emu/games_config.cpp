#include "stdafx.h"
#include "games_config.h"
#include "util/logs.hpp"
#include "util/yaml.hpp"
#include "Utilities/File.h"

LOG_CHANNEL(cfg_log, "CFG");

games_config::games_config()
{
	load();
}

games_config::~games_config()
{
	if (m_dirty)
	{
		save();
	}
}

const std::map<std::string, std::string> games_config::get_games() const
{
	std::lock_guard lock(m_mutex);
	return m_games;
}

std::string games_config::get_path(const std::string& title_id) const
{
	if (title_id.empty())
	{
		return {};
	}

	std::lock_guard lock(m_mutex);

	if (const auto it = m_games.find(title_id); it != m_games.cend())
	{
		return it->second;
	}

	return {};
}

bool games_config::add_game(const std::string& key, const std::string& path)
{
	std::lock_guard lock(m_mutex);

	// Access or create node if does not exist
	if (auto it = m_games.find(key); it != m_games.end())
	{
		if (it->second == path)
		{
			// Nothing to do
			return true;
		}

		it->second = path;
	}
	else
	{
		m_games.emplace(key, path);
	}

	m_dirty = true;

	if (m_save_on_dirty)
	{
		return save_nl();
	}

	return true;
}

bool games_config::add_external_hdd_game(const std::string& key, std::string& path)
{
	// Don't use the C00 subdirectory in our game list
	if (path.ends_with("/C00") || path.ends_with("\\C00"))
	{
		path = path.substr(0, path.size() - 4);
	}

	if (add_game(key, path))
	{
		cfg_log.notice("Registered HG game directory for title '%s': %s", key, path);
		return true;
	}

	cfg_log.error("Failed to save HG game location of title '%s' (error=%s)", key, fs::g_tls_error);
	return false;
}

bool games_config::save_nl()
{
	YAML::Emitter out;
	out << m_games;

	fs::pending_file temp(fs::get_config_dir() + "/games.yml");

	if (temp.file && temp.file.write(out.c_str(), out.size()), temp.commit())
	{
		m_dirty = false;
		return true;
	}

	cfg_log.error("Failed to save games.yml: %s", fs::g_tls_error);
	return false;
}

bool games_config::save()
{
	std::lock_guard lock(m_mutex);
	return save_nl();
}

void games_config::load()
{
	std::lock_guard lock(m_mutex);

	m_games.clear();

	if (fs::file f{fs::get_config_dir() + "/games.yml", fs::read + fs::create})
	{
		auto [result, error] = yaml_load(f.to_string());

		if (!error.empty())
		{
			cfg_log.error("Failed to load games.yml: %s", error);
		}

		if (!result.IsMap())
		{
			if (!result.IsNull())
			{
				cfg_log.error("Failed to load games.yml: type %d not a map", result.Type());
			}
			return;
		}

		for (const auto& entry : result)
		{
			if (!entry.first.Scalar().empty() && entry.second.IsScalar() && !entry.second.Scalar().empty())
			{
				m_games.emplace(entry.first.Scalar(), entry.second.Scalar());
			}
		}
	}
}
