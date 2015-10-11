#pragma once
#include "Utilities/Log.h"
#include "config.h"

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

std::vector<std::string> GetAdapters();

class SettingsDialog : public wxDialog
{
public:
	SettingsDialog(wxWindow *parent, rpcs3::config_t* cfg = &rpcs3::config);
};

