#pragma once

#include "Emu/Memory/Memory.h"
#include "Emu/Cell/ErrorCodes.h"
#include "Emu/IdManager.h"

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

enum : s32
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
	be_t<s64, 4> atime;
	be_t<s64, 4> mtime;
	be_t<s64, 4> ctime;
	be_t<u64, 4> size;
	be_t<u64, 4> blksize;
};

CHECK_SIZE_ALIGN(CellFsStat, 52, 4);

struct CellFsUtimbuf
{
	be_t<s64, 4> actime;
	be_t<s64, 4> modtime;
};

CHECK_SIZE_ALIGN(CellFsUtimbuf, 16, 4);

struct lv2_fs_mount_point;

struct lv2_fs_object
{
	// ID Manager setups
	using id_base = lv2_fs_object;

	static constexpr u32 id_min = 3;
	static constexpr u32 id_max = 255;

	const id_value<> id{};

	// Mount Point
	const std::add_pointer_t<lv2_fs_mount_point> mp;

	lv2_fs_object(lv2_fs_mount_point* mp)
		: mp(mp)
	{
	}

	static lv2_fs_mount_point* get_mp(const char* filename);
};

struct lv2_file : lv2_fs_object
{
	const fs::file file;
	const s32 mode;
	const s32 flags;

	lv2_file(const char* filename, fs::file&& file, s32 mode, s32 flags)
		: lv2_fs_object(lv2_fs_object::get_mp(filename))
		, file(std::move(file))
		, mode(mode)
		, flags(flags)
	{
	}

	// File reading with intermediate buffer
	u64 op_read(vm::ps3::ptr<void> buf, u64 size);

	// File writing with intermediate buffer
	u64 op_write(vm::ps3::cptr<void> buf, u64 size);
};

struct lv2_dir : lv2_fs_object
{
	const fs::dir dir;

	lv2_dir(const char* filename, fs::dir&& dir)
		: lv2_fs_object(lv2_fs_object::get_mp(filename))
		, dir(std::move(dir))
	{
	}
};

// sys_fs_fcntl arg base class (left empty for PODness)
struct lv2_file_op
{
};

namespace vtable
{
	struct lv2_file_op
	{
		// Speculation
		vm::bptrb<vm::ptrb<void>(vm::ptrb<lv2_file_op>)> get_data;
		vm::bptrb<u32(vm::ptrb<lv2_file_op>)> get_size;
		vm::bptrb<void(vm::ptrb<lv2_file_op>)> _dtor1;
		vm::bptrb<void(vm::ptrb<lv2_file_op>)> _dtor2;
	};
}

// sys_fs_fcntl: read with offset, write with offset
struct lv2_file_op_rw : lv2_file_op
{
	vm::bptrb<vtable::lv2_file_op> _vtable;

	be_t<u32> op;
	be_t<u32> _x8; // ???
	be_t<u32> _xc; // ???

	be_t<u32> fd; // File descriptor (3..255)
	vm::bptrb<void> buf; // Buffer for data
	be_t<u64> offset; // File offset
	be_t<u64> size; // Access size

	be_t<s32> out_code; // Op result
	be_t<u64> out_size; // Size processed
};

CHECK_SIZE(lv2_file_op_rw, 0x38);

// SysCalls
ppu_error_code sys_fs_test(u32 arg1, u32 arg2, vm::ps3::ptr<u32> arg3, u32 arg4, vm::ps3::ptr<char> arg5, u32 arg6);
ppu_error_code sys_fs_open(vm::ps3::cptr<char> path, s32 flags, vm::ps3::ptr<u32> fd, s32 mode, vm::ps3::cptr<void> arg, u64 size);
ppu_error_code sys_fs_read(u32 fd, vm::ps3::ptr<void> buf, u64 nbytes, vm::ps3::ptr<u64> nread);
ppu_error_code sys_fs_write(u32 fd, vm::ps3::cptr<void> buf, u64 nbytes, vm::ps3::ptr<u64> nwrite);
ppu_error_code sys_fs_close(u32 fd);
ppu_error_code sys_fs_opendir(vm::ps3::cptr<char> path, vm::ps3::ptr<u32> fd);
ppu_error_code sys_fs_readdir(u32 fd, vm::ps3::ptr<CellFsDirent> dir, vm::ps3::ptr<u64> nread);
ppu_error_code sys_fs_closedir(u32 fd);
ppu_error_code sys_fs_stat(vm::ps3::cptr<char> path, vm::ps3::ptr<CellFsStat> sb);
ppu_error_code sys_fs_fstat(u32 fd, vm::ps3::ptr<CellFsStat> sb);
ppu_error_code sys_fs_mkdir(vm::ps3::cptr<char> path, s32 mode);
ppu_error_code sys_fs_rename(vm::ps3::cptr<char> from, vm::ps3::cptr<char> to);
ppu_error_code sys_fs_rmdir(vm::ps3::cptr<char> path);
ppu_error_code sys_fs_unlink(vm::ps3::cptr<char> path);
ppu_error_code sys_fs_fcntl(u32 fd, u32 op, vm::ps3::ptr<void> arg, u32 size);
ppu_error_code sys_fs_lseek(u32 fd, s64 offset, s32 whence, vm::ps3::ptr<u64> pos);
ppu_error_code sys_fs_fget_block_size(u32 fd, vm::ps3::ptr<u64> sector_size, vm::ps3::ptr<u64> block_size, vm::ps3::ptr<u64> arg4, vm::ps3::ptr<u64> arg5);
ppu_error_code sys_fs_get_block_size(vm::ps3::cptr<char> path, vm::ps3::ptr<u64> sector_size, vm::ps3::ptr<u64> block_size, vm::ps3::ptr<u64> arg4);
ppu_error_code sys_fs_truncate(vm::ps3::cptr<char> path, u64 size);
ppu_error_code sys_fs_ftruncate(u32 fd, u64 size);
ppu_error_code sys_fs_chmod(vm::ps3::cptr<char> path, s32 mode);
