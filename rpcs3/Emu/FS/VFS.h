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
	std::string device_path;
	std::string path;
	std::string mount;

	VFSManagerEntry()
		: device(vfsDevice_LocalFile)
		, device_path("")
		, path("")
		, mount("")
	{
	}

	VFSManagerEntry(const vfsDeviceType& device, const std::string& path, const std::string& mount)
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
	void Mount(const std::string& ps3_path, const std::string& local_path, vfsDevice* device);
	void UnMount(const std::string& ps3_path);
	void UnMountAll();

	vfsFileBase* OpenFile(const std::string& ps3_path, vfsOpenMode mode) const;
	vfsDirBase* OpenDir(const std::string& ps3_path) const;
	bool CreateFile(const std::string& ps3_path) const;
	bool CreateDir(const std::string& ps3_path) const;
	bool RemoveFile(const std::string& ps3_path) const;
	bool RemoveDir(const std::string& ps3_path) const;
	bool ExistsFile(const std::string& ps3_path) const;
	bool ExistsDir(const std::string& ps3_path) const;
	bool RenameFile(const std::string& ps3_path_from, const std::string& ps3_path_to) const;
	bool RenameDir(const std::string& ps3_path_from, const std::string& ps3_path_to) const;

	vfsDevice* GetDevice(const std::string& ps3_path, std::string& path) const;
	vfsDevice* GetDeviceLocal(const std::string& local_path, std::string& path) const;

	void Init(const std::string& path);
	void SaveLoadDevices(std::vector<VFSManagerEntry>& res, bool is_load);
};