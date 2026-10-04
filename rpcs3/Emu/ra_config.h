#pragma once
#include "util/yaml.hpp"
#include "Utilities/Config.h"
#include "util/logs.hpp"

struct cfg_ra : cfg::node
{
	cfg::_bool enabled{ this, "Enabled", false };
	cfg::_bool hardcore{ this, "Hardcore", false };
	cfg::_bool unofficial{ this, "Unofficial", false };
	cfg::_bool encore{ this, "Encore", false };
	cfg::_bool spectator{ this, "Spectator", false };
	cfg::_bool native_trophies{ this, "NativeTrophies", true };
	cfg::_bool discord{ this, "Discord", false };
	cfg::_bool leaderboard_trackers{ this, "LeaderboardTrackers", true };
	cfg::_bool challenge_indicators{ this, "ChallengeIndicators", true };
	cfg::_bool progress_notifications{ this, "ProgressNotifications", false };
	cfg::string username{ this, "Username", "" };
	cfg::string token{ this, "Token", "" };

	static std::string get_path();
	void load();
	void save() const;
};

extern cfg_ra g_cfg_ra;
