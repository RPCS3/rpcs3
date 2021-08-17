#pragma once

#include "util/types.hpp"
#include <unordered_map>

namespace vk
{
	// Chip classes grouped by vendor in order of release
	enum class chip_class
	{
		unknown,
		AMD_gcn_generic,
		AMD_polaris,
		AMD_vega,
		AMD_navi1x,
		AMD_navi2x,
		NV_generic,
		NV_kepler,
		NV_maxwell,
		NV_pascal,
		NV_volta,
		NV_turing,
		NV_ampere
	};

	enum class driver_vendor
	{
		unknown,
		AMD,
		NVIDIA,
		RADV,
		INTEL,
		ANV
	};

	driver_vendor get_driver_vendor();

	struct chip_family_table
	{
		chip_class default_ = chip_class::unknown;
		std::unordered_map<u32, chip_class> lut;

		void add(u32 first, u32 last, chip_class family);
		void add(u32 id, chip_class family);

		chip_class find(u32 device_id) const;
	};

	chip_class get_chip_family();
	chip_class get_chip_family(u32 vendor_id, u32 device_id);
}
