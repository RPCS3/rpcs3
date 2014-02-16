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
	char* device_path;
	char* path;
	char* mount;
	vfsDeviceType device;

	VFSManagerEntry()
		: device(vfsDevice_LocalFile)
		, device_path("")
		, path("")
		, mount("")
	{
	}
};

struct VFS
{
	ArrayF<vfsDevice> m_devices;
	void Mount(const wxString& ps3_path, const wxString& local_path, vfsDevice* device);
	void UnMount(const wxString& ps3_path);
	void UnMountAll();

	std::shared_ptr<vfsFileBase> OpenFile(const wxString& ps3_path, vfsOpenMode mode) const;
	std::shared_ptr<vfsDirBase> OpenDir(const wxString& ps3_path) const;
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
	void SaveLoadDevices(Array<VFSManagerEntry>& res, bool is_load);
};