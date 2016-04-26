#include "stdafx.h"
#include "Utilities/Config.h"
#include "Emu/System.h"

#include "VFS.h"

#include "Utilities/StrUtil.h"

cfg::string_entry g_cfg_vfs_emulator_dir(cfg::root.vfs, "$(EmulatorDir)"); // Default (empty): taken from fs::get_executable_dir()
cfg::string_entry g_cfg_vfs_dev_hdd0(cfg::root.vfs, "/dev_hdd0/", "$(EmulatorDir)dev_hdd0/");
cfg::string_entry g_cfg_vfs_dev_hdd1(cfg::root.vfs, "/dev_hdd1/", "$(EmulatorDir)dev_hdd1/");
cfg::string_entry g_cfg_vfs_dev_flash(cfg::root.vfs, "/dev_flash/", "$(EmulatorDir)dev_flash/");
cfg::string_entry g_cfg_vfs_dev_usb000(cfg::root.vfs, "/dev_usb000/", "$(EmulatorDir)dev_usb000/");
cfg::string_entry g_cfg_vfs_dev_bdvd(cfg::root.vfs, "/dev_bdvd/"); // Not mounted
cfg::string_entry g_cfg_vfs_app_home(cfg::root.vfs, "/app_home/"); // Not mounted

cfg::bool_entry g_cfg_vfs_allow_host_root(cfg::root.vfs, "Enable /host_root/", true);

void vfs::dump()
{
	LOG_NOTICE(LOADER, "Mount info:");
	LOG_NOTICE(LOADER, "/dev_hdd0/ -> %s", g_cfg_vfs_dev_hdd0.get());
	LOG_NOTICE(LOADER, "/dev_hdd1/ -> %s", g_cfg_vfs_dev_hdd1.get());
	LOG_NOTICE(LOADER, "/dev_flash/ -> %s", g_cfg_vfs_dev_flash.get());
	LOG_NOTICE(LOADER, "/dev_usb/ -> %s", g_cfg_vfs_dev_usb000.get());
	LOG_NOTICE(LOADER, "/dev_usb000/ -> %s", g_cfg_vfs_dev_usb000.get());
	if (g_cfg_vfs_dev_bdvd.size()) LOG_NOTICE(LOADER, "/dev_bdvd/ -> %s", g_cfg_vfs_dev_bdvd.get());
	if (g_cfg_vfs_app_home.size()) LOG_NOTICE(LOADER, "/app_home/ -> %s", g_cfg_vfs_app_home.get());
	if (g_cfg_vfs_allow_host_root) LOG_NOTICE(LOADER, "/host_root/ -> .");
	LOG_NOTICE(LOADER, "");
}

std::string vfs::get(const std::string& vpath)
{
	const cfg::string_entry* vdir = nullptr;
	std::size_t f_pos = vpath.find_first_not_of('/');
	std::size_t start = 0;

	// Compare vpath with device name
	auto detect = [&](const auto& vdev) -> bool
	{
		const std::size_t size = ::size32(vdev) - 1; // Char array size

		if (f_pos && f_pos != -1 && vpath.compare(f_pos - 1, size, vdev, size) == 0)
		{
			start = size;
			return true;
		}

		return false;
	};

	if (g_cfg_vfs_allow_host_root && detect("/host_root/"))
		return vpath.substr(start); // Accessing host FS directly
	else if (detect("/dev_hdd0/"))
		vdir = &g_cfg_vfs_dev_hdd0;
	else if (detect("/dev_hdd1/"))
		vdir = &g_cfg_vfs_dev_hdd1;
	else if (detect("/dev_flash/"))
		vdir = &g_cfg_vfs_dev_flash;
	else if (detect("/dev_usb000/"))
		vdir = &g_cfg_vfs_dev_usb000;
	else if (detect("/dev_usb/"))
		vdir = &g_cfg_vfs_dev_usb000;
	else if (detect("/dev_bdvd/"))
		vdir = &g_cfg_vfs_dev_bdvd;
	else if (detect("/app_home/"))
		vdir = &g_cfg_vfs_app_home;

	// Return empty path if not mounted
	if (!vdir || !start)
	{
		LOG_WARNING(GENERAL, "vfs::get() failed for %s", vpath);
		return{};
	}

	// Replace $(EmulatorDir), concatenate
	return fmt::replace_all(*vdir, "$(EmulatorDir)", g_cfg_vfs_emulator_dir.size() == 0 ? fs::get_executable_dir() : g_cfg_vfs_emulator_dir) + vpath.substr(start);
}
