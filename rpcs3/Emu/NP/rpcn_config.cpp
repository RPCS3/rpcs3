#include "stdafx.h"
#include "rpcn_config.h"
#include "Emu/System.h"

cfg_rpcn g_cfg_rpcn;

LOG_CHANNEL(rpcn_cfg_log, "rpcn config");

void cfg_rpcn::load()
{
	fs::file cfg_file(cfg_rpcn::get_path(), fs::read);
	if (cfg_file)
	{
		from_string(cfg_file.to_string());
	}
}

void cfg_rpcn::save() const
{
#ifdef _WIN32
	const std::string path_to_cfg = fs::get_config_dir() + "config/";
	if (!fs::create_path(path_to_cfg))
	{
		rpcn_cfg_log.error("Could not create path: %s", path_to_cfg);
	}
#endif

	fs::file cfg_file(cfg_rpcn::get_path(), fs::rewrite);
	if (cfg_file)
	{
		cfg_file.write(to_string());	
	}
	else
	{
		rpcn_cfg_log.error("Could not save config: %s", cfg_rpcn::get_path());
	}
}

std::string cfg_rpcn::get_path()
{
#ifdef _WIN32
	return fs::get_config_dir() + "config/rpcn.yml";
#else
	return fs::get_config_dir() + "rpcn.yml";
#endif
}

std::string cfg_rpcn::generate_npid()
{
	std::string gen_npid = "RPCS3_";

	const char list_chars[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};

	std::srand(time(nullptr));

	for (int i = 0; i < 10; i++)
	{
		gen_npid += list_chars[std::rand() % (sizeof(list_chars))];
	}

	return gen_npid;
}

std::string cfg_rpcn::get_host() const
{
	return host.to_string();
}

std::string cfg_rpcn::get_npid()
{
	std::string final_npid = npid.to_string();
	if (final_npid == "")
	{
		final_npid = cfg_rpcn::generate_npid();
		save();
	}

	return final_npid;
}

std::string cfg_rpcn::get_password() const
{
	return password.to_string();
}

std::string cfg_rpcn::get_token() const
{
	return token.to_string();
}

void cfg_rpcn::set_host(const std::string& host)
{
	this->host.from_string(host);
}

void cfg_rpcn::set_npid(const std::string& npid)
{
	this->npid.from_string(npid);
}

void cfg_rpcn::set_password(const std::string& password)
{
	this->password.from_string(password);
}

void cfg_rpcn::set_token(const std::string& token)
{
	this->token.from_string(token);
}
