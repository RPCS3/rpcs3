#include "stdafx.h"
#include "system_utils.hpp"
#include "system_config.h"
#include "vfs_config.h"
#include "Emu/Io/pad_config.h"
#include "Emu/System.h"
#include "util/sysinfo.hpp"
#include "Utilities/File.h"
#include "Utilities/StrUtil.h"
#include "Utilities/Thread.h"
#include "Crypto/unpkg.h"
#include "Crypto/unself.h"
#include "Crypto/unedat.h"

#include <charconv>
#include <thread>

LOG_CHANNEL(sys_log, "SYS");

namespace rpcs3::utils
{
	u32 get_max_threads()
	{
		const u32 max_threads = static_cast<u32>(g_cfg.core.llvm_threads);
		const u32 hw_threads = ::utils::get_thread_count();
		const u32 thread_count = max_threads > 0 ? std::min(max_threads, hw_threads) : hw_threads;
		return thread_count;
	}

	void configure_logs(bool force_enable)
	{
		static bool was_silenced = false;

		const bool silenced = g_cfg.misc.silence_all_logs.get() && !force_enable;

		if (silenced)
		{
			if (!was_silenced)
			{
				sys_log.always()("Disabling logging! Do not create issues on GitHub or on the forums while logging is disabled.");
			}

			logs::silence();
		}
		else
		{
			logs::reset();
			logs::set_channel_levels(g_cfg.log.get_map());

			if (was_silenced)
			{
				sys_log.success("Logging enabled");
			}
		}

		was_silenced = silenced;
	}

	u32 check_user(const std::string& user)
	{
		u32 id = 0;

		if (user.size() == 8)
		{
			std::from_chars(&user.front(), &user.back() + 1, id);
		}

		return id;
	}

	bool install_pkg(const std::string& path)
	{
		sys_log.success("Installing package: %s", path);

		int int_progress = 0;

		std::deque<package_reader> reader;
		reader.emplace_back(path);

		// Run PKG unpacking asynchronously
		named_thread worker("PKG Installer", [&]
		{
			std::deque<std::string> bootables;
			const package_install_result result = package_reader::extract_data(reader, bootables);
			return result.error == package_install_result::error_type::no_error;
		});

		// Wait for the completion
		while (std::this_thread::sleep_for(5ms), worker <= thread_state::aborting)
		{
			// TODO: update unified progress dialog
			const int pval = reader[0].get_progress(100);

			if (pval > int_progress)
			{
				int_progress = pval;
				sys_log.success("... %u%%", int_progress);
			}
		}

		return worker();
	}

	std::string get_emu_dir()
	{
		const std::string& emu_dir_ = g_cfg_vfs.emulator_dir;
		return emu_dir_.empty() ? fs::get_config_dir() : emu_dir_;
	}

	std::string get_games_dir()
	{
		return g_cfg_vfs.get(g_cfg_vfs.games_dir, get_emu_dir());
	}

	std::string get_hdd0_dir()
	{
		return g_cfg_vfs.get(g_cfg_vfs.dev_hdd0, get_emu_dir());
	}

	std::string get_hdd1_dir()
	{
		return g_cfg_vfs.get(g_cfg_vfs.dev_hdd1, get_emu_dir());
	}

	std::string get_cache_dir()
	{
		return fs::get_cache_dir() + "cache/";
	}

	std::string get_cache_dir(std::string_view module_path)
	{
		std::string cache_dir = get_cache_dir();

		const std::string dev_flash = g_cfg_vfs.get_dev_flash();
		const bool in_dev_flash = Emu.IsPathInsideDir(module_path, dev_flash);

		if (in_dev_flash && !Emu.IsPathInsideDir(module_path, dev_flash + "sys/external/"))
		{
			// Add prefix for vsh
			cache_dir += "vsh/";
		}
		else if (!in_dev_flash && !Emu.GetTitleID().empty() && Emu.GetCat() != "1P")
		{
			// Add prefix for anything except dev_flash files, standalone elfs or PS1 classics
			cache_dir += Emu.GetTitleID();
			cache_dir += '/';
		}

		return cache_dir;
	}

	std::string get_rap_file_path(const std::string_view& rap)
	{
		const std::string home_dir = get_hdd0_dir() + "home";

		std::string rap_path;

		for (auto&& entry : fs::dir(home_dir))
		{
			if (entry.is_directory && check_user(entry.name))
			{
				rap_path = fmt::format("%s/%s/exdata/%s.rap", home_dir, entry.name, rap);
				if (fs::is_file(rap_path))
				{
					return rap_path;
				}
			}
		}

		// Return a sample path tested for logging purposes
		return rap_path;
	}

	std::string get_c00_unlock_edat_path(const std::string_view& content_id)
	{
		const std::string home_dir = get_hdd0_dir() + "home";

		std::string edat_path;

		for (auto&& entry : fs::dir(home_dir))
		{
			if (entry.is_directory && check_user(entry.name))
			{
				edat_path = fmt::format("%s/%s/exdata/%s.edat", home_dir, entry.name, content_id);
				if (fs::is_file(edat_path))
				{
					return edat_path;
				}
			}
		}

		// Return a sample path tested for logging purposes
		return edat_path;
	}

	bool verify_c00_unlock_edat(const std::string_view& content_id, bool fast)
	{
		const std::string edat_path = rpcs3::utils::get_c00_unlock_edat_path(content_id);

		// Check if user has unlock EDAT installed
		fs::file enc_file(edat_path);

		if (!enc_file)
		{
			sys_log.notice("verify_c00_unlock_edat(): '%s' not found", edat_path);
			return false;
		}

		// Use simple check for GUI
		if (fast)
			return true;

		u128 k_licensee = get_default_self_klic();
		NPD_HEADER npd;

		if (!VerifyEDATHeaderWithKLicense(enc_file, edat_path, reinterpret_cast<u8*>(&k_licensee), &npd))
		{
			sys_log.error("verify_c00_unlock_edat(): Failed to verify npd file '%s'", edat_path);
			return false;
		}

		std::string edat_content_id = npd.content_id;

		if (edat_content_id != content_id)
		{
			sys_log.error("verify_c00_unlock_edat(): Content ID mismatch in npd header of '%s'", edat_path);
			return false;
		}

		// Decrypt EDAT and verify its contents
		fs::file dec_file = DecryptEDAT(enc_file, edat_path, 8, reinterpret_cast<u8*>(&k_licensee));
		if (!dec_file)
		{
			sys_log.error("verify_c00_unlock_edat(): Failed to decrypt '%s'", edat_path);
			return false;
		}

		u32 magic{};
		dec_file.read<u32>(magic);
		if (magic != "GOMA"_u32)
		{
			sys_log.error("verify_c00_unlock_edat(): Bad header magic in unlock EDAT '%s'", edat_path);
			return false;
		}

		// Read null-terminated string
		dec_file.seek(0x10);
		dec_file.read(edat_content_id, 0x30);
		edat_content_id.resize(std::min<usz>(0x30, edat_content_id.find_first_of('\0')));
		if (edat_content_id != content_id)
		{
			sys_log.error("verify_c00_unlock_edat(): Content ID mismatch in unlock EDAT '%s'", edat_path);
			return false;
		}

		// Game has been purchased and EDAT is verified
		return true;
	}

	std::string get_sfo_dir_from_game_path(const std::string& game_path, const std::string& title_id)
	{
		if (fs::is_file(game_path + "/PS3_DISC.SFB"))
		{
			// This is a disc game.
			if (!title_id.empty())
			{
				for (auto&& entry : fs::dir{game_path})
				{
					if (entry.name == "." || entry.name == "..")
					{
						continue;
					}

					const std::string sfo_path = game_path + "/" + entry.name + "/PARAM.SFO";

					if (entry.is_directory && fs::is_file(sfo_path))
					{
						const auto psf = psf::load_object(sfo_path);
						const auto serial = psf::get_string(psf, "TITLE_ID");
						if (serial == title_id)
						{
							return game_path + "/" + entry.name;
						}
					}
				}
			}

			return game_path + "/PS3_GAME";
		}

		const auto psf = psf::load_object(game_path + "/PARAM.SFO");

		const auto category = psf::get_string(psf, "CATEGORY");
		const auto content_id = psf::get_string(psf, "CONTENT_ID");

		if (category == "HG" && !content_id.empty())
		{
			// This is a trial game. Check if the user has EDAT file to unlock it.
			const auto c00_title_id = psf::get_string(psf, "TITLE_ID");

			if (fs::is_file(game_path + "/C00/PARAM.SFO") && verify_c00_unlock_edat(content_id, true))
			{
				// Load full game data.
				sys_log.notice("Found EDAT file %s.edat for trial game %s", content_id, c00_title_id);
				return game_path + "/C00";
			}
		}

		return game_path;
	}

	std::string get_custom_config_dir()
	{
#ifdef _WIN32
		return fs::get_config_dir() + "config/custom_configs/";
#else
		return fs::get_config_dir() + "custom_configs/";
#endif
	}

	std::string get_custom_config_path(const std::string& identifier)
	{
		if (identifier.empty())
		{
			return {};
		}

		return get_custom_config_dir() + "config_" + identifier + ".yml";
	}

	std::string get_input_config_root()
	{
#ifdef _WIN32
		return fs::get_config_dir() + "config/input_configs/";
#else
		return fs::get_config_dir() + "input_configs/";
#endif
	}

	std::string get_input_config_dir(const std::string& title_id)
	{
		return get_input_config_root() + (title_id.empty() ? "global" : title_id) + "/";
	}

	std::string get_custom_input_config_path(const std::string& title_id)
	{
		if (title_id.empty()) return "";
		return get_input_config_dir(title_id) + g_cfg_input_configs.default_config + ".yml";
	}
}
