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
	const std::string path_to_cfg = fs::get_config_dir() + "config/";
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

std::string cfg_rpcn::get_country() const
{
	return country.to_string();
}

std::vector<cfg_rpcn::country_code> cfg_rpcn::get_countries()
{
	std::vector<country_code> countries;
	countries.push_back({"United States", {'u','s'}});
	countries.push_back({"Japan", {'j', 'p'}});
	countries.push_back({"Afghanistan", {'a', 'f'}});
	countries.push_back({"Albania", {'a', 'l'}});
	countries.push_back({"Algeria", {'d', 'z'}});
	countries.push_back({"American Samoa", {'a', 's'}});
	countries.push_back({"&orra", {'a', 'd'}});
	countries.push_back({"Angola", {'a', 'o'}});
	countries.push_back({"Anguilla", {'a', 'i'}});
	countries.push_back({"Antarctica", {'a', 'q'}});
	countries.push_back({"Antigua & Barbuda", {'a', 'g'}});
	countries.push_back({"Argentina", {'a', 'r'}});
	countries.push_back({"Armenia", {'a', 'm'}});
	countries.push_back({"Aruba", {'a', 'w'}});
	countries.push_back({"Australia", {'a', 'u'}});
	countries.push_back({"Austria", {'a', 't'}});
	countries.push_back({"Azerbaijan", {'a', 'z'}});
	countries.push_back({"Bahamas", {'b', 's'}});
	countries.push_back({"Bahrain", {'b', 'h'}});
	countries.push_back({"Bangladesh", {'b', 'd'}});
	countries.push_back({"Barbados", {'b', 'b'}});
	countries.push_back({"Belarus", {'b', 'y'}});
	countries.push_back({"Belgium", {'b', 'e'}});
	countries.push_back({"Belize", {'b', 'z'}});
	countries.push_back({"Benin", {'b', 'j'}});
	countries.push_back({"Bermuda", {'b', 'm'}});
	countries.push_back({"Bhutan", {'b', 't'}});
	countries.push_back({"Bolivia", {'b', 'o'}});
	countries.push_back({"Bosnia & Herzegovina", {'b', 'a'}});
	countries.push_back({"Botswana", {'b', 'w'}});
	countries.push_back({"Brazil", {'b', 'r'}});
	countries.push_back({"BIO Territory", {'i', 'o'}});
	countries.push_back({"British Virgin Islands", {'v', 'g'}});
	countries.push_back({"Brunei", {'b', 'n'}});
	countries.push_back({"Bulgaria", {'b', 'g'}});
	countries.push_back({"Burkina Faso", {'b', 'f'}});
	countries.push_back({"Burundi", {'b', 'i'}});
	countries.push_back({"Cambodia", {'k', 'h'}});
	countries.push_back({"Cameroon", {'c', 'm'}});
	countries.push_back({"Canada", {'c', 'a'}});
	countries.push_back({"Cape Verde", {'c', 'v'}});
	countries.push_back({"Cayman Islands", {'k', 'y'}});
	countries.push_back({"Central African Republic", {'c', 'f'}});
	countries.push_back({"Chad", {'t', 'd'}});
	countries.push_back({"Chile", {'c', 'l'}});
	countries.push_back({"China", {'c', 'n'}});
	countries.push_back({"Christmas Island", {'c', 'x'}});
	countries.push_back({"Cocos Islands", {'c', 'c'}});
	countries.push_back({"Colombia", {'c', 'o'}});
	countries.push_back({"Comoros", {'k', 'm'}});
	countries.push_back({"Congo", {'c', 'g'}});
	countries.push_back({"Cook Islands", {'c', 'k'}});
	countries.push_back({"Costa Rica", {'c', 'r'}});
	countries.push_back({"Croatia", {'h', 'r'}});
	countries.push_back({"Cuba", {'c', 'u'}});
	countries.push_back({"Curacao", {'c', 'w'}});
	countries.push_back({"Cyprus", {'c', 'y'}});
	countries.push_back({"Czech Republic", {'c', 'z'}});
	countries.push_back({"DR Congo", {'c', 'd'}});
	countries.push_back({"Denmark", {'d', 'k'}});
	countries.push_back({"Djibouti", {'d', 'j'}});
	countries.push_back({"Dominica", {'d', 'm'}});
	countries.push_back({"Dominican Republic", {'d', 'o'}});
	countries.push_back({"East Timor", {'t', 'l'}});
	countries.push_back({"Ecuador", {'e', 'c'}});
	countries.push_back({"Egypt", {'e', 'g'}});
	countries.push_back({"El Salvador", {'s', 'v'}});
	countries.push_back({"Equatorial Guinea", {'g', 'q'}});
	countries.push_back({"Eritrea", {'e', 'r'}});
	countries.push_back({"Estonia", {'e', 'e'}});
	countries.push_back({"Eswatini", {'s', 'z'}});
	countries.push_back({"Ethiopia", {'e', 't'}});
	countries.push_back({"Faroe Islands", {'f', 'o'}});
	countries.push_back({"Fiji", {'f', 'j'}});
	countries.push_back({"Finland", {'f', 'i'}});
	countries.push_back({"France", {'f', 'r'}});
	countries.push_back({"French Polynesia", {'p', 'f'}});
	countries.push_back({"Gabon", {'g', 'a'}});
	countries.push_back({"Gambia", {'g', 'm'}});
	countries.push_back({"Georgia", {'g', 'e'}});
	countries.push_back({"Germany", {'d', 'e'}});
	countries.push_back({"Ghana", {'g', 'h'}});
	countries.push_back({"Gibraltar", {'g', 'i'}});
	countries.push_back({"Greece", {'g', 'r'}});
	countries.push_back({"Greenland", {'g', 'l'}});
	countries.push_back({"Grenada", {'g', 'd'}});
	countries.push_back({"Guam", {'g', 'u'}});
	countries.push_back({"Guatemala", {'g', 't'}});
	countries.push_back({"Guernsey", {'g', 'g'}});
	countries.push_back({"Guinea", {'g', 'n'}});
	countries.push_back({"Guinea-Bissau", {'g', 'w'}});
	countries.push_back({"Guyana", {'g', 'y'}});
	countries.push_back({"Haiti", {'h', 't'}});
	countries.push_back({"Honduras", {'h', 'n'}});
	countries.push_back({"Hong Kong", {'h', 'k'}});
	countries.push_back({"Hungary", {'h', 'u'}});
	countries.push_back({"Iceland", {'i', 's'}});
	countries.push_back({"India", {'i', 'n'}});
	countries.push_back({"Indonesia", {'i', 'd'}});
	countries.push_back({"Iran", {'i', 'r'}});
	countries.push_back({"Iraq", {'i', 'q'}});
	countries.push_back({"Ireland", {'i', 'e'}});
	countries.push_back({"Isle of Man", {'i', 'm'}});
	countries.push_back({"Israel", {'i', 'l'}});
	countries.push_back({"Italy", {'i', 't'}});
	countries.push_back({"Ivory Coast", {'c', 'i'}});
	countries.push_back({"Jamaica", {'j', 'm'}});
	countries.push_back({"Jersey", {'j', 'e'}});
	countries.push_back({"Jordan", {'j', 'o'}});
	countries.push_back({"Kazakhstan", {'k', 'z'}});
	countries.push_back({"Kenya", {'k', 'e'}});
	countries.push_back({"Kiribati", {'k', 'i'}});
	countries.push_back({"Kosovo", {'x', 'k'}});
	countries.push_back({"Kuwait", {'k', 'w'}});
	countries.push_back({"Kyrgyzstan", {'k', 'g'}});
	countries.push_back({"Laos", {'l', 'a'}});
	countries.push_back({"Latvia", {'l', 'v'}});
	countries.push_back({"Lebanon", {'l', 'b'}});
	countries.push_back({"Lesotho", {'l', 's'}});
	countries.push_back({"Liberia", {'l', 'r'}});
	countries.push_back({"Libya", {'l', 'y'}});
	countries.push_back({"Liechtenstein", {'l', 'i'}});
	countries.push_back({"Lithuania", {'l', 't'}});
	countries.push_back({"Luxembourg", {'l', 'u'}});
	countries.push_back({"Macao", {'m', 'o'}});
	countries.push_back({"Madagascar", {'m', 'g'}});
	countries.push_back({"Malawi", {'m', 'w'}});
	countries.push_back({"Malaysia", {'m', 'y'}});
	countries.push_back({"Maldives", {'m', 'v'}});
	countries.push_back({"Mali", {'m', 'l'}});
	countries.push_back({"Malta", {'m', 't'}});
	countries.push_back({"Marshall Islands", {'m', 'h'}});
	countries.push_back({"Mauritania", {'m', 'r'}});
	countries.push_back({"Mauritius", {'m', 'u'}});
	countries.push_back({"Mayotte", {'y', 't'}});
	countries.push_back({"Mexico", {'m', 'x'}});
	countries.push_back({"Micronesia", {'f', 'm'}});
	countries.push_back({"Moldova", {'m', 'd'}});
	countries.push_back({"Monaco", {'m', 'c'}});
	countries.push_back({"Mongolia", {'m', 'n'}});
	countries.push_back({"Montenegro", {'m', 'e'}});
	countries.push_back({"Montserrat", {'m', 's'}});
	countries.push_back({"Morocco", {'m', 'a'}});
	countries.push_back({"Mozambique", {'m', 'z'}});
	countries.push_back({"Myanmar", {'m', 'm'}});
	countries.push_back({"Namibia", {'n', 'a'}});
	countries.push_back({"Nauru", {'n', 'r'}});
	countries.push_back({"Nepal", {'n', 'p'}});
	countries.push_back({"Netherlands", {'n', 'l'}});
	countries.push_back({"Netherlands Antilles", {'a', 'n'}});
	countries.push_back({"New Caledonia", {'n', 'c'}});
	countries.push_back({"New Zealand", {'n', 'z'}});
	countries.push_back({"Nicaragua", {'n', 'i'}});
	countries.push_back({"Niger", {'n', 'e'}});
	countries.push_back({"Nigeria", {'n', 'g'}});
	countries.push_back({"Niue", {'n', 'u'}});
	countries.push_back({"North Korea", {'k', 'p'}});
	countries.push_back({"Northern Mariana Islands", {'m', 'p'}});
	countries.push_back({"Norway", {'n', 'o'}});
	countries.push_back({"Oman", {'o', 'm'}});
	countries.push_back({"Pakistan", {'p', 'k'}});
	countries.push_back({"Palau", {'p', 'w'}});
	countries.push_back({"Palestine", {'p', 's'}});
	countries.push_back({"Panama", {'p', 'a'}});
	countries.push_back({"Papua New Guinea", {'p', 'g'}});
	countries.push_back({"Paraguay", {'p', 'y'}});
	countries.push_back({"Peru", {'p', 'e'}});
	countries.push_back({"Philippines", {'p', 'h'}});
	countries.push_back({"Pitcairn", {'p', 'n'}});
	countries.push_back({"Poland", {'p', 'l'}});
	countries.push_back({"Portugal", {'p', 't'}});
	countries.push_back({"Puerto Rico", {'p', 'r'}});
	countries.push_back({"Qatar", {'q', 'a'}});
	countries.push_back({"Reunion", {'r', 'e'}});
	countries.push_back({"Romania", {'r', 'o'}});
	countries.push_back({"Russia", {'r', 'u'}});
	countries.push_back({"Rwanda", {'r', 'w'}});
	countries.push_back({"St Barthelemy", {'b', 'l'}});
	countries.push_back({"St Helena", {'s', 'h'}});
	countries.push_back({"St Kitts & Nevis", {'k', 'n'}});
	countries.push_back({"St Lucia", {'l', 'c'}});
	countries.push_back({"St Martin", {'m', 'f'}});
	countries.push_back({"St Pierre & Miquelon", {'p', 'm'}});
	countries.push_back({"St Vincent & the Grenadines", {'v', 'c'}});
	countries.push_back({"Samoa", {'w', 's'}});
	countries.push_back({"San Marino", {'s', 'm'}});
	countries.push_back({"Sao Tome & Principe", {'s', 't'}});
	countries.push_back({"Saudi Arabia", {'s', 'a'}});
	countries.push_back({"Senegal", {'s', 'n'}});
	countries.push_back({"Serbia", {'r', 's'}});
	countries.push_back({"Seychelles", {'s', 'c'}});
	countries.push_back({"Sierra Leone", {'s', 'l'}});
	countries.push_back({"Singapore", {'s', 'g'}});
	countries.push_back({"Sint Maarten", {'s', 'x'}});
	countries.push_back({"Slovakia", {'s', 'k'}});
	countries.push_back({"Slovenia", {'s', 'i'}});
	countries.push_back({"Solomon Islands", {'s', 'b'}});
	countries.push_back({"Somalia", {'s', 'o'}});
	countries.push_back({"South Africa", {'z', 'a'}});
	countries.push_back({"South Korea", {'k', 'r'}});
	countries.push_back({"South Sudan", {'s', 's'}});
	countries.push_back({"Spain", {'e', 's'}});
	countries.push_back({"Sri Lanka", {'l', 'k'}});
	countries.push_back({"Sudan", {'s', 'd'}});
	countries.push_back({"Suriname", {'s', 'r'}});
	countries.push_back({"Svalbard & Jan Mayen", {'s', 'j'}});
	countries.push_back({"Sweden", {'s', 'e'}});
	countries.push_back({"Switzerland", {'c', 'h'}});
	countries.push_back({"Syria", {'s', 'y'}});
	countries.push_back({"Taiwan", {'t', 'w'}});
	countries.push_back({"Tajikistan", {'t', 'j'}});
	countries.push_back({"Tanzania", {'t', 'z'}});
	countries.push_back({"Thailand", {'t', 'h'}});
	countries.push_back({"Togo", {'t', 'g'}});
	countries.push_back({"Tokelau", {'t', 'k'}});
	countries.push_back({"Tonga", {'t', 'o'}});
	countries.push_back({"Trinidad & Tobago", {'t', 't'}});
	countries.push_back({"Tunisia", {'t', 'n'}});
	countries.push_back({"Turkey", {'t', 'r'}});
	countries.push_back({"Turkmenistan", {'t', 'm'}});
	countries.push_back({"Turks & Caicos Islands", {'t', 'c'}});
	countries.push_back({"Tuvalu", {'t', 'v'}});
	countries.push_back({"Uganda", {'u', 'g'}});
	countries.push_back({"Ukraine", {'u', 'a'}});
	countries.push_back({"United Arab Emirates", {'a', 'e'}});
	countries.push_back({"United Kingdom", {'g', 'b'}});
	countries.push_back({"Uruguay", {'u', 'y'}});
	countries.push_back({"Uzbekistan", {'u', 'z'}});
	countries.push_back({"Vanuatu", {'v', 'u'}});
	countries.push_back({"Vatican", {'v', 'a'}});
	countries.push_back({"Venezuela", {'v', 'e'}});
	countries.push_back({"Vietnam", {'v', 'n'}});
	countries.push_back({"Wallis & Futuna", {'w', 'f'}});
	countries.push_back({"Western Sahara", {'e', 'h'}});
	countries.push_back({"Yemen", {'y', 'e'}});
	countries.push_back({"Zambia", {'z', 'm'}});
	countries.push_back({"Zimbabwe", {'z', 'w'}});

	return countries;
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

void cfg_rpcn::set_host(std::string_view host)
{
	this->host.from_string(host);
}

void cfg_rpcn::set_country(std::string_view country)
{
	this->country.from_string(country);
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
