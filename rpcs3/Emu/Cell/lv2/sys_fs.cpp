#include "stdafx.h"
#include "sys_sync.h"
#include "sys_fs.h"
#include "sys_memory.h"
#include "util/asm.hpp"

#include "Emu/Cell/PPUThread.h"
#include "Crypto/unedat.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/VFS.h"
#include "Emu/vfs_config.h"
#include "Emu/IdManager.h"
#include "Emu/system_utils.hpp"
#include "Emu/Cell/lv2/sys_process.h"

#include <filesystem>
#include <span>
#include <shared_mutex>

LOG_CHANNEL(sys_fs);

lv2_fs_mount_point g_mp_sys_dev_usb{"/dev_usb", "CELL_FS_FAT", "CELL_FS_IOS:USB_MASS_STORAGE", 512, 0x100, 4096, lv2_mp_flag::no_uid_gid};
lv2_fs_mount_point g_mp_sys_dev_dvd{"/dev_ps2disc", "CELL_FS_ISO9660", "CELL_FS_IOS:PATA1_BDVD_DRIVE", 2048, 0x100, 32768, lv2_mp_flag::read_only + lv2_mp_flag::no_uid_gid, &g_mp_sys_dev_usb};
lv2_fs_mount_point g_mp_sys_dev_bdvd{"/dev_bdvd", "CELL_FS_ISO9660", "CELL_FS_IOS:PATA0_BDVD_DRIVE", 2048, 0x4D955, 2048, lv2_mp_flag::read_only + lv2_mp_flag::no_uid_gid, &g_mp_sys_dev_dvd};
lv2_fs_mount_point g_mp_sys_dev_hdd1{"/dev_hdd1", "CELL_FS_FAT", "CELL_FS_UTILITY:HDD1", 512, 0x3FFFF8, 32768, lv2_mp_flag::no_uid_gid + lv2_mp_flag::cache, &g_mp_sys_dev_bdvd};
lv2_fs_mount_point g_mp_sys_dev_hdd0{"/dev_hdd0", "CELL_FS_UFS", "CELL_FS_UTILITY:HDD0", 512, 0x24FAEA98, 4096, {}, &g_mp_sys_dev_hdd1};
lv2_fs_mount_point g_mp_sys_dev_flash3{"/dev_flash3", "CELL_FS_FAT", "CELL_FS_IOS:BUILTIN_FLSH3", 512, 0x400, 8192, lv2_mp_flag::no_uid_gid, &g_mp_sys_dev_hdd0}; // TODO confirm
lv2_fs_mount_point g_mp_sys_dev_flash2{"/dev_flash2", "CELL_FS_FAT", "CELL_FS_IOS:BUILTIN_FLSH2", 512, 0x8000, 8192, lv2_mp_flag::no_uid_gid, &g_mp_sys_dev_flash3}; // TODO confirm
lv2_fs_mount_point g_mp_sys_dev_flash{"/dev_flash", "CELL_FS_FAT", "CELL_FS_IOS:BUILTIN_FLSH1", 512, 0x63E00, 8192, lv2_mp_flag::no_uid_gid, &g_mp_sys_dev_flash2};
lv2_fs_mount_point g_mp_sys_host_root{"/host_root", "CELL_FS_DUMMYFS", "CELL_FS_DUMMY:/", 512, 0x100, 512, lv2_mp_flag::strict_get_block_size + lv2_mp_flag::no_uid_gid, &g_mp_sys_dev_flash};
lv2_fs_mount_point g_mp_sys_app_home{"/app_home", "CELL_FS_DUMMYFS", "CELL_FS_DUMMY:", 512, 0x100, 512, lv2_mp_flag::strict_get_block_size + lv2_mp_flag::no_uid_gid, &g_mp_sys_host_root};
lv2_fs_mount_point g_mp_sys_dev_root{"/", "CELL_FS_ADMINFS", "CELL_FS_ADMINFS:", 512, 0x100, 512, lv2_mp_flag::read_only + lv2_mp_flag::strict_get_block_size + lv2_mp_flag::no_uid_gid, &g_mp_sys_app_home};
lv2_fs_mount_point g_mp_sys_no_device{};
lv2_fs_mount_info  g_mi_sys_not_found{}; // wrapper for &g_mp_sys_no_device

template<>
void fmt_class_string<lv2_file_type>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](lv2_file_type type)
	{
		switch (type)
		{
		case lv2_file_type::regular: return "Regular file";
		case lv2_file_type::sdata: return "SDATA";
		case lv2_file_type::edata: return "EDATA";
		}

		return unknown;
	});
}

template<>
void fmt_class_string<lv2_file>::format(std::string& out, u64 arg)
{
	const auto& file = get_object(arg);

	auto get_size = [](u64 size) -> std::string
	{
		if (size == umax)
		{
			return "N/A";
		}

		std::string size_str;
		switch (std::bit_width(size) / 10 * 10)
		{
		case 0: fmt::append(size_str, "%u", size); break;
		case 10: fmt::append(size_str, "%gKB", size / 1024.); break;
		case 20: fmt::append(size_str, "%gMB", size / (1024. * 1024)); break;

		default:
		case 30: fmt::append(size_str, "%gGB", size / (1024. * 1024 * 1024)); break;
		}

		return size_str;
	};

	const usz pos = file.file ? file.file.pos() : umax;
	const usz size = file.file ? file.file.size() : umax;

	fmt::append(out, u8"%s, '%s', Mode: 0x%x, Flags: 0x%x, Pos/Size: %s/%s (0x%x/0x%x)", file.type, file.name.data(), file.mode, file.flags, get_size(pos), get_size(size), pos, size);
}

template<>
void fmt_class_string<lv2_dir>::format(std::string& out, u64 arg)
{
	const auto& dir = get_object(arg);

	fmt::append(out, u8"Directory, '%s', Entries: %u/%u", dir.name.data(), std::min<u64>(dir.pos, dir.entries.size()), dir.entries.size());
}

bool has_fs_write_rights(std::string_view vpath)
{
	// VSH has access to everything
	if (g_ps3_process_info.has_root_perm())
		return true;

	const auto norm_vpath = lv2_fs_object::get_normalized_path(vpath);
	const auto parent_dir = fs::get_parent_dir_view(norm_vpath);

	// This is not exhaustive, PS3 has a unix filesystem with rights for each directory and files
	// This is mostly meant to protect against games doing insane things(ie NPUB30003 => NPUB30008)
	if (parent_dir == "/dev_hdd0" || parent_dir == "/dev_hdd0/game")
		return false;

	return true;
}

bool verify_mself(const fs::file& mself_file)
{
	FsMselfHeader mself_header;
	if (!mself_file.read<FsMselfHeader>(mself_header))
	{
		sys_fs.error("verify_mself: Didn't read expected bytes for header.");
		return false;
	}

	if (mself_header.m_magic != 0x4D534600u)
	{
		sys_fs.error("verify_mself: Header magic is incorrect.");
		return false;
	}

	if (mself_header.m_format_version != 1u)
	{
		sys_fs.error("verify_mself: Unexpected header format version.");
		return false;
	}

	// sanity check
	if (mself_header.m_entry_size != sizeof(FsMselfEntry))
	{
		sys_fs.error("verify_mself: Unexpected header entry size.");
		return false;
	}

	mself_file.seek(0);

	return true;
}

lv2_fs_mount_info_map::lv2_fs_mount_info_map()
{
	for (auto mp = &g_mp_sys_dev_root; mp; mp = mp->next) // Scan and keep track of pre-mounted devices
	{
		if (mp == &g_mp_sys_dev_usb)
		{
			for (int i = 0; i < 8; i++)
			{
				if (!vfs::get(fmt::format("%s%03d", mp->root, i)).empty())
				{
					add(fmt::format("%s%03d", mp->root, i), mp, fmt::format("%s%03d", mp->device, i), mp->file_system, false);
				}
			}
		}
		else if (mp == &g_mp_sys_dev_root || !vfs::get(mp->root).empty())
		{
			add(std::string(mp->root), mp, mp->device, mp->file_system, mp == &g_mp_sys_dev_flash); // /dev_flash is mounted in read only mode initially
		}
	}
}

lv2_fs_mount_info_map::~lv2_fs_mount_info_map()
{
	for (const auto& [path, info] : map)
		vfs_unmount(path, false); // Do not remove the value from the map we are iterating over.
}

bool lv2_fs_mount_info_map::remove(std::string_view path)
{
	if (const auto iterator = map.find(path); iterator != map.end())
	{
		map.erase(iterator);
		return true;
	}
	return false;
}

const lv2_fs_mount_info& lv2_fs_mount_info_map::lookup(std::string_view path, bool no_cell_fs_path, std::string* mount_path) const
{
	if (path.starts_with("/"sv))
	{
		constexpr std::string_view cell_fs_path = "CELL_FS_PATH:"sv;
		const std::string normalized_path = lv2_fs_object::get_normalized_path(path);
		std::string_view parent_dir;
		u32 parent_level = 0;

		do
		{
			parent_dir = fs::get_parent_dir_view(normalized_path, parent_level++);
			if (const auto iterator = map.find(parent_dir); iterator != map.end())
			{
				if (iterator->second == &g_mp_sys_dev_root && parent_level > 1)
					break;
				if (no_cell_fs_path && iterator->second.device.starts_with(cell_fs_path))
					return lookup(iterator->second.device.substr(cell_fs_path.size()), no_cell_fs_path, mount_path); // Recursively look up the parent mount info
				if (mount_path)
					*mount_path = iterator->first;
				return iterator->second;
			}
		} while (parent_dir.length() > 1); // Exit the loop when parent_dir == "/" or empty
	}

	return g_mi_sys_not_found;
}

u64 lv2_fs_mount_info_map::get_all(CellFsMountInfo* info, u64 len) const
{
	if (!info)
		return map.size();

	u64 count = 0;

	for (const auto& [path, mount_info] : map)
	{
		if (count >= len)
			break;

		strcpy_trunc(info[count].mount_path, path);
		strcpy_trunc(info[count].filesystem, mount_info.file_system);
		strcpy_trunc(info[count].dev_name, mount_info.device);
		if (mount_info.read_only)
			info[count].unk[4] |= 0x10000000;

		count++;
	}

	return count;
}

bool lv2_fs_mount_info_map::is_device_mounted(std::string_view device_name) const
{
	return std::any_of(map.begin(), map.end(), [&](const decltype(map)::value_type& info) { return info.second.device == device_name; });
}

bool lv2_fs_mount_info_map::vfs_unmount(std::string_view vpath, bool remove_from_map)
{
	const std::string local_path = vfs::get(vpath);

	if (local_path.empty())
		return false;

	if (fs::is_file(local_path))
	{
		if (fs::remove_file(local_path))
		{
			sys_fs.notice("Removed simplefs file \"%s\"", local_path);
		}
		else
		{
			sys_fs.error("Failed to remove simplefs file \"%s\"", local_path);
		}
	}

	const bool result = vfs::unmount(vpath);

	if (result && remove_from_map)
		g_fxo->get<lv2_fs_mount_info_map>().remove(vpath);

	return result;
}

std::string lv2_fs_object::get_normalized_path(std::string_view path)
{
	std::string normalized_path = std::filesystem::path(path).lexically_normal().string();

#ifdef _WIN32
	std::replace(normalized_path.begin(), normalized_path.end(), '\\', '/');
#endif

	if (normalized_path.ends_with('/'))
		normalized_path.pop_back();

	return normalized_path.empty() ? "/" : normalized_path;
}

std::string lv2_fs_object::get_device_root(std::string_view filename)
{
	std::string path = get_normalized_path(filename); // Prevent getting fooled by ".." trick such as "/dev_usb000/../dev_flash"

	if (const auto first = path.find_first_not_of("/"sv); first != umax)
	{
		if (const auto pos = path.substr(first).find_first_of("/"sv); pos != umax)
			path = path.substr(0, first + pos);
		path = path.substr(std::max<std::make_signed_t<usz>>(0, first - 1)); // Remove duplicate leading '/' while keeping only one
	}
	else
	{
		path = path.substr(0, 1);
	}

	return path;
}

lv2_fs_mount_point* lv2_fs_object::get_mp(std::string_view filename, std::string* vfs_path)
{
	constexpr std::string_view cell_fs_path = "CELL_FS_PATH:"sv;
	const bool is_cell_fs_path = filename.starts_with(cell_fs_path);

	if (is_cell_fs_path)
		filename.remove_prefix(cell_fs_path.size());

	const bool is_path = filename.starts_with("/"sv);
	std::string mp_name = is_path ? get_device_root(filename) : std::string(filename);

	const auto check_mp = [&]()
	{
		for (auto mp = &g_mp_sys_dev_root; mp; mp = mp->next)
		{
			const auto& device_alias_check = !is_path && (
				(mp == &g_mp_sys_dev_hdd0 && mp_name == "CELL_FS_IOS:PATA0_HDD_DRIVE"sv) ||
				(mp == &g_mp_sys_dev_hdd1 && mp_name == "CELL_FS_IOS:PATA1_HDD_DRIVE"sv) ||
				(mp == &g_mp_sys_dev_flash2 && mp_name == "CELL_FS_IOS:BUILTIN_FLASH"sv)); // TODO confirm

			if (mp == &g_mp_sys_dev_usb)
			{
				if (mp_name.starts_with(is_path ? mp->root : mp->device))
				{
					if (!is_path)
						mp_name = fmt::format("%s%s", mp->root, mp_name.substr(mp->device.size()));
					return mp;
				}
			}
			else if ((is_path ? mp->root : mp->device) == mp_name || device_alias_check)
			{
				if (!is_path)
					mp_name = mp->root;
				return mp;
			}
		}
		return &g_mp_sys_no_device; // Default fallback
	};

	const auto result = check_mp();

	if (vfs_path)
	{
		if (is_cell_fs_path)
			*vfs_path = vfs::get(filename);
		else if (result == &g_mp_sys_dev_hdd0)
			*vfs_path = g_cfg_vfs.get(g_cfg_vfs.dev_hdd0, rpcs3::utils::get_emu_dir());
		else if (result == &g_mp_sys_dev_hdd1)
			*vfs_path = g_cfg_vfs.get(g_cfg_vfs.dev_hdd1, rpcs3::utils::get_emu_dir());
		else if (result == &g_mp_sys_dev_usb)
			*vfs_path = g_cfg_vfs.get_device(g_cfg_vfs.dev_usb, mp_name, rpcs3::utils::get_emu_dir()).path;
		else if (result == &g_mp_sys_dev_bdvd)
			*vfs_path = g_cfg_vfs.get(g_cfg_vfs.dev_bdvd, rpcs3::utils::get_emu_dir());
		else if (result == &g_mp_sys_dev_dvd)
			*vfs_path = g_cfg_vfs.get(g_cfg_vfs.dev_bdvd, rpcs3::utils::get_emu_dir()); // For compatibility
		else if (result == &g_mp_sys_app_home)
			*vfs_path = g_cfg_vfs.get(g_cfg_vfs.app_home, rpcs3::utils::get_emu_dir());
		else if (result == &g_mp_sys_host_root && g_cfg.vfs.host_root)
			*vfs_path = "/";
		else if (result == &g_mp_sys_dev_flash)
			*vfs_path = g_cfg_vfs.get_dev_flash();
		else if (result == &g_mp_sys_dev_flash2)
			*vfs_path = g_cfg_vfs.get_dev_flash2();
		else if (result == &g_mp_sys_dev_flash3)
			*vfs_path = g_cfg_vfs.get_dev_flash3();
		else
			*vfs_path = {};

		if (is_path && !is_cell_fs_path && !vfs_path->empty())
			vfs_path->append(filename.substr(mp_name.size()));
	}

	return result;
}

lv2_fs_object::lv2_fs_object(std::string_view filename)
	: name(get_name(filename))
	, mp(g_fxo->get<lv2_fs_mount_info_map>().lookup(name.data()))
{
}

lv2_fs_object::lv2_fs_object(utils::serial& ar, bool)
	: name(ar)
	, mp(g_fxo->get<lv2_fs_mount_info_map>().lookup(name.data()))
{
}

u64 lv2_file::op_read(const fs::file& file, vm::ptr<void> buf, u64 size, u64 opt_pos)
{
	if (u64 region = buf.addr() >> 28, region_end = (buf.addr() & 0xfff'ffff) + (size & 0xfff'ffff); region == region_end && ((region >> 28) == 0 || region >= 0xC))
	{
		// Optimize reads from safe memory
		return (opt_pos == umax ? file.read(buf.get_ptr(), size) : file.read_at(opt_pos, buf.get_ptr(), size));
	}

	// Copy data from intermediate buffer (avoid passing vm pointer to a native API)
	std::vector<uchar> local_buf(std::min<u64>(size, 65536));

	u64 result = 0;

	while (result < size)
	{
		const u64 block = std::min<u64>(size - result, local_buf.size());
		const u64 nread = (opt_pos == umax ? file.read(local_buf.data(), block) : file.read_at(opt_pos + result, local_buf.data(), block));

		std::memcpy(static_cast<uchar*>(buf.get_ptr()) + result, local_buf.data(), nread);
		result += nread;

		if (nread < block)
		{
			break;
		}
	}

	return result;
}

u64 lv2_file::op_write(const fs::file& file, vm::cptr<void> buf, u64 size)
{
	// Copy data to intermediate buffer (avoid passing vm pointer to a native API)
	std::vector<uchar> local_buf(std::min<u64>(size, 65536));

	u64 result = 0;

	while (result < size)
	{
		const u64 block = std::min<u64>(size - result, local_buf.size());
		std::memcpy(local_buf.data(), static_cast<const uchar*>(buf.get_ptr()) + result, block);
		const u64 nwrite = file.write(+local_buf.data(), block);
		result += nwrite;

		if (nwrite < block)
		{
			break;
		}
	}

	return result;
}

lv2_file::lv2_file(utils::serial& ar)
	: lv2_fs_object(ar, false)
	, mode(ar)
	, flags(ar)
	, type(ar)
{
	[[maybe_unused]] const s32 version = GET_SERIALIZATION_VERSION(lv2_fs);

	ar(lock);

	be_t<u64> arg = 0;
	u64 size = 0;

	switch (type)
	{
	case lv2_file_type::regular: break;
	case lv2_file_type::sdata: arg = 0x18000000010, size = 8; break; // TODO: Fix
	case lv2_file_type::edata: arg = 0x2, size = 8; break;
	}

	const std::string retrieve_real = ar.pop<std::string>();

	if (type == lv2_file_type::edata && version >= 2)
	{
		ar(g_fxo->get<loaded_npdrm_keys>().one_time_key);
	}

	open_result_t res = lv2_file::open(retrieve_real, flags & CELL_FS_O_ACCMODE, mode, size ? &arg : nullptr, size);
	file = std::move(res.file);
	real_path = std::move(res.real_path);

	g_fxo->get<loaded_npdrm_keys>().npdrm_fds.raw() += type != lv2_file_type::regular;
	g_fxo->get<loaded_npdrm_keys>().one_time_key = {};

	if (ar.pop<bool>()) // see lv2_file::save in_mem
	{
		const fs::stat_t stat = ar;

		std::vector<u8> buf(stat.size);
		ar(std::span<u8>(buf.data(), buf.size()));

		file = fs::make_stream<std::vector<u8>>(std::move(buf), stat);
	}

	if (!file)
	{
		sys_fs.error("Failed to load \'%s\' file for savestates (res=%s, vpath=\'%s\', real-path=\'%s\', type=%s, flags=0x%x)", name.data(), res.error, retrieve_real, real_path, type, flags);
		ar.pos += sizeof(u64);
		ensure(!!g_cfg.savestate.state_inspection_mode);
		return;
	}
	else
	{
		sys_fs.success("Loaded file descriptor \'%s\' file for savestates (vpath=\'%s\', type=%s, flags=0x%x, id=%d)", name.data(), retrieve_real, type, flags, idm::last_id());
	}

	file.seek(ar);
}

void lv2_file::save(utils::serial& ar)
{
	USING_SERIALIZATION_VERSION(lv2_fs);
	ar(name, mode, flags, type, lock, ensure(vfs::retrieve(real_path), FN(!x.empty())));

	if (type == lv2_file_type::edata)
	{
		auto file_ptr = file.release();
		ar(static_cast<EDATADecrypter*>(file_ptr.get())->get_key());
		file.reset(std::move(file_ptr));
	}

	if (!mp.read_only && flags & CELL_FS_O_ACCMODE)
	{
		// Ensure accurate timestamps and content on disk
		file.sync();
	}

	// UNIX allows deletion of files while descriptors are still opened
	// descriptors shall keep the data in memory in this case
	const bool in_mem = [&]()
	{
		if (mp.read_only)
		{
			return false;
		}

		fs::file test{real_path};

		if (!test)
		{
			if (fs::is_file(real_path + ".66600"))
			{
				// May be a split-files descriptor, don't even bother
				return false;
			}

			return true;
		}

		fs::file_id test_s = test.get_id();
		fs::file_id file_s = file.get_id();

		return !test_s.is_coherent_with(file_s);
	}();

	ar(in_mem);

	if (in_mem)
	{
		fs::stat_t stats = file.get_stat();

		sys_fs.error("Saving \'%s\' LV2 file descriptor in memory! (exists=%s, type=%s, flags=0x%x, size=0x%x)", name.data(), fs::is_file(real_path), type, flags, stats.size);

		const usz patch_stats_pos = ar.seek_end();

		ar(stats);

		const usz old_end = ar.pad_from_end(stats.size);

		if (usz read_size = file.read_at(0, &ar.data[old_end], stats.size); read_size != stats.size)
		{
			ensure(read_size < stats.size);
			sys_fs.error("Read less than expected! (new-size=0x%x)", read_size);
			stats.size = read_size;
			ar.data.resize(old_end + stats.size);
			write_to_ptr<fs::stat_t>(&ar.data[patch_stats_pos], stats);
		}
	}

	ar(file.pos());
}

lv2_dir::lv2_dir(utils::serial& ar)
	: lv2_fs_object(ar, false)
	, entries([&]
	{
		std::vector<fs::dir_entry> entries;

		u64 size = 0;
		ar.deserialize_vle(size);
		entries.resize(size);

		for (auto& entry : entries)
		{
			ar(entry.name, static_cast<fs::stat_t&>(entry));
		}

		return entries;
	}())
	, pos(ar)
{
}

void lv2_dir::save(utils::serial& ar)
{
	USING_SERIALIZATION_VERSION(lv2_fs);

	ar(name);

	ar.serialize_vle(entries.size());

	for (auto& entry : entries)
	{
		ar(entry.name, static_cast<const fs::stat_t&>(entry));
	}

	ar(pos);
}

loaded_npdrm_keys::loaded_npdrm_keys(utils::serial& ar)
{
	save(ar);
}

void loaded_npdrm_keys::save(utils::serial& ar)
{
	ar(dec_keys_pos);
	ar(std::span(dec_keys, std::min<usz>(std::size(dec_keys), dec_keys_pos)));
}

struct lv2_file::file_view : fs::file_base
{
	const shared_ptr<lv2_file> m_file;
	const u64 m_off;
	u64 m_pos;

	explicit file_view(const shared_ptr<lv2_file>& _file, u64 offset)
		: m_file(_file)
		, m_off(offset)
		, m_pos(0)
	{
	}

	~file_view() override
	{
	}

	fs::stat_t get_stat() override
	{
		fs::stat_t stat = m_file->file.get_stat();

		// TODO: Check this on realhw
		//stat.size = utils::sub_saturate<u64>(stat.size, m_off);

		stat.is_writable = false;
		return stat;
	}

	bool trunc(u64) override
	{
		return false;
	}

	u64 read(void* buffer, u64 size) override
	{
		const u64 result = file_view::read_at(m_pos, buffer, size);

		m_pos += result;
		return result;
	}

	u64 read_at(u64 offset, void* buffer, u64 size) override
	{
		return m_file->file.read_at(m_off + offset, buffer, size);
	}

	u64 write(const void*, u64) override
	{
		return 0;
	}

	u64 seek(s64 offset, fs::seek_mode whence) override
	{
		const s64 new_pos =
			whence == fs::seek_set ? offset :
			whence == fs::seek_cur ? offset + m_pos :
			whence == fs::seek_end ? offset + size() : -1;

		if (new_pos < 0)
		{
			fs::g_tls_error = fs::error::inval;
			return -1;
		}

		m_pos = new_pos;
		return m_pos;
	}

	u64 size() override
	{
		return utils::sub_saturate<u64>(m_file->file.size(), m_off);
	}

	fs::file_id get_id() override
	{
		fs::file_id id = m_file->file.get_id();

		be_t<u64> off = m_off;
		const auto ptr = reinterpret_cast<u8*>(&off);

		id.data.insert(id.data.end(), ptr, ptr + sizeof(off));
		id.type.insert(0, "lv2_file::file_view: "sv);
		return id;
	}
};

fs::file lv2_file::make_view(const shared_ptr<lv2_file>& _file, u64 offset)
{
	fs::file result;
	result.reset(std::make_unique<lv2_file::file_view>(_file, offset));
	return result;
}

std::pair<CellError, std::string> translate_to_str(vm::cptr<char> ptr, bool is_path = true)
{
	constexpr usz max_length = CELL_FS_MAX_FS_PATH_LENGTH + 1;

	std::string path;

	if (!vm::read_string(ptr.addr(), max_length, path, true))
	{
		// Null character lookup has ended whilst pointing at invalid memory
		return {CELL_EFAULT, std::move(path)};
	}

	if (path.size() == max_length)
	{
		return {CELL_ENAMETOOLONG, {}};
	}

	if (is_path && !path.starts_with("/"sv))
	{
		return {CELL_ENOENT, std::move(path)};
	}

	return {{}, std::move(path)};
}

error_code sys_fs_test(ppu_thread&, u32 arg1, u32 arg2, vm::ptr<u32> arg3, u32 arg4, vm::ptr<char> buf, u32 buf_size)
{
	sys_fs.trace("sys_fs_test(arg1=0x%x, arg2=0x%x, arg3=*0x%x, arg4=0x%x, buf=*0x%x, buf_size=0x%x)", arg1, arg2, arg3, arg4, buf, buf_size);

	if (arg1 != 6 || arg2 != 0 || arg4 != sizeof(u32))
	{
		sys_fs.todo("sys_fs_test: unknown arguments (arg1=0x%x, arg2=0x%x, arg3=*0x%x, arg4=0x%x)", arg1, arg2, arg3, arg4);
	}

	if (!arg3)
	{
		return CELL_EFAULT;
	}

	const auto file = idm::get_unlocked<lv2_fs_object>(*arg3);

	if (!file)
	{
		return CELL_EBADF;
	}

	for (u32 i = 0; i < buf_size; i++)
	{
		if (!(buf[i] = file->name[i]))
		{
			return CELL_OK;
		}
	}

	buf[buf_size - 1] = 0;
	return CELL_OK;
}

lv2_file::open_raw_result_t lv2_file::open_raw(const std::string& local_path, s32 flags, s32 /*mode*/, lv2_file_type type, const lv2_fs_mount_info& mp)
{
	// TODO: other checks for path

	if (fs::is_dir(local_path))
	{
		return {CELL_EISDIR};
	}

	bs_t<fs::open_mode> open_mode{};

	switch (flags & CELL_FS_O_ACCMODE)
	{
	case CELL_FS_O_RDONLY: open_mode += fs::read; break;
	case CELL_FS_O_WRONLY: open_mode += fs::write; break;
	case CELL_FS_O_RDWR: open_mode += fs::read + fs::write; break;
	default: break;
	}

	if (mp.read_only)
	{
		if ((flags & CELL_FS_O_ACCMODE) != CELL_FS_O_RDONLY && fs::is_file(local_path))
		{
			return {CELL_EPERM};
		}
	}

	if (flags & CELL_FS_O_CREAT)
	{
		open_mode += fs::create;

		if (flags & CELL_FS_O_EXCL)
		{
			open_mode += fs::excl;
		}
	}

	if (flags & CELL_FS_O_TRUNC)
	{
		open_mode += fs::trunc;
	}

	if (flags & CELL_FS_O_MSELF)
	{
		open_mode = fs::read;
		// mself can be mself or mself | rdonly
		if (flags & ~(CELL_FS_O_MSELF | CELL_FS_O_RDONLY))
		{
			open_mode = {};
		}
	}

	if (flags & CELL_FS_O_UNK)
	{
		sys_fs.warning("lv2_file::open() called with CELL_FS_O_UNK flag enabled. FLAGS: %#o", flags);
	}

	if (mp.read_only)
	{
		// Deactivate mutating flags on read-only FS
		open_mode = fs::read;
	}

	// Tests have shown that invalid combinations get resolved internally (without exceptions), but that would complicate code with minimal accuracy gains.
	// For example, no games are known to try and call TRUNCATE | APPEND | RW, or APPEND | READ, which currently would cause an exception.
	if (flags & ~(CELL_FS_O_UNK | CELL_FS_O_ACCMODE | CELL_FS_O_CREAT | CELL_FS_O_TRUNC | CELL_FS_O_APPEND | CELL_FS_O_EXCL | CELL_FS_O_MSELF))
	{
		open_mode = {}; // error
	}

	if ((flags & CELL_FS_O_ACCMODE) == CELL_FS_O_ACCMODE)
	{
		open_mode = {}; // error
	}

	if (!open_mode)
	{
		fmt::throw_exception("lv2_file::open_raw(): Invalid or unimplemented flags: %#o", flags);
	}

	std::lock_guard lock(mp->mutex);

	fs::file file(local_path, open_mode);

	if (!file && open_mode == fs::read && fs::g_tls_error == fs::error::noent)
	{
		// Try to gather split file (TODO)
		std::vector<fs::file> fragments;

		for (u32 i = 66600; i <= 66699; i++)
		{
			if (fs::file fragment{fmt::format("%s.%u", local_path, i)})
			{
				fragments.emplace_back(std::move(fragment));
			}
			else
			{
				break;
			}
		}

		if (!fragments.empty())
		{
			file = fs::make_gather(std::move(fragments));
		}
	}

	if (!file)
	{
		if (mp.read_only)
		{
			// Failed to create file on read-only FS (file doesn't exist)
			if (flags & CELL_FS_O_ACCMODE && flags & CELL_FS_O_CREAT)
			{
				return {CELL_EPERM};
			}
		}

		if (open_mode & fs::excl && fs::g_tls_error == fs::error::exist)
		{
			return {CELL_EEXIST};
		}

		switch (auto error = fs::g_tls_error)
		{
		case fs::error::noent: return {CELL_ENOENT};
		default: sys_fs.error("lv2_file::open(): unknown error %s", error);
		}

		return {CELL_EIO};
	}

	if (flags & CELL_FS_O_MSELF && !verify_mself(file))
	{
		return {CELL_ENOTMSELF};
	}

	if (type >= lv2_file_type::sdata)
	{
		// check for sdata
		switch (type)
		{
		case lv2_file_type::sdata:
		{
			// check if the file has the NPD header, or else assume its not encrypted
			u32 magic;
			file.read<u32>(magic);
			file.seek(0);
			if (magic == "NPD\0"_u32)
			{
				auto sdata_file = std::make_unique<EDATADecrypter>(std::move(file));
				if (!sdata_file->ReadHeader())
				{
					return {CELL_EFSSPECIFIC};
				}

				file.reset(std::move(sdata_file));
			}

			break;
		}
		// edata
		case lv2_file_type::edata:
		{
			// check if the file has the NPD header, or else assume its not encrypted
			u32 magic;
			file.read<u32>(magic);
			file.seek(0);
			if (magic == "NPD\0"_u32)
			{
				auto& edatkeys = g_fxo->get<loaded_npdrm_keys>();

				const u64 init_pos = edatkeys.dec_keys_pos;
				const auto& dec_keys = edatkeys.dec_keys;
				const u64 max_i = std::min<u64>(std::size(dec_keys), init_pos);

				if (edatkeys.one_time_key)
				{
					auto edata_file = std::make_unique<EDATADecrypter>(std::move(file), edatkeys.one_time_key);
					edatkeys.one_time_key = {};

					if (!edata_file->ReadHeader())
					{
						// Read failure
						return {CELL_EFSSPECIFIC};
					}

					file.reset(std::move(edata_file));
					break;
				}

				for (u64 i = 0;; i++)
				{
					if (i == max_i)
					{
						// Run out of keys to try
						return {CELL_EFSSPECIFIC};
					}

					// Try all registered keys
					auto edata_file = std::make_unique<EDATADecrypter>(std::move(file), dec_keys[(init_pos - i - 1) % std::size(dec_keys)].load());
					if (!edata_file->ReadHeader())
					{
						// Prepare file for the next iteration
						file = std::move(edata_file->m_edata_file);
						continue;
					}

					file.reset(std::move(edata_file));
					break;

				}
			}

			break;
		}
		default: break;
		}
	}

	return {.error = {}, .file = std::move(file)};
}

lv2_file::open_result_t lv2_file::open(std::string_view vpath, s32 flags, s32 mode, const void* arg, u64 size)
{
	if (vpath.empty())
	{
		return {CELL_ENOENT};
	}

	std::string path;
	std::string local_path = vfs::get(vpath, nullptr, &path);

	const auto& mp = g_fxo->get<lv2_fs_mount_info_map>().lookup(vpath);

	if (mp == &g_mp_sys_dev_root)
	{
		return {CELL_EISDIR, path};
	}

	if (local_path.empty())
	{
		return {CELL_ENOTMOUNTED, path};
	}

	if (flags & CELL_FS_O_CREAT && !has_fs_write_rights(vpath) && !fs::is_dir(local_path))
	{
		return {CELL_EACCES};
	}

	lv2_file_type type = lv2_file_type::regular;

	if (size == 8)
	{
		// see lv2_file::open_raw
		switch (*static_cast<const be_t<u64, 1>*>(arg))
		{
		case 0x18000000010: type = lv2_file_type::sdata; break;
		case 0x2: type = lv2_file_type::edata; break;
		default:
			break;
		}
	}

	auto [error, file] = open_raw(local_path, flags, mode, type, mp);

	return {.error = error, .ppath = std::move(path), .real_path = std::move(local_path), .file = std::move(file), .type = type};
}

error_code sys_fs_open(ppu_thread& ppu, vm::cptr<char> path, s32 flags, vm::ptr<u32> fd, s32 mode, vm::cptr<void> arg, u64 size)
{
	ppu.state += cpu_flag::wait;
	lv2_obj::sleep(ppu);

	sys_fs.warning("sys_fs_open(path=%s, flags=%#o, fd=*0x%x, mode=%#o, arg=*0x%x, size=0x%llx)", path, flags, fd, mode, arg, size);

	const auto [path_error, vpath] = translate_to_str(path);

	if (path_error)
	{
		return {path_error, vpath};
	}

	auto [error, ppath, real, file, type] = lv2_file::open(vpath, flags, mode, arg.get_ptr(), size);

	if (error)
	{
		if (error == CELL_EEXIST)
		{
			return not_an_error(CELL_EEXIST);
		}

		return {g_fxo->get<lv2_fs_mount_info_map>().lookup(vpath) == &g_mp_sys_dev_hdd1 ? sys_fs.warning : sys_fs.error, error, path};
	}

	if (const u32 id = idm::import<lv2_fs_object, lv2_file>([&ppath = ppath, &file = file, mode, flags, &real = real, &type = type]() -> shared_ptr<lv2_file>
	{
		shared_ptr<lv2_file> result;

		if (type >= lv2_file_type::sdata && !g_fxo->get<loaded_npdrm_keys>().npdrm_fds.try_inc(16))
		{
			return result;
		}

		result = stx::make_shared<lv2_file>(ppath, std::move(file), mode, flags, real, type);
		sys_fs.warning("sys_fs_open(): fd=%u, %s", idm::last_id(), *result);
		return result;
	}))
	{
		ppu.check_state();
		*fd = id;
		return CELL_OK;
	}

	// Out of file descriptors
	return {CELL_EMFILE, path};
}

error_code sys_fs_read(ppu_thread& ppu, u32 fd, vm::ptr<void> buf, u64 nbytes, vm::ptr<u64> nread)
{
	ppu.state += cpu_flag::wait;
	lv2_obj::sleep(ppu);

	sys_fs.trace("sys_fs_read(fd=%d, buf=*0x%x, nbytes=0x%llx, nread=*0x%x)", fd, buf, nbytes, nread);

	if (!nread)
	{
		return CELL_EFAULT;
	}

	if (!buf)
	{
		nread.try_write(0);
		return CELL_EFAULT;
	}

	const auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(fd);

	if (!file || (nbytes && file->flags & CELL_FS_O_WRONLY))
	{
		nread.try_write(0); // nread writing is allowed to fail, error code is unchanged
		return CELL_EBADF;
	}

	if (!nbytes)
	{
		// Whole function is skipped, only EBADF and EBUSY are checked
		if (file->lock == 1)
		{
			nread.try_write(0);
			return CELL_EBUSY;
		}

		ppu.check_state();
		*nread = 0;
		return CELL_OK;
	}

	std::unique_lock lock(file->mp->mutex);

	if (!file->file)
	{
		return CELL_EBADF;
	}

	if (file->lock == 2)
	{
		nread.try_write(0);
		return CELL_EIO;
	}

	const u64 read_bytes = file->op_read(buf, nbytes);
	const bool failure = !read_bytes && file->file.pos() < file->file.size();
	lock.unlock();
	ppu.check_state();

	*nread = read_bytes;

	if (failure)
	{
		// EDATA corruption perhaps
		return CELL_EFSSPECIFIC;
	}

	return CELL_OK;
}

error_code sys_fs_write(ppu_thread& ppu, u32 fd, vm::cptr<void> buf, u64 nbytes, vm::ptr<u64> nwrite)
{
	ppu.state += cpu_flag::wait;
	lv2_obj::sleep(ppu);

	sys_fs.trace("sys_fs_write(fd=%d, buf=*0x%x, nbytes=0x%llx, nwrite=*0x%x)", fd, buf, nbytes, nwrite);

	if (!nwrite)
	{
		return CELL_EFAULT;
	}

	if (!buf)
	{
		nwrite.try_write(0);
		return CELL_EFAULT;
	}

	const auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(fd);

	if (!file || (nbytes && !(file->flags & CELL_FS_O_ACCMODE)))
	{
		nwrite.try_write(0); // nwrite writing is allowed to fail, error code is unchanged
		return CELL_EBADF;
	}

	if (!nbytes)
	{
		// Whole function is skipped, only EBADF and EBUSY are checked
		if (file->lock == 1)
		{
			nwrite.try_write(0);
			return CELL_EBUSY;
		}

		ppu.check_state();
		*nwrite = 0;
		return CELL_OK;
	}

	if (file->type != lv2_file_type::regular)
	{
		sys_fs.error("%s type: Writing %u bytes to FD=%d (path=%s)", file->type, nbytes, file->name.data());
	}

	if (file->mp.read_only)
	{
		nwrite.try_write(0);
		return CELL_EROFS;
	}

	std::unique_lock lock(file->mp->mutex);

	if (!file->file)
	{
		return CELL_EBADF;
	}

	if (file->lock)
	{
		if (file->lock == 2)
		{
			nwrite.try_write(0);
			return CELL_EIO;
		}

		nwrite.try_write(0);
		return CELL_EBUSY;
	}

	if (file->flags & CELL_FS_O_APPEND)
	{
		file->file.seek(0, fs::seek_end);
	}

	const u64 written = file->op_write(buf, nbytes);
	lock.unlock();
	ppu.check_state();

	*nwrite = written;
	return CELL_OK;
}

error_code sys_fs_close(ppu_thread& ppu, u32 fd)
{
	ppu.state += cpu_flag::wait;
	lv2_obj::sleep(ppu);

	const auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return {CELL_EBADF, fd};
	}

	std::string FD_state_log;

	if (sys_fs.warning)
	{
		FD_state_log = fmt::format("sys_fs_close(fd=%u)", fd);
	}

	{
		std::lock_guard lock(file->mp->mutex);

		if (!file->file)
		{
			sys_fs.warning("%s", FD_state_log);
			return {CELL_EBADF, fd};
		}

		if (!(file->mp.read_only && file->mp->flags & lv2_mp_flag::cache) && file->flags & CELL_FS_O_ACCMODE)
		{
			// Special: Ensure temporary directory for gamedata writes will remain on disk before final gamedata commitment
			file->file.sync(); // For cellGameContentPermit atomicity
		}

		if (!FD_state_log.empty())
		{
			sys_fs.warning("%s: %s", FD_state_log, *file);
		}

		// Free memory associated with fd if any
		if (file->ct_id && file->ct_used)
		{
			auto& default_container = g_fxo->get<default_sys_fs_container>();
			std::lock_guard lock(default_container.mutex);

			if (auto ct = idm::get_unlocked<lv2_memory_container>(file->ct_id))
			{
				ct->free(file->ct_used);
				if (default_container.id == file->ct_id)
				{
					default_container.used -= file->ct_used;
				}
			}
		}

		// Ensure Host file handle won't be kept open after this syscall
		file->file.close();
	}

	ensure(idm::withdraw<lv2_fs_object, lv2_file>(fd, [&](lv2_file& _file) -> CellError
	{
		if (_file.type >= lv2_file_type::sdata)
		{
			g_fxo->get<loaded_npdrm_keys>().npdrm_fds--;
		}

		return {};
	}));

	if (file->lock == 1)
	{
		return {CELL_EBUSY, fd};
	}

	return CELL_OK;
}

error_code sys_fs_opendir(ppu_thread& ppu, vm::cptr<char> path, vm::ptr<u32> fd)
{
	ppu.state += cpu_flag::wait;
	lv2_obj::sleep(ppu);

	sys_fs.warning("sys_fs_opendir(path=%s, fd=*0x%x)", path, fd);

	const auto [path_error, vpath] = translate_to_str(path);

	if (path_error)
	{
		return {path_error, vpath};
	}

	std::string processed_path;
	std::vector<std::string> ext;
	const std::string local_path = vfs::get(vpath, &ext, &processed_path);

	processed_path += "/";

	const auto& mp = g_fxo->get<lv2_fs_mount_info_map>().lookup(vpath);

	if (local_path.empty() && ext.empty())
	{
		return {CELL_ENOTMOUNTED, path};
	}

	// TODO: other checks for path

	if (fs::is_file(local_path))
	{
		return {CELL_ENOTDIR, path};
	}

	std::unique_lock lock(mp->mutex);

	const fs::dir dir(local_path);

	if (!dir)
	{
		switch (const auto error = fs::g_tls_error)
		{
		case fs::error::noent:
		{
			if (ext.empty())
			{
				return {mp == &g_mp_sys_dev_hdd1 ? sys_fs.warning : sys_fs.error, CELL_ENOENT, path};
			}

			break;
		}
		default:
		{
			sys_fs.error("sys_fs_opendir(): unknown error %s", error);
			return {CELL_EIO, path};
		}
		}
	}

	// Build directory as a vector of entries
	std::vector<fs::dir_entry> data;

	if (dir)
	{
		// Add real directories
		while (dir.read(data.emplace_back()))
		{
			// Preprocess entries
			data.back().name = vfs::unescape(data.back().name);

			if (!data.back().is_directory && data.back().name == "."sv)
			{
				// Files hidden from emulation
				data.resize(data.size() - 1);
				continue;
			}

			// Add additional entries for split file candidates (while ends with .66600)
			while (data.back().name.ends_with(".66600"))
			{
				data.emplace_back(data.back()).name.resize(data.back().name.size() - 6);
			}
		}

		data.resize(data.size() - 1);
	}
	else
	{
		data.emplace_back().name += '.';
		data.back().is_directory = true;
		data.emplace_back().name = "..";
		data.back().is_directory = true;
	}

	// Add mount points (TODO)
	for (auto&& ex : ext)
	{
		data.emplace_back().name = std::move(ex);
		data.back().is_directory = true;
	}

	// Sort files, keeping . and ..
	std::stable_sort(data.begin() + 2, data.end(), FN(x.name < y.name));

	// Remove duplicates
	data.erase(std::unique(data.begin(), data.end(), FN(x.name == y.name)), data.end());

	if (const u32 id = idm::make<lv2_fs_object, lv2_dir>(processed_path, std::move(data)))
	{
		lock.unlock();
		ppu.check_state();

		*fd = id;
		return CELL_OK;
	}

	// Out of file descriptors
	return CELL_EMFILE;
}

error_code sys_fs_readdir(ppu_thread& ppu, u32 fd, vm::ptr<CellFsDirent> dir, vm::ptr<u64> nread)
{
	ppu.state += cpu_flag::wait;

	sys_fs.warning("sys_fs_readdir(fd=%d, dir=*0x%x, nread=*0x%x)", fd, dir, nread);

	if (!dir || !nread)
	{
		return CELL_EFAULT;
	}

	const auto directory = idm::get_unlocked<lv2_fs_object, lv2_dir>(fd);

	if (!directory)
	{
		return CELL_EBADF;
	}

	ppu.check_state();

	auto* info = directory->dir_read();

	u64 nread_to_write = 0;

	if (info)
	{
		nread_to_write = sizeof(CellFsDirent);
	}
	else
	{
		// It does actually write polling the last entry. Seems consistent across HDD0 and HDD1 (TODO: check more partitions)
		info = &directory->entries.back();
		nread_to_write = 0;
	}

	CellFsDirent dir_write{};

	dir_write.d_type = info->is_directory ? CELL_FS_TYPE_DIRECTORY : CELL_FS_TYPE_REGULAR;
	dir_write.d_namlen = u8(std::min<usz>(info->name.size(), CELL_FS_MAX_FS_FILE_NAME_LENGTH));
	strcpy_trunc(dir_write.d_name, info->name);

	// TODO: Check more partitions (HDD1 is known to differ in actual filesystem implementation)
	if (directory->mp != &g_mp_sys_dev_hdd1 && nread_to_write == 0)
	{
		// First 3 bytes are being set to 0 here
		dir_write.d_type = 0;
		dir_write.d_namlen = 0;
		dir_write.d_name[0] = '\0';
	}

	*dir = dir_write;

	// Write after dir
	*nread = nread_to_write;
	return CELL_OK;
}

error_code sys_fs_closedir(ppu_thread& ppu, u32 fd)
{
	ppu.state += cpu_flag::wait;
	lv2_obj::sleep(ppu);

	sys_fs.warning("sys_fs_closedir(fd=%d)", fd);

	if (!idm::remove<lv2_fs_object, lv2_dir>(fd))
	{
		return CELL_EBADF;
	}

	return CELL_OK;
}

error_code sys_fs_stat(ppu_thread& ppu, vm::cptr<char> path, vm::ptr<CellFsStat> sb)
{
	ppu.state += cpu_flag::wait;
	lv2_obj::sleep(ppu);

	sys_fs.warning("sys_fs_stat(path=%s, sb=*0x%x)", path, sb);

	const auto [path_error, vpath] = translate_to_str(path);

	if (path_error)
	{
		return {path_error, vpath};
	}

	const std::string local_path = vfs::get(vpath);

	const auto& mp = g_fxo->get<lv2_fs_mount_info_map>().lookup(vpath);

	if (mp == &g_mp_sys_dev_root)
	{
		sb->mode = CELL_FS_S_IFDIR | 0711;
		sb->uid = -1;
		sb->gid = -1;
		sb->atime = -1;
		sb->mtime = -1;
		sb->ctime = -1;
		sb->size = 258;
		sb->blksize = 512;
		return CELL_OK;
	}

	if (local_path.empty())
	{
		// This syscall can be used by games and VSH to test the presence of dev_usb000 ~ dev_usb127
		// Thus there is no need to fuss about CELL_ENOTMOUNTED in this case
		return {sys_fs.warning, CELL_ENOTMOUNTED, path};
	}

	std::unique_lock lock(mp->mutex);

	fs::stat_t info{};

	if (!fs::get_stat(local_path, info))
	{
		switch (auto error = fs::g_tls_error)
		{
		case fs::error::noent:
		{
			// Try to analyse split file (TODO)
			u64 total_size = 0;

			for (u32 i = 66601; i <= 66699; i++)
			{
				if (fs::get_stat(fmt::format("%s.%u", local_path, i), info) && !info.is_directory)
				{
					total_size += info.size;
				}
				else
				{
					break;
				}
			}

			// Use attributes from the first fragment (consistently with sys_fs_open+fstat)
			if (fs::get_stat(local_path + ".66600", info) && !info.is_directory)
			{
				// Success
				info.size += total_size;
				break;
			}

			return {mp == &g_mp_sys_dev_hdd1 ? sys_fs.warning : sys_fs.error, CELL_ENOENT, path};
		}
		default:
		{
			sys_fs.error("sys_fs_stat(): unknown error %s", error);
			return {CELL_EIO, path};
		}
		}
	}

	lock.unlock();
	ppu.check_state();

	s32 mode = info.is_directory ? CELL_FS_S_IFDIR | 0777 : CELL_FS_S_IFREG | 0666;

	if (mp.read_only)
	{
		// Remove write permissions
		mode &= ~0222;
	}

	sb->mode = mode;
	sb->uid = mp->flags & lv2_mp_flag::no_uid_gid ? -1 : 0;
	sb->gid = mp->flags & lv2_mp_flag::no_uid_gid ? -1 : 0;
	sb->atime = info.atime;
	sb->mtime = info.mtime;
	sb->ctime = info.ctime;
	sb->size = info.is_directory ? mp->block_size : info.size;
	sb->blksize = mp->block_size;

	return CELL_OK;
}

error_code sys_fs_fstat(ppu_thread& ppu, u32 fd, vm::ptr<CellFsStat> sb)
{
	ppu.state += cpu_flag::wait;
	lv2_obj::sleep(ppu);

	sys_fs.warning("sys_fs_fstat(fd=%d, sb=*0x%x)", fd, sb);

	const auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	std::unique_lock lock(file->mp->mutex);

	if (!file->file)
	{
		return CELL_EBADF;
	}

	if (file->lock == 2)
	{
		return CELL_EIO;
	}

	const fs::stat_t info = file->file.get_stat();
	lock.unlock();
	ppu.check_state();

	s32 mode = info.is_directory ? CELL_FS_S_IFDIR | 0777 : CELL_FS_S_IFREG | 0666;

	if (file->mp.read_only)
	{
		// Remove write permissions
		mode &= ~0222;
	}

	sb->mode = mode;
	sb->uid = file->mp->flags & lv2_mp_flag::no_uid_gid ? -1 : 0;
	sb->gid = file->mp->flags & lv2_mp_flag::no_uid_gid ? -1 : 0;
	sb->atime = info.atime;
	sb->mtime = info.mtime;
	sb->ctime = info.ctime; // ctime may be incorrect
	sb->size = info.size;
	sb->blksize = file->mp->block_size;
	return CELL_OK;
}

error_code sys_fs_link(ppu_thread&, vm::cptr<char> from, vm::cptr<char> to)
{
	sys_fs.todo("sys_fs_link(from=%s, to=%s)", from, to);

	return CELL_OK;
}

error_code sys_fs_mkdir(ppu_thread& ppu, vm::cptr<char> path, s32 mode)
{
	ppu.state += cpu_flag::wait;
	lv2_obj::sleep(ppu);

	sys_fs.warning("sys_fs_mkdir(path=%s, mode=%#o)", path, mode);

	const auto [path_error, vpath] = translate_to_str(path);

	if (path_error)
	{
		return {path_error, vpath};
	}

	const std::string local_path = vfs::get(vpath);

	const auto& mp = g_fxo->get<lv2_fs_mount_info_map>().lookup(vpath);

	if (mp == &g_mp_sys_dev_root)
	{
		return {CELL_EEXIST, path};
	}

	if (local_path.empty())
	{
		return {CELL_ENOTMOUNTED, path};
	}

	if (mp.read_only)
	{
		return {CELL_EROFS, path};
	}

	if (!fs::exists(local_path) && !has_fs_write_rights(path.get_ptr()))
	{
		return {CELL_EACCES, path};
	}

	std::lock_guard lock(mp->mutex);

	if (!fs::create_dir(local_path))
	{
		switch (auto error = fs::g_tls_error)
		{
		case fs::error::noent:
		{
			return {mp == &g_mp_sys_dev_hdd1 ? sys_fs.warning : sys_fs.error, CELL_ENOENT, path};
		}
		case fs::error::exist:
		{
			return {sys_fs.warning, CELL_EEXIST, path};
		}
		default: sys_fs.error("sys_fs_mkdir(): unknown error %s", error);
		}

		return {CELL_EIO, path}; // ???
	}

	sys_fs.notice("sys_fs_mkdir(): directory %s created", path);
	return CELL_OK;
}

error_code sys_fs_rename(ppu_thread& ppu, vm::cptr<char> from, vm::cptr<char> to)
{
	ppu.state += cpu_flag::wait;
	lv2_obj::sleep(ppu);

	sys_fs.warning("sys_fs_rename(from=%s, to=%s)", from, to);

	const auto [from_error, vfrom] = translate_to_str(from);

	if (from_error)
	{
		return {from_error, vfrom};
	}

	const auto [to_error, vto] = translate_to_str(to);

	if (to_error)
	{
		return {to_error, vto};
	}

	const std::string local_from = vfs::get(vfrom);
	const std::string local_to = vfs::get(vto);

	const auto& mp = g_fxo->get<lv2_fs_mount_info_map>().lookup(vfrom);
	const auto& mp_to = g_fxo->get<lv2_fs_mount_info_map>().lookup(vto);

	if (mp == &g_mp_sys_dev_root || mp_to == &g_mp_sys_dev_root)
	{
		return CELL_EPERM;
	}

	if (local_from.empty() || local_to.empty())
	{
		return CELL_ENOTMOUNTED;
	}

	if (mp != mp_to)
	{
		return CELL_EXDEV;
	}

	if (mp.read_only)
	{
		return CELL_EROFS;
	}

	// Done in vfs::host::rename
	//std::lock_guard lock(mp->mutex);

	if (!vfs::host::rename(local_from, local_to, mp.mp, false))
	{
		switch (auto error = fs::g_tls_error)
		{
		case fs::error::noent: return {CELL_ENOENT, from};
		case fs::error::exist: return {CELL_EEXIST, to};
		default: sys_fs.error("sys_fs_rename(): unknown error %s", error);
		}

		return {CELL_EIO, from}; // ???
	}

	sys_fs.notice("sys_fs_rename(): %s renamed to %s", from, to);
	return CELL_OK;
}

error_code sys_fs_rmdir(ppu_thread& ppu, vm::cptr<char> path)
{
	ppu.state += cpu_flag::wait;
	lv2_obj::sleep(ppu);

	sys_fs.warning("sys_fs_rmdir(path=%s)", path);

	const auto [path_error, vpath] = translate_to_str(path);

	if (path_error)
	{
		return {path_error, vpath};
	}

	const std::string local_path = vfs::get(vpath);

	const auto& mp = g_fxo->get<lv2_fs_mount_info_map>().lookup(vpath);

	if (mp == &g_mp_sys_dev_root)
	{
		return {CELL_EPERM, path};
	}

	if (local_path.empty())
	{
		return {CELL_ENOTMOUNTED, path};
	}

	if (mp.read_only)
	{
		return {CELL_EROFS, path};
	}

	if (fs::is_dir(local_path) && !has_fs_write_rights(path.get_ptr()))
	{
		return {CELL_EACCES};
	}

	std::lock_guard lock(mp->mutex);

	if (!fs::remove_dir(local_path))
	{
		switch (auto error = fs::g_tls_error)
		{
		case fs::error::noent: return {CELL_ENOENT, path};
		case fs::error::notempty: return {CELL_ENOTEMPTY, path};
		default: sys_fs.error("sys_fs_rmdir(): unknown error %s", error);
		}

		return {CELL_EIO, path}; // ???
	}

	sys_fs.notice("sys_fs_rmdir(): directory %s removed", path);
	return CELL_OK;
}

error_code sys_fs_unlink(ppu_thread& ppu, vm::cptr<char> path)
{
	ppu.state += cpu_flag::wait;
	lv2_obj::sleep(ppu);

	sys_fs.warning("sys_fs_unlink(path=%s)", path);

	const auto [path_error, vpath] = translate_to_str(path);

	if (path_error)
	{
		return {path_error, vpath};
	}

	const std::string local_path = vfs::get(vpath);

	std::string mount_path = fs::get_parent_dir(vpath); // Use its parent directory as fallback
	const auto& mp = g_fxo->get<lv2_fs_mount_info_map>().lookup(vpath, true, &mount_path);

	if (mp == &g_mp_sys_dev_root)
	{
		return {CELL_EISDIR, path};
	}

	if (local_path.empty())
	{
		return {CELL_ENOTMOUNTED, path};
	}

	if (fs::is_dir(local_path))
	{
		return {CELL_EISDIR, path};
	}

	if (mp.read_only)
	{
		return {CELL_EROFS, path};
	}

	std::lock_guard lock(mp->mutex);

	if (!vfs::host::unlink(local_path, vfs::get(mount_path)))
	{
		switch (auto error = fs::g_tls_error)
		{
		case fs::error::noent:
		{
			return {mp == &g_mp_sys_dev_hdd1 ? sys_fs.warning : sys_fs.error, CELL_ENOENT, path};
		}
		default: sys_fs.error("sys_fs_unlink(): unknown error %s", error);
		}

		return {CELL_EIO, path}; // ???
	}

	sys_fs.notice("sys_fs_unlink(): file %s deleted", path);
	return CELL_OK;
}

error_code sys_fs_access(ppu_thread&, vm::cptr<char> path, s32 mode)
{
	sys_fs.todo("sys_fs_access(path=%s, mode=%#o)", path, mode);

	return CELL_OK;
}

error_code sys_fs_fcntl(ppu_thread& ppu, u32 fd, u32 op, vm::ptr<void> _arg, u32 _size)
{
	ppu.state += cpu_flag::wait;

	sys_fs.trace("sys_fs_fcntl(fd=%d, op=0x%x, arg=*0x%x, size=0x%x)", fd, op, _arg, _size);

	switch (op)
	{
	case 0x80000004: // Unknown
	{
		if (_size > 4)
		{
			return CELL_EINVAL;
		}

		const auto arg = vm::static_ptr_cast<u32>(_arg);
		*arg = 0;
		break;
	}

	case 0x80000006: // cellFsAllocateFileAreaByFdWithInitialData
	{
		break;
	}

	case 0x80000007: // cellFsAllocateFileAreaByFdWithoutZeroFill
	{
		break;
	}

	case 0x80000008: // cellFsChangeFileSizeByFdWithoutAllocation
	{
		break;
	}

	case 0x8000000a: // cellFsReadWithOffset
	case 0x8000000b: // cellFsWriteWithOffset
	{
		lv2_obj::sleep(ppu);

		const auto arg = vm::static_ptr_cast<lv2_file_op_rw>(_arg);

		if (_size < arg.size())
		{
			return CELL_EINVAL;
		}

		const auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(fd);

		if (!file)
		{
			return CELL_EBADF;
		}

		if (op == 0x8000000a && file->flags & CELL_FS_O_WRONLY)
		{
			return CELL_EBADF;
		}

		if (op == 0x8000000b && !(file->flags & CELL_FS_O_ACCMODE))
		{
			return CELL_EBADF;
		}

		if (op == 0x8000000b && file->flags & CELL_FS_O_APPEND)
		{
			return CELL_EBADF;
		}

		if (op == 0x8000000b && file->mp.read_only)
		{
			return CELL_EROFS;
		}

		if (op == 0x8000000b && file->type != lv2_file_type::regular && arg->size)
		{
			sys_fs.error("%s type: Writing %u bytes to FD=%d (path=%s)", file->type, arg->size, file->name.data());
		}

		std::unique_lock wlock(file->mp->mutex, std::defer_lock);
		std::shared_lock rlock(file->mp->mutex, std::defer_lock);

		if (op == 0x8000000b)
		{
			// Writer lock
			wlock.lock();
		}
		else
		{
			// Reader lock (not needing exclusivity in this special case because the state should not change)
			rlock.lock();
		}

		if (!file->file)
		{
			return CELL_EBADF;
		}

		if (file->lock == 2)
		{
			return CELL_EIO;
		}

		if (op == 0x8000000b && file->lock)
		{
			return CELL_EBUSY;
		}

		u64 old_pos = umax;
		const u64 op_pos = arg->offset;

		if (op == 0x8000000b)
		{
			old_pos = file->file.pos();
			file->file.seek(op_pos);
		}

		arg->out_size = op == 0x8000000a
			? file->op_read(arg->buf, arg->size, op_pos)
			: file->op_write(arg->buf, arg->size);

		if (op == 0x8000000b)
		{
			ensure(old_pos == file->file.seek(old_pos));
		}

		// TODO: EDATA corruption detection

		arg->out_code = CELL_OK;
		return CELL_OK;
	}

	case 0x80000009: // cellFsSdataOpenByFd
	{
		lv2_obj::sleep(ppu);

		const auto arg = vm::static_ptr_cast<lv2_file_op_09>(_arg);

		if (_size < arg.size())
		{
			return CELL_EINVAL;
		}

		const auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(fd);

		if (!file)
		{
			return {CELL_EBADF, "fd=%u", fd};
		}

		sys_fs.warning("sys_fs_fcntl(0x80000009): fd=%d, arg->offset=0x%x, size=0x%x (file: %s)", fd, arg->offset, _size, *file);

		std::lock_guard lock(file->mp->mutex);

		if (!file->file)
		{
			return {CELL_EBADF, "fd=%u", fd};
		}

		auto sdata_file = std::make_unique<EDATADecrypter>(lv2_file::make_view(file, arg->offset));

		if (!sdata_file->ReadHeader())
		{
			return {CELL_EFSSPECIFIC, "fd=%u", fd};
		}

		fs::file stream;
		stream.reset(std::move(sdata_file));
		if (const u32 id = idm::import<lv2_fs_object, lv2_file>([&file = *file, &stream = stream]() -> shared_ptr<lv2_file>
		{
			if (!g_fxo->get<loaded_npdrm_keys>().npdrm_fds.try_inc(16))
			{
				return null_ptr;
			}

			return stx::make_shared<lv2_file>(file, std::move(stream), file.mode, CELL_FS_O_RDONLY, file.real_path, lv2_file_type::sdata);
		}))
		{
			arg->out_code = CELL_OK;
			arg->out_fd = id;
			return CELL_OK;
		}

		// Out of file descriptors
		return CELL_EMFILE;
	}

	case 0xc0000002: // cellFsGetFreeSize (TODO)
	{
		lv2_obj::sleep(ppu);

		const auto arg = vm::static_ptr_cast<lv2_file_c0000002>(_arg);

		const auto& mp = g_fxo->get<lv2_fs_mount_info_map>().lookup("/dev_hdd0");

		arg->out_block_size = mp->block_size;
		arg->out_block_count = (40ull * 1024 * 1024 * 1024 - 1) / mp->block_size; // Read explanation in cellHddGameCheck
		return CELL_OK;
	}

	case 0xc0000003: // cellFsUtilitySetFakeSize
	{
		break;
	}

	case 0xc0000004: // cellFsUtilityGetFakeSize
	{
		break;
	}

	case 0xc0000006: // Unknown
	{
		const auto arg = vm::static_ptr_cast<lv2_file_c0000006>(_arg);

		if (arg->size != 0x20u)
		{
			sys_fs.error("sys_fs_fcntl(0xc0000006): invalid size (0x%x)", arg->size);
			break;
		}

		if (arg->_x4 != 0x10u || arg->_x8 != 0x18u)
		{
			sys_fs.error("sys_fs_fcntl(0xc0000006): invalid args (0x%x, 0x%x)", arg->_x4, arg->_x8);
			break;
		}

		// Load mountpoint (doesn't support multiple // at the start)
		std::string_view vpath{arg->name.get_ptr(), arg->name_size};

		sys_fs.notice("sys_fs_fcntl(0xc0000006): %s", vpath);

		// Check only mountpoint
		vpath = vpath.substr(0, vpath.find_first_of("/", 1));

		// Some mountpoints seem to be handled specially
		if (false)
		{
			// TODO: /dev_hdd1, /dev_usb000, /dev_flash
			//arg->out_code = CELL_OK;
			//arg->out_id = 0x1b5;
		}

		arg->out_code = CELL_ENOTSUP;
		arg->out_id = 0;
		return CELL_OK;
	}

	case 0xc0000007: // cellFsArcadeHddSerialNumber
	{
		const auto arg = vm::static_ptr_cast<lv2_file_c0000007>(_arg);

		arg->out_code = CELL_OK;

		if (const auto size = arg->model_size; size > 0)
			strcpy_trunc(std::span(arg->model.get_ptr(), size),
				fmt::format("%-*s", size - 1, g_cfg.sys.hdd_model.to_string())); // Example: "TOSHIBA MK3265GSX H                     "

		if (const auto size = arg->serial_size; size > 0)
			strcpy_trunc(std::span(arg->serial.get_ptr(), size),
				fmt::format("%*s", size - 1, g_cfg.sys.hdd_serial.to_string())); // Example: "           0A1B2C3D4"
		else
			return CELL_EFAULT; // CELL_EFAULT is returned only when arg->serial_size == 0

		return CELL_OK;
	}

	case 0xc0000008: // cellFsSetDefaultContainer, cellFsSetIoBuffer, cellFsSetIoBufferFromDefaultContainer
	{
		// Allocates memory from a container/default container to a specific fd or default IO processing
		const auto arg          = vm::static_ptr_cast<lv2_file_c0000008>(_arg);
		auto& default_container = g_fxo->get<default_sys_fs_container>();

		std::lock_guard def_container_lock(default_container.mutex);

		if (fd == 0xFFFFFFFF)
		{
			// No check on container is done when setting default container
			default_container.id   = arg->size ? ::narrow<u32>(arg->container_id) : 0u;
			default_container.cap  = arg->size;
			default_container.used = 0;

			arg->out_code = CELL_OK;
			return CELL_OK;
		}

		auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(fd);
		if (!file)
		{
			return CELL_EBADF;
		}

		if (auto ct = idm::get_unlocked<lv2_memory_container>(file->ct_id))
		{
			ct->free(file->ct_used);
			if (default_container.id == file->ct_id)
			{
				default_container.used -= file->ct_used;
			}
		}

		file->ct_id   = 0;
		file->ct_used = 0;

		// Aligns on lower bound
		u32 actual_size = arg->size - (arg->size % ((arg->page_type & CELL_FS_IO_BUFFER_PAGE_SIZE_64KB) ? 0x10000 : 0x100000));

		if (!actual_size)
		{
			arg->out_code = CELL_OK;
			return CELL_OK;
		}

		u32 new_container_id = arg->container_id == 0xFFFFFFFF ? default_container.id : ::narrow<u32>(arg->container_id);
		if (default_container.id == new_container_id && (default_container.used + actual_size) > default_container.cap)
		{
			return CELL_ENOMEM;
		}

		const auto ct = idm::get<lv2_memory_container>(new_container_id, [&](lv2_memory_container& ct) -> CellError
			{
				if (!ct.take(actual_size))
				{
					return CELL_ENOMEM;
				}

				return {};
			});

		if (!ct)
		{
			return CELL_ESRCH;
		}

		if (ct.ret)
		{
			return ct.ret;
		}

		if (default_container.id == new_container_id)
		{
			default_container.used += actual_size;
		}

		file->ct_id   = new_container_id;
		file->ct_used = actual_size;

		arg->out_code = CELL_OK;
		return CELL_OK;
	}

	case 0xc0000015: // USB Vid/Pid query
	case 0xc000001c: // USB Vid/Pid/Serial query
	{
		const auto arg = vm::static_ptr_cast<lv2_file_c0000015>(_arg);
		const bool with_serial = op == 0xc000001c;

		if (arg->size != (with_serial ? sizeof(lv2_file_c000001c) : sizeof(lv2_file_c0000015)))
		{
			sys_fs.error("sys_fs_fcntl(0x%08x): invalid size (0x%x)", op, arg->size);
			break;
		}

		if (arg->_x4 != 0x10u || arg->_x8 != 0x18u)
		{
			sys_fs.error("sys_fs_fcntl(0x%08x): invalid args (0x%x, 0x%x)", op, arg->_x4, arg->_x8);
			break;
		}

		std::string_view vpath{arg->path.get_ptr(), arg->path_size};

		if (vpath.size() == 0)
			return CELL_ENOMEM;

		// Trim trailing '\0'
		if (const auto trim_pos = vpath.find('\0'); trim_pos != umax)
			vpath.remove_suffix(vpath.size() - trim_pos);

		arg->out_code = CELL_ENOTMOUNTED; // arg->out_code is set to CELL_ENOTMOUNTED on real hardware when the device doesn't exist or when the device isn't USB

		if (!vfs::get(vpath).empty())
		{
			if (const auto& mp = g_fxo->get<lv2_fs_mount_info_map>().lookup(vpath, true); mp == &g_mp_sys_dev_usb)
			{
				const cfg::device_info device = g_cfg_vfs.get_device(g_cfg_vfs.dev_usb, fmt::format("%s%s", mp->root, mp.device.substr(mp->device.size())));
				const auto usb_ids = device.get_usb_ids();
				std::tie(arg->vendorID, arg->productID) = usb_ids;

				if (with_serial)
				{
					const auto arg_c000001c = vm::static_ptr_cast<lv2_file_c000001c>(_arg);
					const std::u16string serial = utf8_to_utf16(device.serial); // Serial needs to be encoded to utf-16 BE
					std::copy_n(serial.begin(), std::min(serial.size(), sizeof(arg_c000001c->serial) / sizeof(u16)), arg_c000001c->serial);
				}

				arg->out_code = CELL_OK;
				sys_fs.trace("sys_fs_fcntl(0x%08x): found device \"%s\" (vid=0x%04x, pid=0x%04x, serial=\"%s\")", op, mp.device, usb_ids.first, usb_ids.second, device.serial);
			}
		}

		return CELL_OK;
	}

	case 0xc0000016: // ps2disc_8160A811
	{
		break;
	}

	case 0xc000001a: // cellFsSetDiscReadRetrySetting, 5731DF45
	{
		[[maybe_unused]] const auto arg = vm::static_ptr_cast<lv2_file_c000001a>(_arg);
		return CELL_OK;
	}

	case 0xc0000021: // 9FDBBA89
	{
		break;
	}

	case 0xe0000000: // Unknown (cellFsGetBlockSize)
	{
		break;
	}

	case 0xe0000001: // Unknown (cellFsStat)
	{
		break;
	}

	case 0xe0000003: // Unknown
	{
		break;
	}

	case 0xe0000004: // Unknown
	{
		break;
	}

	case 0xe0000005: // Unknown (cellFsMkdir)
	{
		break;
	}

	case 0xe0000006: // Unknown
	{
		break;
	}

	case 0xe0000007: // Unknown
	{
		break;
	}

	case 0xe0000008: // Unknown (cellFsAclRead)
	{
		break;
	}

	case 0xe0000009: // Unknown (cellFsAccess)
	{
		break;
	}

	case 0xe000000a: // Unknown (E3D28395)
	{
		break;
	}

	case 0xe000000b: // Unknown (cellFsRename, FF29F478)
	{
		break;
	}

	case 0xe000000c: // Unknown (cellFsTruncate)
	{
		break;
	}

	case 0xe000000d: // Unknown (cellFsUtime)
	{
		break;
	}

	case 0xe000000e: // Unknown (cellFsAclWrite)
	{
		break;
	}

	case 0xe000000f: // Unknown (cellFsChmod)
	{
		break;
	}

	case 0xe0000010: // Unknown (cellFsChown)
	{
		break;
	}

	case 0xe0000011: // Unknown
	{
		break;
	}

	case 0xe0000012: // cellFsGetDirectoryEntries
	{
		lv2_obj::sleep(ppu);

		const auto arg = vm::static_ptr_cast<lv2_file_op_dir::dir_info>(_arg);

		if (_size < arg.size())
		{
			return CELL_EINVAL;
		}

		const auto directory = idm::get_unlocked<lv2_fs_object, lv2_dir>(fd);

		if (!directory)
		{
			return CELL_EBADF;
		}

		ppu.check_state();

		u32 read_count = 0;

		// NOTE: This function is actually capable of reading only one entry at a time
		if (const u32 max = arg->max)
		{
			const auto arg_ptr = +arg->ptr;

			if (auto* info = directory->dir_read())
			{
				auto& entry = arg_ptr[read_count++];

				s32 mode = info->is_directory ? CELL_FS_S_IFDIR | 0777 : CELL_FS_S_IFREG | 0666;

				if (directory->mp.read_only)
				{
					// Remove write permissions
					mode &= ~0222;
				}

				entry.attribute.mode = mode;
				entry.attribute.uid = directory->mp->flags & lv2_mp_flag::no_uid_gid ? -1 : 0;
				entry.attribute.gid = directory->mp->flags & lv2_mp_flag::no_uid_gid ? -1 : 0;
				entry.attribute.atime = info->atime;
				entry.attribute.mtime = info->mtime;
				entry.attribute.ctime = info->ctime;
				entry.attribute.size = info->size;
				entry.attribute.blksize = directory->mp->block_size;

				entry.entry_name.d_type = info->is_directory ? CELL_FS_TYPE_DIRECTORY : CELL_FS_TYPE_REGULAR;
				entry.entry_name.d_namlen = u8(std::min<usz>(info->name.size(), CELL_FS_MAX_FS_FILE_NAME_LENGTH));
				strcpy_trunc(entry.entry_name.d_name, info->name);
			}

			// Apparently all this function does to additional buffer elements is to zeroize them
			std::memset(arg_ptr.get_ptr() + read_count, 0, (max - read_count) * arg->ptr.size());
		}

		arg->_size = read_count;
		arg->_code = CELL_OK;
		return CELL_OK;
	}

	case 0xe0000015: // Unknown
	{
		break;
	}

	case 0xe0000016: // cellFsAllocateFileAreaWithInitialData
	{
		break;
	}

	case 0xe0000017: // cellFsAllocateFileAreaWithoutZeroFill
	{
		const auto arg = vm::static_ptr_cast<lv2_file_e0000017>(_arg);

		if (_size < arg->size || arg->_x4 != 0x10u || arg->_x8 != 0x20u)
		{
			return CELL_EINVAL;
		}

		arg->out_code = sys_fs_truncate(ppu, arg->file_path, arg->file_size);
		return CELL_OK;
	}

	case 0xe0000018: // cellFsChangeFileSizeWithoutAllocation
	{
		break;
	}

	case 0xe0000019: // Unknown
	{
		break;
	}

	case 0xe000001b: // Unknown
	{
		break;
	}

	case 0xe000001d: // Unknown
	{
		break;
	}

	case 0xe000001e: // Unknown
	{
		break;
	}

	case 0xe000001f: // Unknown
	{
		break;
	}

	case 0xe0000020: // Unknown
	{
		break;
	}

	case 0xe0000025: // cellFsSdataOpenWithVersion
	{
		const auto arg = vm::static_ptr_cast<lv2_file_e0000025>(_arg);

		if (arg->size != 0x30u)
		{
			sys_fs.error("sys_fs_fcntl(0xe0000025): invalid size (0x%x)", arg->size);
			break;
		}

		if (arg->_x4 != 0x10u || arg->_x8 != 0x28u)
		{
			sys_fs.error("sys_fs_fcntl(0xe0000025): invalid args (0x%x, 0x%x)", arg->_x4, arg->_x8);
			break;
		}

		std::string_view vpath{ arg->name.get_ptr(), arg->name_size };
		vpath = vpath.substr(0, vpath.find_first_of('\0'));

		sys_fs.notice("sys_fs_fcntl(0xe0000025): %s", vpath);

		be_t<u64> sdata_identifier = 0x18000000010;

		lv2_file::open_result_t result = lv2_file::open(vpath, 0, 0, &sdata_identifier, 8);

		if (result.error)
		{
			return result.error;
		}

		if (const u32 id = idm::import<lv2_fs_object, lv2_file>([&]() -> shared_ptr<lv2_file>
		{
			if (!g_fxo->get<loaded_npdrm_keys>().npdrm_fds.try_inc(16))
			{
				return null_ptr;
			}

			return stx::make_shared<lv2_file>(result.ppath, std::move(result.file), 0,  0, std::move(result.real_path), lv2_file_type::sdata);
		}))
		{
			arg->out_code = CELL_OK;
			arg->fd = id;
			return CELL_OK;
		}

		// Out of file descriptors
		return CELL_EMFILE;
	}
	}

	sys_fs.error("sys_fs_fcntl(): Unknown operation 0x%08x (fd=%d, arg=*0x%x, size=0x%x)", op, fd, _arg, _size);
	return CELL_OK;
}

error_code sys_fs_lseek(ppu_thread& ppu, u32 fd, s64 offset, s32 whence, vm::ptr<u64> pos)
{
	ppu.state += cpu_flag::wait;
	lv2_obj::sleep(ppu);

	sys_fs.trace("sys_fs_lseek(fd=%d, offset=0x%llx, whence=0x%x, pos=*0x%x)", fd, offset, whence, pos);

	const auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	std::unique_lock lock(file->mp->mutex);

	if (!file->file)
	{
		return CELL_EBADF;
	}

	if (whence + 0u >= 3)
	{
		return {CELL_EINVAL, whence};
	}

	const u64 result = file->file.seek(offset, static_cast<fs::seek_mode>(whence));

	if (result == umax)
	{
		switch (auto error = fs::g_tls_error)
		{
		case fs::error::inval: return {CELL_EINVAL, "fd=%u, offset=0x%x, whence=%d", fd, offset, whence};
		default: sys_fs.error("sys_fs_lseek(): unknown error %s", error);
		}

		return CELL_EIO; // ???
	}

	lock.unlock();
	ppu.check_state();

	*pos = result;
	return CELL_OK;
}

error_code sys_fs_fdatasync(ppu_thread& ppu, u32 fd)
{
	ppu.state += cpu_flag::wait;
	lv2_obj::sleep(ppu);

	sys_fs.trace("sys_fs_fdadasync(fd=%d)", fd);

	const auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(fd);

	if (!file || !(file->flags & CELL_FS_O_ACCMODE))
	{
		return CELL_EBADF;
	}

	std::lock_guard lock(file->mp->mutex);

	if (!file->file)
	{
		return CELL_EBADF;
	}

	file->file.sync();
	return CELL_OK;
}

error_code sys_fs_fsync(ppu_thread& ppu, u32 fd)
{
	ppu.state += cpu_flag::wait;
	lv2_obj::sleep(ppu);

	sys_fs.trace("sys_fs_fsync(fd=%d)", fd);

	const auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(fd);

	if (!file || !(file->flags & CELL_FS_O_ACCMODE))
	{
		return CELL_EBADF;
	}

	std::lock_guard lock(file->mp->mutex);

	if (!file->file)
	{
		return CELL_EBADF;
	}

	file->file.sync();
	return CELL_OK;
}

error_code sys_fs_fget_block_size(ppu_thread& ppu, u32 fd, vm::ptr<u64> sector_size, vm::ptr<u64> block_size, vm::ptr<u64> arg4, vm::ptr<s32> out_flags)
{
	ppu.state += cpu_flag::wait;

	sys_fs.warning("sys_fs_fget_block_size(fd=%d, sector_size=*0x%x, block_size=*0x%x, arg4=*0x%x, out_flags=*0x%x)", fd, sector_size, block_size, arg4, out_flags);

	const auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	static_cast<void>(ppu.test_stopped());

	// TODO
	*sector_size = file->mp->sector_size;
	*block_size = file->mp->block_size;
	*arg4 = file->mp->sector_size;
	*out_flags = file->flags;

	return CELL_OK;
}

error_code sys_fs_get_block_size(ppu_thread& ppu, vm::cptr<char> path, vm::ptr<u64> sector_size, vm::ptr<u64> block_size, vm::ptr<u64> arg4)
{
	ppu.state += cpu_flag::wait;

	sys_fs.warning("sys_fs_get_block_size(path=%s, sector_size=*0x%x, block_size=*0x%x, arg4=*0x%x)", path, sector_size, block_size, arg4);

	const auto [path_error, vpath] = translate_to_str(path);

	if (path_error)
	{
		return {path_error, vpath};
	}

	const std::string local_path = vfs::get(vpath);

	if (vpath.find_first_not_of('/') == umax)
	{
		return {CELL_EISDIR, path};
	}

	if (local_path.empty())
	{
		return {CELL_ENOTMOUNTED, path};
	}

	const auto& mp = g_fxo->get<lv2_fs_mount_info_map>().lookup(vpath);

	// It appears that /dev_hdd0 mount point is special in this function
	if (mp != &g_mp_sys_dev_hdd0 && (mp->flags & lv2_mp_flag::strict_get_block_size ? !fs::is_file(local_path) : !fs::exists(local_path)))
	{
		switch (auto error = fs::g_tls_error)
		{
		case fs::error::exist: return {CELL_EISDIR, path};
		case fs::error::noent: return {CELL_ENOENT, path};
		default: sys_fs.error("sys_fs_get_block_size(): unknown error %s", error);
		}

		return {CELL_EIO, path}; // ???
	}

	static_cast<void>(ppu.test_stopped());

	// TODO
	*sector_size = mp->sector_size;
	*block_size = mp->block_size;
	*arg4 = mp->sector_size;

	return CELL_OK;
}

error_code sys_fs_truncate(ppu_thread& ppu, vm::cptr<char> path, u64 size)
{
	ppu.state += cpu_flag::wait;
	lv2_obj::sleep(ppu);

	sys_fs.warning("sys_fs_truncate(path=%s, size=0x%llx)", path, size);

	const auto [path_error, vpath] = translate_to_str(path);

	if (path_error)
	{
		return {path_error, vpath};
	}

	const std::string local_path = vfs::get(vpath);

	const auto& mp = g_fxo->get<lv2_fs_mount_info_map>().lookup(vpath);

	if (mp == &g_mp_sys_dev_root)
	{
		return {CELL_EISDIR, path};
	}

	if (local_path.empty())
	{
		return {CELL_ENOTMOUNTED, path};
	}

	if (mp.read_only)
	{
		return {CELL_EROFS, path};
	}

	std::lock_guard lock(mp->mutex);

	if (!fs::truncate_file(local_path, size))
	{
		switch (auto error = fs::g_tls_error)
		{
		case fs::error::noent:
		{
			return {mp == &g_mp_sys_dev_hdd1 ? sys_fs.warning : sys_fs.error, CELL_ENOENT, path};
		}
		default: sys_fs.error("sys_fs_truncate(): unknown error %s", error);
		}

		return {CELL_EIO, path}; // ???
	}

	return CELL_OK;
}

error_code sys_fs_ftruncate(ppu_thread& ppu, u32 fd, u64 size)
{
	ppu.state += cpu_flag::wait;
	lv2_obj::sleep(ppu);

	sys_fs.warning("sys_fs_ftruncate(fd=%d, size=0x%llx)", fd, size);

	const auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(fd);

	if (!file || !(file->flags & CELL_FS_O_ACCMODE))
	{
		return CELL_EBADF;
	}

	if (file->mp.read_only)
	{
		return CELL_EROFS;
	}

	std::lock_guard lock(file->mp->mutex);

	if (!file->file)
	{
		return CELL_EBADF;
	}

	if (file->lock == 2)
	{
		return CELL_EIO;
	}

	if (file->lock)
	{
		return CELL_EBUSY;
	}

	if (!file->file.trunc(size))
	{
		switch (auto error = fs::g_tls_error)
		{
		case fs::error::ok:
		default: sys_fs.error("sys_fs_ftruncate(): unknown error %s", error);
		}

		return CELL_EIO; // ???
	}

	return CELL_OK;
}

error_code sys_fs_symbolic_link(ppu_thread&, vm::cptr<char> target, vm::cptr<char> linkpath)
{
	sys_fs.todo("sys_fs_symbolic_link(target=%s, linkpath=%s)", target, linkpath);

	return CELL_OK;
}

error_code sys_fs_chmod(ppu_thread&, vm::cptr<char> path, s32 mode)
{
	sys_fs.todo("sys_fs_chmod(path=%s, mode=%#o)", path, mode);

	const auto [path_error, vpath] = translate_to_str(path);

	if (path_error)
	{
		return {path_error, vpath};
	}

	const std::string local_path = vfs::get(vpath);

	const auto mp = lv2_fs_object::get_mp(vpath);

	if (local_path.empty())
	{
		return {CELL_ENOTMOUNTED, path};
	}

	if (mp->flags & lv2_mp_flag::read_only)
	{
		return {CELL_EROFS, path};
	}

	std::unique_lock lock(mp->mutex);

	fs::stat_t info{};

	if (!fs::get_stat(local_path, info))
	{
		switch (auto error = fs::g_tls_error)
		{
		case fs::error::noent:
		{
			// Try to locate split files

			for (u32 i = 66601; i <= 66699; i++)
			{
				if (!fs::get_stat(fmt::format("%s.%u", local_path, i), info) && !info.is_directory)
				{
					break;
				}
			}

			if (fs::get_stat(local_path + ".66600", info) && !info.is_directory)
			{
				break;
			}

			return {CELL_ENOENT, path};
		}
		default:
		{
			sys_fs.error("sys_fs_chmod(): unknown error %s", error);
			return {CELL_EIO, path};
		}
		}
	}

	return CELL_OK;
}

error_code sys_fs_chown(ppu_thread&, vm::cptr<char> path, s32 uid, s32 gid)
{
	sys_fs.todo("sys_fs_chown(path=%s, uid=%d, gid=%d)", path, uid, gid);

	return CELL_OK;
}

error_code sys_fs_disk_free(ppu_thread& ppu, vm::cptr<char> path, vm::ptr<u64> total_free, vm::ptr<u64> avail_free)
{
	ppu.state += cpu_flag::wait;
	lv2_obj::sleep(ppu);

	sys_fs.warning("sys_fs_disk_free(path=%s total_free=*0x%x avail_free=*0x%x)", path, total_free, avail_free);

	if (!path)
		return CELL_EFAULT;

	if (!path[0])
		return CELL_EINVAL;

	const std::string_view vpath = path.get_ptr();

	if (vpath == "/"sv)
	{
		return CELL_ENOTSUP;
	}

	// It seems max length is 31, and multiple / at the start aren't supported
	if (vpath.size() > CELL_FS_MAX_MP_LENGTH)
	{
		return {CELL_ENAMETOOLONG, path};
	}

	if (vpath.find_first_not_of('/') != 1)
	{
		return {CELL_EINVAL, path};
	}

	// Get only device path
	const std::string local_path = vfs::get(vpath.substr(0, vpath.find_first_of('/', 1)));

	if (local_path.empty())
	{
		return {CELL_EINVAL, path};
	}

	const auto& mp = g_fxo->get<lv2_fs_mount_info_map>().lookup(vpath);

	if (mp->flags & lv2_mp_flag::strict_get_block_size)
	{
		// TODO:
		return {CELL_ENOTSUP, path};
	}

	if (mp.read_only)
	{
		// TODO: check /dev_bdvd
		ppu.check_state();
		*total_free = 0;
		*avail_free = 0;
		return CELL_OK;
	}

	u64 available = 0;

	// avail_free is the only value used by cellFsGetFreeSize
	if (mp == &g_mp_sys_dev_hdd1)
	{
		available = (1u << 31) - mp->sector_size; // 2GB (TODO: Should be the total size)
	}
	else //if (mp == &g_mp_sys_dev_hdd0)
	{
		available = (40ull * 1024 * 1024 * 1024 - mp->sector_size); // Read explanation in cellHddGameCheck
	}

	// HACK: Hopefully nothing uses this value or once at max because its hacked here:
	// The total size can change based on the size of the directory
	const u64 total = available + fs::get_dir_size(local_path, mp->sector_size);

	ppu.check_state();
	*total_free = total;
	*avail_free = available;

	return CELL_OK;
}

error_code sys_fs_utime(ppu_thread& ppu, vm::cptr<char> path, vm::cptr<CellFsUtimbuf> timep)
{
	ppu.state += cpu_flag::wait;
	lv2_obj::sleep(ppu);

	sys_fs.warning("sys_fs_utime(path=%s, timep=*0x%x)", path, timep);
	sys_fs.warning("** actime=%u, modtime=%u", timep->actime, timep->modtime);

	const auto [path_error, vpath] = translate_to_str(path);

	if (path_error)
	{
		return {path_error, vpath};
	}

	const std::string local_path = vfs::get(vpath);

	const auto& mp = g_fxo->get<lv2_fs_mount_info_map>().lookup(vpath);

	if (mp == &g_mp_sys_dev_root)
	{
		return {CELL_EISDIR, path};
	}

	if (local_path.empty())
	{
		return {CELL_ENOTMOUNTED, path};
	}

	if (mp.read_only)
	{
		return {CELL_EROFS, path};
	}

	std::lock_guard lock(mp->mutex);

	if (!fs::utime(local_path, timep->actime, timep->modtime))
	{
		switch (auto error = fs::g_tls_error)
		{
		case fs::error::noent:
		{
			return {mp == &g_mp_sys_dev_hdd1 ? sys_fs.warning : sys_fs.error, CELL_ENOENT, path};
		}
		default: sys_fs.error("sys_fs_utime(): unknown error %s", error);
		}

		return {CELL_EIO, path}; // ???
	}

	return CELL_OK;
}

error_code sys_fs_acl_read(ppu_thread&, vm::cptr<char> path, vm::ptr<void> ptr)
{
	sys_fs.todo("sys_fs_acl_read(path=%s, ptr=*0x%x)", path, ptr);

	return CELL_OK;
}

error_code sys_fs_acl_write(ppu_thread&, vm::cptr<char> path, vm::ptr<void> ptr)
{
	sys_fs.todo("sys_fs_acl_write(path=%s, ptr=*0x%x)", path, ptr);

	return CELL_OK;
}

error_code sys_fs_lsn_get_cda_size(ppu_thread&, u32 fd, vm::ptr<u64> ptr)
{
	sys_fs.warning("sys_fs_lsn_get_cda_size(fd=%d, ptr=*0x%x)", fd, ptr);

	const auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	// TODO
	*ptr = 0;
	return CELL_OK;
}

error_code sys_fs_lsn_get_cda(ppu_thread&, u32 fd, vm::ptr<void> arg2, u64 arg3, vm::ptr<u64> arg4)
{
	sys_fs.todo("sys_fs_lsn_get_cda(fd=%d, arg2=*0x%x, arg3=0x%x, arg4=*0x%x)", fd, arg2, arg3, arg4);

	return CELL_OK;
}

error_code sys_fs_lsn_lock(ppu_thread&, u32 fd)
{
	sys_fs.trace("sys_fs_lsn_lock(fd=%d)", fd);

	const auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	// TODO: seems to do nothing on /dev_hdd0 or /host_root
	if (file->mp == &g_mp_sys_dev_hdd0 || file->mp->flags & lv2_mp_flag::strict_get_block_size)
	{
		return CELL_OK;
	}

	std::lock_guard lock(file->mp->mutex);
	file->lock.compare_and_swap(0, 1);
	return CELL_OK;
}

error_code sys_fs_lsn_unlock(ppu_thread&, u32 fd)
{
	sys_fs.trace("sys_fs_lsn_unlock(fd=%d)", fd);

	const auto file = idm::get_unlocked<lv2_fs_object, lv2_file>(fd);

	if (!file)
	{
		return CELL_EBADF;
	}

	// See sys_fs_lsn_lock
	if (file->mp == &g_mp_sys_dev_hdd0 || file->mp->flags & lv2_mp_flag::strict_get_block_size)
	{
		return CELL_OK;
	}

	// Unlock unconditionally
	std::lock_guard lock(file->mp->mutex);
	file->lock.compare_and_swap(1, 0);
	return CELL_OK;
}

error_code sys_fs_lsn_read(ppu_thread&, u32 fd, vm::cptr<void> ptr, u64 size)
{
	sys_fs.todo("sys_fs_lsn_read(fd=%d, ptr=*0x%x, size=0x%x)", fd, ptr, size);

	return CELL_OK;
}

error_code sys_fs_lsn_write(ppu_thread&, u32 fd, vm::cptr<void> ptr, u64 size)
{
	sys_fs.todo("sys_fs_lsn_write(fd=%d, ptr=*0x%x, size=0x%x)", fd, ptr, size);

	return CELL_OK;
}

error_code sys_fs_mapped_allocate(ppu_thread&, u32 fd, u64 size, vm::pptr<void> out_ptr)
{
	sys_fs.todo("sys_fs_mapped_allocate(fd=%d, arg2=0x%x, out_ptr=**0x%x)", fd, size, out_ptr);

	return CELL_OK;
}

error_code sys_fs_mapped_free(ppu_thread&, u32 fd, vm::ptr<void> ptr)
{
	sys_fs.todo("sys_fs_mapped_free(fd=%d, ptr=0x%#x)", fd, ptr);

	return CELL_OK;
}

error_code sys_fs_truncate2(ppu_thread&, u32 fd, u64 size)
{
	sys_fs.todo("sys_fs_truncate2(fd=%d, size=0x%x)", fd, size);

	return CELL_OK;
}

error_code sys_fs_get_mount_info_size(ppu_thread&, vm::ptr<u64> len)
{
	sys_fs.warning("sys_fs_get_mount_info_size(len=*0x%x)", len);

	if (!len)
	{
		return CELL_EFAULT;
	}

	*len = g_fxo->get<lv2_fs_mount_info_map>().get_all();

	return CELL_OK;
}

error_code sys_fs_get_mount_info(ppu_thread&, vm::ptr<CellFsMountInfo> info, u64 len, vm::ptr<u64> out_len)
{
	sys_fs.warning("sys_fs_get_mount_info(info=*0x%x, len=0x%x, out_len=*0x%x)", info, len, out_len);

	if (!info || !out_len)
	{
		return CELL_EFAULT;
	}

	*out_len = g_fxo->get<lv2_fs_mount_info_map>().get_all(info.get_ptr(), len);

	return CELL_OK;
}

error_code sys_fs_newfs(ppu_thread& ppu, vm::cptr<char> dev_name, vm::cptr<char> file_system, s32 unk1, vm::cptr<char> str1)
{
	ppu.state += cpu_flag::wait;

	sys_fs.warning("sys_fs_newfs(dev_name=%s, file_system=%s, unk1=0x%x, str1=%s)", dev_name, file_system, unk1, str1);

	const auto [dev_error, device_name] = translate_to_str(dev_name, false);

	if (dev_error)
	{
		return {dev_error, device_name};
	}

	std::string vfs_path;
	const auto mp = lv2_fs_object::get_mp(device_name, &vfs_path);
	std::unique_lock lock(mp->mutex, std::defer_lock);

	if (!g_ps3_process_info.has_root_perm() && mp != &g_mp_sys_dev_usb)
		return {CELL_EPERM, device_name};

	if (mp == &g_mp_sys_no_device)
		return {CELL_ENXIO, device_name};

	if (g_fxo->get<lv2_fs_mount_info_map>().is_device_mounted(device_name) || !lock.try_lock())
		return {CELL_EBUSY, device_name};

	if (vfs_path.empty())
		return {CELL_ENOTSUP, device_name};

	if (mp->flags & lv2_mp_flag::read_only)
		return {CELL_EROFS, device_name};

	if (mp == &g_mp_sys_dev_hdd1)
	{
		const std::string_view appname = g_ps3_process_info.get_cellos_appname();
		vfs_path = fmt::format("%s/caches/%s", vfs_path, appname.substr(0, appname.find_last_of('.')));
	}

	if (!fs::remove_all(vfs_path, false))
	{
		sys_fs.error("sys_fs_newfs(): Failed to clear \"%s\" at \"%s\"", device_name, vfs_path);
		return {CELL_EIO, vfs_path};
	}

	sys_fs.success("sys_fs_newfs(): Successfully cleared \"%s\" at \"%s\"", device_name, vfs_path);
	return CELL_OK;
}

error_code sys_fs_mount(ppu_thread& ppu, vm::cptr<char> dev_name, vm::cptr<char> file_system, vm::cptr<char> path, s32 unk1, s32 prot, s32 unk2, vm::cptr<char> str1, u32 str_len)
{
	ppu.state += cpu_flag::wait;

	sys_fs.warning("sys_fs_mount(dev_name=%s, file_system=%s, path=%s, unk1=0x%x, prot=%d, unk3=0x%x, str1=%s, str_len=%d)", dev_name, file_system, path, unk1, prot, unk2, str1, str_len);

	const auto [dev_error, device_name] = translate_to_str(dev_name, false);

	if (dev_error)
	{
		return {dev_error, device_name};
	}

	const auto [fs_error, filesystem] = translate_to_str(file_system, false);

	if (fs_error)
	{
		return {fs_error, filesystem};
	}

	const auto [path_error, path_sv] = translate_to_str(path);

	if (path_error)
	{
		return {path_error, path_sv};
	}

	const std::string vpath = lv2_fs_object::get_normalized_path(path_sv);

	std::string vfs_path;
	const auto mp = lv2_fs_object::get_mp(device_name, &vfs_path);
	std::unique_lock lock(mp->mutex, std::defer_lock);

	if (!g_ps3_process_info.has_root_perm() && mp != &g_mp_sys_dev_usb)
		return {CELL_EPERM, device_name};

	if (mp == &g_mp_sys_no_device)
		return {CELL_ENXIO, device_name};

	if (g_fxo->get<lv2_fs_mount_info_map>().is_device_mounted(device_name) || !lock.try_lock())
		return {CELL_EBUSY, device_name};

	if (vfs_path.empty())
		return {CELL_ENOTSUP, device_name};

	if (vpath.find_first_not_of('/') == umax || !vfs::get(vpath).empty())
		return {CELL_EEXIST, vpath};

	if (mp == &g_mp_sys_dev_hdd1)
	{
		const std::string_view appname = g_ps3_process_info.get_cellos_appname();
		vfs_path = fmt::format("%s/caches/%s", vfs_path, appname.substr(0, appname.find_last_of('.')));
	}

	if (!vfs_path.ends_with('/'))
		vfs_path += '/';

	if (!fs::is_dir(vfs_path) && !fs::create_dir(vfs_path))
	{
		sys_fs.error("Failed to create directory \"%s\"", vfs_path);
		return {CELL_EIO, vfs_path};
	}

	const bool is_simplefs = filesystem == "CELL_FS_SIMPLEFS"sv;

	if (is_simplefs)
	{
		vfs_path += "simplefs.tmp";
		if (fs::file simplefs_file; simplefs_file.open(vfs_path, fs::create + fs::read + fs::write + fs::trunc + fs::lock))
		{
			const u64 file_size = mp->sector_size; // One sector's size is enough for VSH's simplefs check
			simplefs_file.trunc(file_size);
			sys_fs.notice("Created a simplefs file at \"%s\"", vfs_path);
		}
		else
		{
			sys_fs.error("Failed to create simplefs file \"%s\"", vfs_path);
			return {CELL_EIO, vfs_path};
		}
	}

	if (!vfs::mount(vpath, vfs_path, !is_simplefs))
	{
		if (is_simplefs)
		{
			if (fs::remove_file(vfs_path))
			{
				sys_fs.notice("Removed simplefs file \"%s\"", vfs_path);
			}
			else
			{
				sys_fs.error("Failed to remove simplefs file \"%s\"", vfs_path);
			}
		}

		return CELL_EIO;
	}

	g_fxo->get<lv2_fs_mount_info_map>().add(vpath, mp, device_name, filesystem, prot);

	return CELL_OK;
}

error_code sys_fs_unmount(ppu_thread& ppu, vm::cptr<char> path, s32 unk1, s32 force)
{
	ppu.state += cpu_flag::wait;

	sys_fs.warning("sys_fs_unmount(path=%s, unk1=0x%x, force=%d)", path, unk1, force);

	const auto [path_error, vpath] = translate_to_str(path);

	if (path_error)
	{
		return {path_error, vpath};
	}

	const auto& mp = g_fxo->get<lv2_fs_mount_info_map>().lookup(vpath);
	std::unique_lock lock(mp->mutex, std::defer_lock);

	if (!g_ps3_process_info.has_root_perm() && mp != &g_mp_sys_dev_usb)
		return {CELL_EPERM, vpath};

	if (mp == &g_mp_sys_no_device)
		return {CELL_EINVAL, vpath};

	if (mp == &g_mp_sys_dev_root || (!lock.try_lock() && !force))
		return {CELL_EBUSY, vpath};

	if (!lv2_fs_mount_info_map::vfs_unmount(vpath))
		return {CELL_EIO, vpath};

	return CELL_OK;
}
