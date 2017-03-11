#include "stdafx.h"
#include "sys_fs.h"

#include <mutex>

#include "Emu/Cell/PPUThread.h"
#include "Crypto/unedat.h"
#include "Emu/VFS.h"
#include "Emu/IdManager.h"
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

bool verify_mself(u32 fd, fs::file const& mself_file)
{
	FsMselfHeader mself_header;
	if (!mself_file.read<FsMselfHeader>(mself_header))
	{
		sys_fs.error("verify_mself: Didn't read expected bytes for header.");
		return false;
	}

	if (mself_header.m_magic != 0x4D534600)
	{
		sys_fs.error("verify_mself: Header magic is incorrect.");
		return false;
	}

	if (mself_header.m_format_version != 1)
	{
		sys_fs.error("verify_mself: Unexpected header format version.");
		return false;
	}

	// sanity check
	if (mself_header.m_entry_size != sizeof(FsMselfEntry))
	{
		sys_fs.error("verify_mself: Unexpected header entry size.");
		return false;
	}

	return true;
}

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

struct lv2_file::file_view : fs::file_base
{
	const std::shared_ptr<lv2_file> m_file;
	const u64 m_off;
	u64 m_pos;

	explicit file_view(const std::shared_ptr<lv2_file>& _file, u64 offset)
		: m_file(_file)
		, m_off(offset)
		, m_pos(0)
	{
	}

	~file_view() override
	{
	}

	fs::stat_t stat() override
	{
		return m_file->file.stat();
	}

	bool trunc(u64 length) override
	{
		return false;
	}

	u64 read(void* buffer, u64 size) override
	{
		const u64 old_pos = m_file->file.pos();
		const u64 new_pos = m_file->file.seek(m_off + m_pos);
		const u64 result = m_file->file.read(buffer, size);
		verify(HERE), old_pos == m_file->file.seek(old_pos);

		m_pos += result;
		return result;
	}

	u64 write(const void* buffer, u64 size) override
	{
		return 0;
	}

	u64 seek(s64 offset, fs::seek_mode whence) override
	{
		return
			whence == fs::seek_set ? m_pos = offset :
			whence == fs::seek_cur ? m_pos = offset + m_pos :
			whence == fs::seek_end ? m_pos = offset + size() :
			(fmt::raw_error("lv2_file::file_view::seek(): invalid whence"), 0);
	}

	u64 size() override
	{
		return m_file->file.size();
	}
};

fs::file lv2_file::make_view(const std::shared_ptr<lv2_file>& _file, u64 offset)
{
	fs::file result;
	result.reset(std::make_unique<lv2_file::file_view>(_file, offset));
	return result;
}

error_code sys_fs_test(u32 arg1, u32 arg2, vm::ptr<u32> arg3, u32 arg4, vm::ptr<char> arg5, u32 arg6)
{
	sys_fs.todo("sys_fs_test(arg1=0x%x, arg2=0x%x, arg3=*0x%x, arg4=0x%x, arg5=*0x%x, arg6=0x%x) -> CELL_OK", arg1, arg2, arg3, arg4, arg5, arg6);

	return CELL_OK;
}

error_code sys_fs_open(vm::cptr<char> path, s32 flags, vm::ptr<u32> fd, s32 mode, vm::cptr<void> arg, u64 size)
{
	sys_fs.warning("sys_fs_open(path=%s, flags=%#o, fd=*0x%x, mode=%#o, arg=*0x%x, size=0x%llx)", path, flags, fd, mode, arg, size);

	if (!path[0])
	{
		sys_fs.error("sys_fs_open(%s) failed: path is invalid", path);
		return CELL_EINVAL;
	}

	const std::string& local_path = vfs::get(path.get_ptr());

	if (local_path.empty())
	{
		sys_fs.error("sys_fs_open(%s) failed: device not mounted", path);
		return CELL_ENOTMOUNTED;
	}

	// TODO: other checks for path

	if (fs::is_dir(local_path))
	{
		sys_fs.error("sys_fs_open(%s) failed: path is a directory", path);
		return CELL_EISDIR;
	}

	bs_t<fs::open_mode> open_mode{};

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

	if (flags & CELL_FS_O_MSELF)
	{
		open_mode = fs::read;
		// mself can be mself or mself | rdonly
		if (flags & ~(CELL_FS_O_MSELF | CELL_FS_O_RDONLY))
		{
			open_mode = {};
		}
	}

	if (flags & ~(CELL_FS_O_ACCMODE | CELL_FS_O_CREAT | CELL_FS_O_TRUNC | CELL_FS_O_APPEND | CELL_FS_O_EXCL | CELL_FS_O_MSELF))
	{
		open_mode = {}; // error
	}

	if ((flags & CELL_FS_O_ACCMODE) == CELL_FS_O_ACCMODE)
	{
		open_mode = {}; // error
	}

	if (!test(open_mode))
	{
		fmt::throw_exception("sys_fs_open(%s): Invalid or unimplemented flags: %#o" HERE, path, flags);
	}

	fs::file file(local_path, open_mode);

	if (!file)
	{
		sys_fs.error("sys_fs_open(%s): failed to open file (flags=%#o, mode=%#o)", path, flags, mode);

		if (test(open_mode & fs::excl))
		{
			return CELL_EEXIST; // approximation
		}

		return CELL_ENOENT;
	}

	if ((flags & CELL_FS_O_MSELF) && (!verify_mself(*fd, file)))
		return CELL_ENOTMSELF;

	const auto casted_arg = vm::static_ptr_cast<const u64>(arg);//static_cast<const be_t<u32> *>(arg.get_ptr());
	if (size == 8)
	{
		// check for sdata 
		if (*casted_arg == 0x18000000010)
		{
			// check if the file has the NPD header, or else assume its not encrypted
			u32 magic;
			file.read<u32>(magic);
			file.seek(0);
			if (magic == "NPD\0"_u32)
			{
				auto sdata_file = std::make_unique<EDATADecrypter>(std::move(file));
				if (!sdata_file->ReadHeader())
				{
					sys_fs.error("sys_fs_open(%s): Error reading sdata header!", path);
					return CELL_EFSSPECIFIC;
				}

				file.reset(std::move(sdata_file));
			}
		}
		// edata 
		else if (*casted_arg == 0x2)
		{
			// check if the file has the NPD header, or else assume its not encrypted
			u32 magic;
			file.read<u32>(magic);
			file.seek(0);
			if (magic == "NPD\0"_u32)
			{
				auto edatkeys = fxm::get_always<EdatKeys_t>();
				auto sdata_file = std::make_unique<EDATADecrypter>(std::move(file), edatkeys->devKlic, edatkeys->rifKey);
				if (!sdata_file->ReadHeader())
				{
					sys_fs.error("sys_fs_open(%s): Error reading edata header!", path);
					return CELL_EFSSPECIFIC;
				}

				file.reset(std::move(sdata_file));
			}
		}
	}
	if (const u32 id = idm::make<lv2_fs_object, lv2_file>(path.get_ptr(), std::move(file), mode, flags))
	{
		*fd = id;
		return CELL_OK;
	}

	// Out of file descriptors
	return CELL_EMFILE;
}

error_code sys_fs_read(u32 fd, vm::ptr<void> buf, u64 nbytes, vm::ptr<u64> nread)
{
	sys_fs.trace("sys_fs_read(fd=%d, buf=*0x%x, nbytes=0x%llx, nread=*0x%x)", fd, buf, nbytes, nread);

	if (!buf)
	{
		return CELL_EFAULT;
	}

	const auto file = idm::get<lv2_fs_object, lv2_file>(fd);

	if (!file || file->flags & CELL_FS_O_WRONLY)
	{
		return CELL_EBADF;
	}

	std::lock_guard<std::mutex> lock(file->mp->mutex);

	*nread = file->op_read(buf, nbytes);

	return CELL_OK;
}

error_code sys_fs_write(u32 fd, vm::cptr<void> buf, u64 nbytes, vm::ptr<u64> nwrite)
{
	sys_fs.trace("sys_fs_write(fd=%d, buf=*0x%x, nbytes=0x%llx, nwrite=*0x%x)", fd, buf, nbytes, nwrite);

	const auto file = idm::get<lv2_fs_object, lv2_file>(fd);

	if (!file || !(file->flags & CELL_FS_O_ACCMODE))
	{
		return CELL_EBADF;
	}

	// TODO: return CELL_EBUSY if locked by stream

	std::lock_guard<std::mutex> lock(file->mp->mutex);

	*nwrite = file->op_write(buf, nbytes);

	return CELL_OK;
}

error_code sys_fs_close(u32 fd)
{
	sys_fs.trace("sys_fs_close(fd=%d)", fd);

	const auto file = idm::get<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	// TODO: return CELL_EBUSY if locked

	idm::remove<lv2_fs_object, lv2_file>(fd);

	return CELL_OK;
}

error_code sys_fs_opendir(vm::cptr<char> path, vm::ptr<u32> fd)
{
	sys_fs.warning("sys_fs_opendir(path=%s, fd=*0x%x)", path, fd);

	const std::string& local_path = vfs::get(path.get_ptr());

	if (local_path.empty())
	{
		sys_fs.error("sys_fs_opendir(%s) failed: device not mounted", path);
		return CELL_ENOTMOUNTED;
	}

	// TODO: other checks for path

	if (fs::is_file(local_path))
	{
		sys_fs.error("sys_fs_opendir(%s) failed: path is a file", path);
		return CELL_ENOTDIR;
	}

	fs::dir dir(local_path);

	if (!dir)
	{
		sys_fs.error("sys_fs_opendir(%s): failed to open directory", path);
		return CELL_ENOENT;
	}

	if (const u32 id = idm::make<lv2_fs_object, lv2_dir>(path.get_ptr(), std::move(dir)))
	{
		*fd = id;
		return CELL_OK;
	}

	// Out of file descriptors
	return CELL_EMFILE;
}

error_code sys_fs_readdir(u32 fd, vm::ptr<CellFsDirent> dir, vm::ptr<u64> nread)
{
	sys_fs.warning("sys_fs_readdir(fd=%d, dir=*0x%x, nread=*0x%x)", fd, dir, nread);

	const auto directory = idm::get<lv2_fs_object, lv2_dir>(fd);

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

error_code sys_fs_closedir(u32 fd)
{
	sys_fs.warning("sys_fs_closedir(fd=%d)", fd);

	const auto directory = idm::get<lv2_fs_object, lv2_dir>(fd);

	if (!directory)
	{
		return CELL_EBADF;
	}

	idm::remove<lv2_fs_object, lv2_dir>(fd);

	return CELL_OK;
}

error_code sys_fs_stat(vm::cptr<char> path, vm::ptr<CellFsStat> sb)
{
	sys_fs.warning("sys_fs_stat(path=%s, sb=*0x%x)", path, sb);

	const std::string& local_path = vfs::get(path.get_ptr());

	if (local_path.empty())
	{
		sys_fs.warning("sys_fs_stat(%s) failed: not mounted", path);
		return CELL_ENOTMOUNTED;
	}

	fs::stat_t info;

	if (!fs::stat(local_path, info))
	{
		sys_fs.error("sys_fs_stat(%s) failed: not found", path);
		return CELL_ENOENT;
	}

	sb->mode = info.is_directory ? CELL_FS_S_IFDIR | 0777 : CELL_FS_S_IFREG | 0666;
	sb->uid = 0; // Always zero
	sb->gid = 0; // Always zero
	sb->atime = info.atime;
	sb->mtime = info.mtime;
	sb->ctime = info.ctime;
	sb->size = info.size;
	sb->blksize = 4096; // ???

	return CELL_OK;
}

error_code sys_fs_fstat(u32 fd, vm::ptr<CellFsStat> sb)
{
	sys_fs.warning("sys_fs_fstat(fd=%d, sb=*0x%x)", fd, sb);

	const auto file = idm::get<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	std::lock_guard<std::mutex> lock(file->mp->mutex);

	const fs::stat_t& info = file->file.stat();

	sb->mode = info.is_directory ? CELL_FS_S_IFDIR | 0777 : CELL_FS_S_IFREG | 0666;
	sb->uid = 0; // Always zero
	sb->gid = 0; // Always zero
	sb->atime = info.atime;
	sb->mtime = info.mtime;
	sb->ctime = info.ctime; // ctime may be incorrect
	sb->size = info.size;
	sb->blksize = 4096; // ???

	return CELL_OK;
}

error_code sys_fs_mkdir(vm::cptr<char> path, s32 mode)
{
	sys_fs.warning("sys_fs_mkdir(path=%s, mode=%#o)", path, mode);

	const std::string& local_path = vfs::get(path.get_ptr());

	if (fs::is_dir(local_path))
	{
		return CELL_EEXIST;
	}

	if (!fs::create_path(local_path))
	{
		return CELL_EIO; // ???
	}

	sys_fs.notice("sys_fs_mkdir(): directory %s created", path);
	return CELL_OK;
}

error_code sys_fs_rename(vm::cptr<char> from, vm::cptr<char> to)
{
	sys_fs.warning("sys_fs_rename(from=%s, to=%s)", from, to);

	if (!fs::rename(vfs::get(from.get_ptr()), vfs::get(to.get_ptr())))
	{
		return CELL_ENOENT; // ???
	}

	sys_fs.notice("sys_fs_rename(): %s renamed to %s", from, to);
	return CELL_OK;
}

error_code sys_fs_rmdir(vm::cptr<char> path)
{
	sys_fs.warning("sys_fs_rmdir(path=%s)", path);

	if (!fs::remove_dir(vfs::get(path.get_ptr())))
	{
		switch (auto error = fs::g_tls_error)
		{
		case fs::error::noent: return CELL_ENOENT;
		default: sys_fs.error("sys_fs_rmdir(): unknown error %s", error);
		}

		return CELL_EIO; // ???
	}

	sys_fs.notice("sys_fs_rmdir(): directory %s removed", path);
	return CELL_OK;
}

error_code sys_fs_unlink(vm::cptr<char> path)
{
	sys_fs.warning("sys_fs_unlink(path=%s)", path);

	if (!fs::remove_file(vfs::get(path.get_ptr())))
	{
		switch (auto error = fs::g_tls_error)
		{
		case fs::error::noent: return CELL_ENOENT;
		default: sys_fs.error("sys_fs_unlink(): unknown error %s", error);
		}

		return CELL_EIO; // ???
	}

	sys_fs.notice("sys_fs_unlink(): file %s deleted", path);
	return CELL_OK;
}

error_code sys_fs_fcntl(u32 fd, u32 op, vm::ptr<void> _arg, u32 _size)
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

		const auto file = idm::get<lv2_fs_object, lv2_file>(fd);

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

		verify(HERE), old_pos == file->file.seek(old_pos);

		arg->out_code = CELL_OK;

		break;
	}

	case 0x80000009: // cellFsSdataOpenByFd
	{
		const auto arg = vm::static_ptr_cast<lv2_file_op_09>(_arg);

		if (_size < arg.size())
		{
			return CELL_EINVAL;
		}

		const auto file = idm::get<lv2_fs_object, lv2_file>(fd);

		if (!file)
		{
			return CELL_EBADF;
		}

		auto sdata_file = std::make_unique<EDATADecrypter>(lv2_file::make_view(file, arg->offset));

		if (!sdata_file->ReadHeader())
		{
			return CELL_EFSSPECIFIC;
		}

		fs::file stream;
		stream.reset(std::move(sdata_file));
		if (const u32 id = idm::make<lv2_fs_object, lv2_file>(file->mp, std::move(stream), file->mode, file->flags))
		{
			arg->out_code = CELL_OK;
			arg->out_fd = id;
			return CELL_OK;
		}

		// Out of file descriptors
		return CELL_EMFILE;
	}

	default:
	{
		sys_fs.todo("sys_fs_fcntl(): Unknown operation 0x%08x (fd=%d, arg=*0x%x, size=0x%x)", op, fd, _arg, _size);
	}
	}

	return CELL_OK;
}

error_code sys_fs_lseek(u32 fd, s64 offset, s32 whence, vm::ptr<u64> pos)
{
	sys_fs.trace("sys_fs_lseek(fd=%d, offset=0x%llx, whence=0x%x, pos=*0x%x)", fd, offset, whence, pos);

	if (whence >= 3)
	{
		sys_fs.error("sys_fs_lseek(): invalid seek whence (%d)", whence);
		return CELL_EINVAL;
	}

	const auto file = idm::get<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	std::lock_guard<std::mutex> lock(file->mp->mutex);

	*pos = file->file.seek(offset, static_cast<fs::seek_mode>(whence));

	return CELL_OK;
}

error_code sys_fs_fget_block_size(u32 fd, vm::ptr<u64> sector_size, vm::ptr<u64> block_size, vm::ptr<u64> arg4, vm::ptr<u64> arg5)
{
	sys_fs.todo("sys_fs_fget_block_size(fd=%d, sector_size=*0x%x, block_size=*0x%x, arg4=*0x%x, arg5=*0x%x)", fd, sector_size, block_size, arg4, arg5);

	const auto file = idm::get<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	*sector_size = 4096; // ?
	*block_size = 4096; // ?

	return CELL_OK;
}

error_code sys_fs_get_block_size(vm::cptr<char> path, vm::ptr<u64> sector_size, vm::ptr<u64> block_size, vm::ptr<u64> arg4)
{
	sys_fs.todo("sys_fs_get_block_size(path=%s, sector_size=*0x%x, block_size=*0x%x, arg4=*0x%x, arg5=*0x%x)", path, sector_size, block_size, arg4);

	*sector_size = 4096; // ?
	*block_size = 4096; // ?

	return CELL_OK;
}

error_code sys_fs_truncate(vm::cptr<char> path, u64 size)
{
	sys_fs.warning("sys_fs_truncate(path=%s, size=0x%llx)", path, size);

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

error_code sys_fs_ftruncate(u32 fd, u64 size)
{
	sys_fs.warning("sys_fs_ftruncate(fd=%d, size=0x%llx)", fd, size);

	const auto file = idm::get<lv2_fs_object, lv2_file>(fd);

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

error_code sys_fs_chmod(vm::cptr<char> path, s32 mode)
{
	sys_fs.todo("sys_fs_chmod(path=%s, mode=%#o) -> CELL_OK", path, mode);

	return CELL_OK;
}

error_code sys_fs_utime(vm::ps3::cptr<char> path, vm::ps3::cptr<CellFsUtimbuf> timep)
{
	sys_fs.warning("sys_fs_utime(path=%s, timep=*0x%x)", path, timep);

	if (!fs::utime(vfs::get(path.get_ptr()), timep->actime, timep->modtime))
	{
		switch (auto error = fs::g_tls_error)
		{
		case fs::error::noent: return CELL_ENOENT;
		default: sys_fs.error("sys_fs_utime(): unknown error %s", error);
		}

		return CELL_EIO; // ???
	}

	return CELL_OK;
}
