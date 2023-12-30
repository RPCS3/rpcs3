#include "stdafx.h"
#include "util/types.hpp"
#include "util/logs.hpp"
#include "util/asm.hpp"
#include "util/v128.hpp"
#include "util/simd.hpp"
#include "Utilities/File.h"
#include "Utilities/StrFmt.h"
#include "system_config.h"
#include "savestate_utils.hpp"

#include "System.h"

#include <set>
#include <any>
#include <span>

LOG_CHANNEL(sys_log, "SYS");

struct serial_ver_t
{
	bool used = false;
	u16 current_version = 0;
	std::set<u16> compatible_versions;
};

static std::array<serial_ver_t, 26> s_serial_versions;

#define SERIALIZATION_VER(name, identifier, ...) \
\
	const bool s_##name##_serialization_fill = []() { if (::s_serial_versions[identifier].compatible_versions.empty()) ::s_serial_versions[identifier].compatible_versions = {__VA_ARGS__}; return true; }();\
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

SERIALIZATION_VER(global_version, 0,                            16) // For stuff not listed here
SERIALIZATION_VER(ppu, 1,                                       1)
SERIALIZATION_VER(spu, 2,                                       1)
SERIALIZATION_VER(lv2_sync, 3,                                  1)
SERIALIZATION_VER(lv2_vm, 4,                                    1)
SERIALIZATION_VER(lv2_net, 5,                                   1)
SERIALIZATION_VER(lv2_fs, 6,                                    1)
SERIALIZATION_VER(lv2_prx_overlay, 7,                           1)
SERIALIZATION_VER(lv2_memory, 8,                                1)
SERIALIZATION_VER(lv2_config, 9,                                1)

namespace rsx
{
	SERIALIZATION_VER(rsx, 10,                                  1)
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
SERIALIZATION_VER(cellCamera, 14,                               1)
SERIALIZATION_VER(cellGem, 15,                                  1)
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

std::vector<version_entry> get_savestate_versioning_data(fs::file&& file, std::string_view filepath)
{
	if (!file)
	{
		return {};
	}

	file.seek(0);

	utils::serial ar;
	ar.set_reading_state({}, true);

	ar.m_file_handler = filepath.ends_with(".gz") ? static_cast<std::unique_ptr<utils::serialization_file_handler>>(make_compressed_serialization_file_handler(std::move(file)))
		: make_uncompressed_serialization_file_handler(std::move(file));

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

bool is_savestate_version_compatible(const std::vector<version_entry>& data, bool is_boot_check)
{
	if (data.empty())
	{
		return false;
	}

	bool ok = true;

	for (auto [identifier, version] : data)
	{
		if (identifier >= s_serial_versions.size())
		{
			(is_boot_check ? sys_log.error : sys_log.trace)("Savestate version identifier is unknown! (category=%u, version=%u)", identifier, version);
			ok = false; // Log all mismatches
		}
		else if (!s_serial_versions[identifier].compatible_versions.count(version))
		{
			(is_boot_check ? sys_log.error : sys_log.trace)("Savestate version is not supported. (category=%u, version=%u)", identifier, version);
			ok = false;
		}
		else if (is_boot_check)
		{
			s_serial_versions[identifier].current_version = version;
		}
	}

	if (!ok && is_boot_check)
	{
		for (auto [identifier, _] : data)
		{
			s_serial_versions[identifier].current_version = 0;
		}
	}

	return ok;
}

std::string get_savestate_file(std::string_view title_id, std::string_view boot_path, s64 abs_id, s64 rel_id)
{
	const std::string title = std::string{title_id.empty() ? boot_path.substr(boot_path.find_last_of(fs::delim) + 1) : title_id};

	if (abs_id == -1 && rel_id == -1)
	{
		// Return directory
		return fs::get_cache_dir() + "/savestates/" + title + "/";
	}

	ensure(rel_id < 0 || abs_id >= 0, "Unimplemented!");

	const std::string save_id = fmt::format("%d", abs_id);

	// Make sure that savestate file with higher IDs are placed at the bottom of "by name" file ordering in directory view by adding a single character prefix which tells the ID length
	// While not needing to keep a 59 chars long suffix at all times for this purpose
	const char prefix = ::at32("0123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz"sv, save_id.size());

	std::string path = fs::get_cache_dir() + "/savestates/" + title + "/" + title + '_' + prefix + '_' + save_id + ".SAVESTAT";

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

bool boot_last_savestate(bool testing)
{
	if (!g_cfg.savestate.suspend_emu && !Emu.GetTitleID().empty() && (Emu.IsRunning() || Emu.GetStatus() == system_state::paused))
	{
		const std::string save_dir = fs::get_cache_dir() + "/savestates/";

		std::string savestate_path;
		s64 mtime = smin;

		for (auto&& entry : fs::dir(save_dir))
		{
			if (entry.is_directory)
			{
				continue;
			}

			// Find the latest savestate file compatible with the game (TODO: Check app version and anything more)
			if (entry.name.find(Emu.GetTitleID()) != umax && mtime <= entry.mtime)
			{
				if (std::string path = save_dir + entry.name + ".gz"; is_savestate_compatible(fs::file(path), path))
				{
					savestate_path = std::move(path);
					mtime = entry.mtime;
				}
				else if (std::string path = save_dir + entry.name; is_savestate_compatible(fs::file(path), path))
				{
					savestate_path = std::move(path);
					mtime = entry.mtime;
				}
			}
		}

		const bool result = fs::is_file(savestate_path);

		if (testing)
		{
			sys_log.trace("boot_last_savestate(true) returned %s.", result);
			return result;
		}

		if (result)
		{
			sys_log.success("Booting the most recent savestate \'%s\' using the Reload shortcut.", savestate_path);
			Emu.GracefulShutdown(false);

			if (game_boot_result error = Emu.BootGame(savestate_path, "", true); error != game_boot_result::no_errors)
			{
				sys_log.error("Failed to boot savestate \'%s\' using the Reload shortcut. (error: %s)", savestate_path, error);
			}
			else
			{
				return true;
			}
		}

		sys_log.error("No compatible savestate file found in \'%s\''", save_dir);
	}

	return false;
}

bool load_and_check_reserved(utils::serial& ar, usz size)
{
	u8 bytes[4096];
	std::memset(&bytes[size & (0 - sizeof(v128))], 0, sizeof(v128));
	ensure(size <= std::size(bytes));

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

		constexpr u16 data_mask = 0x7fff;

		if (ar.m_file_handler && ar.m_file_handler->is_null())
		{
			return (tag_bit ? data_mask + 1 : 0);
		}

		u16 tag = static_cast<u16>((static_cast<u16>(ar.pos / 2) & data_mask) | (tag_bit ? data_mask + 1 : 0));
		u16 saved = tag;
		ar(saved);

		sys_log.trace("serial_breathe_and_tag(): %s, object: '%s', next-object: '%s', expected/tag: 0x%x == 0x%x", ar, s_tls_object_name, name, tag, saved);

		if ((saved ^ tag) & data_mask)
		{
			ensure(!ar.is_writing());
			fmt::throw_exception("serial_breathe_and_tag(): %s, object: '%s', next-object: '%s', expected/tag: 0x%x != 0x%x,", ar, s_tls_object_name, name, tag, saved);
		}

		s_tls_object_name = name;
		ar.breathe();
		return saved;
	}
}

// MSVC bug workaround, see above similar case
extern u16 serial_breathe_and_tag(utils::serial& ar, std::string_view name, bool tag_bit)
{
	return ::stx::serial_breathe_and_tag(ar, name, tag_bit);
}
