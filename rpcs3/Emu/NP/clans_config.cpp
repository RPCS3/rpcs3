#include "Utilities/StrUtil.h"
#include "stdafx.h"
#include "clans_config.h"
#include "Utilities/File.h"

cfg_clans g_cfg_clans;

LOG_CHANNEL(clans_config_log, "clans_config");

void cfg_clans::load()
{
	const std::string path = cfg_clans::get_path();

	fs::file cfg_file(path, fs::read);
	if (cfg_file)
	{
		clans_config_log.notice("Loading Clans config. Path: %s", path);
		from_string(cfg_file.to_string());
	}
	else
	{
		clans_config_log.notice("Clans config missing. Using default settings. Path: %s", path);
		from_default();
	}
}

void cfg_clans::save() const
{
#ifdef _WIN32
	const std::string path_to_cfg = fs::get_config_dir(true);
	if (!fs::create_path(path_to_cfg))
	{
		clans_config_log.error("Could not create path: %s", path_to_cfg);
	}
#endif

	const std::string path = cfg_clans::get_path();

	if (!cfg::node::save(path))
	{
		clans_config_log.error("Could not save config: %s (error=%s)", path, fs::g_tls_error);
	}
}

std::string cfg_clans::get_path()
{
	return fs::get_config_dir(true) + "clans.yml";
}

std::string cfg_clans::get_host() const
{
	return host.to_string();
}

bool cfg_clans::get_use_https() const
{
	return use_https.get();
}

std::vector<std::pair<std::string, std::string>> cfg_clans::get_hosts()
{
	std::vector<std::pair<std::string, std::string>> vec_hosts;
	const std::string host_str = hosts.to_string();
	const auto hosts_list = fmt::split_sv(host_str, {"|||"});

	for (const auto& cur_host : hosts_list)
	{
		const auto desc_and_host = fmt::split(cur_host, {"|"});
		if (desc_and_host.size() != 2)
		{
			clans_config_log.error("Invalid host in the list of hosts: %s", cur_host);
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

void cfg_clans::set_host(std::string_view host)
{
	this->host.from_string(host);
}

void cfg_clans::set_use_https(bool use_https)
{
	this->use_https.set(use_https);
}

void cfg_clans::set_hosts(const std::vector<std::pair<std::string, std::string>>& vec_hosts)
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

bool cfg_clans::add_host(std::string_view new_description, std::string_view new_host)
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

bool cfg_clans::del_host(std::string_view del_description, std::string_view del_host)
{
	// Do not delete default servers
	const auto def_desc_and_host = fmt::split_sv(hosts.def, {"|"});
	ensure(def_desc_and_host.size() == 2);
	if (del_description == def_desc_and_host[0] && del_host == def_desc_and_host[1])
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
