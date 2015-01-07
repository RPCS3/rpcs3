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

Module *sys_fs = nullptr;

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

	std::shared_ptr<vfsStream> stream((vfsStream*)Emu.GetVFS().OpenFile(_path, o_mode));

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

	std::shared_ptr<vfsStream> file;
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

	std::shared_ptr<vfsStream> file;
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

	if (!Emu.GetIdManager().RemoveID(fd))
		return CELL_ESRCH;

	return CELL_OK;
}

s32 cellFsOpendir(vm::ptr<const char> path, vm::ptr<u32> fd)
{
	sys_fs->Warning("cellFsOpendir(path=\"%s\", fd_addr=0x%x)", path.get_ptr(), fd.addr());
	
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

	if (!Emu.GetIdManager().RemoveID(fd))
		return CELL_ESRCH;

	return CELL_OK;
}

s32 cellFsStat(vm::ptr<const char> path, vm::ptr<CellFsStat> sb)
{
	sys_fs->Warning("cellFsStat(path=\"%s\", sb_addr=0x%x)", path.get_ptr(), sb.addr());

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

	struct stat buf;

	if (int result = stat(real_path.c_str(), &buf))
	{
		sys_fs->Error("stat('%s') failed -> 0x%x", real_path.c_str(), result);
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
		sys_fs->Error("Mode is the same. Report this to a RPCS3 developer! (%d)", mode);

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
		sys_fs->Error("Size is the same. Report this to a RPCS3 developer! (%d)", size);

	sys_fs->Warning("cellFsStat: \"%s\" not found.", path.get_ptr());
	return CELL_ENOENT;
}

s32 cellFsFstat(u32 fd, vm::ptr<CellFsStat> sb)
{
	sys_fs->Warning("cellFsFstat(fd=%d, sb_addr=0x%x)", fd, sb.addr());

	IDType type;
	std::shared_ptr<vfsStream> file;
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

	// TODO:

	return CELL_OK;
}

s32 cellFsFsync(u32 fd)
{
	sys_fs->Todo("cellFsFsync(fd=0x%x)", fd);

	// TODO:

	return CELL_OK;
}

s32 cellFsRmdir(vm::ptr<const char> path)
{
	sys_fs->Warning("cellFsRmdir(path=\"%s\")", path.get_ptr());

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
	std::shared_ptr<vfsStream> file;
	if (!sys_fs->CheckId(fd, file, type) || type != TYPE_FS_FILE)
		return CELL_ESRCH;

	*pos = file->Seek(offset, seek_mode);
	return CELL_OK;
}

s32 cellFsFtruncate(u32 fd, u64 size)
{
	sys_fs->Warning("cellFsFtruncate(fd=%d, size=%lld)", fd, size);
	
	IDType type;
	std::shared_ptr<vfsStream> file;
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

	std::shared_ptr<vfsStream> file;
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

	*sector_size = 4096; // ?
	*block_size = 4096; // ?

	return CELL_OK;
}

s32 cellFsGetFreeSize(vm::ptr<const char> path, vm::ptr<u32> block_size, vm::ptr<u64> block_count)
{
	sys_fs->Warning("cellFsGetFreeSize(path=\"%s\", block_size_addr=0x%x, block_count_addr=0x%x)",
		path.get_ptr(), block_size.addr(), block_count.addr());

	// TODO: Get real values. Currently, it always returns 40 GB of free space divided in 4 KB blocks
	*block_size = 4096; // ?
	*block_count = 10 * 1024 * 1024; // ?

	return CELL_OK;
}

s32 cellFsGetDirectoryEntries(u32 fd, vm::ptr<CellFsDirectoryEntry> entries, u32 entries_size, vm::ptr<u32> data_count)
{
	sys_fs->Warning("cellFsGetDirectoryEntries(fd=%d, entries_addr=0x%x, entries_size=0x%x, data_count_addr=0x%x)",
		fd, entries.addr(), entries_size, data_count.addr());

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

	std::shared_ptr<vfsStream> file;
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

	std::shared_ptr<vfsStream> file;
	if (!sys_fs->CheckId(fd, file))
		return CELL_ESRCH;

	Memory.Free(fs_config.m_buffer);
	fs_config.m_fs_status = CELL_FS_ST_NOT_INITIALIZED;

	return CELL_OK;
}

s32 cellFsStReadGetRingBuf(u32 fd, vm::ptr<CellFsRingBuffer> ringbuf)
{
	sys_fs->Warning("cellFsStReadGetRingBuf(fd=%d, ringbuf_addr=0x%x)", fd, ringbuf.addr());

	std::shared_ptr<vfsStream> file;
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

	std::shared_ptr<vfsStream> file;
	if (!sys_fs->CheckId(fd, file))
		return CELL_ESRCH;

	*status = fs_config.m_fs_status;

	return CELL_OK;
}

s32 cellFsStReadGetRegid(u32 fd, vm::ptr<u64> regid)
{
	sys_fs->Warning("cellFsStReadGetRingBuf(fd=%d, regid_addr=0x%x)", fd, regid.addr());

	std::shared_ptr<vfsStream> file;
	if (!sys_fs->CheckId(fd, file))
		return CELL_ESRCH;

	*regid = fs_config.m_regid;

	return CELL_OK;
}

s32 cellFsStReadStart(u32 fd, u64 offset, u64 size)
{
	sys_fs->Todo("cellFsStReadStart(fd=%d, offset=0x%llx, size=0x%llx)", fd, offset, size);

	std::shared_ptr<vfsStream> file;
	if (!sys_fs->CheckId(fd, file))
		return CELL_ESRCH;

	fs_config.m_current_addr = fs_config.m_buffer + (u32)offset;
	fs_config.m_fs_status = CELL_FS_ST_PROGRESS;

	return CELL_OK;
}

s32 cellFsStReadStop(u32 fd)
{
	sys_fs->Warning("cellFsStReadStop(fd=%d)", fd);

	std::shared_ptr<vfsStream> file;
	if (!sys_fs->CheckId(fd, file))
		return CELL_ESRCH;

	fs_config.m_fs_status = CELL_FS_ST_STOP;

	return CELL_OK;
}

s32 cellFsStRead(u32 fd, u32 buf_addr, u64 size, vm::ptr<u64> rsize)
{
	sys_fs->Warning("cellFsStRead(fd=%d, buf_addr=0x%x, size=0x%llx, rsize_addr=0x%x)", fd, buf_addr, size, rsize.addr());
	
	std::shared_ptr<vfsStream> file;
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

	std::shared_ptr<vfsStream> file;
	if (!sys_fs->CheckId(fd, file))
		return CELL_ESRCH;

	return CELL_OK;
}

s32 cellFsStReadPutCurrentAddr(u32 fd, u32 addr_addr, u64 size)
{
	sys_fs->Todo("cellFsStReadPutCurrentAddr(fd=%d, addr_addr=0x%x, size=0x%llx)", fd, addr_addr, size);
	
	std::shared_ptr<vfsStream> file;
	if (!sys_fs->CheckId(fd, file))
		return CELL_ESRCH;

	return CELL_OK;
}

s32 cellFsStReadWait(u32 fd, u64 size)
{
	sys_fs->Todo("cellFsStReadWait(fd=%d, size=0x%llx)", fd, size);
	
	std::shared_ptr<vfsStream> file;
	if (!sys_fs->CheckId(fd, file))
		return CELL_ESRCH;
	
	return CELL_OK;
}

s32 cellFsStReadWaitCallback(u32 fd, u64 size, vm::ptr<void (*)(int xfd, u64 xsize)> func)
{
	sys_fs->Todo("cellFsStReadWaitCallback(fd=%d, size=0x%llx, func_addr=0x%x)", fd, size, func.addr());

	std::shared_ptr<vfsStream> file;
	if (!sys_fs->CheckId(fd, file))
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
		sys_fs->Error("'%s' not found! flags: 0x%08x", packed_file.c_str(), vfsRead);
		return CELL_ENOENT;
	}

	if (!unpacked_stream || !unpacked_stream->IsOpened())
	{
		sys_fs->Error("'%s' couldn't be created! flags: 0x%08x", unpacked_file.c_str(), vfsWrite);
		return CELL_ENOENT;
	}

	char buffer[10200];
	packed_stream->Read(buffer, 256);
	u32 format = re32(*(u32*)&buffer[0]);
	if (format != 0x4E504400) // "NPD\x00"
	{
		sys_fs->Error("Illegal format. Expected 0x4E504400, but got 0x%08x", format);
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
		sys_fs->Warning("cellFsSdataOpen: Compressed SDATA files are not supported yet.");
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
			sys_fs->Error("cellFsSdataOpen: Wrong header information.");
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

int cellFsSdataOpen(vm::ptr<const char> path, int flags, vm::ptr<be_t<u32>> fd, vm::ptr<u32> arg, u64 size)
{
	sys_fs->Warning("cellFsSdataOpen(path=\"%s\", flags=0x%x, fd_addr=0x%x, arg_addr=0x%x, size=0x%llx) -> cellFsOpen()",
		path.get_ptr(), flags, fd.addr(), arg.addr(), size);

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

	fd = sys_fs->GetNewId(Emu.GetVFS().OpenFile(unpacked_path, vfsRead), TYPE_FS_FILE);

	return CELL_OK;*/

	return cellFsOpen(path, flags, fd, arg, size);
}

int cellFsSdataOpenByFd(int mself_fd, int flags, vm::ptr<u32> sdata_fd, u64 offset, vm::ptr<u32> arg, u64 size)
{
	sys_fs->Todo("cellFsSdataOpenByFd(mself_fd=0x%x, flags=0x%x, sdata_fd_addr=0x%x, offset=0x%llx, arg_addr=0x%x, size=0x%llx) -> cellFsOpen()",
		mself_fd, flags, sdata_fd.addr(), offset, arg.addr(), size);

	// TODO:

	return CELL_OK;
}

std::atomic<u32> g_FsAioReadID(0);
std::atomic<u32> g_FsAioReadCur(0);
bool aio_init = false;

void fsAioRead(u32 fd, vm::ptr<CellFsAio> aio, int xid, vm::ptr<void(*)(vm::ptr<CellFsAio> xaio, int error, int xid, u64 size)> func)
{
	while (g_FsAioReadCur != xid)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
		if (Emu.IsStopped())
		{
			sys_fs->Warning("fsAioRead() aborted");
			return;
		}
	}

	u32 error = CELL_OK;
	u64 res = 0;
	{
		std::shared_ptr<vfsStream> orig_file;
		if (!sys_fs->CheckId(fd, orig_file))
		{
			sys_fs->Error("Wrong fd (%s)", fd);
			Emu.Pause();
			return;
		}

		u64 nbytes = aio->size;

		vfsStream& file = *orig_file;
		const u64 old_pos = file.Tell();
		file.Seek((u64)aio->offset);

		// TODO: use code from cellFsRead or something
		if (nbytes != (u32)nbytes)
		{
			error = CELL_ENOMEM;
		}
		else
		{
			res = nbytes ? file.Read(aio->buf.get_ptr(), nbytes) : 0;
		}

		file.Seek(old_pos);

		sys_fs->Log("*** fsAioRead(fd=%d, offset=0x%llx, buf_addr=0x%x, size=0x%x, error=0x%x, res=0x%x, xid=0x%x)",
			fd, (u64)aio->offset, aio->buf.addr(), (u64)aio->size, error, res, xid);
	}

	if (func) // start callback thread
	{
		Emu.GetCallbackManager().Async([func, aio, error, xid, res]()
		{
			func(aio, error, xid, res);
		});
	}

	g_FsAioReadCur++;
}

int cellFsAioRead(vm::ptr<CellFsAio> aio, vm::ptr<u32> aio_id, vm::ptr<void(*)(vm::ptr<CellFsAio> xaio, int error, int xid, u64 size)> func)
{
	sys_fs->Warning("cellFsAioRead(aio_addr=0x%x, id_addr=0x%x, func_addr=0x%x)", aio.addr(), aio_id.addr(), func.addr());

	if (!aio_init)
	{
		return CELL_ENXIO;
	}

	std::shared_ptr<vfsStream> orig_file;
	u32 fd = aio->fd;

	if (!sys_fs->CheckId(fd, orig_file))
	{
		return CELL_EBADF;
	}

	//get a unique id for the callback (may be used by cellFsAioCancel)
	const u32 xid = g_FsAioReadID++;
	*aio_id = xid;

	{
		thread t("fsAioRead", std::bind(fsAioRead, fd, aio, xid, func));
		t.detach();
	}

	return CELL_OK;
}

int cellFsAioWrite(vm::ptr<CellFsAio> aio, vm::ptr<u32> aio_id, vm::ptr<void(*)(vm::ptr<CellFsAio> xaio, int error, int xid, u64 size)> func)
{
	sys_fs->Todo("cellFsAioWrite(aio_addr=0x%x, id_addr=0x%x, func_addr=0x%x)", aio.addr(), aio_id.addr(), func.addr());

	// TODO:

	return CELL_OK;
}

int cellFsAioInit(vm::ptr<const char> mount_point)
{
	sys_fs->Warning("cellFsAioInit(mount_point_addr=0x%x (%s))", mount_point.addr(), mount_point.get_ptr());

	aio_init = true;
	return CELL_OK;
}

int cellFsAioFinish(vm::ptr<const char> mount_point)
{
	sys_fs->Warning("cellFsAioFinish(mount_point_addr=0x%x (%s))", mount_point.addr(), mount_point.get_ptr());

	aio_init = false;
	return CELL_OK;
}

int cellFsReadWithOffset(PPUThread& CPU, u32 fd, u64 offset, vm::ptr<void> buf, u64 buffer_size, vm::ptr<be_t<u64>> nread)
{
	sys_fs->Warning("cellFsReadWithOffset(fd=%d, offset=0x%llx, buf_addr=0x%x, buffer_size=%lld nread=0x%llx)",
		fd, offset, buf.addr(), buffer_size, nread.addr());

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

void sys_fs_init(Module *pxThis)
{
	sys_fs = pxThis;

	sys_fs->AddFunc(0x718bf5f8, cellFsOpen);
	sys_fs->AddFunc(0xb1840b53, cellFsSdataOpen);
	sys_fs->AddFunc(0x6d3bb15b, cellFsSdataOpenByFd);
	sys_fs->AddFunc(0x4d5ff8e2, cellFsRead);
	sys_fs->AddFunc(0xecdcf2ab, cellFsWrite);
	sys_fs->AddFunc(0x2cb51f0d, cellFsClose);
	sys_fs->AddFunc(0x3f61245c, cellFsOpendir);
	sys_fs->AddFunc(0x5c74903d, cellFsReaddir);
	sys_fs->AddFunc(0xff42dcc3, cellFsClosedir);
	sys_fs->AddFunc(0x7de6dced, cellFsStat);
	sys_fs->AddFunc(0xef3efa34, cellFsFstat);
	sys_fs->AddFunc(0xba901fe6, cellFsMkdir);
	sys_fs->AddFunc(0xf12eecc8, cellFsRename);
	sys_fs->AddFunc(0x99406d0b, cellFsChmod);
	sys_fs->AddFunc(0x967a162b, cellFsFsync);
	sys_fs->AddFunc(0x2796fdf3, cellFsRmdir);
	sys_fs->AddFunc(0x7f4677a8, cellFsUnlink);
	sys_fs->AddFunc(0xa397d042, cellFsLseek);
	sys_fs->AddFunc(0x0e2939e5, cellFsFtruncate);
	sys_fs->AddFunc(0xc9dc3ac5, cellFsTruncate);
	sys_fs->AddFunc(0xcb588dba, cellFsFGetBlockSize);
	sys_fs->AddFunc(0xc1c507e7, cellFsAioRead);
	sys_fs->AddFunc(0x4cef342e, cellFsAioWrite);
	sys_fs->AddFunc(0xdb869f20, cellFsAioInit);
	sys_fs->AddFunc(0x9f951810, cellFsAioFinish);
	sys_fs->AddFunc(0x1a108ab7, cellFsGetBlockSize);
	sys_fs->AddFunc(0xaa3b4bcd, cellFsGetFreeSize);
	sys_fs->AddFunc(0x0d5b4a14, cellFsReadWithOffset);
	sys_fs->AddFunc(0x9b882495, cellFsGetDirectoryEntries);
	sys_fs->AddFunc(0x2664c8ae, cellFsStReadInit);
	sys_fs->AddFunc(0xd73938df, cellFsStReadFinish);
	sys_fs->AddFunc(0xb3afee8b, cellFsStReadGetRingBuf);
	sys_fs->AddFunc(0xcf34969c, cellFsStReadGetStatus);
	sys_fs->AddFunc(0xbd273a88, cellFsStReadGetRegid);
	sys_fs->AddFunc(0x8df28ff9, cellFsStReadStart);
	sys_fs->AddFunc(0xf8e5d9a0, cellFsStReadStop);
	sys_fs->AddFunc(0x27800c6b, cellFsStRead);
	sys_fs->AddFunc(0x190912f6, cellFsStReadGetCurrentAddr);
	sys_fs->AddFunc(0x81f33783, cellFsStReadPutCurrentAddr);
	sys_fs->AddFunc(0x8f71c5b2, cellFsStReadWait);
	sys_fs->AddFunc(0x866f6aec, cellFsStReadWaitCallback);
}

void sys_fs_load()
{
	g_FsAioReadID = 0;
	g_FsAioReadCur = 0;
	aio_init = false;
}
