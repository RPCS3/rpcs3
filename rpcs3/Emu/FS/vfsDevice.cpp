#include "stdafx.h"
#include "vfsDevice.h"

vfsDevice::vfsDevice(const wxString& ps3_path, const wxString& local_path)
	: m_ps3_path(ps3_path)
	, m_local_path(GetWinPath(local_path))
{
}

wxString vfsDevice::GetLocalPath() const
{
	return m_local_path;
}

wxString vfsDevice::GetPs3Path() const
{
	return m_ps3_path;
}

void vfsDevice::SetPath(const wxString& ps3_path, const wxString& local_path)
{
	m_ps3_path = ps3_path;
	m_local_path = local_path;
}

u32 vfsDevice::CmpPs3Path(const wxString& ps3_path)
{
	const u32 lim = min(m_ps3_path.Len(), ps3_path.Len());
	u32 ret = 0;

	for(u32 i=0; i<lim; ++i, ++ret)
	{
		if(m_ps3_path[i] != ps3_path[i]) break;
	}

	return ret;
}

wxString vfsDevice::ErasePath(const wxString& path, u32 start_dir_count, u32 end_dir_count)
{
	u32 from = 0;
	u32 to = path.Len() - 1;

	for(uint i = 0, dir = 0; i < path.Len(); ++i)
	{
		if(path[i] == '\\' || path[i] == '/' || i == path.Len() - 1)
		{
			if(++dir == start_dir_count)
			{
				from = i;
				break;
			}
		}
	}

	for(int i = path.Len() - 1, dir = 0; i >= 0; --i)
	{
		if(path[i] == '\\' || path[i] == '/' || i == 0)
		{
			if(dir++ == end_dir_count)
			{
				to = i;
				break;
			}
		}
	}

	return path(from, to - from);
}

wxString vfsDevice::GetRoot(const wxString& path)
{
	if(path.IsEmpty()) return wxEmptyString;

	u32 first_dir = path.Len() - 1;

	for(int i = path.Len() - 1, dir = 0, li = path.Len() - 1; i >= 0 && dir < 2; --i)
	{
		if(path[i] == '\\' || path[i] == '/' || i == 0)
		{
			switch(dir++)
			{
			case 0:
				first_dir = i;
			break;

			case 1:
				if(!path(i + 1, li - i).Cmp("USRDIR")) return path(0, i + 1);
			continue;
			}
			
			li = i - 1;
		}
	}

	return path(0, first_dir + 1);
}

wxString vfsDevice::GetRootPs3(const wxString& path)
{
	if(path.IsEmpty()) return wxEmptyString;

	static const wxString& home = "/dev_hdd0/game/";
	u32 last_dir = 0;
	u32 first_dir = path.Len() - 1;

	for(int i = path.Len() - 1, dir = 0; i >= 0; --i)
	{
		if(path[i] == '\\' || path[i] == '/' || i == 0)
		{
			switch(dir++)
			{
			case 1:
				if(!!path(i + 1, last_dir - i - 1).Cmp("USRDIR")) return wxEmptyString;
			break;

			case 2:
				return GetPs3Path(home + path(i + 1, last_dir - i - 1));
			}

			last_dir = i;
		}
	}

	return GetPs3Path(home + path(0, last_dir - 1));
}

wxString vfsDevice::GetWinPath(const wxString& p, bool is_dir)
{
	if(p.IsEmpty()) return wxEmptyString;

	wxString ret;
	bool is_ls = false;

	for(u32 i=0; i<p.Len(); ++i)
	{
		if(p[i] == '/' || p[i] == '\\')
		{
			if(!is_ls)
			{
				ret += '\\';
				is_ls = true;
			}

			continue;
		}

		is_ls = false;
		ret += p[i];
	}

	if(is_dir && ret[ret.Len() - 1] != '\\') ret += '\\';

	return ret;
}

wxString vfsDevice::GetWinPath(const wxString& l, const wxString& r)
{
	if(l.IsEmpty()) return GetWinPath(r, false);
	if(r.IsEmpty()) return GetWinPath(l);

	return GetWinPath(l + '\\' + r, false);
}

wxString vfsDevice::GetPs3Path(const wxString& p, bool is_dir)
{
	if(p.IsEmpty()) return wxEmptyString;

	wxString ret;
	bool is_ls = false;

	for(u32 i=0; i<p.Len(); ++i)
	{
		if(p[i] == '/' || p[i] == '\\')
		{
			if(!is_ls)
			{
				ret += '/';
				is_ls = true;
			}

			continue;
		}

		is_ls = false;
		ret += p[i];
	}

	if(ret[0] != '/') ret = '/' + ret;
	if(is_dir && ret[ret.Len() - 1] != '/') ret += '/';

	return ret;
}

wxString vfsDevice::GetPs3Path(const wxString& l, const wxString& r)
{
	if(l.IsEmpty()) return GetPs3Path(r, false);
	if(r.IsEmpty()) return GetPs3Path(l);

	return GetPs3Path(l + '/' + r, false);
}

void vfsDevice::Lock() const
{
	m_mtx_lock.lock();
}

void vfsDevice::Unlock() const
{
	m_mtx_lock.unlock();
}

bool vfsDevice::TryLock() const
{
	return m_mtx_lock.try_lock();
}