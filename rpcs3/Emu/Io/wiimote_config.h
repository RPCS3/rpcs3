#pragma once

#include "Utilities/Config.h"

struct cfg_wiimote final : cfg::node
{
	cfg_wiimote();
	bool load();
	void save() const;

	struct node_mapping : cfg::node
	{
		node_mapping(cfg::node* _parent) : cfg::node(_parent, "Mapping") {}

		cfg::uint<0, 0xFFFF> trigger{ this, "Trigger", 0x0400 };
		cfg::uint<0, 0xFFFF> a1{ this, "A1", 0x0800 };
		cfg::uint<0, 0xFFFF> a2{ this, "A2", 0x1000 };
		cfg::uint<0, 0xFFFF> a3{ this, "A3", 0x0001 };
		cfg::uint<0, 0xFFFF> b1{ this, "B1", 0x0200 };
		cfg::uint<0, 0xFFFF> b2{ this, "B2", 0x0100 };
		cfg::uint<0, 0xFFFF> b3{ this, "B3", 0x8000 };
		cfg::uint<0, 0xFFFF> c1{ this, "C1", 0x0010 };
		cfg::uint<0, 0xFFFF> c2{ this, "C2", 0x0002 };

		cfg::uint<0, 0xFFFF> b1_alt{ this, "B1_Alt", 0x0008 };
		cfg::uint<0, 0xFFFF> b2_alt{ this, "B2_Alt", 0x0004 };
	} mapping{ this };

	const std::string path;
};

cfg_wiimote& get_wiimote_config();
