#pragma once

// PS5: the launcher's Saves tab: the PS3 saves RPCS3 keeps for the user, a
// folder for each in dev_hdd0/home/<user>/savedata/ (BLUS30127-AUTOSAVE/, its
// name starting with the game's serial), with PARAM.SFO (its title, subtitle
// and details), ICON0.PNG and the game's own files; and their backups, copies
// of such a folder kept in rpcs3/save_backups/<save>/<when>/, which FTP
// reaches. A save can be backed up, restored from its latest backup (itself
// backed up first) and deleted (backed up first); a save deleted, or lost,
// whose backups remain is listed by them, to be restored.

#include "util/types.hpp"

#include <string>
#include <vector>

namespace ps5_saves
{
	struct backup
	{
		std::string path; // with a trailing slash
		s64 time = 0;     // when it was made, in seconds
		u64 size = 0;
	};

	struct save
	{
		std::string dir;       // the folder's name: BLUS30127-AUTOSAVE
		std::string serial;    // its first nine characters, the game's
		std::string path;      // with a trailing slash; empty when only backups remain
		std::string title;     // PARAM.SFO's, or the newest backup's
		std::string subtitle;
		std::string detail;
		std::string icon_path; // ICON0.PNG, empty without one
		u64 size = 0;
		s64 time = 0;          // its files' latest change, in seconds
		std::vector<backup> backups; // the newest first
	};

	// The user's save folder, and the backups' folder
	std::string folder();
	std::string backup_folder();

	// Every save, and every save gone whose backups remain: by game, the game
	// with the latest save first, and each game's saves the latest first
	std::vector<save> read_all();

	// Each returns what happened, for a message: empty when it went well
	std::string back_up(const save& target);
	std::string restore(const save& target, const backup& from);
	std::string remove(const save& target);
	std::string remove_backups(const save& target);
}
