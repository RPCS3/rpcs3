#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
//#include "Emu/SysCalls/SysCalls.h"

#include "Emu/SysCalls/Modules.h"
#include "Emu/FS/VFS.h"
#include "Emu/FS/vfsFile.h"
#include "Emu/FS/vfsDir.h"
#include "lv2Fs.h"

extern Module *sys_fs;

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


s32 cellFsOpen(vm::ptr<const char> path, s32 flags, vm::ptr<be_t<u32>> fd, vm::ptr<u32> arg, u64 size)
{
	sys_fs->Log("cellFsOpen(path=\"%s\", flags=0x%x, fd_addr=0x%x, arg_addr=0x%x, size=0x%llx)",
		path.get_ptr(), flags, fd.addr(), arg.addr(), size);

	const std::string _path = path.get_ptr();

	LV2_LOCK(0);

	s32 _oflags = flags;
	if (flags & CELL_O_CREAT)
	{
		_oflags &= ~CELL_O_CREAT;
		Emu.GetVFS().CreateFile(_path);
	}

	vfsOpenMode o_mode;

	switch (flags & CELL_O_ACCMODE)
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
			auto filePtr = Emu.GetVFS().OpenFile(_path, vfsWrite);
			delete filePtr;
		}
		o_mode = vfsReadWrite;
	break;
	}

	if (_oflags != 0)
	{
		sys_fs->Error("\"%s\" has unknown flags! flags: 0x%08x", path.get_ptr(), flags);
		return CELL_EINVAL;
	}

	if (!Emu.GetVFS().ExistsFile(_path))
	{
		sys_fs->Error("\"%s\" not found! flags: 0x%08x", path.get_ptr(), flags);
		return CELL_ENOENT;
	}

	std::shared_ptr<vfsFileBase> stream(Emu.GetVFS().OpenFile(_path, o_mode));

	if (!stream || !stream->IsOpened())
	{
		sys_fs->Error("\"%s\" not found! flags: 0x%08x", path.get_ptr(), flags);
		return CELL_ENOENT;
	}

	u32 id = sys_fs->GetNewId(stream, TYPE_FS_FILE);
	*fd = id;
	sys_fs->Notice("\"%s\" opened: fd = %d", path.get_ptr(), id);

	return CELL_OK;
}

s32 cellFsRead(u32 fd, vm::ptr<void> buf, u64 nbytes, vm::ptr<be_t<u64>> nread)
{
	sys_fs->Log("cellFsRead(fd=%d, buf_addr=0x%x, nbytes=0x%llx, nread_addr=0x%x)",
		fd, buf.addr(), nbytes, nread.addr());

	LV2_LOCK(0);

	std::shared_ptr<vfsFileBase> file;
	if (!sys_fs->CheckId(fd, file))
		return CELL_ESRCH;

	if (nbytes != (u32)nbytes)
		return CELL_ENOMEM;

	// TODO: checks

	const u64 res = nbytes ? file->Read(buf.get_ptr(), nbytes) : 0;

	if (nread) *nread = res;

	return CELL_OK;
}

s32 cellFsWrite(u32 fd, vm::ptr<const void> buf, u64 nbytes, vm::ptr<u64> nwrite)
{
	sys_fs->Log("cellFsWrite(fd=%d, buf_addr=0x%x, nbytes=0x%llx, nwrite_addr=0x%x)",
		fd, buf.addr(), nbytes, nwrite.addr());

	LV2_LOCK(0);

	std::shared_ptr<vfsFileBase> file;
	if (!sys_fs->CheckId(fd, file)) return CELL_ESRCH;

	if (nbytes != (u32)nbytes) return CELL_ENOMEM;

	// TODO: checks

	const u64 res = nbytes ? file->Write(buf.get_ptr(), nbytes) : 0;

	if (nwrite) *nwrite = res;

	return CELL_OK;
}

s32 cellFsClose(u32 fd)
{
	sys_fs->Warning("cellFsClose(fd=%d)", fd);

	LV2_LOCK(0);

	if (!Emu.GetIdManager().RemoveID(fd))
		return CELL_ESRCH;

	return CELL_OK;
}

s32 cellFsOpendir(vm::ptr<const char> path, vm::ptr<u32> fd)
{
	sys_fs->Warning("cellFsOpendir(path=\"%s\", fd_addr=0x%x)", path.get_ptr(), fd.addr());

	LV2_LOCK(0);
	
	std::shared_ptr<vfsDirBase> dir(Emu.GetVFS().OpenDir(path.get_ptr()));
	if (!dir || !dir->IsOpened())
	{
		return CELL_ENOENT;
	}

	*fd = sys_fs->GetNewId(dir, TYPE_FS_DIR);
	return CELL_OK;
}

s32 cellFsReaddir(u32 fd, vm::ptr<CellFsDirent> dir, vm::ptr<u64> nread)
{
	sys_fs->Warning("cellFsReaddir(fd=%d, dir_addr=0x%x, nread_addr=0x%x)", fd, dir.addr(), nread.addr());

	LV2_LOCK(0);

	std::shared_ptr<vfsDirBase> directory;
	if (!sys_fs->CheckId(fd, directory))
		return CELL_ESRCH;

	const DirEntryInfo* info = directory->Read();
	if (info)
	{
		dir->d_type = (info->flags & DirEntry_TypeFile) ? CELL_FS_TYPE_REGULAR : CELL_FS_TYPE_DIRECTORY;
		dir->d_namlen = u8(std::min((u32)info->name.length(), (u32)CELL_MAX_FS_FILE_NAME_LENGTH));
		strcpy_trunc(dir->d_name, info->name);
		*nread = sizeof(CellFsDirent);
	}
	else
	{
		*nread = 0;
	}

	return CELL_OK;
}

s32 cellFsClosedir(u32 fd)
{
	sys_fs->Warning("cellFsClosedir(fd=%d)", fd);

	LV2_LOCK(0);

	if (!Emu.GetIdManager().RemoveID(fd))
		return CELL_ESRCH;

	return CELL_OK;
}

s32 cellFsStat(vm::ptr<const char> path, vm::ptr<CellFsStat> sb)
{
	sys_fs->Warning("cellFsStat(path=\"%s\", sb_addr=0x%x)", path.get_ptr(), sb.addr());

	LV2_LOCK(0);

	const std::string _path = path.get_ptr();

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
		vfsDir dir(_path);
		if(dir.IsOpened())
		{
			sb->st_mode |= CELL_FS_S_IFDIR;
			return CELL_OK;
		}
	}

	{
		vfsFile f(_path);
		if(f.IsOpened())
		{
			sb->st_mode |= CELL_FS_S_IFREG;
			sb->st_size = f.GetSize();
			return CELL_OK;
		}
	}

	sys_fs->Warning("cellFsStat: \"%s\" not found.", path.get_ptr());
	return CELL_ENOENT;
}

s32 cellFsFstat(u32 fd, vm::ptr<CellFsStat> sb)
{
	sys_fs->Warning("cellFsFstat(fd=%d, sb_addr=0x%x)", fd, sb.addr());

	LV2_LOCK(0);

	IDType type;
	std::shared_ptr<vfsFileBase> file;
	if (!sys_fs->CheckId(fd, file, type) || type != TYPE_FS_FILE)
		return CELL_ESRCH;

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

s32 cellFsMkdir(vm::ptr<const char> path, u32 mode)
{
	sys_fs->Warning("cellFsMkdir(path=\"%s\", mode=0x%x)", path.get_ptr(), mode);

	LV2_LOCK(0);

	const std::string _path = path.get_ptr();

	if (vfsDir().IsExists(_path))
		return CELL_EEXIST;

	if (!Emu.GetVFS().CreateDir(_path))
		return CELL_EBUSY;

	return CELL_OK;
}

s32 cellFsRename(vm::ptr<const char> from, vm::ptr<const char> to)
{
	sys_fs->Warning("cellFsRename(from='%s', to='%s')", from.get_ptr(), to.get_ptr());

	LV2_LOCK(0);

	std::string _from = from.get_ptr();
	std::string _to = to.get_ptr();

	{
		vfsDir dir;
		if(dir.IsExists(_from))
		{
			if(!dir.Rename(_from, _to))
				return CELL_EBUSY;

			return CELL_OK;
		}
	}

	{
		vfsFile f;

		if(f.Exists(_from))
		{
			if(!f.Rename(_from, _to))
				return CELL_EBUSY;

			return CELL_OK;
		}
	}

	return CELL_ENOENT;
}
s32 cellFsChmod(vm::ptr<const char> path, u32 mode)
{
	sys_fs->Todo("cellFsChmod(path=\"%s\", mode=0x%x)", path.get_ptr(), mode);

	LV2_LOCK(0);

	// TODO:

	return CELL_OK;
}

s32 cellFsFsync(u32 fd)
{
	sys_fs->Todo("cellFsFsync(fd=0x%x)", fd);

	LV2_LOCK(0);

	// TODO:

	return CELL_OK;
}

s32 cellFsRmdir(vm::ptr<const char> path)
{
	sys_fs->Warning("cellFsRmdir(path=\"%s\")", path.get_ptr());

	LV2_LOCK(0);

	std::string _path = path.get_ptr();

	vfsDir d;
	if (!d.IsExists(_path))
		return CELL_ENOENT;

	if (!d.Remove(_path))
		return CELL_EBUSY;

	return CELL_OK;
}

s32 cellFsUnlink(vm::ptr<const char> path)
{
	sys_fs->Warning("cellFsUnlink(path=\"%s\")", path.get_ptr());

	LV2_LOCK(0);

	std::string _path = path.get_ptr();

	if (vfsDir().IsExists(_path))
		return CELL_EISDIR;

	if (!Emu.GetVFS().ExistsFile(_path))
		return CELL_ENOENT;

	if (!Emu.GetVFS().RemoveFile(_path))
		return CELL_EACCES;
	
	return CELL_OK;
}

s32 cellFsLseek(u32 fd, s64 offset, u32 whence, vm::ptr<be_t<u64>> pos)
{
	sys_fs->Log("cellFsLseek(fd=%d, offset=0x%llx, whence=0x%x, pos_addr=0x%x)", fd, offset, whence, pos.addr());

	LV2_LOCK(0);

	vfsSeekMode seek_mode;
	switch(whence)
	{
	case CELL_SEEK_SET: seek_mode = vfsSeekSet; break;
	case CELL_SEEK_CUR: seek_mode = vfsSeekCur; break;
	case CELL_SEEK_END: seek_mode = vfsSeekEnd; break;
	default:
		sys_fs->Error(fd, "Unknown seek whence! (0x%x)", whence);
	return CELL_EINVAL;
	}

	IDType type;
	std::shared_ptr<vfsFileBase> file;
	if (!sys_fs->CheckId(fd, file, type) || type != TYPE_FS_FILE)
		return CELL_ESRCH;

	*pos = file->Seek(offset, seek_mode);
	return CELL_OK;
}

s32 cellFsFtruncate(u32 fd, u64 size)
{
	sys_fs->Warning("cellFsFtruncate(fd=%d, size=%lld)", fd, size);

	LV2_LOCK(0);
	
	IDType type;
	std::shared_ptr<vfsFileBase> file;
	if (!sys_fs->CheckId(fd, file, type) || type != TYPE_FS_FILE)
		return CELL_ESRCH;

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

s32 cellFsTruncate(vm::ptr<const char> path, u64 size)
{
	sys_fs->Warning("cellFsTruncate(path=\"%s\", size=%lld)", path.get_ptr(), size);

	LV2_LOCK(0);

	vfsFile f(path.get_ptr(), vfsReadWrite);
	if (!f.IsOpened())
	{
		sys_fs->Warning("cellFsTruncate: \"%s\" not found.", path.get_ptr());
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

s32 cellFsFGetBlockSize(u32 fd, vm::ptr<u64> sector_size, vm::ptr<u64> block_size)
{
	sys_fs->Warning("cellFsFGetBlockSize(fd=%d, sector_size_addr=0x%x, block_size_addr=0x%x)",
		fd, sector_size.addr(), block_size.addr());

	LV2_LOCK(0);

	std::shared_ptr<vfsFileBase> file;
	if (!sys_fs->CheckId(fd, file))
		return CELL_ESRCH;

	*sector_size = 4096; // ?
	*block_size = 4096; // ?

	return CELL_OK;
}

s32 cellFsGetBlockSize(vm::ptr<const char> path, vm::ptr<u64> sector_size, vm::ptr<u64> block_size)
{
	sys_fs->Warning("cellFsGetBlockSize(file='%s', sector_size_addr=0x%x, block_size_addr=0x%x)",
		path.get_ptr(), sector_size.addr(), block_size.addr());

	LV2_LOCK(0);

	*sector_size = 4096; // ?
	*block_size = 4096; // ?

	return CELL_OK;
}

s32 cellFsGetFreeSize(vm::ptr<const char> path, vm::ptr<u32> block_size, vm::ptr<u64> block_count)
{
	sys_fs->Warning("cellFsGetFreeSize(path=\"%s\", block_size_addr=0x%x, block_count_addr=0x%x)",
		path.get_ptr(), block_size.addr(), block_count.addr());

	LV2_LOCK(0);

	// TODO: Get real values. Currently, it always returns 40 GB of free space divided in 4 KB blocks
	*block_size = 4096; // ?
	*block_count = 10 * 1024 * 1024; // ?

	return CELL_OK;
}

s32 cellFsGetDirectoryEntries(u32 fd, vm::ptr<CellFsDirectoryEntry> entries, u32 entries_size, vm::ptr<u32> data_count)
{
	sys_fs->Warning("cellFsGetDirectoryEntries(fd=%d, entries_addr=0x%x, entries_size=0x%x, data_count_addr=0x%x)",
		fd, entries.addr(), entries_size, data_count.addr());

	LV2_LOCK(0);

	std::shared_ptr<vfsDirBase> directory;
	if (!sys_fs->CheckId(fd, directory))
		return CELL_ESRCH;

	const DirEntryInfo* info = directory->Read();
	if (info)
	{
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

		entries->entry_name.d_type = (info->flags & DirEntry_TypeFile) ? CELL_FS_TYPE_REGULAR : CELL_FS_TYPE_DIRECTORY;
		entries->entry_name.d_namlen = u8(std::min((u32)info->name.length(), (u32)CELL_MAX_FS_FILE_NAME_LENGTH));
		strcpy_trunc(entries->entry_name.d_name, info->name);
		*data_count = 1;
	}
	else
	{
		*data_count = 0;
	}

	return CELL_OK;
}

s32 cellFsStReadInit(u32 fd, vm::ptr<CellFsRingBuffer> ringbuf)
{
	sys_fs->Warning("cellFsStReadInit(fd=%d, ringbuf_addr=0x%x)", fd, ringbuf.addr());

	LV2_LOCK(0);

	std::shared_ptr<vfsFileBase> file;
	if (!sys_fs->CheckId(fd, file))
		return CELL_ESRCH;

	fs_config.m_ring_buffer = *ringbuf;

    // If the size is less than 1MB
	if (ringbuf->ringbuf_size < 0x40000000)
		fs_config.m_alloc_mem_size = (((u32)ringbuf->ringbuf_size + 64 * 1024 - 1) / (64 * 1024)) * (64 * 1024);
    else
	    fs_config.m_alloc_mem_size = (((u32)ringbuf->ringbuf_size + 1024 * 1024 - 1) / (1024 * 1024)) * (1024 * 1024);

	// alloc memory
	fs_config.m_buffer = (u32)Memory.Alloc(fs_config.m_alloc_mem_size, 1024);
	memset(vm::get_ptr<void>(fs_config.m_buffer), 0, fs_config.m_alloc_mem_size);

	fs_config.m_fs_status = CELL_FS_ST_INITIALIZED;

	return CELL_OK;
}

s32 cellFsStReadFinish(u32 fd)
{
	sys_fs->Warning("cellFsStReadFinish(fd=%d)", fd);

	LV2_LOCK(0);

	std::shared_ptr<vfsFileBase> file;
	if (!sys_fs->CheckId(fd, file))
		return CELL_ESRCH;

	Memory.Free(fs_config.m_buffer);
	fs_config.m_fs_status = CELL_FS_ST_NOT_INITIALIZED;

	return CELL_OK;
}

s32 cellFsStReadGetRingBuf(u32 fd, vm::ptr<CellFsRingBuffer> ringbuf)
{
	sys_fs->Warning("cellFsStReadGetRingBuf(fd=%d, ringbuf_addr=0x%x)", fd, ringbuf.addr());

	LV2_LOCK(0);

	std::shared_ptr<vfsFileBase> file;
	if (!sys_fs->CheckId(fd, file))
		return CELL_ESRCH;

	*ringbuf = fs_config.m_ring_buffer;

	sys_fs->Warning("*** fs stream config: block_size=0x%llx, copy=%d, ringbuf_size=0x%llx, transfer_rate=0x%llx",
		(u64)ringbuf->block_size, (u32)ringbuf->copy, (u64)ringbuf->ringbuf_size, (u64)ringbuf->transfer_rate);
	return CELL_OK;
}

s32 cellFsStReadGetStatus(u32 fd, vm::ptr<u64> status)
{
	sys_fs->Warning("cellFsStReadGetRingBuf(fd=%d, status_addr=0x%x)", fd, status.addr());

	LV2_LOCK(0);

	std::shared_ptr<vfsFileBase> file;
	if (!sys_fs->CheckId(fd, file))
		return CELL_ESRCH;

	*status = fs_config.m_fs_status;

	return CELL_OK;
}

s32 cellFsStReadGetRegid(u32 fd, vm::ptr<u64> regid)
{
	sys_fs->Warning("cellFsStReadGetRingBuf(fd=%d, regid_addr=0x%x)", fd, regid.addr());

	LV2_LOCK(0);

	std::shared_ptr<vfsFileBase> file;
	if (!sys_fs->CheckId(fd, file))
		return CELL_ESRCH;

	*regid = fs_config.m_regid;

	return CELL_OK;
}

s32 cellFsStReadStart(u32 fd, u64 offset, u64 size)
{
	sys_fs->Todo("cellFsStReadStart(fd=%d, offset=0x%llx, size=0x%llx)", fd, offset, size);

	LV2_LOCK(0);

	std::shared_ptr<vfsFileBase> file;
	if (!sys_fs->CheckId(fd, file))
		return CELL_ESRCH;

	fs_config.m_current_addr = fs_config.m_buffer + (u32)offset;
	fs_config.m_fs_status = CELL_FS_ST_PROGRESS;

	return CELL_OK;
}

s32 cellFsStReadStop(u32 fd)
{
	sys_fs->Warning("cellFsStReadStop(fd=%d)", fd);

	LV2_LOCK(0);

	std::shared_ptr<vfsFileBase> file;
	if (!sys_fs->CheckId(fd, file))
		return CELL_ESRCH;

	fs_config.m_fs_status = CELL_FS_ST_STOP;

	return CELL_OK;
}

s32 cellFsStRead(u32 fd, u32 buf_addr, u64 size, vm::ptr<u64> rsize)
{
	sys_fs->Warning("cellFsStRead(fd=%d, buf_addr=0x%x, size=0x%llx, rsize_addr=0x%x)", fd, buf_addr, size, rsize.addr());

	LV2_LOCK(0);
	
	std::shared_ptr<vfsFileBase> file;
	if (!sys_fs->CheckId(fd, file))
		return CELL_ESRCH;

	// TODO: use ringbuffer (fs_config)
	fs_config.m_regid += size;

	if (file->Eof())
		return CELL_FS_ERANGE;

	*rsize = file->Read(vm::get_ptr<void>(buf_addr), size);

	return CELL_OK;
}

s32 cellFsStReadGetCurrentAddr(u32 fd, vm::ptr<u32> addr, vm::ptr<u64> size)
{
	sys_fs->Todo("cellFsStReadGetCurrentAddr(fd=%d, addr_addr=0x%x, size_addr=0x%x)", fd, addr.addr(), size.addr());

	LV2_LOCK(0);

	std::shared_ptr<vfsFileBase> file;
	if (!sys_fs->CheckId(fd, file))
		return CELL_ESRCH;

	return CELL_OK;
}

s32 cellFsStReadPutCurrentAddr(u32 fd, u32 addr_addr, u64 size)
{
	sys_fs->Todo("cellFsStReadPutCurrentAddr(fd=%d, addr_addr=0x%x, size=0x%llx)", fd, addr_addr, size);

	LV2_LOCK(0);
	
	std::shared_ptr<vfsFileBase> file;
	if (!sys_fs->CheckId(fd, file))
		return CELL_ESRCH;

	return CELL_OK;
}

s32 cellFsStReadWait(u32 fd, u64 size)
{
	sys_fs->Todo("cellFsStReadWait(fd=%d, size=0x%llx)", fd, size);

	LV2_LOCK(0);
	
	std::shared_ptr<vfsFileBase> file;
	if (!sys_fs->CheckId(fd, file))
		return CELL_ESRCH;
	
	return CELL_OK;
}

s32 cellFsStReadWaitCallback(u32 fd, u64 size, vm::ptr<void (*)(int xfd, u64 xsize)> func)
{
	sys_fs->Todo("cellFsStReadWaitCallback(fd=%d, size=0x%llx, func_addr=0x%x)", fd, size, func.addr());

	LV2_LOCK(0);

	std::shared_ptr<vfsFileBase> file;
	if (!sys_fs->CheckId(fd, file))
		return CELL_ESRCH;
	
	return CELL_OK;
}
