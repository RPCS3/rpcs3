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
#include <span>

#include <zlib.h>

LOG_CHANNEL(sys_log, "SYS");

template <>
void fmt_class_string<utils::serial>::format(std::string& out, u64 arg)
{
	const utils::serial& ar = get_object(arg);


	be_t<u64> sample64 = 0;
	const usz read_size = std::min<usz>(ar.data.size(), sizeof(sample64));
	std::memcpy(&sample64, ar.data.data() + ar.data.size() - read_size, read_size);

	fmt::append(out, "{ %s, 0x%x/0x%x, memory=0x%x, sample64=0x%016x }", ar.is_writing() ? "writing" : "reading", ar.pos, ar.data_offset + ar.data.size(), ar.data.size(), sample64);
}

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

std::vector<version_entry> get_savestate_versioning_data(fs::file&& file, std::string_view filepath)
{
	if (!file)
	{
		return {};
	}

	file.seek(0);

	utils::serial ar;
	ar.set_reading_state();

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
	return std::move(ver_data);
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
		return std::move(path_compressed);
	}

	return std::move(path);
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

		ar.seek_end();
		ar.data_offset = ar.pos;
		ar.data.clear();
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
		ar.data_offset = ar.pos;
		ar.data.clear();

		if (ar.data.capacity() >= 0x200'0000)
		{
			// Discard memory
			ar.data.shrink_to_fit();
		}

		return true;
	}

	if (~pos < size - 1)
	{
		// Overflow
		return false;
	}

	if (ar.data.empty() && pos != ar.pos)
	{
		// Relocate instead of over-fetch
		ar.seek_pos(pos);
	}

	const usz read_pre_buffer = ar.data.empty() ? 0 : utils::sub_saturate<usz>(ar.data_offset, pos);

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

	// Adjustment to prevent overflow
	const usz subtrahend = ar.data.empty() ? 0 : 1;
	const usz read_past_buffer = utils::sub_saturate<usz>(pos + (size - subtrahend), ar.data_offset + (ar.data.size() - subtrahend));
	const usz read_limit = utils::sub_saturate<usz>(ar.m_max_data, ar.data_offset); 

	if (read_past_buffer)
	{
		// Read proceeding data
		// More lightweight operation, this is the common operation
		// Allowed to fail, if memory is truly needed an assert would take place later
		const usz old_size = ar.data.size();

		// Try to prefetch data by reading more than requested
		ar.data.resize(std::min<usz>(read_limit, std::max<usz>({ ar.data.capacity(), ar.data.size() + read_past_buffer * 3 / 2, ar.m_avoid_large_prefetch ? usz{4096} : usz{0x10'0000} })));
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

void uncompressed_serialization_file_handler::finalize(utils::serial& ar)
{
	ar.seek_end();
	handle_file_op(ar, 0, umax, nullptr);
	ar.data = {}; // Deallocate and clear
}

void compressed_serialization_file_handler::initialize(utils::serial& ar)
{
	if (!m_stream.has_value())
	{
		m_stream.emplace<z_stream>();
	}

	z_stream& m_zs = std::any_cast<z_stream&>(m_stream);

	if (ar.is_writing())
	{
		if (m_write_inited)
		{
			return;
		}

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
		if (m_read_inited)
		{
			finalize(ar);
		}

		m_zs = {};
		ensure(deflateInit2(&m_zs, 9, Z_DEFLATED, 16 + 15, 9, Z_DEFAULT_STRATEGY) == Z_OK);
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
		m_write_inited = true;
	}
	else
	{
		if (m_read_inited)
		{
			return;
		}

		if (m_write_inited)
		{
			finalize(ar);
		}

		m_zs.avail_in = 0;
		m_zs.avail_out = 0;
		m_zs.next_in = nullptr;
		m_zs.next_out = nullptr;
	#ifndef _MSC_VER
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wold-style-cast"
	#endif
		ensure(inflateInit2(&m_zs, 16 + 15) == Z_OK);
		m_read_inited = true;
	}
}

bool compressed_serialization_file_handler::handle_file_op(utils::serial& ar, usz pos, usz size, const void* data)
{
	if (ar.is_writing())
	{
		initialize(ar);

		z_stream& m_zs = std::any_cast<z_stream&>(m_stream);

		if (data)
		{
			ensure(false);
		}

		// Writing not at the end is forbidden
		ensure(ar.pos == ar.data_offset + ar.data.size());

		m_zs.avail_in = static_cast<uInt>(ar.data.size());
		m_zs.next_in  = ar.data.data();

		do
		{
			m_stream_data.resize(std::max<usz>(m_stream_data.size(), ::compressBound(m_zs.avail_in)));
			m_zs.avail_out = static_cast<uInt>(m_stream_data.size());
			m_zs.next_out  = m_stream_data.data();

			if (deflate(&m_zs, Z_NO_FLUSH) == Z_STREAM_ERROR || m_file->write(m_stream_data.data(), m_stream_data.size() - m_zs.avail_out) != m_stream_data.size() - m_zs.avail_out)
			{
				deflateEnd(&m_zs);
				//m_file->close();
				break;
			}
		}
		while (m_zs.avail_out == 0);

		ar.seek_end();
		ar.data_offset = ar.pos;
		ar.data.clear();

		if (pos == umax && size == umax && *m_file)
		{
			// Request to flush the file to disk
			m_file->sync();
		}

		return true;
	}

	initialize(ar);

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
		ensure(ar.pos >= ar.data_offset);
		ar.data.clear();

		if (ar.data.capacity() >= 0x200'0000)
		{
			// Discard memory
			ar.data.shrink_to_fit();
		}

		return true;
	}

	if (~pos < size - 1)
	{
		// Overflow
		return false;
	}

	// TODO: Investigate if this optimization is worth an implementation for compressed stream
	// if (ar.data.empty() && pos != ar.pos)
	// {
	// 	// Relocate instead of over-fetch
	// 	ar.seek_pos(pos);
	// }

	const usz read_pre_buffer = utils::sub_saturate<usz>(ar.data_offset, pos);

	if (read_pre_buffer)
	{
		// Not allowed with compressed data for now
		// Unless someone implements mechanism for it
		ensure(false);
	}

	// Adjustment to prevent overflow
	const usz subtrahend = ar.data.empty() ? 0 : 1;
	const usz read_past_buffer = utils::sub_saturate<usz>(pos + (size - subtrahend), ar.data_offset + (ar.data.size() - subtrahend));
	const usz read_limit = utils::sub_saturate<usz>(ar.m_max_data, ar.data_offset); 

	if (read_past_buffer)
	{
		// Read proceeding data
		// More lightweight operation, this is the common operation
		// Allowed to fail, if memory is truly needed an assert would take place later
		const usz old_size = ar.data.size();

		// Try to prefetch data by reading more than requested
		ar.data.resize(std::min<usz>(read_limit, std::max<usz>({ ar.data.capacity(), ar.data.size() + read_past_buffer * 3 / 2, ar.m_avoid_large_prefetch ? usz{4096} : usz{0x10'0000} })));
		ar.data.resize(this->read_at(ar, old_size + ar.data_offset, data ? const_cast<void*>(data) : ar.data.data() + old_size, ar.data.size() - old_size) + old_size);
	}

	return true;
}

usz compressed_serialization_file_handler::read_at(utils::serial& ar, usz read_pos, void* data, usz size)
{
	ensure(read_pos == ar.data.size() + ar.data_offset - size);

	if (!size)
	{
		return 0;
	}

	initialize(ar);

	z_stream& m_zs = std::any_cast<z_stream&>(m_stream);

	const usz total_to_read = size;
	usz read_size = 0;
	u8* out_data = static_cast<u8*>(data);

	auto adjust_for_uint = [](usz size)
	{
		return static_cast<uInt>(std::min<usz>(uInt{umax}, size));
	};

	for (; read_size < total_to_read;)
	{
		// Drain extracted memory stash (also before first file read)
		out_data = static_cast<u8*>(data) + read_size;
		m_zs.avail_in = adjust_for_uint(m_stream_data.size() - m_stream_data_index);
		m_zs.next_in = reinterpret_cast<const u8*>(m_stream_data.data() + m_stream_data_index);
		m_zs.next_out = out_data;
		m_zs.avail_out = adjust_for_uint(size - read_size);

		while (read_size < total_to_read && m_zs.avail_in)
		{
			const int res = inflate(&m_zs, Z_BLOCK);

			bool need_more_file_memory = false;

			switch (res)
			{
			case Z_OK:
			case Z_STREAM_END:
				break;
			case Z_BUF_ERROR:
			{
				if (m_zs.avail_in)
				{
					need_more_file_memory = true;
					break;
				}

				[[fallthrough]];
			}
			default:
				inflateEnd(&m_zs);
				m_read_inited = false;
				return read_size;
			}

			read_size = m_zs.next_out - static_cast<u8*>(data);
			m_stream_data_index = m_zs.avail_in ? m_zs.next_in - m_stream_data.data() : m_stream_data.size();

			// Adjust again in case the values simply did not fit into uInt
			m_zs.avail_out = adjust_for_uint(utils::sub_saturate<usz>(total_to_read, read_size));
			m_zs.avail_in = adjust_for_uint(m_stream_data.size() - m_stream_data_index);

			if (need_more_file_memory)
			{
				break;
			}
		}

		if (read_size >= total_to_read)
		{
			break;
		}

		const usz add_size = ar.m_avoid_large_prefetch ? 0x1'0000 : 0x10'0000;
		const usz old_file_buf_size = m_stream_data.size();

		m_stream_data.resize(old_file_buf_size + add_size);
		m_stream_data.resize(old_file_buf_size + m_file->read_at(m_file_read_index, m_stream_data.data() + old_file_buf_size, add_size));

		if (m_stream_data.size() == old_file_buf_size)
		{
			// EOF
			break;
		}

		m_file_read_index += m_stream_data.size() - old_file_buf_size;
	}

	if (m_stream_data.size() - m_stream_data_index <= m_stream_data_index / 5)
	{
		// Shrink to required memory size
		m_stream_data.erase(m_stream_data.begin(), m_stream_data.begin() + m_stream_data_index);

		if (m_stream_data.capacity() >= 0x200'0000)
		{
			// Discard memory
			m_stream_data.shrink_to_fit();
		}

		m_stream_data_index = 0;
	}

	return read_size;
}

void compressed_serialization_file_handler::skip_until(utils::serial& ar)
{
	ensure(!ar.is_writing() && ar.pos >= ar.data_offset);

	if (ar.pos > ar.data_offset)
	{
		handle_file_op(ar, ar.data_offset, ar.pos - ar.data_offset, nullptr);
	}
}

void compressed_serialization_file_handler::finalize(utils::serial& ar)
{
	handle_file_op(ar, 0, umax, nullptr);

	if (!m_stream.has_value())
	{
		return;
	}

	z_stream& m_zs = std::any_cast<z_stream&>(m_stream);

	if (m_read_inited)
	{
		m_read_inited = false;
		ensure(inflateEnd(&m_zs) == Z_OK);
		return;
	}

	m_write_inited = false;

	m_zs.avail_in = 0;
	m_zs.next_in  = nullptr;

	m_stream_data.resize(m_zs.avail_out);

	do
	{
		m_zs.avail_out = static_cast<uInt>(m_stream_data.size());
		m_zs.next_out  = m_stream_data.data();

		if (deflate(&m_zs, Z_FINISH) == Z_STREAM_ERROR)
		{
			break;
		}

		m_file->write(m_stream_data.data(), m_stream_data.size() - m_zs.avail_out);
	}
	while (m_zs.avail_out == 0);

	m_stream_data = {};
	ensure(deflateEnd(&m_zs) == Z_OK);
	ar.data = {}; // Deallocate and clear
}

usz compressed_serialization_file_handler::get_size(const utils::serial& ar, usz recommended) const
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

	return std::max<usz>(utils::mul_saturate<usz>(m_file->size(), 6), memory_available);
}

bool null_serialization_file_handler::handle_file_op(utils::serial&, usz, usz, const void*)
{
	return true;
}

void null_serialization_file_handler::finalize(utils::serial&)
{
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
