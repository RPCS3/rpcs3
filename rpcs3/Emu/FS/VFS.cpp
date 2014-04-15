#include "stdafx.h"
#include "VFS.h"
#include "Emu/HDD/HDD.h"
#include "vfsDeviceLocalFile.h"

int sort_devices(const void* _a, const void* _b)
{
	const vfsDevice& a =  **(const vfsDevice**)_a;
	const vfsDevice& b =  **(const vfsDevice**)_b;

	if(a.GetPs3Path().length() > b.GetPs3Path().length()) return  1;
	if(a.GetPs3Path().length() < b.GetPs3Path().length()) return -1;

	return 0;
}

void VFS::Mount(const std::string& ps3_path, const std::string& local_path, vfsDevice* device)
{
	UnMount(ps3_path);

	device->SetPath(ps3_path, local_path);
	m_devices.push_back(device);

	if(m_devices.size() > 1)
	{
		//std::qsort(m_devices.GetPtr(), m_devices.GetCount(), sizeof(vfsDevice*), sort_devices);
	}
}

void VFS::UnMount(const std::string& ps3_path)
{
	for(u32 i=0; i<m_devices.size(); ++i)
	{
		if(!m_devices[i]->GetPs3Path().compare(ps3_path))
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
		m_devices.erase(m_devices.begin() +i);
	}
}

vfsFileBase* VFS::OpenFile(const std::string& ps3_path, vfsOpenMode mode) const
{
	std::string path;
	if(vfsDevice* dev = GetDevice(ps3_path, path))
	{
		if(vfsFileBase* res = dev->GetNewFileStream())
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

	if(vfsDevice* dev = GetDevice(ps3_path, path))
	{
		if(vfsDirBase* res = dev->GetNewDirStream())
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
	if(vfsDevice* dev = GetDevice(ps3_path, path))
	{
		std::shared_ptr<vfsFileBase> res(dev->GetNewFileStream());

		if(res)
		{
			return res->Create(path);
		}
	}

	return false;
}

bool VFS::CreateDir(const std::string& ps3_path) const
{
	std::string path;
	if(vfsDevice* dev = GetDevice(ps3_path, path))
	{
		std::shared_ptr<vfsDirBase> res(dev->GetNewDirStream());

		if(res)
		{
			return res->Create(path);
		}
	}

	return false;
}

bool VFS::RemoveFile(const std::string& ps3_path) const
{
	std::string path;
	if(vfsDevice* dev = GetDevice(ps3_path, path))
	{
		std::shared_ptr<vfsFileBase> res(dev->GetNewFileStream());

		if(res)
		{
			return res->Remove(path);
		}
	}

	return false;
}

bool VFS::RemoveDir(const std::string& ps3_path) const
{
	std::string path;
	if(vfsDevice* dev = GetDevice(ps3_path, path))
	{
		std::shared_ptr<vfsDirBase> res(dev->GetNewDirStream());

		if(res)
		{
			return res->Remove(path);
		}
	}

	return false;
}

bool VFS::ExistsFile(const std::string& ps3_path) const
{
	std::string path;
	if(vfsDevice* dev = GetDevice(ps3_path, path))
	{
		std::shared_ptr<vfsFileBase> res(dev->GetNewFileStream());

		if(res)
		{
			return res->Exists(path);
		}
	}

	return false;
}

bool VFS::ExistsDir(const std::string& ps3_path) const
{
	std::string path;
	if(vfsDevice* dev = GetDevice(ps3_path, path))
	{
		std::shared_ptr<vfsDirBase> res(dev->GetNewDirStream());

		if(res)
		{
			return res->IsExists(path);
		}
	}

	return false;
}

bool VFS::RenameFile(const std::string& ps3_path_from, const std::string& ps3_path_to) const
{
	std::string path;
	if(vfsDevice* dev = GetDevice(ps3_path_from, path))
	{
		std::shared_ptr<vfsFileBase> res(dev->GetNewFileStream());

		if(res)
		{
			return res->Rename(path, ps3_path_to);
		}
	}

	return false;
}

bool VFS::RenameDir(const std::string& ps3_path_from, const std::string& ps3_path_to) const
{
	std::string path;
	if(vfsDevice* dev = GetDevice(ps3_path_from, path))
	{
		std::shared_ptr<vfsDirBase> res(dev->GetNewDirStream());

		if(res)
		{
			return res->Rename(path, ps3_path_to);
		}
	}

	return false;
}

vfsDevice* VFS::GetDevice(const std::string& ps3_path, std::string& path) const
{
	u32 max_eq;
	s32 max_i=-1;

	for(u32 i=0; i<m_devices.size(); ++i)
	{
		const u32 eq = m_devices[i]->CmpPs3Path(ps3_path);

		if(max_i < 0 || eq > max_eq)
		{
			max_eq = eq;
			max_i = i;
		}
	}

	if(max_i < 0) return nullptr;
	path = vfsDevice::GetWinPath(m_devices[max_i]->GetLocalPath(), ps3_path.substr(max_eq, ps3_path.length() - max_eq));
	return m_devices[max_i];
}

vfsDevice* VFS::GetDeviceLocal(const std::string& local_path, std::string& path) const
{
	u32 max_eq;
	s32 max_i=-1;

	wxFileName file_path(fmt::FromUTF8(local_path));
	file_path.Normalize();
	std::string mormalized_path = fmt::ToUTF8(file_path.GetFullPath());

	for(u32 i=0; i<m_devices.size(); ++i)
	{
		const u32 eq = m_devices[i]->CmpLocalPath(mormalized_path);

		if(max_i < 0 || eq > max_eq)
		{
			max_eq = eq;
			max_i = i;
		}
	}

	if(max_i < 0) return nullptr;

	path = vfsDevice::GetPs3Path(m_devices[max_i]->GetPs3Path(), local_path.substr(max_eq, local_path.length() - max_eq));
	return m_devices[max_i];
}

void VFS::Init(const std::string& path)
{
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
		
		wxString mpath = entry.path;
		mpath.Replace("$(EmulatorDir)", wxGetCwd());
		mpath.Replace("$(GameDir)", fmt::FromUTF8(vfsDevice::GetRoot(path)));
		Mount(entry.mount, fmt::ToUTF8(mpath), dev);
	}
}

void VFS::SaveLoadDevices(std::vector<VFSManagerEntry>& res, bool is_load)
{
	IniEntry<int> entries_count;
	entries_count.Init("count", "VFSManager");

	int count = 0;
	if(is_load)
	{
		count = entries_count.LoadValue(count);

		if(!count)
		{
			res.emplace_back(vfsDevice_LocalFile, "$(EmulatorDir)/dev_hdd0/",   "/dev_hdd0/");
			res.emplace_back(vfsDevice_LocalFile, "$(EmulatorDir)/dev_hdd1/",   "/dev_hdd1/");
			res.emplace_back(vfsDevice_LocalFile, "$(EmulatorDir)/dev_flash/",  "/dev_flash/");
			res.emplace_back(vfsDevice_LocalFile, "$(EmulatorDir)/dev_usb000/", "/dev_usb000/");
			res.emplace_back(vfsDevice_LocalFile, "$(EmulatorDir)/dev_usb000/", "/dev_usb/");
			res.emplace_back(vfsDevice_LocalFile, "$(GameDir)",                 "/app_home/");
			res.emplace_back(vfsDevice_LocalFile, "$(GameDir)/../",             "/dev_bdvd/");
			res.emplace_back(vfsDevice_LocalFile, "",                           "/host_root/");
			res.emplace_back(vfsDevice_LocalFile, "$(GameDir)",                 "/");

			return;
		}

		res.resize(count);
	}
	else
	{
		count = res.size();
		entries_count.SaveValue(res.size());
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
		
		if(is_load)
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
