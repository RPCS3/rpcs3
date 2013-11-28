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

	VFSManagerEntry() : device(vfsDevice_LocalFile)
	{
	}
};

struct VFS
{
	ArrayF<vfsDevice> m_devices;
	void Mount(const wxString& ps3_path, const wxString& local_path, vfsDevice* device);
	void UnMount(const wxString& ps3_path);
	void UnMountAll();

	vfsStream* Open(const wxString& ps3_path, vfsOpenMode mode);
	void Create(const wxString& ps3_path);
	void Close(vfsStream*& device);
	vfsDevice* GetDevice(const wxString& ps3_path, wxString& path);
	vfsDevice* GetDeviceLocal(const wxString& local_path, wxString& path);

	void Init(const wxString& path);
	void SaveLoadDevices(Array<VFSManagerEntry>& res, bool is_load);
};