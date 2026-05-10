#pragma once

namespace disc
{
	enum class disc_type
	{
		invalid,
		unknown,
		ps1,
		ps2,
		ps3
	};

	disc_type get_disc_type(const std::string& path, std::string& disc_root, std::string& ps3_game_dir);
}
