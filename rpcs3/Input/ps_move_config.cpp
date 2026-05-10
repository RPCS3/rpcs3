#include "stdafx.h"
#include "ps_move_config.h"

LOG_CHANNEL(ps_move);

cfg_ps_moves g_cfg_move;

cfg_ps_moves::cfg_ps_moves()
	: cfg::node()
	, path(fs::get_config_dir(true) + "ps_move.yml")
{
}

bool cfg_ps_moves::load()
{
	ps_move.notice("Loading PS Move config from '%s'", path);

	if (fs::file cfg_file{ path, fs::read })
	{
		return from_string(cfg_file.to_string());
	}

	ps_move.notice("PS Move config missing. Using default settings. Path: %s", path);
	from_default();
	return false;
}

void cfg_ps_moves::save() const
{
	ps_move.notice("Saving PS Move config to '%s'", path);

	if (!cfg::node::save(path))
	{
		ps_move.error("Failed to save PS Move config to '%s' (error=%s)", path, fs::g_tls_error);
	}
}
