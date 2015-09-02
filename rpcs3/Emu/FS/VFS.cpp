#include "stdafx.h"
#include "VFS.h"
#include "vfsDir.h"
#include "vfsFile.h"
#include "vfsDirBase.h"
#include "Emu/HDD/HDD.h"
#include "vfsDeviceLocalFile.h"
#include "Ini.h"
#include "Emu/System.h"
#include "Utilities/Log.h"

std::vector<std::string> simplify_path_blocks(const std::string& path)
{
	// fmt::tolower() removed
	std::vector<std::string> path_blocks = std::move(fmt::split(path, { "/", "\\" }));

	for (s32 i = 0; i < path_blocks.size(); ++i)
	{
		if (path_blocks[i] == "." || (i > 0 && path_blocks[i].empty()))
		{
			path_blocks.erase(path_blocks.begin() + i);
			i--;
		}
		else if (i > 0 && path_blocks[i] == "..")
		{
			path_blocks.erase(path_blocks.begin() + (i - 1), path_blocks.begin() + (i + 1));
			i--;
		}
	}

	return path_blocks;
}

std::string simplify_path(const std::string& path, bool is_dir, bool is_ps3)
{
	std::vector<std::string> path_blocks = simplify_path_blocks(path);

	if (path_blocks.empty())
		return "";

	std::string result = fmt::merge(path_blocks, "/");
	
#ifdef _WIN32
	if (is_ps3)
#endif
	{
		result = "/" + result;
	}

	if (is_dir) result = result + "/";

	return result;
}

VFS::~VFS()
{
	UnMountAll();
}

void VFS::Mount(const std::string& ps3_path, const std::string& local_path, vfsDevice* device)
{
	std::string simpl_ps3_path = simplify_path(ps3_path, true, true);

	UnMount(simpl_ps3_path);

	device->SetPath(simpl_ps3_path, simplify_path(local_path, true, false));
	m_devices.push_back(device);

	if (m_devices.size() > 1)
	{
		std::sort(m_devices.begin(), m_devices.end(), [](vfsDevice *a, vfsDevice *b) { return b->GetPs3Path().length() < a->GetPs3Path().length(); });
	}
}

void VFS::Link(const std::string& mount_point, const std::string& ps3_path)
{
	links[simplify_path_blocks(mount_point)] = simplify_path_blocks(ps3_path);
}

std::string VFS::GetLinked(const std::string& ps3_path) const
{
	// fmt::tolower removed
	auto path_blocks = fmt::split(ps3_path, { "/", "\\" });

	for (auto link : links)
	{
		if (path_blocks.size() < link.first.size())
			continue;

		bool is_ok = true;

		for (size_t i = 0; i < link.first.size(); ++i)
		{
			if (link.first[i] != path_blocks[i])
			{
				is_ok = false;
				break;
			}
		}

		if (is_ok)
			return fmt::merge({ link.second, std::vector<std::string>(path_blocks.begin() + link.first.size(), path_blocks.end()) }, "/");
	}

	return ps3_path;
}

void VFS::UnMount(const std::string& ps3_path)
{
	std::string simpl_ps3_path = simplify_path(ps3_path, true, true);

	for (u32 i = 0; i < m_devices.size(); ++i)
	{
		if (!strcmp(m_devices[i]->GetPs3Path().c_str(), simpl_ps3_path.c_str()))
		{
			delete m_devices[i];

			m_devices.erase(m_devices.begin() +i);

			return;
		}
	}
}

void VFS::UnMountAll()
{
	for(u32 i=0; i<m_devices.size(); ++i)
	{
		delete m_devices[i];
	}

	m_devices.clear();
}

vfsFileBase* VFS::OpenFile(const std::string& ps3_path, u32 mode) const
{
	std::string path;

	if (vfsDevice* dev = GetDevice(ps3_path, path))
	{
		if (vfsFileBase* res = dev->GetNewFileStream())
		{
			res->Open(path, mode);
			return res;
		}
	}

	return nullptr;
}

vfsDirBase* VFS::OpenDir(const std::string& ps3_path) const
{
	std::string path;

	if (vfsDevice* dev = GetDevice(ps3_path, path))
	{
		if (vfsDirBase* res = dev->GetNewDirStream())
		{
			res->Open(path);
			return res;
		}
	}

	return nullptr;
}

bool VFS::CreateDir(const std::string& ps3_path) const
{
	std::string path;

	if (vfsDevice* dev = GetDevice(ps3_path, path))
	{
		// return dev->create_dir(path);
		return fs::create_dir(path);
	}

	return false;
}

bool VFS::CreatePath(const std::string& ps3_path) const
{
	std::string path;

	if (vfsDevice* dev = GetDevice(ps3_path, path))
	{
		// return dev->create_path(path);
		return fs::create_path(path);
	}

	return false;
}

bool VFS::RemoveFile(const std::string& ps3_path) const
{
	std::string path;

	if (vfsDevice* dev = GetDevice(ps3_path, path))
	{
		// return dev->remove_file(path);
		return fs::remove_file(path);
	}

	return false;
}

bool VFS::RemoveDir(const std::string& ps3_path) const
{
	std::string path;

	if (vfsDevice* dev = GetDevice(ps3_path, path))
	{
		// return dev->remove_dir(path);
		return fs::remove_dir(path);
	}

	return false;
}

void VFS::DeleteAll(const std::string& ps3_path) const
{
	// Delete directory and all its contents recursively
	for (const auto entry : vfsDir(ps3_path))
	{
		if (entry->name == "." || entry->name == "..")
		{
			continue;
		}

		if (entry->flags & DirEntry_TypeFile)
		{
			RemoveFile(ps3_path + "/" + entry->name);
		}

		if (entry->flags & DirEntry_TypeDir)
		{
			DeleteAll(ps3_path + "/" + entry->name);
		}
	}
}

u64 VFS::GetDirSize(const std::string& ps3_path) const
{
	u64 result = 0;

	for (const auto entry : vfsDir(ps3_path))
	{
		if (entry->name == "." || entry->name == "..")
		{
			continue;
		}

		if (entry->flags & DirEntry_TypeFile)
		{
			result += entry->size;
		}

		if (entry->flags & DirEntry_TypeDir)
		{
			result += GetDirSize(ps3_path + "/" + entry->name);
		}
	}

	return result;
}

bool VFS::ExistsFile(const std::string& ps3_path) const
{
	std::string path;

	if (vfsDevice* dev = GetDevice(ps3_path, path))
	{
		// return dev->is_file(path);
		return fs::is_file(path);
	}

	return false;
}

bool VFS::ExistsDir(const std::string& ps3_path) const
{
	std::string path;

	if (vfsDevice* dev = GetDevice(ps3_path, path))
	{
		// return dev->is_dir(path);
		return fs::is_dir(path);
	}

	return false;
}

bool VFS::Exists(const std::string& ps3_path) const
{
	std::string path;

	if (vfsDevice* dev = GetDevice(ps3_path, path))
	{
		// return dev->exists(path);
		return fs::exists(path);
	}

	return false;
}

bool VFS::Rename(const std::string& ps3_path_from, const std::string& ps3_path_to) const
{
	std::string path_from, path_to;

	if (vfsDevice* dev = GetDevice(ps3_path_from, path_from))
	{
		if (vfsDevice* dev_ = GetDevice(ps3_path_to, path_to))
		{
			// return dev->rename(dev_, path_from, path_to);
			return fs::rename(path_from, path_to);
		}
	}

	return false;
}

bool VFS::CopyFile(const std::string& ps3_path_from, const std::string& ps3_path_to, bool overwrite) const
{
	std::string path_from, path_to;

	if (vfsDevice* dev = GetDevice(ps3_path_from, path_from))
	{
		if (vfsDevice* dev_ = GetDevice(ps3_path_to, path_to))
		{
			// return dev->copy_file(dev_, path_from, path_to, overwrite);
			return fs::copy_file(path_from, path_to, overwrite);
		}
	}

	return false;
}

bool VFS::TruncateFile(const std::string& ps3_path, u64 length) const
{
	std::string path;

	if (vfsDevice* dev = GetDevice(ps3_path, path))
	{
		// return dev->truncate_file(path, length);
		return fs::truncate_file(path, length);
	}

	return false;
}

vfsDevice* VFS::GetDevice(const std::string& ps3_path, std::string& path) const
{
	auto try_get_device = [this, &path](const std::string& ps3_path) -> vfsDevice*
	{
		std::vector<std::string> ps3_path_blocks = simplify_path_blocks(ps3_path);
		size_t max_eq = 0;
		int max_i = -1;

		for (u32 i = 0; i < m_devices.size(); ++i)
		{
			std::vector<std::string> dev_ps3_path_blocks = simplify_path_blocks(m_devices[i]->GetPs3Path());

			if (ps3_path_blocks.size() < dev_ps3_path_blocks.size())
			{
				continue;
			}

			size_t eq = 0;
			for (; eq < dev_ps3_path_blocks.size(); ++eq)
			{
				if (strcmp(ps3_path_blocks[eq].c_str(), dev_ps3_path_blocks[eq].c_str()))
				{
					break;
				}
			}

			if (eq > max_eq)
			{
				max_eq = eq;
				max_i = i;
			}
		}

		if (max_i < 0)
		{
			// Assume that the base dir of the file is /app_home/, if the mapping can't be found
			const auto usrdir = GetDevice("/app_home", path);

			if (!usrdir)
			{
				LOG_ERROR(GENERAL, "Getting /app_home couldn't be located, your VFS settings are most likely wrong.");
				return nullptr;
			}

			for (size_t i = max_eq; i < ps3_path_blocks.size(); i++)
			{
				path += "/" + ps3_path_blocks[i];
			}

			path = simplify_path(path, false, false);

			// Find the slot of /app_home/ mapping
			for (u32 i = 0; i < m_devices.size(); ++i)
			{
				if (m_devices[i]->GetPs3Path() == "/app_home/")
				{
					max_i = i;
					break;
				}
			}

			return m_devices[max_i];
		}

		path = m_devices[max_i]->GetLocalPath();

		for (size_t i = max_eq; i < ps3_path_blocks.size(); i++)
		{
			path += "/" + ps3_path_blocks[i];
		}

		path = simplify_path(path, false, false);
		
		return m_devices[max_i];
	};

	if (!ps3_path.size() || ps3_path[0] != '/')
	{
		return nullptr;
	}

	return try_get_device(GetLinked(ps3_path));

	// What is it? cwd is real path, ps3_path is ps3 path, but GetLinked accepts ps3 path
	//if (auto res = try_get_device(GetLinked(cwd + ps3_path))) 
	//	return res;
}

vfsDevice* VFS::GetDeviceLocal(const std::string& local_path, std::string& path) const
{
	int max_eq = -1;
	int max_i = -1;

	std::vector<std::string> local_path_blocks = simplify_path_blocks(local_path);

	for (u32 i = 0; i < m_devices.size(); ++i)
	{
		std::vector<std::string> dev_local_path_blocks = simplify_path_blocks(m_devices[i]->GetLocalPath());

		if (local_path_blocks.size() < dev_local_path_blocks.size())
			continue;

		int dev_blocks = dev_local_path_blocks.size();

		bool prefix_equal = std::equal(
			std::begin(dev_local_path_blocks),
			std::end(dev_local_path_blocks),
			std::begin(local_path_blocks),
			[](const std::string& a, const std::string& b){ return strcmp(a.c_str(), b.c_str()) == 0; }
		);
		
		if (prefix_equal && dev_blocks > max_eq)
		{
			max_eq = dev_blocks;
			max_i = i;
		}
	}

	if (max_i < 0)
		return nullptr;

	path = m_devices[max_i]->GetPs3Path();

	for (size_t i = max_eq; i < local_path_blocks.size(); i++)
	{
		path += "/" + local_path_blocks[i];
	}

	path = simplify_path(path, false, true);

	return m_devices[max_i];
}

void VFS::Init(const std::string& path)
{
	cwd = simplify_path(path, true, false);

	UnMountAll();

	std::vector<VFSManagerEntry> entries;
	SaveLoadDevices(entries, true);

	for(const VFSManagerEntry& entry : entries)
	{
		vfsDevice* dev;

		switch(entry.device)
		{
		case vfsDevice_LocalFile:
			dev = new vfsDeviceLocalFile();
		break;

		case vfsDevice_HDD:
			dev = new vfsDeviceHDD(entry.device_path);
		break;

		default:
			continue;
		}
		
		std::string mpath = entry.path;
		// TODO: This shouldn't use current dir
		// If no value assigned to SysEmulationDirPath in INI, use the path that with executable.
		if (Ini.SysEmulationDirPathEnable.GetValue())
		{
			fmt::Replace(mpath, "$(EmulatorDir)", Ini.SysEmulationDirPath.GetValue());
		}
		else
		{
			fmt::Replace(mpath, "$(EmulatorDir)", Emu.GetEmulatorPath());
		}
		fmt::Replace(mpath, "$(GameDir)", cwd);
		Mount(entry.mount, mpath, dev);
	}

	Link("/app_home/", "/host_root/" + cwd);
}

void VFS::SaveLoadDevices(std::vector<VFSManagerEntry>& res, bool is_load)
{
	IniEntry<int> entries_count;
	entries_count.Init("count", "VFSManager");

	int count = 0;
	if (is_load)
	{
		count = entries_count.LoadValue(count);

		if (!count)
		{
			res.emplace_back(vfsDevice_LocalFile, "$(EmulatorDir)/dev_hdd0/",   "/dev_hdd0/");
			res.emplace_back(vfsDevice_LocalFile, "$(EmulatorDir)/dev_hdd1/",   "/dev_hdd1/");
			res.emplace_back(vfsDevice_LocalFile, "$(EmulatorDir)/dev_flash/",  "/dev_flash/");
			res.emplace_back(vfsDevice_LocalFile, "$(EmulatorDir)/dev_usb000/", "/dev_usb000/");
			res.emplace_back(vfsDevice_LocalFile, "$(EmulatorDir)/dev_usb000/", "/dev_usb/");
			res.emplace_back(vfsDevice_LocalFile, "",                           "/host_root/");

			return;
		}

		res.resize(count);
	}
	else
	{
		count = (int)res.size();
		entries_count.SaveValue(count);
	}

	// Custom EmulationDir
	if (Ini.SysEmulationDirPathEnable.GetValue())
	{
		std::string dir = Ini.SysEmulationDirPath.GetValue();

		if (dir.empty())
		{
			Ini.SysEmulationDirPath.SetValue(Emu.GetEmulatorPath());
		}

		if (!fs::is_dir(dir))
		{
			LOG_ERROR(GENERAL, "Custom EmulationDir: directory '%s' not found", dir);
		}
		else
		{
			LOG_NOTICE(GENERAL, "Custom EmulationDir: $(EmulatorDir) bound to '%s'", dir);
		}
	}

	for(int i=0; i<count; ++i)
	{
		IniEntry<std::string> entry_path;
		IniEntry<std::string> entry_device_path;
		IniEntry<std::string> entry_mount;
		IniEntry<int> entry_device;

		entry_path.Init(fmt::format("path[%d]", i), "VFSManager");
		entry_device_path.Init(fmt::format("device_path[%d]", i), "VFSManager");
		entry_mount.Init(fmt::format("mount[%d]", i), "VFSManager");
		entry_device.Init(fmt::format("device[%d]", i), "VFSManager");
		
		if (is_load)
		{
			res[i] = VFSManagerEntry();
			res[i].path = entry_path.LoadValue("");
			res[i].device_path = entry_device_path.LoadValue("");
			res[i].mount = entry_mount.LoadValue("");
			res[i].device = (vfsDeviceType)entry_device.LoadValue(vfsDevice_LocalFile);
		}
		else
		{
			entry_path.SaveValue(res[i].path);
			entry_device_path.SaveValue(res[i].device_path);
			entry_mount.SaveValue(res[i].mount);
			entry_device.SaveValue(res[i].device);
		}
	}
}
