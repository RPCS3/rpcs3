#pragma once
#include "vfsFileBase.h"
#include "vfsDirBase.h"

class vfsDevice
{
	std::string m_ps3_path;
	std::string m_local_path;
	mutable std::mutex m_mtx_lock;

public:
	vfsDevice(const std::string& ps3_path, const std::string& local_path);
	vfsDevice() {}

	virtual vfsFileBase* GetNewFileStream()=0;
	virtual vfsDirBase* GetNewDirStream()=0;

	std::string GetLocalPath() const;
	std::string GetPs3Path() const;

	void SetPath(const std::string& ps3_path, const std::string& local_path);

	u32 CmpPs3Path(const std::string& ps3_path);
	u32 CmpLocalPath(const std::string& local_path);

	static std::string ErasePath(const std::string& local_path, u32 start_dir_count, u32 end_dir_count);
	static std::string GetRoot(const std::string& local_path);
	static std::string GetRootPs3(const std::string& local_path);
	static std::string GetWinPath(const std::string& p, bool is_dir = true);
	static std::string GetWinPath(const std::string& l, const std::string& r);
	static std::string GetPs3Path(const std::string& p, bool is_dir = true);
	static std::string GetPs3Path(const std::string& l, const std::string& r);

	void Lock() const;
	void Unlock() const;
	bool TryLock() const;
};

class vfsDeviceLocker
{
	vfsDevice& m_device;

public:
	vfsDeviceLocker(vfsDevice& device) : m_device(device)
	{
		m_device.Lock();
	}

	~vfsDeviceLocker()
	{
		m_device.Unlock();
	}
};