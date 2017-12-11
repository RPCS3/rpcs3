#include "stdafx.h"
#include "sys_fs.h"

#include <mutex>

#include "Emu/Cell/PPUThread.h"
#include "Crypto/unedat.h"
#include "Emu/VFS.h"
#include "Emu/IdManager.h"
#include "Utilities/StrUtil.h"

namespace vm { using namespace ps3; }

logs::channel sys_fs("sys_fs");

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
		const s64 new_pos =
			whence == fs::seek_set ? offset :
			whence == fs::seek_cur ? offset + m_pos :
			whence == fs::seek_end ? offset + size() :
			(fmt::raw_error("lv2_file::file_view::seek(): invalid whence"), 0);

		if (new_pos < 0)
		{
			fs::g_tls_error = fs::error::inval;
			return -1;
		}

		m_pos = new_pos;
		return m_pos;
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

error_code sys_fs_test(u32 arg1, u32 arg2, vm::ptr<u32> arg3, u32 arg4, vm::ptr<char> buf, u32 buf_size)
{
	sys_fs.trace("sys_fs_test(arg1=0x%x, arg2=0x%x, arg3=*0x%x, arg4=0x%x, buf=*0x%x, buf_size=0x%x)", arg1, arg2, arg3, arg4, buf, buf_size);

	if (arg1 != 6 || arg2 != 0 || arg4 != sizeof(u32))
	{
		sys_fs.todo("sys_fs_test: unknown arguments (arg1=0x%x, arg2=0x%x, arg3=*0x%x, arg4=0x%x)", arg1, arg2, arg3, arg4);
	}

	if (!arg3)
	{
		return CELL_EFAULT;
	}

	const auto file = idm::get<lv2_fs_object>(*arg3);

	if (!file)
	{
		return CELL_EBADF;
	}

	for (u32 i = 0; i < buf_size; i++)
	{
		if (!(buf[i] = file->name[i]))
		{
			return CELL_OK;
		}
	}

	buf[buf_size - 1] = 0;
	return CELL_OK;
}

error_code sys_fs_open(vm::cptr<char> path, s32 flags, vm::ptr<u32> fd, s32 mode, vm::cptr<void> arg, u64 size)
{
	sys_fs.warning("sys_fs_open(path=%s, flags=%#o, fd=*0x%x, mode=%#o, arg=*0x%x, size=0x%llx)", path, flags, fd, mode, arg, size);

	if (!path[0])
	{
		return CELL_EINVAL;
	}

	const std::string& local_path = vfs::get(path.get_ptr());

	if (local_path.empty())
	{
		return {CELL_ENOTMOUNTED, path};
	}

	// TODO: other checks for path

	if (local_path == "/" || fs::is_dir(local_path))
	{
		return {CELL_EISDIR, path};
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
		if (test(open_mode & fs::excl) && fs::g_tls_error == fs::error::exist)
		{
			return not_an_error(CELL_EEXIST);
		}

		switch (auto error = fs::g_tls_error)
		{
		case fs::error::noent: return {CELL_ENOENT, path};
		default: sys_fs.error("sys_fs_open(): unknown error %s", error);
		}

		return {CELL_EIO, path};
	}

	if ((flags & CELL_FS_O_MSELF) && (!verify_mself(*fd, file)))
	{
		return {CELL_ENOTMSELF, path};
	}

	const auto casted_arg = vm::static_ptr_cast<const u64>(arg);

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
					return {CELL_EFSSPECIFIC, path};
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
				auto edatkeys = fxm::get_always<LoadedNpdrmKeys_t>();
				auto sdata_file = std::make_unique<EDATADecrypter>(std::move(file), edatkeys->devKlic, edatkeys->rifKey);
				if (!sdata_file->ReadHeader())
				{
					return {CELL_EFSSPECIFIC, path};
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
	return {CELL_EMFILE, path};
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

	std::lock_guard<std::mutex> lock(file->mp->mutex);

	if (file->lock)
	{
		return CELL_EBUSY;
	}

	*nwrite = file->op_write(buf, nbytes);

	return CELL_OK;
}

error_code sys_fs_close(u32 fd)
{
	sys_fs.trace("sys_fs_close(fd=%d)", fd);

	const auto file = idm::withdraw<lv2_fs_object, lv2_file>(fd, [](lv2_file& file) -> CellError
	{
		if (file.lock)
		{
			return CELL_EBUSY;
		}

		return {};
	});

	if (!file)
	{
		return CELL_EBADF;
	}

	if (file.ret)
	{
		return file.ret;
	}

	return CELL_OK;
}

error_code sys_fs_opendir(vm::cptr<char> path, vm::ptr<u32> fd)
{
	sys_fs.warning("sys_fs_opendir(path=%s, fd=*0x%x)", path, fd);

	const std::string& local_path = vfs::get(path.get_ptr());

	if (local_path.empty())
	{
		return {CELL_ENOTMOUNTED, path};
	}

	// TODO: other checks for path

	if (local_path == "/")
	{
		return {CELL_EPERM, path};
	}

	if (fs::is_file(local_path))
	{
		return {CELL_ENOTDIR, path};
	}

	fs::dir dir(local_path);

	if (!dir)
	{
		switch (auto error = fs::g_tls_error)
		{
		case fs::error::noent: return {CELL_ENOENT, path};
		default: sys_fs.error("sys_fs_opendir(): unknown error %s", error);
		}

		return {CELL_EIO, path};
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
		const std::string vfs_name = vfs::unescape(info.name);
		dir->d_type = info.is_directory ? CELL_FS_TYPE_DIRECTORY : CELL_FS_TYPE_REGULAR;
		dir->d_namlen = u8(std::min<size_t>(vfs_name.size(), CELL_FS_MAX_FS_FILE_NAME_LENGTH));
		strcpy_trunc(dir->d_name, vfs_name);
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

	if (!path)
		return CELL_EFAULT;

	const std::string local_path = vfs::get(path.get_ptr());

	if (local_path.empty())
	{
		return {CELL_ENOTMOUNTED, path};
	}

	fs::stat_t info{};

	if (local_path == "/")
	{
		sb->mode = CELL_FS_S_IFDIR | 0444;
		return CELL_OK;
	}

	if (!fs::stat(local_path, info))
	{
		switch (auto error = fs::g_tls_error)
		{
		case fs::error::noent: return {CELL_ENOENT, path};
		default: sys_fs.error("sys_fs_stat(): unknown error %s", error);
		}

		return {CELL_EIO, path};
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

error_code sys_fs_link(vm::cptr<char> from, vm::cptr<char> to)
{
	sys_fs.todo("sys_fs_link(from=%s, to=%s)", from, to);

	return CELL_OK;
}

error_code sys_fs_mkdir(vm::cptr<char> path, s32 mode)
{
	sys_fs.warning("sys_fs_mkdir(path=%s, mode=%#o)", path, mode);

	const std::string local_path = vfs::get(path.get_ptr());

	if (local_path.empty())
	{
		return {CELL_ENOTMOUNTED, path};
	}

	if (local_path == "/")
	{
		return {CELL_EEXIST, path};
	}

	if (!fs::create_path(local_path))
	{
		switch (auto error = fs::g_tls_error)
		{
		case fs::error::noent: return {CELL_ENOENT, path};
		case fs::error::exist: return {CELL_EEXIST, path};
		default: sys_fs.error("sys_fs_mkdir(): unknown error %s", error);
		}

		return {CELL_EIO, path}; // ???
	}

	sys_fs.notice("sys_fs_mkdir(): directory %s created", path);
	return CELL_OK;
}

error_code sys_fs_rename(vm::cptr<char> from, vm::cptr<char> to)
{
	sys_fs.warning("sys_fs_rename(from=%s, to=%s)", from, to);

	const std::string local_from = vfs::get(from.get_ptr());
	const std::string local_to = vfs::get(to.get_ptr());

	if (local_from.empty() || local_to.empty())
	{
		return CELL_ENOTMOUNTED;
	}

	if (local_to == "/" || local_from == "/")
	{
		return CELL_EPERM;
	}

	if (!fs::rename(local_from, local_to, false))
	{
		switch (auto error = fs::g_tls_error)
		{
		case fs::error::noent: return {CELL_ENOENT, from};
		case fs::error::exist: return {CELL_EEXIST, to};
		default: sys_fs.error("sys_fs_rename(): unknown error %s", error);
		}

		return {CELL_EIO, from}; // ???
	}

	sys_fs.notice("sys_fs_rename(): %s renamed to %s", from, to);
	return CELL_OK;
}

error_code sys_fs_rmdir(vm::cptr<char> path)
{
	sys_fs.warning("sys_fs_rmdir(path=%s)", path);

	const std::string local_path = vfs::get(path.get_ptr());

	if (local_path.empty())
	{
		return {CELL_ENOTMOUNTED, path};
	}

	if (local_path == "/")
	{
		return {CELL_EPERM, path};
	}

	if (!fs::remove_dir(local_path))
	{
		switch (auto error = fs::g_tls_error)
		{
		case fs::error::noent: return {CELL_ENOENT, path};
		case fs::error::notempty: return {CELL_ENOTEMPTY, path};
		default: sys_fs.error("sys_fs_rmdir(): unknown error %s", error);
		}

		return {CELL_EIO, path}; // ???
	}

	sys_fs.notice("sys_fs_rmdir(): directory %s removed", path);
	return CELL_OK;
}

error_code sys_fs_unlink(vm::cptr<char> path)
{
	sys_fs.warning("sys_fs_unlink(path=%s)", path);

	// If path is just an empty string
	if (!*path.get_ptr())
	{
		return CELL_ENOENT;
	}

	const std::string local_path = vfs::get(path.get_ptr());

	if (local_path.empty())
	{
		return {CELL_ENOTMOUNTED, path};
	}

	if (local_path == "/")
	{
		return {CELL_EISDIR, path};
	}

	if (!fs::remove_file(local_path))
	{
		switch (auto error = fs::g_tls_error)
		{
		case fs::error::noent: return {CELL_ENOENT, path};
		default: sys_fs.error("sys_fs_unlink(): unknown error %s", error);
		}

		return {CELL_EIO, path}; // ???
	}

	sys_fs.notice("sys_fs_unlink(): file %s deleted", path);
	return CELL_OK;
}

error_code sys_fs_access(vm::ps3::cptr<char> path, s32 mode)
{
	sys_fs.todo("sys_fs_access(path=%s, mode=%#o)", path, mode);

	return CELL_OK;
}

error_code sys_fs_fcntl(u32 fd, u32 op, vm::ptr<void> _arg, u32 _size)
{
	sys_fs.trace("sys_fs_fcntl(fd=%d, op=0x%x, arg=*0x%x, size=0x%x)", fd, op, _arg, _size);

	switch (op)
	{
	case 0x80000006: // cellFsAllocateFileAreaByFdWithInitialData
	{
		break;
	}

	case 0x80000007: // cellFsAllocateFileAreaByFdWithoutZeroFill
	{
		break;
	}

	case 0x80000008: // cellFsChangeFileSizeByFdWithoutAllocation
	{
		break;
	}

	case 0x8000000a: // cellFsReadWithOffset
	case 0x8000000b: // cellFsWriteWithOffset
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

		if (op == 0x8000000a && file->flags & CELL_FS_O_WRONLY)
		{
			return CELL_EBADF;
		}

		if (op == 0x8000000b && !(file->flags & CELL_FS_O_ACCMODE))
		{
			return CELL_EBADF;
		}

		std::lock_guard<std::mutex> lock(file->mp->mutex);

		if (op == 0x8000000b && file->lock)
		{
			return CELL_EBUSY;
		}

		const u64 old_pos = file->file.pos();
		const u64 new_pos = file->file.seek(arg->offset);

		arg->out_size = op == 0x8000000a
			? file->op_read(arg->buf, arg->size)
			: file->op_write(arg->buf, arg->size);

		verify(HERE), old_pos == file->file.seek(old_pos);

		arg->out_code = CELL_OK;
		return CELL_OK;
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
		if (const u32 id = idm::make<lv2_fs_object, lv2_file>(*file, std::move(stream), file->mode, file->flags))
		{
			arg->out_code = CELL_OK;
			arg->out_fd = id;
			return CELL_OK;
		}

		// Out of file descriptors
		return CELL_EMFILE;
	}

	case 0xc0000002: // cellFsGetFreeSize (TODO)
	{
		const auto arg = vm::static_ptr_cast<lv2_file_c0000002>(_arg);

		const std::string local_path = vfs::get(arg->path.get_ptr());

		if (local_path.empty())
		{
			return {CELL_ENOTMOUNTED, arg->path};
		}

		if (local_path == "/")
		{
			return {CELL_EPERM, arg->path};
		}

		fs::device_stat info;
		if (!fs::statfs(local_path, info))
		{
			switch (auto error = fs::g_tls_error)
			{
			case fs::error::noent: return {CELL_ENOENT, arg->path};
			default: sys_fs.error("sys_fs_fcntl(0xc0000002): unknown error %s", error);
			}

			return CELL_EIO; // ???
		}

		arg->out_code = CELL_OK;
		arg->out_block_size = 4096;
		arg->out_block_count = info.avail_free / 4096;
		return CELL_OK;
	}

	case 0xc0000006: // Unknown
	{
		const auto arg = vm::static_ptr_cast<lv2_file_c0000006>(_arg);

		sys_fs.warning("0xc0000006: 0x%x, 0x%x, 0x%x, %s, 0x%x, 0x%x, 0x%x", arg->size, arg->_x4, arg->_x8, arg->name, arg->_x14, arg->_x18, arg->_x1c);
		return CELL_OK;
	}

	case 0xc0000007: // cellFsArcadeHddSerialNumber
	{
		break;
	}

	case 0xc0000008: // cellFsSetDefaultContainer, cellFsSetIoBuffer, cellFsSetIoBufferFromDefaultContainer
	{
		break;
	}

	case 0xc0000015: // Unknown
	{
		break;
	}

	case 0xc0000016: // ps2disc_8160A811
	{
		break;
	}

	case 0xc000001a: // cellFsSetDiscReadRetrySetting, 5731DF45
	{
		break;
	}

	case 0xc0000021: // 9FDBBA89
	{
		break;
	}

	case 0xe0000000: // Unknown (cellFsGetBlockSize)
	{
		break;
	}

	case 0xe0000001: // Unknown (cellFsStat)
	{
		break;
	}

	case 0xe0000003: // Unknown
	{
		break;
	}

	case 0xe0000004: // Unknown
	{
		break;
	}

	case 0xe0000005: // Unknown (cellFsMkdir)
	{
		break;
	}

	case 0xe0000006: // Unknown
	{
		break;
	}

	case 0xe0000007: // Unknown
	{
		break;
	}

	case 0xe0000008: // Unknown (cellFsAclRead)
	{
		break;
	}

	case 0xe0000009: // Unknown (cellFsAccess)
	{
		break;
	}

	case 0xe000000a: // Unknown (E3D28395)
	{
		break;
	}

	case 0xe000000b: // Unknown (cellFsRename, FF29F478)
	{
		break;
	}

	case 0xe000000c: // Unknown (cellFsTruncate)
	{
		break;
	}

	case 0xe000000d: // Unknown (cellFsUtime)
	{
		break;
	}

	case 0xe000000e: // Unknown (cellFsAclWrite)
	{
		break;
	}

	case 0xe000000f: // Unknown (cellFsChmod)
	{
		break;
	}

	case 0xe0000010: // Unknown (cellFsChown)
	{
		break;
	}

	case 0xe0000011: // Unknown
	{
		break;
	}

	case 0xe0000012: // cellFsGetDirectoryEntries
	{
		const auto arg = vm::static_ptr_cast<lv2_file_op_dir::dir_info>(_arg);

		if (_size < arg.size())
		{
			return CELL_EINVAL;
		}

		const auto directory = idm::get<lv2_fs_object, lv2_dir>(fd);

		if (!directory)
		{
			return CELL_EBADF;
		}

		for (; arg->_size < arg->max; arg->_size++)
		{
			fs::dir_entry info;

			if (directory->dir.read(info))
			{
				auto& entry = arg->ptr[arg->_size];

				entry.attribute.mode = info.is_directory ? CELL_FS_S_IFDIR | 0777 : CELL_FS_S_IFREG | 0666;
				entry.attribute.uid = 0;
				entry.attribute.gid = 0;
				entry.attribute.atime = info.atime;
				entry.attribute.mtime = info.mtime;
				entry.attribute.ctime = info.ctime;
				entry.attribute.size = info.size;
				entry.attribute.blksize = 4096; // ???

				const std::string vfs_name = vfs::unescape(info.name);
				entry.entry_name.d_type = info.is_directory ? CELL_FS_TYPE_DIRECTORY : CELL_FS_TYPE_REGULAR;
				entry.entry_name.d_namlen = u8(std::min<size_t>(vfs_name.size(), CELL_FS_MAX_FS_FILE_NAME_LENGTH));
				strcpy_trunc(entry.entry_name.d_name, vfs_name);
			}
			else
			{
				break;
			}
		}

		arg->_code = CELL_OK;
		return CELL_OK;
	}

	case 0xe0000015: // Unknown
	{
		break;
	}

	case 0xe0000016: // cellFsAllocateFileAreaWithInitialData
	{
		break;
	}

	case 0xe0000017: // cellFsAllocateFileAreaWithoutZeroFill
	{
		const auto arg = vm::static_ptr_cast<lv2_file_e0000017>(_arg);

		if (_size < arg->size || arg->_x4 != 0x10 || arg->_x8 != 0x20)
		{
			return CELL_EINVAL;
		}

		arg->out_code = sys_fs_truncate(arg->file_path, arg->file_size);
		return CELL_OK;
	}

	case 0xe0000018: // cellFsChangeFileSizeWithoutAllocation
	{
		break;
	}

	case 0xe0000019: // Unknown
	{
		break;
	}

	case 0xe000001b: // Unknown
	{
		break;
	}

	case 0xe000001d: // Unknown
	{
		break;
	}

	case 0xe000001e: // Unknown
	{
		break;
	}

	case 0xe000001f: // Unknown
	{
		break;
	}

	case 0xe0000020: // Unknown
	{
		break;
	}

	case 0x00000025: // cellFsSdataOpenWithVersion
	{
		break;
	}
	}

	sys_fs.fatal("sys_fs_fcntl(): Unknown operation 0x%08x (fd=%d, arg=*0x%x, size=0x%x)", op, fd, _arg, _size);
	return CELL_OK;
}

error_code sys_fs_lseek(u32 fd, s64 offset, s32 whence, vm::ptr<u64> pos)
{
	sys_fs.trace("sys_fs_lseek(fd=%d, offset=0x%llx, whence=0x%x, pos=*0x%x)", fd, offset, whence, pos);

	if (whence >= 3)
	{
		return {CELL_EINVAL, whence};
	}

	const auto file = idm::get<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	std::lock_guard<std::mutex> lock(file->mp->mutex);

	const u64 result = file->file.seek(offset, static_cast<fs::seek_mode>(whence));

	if (result == -1)
	{
		switch (auto error = fs::g_tls_error)
		{
		case fs::error::inval: return CELL_EINVAL;
		default: sys_fs.error("sys_fs_lseek(): unknown error %s", error);
		}

		return CELL_EIO; // ???
	}

	*pos = result;
	return CELL_OK;
}

error_code sys_fs_fdatasync(u32 fd)
{
	sys_fs.trace("sys_fs_fdadasync(fd=%d)", fd);

	const auto file = idm::get<lv2_fs_object, lv2_file>(fd);

	if (!file || !(file->flags & CELL_FS_O_ACCMODE))
	{
		return CELL_EBADF;
	}

	file->file.sync();

	return CELL_OK;
}

error_code sys_fs_fsync(u32 fd)
{
	sys_fs.trace("sys_fs_fsync(fd=%d)", fd);

	const auto file = idm::get<lv2_fs_object, lv2_file>(fd);

	if (!file || !(file->flags & CELL_FS_O_ACCMODE))
	{
		return CELL_EBADF;
	}

	file->file.sync();

	return CELL_OK;
}

error_code sys_fs_fget_block_size(u32 fd, vm::ptr<u64> sector_size, vm::ptr<u64> block_size, vm::ptr<u64> arg4, vm::ptr<s32> arg5)
{
	sys_fs.warning("sys_fs_fget_block_size(fd=%d, sector_size=*0x%x, block_size=*0x%x, arg4=*0x%x, arg5=*0x%x)", fd, sector_size, block_size, arg4, arg5);

	const auto file = idm::get<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	// TODO
	*sector_size = 4096;
	*block_size = 4096;
	*arg4 = 0;
	*arg5 = file->mode;

	return CELL_OK;
}

error_code sys_fs_get_block_size(vm::cptr<char> path, vm::ptr<u64> sector_size, vm::ptr<u64> block_size, vm::ptr<u64> arg4)
{
	sys_fs.warning("sys_fs_get_block_size(path=%s, sector_size=*0x%x, block_size=*0x%x, arg4=*0x%x, arg5=*0x%x)", path, sector_size, block_size, arg4);

	// TODO
	*sector_size = 4096;
	*block_size = 4096;
	*arg4 = 0;

	return CELL_OK;
}

error_code sys_fs_truncate(vm::cptr<char> path, u64 size)
{
	sys_fs.warning("sys_fs_truncate(path=%s, size=0x%llx)", path, size);

	const std::string local_path = vfs::get(path.get_ptr());

	if (local_path.empty())
	{
		return {CELL_ENOTMOUNTED, path};
	}

	if (local_path == "/")
	{
		return {CELL_EISDIR, path};
	}

	if (!fs::truncate_file(local_path, size))
	{
		switch (auto error = fs::g_tls_error)
		{
		case fs::error::noent: return {CELL_ENOENT, path};
		default: sys_fs.error("sys_fs_truncate(): unknown error %s", error);
		}

		return {CELL_EIO, path}; // ???
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

	if (file->lock)
	{
		return CELL_EBUSY;
	}

	if (file->flags & CELL_FS_O_APPEND)
	{
		const u64 fsize = file->file.size();

		if (size > fsize && !file->file.write(std::vector<u8>(size - fsize)))
		{
			return CELL_ENOSPC;
		}
	}
	else if (!file->file.trunc(size))
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

error_code sys_fs_symbolic_link(vm::cptr<char> target, vm::cptr<char> linkpath)
{
	sys_fs.todo("sys_fs_symbolic_link(target=%s, linkpath=%s)", target, linkpath);

	return CELL_OK;
}

error_code sys_fs_chmod(vm::cptr<char> path, s32 mode)
{
	sys_fs.todo("sys_fs_chmod(path=%s, mode=%#o)", path, mode);

	return CELL_OK;
}

error_code sys_fs_chown(vm::cptr<char> path, s32 uid, s32 gid)
{
	sys_fs.todo("sys_fs_chown(path=%s, uid=%d, gid=%d)", path, uid, gid);

	return CELL_OK;
}

error_code sys_fs_disk_free(vm::ps3::cptr<char> path, vm::ptr<u64> total_free, vm::ptr<u64> avail_free)
{
	sys_fs.warning("sys_fs_disk_free(path=%s total_free=*0x%x avail_free=*0x%x)", path, total_free, avail_free);

	const std::string local_path = vfs::get(path.get_ptr());

	if (local_path.empty())
	{
		return {CELL_ENOTMOUNTED, path};
	}

	if (local_path == "/")
	{
		return {CELL_EPERM, path};
	}

	fs::device_stat info;
	if (!fs::statfs(local_path, info))
	{
		switch (auto error = fs::g_tls_error)
		{
		case fs::error::noent: return {CELL_ENOENT, path};
		default: sys_fs.error("sys_fs_disk_free(): unknown error %s", error);
		}

		return {CELL_EIO, path};  // ???
	}

	*total_free = info.total_free;
	*avail_free = info.avail_free; //Only value used by cellFsGetFreeSize

	return CELL_OK;
}

error_code sys_fs_utime(vm::ps3::cptr<char> path, vm::ps3::cptr<CellFsUtimbuf> timep)
{
	sys_fs.warning("sys_fs_utime(path=%s, timep=*0x%x)", path, timep);
	sys_fs.warning("** actime=%u, modtime=%u", timep->actime, timep->modtime);

	const std::string local_path = vfs::get(path.get_ptr());

	if (local_path.empty())
	{
		return {CELL_ENOTMOUNTED, path};
	}

	if (local_path == "/")
	{
		return {CELL_EISDIR, path};
	}

	if (!fs::utime(local_path, timep->actime, timep->modtime))
	{
		switch (auto error = fs::g_tls_error)
		{
		case fs::error::noent: return {CELL_ENOENT, path};
		default: sys_fs.error("sys_fs_utime(): unknown error %s", error);
		}

		return {CELL_EIO, path}; // ???
	}

	return CELL_OK;
}

error_code sys_fs_acl_read(vm::ps3::cptr<char> path, vm::ps3::ptr<void> ptr)
{
	sys_fs.todo("sys_fs_acl_read(path=%s, ptr=*0x%x)", path, ptr);

	return CELL_OK;
}

error_code sys_fs_acl_write(vm::ps3::cptr<char> path, vm::ps3::ptr<void> ptr)
{
	sys_fs.todo("sys_fs_acl_write(path=%s, ptr=*0x%x)", path, ptr);

	return CELL_OK;
}

error_code sys_fs_lsn_get_cda_size(u32 fd, vm::ps3::ptr<u64> ptr)
{
	sys_fs.warning("sys_fs_lsn_get_cda_size(fd=%d, ptr=*0x%x)", fd, ptr);

	const auto file = idm::get<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	// TODO
	*ptr = 0;
	return CELL_OK;
}

error_code sys_fs_lsn_get_cda(u32 fd, vm::ps3::ptr<void> arg2, u64 arg3, vm::ps3::ptr<u64> arg4)
{
	sys_fs.todo("sys_fs_lsn_get_cda(fd=%d, arg2=*0x%x, arg3=0x%x, arg4=*0x%x)", fd, arg2, arg3, arg4);

	return CELL_OK;
}

error_code sys_fs_lsn_lock(u32 fd)
{
	sys_fs.trace("sys_fs_lsn_lock(fd=%d)", fd);

	const auto file = idm::get<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	// TODO: research correct implementation
	if (!file->lock.compare_and_swap_test(0, 1))
	{
		return CELL_EBUSY;
	}

	return CELL_OK;
}

error_code sys_fs_lsn_unlock(u32 fd)
{
	sys_fs.trace("sys_fs_lsn_unlock(fd=%d)", fd);

	const auto file = idm::get<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	// TODO: research correct implementation
	if (!file->lock.compare_and_swap_test(1, 0))
	{
		return CELL_EPERM;
	}

	return CELL_OK;
}

error_code sys_fs_lsn_read(u32 fd, vm::ps3::cptr<void> ptr, u64 size)
{
	sys_fs.todo("sys_fs_lsn_read(fd=%d, ptr=*0x%x, size=0x%x)", fd, ptr, size);

	return CELL_OK;
}

error_code sys_fs_lsn_write(u32 fd, vm::ps3::cptr<void> ptr, u64 size)
{
	sys_fs.todo("sys_fs_lsn_write(fd=%d, ptr=*0x%x, size=0x%x)", fd, ptr, size);

	return CELL_OK;
}

error_code sys_fs_mapped_allocate(u32 fd, u64 size, vm::ps3::pptr<void> out_ptr)
{
	sys_fs.todo("sys_fs_mapped_allocate(fd=%d, arg2=0x%x, out_ptr=**0x%x)", fd, size, out_ptr);

	return CELL_OK;
}

error_code sys_fs_mapped_free(u32 fd, vm::ps3::ptr<void> ptr)
{
	sys_fs.todo("sys_fs_mapped_free(fd=%d, ptr=0x%#x)", fd, ptr);

	return CELL_OK;
}

error_code sys_fs_truncate2(u32 fd, u64 size)
{
	sys_fs.todo("sys_fs_truncate2(fd=%d, size=0x%x)", fd, size);

	return CELL_OK;
}
