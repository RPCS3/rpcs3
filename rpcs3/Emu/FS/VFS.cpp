#include "stdafx.h"
#include "VFS.h"
#include "Emu/HDD/HDD.h"
#include "vfsDeviceLocalFile.h"

int sort_devices(const void* _a, const void* _b)
{
	const vfsDevice& a =  **(const vfsDevice**)_a;
	const vfsDevice& b =  **(const vfsDevice**)_b;

	if(a.GetPs3Path().Len() > b.GetPs3Path().Len()) return  1;
	if(a.GetPs3Path().Len() < b.GetPs3Path().Len()) return -1;

	return 0;
}

void VFS::Mount(const wxString& ps3_path, const wxString& local_path, vfsDevice* device)
{
	UnMount(ps3_path);

	device->SetPath(ps3_path, local_path);
	m_devices.Add(device);

	if(m_devices.GetCount() > 1)
	{
		//std::qsort(m_devices.GetPtr(), m_devices.GetCount(), sizeof(vfsDevice*), sort_devices);
	}
}

void VFS::UnMount(const wxString& ps3_path)
{
	for(u32 i=0; i<m_devices.GetCount(); ++i)
	{
		if(!m_devices[i].GetPs3Path().Cmp(ps3_path))
		{
			delete &m_devices[i];
			m_devices.RemoveFAt(i);

			return;
		}
	}
}

void VFS::UnMountAll()
{
	for(u32 i=0; i<m_devices.GetCount(); ++i)
	{
		delete &m_devices[i];
		m_devices.RemoveFAt(i);
	}
}

vfsFileBase* VFS::OpenFile(const wxString& ps3_path, vfsOpenMode mode) const
{
	wxString path;
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

vfsDirBase* VFS::OpenDir(const wxString& ps3_path) const
{
	wxString path;

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

bool VFS::CreateFile(const wxString& ps3_path) const
{
	wxString path;
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

bool VFS::CreateDir(const wxString& ps3_path) const
{
	wxString path;
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

bool VFS::RemoveFile(const wxString& ps3_path) const
{
	wxString path;
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

bool VFS::RemoveDir(const wxString& ps3_path) const
{
	wxString path;
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

bool VFS::ExistsFile(const wxString& ps3_path) const
{
	wxString path;
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

bool VFS::ExistsDir(const wxString& ps3_path) const
{
	wxString path;
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

bool VFS::RenameFile(const wxString& ps3_path_from, const wxString& ps3_path_to) const
{
	wxString path;
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

bool VFS::RenameDir(const wxString& ps3_path_from, const wxString& ps3_path_to) const
{
	wxString path;
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

vfsDevice* VFS::GetDevice(const wxString& ps3_path, wxString& path) const
{
	u32 max_eq;
	s32 max_i=-1;

	for(u32 i=0; i<m_devices.GetCount(); ++i)
	{
		const u32 eq = m_devices[i].CmpPs3Path(ps3_path);

		if(max_i < 0 || eq > max_eq)
		{
			max_eq = eq;
			max_i = i;
		}
	}

	if(max_i < 0) return nullptr;
	path = vfsDevice::GetWinPath(m_devices[max_i].GetLocalPath(), ps3_path(max_eq, ps3_path.Len() - max_eq));
	return &m_devices[max_i];
}

vfsDevice* VFS::GetDeviceLocal(const wxString& local_path, wxString& path) const
{
	u32 max_eq;
	s32 max_i=-1;

	wxFileName file_path(local_path);
	file_path.Normalize();
	wxString mormalized_path = file_path.GetFullPath();

	for(u32 i=0; i<m_devices.GetCount(); ++i)
	{
		const u32 eq = m_devices[i].CmpLocalPath(mormalized_path);

		if(max_i < 0 || eq > max_eq)
		{
			max_eq = eq;
			max_i = i;
		}
	}

	if(max_i < 0) return nullptr;

	path = vfsDevice::GetPs3Path(m_devices[max_i].GetPs3Path(), local_path(max_eq, local_path.Len() - max_eq));
	return &m_devices[max_i];
}

void VFS::Init(const wxString& path)
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
		mpath.Replace("$(GameDir)", vfsDevice::GetRoot(path));
		Mount(entry.mount, mpath, dev);
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
		IniEntry<wxString> entry_path;
		IniEntry<wxString> entry_device_path;
		IniEntry<wxString> entry_mount;
		IniEntry<int> entry_device;

		entry_path.Init(wxString::Format("path[%d]", i), "VFSManager");
		entry_device_path.Init(wxString::Format("device_path[%d]", i), "VFSManager");
		entry_mount.Init(wxString::Format("mount[%d]", i), "VFSManager");
		entry_device.Init(wxString::Format("device[%d]", i), "VFSManager");
		
		if(is_load)
		{
			res[i] = VFSManagerEntry();
			res[i].path = strdup(entry_path.LoadValue(wxEmptyString).c_str());
			res[i].device_path = strdup(entry_device_path.LoadValue(wxEmptyString).c_str());
			res[i].mount = strdup(entry_mount.LoadValue(wxEmptyString).c_str());
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
