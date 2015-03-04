#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/Callback.h"
#include "Emu/SysCalls/CB_FUNC.h"

#ifdef _WIN32
	#include <windows.h>
	#undef CreateFile
#else
	#include <sys/types.h>
	#include <sys/stat.h>
#endif

#include "Emu/FS/VFS.h"
#include "Emu/FS/vfsFile.h"
#include "Emu/FS/vfsDir.h"
#include "cellFs.h"

extern Module sys_fs;

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


s32 cellFsOpen(vm::ptr<const char> path, s32 flags, vm::ptr<be_t<u32>> fd, vm::ptr<const void> arg, u64 size)
{
	sys_fs.Log("cellFsOpen(path_addr=0x%x, flags=0x%x, fd=0x%x, arg=0x%x, size=0x%llx)", path.addr(), flags, fd, arg, size);
	sys_fs.Log("cellFsOpen(path='%s')", path.get_ptr());

	const std::string _path = path.get_ptr();

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
		sys_fs.Error("cellFsOpen(): '%s' has unknown flags! flags: 0x%08x", path.get_ptr(), flags);
		return CELL_EINVAL;
	}

	if (!Emu.GetVFS().ExistsFile(_path))
	{
		sys_fs.Error("cellFsOpen(): '%s' not found! flags: 0x%08x", path.get_ptr(), flags);
		return CELL_ENOENT;
	}

	std::shared_ptr<vfsStream> stream((vfsStream*)Emu.GetVFS().OpenFile(_path, o_mode));

	if (!stream || !stream->IsOpened())
	{
		sys_fs.Error("cellFsOpen(): '%s' not found! flags: 0x%08x", path.get_ptr(), flags);
		return CELL_ENOENT;
	}

	u32 id = sys_fs.GetNewId(stream, TYPE_FS_FILE);
	*fd = id;
	sys_fs.Notice("cellFsOpen(): '%s' opened, id -> 0x%x", path.get_ptr(), id);

	return CELL_OK;
}

s32 cellFsRead(u32 fd, vm::ptr<void> buf, u64 nbytes, vm::ptr<be_t<u64>> nread)
{
	sys_fs.Log("cellFsRead(fd=0x%x, buf=0x%x, nbytes=0x%llx, nread=0x%x)", fd, buf, nbytes, nread);

	std::shared_ptr<vfsStream> file;
	if (!sys_fs.CheckId(fd, file))
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
	sys_fs.Log("cellFsWrite(fd=0x%x, buf=0x%x, nbytes=0x%llx, nwrite=0x%x)", fd, buf, nbytes, nwrite);

	std::shared_ptr<vfsStream> file;
	if (!sys_fs.CheckId(fd, file)) return CELL_ESRCH;

	if (nbytes != (u32)nbytes) return CELL_ENOMEM;

	// TODO: checks

	const u64 res = nbytes ? file->Write(buf.get_ptr(), nbytes) : 0;

	if (nwrite) *nwrite = res;

	return CELL_OK;
}

s32 cellFsClose(u32 fd)
{
	sys_fs.Warning("cellFsClose(fd=0x%x)", fd);

	if (!Emu.GetIdManager().RemoveID(fd))
		return CELL_ESRCH;

	return CELL_OK;
}

s32 cellFsOpendir(vm::ptr<const char> path, vm::ptr<u32> fd)
{
	sys_fs.Warning("cellFsOpendir(path_addr=0x%x, fd=0x%x)", path.addr(), fd);
	sys_fs.Warning("cellFsOpendir(path='%s')", path.get_ptr());
	
	std::shared_ptr<vfsDirBase> dir(Emu.GetVFS().OpenDir(path.get_ptr()));
	if (!dir || !dir->IsOpened())
	{
		return CELL_ENOENT;
	}

	*fd = sys_fs.GetNewId(dir, TYPE_FS_DIR);
	return CELL_OK;
}

s32 cellFsReaddir(u32 fd, vm::ptr<CellFsDirent> dir, vm::ptr<u64> nread)
{
	sys_fs.Warning("cellFsReaddir(fd=0x%x, dir=0x%x, nread=0x%x)", fd, dir, nread);

	std::shared_ptr<vfsDirBase> directory;
	if (!sys_fs.CheckId(fd, directory))
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
	sys_fs.Warning("cellFsClosedir(fd=0x%x)", fd);

	if (!Emu.GetIdManager().RemoveID(fd))
		return CELL_ESRCH;

	return CELL_OK;
}

s32 cellFsStat(vm::ptr<const char> path, vm::ptr<CellFsStat> sb)
{
	sys_fs.Warning("cellFsStat(path_addr=0x%x, sb=0x%x)", path.addr(), sb);
	sys_fs.Warning("cellFsStat(path='%s')", path.get_ptr());

	const std::string _path = path.get_ptr();

	u32 mode = 0;
	s32 uid = 0;
	s32 gid = 0;
	u64 atime = 0;
	u64 mtime = 0;
	u64 ctime = 0;
	u64 size = 0;

	std::string real_path;

	Emu.GetVFS().GetDevice(_path, real_path);

	int stat_result;
#ifdef _WIN32
	struct _stat64 buf;
	stat_result = _stat64(real_path.c_str(), &buf);
#else
	struct stat buf;
	stat_result = stat(real_path.c_str(), &buf);
#endif
	if (stat_result)
	{
		sys_fs.Error("cellFsStat(): stat('%s') failed -> 0x%x", real_path.c_str(), stat_result);
	}
	else
	{
		mode = buf.st_mode;
		uid = buf.st_uid;
		gid = buf.st_gid;
		atime = buf.st_atime;
		mtime = buf.st_mtime;
		ctime = buf.st_ctime;
		size = buf.st_size;
	}

	sb->st_mode = 
		CELL_FS_S_IRUSR | CELL_FS_S_IWUSR | CELL_FS_S_IXUSR |
		CELL_FS_S_IRGRP | CELL_FS_S_IWGRP | CELL_FS_S_IXGRP |
		CELL_FS_S_IROTH | CELL_FS_S_IWOTH | CELL_FS_S_IXOTH;

	if (sb->st_mode == mode)
		sys_fs.Error("cellFsStat(): mode is the same (0x%x)", mode);

	sb->st_uid = uid;
	sb->st_gid = gid;
	sb->st_atime_ = atime;
	sb->st_mtime_ = mtime;
	sb->st_ctime_ = ctime;
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

	if (sb->st_size == size && size != 0)
		sys_fs.Error("cellFsStat(): size is the same (0x%x)", size);

	sys_fs.Warning("cellFsStat(): '%s' not found", path.get_ptr());
	return CELL_ENOENT;
}

s32 cellFsFstat(u32 fd, vm::ptr<CellFsStat> sb)
{
	sys_fs.Warning("cellFsFstat(fd=0x%x, sb=0x%x)", fd, sb);

	IDType type;
	std::shared_ptr<vfsStream> file;
	if (!sys_fs.CheckId(fd, file, type) || type != TYPE_FS_FILE)
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
	sys_fs.Warning("cellFsMkdir(path_addr=0x%x, mode=0x%x)", path.addr(), mode);
	sys_fs.Warning("cellFsMkdir(path='%s')", path.get_ptr());

	const std::string _path = path.get_ptr();

	if (vfsDir().IsExists(_path))
		return CELL_EEXIST;

	if (!Emu.GetVFS().CreateDir(_path))
		return CELL_EBUSY;

	sys_fs.Notice("cellFsMkdir(): directory '%s' created", path.get_ptr());
	return CELL_OK;
}

s32 cellFsRename(vm::ptr<const char> from, vm::ptr<const char> to)
{
	sys_fs.Warning("cellFsRename(from_addr=0x%x, to_addr=0x%x)", from.addr(), to.addr());
	sys_fs.Warning("cellFsRename(from='%s', to='%s')", from.get_ptr(), to.get_ptr());

	std::string _from = from.get_ptr();
	std::string _to = to.get_ptr();

	{
		vfsDir dir;
		if(dir.IsExists(_from))
		{
			if(!dir.Rename(_from, _to))
				return CELL_EBUSY;

			sys_fs.Notice("cellFsRename(): directory '%s' renamed to '%s'", from.get_ptr(), to.get_ptr());
			return CELL_OK;
		}
	}

	{
		vfsFile f;

		if(f.Exists(_from))
		{
			if(!f.Rename(_from, _to))
				return CELL_EBUSY;

			sys_fs.Notice("cellFsRename(): file '%s' renamed to '%s'", from.get_ptr(), to.get_ptr());
			return CELL_OK;
		}
	}

	return CELL_ENOENT;
}
s32 cellFsChmod(vm::ptr<const char> path, u32 mode)
{
	sys_fs.Todo("cellFsChmod(path_addr=0x%x, mode=0x%x)", path.addr(), mode);
	sys_fs.Todo("cellFsChmod(path='%s')", path.get_ptr());

	// TODO:

	return CELL_OK;
}

s32 cellFsFsync(u32 fd)
{
	sys_fs.Todo("cellFsFsync(fd=0x%x)", fd);

	// TODO:

	return CELL_OK;
}

s32 cellFsRmdir(vm::ptr<const char> path)
{
	sys_fs.Warning("cellFsRmdir(path_addr=0x%x)", path.addr());
	sys_fs.Warning("cellFsRmdir(path='%s')", path.get_ptr());

	std::string _path = path.get_ptr();

	vfsDir d;
	if (!d.IsExists(_path))
		return CELL_ENOENT;

	if (!d.Remove(_path))
		return CELL_EBUSY;

	sys_fs.Notice("cellFsRmdir(): directory '%s' removed", path.get_ptr());
	return CELL_OK;
}

s32 cellFsUnlink(vm::ptr<const char> path)
{
	sys_fs.Warning("cellFsUnlink(path_addr=0x%x)", path.addr());
	sys_fs.Warning("cellFsUnlink(path='%s')", path.get_ptr());

	std::string _path = path.get_ptr();

	if (vfsDir().IsExists(_path))
		return CELL_EISDIR;

	if (!Emu.GetVFS().ExistsFile(_path))
		return CELL_ENOENT;

	if (!Emu.GetVFS().RemoveFile(_path))
		return CELL_EACCES;
	
	sys_fs.Notice("cellFsUnlink(): file '%s' removed", path.get_ptr());
	return CELL_OK;
}

s32 cellFsLseek(u32 fd, s64 offset, u32 whence, vm::ptr<be_t<u64>> pos)
{
	sys_fs.Log("cellFsLseek(fd=0x%x, offset=0x%llx, whence=0x%x, pos=0x%x)", fd, offset, whence, pos);

	vfsSeekMode seek_mode;
	switch(whence)
	{
	case CELL_SEEK_SET: seek_mode = vfsSeekSet; break;
	case CELL_SEEK_CUR: seek_mode = vfsSeekCur; break;
	case CELL_SEEK_END: seek_mode = vfsSeekEnd; break;
	default:
		sys_fs.Error("cellFsLseek(fd=0x%x): Unknown seek whence! (0x%x)", fd, whence);
	return CELL_EINVAL;
	}

	IDType type;
	std::shared_ptr<vfsStream> file;
	if (!sys_fs.CheckId(fd, file, type) || type != TYPE_FS_FILE)
		return CELL_ESRCH;

	*pos = file->Seek(offset, seek_mode);
	return CELL_OK;
}

s32 cellFsFtruncate(u32 fd, u64 size)
{
	sys_fs.Warning("cellFsFtruncate(fd=0x%x, size=0x%llx)", fd, size);
	
	IDType type;
	std::shared_ptr<vfsStream> file;
	if (!sys_fs.CheckId(fd, file, type) || type != TYPE_FS_FILE)
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
	sys_fs.Warning("cellFsTruncate(path_addr=0x%x, size=0x%llx)", path.addr(), size);
	sys_fs.Warning("cellFsTruncate(path='%s')", path.get_ptr());

	vfsFile f(path.get_ptr(), vfsReadWrite);
	if (!f.IsOpened())
	{
		sys_fs.Warning("cellFsTruncate(): '%s' not found", path.get_ptr());
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
	sys_fs.Warning("cellFsFGetBlockSize(fd=0x%x, sector_size=0x%x, block_size=0x%x)", fd, sector_size, block_size);

	std::shared_ptr<vfsStream> file;
	if (!sys_fs.CheckId(fd, file))
		return CELL_ESRCH;

	*sector_size = 4096; // ?
	*block_size = 4096; // ?

	return CELL_OK;
}

s32 cellFsGetBlockSize(vm::ptr<const char> path, vm::ptr<u64> sector_size, vm::ptr<u64> block_size)
{
	sys_fs.Warning("cellFsGetBlockSize(path_addr=0x%x, sector_size=0x%x, block_size=0x%x)", path.addr(), sector_size, block_size);
	sys_fs.Warning("cellFsGetBlockSize(path='%s')", path.get_ptr());

	*sector_size = 4096; // ?
	*block_size = 4096; // ?

	return CELL_OK;
}

s32 cellFsGetFreeSize(vm::ptr<const char> path, vm::ptr<u32> block_size, vm::ptr<u64> block_count)
{
	sys_fs.Warning("cellFsGetFreeSize(path_addr=0x%x, block_size=0x%x, block_count=0x%x)", path.addr(), block_size, block_count);
	sys_fs.Warning("cellFsGetFreeSize(path='%s')", path.get_ptr());

	// TODO: Get real values. Currently, it always returns 40 GB of free space divided in 4 KB blocks
	*block_size = 4096; // ?
	*block_count = 10 * 1024 * 1024; // ?

	return CELL_OK;
}

s32 cellFsGetDirectoryEntries(u32 fd, vm::ptr<CellFsDirectoryEntry> entries, u32 entries_size, vm::ptr<u32> data_count)
{
	sys_fs.Warning("cellFsGetDirectoryEntries(fd=0x%x, entries=0x%x, entries_size=0x%x, data_count=0x%x)", fd, entries, entries_size, data_count);

	std::shared_ptr<vfsDirBase> directory;
	if (!sys_fs.CheckId(fd, directory))
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
	sys_fs.Warning("cellFsStReadInit(fd=0x%x, ringbuf=0x%x)", fd, ringbuf);

	std::shared_ptr<vfsStream> file;
	if (!sys_fs.CheckId(fd, file))
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
	sys_fs.Warning("cellFsStReadFinish(fd=0x%x)", fd);

	std::shared_ptr<vfsStream> file;
	if (!sys_fs.CheckId(fd, file))
		return CELL_ESRCH;

	Memory.Free(fs_config.m_buffer);
	fs_config.m_fs_status = CELL_FS_ST_NOT_INITIALIZED;

	return CELL_OK;
}

s32 cellFsStReadGetRingBuf(u32 fd, vm::ptr<CellFsRingBuffer> ringbuf)
{
	sys_fs.Warning("cellFsStReadGetRingBuf(fd=0x%x, ringbuf=0x%x)", fd, ringbuf);

	std::shared_ptr<vfsStream> file;
	if (!sys_fs.CheckId(fd, file))
		return CELL_ESRCH;

	*ringbuf = fs_config.m_ring_buffer;

	sys_fs.Warning("*** fs stream config: block_size=0x%llx, copy=0x%x, ringbuf_size=0x%llx, transfer_rate=0x%llx",
		ringbuf->block_size, ringbuf->copy, ringbuf->ringbuf_size, ringbuf->transfer_rate);
	return CELL_OK;
}

s32 cellFsStReadGetStatus(u32 fd, vm::ptr<u64> status)
{
	sys_fs.Warning("cellFsStReadGetRingBuf(fd=0x%x, status=0x%x)", fd, status);

	std::shared_ptr<vfsStream> file;
	if (!sys_fs.CheckId(fd, file))
		return CELL_ESRCH;

	*status = fs_config.m_fs_status;

	return CELL_OK;
}

s32 cellFsStReadGetRegid(u32 fd, vm::ptr<u64> regid)
{
	sys_fs.Warning("cellFsStReadGetRingBuf(fd=0x%x, regid=0x%x)", fd, regid);

	std::shared_ptr<vfsStream> file;
	if (!sys_fs.CheckId(fd, file))
		return CELL_ESRCH;

	*regid = fs_config.m_regid;

	return CELL_OK;
}

s32 cellFsStReadStart(u32 fd, u64 offset, u64 size)
{
	sys_fs.Todo("cellFsStReadStart(fd=0x%x, offset=0x%llx, size=0x%llx)", fd, offset, size);

	std::shared_ptr<vfsStream> file;
	if (!sys_fs.CheckId(fd, file))
		return CELL_ESRCH;

	fs_config.m_current_addr = fs_config.m_buffer + (u32)offset;
	fs_config.m_fs_status = CELL_FS_ST_PROGRESS;

	return CELL_OK;
}

s32 cellFsStReadStop(u32 fd)
{
	sys_fs.Warning("cellFsStReadStop(fd=0x%x)", fd);

	std::shared_ptr<vfsStream> file;
	if (!sys_fs.CheckId(fd, file))
		return CELL_ESRCH;

	fs_config.m_fs_status = CELL_FS_ST_STOP;

	return CELL_OK;
}

s32 cellFsStRead(u32 fd, vm::ptr<u8> buf, u64 size, vm::ptr<u64> rsize)
{
	sys_fs.Warning("cellFsStRead(fd=0x%x, buf=0x%x, size=0x%llx, rsize=0x%x)", fd, buf, size, rsize);
	
	std::shared_ptr<vfsStream> file;
	if (!sys_fs.CheckId(fd, file))
		return CELL_ESRCH;

	// TODO: use ringbuffer (fs_config)
	fs_config.m_regid += size;

	if (file->Eof())
		return CELL_FS_ERANGE;

	*rsize = file->Read(buf.get_ptr(), size);

	return CELL_OK;
}

s32 cellFsStReadGetCurrentAddr(u32 fd, vm::ptr<vm::ptr<u8>> addr, vm::ptr<u64> size)
{
	sys_fs.Todo("cellFsStReadGetCurrentAddr(fd=0x%x, addr=0x%x, size=0x%x)", fd, addr, size);

	std::shared_ptr<vfsStream> file;
	if (!sys_fs.CheckId(fd, file))
		return CELL_ESRCH;

	return CELL_OK;
}

s32 cellFsStReadPutCurrentAddr(u32 fd, vm::ptr<u8> addr, u64 size)
{
	sys_fs.Todo("cellFsStReadPutCurrentAddr(fd=0x%x, addr=0x%x, size=0x%llx)", fd, addr, size);
	
	std::shared_ptr<vfsStream> file;
	if (!sys_fs.CheckId(fd, file))
		return CELL_ESRCH;

	return CELL_OK;
}

s32 cellFsStReadWait(u32 fd, u64 size)
{
	sys_fs.Todo("cellFsStReadWait(fd=0x%x, size=0x%llx)", fd, size);
	
	std::shared_ptr<vfsStream> file;
	if (!sys_fs.CheckId(fd, file))
		return CELL_ESRCH;
	
	return CELL_OK;
}

s32 cellFsStReadWaitCallback(u32 fd, u64 size, vm::ptr<void(int xfd, u64 xsize)> func)
{
	sys_fs.Todo("cellFsStReadWaitCallback(fd=0x%x, size=0x%llx, func=0x%x)", fd, size, func);

	std::shared_ptr<vfsStream> file;
	if (!sys_fs.CheckId(fd, file))
		return CELL_ESRCH;
	
	return CELL_OK;
}

bool sdata_check(u32 version, u32 flags, u64 filesizeInput, u64 filesizeTmp)
{
	if (version > 4 || flags & 0x7EFFFFC0){
		printf("ERROR: unknown version");
		return false;
	}

	if ((version == 1 && (flags & 0x7FFFFFFE)) ||
		(version == 2 && (flags & 0x7EFFFFC0))){
		printf("ERROR: unknown or unsupported type");
		return false;
	}

	if (filesizeTmp > filesizeInput){
		printf("ERROR: input file size is too short.");
		return false;
	}

	if (!(flags & 0x80000000)){
		printf("ERROR: cannot extract finalized edata.");
		return false;
	}

	return true;
}

int sdata_unpack(const std::string& packed_file, const std::string& unpacked_file)
{
	std::shared_ptr<vfsFileBase> packed_stream(Emu.GetVFS().OpenFile(packed_file, vfsRead));
	std::shared_ptr<vfsFileBase> unpacked_stream(Emu.GetVFS().OpenFile(unpacked_file, vfsWrite));

	if (!packed_stream || !packed_stream->IsOpened())
	{
		sys_fs.Error("'%s' not found! flags: 0x%02x", packed_file.c_str(), vfsRead);
		return CELL_ENOENT;
	}

	if (!unpacked_stream || !unpacked_stream->IsOpened())
	{
		sys_fs.Error("'%s' couldn't be created! flags: 0x%02x", unpacked_file.c_str(), vfsWrite);
		return CELL_ENOENT;
	}

	char buffer[10200];
	packed_stream->Read(buffer, 256);
	u32 format = re32(*(u32*)&buffer[0]);
	if (format != 0x4E504400) // "NPD\x00"
	{
		sys_fs.Error("Illegal format. Expected 0x4E504400, but got 0x%08x", format);
		return CELL_EFSSPECIFIC;
	}

	u32 version = re32(*(u32*)&buffer[0x04]);
	u32 flags = re32(*(u32*)&buffer[0x80]);
	u32 blockSize = re32(*(u32*)&buffer[0x84]);
	u64 filesizeOutput = re64(*(u64*)&buffer[0x88]);
	u64 filesizeInput = packed_stream->GetSize();
	u32 blockCount = (u32)((filesizeOutput + blockSize - 1) / blockSize);

	// SDATA file is compressed
	if (flags & 0x1)
	{
		sys_fs.Warning("cellFsSdataOpen: Compressed SDATA files are not supported yet.");
		return CELL_EFSSPECIFIC;
	}

	// SDATA file is NOT compressed
	else
	{
		u32 t1 = (flags & 0x20) ? 0x20 : 0x10;
		u32 startOffset = (blockCount * t1) + 0x100;
		u64 filesizeTmp = (filesizeOutput + 0xF) & 0xFFFFFFF0 + startOffset;

		if (!sdata_check(version, flags, filesizeInput, filesizeTmp))
		{
			sys_fs.Error("cellFsSdataOpen: Wrong header information.");
			return CELL_EFSSPECIFIC;
		}

		if (flags & 0x20)
			packed_stream->Seek(0x100);
		else
			packed_stream->Seek(startOffset);

		for (u32 i = 0; i < blockCount; i++)
		{
			if (flags & 0x20)
				packed_stream->Seek(packed_stream->Tell() + t1);

			if (!(blockCount - i - 1))
				blockSize = (u32)(filesizeOutput - i * blockSize);

			packed_stream->Read(buffer + 256, blockSize);
			unpacked_stream->Write(buffer + 256, blockSize);
		}
	}

	return CELL_OK;
}

s32 cellFsSdataOpen(vm::ptr<const char> path, s32 flags, vm::ptr<be_t<u32>> fd, vm::ptr<const void> arg, u64 size)
{
	sys_fs.Warning("cellFsSdataOpen(path_addr=0x%x, flags=0x%x, fd=0x%x, arg=0x%x, size=0x%llx) -> cellFsOpen()", path.addr(), flags, fd, arg, size);
	sys_fs.Warning("cellFsSdataOpen(path='%s')", path.get_ptr());

	/*if (flags != CELL_O_RDONLY)
	return CELL_EINVAL;

	std::string suffix = path.substr(path.length() - 5, 5);
	if (suffix != ".sdat" && suffix != ".SDAT")
	return CELL_ENOTSDATA;

	std::string::size_type last_slash = path.rfind('/'); //TODO: use a filesystem library to solve this more robustly
	last_slash = last_slash == std::string::npos ? 0 : last_slash+1;
	std::string unpacked_path = "/dev_hdd1/"+path.substr(last_slash,path.length()-last_slash)+".unpacked";
	int ret = sdata_unpack(path, unpacked_path);
	if (ret) return ret;

	fd = sys_fs.GetNewId(Emu.GetVFS().OpenFile(unpacked_path, vfsRead), TYPE_FS_FILE);

	return CELL_OK;*/

	return cellFsOpen(path, flags, fd, arg, size);
}

s32 cellFsSdataOpenByFd(u32 mself_fd, s32 flags, vm::ptr<u32> sdata_fd, u64 offset, vm::ptr<const void> arg, u64 size)
{
	sys_fs.Todo("cellFsSdataOpenByFd(mself_fd=0x%x, flags=0x%x, sdata_fd=0x%x, offset=0x%llx, arg=0x%x, size=0x%llx)", mself_fd, flags, sdata_fd, offset, arg, size);

	// TODO:

	return CELL_OK;
}

std::atomic<s32> g_FsAioReadID(0);
std::atomic<s32> g_FsAioReadCur(0);
bool aio_init = false;

void fsAioRead(u32 fd, vm::ptr<CellFsAio> aio, int xid, vm::ptr<void(vm::ptr<CellFsAio> xaio, int error, int xid, u64 size)> func)
{
	while (g_FsAioReadCur != xid)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
		if (Emu.IsStopped())
		{
			sys_fs.Warning("fsAioRead() aborted");
			return;
		}
	}

	u32 error = CELL_OK;
	u64 res = 0;
	{
		std::shared_ptr<vfsStream> orig_file;
		if (!sys_fs.CheckId(fd, orig_file))
		{
			sys_fs.Error("Wrong fd (%s)", fd);
			Emu.Pause();
			return;
		}

		u64 nbytes = aio->size;

		vfsStream& file = *orig_file;
		const u64 old_pos = file.Tell();
		file.Seek((u64)aio->offset);

		if (nbytes != (u32)nbytes)
		{
			error = CELL_ENOMEM;
		}
		else
		{
			res = nbytes ? file.Read(aio->buf.get_ptr(), nbytes) : 0;
		}

		file.Seek(old_pos);

		sys_fs.Log("*** fsAioRead(fd=%d, offset=0x%llx, buf=0x%x, size=0x%llx, error=0x%x, res=0x%llx, xid=0x%x)",
			fd, aio->offset, aio->buf, aio->size, error, res, xid);
	}

	if (func)
	{
		Emu.GetCallbackManager().Async([func, aio, error, xid, res](PPUThread& CPU)
		{
			func(CPU, aio, error, xid, res);
		});
	}

	g_FsAioReadCur++;
}

s32 cellFsAioRead(vm::ptr<CellFsAio> aio, vm::ptr<s32> id, vm::ptr<void(vm::ptr<CellFsAio> xaio, s32 error, s32 xid, u64 size)> func)
{
	sys_fs.Warning("cellFsAioRead(aio=0x%x, id=0x%x, func=0x%x)", aio, id, func);

	if (!aio_init)
	{
		return CELL_ENXIO;
	}

	std::shared_ptr<vfsStream> orig_file;
	u32 fd = aio->fd;

	if (!sys_fs.CheckId(fd, orig_file))
	{
		return CELL_EBADF;
	}

	//get a unique id for the callback (may be used by cellFsAioCancel)
	const s32 xid = g_FsAioReadID++;
	*id = xid;

	thread_t t("CellFsAio Reading Thread", std::bind(fsAioRead, fd, aio, xid, func));
	return CELL_OK;
}

s32 cellFsAioWrite(vm::ptr<CellFsAio> aio, vm::ptr<s32> id, vm::ptr<void(vm::ptr<CellFsAio> xaio, s32 error, s32 xid, u64 size)> func)
{
	sys_fs.Todo("cellFsAioWrite(aio=0x%x, id=0x%x, func=0x%x)", aio, id, func);

	// TODO:

	return CELL_OK;
}

s32 cellFsAioInit(vm::ptr<const char> mount_point)
{
	sys_fs.Warning("cellFsAioInit(mount_point_addr=0x%x)", mount_point.addr());
	sys_fs.Warning("cellFsAioInit(mount_point='%s')", mount_point.get_ptr());

	aio_init = true;
	return CELL_OK;
}

s32 cellFsAioFinish(vm::ptr<const char> mount_point)
{
	sys_fs.Warning("cellFsAioFinish(mount_point_addr=0x%x)", mount_point.addr());
	sys_fs.Warning("cellFsAioFinish(mount_point='%s')", mount_point.get_ptr());

	//aio_init = false;
	return CELL_OK;
}

s32 cellFsReadWithOffset(PPUThread& CPU, u32 fd, u64 offset, vm::ptr<void> buf, u64 buffer_size, vm::ptr<be_t<u64>> nread)
{
	sys_fs.Warning("cellFsReadWithOffset(fd=%d, offset=0x%llx, buf=0x%x, buffer_size=%lld, nread=0x%llx)", fd, offset, buf, buffer_size, nread);

	int ret;
	vm::stackvar<be_t<u64>> oldPos(CPU), newPos(CPU);
	ret = cellFsLseek(fd, 0, CELL_SEEK_CUR, oldPos);       // Save the current position
	if (ret) return ret;
	ret = cellFsLseek(fd, offset, CELL_SEEK_SET, newPos);  // Move to the specified offset
	if (ret) return ret;
	ret = cellFsRead(fd, buf, buffer_size, nread);    // Read the file
	if (ret) return ret;
	ret = cellFsLseek(fd, oldPos.value(), CELL_SEEK_SET, newPos);  // Return to the old position
	if (ret) return ret;

	return CELL_OK;
}

s32 cellFsSetDefaultContainer(u32 id, u32 total_limit)
{
	sys_fs.Todo("cellFsSetDefaultContainer(id=%d, total_limit=%d)", id, total_limit);

	return CELL_OK;
}

s32 cellFsSetIoBufferFromDefaultContainer(u32 fd, u32 buffer_size, u32 page_type)
{
	sys_fs.Todo("cellFsSetIoBufferFromDefaultContainer(fd=%d, buffer_size=%d, page_type=%d)", fd, buffer_size, page_type);

	std::shared_ptr<vfsStream> file;
	if (!sys_fs.CheckId(fd, file))
		return CELL_ESRCH;

	return CELL_OK;
}

Module sys_fs("sys_fs", []()
{
	g_FsAioReadID = 0;
	g_FsAioReadCur = 0;
	aio_init = false;

	REG_FUNC(sys_fs, cellFsOpen);
	REG_FUNC(sys_fs, cellFsSdataOpen);
	REG_FUNC(sys_fs, cellFsSdataOpenByFd);
	REG_FUNC(sys_fs, cellFsRead);
	REG_FUNC(sys_fs, cellFsWrite);
	REG_FUNC(sys_fs, cellFsClose);
	REG_FUNC(sys_fs, cellFsOpendir);
	REG_FUNC(sys_fs, cellFsReaddir);
	REG_FUNC(sys_fs, cellFsClosedir);
	REG_FUNC(sys_fs, cellFsStat);
	REG_FUNC(sys_fs, cellFsFstat);
	REG_FUNC(sys_fs, cellFsMkdir);
	REG_FUNC(sys_fs, cellFsRename);
	REG_FUNC(sys_fs, cellFsChmod);
	REG_FUNC(sys_fs, cellFsFsync);
	REG_FUNC(sys_fs, cellFsRmdir);
	REG_FUNC(sys_fs, cellFsUnlink);
	REG_FUNC(sys_fs, cellFsLseek);
	REG_FUNC(sys_fs, cellFsFtruncate);
	REG_FUNC(sys_fs, cellFsTruncate);
	REG_FUNC(sys_fs, cellFsFGetBlockSize);
	REG_FUNC(sys_fs, cellFsAioRead);
	REG_FUNC(sys_fs, cellFsAioWrite);
	REG_FUNC(sys_fs, cellFsAioInit);
	REG_FUNC(sys_fs, cellFsAioFinish);
	REG_FUNC(sys_fs, cellFsGetBlockSize);
	REG_FUNC(sys_fs, cellFsGetFreeSize);
	REG_FUNC(sys_fs, cellFsReadWithOffset);
	REG_FUNC(sys_fs, cellFsGetDirectoryEntries);
	REG_FUNC(sys_fs, cellFsStReadInit);
	REG_FUNC(sys_fs, cellFsStReadFinish);
	REG_FUNC(sys_fs, cellFsStReadGetRingBuf);
	REG_FUNC(sys_fs, cellFsStReadGetStatus);
	REG_FUNC(sys_fs, cellFsStReadGetRegid);
	REG_FUNC(sys_fs, cellFsStReadStart);
	REG_FUNC(sys_fs, cellFsStReadStop);
	REG_FUNC(sys_fs, cellFsStRead);
	REG_FUNC(sys_fs, cellFsStReadGetCurrentAddr);
	REG_FUNC(sys_fs, cellFsStReadPutCurrentAddr);
	REG_FUNC(sys_fs, cellFsStReadWait);
	REG_FUNC(sys_fs, cellFsStReadWaitCallback);
	REG_FUNC(sys_fs, cellFsSetDefaultContainer);
	REG_FUNC(sys_fs, cellFsSetIoBufferFromDefaultContainer);
});
