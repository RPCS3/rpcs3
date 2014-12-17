#include "stdafx.h"
#include "VFS.h"
#include "vfsDirBase.h"
#include "Emu/HDD/HDD.h"
#include "vfsDeviceLocalFile.h"
#include "Ini.h"
#include "Emu/System.h"

#undef CreateFile

std::vector<std::string> simplify_path_blocks(const std::string& path)
{
	std::vector<std::string> path_blocks = std::move(fmt::split(fmt::tolower(path), { "/", "\\" }));

	for (size_t i = 0; i < path_blocks.size(); ++i)
	{
		if (path_blocks[i] == ".")
		{
			path_blocks.erase(path_blocks.begin() + i--);
		}
		else if (i && path_blocks[i] == "..")
		{
			path_blocks.erase(path_blocks.begin() + (i - 1), path_blocks.begin() + (i + 1));
			i--;
		}
	}

	return path_blocks;
}

std::string simplify_path(const std::string& path, bool is_dir)
{
	std::vector<std::string> path_blocks = simplify_path_blocks(path);

	std::string result;

	if (path_blocks.empty())
		return result;

	if (is_dir)
	{
		result = fmt::merge(path_blocks, "/");
	}
	else
	{
		result = fmt::merge(std::vector<std::string>(path_blocks.begin(), path_blocks.end() - 1), "/") + path_blocks[path_blocks.size() - 1];
	}

	return result;
}

VFS::~VFS()
{
	UnMountAll();
}

void VFS::Mount(const std::string& ps3_path, const std::string& local_path, vfsDevice* device)
{
	std::string simpl_ps3_path = simplify_path(ps3_path, true);

	UnMount(simpl_ps3_path);

	device->SetPath(simpl_ps3_path, simplify_path(local_path, true));
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

std::string VFS::GetLinked(std::string ps3_path) const
{
	ps3_path = fmt::tolower(ps3_path);
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
	std::string simpl_ps3_path = simplify_path(ps3_path, true);

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

vfsFileBase* VFS::OpenFile(const std::string& ps3_path, vfsOpenMode mode) const
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

bool VFS::CreateFile(const std::string& ps3_path) const
{
	std::string path;
	if (vfsDevice* dev = GetDevice(ps3_path, path))
	{
		std::shared_ptr<vfsFileBase> res(dev->GetNewFileStream());

		if (res)
		{
			return res->Create(path);
		}
	}

	return false;
}

bool VFS::CreateDir(const std::string& ps3_path) const
{
	std::string path;
	if (vfsDevice* dev = GetDevice(ps3_path, path))
	{
		std::shared_ptr<vfsDirBase> res(dev->GetNewDirStream());

		if (res)
		{
			return res->Create(path);
		}
	}

	return false;
}

bool VFS::RemoveFile(const std::string& ps3_path) const
{
	std::string path;
	if (vfsDevice* dev = GetDevice(ps3_path, path))
	{
		std::shared_ptr<vfsFileBase> res(dev->GetNewFileStream());

		if (res)
		{
			return res->Remove(path);
		}
	}

	return false;
}

bool VFS::RemoveDir(const std::string& ps3_path) const
{
	std::string path;
	if (vfsDevice* dev = GetDevice(ps3_path, path))
	{
		std::shared_ptr<vfsDirBase> res(dev->GetNewDirStream());

		if (res)
		{
			return res->Remove(path);
		}
	}

	return false;
}

bool VFS::ExistsFile(const std::string& ps3_path) const
{
	std::string path;
	if (vfsDevice* dev = GetDevice(ps3_path, path))
	{
		std::shared_ptr<vfsFileBase> res(dev->GetNewFileStream());

		if (res)
		{
			return res->Exists(path);
		}
	}

	return false;
}

bool VFS::ExistsDir(const std::string& ps3_path) const
{
	std::string path;
	if (vfsDevice* dev = GetDevice(ps3_path, path))
	{
		std::shared_ptr<vfsDirBase> res(dev->GetNewDirStream());

		if (res)
		{
			return res->IsExists(path);
		}
	}

	return false;
}

bool VFS::RenameFile(const std::string& ps3_path_from, const std::string& ps3_path_to) const
{
	std::string path;
	if (vfsDevice* dev = GetDevice(ps3_path_from, path))
	{
		std::shared_ptr<vfsFileBase> res(dev->GetNewFileStream());

		if (res)
		{
			return res->Rename(path, ps3_path_to);
		}
	}

	return false;
}

bool VFS::RenameDir(const std::string& ps3_path_from, const std::string& ps3_path_to) const
{
	std::string path;
	if (vfsDevice* dev = GetDevice(ps3_path_from, path))
	{
		std::shared_ptr<vfsDirBase> res(dev->GetNewDirStream());

		if (res)
		{
			return res->Rename(path, ps3_path_to);
		}
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
				continue;

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
			return nullptr;

		path = m_devices[max_i]->GetLocalPath();

		for (u32 i = max_eq; i < ps3_path_blocks.size(); i++)
		{
			path += "/" + ps3_path_blocks[i];
		}

		path = simplify_path(path, false);

		return m_devices[max_i];
	};

	if (auto res = try_get_device(GetLinked(ps3_path)))
		return res;

	if (auto res = try_get_device(GetLinked(cwd + ps3_path)))
		return res;

	return nullptr;
}

vfsDevice* VFS::GetDeviceLocal(const std::string& local_path, std::string& path) const
{
	size_t max_eq = 0;
	int max_i = -1;

	std::vector<std::string> local_path_blocks = simplify_path_blocks(local_path);

	for (u32 i = 0; i < m_devices.size(); ++i)
	{
		std::vector<std::string> dev_local_path_blocks_blocks = simplify_path_blocks(m_devices[i]->GetLocalPath());

		if (local_path_blocks.size() < dev_local_path_blocks_blocks.size())
			continue;

		size_t eq = 0;
		for (; eq < dev_local_path_blocks_blocks.size(); ++eq)
		{
			if (strcmp(local_path_blocks[eq].c_str(), dev_local_path_blocks_blocks[eq].c_str()))
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
		return nullptr;

	path = m_devices[max_i]->GetPs3Path();

	for (u32 i = max_eq; i < local_path_blocks.size(); i++)
	{
		path += "/" + local_path_blocks[i];
	}

	path = simplify_path(path, false);

	return m_devices[max_i];
}

void VFS::Init(const std::string& path)
{
	cwd = simplify_path(path, true);

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
		fmt::Replace(mpath, "$(EmulatorDir)", Emu.GetEmulatorPath());
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
			res.emplace_back(vfsDevice_LocalFile, "$(GameDir)/../../",          "/dev_bdvd/");
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

	for(int i=0; i<count; ++i)
	{
		IniEntry<std::string> entry_path;
		IniEntry<std::string> entry_device_path;
		IniEntry<std::string> entry_mount;
		IniEntry<int> entry_device;

		entry_path.Init(fmt::Format("path[%d]", i), "VFSManager");
		entry_device_path.Init(fmt::Format("device_path[%d]", i), "VFSManager");
		entry_mount.Init(fmt::Format("mount[%d]", i), "VFSManager");
		entry_device.Init(fmt::Format("device[%d]", i), "VFSManager");
		
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
