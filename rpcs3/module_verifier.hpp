#pragma once
#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>

#include <util/types.hpp>
#include <Utilities/StrUtil.h>
#include <Utilities/StrFmt.h>

[[noreturn]] void report_fatal_error(std::string_view text, bool is_html, bool include_help_text);

// Validates that system modules are properly installed
// Only relevant for WIN32
class WIN32_module_verifier
{
	struct module_info_t
	{
		std::wstring_view name;
		std::string_view package_name;
		std::string_view dl_link;
	};

	const std::vector<module_info_t> special_module_infos = {
		{ L"vulkan-1.dll", "Vulkan Runtime", "https://sdk.lunarg.com/sdk/download/latest/windows/vulkan-runtime.exe" },
		{ L"msvcp140.dll", "Microsoft Visual C++ 2015-2019 Redistributable", "https://aka.ms/vs/16/release/VC_redist.x64.exe" },
		{ L"vcruntime140.dll", "Microsoft Visual C++ 2015-2019 Redistributable", "https://aka.ms/vs/16/release/VC_redist.x64.exe" },
		{ L"msvcp140_1.dll", "Microsoft Visual C++ 2015-2019 Redistributable", "https://aka.ms/vs/16/release/VC_redist.x64.exe" },
		{ L"vcruntime140_1.dll", "Microsoft Visual C++ 2015-2019 Redistributable", "https://aka.ms/vs/16/release/VC_redist.x64.exe" }
	};

	// Inline impl. This class is only referenced once.
	void run_module_verification()
	{
		WCHAR windir[MAX_PATH];
		if (!GetWindowsDirectory(windir, MAX_PATH))
		{
			report_fatal_error(fmt::format("WIN32_module_verifier: Failed to query WindowsDirectory"), false, true);
		}

		const std::wstring_view windir_wsv = windir;

		for (const auto& module : special_module_infos)
		{
			const HMODULE hModule = GetModuleHandle(module.name.data());
			if (hModule == NULL)
			{
				continue;
			}

			WCHAR wpath[MAX_PATH];
			if (const auto len = GetModuleFileName(hModule, wpath, MAX_PATH))
			{
				const std::wstring_view path_wsv = wpath;
				if (path_wsv.find(windir_wsv) != 0)
				{
					const std::string path = wchar_to_utf8(wpath);
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
					report_fatal_error(error_message, true, false);
				}
			}
		}
	}

	WIN32_module_verifier() = default;

public:

	static void run()
	{
		WIN32_module_verifier verifier{};
		verifier.run_module_verification();
	}
};

#endif // _WIN32
