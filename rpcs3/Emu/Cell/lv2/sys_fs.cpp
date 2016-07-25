#include "stdafx.h"
#include "sys_fs.h"

#include <mutex>

#include "Emu/VFS.h"
#include "Utilities/StrUtil.h"

namespace vm { using namespace ps3; }

logs::channel sys_fs("sys_fs", logs::level::notice);

struct lv2_fs_mount_point
{
	std::mutex mutex;
};

lv2_fs_mount_point g_mp_sys_dev_hdd0;
lv2_fs_mount_point g_mp_sys_dev_hdd1;
lv2_fs_mount_point g_mp_sys_dev_usb;
lv2_fs_mount_point g_mp_sys_dev_bdvd;
lv2_fs_mount_point g_mp_sys_app_home;
lv2_fs_mount_point g_mp_sys_host_root;

lv2_fs_mount_point* lv2_fs_object::get_mp(const char* filename)
{
	// TODO
	return &g_mp_sys_dev_hdd0;
}

u64 lv2_file::op_read(vm::ps3::ptr<void> buf, u64 size)
{
	// Copy data from intermediate buffer (avoid passing vm pointer to a native API)
	std::unique_ptr<u8[]> local_buf(new u8[size]);
	const u64 result = file.read(local_buf.get(), size);
	std::memcpy(buf.get_ptr(), local_buf.get(), result);
	return result;
}

u64 lv2_file::op_write(vm::ps3::cptr<void> buf, u64 size)
{
	// Copy data to intermediate buffer (avoid passing vm pointer to a native API)
	std::unique_ptr<u8[]> local_buf(new u8[size]);
	std::memcpy(local_buf.get(), buf.get_ptr(), size);
	return file.write(local_buf.get(), size);
}

ppu_error_code sys_fs_test(u32 arg1, u32 arg2, vm::ptr<u32> arg3, u32 arg4, vm::ptr<char> arg5, u32 arg6)
{
	sys_fs.todo("sys_fs_test(arg1=0x%x, arg2=0x%x, arg3=*0x%x, arg4=0x%x, arg5=*0x%x, arg6=0x%x) -> CELL_OK", arg1, arg2, arg3, arg4, arg5, arg6);

	return CELL_OK;
}

ppu_error_code sys_fs_open(vm::cptr<char> path, s32 flags, vm::ptr<u32> fd, s32 mode, vm::cptr<void> arg, u64 size)
{
	sys_fs.warning("sys_fs_open(path=*0x%x, flags=%#o, fd=*0x%x, mode=%#o, arg=*0x%x, size=0x%llx)", path, flags, fd, mode, arg, size);
	sys_fs.warning("*** path = '%s'", path.get_ptr());

	if (!path[0])
	{
		sys_fs.error("sys_fs_open('%s') failed: path is invalid", path.get_ptr());
		return CELL_EINVAL;
	}

	const std::string& local_path = vfs::get(path.get_ptr());

	if (local_path.empty())
	{
		sys_fs.error("sys_fs_open('%s') failed: device not mounted", path.get_ptr());
		return CELL_ENOTMOUNTED;
	}

	// TODO: other checks for path

	if (fs::is_dir(local_path))
	{
		sys_fs.error("sys_fs_open('%s') failed: path is a directory", path.get_ptr());
		return CELL_EISDIR;
	}

	bitset_t<fs::open_mode> open_mode{};

	switch (flags & CELL_FS_O_ACCMODE)
	{
	case CELL_FS_O_RDONLY: open_mode += fs::read; break;
	case CELL_FS_O_WRONLY: open_mode += fs::write; break;
	case CELL_FS_O_RDWR: open_mode += fs::read + fs::write; break;
	}

	if (flags & CELL_FS_O_CREAT)
	{
		open_mode += fs::create;
	}

	if (flags & CELL_FS_O_TRUNC)
	{
		open_mode += fs::trunc;
	}

	if (flags & CELL_FS_O_APPEND)
	{
		open_mode += fs::append;
	}

	if (flags & CELL_FS_O_EXCL)
	{
		if (flags & CELL_FS_O_CREAT)
		{
			open_mode += fs::excl;
		}
		else
		{
			open_mode = {}; // error
		}
	}

	if (flags & ~(CELL_FS_O_ACCMODE | CELL_FS_O_CREAT | CELL_FS_O_TRUNC | CELL_FS_O_APPEND | CELL_FS_O_EXCL))
	{
		open_mode = {}; // error
	}

	if ((flags & CELL_FS_O_ACCMODE) == CELL_FS_O_ACCMODE)
	{
		open_mode = {}; // error
	}

	if (!open_mode)
	{
		throw EXCEPTION("Invalid or unimplemented flags (%#o): '%s'", flags, path.get_ptr());
	}

	fs::file file(local_path, open_mode);

	if (!file)
	{
		sys_fs.error("sys_fs_open('%s'): failed to open file (flags=%#o, mode=%#o)", path.get_ptr(), flags, mode);

		if (open_mode & fs::excl)
		{
			return CELL_EEXIST; // approximation
		}

		return CELL_ENOENT;
	}

	const auto _file = idm::make_ptr<lv2_file>(path.get_ptr(), std::move(file), mode, flags);

	if (!_file)
	{
		// out of file descriptors
		return CELL_EMFILE;
	}

	*fd = _file->id;

	return CELL_OK;
}

ppu_error_code sys_fs_read(u32 fd, vm::ptr<void> buf, u64 nbytes, vm::ptr<u64> nread)
{
	sys_fs.trace("sys_fs_read(fd=%d, buf=*0x%x, nbytes=0x%llx, nread=*0x%x)", fd, buf, nbytes, nread);

	if (!buf)
	{
		return CELL_EFAULT;
	}

	const auto file = idm::get<lv2_file>(fd);

	if (!file || file->flags & CELL_FS_O_WRONLY)
	{
		return CELL_EBADF;
	}

	std::lock_guard<std::mutex> lock(file->mp->mutex);

	*nread = file->op_read(buf, nbytes);

	return CELL_OK;
}

ppu_error_code sys_fs_write(u32 fd, vm::cptr<void> buf, u64 nbytes, vm::ptr<u64> nwrite)
{
	sys_fs.trace("sys_fs_write(fd=%d, buf=*0x%x, nbytes=0x%llx, nwrite=*0x%x)", fd, buf, nbytes, nwrite);

	const auto file = idm::get<lv2_file>(fd);

	if (!file || !(file->flags & CELL_FS_O_ACCMODE))
	{
		return CELL_EBADF;
	}

	// TODO: return CELL_EBUSY if locked by stream

	std::lock_guard<std::mutex> lock(file->mp->mutex);

	*nwrite = file->op_write(buf, nbytes);

	return CELL_OK;
}

ppu_error_code sys_fs_close(u32 fd)
{
	sys_fs.trace("sys_fs_close(fd=%d)", fd);

	const auto file = idm::get<lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	// TODO: return CELL_EBUSY if locked

	idm::remove<lv2_file>(fd);

	return CELL_OK;
}

ppu_error_code sys_fs_opendir(vm::cptr<char> path, vm::ptr<u32> fd)
{
	sys_fs.warning("sys_fs_opendir(path=*0x%x, fd=*0x%x)", path, fd);
	sys_fs.warning("*** path = '%s'", path.get_ptr());

	const std::string& local_path = vfs::get(path.get_ptr());

	if (local_path.empty())
	{
		sys_fs.error("sys_fs_opendir('%s') failed: device not mounted", path.get_ptr());
		return CELL_ENOTMOUNTED;
	}

	// TODO: other checks for path

	if (fs::is_file(local_path))
	{
		sys_fs.error("sys_fs_opendir('%s') failed: path is a file", path.get_ptr());
		return CELL_ENOTDIR;
	}

	fs::dir dir(local_path);

	if (!dir)
	{
		sys_fs.error("sys_fs_opendir('%s'): failed to open directory", path.get_ptr());
		return CELL_ENOENT;
	}

	const auto _dir = idm::make_ptr<lv2_dir>(path.get_ptr(), std::move(dir));

	if (!_dir)
	{
		// out of file descriptors
		return CELL_EMFILE;
	}

	*fd = _dir->id;

	return CELL_OK;
}

ppu_error_code sys_fs_readdir(u32 fd, vm::ptr<CellFsDirent> dir, vm::ptr<u64> nread)
{
	sys_fs.warning("sys_fs_readdir(fd=%d, dir=*0x%x, nread=*0x%x)", fd, dir, nread);

	const auto directory = idm::get<lv2_dir>(fd);

	if (!directory)
	{
		return CELL_EBADF;
	}

	fs::dir_entry info;

	if (directory->dir.read(info))
	{
		dir->d_type = info.is_directory ? CELL_FS_TYPE_DIRECTORY : CELL_FS_TYPE_REGULAR;
		dir->d_namlen = u8(std::min<size_t>(info.name.size(), CELL_FS_MAX_FS_FILE_NAME_LENGTH));
		strcpy_trunc(dir->d_name, info.name);
		*nread = sizeof(CellFsDirent);
	}
	else
	{
		*nread = 0;
	}

	return CELL_OK;
}

ppu_error_code sys_fs_closedir(u32 fd)
{
	sys_fs.trace("sys_fs_closedir(fd=%d)", fd);

	const auto directory = idm::get<lv2_dir>(fd);

	if (!directory)
	{
		return CELL_EBADF;
	}

	idm::remove<lv2_dir>(fd);

	return CELL_OK;
}

ppu_error_code sys_fs_stat(vm::cptr<char> path, vm::ptr<CellFsStat> sb)
{
	sys_fs.warning("sys_fs_stat(path=*0x%x, sb=*0x%x)", path, sb);
	sys_fs.warning("*** path = '%s'", path.get_ptr());

	const std::string& local_path = vfs::get(path.get_ptr());

	if (local_path.empty())
	{
		sys_fs.warning("sys_fs_stat('%s') failed: not mounted", path.get_ptr());
		return CELL_ENOTMOUNTED;
	}

	fs::stat_t info;

	if (!fs::stat(local_path, info))
	{
		sys_fs.error("sys_fs_stat('%s') failed: not found", path.get_ptr());
		return CELL_ENOENT;
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

ppu_error_code sys_fs_fstat(u32 fd, vm::ptr<CellFsStat> sb)
{
	sys_fs.warning("sys_fs_fstat(fd=%d, sb=*0x%x)", fd, sb);

	const auto file = idm::get<lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	std::lock_guard<std::mutex> lock(file->mp->mutex);

	const fs::stat_t& info = file->file.stat();

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

ppu_error_code sys_fs_mkdir(vm::cptr<char> path, s32 mode)
{
	sys_fs.warning("sys_fs_mkdir(path=*0x%x, mode=%#o)", path, mode);
	sys_fs.warning("*** path = '%s'", path.get_ptr());

	const std::string& local_path = vfs::get(path.get_ptr());

	if (fs::is_dir(local_path))
	{
		return CELL_EEXIST;
	}

	if (!fs::create_path(local_path))
	{
		return CELL_EIO; // ???
	}

	sys_fs.notice("sys_fs_mkdir(): directory '%s' created", path.get_ptr());
	return CELL_OK;
}

ppu_error_code sys_fs_rename(vm::cptr<char> from, vm::cptr<char> to)
{
	sys_fs.warning("sys_fs_rename(from=*0x%x, to=*0x%x)", from, to);
	sys_fs.warning("*** from = '%s'", from.get_ptr());
	sys_fs.warning("*** to   = '%s'", to.get_ptr());

	if (!fs::rename(vfs::get(from.get_ptr()), vfs::get(to.get_ptr())))
	{
		return CELL_ENOENT; // ???
	}

	sys_fs.notice("sys_fs_rename(): '%s' renamed to '%s'", from.get_ptr(), to.get_ptr());
	return CELL_OK;
}

ppu_error_code sys_fs_rmdir(vm::cptr<char> path)
{
	sys_fs.warning("sys_fs_rmdir(path=*0x%x)", path);
	sys_fs.warning("*** path = '%s'", path.get_ptr());

	if (!fs::remove_dir(vfs::get(path.get_ptr())))
	{
		switch (auto error = fs::g_tls_error)
		{
		case fs::error::noent: return CELL_ENOENT;
		default: sys_fs.error("sys_fs_rmdir(): unknown error %s", error);
		}

		return CELL_EIO; // ???
	}

	sys_fs.notice("sys_fs_rmdir(): directory '%s' removed", path.get_ptr());
	return CELL_OK;
}

ppu_error_code sys_fs_unlink(vm::cptr<char> path)
{
	sys_fs.warning("sys_fs_unlink(path=*0x%x)", path);
	sys_fs.warning("*** path = '%s'", path.get_ptr());

	if (!fs::remove_file(vfs::get(path.get_ptr())))
	{
		switch (auto error = fs::g_tls_error)
		{
		case fs::error::noent: return CELL_ENOENT;
		default: sys_fs.error("sys_fs_unlink(): unknown error %s", error);
		}

		return CELL_EIO; // ???
	}

	sys_fs.notice("sys_fs_unlink(): file '%s' deleted", path.get_ptr());
	return CELL_OK;
}

ppu_error_code sys_fs_fcntl(u32 fd, u32 op, vm::ptr<void> _arg, u32 _size)
{
	sys_fs.trace("sys_fs_fcntl(fd=%d, op=0x%x, arg=*0x%x, size=0x%x)", fd, op, _arg, _size);

	switch (op)
	{
	case 0x8000000A: // Read with offset
	case 0x8000000B: // Write with offset
	{
		const auto arg = vm::static_ptr_cast<lv2_file_op_rw>(_arg);

		if (_size < arg.size())
		{
			return CELL_EINVAL;
		}

		const auto file = idm::get<lv2_file>(fd);

		if (!file)
		{
			return CELL_EBADF;
		}

		if (op == 0x8000000A && file->flags & CELL_FS_O_WRONLY)
		{
			return CELL_EBADF;
		}

		if (op == 0x8000000B && !(file->flags & CELL_FS_O_ACCMODE))
		{
			return CELL_EBADF;
		}

		std::lock_guard<std::mutex> lock(file->mp->mutex);

		const u64 old_pos = file->file.pos();
		const u64 new_pos = file->file.seek(arg->offset);

		arg->out_size = op == 0x8000000A
			? file->op_read(arg->buf, arg->size)
			: file->op_write(arg->buf, arg->size);

		VERIFY(old_pos == file->file.seek(old_pos));

		arg->out_code = CELL_OK;

		break;
	}
	default:
	{
		sys_fs.todo("sys_fs_fcntl(): Unknown operation 0x%08x (fd=%d, arg=*0x%x, size=0x%x)", op, fd, _arg, _size);
	}
	}

	return CELL_OK;
}

ppu_error_code sys_fs_lseek(u32 fd, s64 offset, s32 whence, vm::ptr<u64> pos)
{
	sys_fs.trace("sys_fs_lseek(fd=%d, offset=0x%llx, whence=0x%x, pos=*0x%x)", fd, offset, whence, pos);

	if (whence >= 3)
	{
		sys_fs.error("sys_fs_lseek(): invalid seek whence (%d)", whence);
		return CELL_EINVAL;
	}

	const auto file = idm::get<lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	std::lock_guard<std::mutex> lock(file->mp->mutex);

	*pos = file->file.seek(offset, static_cast<fs::seek_mode>(whence));

	return CELL_OK;
}

ppu_error_code sys_fs_fget_block_size(u32 fd, vm::ptr<u64> sector_size, vm::ptr<u64> block_size, vm::ptr<u64> arg4, vm::ptr<u64> arg5)
{
	sys_fs.todo("sys_fs_fget_block_size(fd=%d, sector_size=*0x%x, block_size=*0x%x, arg4=*0x%x, arg5=*0x%x)", fd, sector_size, block_size, arg4, arg5);

	const auto file = idm::get<lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	*sector_size = 4096; // ?
	*block_size = 4096; // ?

	return CELL_OK;
}

ppu_error_code sys_fs_get_block_size(vm::cptr<char> path, vm::ptr<u64> sector_size, vm::ptr<u64> block_size, vm::ptr<u64> arg4)
{
	sys_fs.todo("sys_fs_get_block_size(path=*0x%x, sector_size=*0x%x, block_size=*0x%x, arg4=*0x%x, arg5=*0x%x)", path, sector_size, block_size, arg4);
	sys_fs.todo("*** path = '%s'", path.get_ptr());

	*sector_size = 4096; // ?
	*block_size = 4096; // ?

	return CELL_OK;
}

ppu_error_code sys_fs_truncate(vm::cptr<char> path, u64 size)
{
	sys_fs.warning("sys_fs_truncate(path=*0x%x, size=0x%llx)", path, size);
	sys_fs.warning("*** path = '%s'", path.get_ptr());

	if (!fs::truncate_file(vfs::get(path.get_ptr()), size))
	{
		switch (auto error = fs::g_tls_error)
		{
		case fs::error::noent: return CELL_ENOENT;
		default: sys_fs.error("sys_fs_truncate(): unknown error %s", error);
		}

		return CELL_EIO; // ???
	}

	return CELL_OK;
}

ppu_error_code sys_fs_ftruncate(u32 fd, u64 size)
{
	sys_fs.warning("sys_fs_ftruncate(fd=%d, size=0x%llx)", fd, size);

	const auto file = idm::get<lv2_file>(fd);

	if (!file || !(file->flags & CELL_FS_O_ACCMODE))
	{
		return CELL_EBADF;
	}

	std::lock_guard<std::mutex> lock(file->mp->mutex);

	if (!file->file.trunc(size))
	{
		switch (auto error = fs::g_tls_error)
		{
		case fs::error::ok:
		default: sys_fs.error("sys_fs_ftruncate(): unknown error %s", error);
		}

		return CELL_EIO; // ???
	}

	return CELL_OK;
}

ppu_error_code sys_fs_chmod(vm::cptr<char> path, s32 mode)
{
	sys_fs.todo("sys_fs_chmod(path=*0x%x, mode=%#o) -> CELL_OK", path, mode);
	sys_fs.todo("*** path = '%s'", path.get_ptr());

	return CELL_OK;
}
