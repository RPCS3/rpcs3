#pragma once

#include "Utilities/Config.h"

struct cfg_vfs : cfg::node
{
	std::string get(const cfg::string& _cfg, std::string_view emu_dir = {}) const;
	void load();
	void save() const;
	static std::string get_path();

	cfg::string emulator_dir{ this, "$(EmulatorDir)" }; // Default (empty): taken from fs::get_config_dir()
	cfg::string dev_hdd0{ this, "/dev_hdd0/", "$(EmulatorDir)dev_hdd0/" };
	cfg::string dev_hdd1{ this, "/dev_hdd1/", "$(EmulatorDir)dev_hdd1/" };
	cfg::string dev_flash{ this, "/dev_flash/", "$(EmulatorDir)dev_flash/" };
	cfg::string dev_flash2{ this, "/dev_flash2/", "$(EmulatorDir)dev_flash2/" };
	cfg::string dev_flash3{ this, "/dev_flash3/", "$(EmulatorDir)dev_flash3/" };
	cfg::string dev_bdvd{ this, "/dev_bdvd/", "$(EmulatorDir)dev_bdvd/" }; // Only mounted in some special cases
	cfg::string games_dir{ this, "/games/", "$(EmulatorDir)games/" }; // Not mounted
	cfg::string app_home{ this, "/app_home/" }; // Not mounted

	cfg::device_entry dev_usb{ this, "/dev_usb***/",
		{
			{ "/dev_usb000", cfg::device_info{.path = "$(EmulatorDir)dev_usb000/"} }
		}};

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

	cfg::device_info get_device(const cfg::device_entry& _cfg, std::string_view key, std::string_view emu_dir = {}) const;

private:
	std::string get(const std::string& _cfg, const std::string& def, std::string_view emu_dir) const;
	cfg::device_info get_device_info(const cfg::device_entry& _cfg, std::string_view key, std::string_view emu_dir = {}) const;
};

extern cfg_vfs g_cfg_vfs;
