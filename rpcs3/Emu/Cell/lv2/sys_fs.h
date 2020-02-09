#pragma once

#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/ErrorCodes.h"
#include "Utilities/File.h"

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
	CELL_FS_O_UNK     = 01000000, // Tests have shown this is independent of other flags.  Only known to be called in Rockband games.
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

struct CellFsDirectoryEntry
{
	CellFsStat attribute;
	CellFsDirent entry_name;
};

struct CellFsUtimbuf
{
	be_t<s64, 4> actime;
	be_t<s64, 4> modtime;
};

CHECK_SIZE_ALIGN(CellFsUtimbuf, 16, 4);

// MSelf file structs
struct FsMselfHeader
{
	be_t<u32> m_magic;
	be_t<u32> m_format_version;
	be_t<u64> m_file_size;
	be_t<u32> m_entry_num;
	be_t<u32> m_entry_size;
	u8 m_reserve[40];

};

struct FsMselfEntry
{
	char m_name[32];
	be_t<u64> m_offset;
	be_t<u64> m_size;
	u8 m_reserve[16];
};

struct lv2_fs_mount_point;

enum class lv2_mp_flag
{
	read_only,
	no_uid_gid,
	strict_get_block_size,

	__bitset_enum_max
};

struct lv2_fs_object
{
	using id_type = lv2_fs_object;

	static const u32 id_base = 3;
	static const u32 id_step = 1;
	static const u32 id_count = 255 - id_base;

	// Mount Point
	const std::add_pointer_t<lv2_fs_mount_point> mp;

	// File Name (max 1055)
	const std::array<char, 0x420> name;

	lv2_fs_object(lv2_fs_mount_point* mp, std::string_view filename)
		: mp(mp)
		, name(get_name(filename))
	{
	}

	static lv2_fs_mount_point* get_mp(std::string_view filename);

	static std::array<char, 0x420> get_name(std::string_view filename)
	{
		std::array<char, 0x420> name;

		if (filename.size() >= 0x420)
		{
			filename = filename.substr(0, 0x420 - 1);
		}

		filename.copy(name.data(), filename.size());
		name[filename.size()] = 0;
		return name;
	}
};

struct lv2_file final : lv2_fs_object
{
	const fs::file file;
	const s32 mode;
	const s32 flags;

	// Stream lock
	atomic_t<u32> lock{0};

	lv2_file(std::string_view filename, fs::file&& file, s32 mode, s32 flags)
		: lv2_fs_object(lv2_fs_object::get_mp(filename), filename)
		, file(std::move(file))
		, mode(mode)
		, flags(flags)
	{
	}

	lv2_file(const lv2_file& host, fs::file&& file, s32 mode, s32 flags)
		: lv2_fs_object(host.mp, host.name.data())
		, file(std::move(file))
		, mode(mode)
		, flags(flags)
	{
	}

	// File reading with intermediate buffer
	u64 op_read(vm::ptr<void> buf, u64 size);

	// File writing with intermediate buffer
	u64 op_write(vm::cptr<void> buf, u64 size);

	// For MSELF support
	struct file_view;

	// Make file view from lv2_file object (for MSELF support)
	static fs::file make_view(const std::shared_ptr<lv2_file>& _file, u64 offset);
};

struct lv2_dir final : lv2_fs_object
{
	const std::vector<fs::dir_entry> entries;

	// Current reading position
	atomic_t<u64> pos{0};

	lv2_dir(std::string_view filename, std::vector<fs::dir_entry>&& entries)
		: lv2_fs_object(lv2_fs_object::get_mp(filename), filename)
		, entries(std::move(entries))
	{
	}

	// Read next
	const fs::dir_entry* dir_read()
	{
		if (const u64 cur = pos++; cur < entries.size())
		{
			return &entries[cur];
		}

		return nullptr;
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

// sys_fs_fcntl: cellFsSdataOpenByFd
struct lv2_file_op_09 : lv2_file_op
{
	vm::bptrb<vtable::lv2_file_op> _vtable;

	be_t<u32> op;
	be_t<u32> _x8;
	be_t<u32> _xc;

	be_t<u32> fd;
	be_t<u64> offset;
	be_t<u32> _vtabl2;
	be_t<u32> arg1; // 0x180
	be_t<u32> arg2; // 0x10
	be_t<u32> arg_size; // 6th arg
	be_t<u32> arg_ptr; // 5th arg

	be_t<u32> _x34;
	be_t<s32> out_code;
	be_t<u32> out_fd;
};

CHECK_SIZE(lv2_file_op_09, 0x40);

// sys_fs_fnctl: cellFsGetDirectoryEntries
struct lv2_file_op_dir : lv2_file_op
{
	struct dir_info : lv2_file_op
	{
		be_t<s32> _code; // Op result
		be_t<u32> _size; // Number of entries written
		vm::bptrb<CellFsDirectoryEntry> ptr;
		be_t<u32> max;
	};

	CHECK_SIZE(dir_info, 0x10);

	vm::bptrb<vtable::lv2_file_op> _vtable;

	be_t<u32> op;
	be_t<u32> _x8;
	dir_info arg;
};

CHECK_SIZE(lv2_file_op_dir, 0x1c);

// sys_fs_fcntl: cellFsGetFreeSize (for dev_hdd0)
struct lv2_file_c0000002 : lv2_file_op
{
	vm::bptrb<vtable::lv2_file_op> _vtable;

	be_t<u32> op;
	be_t<u32> _x8;
	vm::bcptr<char> path;
	be_t<u32> _x10; // 0
	be_t<u32> _x14;

	be_t<u32> out_code; // CELL_ENOSYS
	be_t<u32> out_block_size;
	be_t<u64> out_block_count;
};

CHECK_SIZE(lv2_file_c0000002, 0x28);

// sys_fs_fcntl: unknown (called before cellFsOpen, for example)
struct lv2_file_c0000006 : lv2_file_op
{
	be_t<u32> size; // 0x20
	be_t<u32> _x4;  // 0x10
	be_t<u32> _x8;  // 0x18 - offset of out_code
	be_t<u32> name_size;
	vm::bcptr<char> name;
	be_t<u32> _x14; // 0
	be_t<u32> out_code; // 0x80010003
	be_t<u32> out_id; // set to 0, may return 0x1b5
};

CHECK_SIZE(lv2_file_c0000006, 0x20);

// sys_fs_fcntl: cellFsAllocateFileAreaWithoutZeroFill
struct lv2_file_e0000017 : lv2_file_op
{
	be_t<u32> size; // 0x28
	be_t<u32> _x4; // 0x10, offset
	be_t<u32> _x8; // 0x20, offset
	be_t<u32> _xc; // -
	vm::bcptr<char> file_path;
	be_t<u64> file_size;
	be_t<u32> out_code;
};

CHECK_SIZE(lv2_file_e0000017, 0x28);

struct CellFsMountInfo
{
	char mount_path[0x20]; // 0x0
	char filesystem[0x20]; // 0x20
	char dev_name[0x40];   // 0x40
	be_t<u32> unk1;        // 0x80
	be_t<u32> unk2;        // 0x84
	be_t<u32> unk3;        // 0x88
	be_t<u32> unk4;        // 0x8C
	be_t<u32> unk5;        // 0x90
};

CHECK_SIZE(CellFsMountInfo, 0x94);

// Syscalls

error_code sys_fs_test(ppu_thread& ppu, u32 arg1, u32 arg2, vm::ptr<u32> arg3, u32 arg4, vm::ptr<char> buf, u32 buf_size);
error_code sys_fs_open(ppu_thread& ppu, vm::cptr<char> path, s32 flags, vm::ptr<u32> fd, s32 mode, vm::cptr<void> arg, u64 size);
error_code sys_fs_read(ppu_thread& ppu, u32 fd, vm::ptr<void> buf, u64 nbytes, vm::ptr<u64> nread);
error_code sys_fs_write(ppu_thread& ppu, u32 fd, vm::cptr<void> buf, u64 nbytes, vm::ptr<u64> nwrite);
error_code sys_fs_close(ppu_thread& ppu, u32 fd);
error_code sys_fs_opendir(ppu_thread& ppu, vm::cptr<char> path, vm::ptr<u32> fd);
error_code sys_fs_readdir(ppu_thread& ppu, u32 fd, vm::ptr<CellFsDirent> dir, vm::ptr<u64> nread);
error_code sys_fs_closedir(ppu_thread& ppu, u32 fd);
error_code sys_fs_stat(ppu_thread& ppu, vm::cptr<char> path, vm::ptr<CellFsStat> sb);
error_code sys_fs_fstat(ppu_thread& ppu, u32 fd, vm::ptr<CellFsStat> sb);
error_code sys_fs_link(ppu_thread& ppu, vm::cptr<char> from, vm::cptr<char> to);
error_code sys_fs_mkdir(ppu_thread& ppu, vm::cptr<char> path, s32 mode);
error_code sys_fs_rename(ppu_thread& ppu, vm::cptr<char> from, vm::cptr<char> to);
error_code sys_fs_rmdir(ppu_thread& ppu, vm::cptr<char> path);
error_code sys_fs_unlink(ppu_thread& ppu, vm::cptr<char> path);
error_code sys_fs_access(ppu_thread& ppu, vm::cptr<char> path, s32 mode);
error_code sys_fs_fcntl(ppu_thread& ppu, u32 fd, u32 op, vm::ptr<void> arg, u32 size);
error_code sys_fs_lseek(ppu_thread& ppu, u32 fd, s64 offset, s32 whence, vm::ptr<u64> pos);
error_code sys_fs_fdatasync(ppu_thread& ppu, u32 fd);
error_code sys_fs_fsync(ppu_thread& ppu, u32 fd);
error_code sys_fs_fget_block_size(ppu_thread& ppu, u32 fd, vm::ptr<u64> sector_size, vm::ptr<u64> block_size, vm::ptr<u64> arg4, vm::ptr<s32> out_flags);
error_code sys_fs_get_block_size(ppu_thread& ppu, vm::cptr<char> path, vm::ptr<u64> sector_size, vm::ptr<u64> block_size, vm::ptr<u64> arg4);
error_code sys_fs_truncate(ppu_thread& ppu, vm::cptr<char> path, u64 size);
error_code sys_fs_ftruncate(ppu_thread& ppu, u32 fd, u64 size);
error_code sys_fs_symbolic_link(ppu_thread& ppu, vm::cptr<char> target, vm::cptr<char> linkpath);
error_code sys_fs_chmod(ppu_thread& ppu, vm::cptr<char> path, s32 mode);
error_code sys_fs_chown(ppu_thread& ppu, vm::cptr<char> path, s32 uid, s32 gid);
error_code sys_fs_disk_free(ppu_thread& ppu, vm::cptr<char> path, vm::ptr<u64> total_free, vm::ptr<u64> avail_free);
error_code sys_fs_utime(ppu_thread& ppu, vm::cptr<char> path, vm::cptr<CellFsUtimbuf> timep);
error_code sys_fs_acl_read(ppu_thread& ppu, vm::cptr<char> path, vm::ptr<void>);
error_code sys_fs_acl_write(ppu_thread& ppu, vm::cptr<char> path, vm::ptr<void>);
error_code sys_fs_lsn_get_cda_size(ppu_thread& ppu, u32 fd, vm::ptr<u64> ptr);
error_code sys_fs_lsn_get_cda(ppu_thread& ppu, u32 fd, vm::ptr<void>, u64, vm::ptr<u64>);
error_code sys_fs_lsn_lock(ppu_thread& ppu, u32 fd);
error_code sys_fs_lsn_unlock(ppu_thread& ppu, u32 fd);
error_code sys_fs_lsn_read(ppu_thread& ppu, u32 fd, vm::cptr<void>, u64);
error_code sys_fs_lsn_write(ppu_thread& ppu, u32 fd, vm::cptr<void>, u64);
error_code sys_fs_mapped_allocate(ppu_thread& ppu, u32 fd, u64, vm::pptr<void> out_ptr);
error_code sys_fs_mapped_free(ppu_thread& ppu, u32 fd, vm::ptr<void> ptr);
error_code sys_fs_truncate2(ppu_thread& ppu, u32 fd, u64 size);
error_code sys_fs_mount(ppu_thread& ppu, vm::cptr<char> dev_name, vm::cptr<char> file_system, vm::cptr<char> path, s32 unk1, s32 prot, s32 unk3, vm::cptr<char> str1, u32 str_len);
error_code sys_fs_get_mount_info_size(ppu_thread& ppu, vm::ptr<u64> len);
error_code sys_fs_get_mount_info(ppu_thread& ppu, vm::ptr<CellFsMountInfo> info, u32 len, vm::ptr<u64> out_len);
