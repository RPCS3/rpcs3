#include "chip_class.h"
#include "util/logs.hpp"

namespace vk
{
	static const chip_family_table s_AMD_family_tree = []()
	{
		chip_family_table table;
		table.default_ = chip_class::AMD_gcn_generic;

		// AMD cards. See https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/amd/amdgpu/amdgpu_drv.c
		table.add(0x67C0, 0x67FF, chip_class::AMD_polaris); // Polaris 10/11
		table.add(0x6FDF,         chip_class::AMD_polaris); // RX 580 2048SP
		table.add(0x6980, 0x699F, chip_class::AMD_polaris); // Polaris 12
		table.add(0x694C, 0x694F, chip_class::AMD_vega);    // Vega M
		table.add(0x6860, 0x686F, chip_class::AMD_vega);    // Vega Pro
		table.add(0x687F,         chip_class::AMD_vega);    // Vega 56/64
		table.add(0x69A0, 0x69AF, chip_class::AMD_vega);    // Vega 12
		table.add(0x66A0, 0x66AF, chip_class::AMD_vega);    // Vega 20
		table.add(0x15DD,         chip_class::AMD_vega);    // Raven Ridge
		table.add(0x15D8,         chip_class::AMD_vega);    // Raven Ridge
		table.add(0x7310, 0x731F, chip_class::AMD_navi1x);  // Navi 10
		table.add(0x7360, 0x7362, chip_class::AMD_navi1x);  // Navi 12
		table.add(0x7340, 0x734F, chip_class::AMD_navi1x);  // Navi 14
		table.add(0x73A0, 0x73BF, chip_class::AMD_navi2x);  // Navi 21 (Sienna Cichlid)
		table.add(0x73C0, 0x73DF, chip_class::AMD_navi2x);  // Navi 22 (Navy Flounder)
		table.add(0x73E0, 0x73FF, chip_class::AMD_navi2x);  // Navi 23 (Dimgrey Cavefish)
		table.add(0x7420, 0x743F, chip_class::AMD_navi2x);  // Navi 24 (Beige Goby)
		table.add(0x163F,         chip_class::AMD_navi2x);  // Navi 2X (Van Gogh)
		table.add(0x164D, 0x1681, chip_class::AMD_navi2x);  // Navi 2X (Yellow Carp)

		return table;
	}();

	static const chip_family_table s_NV_family_tree = []()
	{
		chip_family_table table;
		table.default_ = chip_class::NV_generic;

		// NV cards. See https://envytools.readthedocs.io/en/latest/hw/pciid.html
		// NOTE: Since NV device IDs are linearly incremented per generation, there is no need to carefully check all the ranges
		table.add(0x1180, 0x11FA, chip_class::NV_kepler);  // GK104, 106
		table.add(0x0FC0, 0x0FFF, chip_class::NV_kepler);  // GK107
		table.add(0x1003, 0x102F, chip_class::NV_kepler);  // GK110, GK210
		table.add(0x1280, 0x12BA, chip_class::NV_kepler);  // GK208
		table.add(0x1381, 0x13B0, chip_class::NV_maxwell); // GM107
		table.add(0x1340, 0x134D, chip_class::NV_maxwell); // GM108
		table.add(0x13C0, 0x13D9, chip_class::NV_maxwell); // GM204
		table.add(0x1401, 0x1427, chip_class::NV_maxwell); // GM206
		table.add(0x15F7, 0x15F9, chip_class::NV_pascal);  // GP100 (Tesla P100)
		table.add(0x1B00, 0x1D80, chip_class::NV_pascal);
		table.add(0x1D81, 0x1DBA, chip_class::NV_volta);
		table.add(0x1E02, 0x1F54, chip_class::NV_turing);  // TU102, TU104, TU106, TU106M, TU106GL (RTX 20 series)
		table.add(0x1F82, 0x1FB9, chip_class::NV_turing);  // TU117, TU117M, TU117GL
		table.add(0x2182, 0x21D1, chip_class::NV_turing);  // TU116, TU116M, TU116GL
		table.add(0x20B0, 0x20BE, chip_class::NV_ampere);  // GA100
		table.add(0x2204, 0x25AF, chip_class::NV_ampere);  // GA10x (RTX 30 series)

		return table;
	}();

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

	chip_class chip_family_table::find(u32 device_id) const
	{
		if (auto found = lut.find(device_id); found != lut.end())
		{
			return found->second;
		}

		rsx_log.warning("Unknown chip with device ID 0x%x", device_id);
		return default_;
	}
}
