#pragma once

#include "vfsDevice.h"

enum vfsDeviceType
{
	vfsDevice_LocalFile,
	vfsDevice_HDD,
};

static const char* vfsDeviceTypeNames[] = 
{
	"Local",
	"HDD",
};

struct VFSManagerEntry
{
	vfsDeviceType device;
	const char* device_path;
	const char* path;
	const char* mount;

	VFSManagerEntry()
		: device(vfsDevice_LocalFile)
		, device_path("")
		, path("")
		, mount("")
	{
	}

	VFSManagerEntry(const vfsDeviceType& device, const char* path, const char* mount)
		: device(device)
		, device_path("")
		, path(path)
		, mount(mount)

	{
	}
};

struct VFS
{
	ArrayF<vfsDevice> m_devices;
	void Mount(const wxString& ps3_path, const wxString& local_path, vfsDevice* device);
	void UnMount(const wxString& ps3_path);
	void UnMountAll();

	vfsFileBase* OpenFile(const wxString& ps3_path, vfsOpenMode mode) const;
	vfsDirBase* OpenDir(const wxString& ps3_path) const;
	bool CreateFile(const wxString& ps3_path) const;
	bool CreateDir(const wxString& ps3_path) const;
	bool RemoveFile(const wxString& ps3_path) const;
	bool RemoveDir(const wxString& ps3_path) const;
	bool ExistsFile(const wxString& ps3_path) const;
	bool ExistsDir(const wxString& ps3_path) const;
	bool RenameFile(const wxString& ps3_path_from, const wxString& ps3_path_to) const;
	bool RenameDir(const wxString& ps3_path_from, const wxString& ps3_path_to) const;

	vfsDevice* GetDevice(const wxString& ps3_path, wxString& path) const;
	vfsDevice* GetDeviceLocal(const wxString& local_path, wxString& path) const;

	void Init(const wxString& path);
	void SaveLoadDevices(std::vector<VFSManagerEntry>& res, bool is_load);
};