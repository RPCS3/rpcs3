#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/SysCalls.h"

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

#include "sys_fs.h"

SysCallBase sys_fs("sys_fs");

s32 sys_fs_open(vm::ptr<const char> path, s32 flags, vm::ptr<u32> fd, u32 mode, vm::ptr<const void> arg, u64 size)
{
	sys_fs.Warning("sys_fs_open(path=*0x%x, flags=%d, fd=*0x%x, arg=*0x%x, size=0x%llx)", path, flags, fd, arg, size);
	sys_fs.Warning("*** path = '%s'", path.get_ptr());

	std::shared_ptr<vfsStream> file;

	if (mode)
	{
		sys_fs.Error("sys_fs_open(): unknown mode (0x%x)", mode);
		return CELL_FS_EINVAL;
	}

	// TODO: other checks for path

	if (Emu.GetVFS().ExistsDir(path.get_ptr()))
	{
		sys_fs.Error("sys_fs_open(): '%s' is a directory", path.get_ptr());
		return CELL_FS_EISDIR;
	}

	switch (flags)
	{
	case CELL_FS_O_RDONLY:
	{
		file.reset(Emu.GetVFS().OpenFile(path.get_ptr(), vfsRead));
		break;
	}
	//case CELL_FS_O_WRONLY:
	//case CELL_FS_O_WRONLY | CELL_FS_O_CREAT:

	case CELL_FS_O_WRONLY | CELL_FS_O_CREAT | CELL_FS_O_EXCL:
	{
		file.reset(Emu.GetVFS().OpenFile(path.get_ptr(), vfsWriteExcl));

		if ((!file || !file->IsOpened()) && Emu.GetVFS().ExistsFile(path.get_ptr()))
		{
			return CELL_FS_EEXIST;
		}

		break;
	}
	case CELL_FS_O_WRONLY | CELL_FS_O_CREAT | CELL_FS_O_TRUNC:
	{
		Emu.GetVFS().CreateFile(path.get_ptr());
		file.reset(Emu.GetVFS().OpenFile(path.get_ptr(), vfsWrite));
		break;
	}

	case CELL_FS_O_WRONLY | CELL_FS_O_CREAT | CELL_FS_O_APPEND:
	{
		Emu.GetVFS().CreateFile(path.get_ptr());
		file.reset(Emu.GetVFS().OpenFile(path.get_ptr(), vfsWriteAppend));
		break;
	}
	case CELL_FS_O_RDWR:
	{
		file.reset(Emu.GetVFS().OpenFile(path.get_ptr(), vfsReadWrite));
		break;
	}
	case CELL_FS_O_RDWR | CELL_FS_O_CREAT:
	{
		Emu.GetVFS().CreateFile(path.get_ptr());
		file.reset(Emu.GetVFS().OpenFile(path.get_ptr(), vfsReadWrite));
		break;
	}
	//case CELL_FS_O_RDWR | CELL_FS_O_CREAT | CELL_FS_O_EXCL:
	//case CELL_FS_O_RDWR | CELL_FS_O_CREAT | CELL_FS_O_TRUNC:

	default:
	{
		sys_fs.Error("sys_fs_open(): invalid or unimplemented flags (%d)", flags);
		return CELL_FS_EINVAL;
	}
	}

	if (!file || !file->IsOpened())
	{
		sys_fs.Error("sys_fs_open(): failed to open '%s' (flags=%d, mode=0x%x)", path.get_ptr(), flags, mode);
		return CELL_FS_ENOENT;
	}

	*fd = Emu.GetIdManager().GetNewID(file, TYPE_FS_FILE);

	return CELL_OK;
}

s32 sys_fs_read(u32 fd, vm::ptr<void> buf, u64 nbytes, vm::ptr<u64> nread)
{
	sys_fs.Log("sys_fs_read(fd=0x%x, buf=0x%x, nbytes=0x%llx, nread=0x%x)", fd, buf, nbytes, nread);

	std::shared_ptr<vfsStream> file;

	if (!Emu.GetIdManager().GetIDData(fd, file))
	{
		return CELL_FS_EBADF; // TODO: return if not opened for reading
	}

	*nread = file->Read(buf.get_ptr(), nbytes);

	return CELL_OK;
}

s32 sys_fs_write(u32 fd, vm::ptr<const void> buf, u64 nbytes, vm::ptr<u64> nwrite)
{
	sys_fs.Log("sys_fs_write(fd=0x%x, buf=*0x%x, nbytes=0x%llx, nwrite=*0x%x)", fd, buf, nbytes, nwrite);

	std::shared_ptr<vfsStream> file;

	if (!Emu.GetIdManager().GetIDData(fd, file))
	{
		return CELL_FS_EBADF; // TODO: return if not opened for writing
	}

	*nwrite = file->Write(buf.get_ptr(), nbytes);

	return CELL_OK;
}

s32 sys_fs_close(u32 fd)
{
	sys_fs.Log("sys_fs_close(fd=0x%x)", fd);

	std::shared_ptr<vfsStream> file;

	if (!Emu.GetIdManager().GetIDData(fd, file))
	{
		return CELL_FS_EBADF;
	}

	if (false)
	{
		return CELL_FS_EBUSY; // TODO: return if locked
	}

	Emu.GetIdManager().RemoveID<vfsStream>(fd);

	return CELL_OK;
}

s32 sys_fs_opendir(vm::ptr<const char> path, vm::ptr<u32> fd)
{
	sys_fs.Warning("sys_fs_opendir(path=*0x%x, fd=*0x%x)", path, fd);
	sys_fs.Warning("*** path = '%s'", path.get_ptr());

	std::shared_ptr<vfsDirBase> directory(Emu.GetVFS().OpenDir(path.get_ptr()));

	if (!directory || !directory->IsOpened())
	{
		return CELL_FS_ENOENT;
	}

	*fd = Emu.GetIdManager().GetNewID(directory, TYPE_FS_DIR);

	return CELL_OK;
}

s32 sys_fs_readdir(u32 fd, vm::ptr<CellFsDirent> dir, vm::ptr<u64> nread)
{
	sys_fs.Warning("sys_fs_readdir(fd=0x%x, dir=*0x%x, nread=*0x%x)", fd, dir, nread);

	std::shared_ptr<vfsDirBase> directory;

	if (!Emu.GetIdManager().GetIDData(fd, directory))
	{
		return CELL_ESRCH;
	}

	const DirEntryInfo* info = directory->Read();

	if (info)
	{
		dir->d_type = (info->flags & DirEntry_TypeFile) ? CELL_FS_TYPE_REGULAR : CELL_FS_TYPE_DIRECTORY;
		dir->d_namlen = u8(std::min<size_t>(info->name.length(), CELL_FS_MAX_FS_FILE_NAME_LENGTH));
		strcpy_trunc(dir->d_name, info->name);
		*nread = sizeof(CellFsDirent);
	}
	else
	{
		*nread = 0;
	}

	return CELL_OK;
}

s32 sys_fs_closedir(u32 fd)
{
	sys_fs.Log("sys_fs_closedir(fd=0x%x)", fd);

	std::shared_ptr<vfsDirBase> directory;

	if (!Emu.GetIdManager().GetIDData(fd, directory))
	{
		return CELL_ESRCH;
	}

	Emu.GetIdManager().RemoveID<vfsDirBase>(fd);

	return CELL_OK;
}

s32 sys_fs_stat(vm::ptr<const char> path, vm::ptr<CellFsStat> sb)
{
	sys_fs.Warning("sys_fs_stat(path=*0x%x, sb=*0x%x)", path, sb);
	sys_fs.Warning("*** path = '%s'", path.get_ptr());

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
		sys_fs.Error("sys_fs_stat(): stat('%s') failed -> 0x%x", real_path.c_str(), stat_result);
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

	sb->mode =
		CELL_FS_S_IRUSR | CELL_FS_S_IWUSR | CELL_FS_S_IXUSR |
		CELL_FS_S_IRGRP | CELL_FS_S_IWGRP | CELL_FS_S_IXGRP |
		CELL_FS_S_IROTH | CELL_FS_S_IWOTH | CELL_FS_S_IXOTH;

	if (sb->mode == mode)
		sys_fs.Error("sys_fs_stat(): mode is the same (0x%x)", mode);

	sb->uid = uid;
	sb->gid = gid;
	sb->atime = atime;
	sb->mtime = mtime;
	sb->ctime = ctime;
	sb->blksize = 4096;

	{
		vfsDir dir(_path);
		if (dir.IsOpened())
		{
			sb->mode |= CELL_FS_S_IFDIR;
			return CELL_OK;
		}
	}

	{
		vfsFile f(_path);
		if (f.IsOpened())
		{
			sb->mode |= CELL_FS_S_IFREG;
			sb->size = f.GetSize();
			return CELL_OK;
		}
	}

	if (sb->size == size && size != 0)
		sys_fs.Error("sys_fs_stat(): size is the same (0x%x)", size);

	sys_fs.Warning("sys_fs_stat(): '%s' not found", path.get_ptr());
	return CELL_ENOENT;
}

s32 sys_fs_fstat(u32 fd, vm::ptr<CellFsStat> sb)
{
	sys_fs.Warning("sys_fs_fstat(fd=0x%x, sb=*0x%x)", fd, sb);

	std::shared_ptr<vfsStream> file;
	if (!Emu.GetIdManager().GetIDData(fd, file))
		return CELL_ESRCH;

	sb->mode =
		CELL_FS_S_IRUSR | CELL_FS_S_IWUSR | CELL_FS_S_IXUSR |
		CELL_FS_S_IRGRP | CELL_FS_S_IWGRP | CELL_FS_S_IXGRP |
		CELL_FS_S_IROTH | CELL_FS_S_IWOTH | CELL_FS_S_IXOTH;

	sb->mode |= CELL_FS_S_IFREG; //TODO: dir CELL_FS_S_IFDIR
	sb->uid = 0;
	sb->gid = 0;
	sb->atime = 0; //TODO
	sb->mtime = 0; //TODO
	sb->ctime = 0; //TODO
	sb->size = file->GetSize();
	sb->blksize = 4096;

	return CELL_OK;
}

s32 sys_fs_mkdir(vm::ptr<const char> path, CellFsMode mode)
{
	sys_fs.Warning("sys_fs_mkdir(path=*0x%x, mode=%d)", path, mode);
	sys_fs.Warning("*** path = '%s'", path.get_ptr());

	const std::string _path = path.get_ptr();

	if (vfsDir().IsExists(_path))
		return CELL_EEXIST;

	if (!Emu.GetVFS().CreateDir(_path))
		return CELL_EBUSY;

	sys_fs.Notice("sys_fs_mkdir(): directory '%s' created", path.get_ptr());
	return CELL_OK;
}

s32 sys_fs_rename(vm::ptr<const char> from, vm::ptr<const char> to)
{
	sys_fs.Warning("sys_fs_rename(from=*0x%x, to=*0x%x)", from, to);
	sys_fs.Warning("*** from = '%s'", from.get_ptr());
	sys_fs.Warning("*** to   = '%s'", to.get_ptr());

	std::string _from = from.get_ptr();
	std::string _to = to.get_ptr();

	{
		vfsDir dir;
		if (dir.IsExists(_from))
		{
			if (!dir.Rename(_from, _to))
				return CELL_EBUSY;

			sys_fs.Notice("sys_fs_rename(): directory '%s' renamed to '%s'", from.get_ptr(), to.get_ptr());
			return CELL_OK;
		}
	}

	{
		vfsFile f;

		if (f.Exists(_from))
		{
			if (!f.Rename(_from, _to))
				return CELL_EBUSY;

			sys_fs.Notice("sys_fs_rename(): file '%s' renamed to '%s'", from.get_ptr(), to.get_ptr());
			return CELL_OK;
		}
	}

	return CELL_ENOENT;
}

s32 sys_fs_rmdir(vm::ptr<const char> path)
{
	sys_fs.Warning("sys_fs_rmdir(path=*0x%x)", path);
	sys_fs.Warning("*** path = '%s'", path.get_ptr());

	std::string _path = path.get_ptr();

	vfsDir d;
	if (!d.IsExists(_path))
		return CELL_ENOENT;

	if (!d.Remove(_path))
		return CELL_EBUSY;

	sys_fs.Notice("sys_fs_rmdir(): directory '%s' removed", path.get_ptr());
	return CELL_OK;
}

s32 sys_fs_unlink(vm::ptr<const char> path)
{
	sys_fs.Warning("sys_fs_unlink(path=*0x%x)", path);
	sys_fs.Warning("*** path = '%s'", path.get_ptr());

	std::string _path = path.get_ptr();

	if (vfsDir().IsExists(_path))
		return CELL_EISDIR;

	if (!Emu.GetVFS().ExistsFile(_path))
		return CELL_ENOENT;

	if (!Emu.GetVFS().RemoveFile(_path))
		return CELL_EACCES;

	sys_fs.Notice("sys_fs_unlink(): file '%s' deleted", path.get_ptr());
	return CELL_OK;
}

s32 sys_fs_lseek(u32 fd, s64 offset, s32 whence, vm::ptr<u64> pos)
{
	sys_fs.Log("sys_fs_lseek(fd=0x%x, offset=0x%llx, whence=0x%x, pos=*0x%x)", fd, offset, whence, pos);

	vfsSeekMode seek_mode;
	switch (whence)
	{
	case CELL_FS_SEEK_SET: seek_mode = vfsSeekSet; break;
	case CELL_FS_SEEK_CUR: seek_mode = vfsSeekCur; break;
	case CELL_FS_SEEK_END: seek_mode = vfsSeekEnd; break;
	default:
		sys_fs.Error("sys_fs_lseek(fd=0x%x): unknown seek whence (0x%x)", fd, whence);
		return CELL_EINVAL;
	}

	std::shared_ptr<vfsStream> file;
	if (!Emu.GetIdManager().GetIDData(fd, file))
		return CELL_ESRCH;

	*pos = file->Seek(offset, seek_mode);
	return CELL_OK;
}

s32 sys_fs_fget_block_size(u32 fd, vm::ptr<u64> sector_size, vm::ptr<u64> block_size, vm::ptr<u64> arg4, vm::ptr<u64> arg5)
{
	sys_fs.Todo("sys_fs_fget_block_size(fd=%d, sector_size=*0x%x, block_size=*0x%x, arg4=*0x%x, arg5=*0x%x)", fd, sector_size, block_size, arg4, arg5);

	std::shared_ptr<vfsStream> file;
	if (!Emu.GetIdManager().GetIDData(fd, file))
		return CELL_ESRCH;

	*sector_size = 4096; // ?
	*block_size = 4096; // ?

	return CELL_OK;
}

s32 sys_fs_get_block_size(vm::ptr<const char> path, vm::ptr<u64> sector_size, vm::ptr<u64> block_size, vm::ptr<u64> arg4)
{
	sys_fs.Todo("sys_fs_get_block_size(path=*0x%x, sector_size=*0x%x, block_size=*0x%x, arg4=*0x%x, arg5=*0x%x)", path, sector_size, block_size, arg4);
	sys_fs.Todo("*** path = '%s'", path.get_ptr());

	*sector_size = 4096; // ?
	*block_size = 4096; // ?

	return CELL_OK;
}

s32 sys_fs_truncate(vm::ptr<const char> path, u64 size)
{
	sys_fs.Warning("sys_fs_truncate(path=*0x%x, size=0x%llx)", path, size);
	sys_fs.Warning("*** path = '%s'", path.get_ptr());

	vfsFile f(path.get_ptr(), vfsReadWrite);
	if (!f.IsOpened())
	{
		sys_fs.Warning("sys_fs_truncate(): '%s' not found", path.get_ptr());
		return CELL_ENOENT;
	}
	u64 initialSize = f.GetSize();

	if (initialSize < size)
	{
		u64 last_pos = f.Tell();
		f.Seek(0, vfsSeekEnd);
		static const char nullbyte = 0;
		f.Seek(size - initialSize - 1, vfsSeekCur);
		f.Write(&nullbyte, sizeof(char));
		f.Seek(last_pos, vfsSeekSet);
	}

	if (initialSize > size)
	{
		// (TODO)
	}

	return CELL_OK;
}

s32 sys_fs_ftruncate(u32 fd, u64 size)
{
	sys_fs.Warning("sys_fs_ftruncate(fd=0x%x, size=0x%llx)", fd, size);

	std::shared_ptr<vfsStream> file;
	if (!Emu.GetIdManager().GetIDData(fd, file))
		return CELL_ESRCH;

	u64 initialSize = file->GetSize();

	if (initialSize < size)
	{
		u64 last_pos = file->Tell();
		file->Seek(0, vfsSeekEnd);
		static const char nullbyte = 0;
		file->Seek(size - initialSize - 1, vfsSeekCur);
		file->Write(&nullbyte, sizeof(char));
		file->Seek(last_pos, vfsSeekSet);
	}

	if (initialSize > size)
	{
		// (TODO)
	}

	return CELL_OK;
}

s32 sys_fs_chmod(vm::ptr<const char> path, CellFsMode mode)
{
	sys_fs.Todo("sys_fs_chmod(path=*0x%x, mode=%d) -> CELL_OK", path, mode);
	sys_fs.Todo("*** path = '%s'", path.get_ptr());

	return CELL_OK;
}
