#include "stdafx.h"
#include "module_verifier.hpp"

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <Shlwapi.h>

#include <util/types.hpp>
#include <util/logs.hpp>
#include <Utilities/StrUtil.h>
#include <Utilities/StrFmt.h>

LOG_CHANNEL(sys_log, "SYS");

[[noreturn]] void report_fatal_error(std::string_view text, bool is_html, bool include_help_text);

void WIN32_module_verifier::run_module_verification()
{
	struct module_info_t
	{
		std::wstring_view name;
		std::string_view package_name;
		std::string_view dl_link;
	};

	const std::vector<module_info_t> special_module_infos = {
		{ L"vulkan-1.dll", "Vulkan Runtime", "https://sdk.lunarg.com/sdk/download/latest/windows/vulkan-runtime.exe" },
		{ L"msvcp140.dll", "Microsoft Visual C++ 2015-2022 Redistributable", "https://aka.ms/vs/17/release/VC_redist.x64.exe" },
		{ L"vcruntime140.dll", "Microsoft Visual C++ 2015-2022 Redistributable", "https://aka.ms/vs/17/release/VC_redist.x64.exe" },
		{ L"msvcp140_1.dll", "Microsoft Visual C++ 2015-2022 Redistributable", "https://aka.ms/vs/17/release/VC_redist.x64.exe" },
		{ L"vcruntime140_1.dll", "Microsoft Visual C++ 2015-2022 Redistributable", "https://aka.ms/vs/17/release/VC_redist.x64.exe" }
	};

	WCHAR windir[MAX_PATH];
	if (!GetWindowsDirectory(windir, MAX_PATH))
	{
		report_fatal_error(fmt::format("WIN32_module_verifier: Failed to query WindowsDirectory"), false, true);
	}

	for (const auto& module : special_module_infos)
	{
		const HMODULE hModule = GetModuleHandle(module.name.data());
		if (hModule == NULL)
		{
			continue;
		}

		WCHAR wpath[MAX_PATH];
		const auto len = GetModuleFileName(hModule, wpath, MAX_PATH);
		if (!len)
		{
			continue;
		}

		if (::StrStrI(wpath, windir) != wpath)
		{
			const std::string path = wchar_to_utf8(wpath);
			const std::string win_path = wchar_to_utf8(windir);
			const std::string module_name = wchar_to_utf8(module.name);
			const std::string error_message = fmt::format(
				"<p>"
				"The module <strong>%s</strong> was incorrectly installed at<br>"
				"'%s'<br>"
				"<br>"
				"This module is part of the <strong>%s</strong> package.<br>"
				"Install this package, then delete <strong>%s</strong> from rpcs3's installation directory.<br>"
				"<br>"
				"You can install this package from this URL:<br>"
				"<a href='%s'>%s</a>"
				"</p>",
				module_name,
				path,
				module.package_name,
				module_name,
				module.dl_link,
				module.dl_link
			);

			sys_log.error("Found incorrectly installed module: '%s' (path='%s', windows='%s')", module_name, path, win_path);

			report_fatal_error(error_message, true, false);
		}
	}
}

void WIN32_module_verifier::run()
{
	WIN32_module_verifier verifier{};
	verifier.run_module_verification();
}

#endif // _WIN32
