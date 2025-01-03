#include "stdafx.h"
#include "IPC_config.h"

cfg_ipc g_cfg_ipc;

LOG_CHANNEL(IPC);

void cfg_ipc::load()
{
	const std::string path = cfg_ipc::get_path();

	fs::file cfg_file(path, fs::read);
	if (cfg_file)
	{
		IPC.notice("Loading IPC config. Path: %s", path);
		from_string(cfg_file.to_string());
	}
	else
	{
		IPC.notice("IPC config missing. Using default settings. Path: %s", path);
		from_default();
	}
}

void cfg_ipc::save() const
{
#ifdef _WIN32
	const std::string path_to_cfg = fs::get_config_dir(true);
	if (!fs::create_path(path_to_cfg))
	{
		IPC.error("Could not create path: %s", path_to_cfg);
	}
#endif

	const std::string path = cfg_ipc::get_path();

	if (!cfg::node::save(path))
	{
		IPC.error("Could not save config: %s (error=%s)", path, fs::g_tls_error);
	}
}

std::string cfg_ipc::get_path()
{
	return fs::get_config_dir(true) + "ipc.yml";
}

bool cfg_ipc::get_server_enabled() const
{
	return ipc_server_enabled.get();
}

int cfg_ipc::get_port() const
{
	return ipc_port;
}

void cfg_ipc::set_server_enabled(const bool enabled)
{
	this->ipc_server_enabled.set(enabled);
}

void cfg_ipc::set_port(const int port)
{
	this->ipc_port.set(port);
}
