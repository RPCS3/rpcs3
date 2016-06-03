#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"

#include "Emu/Cell/lv2/sys_fs.h"
#include "cellFs.h"

#include "Utilities/StrUtil.h"

logs::channel cellFs("cellFs", logs::level::notice);

s32 cellFsOpen(vm::cptr<char> path, s32 flags, vm::ptr<u32> fd, vm::cptr<void> arg, u64 size)
{
	cellFs.warning("cellFsOpen(path=*0x%x, flags=%#o, fd=*0x%x, arg=*0x%x, size=0x%llx) -> sys_fs_open()", path, flags, fd, arg, size);

	// TODO

	// call the syscall
	return sys_fs_open(path, flags, fd, flags & CELL_FS_O_CREAT ? CELL_FS_S_IRUSR | CELL_FS_S_IWUSR : 0, arg, size);
}

s32 cellFsRead(u32 fd, vm::ptr<void> buf, u64 nbytes, vm::ptr<u64> nread)
{
	cellFs.trace("cellFsRead(fd=0x%x, buf=0x%x, nbytes=0x%llx, nread=0x%x)", fd, buf, nbytes, nread);

	// call the syscall
	return sys_fs_read(fd, buf, nbytes, nread ? nread : vm::var<u64>{});
}

s32 cellFsWrite(u32 fd, vm::cptr<void> buf, u64 nbytes, vm::ptr<u64> nwrite)
{
	cellFs.trace("cellFsWrite(fd=0x%x, buf=*0x%x, nbytes=0x%llx, nwrite=*0x%x)", fd, buf, nbytes, nwrite);

	// call the syscall
	return sys_fs_write(fd, buf, nbytes, nwrite ? nwrite : vm::var<u64>{});
}

s32 cellFsClose(u32 fd)
{
	cellFs.trace("cellFsClose(fd=0x%x)", fd);

	// call the syscall
	return sys_fs_close(fd);
}

s32 cellFsOpendir(vm::cptr<char> path, vm::ptr<u32> fd)
{
	cellFs.warning("cellFsOpendir(path=*0x%x, fd=*0x%x) -> sys_fs_opendir()", path, fd);

	// TODO

	// call the syscall
	return sys_fs_opendir(path, fd);
}

s32 cellFsReaddir(u32 fd, vm::ptr<CellFsDirent> dir, vm::ptr<u64> nread)
{
	cellFs.trace("cellFsReaddir(fd=0x%x, dir=*0x%x, nread=*0x%x)", fd, dir, nread);

	// call the syscall
	return dir && nread ? sys_fs_readdir(fd, dir, nread) : CELL_FS_EFAULT;
}

s32 cellFsClosedir(u32 fd)
{
	cellFs.trace("cellFsClosedir(fd=0x%x)", fd);

	// call the syscall
	return sys_fs_closedir(fd);
}

s32 cellFsStat(vm::cptr<char> path, vm::ptr<CellFsStat> sb)
{
	cellFs.warning("cellFsStat(path=*0x%x, sb=*0x%x) -> sys_fs_stat()", path, sb);

	// TODO

	// call the syscall
	return sys_fs_stat(path, sb);
}

s32 cellFsFstat(u32 fd, vm::ptr<CellFsStat> sb)
{
	cellFs.trace("cellFsFstat(fd=0x%x, sb=*0x%x)", fd, sb);

	// call the syscall
	return sys_fs_fstat(fd, sb);
}

s32 cellFsMkdir(vm::cptr<char> path, s32 mode)
{
	cellFs.warning("cellFsMkdir(path=*0x%x, mode=%#o) -> sys_fs_mkdir()", path, mode);

	// TODO

	// call the syscall
	return sys_fs_mkdir(path, mode);
}

s32 cellFsRename(vm::cptr<char> from, vm::cptr<char> to)
{
	cellFs.warning("cellFsRename(from=*0x%x, to=*0x%x) -> sys_fs_rename()", from, to);

	// TODO

	// call the syscall
	return sys_fs_rename(from, to);
}

s32 cellFsRmdir(vm::cptr<char> path)
{
	cellFs.warning("cellFsRmdir(path=*0x%x) -> sys_fs_rmdir()", path);

	// TODO

	// call the syscall
	return sys_fs_rmdir(path);
}

s32 cellFsUnlink(vm::cptr<char> path)
{
	cellFs.warning("cellFsUnlink(path=*0x%x) -> sys_fs_unlink()", path);

	// TODO

	// call the syscall
	return sys_fs_unlink(path);
}

s32 cellFsLseek(u32 fd, s64 offset, u32 whence, vm::ptr<u64> pos)
{
	cellFs.trace("cellFsLseek(fd=0x%x, offset=0x%llx, whence=0x%x, pos=*0x%x)", fd, offset, whence, pos);

	// call the syscall
	return pos ? sys_fs_lseek(fd, offset, whence, pos) : CELL_FS_EFAULT;
}

s32 cellFsFsync(u32 fd)
{
	cellFs.todo("cellFsFsync(fd=0x%x)", fd);

	return CELL_OK;
}

s32 cellFsFGetBlockSize(u32 fd, vm::ptr<u64> sector_size, vm::ptr<u64> block_size)
{
	cellFs.trace("cellFsFGetBlockSize(fd=0x%x, sector_size=*0x%x, block_size=*0x%x)", fd, sector_size, block_size);

	// call the syscall
	return sector_size && block_size ? sys_fs_fget_block_size(fd, sector_size, block_size, vm::var<u64>{}, vm::var<u64>{}) : CELL_FS_EFAULT;
}

s32 cellFsGetBlockSize(vm::cptr<char> path, vm::ptr<u64> sector_size, vm::ptr<u64> block_size)
{
	cellFs.warning("cellFsGetBlockSize(path=*0x%x, sector_size=*0x%x, block_size=*0x%x) -> sys_fs_get_block_size()", path, sector_size, block_size);

	// TODO

	// call the syscall
	return sys_fs_get_block_size(path, sector_size, block_size, vm::var<u64>{});
}

s32 cellFsTruncate(vm::cptr<char> path, u64 size)
{
	cellFs.warning("cellFsTruncate(path=*0x%x, size=0x%llx) -> sys_fs_truncate()", path, size);

	// TODO

	// call the syscall
	return sys_fs_truncate(path, size);
}

s32 cellFsFtruncate(u32 fd, u64 size)
{
	cellFs.trace("cellFsFtruncate(fd=0x%x, size=0x%llx)", fd, size);

	// call the syscall
	return sys_fs_ftruncate(fd, size);
}

s32 cellFsChmod(vm::cptr<char> path, s32 mode)
{
	cellFs.warning("cellFsChmod(path=*0x%x, mode=%#o) -> sys_fs_chmod()", path, mode);

	// TODO

	// call the syscall
	return sys_fs_chmod(path, mode);
}

s32 cellFsGetFreeSize(vm::cptr<char> path, vm::ptr<u32> block_size, vm::ptr<u64> block_count)
{
	cellFs.warning("cellFsGetFreeSize(path=*0x%x, block_size=*0x%x, block_count=*0x%x)", path, block_size, block_count);
	cellFs.warning("*** path = '%s'", path.get_ptr());

	// TODO: Get real values. Currently, it always returns 40 GB of free space divided in 4 KB blocks
	*block_size = 4096; // ?
	*block_count = 10 * 1024 * 1024; // ?

	return CELL_OK;
}

s32 cellFsGetDirectoryEntries(u32 fd, vm::ptr<CellFsDirectoryEntry> entries, u32 entries_size, vm::ptr<u32> data_count)
{
	cellFs.warning("cellFsGetDirectoryEntries(fd=%d, entries=*0x%x, entries_size=0x%x, data_count=*0x%x)", fd, entries, entries_size, data_count);

	const auto directory = idm::get<lv2_dir_t>(fd);

	if (!directory)
	{
		return CELL_FS_EBADF;
	}

	u32 count = 0;

	entries_size /= sizeof(CellFsDirectoryEntry);

	for (; count < entries_size; count++)
	{
		fs::dir_entry info;

		if (directory->dir.read(info))
		{
			entries[count].attribute.mode = info.is_directory ? CELL_FS_S_IFDIR | 0777 : CELL_FS_S_IFREG | 0666;
			entries[count].attribute.uid = 1; // ???
			entries[count].attribute.gid = 1; // ???
			entries[count].attribute.atime = info.atime;
			entries[count].attribute.mtime = info.mtime;
			entries[count].attribute.ctime = info.ctime;
			entries[count].attribute.size = info.size;
			entries[count].attribute.blksize = 4096; // ???

			entries[count].entry_name.d_type = info.is_directory ? CELL_FS_TYPE_DIRECTORY : CELL_FS_TYPE_REGULAR;
			entries[count].entry_name.d_namlen = u8(std::min<size_t>(info.name.size(), CELL_FS_MAX_FS_FILE_NAME_LENGTH));
			strcpy_trunc(entries[count].entry_name.d_name, info.name);
		}
		else
		{
			break;
		}
	}

	*data_count = count;

	return CELL_OK;
}

s32 cellFsReadWithOffset(u32 fd, u64 offset, vm::ptr<void> buf, u64 buffer_size, vm::ptr<u64> nread)
{
	cellFs.trace("cellFsReadWithOffset(fd=%d, offset=0x%llx, buf=*0x%x, buffer_size=0x%llx, nread=*0x%x)", fd, offset, buf, buffer_size, nread);

	// TODO: use single sys_fs_fcntl syscall

	const auto file = idm::get<lv2_file_t>(fd);

	if (!file || file->flags & CELL_FS_O_WRONLY)
	{
		return CELL_FS_EBADF;
	}

	std::lock_guard<std::mutex> lock(file->mutex);

	const auto old_pos = file->file.pos(); file->file.seek(offset);

	const auto read = file->file.read(buf.get_ptr(), buffer_size);

	file->file.seek(old_pos);

	if (nread)
	{
		*nread = read;
	}

	return CELL_OK;
}

s32 cellFsWriteWithOffset(u32 fd, u64 offset, vm::cptr<void> buf, u64 data_size, vm::ptr<u64> nwrite)
{
	cellFs.trace("cellFsWriteWithOffset(fd=%d, offset=0x%llx, buf=*0x%x, data_size=0x%llx, nwrite=*0x%x)", fd, offset, buf, data_size, nwrite);

	// TODO: use single sys_fs_fcntl syscall

	const auto file = idm::get<lv2_file_t>(fd);

	if (!file || !(file->flags & CELL_FS_O_ACCMODE))
	{
		return CELL_FS_EBADF;
	}

	std::lock_guard<std::mutex> lock(file->mutex);

	const auto old_pos = file->file.pos(); file->file.seek(offset);

	const auto written = file->file.write(buf.get_ptr(), data_size);

	file->file.seek(old_pos);

	if (nwrite)
	{
		*nwrite = written;
	}

	return CELL_OK;
}

s32 cellFsStReadInit(u32 fd, vm::cptr<CellFsRingBuffer> ringbuf)
{
	cellFs.warning("cellFsStReadInit(fd=%d, ringbuf=*0x%x)", fd, ringbuf);

	if (ringbuf->copy & ~CELL_FS_ST_COPYLESS)
	{
		return CELL_FS_EINVAL;
	}

	if (ringbuf->block_size & 0xfff) // check if a multiple of sector size
	{
		return CELL_FS_EINVAL;
	}

	if (ringbuf->ringbuf_size % ringbuf->block_size) // check if a multiple of block_size
	{
		return CELL_FS_EINVAL;
	}

	const auto file = idm::get<lv2_file_t>(fd);

	if (!file)
	{
		return CELL_FS_EBADF;
	}
	
	if (file->flags & CELL_FS_O_WRONLY)
	{
		return CELL_FS_EPERM;
	}

	std::lock_guard<std::mutex> lock(file->mutex);

	if (!file->st_status.compare_and_swap_test(SSS_NOT_INITIALIZED, SSS_INITIALIZED))
	{
		return CELL_FS_EBUSY;
	}

	file->st_ringbuf_size = ringbuf->ringbuf_size;
	file->st_block_size = ringbuf->ringbuf_size;
	file->st_trans_rate = ringbuf->transfer_rate;
	file->st_copyless = ringbuf->copy == CELL_FS_ST_COPYLESS;

	const u64 alloc_size = align(file->st_ringbuf_size, file->st_ringbuf_size < 1024 * 1024 ? 64 * 1024 : 1024 * 1024);

	file->st_buffer = vm::alloc(static_cast<u32>(alloc_size), vm::main);
	file->st_read_size = 0;
	file->st_total_read = 0;
	file->st_copied = 0;

	return CELL_OK;
}

s32 cellFsStReadFinish(u32 fd)
{
	cellFs.warning("cellFsStReadFinish(fd=%d)", fd);

	const auto file = idm::get<lv2_file_t>(fd);

	if (!file)
	{
		return CELL_FS_EBADF; // ???
	}

	std::lock_guard<std::mutex> lock(file->mutex);

	if (!file->st_status.compare_and_swap_test(SSS_INITIALIZED, SSS_NOT_INITIALIZED))
	{
		return CELL_FS_ENXIO;
	}

	vm::dealloc(file->st_buffer, vm::main);

	return CELL_OK;
}

s32 cellFsStReadGetRingBuf(u32 fd, vm::ptr<CellFsRingBuffer> ringbuf)
{
	cellFs.warning("cellFsStReadGetRingBuf(fd=%d, ringbuf=*0x%x)", fd, ringbuf);

	const auto file = idm::get<lv2_file_t>(fd);

	if (!file)
	{
		return CELL_FS_EBADF;
	}

	if (file->st_status == SSS_NOT_INITIALIZED)
	{
		return CELL_FS_ENXIO;
	}

	ringbuf->ringbuf_size = file->st_ringbuf_size;
	ringbuf->block_size = file->st_block_size;
	ringbuf->transfer_rate = file->st_trans_rate;
	ringbuf->copy = file->st_copyless ? CELL_FS_ST_COPYLESS : CELL_FS_ST_COPY;

	return CELL_OK;
}

s32 cellFsStReadGetStatus(u32 fd, vm::ptr<u64> status)
{
	cellFs.warning("cellFsStReadGetRingBuf(fd=%d, status=*0x%x)", fd, status);

	const auto file = idm::get<lv2_file_t>(fd);

	if (!file)
	{
		return CELL_FS_EBADF;
	}

	switch (file->st_status.load())
	{
	case SSS_INITIALIZED:
	case SSS_STOPPED:
	{
		*status = CELL_FS_ST_INITIALIZED | CELL_FS_ST_STOP;
		break;
	}
	case SSS_STARTED:
	{
		*status = CELL_FS_ST_INITIALIZED | CELL_FS_ST_PROGRESS;
		break;
	}
	default:
	{
		*status = CELL_FS_ST_NOT_INITIALIZED | CELL_FS_ST_STOP;
		break;
	}
	}

	return CELL_OK;
}

s32 cellFsStReadGetRegid(u32 fd, vm::ptr<u64> regid)
{
	cellFs.warning("cellFsStReadGetRingBuf(fd=%d, regid=*0x%x)", fd, regid);

	const auto file = idm::get<lv2_file_t>(fd);

	if (!file)
	{
		return CELL_FS_EBADF;
	}

	if (file->st_status == SSS_NOT_INITIALIZED)
	{
		return CELL_FS_ENXIO;
	}

	*regid = file->st_total_read - file->st_copied;

	return CELL_OK;
}

s32 cellFsStReadStart(u32 fd, u64 offset, u64 size)
{
	cellFs.warning("cellFsStReadStart(fd=%d, offset=0x%llx, size=0x%llx)", fd, offset, size);

	const auto file = idm::get<lv2_file_t>(fd);

	if (!file)
	{
		return CELL_FS_EBADF;
	}

	switch (auto status = file->st_status.compare_and_swap(SSS_INITIALIZED, SSS_STARTED))
	{
	case SSS_NOT_INITIALIZED:
	{
		return CELL_FS_ENXIO;
	}

	case SSS_STARTED:
	{
		return CELL_FS_EBUSY;
	}
	}

	offset = std::min<u64>(file->file.size(), offset);
	size = std::min<u64>(file->file.size() - offset, size);

	file->st_read_size = size;

	file->st_thread = thread_ctrl::spawn("FS ST Thread", [=]()
	{
		std::unique_lock<std::mutex> lock(file->mutex);

		while (file->st_status == SSS_STARTED && !Emu.IsStopped())
		{
			// check free space in buffer and available data in stream
			if (file->st_total_read - file->st_copied <= file->st_ringbuf_size - file->st_block_size && file->st_total_read < file->st_read_size)
			{
				// get buffer position
				const u32 position = vm::cast(file->st_buffer + file->st_total_read % file->st_ringbuf_size, HERE);

				// read data
				auto old = file->file.pos();
				file->file.seek(offset + file->st_total_read);
				auto res = file->file.read(vm::base(position), file->st_block_size);
				file->file.seek(old);

				// notify
				file->st_total_read += res;
				file->cv.notify_one();
			}

			// check callback condition if set
			if (file->st_callback.load().func)
			{
				const u64 available = file->st_total_read - file->st_copied;

				if (available >= file->st_callback.load().size)
				{
					const auto func = file->st_callback.exchange({}).func;

					Emu.GetCallbackManager().Async([=](PPUThread& ppu)
					{
						func(ppu, fd, available);
					});
				}
			}

			file->cv.wait_for(lock, 1ms);
		}

		file->st_status.compare_and_swap(SSS_STOPPED, SSS_INITIALIZED);
		file->st_read_size = 0;
		file->st_total_read = 0;
		file->st_copied = 0;
		file->st_callback.store({});
	});

	return CELL_OK;
}

s32 cellFsStReadStop(u32 fd)
{
	cellFs.warning("cellFsStReadStop(fd=%d)", fd);

	const auto file = idm::get<lv2_file_t>(fd);

	if (!file)
	{
		return CELL_FS_EBADF;
	}

	switch (auto status = file->st_status.compare_and_swap(SSS_STARTED, SSS_STOPPED))
	{
	case SSS_NOT_INITIALIZED:
	{
		return CELL_FS_ENXIO;
	}

	case SSS_INITIALIZED:
	case SSS_STOPPED:
	{
		return CELL_OK;
	}
	}

	file->cv.notify_all();
	file->st_thread->join();

	return CELL_OK;
}

s32 cellFsStRead(u32 fd, vm::ptr<u8> buf, u64 size, vm::ptr<u64> rsize)
{
	cellFs.warning("cellFsStRead(fd=%d, buf=*0x%x, size=0x%llx, rsize=*0x%x)", fd, buf, size, rsize);
	
	const auto file = idm::get<lv2_file_t>(fd);

	if (!file)
	{
		return CELL_FS_EBADF;
	}

	if (file->st_status == SSS_NOT_INITIALIZED || file->st_copyless)
	{
		return CELL_FS_ENXIO;
	}

	const u64 copied = file->st_copied;
	const u32 position = vm::cast(file->st_buffer + copied % file->st_ringbuf_size, HERE);
	const u64 total_read = file->st_total_read;
	const u64 copy_size = (*rsize = std::min<u64>(size, total_read - copied)); // write rsize
	
	// copy data
	const u64 first_size = std::min<u64>(copy_size, file->st_ringbuf_size - (position - file->st_buffer));
	std::memcpy(buf.get_ptr(), vm::base(position), first_size);
	std::memcpy((buf + first_size).get_ptr(), vm::base(file->st_buffer), copy_size - first_size);

	// notify
	file->st_copied += copy_size;
	file->cv.notify_one();

	// check end of stream
	return total_read < file->st_read_size ? CELL_OK : CELL_FS_ERANGE;
}

s32 cellFsStReadGetCurrentAddr(u32 fd, vm::ptr<u32> addr, vm::ptr<u64> size)
{
	cellFs.warning("cellFsStReadGetCurrentAddr(fd=%d, addr=*0x%x, size=*0x%x)", fd, addr, size);

	const auto file = idm::get<lv2_file_t>(fd);

	if (!file)
	{
		return CELL_FS_EBADF;
	}

	if (file->st_status == SSS_NOT_INITIALIZED || !file->st_copyless)
	{
		return CELL_FS_ENXIO;
	}

	const u64 copied = file->st_copied;
	const u32 position = vm::cast(file->st_buffer + copied % file->st_ringbuf_size, HERE);
	const u64 total_read = file->st_total_read;

	if ((*size = std::min<u64>(file->st_ringbuf_size - (position - file->st_buffer), total_read - copied)))
	{
		*addr = position;
	}
	else
	{
		*addr = 0;
	}

	// check end of stream
	return total_read < file->st_read_size ? CELL_OK : CELL_FS_ERANGE;
}

s32 cellFsStReadPutCurrentAddr(u32 fd, vm::ptr<u8> addr, u64 size)
{
	cellFs.warning("cellFsStReadPutCurrentAddr(fd=%d, addr=*0x%x, size=0x%llx)", fd, addr, size);
	
	const auto file = idm::get<lv2_file_t>(fd);

	if (!file)
	{
		return CELL_FS_EBADF;
	}

	if (file->st_status == SSS_NOT_INITIALIZED || !file->st_copyless)
	{
		return CELL_FS_ENXIO;
	}

	const u64 copied = file->st_copied;
	const u64 total_read = file->st_total_read;

	// notify
	file->st_copied += size;
	file->cv.notify_one();

	// check end of stream
	return total_read < file->st_read_size ? CELL_OK : CELL_FS_ERANGE;
}

s32 cellFsStReadWait(u32 fd, u64 size)
{
	cellFs.warning("cellFsStReadWait(fd=%d, size=0x%llx)", fd, size);
	
	const auto file = idm::get<lv2_file_t>(fd);

	if (!file)
	{
		return CELL_FS_EBADF;
	}

	if (file->st_status == SSS_NOT_INITIALIZED)
	{
		return CELL_FS_ENXIO;
	}

	std::unique_lock<std::mutex> lock(file->mutex);

	// wait for size availability or stream end
	while (file->st_total_read - file->st_copied < size && file->st_total_read < file->st_read_size)
	{
		CHECK_EMU_STATUS;

		file->cv.wait_for(lock, 1ms);
	}
	
	return CELL_OK;
}

s32 cellFsStReadWaitCallback(u32 fd, u64 size, fs_st_cb_t func)
{
	cellFs.warning("cellFsStReadWaitCallback(fd=%d, size=0x%llx, func=*0x%x)", fd, size, func);

	const auto file = idm::get<lv2_file_t>(fd);

	if (!file)
	{
		return CELL_FS_EBADF;
	}

	if (file->st_status == SSS_NOT_INITIALIZED)
	{
		return CELL_FS_ENXIO;
	}

	if (!file->st_callback.compare_and_swap_test({}, { size, func }))
	{
		return CELL_FS_EIO;
	}
	
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

s32 sdata_unpack(const std::string& packed_file, const std::string& unpacked_file)
{
	fs::file packed_stream(vfs::get(packed_file));
	fs::file unpacked_stream(vfs::get(unpacked_file), fs::rewrite);

	if (!packed_stream)
	{
		cellFs.error("File '%s' not found!", packed_file);
		return CELL_ENOENT;
	}

	if (!unpacked_stream)
	{
		cellFs.error("File '%s' couldn't be created!", unpacked_file);
		return CELL_ENOENT;
	}

	char buffer[10200];
	packed_stream.read(buffer, 256);
	u32 format = *(be_t<u32>*)&buffer[0];
	if (format != 0x4E504400) // "NPD\x00"
	{
		cellFs.error("Illegal format. Expected 0x4E504400, but got 0x%08x", format);
		return CELL_EFSSPECIFIC;
	}

	u32 version = *(be_t<u32>*)&buffer[0x04];
	u32 flags = *(be_t<u32>*)&buffer[0x80];
	u32 blockSize = *(be_t<u32>*)&buffer[0x84];
	u64 filesizeOutput = *(be_t<u64>*)&buffer[0x88];
	u64 filesizeInput = packed_stream.size();
	u32 blockCount = (u32)((filesizeOutput + blockSize - 1) / blockSize);

	// SDATA file is compressed
	if (flags & 0x1)
	{
		cellFs.warning("cellFsSdataOpen: Compressed SDATA files are not supported yet.");
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
			cellFs.error("cellFsSdataOpen: Wrong header information.");
			return CELL_EFSSPECIFIC;
		}

		if (flags & 0x20)
		{
			packed_stream.seek(0x100);
		}
		else
		{
			packed_stream.seek(startOffset);
		}

		for (u32 i = 0; i < blockCount; i++)
		{
			if (flags & 0x20)
			{
				packed_stream.seek(t1, fs::seek_cur);
			}

			if (!(blockCount - i - 1))
			{
				blockSize = (u32)(filesizeOutput - i * blockSize);
			}

			packed_stream.read(buffer + 256, blockSize);
			unpacked_stream.write(buffer + 256, blockSize);
		}
	}

	return CELL_OK;
}

s32 cellFsSdataOpen(vm::cptr<char> path, s32 flags, vm::ptr<u32> fd, vm::cptr<void> arg, u64 size)
{
	cellFs.notice("cellFsSdataOpen(path=*0x%x, flags=%#o, fd=*0x%x, arg=*0x%x, size=0x%llx)", path, flags, fd, arg, size);

	if (flags != CELL_FS_O_RDONLY)
	{
		return CELL_FS_EINVAL;
	}

	return cellFsOpen(path, CELL_FS_O_RDONLY, fd, vm::make_var<be_t<u32>[2]>({ 0x180, 0x10 }), 8);

	// Don't implement sdata decryption in this function, it should be done in sys_fs_open() syscall or somewhere else

	/*
	std::string suffix = path.substr(path.length() - 5, 5);
	if (suffix != ".sdat" && suffix != ".SDAT")
	return CELL_ENOTSDATA;

	std::string::size_type last_slash = path.rfind('/'); //TODO: use a filesystem library to solve this more robustly
	last_slash = last_slash == std::string::npos ? 0 : last_slash+1;
	std::string unpacked_path = "/dev_hdd1/"+path.substr(last_slash,path.length()-last_slash)+".unpacked";
	s32 ret = sdata_unpack(path, unpacked_path);
	if (ret) return ret;

	fd = idm::GetNewID(Emu.GetVFS().OpenFile(unpacked_path, vfsRead), TYPE_FS_FILE);

	return CELL_OK;
	*/
}

s32 cellFsSdataOpenByFd(u32 mself_fd, s32 flags, vm::ptr<u32> sdata_fd, u64 offset, vm::cptr<void> arg, u64 size)
{
	cellFs.todo("cellFsSdataOpenByFd(mself_fd=0x%x, flags=%#o, sdata_fd=*0x%x, offset=0x%llx, arg=*0x%x, size=0x%llx)", mself_fd, flags, sdata_fd, offset, arg, size);

	// TODO:

	return CELL_OK;
}

using fs_aio_cb_t = vm::ptr<void(vm::ptr<CellFsAio> xaio, s32 error, s32 xid, u64 size)>;

void fsAio(vm::ptr<CellFsAio> aio, bool write, s32 xid, fs_aio_cb_t func)
{
	cellFs.notice("FS AIO Request(%d): fd=%d, offset=0x%llx, buf=*0x%x, size=0x%llx, user_data=0x%llx", xid, aio->fd, aio->offset, aio->buf, aio->size, aio->user_data);

	s32 error = CELL_OK;
	u64 result = 0;

	const auto file = idm::get<lv2_file_t>(aio->fd);

	if (!file || (!write && file->flags & CELL_FS_O_WRONLY) || (write && !(file->flags & CELL_FS_O_ACCMODE)))
	{
		error = CELL_FS_EBADF;
	}
	else
	{
		std::lock_guard<std::mutex> lock(file->mutex);

		const auto old_pos = file->file.pos(); file->file.seek(aio->offset);

		result = write
			? file->file.write(aio->buf.get_ptr(), aio->size)
			: file->file.read(aio->buf.get_ptr(), aio->size);

		file->file.seek(old_pos);
	}

	// should be executed directly by FS AIO thread
	Emu.GetCallbackManager().Async([=](PPUThread& ppu)
	{
		func(ppu, aio, error, xid, result);
	});
}

s32 cellFsAioInit(vm::cptr<char> mount_point)
{
	cellFs.warning("cellFsAioInit(mount_point=*0x%x)", mount_point);
	cellFs.warning("*** mount_point = '%s'", mount_point.get_ptr());

	// TODO: create AIO thread (if not exists) for specified mount point

	return CELL_OK;
}

s32 cellFsAioFinish(vm::cptr<char> mount_point)
{
	cellFs.warning("cellFsAioFinish(mount_point=*0x%x)", mount_point);
	cellFs.warning("*** mount_point = '%s'", mount_point.get_ptr());

	// TODO: delete existing AIO thread for specified mount point

	return CELL_OK;
}

atomic_t<s32> g_fs_aio_id;

s32 cellFsAioRead(vm::ptr<CellFsAio> aio, vm::ptr<s32> id, fs_aio_cb_t func)
{
	cellFs.warning("cellFsAioRead(aio=*0x%x, id=*0x%x, func=*0x%x)", aio, id, func);

	// TODO: detect mount point and send AIO request to the AIO thread of this mount point

	const s32 xid = (*id = ++g_fs_aio_id);

	thread_ctrl::spawn("FS AIO Read Thread", COPY_EXPR(fsAio(aio, false, xid, func)));

	return CELL_OK;
}

s32 cellFsAioWrite(vm::ptr<CellFsAio> aio, vm::ptr<s32> id, fs_aio_cb_t func)
{
	cellFs.warning("cellFsAioWrite(aio=*0x%x, id=*0x%x, func=*0x%x)", aio, id, func);

	// TODO: detect mount point and send AIO request to the AIO thread of this mount point

	const s32 xid = (*id = ++g_fs_aio_id);

	thread_ctrl::spawn("FS AIO Write Thread", COPY_EXPR(fsAio(aio, true, xid, func)));

	return CELL_OK;
}

s32 cellFsAioCancel(s32 id)
{
	cellFs.warning("cellFsAioCancel(id=%d) -> CELL_FS_EINVAL", id);

	// TODO: cancelled requests return CELL_FS_ECANCELED through their own callbacks

	return CELL_FS_EINVAL;
}

s32 cellFsSetDefaultContainer(u32 id, u32 total_limit)
{
	cellFs.todo("cellFsSetDefaultContainer(id=0x%x, total_limit=%d)", id, total_limit);

	return CELL_OK;
}

s32 cellFsSetIoBufferFromDefaultContainer(u32 fd, u32 buffer_size, u32 page_type)
{
	cellFs.todo("cellFsSetIoBufferFromDefaultContainer(fd=%d, buffer_size=%d, page_type=%d)", fd, buffer_size, page_type);

	const auto file = idm::get<lv2_file_t>(fd);

	if (!file)
	{
		return CELL_FS_EBADF;
	}

	return CELL_OK;
}

s32 cellFsUtime()
{
	throw EXCEPTION("");
}

s32 cellFsArcadeHddSerialNumber()
{
	throw EXCEPTION("");
}

s32 cellFsAllocateFileAreaWithInitialData()
{
	throw EXCEPTION("");
}

s32 cellFsAllocateFileAreaByFdWithoutZeroFill()
{
	throw EXCEPTION("");
}

s32 cellFsSetIoBuffer()
{
	throw EXCEPTION("");
}

s32 cellFsAllocateFileAreaByFdWithInitialData()
{
	throw EXCEPTION("");
}

s32 cellFsTruncate2()
{
	throw EXCEPTION("");
}

s32 cellFsChangeFileSizeWithoutAllocation()
{
	throw EXCEPTION("");
}

s32 cellFsAllocateFileAreaWithoutZeroFill(vm::cptr<char> path, u64 size)
{
	cellFs.warning("cellFsAllocateFileAreaWithoutZeroFill(path=*0x%x, size=0x%llx)", path, size);
	return sys_fs_truncate(path, size);
}

s32 cellFsChangeFileSizeByFdWithoutAllocation()
{
	throw EXCEPTION("");
}

s32 cellFsSetDiscReadRetrySetting()
{
	throw EXCEPTION("");
}

s32 cellFsRegisterConversionCallback()
{
	throw EXCEPTION("");
}

s32 cellFsUnregisterL10nCallbacks()
{
	throw EXCEPTION("");
}


DECLARE(ppu_module_manager::cellFs)("sys_fs", []()
{
	REG_FUNC(sys_fs, cellFsOpen);
	REG_FUNC(sys_fs, cellFsSdataOpen);
	REG_FUNC(sys_fs, cellFsSdataOpenByFd);
	REG_FUNC(sys_fs, cellFsRead, MFF_PERFECT);
	REG_FUNC(sys_fs, cellFsWrite, MFF_PERFECT);
	REG_FUNC(sys_fs, cellFsClose, MFF_PERFECT);
	REG_FUNC(sys_fs, cellFsOpendir);
	REG_FUNC(sys_fs, cellFsReaddir, MFF_PERFECT);
	REG_FUNC(sys_fs, cellFsClosedir, MFF_PERFECT);
	REG_FUNC(sys_fs, cellFsStat);
	REG_FUNC(sys_fs, cellFsFstat, MFF_PERFECT);
	REG_FUNC(sys_fs, cellFsMkdir);
	REG_FUNC(sys_fs, cellFsRename);
	REG_FUNC(sys_fs, cellFsChmod);
	REG_FUNC(sys_fs, cellFsFsync);
	REG_FUNC(sys_fs, cellFsRmdir);
	REG_FUNC(sys_fs, cellFsUnlink);
	REG_FUNC(sys_fs, cellFsLseek, MFF_PERFECT);
	REG_FUNC(sys_fs, cellFsFtruncate, MFF_PERFECT);
	REG_FUNC(sys_fs, cellFsTruncate);
	REG_FUNC(sys_fs, cellFsFGetBlockSize, MFF_PERFECT);
	REG_FUNC(sys_fs, cellFsAioInit);
	REG_FUNC(sys_fs, cellFsAioFinish);
	REG_FUNC(sys_fs, cellFsAioRead);
	REG_FUNC(sys_fs, cellFsAioWrite);
	REG_FUNC(sys_fs, cellFsAioCancel);
	REG_FUNC(sys_fs, cellFsGetBlockSize);
	REG_FUNC(sys_fs, cellFsGetFreeSize);
	REG_FUNC(sys_fs, cellFsReadWithOffset);
	REG_FUNC(sys_fs, cellFsWriteWithOffset);
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
	REG_FUNC(sys_fs, cellFsUtime);
	REG_FUNC(sys_fs, cellFsArcadeHddSerialNumber);
	REG_FUNC(sys_fs, cellFsAllocateFileAreaWithInitialData);
	REG_FUNC(sys_fs, cellFsAllocateFileAreaByFdWithoutZeroFill);
	REG_FUNC(sys_fs, cellFsSetIoBuffer);
	REG_FUNC(sys_fs, cellFsAllocateFileAreaByFdWithInitialData);
	REG_FUNC(sys_fs, cellFsTruncate2);
	REG_FUNC(sys_fs, cellFsChangeFileSizeWithoutAllocation);
	REG_FUNC(sys_fs, cellFsAllocateFileAreaWithoutZeroFill);
	REG_FUNC(sys_fs, cellFsChangeFileSizeByFdWithoutAllocation);
	REG_FUNC(sys_fs, cellFsSetDiscReadRetrySetting);
	REG_FUNC(sys_fs, cellFsRegisterConversionCallback);
	REG_FUNC(sys_fs, cellFsUnregisterL10nCallbacks);
});
