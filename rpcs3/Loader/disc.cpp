#include "stdafx.h"
#include "disc.h"
#include "PSF.h"
#include "util/logs.hpp"
#include "Utilities/StrUtil.h"
#include "Emu/System.h"
#include "Emu/system_utils.hpp"

LOG_CHANNEL(disc_log, "DISC");

namespace disc
{
	disc_type get_disc_type(const std::string& path, std::string& disc_root, std::string& ps3_game_dir)
	{
		disc_type type = disc_type::unknown;

		disc_root.clear();
		ps3_game_dir.clear();

		if (path.empty())
		{
			disc_log.error("Can not determine disc type. Path is empty.");
			return disc_type::invalid;
		}

		if (!fs::is_dir(path))
		{
			disc_log.error("Can not determine disc type. Path not a directory: '%s'", path);
			return disc_type::invalid;
		}

		// Check for PS3 game first.
		std::string elf_path;
		if (const game_boot_result result = Emulator::GetElfPathFromDir(elf_path, path);
			result == game_boot_result::no_errors)
		{
			// Every error past this point is considered a corrupt disc.

			std::string sfb_dir;
			const std::string elf_dir = fs::get_parent_dir(elf_path);

			Emulator::GetBdvdDir(disc_root, sfb_dir, ps3_game_dir, elf_dir);

			if (!fs::is_dir(disc_root) || !fs::is_dir(sfb_dir) || ps3_game_dir.empty())
			{
				disc_log.error("Not a PS3 disc: invalid folder (bdvd_dir='%s', sfb_dir='%s', game_dir='%s')", disc_root, sfb_dir, ps3_game_dir);
				return disc_type::invalid;
			}

			// Load PARAM.SFO
			const std::string sfo_dir = rpcs3::utils::get_sfo_dir_from_game_path(elf_dir + "/../", "");
			const psf::registry _psf = psf::load_object(sfo_dir + "/PARAM.SFO");

			if (_psf.empty())
			{
				disc_log.error("Not a PS3 disc: Corrupted PARAM.SFO found! (path='%s/PARAM.SFO')", sfo_dir);
				return disc_type::invalid;
			}

			const std::string cat = std::string(psf::get_string(_psf, "CATEGORY"));

			if (cat != "DG")
			{
				disc_log.error("Not a PS3 disc: Wrong category '%s'.", cat);
				return disc_type::invalid;
			}

			return disc_type::ps3;
		}

		disc_log.notice("Not a PS3 disc: Elf not found. Looking for SYSTEM.CNF... (path='%s')", path);

		std::vector<std::string> lines;

		// Try to find SYSTEM.CNF
		for (std::string search_dir = path;;)
		{
			if (fs::file file(search_dir + "/SYSTEM.CNF"); file)
			{
				disc_root = search_dir + "/";
				lines = fmt::split(file.to_string(), {"\n"});
				break;
			}

			std::string parent_dir = fs::get_parent_dir(search_dir);

			if (parent_dir.size() == search_dir.size())
			{
				// Keep looking until root directory is reached
				disc_log.error("SYSTEM.CNF not found in path: '%s'", path);
				return disc_type::invalid;
			}

			search_dir = std::move(parent_dir);
		}

		for (usz i = 0; i < lines.size(); i++)
		{
			const std::string& line = lines[i];
			const usz pos = line.find('=');

			if (pos == umax)
			{
				continue;
			}

			const std::string key = fmt::trim(line.substr(0, pos));
			std::string value;

			if (pos != (line.size() - 1))
			{
				value = fmt::trim(line.substr(pos + 1));
			}

			if (value.empty() && i != (lines.size() - 1) && line.size() != 1)
			{
				// Some games have a character on the last line of the file, don't print the error in those cases.
				disc_log.warning("Unusual or malformed entry in SYSTEM.CNF ignored: %s", line);
				continue;
			}

			if (key == "BOOT2")
			{
				disc_log.notice("SYSTEM.CNF - Detected PS2 Disc = %s", value);
				type = disc_type::ps2;
			}
			else if (key == "BOOT")
			{
				disc_log.notice("SYSTEM.CNF - Detected PSX/PSone Disc = %s", value);
				type = disc_type::ps1;
			}
			else if (key == "VMODE")
			{
				disc_log.notice("SYSTEM.CNF - Disc region type = %s", value);
			}
			else if (key == "VER")
			{
				disc_log.notice("SYSTEM.CNF - Software version = %s", value);
			}
		}

		if (type == disc_type::unknown)
		{
			disc_log.error("SYSTEM.CNF - Disc is not a PSX/PSone or PS2 game!");
		}

		return type;
	}
}
