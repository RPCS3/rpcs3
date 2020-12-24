#include "chip_class.h"
#include "util/logs.hpp"

namespace vk
{
	chip_class g_chip_class = chip_class::unknown;

	chip_class get_chip_family()
	{
		return g_chip_class;
	}

	chip_class get_chip_family(u32 vendor_id, u32 device_id)
	{
		if (vendor_id == 0x10DE)
		{
			return s_NV_family_tree.find(device_id);
		}

		if (vendor_id == 0x1002)
		{
			return s_AMD_family_tree.find(device_id);
		}

		return chip_class::unknown;
	}

	void chip_family_table::add(u32 first, u32 last, chip_class family)
	{
		for (auto i = first; i <= last; ++i)
		{
			lut[i] = family;
		}
	}

	void chip_family_table::add(u32 id, chip_class family)
	{
		lut[id] = family;
	}

	chip_class chip_family_table::find(u32 device_id)
	{
		if (auto found = lut.find(device_id); found != lut.end())
		{
			return found->second;
		}

		rsx_log.warning("Unknown chip with device ID 0x%x", device_id);
		return default_;
	}
}
