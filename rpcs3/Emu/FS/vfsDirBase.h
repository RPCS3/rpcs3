#pragma once

enum DirEntryFlags
{
	DirEntry_TypeDir = 0x1,
	DirEntry_TypeFile = 0x2,
	DirEntry_TypeMask = 0x3,
	DirEntry_PermWritable = 0x20,
	DirEntry_PermReadable = 0x40,
	DirEntry_PermExecutable = 0x80,
};

struct DirEntryInfo
{
	std::string name;
	u32 flags;
	time_t create_time;
	time_t access_time;
	time_t modify_time;

	DirEntryInfo()
		: flags(0)
		, create_time(0)
		, access_time(0)
		, modify_time(0)
	{
	}
};

class vfsDirBase
{
protected:
	std::string m_cwd;
	std::vector<DirEntryInfo> m_entries;
	uint m_pos;
	vfsDevice* m_device;

public:
	vfsDirBase(vfsDevice* device);
	virtual ~vfsDirBase();

	virtual bool Open(const std::string& path);
	virtual bool IsOpened() const;
	virtual bool IsExists(const std::string& path) const;
	virtual const std::vector<DirEntryInfo>& GetEntries() const;
	virtual void Close();
	virtual std::string GetPath() const;

	virtual bool Create(const std::string& path) = 0;
	//virtual bool Create(const DirEntryInfo& info)=0;
	virtual bool Rename(const std::string& from, const std::string& to) = 0;
	virtual bool Remove(const std::string& path) = 0;
	virtual const DirEntryInfo* Read();
};
