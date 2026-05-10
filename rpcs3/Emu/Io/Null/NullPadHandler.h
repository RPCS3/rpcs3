#pragma once

#include "Emu/Io/PadHandler.h"

class NullPadHandler final : public PadHandlerBase
{
public:
	NullPadHandler() : PadHandlerBase(pad_handler::null)
	{
		b_has_pressure_intensity_button = false;
	}

	void init_config(cfg_pad* cfg) override
	{
		if (!cfg) return;

		// Reset default button mapping
		cfg->ls_left.def  = "";
		cfg->ls_down.def  = "";
		cfg->ls_right.def = "";
		cfg->ls_up.def    = "";
		cfg->rs_left.def  = "";
		cfg->rs_down.def  = "";
		cfg->rs_right.def = "";
		cfg->rs_up.def    = "";
		cfg->start.def    = "";
		cfg->select.def   = "";
		cfg->ps.def       = "";
		cfg->square.def   = "";
		cfg->cross.def    = "";
		cfg->circle.def   = "";
		cfg->triangle.def = "";
		cfg->left.def     = "";
		cfg->down.def     = "";
		cfg->right.def    = "";
		cfg->up.def       = "";
		cfg->r1.def       = "";
		cfg->r2.def       = "";
		cfg->r3.def       = "";
		cfg->l1.def       = "";
		cfg->l2.def       = "";
		cfg->l3.def       = "";

		cfg->pressure_intensity_button.def = "";
		cfg->analog_limiter_button.def = "";
		cfg->orientation_reset_button.def = "";

		// Apply defaults
		cfg->from_default();
	}

	std::vector<pad_list_entry> list_devices() override
	{
		std::vector<pad_list_entry> nulllist;
		nulllist.emplace_back("Default Null Device", false);
		return nulllist;
	}

	bool bindPadToDevice(std::shared_ptr<Pad> /*pad*/) override
	{
		return true;
	}

	void process() override
	{
	}
};
