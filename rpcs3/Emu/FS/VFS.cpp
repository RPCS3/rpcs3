#include "stdafx.h"
#include "VFS.h"

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
			m_devices.RemoveAt(i);

			return;
		}
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
