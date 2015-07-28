#pragma once

#include "Utilities/Thread.h"

namespace vm { using namespace ps3; }

#pragma pack(push, 4)

// Error Codes
enum : s32
{
	CELL_FS_EDOM = CELL_EDOM,
	CELL_FS_EFAULT = CELL_EFAULT,
	CELL_FS_EFBIG = CELL_EFBIG,
	CELL_FS_EFPOS = CELL_EFPOS,
	CELL_FS_EMLINK = CELL_EMLINK,
	CELL_FS_ENFILE = CELL_ENFILE,
	CELL_FS_ENOENT = CELL_ENOENT,
	CELL_FS_ENOSPC = CELL_ENOSPC,
	CELL_FS_ENOTTY = CELL_ENOTTY,
	CELL_FS_EPIPE = CELL_EPIPE,
	CELL_FS_ERANGE = CELL_ERANGE,
	CELL_FS_EROFS = CELL_EROFS,
	CELL_FS_ESPIPE = CELL_ESPIPE,
	CELL_FS_E2BIG = CELL_E2BIG,
	CELL_FS_EACCES = CELL_EACCES,
	CELL_FS_EAGAIN = CELL_EAGAIN,
	CELL_FS_EBADF = CELL_EBADF,
	CELL_FS_EBUSY = CELL_EBUSY,
	//CELL_FS_ECHILD = CELL_ECHILD,
	CELL_FS_EEXIST = CELL_EEXIST,
	CELL_FS_EINTR = CELL_EINTR,
	CELL_FS_EINVAL = CELL_EINVAL,
	CELL_FS_EIO = CELL_EIO,
	CELL_FS_EISDIR = CELL_EISDIR,
	CELL_FS_EMFILE = CELL_EMFILE,
	CELL_FS_ENODEV = CELL_ENODEV,
	CELL_FS_ENOEXEC = CELL_ENOEXEC,
	CELL_FS_ENOMEM = CELL_ENOMEM,
	CELL_FS_ENOTDIR = CELL_ENOTDIR,
	CELL_FS_ENXIO = CELL_ENXIO,
	CELL_FS_EPERM = CELL_EPERM,
	CELL_FS_ESRCH = CELL_ESRCH,
	CELL_FS_EXDEV = CELL_EXDEV,
	CELL_FS_EBADMSG = CELL_EBADMSG,
	CELL_FS_ECANCELED = CELL_ECANCELED,
	CELL_FS_EDEADLK = CELL_EDEADLK,
	CELL_FS_EILSEQ = CELL_EILSEQ,
	CELL_FS_EINPROGRESS = CELL_EINPROGRESS,
	CELL_FS_EMSGSIZE = CELL_EMSGSIZE,
	CELL_FS_ENAMETOOLONG = CELL_ENAMETOOLONG,
	CELL_FS_ENOLCK = CELL_ENOLCK,
	CELL_FS_ENOSYS = CELL_ENOSYS,
	CELL_FS_ENOTEMPTY = CELL_ENOTEMPTY,
	CELL_FS_ENOTSUP = CELL_ENOTSUP,
	CELL_FS_ETIMEDOUT = CELL_ETIMEDOUT,
	CELL_FS_EFSSPECIFIC = CELL_EFSSPECIFIC,
	CELL_FS_EOVERFLOW = CELL_EOVERFLOW,
	CELL_FS_ENOTMOUNTED = CELL_ENOTMOUNTED,
	CELL_FS_ENOTMSELF = CELL_ENOTMSELF,
	CELL_FS_ENOTSDATA = CELL_ENOTSDATA,
	CELL_FS_EAUTHFATAL = CELL_EAUTHFATAL,
};

// Open Flags
enum : s32
{
	CELL_FS_O_RDONLY  = 000000,
	CELL_FS_O_WRONLY  = 000001,
	CELL_FS_O_RDWR    = 000002,
	CELL_FS_O_ACCMODE = 000003,
	CELL_FS_O_CREAT   = 000100,
	CELL_FS_O_EXCL    = 000200,
	CELL_FS_O_TRUNC   = 001000,
	CELL_FS_O_APPEND  = 002000,
	CELL_FS_O_MSELF   = 010000,
};

// Seek Mode
enum : s32
{
	CELL_FS_SEEK_SET,
	CELL_FS_SEEK_CUR,
	CELL_FS_SEEK_END,
};

enum : s32
{
	CELL_FS_MAX_FS_PATH_LENGTH = 1024,
	CELL_FS_MAX_FS_FILE_NAME_LENGTH = 255,
	CELL_FS_MAX_MP_LENGTH = 31,
};

enum CellFsMode : s32
{
	CELL_FS_S_IFMT  = 0170000,
	CELL_FS_S_IFDIR = 0040000, // directory
	CELL_FS_S_IFREG = 0100000, // regular
	CELL_FS_S_IFLNK = 0120000, // symbolic link
	CELL_FS_S_IFWHT = 0160000, // unknown

	CELL_FS_S_IRUSR = 0000400, // R for owner
	CELL_FS_S_IWUSR = 0000200, // W for owner
	CELL_FS_S_IXUSR = 0000100, // X for owner

	CELL_FS_S_IRGRP = 0000040, // R for group
	CELL_FS_S_IWGRP = 0000020, // W for group
	CELL_FS_S_IXGRP = 0000010, // X for group

	CELL_FS_S_IROTH = 0000004, // R for other
	CELL_FS_S_IWOTH = 0000002, // W for other
	CELL_FS_S_IXOTH = 0000001, // X for other
};

// CellFsDirent.d_type
enum : u8
{
	CELL_FS_TYPE_UNKNOWN   = 0,
	CELL_FS_TYPE_DIRECTORY = 1,
	CELL_FS_TYPE_REGULAR   = 2,
	CELL_FS_TYPE_SYMLINK   = 3,
};

struct CellFsDirent
{
	u8 d_type;
	u8 d_namlen;
	char d_name[256];
};

struct CellFsStat
{
	be_t<s32> mode;
	be_t<s32> uid;
	be_t<s32> gid;
	be_t<s64> atime;
	be_t<s64> mtime;
	be_t<s64> ctime;
	be_t<u64> size;
	be_t<u64> blksize;
};

struct CellFsUtimbuf
{
	be_t<s64> actime;
	be_t<s64> modtime;
};

#pragma pack(pop)

struct vfsStream;

// Stream Support Status (st_status)
enum : u32
{
	SSS_NOT_INITIALIZED = 0,
	SSS_INITIALIZED,
	SSS_STARTED,
	SSS_STOPPED,
};

using fs_st_cb_t = vm::ptr<void(u32 xfd, u64 xsize)>;

struct fs_st_cb_rec_t
{
	u64 size;
	fs_st_cb_t func;
	u32 pad;
};

struct lv2_file_t
{
	const std::shared_ptr<vfsStream> file;
	const s32 mode;
	const s32 flags;

	std::mutex mutex;
	std::condition_variable cv;

	atomic_t<u32> st_status;
	
	u64 st_ringbuf_size;
	u64 st_block_size;
	u64 st_trans_rate;
	bool st_copyless;

	thread_t st_thread;

	u32 st_buffer;
	u64 st_read_size;
	std::atomic<u64> st_total_read;
	std::atomic<u64> st_copied;

	atomic_t<fs_st_cb_rec_t> st_callback;

	lv2_file_t(std::shared_ptr<vfsStream> file, s32 mode, s32 flags)
		: file(std::move(file))
		, mode(mode)
		, flags(flags)
		, st_status({ SSS_NOT_INITIALIZED })
		, st_callback({})
	{
	}

	~lv2_file_t();
};

REG_ID_TYPE(lv2_file_t, 0x73); // SYS_FS_FD_OBJECT

class vfsDirBase;

struct lv2_dir_t
{
	const std::shared_ptr<vfsDirBase> dir;

	lv2_dir_t(std::shared_ptr<vfsDirBase> dir)
		: dir(std::move(dir))
	{
	}

	~lv2_dir_t();
};

REG_ID_TYPE(lv2_dir_t, 0x73); // SYS_FS_FD_OBJECT

// SysCalls
s32 sys_fs_test(u32 arg1, u32 arg2, vm::ptr<u32> arg3, u32 arg4, vm::ptr<char> arg5, u32 arg6);
s32 sys_fs_open(vm::cptr<char> path, s32 flags, vm::ptr<u32> fd, s32 mode, vm::cptr<void> arg, u64 size);
s32 sys_fs_read(u32 fd, vm::ptr<void> buf, u64 nbytes, vm::ptr<u64> nread);
s32 sys_fs_write(u32 fd, vm::cptr<void> buf, u64 nbytes, vm::ptr<u64> nwrite);
s32 sys_fs_close(u32 fd);
s32 sys_fs_opendir(vm::cptr<char> path, vm::ptr<u32> fd);
s32 sys_fs_readdir(u32 fd, vm::ptr<CellFsDirent> dir, vm::ptr<u64> nread);
s32 sys_fs_closedir(u32 fd);
s32 sys_fs_stat(vm::cptr<char> path, vm::ptr<CellFsStat> sb);
s32 sys_fs_fstat(u32 fd, vm::ptr<CellFsStat> sb);
s32 sys_fs_mkdir(vm::cptr<char> path, s32 mode);
s32 sys_fs_rename(vm::cptr<char> from, vm::cptr<char> to);
s32 sys_fs_rmdir(vm::cptr<char> path);
s32 sys_fs_unlink(vm::cptr<char> path);
s32 sys_fs_fcntl(u32 fd, s32 flags, u32 addr, u32 arg4, u32 arg5, u32 arg6);
s32 sys_fs_lseek(u32 fd, s64 offset, s32 whence, vm::ptr<u64> pos);
s32 sys_fs_fget_block_size(u32 fd, vm::ptr<u64> sector_size, vm::ptr<u64> block_size, vm::ptr<u64> arg4, vm::ptr<u64> arg5);
s32 sys_fs_get_block_size(vm::cptr<char> path, vm::ptr<u64> sector_size, vm::ptr<u64> block_size, vm::ptr<u64> arg4);
s32 sys_fs_truncate(vm::cptr<char> path, u64 size);
s32 sys_fs_ftruncate(u32 fd, u64 size);
s32 sys_fs_chmod(vm::cptr<char> path, s32 mode);
