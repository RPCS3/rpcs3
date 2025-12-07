#pragma once

#include "Utilities/Config.h"

struct cfg_clans : cfg::node
{
	cfg::uint32 version{this, "Version", 1};
	cfg::string host{this, "Host", "clans.rpcs3.net"};
	cfg::string hosts{this, "Hosts", "Official Clans Server|clans.rpcs3.net"};

	void load();
	void save() const;

	std::string get_host() const;
	std::vector<std::pair<std::string, std::string>> get_hosts();

	void set_host(std::string_view host);
	bool add_host(std::string_view description, std::string_view host);
	bool del_host(std::string_view description, std::string_view host);

private:
	static std::string get_path();
	void set_hosts(const std::vector<std::pair<std::string, std::string>>& vec_hosts);
};

extern cfg_clans g_cfg_clans;
