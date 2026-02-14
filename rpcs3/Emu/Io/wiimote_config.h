#pragma once

#include "Utilities/Config.h"
#include "Input/wiimote_types.h"

struct cfg_wiimote : cfg::node
{
	cfg_wiimote();
	bool load();
	void save() const;

	cfg::_bool use_for_guncon{ this, "UseForGunCon", true };

	struct node_mapping : cfg::node
	{
		node_mapping(cfg::node* _parent) : cfg::node(_parent, "Mapping") {}

		cfg::_enum<wiimote_button> trigger{ this, "Trigger", wiimote_button::B };
		cfg::_enum<wiimote_button> a1{ this, "A1", wiimote_button::A };
		cfg::_enum<wiimote_button> a2{ this, "A2", wiimote_button::Minus };
		cfg::_enum<wiimote_button> a3{ this, "A3", wiimote_button::Left };
		cfg::_enum<wiimote_button> b1{ this, "B1", wiimote_button::One };
		cfg::_enum<wiimote_button> b2{ this, "B2", wiimote_button::Two };
		cfg::_enum<wiimote_button> b3{ this, "B3", wiimote_button::Home };
		cfg::_enum<wiimote_button> c1{ this, "C1", wiimote_button::Plus };
		cfg::_enum<wiimote_button> c2{ this, "C2", wiimote_button::Right };

		cfg::_enum<wiimote_button> b1_alt{ this, "B1_Alt", wiimote_button::Up };
		cfg::_enum<wiimote_button> b2_alt{ this, "B2_Alt", wiimote_button::Down };
	} mapping{ this };

	const std::string path;
};

cfg_wiimote& get_wiimote_config();
