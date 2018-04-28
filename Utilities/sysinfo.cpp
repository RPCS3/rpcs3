#include "sysinfo.h"
#include "StrFmt.h"
#include <QMessageBox>

#ifdef _WIN32
#include "windows.h"
#else
#include <unistd.h>
#endif

bool utils::has_ssse3()
{
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x1 && get_cpuid(1, 0)[2] & 0x200;
	return g_value;
}

bool utils::has_sse41()
{
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x1 && get_cpuid(1, 0)[2] & 0x80000;
	return g_value;
}

bool utils::has_avx()
{
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x1 && get_cpuid(1, 0)[2] & 0x10000000 && (get_cpuid(1, 0)[2] & 0x0C000000) == 0x0C000000 && (get_xgetbv(0) & 0x6) == 0x6;
	return g_value;
}

bool utils::has_avx2()
{
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x7 && get_cpuid(7, 0)[1] & 0x20 && (get_cpuid(1, 0)[2] & 0x0C000000) == 0x0C000000 && (get_xgetbv(0) & 0x6) == 0x6;
	return g_value;
}

bool utils::has_rtm()
{
	// Check RTM
	static bool g_value = get_cpuid(0, 0)[0] >= 0x7 && (get_cpuid(7, 0)[1] & 0x800) == 0x800;
	
	// Do these checks only if TSX is available and only once
	static bool RTMChecked = !g_value;
	if (RTMChecked)
	{
		return g_value;
	}
	else
	{
		// Get CPU Model
		int Model = (get_cpuid(1, 0)[0] >> 4) & 0xf;
		Model += ((get_cpuid(1, 0)[0] >> 16) & 0xf) << 4;
		
		// If CPU is Haswell, display warning Message
		if (Model != 0x3c && Model != 0x3f && Model != 0x45 && Model != 0x46)
		{
			QMessageBox msg;
			msg.setWindowTitle(tr("Haswell TSX Warning"));
			msg.setIcon(QMessageBox::Critical);
			msg.setTextFormat(Qt::RichText);
			msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
			msg.setDefaultButton(QMessageBox::No);
			msg.setText(QString(tr(
				R"(
					<p style="white-space: nowrap;">
						RPCS3 has detected you're using TSX functions on a Haswell CPU.<br>
						Intel has deactivated these functions on your CPU in newer Microcode revisions, because using them leads to unpredicted behaviour.<br>
						That means using TSX may break games, or even <b>damage</b> your data.<br>
						We recommend to disable this feature and update your computer BIOS.<br><br>
						Do you wish to use TSX anyway?
					</p>
				)"
			)).arg(Qt::convertFromPlainText(STRINGIZE(BRANCH))));
			msg.layout()->setSizeConstraint(QLayout::SetFixedSize);

			if (msg.exec() != QMessageBox::Yes)
			{
				g_value = !g_value;
			}
			RTMChecked = !RTMChecked;
		}
	}
	return g_value;
}

bool utils::has_512()
{
	// Check AVX512F, AVX512CD, AVX512DQ, AVX512BW, AVX512VL extensions (Skylake-X level support)
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x7 && (get_cpuid(7, 0)[1] & 0xd0030000) == 0xd0030000 && (get_cpuid(1,0)[2] & 0x0C000000) == 0x0C000000 && (get_xgetbv(0) & 0xe6) == 0xe6;
	return g_value;
}

bool utils::has_xop()
{
	static const bool g_value = has_avx() && get_cpuid(0x80000001, 0)[2] & 0x800;
	return g_value;
}

std::string utils::get_system_info()
{
	std::string result;
	std::string brand;

	if (get_cpuid(0x80000000, 0)[0] >= 0x80000004)
	{
		for (u32 i = 0; i < 3; i++)
		{
			brand.append(reinterpret_cast<const char*>(get_cpuid(0x80000002 + i, 0).data()), 16);
		}
	}
	else
	{
		brand = "Unknown CPU";
	}

	brand.erase(0, brand.find_first_not_of(' '));
	brand.erase(brand.find_last_not_of(' ') + 1);

	while (auto found = brand.find("  ") + 1)
	{
		brand.erase(brand.begin() + found);
	}

#ifdef _WIN32
	::SYSTEM_INFO sysInfo;
	::GetNativeSystemInfo(&sysInfo);
	::MEMORYSTATUSEX memInfo;
	memInfo.dwLength = sizeof(memInfo);
	::GlobalMemoryStatusEx(&memInfo);
	const u32 num_proc = sysInfo.dwNumberOfProcessors;
	const u64 mem_total = memInfo.ullTotalPhys;
#else
	const u32 num_proc = ::sysconf(_SC_NPROCESSORS_ONLN);
	const u64 mem_total = ::sysconf(_SC_PHYS_PAGES) * ::sysconf(_SC_PAGE_SIZE);
#endif

	fmt::append(result, "%s | %d Threads | %.2f GiB RAM", brand, num_proc, mem_total / (1024.0f * 1024 * 1024));

	if (has_avx())
	{
		result += " | AVX";

		if (has_avx2())
		{
			result += '+';
		}

		if (has_512())
		{
			result += '+';
		}

		if (has_xop())
		{
			result += 'x';
		}
	}

	if (has_rtm())
	{
		result += " | TSX";
	}

	return result;
}
