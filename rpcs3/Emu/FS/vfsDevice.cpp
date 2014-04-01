#include "stdafx.h"
#include "vfsDevice.h"

vfsDevice::vfsDevice(const std::string& ps3_path, const std::string& local_path)
	: m_ps3_path(ps3_path)
	, m_local_path(GetWinPath(local_path))
{
}

std::string vfsDevice::GetLocalPath() const
{
	return m_local_path;
}

std::string vfsDevice::GetPs3Path() const
{
	return m_ps3_path;
}

void vfsDevice::SetPath(const std::string& ps3_path, const std::string& local_path)
{
	m_ps3_path = ps3_path;
	m_local_path = local_path;
}

u32 vfsDevice::CmpPs3Path(const std::string& ps3_path)
{
	const u32 lim = min(m_ps3_path.length(), ps3_path.length());
	u32 ret = 0;

	for(u32 i=0; i<lim; ++i, ++ret)
	{
		if(m_ps3_path[i] != ps3_path[i]) 
		{
			ret = 0;
			break;
		}
	}

	return ret;
}

u32 vfsDevice::CmpLocalPath(const std::string& local_path)
{
	if(local_path.length() < m_local_path.length())
		return 0;

	wxFileName path0(fmt::FromUTF8(m_local_path));
	path0.Normalize();

#ifdef _WIN32
#define DL '\\'
#else
#define DL '/'
#endif

	wxArrayString arr0 = wxSplit(path0.GetFullPath(), DL);
	wxArrayString arr1 = wxSplit(fmt::FromUTF8(local_path), DL);

	const u32 lim = min(arr0.GetCount(), arr1.GetCount());
	u32 ret = 0;

	for(u32 i=0; i<lim; ret += arr0[i++].Len() + 1)
	{
		if(arr0[i].CmpNoCase(arr1[i]) != 0)
		{
			break;
		}
	}

	return ret;
}

std::string vfsDevice::ErasePath(const std::string& path, u32 start_dir_count, u32 end_dir_count)
{
	u32 from = 0;
	u32 to = path.length() - 1;

	for(uint i = 0, dir = 0; i < path.length(); ++i)
	{
		if(path[i] == '\\' || path[i] == '/' || i == path.length() - 1)
		{
			if(++dir == start_dir_count)
			{
				from = i;
				break;
			}
		}
	}

	for(int i = path.length() - 1, dir = 0; i >= 0; --i)
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

	return path.substr(from, to - from);
}

std::string vfsDevice::GetRoot(const std::string& path)
{
	//return fmt::ToUTF8(wxFileName(fmt::FromUTF8(path), wxPATH_UNIX).GetPath());
	if(path.empty()) return "";

	u32 first_dir = path.length() - 1;

	for(int i = path.length() - 1, dir = 0, li = path.length() - 1; i >= 0 && dir < 2; --i)
	{
		if(path[i] == '\\' || path[i] == '/' || i == 0)
		{
			switch(dir++)
			{
			case 0:
				first_dir = i;
			break;

			case 1:
				if(!path.substr(i + 1, li - i).compare("USRDIR")) return path.substr(0, i + 1);
			continue;
			}
			
			li = i - 1;
		}
	}

	return path.substr(0, first_dir + 1);
}

std::string vfsDevice::GetRootPs3(const std::string& path)
{
	if(path.empty()) return "";

	static const std::string home = "/dev_hdd0/game/";
	u32 last_dir = 0;
	u32 first_dir = path.length() - 1;

	for(int i = path.length() - 1, dir = 0; i >= 0; --i)
	{
		if(path[i] == '\\' || path[i] == '/' || i == 0)
		{
			switch(dir++)
			{
			case 1:
				if(path.substr(i + 1, last_dir - i - 1) == "USRDIR") return "";
			break;

			case 2:
				return GetPs3Path(home + path.substr(i + 1, last_dir - i - 1));
			}

			last_dir = i;
		}
	}

	return GetPs3Path(home + path.substr(0, last_dir - 1));
}

std::string vfsDevice::GetWinPath(const std::string& p, bool is_dir)
{
	if(p.empty()) return "";

	std::string ret;
	bool is_ls = false;

	for(u32 i=0; i<p.length(); ++i)
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

	if(is_dir && ret[ret.length() - 1] != '/' && ret[ret.length() - 1] != '\\') ret += '/'; // ???

	wxFileName res(fmt::FromUTF8(ret));
	res.Normalize();
	return fmt::ToUTF8(res.GetFullPath());
}

std::string vfsDevice::GetWinPath(const std::string& l, const std::string& r)
{
	if(l.empty()) return GetWinPath(r, false);
	if(r.empty()) return GetWinPath(l);

	return GetWinPath(l + '/' + r, false);
}

std::string vfsDevice::GetPs3Path(const std::string& p, bool is_dir)
{
	if(p.empty()) return "";

	std::string ret;
	bool is_ls = false;

	for(u32 i=0; i<p.length(); ++i)
	{
		if(p[i] == L'/' || p[i] == L'\\')
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
	if(is_dir && ret[ret.length() - 1] != '/') ret += '/';

	return ret;
}

std::string vfsDevice::GetPs3Path(const std::string& l, const std::string& r)
{
	if(l.empty()) return GetPs3Path(r, false);
	if(r.empty()) return GetPs3Path(l);

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