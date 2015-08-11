#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/SysCalls.h"

#include "Emu/FS/VFS.h"
#include "Emu/FS/vfsFile.h"
#include "Emu/FS/vfsLocalFile.h"
#include "Emu/FS/vfsDir.h"

#include "sys_fs.h"

SysCallBase sys_fs("sys_fs");

s32 sys_fs_test(u32 arg1, u32 arg2, vm::ptr<u32> arg3, u32 arg4, vm::ptr<char> arg5, u32 arg6)
{
	sys_fs.Todo("sys_fs_test(arg1=0x%x, arg2=0x%x, arg3=*0x%x, arg4=0x%x, arg5=*0x%x, arg6=0x%x) -> CELL_OK", arg1, arg2, arg3, arg4, arg5, arg6);

	return CELL_OK;
}

s32 sys_fs_open(vm::cptr<char> path, s32 flags, vm::ptr<u32> fd, s32 mode, vm::cptr<void> arg, u64 size)
{
	sys_fs.Warning("sys_fs_open(path=*0x%x, flags=%#o, fd=*0x%x, mode=%#o, arg=*0x%x, size=0x%llx)", path, flags, fd, mode, arg, size);
	sys_fs.Warning("*** path = '%s'", path.get_ptr());

	if (!path[0])
	{
		sys_fs.Error("sys_fs_open('%s') failed: path is invalid", path.get_ptr());
		return CELL_FS_EINVAL;
	}

	std::string local_path;

	const auto device = Emu.GetVFS().GetDevice(path.get_ptr(), local_path);

	if (!device)
	{
		sys_fs.Error("sys_fs_open('%s') failed: device not mounted", path.get_ptr());
		return CELL_FS_ENOTMOUNTED;
	}

	// TODO: other checks for path

	if (fs::is_dir(local_path))
	{
		sys_fs.Error("sys_fs_open('%s') failed: path is a directory", path.get_ptr());
		return CELL_FS_EISDIR;
	}

	u32 open_mode = 0;

	switch (flags & CELL_FS_O_ACCMODE)
	{
	case CELL_FS_O_RDONLY: open_mode |= o_read; break;
	case CELL_FS_O_WRONLY: open_mode |= o_write; break;
	case CELL_FS_O_RDWR: open_mode |= o_read | o_write; break;
	}

	if (flags & CELL_FS_O_CREAT)
	{
		open_mode |= o_create;
	}

	if (flags & CELL_FS_O_TRUNC)
	{
		open_mode |= o_trunc;
	}

	if (flags & CELL_FS_O_APPEND)
	{
		open_mode |= o_append;
	}

	if (flags & CELL_FS_O_EXCL)
	{
		if (flags & CELL_FS_O_CREAT)
		{
			open_mode |= o_excl;
		}
		else
		{
			open_mode = 0; // error
		}
	}

	if (flags & ~(CELL_FS_O_ACCMODE | CELL_FS_O_CREAT | CELL_FS_O_TRUNC | CELL_FS_O_APPEND | CELL_FS_O_EXCL))
	{
		open_mode = 0; // error
	}

	if ((flags & CELL_FS_O_ACCMODE) == CELL_FS_O_ACCMODE)
	{
		open_mode = 0; // error
	}

	if (!open_mode)
	{
		throw EXCEPTION("Invalid or unimplemented flags (%#o): '%s'", flags, path.get_ptr());
	}

	std::shared_ptr<vfsStream> file(Emu.GetVFS().OpenFile(path.get_ptr(), open_mode));

	if (!file || !file->IsOpened())
	{
		sys_fs.Error("sys_fs_open('%s'): failed to open file (flags=%#o, mode=%#o)", path.get_ptr(), flags, mode);

		if (open_mode & o_excl)
		{
			return CELL_FS_EEXIST; // approximation
		}

		return CELL_FS_ENOENT;
	}

	const auto _file = idm::make_ptr<lv2_file_t>(std::move(file), mode, flags);

	if (!_file)
	{
		// out of file descriptors
		return CELL_FS_EMFILE;
	}

	*fd = idm::get_last_id();

	return CELL_OK;
}

s32 sys_fs_read(u32 fd, vm::ptr<void> buf, u64 nbytes, vm::ptr<u64> nread)
{
	sys_fs.Log("sys_fs_read(fd=%d, buf=0x%x, nbytes=0x%llx, nread=0x%x)", fd, buf, nbytes, nread);

	const auto file = idm::get<lv2_file_t>(fd);

	if (!file || file->flags & CELL_FS_O_WRONLY)
	{
		return CELL_FS_EBADF;
	}

	std::lock_guard<std::mutex> lock(file->mutex);

	*nread = file->file->Read(buf.get_ptr(), nbytes);

	return CELL_OK;
}

s32 sys_fs_write(u32 fd, vm::cptr<void> buf, u64 nbytes, vm::ptr<u64> nwrite)
{
	sys_fs.Log("sys_fs_write(fd=%d, buf=*0x%x, nbytes=0x%llx, nwrite=*0x%x)", fd, buf, nbytes, nwrite);

	const auto file = idm::get<lv2_file_t>(fd);

	if (!file || !(file->flags & CELL_FS_O_ACCMODE))
	{
		return CELL_FS_EBADF;
	}

	// TODO: return CELL_FS_EBUSY if locked by stream

	std::lock_guard<std::mutex> lock(file->mutex);

	*nwrite = file->file->Write(buf.get_ptr(), nbytes);

	return CELL_OK;
}

s32 sys_fs_close(u32 fd)
{
	sys_fs.Log("sys_fs_close(fd=%d)", fd);

	const auto file = idm::get<lv2_file_t>(fd);

	if (!file)
	{
		return CELL_FS_EBADF;
	}

	// TODO: return CELL_FS_EBUSY if locked

	idm::remove<lv2_file_t>(fd);

	return CELL_OK;
}

s32 sys_fs_opendir(vm::cptr<char> path, vm::ptr<u32> fd)
{
	sys_fs.Warning("sys_fs_opendir(path=*0x%x, fd=*0x%x)", path, fd);
	sys_fs.Warning("*** path = '%s'", path.get_ptr());

	std::shared_ptr<vfsDirBase> dir(Emu.GetVFS().OpenDir(path.get_ptr()));

	if (!dir || !dir->IsOpened())
	{
		sys_fs.Error("sys_fs_opendir('%s'): failed to open directory", path.get_ptr());
		return CELL_FS_ENOENT;
	}

	const auto _dir = idm::make_ptr<lv2_dir_t>(std::move(dir));

	if (!_dir)
	{
		// out of file descriptors
		return CELL_FS_EMFILE;
	}

	*fd = idm::get_last_id();

	return CELL_OK;
}

s32 sys_fs_readdir(u32 fd, vm::ptr<CellFsDirent> dir, vm::ptr<u64> nread)
{
	sys_fs.Warning("sys_fs_readdir(fd=%d, dir=*0x%x, nread=*0x%x)", fd, dir, nread);

	const auto directory = idm::get<lv2_dir_t>(fd);

	if (!directory)
	{
		return CELL_FS_EBADF;
	}

	const DirEntryInfo* info = directory->dir->Read();

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
	sys_fs.Log("sys_fs_closedir(fd=%d)", fd);

	const auto directory = idm::get<lv2_dir_t>(fd);

	if (!directory)
	{
		return CELL_FS_EBADF;
	}

	idm::remove<lv2_dir_t>(fd);

	return CELL_OK;
}

s32 sys_fs_stat(vm::cptr<char> path, vm::ptr<CellFsStat> sb)
{
	sys_fs.Warning("sys_fs_stat(path=*0x%x, sb=*0x%x)", path, sb);
	sys_fs.Warning("*** path = '%s'", path.get_ptr());

	std::string local_path;

	if (!Emu.GetVFS().GetDevice(path.get_ptr(), local_path))
	{
		sys_fs.Warning("sys_fs_stat('%s') failed: not mounted", path.get_ptr());
		return CELL_FS_ENOTMOUNTED;
	}

	fs::stat_t info;

	if (!fs::stat(local_path, info))
	{
		sys_fs.Error("sys_fs_stat('%s') failed: not found", path.get_ptr());
		return CELL_FS_ENOENT;
	}

	sb->mode = info.is_directory ? CELL_FS_S_IFDIR | 0777 : CELL_FS_S_IFREG | 0666;
	sb->uid = 1; // ???
	sb->gid = 1; // ???
	sb->atime = info.atime;
	sb->mtime = info.mtime;
	sb->ctime = info.ctime;
	sb->size = info.size;
	sb->blksize = 4096; // ???

	return CELL_OK;
}

s32 sys_fs_fstat(u32 fd, vm::ptr<CellFsStat> sb)
{
	sys_fs.Warning("sys_fs_fstat(fd=%d, sb=*0x%x)", fd, sb);

	const auto file = idm::get<lv2_file_t>(fd);

	if (!file)
	{
		return CELL_FS_EBADF;
	}

	std::lock_guard<std::mutex> lock(file->mutex);

	const auto local_file = dynamic_cast<vfsLocalFile*>(file->file.get());

	if (!local_file)
	{
		sys_fs.Error("sys_fs_fstat(fd=0x%x): not a local file");
		return CELL_FS_ENOTSUP;
	}

	fs::stat_t info;

	if (!local_file->GetFile().stat(info))
	{
		return CELL_FS_EIO; // ???
	}

	sb->mode = info.is_directory ? CELL_FS_S_IFDIR | 0777 : CELL_FS_S_IFREG | 0666;
	sb->uid = 1; // ???
	sb->gid = 1; // ???
	sb->atime = info.atime;
	sb->mtime = info.mtime;
	sb->ctime = info.ctime; // ctime may be incorrect
	sb->size = info.size;
	sb->blksize = 4096; // ???

	return CELL_OK;
}

s32 sys_fs_mkdir(vm::cptr<char> path, s32 mode)
{
	sys_fs.Warning("sys_fs_mkdir(path=*0x%x, mode=%#o)", path, mode);
	sys_fs.Warning("*** path = '%s'", path.get_ptr());

	std::string ps3_path = path.get_ptr();

	if (Emu.GetVFS().ExistsDir(ps3_path))
	{
		return CELL_FS_EEXIST;
	}

	if (!Emu.GetVFS().CreatePath(ps3_path))
	{
		return CELL_FS_EIO; // ???
	}

	sys_fs.Notice("sys_fs_mkdir(): directory '%s' created", path.get_ptr());
	return CELL_OK;
}

s32 sys_fs_rename(vm::cptr<char> from, vm::cptr<char> to)
{
	sys_fs.Warning("sys_fs_rename(from=*0x%x, to=*0x%x)", from, to);
	sys_fs.Warning("*** from = '%s'", from.get_ptr());
	sys_fs.Warning("*** to   = '%s'", to.get_ptr());

	if (!Emu.GetVFS().Rename(from.get_ptr(), to.get_ptr()))
	{
		return CELL_FS_ENOENT; // ???
	}

	sys_fs.Notice("sys_fs_rename(): '%s' renamed to '%s'", from.get_ptr(), to.get_ptr());
	return CELL_OK;
}

s32 sys_fs_rmdir(vm::cptr<char> path)
{
	sys_fs.Warning("sys_fs_rmdir(path=*0x%x)", path);
	sys_fs.Warning("*** path = '%s'", path.get_ptr());

	std::string ps3_path = path.get_ptr();

	if (!Emu.GetVFS().ExistsDir(ps3_path))
	{
		return CELL_FS_ENOENT;
	}

	if (!Emu.GetVFS().RemoveDir(ps3_path))
	{
		return CELL_FS_EIO; // ???
	}

	sys_fs.Notice("sys_fs_rmdir(): directory '%s' removed", path.get_ptr());
	return CELL_OK;
}

s32 sys_fs_unlink(vm::cptr<char> path)
{
	sys_fs.Warning("sys_fs_unlink(path=*0x%x)", path);
	sys_fs.Warning("*** path = '%s'", path.get_ptr());

	std::string ps3_path = path.get_ptr();

	if (!Emu.GetVFS().ExistsFile(ps3_path))
	{
		return CELL_FS_ENOENT;
	}

	if (!Emu.GetVFS().RemoveFile(ps3_path))
	{
		return CELL_FS_EIO; // ???
	}

	sys_fs.Notice("sys_fs_unlink(): file '%s' deleted", path.get_ptr());
	return CELL_OK;
}

s32 sys_fs_fcntl(u32 fd, s32 flags, u32 addr, u32 arg4, u32 arg5, u32 arg6)
{
	sys_fs.Todo("sys_fs_fcntl(fd=0x%x, flags=0x%x, addr=*0x%x, arg4=0x%x, arg5=0x%x, arg6=0x%x) -> CELL_OK", fd, flags, addr, arg4, arg5, arg6);

	return CELL_OK;
}

s32 sys_fs_lseek(u32 fd, s64 offset, s32 whence, vm::ptr<u64> pos)
{
	sys_fs.Log("sys_fs_lseek(fd=%d, offset=0x%llx, whence=0x%x, pos=*0x%x)", fd, offset, whence, pos);

	if (whence >= 3)
	{
		sys_fs.Error("sys_fs_lseek(): unknown seek whence (%d)", whence);
		return CELL_FS_EINVAL;
	}

	const auto file = idm::get<lv2_file_t>(fd);

	if (!file)
	{
		return CELL_FS_EBADF;
	}

	std::lock_guard<std::mutex> lock(file->mutex);

	*pos = file->file->Seek(offset, whence);

	return CELL_OK;
}

s32 sys_fs_fget_block_size(u32 fd, vm::ptr<u64> sector_size, vm::ptr<u64> block_size, vm::ptr<u64> arg4, vm::ptr<u64> arg5)
{
	sys_fs.Todo("sys_fs_fget_block_size(fd=%d, sector_size=*0x%x, block_size=*0x%x, arg4=*0x%x, arg5=*0x%x)", fd, sector_size, block_size, arg4, arg5);

	const auto file = idm::get<lv2_file_t>(fd);

	if (!file)
	{
		return CELL_FS_EBADF;
	}

	*sector_size = 4096; // ?
	*block_size = 4096; // ?

	return CELL_OK;
}

s32 sys_fs_get_block_size(vm::cptr<char> path, vm::ptr<u64> sector_size, vm::ptr<u64> block_size, vm::ptr<u64> arg4)
{
	sys_fs.Todo("sys_fs_get_block_size(path=*0x%x, sector_size=*0x%x, block_size=*0x%x, arg4=*0x%x, arg5=*0x%x)", path, sector_size, block_size, arg4);
	sys_fs.Todo("*** path = '%s'", path.get_ptr());

	*sector_size = 4096; // ?
	*block_size = 4096; // ?

	return CELL_OK;
}

s32 sys_fs_truncate(vm::cptr<char> path, u64 size)
{
	sys_fs.Warning("sys_fs_truncate(path=*0x%x, size=0x%llx)", path, size);
	sys_fs.Warning("*** path = '%s'", path.get_ptr());

	std::string ps3_path = path.get_ptr();

	if (!Emu.GetVFS().ExistsFile(ps3_path))
	{
		return CELL_FS_ENOENT;
	}

	if (!Emu.GetVFS().TruncateFile(ps3_path, size))
	{
		return CELL_FS_EIO; // ???
	}

	return CELL_OK;
}

s32 sys_fs_ftruncate(u32 fd, u64 size)
{
	sys_fs.Warning("sys_fs_ftruncate(fd=%d, size=0x%llx)", fd, size);

	const auto file = idm::get<lv2_file_t>(fd);

	if (!file || !(file->flags & CELL_FS_O_ACCMODE))
	{
		return CELL_FS_EBADF;
	}

	std::lock_guard<std::mutex> lock(file->mutex);

	const auto local_file = dynamic_cast<vfsLocalFile*>(file->file.get());

	if (!local_file)
	{
		sys_fs.Error("sys_fs_ftruncate(fd=0x%x): not a local file");
		return CELL_FS_ENOTSUP;
	}

	if (!local_file->GetFile().trunc(size))
	{
		return CELL_FS_EIO; // ???
	}

	return CELL_OK;
}

s32 sys_fs_chmod(vm::cptr<char> path, s32 mode)
{
	sys_fs.Todo("sys_fs_chmod(path=*0x%x, mode=%#o) -> CELL_OK", path, mode);
	sys_fs.Todo("*** path = '%s'", path.get_ptr());

	return CELL_OK;
}
