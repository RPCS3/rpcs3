#include "stdafx.h"
#include "SC_FileSystem.h"
#include "Emu/SysCalls/SysCalls.h"

extern Module sys_fs;

enum
{
	IDFlag_File = 1,
	IDFlag_Dir = 2,
};

struct FsRingBuffer
{
	u64 m_ringbuf_size;
	u64 m_block_size;
	u64 m_transfer_rate;
	u32 m_copy;
};

struct FsRingBufferConfig
{
	FsRingBuffer m_ring_buffer;
	u32 m_buffer;
	u64 m_fs_status;
	u64 m_regid;
	u32 m_alloc_mem_size;
	u32 m_current_addr;

	FsRingBufferConfig()
		: m_fs_status(CELL_FS_ST_NOT_INITIALIZED)
		, m_regid(0)
		, m_alloc_mem_size(0)
		, m_current_addr(0)
	{
		memset(&m_ring_buffer, 0, sizeof(FsRingBuffer));
	}

	~FsRingBufferConfig()
	{
		memset(&m_ring_buffer, 0, sizeof(FsRingBuffer));
	}
} m_fs_config;


int cellFsOpen(u32 path_addr, int flags, mem32_t fd, mem32_t arg, u64 size)
{
	const std::string& path = Memory.ReadString(path_addr);
	sys_fs.Log("cellFsOpen(path=\"%s\", flags=0x%x, fd_addr=0x%x, arg_addr=0x%x, size=0x%llx)",
		path.c_str(), flags, fd.GetAddr(), arg.GetAddr(), size);

	const std::string& ppath = path;
	//ConLog.Warning("path: %s [%s]", ppath.c_str(), path.c_str());

	s32 _oflags = flags;
	if(flags & CELL_O_CREAT)
	{
		_oflags &= ~CELL_O_CREAT;
		Emu.GetVFS().CreateFile(ppath);
	}

	vfsOpenMode o_mode;

	switch(flags & CELL_O_ACCMODE)
	{
	case CELL_O_RDONLY:
		_oflags &= ~CELL_O_RDONLY;
		o_mode = vfsRead;
	break;

	case CELL_O_WRONLY:
		_oflags &= ~CELL_O_WRONLY;

		if(flags & CELL_O_APPEND)
		{
			_oflags &= ~CELL_O_APPEND;
			o_mode = vfsWriteAppend;
		}
		else if(flags & CELL_O_EXCL)
		{
			_oflags &= ~CELL_O_EXCL;
			o_mode = vfsWriteExcl;
		}
		else //if (flags & CELL_O_TRUNC)
		{
			_oflags &= ~CELL_O_TRUNC;
			o_mode = vfsWrite;
		}
	break;

	case CELL_O_RDWR:
		_oflags &= ~CELL_O_RDWR;
		if (flags & CELL_O_TRUNC)
		{
			_oflags &= ~CELL_O_TRUNC;
			//truncate file before opening it as read/write
			Emu.GetVFS().OpenFile(ppath, vfsWrite);
		}
		o_mode = vfsReadWrite;
	break;
	}

	if(_oflags != 0)
	{
		sys_fs.Error("\"%s\" has unknown flags! flags: 0x%08x", ppath.c_str(), flags);
		return CELL_EINVAL;
	}

	vfsFileBase* stream = Emu.GetVFS().OpenFile(ppath, o_mode);

	if(!stream || !stream->IsOpened())
	{
		sys_fs.Error("\"%s\" not found! flags: 0x%08x", ppath.c_str(), flags);
		return CELL_ENOENT;
	}

	fd = sys_fs.GetNewId(stream, IDFlag_File);
	ConLog.Warning("*** cellFsOpen(path=\"%s\"): fd = %d", path.c_str(), fd.GetValue());

	return CELL_OK;
}

int cellFsRead(u32 fd, u32 buf_addr, u64 nbytes, mem64_t nread)
{
	sys_fs.Log("cellFsRead(fd=%d, buf_addr=0x%x, nbytes=0x%llx, nread_addr=0x%x)",
		fd, buf_addr, nbytes, nread.GetAddr());
	vfsStream* file;
	if(!sys_fs.CheckId(fd, file)) return CELL_ESRCH;

	if (nread.GetAddr() && !nread.IsGood()) return CELL_EFAULT;

	u32 res = 0;
	u32 count = nbytes;
	if (nbytes != (u64)count) return CELL_ENOMEM;

	if (!Memory.IsGoodAddr(buf_addr)) return CELL_EFAULT;

	if (count) if (u32 frag = buf_addr & 4095) // memory page fragment
	{
		u32 req = min(count, 4096 - frag);
		u32 read = file->Read(Memory + buf_addr, req);
		buf_addr += req;
		res += read;
		count -= req;
		if (read < req) goto fin;
	}

	for (u32 pages = count / 4096; pages > 0; pages--) // full pages
	{
		if (!Memory.IsGoodAddr(buf_addr)) goto fin; // ??? (probably EFAULT)
		u32 read = file->Read(Memory + buf_addr, 4096);
		buf_addr += 4096;
		res += read;
		count -= 4096;
		if (read < 4096) goto fin;
	}

	if (count) // last fragment
	{
		if (!Memory.IsGoodAddr(buf_addr)) goto fin;
		res += file->Read(Memory + buf_addr, count);
	}

fin:

	if (nread.GetAddr()) nread = res; // write value if not NULL

	return CELL_OK;
}

int cellFsWrite(u32 fd, u32 buf_addr, u64 nbytes, mem64_t nwrite)
{
	sys_fs.Log("cellFsWrite(fd=%d, buf_addr=0x%x, nbytes=0x%llx, nwrite_addr=0x%x)",
		fd, buf_addr, nbytes, nwrite.GetAddr());
	vfsStream* file;
	if(!sys_fs.CheckId(fd, file)) return CELL_ESRCH;

	if(Memory.IsGoodAddr(buf_addr) && !Memory.IsGoodAddr(buf_addr, nbytes))
	{
		MemoryBlock& block = Memory.GetMemByAddr(buf_addr);
		nbytes = block.GetSize() - (buf_addr - block.GetStartAddr());
	}

	const u64 res = nbytes ? file->Write(Memory.GetMemFromAddr(buf_addr), nbytes) : 0;

	if(nwrite.IsGood())
		nwrite = res;

	return CELL_OK;
}

int cellFsClose(u32 fd)
{
	sys_fs.Warning("cellFsClose(fd=%d)", fd);

	if(!Emu.GetIdManager().RemoveID(fd))
		return CELL_ESRCH;

	return CELL_OK;
}

int cellFsOpendir(u32 path_addr, mem32_t fd)
{
	const std::string& path = Memory.ReadString(path_addr);
	sys_fs.Warning("cellFsOpendir(path=\"%s\", fd_addr=0x%x)", path.c_str(), fd.GetAddr());
	
	if(!Memory.IsGoodAddr(path_addr) || !fd.IsGood())
		return CELL_EFAULT;

	vfsDirBase* dir = Emu.GetVFS().OpenDir(path);
	if(!dir || !dir->IsOpened())
	{
		delete dir;
		return CELL_ENOENT;
	}

	fd = sys_fs.GetNewId(dir, IDFlag_Dir);
	return CELL_OK;
}

int cellFsReaddir(u32 fd, mem_ptr_t<CellFsDirent> dir, mem64_t nread)
{
	sys_fs.Log("cellFsReaddir(fd=%d, dir_addr=0x%x, nread_addr=0x%x)", fd, dir.GetAddr(), nread.GetAddr());

	vfsDirBase* directory;
	if(!sys_fs.CheckId(fd, directory))
		return CELL_ESRCH;
	if(!dir.IsGood() || !nread.IsGood())
		return CELL_EFAULT;

	const DirEntryInfo* info = directory->Read();
	if(info)
	{
		nread = 1;
		Memory.WriteString(dir.GetAddr()+2, info->name);
		dir->d_namlen = info->name.length();
		dir->d_type = (info->flags & DirEntry_TypeFile) ? CELL_FS_TYPE_REGULAR : CELL_FS_TYPE_DIRECTORY;
	}
	else
	{
		nread = 0;
	}

	return CELL_OK;
}

int cellFsClosedir(u32 fd)
{
	sys_fs.Log("cellFsClosedir(fd=%d)", fd);

	if(!Emu.GetIdManager().RemoveID(fd))
		return CELL_ESRCH;

	return CELL_OK;
}

int cellFsStat(const u32 path_addr, mem_ptr_t<CellFsStat> sb)
{
	const std::string& path = Memory.ReadString(path_addr);
	sys_fs.Log("cellFsStat(path=\"%s\", sb_addr: 0x%x)", path.c_str(), sb.GetAddr());

	sb->st_mode = 
		CELL_FS_S_IRUSR | CELL_FS_S_IWUSR | CELL_FS_S_IXUSR |
		CELL_FS_S_IRGRP | CELL_FS_S_IWGRP | CELL_FS_S_IXGRP |
		CELL_FS_S_IROTH | CELL_FS_S_IWOTH | CELL_FS_S_IXOTH;

	sb->st_uid = 0;
	sb->st_gid = 0;
	sb->st_atime_ = 0; //TODO
	sb->st_mtime_ = 0; //TODO
	sb->st_ctime_ = 0; //TODO
	sb->st_blksize = 4096;

	{
		vfsDir dir(path);
		if(dir.IsOpened())
		{
			sb->st_mode |= CELL_FS_S_IFDIR;
			return CELL_OK;
		}
	}

	{
		vfsFile f(path);
		if(f.IsOpened())
		{
			sb->st_mode |= CELL_FS_S_IFREG;
			sb->st_size = f.GetSize();
			return CELL_OK;
		}
	}

	sys_fs.Warning("cellFsStat: \"%s\" not found.", path.c_str());
	return CELL_ENOENT;
}

int cellFsFstat(u32 fd, mem_ptr_t<CellFsStat> sb)
{
	sys_fs.Log("cellFsFstat(fd=%d, sb_addr: 0x%x)", fd, sb.GetAddr());

	u32 attr;
	vfsStream* file;
	if(!sys_fs.CheckId(fd, file, attr) || attr != IDFlag_File) return CELL_ESRCH;

	sb->st_mode = 
		CELL_FS_S_IRUSR | CELL_FS_S_IWUSR | CELL_FS_S_IXUSR |
		CELL_FS_S_IRGRP | CELL_FS_S_IWGRP | CELL_FS_S_IXGRP |
		CELL_FS_S_IROTH | CELL_FS_S_IWOTH | CELL_FS_S_IXOTH;

	sb->st_mode |= CELL_FS_S_IFREG; //TODO: dir CELL_FS_S_IFDIR
	sb->st_uid = 0;
	sb->st_gid = 0;
	sb->st_atime_ = 0; //TODO
	sb->st_mtime_ = 0; //TODO
	sb->st_ctime_ = 0; //TODO
	sb->st_size = file->GetSize();
	sb->st_blksize = 4096;

	return CELL_OK;
}

int cellFsMkdir(u32 path_addr, u32 mode)
{
	const std::string& ps3_path = Memory.ReadString(path_addr);
	sys_fs.Log("cellFsMkdir(path=\"%s\", mode=0x%x)", ps3_path.c_str(), mode);
	
	/*vfsDir dir;
	if(dir.IsExists(ps3_path))
		return CELL_EEXIST;
	if(!dir.Create(ps3_path))
		return CELL_EBUSY;*/

	if(Emu.GetVFS().ExistsDir(ps3_path))
		return CELL_EEXIST;
	if(!Emu.GetVFS().CreateDir(ps3_path))
		return CELL_EBUSY;

	return CELL_OK;
}

int cellFsRename(u32 from_addr, u32 to_addr)
{
	const std::string& ps3_from = Memory.ReadString(from_addr);
	const std::string& ps3_to = Memory.ReadString(to_addr);

	{
		vfsDir dir;
		if(dir.IsExists(ps3_from))
		{
			if(!dir.Rename(ps3_from, ps3_to))
				return CELL_EBUSY;

			return CELL_OK;
		}
	}

	{
		vfsFile f;
		if(f.Exists(ps3_from))
		{
			if(!f.Rename(ps3_from, ps3_to))
				return CELL_EBUSY;

			return CELL_OK;
		}
	}

	return CELL_ENOENT;
}

int cellFsRmdir(u32 path_addr)
{
	const std::string& ps3_path = Memory.ReadString(path_addr);
	sys_fs.Log("cellFsRmdir(path=\"%s\")", ps3_path.c_str());

	vfsDir d;
	if(!d.IsExists(ps3_path))
		return CELL_ENOENT;

	if(!d.Remove(ps3_path))
		return CELL_EBUSY;

	return CELL_OK;
}

int cellFsUnlink(u32 path_addr)
{
	const std::string& ps3_path = Memory.ReadString(path_addr);
	sys_fs.Warning("cellFsUnlink(path=\"%s\")", ps3_path.c_str());

	if (ps3_path.empty())
		return CELL_EFAULT;

	if (Emu.GetVFS().ExistsDir(ps3_path))
		return CELL_EISDIR;

	if (!Emu.GetVFS().ExistsFile(ps3_path))
		return CELL_ENOENT;

	if (!Emu.GetVFS().RemoveFile(ps3_path))
		return CELL_EACCES;
	
	return CELL_OK;
}

int cellFsLseek(u32 fd, s64 offset, u32 whence, mem64_t pos)
{
	vfsSeekMode seek_mode;
	sys_fs.Log("cellFsLseek(fd=%d, offset=0x%llx, whence=0x%x, pos_addr=0x%x)", fd, offset, whence, pos.GetAddr());
	switch(whence)
	{
	case CELL_SEEK_SET: seek_mode = vfsSeekSet; break;
	case CELL_SEEK_CUR: seek_mode = vfsSeekCur; break;
	case CELL_SEEK_END: seek_mode = vfsSeekEnd; break;
	default:
		sys_fs.Error(fd, "Unknown seek whence! (0x%x)", whence);
	return CELL_EINVAL;
	}

	u32 attr;
	vfsStream* file;
	if(!sys_fs.CheckId(fd, file, attr) || attr != IDFlag_File) return CELL_ESRCH;
	pos = file->Seek(offset, seek_mode);
	return CELL_OK;
}

int cellFsFtruncate(u32 fd, u64 size)
{
	sys_fs.Log("cellFsFtruncate(fd=%d, size=%lld)", fd, size);
	u32 attr;
	vfsStream* file;
	if(!sys_fs.CheckId(fd, file, attr) || attr != IDFlag_File) return CELL_ESRCH;
	u64 initialSize = file->GetSize();

	if (initialSize < size)
	{
		u64 last_pos = file->Tell();
		file->Seek(0, vfsSeekEnd);
		static const char nullbyte = 0;
		file->Seek(size-initialSize-1, vfsSeekCur);
		file->Write(&nullbyte, sizeof(char));
		file->Seek(last_pos, vfsSeekSet);
	}

	if (initialSize > size)
	{
		// (TODO)
	}

	return CELL_OK;
}

int cellFsTruncate(u32 path_addr, u64 size)
{
	const std::string& path = Memory.ReadString(path_addr);
	sys_fs.Log("cellFsTruncate(path=\"%s\", size=%lld)", path.c_str(), size);

	vfsFile f(path, vfsReadWrite);
	if(!f.IsOpened())
	{
		sys_fs.Warning("cellFsTruncate: \"%s\" not found.", path.c_str());
		return CELL_ENOENT;
	}
	u64 initialSize = f.GetSize();

	if (initialSize < size)
	{
		u64 last_pos = f.Tell();
		f.Seek(0, vfsSeekEnd);
		static const char nullbyte = 0;
		f.Seek(size-initialSize-1, vfsSeekCur);
		f.Write(&nullbyte, sizeof(char));
		f.Seek(last_pos, vfsSeekSet);
	}

	if (initialSize > size)
	{
		// (TODO)
	}

	return CELL_OK;
}

int cellFsFGetBlockSize(u32 fd, mem64_t sector_size, mem64_t block_size)
{
	sys_fs.Log("cellFsFGetBlockSize(fd=%d, sector_size_addr: 0x%x, block_size_addr: 0x%x)", fd, sector_size.GetAddr(), block_size.GetAddr());

	vfsStream* file;
	if(!sys_fs.CheckId(fd, file)) return CELL_ESRCH;

	sector_size = 4096; // ?
	block_size = 4096; // ?

	return CELL_OK;
}

int cellFsGetBlockSize(u32 path_addr, mem64_t sector_size, mem64_t block_size)
{
	sys_fs.Log("cellFsGetBlockSize(file: %s, sector_size_addr: 0x%x, block_size_addr: 0x%x)", Memory.ReadString(path_addr).c_str(), sector_size.GetAddr(), block_size.GetAddr());

	sector_size = 4096; // ?
	block_size = 4096; // ?

	return CELL_OK;
}

int cellFsGetFreeSize(u32 path_addr, mem32_t block_size, mem64_t block_count)
{
	const std::string& ps3_path = Memory.ReadString(path_addr);
	sys_fs.Warning("cellFsGetFreeSize(path=\"%s\", block_size_addr=0x%x, block_count_addr=0x%x)",
		ps3_path.c_str(), block_size.GetAddr(), block_count.GetAddr());

	if (!Memory.IsGoodAddr(path_addr) || !block_size.IsGood() || !block_count.IsGood())
		return CELL_EFAULT;

	if (ps3_path.empty())
		return CELL_EINVAL;

	// TODO: Get real values. Currently, it always returns 40 GB of free space divided in 4 KB blocks
	block_size = 4096; // ?
	block_count = 10485760; // ?

	return CELL_OK;
}

int cellFsGetDirectoryEntries(u32 fd, mem_ptr_t<CellFsDirectoryEntry> entries, u32 entries_size, mem32_t data_count)
{
	sys_fs.Log("cellFsGetDirectoryEntries(fd=%d, entries_addr=0x%x, entries_size = 0x%x, data_count_addr=0x%x)", fd, entries.GetAddr(), entries_size, data_count.GetAddr());

	vfsDirBase* directory;
	if(!sys_fs.CheckId(fd, directory))
		return CELL_ESRCH;
	if(!entries.IsGood() || !data_count.IsGood())
		return CELL_EFAULT;

	const DirEntryInfo* info = directory->Read();
	if(info)
	{
		data_count = 1;
		Memory.WriteString(entries.GetAddr()+2, info->name);
		entries->entry_name.d_namlen = info->name.length();
		entries->entry_name.d_type = (info->flags & DirEntry_TypeFile) ? CELL_FS_TYPE_REGULAR : CELL_FS_TYPE_DIRECTORY;

		entries->attribute.st_mode = 
		CELL_FS_S_IRUSR | CELL_FS_S_IWUSR | CELL_FS_S_IXUSR |
		CELL_FS_S_IRGRP | CELL_FS_S_IWGRP | CELL_FS_S_IXGRP |
		CELL_FS_S_IROTH | CELL_FS_S_IWOTH | CELL_FS_S_IXOTH;

		entries->attribute.st_uid = 0;
		entries->attribute.st_gid = 0;
		entries->attribute.st_atime_ = 0; //TODO
		entries->attribute.st_mtime_ = 0; //TODO
		entries->attribute.st_ctime_ = 0; //TODO
		entries->attribute.st_blksize = 4096;
	}
	else
	{
		data_count = 0;
	}

	return CELL_OK;
}

int cellFsStReadInit(u32 fd, mem_ptr_t<CellFsRingBuffer> ringbuf)
{
	sys_fs.Warning("cellFsStReadInit(fd=%d, ringbuf_addr=0x%x)", fd, ringbuf.GetAddr());

	vfsStream* file;
	if(!sys_fs.CheckId(fd, file)) return CELL_ESRCH;

	if(!ringbuf.IsGood())
		return CELL_EFAULT;

	FsRingBuffer& buffer = m_fs_config.m_ring_buffer;

	buffer.m_block_size = ringbuf->block_size;
	buffer.m_copy = ringbuf->copy;
	buffer.m_ringbuf_size = ringbuf->ringbuf_size;
	buffer.m_transfer_rate = ringbuf->transfer_rate;

	if(buffer.m_ringbuf_size < 0x40000000) // If the size is less than 1MB
		m_fs_config.m_alloc_mem_size = ((ringbuf->ringbuf_size + 64 * 1024 - 1) / (64 * 1024)) * (64 * 1024);
	m_fs_config.m_alloc_mem_size = ((ringbuf->ringbuf_size + 1024 * 1024 - 1) / (1024 * 1024)) * (1024 * 1024);

	// alloc memory
	m_fs_config.m_buffer = Memory.Alloc(m_fs_config.m_alloc_mem_size, 1024);
	memset(Memory + m_fs_config.m_buffer, 0, m_fs_config.m_alloc_mem_size);

	m_fs_config.m_fs_status = CELL_FS_ST_INITIALIZED;

	return CELL_OK;
}

int cellFsStReadFinish(u32 fd)
{
	sys_fs.Warning("cellFsStReadFinish(fd=%d)", fd);

	vfsStream* file;
	if(!sys_fs.CheckId(fd, file)) return CELL_ESRCH;

	Memory.Free(m_fs_config.m_buffer);
	m_fs_config.m_fs_status = CELL_FS_ST_NOT_INITIALIZED;

	return CELL_OK;
}

int cellFsStReadGetRingBuf(u32 fd, mem_ptr_t<CellFsRingBuffer> ringbuf)
{
	sys_fs.Warning("cellFsStReadGetRingBuf(fd=%d, ringbuf_addr=0x%x)", fd, ringbuf.GetAddr());

	vfsStream* file;
	if(!sys_fs.CheckId(fd, file)) return CELL_ESRCH;

	if(!ringbuf.IsGood())
		return CELL_EFAULT;

	FsRingBuffer& buffer = m_fs_config.m_ring_buffer;

	ringbuf->block_size = buffer.m_block_size;
	ringbuf->copy = buffer.m_copy;
	ringbuf->ringbuf_size = buffer.m_ringbuf_size;
	ringbuf->transfer_rate = buffer.m_transfer_rate;

	sys_fs.Warning("*** fs stream config: block_size=0x%llx, copy=%d, ringbuf_size = 0x%llx, transfer_rate = 0x%llx", ringbuf->block_size, ringbuf->copy,
																								ringbuf->ringbuf_size, ringbuf->transfer_rate);
	return CELL_OK;
}

int cellFsStReadGetStatus(u32 fd, mem64_t status)
{
	sys_fs.Warning("cellFsStReadGetRingBuf(fd=%d, status_addr=0x%x)", fd, status.GetAddr());

	vfsStream* file;
	if(!sys_fs.CheckId(fd, file)) return CELL_ESRCH;

	status = m_fs_config.m_fs_status;

	return CELL_OK;
}

int cellFsStReadGetRegid(u32 fd, mem64_t regid)
{
	sys_fs.Warning("cellFsStReadGetRingBuf(fd=%d, regid_addr=0x%x)", fd, regid.GetAddr());

	vfsStream* file;
	if(!sys_fs.CheckId(fd, file)) return CELL_ESRCH;

	regid = m_fs_config.m_regid;

	return CELL_OK;
}

int cellFsStReadStart(u32 fd, u64 offset, u64 size)
{
	sys_fs.Warning("TODO: cellFsStReadStart(fd=%d, offset=0x%llx, size=0x%llx)", fd, offset, size);

	vfsStream* file;
	if(!sys_fs.CheckId(fd, file)) return CELL_ESRCH;

	m_fs_config.m_current_addr = m_fs_config.m_buffer + (u32)offset;
	m_fs_config.m_fs_status = CELL_FS_ST_PROGRESS;

	return CELL_OK;
}

int cellFsStReadStop(u32 fd)
{
	sys_fs.Warning("cellFsStReadStop(fd=%d)", fd);

	vfsStream* file;
	if(!sys_fs.CheckId(fd, file)) return CELL_ESRCH;

	m_fs_config.m_fs_status = CELL_FS_ST_STOP;

	return CELL_OK;
}

int cellFsStRead(u32 fd, u32 buf_addr, u64 size, mem64_t rsize)
{
	sys_fs.Warning("TODO: cellFsStRead(fd=%d, buf_addr=0x%x, size=0x%llx, rsize_addr = 0x%x)", fd, buf_addr, size, rsize.GetAddr());
	
	vfsStream* file;
	if(!sys_fs.CheckId(fd, file)) return CELL_ESRCH;

	if (rsize.GetAddr() && !rsize.IsGood()) return CELL_EFAULT;

	m_fs_config.m_regid += size;
	rsize = m_fs_config.m_regid;

	return CELL_OK;
}

int cellFsStReadGetCurrentAddr(u32 fd, mem32_t addr_addr, mem64_t size)
{
	sys_fs.Warning("TODO: cellFsStReadGetCurrentAddr(fd=%d, addr_addr=0x%x, size_addr = 0x%x)", fd, addr_addr.GetAddr(), size.GetAddr());

	vfsStream* file;
	if(!sys_fs.CheckId(fd, file)) return CELL_ESRCH;

	if (!addr_addr.IsGood() && !size.IsGood()) return CELL_EFAULT;
	
	return CELL_OK;
}

int cellFsStReadPutCurrentAddr(u32 fd, u32 addr_addr, u64 size)
{
	sys_fs.Warning("TODO: cellFsStReadPutCurrentAddr(fd=%d, addr_addr=0x%x, size = 0x%llx)", fd, addr_addr, size);
	
	vfsStream* file;
	if(!sys_fs.CheckId(fd, file)) return CELL_ESRCH;

	if (!Memory.IsGoodAddr(addr_addr)) return CELL_EFAULT;

	return CELL_OK;
}

int cellFsStReadWait(u32 fd, u64 size)
{
	sys_fs.Warning("TODO: cellFsStReadWait(fd=%d, size = 0x%llx)", fd, size);
	
	vfsStream* file;
	if(!sys_fs.CheckId(fd, file)) return CELL_ESRCH;
	
	return CELL_OK;
}

int cellFsStReadWaitCallback(u32 fd, u64 size, mem_func_ptr_t<void (*)(int xfd, u64 xsize)> func)
{
	sys_fs.Warning("TODO: cellFsStReadWaitCallback(fd=%d, size = 0x%llx, func_addr = 0x%x)", fd, size, func.GetAddr());

	if (!func.IsGood())
		return CELL_EFAULT;

	vfsStream* file;
	if(!sys_fs.CheckId(fd, file)) return CELL_ESRCH;
	
	return CELL_OK;
}