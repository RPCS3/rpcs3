#pragma once
#include "Utilities/Log.h"

#ifdef _WIN32
#include <windows.h>
#include <iphlpapi.h>

#pragma comment(lib, "iphlpapi.lib")
#else

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#endif

class SettingsDialog : public wxDialog
{
public:
	SettingsDialog(wxWindow *parent);
};

static std::vector<std::string> GetAdapters()
{
	std::vector<std::string> adapters;
#ifdef _WIN32
	PIP_ADAPTER_INFO pAdapterInfo;
	pAdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
	ULONG buflen = sizeof(IP_ADAPTER_INFO);

	if (GetAdaptersInfo(pAdapterInfo, &buflen) == ERROR_BUFFER_OVERFLOW)
	{
		free(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO*)malloc(buflen);
	}

	if (GetAdaptersInfo(pAdapterInfo, &buflen) == NO_ERROR)
	{
		PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
		while (pAdapter)
		{
			adapters.emplace_back(pAdapter->Description);
			pAdapter = pAdapter->Next;
		}
	}
	else
	{
		LOG_ERROR(HLE, "Call to GetAdaptersInfo failed.");
	}
#else
	struct ifaddrs *ifaddr, *ifa;
	int family, s, n;
	char host[NI_MAXHOST];

	if (getifaddrs(&ifaddr) == -1)
	{
		LOG_ERROR(HLE, "Call to getifaddrs returned negative.");
	}

	for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++)
	{
		if (ifa->ifa_addr == NULL)
		{
			continue;
		}

		family = ifa->ifa_addr->sa_family;

		if (family == AF_INET || family == AF_INET6)
		{
			adapters.emplace_back(ifa->ifa_name);
		}
	}

	freeifaddrs(ifaddr);
#endif

	return adapters;
}
