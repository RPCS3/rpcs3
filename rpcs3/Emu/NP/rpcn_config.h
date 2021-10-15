#pragma once

#include "Utilities/Config.h"

struct cfg_rpcn : cfg::node
{
	cfg::string host{this, "Host", "np.rpcs3.net"};
	cfg::string npid{this, "NPID", ""};
	cfg::string password{this, "Password", ""};
	cfg::string token{this, "Token", ""};

	void load();
	void save() const;

	std::string get_host() const;
	std::string get_npid(); // not const because it can save if npid is requested and it has never been set
	std::string get_password() const;
	std::string get_token() const;

	void set_host(const std::string& host);
	void set_npid(const std::string& npid);
	void set_password(const std::string& password);
	void set_token(const std::string& token);

	private:
	static std::string get_path();
	static std::string generate_npid();
};

extern cfg_rpcn g_cfg_rpcn;
