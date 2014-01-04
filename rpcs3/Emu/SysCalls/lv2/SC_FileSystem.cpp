#include "stdafx.h"
#include "SC_FileSystem.h"
#include "Emu/SysCalls/SysCalls.h"

extern Module sys_fs;

int cellFsOpen(u32 path_addr, int flags, mem32_t fd, mem32_t arg, u64 size)
{
	const wxString& path = wxString(Memory.ReadString(path_addr), wxConvUTF8);
	sys_fs.Log("cellFsOpen(path: %s, flags: 0x%x, fd_addr: 0x%x, arg_addr: 0x%x, size: 0x%llx)",
		path.mb_str(), flags, fd.GetAddr(), arg.GetAddr(), size);

	const wxString& ppath = path;
	//ConLog.Warning("path: %s [%s]", ppath, path);

	s32 _oflags = flags;
	if(flags & CELL_O_CREAT)
	{
		_oflags &= ~CELL_O_CREAT;
		/*
		//create path
		for(uint p=1;p<ppath.Length();p++)
		{
			for(;p<ppath.Length(); p++) if(ppath[p] == '/') break;
			
			if(p == ppath.Length()) break;
			const wxString& dir = ppath(0, p);
			if(!wxDirExists(dir))
			{
				ConLog.Write("create dir: %s", dir);
				wxMkdir(dir);
			}
		}
		//create file
		if(!wxFileExists(ppath))
		{	
			wxFile f;
			f.Create(ppath);
			f.Close();
		}
		*/

		Emu.GetVFS().Create(ppath);
	}

	vfsOpenMode o_mode;

	switch(flags & CELL_O_ACCMODE)
	{
	case CELL_O_RDONLY:
		_oflags &= ~CELL_O_RDONLY;
		o_mode = vfsRead;
	break;

	case CELL_O_WRONLY:
		_oflags &= ~CELL_O_WRONLY;

		if(flags & CELL_O_APPEND)
		{
			_oflags &= ~CELL_O_APPEND;
			o_mode = vfsWriteAppend;
		}
		else if(flags & CELL_O_EXCL)
		{
			_oflags &= ~CELL_O_EXCL;
			o_mode = vfsWriteExcl;
		}
		else //if(flags & CELL_O_TRUNC)
		{
			_oflags &= ~CELL_O_TRUNC;
			o_mode = vfsWrite;
		}
	break;

	case CELL_O_RDWR:
		_oflags &= ~CELL_O_RDWR;
		o_mode = vfsReadWrite;
	break;
	}

	if(_oflags != 0)
	{
		sys_fs.Error("'%s' has unknown flags! flags: 0x%08x", ppath.mb_str(), flags);
		return CELL_EINVAL;
	}

	vfsStream* stream = Emu.GetVFS().Open(ppath, o_mode);

	if(!stream || !stream->IsOpened())
	{
		sys_fs.Error("'%s' not found! flags: 0x%08x", ppath.mb_str(), flags);
		delete stream;

		return CELL_ENOENT;
	}

	fd = sys_fs.GetNewId(stream, flags);

	return CELL_OK;
}

int cellFsRead(u32 fd, u32 buf_addr, u64 nbytes, mem64_t nread)
{
	sys_fs.Log("cellFsRead(fd: %d, buf_addr: 0x%x, nbytes: 0x%llx, nread_addr: 0x%x)",
		fd, buf_addr, nbytes, nread.GetAddr());
	ID id;
	if(!sys_fs.CheckId(fd, id)) return CELL_ESRCH;
	vfsStream& file = *(vfsStream*)id.m_data;

	if(Memory.IsGoodAddr(buf_addr) && !Memory.IsGoodAddr(buf_addr, nbytes))
	{
		MemoryBlock& block = Memory.GetMemByAddr(buf_addr);
		nbytes = block.GetSize() - (buf_addr - block.GetStartAddr());
	}

	const u64 res = nbytes ? file.Read(Memory.GetMemFromAddr(buf_addr), nbytes) : 0;

	if(nread.IsGood())
		nread = res;

	return CELL_OK;
}

int cellFsWrite(u32 fd, u32 buf_addr, u64 nbytes, mem64_t nwrite)
{
	sys_fs.Log("cellFsWrite(fd: %d, buf_addr: 0x%x, nbytes: 0x%llx, nwrite_addr: 0x%x)",
		fd, buf_addr, nbytes, nwrite.GetAddr());
	ID id;
	if(!sys_fs.CheckId(fd, id)) return CELL_ESRCH;
	vfsStream& file = *(vfsStream*)id.m_data;
	if(Memory.IsGoodAddr(buf_addr) && !Memory.IsGoodAddr(buf_addr, nbytes))
	{
		MemoryBlock& block = Memory.GetMemByAddr(buf_addr);
		nbytes = block.GetSize() - (buf_addr - block.GetStartAddr());
	}

	const u64 res = nbytes ? file.Write(Memory.GetMemFromAddr(buf_addr), nbytes) : 0;

	if(nwrite.IsGood())
		nwrite = res;

	return CELL_OK;
}

int cellFsClose(u32 fd)
{
	sys_fs.Log("cellFsClose(fd: %d)", fd);
	ID id;
	if(!sys_fs.CheckId(fd, id)) return CELL_ESRCH;
	vfsStream& file = *(vfsStream*)id.m_data;
	file.Close();
	Emu.GetIdManager().RemoveID(fd);
	return CELL_OK;
}

int cellFsOpendir(u32 path_addr, mem32_t fd)
{
	const wxString& path = Memory.ReadString(path_addr);
	sys_fs.Error("cellFsOpendir(path_addr: 0x%x(%s), fd_addr: 0x%x)", path_addr, path.mb_str(), fd.GetAddr());
	if(!Memory.IsGoodAddr(path_addr) || !fd.IsGood()) return CELL_EFAULT;

	return CELL_OK;
}

int cellFsReaddir(u32 fd, u32 dir_addr, mem64_t nread)
{
	sys_fs.Error("cellFsReaddir(fd: %d, dir_addr: 0x%x, nread_addr: 0x%x)", fd, dir_addr, nread.GetAddr());
	return CELL_OK;
}

int cellFsClosedir(u32 fd)
{
	sys_fs.Error("cellFsClosedir(fd: %d)", fd);
	return CELL_OK;
}

int cellFsStat(const u32 path_addr, mem_ptr_t<CellFsStat> sb)
{
	const wxString& path = Memory.ReadString(path_addr);
	sys_fs.Log("cellFsStat(path: %s, sb_addr: 0x%x)", path.mb_str(), sb.GetAddr());

	sb->st_mode = 
		CELL_FS_S_IRUSR | CELL_FS_S_IWUSR | CELL_FS_S_IXUSR |
		CELL_FS_S_IRGRP | CELL_FS_S_IWGRP | CELL_FS_S_IXGRP |
		CELL_FS_S_IROTH | CELL_FS_S_IWOTH | CELL_FS_S_IXOTH;

	sb->st_uid = 0;
	sb->st_gid = 0;
	sb->st_atime_ = 0; //TODO
	sb->st_mtime_ = 0; //TODO
	sb->st_ctime_ = 0; //TODO
	sb->st_blksize = 4096;

	// Check if path is a mount point. (TODO: Add information in sb_addr)
	for(u32 i=0; i<Emu.GetVFS().m_devices.GetCount(); ++i)
	{
		if(path.CmpNoCase(Emu.GetVFS().m_devices[i].GetPs3Path().RemoveLast(1)) == 0)
		{
			sys_fs.Log("cellFsFstat: '%s' is a mount point.", path.mb_str());
			sb->st_mode |= CELL_FS_S_IFDIR;
			return CELL_OK;
		}
	}

	vfsFile f(path);

	if(!f.IsOpened())
	{
		sys_fs.Warning("cellFsFstat: '%s' not found.", path.mb_str());
		return CELL_ENOENT;
	}

	sb->st_mode |= CELL_FS_S_IFREG; //TODO: dir CELL_FS_S_IFDIR
	sb->st_size = f.GetSize();

	return CELL_OK;
}

int cellFsFstat(u32 fd, mem_ptr_t<CellFsStat> sb)
{
	sys_fs.Log("cellFsFstat(fd: %d, sb_addr: 0x%x)", fd, sb.GetAddr());
	ID id;
	if(!sys_fs.CheckId(fd, id)) return CELL_ESRCH;
	vfsStream& file = *(vfsStream*)id.m_data;

	sb->st_mode = 
		CELL_FS_S_IRUSR | CELL_FS_S_IWUSR | CELL_FS_S_IXUSR |
		CELL_FS_S_IRGRP | CELL_FS_S_IWGRP | CELL_FS_S_IXGRP |
		CELL_FS_S_IROTH | CELL_FS_S_IWOTH | CELL_FS_S_IXOTH;

	sb->st_mode |= CELL_FS_S_IFREG; //TODO: dir CELL_FS_S_IFDIR
	sb->st_uid = 0;
	sb->st_gid = 0;
	sb->st_atime_ = 0; //TODO
	sb->st_mtime_ = 0; //TODO
	sb->st_ctime_ = 0; //TODO
	sb->st_size = file.GetSize();
	sb->st_blksize = 4096;

	return CELL_OK;
}

int cellFsMkdir(u32 path_addr, u32 mode)
{
	const wxString& ps3_path = Memory.ReadString(path_addr);
	wxString path;
	Emu.GetVFS().GetDevice(ps3_path, path);
	sys_fs.Log("cellFsMkdir(path: %s, mode: 0x%x)", path.mb_str(), mode);
	if(wxDirExists(path)) return CELL_EEXIST;
	if(!wxMkdir(path)) return CELL_EBUSY;
	return CELL_OK;
}

int cellFsRename(u32 from_addr, u32 to_addr)
{
	const wxString& ps3_from = Memory.ReadString(from_addr);
	const wxString& ps3_to = Memory.ReadString(to_addr);
	wxString from;
	wxString to;
	Emu.GetVFS().GetDevice(ps3_from, from);
	Emu.GetVFS().GetDevice(ps3_to, to);

	sys_fs.Log("cellFsRename(from: %s, to: %s)", from.mb_str(), to.mb_str());
	if(!wxFileExists(from)) return CELL_ENOENT;
	if(wxFileExists(to)) return CELL_EEXIST;
	if(!wxRenameFile(from, to)) return CELL_EBUSY; // (TODO: RenameFile(a,b) = CopyFile(a,b) + RemoveFile(a), therefore file "a" will not be removed if it is opened)
	return CELL_OK;
}

int cellFsRmdir(u32 path_addr)
{
	const wxString& ps3_path = Memory.ReadString(path_addr);
	wxString path;
	Emu.GetVFS().GetDevice(ps3_path, path);
	sys_fs.Log("cellFsRmdir(path: %s)", path.mb_str());
	if(!wxDirExists(path)) return CELL_ENOENT;
	if(!wxRmdir(path)) return CELL_EBUSY; // (TODO: Under certain conditions it is not able to delete the folder)
	return CELL_OK;
}

int cellFsUnlink(u32 path_addr)
{
	const wxString& ps3_path = Memory.ReadString(path_addr);
	wxString path;
	Emu.GetVFS().GetDevice(ps3_path, path);
	sys_fs.Error("cellFsUnlink(path: %s)", path.mb_str());
	return CELL_OK;
}

int cellFsLseek(u32 fd, s64 offset, u32 whence, mem64_t pos)
{
	vfsSeekMode seek_mode;
	sys_fs.Log("cellFsLseek(fd: %d, offset: 0x%llx, whence: %d, pos_addr: 0x%x)", fd, offset, whence, pos.GetAddr());
	switch(whence)
	{
	case CELL_SEEK_SET: seek_mode = vfsSeekSet; break;
	case CELL_SEEK_CUR: seek_mode = vfsSeekCur; break;
	case CELL_SEEK_END: seek_mode = vfsSeekEnd; break;
	default:
		sys_fs.Error(fd, "Unknown seek whence! (%d)", whence);
	return CELL_EINVAL;
	}
	ID id;
	if(!sys_fs.CheckId(fd, id)) return CELL_ESRCH;
	vfsStream& file = *(vfsStream*)id.m_data;
	pos = file.Seek(offset, seek_mode);
	return CELL_OK;
}

int cellFsFtruncate(u32 fd, u64 size)
{
	sys_fs.Log("cellFsFtruncate(fd: %d, size: %lld)", fd, size);
	ID id;
	if(!sys_fs.CheckId(fd, id)) return CELL_ESRCH;
	vfsStream& file = *(vfsStream*)id.m_data;
	u64 initialSize = file.GetSize();

	if (initialSize < size)
	{
		u64 last_pos = file.Tell();
		file.Seek(0, vfsSeekEnd);
		static const char nullbyte = 0;
		file.Seek(size-initialSize-1, vfsSeekCur);
		file.Write(&nullbyte, sizeof(char));
		file.Seek(last_pos, vfsSeekSet);
	}

	if (initialSize > size)
	{
		// (TODO)
	}

	return CELL_OK;
}

int cellFsTruncate(u32 path_addr, u64 size)
{
	const wxString& path = Memory.ReadString(path_addr);
	sys_fs.Log("cellFsTruncate(path: %s, size: %lld)", path.mb_str(), size);

	vfsFile f(path, vfsReadWrite);
	if(!f.IsOpened())
	{
		sys_fs.Warning("cellFsTruncate: '%s' not found.", path.mb_str());
		return CELL_ENOENT;
	}
	u64 initialSize = f.GetSize();

	if (initialSize < size)
	{
		u64 last_pos = f.Tell();
		f.Seek(0, vfsSeekEnd);
		static const char nullbyte = 0;
		f.Seek(size-initialSize-1, vfsSeekCur);
		f.Write(&nullbyte, sizeof(char));
		f.Seek(last_pos, vfsSeekSet);
	}

	if (initialSize > size)
	{
		// (TODO)
	}

	return CELL_OK;
}

int cellFsFGetBlockSize(u32 fd, mem64_t sector_size, mem64_t block_size)
{
	sys_fs.Log("cellFsFGetBlockSize(fd: %d, sector_size_addr: 0x%x, block_size_addr: 0x%x)", fd, sector_size.GetAddr(), block_size.GetAddr());
	
	sector_size = 4096; // ?
	block_size = 4096; // ?

	return CELL_OK;
}

std::atomic<u32> g_FsAioReadID = 0;

int cellFsAioRead(mem_ptr_t<CellFsAio> aio, mem32_t aio_id, u32 func_addr)
{
	sys_fs.Warning("cellFsAioRead(aio_addr: 0x%x, id_addr: 0x%x, func_addr: 0x%x)", aio.GetAddr(), aio_id.GetAddr(), func_addr);

	ID id;
	u32 fd = (u32)aio->fd;
	if(!sys_fs.CheckId(fd, id)) return CELL_ESRCH;
	vfsFileBase& orig_file = *(vfsFileBase*)id.m_data;
	//open the file again (to prevent access conflicts roughly)
	vfsStream file = *Emu.GetVFS().Open(orig_file.GetPath(), vfsRead);

	u64 nbytes = (u64)aio->size;
	const u32 buf_addr = (u32)aio->buf_addr;
	if(Memory.IsGoodAddr(buf_addr) && !Memory.IsGoodAddr(buf_addr, nbytes))
	{
		MemoryBlock& block = Memory.GetMemByAddr(buf_addr);
		nbytes = block.GetSize() - (buf_addr - block.GetStartAddr());
	}

	//read data immediately (actually it should be read in special thread)
	file.Seek((u64)aio->offset);
	const u64 res = nbytes ? file.Read(Memory.GetMemFromAddr(buf_addr), nbytes) : 0;
	file.Close();

	//get a unique id for the callback
	const u32 xid = g_FsAioReadID++;
	aio_id = xid;

	//TODO: init the callback
	/*CPUThread& new_thread = Emu.GetCPU().AddThread(CPU_THREAD_PPU);
	new_thread.SetEntry(func_addr);
	new_thread.SetPrio(1001);
	new_thread.SetStackSize(0x10000);
	new_thread.SetName("FsAioReadCallback");
	new_thread.SetArg(0, aio.GetAddr()); //xaio
	new_thread.SetArg(1, CELL_OK); //error code
	new_thread.SetArg(2, xid); //xid (unique id)
	new_thread.SetArg(3, res); //size (bytes read)
	new_thread.Run();
	new_thread.Exec();*/
	
	return CELL_OK;
}
