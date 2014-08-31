#pragma once

enum CellFsOflag
{
	CELL_O_RDONLY  = 000000,
	CELL_O_WRONLY  = 000001,
	CELL_O_RDWR    = 000002,
	CELL_O_ACCMODE = 000003,
	CELL_O_CREAT   = 000100,
	CELL_O_EXCL    = 000200,
	CELL_O_TRUNC   = 001000,
	CELL_O_APPEND  = 002000,
	CELL_O_MSELF   = 010000,
};

static const u32 CELL_FS_TYPE_UNKNOWN = 0;

enum CellFsSeek
{
	CELL_SEEK_SET,
	CELL_SEEK_CUR,
	CELL_SEEK_END,
};

enum CellFsLength
{
	CELL_MAX_FS_PATH_LENGTH = 1024,
	CELL_MAX_FS_FILE_NAME_LENGTH = 255,
	CELL_MAX_FS_MP_LENGTH = 31,
};

enum
{
	CELL_FS_S_IFDIR = 0040000, //directory
	CELL_FS_S_IFREG = 0100000, //regular
	CELL_FS_S_IFLNK = 0120000, //symbolic link
	CELL_FS_S_IFWHT = 0160000, //unknown

	CELL_FS_S_IRUSR = 0000400, //R for owner
	CELL_FS_S_IWUSR = 0000200, //W for owner
	CELL_FS_S_IXUSR = 0000100, //X for owner

	CELL_FS_S_IRGRP = 0000040, //R for group
	CELL_FS_S_IWGRP = 0000020, //W for group
	CELL_FS_S_IXGRP = 0000010, //X for group

	CELL_FS_S_IROTH = 0000004, //R for other
	CELL_FS_S_IWOTH = 0000002, //W for other
	CELL_FS_S_IXOTH = 0000001, //X for other
};

enum FsDirentType
{
	CELL_FS_TYPE_DIRECTORY = 1,
	CELL_FS_TYPE_REGULAR   = 2,
	CELL_FS_TYPE_SYMLINK   = 3,
};

enum CellFsRingBufferCopy
{
	CELL_FS_ST_COPY        = 0,
	CELL_FS_ST_COPYLESS    = 1,
};

enum cellFsStStatus
{
	CELL_FS_ST_INITIALIZED     = 0x0001,
	CELL_FS_ST_NOT_INITIALIZED = 0x0002,
	CELL_FS_ST_STOP            = 0x0100,
	CELL_FS_ST_PROGRESS        = 0x0200,
};


#pragma pack(push, 4)

struct CellFsStat
{
	be_t<u32> st_mode;
	be_t<s32> st_uid;
	be_t<s32> st_gid;
	be_t<u64> st_atime_;
	be_t<u64> st_mtime_;
	be_t<u64> st_ctime_;
	be_t<u64> st_size;
	be_t<u64> st_blksize;
};

struct CellFsUtimbuf
{
	be_t<u64> actime;
	be_t<u64> modtime;
};

struct CellFsDirent
{
	u8 d_type;
	u8 d_namlen;
	char d_name[CELL_MAX_FS_FILE_NAME_LENGTH + 1];
};

#pragma pack(pop)

struct CellFsAio
{
	be_t<u32> fd;
	be_t<u64> offset;
	be_t<u32> buf_addr;
	be_t<u64> size;
	be_t<u64> user_data;
};

struct CellFsDirectoryEntry
{
	CellFsStat attribute;
	CellFsDirent entry_name;
};

struct CellFsRingBuffer 
{
	be_t<u64> ringbuf_size;
	be_t<u64> block_size;
	be_t<u64> transfer_rate;
	be_t<u32> copy;
};

// SysCalls
s32 cellFsOpen(u32 path_addr, s32 flags, mem32_t fd, mem32_t arg, u64 size);
s32 cellFsRead(u32 fd, u32 buf_addr, u64 nbytes, vm::ptr<be_t<u64>> nread);
s32 cellFsWrite(u32 fd, u32 buf_addr, u64 nbytes, vm::ptr<be_t<u64>> nwrite);
s32 cellFsClose(u32 fd);
s32 cellFsOpendir(u32 path_addr, mem32_t fd);
s32 cellFsReaddir(u32 fd, mem_ptr_t<CellFsDirent> dir, vm::ptr<be_t<u64>> nread);
s32 cellFsClosedir(u32 fd);
s32 cellFsStat(u32 path_addr, mem_ptr_t<CellFsStat> sb);
s32 cellFsFstat(u32 fd, mem_ptr_t<CellFsStat> sb);
s32 cellFsMkdir(u32 path_addr, u32 mode);
s32 cellFsRename(u32 from_addr, u32 to_addr);
s32 cellFsChmod(u32 path_addr, u32 mode);
s32 cellFsFsync(u32 fd);
s32 cellFsRmdir(u32 path_addr);
s32 cellFsUnlink(u32 path_addr);
s32 cellFsLseek(u32 fd, s64 offset, u32 whence, vm::ptr<be_t<u64>> pos);
s32 cellFsFtruncate(u32 fd, u64 size);
s32 cellFsTruncate(u32 path_addr, u64 size);
s32 cellFsFGetBlockSize(u32 fd, vm::ptr<be_t<u64>> sector_size, vm::ptr<be_t<u64>> block_size);
s32 cellFsGetBlockSize(u32 path_addr, vm::ptr<be_t<u64>> sector_size, vm::ptr<be_t<u64>> block_size);
s32 cellFsGetFreeSize(u32 path_addr, vm::ptr<be_t<u32>> block_size, vm::ptr<be_t<u64>> block_count);
s32 cellFsGetDirectoryEntries(u32 fd, mem_ptr_t<CellFsDirectoryEntry> entries, u32 entries_size, mem32_t data_count);
s32 cellFsStReadInit(u32 fd, mem_ptr_t<CellFsRingBuffer> ringbuf);
s32 cellFsStReadFinish(u32 fd);
s32 cellFsStReadGetRingBuf(u32 fd, mem_ptr_t<CellFsRingBuffer> ringbuf);
s32 cellFsStReadGetStatus(u32 fd, vm::ptr<be_t<u64>> status);
s32 cellFsStReadGetRegid(u32 fd, vm::ptr<be_t<u64>> regid);
s32 cellFsStReadStart(u32 fd, u64 offset, u64 size);
s32 cellFsStReadStop(u32 fd);
s32 cellFsStRead(u32 fd, u32 buf_addr, u64 size, vm::ptr<be_t<u64>> rsize);
s32 cellFsStReadGetCurrentAddr(u32 fd, mem32_t addr_addr, vm::ptr<be_t<u64>> size);
s32 cellFsStReadPutCurrentAddr(u32 fd, u32 addr_addr, u64 size);
s32 cellFsStReadWait(u32 fd, u64 size);
s32 cellFsStReadWaitCallback(u32 fd, u64 size, mem_func_ptr_t<void (*)(int xfd, u64 xsize)> func);
