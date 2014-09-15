#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/Callback.h"

#include "Emu/FS/VFS.h"
#include "Emu/FS/vfsFileBase.h"
#include "Emu/SysCalls/lv2/lv2Fs.h"

Module *sys_fs = nullptr;

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
	
	if(!packed_stream || !packed_stream->IsOpened())
	{
		sys_fs->Error("'%s' not found! flags: 0x%08x", packed_file.c_str(), vfsRead);
		return CELL_ENOENT;
	}

	if(!unpacked_stream || !unpacked_stream->IsOpened())
	{
		sys_fs->Error("'%s' couldn't be created! flags: 0x%08x", unpacked_file.c_str(), vfsWrite);
		return CELL_ENOENT;
	}

	char buffer [10200];
	packed_stream->Read(buffer, 256);
	u32 format = re32(*(u32*)&buffer[0]);
	if (format != 0x4E504400) // "NPD\x00"
	{
		sys_fs->Error("Illegal format. Expected 0x4E504400, but got 0x%08x", format);
		return CELL_EFSSPECIFIC;
	}

	u32 version	       = re32(*(u32*)&buffer[0x04]);
	u32 flags          = re32(*(u32*)&buffer[0x80]);
	u32 blockSize      = re32(*(u32*)&buffer[0x84]);
	u64 filesizeOutput = re64(*(u64*)&buffer[0x88]);
	u64 filesizeInput  = packed_stream->GetSize();
	u32 blockCount     = (u32)((filesizeOutput + blockSize - 1) / blockSize);

	// SDATA file is compressed
	if (flags & 0x1)
	{
		sys_fs->Warning("cellFsSdataOpen: Compressed SDATA files are not supported yet.");
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
			sys_fs->Error("cellFsSdataOpen: Wrong header information.");
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
				blockSize = (u32)(filesizeOutput - i * blockSize);

			packed_stream->Read(buffer+256, blockSize);
			unpacked_stream->Write(buffer+256, blockSize);
		}
	}

	return CELL_OK;
}
	

int cellFsSdataOpen(vm::ptr<const char> path, int flags, vm::ptr<be_t<u32>> fd, vm::ptr<be_t<u32>> arg, u64 size)
{
	sys_fs->Warning("cellFsSdataOpen(path=\"%s\", flags=0x%x, fd_addr=0x%x, arg_addr=0x%x, size=0x%llx) -> cellFsOpen()",
		path.get_ptr(), flags, fd.addr(), arg.addr(), size);

	/*if (flags != CELL_O_RDONLY)
		return CELL_EINVAL;

	std::string suffix = path.substr(path.length() - 5, 5);
	if (suffix != ".sdat" && suffix != ".SDAT")
		return CELL_ENOTSDATA;

	std::string::size_type last_slash = path.rfind('/'); //TODO: use a filesystem library to solve this more robustly
	last_slash = last_slash == std::string::npos ? 0 : last_slash+1;
	std::string unpacked_path = "/dev_hdd1/"+path.substr(last_slash,path.length()-last_slash)+".unpacked";
	int ret = sdata_unpack(path, unpacked_path);
	if (ret) return ret;

	fd = sys_fs->GetNewId(Emu.GetVFS().OpenFile(unpacked_path, vfsRead), TYPE_FS_FILE);

	return CELL_OK;*/

	return cellFsOpen(path, flags, fd, arg, size);
}

int cellFsSdataOpenByFd(int mself_fd, int flags, vm::ptr<be_t<u32>> sdata_fd, u64 offset, vm::ptr<be_t<u32>> arg, u64 size)
{
	sys_fs->Todo("cellFsSdataOpenByFd(mself_fd=0x%x, flags=0x%x, sdata_fd_addr=0x%x, offset=0x%llx, arg_addr=0x%x, size=0x%llx) -> cellFsOpen()",
		mself_fd, flags, sdata_fd.addr(), offset, arg.addr(), size);

	// TODO:

	return CELL_OK;
}

std::atomic<u32> g_FsAioReadID( 0 );
std::atomic<u32> g_FsAioReadCur( 0 );
bool aio_init = false;

void fsAioRead(u32 fd, vm::ptr<CellFsAio> aio, int xid, vm::ptr<void (*)(vm::ptr<CellFsAio> xaio, int error, int xid, u64 size)> func)
{
	while (g_FsAioReadCur != xid)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		if (Emu.IsStopped())
		{
			sys_fs->Warning("fsAioRead() aborted");
			return;
		}
	}

	u32 error = CELL_OK;
	u64 res = 0;
	{
		LV2_LOCK(0);

		vfsFileBase* orig_file;
		if (!sys_fs->CheckId(fd, orig_file))
		{
			sys_fs->Error("Wrong fd (%s)", fd);
			Emu.Pause();
			return;
		}

		u64 nbytes = aio->size;

		vfsStream& file = *(vfsStream*)orig_file;
		const u64 old_pos = file.Tell();
		file.Seek((u64)aio->offset);

		// TODO: use code from cellFsRead or something
		if (nbytes != (u32)nbytes)
		{
			error = CELL_ENOMEM;
		}
		else
		{
			res = nbytes ? file.Read(aio->buf.get_ptr(), nbytes) : 0;
		}

		file.Seek(old_pos);

		sys_fs->Log("*** fsAioRead(fd=%d, offset=0x%llx, buf_addr=0x%x, size=0x%x, error=0x%x, res=0x%x, xid=0x%x [%s])",
			fd, (u64)aio->offset, aio->buf.addr(), (u64)aio->size, error, res, xid, orig_file->GetPath().c_str());
	}	

	if (func) // start callback thread
	{
		Emu.GetCallbackManager().Async([func, aio, error, xid, res]()
		{
			func(aio, error, xid, res);
		});
	}

	g_FsAioReadCur++;
}

int cellFsAioRead(vm::ptr<CellFsAio> aio, vm::ptr<be_t<u32>> aio_id, vm::ptr<void(*)(vm::ptr<CellFsAio> xaio, int error, int xid, u64 size)> func)
{
	sys_fs->Warning("cellFsAioRead(aio_addr=0x%x, id_addr=0x%x, func_addr=0x%x)", aio.addr(), aio_id.addr(), func.addr());

	LV2_LOCK(0);

	if (!aio_init)
	{
		return CELL_ENXIO;
	}

	vfsFileBase* orig_file;
	u32 fd = aio->fd;

	if (!sys_fs->CheckId(fd, orig_file))
	{
		return CELL_EBADF;
	}

	//get a unique id for the callback (may be used by cellFsAioCancel)
	const u32 xid = g_FsAioReadID++;
	*aio_id = xid;

	{
		thread t("fsAioRead", std::bind(fsAioRead, fd, aio, xid, func));
		t.detach();
	}

	return CELL_OK;
}

int cellFsAioWrite(vm::ptr<CellFsAio> aio, vm::ptr<be_t<u32>> aio_id, vm::ptr<void(*)(vm::ptr<CellFsAio> xaio, int error, int xid, u64 size)> func)
{
	sys_fs->Todo("cellFsAioWrite(aio_addr=0x%x, id_addr=0x%x, func_addr=0x%x)", aio.addr(), aio_id.addr(), func.addr());

	LV2_LOCK(0);

	// TODO:

	return CELL_OK;
}

int cellFsAioInit(vm::ptr<const char> mount_point)
{
	sys_fs->Warning("cellFsAioInit(mount_point_addr=0x%x (%s))", mount_point.addr(), mount_point.get_ptr());

	LV2_LOCK(0);

	aio_init = true;
	return CELL_OK;
}

int cellFsAioFinish(vm::ptr<const char> mount_point)
{
	sys_fs->Warning("cellFsAioFinish(mount_point_addr=0x%x (%s))", mount_point.addr(), mount_point.get_ptr());

	LV2_LOCK(0);

	aio_init = false;
	return CELL_OK;
}

int cellFsReadWithOffset(u32 fd, u64 offset, vm::ptr<void> buf, u64 buffer_size, vm::ptr<be_t<u64>> nread)
{
	sys_fs->Warning("cellFsReadWithOffset(fd=%d, offset=0x%llx, buf_addr=0x%x, buffer_size=%lld nread=0x%llx)",
		fd, offset, buf.addr(), buffer_size, nread.addr());

	LV2_LOCK(0);

	int ret;
	vm::var<be_t<u64>> oldPos, newPos;
	ret = cellFsLseek(fd, 0, CELL_SEEK_CUR, oldPos);       // Save the current position
	if (ret) return ret;
	ret = cellFsLseek(fd, offset, CELL_SEEK_SET, newPos);  // Move to the specified offset
	if (ret) return ret;
	ret = cellFsRead(fd, buf, buffer_size, nread);    // Read the file
	if (ret) return ret;
	ret = cellFsLseek(fd, oldPos.value(), CELL_SEEK_SET, newPos);  // Return to the old position
	if (ret) return ret;

	return CELL_OK;
}

void sys_fs_init(Module *pxThis)
{
	sys_fs = pxThis;

	sys_fs->AddFunc(0x718bf5f8, cellFsOpen);
	sys_fs->AddFunc(0xb1840b53, cellFsSdataOpen);
	sys_fs->AddFunc(0x6d3bb15b, cellFsSdataOpenByFd);
	sys_fs->AddFunc(0x4d5ff8e2, cellFsRead);
	sys_fs->AddFunc(0xecdcf2ab, cellFsWrite);
	sys_fs->AddFunc(0x2cb51f0d, cellFsClose);
	sys_fs->AddFunc(0x3f61245c, cellFsOpendir);
	sys_fs->AddFunc(0x5c74903d, cellFsReaddir);
	sys_fs->AddFunc(0xff42dcc3, cellFsClosedir);
	sys_fs->AddFunc(0x7de6dced, cellFsStat);
	sys_fs->AddFunc(0xef3efa34, cellFsFstat);
	sys_fs->AddFunc(0xba901fe6, cellFsMkdir);
	sys_fs->AddFunc(0xf12eecc8, cellFsRename);
	sys_fs->AddFunc(0x99406d0b, cellFsChmod);
	sys_fs->AddFunc(0x967a162b, cellFsFsync);
	sys_fs->AddFunc(0x2796fdf3, cellFsRmdir);
	sys_fs->AddFunc(0x7f4677a8, cellFsUnlink);
	sys_fs->AddFunc(0xa397d042, cellFsLseek);
	sys_fs->AddFunc(0x0e2939e5, cellFsFtruncate);
	sys_fs->AddFunc(0xc9dc3ac5, cellFsTruncate);
	sys_fs->AddFunc(0xcb588dba, cellFsFGetBlockSize);
	sys_fs->AddFunc(0xc1c507e7, cellFsAioRead);
	sys_fs->AddFunc(0x4cef342e, cellFsAioWrite);
	sys_fs->AddFunc(0xdb869f20, cellFsAioInit);
	sys_fs->AddFunc(0x9f951810, cellFsAioFinish);
	sys_fs->AddFunc(0x1a108ab7, cellFsGetBlockSize);
	sys_fs->AddFunc(0xaa3b4bcd, cellFsGetFreeSize);
	sys_fs->AddFunc(0x0d5b4a14, cellFsReadWithOffset);
	sys_fs->AddFunc(0x9b882495, cellFsGetDirectoryEntries);
	sys_fs->AddFunc(0x2664c8ae, cellFsStReadInit);
	sys_fs->AddFunc(0xd73938df, cellFsStReadFinish);
	sys_fs->AddFunc(0xb3afee8b, cellFsStReadGetRingBuf);
	sys_fs->AddFunc(0xcf34969c, cellFsStReadGetStatus);
	sys_fs->AddFunc(0xbd273a88, cellFsStReadGetRegid);
	sys_fs->AddFunc(0x8df28ff9, cellFsStReadStart);
	sys_fs->AddFunc(0xf8e5d9a0, cellFsStReadStop);
	sys_fs->AddFunc(0x27800c6b, cellFsStRead);
	sys_fs->AddFunc(0x190912f6, cellFsStReadGetCurrentAddr);
	sys_fs->AddFunc(0x81f33783, cellFsStReadPutCurrentAddr);
	sys_fs->AddFunc(0x8f71c5b2, cellFsStReadWait);
	sys_fs->AddFunc(0x866f6aec, cellFsStReadWaitCallback);
}

void sys_fs_load()
{
	g_FsAioReadID = 0;
	g_FsAioReadCur = 0;
	aio_init = false;
}
