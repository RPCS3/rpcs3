#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void sys_fs_init();
Module sys_fs(0x000e, sys_fs_init);

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

int sdata_unpack(wxString packed_file, wxString unpacked_file)
{
	std::shared_ptr<vfsFileBase> packed_stream(Emu.GetVFS().OpenFile(packed_file, vfsRead));
	std::shared_ptr<vfsFileBase> unpacked_stream(Emu.GetVFS().OpenFile(unpacked_file, vfsWrite));
	
	if(!packed_stream || !packed_stream->IsOpened())
	{
		sys_fs.Error("'%s' not found! flags: 0x%08x", packed_file.wx_str(), vfsRead);
		return CELL_ENOENT;
	}

	if(!unpacked_stream || !unpacked_stream->IsOpened())
	{
		sys_fs.Error("'%s' couldn't be created! flags: 0x%08x", unpacked_file.wx_str(), vfsWrite);
		return CELL_ENOENT;
	}

	char buffer [10200];
	packed_stream->Read(buffer, 256);
	u32 format = re32(*(u32*)&buffer[0]);
	if (format != 0x4E504400) // "NPD\x00"
	{
		sys_fs.Error("Illegal format. Expected 0x4E504400, but got 0x%08x", format);
		return CELL_EFSSPECIFIC;
	}

	u32 version	       = re32(*(u32*)&buffer[0x04]);
	u32 flags          = re32(*(u32*)&buffer[0x80]);
	u32 blockSize      = re32(*(u32*)&buffer[0x84]);
	u64 filesizeOutput = re64(*(u64*)&buffer[0x88]);
	u64 filesizeInput  = packed_stream->GetSize();
	u32 blockCount     = (filesizeOutput + blockSize-1) / blockSize;

	// SDATA file is compressed
	if (flags & 0x1)
	{
		sys_fs.Warning("cellFsSdataOpen: Compressed SDATA files are not supported yet.");
		return CELL_EFSSPECIFIC;
	}

	// SDATA file is NOT compressed
	else 
	{
		u32 t1 = (flags & 0x20) ? 0x20 : 0x10;
		u32 startOffset = (blockCount * t1) + 0x100;
		u64 filesizeTmp = (filesizeOutput+0xF)&0xFFFFFFF0 + startOffset;

		if (!sdata_check(version, flags, filesizeInput, filesizeTmp))
		{
			sys_fs.Error("cellFsSdataOpen: Wrong header information.");
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

			if (!(blockCount-i-1))
				blockSize = filesizeOutput-i*blockSize;

			packed_stream->Read(buffer+256, blockSize);
			unpacked_stream->Write(buffer+256, blockSize);
		}
	}

	return CELL_OK;
}
	

int cellFsSdataOpen(u32 path_addr, int flags, mem32_t fd, mem32_t arg, u64 size)
{
	const wxString& path = Memory.ReadString(path_addr);
	sys_fs.Warning("cellFsSdataOpen(path=\"%s\", flags=0x%x, fd_addr=0x%x, arg_addr=0x%x, size=0x%llx)",
		path.wx_str(), flags, fd.GetAddr(), arg.GetAddr(), size);

	if (!fd.IsGood() || (!arg.IsGood() && size))
		return CELL_EFAULT;

	if (flags != CELL_O_RDONLY)
		return CELL_EINVAL;

	if (!path.Lower().EndsWith(".sdat"))
		return CELL_ENOTSDATA;

	wxString unpacked_path = "/dev_hdd1/"+path.AfterLast('/')+".unpacked";
	int ret = sdata_unpack(path, unpacked_path);
	if (ret) return ret;

	fd = sys_fs.GetNewId(Emu.GetVFS().OpenFile(unpacked_path, vfsRead), flags);

	return CELL_OK;
}

std::atomic<u32> g_FsAioReadID( 0 );
std::atomic<u32> g_FsAioReadCur( 0 );
bool aio_init = false;

void fsAioRead(u32 fd, mem_ptr_t<CellFsAio> aio, int xid, mem_func_ptr_t<void (*)(mem_ptr_t<CellFsAio> xaio, int error, int xid, u64 size)> func)
{
	while (g_FsAioReadCur != xid)
	{
		Sleep(1);
		if (Emu.IsStopped())
		{
			ConLog.Warning("fsAioRead() aborted");
			return;
		}
	}

	vfsFileBase* orig_file;
	if(!sys_fs.CheckId(fd, orig_file)) return;

	const wxString path = orig_file->GetPath().AfterFirst('/');

	u64 nbytes = aio->size;
	u32 buf_addr = aio->buf_addr;

	u32 res = 0;
	u32 error = CELL_OK;

	vfsStream& file = *(vfsStream*)orig_file;
	const u64 old_pos = file.Tell();
	file.Seek((u64)aio->offset);

	u32 count = nbytes;
	if (nbytes != (u64)count)
	{
		error = CELL_ENOMEM;
		goto fin;
	}

	if (!Memory.IsGoodAddr(buf_addr))
	{
		error = CELL_EFAULT;
		goto fin;
	}

	if (count) if (u32 frag = buf_addr & 4095) // memory page fragment
	{
		u32 req = min(count, 4096 - frag);
		u32 read = file.Read(Memory + buf_addr, req);
		buf_addr += req;
		res += read;
		count -= req;
		if (read < req) goto fin;
	}

	for (u32 pages = count / 4096; pages > 0; pages--) // full pages
	{
		if (!Memory.IsGoodAddr(buf_addr)) goto fin; // ??? (probably EFAULT)
		u32 read = file.Read(Memory + buf_addr, 4096);
		buf_addr += 4096;
		res += read;
		count -= 4096;
		if (read < 4096) goto fin;
	}

	if (count) // last fragment
	{
		if (!Memory.IsGoodAddr(buf_addr)) goto fin;
		res += file.Read(Memory + buf_addr, count);
	}

fin:
	file.Seek(old_pos);

	ConLog.Warning("*** fsAioRead(fd=%d, offset=0x%llx, buf_addr=0x%x, size=0x%x, error=0x%x, res=0x%x, xid=0x%x [%s])",
		fd, (u64)aio->offset, buf_addr, (u64)aio->size, error, res, xid, path.wx_str());

	if (func) // start callback thread
	{
		func.async(aio, error, xid, res);
	}

	/*CPUThread& thr = Emu.GetCallbackThread();
	while (thr.IsAlive())
	{
		Sleep(1);
		if (Emu.IsStopped())
		{
			ConLog.Warning("fsAioRead() aborted");
			break;
		}
	}*/

	g_FsAioReadCur++;
}

int cellFsAioRead(mem_ptr_t<CellFsAio> aio, mem32_t aio_id, mem_func_ptr_t<void (*)(mem_ptr_t<CellFsAio> xaio, int error, int xid, u64 size)> func)
{
	sys_fs.Warning("cellFsAioRead(aio_addr=0x%x, id_addr=0x%x, func_addr=0x%x)", aio.GetAddr(), aio_id.GetAddr(), func.GetAddr());

	if (!aio.IsGood() || !aio_id.IsGood() || !func.IsGood())
	{
		return CELL_EFAULT;
	}

	if (!aio_init)
	{
		return CELL_ENXIO;
	}

	vfsFileBase* orig_file;
	u32 fd = aio->fd;
	if (!sys_fs.CheckId(fd, orig_file)) return CELL_EBADF;

	//get a unique id for the callback (may be used by cellFsAioCancel)
	const u32 xid = g_FsAioReadID++;
	aio_id = xid;

	{
		thread t("fsAioRead", std::bind(fsAioRead, fd, aio, xid, func));
		t.detach();
	}

	return CELL_OK;
}

int cellFsAioInit(mem8_ptr_t mount_point)
{
	wxString mp = Memory.ReadString(mount_point.GetAddr());
	sys_fs.Warning("cellFsAioInit(mount_point_addr=0x%x (%s))", mount_point.GetAddr(), mp.wx_str());
	aio_init = true;
	return CELL_OK;
}

int cellFsAioFinish(mem8_ptr_t mount_point)
{
	wxString mp = Memory.ReadString(mount_point.GetAddr());
	sys_fs.Warning("cellFsAioFinish(mount_point_addr=0x%x (%s))", mount_point.GetAddr(), mp.wx_str());
	aio_init = false;
	return CELL_OK;
}

int cellFsReadWithOffset(u32 fd, u64 offset, u32 buf_addr, u64 buffer_size, mem64_t nread)
{
	sys_fs.Warning("cellFsReadWithOffset(fd=%d, offset=0x%llx, buf_addr=0x%x, buffer_size=%lld nread=0x%llx)",
		fd, offset, buf_addr, buffer_size, nread.GetAddr());

	int ret;
	MemoryAllocator<be_t<u64>> oldPos, newPos;
	ret = cellFsLseek(fd, 0, CELL_SEEK_CUR, oldPos.GetAddr());       // Save the current position
	if (ret) return ret;
	ret = cellFsLseek(fd, offset, CELL_SEEK_SET, newPos.GetAddr());  // Move to the specified offset
	if (ret) return ret;
	ret = cellFsRead(fd, buf_addr, buffer_size, nread.GetAddr());    // Read the file
	if (ret) return ret;
	ret = cellFsLseek(fd, Memory.Read64(oldPos.GetAddr()), CELL_SEEK_SET, newPos.GetAddr());  // Return to the old position
	if (ret) return ret;

	return CELL_OK;
}

void sys_fs_init()
{
	sys_fs.AddFunc(0x718bf5f8, cellFsOpen);
	sys_fs.AddFunc(0xb1840b53, cellFsSdataOpen);
	sys_fs.AddFunc(0x4d5ff8e2, cellFsRead);
	sys_fs.AddFunc(0xecdcf2ab, cellFsWrite);
	sys_fs.AddFunc(0x2cb51f0d, cellFsClose);
	sys_fs.AddFunc(0x3f61245c, cellFsOpendir);
	sys_fs.AddFunc(0x5c74903d, cellFsReaddir);
	sys_fs.AddFunc(0xff42dcc3, cellFsClosedir);
	sys_fs.AddFunc(0x7de6dced, cellFsStat);
	sys_fs.AddFunc(0xef3efa34, cellFsFstat);
	sys_fs.AddFunc(0xba901fe6, cellFsMkdir);
	sys_fs.AddFunc(0xf12eecc8, cellFsRename);
	sys_fs.AddFunc(0x2796fdf3, cellFsRmdir);
	sys_fs.AddFunc(0x7f4677a8, cellFsUnlink);
	sys_fs.AddFunc(0xa397d042, cellFsLseek);
	sys_fs.AddFunc(0x0e2939e5, cellFsFtruncate);
	sys_fs.AddFunc(0xc9dc3ac5, cellFsTruncate);
	sys_fs.AddFunc(0xcb588dba, cellFsFGetBlockSize);
	sys_fs.AddFunc(0xc1c507e7, cellFsAioRead);
	sys_fs.AddFunc(0xdb869f20, cellFsAioInit);
	sys_fs.AddFunc(0x9f951810, cellFsAioFinish);
	sys_fs.AddFunc(0x1a108ab7, cellFsGetBlockSize);
	sys_fs.AddFunc(0xaa3b4bcd, cellFsGetFreeSize);
	sys_fs.AddFunc(0x0d5b4a14, cellFsReadWithOffset);
	sys_fs.AddFunc(0x9b882495, cellFsGetDirectoryEntries);
	sys_fs.AddFunc(0x2664c8ae, cellFsStReadInit);
	sys_fs.AddFunc(0xd73938df, cellFsStReadFinish);
	sys_fs.AddFunc(0xb3afee8b, cellFsStReadGetRingBuf);
	sys_fs.AddFunc(0xcf34969c, cellFsStReadGetStatus);
	sys_fs.AddFunc(0xbd273a88, cellFsStReadGetRegid);
	sys_fs.AddFunc(0x8df28ff9, cellFsStReadStart);
	sys_fs.AddFunc(0xf8e5d9a0, cellFsStReadStop);
	sys_fs.AddFunc(0x27800c6b, cellFsStRead);
	sys_fs.AddFunc(0x190912f6, cellFsStReadGetCurrentAddr);
	sys_fs.AddFunc(0x81f33783, cellFsStReadPutCurrentAddr);
	sys_fs.AddFunc(0x8f71c5b2, cellFsStReadWait);
	sys_fs.AddFunc(0x866f6aec, cellFsStReadWaitCallback);

	aio_init = false;
}
