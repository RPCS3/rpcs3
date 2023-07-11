#pragma once

#include "Utilities/mutex.h"
#include <map>

class associative_map_file
{
public:
	associative_map_file(std::string path);
	~associative_map_file();

	void set_save_on_dirty(bool enabled) { m_save_on_dirty = enabled; }

	const std::map<std::string, std::string> get_all_entries() const;
	bool is_dirty() const { return m_dirty; }

	std::string get_entry(std::string_view key, bool unlocked = false) const noexcept;
	bool move_entry(std::string_view key, bool unlocked = false) noexcept;
	usz move_entries_with_suffix(std::string_view suffix, bool unlocked = false) noexcept;
	bool remove_entry(std::string_view key, bool unlocked = false) noexcept;

	template <typename Func, typename... Args>
	std::string get_entry(std::string_view key, Func&& func, Args&&... args) const noexcept
	{
		if (key.empty())
		{
			return {};
		}

		reader_lock lock(m_mutex);

		std::string data = get_entry(key);

		if (data.empty())
		{
			return std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
		}

		return data;
	}

	template <typename Func, typename... Args>
	auto atomic_op(Func&& func, Args&&... args) const noexcept
	{
		std::lock_guard lock(m_mutex);
		return std::forward<Func>(func)(std::forward<Args>(args)...);
	}

	bool set_entry(const std::string& key, const std::string& data, bool unlocked = false) noexcept;
	bool save();

	const std::string& get_file_path() const;
private:
	bool save_nl();
	void load();

	std::map<std::string, std::string, std::less<>> m_map;
	mutable shared_mutex m_mutex;

	bool m_dirty = false;
	bool m_save_on_dirty = true;

	std::string m_path;
};
