#include "stdafx.h"
#include "system_utils.hpp"
#include "system_config.h"
#include "Emu/Io/pad_config.h"
#include "util/sysinfo.hpp"
#include "Utilities/File.h"
#include "Utilities/StrUtil.h"
#include "Utilities/Thread.h"
#include "Crypto/unpkg.h"
#include "Crypto/unself.h"
#include "Crypto/unedat.h"

#include <charconv>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#endif

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

	void configure_logs()
	{
		static bool was_silenced = false;

		const bool silenced = g_cfg.misc.silence_all_logs.get();

		if (silenced)
		{
			if (!was_silenced)
			{
				sys_log.success("Disabling logging! Do not create issues on GitHub or on the forums while logging is disabled.");
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

		atomic_t<double> progress(0.);
		int int_progress = 0;

		// Run PKG unpacking asynchronously
		named_thread worker("PKG Installer", [&]
		{
			package_reader reader(path);
			return reader.extract_data(progress);
		});

		// Wait for the completion
		while (std::this_thread::sleep_for(5ms), worker <= thread_state::aborting)
		{
			// TODO: update unified progress dialog
			double pval = progress;
			pval < 0 ? pval += 1. : pval;
			pval *= 100.;

			if (static_cast<int>(pval) > int_progress)
			{
				int_progress = static_cast<int>(pval);
				sys_log.success("... %u%%", int_progress);
			}
		}

		return worker();
	}

#ifdef _WIN32
	std::string get_exe_dir()
	{
		wchar_t buffer[32767];
		GetModuleFileNameW(nullptr, buffer, sizeof(buffer) / 2);

		const std::string path_to_exe = wchar_to_utf8(buffer);
		const usz last = path_to_exe.find_last_of('\\');
		return last == std::string::npos ? std::string("") : path_to_exe.substr(0, last + 1);
	}
#endif

	std::string get_emu_dir()
	{
		const std::string& emu_dir_ = g_cfg.vfs.emulator_dir;
		return emu_dir_.empty() ? fs::get_config_dir() : emu_dir_;
	}

	std::string get_hdd0_dir()
	{
		return g_cfg.vfs.get(g_cfg.vfs.dev_hdd0, get_emu_dir());
	}

	std::string get_hdd1_dir()
	{
		return g_cfg.vfs.get(g_cfg.vfs.dev_hdd1, get_emu_dir());
	}

	std::string get_cache_dir()
	{
		return fs::get_cache_dir() + "cache/";
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

	std::string get_c00_unlock_edat_path(const std::string_view& title_id, const std::string_view& content_id)
	{
		const std::string home_dir = get_hdd0_dir() + "home";

		std::string edat_path;

		for (auto&& entry : fs::dir(home_dir))
		{
			if (entry.is_directory && check_user(entry.name))
			{
				edat_path = fmt::format("%s/%s/exdata/%s/%s.edat", home_dir, entry.name, title_id, content_id);
				if (fs::is_file(edat_path))
				{
					return edat_path;
				}
			}
		}

		// Return a sample path tested for logging purposes
		return edat_path;
	}

	bool verify_c00_unlock_edat(const std::string_view& title_id, const std::string_view& content_id)
	{
		const std::string edat_path = rpcs3::utils::get_c00_unlock_edat_path(title_id, content_id);

		// Check if user has unlock EDAT installed
		if (!fs::is_file(edat_path))
		{
			sys_log.notice("verify_c00_unlock_edat(): '%s' not found", edat_path);
			return false;
		}

		const fs::file enc_file(edat_path);
		u128 k_licensee = get_default_self_klic();
		std::string edat_content_id;

		if (!VerifyEDATHeaderWithKLicense(enc_file, edat_path, reinterpret_cast<u8*>(&k_licensee), &edat_content_id))
		{
			sys_log.error("verify_c00_unlock_edat(): Failed to verify npd file '%s'", edat_path);
			return false;
		}

		if (edat_content_id != content_id)
		{
			sys_log.error("verify_c00_unlock_edat(): Content ID mismatch in npd header of '%s'", edat_path);
			return false;
		}

		// Check if required RAP is present
		std::string rap_path = rpcs3::utils::get_rap_file_path(content_id);

		if (!fs::is_file(rap_path))
		{
			// Not necessarily an error
			sys_log.warning("verify_c00_unlock_edat(): RAP file not found: '%s'", rap_path);
			rap_path.clear();
		}

		// Decrypt EDAT and verify its contents
		fs::file dec_file = DecryptEDAT(enc_file, edat_path, 8, rap_path, reinterpret_cast<u8*>(&k_licensee), false);
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
		dec_file.read<true>(edat_content_id, 0x30);
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
						const auto psf = psf::load_object(fs::file(sfo_path));
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

		const auto psf = psf::load_object(fs::file(game_path + "/PARAM.SFO"));

		const auto category = psf::get_string(psf, "CATEGORY");
		const auto content_id = psf::get_string(psf, "CONTENT_ID");

		if (category == "HG" && !content_id.empty())
		{
			// This is a trial game. Check if the user has EDAT file to unlock it.
			const auto c00_title_id = psf::get_string(psf, "TITLE_ID");

			if (fs::is_file(game_path + "/C00/PARAM.SFO") && verify_c00_unlock_edat(c00_title_id, content_id))
			{
				// Load full game data.
				sys_log.notice("Verified EDAT file %s.edat for trial game %s", content_id, c00_title_id);
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

	std::string get_custom_config_path(const std::string& title_id)
	{
		return get_custom_config_dir() + "config_" + title_id + ".yml";
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
		return get_input_config_dir(title_id) + g_cfg_profile.default_profile + ".yml";
	}
}
