#include "stdafx.h"
#include "util/types.hpp"
#include "util/logs.hpp"
#include "util/asm.hpp"
#include "Utilities/File.h"
#include "Utilities/StrFmt.h"
#include "system_config.h"
#include "savestate_utils.hpp"

#include "System.h"

#include <set>

LOG_CHANNEL(sys_log, "SYS");

template <>
void fmt_class_string<utils::serial>::format(std::string& out, u64 arg)
{
	const utils::serial& ar = get_object(arg);

	fmt::append(out, "{ %s, 0x%x/0%x, memory=0x%x }", ar.is_writing() ? "writing" : "reading", ar.pos, ar.data_offset + ar.data.size(), ar.data.size());
}

struct serial_ver_t
{
	bool used = false;
	s32 current_version = 0;
	std::set<s32> compatible_versions;
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

SERIALIZATION_VER(global_version, 0,                            15) // For stuff not listed here
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
SERIALIZATION_VER(sys_io, 23,                                   1)

// Misc versions for HLE/LLE not included so main version would not invalidated
SERIALIZATION_VER(LLE, 24,                                      1)
SERIALIZATION_VER(HLE, 25,                                      1)

std::vector<std::pair<u16, u16>> get_savestate_versioning_data(fs::file&& file)
{
	if (!file)
	{
		return {};
	}

	file.seek(0);

	if (u64 r = 0; !file.read(r) || r != "RPCS3SAV"_u64)
	{
		return {};
	}

	file.seek(10);

	u64 offs = 0;
	file.read(offs);

	const usz fsize = file.size();

	if (!offs || fsize <= offs)
	{
		return {};
	}

	utils::serial ar;
	ar.set_reading_state();
	ar.m_file_handler = make_uncompressed_serialization_file_handler(std::move(file));
	ar.seek_pos(offs);
	return ar;
}

bool is_savestate_version_compatible(const std::vector<std::pair<u16, u16>>& data, bool is_boot_check)
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

	return fs::get_cache_dir() + "/savestates/" + title + "/" + title + '_' + prefix + '_' + save_id + ".SAVESTAT";
}

bool is_savestate_compatible(fs::file&& file)
{
	return is_savestate_version_compatible(get_savestate_versioning_data(std::move(file)), false);
}

std::vector<std::pair<u16, u16>> read_used_savestate_versions()
{
	std::vector<std::pair<u16, u16>> used_serial;
	used_serial.reserve(s_serial_versions.size());

	for (serial_ver_t& ver : s_serial_versions)
	{
		if (std::exchange(ver.used, false))
		{
			used_serial.emplace_back(&ver - s_serial_versions.data(), *ver.compatible_versions.rbegin());
		}

		ver.current_version = 0;
	}

	return used_serial;
}

bool boot_last_savestate(bool testing)
{
	if (!g_cfg.savestate.suspend_emu && !Emu.GetTitleID().empty() && (Emu.IsRunning() || Emu.GetStatus() == system_state::paused))
	{
		extern bool is_savestate_compatible(fs::file&& file);

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
				if (std::string path = save_dir + entry.name; is_savestate_compatible(fs::file(path)))
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

bool uncompressed_serialization_file_handler::handle_file_op(utils::serial& ar, usz pos, usz size, const void* data)
{
	if (ar.is_writing())
	{
		if (data)
		{
			m_file->seek(pos);
			m_file->write(data, size);
			return  true;
		}

		m_file->seek(ar.data_offset);
		m_file->write(ar.data);

		if (pos == umax && size == umax)
		{
			// Request to flush the file to disk
			m_file->sync();
		}

		ar.data_offset += ar.data.size();
		ar.data.clear();
		ar.seek_end();
		return true;
	}

	if (!size)
	{
		return true;
	}

	if (pos == 0 && size == umax)
	{
		// Discard loaded data until pos if profitable
		const usz limit = ar.data_offset + ar.data.size();

		if (ar.pos > ar.data_offset && ar.pos < limit)
		{
			const usz may_discard_bytes = ar.pos - ar.data_offset;
			const usz moved_byte_count_on_discard = limit - ar.pos;

			// Cheeck profitability (check recycled memory and std::memmove costs)
			if (may_discard_bytes >= 0x50'0000 || (may_discard_bytes >= 0x20'0000 && moved_byte_count_on_discard / may_discard_bytes < 3))
			{
				ar.data_offset += may_discard_bytes;
				ar.data.erase(ar.data.begin(), ar.data.begin() + may_discard_bytes);

				if (ar.data.capacity() >= 0x200'0000)
				{
					// Discard memory
					ar.data.shrink_to_fit();
				}
			}

			return true;
		}

		// Discard all loaded data
		ar.data_offset += ar.data.size();
		ar.data.clear();

		if (ar.data.capacity() >= 0x200'0000)
		{
			// Discard memory
			ar.data.shrink_to_fit();
		}

		return true;
	}

	if (~size < pos)
	{
		// Overflow
		return false;
	}

	if (ar.data.empty() && pos != ar.pos)
	{
		// Relocate instead oof over-fetch
		ar.seek_pos(pos);
	}

	const usz read_pre_buffer = utils::sub_saturate<usz>(ar.data_offset, pos);

	if (read_pre_buffer)
	{
		// Read past data
		// Harsh operation on performance, luckily rare and not typically needed
		// Also this may would be disallowed when moving to compressed files
		// This may be a result of wrong usage of breathe() function
		ar.data.resize(ar.data.size() + read_pre_buffer);
		std::memmove(ar.data.data() + read_pre_buffer, ar.data.data(), ar.data.size() - read_pre_buffer);
		ensure(m_file->read_at(pos, ar.data.data(), read_pre_buffer) == read_pre_buffer);
		ar.data_offset -= read_pre_buffer;
	}

	const usz read_past_buffer = utils::sub_saturate<usz>(pos + size, ar.data_offset + ar.data.size());

	if (read_past_buffer)
	{
		// Read proceeding data
		// More lightweight operation, this is the common operation
		// Allowed to fail, if memory is truly needed an assert would take place later
		const usz old_size = ar.data.size();

		// Try to prefetch data by reading more than requested
		ar.data.resize(std::max<usz>({ ar.data.capacity(), ar.data.size() + read_past_buffer * 3 / 2, ar.m_avoid_large_prefetch ? usz{4096} : usz{0x10'0000} }));
		ar.data.resize(m_file->read_at(old_size + ar.data_offset, data ? const_cast<void*>(data) : ar.data.data() + old_size, ar.data.size() - old_size) + old_size);
	}

	return true;
}


usz uncompressed_serialization_file_handler::get_size(const utils::serial& ar, usz recommended) const
{
	if (ar.is_writing())
	{
		return m_file->size();
	}

	const usz memory_available = ar.data_offset + ar.data.size();

	if (memory_available >= recommended)
	{
		// Avoid calling size() if possible
		return memory_available;
	}

	return std::max<usz>(m_file->size(), memory_available);
}

namespace stx
{
	extern void serial_breathe(utils::serial& ar)
	{
		ar.breathe();
	}
}

// MSVC bug workaround, see above similar case
extern void serial_breathe(utils::serial& ar)
{
	::stx::serial_breathe(ar);
}
