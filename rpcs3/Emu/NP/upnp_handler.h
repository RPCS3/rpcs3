#pragma once

#include <unordered_map>

#include <miniupnpc.h>

#include "upnp_config.h"
#include "Utilities/mutex.h"

class upnp_handler
{
public:
	~upnp_handler();

	void upnp_enable();
	void add_port_redir(std::string_view addr, u16 internal_port, std::string_view protocol);
	void remove_port_redir(u16 internal_port, std::string_view protocol);

	bool is_active() const;

private:
	void remove_port_redir_external(u16 external_port, std::string_view protocol, bool verbose = true);

private:
	atomic_t<bool> m_active = false;

	shared_mutex m_mutex;
	cfg_upnp m_cfg;
	IGDdatas m_igd_data{};
	UPNPUrls m_igd_urls{};
	std::unordered_map<std::string, std::unordered_map<u16, u16>> m_bindings;
};
