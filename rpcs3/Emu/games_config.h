#pragma once

#include "Utilities/mutex.h"
#include <map>

class games_config
{
public:
	games_config();
	virtual ~games_config();

	void set_save_on_dirty(bool enabled) { m_save_on_dirty = enabled; }

	const std::map<std::string, std::string> get_games() const;
	bool is_dirty() const { return m_dirty; }

	std::string get_path(const std::string& title_id) const;

	bool add_game(const std::string& key, const std::string& path);
	bool add_external_hdd_game(const std::string& key, std::string& path);
	bool save();

private:
	bool save_nl();
	void load();

	std::map<std::string, std::string> m_games;
	mutable shared_mutex m_mutex;

	bool m_dirty = false;
	bool m_save_on_dirty = true;
};
