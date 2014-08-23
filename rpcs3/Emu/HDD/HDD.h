#pragma once
#include <algorithm>
#include "Emu/FS/vfsDevice.h"
#include "Emu/FS/vfsLocalFile.h"

static const u64 g_hdd_magic = *(u64*)"PS3eHDD\0";
static const u16 g_hdd_version = 0x0001;

struct vfsHDD_Block
{
	struct
	{
		u64 is_used    : 1;
		u64 next_block : 63;
	};
} static const g_null_block = {0}, g_used_block = {1};

struct vfsHDD_Hdr : public vfsHDD_Block
{
	u64 magic;
	u16 version;
	u64 block_count;
	u32 block_size;
};

enum vfsHDD_EntryType : u8
{
	vfsHDD_Entry_Dir = 0,
	vfsHDD_Entry_File = 1,
	vfsHDD_Entry_Link = 2,
};

struct vfsHDD_Entry : public vfsHDD_Block
{
	u64 data_block;
	vfsOpenMode access;
	vfsHDD_EntryType type;
	u64 size;
	u64 ctime;
	u64 mtime;
	u64 atime;
};

class vfsHDDManager
{
public:
	static void CreateBlock(vfsHDD_Block& block);

	static void CreateEntry(vfsHDD_Entry& entry);

	static void CreateHDD(const std::string& path, u64 size, u64 block_size);

	void Format();

	void AppendEntry(vfsHDD_Entry entry);
};


class vfsHDDFile
{
	u64 m_info_block;
	vfsHDD_Entry m_info;
	const vfsHDD_Hdr& m_hdd_info;
	vfsLocalFile& m_hdd;
	u32 m_position;
	u64 m_cur_block;

	bool goto_block(u64 n);

	void RemoveBlocks(u64 start_block);

	void WriteBlock(u64 block, const vfsHDD_Block& data);

	void ReadBlock(u64 block, vfsHDD_Block& data);

	void WriteEntry(u64 block, const vfsHDD_Entry& data);

	void ReadEntry(u64 block, vfsHDD_Entry& data);

	void ReadEntry(u64 block, vfsHDD_Entry& data, std::string& name);

	void ReadEntry(u64 block, std::string& name);

	void WriteEntry(u64 block, const vfsHDD_Entry& data, const std::string& name);

	__forceinline u32 GetMaxNameLen() const
	{
		return m_hdd_info.block_size - sizeof(vfsHDD_Entry);
	}

public:
	vfsHDDFile(vfsLocalFile& hdd, const vfsHDD_Hdr& hdd_info)
		: m_hdd(hdd)
		, m_hdd_info(hdd_info)
	{
	}

	~vfsHDDFile()
	{
	}

	void Open(u64 info_block);

	u64 FindFreeBlock();

	u64 GetSize() const
	{
		return m_info.size;
	}

	bool Seek(u64 pos);

	void SaveInfo();

	u64 Read(void* dst, u64 size);

	u64 Write(const void* src, u64 size);

	bool Eof() const
	{
		return m_info.size <= (m_cur_block * m_hdd_info.block_size + m_position);
	}
};

class vfsDeviceHDD : public vfsDevice
{
	std::string m_hdd_path;

public:
	vfsDeviceHDD(const std::string& hdd_path);

	virtual vfsFileBase* GetNewFileStream() override;
	virtual vfsDirBase* GetNewDirStream() override;
};

class vfsHDD : public vfsFileBase
{
	vfsHDD_Hdr m_hdd_info;
	vfsLocalFile m_hdd_file;
	const std::string& m_hdd_path;
	vfsHDD_Entry m_cur_dir;
	u64 m_cur_dir_block;
	vfsHDDFile m_file;

public:
	vfsHDD(vfsDevice* device, const std::string& hdd_path);

	__forceinline u32 GetMaxNameLen() const
	{
		return m_hdd_info.block_size - sizeof(vfsHDD_Entry);
	}

	bool SearchEntry(const std::string& name, u64& entry_block, u64* parent_block = nullptr);

	int OpenDir(const std::string& name);

	bool Rename(const std::string& from, const std::string& to);

	u64 FindFreeBlock();

	void WriteBlock(u64 block, const vfsHDD_Block& data);

	void ReadBlock(u64 block, vfsHDD_Block& data);

	void WriteEntry(u64 block, const vfsHDD_Entry& data);

	void ReadEntry(u64 block, vfsHDD_Entry& data);

	void ReadEntry(u64 block, vfsHDD_Entry& data, std::string& name);

	void ReadEntry(u64 block, std::string& name);

	void WriteEntry(u64 block, const vfsHDD_Entry& data, const std::string& name);

	bool Create(vfsHDD_EntryType type, const std::string& name);

	bool GetFirstEntry(u64& block, vfsHDD_Entry& entry, std::string& name);

	bool GetNextEntry(u64& block, vfsHDD_Entry& entry, std::string& name);

	virtual bool Open(const std::string& path, vfsOpenMode mode = vfsRead);

	bool HasEntry(const std::string& name);

	void RemoveBlocksDir(u64 start_block);

	void RemoveBlocksFile(u64 start_block);

	bool RemoveEntry(const std::string& name);

	virtual bool Create(const std::string& path);

	virtual u32 Write(const void* src, u32 size);

	virtual u32 Read(void* dst, u32 size);

	virtual u64 Seek(s64 offset, vfsSeekMode mode = vfsSeekSet);

	virtual bool Eof();

	virtual u64 GetSize();
};
