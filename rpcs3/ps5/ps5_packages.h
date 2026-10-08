#pragma once

// PS5: the launcher's Install tab: PS3 packages put in the title's packages
// folder over FTP (/app0/rpcs3/packages/), read and installed with RPCS3's
// own package installer (Crypto/unpkg.h), as the desktop's File > Install
// Packages/Raps/Edats does. A game installs into dev_hdd0/game as a game of
// its own; an update or an add-on into its game's folder there; a .rap or
// .edat licence is copied to the user's exdata folder.

#include "util/types.hpp"

#include <memory>
#include <string>
#include <vector>

namespace ps5_packages
{
	enum class kind : u8
	{
		game,
		update,
		addon,
		other,
		licence,
		unreadable
	};

	struct file
	{
		std::string name;              // in the folder
		std::string path;
		u64 size = 0;
		kind type = kind::unreadable;
		std::string title;
		std::string title_id;
		std::string version;           // APP_VER, else VERSION
		std::string target_version;    // an update's TARGET_APP_VER: the version it goes on
		std::string installed_version; // what dev_hdd0/game/<title id> holds now
		bool chosen = true;
	};

	// The packages folder, and as FTP sees it
	std::string folder();
	std::string ftp_folder();

	// The folder's packages and licences, in the order they would install:
	// licences, then games, then each game's updates and add-ons, oldest first
	std::vector<file> scan();

	// "Game", "Update 01.08", "Add-on", "Licence"...
	std::string kind_text(const file& f);
	std::string size_text(u64 bytes);

	// One installation of the chosen files, in the order given, on a thread
	// of its own. A file that fails does not stop the ones after it
	class installer
	{
	public:
		enum class state : u8
		{
			waiting,
			installing,
			installed,
			failed,
			cancelled
		};

		installer(std::vector<file> files, bool delete_when_installed);
		~installer(); // cancels, and waits for the thread

		void cancel();
		bool done() const;
		bool cancelled() const;

		const std::vector<file>& files() const;
		state state_of(usz index) const;
		f32 progress_of(usz index) const;           // 0 to 1
		std::string message_of(usz index) const;    // once it is no longer installing
		f32 progress() const;                       // of everything, by size

	private:
		struct impl;
		std::unique_ptr<impl> m;
	};
}
