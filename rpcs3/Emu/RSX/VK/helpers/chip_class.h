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
		INTEL
	};

	struct chip_family_table
	{
		chip_class default_ = chip_class::unknown;
		std::unordered_map<u32, chip_class> lut;

		void add(u32 first, u32 last, chip_class family);
		void add(u32 id, chip_class family);

		chip_class find(u32 device_id);
	};

	static chip_family_table s_AMD_family_tree = []()
	{
		chip_family_table table;
		table.default_ = chip_class::AMD_gcn_generic;

		// AMD cards. See https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/amd/amdgpu/amdgpu_drv.c
		table.add(0x67C0, 0x67FF, chip_class::AMD_polaris);
		table.add(0x6FDF, chip_class::AMD_polaris); // RX580 2048SP
		table.add(0x6980, 0x699F, chip_class::AMD_polaris); // Polaris12
		table.add(0x694C, 0x694F, chip_class::AMD_vega); // VegaM
		table.add(0x6860, 0x686F, chip_class::AMD_vega); // VegaPro
		table.add(0x687F, chip_class::AMD_vega); // Vega56/64
		table.add(0x69A0, 0x69AF, chip_class::AMD_vega); // Vega12
		table.add(0x66A0, 0x66AF, chip_class::AMD_vega); // Vega20
		table.add(0x15DD, chip_class::AMD_vega); // Raven Ridge
		table.add(0x15D8, chip_class::AMD_vega); // Raven Ridge
		table.add(0x7310, 0x731F, chip_class::AMD_navi1x); // Navi10
		table.add(0x7340, 0x734F, chip_class::AMD_navi1x); // Navi14
		table.add(0x73A0, 0x73BF, chip_class::AMD_navi2x); // Sienna cichlid

		return table;
	}();

	static chip_family_table s_NV_family_tree = []()
	{
		chip_family_table table;
		table.default_ = chip_class::NV_generic;

		// NV cards. See https://envytools.readthedocs.io/en/latest/hw/pciid.html
		// NOTE: Since NV device IDs are linearly incremented per generation, there is no need to carefully check all the ranges
		table.add(0x1180, 0x11fa, chip_class::NV_kepler); // GK104, 106
		table.add(0x0FC0, 0x0FFF, chip_class::NV_kepler); // GK107
		table.add(0x1003, 0x1028, chip_class::NV_kepler); // GK110
		table.add(0x1280, 0x12BA, chip_class::NV_kepler); // GK208
		table.add(0x1381, 0x13B0, chip_class::NV_maxwell); // GM107
		table.add(0x1340, 0x134D, chip_class::NV_maxwell); // GM108
		table.add(0x13C0, 0x13D9, chip_class::NV_maxwell); // GM204
		table.add(0x1401, 0x1427, chip_class::NV_maxwell); // GM206
		table.add(0x15F7, 0x15F9, chip_class::NV_pascal); // GP100 (Tesla P100)
		table.add(0x1B00, 0x1D80, chip_class::NV_pascal);
		table.add(0x1D81, 0x1DBA, chip_class::NV_volta);
		table.add(0x1E02, 0x1F54, chip_class::NV_turing); // TU102, TU104, TU106, TU106M, TU106GL (RTX 20 series)
		table.add(0x1F82, 0x1FB9, chip_class::NV_turing); // TU117, TU117M, TU117GL
		table.add(0x2182, 0x21D1, chip_class::NV_turing); // TU116, TU116M, TU116GL
		table.add(0x20B0, 0x20BE, chip_class::NV_ampere); // GA100
		table.add(0x2204, 0x25AF, chip_class::NV_ampere); // GA10x (RTX 30 series)

		return table;
	}();

	chip_class get_chip_family();
	chip_class get_chip_family(u32 vendor_id, u32 device_id);
}
