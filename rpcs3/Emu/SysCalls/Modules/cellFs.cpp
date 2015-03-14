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

struct CellFsDirectoryEntry
{
	CellFsStat attribute;
	CellFsDirent entry_name;
};

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
	return dir && nread ? sys_fs_readdir(fd, dir, nread) : CELL_EFAULT;
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
	return pos ? sys_fs_lseek(fd, offset, whence, pos) : CELL_EFAULT;
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
	return sector_size && block_size ? sys_fs_fget_block_size(fd, sector_size, block_size, vm::stackvar<be_t<u64>>(CPU), vm::stackvar<be_t<u64>>(CPU)) : CELL_EFAULT;
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

	std::shared_ptr<vfsDirBase> directory;
	if (!Emu.GetIdManager().GetIDData(fd, directory))
		return CELL_ESRCH;

	const DirEntryInfo* info = directory->Read();
	if (info)
	{
		entries->attribute.mode = 
		CELL_FS_S_IRUSR | CELL_FS_S_IWUSR | CELL_FS_S_IXUSR |
		CELL_FS_S_IRGRP | CELL_FS_S_IWGRP | CELL_FS_S_IXGRP |
		CELL_FS_S_IROTH | CELL_FS_S_IWOTH | CELL_FS_S_IXOTH;

		entries->attribute.uid = 0;
		entries->attribute.gid = 0;
		entries->attribute.atime = 0; //TODO
		entries->attribute.mtime = 0; //TODO
		entries->attribute.ctime = 0; //TODO
		entries->attribute.blksize = 4096;

		entries->entry_name.d_type = (info->flags & DirEntry_TypeFile) ? CELL_FS_TYPE_REGULAR : CELL_FS_TYPE_DIRECTORY;
		entries->entry_name.d_namlen = u8(std::min<size_t>(info->name.length(), CELL_FS_MAX_FS_FILE_NAME_LENGTH));
		strcpy_trunc(entries->entry_name.d_name, info->name);
		*data_count = 1;
	}
	else
	{
		*data_count = 0;
	}

	return CELL_OK;
}

s32 cellFsReadWithOffset(u32 fd, u64 offset, vm::ptr<void> buf, u64 buffer_size, vm::ptr<u64> nread)
{
	cellFs.Log("cellFsReadWithOffset(fd=0x%x, offset=0x%llx, buf=*0x%x, buffer_size=0x%llx, nread=*0x%x)", fd, offset, buf, buffer_size, nread);

	// TODO: use single sys_fs_fcntl syscall

	std::shared_ptr<fs_file_t> file;

	if (!Emu.GetIdManager().GetIDData(fd, file) || file->flags & CELL_FS_O_WRONLY)
	{
		return CELL_FS_EBADF;
	}

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

	std::shared_ptr<fs_file_t> file;

	if (!Emu.GetIdManager().GetIDData(fd, file) || !(file->flags & CELL_FS_O_ACCMODE))
	{
		return CELL_FS_EBADF;
	}

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

s32 cellFsStReadInit(u32 fd, vm::ptr<CellFsRingBuffer> ringbuf)
{
	cellFs.Warning("cellFsStReadInit(fd=0x%x, ringbuf=*0x%x)", fd, ringbuf);

	std::shared_ptr<fs_file_t> file;
	if (!Emu.GetIdManager().GetIDData(fd, file))
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
	cellFs.Warning("cellFsStReadFinish(fd=0x%x)", fd);

	std::shared_ptr<fs_file_t> file;
	if (!Emu.GetIdManager().GetIDData(fd, file))
		return CELL_ESRCH;

	Memory.Free(fs_config.m_buffer);
	fs_config.m_fs_status = CELL_FS_ST_NOT_INITIALIZED;

	return CELL_OK;
}

s32 cellFsStReadGetRingBuf(u32 fd, vm::ptr<CellFsRingBuffer> ringbuf)
{
	cellFs.Warning("cellFsStReadGetRingBuf(fd=0x%x, ringbuf=*0x%x)", fd, ringbuf);

	std::shared_ptr<fs_file_t> file;
	if (!Emu.GetIdManager().GetIDData(fd, file))
		return CELL_ESRCH;

	*ringbuf = fs_config.m_ring_buffer;

	cellFs.Warning("*** fs stream config: block_size=0x%llx, copy=0x%x, ringbuf_size=0x%llx, transfer_rate=0x%llx",
		ringbuf->block_size, ringbuf->copy, ringbuf->ringbuf_size, ringbuf->transfer_rate);
	return CELL_OK;
}

s32 cellFsStReadGetStatus(u32 fd, vm::ptr<u64> status)
{
	cellFs.Warning("cellFsStReadGetRingBuf(fd=0x%x, status=*0x%x)", fd, status);

	std::shared_ptr<fs_file_t> file;
	if (!Emu.GetIdManager().GetIDData(fd, file))
		return CELL_ESRCH;

	*status = fs_config.m_fs_status;

	return CELL_OK;
}

s32 cellFsStReadGetRegid(u32 fd, vm::ptr<u64> regid)
{
	cellFs.Warning("cellFsStReadGetRingBuf(fd=0x%x, regid=*0x%x)", fd, regid);

	std::shared_ptr<fs_file_t> file;
	if (!Emu.GetIdManager().GetIDData(fd, file))
		return CELL_ESRCH;

	*regid = fs_config.m_regid;

	return CELL_OK;
}

s32 cellFsStReadStart(u32 fd, u64 offset, u64 size)
{
	cellFs.Todo("cellFsStReadStart(fd=0x%x, offset=0x%llx, size=0x%llx)", fd, offset, size);

	std::shared_ptr<fs_file_t> file;
	if (!Emu.GetIdManager().GetIDData(fd, file))
		return CELL_ESRCH;

	fs_config.m_current_addr = fs_config.m_buffer + (u32)offset;
	fs_config.m_fs_status = CELL_FS_ST_PROGRESS;

	return CELL_OK;
}

s32 cellFsStReadStop(u32 fd)
{
	cellFs.Warning("cellFsStReadStop(fd=0x%x)", fd);

	std::shared_ptr<fs_file_t> file;
	if (!Emu.GetIdManager().GetIDData(fd, file))
		return CELL_ESRCH;

	fs_config.m_fs_status = CELL_FS_ST_STOP;

	return CELL_OK;
}

s32 cellFsStRead(u32 fd, vm::ptr<u8> buf, u64 size, vm::ptr<u64> rsize)
{
	cellFs.Warning("cellFsStRead(fd=0x%x, buf=*0x%x, size=0x%llx, rsize=*0x%x)", fd, buf, size, rsize);
	
	std::shared_ptr<fs_file_t> file;
	if (!Emu.GetIdManager().GetIDData(fd, file))
		return CELL_ESRCH;

	// TODO: use ringbuffer (fs_config)
	fs_config.m_regid += size;

	if (file->file->Eof())
		return CELL_FS_ERANGE;

	*rsize = file->file->Read(buf.get_ptr(), size);

	return CELL_OK;
}

s32 cellFsStReadGetCurrentAddr(u32 fd, vm::ptr<vm::ptr<u8>> addr, vm::ptr<u64> size)
{
	cellFs.Todo("cellFsStReadGetCurrentAddr(fd=0x%x, addr=*0x%x, size=*0x%x)", fd, addr, size);

	std::shared_ptr<fs_file_t> file;
	if (!Emu.GetIdManager().GetIDData(fd, file))
		return CELL_ESRCH;

	return CELL_OK;
}

s32 cellFsStReadPutCurrentAddr(u32 fd, vm::ptr<u8> addr, u64 size)
{
	cellFs.Todo("cellFsStReadPutCurrentAddr(fd=0x%x, addr=*0x%x, size=0x%llx)", fd, addr, size);
	
	std::shared_ptr<fs_file_t> file;
	if (!Emu.GetIdManager().GetIDData(fd, file))
		return CELL_ESRCH;

	return CELL_OK;
}

s32 cellFsStReadWait(u32 fd, u64 size)
{
	cellFs.Todo("cellFsStReadWait(fd=0x%x, size=0x%llx)", fd, size);
	
	std::shared_ptr<fs_file_t> file;
	if (!Emu.GetIdManager().GetIDData(fd, file))
		return CELL_ESRCH;
	
	return CELL_OK;
}

s32 cellFsStReadWaitCallback(u32 fd, u64 size, vm::ptr<void(int xfd, u64 xsize)> func)
{
	cellFs.Todo("cellFsStReadWaitCallback(fd=0x%x, size=0x%llx, func=*0x%x)", fd, size, func);

	std::shared_ptr<fs_file_t> file;
	if (!Emu.GetIdManager().GetIDData(fd, file))
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
		cellFs.Error("'%s' not found! flags: 0x%02x", packed_file.c_str(), vfsRead);
		return CELL_ENOENT;
	}

	if (!unpacked_stream || !unpacked_stream->IsOpened())
	{
		cellFs.Error("'%s' couldn't be created! flags: 0x%02x", unpacked_file.c_str(), vfsWrite);
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
		return CELL_EINVAL;
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

std::mutex g_fs_aio_mutex;

using fs_aio_cb_t = vm::ptr<void(vm::ptr<CellFsAio> xaio, s32 error, s32 xid, u64 size)>;

void fsAioRead(vm::ptr<CellFsAio> aio, s32 xid, fs_aio_cb_t func)
{
	std::lock_guard<std::mutex> lock(g_fs_aio_mutex);

	s32 error = CELL_OK;
	u64 nread = 0;

	std::shared_ptr<fs_file_t> file;

	if (!Emu.GetIdManager().GetIDData(aio->fd, file) || file->flags & CELL_FS_O_WRONLY)
	{
		error = CELL_FS_EBADF;
	}
	else
	{
		const auto old_position = file->file->Tell();

		file->file->Seek(aio->offset);

		nread = file->file->Read(aio->buf.get_ptr(), aio->size);

		file->file->Seek(old_position);
	}

	// should be executed directly by FS AIO thread
	Emu.GetCallbackManager().Async([func, aio, error, xid, nread](PPUThread& CPU)
	{
		func(CPU, aio, error, xid, nread);
	});
}

void fsAioWrite(vm::ptr<CellFsAio> aio, s32 xid, fs_aio_cb_t func)
{
	std::lock_guard<std::mutex> lock(g_fs_aio_mutex);

	s32 error = CELL_OK;
	u64 nwritten = 0;

	std::shared_ptr<fs_file_t> file;

	if (!Emu.GetIdManager().GetIDData(aio->fd, file) || !(file->flags & CELL_FS_O_ACCMODE))
	{
		error = CELL_FS_EBADF;
	}
	else
	{
		const auto old_position = file->file->Tell();

		file->file->Seek(aio->offset);

		nwritten = file->file->Write(aio->buf.get_ptr(), aio->size);

		file->file->Seek(old_position);
	}

	// should be executed directly by FS AIO thread
	Emu.GetCallbackManager().Async([func, aio, error, xid, nwritten](PPUThread& CPU)
	{
		func(CPU, aio, error, xid, nwritten);
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

	thread_t("FS AIO Read Thread", std::bind(fsAioRead, aio, (*id = ++g_fs_aio_id), func)).detach();

	return CELL_OK;
}

s32 cellFsAioWrite(vm::ptr<CellFsAio> aio, vm::ptr<s32> id, fs_aio_cb_t func)
{
	cellFs.Todo("cellFsAioWrite(aio=*0x%x, id=*0x%x, func=*0x%x)", aio, id, func);

	if (!Emu.GetIdManager().CheckID<fs_file_t>(aio->fd))
	{
		return CELL_FS_EBADF;
	}

	// TODO: detect mount point and send AIO request to the AIO thread of this mount point

	thread_t("FS AIO Write Thread", std::bind(fsAioWrite, aio, (*id = ++g_fs_aio_id), func)).detach();

	return CELL_OK;
}

s32 cellFsAioCancel(s32 id)
{
	cellFs.Todo("cellFsAioCancel(id=%d) -> CELL_FS_EINVAL", id);

	// TODO: cancelled requests return CELL_FS_ECANCELED through their own callbacks

	return CELL_FS_EINVAL;
}

s32 cellFsSetDefaultContainer(u32 id, u32 total_limit)
{
	cellFs.Todo("cellFsSetDefaultContainer(id=%d, total_limit=%d)", id, total_limit);

	return CELL_OK;
}

s32 cellFsSetIoBufferFromDefaultContainer(u32 fd, u32 buffer_size, u32 page_type)
{
	cellFs.Todo("cellFsSetIoBufferFromDefaultContainer(fd=%d, buffer_size=%d, page_type=%d)", fd, buffer_size, page_type);

	std::shared_ptr<fs_file_t> file;
	if (!Emu.GetIdManager().GetIDData(fd, file))
		return CELL_ESRCH;

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
