#include "stdafx.h"
#include "shortcut_utils.h"
#include "qt_utils.h"
#include "Emu/VFS.h"
#include "Utilities/File.h"
#include "Utilities/StrUtil.h"

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

namespace gui::utils
{
	bool create_square_shortcut_icon_file(const std::string& src_icon_path, const std::string& target_icon_dir, std::string& target_icon_path, const std::string& extension, int size)
	{
		if (src_icon_path.empty() || target_icon_dir.empty() || extension.empty())
		{
			sys_log.error("Failed to create shortcut. Icon parameters empty.");
			return false;
		}

		QPixmap icon(QString::fromStdString(src_icon_path));
		if (!gui::utils::create_square_pixmap(icon, size))
		{
			sys_log.error("Failed to create shortcut. Icon empty.");
			return false;
		}

		target_icon_path = target_icon_dir + "shortcut." + fmt::to_lower(extension);

		QFile icon_file(QString::fromStdString(target_icon_path));
		if (!icon_file.open(QFile::OpenModeFlag::ReadWrite | QFile::OpenModeFlag::Truncate))
		{
			sys_log.error("Failed to create icon file '%s': %s", target_icon_path, icon_file.errorString());
			return false;
		}

		// Use QImageWriter instead of QPixmap::save in order to be able to log errors
		if (QImageWriter writer(&icon_file, fmt::to_upper(extension).c_str()); !writer.write(icon.toImage()))
		{
			sys_log.error("Failed to write icon file '%s': %s", target_icon_path, writer.errorString());
			return false;
		}

		icon_file.close();
		return true;
	}

	bool create_shortcut(const std::string& name,
	    [[maybe_unused]] const std::string& serial,
	    [[maybe_unused]] const std::string& target_cli_args,
	    [[maybe_unused]] const std::string& description,
	    [[maybe_unused]] const std::string& src_icon_path,
	    [[maybe_unused]] const std::string& target_icon_dir,
	    shortcut_location location)
	{
		if (name.empty())
		{
			sys_log.error("Cannot create shortcuts without a name");
			return false;
		}

		// Remove illegal characters from filename
		const std::string simple_name = QString::fromStdString(vfs::escape(name, true)).simplified().toStdString();
		if (simple_name.empty() || simple_name == "." || simple_name == "..")
		{
			sys_log.error("Failed to create shortcut: Cleaned file name empty or not allowed");
			return false;
		}

		std::string link_path;

		if (location == shortcut_location::desktop)
		{
			link_path = QStandardPaths::writableLocation(QStandardPaths::StandardLocation::DesktopLocation).toStdString();
		}
		else if (location == shortcut_location::applications)
		{
			link_path = QStandardPaths::writableLocation(QStandardPaths::StandardLocation::ApplicationsLocation).toStdString();
		}
#ifdef _WIN32
		else if (location == shortcut_location::rpcs3_shortcuts)
		{
			link_path = rpcs3::utils::get_games_dir() + "/shortcuts/";
			fs::create_dir(link_path);
		}
#endif

		if (!fs::is_dir(link_path) && !fs::create_dir(link_path))
		{
			sys_log.error("Failed to create shortcut. Folder does not exist: %s", link_path);
			return false;
		}

		if (location == shortcut_location::applications)
		{
			link_path += "/RPCS3";

			if (!fs::create_path(link_path))
			{
				sys_log.error("Failed to create shortcut. Could not create path: %s (%s)", link_path, fs::g_tls_error);
				return false;
			}
		}

#ifdef _WIN32
		const auto str_error = [](HRESULT hr) -> std::string
		{
			_com_error err(hr);
			const TCHAR* errMsg = err.ErrorMessage();
			return fmt::format("%s [%d]", wchar_to_utf8(errMsg), hr);
		};

		fmt::append(link_path, "/%s.lnk", simple_name);

		sys_log.notice("Creating shortcut '%s' with arguments '%s' and .ico dir '%s'", link_path, target_cli_args, target_icon_dir);

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

		const std::string working_dir{ fs::get_executable_dir() };
		const std::string rpcs3_path{ working_dir + "rpcs3.exe" };

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

		if (!src_icon_path.empty() && !target_icon_dir.empty())
		{
			std::string target_icon_path;
			if (!create_square_shortcut_icon_file(src_icon_path, target_icon_dir, target_icon_path, "ico", 512))
				return cleanup(false, ".ico creation failed");

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
			if (!create_square_shortcut_icon_file(src_icon_path, resources_dir, target_icon_path, "icns", 512))
			{
				// Error is logged in create_square_shortcut_icon_file
				return false;
			}
		}

		return true;

#else

		const std::string exe_path = fs::get_executable_path();
		if (exe_path.empty())
		{
			sys_log.error("Failed to create shortcut. Executable path empty.");
			return false;
		}

		fmt::append(link_path, "/%s.desktop", simple_name);

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

		if (!src_icon_path.empty() && !target_icon_dir.empty())
		{
			std::string target_icon_path;
			if (!create_square_shortcut_icon_file(src_icon_path, target_icon_dir, target_icon_path, "png", 512))
			{
				// Error is logged in create_square_shortcut_icon_file
				return false;
			}

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
}
