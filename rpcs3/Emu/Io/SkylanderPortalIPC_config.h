#pragma once

#include "Utilities/Config.h"

struct cfg_sky_ipc : cfg::node
{
	cfg::_bool sky_ipc_server_enabled{ this, "Skylanders IPC Server enabled", false };
	cfg::_int<1025, 65535> sky_ipc_port{ this, "Skylanders IPC Port", 28013 };

	void load();
	void save() const;

	bool get_server_enabled() const;
	int get_port() const;

	void set_server_enabled(const bool enabled);
	void set_port(const int port);

private:
	static std::string get_path();
};

extern cfg_sky_ipc g_cfg_sky_ipc;
