#include "stdafx.h"
#include "util/types.hpp"
#include "util/logs.hpp"
#include "util/v128.hpp"
#include "util/simd.hpp"
#include "Utilities/File.h"
#include "Utilities/StrFmt.h"
#include "system_config.h"
#include "savestate_utils.hpp"

#include "System.h"

#include <set>
#include <span>

LOG_CHANNEL(sys_log, "SYS");

struct serial_ver_t
{
	std::string ver_name;
	bool used = false;
	u16 current_version = 0;
	std::set<u16> compatible_versions;
};

static std::array<serial_ver_t, 27> s_serial_versions;

#define SERIALIZATION_VER(name, identifier, ...) \
\
	const bool s_##name##_serialization_fill = []() { auto& e = ::s_serial_versions[identifier]; if (e.compatible_versions.empty()) { e.compatible_versions = {__VA_ARGS__}; e.ver_name = #name; } return true; }();\
\
	extern void using_##name##_serialization()\
	{\
		ensure(Emu.IsStopped());\
		::s_serial_versions[identifier].used = true;\
	}\
\
	extern s32 get_##name##_serialization_version()\
	{\
		return ::s_serial_versions[identifier].current_version;\
	}

SERIALIZATION_VER(global_version, 0,                            19) // For stuff not listed here
SERIALIZATION_VER(ppu, 1,                                       1, 2/*PPU sleep order*/, 3/*PPU FNID and module*/)
SERIALIZATION_VER(spu, 2,                                       1)
SERIALIZATION_VER(lv2_sync, 3,                                  1)
SERIALIZATION_VER(lv2_vm, 4,                                    1)
SERIALIZATION_VER(lv2_net, 5,                                   1, 2/*TCP Feign conection loss*/)
SERIALIZATION_VER(lv2_fs, 6,                                    1, 2/*NPDRM key saving*/)
SERIALIZATION_VER(lv2_prx_overlay, 7,                           1)
SERIALIZATION_VER(lv2_memory, 8,                                1)
SERIALIZATION_VER(lv2_config, 9,                                1)

namespace rsx
{
	SERIALIZATION_VER(rsx, 10,                                  1, 2/*Pending flip*/, 3/*avconf scan_mode*/)
}

namespace np
{
	SERIALIZATION_VER(sceNp, 11,                                1)
}

#ifdef _MSC_VER
// Compiler bug, lambda function body does seem to inherit used namespace atleast for function declaration
SERIALIZATION_VER(rsx, 10)
SERIALIZATION_VER(sceNp, 11)
#endif

SERIALIZATION_VER(cellVdec, 12,                                 1)
SERIALIZATION_VER(cellAudio, 13,                                1)
SERIALIZATION_VER(cellCamera, 14,                               1, 2/*gem_camera_shared*/)
SERIALIZATION_VER(cellGem, 15,                                  1, 2/*calibration_status_flags*/, 3/*video_conversion*/)
SERIALIZATION_VER(sceNpTrophy, 16,                              1)
SERIALIZATION_VER(cellMusic, 17,                                1)
SERIALIZATION_VER(cellVoice, 18,                                1)
SERIALIZATION_VER(cellGcm, 19,                                  1)
SERIALIZATION_VER(sysPrxForUser, 20,                            1)
SERIALIZATION_VER(cellSaveData, 21,                             1)
SERIALIZATION_VER(cellAudioOut, 22,                             1)
SERIALIZATION_VER(sys_io, 23,                                   2)

// Misc versions for HLE/LLE not included so main version would not invalidated
SERIALIZATION_VER(LLE, 24,                                      1)
SERIALIZATION_VER(HLE, 25,                                      1)

SERIALIZATION_VER(cellSysutil, 26,                              1, 2/*AVC2 Muting,Volume*/)

template <>
void fmt_class_string<std::remove_cvref_t<decltype(s_serial_versions)>>::format(std::string& out, u64 arg)
{
	bool is_first = true;

	const auto& serials = get_object(arg);

	out += "{ ";

	for (auto& entry : serials)
	{
		if (entry.current_version)
		{
			if (!is_first)
			{
				out += ", ";
			}

			is_first = false;
			fmt::append(out, "%s=%d", entry.ver_name, entry.current_version);
		}
	}

	out += " }";
}

std::vector<version_entry> get_savestate_versioning_data(fs::file&& file, std::string_view filepath)
{
	if (!file)
	{
		return {};
	}

	file.seek(0);

	utils::serial ar;
	ar.set_reading_state({}, true);

	if (filepath.ends_with(".zst"))
	{
		ar.m_file_handler = make_compressed_zstd_serialization_file_handler(std::move(file));
	}
	else if (filepath.ends_with(".gz"))
	{
		ar.m_file_handler = make_compressed_serialization_file_handler(std::move(file));
	}
	else
	{
		ar.m_file_handler = make_uncompressed_serialization_file_handler(std::move(file));
	}

	if (u64 r = 0; ar.try_read(r) != 0 || r != "RPCS3SAV"_u64)
	{
		return {};
	}

	ar.pos = 10;

	u64 offs = ar.try_read<u64>().second;

	const usz fsize = ar.get_size(offs);

	if (!offs || fsize <= offs)
	{
		return {};
	}

	ar.seek_pos(offs);
	ar.breathe(true);

	std::vector<version_entry> ver_data = ar.pop<std::vector<version_entry>>();
	return ver_data;
}

std::shared_ptr<utils::serial> make_savestate_reader(const std::string& path)
{
	std::shared_ptr<utils::serial> ar;

	fs::file save{path, fs::isfile + fs::read};

	if (!save)
	{
		return ar;
	}

	if (path.ends_with(".SAVESTAT") && save.size() >= 8 && save.read<u64>() == "RPCS3SAV"_u64)
	{
		ar = std::make_shared<utils::serial>();
		ar->set_reading_state();

		ar->m_file_handler = make_uncompressed_serialization_file_handler(std::move(save));
	}
	else if (path.ends_with(".zst"))
	{
		ar = std::make_shared<utils::serial>();
		ar->set_reading_state();

		ar->m_file_handler = make_compressed_zstd_serialization_file_handler(std::move(save));

		if (ar->try_read<u64>().second != "RPCS3SAV"_u64)
		{
			ar.reset();
		}
		else
		{
			ar->pos = 0;
		}
	}
	else if (path.ends_with(".gz"))
	{
		ar = std::make_shared<utils::serial>();
		ar->set_reading_state();

		ar->m_file_handler = make_compressed_serialization_file_handler(std::move(save));

		if (ar->try_read<u64>().second != "RPCS3SAV"_u64)
		{
			ar.reset();
		}
		else
		{
			ar->pos = 0;
		}
	}

	return ar;
}

bool is_savestate_version_compatible(const std::vector<version_entry>& data, bool is_boot_check)
{
	if (data.empty())
	{
		return false;
	}

	bool ok = true;

	if (is_boot_check)
	{
		for (auto& entry : s_serial_versions)
		{
			// Version 0 means that the entire constructor using the version should be skipped
			entry.current_version = 0;
		}
	}

	auto& channel = (is_boot_check ? sys_log.error : sys_log.trace);

	for (const auto& entry : data)
	{
		if (entry.type >= s_serial_versions.size())
		{
			channel("Savestate version identifier is unknown! (category=%u, version=%u)", entry.type, entry.version);
			ok = false; // Log all mismatches
		}
		else if (!s_serial_versions[entry.type].compatible_versions.count(entry.version))
		{
			channel("Savestate version is not supported. (category=%u, version=%u)", entry.type, entry.version);
			ok = false;
		}
		else if (is_boot_check)
		{
			s_serial_versions[entry.type].current_version = entry.version;
		}
	}

	if (is_boot_check)
	{
		if (ok)
		{
			sys_log.success("Savestate versions: %s", s_serial_versions);
		}
		else
		{
			for (auto& entry : s_serial_versions)
			{
				entry.current_version = 0;
			}
		}
	}

	return ok;
}

std::string get_savestate_file(std::string_view title_id, std::string_view boot_path, s64 rel_id, u64 aggregate_file_size)
{
	const std::string title = std::string{title_id.empty() ? boot_path.substr(boot_path.find_last_of(fs::delim) + 1) : title_id};

	// Internal functionality ATM
	constexpr s64 abs_id = 0;

	if (aggregate_file_size == umax && rel_id == -1)
	{
		// Return directory
		return fs::get_config_dir() + "savestates/" + title + "/";
	}

	if (rel_id >= 0)
	{
		std::string dir_path = fs::get_config_dir() + "savestates/" + title + "/";

		fs::dir dir_view{dir_path};

		std::map<std::string, usz, std::greater<>> save_files;

		for (auto&& dir_entry : dir_view)
		{
			if (dir_entry.is_directory || dir_entry.size <= 1024)
			{
				continue;
			}

			const std::string& entry = dir_entry.name;

			if (!title_id.empty() && !entry.starts_with(title + "_"))
			{
				// Check prefix only for certified applications
				// Because ELF file names can be long and unhelpful
				continue;
			}

			if (entry.ends_with(".SAVESTAT.zst") || entry.ends_with(".SAVESTAT.gz") || entry.ends_with(".SAVESTAT"))
			{
				if (usz dot_idx = entry.rfind(".SAVESTAT"); dot_idx && dot_idx != umax)
				{
					if (usz uc_pos = entry.rfind("_", dot_idx - 1); uc_pos != umax && uc_pos + 1 < dot_idx)
					{
						if (std::all_of(entry.begin() + uc_pos + 1, entry.begin() + dot_idx, [](char c) { return c >= '0' && c <= '9'; }))
						{
							save_files.emplace(entry, dir_entry.size);	
						}
					}
				}
			}
		}

		std::string rel_path;
		std::string size_based_path;

		auto prepare_return_value = [](std::string& dir_path, std::string& filename) -> std::string&&
		{
			dir_path.append(filename);
			filename = std::move(dir_path);
			return std::move(filename);
		};

		if (rel_id > 0)
		{
			if (static_cast<usz>(rel_id - 1) < save_files.size())
			{
				rel_path = std::next(save_files.begin(), rel_id - 1)->first;
			}
		}

		if (aggregate_file_size != umax)
		{
			usz size_sum = 0;

			for (auto&& [path, size] : save_files)
			{
				if (size_sum >= aggregate_file_size)
				{
					size_based_path = path;
					break;
				}

				size_sum += size;
			}
		}

		if (!rel_path.empty() || !size_based_path.empty())
		{
			if (rel_path > size_based_path)
			{
				return prepare_return_value(dir_path, rel_path);
			}
			else
			{
				return prepare_return_value(dir_path, size_based_path);
			}
		}

		if (rel_id > 0 || aggregate_file_size != umax)
		{
			return {};
		}

		// Increment number in string in reverse
		// Return index of new character if appended a character, umax otherwise
		auto increment_string = [](std::string& out, usz pos) -> usz
		{
			while (pos != umax && out[pos] == '9')
			{
				out[pos] = '0';
				pos--;
			}

			if (pos == umax || (out[pos] < '0' || out[pos] > '9'))
			{
				out.insert(out.begin() + (pos + 1), '1');
				return pos + 1;
			}

			out[pos]++;
			return umax;
		};

		if (!save_files.empty())
		{
			std::string last_entry = save_files.begin()->first;

			// Increment entry ID
			if (usz inc_pos = increment_string(last_entry, last_entry.rfind(".SAVESTAT") - 1); inc_pos != umax)
			{
				// Increment entry suffix - ID has become wider in length (keeps the string in alphbetic ordering)
				ensure(inc_pos >= 2);
				ensure(last_entry[inc_pos - 2]++ != 'z');
			}

			return prepare_return_value(dir_path, last_entry);
		}

		// Fallback - create new file
	}

	const std::string save_id = fmt::format("%d", abs_id);

	// Make sure that savestate file with higher IDs are placed at the bottom of "by name" file ordering in directory view by adding a single character prefix which tells the ID length
	// While not needing to keep a 59 chars long suffix at all times for this purpose
	const char prefix = ::at32("0123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz"sv, save_id.size());

	std::string path = fs::get_config_dir() + "/savestates/" + title + "/" + title + '_' + prefix + '_' + save_id + ".SAVESTAT";

	if (std::string path_compressed = path + ".zst"; fs::is_file(path_compressed))
	{
		return path_compressed;
	}

	if (std::string path_compressed = path + ".gz"; fs::is_file(path_compressed))
	{
		return path_compressed;
	}

	return path;
}

bool is_savestate_compatible(fs::file&& file, std::string_view filepath)
{
	return is_savestate_version_compatible(get_savestate_versioning_data(std::move(file), filepath), false);
}

bool is_savestate_compatible(const std::string& filepath)
{
	if (fs::file file{filepath, fs::isfile + fs::read})
	{
		return is_savestate_compatible(std::move(file), filepath);
	}
	return false;
}

std::vector<version_entry> read_used_savestate_versions()
{
	std::vector<version_entry> used_serial;
	used_serial.reserve(s_serial_versions.size());

	for (serial_ver_t& ver : s_serial_versions)
	{
		if (std::exchange(ver.used, false))
		{
			used_serial.push_back(version_entry{static_cast<u16>(&ver - s_serial_versions.data()), *ver.compatible_versions.rbegin()});
		}

		ver.current_version = 0;
	}

	return used_serial;
}

bool boot_current_game_savestate(bool testing, u32 index)
{
	if (index == 0 || index > g_cfg.savestate.max_files.get())
	{
		return false;
	}

	if (!g_cfg.savestate.suspend_emu && !Emu.GetTitleID().empty() && (Emu.IsRunning() || Emu.GetStatus() == system_state::paused))
	{
		const std::string savestate_path = get_savestate_file(Emu.GetTitleID(), Emu.GetBoot(), index);

		const bool result = fs::is_file(savestate_path);

		if (testing)
		{
			sys_log.trace("boot_current_game_savestate(true, %d) returned %s.", index, result);
			return result;
		}

		if (result)
		{
			sys_log.success("Booting the most recent savestate \'%s\' using the Reload shortcut.", savestate_path);

			// Make sure we keep the game window opened
			Emu.SetContinuousMode(true);
			Emu.GracefulShutdown(false, false, false, true);

			if (game_boot_result error = Emu.BootGame(savestate_path, "", true); error != game_boot_result::no_errors)
			{
				sys_log.error("Failed to boot savestate \'%s\' using the Reload shortcut. (error: %s)", savestate_path, error);
			}
			else
			{
				return true;
			}
		}

		sys_log.error("No compatible savestate file found in \'%s\''", get_savestate_file(Emu.GetTitleID(), Emu.GetBoot(), -1));
	}

	return false;
}

void clean_savestates(std::string_view title_id, std::string_view boot_path, usz max_files, usz max_files_size)
{
	ensure(max_files && max_files != umax);

	bool logged_limits = false;

	while (true)
	{
		const std::string to_remove = get_savestate_file(title_id, boot_path, max_files + 1, max_files_size == 0 ? u64{umax} : max_files_size);

		if (to_remove.empty())
		{
			break;
		}	

		if (!fs::remove_file(to_remove))
		{
			sys_log.error("Failed to remove savestate file at '%s'! (error: %s)", to_remove, fs::g_tls_error);
			break;
		}
		else
		{
			if (!logged_limits)
			{
				sys_log.success("Maximum save state files set: %d.\nMaximum save state disk space set: %d (MiB).\nRemoved old savestate file at '%s'.\n"
					, max_files, max_files_size, to_remove);
				logged_limits = true;
			}
			else
			{
				sys_log.success("Removed old savestate file at '%s'.", to_remove);
			}
		}
	}
}

bool load_and_check_reserved(utils::serial& ar, usz size)
{
	u8 bytes[4096];
	ensure(size < std::size(bytes));
	std::memset(&bytes[size & (0 - sizeof(v128))], 0, sizeof(v128));

	const usz old_pos = ar.pos;
	ar(std::span<u8>(bytes, size));

	// Check if all are 0
	for (usz i = 0; i < size; i += sizeof(v128))
	{
		if (v128::loadu(&bytes[i]) != v128{})
		{
			return false;
		}
	}

	return old_pos + size == ar.pos;
}

namespace stx
{
	extern u16 serial_breathe_and_tag(utils::serial& ar, std::string_view name, bool tag_bit)
	{
		thread_local std::string_view s_tls_object_name = "none";
		thread_local u64 s_tls_call_count = 1, s_tls_current_pos = 0;

		if (s_tls_current_pos >= ar.pos)
		{
			// Reset, probably a new utils::serial object
			s_tls_call_count = 1;
			s_tls_object_name = "none"sv;
		}

		s_tls_current_pos = ar.pos;

		constexpr u16 data_mask = 0x7fff;

		if (ar.m_file_handler && ar.m_file_handler->is_null())
		{
			return (tag_bit ? data_mask + 1 : 0);
		}

		u16 tag = static_cast<u16>((static_cast<u16>(ar.pos / 2) & data_mask) | (tag_bit ? data_mask + 1 : 0));
		u16 saved = tag;
		ar(saved);

		sys_log.trace("serial_breathe_and_tag(%u): %s, object: '%s', next-object: '%s', expected/tag: 0x%x == 0x%x", s_tls_call_count, ar, s_tls_object_name, name, tag, saved);

		if ((saved ^ tag) & data_mask)
		{
			ensure(!ar.is_writing());
			fmt::throw_exception("serial_breathe_and_tag(%u): %s\nobject: '%s', next-object: '%s', expected/tag: 0x%x != 0x%x,", s_tls_call_count, ar, s_tls_object_name, name, tag, saved);
		}

		s_tls_object_name = name;
		s_tls_call_count++;
		ar.breathe();
		return saved;
	}
}

// MSVC bug workaround, see above similar case
extern u16 serial_breathe_and_tag(utils::serial& ar, std::string_view name, bool tag_bit)
{
	return ::stx::serial_breathe_and_tag(ar, name, tag_bit);
}

[[noreturn]] void hle_locks_t::lock()
{
	// Unreachable
	fmt::throw_exception("Unreachable");
}

bool hle_locks_t::try_lock()
{
	while (true)
	{
		auto [old, success] = lock_val.fetch_op([](s64& value)
		{
			if (value >= 0)
			{
				value++;
				return true;
			}

			return false;
		});

		if (success)
		{
			return true;
		}

		if (old == finalized)
		{
			break;
		}

		lock_val.wait(old);
	}

	return false;
}

void hle_locks_t::unlock()
{
	lock_val--;
}

bool hle_locks_t::try_finalize(std::function<bool()> test)
{
	if (!test())
	{
		return false;
	}

	if (!lock_val.compare_and_swap_test(0, waiting_for_evaluation))
	{
		return false;
	}

	if (!test())
	{
		// Failed
		ensure(lock_val.compare_and_swap_test(waiting_for_evaluation, 0));
		return false;
	}

	ensure(lock_val.compare_and_swap_test(waiting_for_evaluation, finalized));

	// Sanity check when debugging (the result is not expected to change after finalization)
	//ensure(test());

	lock_val.notify_all();
	return true;
}

