#include "stdafx.h"
#include "Emu/VFS.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"

#include "Emu/Cell/lv2/sys_fs.h"
#include "Emu/Cell/lv2/sys_sync.h"
#include "cellFs.h"

#include <mutex>

LOG_CHANNEL(cellFs);

error_code cellFsGetPath(ppu_thread& ppu, u32 fd, vm::ptr<char> out_path)
{
	cellFs.trace("cellFsGetPath(fd=%d, out_path=*0x%x)", fd, out_path);

	if (!out_path)
	{
		return CELL_EFAULT;
	}

	return sys_fs_test(ppu, 6, 0, vm::var<u32>{fd}, sizeof(u32), out_path, 0x420);
}

error_code cellFsOpen(ppu_thread& ppu, vm::cptr<char> path, s32 flags, vm::ptr<u32> fd, vm::cptr<void> arg, u64 size)
{
	cellFs.trace("cellFsOpen(path=%s, flags=%#o, fd=*0x%x, arg=*0x%x, size=0x%llx)", path, flags, fd, arg, size);

	if (!fd)
	{
		return CELL_EFAULT;
	}

	// TODO
	return sys_fs_open(ppu, path, flags, fd, flags & CELL_FS_O_CREAT ? CELL_FS_S_IRUSR | CELL_FS_S_IWUSR : 0, arg, size);
}

error_code cellFsOpen2()
{
	UNIMPLEMENTED_FUNC(cellFs);
	return CELL_OK;
}

error_code cellFsSdataOpen(ppu_thread& ppu, vm::cptr<char> path, s32 flags, vm::ptr<u32> fd, vm::cptr<void> arg, u64 size)
{
	cellFs.trace("cellFsSdataOpen(path=%s, flags=%#o, fd=*0x%x, arg=*0x%x, size=0x%llx)", path, flags, fd, arg, size);

	if (flags != CELL_FS_O_RDONLY)
	{
		return CELL_EINVAL;
	}

	return cellFsOpen(ppu, path, CELL_FS_O_RDONLY, fd, vm::make_var<be_t<u32>[2]>({0x180, 0x10}), 8);
}

error_code cellFsRead(ppu_thread& ppu, u32 fd, vm::ptr<void> buf, u64 nbytes, vm::ptr<u64> nread)
{
	cellFs.trace("cellFsRead(fd=0x%x, buf=0x%x, nbytes=0x%llx, nread=0x%x)", fd, buf, nbytes, nread);

	return sys_fs_read(ppu, fd, buf, nbytes, nread ? nread : vm::var<u64>{});
}

error_code cellFsWrite(ppu_thread& ppu, u32 fd, vm::cptr<void> buf, u64 nbytes, vm::ptr<u64> nwrite)
{
	cellFs.trace("cellFsWrite(fd=0x%x, buf=*0x%x, nbytes=0x%llx, nwrite=*0x%x)", fd, buf, nbytes, nwrite);

	return sys_fs_write(ppu, fd, buf, nbytes, nwrite ? nwrite : vm::var<u64>{});
}

error_code cellFsClose(ppu_thread& ppu, u32 fd)
{
	cellFs.trace("cellFsClose(fd=0x%x)", fd);

	return sys_fs_close(ppu, fd);
}

error_code cellFsOpendir(ppu_thread& ppu, vm::cptr<char> path, vm::ptr<u32> fd)
{
	cellFs.trace("cellFsOpendir(path=%s, fd=*0x%x)", path, fd);

	if (!fd)
	{
		return CELL_EFAULT;
	}

	// TODO
	return sys_fs_opendir(ppu, path, fd);
}

error_code cellFsReaddir(ppu_thread& ppu, u32 fd, vm::ptr<CellFsDirent> dir, vm::ptr<u64> nread)
{
	cellFs.trace("cellFsReaddir(fd=0x%x, dir=*0x%x, nread=*0x%x)", fd, dir, nread);

	if (!dir || !nread)
	{
		return CELL_EFAULT;
	}

	return sys_fs_readdir(ppu, fd, dir, nread);
}

error_code cellFsClosedir(ppu_thread& ppu, u32 fd)
{
	cellFs.trace("cellFsClosedir(fd=0x%x)", fd);

	return sys_fs_closedir(ppu, fd);
}

error_code cellFsStat(ppu_thread& ppu, vm::cptr<char> path, vm::ptr<CellFsStat> sb)
{
	cellFs.trace("cellFsStat(path=%s, sb=*0x%x)", path, sb);

	if (!sb)
	{
		return CELL_EFAULT;
	}

	// TODO
	return sys_fs_stat(ppu, path, sb);
}

error_code cellFsFstat(ppu_thread& ppu, u32 fd, vm::ptr<CellFsStat> sb)
{
	cellFs.trace("cellFsFstat(fd=0x%x, sb=*0x%x)", fd, sb);

	return sys_fs_fstat(ppu, fd, sb);
}

error_code cellFsLink()
{
	UNIMPLEMENTED_FUNC(cellFs);
	return CELL_OK;
}

error_code cellFsMkdir(ppu_thread& ppu, vm::cptr<char> path, s32 mode)
{
	cellFs.trace("cellFsMkdir(path=%s, mode=%#o)", path, mode);

	// TODO
	return sys_fs_mkdir(ppu, path, mode);
}

error_code cellFsRename(ppu_thread& ppu, vm::cptr<char> from, vm::cptr<char> to)
{
	cellFs.trace("cellFsRename(from=%s, to=%s)", from, to);

	// TODO
	return sys_fs_rename(ppu, from, to);
}

error_code cellFsRmdir(ppu_thread& ppu, vm::cptr<char> path)
{
	cellFs.trace("cellFsRmdir(path=%s)", path);

	// TODO
	return sys_fs_rmdir(ppu, path);
}

error_code cellFsUnlink(ppu_thread& ppu, vm::cptr<char> path)
{
	cellFs.trace("cellFsUnlink(path=%s)", path);

	// TODO
	return sys_fs_unlink(ppu, path);
}

error_code cellFsUtime(ppu_thread& ppu, vm::cptr<char> path, vm::cptr<CellFsUtimbuf> timep)
{
	cellFs.trace("cellFsUtime(path=%s, timep=*0x%x)", path, timep);

	if (!timep)
	{
		return CELL_EFAULT;
	}

	// TODO
	return sys_fs_utime(ppu, path, timep);
}

error_code cellFsAccess()
{
	UNIMPLEMENTED_FUNC(cellFs);
	return CELL_OK;
}

error_code cellFsFcntl()
{
	UNIMPLEMENTED_FUNC(cellFs);
	return CELL_OK;
}

error_code cellFsLseek(ppu_thread& ppu, u32 fd, s64 offset, u32 whence, vm::ptr<u64> pos)
{
	cellFs.trace("cellFsLseek(fd=0x%x, offset=0x%llx, whence=0x%x, pos=*0x%x)", fd, offset, whence, pos);

	if (!pos)
	{
		return CELL_EFAULT;
	}

	return sys_fs_lseek(ppu, fd, offset, whence, pos);
}

error_code cellFsFdatasync(ppu_thread& ppu, u32 fd)
{
	cellFs.trace("cellFsFdatasync(fd=%d)", fd);

	return sys_fs_fdatasync(ppu, fd);
}

error_code cellFsFsync(ppu_thread& ppu, u32 fd)
{
	cellFs.trace("cellFsFsync(fd=%d)", fd);

	return sys_fs_fsync(ppu, fd);
}

error_code cellFsFGetBlockSize(ppu_thread& ppu, u32 fd, vm::ptr<u64> sector_size, vm::ptr<u64> block_size)
{
	cellFs.trace("cellFsFGetBlockSize(fd=0x%x, sector_size=*0x%x, block_size=*0x%x)", fd, sector_size, block_size);

	if (!sector_size || !block_size)
	{
		return CELL_EFAULT;
	}

	return sys_fs_fget_block_size(ppu, fd, sector_size, block_size, vm::var<u64>{}, vm::var<s32>{});
}

error_code cellFsFGetBlockSize2()
{
	UNIMPLEMENTED_FUNC(cellFs);
	return CELL_OK;
}

error_code cellFsGetBlockSize(ppu_thread& ppu, vm::cptr<char> path, vm::ptr<u64> sector_size, vm::ptr<u64> block_size)
{
	cellFs.trace("cellFsGetBlockSize(path=%s, sector_size=*0x%x, block_size=*0x%x)", path, sector_size, block_size);

	if (!path || !sector_size || !block_size)
	{
		return CELL_EFAULT;
	}

	// TODO
	return sys_fs_get_block_size(ppu, path, sector_size, block_size, vm::var<u64>{});
}

error_code cellFsGetBlockSize2()
{
	UNIMPLEMENTED_FUNC(cellFs);
	return CELL_OK;
}

error_code cellFsAclRead()
{
	UNIMPLEMENTED_FUNC(cellFs);
	return CELL_OK;
}

error_code cellFsAclWrite()
{
	UNIMPLEMENTED_FUNC(cellFs);
	return CELL_OK;
}

error_code cellFsLsnGetCDASize()
{
	UNIMPLEMENTED_FUNC(cellFs);
	return CELL_OK;
}

error_code cellFsLsnGetCDA()
{
	UNIMPLEMENTED_FUNC(cellFs);
	return CELL_OK;
}

error_code cellFsLsnLock()
{
	UNIMPLEMENTED_FUNC(cellFs);
	return CELL_OK;
}

error_code cellFsLsnUnlock()
{
	UNIMPLEMENTED_FUNC(cellFs);
	return CELL_OK;
}

error_code cellFsLsnRead()
{
	UNIMPLEMENTED_FUNC(cellFs);
	return CELL_OK;
}

error_code cellFsLsnRead2()
{
	UNIMPLEMENTED_FUNC(cellFs);
	return CELL_OK;
}

error_code cellFsTruncate(ppu_thread& ppu, vm::cptr<char> path, u64 size)
{
	cellFs.trace("cellFsTruncate(path=%s, size=0x%llx)", path, size);

	// TODO
	return sys_fs_truncate(ppu, path, size);
}

error_code cellFsFtruncate(ppu_thread& ppu, u32 fd, u64 size)
{
	cellFs.trace("cellFsFtruncate(fd=0x%x, size=0x%llx)", fd, size);

	return sys_fs_ftruncate(ppu, fd, size);
}

error_code cellFsSymbolicLink()
{
	UNIMPLEMENTED_FUNC(cellFs);
	return CELL_OK;
}

error_code cellFsChmod(ppu_thread& ppu, vm::cptr<char> path, s32 mode)
{
	cellFs.trace("cellFsChmod(path=%s, mode=%#o)", path, mode);

	// TODO
	return sys_fs_chmod(ppu, path, mode);
}

error_code cellFsChown()
{
	UNIMPLEMENTED_FUNC(cellFs);
	return CELL_OK;
}

error_code cellFsGetFreeSize(ppu_thread& ppu, vm::cptr<char> path, vm::ptr<u32> block_size, vm::ptr<u64> block_count)
{
	cellFs.todo("cellFsGetFreeSize(path=%s, block_size=*0x%x, block_count=*0x%x)", path, block_size, block_count);

	if (!path || !block_size || !block_count)
	{
		return CELL_EFAULT;
	}

	*block_size = 0;
	*block_count = 0;

	vm::var<lv2_file_c0000002> op;
	op->_vtable = vm::cast(0xfae12000);
	op->op = 0xC0000002;
	op->out_code = 0x80010003;
	op->path = path;

	if (!std::strncmp(path.get_ptr(), "/dev_hdd0", 9))
	{
		sys_fs_fcntl(ppu, -1, 0xC0000002, op, sizeof(*op));
		*block_count = op->out_block_count;
		*block_size = op->out_block_size;
	}
	else
	{
		vm::var<u64> _block_size, avail;

		if (auto err = sys_fs_disk_free(ppu, path, vm::var<u64>{}, avail))
		{
			if (err + 0u == CELL_EPERM)
			{
				return not_an_error(CELL_EINVAL);
			}

			return err;
		}

		if (!*avail)
		{
			return CELL_ENOTDIR;
		}

		if (auto err = cellFsGetBlockSize(ppu, path, vm::var<u64>{}, _block_size))
		{
			return err;
		}

		*block_count = *avail / *_block_size;
		*block_size = ::narrow<u32>(*_block_size);
	}

	return CELL_OK;
}

error_code cellFsMappedAllocate()
{
	UNIMPLEMENTED_FUNC(cellFs);
	return CELL_OK;
}

error_code cellFsMappedFree()
{
	UNIMPLEMENTED_FUNC(cellFs);
	return CELL_OK;
}

error_code cellFsTruncate2()
{
	UNIMPLEMENTED_FUNC(cellFs);
	return CELL_OK;
}

error_code cellFsGetDirectoryEntries(ppu_thread& ppu, u32 fd, vm::ptr<CellFsDirectoryEntry> entries, u32 entries_size, vm::ptr<u32> data_count)
{
	cellFs.trace("cellFsGetDirectoryEntries(fd=%d, entries=*0x%x, entries_size=0x%x, data_count=*0x%x)", fd, entries, entries_size, data_count);

	if (!data_count || !entries)
	{
		return CELL_EFAULT;
	}

	if (fd - 3 > 252)
	{
		return CELL_EBADF;
	}

	vm::var<lv2_file_op_dir> op;

	op->_vtable = vm::cast(0xfae12000); // Intentionally wrong (provide correct vtable if necessary)

	op->op = 0xe0000012;
	op->arg._code = 0;
	op->arg._size = 0;
	op->arg.ptr   = entries;
	op->arg.max   = entries_size / sizeof(CellFsDirectoryEntry);

	const s32 rc = sys_fs_fcntl(ppu, fd, 0xe0000012, op.ptr(&lv2_file_op_dir::arg), 0x10);

	*data_count = op->arg._size;

	if (!rc && op->arg._code)
	{
		return CellError(+op->arg._code);
	}

	return not_an_error(rc);
}

error_code cellFsReadWithOffset(ppu_thread& ppu, u32 fd, u64 offset, vm::ptr<void> buf, u64 buffer_size, vm::ptr<u64> nread)
{
	cellFs.trace("cellFsReadWithOffset(fd=%d, offset=0x%llx, buf=*0x%x, buffer_size=0x%llx, nread=*0x%x)", fd, offset, buf, buffer_size, nread);

	if (fd - 3 > 252)
	{
		if (nread) *nread = 0;
		return CELL_EBADF;
	}

	vm::var<lv2_file_op_rw> arg;

	arg->_vtable = vm::cast(0xfa8a0000); // Intentionally wrong (provide correct vtable if necessary)

	arg->op = 0x8000000a;
	arg->fd = fd;
	arg->buf = buf;
	arg->offset = offset;
	arg->size = buffer_size;

	const s32 rc = sys_fs_fcntl(ppu, fd, 0x8000000a, arg, arg.size());

	// Write size read
	if (nread)
	{
		*nread = rc && rc + 0u != CELL_EFSSPECIFIC ? 0 : arg->out_size.value();
	}

	if (!rc && arg->out_code)
	{
		return CellError(+arg->out_code);
	}

	return not_an_error(rc);
}

error_code cellFsWriteWithOffset(ppu_thread& ppu, u32 fd, u64 offset, vm::cptr<void> buf, u64 data_size, vm::ptr<u64> nwrite)
{
	cellFs.trace("cellFsWriteWithOffset(fd=%d, offset=0x%llx, buf=*0x%x, data_size=0x%llx, nwrite=*0x%x)", fd, offset, buf, data_size, nwrite);

	if (!buf)
	{
		if (nwrite) *nwrite = 0;
		return CELL_EFAULT;
	}

	if (fd - 3 > 252)
	{
		if (nwrite) *nwrite = 0;
		return CELL_EBADF;
	}

	vm::var<lv2_file_op_rw> arg;

	arg->_vtable = vm::cast(0xfa8b0000); // Intentionally wrong (provide correct vtable if necessary)

	arg->op = 0x8000000b;
	arg->fd = fd;
	arg->buf = vm::const_ptr_cast<void>(buf);
	arg->offset = offset;
	arg->size = data_size;

	const s32 rc = sys_fs_fcntl(ppu, fd, 0x8000000b, arg, arg.size());

	// Write size written
	if (nwrite)
	{
		*nwrite = rc && rc + 0u != CELL_EFSSPECIFIC ? 0 : arg->out_size.value();
	}

	if (!rc && arg->out_code)
	{
		return CellError(+arg->out_code);
	}

	return not_an_error(rc);
}

error_code cellFsSdataOpenByFd(ppu_thread& ppu, u32 mself_fd, s32 flags, vm::ptr<u32> sdata_fd, u64 offset, vm::cptr<void> arg, u64 size)
{
	cellFs.trace("cellFsSdataOpenByFd(mself_fd=0x%x, flags=%#o, sdata_fd=*0x%x, offset=0x%llx, arg=*0x%x, size=0x%llx)", mself_fd, flags, sdata_fd, offset, arg, size);

	if (!sdata_fd)
	{
		return CELL_EFAULT;
	}

	*sdata_fd = -1;

	if (mself_fd < 3 || mself_fd > 255)
	{
		return CELL_EBADF;
	}

	if (flags)
	{
		return CELL_EINVAL;
	}

	vm::var<lv2_file_op_09> ctrl;
	ctrl->_vtable = vm::cast(0xfa880000); // Intentionally wrong (provide correct vtable if necessary)
	ctrl->op = 0x80000009;
	ctrl->fd = mself_fd;
	ctrl->offset = offset;
	ctrl->_vtabl2 = vm::cast(0xfa880020);
	ctrl->arg1 = 0x180;
	ctrl->arg2 = 0x10;
	ctrl->arg_ptr = arg.addr();
	ctrl->arg_size = u32(size);

	if (const s32 rc = sys_fs_fcntl(ppu, mself_fd, 0x80000009, ctrl, 0x40))
	{
		return not_an_error(rc);
	}

	if (const s32 rc = ctrl->out_code)
	{
		return CellError(rc);
	}

	*sdata_fd = ctrl->out_fd;
	return CELL_OK;
}

error_code cellFsSdataOpenWithVersion()
{
	UNIMPLEMENTED_FUNC(cellFs);
	return CELL_OK;
}

error_code cellFsSetAttribute()
{
	UNIMPLEMENTED_FUNC(cellFs);
	return CELL_OK;
}

error_code cellFsSetDefaultContainer(u32 id, u32 total_limit)
{
	cellFs.todo("cellFsSetDefaultContainer(id=0x%x, total_limit=%u)", id, total_limit);

	return CELL_OK;
}

error_code cellFsSetIoBuffer()
{
	UNIMPLEMENTED_FUNC(cellFs);
	return CELL_OK;
}

error_code cellFsSetIoBufferFromDefaultContainer(u32 fd, u32 buffer_size, u32 page_type)
{
	cellFs.todo("cellFsSetIoBufferFromDefaultContainer(fd=%d, buffer_size=%d, page_type=%d)", fd, buffer_size, page_type);

	const auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	return CELL_OK;
}

error_code cellFsAllocateFileAreaWithInitialData()
{
	UNIMPLEMENTED_FUNC(cellFs);
	return CELL_OK;
}

error_code cellFsAllocateFileAreaByFdWithoutZeroFill()
{
	UNIMPLEMENTED_FUNC(cellFs);
	return CELL_OK;
}

error_code cellFsAllocateFileAreaByFdWithInitialData()
{
	UNIMPLEMENTED_FUNC(cellFs);
	return CELL_OK;
}

error_code cellFsAllocateFileAreaWithoutZeroFill(ppu_thread& ppu, vm::cptr<char> path, u64 size)
{
	cellFs.trace("cellFsAllocateFileAreaWithoutZeroFill(path=%s, size=0x%llx)", path, size);

	if (!path)
	{
		return CELL_EFAULT;
	}

	vm::var<lv2_file_e0000017> ctrl;
	ctrl->size = ctrl.size();
	ctrl->_x4  = 0x10;
	ctrl->_x8  = 0x20;

	ctrl->file_path = path;
	ctrl->file_size = size;
	ctrl->out_code  = CELL_ENOSYS;

	// TODO
	if (s32 rc = sys_fs_fcntl(ppu, -1, 0xe0000017, ctrl, ctrl->size))
	{
		return not_an_error(rc);
	}

	if (s32 rc = ctrl->out_code)
	{
		return CellError(rc);
	}

	return CELL_OK;
}

error_code cellFsChangeFileSizeWithoutAllocation()
{
	UNIMPLEMENTED_FUNC(cellFs);
	return CELL_OK;
}

error_code cellFsChangeFileSizeByFdWithoutAllocation()
{
	UNIMPLEMENTED_FUNC(cellFs);
	return CELL_OK;
}

error_code cellFsSetDiscReadRetrySetting()
{
	UNIMPLEMENTED_FUNC(cellFs);
	return CELL_OK;
}

s32 cellFsStReadInit(u32 fd, vm::cptr<CellFsRingBuffer> ringbuf)
{
	cellFs.todo("cellFsStReadInit(fd=%d, ringbuf=*0x%x)", fd, ringbuf);

	if (ringbuf->copy & ~CELL_FS_ST_COPYLESS)
	{
		return CELL_EINVAL;
	}

	if (ringbuf->block_size & 0xfff) // check if a multiple of sector size
	{
		return CELL_EINVAL;
	}

	if (ringbuf->ringbuf_size % ringbuf->block_size) // check if a multiple of block_size
	{
		return CELL_EINVAL;
	}

	const auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	if (file->flags & CELL_FS_O_WRONLY)
	{
		return CELL_EPERM;
	}

	// TODO

	return CELL_OK;
}

s32 cellFsStReadFinish(u32 fd)
{
	cellFs.todo("cellFsStReadFinish(fd=%d)", fd);

	const auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF; // ???
	}

	// TODO

	return CELL_OK;
}

s32 cellFsStReadGetRingBuf(u32 fd, vm::ptr<CellFsRingBuffer> ringbuf)
{
	cellFs.todo("cellFsStReadGetRingBuf(fd=%d, ringbuf=*0x%x)", fd, ringbuf);

	const auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	// TODO

	return CELL_OK;
}

s32 cellFsStReadGetStatus(u32 fd, vm::ptr<u64> status)
{
	cellFs.todo("cellFsStReadGetRingBuf(fd=%d, status=*0x%x)", fd, status);

	const auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	// TODO

	return CELL_OK;
}

s32 cellFsStReadGetRegid(u32 fd, vm::ptr<u64> regid)
{
	cellFs.todo("cellFsStReadGetRingBuf(fd=%d, regid=*0x%x)", fd, regid);

	const auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	// TODO

	return CELL_OK;
}

s32 cellFsStReadStart(u32 fd, u64 offset, u64 size)
{
	cellFs.todo("cellFsStReadStart(fd=%d, offset=0x%llx, size=0x%llx)", fd, offset, size);

	const auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	// TODO

	return CELL_OK;
}

s32 cellFsStReadStop(u32 fd)
{
	cellFs.todo("cellFsStReadStop(fd=%d)", fd);

	const auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	// TODO

	return CELL_OK;
}

s32 cellFsStRead(u32 fd, vm::ptr<u8> buf, u64 size, vm::ptr<u64> rsize)
{
	cellFs.todo("cellFsStRead(fd=%d, buf=*0x%x, size=0x%llx, rsize=*0x%x)", fd, buf, size, rsize);

	const auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	// TODO

	return CELL_OK;
}

s32 cellFsStReadGetCurrentAddr(u32 fd, vm::ptr<u32> addr, vm::ptr<u64> size)
{
	cellFs.todo("cellFsStReadGetCurrentAddr(fd=%d, addr=*0x%x, size=*0x%x)", fd, addr, size);

	const auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	// TODO

	return CELL_OK;
}

s32 cellFsStReadPutCurrentAddr(u32 fd, vm::ptr<u8> addr, u64 size)
{
	cellFs.todo("cellFsStReadPutCurrentAddr(fd=%d, addr=*0x%x, size=0x%llx)", fd, addr, size);

	const auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	// TODO

	return CELL_OK;
}

s32 cellFsStReadWait(u32 fd, u64 size)
{
	cellFs.todo("cellFsStReadWait(fd=%d, size=0x%llx)", fd, size);

	const auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	// TODO

	return CELL_OK;
}

s32 cellFsStReadWaitCallback(u32 fd, u64 size, vm::ptr<void(s32 xfd, u64 xsize)> func)
{
	cellFs.todo("cellFsStReadWaitCallback(fd=%d, size=0x%llx, func=*0x%x)", fd, size, func);

	const auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	// TODO

	return CELL_OK;
}

using fs_aio_cb_t = vm::ptr<void(vm::ptr<CellFsAio> xaio, s32 error, s32 xid, u64 size)>;

struct fs_aio_thread : ppu_thread
{
	using ppu_thread::ppu_thread;

	void non_task()
	{
		while (cmd64 cmd = cmd_wait())
		{
			const u32 type = cmd.arg1<u32>();
			const s32 xid = cmd.arg2<s32>();
			const cmd64 cmd2 = cmd_get(1);
			const auto aio = cmd2.arg1<vm::ptr<CellFsAio>>();
			const auto func = cmd2.arg2<fs_aio_cb_t>();
			cmd_pop(1);

			s32 error = CELL_EBADF;
			u64 result = 0;

			const auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(aio->fd);

			if (!file || (type == 1 && file->flags & CELL_FS_O_WRONLY) || (type == 2 && !(file->flags & CELL_FS_O_ACCMODE)))
			{
			}
			else if (std::lock_guard lock(file->mp->mutex); file->file)
			{
				const auto old_pos = file->file.pos(); file->file.seek(aio->offset);

				result = type == 2
					? file->op_write(aio->buf, aio->size)
					: file->op_read(aio->buf, aio->size);

				file->file.seek(old_pos);
				error = CELL_OK;
			}

			func(*this, aio, error, xid, result);
			lv2_obj::sleep(*this);
		}
	}
};

struct fs_aio_manager
{
	std::shared_ptr<fs_aio_thread> thread;

	shared_mutex mutex;
};

s32 cellFsAioInit(vm::cptr<char> mount_point)
{
	cellFs.warning("cellFsAioInit(mount_point=%s)", mount_point);

	// TODO: create AIO thread (if not exists) for specified mount point
	cellFs.fatal("cellFsAio disabled, use LLE.");

	return CELL_OK;
}

s32 cellFsAioFinish(vm::cptr<char> mount_point)
{
	cellFs.warning("cellFsAioFinish(mount_point=%s)", mount_point);

	// TODO: delete existing AIO thread for specified mount point

	return CELL_OK;
}

atomic_t<s32> g_fs_aio_id;

s32 cellFsAioRead(vm::ptr<CellFsAio> aio, vm::ptr<s32> id, fs_aio_cb_t func)
{
	cellFs.warning("cellFsAioRead(aio=*0x%x, id=*0x%x, func=*0x%x)", aio, id, func);

	// TODO: detect mount point and send AIO request to the AIO thread of this mount point

	auto& m = g_fxo->get<fs_aio_manager>();

	if (!m.thread)
	{
		return CELL_ENXIO;
	}

	const s32 xid = (*id = ++g_fs_aio_id);

	m.thread->cmd_list
	({
		{ 1, xid },
		{ aio, func },
	});

	return CELL_OK;
}

s32 cellFsAioWrite(vm::ptr<CellFsAio> aio, vm::ptr<s32> id, fs_aio_cb_t func)
{
	cellFs.warning("cellFsAioWrite(aio=*0x%x, id=*0x%x, func=*0x%x)", aio, id, func);

	// TODO: detect mount point and send AIO request to the AIO thread of this mount point

	auto& m = g_fxo->get<fs_aio_manager>();

	if (!m.thread)
	{
		return CELL_ENXIO;
	}

	const s32 xid = (*id = ++g_fs_aio_id);

	m.thread->cmd_list
	({
		{ 2, xid },
		{ aio, func },
	});

	return CELL_OK;
}

s32 cellFsAioCancel(s32 id)
{
	cellFs.todo("cellFsAioCancel(id=%d) -> CELL_EINVAL", id);

	// TODO: cancelled requests return CELL_ECANCELED through their own callbacks

	return CELL_EINVAL;
}

s32 cellFsArcadeHddSerialNumber()
{
	cellFs.todo("cellFsArcadeHddSerialNumber()");
	return CELL_OK;
}

s32 cellFsRegisterConversionCallback()
{
	cellFs.todo("cellFsRegisterConversionCallback()");
	return CELL_OK;
}

s32 cellFsUnregisterL10nCallbacks()
{
	cellFs.todo("cellFsUnregisterL10nCallbacks()");
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellFs)("sys_fs", []()
{
	REG_FUNC(sys_fs, cellFsAccess);
	REG_FUNC(sys_fs, cellFsAclRead);
	REG_FUNC(sys_fs, cellFsAclWrite);
	REG_FUNC(sys_fs, cellFsAioCancel);
	REG_FUNC(sys_fs, cellFsAioFinish);
	REG_FUNC(sys_fs, cellFsAioInit);
	REG_FUNC(sys_fs, cellFsAioRead);
	REG_FUNC(sys_fs, cellFsAioWrite);
	REG_FUNC(sys_fs, cellFsAllocateFileAreaByFdWithInitialData);
	REG_FUNC(sys_fs, cellFsAllocateFileAreaByFdWithoutZeroFill);
	REG_FUNC(sys_fs, cellFsAllocateFileAreaWithInitialData);
	REG_FUNC(sys_fs, cellFsAllocateFileAreaWithoutZeroFill);
	REG_FUNC(sys_fs, cellFsArcadeHddSerialNumber);
	REG_FUNC(sys_fs, cellFsChangeFileSizeByFdWithoutAllocation);
	REG_FUNC(sys_fs, cellFsChangeFileSizeWithoutAllocation);
	REG_FUNC(sys_fs, cellFsChmod);
	REG_FUNC(sys_fs, cellFsChown);
	REG_FUNC(sys_fs, cellFsClose).flag(MFF_PERFECT);
	REG_FUNC(sys_fs, cellFsClosedir).flag(MFF_PERFECT);
	REG_FUNC(sys_fs, cellFsFcntl);
	REG_FUNC(sys_fs, cellFsFdatasync);
	REG_FUNC(sys_fs, cellFsFGetBlockSize).flag(MFF_PERFECT);
	REG_FUNC(sys_fs, cellFsFGetBlockSize2);
	REG_FUNC(sys_fs, cellFsFstat).flags = MFF_PERFECT;
	REG_FUNC(sys_fs, cellFsFsync);
	REG_FUNC(sys_fs, cellFsFtruncate).flag(MFF_PERFECT);
	REG_FUNC(sys_fs, cellFsGetBlockSize);
	REG_FUNC(sys_fs, cellFsGetBlockSize2);
	REG_FUNC(sys_fs, cellFsGetDirectoryEntries);
	REG_FUNC(sys_fs, cellFsGetFreeSize);
	REG_FUNC(sys_fs, cellFsGetPath);
	REG_FUNC(sys_fs, cellFsLink);
	REG_FUNC(sys_fs, cellFsLseek).flag(MFF_PERFECT);
	REG_FUNC(sys_fs, cellFsLsnGetCDA);
	REG_FUNC(sys_fs, cellFsLsnGetCDASize);
	REG_FUNC(sys_fs, cellFsLsnLock);
	REG_FUNC(sys_fs, cellFsLsnRead);
	REG_FUNC(sys_fs, cellFsLsnRead2);
	REG_FUNC(sys_fs, cellFsLsnUnlock);
	REG_FUNC(sys_fs, cellFsMappedAllocate);
	REG_FUNC(sys_fs, cellFsMappedFree);
	REG_FUNC(sys_fs, cellFsMkdir);
	REG_FUNC(sys_fs, cellFsOpen);
	REG_FUNC(sys_fs, cellFsOpen2);
	REG_FUNC(sys_fs, cellFsOpendir);
	REG_FUNC(sys_fs, cellFsRead).flag(MFF_PERFECT);
	REG_FUNC(sys_fs, cellFsReaddir).flag(MFF_PERFECT);
	REG_FUNC(sys_fs, cellFsReadWithOffset);
	REG_FUNC(sys_fs, cellFsRegisterConversionCallback);
	REG_FUNC(sys_fs, cellFsRename);
	REG_FUNC(sys_fs, cellFsRmdir);
	REG_FUNC(sys_fs, cellFsSdataOpen);
	REG_FUNC(sys_fs, cellFsSdataOpenByFd);
	REG_FUNC(sys_fs, cellFsSdataOpenWithVersion);
	REG_FUNC(sys_fs, cellFsSetAttribute);
	REG_FUNC(sys_fs, cellFsSetDefaultContainer);
	REG_FUNC(sys_fs, cellFsSetDiscReadRetrySetting);
	REG_FUNC(sys_fs, cellFsSetIoBuffer);
	REG_FUNC(sys_fs, cellFsSetIoBufferFromDefaultContainer);
	REG_FUNC(sys_fs, cellFsStat);
	REG_FUNC(sys_fs, cellFsStRead);
	REG_FUNC(sys_fs, cellFsStReadFinish);
	REG_FUNC(sys_fs, cellFsStReadGetCurrentAddr);
	REG_FUNC(sys_fs, cellFsStReadGetRegid);
	REG_FUNC(sys_fs, cellFsStReadGetRingBuf);
	REG_FUNC(sys_fs, cellFsStReadGetStatus);
	REG_FUNC(sys_fs, cellFsStReadInit);
	REG_FUNC(sys_fs, cellFsStReadPutCurrentAddr);
	REG_FUNC(sys_fs, cellFsStReadStart);
	REG_FUNC(sys_fs, cellFsStReadStop);
	REG_FUNC(sys_fs, cellFsStReadWait);
	REG_FUNC(sys_fs, cellFsStReadWaitCallback);
	REG_FUNC(sys_fs, cellFsSymbolicLink);
	REG_FUNC(sys_fs, cellFsTruncate);
	REG_FUNC(sys_fs, cellFsTruncate2);
	REG_FUNC(sys_fs, cellFsUnlink);
	REG_FUNC(sys_fs, cellFsUnregisterL10nCallbacks);
	REG_FUNC(sys_fs, cellFsUtime);
	REG_FUNC(sys_fs, cellFsWrite).flag(MFF_PERFECT);
	REG_FUNC(sys_fs, cellFsWriteWithOffset);
});
