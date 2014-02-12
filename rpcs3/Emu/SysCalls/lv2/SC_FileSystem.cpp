#include "stdafx.h"
#include "SC_FileSystem.h"
#include "Emu/SysCalls/SysCalls.h"

extern Module sys_fs;

int cellFsOpen(u32 path_addr, int flags, mem32_t fd, mem32_t arg, u64 size)
{
	const wxString& path = Memory.ReadString(path_addr);
	sys_fs.Log("cellFsOpen(path=\"%s\", flags=0x%x, fd_addr=0x%x, arg_addr=0x%x, size=0x%llx)",
		path.wx_str(), flags, fd.GetAddr(), arg.GetAddr(), size);

	const wxString& ppath = path;
	//ConLog.Warning("path: %s [%s]", ppath.wx_str(), path.wx_str());

	s32 _oflags = flags;
	if(flags & CELL_O_CREAT)
	{
		_oflags &= ~CELL_O_CREAT;
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
		else //if (flags & CELL_O_TRUNC)
		{
			_oflags &= ~CELL_O_TRUNC;
			o_mode = vfsWrite;
		}
	break;

	case CELL_O_RDWR:
		_oflags &= ~CELL_O_RDWR;
		if (flags & CELL_O_TRUNC)
		{
			_oflags &= ~CELL_O_TRUNC;
			//truncate file before opening it as read/write
			vfsStream* stream = Emu.GetVFS().Open(ppath, vfsWrite);
			stream->Close();
			delete stream;
		}
		o_mode = vfsReadWrite;
	break;
	}

	if(_oflags != 0)
	{
		sys_fs.Error("\"%s\" has unknown flags! flags: 0x%08x", ppath.wx_str(), flags);
		return CELL_EINVAL;
	}

	vfsStream* stream = Emu.GetVFS().Open(ppath, o_mode);

	if(!stream || !stream->IsOpened())
	{
		sys_fs.Error("\"%s\" not found! flags: 0x%08x", ppath.wx_str(), flags);
		delete stream;

		return CELL_ENOENT;
	}

	fd = sys_fs.GetNewId(stream, flags);
	ConLog.Warning("*** cellFsOpen(path=\"%s\"): fd = %d", path.wx_str(), fd.GetValue());

	return CELL_OK;
}

int cellFsRead(u32 fd, u32 buf_addr, u64 nbytes, mem64_t nread)
{
	sys_fs.Log("cellFsRead(fd=%d, buf_addr=0x%x, nbytes=0x%llx, nread_addr=0x%x)",
		fd, buf_addr, nbytes, nread.GetAddr());
	vfsStream* file;
	if(!sys_fs.CheckId(fd, file)) return CELL_ESRCH;

	if(Memory.IsGoodAddr(buf_addr) && !Memory.IsGoodAddr(buf_addr, nbytes))
	{
		MemoryBlock& block = Memory.GetMemByAddr(buf_addr);
		nbytes = block.GetSize() - (buf_addr - block.GetStartAddr());
	}

	const u64 res = nbytes ? file->Read(Memory.GetMemFromAddr(buf_addr), nbytes) : 0;

	if(nread.IsGood())
		nread = res;

	return CELL_OK;
}

int cellFsWrite(u32 fd, u32 buf_addr, u64 nbytes, mem64_t nwrite)
{
	sys_fs.Log("cellFsWrite(fd=%d, buf_addr=0x%x, nbytes=0x%llx, nwrite_addr=0x%x)",
		fd, buf_addr, nbytes, nwrite.GetAddr());
	vfsStream* file;
	if(!sys_fs.CheckId(fd, file)) return CELL_ESRCH;

	if(Memory.IsGoodAddr(buf_addr) && !Memory.IsGoodAddr(buf_addr, nbytes))
	{
		MemoryBlock& block = Memory.GetMemByAddr(buf_addr);
		nbytes = block.GetSize() - (buf_addr - block.GetStartAddr());
	}

	const u64 res = nbytes ? file->Write(Memory.GetMemFromAddr(buf_addr), nbytes) : 0;

	if(nwrite.IsGood())
		nwrite = res;

	return CELL_OK;
}

int cellFsClose(u32 fd)
{
	sys_fs.Warning("cellFsClose(fd=%d)", fd);

	if(!Emu.GetIdManager().RemoveID(fd))
		return CELL_ESRCH;

	return CELL_OK;
}

int cellFsOpendir(u32 path_addr, mem32_t fd)
{
	const wxString& path = Memory.ReadString(path_addr);
	sys_fs.Warning("cellFsOpendir(path=\"%s\", fd_addr=0x%x)", path.wx_str(), fd.GetAddr());
	
	if(!Memory.IsGoodAddr(path_addr) || !fd.IsGood())
		return CELL_EFAULT;

	wxString localPath;
	Emu.GetVFS().GetDevice(path, localPath);
	vfsLocalDir* dir = new vfsLocalDir(localPath);
	if(!dir->Open(localPath))
		return CELL_ENOENT;

	fd = sys_fs.GetNewId(dir);
	return CELL_OK;
}

int cellFsReaddir(u32 fd, mem_ptr_t<CellFsDirent> dir, mem64_t nread)
{
	sys_fs.Log("cellFsReaddir(fd=%d, dir_addr=0x%x, nread_addr=0x%x)", fd, dir.GetAddr(), nread.GetAddr());

	vfsLocalDir* directory;
	if(!sys_fs.CheckId(fd, directory))
		return CELL_ESRCH;
	if(!dir.IsGood() || !nread.IsGood())
		return CELL_EFAULT;

	const DirEntryInfo* info = directory->Read();
	if(info)
	{
		nread = 1;
		Memory.WriteString(dir.GetAddr()+2, info->name.wx_str());
		dir->d_namlen = info->name.Length();
		dir->d_type = (info->flags & 0x1) ? CELL_FS_TYPE_REGULAR : CELL_FS_TYPE_DIRECTORY;
	}
	else
	{
		nread = 0;
	}

	return CELL_OK;
}

int cellFsClosedir(u32 fd)
{
	sys_fs.Log("cellFsClosedir(fd=%d)", fd);

	if(!Emu.GetIdManager().RemoveID(fd))
		return CELL_ESRCH;

	return CELL_OK;
}

int cellFsStat(const u32 path_addr, mem_ptr_t<CellFsStat> sb)
{
	const wxString& path = Memory.ReadString(path_addr);
	sys_fs.Log("cellFsStat(path=\"%s\", sb_addr: 0x%x)", path.wx_str(), sb.GetAddr());

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
			sys_fs.Log("cellFsStat: \"%s\" is a mount point.", path.wx_str());
			sb->st_mode |= CELL_FS_S_IFDIR;
			return CELL_OK;
		}
	}

	// TODO: Temporary solution until vfsDir is implemented
	wxString real_path;
	Emu.GetVFS().GetDevice(path, real_path);
	struct stat s;
	if(stat(real_path.c_str(), &s) == 0)
	{
		if(s.st_mode & S_IFDIR)
		{
			sb->st_mode |= CELL_FS_S_IFDIR;
		}
		else if(s.st_mode & S_IFREG)
		{
			vfsFile f(path);
			sb->st_mode |= CELL_FS_S_IFREG;
			sb->st_size = f.GetSize();
		}
	}
	else
	{
		sys_fs.Warning("cellFsStat: \"%s\" not found.", path.wx_str());
		return CELL_ENOENT;
	}

	return CELL_OK;
}

int cellFsFstat(u32 fd, mem_ptr_t<CellFsStat> sb)
{
	sys_fs.Log("cellFsFstat(fd=%d, sb_addr: 0x%x)", fd, sb.GetAddr());

	vfsStream* file;
	if(!sys_fs.CheckId(fd, file)) return CELL_ESRCH;

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
	sb->st_size = file->GetSize();
	sb->st_blksize = 4096;

	return CELL_OK;
}

int cellFsMkdir(u32 path_addr, u32 mode)
{
	const wxString& ps3_path = Memory.ReadString(path_addr);
	sys_fs.Log("cellFsMkdir(path=\"%s\", mode=0x%x)", ps3_path.wx_str(), mode);

	wxString localPath;
	Emu.GetVFS().GetDevice(ps3_path, localPath);
	if(wxDirExists(localPath)) return CELL_EEXIST;
	if(!wxMkdir(localPath)) return CELL_EBUSY;
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

	sys_fs.Log("cellFsRename(from=\"%s\", to=\"%s\")", ps3_from.wx_str(), ps3_to.wx_str());
	if(!wxFileExists(from)) return CELL_ENOENT;
	if(wxFileExists(to)) return CELL_EEXIST;
	if(!wxRenameFile(from, to)) return CELL_EBUSY; // (TODO: RenameFile(a,b) = CopyFile(a,b) + RemoveFile(a), therefore file "a" will not be removed if it is opened)
	return CELL_OK;
}

int cellFsRmdir(u32 path_addr)
{
	const wxString& ps3_path = Memory.ReadString(path_addr);
	sys_fs.Log("cellFsRmdir(path=\"%s\")", ps3_path.wx_str());

	wxString localPath;
	Emu.GetVFS().GetDevice(ps3_path, localPath);	
	if(!wxDirExists(localPath)) return CELL_ENOENT;
	if(!wxRmdir(localPath)) return CELL_EBUSY; // (TODO: Under certain conditions it is not able to delete the folder)
	return CELL_OK;
}

int cellFsUnlink(u32 path_addr)
{
	const wxString& ps3_path = Memory.ReadString(path_addr);
	sys_fs.Warning("cellFsUnlink(path=\"%s\")", ps3_path.wx_str());

	wxString localPath;
	Emu.GetVFS().GetDevice(ps3_path, localPath);
	wxRemoveFile(localPath);
	return CELL_OK;
}

int cellFsLseek(u32 fd, s64 offset, u32 whence, mem64_t pos)
{
	vfsSeekMode seek_mode;
	sys_fs.Log("cellFsLseek(fd=%d, offset=0x%llx, whence=0x%x, pos_addr=0x%x)", fd, offset, whence, pos.GetAddr());
	switch(whence)
	{
	case CELL_SEEK_SET: seek_mode = vfsSeekSet; break;
	case CELL_SEEK_CUR: seek_mode = vfsSeekCur; break;
	case CELL_SEEK_END: seek_mode = vfsSeekEnd; break;
	default:
		sys_fs.Error(fd, "Unknown seek whence! (0x%x)", whence);
	return CELL_EINVAL;
	}

	vfsStream* file;
	if(!sys_fs.CheckId(fd, file)) return CELL_ESRCH;
	pos = file->Seek(offset, seek_mode);
	return CELL_OK;
}

int cellFsFtruncate(u32 fd, u64 size)
{
	sys_fs.Log("cellFsFtruncate(fd=%d, size=%lld)", fd, size);
	vfsStream* file;
	if(!sys_fs.CheckId(fd, file)) return CELL_ESRCH;
	u64 initialSize = file->GetSize();

	if (initialSize < size)
	{
		u64 last_pos = file->Tell();
		file->Seek(0, vfsSeekEnd);
		static const char nullbyte = 0;
		file->Seek(size-initialSize-1, vfsSeekCur);
		file->Write(&nullbyte, sizeof(char));
		file->Seek(last_pos, vfsSeekSet);
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
	sys_fs.Log("cellFsTruncate(path=\"%s\", size=%lld)", path.wx_str(), size);

	vfsFile f(path, vfsReadWrite);
	if(!f.IsOpened())
	{
		sys_fs.Warning("cellFsTruncate: \"%s\" not found.", path.wx_str());
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
	sys_fs.Log("cellFsFGetBlockSize(fd=%d, sector_size_addr: 0x%x, block_size_addr: 0x%x)", fd, sector_size.GetAddr(), block_size.GetAddr());
	
	sector_size = 4096; // ?
	block_size = 4096; // ?

	return CELL_OK;
}

int cellFsGetBlockSize(u32 path_addr, mem64_t sector_size, mem64_t block_size)
{
	sys_fs.Log("cellFsGetBlockSize(file: %s, sector_size_addr: 0x%x, block_size_addr: 0x%x)", Memory.ReadString(path_addr).wx_str(), sector_size.GetAddr(), block_size.GetAddr());

	sector_size = 4096; // ?
	block_size = 4096; // ?

	return CELL_OK;
}
