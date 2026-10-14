#include "stdafx.h"
#include "SkylanderPortalIPC_config.h"

cfg_sky_ipc g_cfg_sky_ipc;

LOG_CHANNEL(skylander_ipc_log, "SkylanderIPC");

void cfg_sky_ipc::load()
{
	const std::string path = cfg_sky_ipc::get_path();

	fs::file cfg_file(path, fs::read);
	if (cfg_file)
	{
		skylander_ipc_log.notice("Loading Skylanders IPC config. Path: %s", path);
		from_string(cfg_file.to_string());
	}
	else
	{
		skylander_ipc_log.notice("Skylanders IPC config missing. Using default settings. Path: %s", path);
		from_default();
	}
}

void cfg_sky_ipc::save() const
{
#ifdef _WIN32
	const std::string path_to_cfg = fs::get_config_dir(true);
	if (!fs::create_path(path_to_cfg))
	{
		skylander_ipc_log.error("Could not create path: %s", path_to_cfg);
	}
#endif

	const std::string path = cfg_sky_ipc::get_path();

	if (!cfg::node::save(path))
	{
		skylander_ipc_log.error("Could not save config: %s (error=%s)", path, fs::g_tls_error);
	}
}

std::string cfg_sky_ipc::get_path()
{
	return fs::get_config_dir(true) + "sky_ipc.yml";
}

bool cfg_sky_ipc::get_server_enabled() const
{
	return sky_ipc_server_enabled.get();
}

int cfg_sky_ipc::get_port() const
{
	return sky_ipc_port;
}

void cfg_sky_ipc::set_server_enabled(const bool enabled)
{
	this->sky_ipc_server_enabled.set(enabled);
}

void cfg_sky_ipc::set_port(const int port)
{
	this->sky_ipc_port.set(port);
}
