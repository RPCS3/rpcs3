#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/FS/vfsFile.h"
#include "Emu/FS/vfsDir.h"

#include "lv2Fs.h"
#include "Emu/SysCalls/SysCalls.h"

extern Module *sys_fs;

enum
{
	IDFlag_File = 1,
	IDFlag_Dir = 2,
};

struct FsRingBufferConfig
{
	CellFsRingBuffer m_ring_buffer; // Currently unused after assignment
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
		, m_ring_buffer() { }

} fs_config;


s32 cellFsOpen(u32 path_addr, s32 flags, mem32_t fd, mem32_t arg, u64 size)
{
	const std::string& path = Memory.ReadString(path_addr);
	sys_fs->Log("cellFsOpen(path=\"%s\", flags=0x%x, fd_addr=0x%x, arg_addr=0x%x, size=0x%llx)",
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
			auto filePtr = Emu.GetVFS().OpenFile(ppath, vfsWrite);
			delete filePtr;
		}
		o_mode = vfsReadWrite;
	break;
	}

	if(_oflags != 0)
	{
		sys_fs->Error("\"%s\" has unknown flags! flags: 0x%08x", ppath.c_str(), flags);
		return CELL_EINVAL;
	}

	if (!Emu.GetVFS().ExistsFile(ppath))
	{
		sys_fs->Error("\"%s\" not found! flags: 0x%08x", ppath.c_str(), flags);
		return CELL_ENOENT;
	}

	vfsFileBase* stream = Emu.GetVFS().OpenFile(ppath, o_mode);

	if(!stream || !stream->IsOpened())
	{
		delete stream;
		sys_fs->Error("\"%s\" not found! flags: 0x%08x", ppath.c_str(), flags);
		return CELL_ENOENT;
	}

	fd = sys_fs->GetNewId(stream, IDFlag_File);
	LOG_NOTICE(HLE, "\"%s\" opened: fd = %d", path.c_str(), fd.GetValue());

	return CELL_OK;
}

s32 cellFsRead(u32 fd, u32 buf_addr, u64 nbytes, mem64_t nread)
{
	sys_fs->Log("cellFsRead(fd=%d, buf_addr=0x%x, nbytes=0x%llx, nread_addr=0x%x)",
		fd, buf_addr, nbytes, nread.GetAddr());
	vfsStream* file;
	if(!sys_fs->CheckId(fd, file)) return CELL_ESRCH;

	if (nbytes != (u32)nbytes) return CELL_ENOMEM;

	// TODO: checks

	const u64 res = nbytes ? file->Read(Memory.GetMemFromAddr(buf_addr), nbytes) : 0;

	if (nread.GetAddr()) nread = res; // write value if not NULL

	return CELL_OK;
}

s32 cellFsWrite(u32 fd, u32 buf_addr, u64 nbytes, mem64_t nwrite)
{
	sys_fs->Log("cellFsWrite(fd=%d, buf_addr=0x%x, nbytes=0x%llx, nwrite_addr=0x%x)",
		fd, buf_addr, nbytes, nwrite.GetAddr());
	vfsStream* file;
	if(!sys_fs->CheckId(fd, file)) return CELL_ESRCH;

	if (nbytes != (u32)nbytes) return CELL_ENOMEM;

	// TODO: checks

	const u64 res = nbytes ? file->Write(Memory.GetMemFromAddr(buf_addr), nbytes) : 0;

	if (nwrite.GetAddr()) nwrite = res; // write value if not NULL

	return CELL_OK;
}

s32 cellFsClose(u32 fd)
{
	sys_fs->Warning("cellFsClose(fd=%d)", fd);

	if(!Emu.GetIdManager().RemoveID(fd))
		return CELL_ESRCH;

	return CELL_OK;
}

s32 cellFsOpendir(u32 path_addr, mem32_t fd)
{
	const std::string& path = Memory.ReadString(path_addr);
	sys_fs->Warning("cellFsOpendir(path=\"%s\", fd_addr=0x%x)", path.c_str(), fd.GetAddr());
	
	vfsDirBase* dir = Emu.GetVFS().OpenDir(path);
	if(!dir || !dir->IsOpened())
	{
		delete dir;
		return CELL_ENOENT;
	}

	fd = sys_fs->GetNewId(dir, IDFlag_Dir);
	return CELL_OK;
}

s32 cellFsReaddir(u32 fd, mem_ptr_t<CellFsDirent> dir, mem64_t nread)
{
	sys_fs->Log("cellFsReaddir(fd=%d, dir_addr=0x%x, nread_addr=0x%x)", fd, dir.GetAddr(), nread.GetAddr());

	vfsDirBase* directory;
	if(!sys_fs->CheckId(fd, directory))
		return CELL_ESRCH;

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

s32 cellFsClosedir(u32 fd)
{
	sys_fs->Log("cellFsClosedir(fd=%d)", fd);

	if(!Emu.GetIdManager().RemoveID(fd))
		return CELL_ESRCH;

	return CELL_OK;
}

s32 cellFsStat(const u32 path_addr, mem_ptr_t<CellFsStat> sb)
{
	const std::string& path = Memory.ReadString(path_addr);
	sys_fs->Warning("cellFsStat(path=\"%s\", sb_addr: 0x%x)", path.c_str(), sb.GetAddr());

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

	sys_fs->Warning("cellFsStat: \"%s\" not found.", path.c_str());
	return CELL_ENOENT;
}

s32 cellFsFstat(u32 fd, mem_ptr_t<CellFsStat> sb)
{
	sys_fs->Log("cellFsFstat(fd=%d, sb_addr: 0x%x)", fd, sb.GetAddr());

	u32 attr;
	vfsStream* file;
	if(!sys_fs->CheckId(fd, file, attr) || attr != IDFlag_File) return CELL_ESRCH;

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

s32 cellFsMkdir(u32 path_addr, u32 mode)
{
	const std::string& ps3_path = Memory.ReadString(path_addr);
	sys_fs->Log("cellFsMkdir(path=\"%s\", mode=0x%x)", ps3_path.c_str(), mode);
	
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

s32 cellFsRename(u32 from_addr, u32 to_addr)
{
	const std::string& ps3_from = Memory.ReadString(from_addr);
	const std::string& ps3_to = Memory.ReadString(to_addr);
	sys_fs->Log("cellFsRename(from='%s' (from_addr=0x%x), to='%s' (to_addr=0x%x))", ps3_from.c_str(), from_addr, ps3_to.c_str(), to_addr);

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
s32 cellFsChmod(u32 path_addr, u32 mode)
{
	const std::string& ps3_path = Memory.ReadString(path_addr);
	sys_fs->Todo("cellFsChmod(path=\"%s\", mode=0x%x)", ps3_path.c_str(), mode);

	// TODO:

	return CELL_OK;
}

s32 cellFsFsync(u32 fd)
{
	sys_fs->Todo("cellFsFsync(fd=0x%x)", fd);

	// TODO:

	return CELL_OK;
}

s32 cellFsRmdir(u32 path_addr)
{
	const std::string& ps3_path = Memory.ReadString(path_addr);
	sys_fs->Log("cellFsRmdir(path=\"%s\")", ps3_path.c_str());

	vfsDir d;
	if(!d.IsExists(ps3_path))
		return CELL_ENOENT;

	if(!d.Remove(ps3_path))
		return CELL_EBUSY;

	return CELL_OK;
}

s32 cellFsUnlink(u32 path_addr)
{
	const std::string& ps3_path = Memory.ReadString(path_addr);
	sys_fs->Warning("cellFsUnlink(path=\"%s\")", ps3_path.c_str());

	if (Emu.GetVFS().ExistsDir(ps3_path))
		return CELL_EISDIR;

	if (!Emu.GetVFS().ExistsFile(ps3_path))
		return CELL_ENOENT;

	if (!Emu.GetVFS().RemoveFile(ps3_path))
		return CELL_EACCES;
	
	return CELL_OK;
}

s32 cellFsLseek(u32 fd, s64 offset, u32 whence, mem64_t pos)
{
	vfsSeekMode seek_mode;
	sys_fs->Log("cellFsLseek(fd=%d, offset=0x%llx, whence=0x%x, pos_addr=0x%x)", fd, offset, whence, pos.GetAddr());
	switch(whence)
	{
	case CELL_SEEK_SET: seek_mode = vfsSeekSet; break;
	case CELL_SEEK_CUR: seek_mode = vfsSeekCur; break;
	case CELL_SEEK_END: seek_mode = vfsSeekEnd; break;
	default:
		sys_fs->Error(fd, "Unknown seek whence! (0x%x)", whence);
	return CELL_EINVAL;
	}

	u32 attr;
	vfsStream* file;
	if(!sys_fs->CheckId(fd, file, attr) || attr != IDFlag_File) return CELL_ESRCH;
	pos = file->Seek(offset, seek_mode);
	return CELL_OK;
}

s32 cellFsFtruncate(u32 fd, u64 size)
{
	sys_fs->Log("cellFsFtruncate(fd=%d, size=%lld)", fd, size);
	u32 attr;
	vfsStream* file;
	if(!sys_fs->CheckId(fd, file, attr) || attr != IDFlag_File) return CELL_ESRCH;
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

s32 cellFsTruncate(u32 path_addr, u64 size)
{
	const std::string& path = Memory.ReadString(path_addr);
	sys_fs->Log("cellFsTruncate(path=\"%s\", size=%lld)", path.c_str(), size);

	vfsFile f(path, vfsReadWrite);
	if(!f.IsOpened())
	{
		sys_fs->Warning("cellFsTruncate: \"%s\" not found.", path.c_str());
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

s32 cellFsFGetBlockSize(u32 fd, mem64_t sector_size, mem64_t block_size)
{
	sys_fs->Log("cellFsFGetBlockSize(fd=%d, sector_size_addr: 0x%x, block_size_addr: 0x%x)", fd, sector_size.GetAddr(), block_size.GetAddr());

	vfsStream* file;
	if(!sys_fs->CheckId(fd, file)) return CELL_ESRCH;

	sector_size = 4096; // ?
	block_size = 4096; // ?

	return CELL_OK;
}

s32 cellFsGetBlockSize(u32 path_addr, mem64_t sector_size, mem64_t block_size)
{
	sys_fs->Log("cellFsGetBlockSize(file: %s, sector_size_addr: 0x%x, block_size_addr: 0x%x)", Memory.ReadString(path_addr).c_str(), sector_size.GetAddr(), block_size.GetAddr());

	sector_size = 4096; // ?
	block_size = 4096; // ?

	return CELL_OK;
}

s32 cellFsGetFreeSize(u32 path_addr, mem32_t block_size, mem64_t block_count)
{
	const std::string& ps3_path = Memory.ReadString(path_addr);
	sys_fs->Warning("cellFsGetFreeSize(path=\"%s\", block_size_addr=0x%x, block_count_addr=0x%x)",
		ps3_path.c_str(), block_size.GetAddr(), block_count.GetAddr());

	if (ps3_path.empty())
		return CELL_EINVAL;

	// TODO: Get real values. Currently, it always returns 40 GB of free space divided in 4 KB blocks
	block_size = 4096; // ?
	block_count = 10485760; // ?

	return CELL_OK;
}

s32 cellFsGetDirectoryEntries(u32 fd, mem_ptr_t<CellFsDirectoryEntry> entries, u32 entries_size, mem32_t data_count)
{
	sys_fs->Log("cellFsGetDirectoryEntries(fd=%d, entries_addr=0x%x, entries_size = 0x%x, data_count_addr=0x%x)", fd, entries.GetAddr(), entries_size, data_count.GetAddr());

	vfsDirBase* directory;
	if(!sys_fs->CheckId(fd, directory))
		return CELL_ESRCH;

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

s32 cellFsStReadInit(u32 fd, mem_ptr_t<CellFsRingBuffer> ringbuf)
{
	sys_fs->Warning("cellFsStReadInit(fd=%d, ringbuf_addr=0x%x)", fd, ringbuf.GetAddr());

	vfsStream* file;
	if(!sys_fs->CheckId(fd, file)) return CELL_ESRCH;

	fs_config.m_ring_buffer = *ringbuf;

	if(ringbuf->ringbuf_size < 0x40000000) // If the size is less than 1MB
		fs_config.m_alloc_mem_size = ((ringbuf->ringbuf_size + 64 * 1024 - 1) / (64 * 1024)) * (64 * 1024);
	fs_config.m_alloc_mem_size = ((ringbuf->ringbuf_size + 1024 * 1024 - 1) / (1024 * 1024)) * (1024 * 1024);

	// alloc memory
	fs_config.m_buffer = Memory.Alloc(fs_config.m_alloc_mem_size, 1024);
	memset(Memory + fs_config.m_buffer, 0, fs_config.m_alloc_mem_size);

	fs_config.m_fs_status = CELL_FS_ST_INITIALIZED;

	return CELL_OK;
}

s32 cellFsStReadFinish(u32 fd)
{
	sys_fs->Warning("cellFsStReadFinish(fd=%d)", fd);

	vfsStream* file;
	if(!sys_fs->CheckId(fd, file)) return CELL_ESRCH;

	Memory.Free(fs_config.m_buffer);
	fs_config.m_fs_status = CELL_FS_ST_NOT_INITIALIZED;

	return CELL_OK;
}

s32 cellFsStReadGetRingBuf(u32 fd, mem_ptr_t<CellFsRingBuffer> ringbuf)
{
	sys_fs->Warning("cellFsStReadGetRingBuf(fd=%d, ringbuf_addr=0x%x)", fd, ringbuf.GetAddr());

	vfsStream* file;
	if(!sys_fs->CheckId(fd, file)) return CELL_ESRCH;

	*ringbuf = fs_config.m_ring_buffer;

	sys_fs->Warning("*** fs stream config: block_size=0x%llx, copy=%d, ringbuf_size = 0x%llx, transfer_rate = 0x%llx",
		ringbuf->block_size, ringbuf->copy,ringbuf->ringbuf_size, ringbuf->transfer_rate);
	return CELL_OK;
}

s32 cellFsStReadGetStatus(u32 fd, mem64_t status)
{
	sys_fs->Warning("cellFsStReadGetRingBuf(fd=%d, status_addr=0x%x)", fd, status.GetAddr());

	vfsStream* file;
	if(!sys_fs->CheckId(fd, file)) return CELL_ESRCH;

	status = fs_config.m_fs_status;

	return CELL_OK;
}

s32 cellFsStReadGetRegid(u32 fd, mem64_t regid)
{
	sys_fs->Warning("cellFsStReadGetRingBuf(fd=%d, regid_addr=0x%x)", fd, regid.GetAddr());

	vfsStream* file;
	if(!sys_fs->CheckId(fd, file)) return CELL_ESRCH;

	regid = fs_config.m_regid;

	return CELL_OK;
}

s32 cellFsStReadStart(u32 fd, u64 offset, u64 size)
{
	sys_fs->Todo("cellFsStReadStart(fd=%d, offset=0x%llx, size=0x%llx)", fd, offset, size);

	vfsStream* file;
	if(!sys_fs->CheckId(fd, file)) return CELL_ESRCH;

	fs_config.m_current_addr = fs_config.m_buffer + (u32)offset;
	fs_config.m_fs_status = CELL_FS_ST_PROGRESS;

	return CELL_OK;
}

s32 cellFsStReadStop(u32 fd)
{
	sys_fs->Warning("cellFsStReadStop(fd=%d)", fd);

	vfsStream* file;
	if(!sys_fs->CheckId(fd, file)) return CELL_ESRCH;

	fs_config.m_fs_status = CELL_FS_ST_STOP;

	return CELL_OK;
}

s32 cellFsStRead(u32 fd, u32 buf_addr, u64 size, mem64_t rsize)
{
	sys_fs->Todo("cellFsStRead(fd=%d, buf_addr=0x%x, size=0x%llx, rsize_addr = 0x%x)", fd, buf_addr, size, rsize.GetAddr());
	
	vfsStream* file;
	if(!sys_fs->CheckId(fd, file)) return CELL_ESRCH;

	fs_config.m_regid += size;
	rsize = fs_config.m_regid;

	return CELL_OK;
}

s32 cellFsStReadGetCurrentAddr(u32 fd, mem32_t addr_addr, mem64_t size)
{
	sys_fs->Todo("cellFsStReadGetCurrentAddr(fd=%d, addr_addr=0x%x, size_addr = 0x%x)", fd, addr_addr.GetAddr(), size.GetAddr());

	vfsStream* file;
	if(!sys_fs->CheckId(fd, file)) return CELL_ESRCH;

	return CELL_OK;
}

s32 cellFsStReadPutCurrentAddr(u32 fd, u32 addr_addr, u64 size)
{
	sys_fs->Todo("cellFsStReadPutCurrentAddr(fd=%d, addr_addr=0x%x, size = 0x%llx)", fd, addr_addr, size);
	
	vfsStream* file;
	if(!sys_fs->CheckId(fd, file)) return CELL_ESRCH;

	return CELL_OK;
}

s32 cellFsStReadWait(u32 fd, u64 size)
{
	sys_fs->Todo("cellFsStReadWait(fd=%d, size = 0x%llx)", fd, size);
	
	vfsStream* file;
	if(!sys_fs->CheckId(fd, file)) return CELL_ESRCH;
	
	return CELL_OK;
}

s32 cellFsStReadWaitCallback(u32 fd, u64 size, mem_func_ptr_t<void (*)(int xfd, u64 xsize)> func)
{
	sys_fs->Todo("cellFsStReadWaitCallback(fd=%d, size = 0x%llx, func_addr = 0x%x)", fd, size, func.GetAddr());

	vfsStream* file;
	if(!sys_fs->CheckId(fd, file)) return CELL_ESRCH;
	
	return CELL_OK;
}
