#pragma once

#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/ErrorCodes.h"
#include "Utilities/File.h"
#include "Utilities/StrUtil.h"

#include <string>

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

enum : u32
{
	CELL_FS_IO_BUFFER_PAGE_SIZE_64KB = 0x0002,
	CELL_FS_IO_BUFFER_PAGE_SIZE_1MB  = 0x0004,
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

enum class lv2_mp_flag
{
	read_only,
	no_uid_gid,
	strict_get_block_size,
	cache,

	__bitset_enum_max
};

enum class lv2_file_type
{
	regular = 0,
	sdata,
	edata,
};

struct lv2_fs_mount_point
{
	const std::string_view root;
	const std::string_view file_system;
	const std::string_view device;
	const u32 sector_size = 512;
	const u64 sector_count = 256;
	const u32 block_size = 4096;
	const bs_t<lv2_mp_flag> flags{};
	lv2_fs_mount_point* const next = nullptr;

	mutable shared_mutex mutex;
};

extern lv2_fs_mount_point g_mp_sys_dev_hdd0;
extern lv2_fs_mount_point g_mp_sys_no_device;

struct lv2_fs_mount_info
{
	lv2_fs_mount_point* const mp;
	const std::string device;
	const std::string file_system;
	const bool read_only;

	lv2_fs_mount_info(lv2_fs_mount_point* mp = nullptr, std::string_view device = {}, std::string_view file_system = {}, bool read_only = false)
		: mp(mp ? mp : &g_mp_sys_no_device)
		, device(device.empty() ? this->mp->device : device)
		, file_system(file_system.empty() ? this->mp->file_system : file_system)
		, read_only((this->mp->flags & lv2_mp_flag::read_only) || read_only) // respect the original flags of the mount point as well
	{
	}

	constexpr bool operator==(const lv2_fs_mount_info& rhs) const noexcept
	{
		return this == &rhs;
	}
	constexpr bool operator==(const lv2_fs_mount_point* const& rhs) const noexcept
	{
		return mp == rhs;
	}
	constexpr lv2_fs_mount_point* operator->() const noexcept
	{
		return mp;
	}
};

extern lv2_fs_mount_info g_mi_sys_not_found;

struct CellFsMountInfo; // Forward Declaration

struct lv2_fs_mount_info_map
{
public:
	SAVESTATE_INIT_POS(40);

	lv2_fs_mount_info_map();
	lv2_fs_mount_info_map(const lv2_fs_mount_info_map&) = delete;
	lv2_fs_mount_info_map& operator=(const lv2_fs_mount_info_map&) = delete;
	~lv2_fs_mount_info_map();

	// Forwarding arguments to map.try_emplace(): refer to the constructor of lv2_fs_mount_info
	template <typename... Args>
	bool add(Args&&... args)
	{
		return map.try_emplace(std::forward<Args>(args)...).second;
	}
	bool remove(std::string_view path);
	const lv2_fs_mount_info& lookup(std::string_view path, bool no_cell_fs_path = false, std::string* mount_path = nullptr) const;
	u64 get_all(CellFsMountInfo* info = nullptr, u64 len = 0) const;
	bool is_device_mounted(std::string_view device_name) const;

	static bool vfs_unmount(std::string_view vpath, bool remove_from_map = true);

private:
	std::unordered_map<std::string, lv2_fs_mount_info, fmt::string_hash, std::equal_to<>> map;
};

struct lv2_fs_object
{
	static constexpr u32 id_base = 3;
	static constexpr u32 id_step = 1;
	static constexpr u32 id_count = 255 - id_base;
	static constexpr bool id_lowest = true;
	SAVESTATE_INIT_POS(49);

	// File Name (max 1055)
	const std::array<char, 0x420> name;

	// Mount Info
	const lv2_fs_mount_info& mp;

protected:
	lv2_fs_object(std::string_view filename);
	lv2_fs_object(utils::serial& ar, bool dummy);

public:
	lv2_fs_object(const lv2_fs_object&) = delete;

	lv2_fs_object& operator=(const lv2_fs_object&) = delete;

	// Normalize a virtual path
	static std::string get_normalized_path(std::string_view path);

	// Get the device's root path (e.g. "/dev_hdd0") from a given path
	static std::string get_device_root(std::string_view filename);

	// Filename can be either a path starting with '/' or a CELL_FS device name
	// This should be used only when handling devices that are not mounted
	// Otherwise, use g_fxo->get<lv2_fs_mount_info_map>().lookup() to look up mounted devices accurately
	static lv2_fs_mount_point* get_mp(std::string_view filename, std::string* vfs_path = nullptr);

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

	void save(utils::serial&) {}
};

struct lv2_file final : lv2_fs_object
{
	static constexpr u32 id_type = 1;

	fs::file file;
	const s32 mode;
	const s32 flags;
	std::string real_path;
	const lv2_file_type type;

	// IO Container
	u32 ct_id{}, ct_used{};

	// Stream lock
	atomic_t<u32> lock{0};

	// Some variables for convenience of data restoration
	struct save_restore_t
	{
		u64 seek_pos;
		u64 atime;
		u64 mtime;
	} restore_data{};

	lv2_file(std::string_view filename, fs::file&& file, s32 mode, s32 flags, const std::string& real_path, lv2_file_type type = {})
		: lv2_fs_object(filename)
		, file(std::move(file))
		, mode(mode)
		, flags(flags)
		, real_path(real_path)
		, type(type)
	{
	}

	lv2_file(const lv2_file& host, fs::file&& file, s32 mode, s32 flags, const std::string& real_path, lv2_file_type type = {})
		: lv2_fs_object(host.name.data())
		, file(std::move(file))
		, mode(mode)
		, flags(flags)
		, real_path(real_path)
		, type(type)
	{
	}

	lv2_file(utils::serial& ar);
	void save(utils::serial& ar);

	struct open_raw_result_t
	{
		CellError error;
		fs::file file;
	};

	struct open_result_t
	{
		CellError error;
		std::string ppath;
		std::string real_path;
		fs::file file;
		lv2_file_type type;
	};

	// Open a file with wrapped logic of sys_fs_open
	static open_raw_result_t open_raw(const std::string& path, s32 flags, s32 mode, lv2_file_type type = lv2_file_type::regular, const lv2_fs_mount_info& mp = g_mi_sys_not_found);
	static open_result_t open(std::string_view vpath, s32 flags, s32 mode, const void* arg = {}, u64 size = 0);

	// File reading with intermediate buffer
	static u64 op_read(const fs::file& file, vm::ptr<void> buf, u64 size, u64 opt_pos = umax);

	u64 op_read(vm::ptr<void> buf, u64 size, u64 opt_pos = umax) const
	{
		return op_read(file, buf, size, opt_pos);
	}

	// File writing with intermediate buffer
	static u64 op_write(const fs::file& file, vm::cptr<void> buf, u64 size);

	u64 op_write(vm::cptr<void> buf, u64 size) const
	{
		return op_write(file, buf, size);
	}

	// For MSELF support
	struct file_view;

	// Make file view from lv2_file object (for MSELF support)
	static fs::file make_view(const shared_ptr<lv2_file>& _file, u64 offset);
};

struct lv2_dir final : lv2_fs_object
{
	static constexpr u32 id_type = 2;

	const std::vector<fs::dir_entry> entries;

	// Current reading position
	atomic_t<u64> pos{0};

	lv2_dir(std::string_view filename, std::vector<fs::dir_entry>&& entries)
		: lv2_fs_object(filename)
		, entries(std::move(entries))
	{
	}

	lv2_dir(utils::serial& ar);
	void save(utils::serial& ar);

	// Read next
	const fs::dir_entry* dir_read()
	{
		const u64 old_pos = pos;

		if (const u64 cur = (old_pos < entries.size() ? pos++ : old_pos); cur < entries.size())
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

struct lv2_file_e0000025 : lv2_file_op
{
	be_t<u32> size; // 0x30
	be_t<u32> _x4;  // 0x10
	be_t<u32> _x8;  // 0x28 - offset of out_code
	be_t<u32> name_size;
	vm::bcptr<char> name;
	be_t<u32> _x14;
	be_t<u32> _x18;  // 0
	be_t<u32> _x1c;  // 0
	be_t<u32> _x20;  // 16
	be_t<u32> _x24;  // unk, seems to be memory location
	be_t<u32> out_code;  // out_code
	be_t<u32> fd;  // 0xffffffff - likely fd out
};

CHECK_SIZE(lv2_file_e0000025, 0x30);

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

// sys_fs_fcntl: cellFsArcadeHddSerialNumber
struct lv2_file_c0000007 : lv2_file_op
{
	be_t<u32> out_code; // set to 0
	vm::bcptr<char> device; // CELL_FS_IOS:ATA_HDD
	be_t<u32> device_size;  // 0x14
	vm::bptr<char> model;
	be_t<u32> model_size; // 0x29
	vm::bptr<char> serial;
	be_t<u32> serial_size; // 0x15
};

CHECK_SIZE(lv2_file_c0000007, 0x1c);

struct lv2_file_c0000008 : lv2_file_op
{
	u8 _x0[4];
	be_t<u32> op; // 0xC0000008
	u8 _x8[8];
	be_t<u64> container_id;
	be_t<u32> size;
	be_t<u32> page_type;  // 0x4000 for cellFsSetDefaultContainer
	                      // 0x4000 | page_type given by user, valid values seem to be:
	                      // CELL_FS_IO_BUFFER_PAGE_SIZE_64KB 0x0002
	                      // CELL_FS_IO_BUFFER_PAGE_SIZE_1MB  0x0004
	be_t<u32> out_code;
	u8 _x24[4];
};

CHECK_SIZE(lv2_file_c0000008, 0x28);

struct lv2_file_c0000015 : lv2_file_op
{
	be_t<u32> size; // 0x20
	be_t<u32> _x4;  // 0x10
	be_t<u32> _x8;  // 0x18 - offset of out_code
	be_t<u32> path_size;
	vm::bcptr<char> path;
	be_t<u32> _x14; //
	be_t<u16> vendorID;
	be_t<u16> productID;
	be_t<u32> out_code; // set to 0
};

CHECK_SIZE(lv2_file_c0000015, 0x20);

struct lv2_file_c000001a : lv2_file_op
{
	be_t<u32> disc_retry_type; // CELL_FS_DISC_READ_RETRY_NONE results in a 0 here
	                           // CELL_FS_DISC_READ_RETRY_DEFAULT results in a 0x63 here
	be_t<u32> _x4;             // 0
	be_t<u32> _x8;             // 0x000186A0
	be_t<u32> _xC;             // 0
	be_t<u32> _x10;            // 0
	be_t<u32> _x14;            // 0
};

CHECK_SIZE(lv2_file_c000001a, 0x18);

struct lv2_file_c000001c : lv2_file_op
{
	be_t<u32> size; // 0x60
	be_t<u32> _x4;  // 0x10
	be_t<u32> _x8;  // 0x18 - offset of out_code
	be_t<u32> path_size;
	vm::bcptr<char> path;
	be_t<u32> unk1;
	be_t<u16> vendorID;
	be_t<u16> productID;
	be_t<u32> out_code; // set to 0
	be_t<u16> serial[32];
};

CHECK_SIZE(lv2_file_c000001c, 0x60);

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
	be_t<u32> unk[5];      // 0x80, probably attributes
};

CHECK_SIZE(CellFsMountInfo, 0x94);

// Default IO container
struct default_sys_fs_container
{
	shared_mutex mutex;
	u32 id   = 0;
	u32 cap  = 0;
	u32 used = 0;
};

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
error_code sys_fs_newfs(ppu_thread& ppu, vm::cptr<char> dev_name, vm::cptr<char> file_system, s32 unk1, vm::cptr<char> str1);
error_code sys_fs_mount(ppu_thread& ppu, vm::cptr<char> dev_name, vm::cptr<char> file_system, vm::cptr<char> path, s32 unk1, s32 prot, s32 unk2, vm::cptr<char> str1, u32 str_len);
error_code sys_fs_unmount(ppu_thread& ppu, vm::cptr<char> path, s32 unk1, s32 force);
error_code sys_fs_get_mount_info_size(ppu_thread& ppu, vm::ptr<u64> len);
error_code sys_fs_get_mount_info(ppu_thread& ppu, vm::ptr<CellFsMountInfo> info, u64 len, vm::ptr<u64> out_len);
