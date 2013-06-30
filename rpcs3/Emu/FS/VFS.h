#pragma once
#include "vfsDevice.h"

struct VFS
{
	ArrayF<vfsDevice> m_devices;

	void Mount(const wxString& ps3_path, const wxString& local_path, vfsDevice* device);
	void UnMount(const wxString& ps3_path);

	vfsStream* Open(const wxString& ps3_path, vfsOpenMode mode);
	void Create(const wxString& ps3_path);
	void Close(vfsStream*& device);
	vfsDevice* GetDevice(const wxString& ps3_path, wxString& path);
};