#pragma once
#include <map>

class vfsDevice;
struct vfsFileBase;
class vfsDirBase;
enum vfsOpenMode : u8;

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

std::vector<std::string> simplify_path_blocks(const std::string& path);
std::string simplify_path(const std::string& path, bool is_dir, bool is_ps3);

#undef CreateFile
#undef CopyFile

struct VFS
{
	~VFS();

	std::string cwd;

	//TODO: find out where these are supposed to be deleted or just make it shared_ptr
	//and also make GetDevice and GetDeviceLocal return shared_ptr then.
	// A vfsDevice will be deleted when they're unmounted or the VFS struct is destroyed.
	// This will cause problems if other code stores the pointer returned by GetDevice/GetDeviceLocal
	// and tries to use it after the device is unmounted.
	std::vector<vfsDevice *> m_devices;

	struct links_sorter
	{
		bool operator()(const std::vector<std::string>& a, const std::vector<std::string>& b) const
		{
			return b.size() < a.size();
		}
	};

	std::map<std::vector<std::string>, std::vector<std::string>, links_sorter> links;

	void Mount(const std::string& ps3_path, const std::string& local_path, vfsDevice* device);
	void Link(const std::string& mount_point, const std::string& ps3_path);
	void UnMount(const std::string& ps3_path);
	void UnMountAll();

	std::string GetLinked(const std::string& ps3_path) const;

	vfsFileBase* OpenFile(const std::string& ps3_path, vfsOpenMode mode) const;
	vfsDirBase* OpenDir(const std::string& ps3_path) const;
	bool CreateFile(const std::string& ps3_path, bool overwrite = false) const;
	bool CreateDir(const std::string& ps3_path) const;
	bool RemoveFile(const std::string& ps3_path) const;
	bool RemoveDir(const std::string& ps3_path) const;
	bool ExistsFile(const std::string& ps3_path) const;
	bool ExistsDir(const std::string& ps3_path) const;
	bool RenameFile(const std::string& ps3_path_from, const std::string& ps3_path_to) const;
	bool RenameDir(const std::string& ps3_path_from, const std::string& ps3_path_to) const;
	bool CopyFile(const std::string& ps3_path_from, const std::string& ps3_path_to, bool overwrite = true) const;
	bool TruncateFile(const std::string& ps3_path, u64 length) const;

	vfsDevice* GetDevice(const std::string& ps3_path, std::string& path) const;
	vfsDevice* GetDeviceLocal(const std::string& local_path, std::string& path) const;

	void Init(const std::string& path);
	void SaveLoadDevices(std::vector<VFSManagerEntry>& res, bool is_load);
};
