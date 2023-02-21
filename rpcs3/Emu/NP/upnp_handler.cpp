#include "stdafx.h"
#include "upnp_handler.h"
#include "util/logs.hpp"

#include <miniwget.h>
#include <upnpcommands.h>

LOG_CHANNEL(upnp_log, "UPNP");

upnp_handler::~upnp_handler()
{
	std::lock_guard lock(m_mutex);

	for (const auto& [protocol, prot_bindings] : m_bindings)
	{
		for (const auto& [internal_port, external_port] : prot_bindings)
		{
			remove_port_redir_external(external_port, protocol);
		}
	}

	m_active = false;
}

void upnp_handler::upnp_enable()
{
	std::lock_guard lock(m_mutex);

	m_cfg.load();

	auto check_igd = [&](const char* url) -> bool
	{
		int desc_xml_size = 0;
		int status_code = 0;

		m_igd_data = {};
		m_igd_urls = {};

		char* desc_xml = static_cast<char*>(miniwget(url, &desc_xml_size, 1, &status_code));

		if (!desc_xml)
			return false;

		parserootdesc(desc_xml, desc_xml_size, &m_igd_data);
		free(desc_xml);
		desc_xml = nullptr;
		GetUPNPUrls(&m_igd_urls, &m_igd_data, url, 1);

		return true;
	};

	std::string dev_url = m_cfg.get_device_url();

	if (!dev_url.empty())
	{
		if (check_igd(dev_url.c_str()))
		{
			upnp_log.notice("Saved UPNP(%s) enabled", dev_url);
			m_active = true;
			return;
		}

		upnp_log.error("Saved UPNP(%s) isn't available anymore", dev_url);
	}

	upnp_log.notice("Starting UPNP search");

	int upnperror = 0;
	UPNPDev* devlist = upnpDiscover(2000, nullptr, nullptr, 0, 0, 2, &upnperror);

	if (!devlist)
	{
		upnp_log.error("No UPNP device was found");
		return;
	}

	const UPNPDev* dev = devlist;
	for (; dev; dev = dev->pNext)
	{
		if (strstr(dev->st, "InternetGatewayDevice"))
			break;
	}

	if (dev)
	{
		int desc_xml_size = 0;
		int status_code = 0;
		char* desc_xml = static_cast<char*>(miniwget(dev->descURL, &desc_xml_size, 1, &status_code));

		if (desc_xml)
		{
			IGDdatas igd_data{};
			UPNPUrls igd_urls{};
			parserootdesc(desc_xml, desc_xml_size, &igd_data);
			free(desc_xml);
			desc_xml = nullptr;
			GetUPNPUrls(&igd_urls, &igd_data, dev->descURL, 1);

			upnp_log.notice("Found UPnP device type:%s at %s", dev->st, dev->descURL);

			m_cfg.set_device_url(dev->descURL);
			m_cfg.save();

			m_active = true;
		}
		else
		{
			upnp_log.error("Failed to retrieve UPNP xml for %s", dev->descURL);
		}
	}
	else
	{
		upnp_log.error("No UPNP IGD device was found");
	}

	freeUPNPDevlist(devlist);
}

void upnp_handler::add_port_redir(std::string_view addr, u16 internal_port, std::string_view protocol)
{
	if (!m_active)
		return;

	std::lock_guard lock(m_mutex);

	u16 external_port = internal_port;
	std::string internal_port_str = fmt::format("%d", internal_port);
	int res = 0;

	for (u16 external_port = internal_port; external_port < internal_port + 100; external_port++)
	{
		std::string external_port_str = fmt::format("%d", external_port);
		res = UPNP_AddPortMapping(m_igd_urls.controlURL, m_igd_data.first.servicetype, external_port_str.c_str(), internal_port_str.c_str(), addr.data(), "RPCS3", protocol.data(), nullptr, nullptr);
		if (res == UPNPCOMMAND_SUCCESS)
		{
			m_bindings[std::string(protocol)][internal_port] = external_port;
			upnp_log.notice("Successfully bound %s:%d(%s) to IGD:%d", addr, internal_port, protocol, external_port);
			return;
		}

		// need more testing, may vary per router, etc, for now assume port conflict silently
		// else if (res != 718) // ConflictInMappingEntry
		// {
		// 	upnp_log.error("Failed to bind %s:%d(%s) to IGD:%d: %d", addr, internal_port, protocol, external_port, res);
		// 	return;
		// }
	}

	upnp_log.error("Failed to bind %s:%d(%s) to IGD:(%d=>%d): %d", addr, internal_port, protocol, internal_port, external_port, res);
}

void upnp_handler::remove_port_redir(u16 internal_port, std::string_view protocol)
{
	if (!m_active)
		return;

	std::lock_guard lock(m_mutex);

	const std::string str_protocol(protocol);

	if (!m_bindings.contains(str_protocol) || !::at32(m_bindings, str_protocol).contains(internal_port))
	{
		upnp_log.error("tried to unbind port mapping %d to IGD(%s) but it isn't bound", internal_port, protocol);
		return;
	}

	const u16 external_port = ::at32(::at32(m_bindings, str_protocol), internal_port);

	remove_port_redir_external(external_port, protocol);

	ensure(::at32(m_bindings, str_protocol).erase(internal_port));
	upnp_log.notice("Successfully deleted port mapping %d to IGD:%d(%s)", internal_port, external_port, protocol);
}

void upnp_handler::remove_port_redir_external(u16 external_port, std::string_view protocol, bool verbose)
{
	const std::string str_ext_port = fmt::format("%d", external_port);

	if (int res = UPNP_DeletePortMapping(m_igd_urls.controlURL, m_igd_data.first.servicetype, str_ext_port.c_str(), protocol.data(), nullptr); res != 0 && verbose)
		upnp_log.error("Failed to delete port mapping IGD:%s(%s): %d", str_ext_port, protocol, res);
}

bool upnp_handler::is_active() const
{
	return m_active;
}
