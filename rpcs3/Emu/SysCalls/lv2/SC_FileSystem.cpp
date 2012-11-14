#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"

typedef u64 GPRtype;
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

SysCallBase sc_log("cellFs");

wxString ReadString(const u32 addr)
{
	return GetWinPath(Memory.ReadString(addr));
}

int cellFsOpen(const u32 path_addr, const int flags, const u32 fd_addr, const u32 arg_addr, const u64 size)
{
	const wxString& path = Memory.ReadString(path_addr);
	sc_log.Log("cellFsOpen(path: %s, flags: 0x%x, fd_addr: 0x%x, arg_addr: 0x%x, size: 0x%llx)",
		path, flags, fd_addr, arg_addr, size);

	const wxString& ppath = GetWinPath(path);
	//ConLog.Warning("path: %s [%s]", ppath, path);

	s32 _oflags = flags;
	if(flags & LV2_O_CREAT)
	{
		_oflags &= ~LV2_O_CREAT;
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
	}

	wxFile::OpenMode o_mode;

	switch(flags & LV2_O_ACCMODE)
	{
	case LV2_O_RDONLY:
		_oflags &= ~LV2_O_RDONLY;
		o_mode = wxFile::read;
	break;

	case LV2_O_WRONLY:
		_oflags &= ~LV2_O_WRONLY;

		if(flags & LV2_O_APPEND)
		{
			_oflags &= ~LV2_O_APPEND;
			o_mode = wxFile::write_append;
		}
		else if(flags & LV2_O_EXCL)
		{
			_oflags &= ~LV2_O_EXCL;
			o_mode = wxFile::write_excl;
		}
		else //if(flags & LV2_O_TRUNC)
		{
			_oflags &= ~LV2_O_TRUNC;
			o_mode = wxFile::write;
		}
	break;

	case LV2_O_RDWR:
		_oflags &= ~LV2_O_RDWR;
		o_mode = wxFile::read_write;
	break;
	}

	if(_oflags != 0)
	{
		sc_log.Error("'%s' has unknown flags! flags: 0x%08x", ppath, flags);
		return CELL_EINVAL;
	}

	if(!wxFile::Access(ppath, o_mode))
	{
		sc_log.Error("'%s' not found! flags: 0x%08x", ppath, flags);
		return CELL_ENOENT;
	}

	Memory.Write32(fd_addr, 
		Emu.GetIdManager().GetNewID(sc_log.GetName(), new wxFile(ppath, o_mode), flags));

	return CELL_OK;
}

int cellFsRead(const u32 fd, const u32 buf_addr, const u64 nbytes, const u32 nread_addr)
{
	sc_log.Log("cellFsRead(fd: %d, buf_addr: 0x%x, nbytes: 0x%llx, nread_addr: 0x%x)",
		fd, buf_addr, nbytes, nread_addr);
	if(!Emu.GetIdManager().CheckID(fd)) return CELL_ESRCH;
	const ID& id = Emu.GetIdManager().GetIDData(fd);
	if(!!id.m_name.Cmp(sc_log.GetName())) return CELL_ESRCH;
	wxFile& file = *(wxFile*)id.m_data;

	Memory.Write64NN(nread_addr, file.Read(Memory.GetMemFromAddr(buf_addr), nbytes));

	return CELL_OK;
}

int cellFsWrite(const u32 fd, const u32 buf_addr, const u64 nbytes, const u32 nwrite_addr)
{
	sc_log.Log("cellFsWrite(fd: %d, buf_addr: 0x%x, nbytes: 0x%llx, nwrite_addr: 0x%x)",
		fd, buf_addr, nbytes, nwrite_addr);
	if(!Emu.GetIdManager().CheckID(fd)) return CELL_ESRCH;
	const ID& id = Emu.GetIdManager().GetIDData(fd);
	if(!!id.m_name.Cmp(sc_log.GetName())) return CELL_ESRCH;
	wxFile& file = *(wxFile*)id.m_data;

	Memory.Write64NN(nwrite_addr, file.Write(Memory.GetMemFromAddr(buf_addr), nbytes));

	return CELL_OK;
}

int cellFsClose(const u32 fd)
{
	sc_log.Log("cellFsClose(fd: %d)", fd);
	if(!Emu.GetIdManager().CheckID(fd)) return CELL_ESRCH;
	const ID& id = Emu.GetIdManager().GetIDData(fd);
	if(!!id.m_name.Cmp(sc_log.GetName())) return CELL_ESRCH;
	wxFile& file = *(wxFile*)id.m_data;
	file.Close();
	Emu.GetIdManager().RemoveID(fd);
	return CELL_OK;
}

int cellFsOpendir(const u32 path_addr, const u32 fd_addr)
{
	const wxString& path = Memory.ReadString(path_addr);
	sc_log.Error("cellFsOpendir(path: %s, fd_addr: 0x%x)", path, fd_addr);
	return CELL_OK;
}

int cellFsReaddir(const u32 fd, const u32 dir_addr, const u32 nread_addr)
{
	sc_log.Error("cellFsReaddir(fd: %d, dir_addr: 0x%x, nread_addr: 0x%x)", fd, dir_addr, nread_addr);
	return CELL_OK;
}

int cellFsClosedir(const u32 fd)
{
	sc_log.Error("cellFsClosedir(fd: %d)", fd);
	return CELL_OK;
}

int cellFsStat(const u32 path_addr, const u32 sb_addr)
{
	const wxString& path = ReadString(path_addr);
	sc_log.Log("cellFsFstat(path: %s, sb_addr: 0x%x)", path, sb_addr);

	if(!wxFileExists(path)) return CELL_ENOENT;

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
	stat.st_size = wxFile(path).Length();
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

int cellFsFstat(u32 fd, u32 sb_addr)
{
	sc_log.Log("cellFsFstat(fd: %d, sb_addr: 0x%x)", fd, sb_addr);
	if(!Emu.GetIdManager().CheckID(fd)) return CELL_ESRCH;
	const ID& id = Emu.GetIdManager().GetIDData(fd);
	if(!!id.m_name.Cmp(sc_log.GetName())) return CELL_ESRCH;
	wxFile& file = *(wxFile*)id.m_data;

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
	stat.st_size = file.Length();
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

int cellFsMkdir(const u32 path_addr, const u32 mode)
{
	const wxString& path = ReadString(path_addr);
	sc_log.Log("cellFsMkdir(path: %s, mode: 0x%x)", path, mode);
	if(wxDirExists(path)) return CELL_EEXIST;
	if(!wxMkdir(path)) return CELL_EBUSY;
	return CELL_OK;
}

int cellFsRename(const u32 from_addr, const u32 to_addr)
{
	const wxString& from = ReadString(from_addr);
	const wxString& to = ReadString(to_addr);
	sc_log.Log("cellFsRename(from: %s, to: %s)", from, to);
	if(!wxFileExists(from)) return CELL_ENOENT;
	if(wxFileExists(to)) return CELL_EEXIST;
	if(!wxRenameFile(from, to)) return CELL_EBUSY;
	return CELL_OK;
}

int cellFsRmdir(const u32 path_addr)
{
	const wxString& path = ReadString(path_addr);
	sc_log.Log("cellFsRmdir(path: %s)", path);
	if(!wxDirExists(path)) return CELL_ENOENT;
	if(!wxRmdir(path)) return CELL_EBUSY;
	return CELL_OK;
}

int cellFsUnlink(const u32 path_addr)
{
	const wxString& path = ReadString(path_addr);
	sc_log.Error("cellFsUnlink(path: %s)", path);
	return CELL_OK;
}

int cellFsLseek(const u32 fd, const s64 offset, const u32 whence, const u32 pos_addr)
{
	wxSeekMode seek_mode;
	sc_log.Log("cellFsLseek(fd: %d, offset: 0x%llx, whence: %d, pos_addr: 0x%x)", fd, offset, whence, pos_addr);
	switch(whence)
	{
	case LV2_SEEK_SET: seek_mode = wxFromStart; break;
	case LV2_SEEK_CUR: seek_mode = wxFromCurrent; break;
	case LV2_SEEK_END: seek_mode = wxFromEnd; break;
	default:
		sc_log.Error(fd, "Unknown seek whence! (%d)", whence);
	return CELL_EINVAL;
	}
	if(!Emu.GetIdManager().CheckID(fd)) return CELL_ESRCH;
	const ID& id = Emu.GetIdManager().GetIDData(fd);
	if(!!id.m_name.Cmp(sc_log.GetName())) return CELL_ESRCH;
	wxFile& file = *(wxFile*)id.m_data;
	Memory.Write64(pos_addr, file.Seek(offset, seek_mode));
	return CELL_OK;
}