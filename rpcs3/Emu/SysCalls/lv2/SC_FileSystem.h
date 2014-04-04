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


#pragma pack(4)

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

#pragma pack()

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
