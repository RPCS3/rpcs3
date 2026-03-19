#include "stdafx.h"
#include "shortcut_utils.h"
#include "steam_utils.h"
#include "qt_utils.h"
#include "gui_game_info.h"

#include "Emu/VFS.h"
#include "Emu/vfs_config.h"
#include "Emu/system_utils.hpp"
#include "Utilities/File.h"
#include "Utilities/StrUtil.h"
#include "Loader/ISO.h"

#ifdef _WIN32
#include <Windows.h>
#include <shlobj.h>
#include <winnls.h>
#include <shobjidl.h>
#include <objbase.h>
#include <objidl.h>
#include <shlguid.h>
#include <comdef.h>

#include "Emu/system_utils.hpp"
#include <wrl/client.h>
#else
#include <sys/stat.h>
#include <errno.h>
#endif

#include <QFile>
#include <QImageWriter>
#include <QPixmap>
#include <QStandardPaths>

LOG_CHANNEL(sys_log, "SYS");

template <>
void fmt_class_string<gui::utils::shortcut_location>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](gui::utils::shortcut_location value)
	{
		switch (value)
		{
		case gui::utils::shortcut_location::desktop: return "desktop";
		case gui::utils::shortcut_location::applications: return "applications";
		case gui::utils::shortcut_location::steam: return "steam";
#ifdef _WIN32
		case gui::utils::shortcut_location::rpcs3_shortcuts: return "rpcs3";
#endif
		}

		return unknown;
	});
}

namespace gui::utils
{
#ifdef _WIN32
	static const std::string icon_extension = "ico";
#elif defined(__APPLE__)
	static const std::string icon_extension = "icns";
#else
	static const std::string icon_extension = "png";
#endif

	bool create_square_shortcut_icon_file(const std::string& path, const std::string& src_icon_path, const std::string& target_icon_dir, std::string& target_icon_path, int size)
	{
		if (src_icon_path.empty() || target_icon_dir.empty())
		{
			sys_log.error("Failed to create shortcut. Icon parameters empty.");
			return false;
		}

		const bool is_archive = is_file_iso(path);

		QPixmap icon;
		if (!load_icon(icon, src_icon_path, is_archive ? path : ""))
		{
			sys_log.error("Failed to create shortcut. Failed to load %sicon: '%s'", is_archive ? "iso " : "", src_icon_path);
			return false;
		}

		if (!create_square_pixmap(icon, size))
		{
			sys_log.error("Failed to create shortcut. Icon empty.");
			return false;
		}

		target_icon_path = target_icon_dir + "shortcut." + fmt::to_lower(icon_extension);

		QFile icon_file(QString::fromStdString(target_icon_path));
		if (!icon_file.open(QFile::OpenModeFlag::ReadWrite | QFile::OpenModeFlag::Truncate))
		{
			sys_log.error("Failed to create icon file '%s': %s", target_icon_path, icon_file.errorString());
			return false;
		}

		// Use QImageWriter instead of QPixmap::save in order to be able to log errors
		if (QImageWriter writer(&icon_file, fmt::to_upper(icon_extension).c_str()); !writer.write(icon.toImage()))
		{
			sys_log.error("Failed to write icon file '%s': %s", target_icon_path, writer.errorString());
			return false;
		}

		icon_file.close();

		sys_log.notice("Created shortcut icon file '%s'", target_icon_path);
		return true;
	}

	static std::string make_simple_name(const std::string& name)
	{
		const std::string simple_name = QString::fromStdString(vfs::escape(name, true)).simplified().toStdString();
		return simple_name;
	}

	bool create_shortcut(const std::string& name,
	                     const std::string& path,
	    [[maybe_unused]] const std::string& serial,
	                     const std::string& target_cli_args,
	    [[maybe_unused]] const std::string& description,
	                     const std::string& src_icon_path,
	    [[maybe_unused]] const std::string& target_icon_dir,
	                     const std::string& src_banner_path,
	                     shortcut_location location,
	                     std::shared_ptr<steam_shortcut> steam_sc,
	                     std::shared_ptr<iso_archive> archive)
	{
		if (name.empty())
		{
			sys_log.error("Cannot create shortcuts without a name");
			return false;
		}

		// Remove illegal characters from filename
		const std::string simple_name = make_simple_name(name);
		if (simple_name.empty() || simple_name == "." || simple_name == "..")
		{
			sys_log.error("Failed to create shortcut: Cleaned file name empty or not allowed");
			return false;
		}

		std::string link_path;
		bool append_rpcs3 = false;

		switch (location)
		{
		case shortcut_location::desktop:
			link_path = QStandardPaths::writableLocation(QStandardPaths::StandardLocation::DesktopLocation).toStdString();
			break;
		case shortcut_location::applications:
			link_path = QStandardPaths::writableLocation(QStandardPaths::StandardLocation::ApplicationsLocation).toStdString();
			append_rpcs3 = true;
			break;
		case shortcut_location::steam:
#ifdef __APPLE__
			link_path = QStandardPaths::writableLocation(QStandardPaths::StandardLocation::ApplicationsLocation).toStdString();
			append_rpcs3 = true;
#endif
			break;
#ifdef _WIN32
		case shortcut_location::rpcs3_shortcuts:
			link_path = rpcs3::utils::get_games_shortcuts_dir();
			fs::create_dir(link_path);
			break;
#endif
		}

		if (!link_path.empty() && !fs::is_dir(link_path) && !fs::create_dir(link_path))
		{
			sys_log.error("Failed to create shortcut. Folder does not exist: %s", link_path);
			return false;
		}

		if (append_rpcs3)
		{
			link_path += "/RPCS3";

			if (!fs::create_path(link_path))
			{
				sys_log.error("Failed to create shortcut. Could not create path: %s (%s)", link_path, fs::g_tls_error);
				return false;
			}
		}

#ifdef _WIN32
		const std::string working_dir{fs::get_executable_dir()};
		const std::string rpcs3_path{fs::get_executable_path()};
		std::string target_icon_path;

		if (!src_icon_path.empty() && !target_icon_dir.empty())
		{
			if (!create_square_shortcut_icon_file(path, src_icon_path, target_icon_dir, target_icon_path, 512))
			{
				sys_log.error("Failed to create shortcut: .ico creation failed");
				return false;
			}
		}

		if (location == shortcut_location::steam)
		{
			sys_log.notice("Creating %s shortcut with arguments '%s'", location, target_cli_args);
			ensure(steam_sc)->add_shortcut(simple_name, rpcs3_path, working_dir, target_cli_args, target_icon_path, src_icon_path, src_banner_path, archive);
			return true;
		}

		sys_log.notice("Creating %s shortcut '%s' with arguments '%s' and .ico dir '%s'", location, link_path, target_cli_args, target_icon_dir);

		const auto str_error = [](HRESULT hr) -> std::string
		{
			_com_error err(hr);
			const TCHAR* errMsg = err.ErrorMessage();
			return fmt::format("%s [%d]", wchar_to_utf8(errMsg), hr);
		};

		fmt::append(link_path, "/%s.lnk", simple_name);

		// https://stackoverflow.com/questions/3906974/how-to-programmatically-create-a-shortcut-using-win32
		HRESULT res = CoInitialize(NULL);
		if (FAILED(res))
		{
			sys_log.error("Failed to create shortcut: CoInitialize failed (%s)", str_error(res));
			return false;
		}

		Microsoft::WRL::ComPtr<IShellLink> pShellLink;
		Microsoft::WRL::ComPtr<IPersistFile> pPersistFile;

		const auto cleanup = [&](bool return_value, const std::string& fail_reason) -> bool
		{
			if (!return_value) sys_log.error("Failed to create shortcut: %s", fail_reason);
			CoUninitialize();
			return return_value;
		};

		res = CoCreateInstance(__uuidof(ShellLink), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pShellLink));
		if (FAILED(res))
			return cleanup(false, "CoCreateInstance failed");

		const std::wstring w_target_file = utf8_to_wchar(rpcs3_path);
		res = pShellLink->SetPath(w_target_file.c_str());
		if (FAILED(res))
			return cleanup(false, fmt::format("SetPath failed (%s)", str_error(res)));

		const std::wstring w_working_dir = utf8_to_wchar(working_dir);
		res = pShellLink->SetWorkingDirectory(w_working_dir.c_str());
		if (FAILED(res))
			return cleanup(false, fmt::format("SetWorkingDirectory failed (%s)", str_error(res)));

		if (!target_cli_args.empty())
		{
			const std::wstring w_target_cli_args = utf8_to_wchar(target_cli_args);
			res = pShellLink->SetArguments(w_target_cli_args.c_str());
			if (FAILED(res))
				return cleanup(false, fmt::format("SetArguments failed (%s)", str_error(res)));
		}

		if (!description.empty())
		{
			const std::wstring w_descpription = utf8_to_wchar(description);
			res = pShellLink->SetDescription(w_descpription.c_str());
			if (FAILED(res))
				return cleanup(false, fmt::format("SetDescription failed (%s)", str_error(res)));
		}

		if (!target_icon_path.empty())
		{
			const std::wstring w_icon_path = utf8_to_wchar(target_icon_path);
			res = pShellLink->SetIconLocation(w_icon_path.c_str(), 0);
			if (FAILED(res))
				return cleanup(false, fmt::format("SetIconLocation failed (%s)", str_error(res)));
		}

		// Use the IPersistFile object to save the shell link
		res = pShellLink.As(&pPersistFile);
		if (FAILED(res))
			return cleanup(false, fmt::format("QueryInterface failed (%s)", str_error(res)));

		// Save shortcut
		const std::wstring w_link_file = utf8_to_wchar(link_path);
		res = pPersistFile->Save(w_link_file.c_str(), TRUE);
		if (FAILED(res))
		{
			if (location == shortcut_location::desktop)
			{
				return cleanup(false, fmt::format("Saving file to desktop failed (%s)", str_error(res)));
			}
			else
			{
				return cleanup(false, fmt::format("Saving file to start menu failed (%s)", str_error(res)));
			}
		}

		return cleanup(true, {});

#elif defined(__APPLE__)

		fmt::append(link_path, "/%s.app", simple_name);

		sys_log.notice("Creating %s shortcut '%s' with arguments '%s'", location, link_path, target_cli_args);

		const std::string contents_dir = link_path + "/Contents/";
		const std::string macos_dir = contents_dir + "MacOS/";
		const std::string resources_dir = contents_dir + "Resources/";

		if (!fs::create_path(contents_dir) || !fs::create_path(macos_dir) || !fs::create_path(resources_dir))
		{
			sys_log.error("Failed to create shortcut. Could not create app bundle structure. (%s)", fs::g_tls_error);
			return false;
		}

		const std::string plist_path = contents_dir + "Info.plist";
		const std::string launcher_path = macos_dir + "launcher";

		std::string launcher_content;
		fmt::append(launcher_content, "#!/bin/bash\nopen -b net.rpcs3.rpcs3 --args %s", target_cli_args);

		fs::file launcher_file(launcher_path, fs::read + fs::rewrite);
		if (!launcher_file)
		{
			sys_log.error("Failed to create launcher file: %s (%s)", launcher_path, fs::g_tls_error);
			return false;
		}
		if (launcher_file.write(launcher_content.data(), launcher_content.size()) != launcher_content.size())
		{
			sys_log.error("Failed to write launcher file: %s", launcher_path);
			return false;
		}
		launcher_file.close();

		if (chmod(launcher_path.c_str(), S_IRWXU) != 0)
		{
			sys_log.error("Failed to change file permissions for launcher file: %s (%d)", strerror(errno), errno);
			return false;
		}

		const std::string plist_content = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
										  "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
										  "<plist version=\"1.0\">\n"
										  "<dict>\n"
										  "\t<key>CFBundleExecutable</key>\n"
										  "\t<string>launcher</string>\n"
										  "\t<key>CFBundleIconFile</key>\n"
										  "\t<string>shortcut.icns</string>\n"
										  "\t<key>CFBundleInfoDictionaryVersion</key>\n"
										  "\t<string>1.0</string>\n"
										  "\t<key>CFBundlePackageType</key>\n"
										  "\t<string>APPL</string>\n"
										  "\t<key>CFBundleSignature</key>\n"
										  "\t<string>\?\?\?\?</string>\n"
#if defined(ARCH_ARM64)
										  "\t<key>CFBundleIdentifier</key>\n"
										  "\t<string>net.rpcs3" + (serial.empty() ? "" : ("." + serial)) + "</string>\n"
										  "\t<key>LSArchitecturePriority</key>\n"
										  "\t<array>\n"
										  "\t\t<string>arm64</string>\n"
										  "\t</array>\n"
										  "\t<key>LSRequiresNativeExecution</key>\n"
										  "\t<true/>\n"
#endif
										  "</dict>\n"
										  "</plist>\n";

		fs::file plist_file(plist_path, fs::read + fs::rewrite);
		if (!plist_file)
		{
			sys_log.error("Failed to create plist file: %s (%s)", plist_path, fs::g_tls_error);
			return false;
		}
		if (plist_file.write(plist_content.data(), plist_content.size()) != plist_content.size())
		{
			sys_log.error("Failed to write plist file: %s", plist_path);
			return false;
		}
		plist_file.close();

		if (!src_icon_path.empty())
		{
			std::string target_icon_path = resources_dir;
			if (!create_square_shortcut_icon_file(path, src_icon_path, resources_dir, target_icon_path, 512))
			{
				// Error is logged in create_square_shortcut_icon_file
				return false;
			}
		}

		if (location == shortcut_location::steam)
		{
			ensure(steam_sc)->add_shortcut(simple_name, link_path, macos_dir, ""/*target_cli_args are already in the launcher*/, "", src_icon_path, src_banner_path, archive);
			return true;
		}

		return true;

#else
		const std::string exe_path = fs::get_executable_path();
		if (exe_path.empty())
		{
			sys_log.error("Failed to create shortcut. Executable path empty.");
			return false;
		}

		std::string target_icon_path;
		if (!src_icon_path.empty() && !target_icon_dir.empty())
		{
			if (!create_square_shortcut_icon_file(path, src_icon_path, target_icon_dir, target_icon_path, 512))
			{
				// Error is logged in create_square_shortcut_icon_file
				return false;
			}
		}

		if (location == shortcut_location::steam)
		{
			sys_log.notice("Creating %s shortcut with arguments '%s'", location, target_cli_args);
			const std::string working_dir{fs::get_executable_dir()};
			ensure(steam_sc)->add_shortcut(simple_name, exe_path, working_dir, target_cli_args, target_icon_path, src_icon_path, src_banner_path, archive);
			return true;
		}

		fmt::append(link_path, "/%s.desktop", simple_name);

		sys_log.notice("Creating %s shortcut '%s' for '%s' with arguments '%s'", location, link_path, exe_path, target_cli_args);

		std::string file_content;
		fmt::append(file_content, "[Desktop Entry]\n");
		fmt::append(file_content, "Encoding=UTF-8\n");
		fmt::append(file_content, "Version=1.0\n");
		fmt::append(file_content, "Type=Application\n");
		fmt::append(file_content, "Terminal=false\n");
		fmt::append(file_content, "Exec=\"%s\" %s\n", exe_path, target_cli_args);
		fmt::append(file_content, "Name=%s\n", name);
		fmt::append(file_content, "Categories=Application;Game\n");

		if (!description.empty())
		{
			fmt::append(file_content, "Comment=%s\n", QString::fromStdString(description).simplified());
		}

		if (!target_icon_path.empty())
		{
			fmt::append(file_content, "Icon=%s\n", target_icon_path);
		}

		fs::file shortcut_file(link_path, fs::read + fs::rewrite);
		if (!shortcut_file)
		{
			sys_log.error("Failed to create .desktop file: %s (%s)", link_path, fs::g_tls_error);
			return false;
		}
		if (shortcut_file.write(file_content.data(), file_content.size()) != file_content.size())
		{
			sys_log.error("Failed to write .desktop file: %s", link_path);
			return false;
		}
		shortcut_file.close();

		if (location == shortcut_location::desktop)
		{
			if (chmod(link_path.c_str(), S_IRWXU) != 0) // enables user to execute file
			{
				// Simply log failure. At least we have the file.
				sys_log.error("Failed to change file permissions for .desktop file: %s (%d)", strerror(errno), errno);
			}
		}

		return true;
#endif
	}

	bool create_shortcuts(const std::shared_ptr<gui_game_info>& game,
	                      const std::set<shortcut_location>& locations,
	                      std::shared_ptr<steam_shortcut> steam_sc)
	{
		if (!game || locations.empty()) return false;

		std::string gameid_token_value;

		const std::string dev_flash = g_cfg_vfs.get_dev_flash();
		const bool is_iso = is_file_iso(game->info.path);
		std::shared_ptr<iso_archive> archive;

		const auto file_exists = [&archive](const std::string& path)
		{
			return archive ? archive->is_file(path) : fs::is_file(path);
		};

		if (is_iso)
		{
			gameid_token_value = game->info.serial;
			archive = std::make_shared<iso_archive>(game->info.path);
		}
		else if (game->info.category == "DG" && !fs::is_file(rpcs3::utils::get_hdd0_dir() + "/game/" + game->info.serial + "/USRDIR/EBOOT.BIN"))
		{
			const usz ps3_game_dir_pos = fs::get_parent_dir(game->info.path).size();
			std::string relative_boot_dir = game->info.path.substr(ps3_game_dir_pos);

			if (usz char_pos = relative_boot_dir.find_first_not_of(fs::delim); char_pos != umax)
			{
				relative_boot_dir = relative_boot_dir.substr(char_pos);
			}
			else
			{
				relative_boot_dir.clear();
			}

			if (!relative_boot_dir.empty())
			{
				if (relative_boot_dir != "PS3_GAME")
				{
					gameid_token_value = game->info.serial + "/" + relative_boot_dir;
				}
				else
				{
					gameid_token_value = game->info.serial;
				}
			}
		}
		else
		{
			gameid_token_value = game->info.serial;
		}

		const std::string target_icon_dir = fmt::format("%sIcons/game_icons/%s/", fs::get_config_dir(), game->info.serial);

		if (!fs::create_path(target_icon_dir))
		{
			sys_log.error("Failed to create shortcut path %s (%s)", QString::fromStdString(game->info.name).simplified(), target_icon_dir, fs::g_tls_error);
			return false;
		}

		bool success = true;
		const bool is_vsh = game->info.path.starts_with(dev_flash);
		const std::string cli_arg_token = is_vsh ? "RPCS3_VFS" : "RPCS3_GAMEID";
		const std::string cli_arg_value = is_vsh ? ("dev_flash/" + game->info.path.substr(dev_flash.size())) : gameid_token_value;

		for (shortcut_location location : locations)
		{
			std::string banner_path;

			if (location == shortcut_location::steam)
			{
				// Try to find a nice banner for steam
				const std::string sfo_dir = is_iso ? "PS3_GAME" : rpcs3::utils::get_sfo_dir_from_game_path(game->info.path);

				for (const std::string& filename : {"PIC1.PNG"s, "PIC3.PNG"s, "PIC0.PNG"s, "PIC2.PNG"s, "ICON0.PNG"s})
				{
					if (const std::string filepath = fmt::format("%s/%s", sfo_dir, filename); file_exists(filepath))
					{
						banner_path = filepath;
						break;
					}
				}
			}

#ifdef __linux__
			const std::string percent = location == shortcut_location::steam ? "%" : "%%";
#else
			const std::string percent = "%";
#endif
			const std::string target_cli_args = fmt::format("--no-gui \"%s%s%s:%s\"", percent, cli_arg_token, percent, cli_arg_value);

			if (!gameid_token_value.empty() && create_shortcut(game->info.name, game->icon_in_archive ? game->info.path : "", game->info.serial, target_cli_args, game->info.name, game->info.icon_path, target_icon_dir, banner_path, location, steam_sc, archive))
			{
				if (location == shortcut_location::steam)
				{
					// Creation is done in caller
					sys_log.notice("Prepared %s shortcut for '%s'", location, QString::fromStdString(game->info.name).simplified());
				}
				else
				{
					sys_log.success("Created %s shortcut for '%s'", location, QString::fromStdString(game->info.name).simplified());
				}
			}
			else
			{
				sys_log.error("Failed to create %s shortcut for '%s'", location, QString::fromStdString(game->info.name).simplified());
				success = false;
			}
		}

		return success;
	}

	bool batch_create_shortcuts(const std::vector<std::shared_ptr<gui_game_info>>& games,
	                            const std::set<shortcut_location>& locations)
	{
		if (games.empty() || locations.empty()) return false;

		std::shared_ptr<steam_shortcut> steam_sc;

		if (locations.contains(shortcut_location::steam))
		{
			// Batch steam shortcut creation
			steam_sc = std::make_shared<steam_shortcut>();
		}

		bool result = true;

		for (const auto& game : games)
		{
			result &= create_shortcuts(game, locations, steam_sc);
		}

		if (steam_sc)
		{
			result &= steam_sc->write_file();
		}

		return result;
	}

	static void remove_shortcuts(const std::string& simple_name, [[maybe_unused]] const std::string& serial)
	{
		if (simple_name.empty() || simple_name == "." || simple_name == "..")
		{
			sys_log.error("Failed to remove shortcuts: Cleaned file name empty or not allowed");
			return;
		}

		const auto remove_path = [](const std::string& path, bool is_file)
		{
			if (!path.empty())
			{
				if (is_file && fs::is_file(path))
				{
					if (fs::remove_file(path))
					{
						sys_log.success("Removed shortcut file '%s'", path);
					}
					else
					{
						sys_log.error("Failed to remove shortcut file '%s': error='%s'", path, fs::g_tls_error);
					}
				}
				else if (!is_file && fs::is_dir(path))
				{
					if (fs::remove_all(path))
					{
						sys_log.success("Removed shortcut directory '%s'", path);
					}
					else
					{
						sys_log.error("Failed to remove shortcut directory '%s': error='%s'", path, fs::g_tls_error);
					}
				}
			}
		};

		std::vector<shortcut_location> locations = {
			shortcut_location::desktop,
			shortcut_location::applications,
			//shortcut_location::steam, // Handled separately
		};
#ifdef _WIN32
		locations.push_back(shortcut_location::rpcs3_shortcuts);
#endif

		for (shortcut_location location : locations)
		{
			std::string link_path;

			switch (location)
			{
			case shortcut_location::desktop:
				link_path = QStandardPaths::writableLocation(QStandardPaths::StandardLocation::DesktopLocation).toStdString();
				break;
			case shortcut_location::applications:
				link_path = QStandardPaths::writableLocation(QStandardPaths::StandardLocation::ApplicationsLocation).toStdString();
				link_path += "/RPCS3";
				break;
			case shortcut_location::steam:
				fmt::throw_exception("Steam shortcuts should be removed in batch");
				break;
#ifdef _WIN32
			case shortcut_location::rpcs3_shortcuts:
				link_path = rpcs3::utils::get_games_shortcuts_dir();
				break;
#endif
			}

#ifdef _WIN32
			fmt::append(link_path, "/%s.lnk", simple_name);
			remove_path(link_path, true);
#elif defined(__APPLE__)
			fmt::append(link_path, "/%s.app", simple_name);
			remove_path(link_path, false);
#else
			fmt::append(link_path, "/%s.desktop", simple_name);
			remove_path(link_path, true);
#endif
		}

		const std::string icon_path = fmt::format("%sIcons/game_icons/%s/shortcut.%s", fs::get_config_dir(), serial, icon_extension);
		remove_path(icon_path, true);
	}

	void batch_remove_shortcuts(const std::vector<std::pair<std::string /*name*/, std::string /*serial*/>>& games)
	{
		if (games.empty()) return;

		// Batch steam shortcut removal
		const std::string exe_path = fs::get_executable_path();
		const std::string working_dir = fs::get_executable_dir();
		const bool is_steam_installed = steam_shortcut::steam_installed();
		steam_shortcut steam_sc{};

		for (const auto& [name, serial] : games)
		{
			const std::string simple_name = make_simple_name(name);

			remove_shortcuts(simple_name, serial);

			if (is_steam_installed)
			{
				steam_sc.remove_shortcut(simple_name, exe_path, working_dir);
			}
		}

		if (is_steam_installed && !steam_sc.write_file())
		{
			sys_log.error("Failed to remove steam shortcuts");
		}
	}
}
