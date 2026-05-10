#include "stdafx.h"
#include "AArch64Common.h"

#include <thread>
#include <map>

#if defined(__APPLE__)
#include <sys/sysctl.h>
#endif

namespace aarch64
{
#if !defined(__APPLE__)
    struct cpu_entry_t
    {
        u32 vendor;
        u32 part;
        const char* arch;
        const char* family;
        const char* name;
    };

    struct cpu_vendor_t
    {
        u32 id;
        const char* name;
        const char* short_name;
    };

    static cpu_vendor_t s_vendors_list[] =
    {
        { 0x41, "Arm Limited.", "ARM" },
        { 0x42, "Broadcom Corporation.", "Broadcom" },
        { 0x43, "Cavium Inc.", "Cavium" },
        { 0x44, "Digital Equipment Corporation.", "DEC" },
        { 0x46, "Fujitsu Ltd.", "Fujitsu" },
        { 0x49, "Infineon Technologies AG.", "Infineon" },
        { 0x4D, "Motorola or Freescale Semiconductor Inc.", "Motorola" },
        { 0x4E, "NVIDIA Corporation.", "NVIDIA" },
        { 0x50, "Applied Micro Circuits Corporation.", "AMCC" },
        { 0x51, "Qualcomm Inc.", "Qualcomm" },
        { 0x56, "Marvell International Ltd.", "Marvell" },
        { 0x69, "Intel Corporation.", "Intel" },
        { 0xC0, "Ampere Computing", "Ampere" },

        // Unofficial but existing in the wild
        { 0x61, "Apple Inc.", "Apple" },
    };

    static cpu_entry_t s_cpu_list[] =
    {
        // ARM
        { 0x41, 0xd01, "armv8-a+crc+simd", "", "Cortex-A32" },
        { 0x41, 0xd04, "armv8-a+crc+simd", "", "Cortex-A35" },
        { 0x41, 0xd03, "armv8-a+crc+simd", "", "Cortex-A53" },
        { 0x41, 0xd07, "armv8-a+crc+simd", "", "Cortex-A57" },
        { 0x41, 0xd08, "armv8-a+crc+simd", "", "Cortex-A72" },
        { 0x41, 0xd09, "armv8-a+crc+simd", "", "Cortex-A73" },
        { 0x41, 0xd05, "armv8.2-a+fp16+dotprod", "", "Cortex-A55" },
        { 0x41, 0xd0a, "armv8.2-a+fp16+dotprod", "", "Cortex-A75" },
        { 0x41, 0xd0b, "armv8.2-a+fp16+dotprod", "", "Cortex-A76" },
        { 0x41, 0xd0e, "armv8.2-a+fp16+dotprod", "", "Cortex-A76ae" },
        { 0x41, 0xd0d, "armv8.2-a+fp16+dotprod", "", "Cortex-A77" },
        { 0x41, 0xd41, "armv8.2-a+fp16+dotprod", "", "Cortex-A78" },
        { 0x41, 0xd42, "armv8.2-a+fp16+dotprod", "", "Cortex-A78ae" },
        { 0x41, 0xd4b, "armv8.2-a+fp16+dotprod", "", "Cortex-A78c" },
        { 0x41, 0xd47, "armv9-a+fp16+bf16+i8mm", "", "Cortex-A710" },
        { 0x41, 0xd44, "armv8.2-a+fp16+dotprod", "", "Cortex-X1" },
        { 0x41, 0xd4c, "armv8.2-a+fp16+dotprod", "", "Cortex-X1c" },
        { 0x41, 0xd0c, "armv8.2-a+fp16+dotprod", "", "Neoverse-N1" },
        { 0x41, 0xd40, "armv8.4-a+fp16+bf16+i8mm", "", "Neoverse-V1" },
        { 0x41, 0xd49, "armv8.5-a+fp16+bf16+i8mm", "", "Neoverse-N2" },
        { 0x41, 0xd23, "armv8.1-m.main+pacbti+mve.fp+fp.dp", "", "Cortex-M85" },
        { 0x41, 0xd13, "armv8-r+crc+simd", "", "Cortex-R52" },
        { 0x41, 0xd16, "armv8-r+crc+simd", "", "Cortex-R52+" },

        // APPLE
        { 0x61, 0x22, "armv8.5-a", "M1", "Firestorm" },
        { 0x61, 0x23, "armv8.5-a", "M1", "IceStorm" },
        { 0x61, 0x28, "armv8.5-a", "M1 Max", "Firestorm" },
        { 0x61, 0x29, "armv8.5-a", "M1 Max", "Icestorm" },
        { 0x61, 0x24, "armv8.5-a", "M1 Pro", "Firestorm" },
        { 0x61, 0x25, "armv8.5-a", "M1 Pro", "Icestorm" },
        { 0x61, 0x32, "armv8.5-a", "M2", "Avalanche" },
        { 0x61, 0x33, "armv8.5-a", "M2", "Blizzard" },

        // QUALCOMM
        { 0x51, 0x01, "armv8.5-a", "Snapdragon", "X-Elite" },
    };

    static const cpu_vendor_t* find_cpu_vendor(u64 id)
    {
        for (const auto& vendor : s_vendors_list)
        {
            if (id == vendor.id)
            {
                return &vendor;
            }
        }

        return nullptr;
    }

    static const cpu_entry_t* find_cpu_part(u64 vendor, u64 part)
    {
        for (const auto& cpu : s_cpu_list)
        {
            if (cpu.vendor == vendor && cpu.part == part)
            {
                return &cpu;
            }
        }

        return nullptr;
    }

    // Read main ID register
    static u64 read_MIDR_EL1([[maybe_unused]] u32 cpu_id)
    {
#if defined(__linux__)
        const std::string path = fmt::format("/sys/devices/system/cpu/cpu%u/regs/identification/midr_el1", cpu_id);
        if (!fs::is_file(path))
        {
            return umax;
        }

        std::string value;
        if (!fs::file(path, fs::read).read(value, 18))
        {
            return 0;
        }
        return std::stoull(value, nullptr, 16);
#else
        // Unimplemented
        return 0;
#endif
    }

    std::string get_cpu_name()
    {
        std::map<u64, int> core_layout;
        for (u32 i = 0; i < std::thread::hardware_concurrency(); ++i)
        {
            const auto midr = read_MIDR_EL1(i);
            if (midr == umax)
            {
                break;
            }

            core_layout[midr]++;
        }

        if (core_layout.empty())
        {
            return {};
        }

        const cpu_entry_t* lowest_part_info = nullptr; 
        for (const auto& [midr, count] : core_layout)
        {
            const auto implementer_id = (midr >> 24) & 0xff;
            const auto part_id = (midr >> 4) & 0xfff;

            const auto part_info = find_cpu_part(implementer_id, part_id);
            if (!part_info)
            {
                return {};
            }

            if (lowest_part_info == nullptr || lowest_part_info > part_info)
            {
                lowest_part_info = part_info;
            }
        }

        return lowest_part_info ? lowest_part_info->name : "";
    }

    std::string get_cpu_brand()
    {
        // Fetch vendor and part numbers. ARM CPUs often have more than 1 architecture on the SoC, so we check all of them.
        std::map<u64, int> core_layout;
        for (u32 i = 0; i < std::thread::hardware_concurrency(); ++i)
        {
            const auto midr = read_MIDR_EL1(i);
            if (midr == umax)
            {
                break;
            }

            core_layout[midr]++;
        }

        if (core_layout.empty())
        {
            return "Unidentified CPU";
        }

        std::string vendor_name;
        std::string part_family;
        std::vector<std::string> core_names;
        for (const auto& [midr, count] : core_layout)
        {
            const auto implementer_id = (midr >> 24) & 0xff;
            const auto part_id = (midr >> 4) & 0xfff;

            if (vendor_name.empty())
            {
                const auto vendor_info = find_cpu_vendor(implementer_id);
                vendor_name = vendor_info ? vendor_info->short_name : "Unknown";
            }

            const auto part_info = find_cpu_part(implementer_id, part_id);
            if (!part_info)
            {
                core_names.push_back(fmt::format("%dx\"Unidentified cores\"", count));
                continue;
            }

            if (part_family.empty() && part_info->family)
            {
                part_family = part_info->family;
            }

            core_names.push_back(fmt::format("%dx\"%s\"", count, part_info->name));
        }

        // Assemble everything
        std::string result = vendor_name + " ";
        std::string suffix;
        if (!part_family.empty())
        {
            // Since we have a known family name, the core layout is just extra info.
            // Wrap core layout in brackets.
            result += part_family + " (";
            suffix = ")";
        }
        result += fmt::merge(core_names, " + ");
        result += suffix;
        return result;
    }
#else
    static std::string sysctl_s(const std::string_view& variable_name)
    {
        // Determine required buffer size
        size_t length = 0;
        if (sysctlbyname(variable_name.data(), nullptr, &length, nullptr, 0) == -1)
        {
            return "";
        }

        // Allocate space for the variable.
        std::vector<char> text(length + 1);
        text[length] = 0;
        if (sysctlbyname(variable_name.data(), text.data(), &length, nullptr, 0) == -1)
        {
            return "";
        }

        return text.data();
    }

    static u64 sysctl_u64(const std::string_view& variable_name)
    {
        u64 value = 0;
        size_t data_len = sizeof(value);
        if (sysctlbyname(variable_name.data(), &value, &data_len, nullptr, 0) == -1)
        {
            return umax;
        }
        return value;
    }

    // We can get the brand name from sysctl directly
    // Once we have windows implemented, we should probably separate the different OS-dependent bits to avoid clutter
    std::string get_cpu_brand()
    {
        const auto brand = sysctl_s("machdep.cpu.brand_string");
        if (brand.empty())
        {
            return "Unidentified CPU";
        }

        // Parse extra core information (P and E cores)
        if (sysctl_u64("hw.nperflevels") < 2)
        {
            return brand;
        }

        u64 pcores = sysctl_u64("hw.perflevel0.physicalcpu");
        u64 ecores = sysctl_u64("hw.perflevel1.physicalcpu");

        if (sysctl_s("hw.perflevel0.name") == "Efficiency")
        {
            std::swap(ecores, pcores);
        }

        return fmt::format("%s (%lluP+%lluE)", brand, pcores, ecores);
    }
#endif
}
