#pragma once
#include "Emu/FS/vfsDevice.h"

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
	static void CreateBlock(vfsHDD_Block& block)
	{
		block.is_used = true;
		block.next_block = 0;
	}

	static void CreateEntry(vfsHDD_Entry& entry)
	{
		memset(&entry, 0, sizeof(vfsHDD_Entry));
		u64 ctime = time(nullptr);
		entry.atime = ctime;
		entry.ctime = ctime;
		entry.mtime = ctime;
		entry.access = vfsReadWrite;
		CreateBlock(entry);
	}

	static void CreateHDD(const std::string& path, u64 size, u64 block_size)
	{
		wxFile f(fmt::FromUTF8(path), wxFile::write);

		static const u64 cur_dir_block = 1;

		vfsHDD_Hdr hdr;
		CreateBlock(hdr);
		hdr.next_block = cur_dir_block;
		hdr.magic = g_hdd_magic;
		hdr.version = g_hdd_version;
		hdr.block_count = (size + block_size) / block_size;
		hdr.block_size = block_size;
		f.Write(&hdr, sizeof(vfsHDD_Hdr));

		{
			vfsHDD_Entry entry;
			CreateEntry(entry);
			entry.type = vfsHDD_Entry_Dir;
			entry.data_block = hdr.next_block;
			entry.next_block = 0;

			f.Seek(cur_dir_block * hdr.block_size);
			f.Write(&entry, sizeof(vfsHDD_Entry));
			f.Write(".");
		}

		u8 null = 0;
		f.Seek(hdr.block_count * hdr.block_size - sizeof(null));
		f.Write(&null, sizeof(null));
	}

	void Format()
	{
	}

	void AppendEntry(vfsHDD_Entry entry)
	{
	}
};


class vfsHDDFile
{
	u64 m_info_block;
	vfsHDD_Entry m_info;
	const vfsHDD_Hdr& m_hdd_info;
	vfsLocalFile& m_hdd;
	u32 m_position;
	u64 m_cur_block;

	bool goto_block(u64 n)
	{
		vfsHDD_Block block_info;

		if(m_info.data_block >= m_hdd_info.block_count)
		{
			return false;
		}

		m_hdd.Seek(m_info.data_block * m_hdd_info.block_size);
		block_info.next_block = m_info.data_block;

		for(u64 i=0; i<n; ++i)
		{
			if(!block_info.next_block || !block_info.is_used || block_info.next_block >= m_hdd_info.block_count)
			{
				return false;
			}

			m_hdd.Seek(block_info.next_block * m_hdd_info.block_size);
			m_hdd.Read(&block_info, sizeof(vfsHDD_Block));
		}

		return true;
	}

	void RemoveBlocks(u64 start_block)
	{
		vfsHDD_Block block_info;
		block_info.next_block = start_block;

		while(block_info.next_block && block_info.is_used)
		{
			u64 offset = block_info.next_block * m_hdd_info.block_size;

			ReadBlock(offset, block_info);
			WriteBlock(offset, g_null_block);
		}
	}

	void WriteBlock(u64 block, const vfsHDD_Block& data)
	{
		m_hdd.Seek(block * m_hdd_info.block_size);
		m_hdd.Write(&data, sizeof(vfsHDD_Block));
	}

	void ReadBlock(u64 block, vfsHDD_Block& data)
	{
		m_hdd.Seek(block * m_hdd_info.block_size);
		m_hdd.Read(&data, sizeof(vfsHDD_Block));
	}

	void WriteEntry(u64 block, const vfsHDD_Entry& data)
	{
		m_hdd.Seek(block * m_hdd_info.block_size);
		m_hdd.Write(&data, sizeof(vfsHDD_Entry));
	}

	void ReadEntry(u64 block, vfsHDD_Entry& data)
	{
		m_hdd.Seek(block * m_hdd_info.block_size);
		m_hdd.Read(&data, sizeof(vfsHDD_Entry));
	}

	void ReadEntry(u64 block, vfsHDD_Entry& data, std::string& name)
	{
		m_hdd.Seek(block * m_hdd_info.block_size);
		m_hdd.Read(&data, sizeof(vfsHDD_Entry));
		name.resize(GetMaxNameLen());
		m_hdd.Read(&name.front(), GetMaxNameLen());
	}

	void ReadEntry(u64 block, std::string& name)
	{
		m_hdd.Seek(block * m_hdd_info.block_size + sizeof(vfsHDD_Entry));
		name.resize(GetMaxNameLen());
		m_hdd.Read(&name.front(), GetMaxNameLen());
	}

	void WriteEntry(u64 block, const vfsHDD_Entry& data, const std::string& name)
	{
		m_hdd.Seek(block * m_hdd_info.block_size);
		m_hdd.Write(&data, sizeof(vfsHDD_Entry));
		m_hdd.Write(name.c_str(), min<size_t>(GetMaxNameLen() - 1, name.length() + 1));
	}

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

	void Open(u64 info_block)
	{
		m_info_block = info_block;
		ReadEntry(m_info_block, m_info);
		m_position = 0;
		m_cur_block = m_info.data_block;
	}

	u64 FindFreeBlock()
	{
		vfsHDD_Block block_info;

		for(u64 i = 0; i<m_hdd_info.block_count; ++i)
		{
			ReadBlock(i, block_info);

			if(!block_info.is_used)
			{
				return i;
			}
		}

		return 0;
	}

	u64 GetSize() const
	{
		return m_info.size;
	}

	bool Seek(u64 pos)
	{
		if(!goto_block(pos / m_hdd_info.block_size))
		{
			return false;
		}

		m_position = pos % m_hdd_info.block_size;
		return true;
	}

	void SaveInfo()
	{
		m_hdd.Seek(m_info_block * m_hdd_info.block_size);
		m_hdd.Write(&m_info, sizeof(vfsHDD_Entry));
	}

	u64 Read(void* dst, u64 size)
	{
		if(!size)
			return 0;

		//vfsDeviceLocker lock(m_hdd);

		const u32 block_size = m_hdd_info.block_size - sizeof(vfsHDD_Block);
		u64 rsize = min<u64>(block_size - m_position, size);

		vfsHDD_Block cur_block_info;
		m_hdd.Seek(m_cur_block * m_hdd_info.block_size);
		m_hdd.Read(&cur_block_info, sizeof(vfsHDD_Block));
		m_hdd.Seek(m_cur_block * m_hdd_info.block_size + sizeof(vfsHDD_Block) + m_position);
		m_hdd.Read(dst, rsize);
		size -= rsize;
		m_position += rsize;
		if(!size)
		{
			return rsize;
		}

		u64 offset = rsize;

		for(; size; size -= rsize, offset += rsize)
		{
			if(!cur_block_info.is_used || !cur_block_info.next_block || cur_block_info.next_block >= m_hdd_info.block_count)
			{
				return offset;
			}

			m_cur_block = cur_block_info.next_block;
			rsize = min<u64>(block_size, size);

			m_hdd.Seek(cur_block_info.next_block * m_hdd_info.block_size);
			m_hdd.Read(&cur_block_info, sizeof(vfsHDD_Block));

			if(m_hdd.Read((u8*)dst + offset, rsize) != rsize)
			{
				return offset;
			}
		}

		m_position = rsize;

		return offset;
	}

	u64 Write(const void* src, u64 size)
	{
		if(!size)
			return 0;

		//vfsDeviceLocker lock(m_hdd);

		const u32 block_size = m_hdd_info.block_size - sizeof(vfsHDD_Block);

		if(!m_cur_block)
		{
			if(!m_info.data_block)
			{
				u64 new_block = FindFreeBlock();

				if(!new_block)
				{
					return 0;
				}

				WriteBlock(new_block, g_used_block);
				m_info.data_block = new_block;
				m_info.size = 0;
				SaveInfo();
			}

			m_cur_block = m_info.data_block;
			m_position = 0;
		}

		u64 wsize = min<u64>(block_size - m_position, size);

		vfsHDD_Block block_info;
		ReadBlock(m_cur_block, block_info);

		if(wsize)
		{
			m_hdd.Seek(m_cur_block * m_hdd_info.block_size + sizeof(vfsHDD_Block) + m_position);
			m_hdd.Write(src, wsize);
			size -= wsize;
			m_info.size += wsize;
			m_position += wsize;
			SaveInfo();

			if(!size)
				return wsize;
		}

		u64 last_block = m_cur_block;
		block_info.is_used = true;
		u64 offset = wsize;

		for(; size; size -= wsize, offset += wsize, m_info.size += wsize)
		{
			u64 new_block = FindFreeBlock();

			if(!new_block)
			{
				m_position = 0;
				SaveInfo();
				return offset;
			}

			m_cur_block = new_block;
			wsize = min<u64>(block_size, size);

			block_info.next_block = m_cur_block;
			m_hdd.Seek(last_block * m_hdd_info.block_size);
			if(m_hdd.Write(&block_info, sizeof(vfsHDD_Block)) != sizeof(vfsHDD_Block))
			{
				m_position = 0;
				SaveInfo();
				return offset;
			}

			block_info.next_block = 0;
			m_hdd.Seek(m_cur_block * m_hdd_info.block_size);
			if(m_hdd.Write(&block_info, sizeof(vfsHDD_Block)) != sizeof(vfsHDD_Block))
			{
				m_position = 0;
				SaveInfo();
				return offset;
			}
			if((m_position = m_hdd.Write((u8*)src + offset, wsize)) != wsize)
			{
				m_info.size += wsize;
				SaveInfo();
				return offset;
			}

			last_block = m_cur_block;
		}

		SaveInfo();
		m_position = wsize;
		return offset;
	}

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
	vfsHDD(vfsDevice* device, const std::string& hdd_path)
		: m_hdd_file(device)
		, m_file(m_hdd_file, m_hdd_info)
		, m_hdd_path(hdd_path)
		, vfsFileBase(device)
	{
		m_hdd_file.Open(hdd_path, vfsReadWrite);
		m_hdd_file.Read(&m_hdd_info, sizeof(vfsHDD_Hdr));
		m_cur_dir_block = m_hdd_info.next_block;
		if(!m_hdd_info.block_size)
		{
			ConLog.Error("Bad block size!");
			m_hdd_info.block_size = 2048;
		}
		m_hdd_file.Seek(m_cur_dir_block * m_hdd_info.block_size);
		m_hdd_file.Read(&m_cur_dir, sizeof(vfsHDD_Entry));
	}

	__forceinline u32 GetMaxNameLen() const
	{
		return m_hdd_info.block_size - sizeof(vfsHDD_Entry);
	}

	bool SearchEntry(const std::string& name, u64& entry_block, u64* parent_block = nullptr)
	{
		u64 last_block = 0;
		u64 block = m_cur_dir_block;
		vfsHDD_Entry entry;
		std::string buf;

		while(block)
		{
			ReadEntry(block, entry, buf);

			if (fmt::CmpNoCase(name,buf) == 0)
			{
				entry_block = block;
				if(parent_block)
					*parent_block = last_block;

				return true;
			}

			last_block = block;
			block = entry.is_used ? entry.next_block : 0ULL;
		}

		return false;
	}

	int OpenDir(const std::string& name)
	{
		ConLog.Warning("OpenDir(%s)", name.c_str());
		u64 entry_block;
		if(!SearchEntry(name, entry_block))
			return -1;

		m_hdd_file.Seek(entry_block * m_hdd_info.block_size);
		vfsHDD_Entry entry;
		m_hdd_file.Read(&entry, sizeof(vfsHDD_Entry));
		if(entry.type == vfsHDD_Entry_File)
			return 1;

		m_cur_dir_block = entry.data_block;
		ReadEntry(m_cur_dir_block, m_cur_dir);

		return 0;
	}

	bool Rename(const std::string& from, const std::string& to)
	{
		u64 entry_block;
		if(!SearchEntry(from, entry_block))
		{
			return false;
		}

		vfsHDD_Entry entry;
		ReadEntry(entry_block, entry);
		WriteEntry(entry_block, entry, to);

		return true;
	}

	u64 FindFreeBlock()
	{
		vfsHDD_Block block_info;

		for(u64 i = 0; i<m_hdd_info.block_count; ++i)
		{
			ReadBlock(i, block_info);

			if(!block_info.is_used)
			{
				return i;
			}
		}

		return 0;
	}

	void WriteBlock(u64 block, const vfsHDD_Block& data)
	{
		m_hdd_file.Seek(block * m_hdd_info.block_size);
		m_hdd_file.Write(&data, sizeof(vfsHDD_Block));
	}

	void ReadBlock(u64 block, vfsHDD_Block& data)
	{
		m_hdd_file.Seek(block * m_hdd_info.block_size);
		m_hdd_file.Read(&data, sizeof(vfsHDD_Block));
	}

	void WriteEntry(u64 block, const vfsHDD_Entry& data)
	{
		m_hdd_file.Seek(block * m_hdd_info.block_size);
		m_hdd_file.Write(&data, sizeof(vfsHDD_Entry));
	}

	void ReadEntry(u64 block, vfsHDD_Entry& data)
	{
		m_hdd_file.Seek(block * m_hdd_info.block_size);
		m_hdd_file.Read(&data, sizeof(vfsHDD_Entry));
	}

	void ReadEntry(u64 block, vfsHDD_Entry& data, std::string& name)
	{
		m_hdd_file.Seek(block * m_hdd_info.block_size);
		m_hdd_file.Read(&data, sizeof(vfsHDD_Entry));
		name.resize(GetMaxNameLen());
		m_hdd_file.Read(&name.front(), GetMaxNameLen());
	}

	void ReadEntry(u64 block, std::string& name)
	{
		m_hdd_file.Seek(block * m_hdd_info.block_size + sizeof(vfsHDD_Entry));
		name.resize(GetMaxNameLen());
		m_hdd_file.Read(&name.front(), GetMaxNameLen());
	}

	void WriteEntry(u64 block, const vfsHDD_Entry& data, const std::string& name)
	{
		m_hdd_file.Seek(block * m_hdd_info.block_size);
		m_hdd_file.Write(&data, sizeof(vfsHDD_Entry));
		m_hdd_file.Write(name.c_str(), min<size_t>(GetMaxNameLen() - 1, name.length() + 1));
	}

	bool Create(vfsHDD_EntryType type, const std::string& name)
	{
		if(HasEntry(name))
		{
			return false;
		}

		u64 new_block = FindFreeBlock();
		if(!new_block)
		{
			return false;
		}

		ConLog.Write("CREATING ENTRY AT 0x%llx", new_block);
		WriteBlock(new_block, g_used_block);

		{
			vfsHDD_Entry new_entry;
			vfsHDDManager::CreateEntry(new_entry);
			new_entry.next_block = 0;
			new_entry.type = type;

			if(type == vfsHDD_Entry_Dir)
			{
				u64 block_cur = FindFreeBlock();

				if(!block_cur)
				{
					return false;
				}

				WriteBlock(block_cur, g_used_block);

				u64 block_last = FindFreeBlock();

				if(!block_last)
				{
					return false;
				}

				WriteBlock(block_last, g_used_block);

				vfsHDD_Entry entry_cur, entry_last;
				vfsHDDManager::CreateEntry(entry_cur);
				vfsHDDManager::CreateEntry(entry_last);

				entry_cur.type = vfsHDD_Entry_Dir;
				entry_cur.data_block = block_cur;
				entry_cur.next_block = block_last;

				entry_last.type = vfsHDD_Entry_Dir;
				entry_last.data_block = m_cur_dir_block;
				entry_last.next_block = 0;

				new_entry.data_block = block_cur;

				WriteEntry(block_cur, entry_cur, ".");
				WriteEntry(block_last, entry_last, "..");
			}

			WriteEntry(new_block, new_entry, name);
		}

		{
			u64 block = m_cur_dir_block;

			vfsHDD_Block tmp;
			while(block)
			{
				ReadBlock(block, tmp);

				if(!tmp.next_block)
					break;

				block = tmp.next_block;
			}

			tmp.next_block = new_block;
			WriteBlock(block, tmp);
		}

		return true;
	}

	bool GetFirstEntry(u64& block, vfsHDD_Entry& entry, std::string& name)
	{
		if(!m_cur_dir_block)
		{
			return false;
		}

		ReadEntry(m_cur_dir_block, entry, name);
		block = entry.is_used ? entry.next_block : 0;

		return true;
	}

	bool GetNextEntry(u64& block, vfsHDD_Entry& entry, std::string& name)
	{
		if(!block)
		{
			return false;
		}

		ReadEntry(block, entry, name);

		block = entry.is_used ? entry.next_block : 0;
		return true;
	}

	virtual bool Open(const std::string& path, vfsOpenMode mode = vfsRead)
	{
		const char* s = path.c_str();
		u64 from = 0;
		u64 pos = 0;
		u64 file_pos = -1;

		do
		{
			if(s[pos] == '\\' || s[pos] == '/' || s[pos] == '\0') // ???
			{
				if(file_pos != -1)
				{
					return false;
				}

				if(from != -1)
				{
					if(pos - from > 1)
					{
						int res = OpenDir(std::string(s + from, pos));
						if(res == -1)
						{
							return false;
						}
						
						if(res == 1)
						{
							file_pos = from;
						}
					}

					from = pos;
				}
				else
				{
					from = pos;
				}
			}
		}
		while(s[pos++] != '\0');

		if(file_pos == -1)
		{
			return false;
		}

		u64 file_block;
		if(!SearchEntry(std::string(s + file_pos), file_block))
		{
			return false;
		}

		ConLog.Write("ENTRY FOUND AT 0x%llx", file_block);
		m_file.Open(file_block);

		return vfsFileBase::Open(path, mode);
	}

	bool HasEntry(const std::string& name)
	{
		u64 file_block;
		if(!SearchEntry(name, file_block))
		{
			return false;
		}

		return true;
	}

	void RemoveBlocksDir(u64 start_block)
	{
		std::string name;
		u64 block = start_block;
		vfsHDD_Entry entry;

		while(block)
		{
			ReadEntry(block, entry, name);
			WriteBlock(block, g_null_block);

			if(entry.type == vfsHDD_Entry_Dir && name != "." && name != "..")
			{
				ConLog.Warning("Removing sub folder '%s'", name.c_str());
				RemoveBlocksDir(entry.data_block);
			}
			else if(entry.type == vfsHDD_Entry_File)
			{
				RemoveBlocksFile(entry.data_block);
			}

			block = entry.next_block;
		}
	}

	void RemoveBlocksFile(u64 start_block)
	{
		u64 block = start_block;
		vfsHDD_Block block_data;

		while(block)
		{
			ReadBlock(block, block_data);
			WriteBlock(block, g_null_block);

			block = block_data.next_block;
		}
	}

	bool RemoveEntry(const std::string& name)
	{
		u64 entry_block, parent_entry;
		if(!SearchEntry(name, entry_block, &parent_entry))
		{
			return false;
		}

		vfsHDD_Entry entry;
		ReadEntry(entry_block, entry);
		if(entry.type == vfsHDD_Entry_Dir)
		{
			RemoveBlocksDir(entry.data_block);
		}
		else if(entry.type == vfsHDD_Entry_File)
		{
			RemoveBlocksFile(entry.data_block);
		}

		if(parent_entry)
		{
			u64 next = entry.next_block;
			ReadEntry(parent_entry, entry);
			entry.next_block = next;
			WriteEntry(parent_entry, entry);
		}
		WriteBlock(entry_block, g_null_block);
		return true;
	}

	virtual bool Create(const std::string& path)
	{
		return false;
	}

	virtual u32 Write(const void* src, u32 size)
	{
		return vfsFileBase::Write(src, m_file.Write(src, size));
	}

	virtual u32 Read(void* dst, u32 size)
	{
		return vfsFileBase::Read(dst, m_file.Read(dst, size));
	}

	virtual u64 Seek(s64 offset, vfsSeekMode mode = vfsSeekSet)
	{
		switch(mode)
		{
		case vfsSeekCur:
			m_file.Seek(Tell() + offset);
		break;

		case vfsSeekSet:
			m_file.Seek(offset);
		break;

		case vfsSeekEnd:
			m_file.Seek(m_file.GetSize() + offset);
		break;
		}

		return vfsFileBase::Seek(offset, mode);
	}

	virtual bool Eof()
	{
		return m_file.Eof();
	}

	virtual u64 GetSize()
	{
		return m_file.GetSize();
	}
};
