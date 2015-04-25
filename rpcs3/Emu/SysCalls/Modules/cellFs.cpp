#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/Callback.h"
#include "Emu/SysCalls/CB_FUNC.h"

#include "Emu/FS/VFS.h"
#include "Emu/FS/vfsFile.h"
#include "Emu/FS/vfsDir.h"

#include "Emu/SysCalls/lv2/sys_fs.h"
#include "cellFs.h"

extern Module cellFs;

s32 cellFsOpen(vm::ptr<const char> path, s32 flags, vm::ptr<u32> fd, vm::ptr<const void> arg, u64 size)
{
	cellFs.Warning("cellFsOpen(path=*0x%x, flags=%#o, fd=*0x%x, arg=*0x%x, size=0x%llx) -> sys_fs_open()", path, flags, fd, arg, size);

	// TODO

	// call the syscall
	return sys_fs_open(path, flags, fd, flags & CELL_FS_O_CREAT ? CELL_FS_S_IRUSR | CELL_FS_S_IWUSR : 0, arg, size);
}

s32 cellFsRead(PPUThread& CPU, u32 fd, vm::ptr<void> buf, u64 nbytes, vm::ptr<u64> nread)
{
	cellFs.Log("cellFsRead(fd=0x%x, buf=0x%x, nbytes=0x%llx, nread=0x%x)", fd, buf, nbytes, nread);

	// call the syscall
	return sys_fs_read(fd, buf, nbytes, nread ? nread : vm::stackvar<be_t<u64>>(CPU));
}

s32 cellFsWrite(PPUThread& CPU, u32 fd, vm::ptr<const void> buf, u64 nbytes, vm::ptr<u64> nwrite)
{
	cellFs.Log("cellFsWrite(fd=0x%x, buf=*0x%x, nbytes=0x%llx, nwrite=*0x%x)", fd, buf, nbytes, nwrite);

	// call the syscall
	return sys_fs_write(fd, buf, nbytes, nwrite ? nwrite : vm::stackvar<be_t<u64>>(CPU));
}

s32 cellFsClose(u32 fd)
{
	cellFs.Log("cellFsClose(fd=0x%x)", fd);

	// call the syscall
	return sys_fs_close(fd);
}

s32 cellFsOpendir(vm::ptr<const char> path, vm::ptr<u32> fd)
{
	cellFs.Warning("cellFsOpendir(path=*0x%x, fd=*0x%x) -> sys_fs_opendir()", path, fd);

	// TODO

	// call the syscall
	return sys_fs_opendir(path, fd);
}

s32 cellFsReaddir(u32 fd, vm::ptr<CellFsDirent> dir, vm::ptr<u64> nread)
{
	cellFs.Log("cellFsReaddir(fd=0x%x, dir=*0x%x, nread=*0x%x)", fd, dir, nread);

	// call the syscall
	return dir && nread ? sys_fs_readdir(fd, dir, nread) : CELL_FS_EFAULT;
}

s32 cellFsClosedir(u32 fd)
{
	cellFs.Log("cellFsClosedir(fd=0x%x)", fd);

	// call the syscall
	return sys_fs_closedir(fd);
}

s32 cellFsStat(vm::ptr<const char> path, vm::ptr<CellFsStat> sb)
{
	cellFs.Warning("cellFsStat(path=*0x%x, sb=*0x%x) -> sys_fs_stat()", path, sb);

	// TODO

	// call the syscall
	return sys_fs_stat(path, sb);
}

s32 cellFsFstat(u32 fd, vm::ptr<CellFsStat> sb)
{
	cellFs.Log("cellFsFstat(fd=0x%x, sb=*0x%x)", fd, sb);

	// call the syscall
	return sys_fs_fstat(fd, sb);
}

s32 cellFsMkdir(vm::ptr<const char> path, s32 mode)
{
	cellFs.Warning("cellFsMkdir(path=*0x%x, mode=%#o) -> sys_fs_mkdir()", path, mode);

	// TODO

	// call the syscall
	return sys_fs_mkdir(path, mode);
}

s32 cellFsRename(vm::ptr<const char> from, vm::ptr<const char> to)
{
	cellFs.Warning("cellFsRename(from=*0x%x, to=*0x%x) -> sys_fs_rename()", from, to);

	// TODO

	// call the syscall
	return sys_fs_rename(from, to);
}

s32 cellFsRmdir(vm::ptr<const char> path)
{
	cellFs.Warning("cellFsRmdir(path=*0x%x) -> sys_fs_rmdir()", path);

	// TODO

	// call the syscall
	return sys_fs_rmdir(path);
}

s32 cellFsUnlink(vm::ptr<const char> path)
{
	cellFs.Warning("cellFsUnlink(path=*0x%x) -> sys_fs_unlink()", path);

	// TODO

	// call the syscall
	return sys_fs_unlink(path);
}

s32 cellFsLseek(u32 fd, s64 offset, u32 whence, vm::ptr<u64> pos)
{
	cellFs.Log("cellFsLseek(fd=0x%x, offset=0x%llx, whence=0x%x, pos=*0x%x)", fd, offset, whence, pos);

	// call the syscall
	return pos ? sys_fs_lseek(fd, offset, whence, pos) : CELL_FS_EFAULT;
}

s32 cellFsFsync(u32 fd)
{
	cellFs.Todo("cellFsFsync(fd=0x%x)", fd);

	return CELL_OK;
}

s32 cellFsFGetBlockSize(PPUThread& CPU, u32 fd, vm::ptr<u64> sector_size, vm::ptr<u64> block_size)
{
	cellFs.Log("cellFsFGetBlockSize(fd=0x%x, sector_size=*0x%x, block_size=*0x%x)", fd, sector_size, block_size);

	// call the syscall
	return sector_size && block_size ? sys_fs_fget_block_size(fd, sector_size, block_size, vm::stackvar<be_t<u64>>(CPU), vm::stackvar<be_t<u64>>(CPU)) : CELL_FS_EFAULT;
}

s32 cellFsGetBlockSize(PPUThread& CPU, vm::ptr<const char> path, vm::ptr<u64> sector_size, vm::ptr<u64> block_size)
{
	cellFs.Warning("cellFsGetBlockSize(path=*0x%x, sector_size=*0x%x, block_size=*0x%x) -> sys_fs_get_block_size()", path, sector_size, block_size);

	// TODO

	// call the syscall
	return sys_fs_get_block_size(path, sector_size, block_size, vm::stackvar<be_t<u64>>(CPU));
}

s32 cellFsTruncate(vm::ptr<const char> path, u64 size)
{
	cellFs.Warning("cellFsTruncate(path=*0x%x, size=0x%llx) -> sys_fs_truncate()", path, size);

	// TODO

	// call the syscall
	return sys_fs_truncate(path, size);
}

s32 cellFsFtruncate(u32 fd, u64 size)
{
	cellFs.Log("cellFsFtruncate(fd=0x%x, size=0x%llx)", fd, size);

	// call the syscall
	return sys_fs_ftruncate(fd, size);
}

s32 cellFsChmod(vm::ptr<const char> path, s32 mode)
{
	cellFs.Warning("cellFsChmod(path=*0x%x, mode=%#o) -> sys_fs_chmod()", path, mode);

	// TODO

	// call the syscall
	return sys_fs_chmod(path, mode);
}

s32 cellFsGetFreeSize(vm::ptr<const char> path, vm::ptr<u32> block_size, vm::ptr<u64> block_count)
{
	cellFs.Warning("cellFsGetFreeSize(path=*0x%x, block_size=*0x%x, block_count=*0x%x)", path, block_size, block_count);
	cellFs.Warning("*** path = '%s'", path.get_ptr());

	// TODO: Get real values. Currently, it always returns 40 GB of free space divided in 4 KB blocks
	*block_size = 4096; // ?
	*block_count = 10 * 1024 * 1024; // ?

	return CELL_OK;
}

s32 cellFsGetDirectoryEntries(u32 fd, vm::ptr<CellFsDirectoryEntry> entries, u32 entries_size, vm::ptr<u32> data_count)
{
	cellFs.Warning("cellFsGetDirectoryEntries(fd=0x%x, entries=*0x%x, entries_size=0x%x, data_count=*0x%x)", fd, entries, entries_size, data_count);

	const auto directory = Emu.GetIdManager().GetIDData<vfsDirBase>(fd);

	if (!directory)
	{
		return CELL_FS_EBADF;
	}

	u32 count = 0;

	entries_size /= sizeof(CellFsDirectoryEntry);

	for (; count < entries_size; count++)
	{
		if (const auto info = directory->Read())
		{
			entries[count].attribute.mode = info->flags & DirEntry_TypeDir ? CELL_FS_S_IFDIR | 0777 : CELL_FS_S_IFREG | 0666;
			entries[count].attribute.uid = 1; // ???
			entries[count].attribute.gid = 1; // ???
			entries[count].attribute.atime = info->access_time;
			entries[count].attribute.mtime = info->modify_time;
			entries[count].attribute.ctime = info->create_time;
			entries[count].attribute.size = info->size;
			entries[count].attribute.blksize = 4096; // ???

			entries[count].entry_name.d_type = info->flags & DirEntry_TypeFile ? CELL_FS_TYPE_REGULAR : CELL_FS_TYPE_DIRECTORY;
			entries[count].entry_name.d_namlen = u8(std::min<size_t>(info->name.length(), CELL_FS_MAX_FS_FILE_NAME_LENGTH));
			strcpy_trunc(entries[count].entry_name.d_name, info->name);
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
	cellFs.Log("cellFsReadWithOffset(fd=0x%x, offset=0x%llx, buf=*0x%x, buffer_size=0x%llx, nread=*0x%x)", fd, offset, buf, buffer_size, nread);

	// TODO: use single sys_fs_fcntl syscall

	const auto file = Emu.GetIdManager().GetIDData<fs_file_t>(fd);

	if (!file || file->flags & CELL_FS_O_WRONLY)
	{
		return CELL_FS_EBADF;
	}

	std::lock_guard<std::mutex> lock(file->mutex);

	const auto old_position = file->file->Tell();

	file->file->Seek(offset);

	const auto read = file->file->Read(buf.get_ptr(), buffer_size);

	file->file->Seek(old_position);

	if (nread)
	{
		*nread = read;
	}

	return CELL_OK;
}

s32 cellFsWriteWithOffset(u32 fd, u64 offset, vm::ptr<const void> buf, u64 data_size, vm::ptr<u64> nwrite)
{
	cellFs.Log("cellFsWriteWithOffset(fd=0x%x, offset=0x%llx, buf=*0x%x, data_size=0x%llx, nwrite=*0x%x)", fd, offset, buf, data_size, nwrite);

	// TODO: use single sys_fs_fcntl syscall

	const auto file = Emu.GetIdManager().GetIDData<fs_file_t>(fd);

	if (!file || !(file->flags & CELL_FS_O_ACCMODE))
	{
		return CELL_FS_EBADF;
	}

	std::lock_guard<std::mutex> lock(file->mutex);

	const auto old_position = file->file->Tell();

	file->file->Seek(offset);

	const auto written = file->file->Write(buf.get_ptr(), data_size);

	file->file->Seek(old_position);

	if (nwrite)
	{
		*nwrite = written;
	}

	return CELL_OK;
}

s32 cellFsStReadInit(u32 fd, vm::ptr<const CellFsRingBuffer> ringbuf)
{
	cellFs.Warning("cellFsStReadInit(fd=0x%x, ringbuf=*0x%x)", fd, ringbuf);

	if (ringbuf->copy.data() & ~se32(CELL_FS_ST_COPYLESS))
	{
		return CELL_FS_EINVAL;
	}

	if (ringbuf->block_size.data() & se64(0xfff)) // check if a multiple of sector size
	{
		return CELL_FS_EINVAL;
	}

	if (ringbuf->ringbuf_size % ringbuf->block_size) // check if a multiple of block_size
	{
		return CELL_FS_EINVAL;
	}

	const auto file = Emu.GetIdManager().GetIDData<fs_file_t>(fd);

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
	file->st_copyless = ringbuf->copy.data() == se32(CELL_FS_ST_COPYLESS);

	const u64 alloc_size = align(file->st_ringbuf_size, file->st_ringbuf_size < 1024 * 1024 ? 64 * 1024 : 1024 * 1024);

	file->st_buffer = vm::alloc(static_cast<u32>(alloc_size), vm::main);
	file->st_read_size = 0;
	file->st_total_read = 0;
	file->st_copied = 0;

	return CELL_OK;
}

s32 cellFsStReadFinish(u32 fd)
{
	cellFs.Warning("cellFsStReadFinish(fd=0x%x)", fd);

	const auto file = Emu.GetIdManager().GetIDData<fs_file_t>(fd);

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
	cellFs.Warning("cellFsStReadGetRingBuf(fd=0x%x, ringbuf=*0x%x)", fd, ringbuf);

	const auto file = Emu.GetIdManager().GetIDData<fs_file_t>(fd);

	if (!file)
	{
		return CELL_FS_EBADF;
	}

	if (file->st_status.read_sync() == SSS_NOT_INITIALIZED)
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
	cellFs.Warning("cellFsStReadGetRingBuf(fd=0x%x, status=*0x%x)", fd, status);

	const auto file = Emu.GetIdManager().GetIDData<fs_file_t>(fd);

	if (!file)
	{
		return CELL_FS_EBADF;
	}

	switch (file->st_status.read_sync())
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
	cellFs.Warning("cellFsStReadGetRingBuf(fd=0x%x, regid=*0x%x)", fd, regid);

	const auto file = Emu.GetIdManager().GetIDData<fs_file_t>(fd);

	if (!file)
	{
		return CELL_FS_EBADF;
	}

	if (file->st_status.read_sync() == SSS_NOT_INITIALIZED)
	{
		return CELL_FS_ENXIO;
	}

	*regid = file->st_total_read - file->st_copied;

	return CELL_OK;
}

s32 cellFsStReadStart(u32 fd, u64 offset, u64 size)
{
	cellFs.Warning("cellFsStReadStart(fd=0x%x, offset=0x%llx, size=0x%llx)", fd, offset, size);

	const auto file = Emu.GetIdManager().GetIDData<fs_file_t>(fd);

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

	offset = std::min<u64>(file->file->GetSize(), offset);
	size = std::min<u64>(file->file->GetSize() - offset, size);

	file->st_thread.set_name(fmt::format("FS ST Thread[0x%x]", fd));
	file->st_read_size = size;

	file->st_thread.start([=]()
	{
		std::unique_lock<std::mutex> lock(file->mutex);

		while (file->st_status.read_relaxed() == SSS_STARTED && !Emu.IsStopped())
		{
			// check free space in buffer and available data in stream
			if (file->st_total_read - file->st_copied <= file->st_ringbuf_size - file->st_block_size && file->st_total_read < file->st_read_size)
			{
				// get buffer position
				const u32 position = vm::cast(file->st_buffer + file->st_total_read % file->st_ringbuf_size);

				// read data
				auto old = file->file->Tell();
				file->file->Seek(offset + file->st_total_read);
				auto res = file->file->Read(vm::get_ptr(position), file->st_block_size);
				file->file->Seek(old);

				// notify
				file->st_total_read += res;
				file->cv.notify_one();
			}

			// check callback condition if set
			if (file->st_callback.data.func)
			{
				const u64 available = file->st_total_read - file->st_copied;

				if (available >= file->st_callback.data.size)
				{
					const auto func = file->st_callback.exchange({}).func;

					Emu.GetCallbackManager().Async([=](PPUThread& CPU)
					{
						func(CPU, fd, available);
					});
				}
			}

			file->cv.wait_for(lock, std::chrono::milliseconds(1));
		}

		file->st_status.compare_and_swap(SSS_STOPPED, SSS_INITIALIZED);
		file->st_read_size = 0;
		file->st_total_read = 0;
		file->st_copied = 0;
		file->st_callback.data = {};
	});

	return CELL_OK;
}

s32 cellFsStReadStop(u32 fd)
{
	cellFs.Warning("cellFsStReadStop(fd=0x%x)", fd);

	const auto file = Emu.GetIdManager().GetIDData<fs_file_t>(fd);

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
	file->st_thread.join();

	return CELL_OK;
}

s32 cellFsStRead(u32 fd, vm::ptr<u8> buf, u64 size, vm::ptr<u64> rsize)
{
	cellFs.Warning("cellFsStRead(fd=0x%x, buf=*0x%x, size=0x%llx, rsize=*0x%x)", fd, buf, size, rsize);
	
	const auto file = Emu.GetIdManager().GetIDData<fs_file_t>(fd);

	if (!file)
	{
		return CELL_FS_EBADF;
	}

	if (file->st_status.read_sync() == SSS_NOT_INITIALIZED || file->st_copyless)
	{
		return CELL_FS_ENXIO;
	}

	const u64 copied = file->st_copied.load();
	const u32 position = vm::cast(file->st_buffer + copied % file->st_ringbuf_size);
	const u64 total_read = file->st_total_read.load();
	const u32 copy_size = vm::cast(*rsize = std::min<u64>(size, total_read - copied));
	
	// copy data
	const u32 first_size = std::min<u32>(copy_size, file->st_ringbuf_size - (position - file->st_buffer));
	memcpy(buf.get_ptr(), vm::get_ptr(position), first_size);
	memcpy((buf + first_size).get_ptr(), vm::get_ptr(file->st_buffer), copy_size - first_size);

	// notify
	file->st_copied += copy_size;
	file->cv.notify_one();

	// check end of stream
	return total_read < file->st_read_size ? CELL_OK : CELL_FS_ERANGE;
}

s32 cellFsStReadGetCurrentAddr(u32 fd, vm::ptr<u32> addr, vm::ptr<u64> size)
{
	cellFs.Warning("cellFsStReadGetCurrentAddr(fd=0x%x, addr=*0x%x, size=*0x%x)", fd, addr, size);

	const auto file = Emu.GetIdManager().GetIDData<fs_file_t>(fd);

	if (!file)
	{
		return CELL_FS_EBADF;
	}

	if (file->st_status.read_sync() == SSS_NOT_INITIALIZED || !file->st_copyless)
	{
		return CELL_FS_ENXIO;
	}

	const u64 copied = file->st_copied.load();
	const u32 position = vm::cast(file->st_buffer + copied % file->st_ringbuf_size);
	const u64 total_read = file->st_total_read.load();

	if ((*size = std::min<u64>(file->st_ringbuf_size - (position - file->st_buffer), total_read - copied)).data())
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
	cellFs.Warning("cellFsStReadPutCurrentAddr(fd=0x%x, addr=*0x%x, size=0x%llx)", fd, addr, size);
	
	const auto file = Emu.GetIdManager().GetIDData<fs_file_t>(fd);

	if (!file)
	{
		return CELL_FS_EBADF;
	}

	if (file->st_status.read_sync() == SSS_NOT_INITIALIZED || !file->st_copyless)
	{
		return CELL_FS_ENXIO;
	}

	const u64 copied = file->st_copied.load();
	const u64 total_read = file->st_total_read.load();

	// notify
	file->st_copied += size;
	file->cv.notify_one();

	// check end of stream
	return total_read < file->st_read_size ? CELL_OK : CELL_FS_ERANGE;
}

s32 cellFsStReadWait(u32 fd, u64 size)
{
	cellFs.Warning("cellFsStReadWait(fd=0x%x, size=0x%llx)", fd, size);
	
	const auto file = Emu.GetIdManager().GetIDData<fs_file_t>(fd);

	if (!file)
	{
		return CELL_FS_EBADF;
	}

	if (file->st_status.read_sync() == SSS_NOT_INITIALIZED)
	{
		return CELL_FS_ENXIO;
	}

	std::unique_lock<std::mutex> lock(file->mutex);

	while (file->st_total_read - file->st_copied < size && file->st_total_read < file->st_read_size)
	{
		// wait for size availability or stream end

		if (Emu.IsStopped())
		{
			cellFs.Warning("cellFsStReadWait(0x%x) aborted", fd);
			return CELL_OK;
		}

		file->cv.wait_for(lock, std::chrono::milliseconds(1));
	}
	
	return CELL_OK;
}

s32 cellFsStReadWaitCallback(u32 fd, u64 size, fs_st_cb_t func)
{
	cellFs.Warning("cellFsStReadWaitCallback(fd=0x%x, size=0x%llx, func=*0x%x)", fd, size, func);

	const auto file = Emu.GetIdManager().GetIDData<fs_file_t>(fd);

	if (!file)
	{
		return CELL_FS_EBADF;
	}

	if (file->st_status.read_sync() == SSS_NOT_INITIALIZED)
	{
		return CELL_FS_ENXIO;
	}

	if (!file->st_callback.compare_and_swap_test({}, { vm::cast(size, "size"), func }))
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

int sdata_unpack(const std::string& packed_file, const std::string& unpacked_file)
{
	std::shared_ptr<vfsFileBase> packed_stream(Emu.GetVFS().OpenFile(packed_file, vfsRead));
	std::shared_ptr<vfsFileBase> unpacked_stream(Emu.GetVFS().OpenFile(unpacked_file, vfsWriteNew));

	if (!packed_stream || !packed_stream->IsOpened())
	{
		cellFs.Error("File '%s' not found!", packed_file.c_str());
		return CELL_ENOENT;
	}

	if (!unpacked_stream || !unpacked_stream->IsOpened())
	{
		cellFs.Error("File '%s' couldn't be created!", unpacked_file.c_str());
		return CELL_ENOENT;
	}

	char buffer[10200];
	packed_stream->Read(buffer, 256);
	u32 format = re32(*(u32*)&buffer[0]);
	if (format != 0x4E504400) // "NPD\x00"
	{
		cellFs.Error("Illegal format. Expected 0x4E504400, but got 0x%08x", format);
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
		cellFs.Warning("cellFsSdataOpen: Compressed SDATA files are not supported yet.");
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
			cellFs.Error("cellFsSdataOpen: Wrong header information.");
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

s32 cellFsSdataOpen(PPUThread& CPU, vm::ptr<const char> path, s32 flags, vm::ptr<u32> fd, vm::ptr<const void> arg, u64 size)
{
	cellFs.Log("cellFsSdataOpen(path=*0x%x, flags=%#o, fd=*0x%x, arg=*0x%x, size=0x%llx)", path, flags, fd, arg, size);

	if (flags != CELL_FS_O_RDONLY)
	{
		return CELL_FS_EINVAL;
	}

	return cellFsOpen(path, CELL_FS_O_RDONLY, fd, vm::stackvar<be_t<u64>>(CPU), 8);

	// Don't implement sdata decryption in this function, it should be done in sys_fs_open() syscall or somewhere else

	/*
	std::string suffix = path.substr(path.length() - 5, 5);
	if (suffix != ".sdat" && suffix != ".SDAT")
	return CELL_ENOTSDATA;

	std::string::size_type last_slash = path.rfind('/'); //TODO: use a filesystem library to solve this more robustly
	last_slash = last_slash == std::string::npos ? 0 : last_slash+1;
	std::string unpacked_path = "/dev_hdd1/"+path.substr(last_slash,path.length()-last_slash)+".unpacked";
	int ret = sdata_unpack(path, unpacked_path);
	if (ret) return ret;

	fd = Emu.GetIdManager().GetNewID(Emu.GetVFS().OpenFile(unpacked_path, vfsRead), TYPE_FS_FILE);

	return CELL_OK;
	*/
}

s32 cellFsSdataOpenByFd(u32 mself_fd, s32 flags, vm::ptr<u32> sdata_fd, u64 offset, vm::ptr<const void> arg, u64 size)
{
	cellFs.Todo("cellFsSdataOpenByFd(mself_fd=0x%x, flags=%#o, sdata_fd=*0x%x, offset=0x%llx, arg=*0x%x, size=0x%llx)", mself_fd, flags, sdata_fd, offset, arg, size);

	// TODO:

	return CELL_OK;
}

using fs_aio_cb_t = vm::ptr<void(vm::ptr<CellFsAio> xaio, s32 error, s32 xid, u64 size)>;

void fsAio(vm::ptr<CellFsAio> aio, bool write, s32 xid, fs_aio_cb_t func)
{
	cellFs.Notice("FS AIO Request(%d): fd=0x%x, offset=0x%llx, buf=*0x%x, size=0x%llx, user_data=0x%llx", xid, aio->fd, aio->offset, aio->buf, aio->size, aio->user_data);

	s32 error = CELL_OK;
	u64 result = 0;

	const auto file = Emu.GetIdManager().GetIDData<fs_file_t>(aio->fd);

	if (!file || (!write && file->flags & CELL_FS_O_WRONLY) || (write && !(file->flags & CELL_FS_O_ACCMODE)))
	{
		error = CELL_FS_EBADF;
	}
	else
	{
		std::lock_guard<std::mutex> lock(file->mutex);

		const auto old_position = file->file->Tell();

		file->file->Seek(aio->offset);

		result = write ? file->file->Write(aio->buf.get_ptr(), aio->size) : file->file->Read(aio->buf.get_ptr(), aio->size);

		file->file->Seek(old_position);
	}

	// should be executed directly by FS AIO thread
	Emu.GetCallbackManager().Async([=](PPUThread& CPU)
	{
		func(CPU, aio, error, xid, result);
	});
}

s32 cellFsAioInit(vm::ptr<const char> mount_point)
{
	cellFs.Warning("cellFsAioInit(mount_point=*0x%x)", mount_point);
	cellFs.Warning("*** mount_point = '%s'", mount_point.get_ptr());

	// TODO: create AIO thread (if not exists) for specified mount point

	return CELL_OK;
}

s32 cellFsAioFinish(vm::ptr<const char> mount_point)
{
	cellFs.Warning("cellFsAioFinish(mount_point=*0x%x)", mount_point);
	cellFs.Warning("*** mount_point = '%s'", mount_point.get_ptr());

	// TODO: delete existing AIO thread for specified mount point

	return CELL_OK;
}

std::atomic<s32> g_fs_aio_id(0);

s32 cellFsAioRead(vm::ptr<CellFsAio> aio, vm::ptr<s32> id, fs_aio_cb_t func)
{
	cellFs.Warning("cellFsAioRead(aio=*0x%x, id=*0x%x, func=*0x%x)", aio, id, func);

	if (!Emu.GetIdManager().CheckID<fs_file_t>(aio->fd))
	{
		return CELL_FS_EBADF;
	}

	// TODO: detect mount point and send AIO request to the AIO thread of this mount point

	const s32 xid = (*id = ++g_fs_aio_id);

	thread_t("FS AIO Read Thread", std::bind(fsAio, aio, false, xid, func)).detach();

	return CELL_OK;
}

s32 cellFsAioWrite(vm::ptr<CellFsAio> aio, vm::ptr<s32> id, fs_aio_cb_t func)
{
	cellFs.Warning("cellFsAioWrite(aio=*0x%x, id=*0x%x, func=*0x%x)", aio, id, func);

	if (!Emu.GetIdManager().CheckID<fs_file_t>(aio->fd))
	{
		return CELL_FS_EBADF;
	}

	// TODO: detect mount point and send AIO request to the AIO thread of this mount point

	const s32 xid = (*id = ++g_fs_aio_id);

	thread_t("FS AIO Write Thread", std::bind(fsAio, aio, true, xid, func)).detach();

	return CELL_OK;
}

s32 cellFsAioCancel(s32 id)
{
	cellFs.Warning("cellFsAioCancel(id=%d) -> CELL_FS_EINVAL", id);

	// TODO: cancelled requests return CELL_FS_ECANCELED through their own callbacks

	return CELL_FS_EINVAL;
}

s32 cellFsSetDefaultContainer(u32 id, u32 total_limit)
{
	cellFs.Todo("cellFsSetDefaultContainer(id=0x%x, total_limit=%d)", id, total_limit);

	return CELL_OK;
}

s32 cellFsSetIoBufferFromDefaultContainer(u32 fd, u32 buffer_size, u32 page_type)
{
	cellFs.Todo("cellFsSetIoBufferFromDefaultContainer(fd=0x%x, buffer_size=%d, page_type=%d)", fd, buffer_size, page_type);

	const auto file = Emu.GetIdManager().GetIDData<fs_file_t>(fd);

	if (!file)
	{
		return CELL_FS_EBADF;
	}

	return CELL_OK;
}

Module cellFs("cellFs", []()
{
	REG_FUNC(cellFs, cellFsOpen);
	REG_FUNC(cellFs, cellFsSdataOpen);
	REG_FUNC(cellFs, cellFsSdataOpenByFd);
	REG_FUNC(cellFs, cellFsRead);
	REG_FUNC(cellFs, cellFsWrite);
	REG_FUNC(cellFs, cellFsClose);
	REG_FUNC(cellFs, cellFsOpendir);
	REG_FUNC(cellFs, cellFsReaddir);
	REG_FUNC(cellFs, cellFsClosedir);
	REG_FUNC(cellFs, cellFsStat);
	REG_FUNC(cellFs, cellFsFstat);
	REG_FUNC(cellFs, cellFsMkdir);
	REG_FUNC(cellFs, cellFsRename);
	REG_FUNC(cellFs, cellFsChmod);
	REG_FUNC(cellFs, cellFsFsync);
	REG_FUNC(cellFs, cellFsRmdir);
	REG_FUNC(cellFs, cellFsUnlink);
	REG_FUNC(cellFs, cellFsLseek);
	REG_FUNC(cellFs, cellFsFtruncate);
	REG_FUNC(cellFs, cellFsTruncate);
	REG_FUNC(cellFs, cellFsFGetBlockSize);
	REG_FUNC(cellFs, cellFsAioInit);
	REG_FUNC(cellFs, cellFsAioFinish);
	REG_FUNC(cellFs, cellFsAioRead);
	REG_FUNC(cellFs, cellFsAioWrite);
	REG_FUNC(cellFs, cellFsAioCancel);
	REG_FUNC(cellFs, cellFsGetBlockSize);
	REG_FUNC(cellFs, cellFsGetFreeSize);
	REG_FUNC(cellFs, cellFsReadWithOffset);
	REG_FUNC(cellFs, cellFsWriteWithOffset);
	REG_FUNC(cellFs, cellFsGetDirectoryEntries);
	REG_FUNC(cellFs, cellFsStReadInit);
	REG_FUNC(cellFs, cellFsStReadFinish);
	REG_FUNC(cellFs, cellFsStReadGetRingBuf);
	REG_FUNC(cellFs, cellFsStReadGetStatus);
	REG_FUNC(cellFs, cellFsStReadGetRegid);
	REG_FUNC(cellFs, cellFsStReadStart);
	REG_FUNC(cellFs, cellFsStReadStop);
	REG_FUNC(cellFs, cellFsStRead);
	REG_FUNC(cellFs, cellFsStReadGetCurrentAddr);
	REG_FUNC(cellFs, cellFsStReadPutCurrentAddr);
	REG_FUNC(cellFs, cellFsStReadWait);
	REG_FUNC(cellFs, cellFsStReadWaitCallback);
	REG_FUNC(cellFs, cellFsSetDefaultContainer);
	REG_FUNC(cellFs, cellFsSetIoBufferFromDefaultContainer);
});
