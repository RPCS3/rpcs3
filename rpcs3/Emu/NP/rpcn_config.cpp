#include "stdafx.h"
#include "rpcn_config.h"
#include "Utilities/File.h"

cfg_rpcn g_cfg_rpcn;

LOG_CHANNEL(rpcn_log, "rpcn");

void cfg_rpcn::load()
{
	const std::string path = cfg_rpcn::get_path();

	fs::file cfg_file(path, fs::read);
	if (cfg_file)
	{
		rpcn_log.notice("Loading RPCN config. Path: %s", path);
		from_string(cfg_file.to_string());
	}
	else
	{
		rpcn_log.notice("RPCN config missing. Using default settings. Path: %s", path);
		from_default();
	}

	// Update config from old version(s)
	if (version == 1)
	{
		password.from_string("");
		version.set(2);
		save();
	}
}

void cfg_rpcn::save() const
{
#ifdef _WIN32
	const std::string path_to_cfg = fs::get_config_dir(true);
	if (!fs::create_path(path_to_cfg))
	{
		rpcn_log.error("Could not create path: %s", path_to_cfg);
	}
#endif

	const std::string path = cfg_rpcn::get_path();

	if (!cfg::node::save(path))
	{
		rpcn_log.error("Could not save config: %s (error=%s)", path, fs::g_tls_error);
	}
}

std::string cfg_rpcn::get_path()
{
	return fs::get_config_dir(true) + "rpcn.yml";
}

std::string cfg_rpcn::generate_npid()
{
	std::string gen_npid = "RPCS3_";

	const char list_chars[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
		'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};

	std::srand(static_cast<u32>(time(nullptr)));

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

std::vector<std::pair<std::string, std::string>> cfg_rpcn::get_hosts()
{
	std::vector<std::pair<std::string, std::string>> vec_hosts;
	auto hosts_list = fmt::split(hosts.to_string(), {"|||"});

	for (const auto& cur_host : hosts_list)
	{
		auto desc_and_host = fmt::split(cur_host, {"|"});
		if (desc_and_host.size() != 2)
		{
			rpcn_log.error("Invalid host in the list of hosts: %s", cur_host);
			continue;
		}
		vec_hosts.push_back(std::make_pair(std::move(desc_and_host[0]), std::move(desc_and_host[1])));
	}

	if (vec_hosts.empty())
	{
		hosts.from_default();
		save();
		return get_hosts();
	}

	return vec_hosts;
}

std::string cfg_rpcn::get_npid()
{
	std::string final_npid = npid.to_string();
	if (final_npid.empty())
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

bool cfg_rpcn::get_ipv6_support() const
{
	return ipv6_support.get();
}

void cfg_rpcn::set_host(std::string_view host)
{
	this->host.from_string(host);
}

void cfg_rpcn::set_npid(std::string_view npid)
{
	this->npid.from_string(npid);
}

void cfg_rpcn::set_password(std::string_view password)
{
	this->password.from_string(password);
}

void cfg_rpcn::set_token(std::string_view token)
{
	this->token.from_string(token);
}

void cfg_rpcn::set_ipv6_support(bool ipv6_support)
{
	this->ipv6_support.set(ipv6_support);
}

void cfg_rpcn::set_hosts(const std::vector<std::pair<std::string, std::string>>& vec_hosts)
{
	std::string final_string;
	for (const auto& [cur_desc, cur_host] : vec_hosts)
	{
		fmt::append(final_string, "%s|%s|||", cur_desc, cur_host);
	}

	if (final_string.empty())
	{
		hosts.from_default();
		return;
	}

	final_string.resize(final_string.size() - 3);
	hosts.from_string(final_string);
}

bool cfg_rpcn::add_host(std::string_view new_description, std::string_view new_host)
{
	auto cur_hosts = get_hosts();

	for (const auto& [cur_desc, cur_host] : cur_hosts)
	{
		if (cur_desc == new_description && cur_host == new_host)
			return false;
	}

	cur_hosts.push_back(std::make_pair(std::string(new_description), std::string(new_host)));
	set_hosts(cur_hosts);

	return true;
}

bool cfg_rpcn::del_host(std::string_view del_description, std::string_view del_host)
{
	// Do not delete default servers
	if ((del_description == "Official RPCN Server" && del_host == "np.rpcs3.net") ||
		(del_description == "RPCN Test Server" && del_host == "test-np.rpcs3.net"))
	{
		return true;
	}

	auto cur_hosts = get_hosts();

	for (auto it = cur_hosts.begin(); it != cur_hosts.end(); it++)
	{
		if (it->first == del_description && it->second == del_host)
		{
			cur_hosts.erase(it);
			set_hosts(cur_hosts);
			return true;
		}
	}

	return false;
}

std::optional<std::pair<std::string, u16>> parse_rpcn_host(std::string_view host)
{
	if (host.empty())
	{
		rpcn_log.error("RPCN host is empty!");
		return std::nullopt;
	}

	auto splithost = fmt::split(host, {":"});
	if (splithost.size() != 1 && splithost.size() != 2)
	{
		rpcn_log.error("RPCN host is invalid!");
		return std::nullopt;
	}

	u16 port = 31313;

	if (splithost.size() == 2)
	{
		port = ::narrow<u16>(std::stoul(splithost[1]));
		if (port == 0)
		{
			rpcn_log.error("RPCN port is invalid!");
			return std::nullopt;
		}
	}

	return std::make_pair(std::move(splithost[0]), port);
}
