#pragma once

#include "util/types.hpp"
#include <unordered_map>

namespace vk
{
	// Chip classes grouped by vendor in order of release
	enum class chip_class
	{
		unknown,

		// AMD
		AMD_gcn_generic,
		AMD_polaris,
		AMD_vega,
		AMD_navi1x,
		AMD_navi2x,
		AMD_navi3x,
		_AMD_ENUM_MAX_, // Do not insert AMD enums beyond this point

		// NVIDIA
		NV_generic,
		NV_kepler,
		NV_maxwell,
		NV_pascal,
		NV_volta,
		NV_turing,
		NV_ampere,
		NV_lovelace,
		_NV_ENUM_MAX_, // Do not insert NV enums beyond this point

		// APPLE
		APPLE_HK_generic,
		APPLE_MVK,
		_APPLE_ENUM_MAX, // Do not insert APPLE enums beyond this point

		// INTEL
		INTEL_generic,
		INTEL_alchemist,
		_INTEL_ENUM_MAX_, // Do not insert INTEL enums beyond this point
	};

	enum class driver_vendor
	{
		unknown,
		AMD,
		NVIDIA,
		RADV,
		INTEL,
		ANV,
		MVK,
		DOZEN,
		LAVAPIPE,
		NVK,
		V3DV,
		HONEYKRISP,
		PANVK,
		ARM_MALI
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

	static inline bool is_NVIDIA(chip_class chip) { return chip >= chip_class::NV_generic && chip < chip_class::_NV_ENUM_MAX_; }
	static inline bool is_AMD(chip_class chip) { return chip >= chip_class::AMD_gcn_generic && chip < chip_class::_AMD_ENUM_MAX_; }
	static inline bool is_INTEL(chip_class chip) { return chip >= chip_class::INTEL_generic && chip < chip_class::_INTEL_ENUM_MAX_; }

	static inline bool is_NVIDIA(driver_vendor vendor) { return vendor == driver_vendor::NVIDIA || vendor == driver_vendor::NVK; }
	static inline bool is_AMD(driver_vendor vendor) { return vendor == driver_vendor::AMD || vendor == driver_vendor::RADV; }
	static inline bool is_INTEL(driver_vendor vendor) { return vendor == driver_vendor::INTEL || vendor == driver_vendor::ANV; }
}
