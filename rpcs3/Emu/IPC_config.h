#pragma once

#include "Utilities/Config.h"

struct cfg_ipc : cfg::node
{
	cfg::_bool ipc_server_enabled{ this, "IPC Server enabled", false };
	cfg::_int<1025, 65535> ipc_port{ this, "IPC Port", 28012 };

	void load();
	void save() const;

	bool get_server_enabled() const;
	int get_port() const;

	void set_server_enabled(const bool enabled);
	void set_port(const int port);

private:
	static std::string get_path();
};

extern cfg_ipc g_cfg_ipc;
