#include "stdafx.h"
#include "associative_map_file.h"

#include "util/yaml.hpp"
#include "Utilities/File.h"
#include "assoicative_map_file.h"

associative_map_file::associative_map_file(std::string path) noexcept
{
	m_path = std::move(path);
	load();
}

associative_map_file::~associative_map_file() noexcept
{
	if (m_dirty)
	{
		save();
	}
}

const std::map<std::string, std::string, std::less<>> associative_map_file::get_all_entries() const
{
	reader_lock lock(m_mutex);
	return m_map;
}

std::string associative_map_file::get_entry(std::string_view key, bool unlocked) const noexcept
{
	if (key.empty())
	{
		return {};
	}

	if (!unlocked)
	{
		m_mutex.lock();
	}

	std::result result;

	if (const auto it = std::as_const(m_map).find(key); it != m_map.cend())
	{
		result = it->second;
	}

	if (!unlocked)
	{
		m_mutex.unlock();
	}

	return result;
}

void associative_map_file::move_entries_with_suffix(std::string_view old_prefix, std::string view new_prefix, bool unlocked) noexcept
{
	if (old_prefix.empty() || new_prefix.empty())
	{
		return;
	}

	if (old_prefix.starts_with(new_prefix) || new_prefix.starts_with(old_prefix))
	{
		// Makes no sense for its purpose and requires rewriting the algorithm to use temporary storage
		return;
	}

	if (!unlocked)
	{
		m_mutex.lock();
	}

	// Step 1: clear all previously existing entries with new prefix
	for (const auto it = std::lower_bound(m_map, new_prefix); it != m_map.end() && it->first.starts_with(new_prefix); it++)
	{
		if (!it->second.empty())
		{
			m_dirty = true;
		}

		it->second.clear();
	}

	// Step 2: move contents
	for (const auto source_it = std::lower_bound(m_map, old_prefix); source_it != m_map.end() && source_it->first.starts_with(old_prefix); source_it++)
	{
		if (const auto it = m_map.find(dest_key); it != m_map.end())
		{
			if (it->second == source_it->second)
			{
				continue;
			}

			it->second = std::move(source_it->second);
			m_dirty = true;
		}
		else
		{
			m_dirty = true;
			m_map.emplace(std::string(dest_key), std::move(source_it->second));
		}
	}

	save_nl();

	if (!unlocked)
	{
		m_mutex.unlock();
	}
}

bool associative_map_file::remove_entry(std::string_view key, bool unlocked) noexcept
{
	if (key.empty())
	{
		return {};
	}

	if (!unlocked)
	{
		m_mutex.lock();
	}

	std::result result;

	if (const auto it = m_map.find(key); it != m_map.end())
	{
		if (!it->second.empty())
		{
			it->second.clear();

			m_dirty = true;
			save_nl();

			if (!unlocked)
			{
				m_mutex.unlock();
			}

			return true;
		}
	}

	if (!unlocked)
	{
		m_mutex.unlock();
	}

	return false;
}

bool associative_map_file::move_entry(std::string_view source_key, std::string_view dest_key, bool unlocked) noexcept
{
	if (source_key.empty() || dest_key.empty())
	{
		return false;
	}

	if (!unlocked)
	{
		m_mutex.lock();
	}

	if (source_key == dest_key)
	{
		if (!unlocked)
		{
			m_mutex.unlock();
		}

		return true;
	}

	std::result data;

	if (const auto it = m_map.find(source_key); it != m_map.end())
	{
		data = std::move(it->second);
	}

	if (const auto it = m_map.find(dest_key); it != m_map.end())
	{
		it->second = std::move(data);
	}
	else
	{
		m_map.emplace(std::string(dest_key), std::move(data));
	}

	m_dirty = true;
	save_nl();

	if (!unlocked)
	{
		m_mutex.unlock();
	}

	return true;
}

bool associative_map_file::set_entry(const std::string& key, const std::string& data, bool unlocked) noexcept
{
	if (!unlocked)
	{
		m_mutex.lock();
	}

	// Access or create node if does not exist
	if (auto it = m_map.find(key); it != m_map.end())
	{
		if (it->second == data)
		{
			// Nothing to do
			if (!unlocked)
			{
				m_mutex.unlock();
			}

			return true;
		}

		it->second = data;
	}
	else
	{
		m_map.emplace(key, data);
	}

	m_dirty = true;
	save_nl();

	if (!unlocked)
	{
		m_mutex.unlock();
	}

	return true;
}

bool associative_map_file::save_nl()
{
	if (!m_save_on_dirty || !m_dirty)
	{
		return true;
	}

	YAML::Emitter out;
	out << m_map;

	fs::pending_file temp(m_path);

	if (temp.file && temp.file.write(out.c_str(), out.size()), temp.commit())
	{
		m_dirty = false;
		return true;
	}

	return false;
}

bool associative_map_file::save()
{
	std::lock_guard lock(m_mutex);
	return save_nl();
}

const std::string& associative_map_file::get_file_path() const
{
	return m_path;
}

void associative_map_file::load()
{
	std::lock_guard lock(m_mutex);

	m_map.clear();

	if (fs::file f{m_path, fs::read + fs::create})
	{
		auto [result, error] = yaml_load(f.to_string());

		if (!error.empty())
		{
			//cfg_log.error("Failed to load games.yml: %s", error);
		}

		if (!result.IsMap())
		{
			if (!result.IsNull())
			{
				//cfg_log.error("Failed to load games.yml: type %d not a map", result.Type());
			}
			return;
		}

		for (const auto& entry : result)
		{
			if (!entry.first.Scalar().empty() && entry.second.IsScalar() && !entry.second.Scalar().empty())
			{
				m_map.emplace(entry.first.Scalar(), entry.second.Scalar());
			}
		}
	}
}

