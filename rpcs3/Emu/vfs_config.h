#pragma once

#include "Utilities/Config.h"

struct cfg_vfs : cfg::node
{
	std::string get(const cfg::string&, std::string_view emu_dir = {}) const;
	void load();
	void save() const;
	static std::string get_path();

	cfg::string emulator_dir{ this, "$(EmulatorDir)" }; // Default (empty): taken from fs::get_config_dir()
	cfg::string dev_hdd0{ this, "/dev_hdd0/", "$(EmulatorDir)dev_hdd0/" };
	cfg::string dev_hdd1{ this, "/dev_hdd1/", "$(EmulatorDir)dev_hdd1/" };
	cfg::string dev_flash{ this, "/dev_flash/", "$(EmulatorDir)dev_flash/" };
	cfg::string dev_flash2{ this, "/dev_flash2/", "$(EmulatorDir)dev_flash2/" };
	cfg::string dev_flash3{ this, "/dev_flash3/", "$(EmulatorDir)dev_flash3/" };
	cfg::string dev_usb000{ this, "/dev_usb000/", "$(EmulatorDir)dev_usb000/" };
	cfg::string dev_bdvd{ this, "/dev_bdvd/" }; // Not mounted
	cfg::string app_home{ this, "/app_home/" }; // Not mounted

	std::string get_dev_flash() const
	{
		return get(dev_flash);
	}

	std::string get_dev_flash2() const
	{
		return get(dev_flash2);
	}

	std::string get_dev_flash3() const
	{
		return get(dev_flash3);
	}
};

extern cfg_vfs g_cfg_vfs;
