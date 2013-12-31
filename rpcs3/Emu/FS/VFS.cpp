#include "stdafx.h"
#include "VFS.h"
#include "Emu/HDD/HDD.h"

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
		std::qsort(m_devices.GetPtr(), m_devices.GetCount(), sizeof(vfsDevice*), sort_devices);
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

vfsStream* VFS::Open(const wxString& ps3_path, vfsOpenMode mode)
{
	vfsDevice* stream = nullptr;

	wxString path;
	if(vfsDevice* dev = GetDevice(ps3_path, path))
	{
		stream = dev->GetNew();
		stream->Open(path, mode);
	}

	return stream;
}

void VFS::Create(const wxString& ps3_path)
{
	wxString path;

	if(vfsDevice* dev = GetDevice(ps3_path, path))
	{
		dev->Create(path);
	}
}

void VFS::Close(vfsStream*& device)
{
	delete device;
	device = nullptr;
}

vfsDevice* VFS::GetDevice(const wxString& ps3_path, wxString& path)
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

vfsDevice* VFS::GetDeviceLocal(const wxString& local_path, wxString& path)
{
	u32 max_eq;
	s32 max_i=-1;

	for(u32 i=0; i<m_devices.GetCount(); ++i)
	{
		const u32 eq = m_devices[i].CmpLocalPath(local_path);

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
	Array<VFSManagerEntry> entries;
	SaveLoadDevices(entries, true);

	for(uint i=0; i<entries.GetCount(); ++i)
	{
		vfsDevice* dev;

		switch(entries[i].device)
		{
		case vfsDevice_LocalFile:
			dev = new vfsLocalFile();
		break;

		case vfsDevice_HDD:
			dev = new vfsHDD(entries[i].device_path);
		break;

		default:
			continue;
		}
		
		wxString mpath = entries[i].path;
		mpath.Replace("$(EmulatorDir)", wxGetCwd());
		mpath.Replace("$(GameDir)", vfsDevice::GetRoot(path));
		Mount(entries[i].mount, mpath, dev);
	}
}

void VFS::SaveLoadDevices(Array<VFSManagerEntry>& res, bool is_load)
{
	IniEntry<int> entries_count;
	entries_count.Init("count", "VFSManager");

	int count = 0;
	if(is_load)
	{
		count = entries_count.LoadValue(count);

		if(!count)
		{
			int idx;
			idx = res.Move(new VFSManagerEntry());
			res[idx].path = "$(EmulatorDir)\\dev_hdd0\\";
			res[idx].mount = "/dev_hdd0/";
			res[idx].device = vfsDevice_LocalFile;

			idx = res.Move(new VFSManagerEntry());
			res[idx].path = "$(EmulatorDir)\\dev_hdd1\\";
			res[idx].mount = "/dev_hdd1/";
			res[idx].device = vfsDevice_LocalFile;

			idx = res.Move(new VFSManagerEntry());
			res[idx].path = "$(EmulatorDir)\\dev_usb000\\";
			res[idx].mount = "/dev_usb000/";
			res[idx].device = vfsDevice_LocalFile;

			idx = res.Move(new VFSManagerEntry());
			res[idx].path = "$(EmulatorDir)\\dev_usb000\\";
			res[idx].mount = "/dev_usb/";
			res[idx].device = vfsDevice_LocalFile;

			idx = res.Move(new VFSManagerEntry());
			res[idx].path = "$(GameDir)";
			res[idx].mount = "/app_home/";
			res[idx].device = vfsDevice_LocalFile;

			idx = res.Move(new VFSManagerEntry());
			res[idx].path = "$(GameDir)\\..\\";
			res[idx].mount = "/dev_bdvd/";
			res[idx].device = vfsDevice_LocalFile;

			idx = res.Move(new VFSManagerEntry());
			res[idx].path = "";
			res[idx].mount = "/host_root/";
			res[idx].device = vfsDevice_LocalFile;

			idx = res.Move(new VFSManagerEntry());
			res[idx].path = "$(GameDir)";
			res[idx].mount = "/";
			res[idx].device = vfsDevice_LocalFile;

			return;
		}

		res.SetCount(count);
	}
	else
	{
		count = res.GetCount();
		entries_count.SaveValue(res.GetCount());
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
			new (res + i) VFSManagerEntry();
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
