#pragma once

#include "Utilities/Config.h"

struct cfg_rpcn : cfg::node
{
	cfg::uint32 version{this, "Version", 1};
	cfg::string host{this, "Host", "np.rpcs3.net"};
	cfg::string country{this, "Country", "us"};
	cfg::string npid{this, "NPID", ""};
	cfg::string password{this, "Password", ""};
	cfg::string token{this, "Token", ""};
	cfg::string hosts{this, "Hosts", "Official RPCN Server|np.rpcs3.net|||RPCN Test Server|test-np.rpcs3.net"};

	void load();
	void save() const;

	std::string get_host() const;
	std::string get_country() const;
	std::string get_npid(); // not const because it can save if npid is requested and it has never been set
	std::string get_password() const;
	std::string get_token() const;
	std::vector<std::pair<std::string, std::string>> get_hosts(); // saves default if no valid server in the list

	void set_host(std::string_view host);
	void set_country(std::string_view country);
	void set_npid(std::string_view npid);
	void set_password(std::string_view password);
	void set_token(std::string_view token);
	bool add_host(std::string_view description, std::string_view host);
	bool del_host(std::string_view description, std::string_view host);

	struct country_code
	{
		std::string name;
		std::string ccode;
	};
	std::vector<country_code> get_countries();

private:
	static std::string get_path();
	static std::string generate_npid();
	void set_hosts(const std::vector<std::pair<std::string, std::string>>& vec_hosts);
};

extern cfg_rpcn g_cfg_rpcn;
