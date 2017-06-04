#pragma once

class vfsDevice;

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
	u64 size;
	time_t create_time;
	time_t access_time;
	time_t modify_time;

	DirEntryInfo()
		: flags(0)
		, size(0)
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
	virtual const std::vector<DirEntryInfo>& GetEntries() const;
	virtual void Close();
	virtual std::string GetPath() const;

	virtual const DirEntryInfo* Read();
	virtual const DirEntryInfo* First();

	class iterator
	{
		vfsDirBase *parent;
		const DirEntryInfo* data;

	public:
		iterator(vfsDirBase* parent)
			: parent(parent)
			, data(parent->First())
		{
		}

		iterator(vfsDirBase* parent, const DirEntryInfo* data)
			: parent(parent)
			, data(data)
		{
		}

		iterator& operator++()
		{
			data = parent->Read();
			return *this;
		}

		iterator operator++(int)
		{
			const DirEntryInfo* olddata = data;
			data = parent->Read();
			return iterator(parent, olddata);
		}

		const DirEntryInfo* operator *()
		{
			return data;
		}

		bool operator !=(iterator other) const
		{
			return data != other.data;
		}
	};

	iterator begin()
	{
		return iterator(this);
	}

	iterator end()
	{
		return iterator(this, nullptr);
	}
};
