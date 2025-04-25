#pragma once

#include "Utilities/Config.h"

struct cfg_rpcn : cfg::node
{
	cfg::uint32 version{this, "Version", 1};
	cfg::string host{this, "Host", "np.rpcs3.net"};
	cfg::string npid{this, "NPID", ""};
	cfg::string password{this, "Password", ""};
	cfg::string token{this, "Token", ""};
	cfg::string hosts{this, "Hosts", "Official RPCN Server|np.rpcs3.net|||RPCN Test Server|test-np.rpcs3.net"};
	cfg::_bool ipv6_support{this, "IPv6 support", true};

	void load();
	void save() const;

	std::string get_host() const;
	std::string get_npid(); // not const because it can save if npid is requested and it has never been set
	std::string get_password() const;
	std::string get_token() const;
	bool get_ipv6_support() const;
	std::vector<std::pair<std::string, std::string>> get_hosts(); // saves default if no valid server in the list

	void set_host(std::string_view host);
	void set_npid(std::string_view npid);
	void set_password(std::string_view password);
	void set_token(std::string_view token);
	void set_ipv6_support(bool ipv6_support);
	bool add_host(std::string_view description, std::string_view host);
	bool del_host(std::string_view description, std::string_view host);

private:
	static std::string get_path();
	static std::string generate_npid();
	void set_hosts(const std::vector<std::pair<std::string, std::string>>& vec_hosts);
};

std::optional<std::pair<std::string, u16>> parse_rpcn_host(std::string_view host);

extern cfg_rpcn g_cfg_rpcn;
