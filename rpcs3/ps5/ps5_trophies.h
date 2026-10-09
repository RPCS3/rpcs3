#pragma once

// PS5: the launcher's Trophies tab: the trophies RPCS3 keeps for the user, a
// folder for each game's trophy set in dev_hdd0/home/<user>/trophy/
// (NPWR00000_00/), as the desktop's Trophy Manager reads them: TROPCONF.SFM
// (the set's XML: its name, and each trophy's grade, name, description and
// whether it is hidden), TROPUSR.DAT (which are earned, and when), ICON0.PNG
// and a TROPnnn.PNG for each trophy. Only read here: a set with no
// TROPUSR.DAT yet is shown with nothing earned, and nothing is written.

#include "util/types.hpp"

#include <string>
#include <vector>

namespace ps5_trophies
{
	enum class grade : u8
	{
		platinum,
		gold,
		silver,
		bronze,
		unknown
	};

	struct trophy
	{
		u32 id = 0;
		grade kind = grade::unknown;
		bool hidden = false;
		bool earned = false;
		u64 earned_tick = 0;   // a CellRtc tick: microseconds since 0001-01-01, UTC
		std::string name;
		std::string detail;
		std::string group;     // the add-on's name, or empty for the game's own
		std::string icon_path; // TROPnnn.PNG
	};

	struct game
	{
		std::string folder;    // NPWR00000_00
		std::string path;      // with a trailing slash
		std::string name;
		std::string icon_path; // ICON0.PNG
		std::vector<trophy> trophies;
		u32 earned = 0;
		u32 count[4]{};        // by grade, as the enum
		u32 earned_count[4]{};
		u64 last_tick = 0;     // the latest trophy's
		f32 progress = 0.f;    // 0..1, by the grades' points
	};

	// The user's trophy folder
	std::string folder();

	// Every set in it with at least one trophy: the latest earned first, then
	// the sets with none earned by name
	std::vector<game> read_all();

	// "25 Mar 2023", in the console's local time
	std::string date_text(u64 tick);

	// "Platinum", "Gold"...
	const char* grade_text(grade kind);
}
