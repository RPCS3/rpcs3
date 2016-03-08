#pragma once
#include "config.h"

std::vector<std::string> GetAdapters();

class SettingsDialog : public wxDialog
{
public:
	SettingsDialog(wxWindow *parent, rpcs3::config_t* cfg = &rpcs3::config);
};

