#pragma once
#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>

#include <util/types.hpp>
#include <Utilities/StrUtil.h>
#include <Utilities/StrFmt.h>

[[noreturn]] void report_fatal_error(std::string_view, bool);

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
		{ L"msvcp140.dll", "C++ Redistributable for Visual Studio 2015", "https://www.microsoft.com/en-us/download/details.aspx?id=48145" },
		{ L"vcruntime140.dll", "C++ Redistributable for Visual Studio 2015", "https://www.microsoft.com/en-us/download/details.aspx?id=48145" },
		{ L"msvcp140_1.dll", "C++ Redistributable for Visual Studio 2015", "https://www.microsoft.com/en-us/download/details.aspx?id=48145" },
		{ L"vcruntime140_1.dll", "C++ Redistributable for Visual Studio 2015", "https://www.microsoft.com/en-us/download/details.aspx?id=48145" }
	};

	// Unless we support ReactOS in future, this is a constant
	const std::wstring_view system_root = L"C:\\WINDOWS\\";

	// Inline impl. This class is only referenced once.
	void run_module_verification()
	{
		for (const auto& module : special_module_infos)
		{
			const auto hModule = GetModuleHandle(module.name.data());
			if (hModule == NULL)
			{
				continue;
			}

			WCHAR path[MAX_PATH];
			if (const auto len = GetModuleFileName(hModule, path, MAX_PATH))
			{
				const std::wstring_view s = path;
				if (s.find(system_root) != 0)
				{
					const auto error_message = fmt::format(
						"<p>"
						"The module '%s' was incorrectly installed.<br>"
						"This module is part of the '%s' package.<br>"
						"You can install this package from this URL:<br>"
						"<a href='%s'>%s</a>"
						"</p>",
						wchar_to_utf8(module.name),
						module.package_name,
						module.dl_link,
						module.dl_link
					);
					report_fatal_error(error_message, true);
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
