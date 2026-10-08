#pragma once

// PS5: a game's patches, as the desktop's patch manager offers them
// (rpcs3qt/patch_manager_dialog.cpp), for the launcher's Patches page.
//
// RPCS3's patch files are read from its patches folder (/app0/rpcs3/patches/:
// patch.yml, imported_patch.yml and every *_patch.yml, in the order the
// desktop reads them), the patches for one game's serial and version are
// listed, and what the player turns on is written to patch_config.yml, which
// the patch engine reads when the game boots (Emu/System.cpp). The title ships
// no patch file: the player puts RPCS3's patch.yml there, or the page
// rpcs3.net serves it in, which read() unpacks.

#include "Utilities/bin_patch.h"

#include <string>
#include <vector>

namespace ps5_patches
{
	// Where one of the patch's entries is turned on: its executable's hash and
	// its keys in the file, as the patch engine looks them up at boot
	struct target
	{
		std::string hash;
		std::string title;
		std::string serial;
		std::string app_version;
	};

	// A value the patch lets the player set, with its range or its choices
	struct value
	{
		std::string key;
		patch_engine::patch_config_value current;
		f64 default_value = 0.;
	};

	struct patch
	{
		std::string description;
		std::string author;
		std::string notes;
		std::string patch_version;
		std::string group;           // only one patch of a group is applied
		bool every_game = false;     // its title and serial are "All"
		bool every_version = false;  // for any version of the game
		bool enabled = false;
		std::vector<target> targets; // one for each executable it patches
		std::vector<value> values;
	};

	struct game
	{
		std::string serial;
		std::string app_version;
		patch_engine::patch_map map;      // everything read: patch_config.yml is written whole from it
		std::vector<patch> patches;       // this game's by name, then those for every game
		usz files = 0;                    // patch files read
		usz total = 0;                    // patches in them, for any game
		std::vector<std::string> broken;  // files with errors
		std::vector<std::string> other_versions; // versions of this game with patches, when this one has none
		std::string unpacked;             // a file from rpcs3.net unpacked into patch.yml, if one was
		bool changed = false;
	};

	// The patches folder, as FTP sees it
	std::string ftp_folder();

	// Reads every patch file and picks the game's patches
	game read(const std::string& serial, const std::string& app_version);

	// Turns a patch on or off; turning one on turns off the others of its group
	void set_enabled(game& g, usz index, bool enabled);

	// Moves a value a step (direction -1 or 1, `big` for ten): an allowed value
	// to the next, a range by a step of its own
	void step_value(game& g, usz index, usz value, s32 direction, bool big);
	void reset_value(game& g, usz index, usz value);

	// Writes patch_config.yml when anything changed
	void write(game& g);

	// A value as the page shows it: its allowed value's label, or its number
	std::string value_text(const value& v);
	std::string value_text(const value& v, f64 number);
}
