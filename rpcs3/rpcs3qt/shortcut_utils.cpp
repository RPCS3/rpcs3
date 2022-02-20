#include "stdafx.h"
#include "shortcut_utils.h"
#include "qt_utils.h"
#include "Emu/system_utils.hpp"
#include "Emu/VFS.h"
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
#elif !defined(__APPLE__)
#include <sys/stat.h>
#include <errno.h>
#endif

#include <QFile>
#include <QPixmap>

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
			sys_log.error("Failed to create icon file: %s", target_icon_path);
			return false;
		}

		if (!icon.save(&icon_file, fmt::to_upper(extension).c_str()))
		{
			sys_log.error("Failed to write icon file: %s", target_icon_path);
			return false;
		}

		icon_file.close();
		return true;
	}

	bool create_shortcut(const std::string& name,
	    [[maybe_unused]] const std::string& target_cli_args,
	    [[maybe_unused]] const std::string& description,
	    [[maybe_unused]] const std::string& src_icon_path,
	    [[maybe_unused]] const std::string& target_icon_dir,
	    bool is_desktop_shortcut)
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

#ifdef _WIN32
		std::string link_file;

		if (const char* home = getenv("USERPROFILE"))
		{
			if (is_desktop_shortcut)
			{
				link_file = fmt::format("%s/Desktop/%s.lnk", home, simple_name);
			}
			else
			{
				const std::string programs_dir = fmt::format("%s/AppData/Roaming/Microsoft/Windows/Start Menu/Programs/RPCS3", home);
				if (!fs::create_path(programs_dir))
				{
					sys_log.error("Failed to create shortcut: Could not create start menu directory: %s", programs_dir);
					return false;
				}
				link_file = fmt::format("%s/%s.lnk", programs_dir, simple_name);
			}
		}
		else
		{
			sys_log.error("Failed to create shortcut: home path empty");
			return false;
		}

		sys_log.notice("Creating shortcut '%s' with arguments '%s' and .ico dir '%s'", link_file, target_cli_args, target_icon_dir);

		const auto str_error = [](HRESULT hr) -> std::string
		{
			_com_error err(hr);
			const TCHAR* errMsg = err.ErrorMessage();
			return fmt::format("%s [%d]", wchar_to_utf8(errMsg), hr);
		};

		// https://stackoverflow.com/questions/3906974/how-to-programmatically-create-a-shortcut-using-win32
		HRESULT res = CoInitialize(NULL);
		if (FAILED(res))
		{
			sys_log.error("Failed to create shortcut: CoInitialize failed (%s)", str_error(res));
			return false;
		}

		IShellLink* pShellLink = nullptr;
		IPersistFile* pPersistFile = nullptr;

		const auto cleanup = [&](bool return_value, const std::string& fail_reason) -> bool
		{
			if (!return_value) sys_log.error("Failed to create shortcut: %s", fail_reason);
			if (pPersistFile) pPersistFile->Release();
			if (pShellLink) pShellLink->Release();
			CoUninitialize();
			return return_value;
		};

		res = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&pShellLink);
		if (FAILED(res))
			return cleanup(false, "CoCreateInstance failed");

		const std::string working_dir{ rpcs3::utils::get_exe_dir() };
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
		res = pShellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&pPersistFile);
		if (FAILED(res))
			return cleanup(false, fmt::format("QueryInterface failed (%s)", str_error(res)));

		// Save shortcut
		const std::wstring w_link_file = utf8_to_wchar(link_file);
		res = pPersistFile->Save(w_link_file.c_str(), TRUE);
		if (FAILED(res))
		{
			if (is_desktop_shortcut)
			{
				return cleanup(false, fmt::format("Saving file to desktop failed (%s)", str_error(res)));
			}
			else
			{
				return cleanup(false, fmt::format("Saving file to start menu failed (%s)", str_error(res)));
			}
		}

		return cleanup(true, {});

#elif !defined(__APPLE__)

		const std::string exe_path = rpcs3::utils::get_executable_path();
		if (exe_path.empty())
		{
			sys_log.error("Failed to create shortcut. Executable path empty.");
			return false;
		}

		std::string link_path;

		if (const char* home = ::getenv("HOME"))
		{
			if (is_desktop_shortcut)
			{
				link_path = fmt::format("%s/Desktop/%s.desktop", home, simple_name);
			}
			else
			{
				link_path = fmt::format("%s/.local/share/applications/%s.desktop", home, simple_name);
			}
		}
		else
		{
			sys_log.error("Failed to create shortcut. home path empty.");
			return false;
		}

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
			fmt::append(file_content, "Comment=%s\n", QString::fromStdString(description).simplified().toStdString());
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
			sys_log.error("Failed to create .desktop file: %s", link_path);
			return false;
		}
		if (shortcut_file.write(file_content.data(), file_content.size()) != file_content.size())
		{
			sys_log.error("Failed to write .desktop file: %s", link_path);
			return false;
		}
		shortcut_file.close();

		if (is_desktop_shortcut)
		{
			if (chmod(link_path.c_str(), S_IRWXU) != 0) // enables user to execute file
			{
				// Simply log failure. At least we have the file.
				sys_log.error("Failed to change file permissions for .desktop file: %s (%d)", strerror(errno), errno);
			}
		}

		return true;
#else
		sys_log.error("Cannot create shortcuts on this operating system");
		return false;
#endif
	}
}
