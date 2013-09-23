#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"

enum Lv2FsOflag
{
	LV2_O_RDONLY	= 000000,
	LV2_O_WRONLY	= 000001,
	LV2_O_RDWR		= 000002,
	LV2_O_ACCMODE	= 000003,
	LV2_O_CREAT		= 000100,
	LV2_O_EXCL		= 000200,
	LV2_O_TRUNC		= 001000,
	LV2_O_APPEND	= 002000,
	LV2_O_MSELF		= 010000,
};

#define CELL_FS_TYPE_UNKNOWN   0

enum Lv2FsSeek
{
	LV2_SEEK_SET,
	LV2_SEEK_CUR,
	LV2_SEEK_END,
};

enum Lv2FsLength
{
	LV2_MAX_FS_PATH_LENGTH = 1024,
	LV2_MAX_FS_FILE_NAME_LENGTH = 255,
	LV2_MAX_FS_MP_LENGTH = 31,
};

enum
{
	CELL_FS_S_IFDIR = 0040000,	//directory
	CELL_FS_S_IFREG = 0100000,	//regular
	CELL_FS_S_IFLNK = 0120000,	//symbolic link
	CELL_FS_S_IFWHT = 0160000,	//unknown

	CELL_FS_S_IRUSR = 0000400,	//R for owner
	CELL_FS_S_IWUSR = 0000200,	//W for owner
	CELL_FS_S_IXUSR = 0000100,	//X for owner

	CELL_FS_S_IRGRP = 0000040,	//R for group
	CELL_FS_S_IWGRP = 0000020,	//W for group
	CELL_FS_S_IXGRP = 0000010,	//X for group

	CELL_FS_S_IROTH = 0000004,	//R for other
	CELL_FS_S_IWOTH = 0000002,	//W for other
	CELL_FS_S_IXOTH = 0000001,	//X for other
};

struct Lv2FsStat
{
	u32 st_mode;
	s32 st_uid;
	s32 st_gid;
	time_t st_atime;
	time_t st_mtime;
	time_t st_ctime;
	u64 st_size;
	u64 st_blksize;
};

struct Lv2FsUtimbuf
{
	time_t actime;
	time_t modtime;
};

struct Lv2FsDirent
{
	u8 d_type;
	u8 d_namlen;
	char d_name[LV2_MAX_FS_FILE_NAME_LENGTH + 1];
};

enum FsDirentType
{
	CELL_FS_TYPE_DIRECTORY	= 1,
	CELL_FS_TYPE_REGULAR	= 2,
	CELL_FS_TYPE_SYMLINK	= 3,
};

extern Module sys_fs;

int cellFsOpen(u32 path_addr, int flags, u32 fd_addr, u32 arg_addr, u64 size)
{
	const wxString& path = Memory.ReadString(path_addr);
	sys_fs.Log("cellFsOpen(path: %s, flags: 0x%x, fd_addr: 0x%x, arg_addr: 0x%x, size: 0x%llx)",
		path, flags, fd_addr, arg_addr, size);

	const wxString& ppath = path;
	//ConLog.Warning("path: %s [%s]", ppath, path);

	s32 _oflags = flags;
	if(flags & LV2_O_CREAT)
	{
		_oflags &= ~LV2_O_CREAT;
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

	switch(flags & LV2_O_ACCMODE)
	{
	case LV2_O_RDONLY:
		_oflags &= ~LV2_O_RDONLY;
		o_mode = vfsRead;
	break;

	case LV2_O_WRONLY:
		_oflags &= ~LV2_O_WRONLY;

		if(flags & LV2_O_APPEND)
		{
			_oflags &= ~LV2_O_APPEND;
			o_mode = vfsWriteAppend;
		}
		else if(flags & LV2_O_EXCL)
		{
			_oflags &= ~LV2_O_EXCL;
			o_mode = vfsWriteExcl;
		}
		else //if(flags & LV2_O_TRUNC)
		{
			_oflags &= ~LV2_O_TRUNC;
			o_mode = vfsWrite;
		}
	break;

	case LV2_O_RDWR:
		_oflags &= ~LV2_O_RDWR;
		o_mode = vfsReadWrite;
	break;
	}

	if(_oflags != 0)
	{
		sys_fs.Error("'%s' has unknown flags! flags: 0x%08x", ppath, flags);
		return CELL_EINVAL;
	}

	vfsStream* stream = Emu.GetVFS().Open(ppath, o_mode);

	if(!stream || !stream->IsOpened())
	{
		sys_fs.Error("'%s' not found! flags: 0x%08x", ppath, flags);
		delete stream;

		return CELL_ENOENT;
	}

	Memory.Write32(fd_addr, sys_fs.GetNewId(stream, flags));

	return CELL_OK;
}

int cellFsRead(u32 fd, u32 buf_addr, u64 nbytes, u32 nread_addr)
{
	sys_fs.Log("cellFsRead(fd: %d, buf_addr: 0x%x, nbytes: 0x%llx, nread_addr: 0x%x)",
		fd, buf_addr, nbytes, nread_addr);
	ID id;
	if(!sys_fs.CheckId(fd, id)) return CELL_ESRCH;
	vfsStream& file = *(vfsStream*)id.m_data;

	Memory.Write64NN(nread_addr, file.Read(Memory.GetMemFromAddr(buf_addr), nbytes));

	return CELL_OK;
}

int cellFsWrite(u32 fd, u32 buf_addr, u64 nbytes, u32 nwrite_addr)
{
	sys_fs.Log("cellFsWrite(fd: %d, buf_addr: 0x%x, nbytes: 0x%llx, nwrite_addr: 0x%x)",
		fd, buf_addr, nbytes, nwrite_addr);
	ID id;
	if(!sys_fs.CheckId(fd, id)) return CELL_ESRCH;
	vfsStream& file = *(vfsStream*)id.m_data;
	if(Memory.IsGoodAddr(buf_addr) && !Memory.IsGoodAddr(buf_addr, nbytes))
	{
		MemoryBlock& block = Memory.GetMemByAddr(buf_addr);
		nbytes = block.GetSize() - (buf_addr - block.GetStartAddr());
	}

	int count = nbytes ? file.Write(Memory.GetMemFromAddr(buf_addr), nbytes) : 0;
	Memory.Write64NN(nwrite_addr, count);
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

int cellFsOpendir(u32 path_addr, u32 fd_addr)
{
	const wxString& path = Memory.ReadString(path_addr);
	sys_fs.Error("cellFsOpendir(path_addr: 0x%x(%s), fd_addr: 0x%x)", path_addr, path, fd_addr);
	if(!Memory.IsGoodAddr(path_addr, sizeof(u32)) || !Memory.IsGoodAddr(fd_addr, sizeof(u32))) return CELL_EFAULT;

	return CELL_OK;
}

int cellFsReaddir(u32 fd, u32 dir_addr, u32 nread_addr)
{
	sys_fs.Error("cellFsReaddir(fd: %d, dir_addr: 0x%x, nread_addr: 0x%x)", fd, dir_addr, nread_addr);
	return CELL_OK;
}

int cellFsClosedir(u32 fd)
{
	sys_fs.Error("cellFsClosedir(fd: %d)", fd);
	return CELL_OK;
}

int cellFsStat(const u32 path_addr, const u32 sb_addr)
{
	const wxString& path = Memory.ReadString(path_addr);
	sys_fs.Log("cellFsFstat(path: %s, sb_addr: 0x%x)", path, sb_addr);

	// Check if path is a mount point. (TODO: Add information in sb_addr)
	for(u32 i=0; i<Emu.GetVFS().m_devices.GetCount(); ++i)
	{
		if  (path == Emu.GetVFS().m_devices[i].GetPs3Path().RemoveLast(1))
		{
			sys_fs.Log("cellFsFstat: '%s' is a mount point.", path);
			return CELL_OK;
		}
	}

	vfsStream* f = Emu.GetVFS().Open(path, vfsRead);
	if(!f || !f->IsOpened())
	{
		sys_fs.Warning("cellFsFstat: '%s' not found.", path);
		Emu.GetVFS().Close(f);
		return CELL_ENOENT;
	}

	Lv2FsStat stat;
	stat.st_mode = 
		CELL_FS_S_IRUSR | CELL_FS_S_IWUSR | CELL_FS_S_IXUSR |
		CELL_FS_S_IRGRP | CELL_FS_S_IWGRP | CELL_FS_S_IXGRP |
		CELL_FS_S_IROTH | CELL_FS_S_IWOTH | CELL_FS_S_IXOTH;

	stat.st_mode |= CELL_FS_S_IFREG; //TODO: dir CELL_FS_S_IFDIR
	stat.st_uid = 0;
	stat.st_gid = 0;
	stat.st_atime = 0; //TODO
	stat.st_mtime = 0; //TODO
	stat.st_ctime = 0; //TODO
	stat.st_size = f->GetSize();
	stat.st_blksize = 4096;

	mem_class_t stat_c(sb_addr);
	stat_c += stat.st_mode;
	stat_c += stat.st_uid;
	stat_c += stat.st_gid;
	stat_c += stat.st_atime;
	stat_c += stat.st_mtime;
	stat_c += stat.st_ctime;
	stat_c += stat.st_size;
	stat_c += stat.st_blksize;

	Emu.GetVFS().Close(f);

	return CELL_OK;
}

int cellFsFstat(u32 fd, u32 sb_addr)
{
	sys_fs.Log("cellFsFstat(fd: %d, sb_addr: 0x%x)", fd, sb_addr);
	ID id;
	if(!sys_fs.CheckId(fd, id)) return CELL_ESRCH;
	vfsStream& file = *(vfsStream*)id.m_data;

	Lv2FsStat stat;
	stat.st_mode = 
		CELL_FS_S_IRUSR | CELL_FS_S_IWUSR | CELL_FS_S_IXUSR |
		CELL_FS_S_IRGRP | CELL_FS_S_IWGRP | CELL_FS_S_IXGRP |
		CELL_FS_S_IROTH | CELL_FS_S_IWOTH | CELL_FS_S_IXOTH;

	stat.st_mode |= CELL_FS_S_IFREG; //TODO: dir CELL_FS_S_IFDIR
	stat.st_uid = 0;
	stat.st_gid = 0;
	stat.st_atime = 0; //TODO
	stat.st_mtime = 0; //TODO
	stat.st_ctime = 0; //TODO
	stat.st_size = file.GetSize();
	stat.st_blksize = 4096;

	mem_class_t stat_c(sb_addr);
	stat_c += stat.st_mode;
	stat_c += stat.st_uid;
	stat_c += stat.st_gid;
	stat_c += stat.st_atime;
	stat_c += stat.st_mtime;
	stat_c += stat.st_ctime;
	stat_c += stat.st_size;
	stat_c += stat.st_blksize;

	return CELL_OK;
}

int cellFsMkdir(u32 path_addr, u32 mode)
{
	const wxString& ps3_path = Memory.ReadString(path_addr);
	wxString path;
	Emu.GetVFS().GetDevice(ps3_path, path);
	sys_fs.Log("cellFsMkdir(path: %s, mode: 0x%x)", path, mode);
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

	sys_fs.Log("cellFsRename(from: %s, to: %s)", from, to);
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
	sys_fs.Log("cellFsRmdir(path: %s)", path);
	if(!wxDirExists(path)) return CELL_ENOENT;
	if(!wxRmdir(path)) return CELL_EBUSY; // (TODO: Under certain conditions it is not able to delete the folder)
	return CELL_OK;
}

int cellFsUnlink(u32 path_addr)
{
	const wxString& ps3_path = Memory.ReadString(path_addr);
	wxString path;
	Emu.GetVFS().GetDevice(ps3_path, path);
	sys_fs.Error("cellFsUnlink(path: %s)", path);
	return CELL_OK;
}

int cellFsLseek(u32 fd, s64 offset, u32 whence, u32 pos_addr)
{
	vfsSeekMode seek_mode;
	sys_fs.Log("cellFsLseek(fd: %d, offset: 0x%llx, whence: %d, pos_addr: 0x%x)", fd, offset, whence, pos_addr);
	switch(whence)
	{
	case LV2_SEEK_SET: seek_mode = vfsSeekSet; break;
	case LV2_SEEK_CUR: seek_mode = vfsSeekCur; break;
	case LV2_SEEK_END: seek_mode = vfsSeekEnd; break;
	default:
		sys_fs.Error(fd, "Unknown seek whence! (%d)", whence);
	return CELL_EINVAL;
	}
	ID id;
	if(!sys_fs.CheckId(fd, id)) return CELL_ESRCH;
	vfsStream& file = *(vfsStream*)id.m_data;
	Memory.Write64(pos_addr, file.Seek(offset, seek_mode));
	return CELL_OK;
}

int cellFsFtruncate(u32 fd, u64 size)
{
	sys_fs.Log("cellFsFtruncate(fd: %d, size: %lld)", fd, size);
	ID id;
	if(!sys_fs.CheckId(fd, id)) return CELL_ESRCH;
	vfsStream& file = *(vfsStream*)id.m_data;
	u64 initialSize = file.GetSize();

	if (initialSize < size)  // Is there any better way to fill the remaining bytes with 0, without allocating huge buffers in memory, or writing such a spaghetti code?
	{
		u64 last_pos = file.Tell();
		file.Seek(0, vfsSeekEnd);
		char* nullblock = (char*)calloc(4096, sizeof(char));
		for(u64 i = (size-initialSize)/4096; i > 0; i--){
			file.Write(nullblock, 4096);
		}
		free(nullblock);
		char nullbyte = 0;
		for(u64 i = (size-initialSize)%4096; i > 0; i--){
			file.Write(&nullbyte, 1);
		}
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
	sys_fs.Log("cellFsTruncate(path_addr: %s, size: %lld)", path, size);

	vfsStream* f = Emu.GetVFS().Open(path, vfsRead);
	if(!f || !f->IsOpened())
	{
		sys_fs.Warning("cellFsTruncate: '%s' not found.", path);
		Emu.GetVFS().Close(f);
		return CELL_ENOENT;
	}
	u64 initialSize = f->GetSize();

	if (initialSize < size)  // Is there any better way to fill the remaining bytes with 0, without allocating huge buffers in memory, or writing such a spaghetti code?
	{
		u64 last_pos = f->Tell();
		f->Seek(0, vfsSeekEnd);
		char* nullblock = (char*)calloc(4096, sizeof(char));
		for(u64 i = (size-initialSize)/4096; i > 0; i--){
			f->Write(nullblock, 4096);
		}
		free(nullblock);
		char nullbyte = 0;
		for(u64 i = (size-initialSize)%4096; i > 0; i--){
			f->Write(&nullbyte, 1);
		}
		f->Seek(last_pos, vfsSeekSet);
	}

	if (initialSize > size)
	{
		// (TODO)
	}

	Emu.GetVFS().Close(f);
	return CELL_OK;
}

int cellFsFGetBlockSize(u32 fd, u32 sector_size_addr, u32 block_size_addr)
{
	sys_fs.Log("cellFsFGetBlockSize(fd: %d, sector_size_addr: 0x%x, block_size_addr: 0x%x)", fd, sector_size_addr, block_size_addr);
	
	Memory.Write64(sector_size_addr, 4096);	// ?
	Memory.Write64(block_size_addr, 4096);	// ?

	return CELL_OK;
}