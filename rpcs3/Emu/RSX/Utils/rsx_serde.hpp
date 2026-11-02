#pragma once

#include "util/serialization.hpp"
#include "Utilities/Thread.h"
#include "Emu/RSX/rsx_methods.h"
#include "Emu/RSX/Capture/rsx_replay.h"
#include "Emu/RSX/NV47/FW/RSXIOMap.hpp"

// Serializers of RSX types, for savestates and RSX captures.
// NOTE: These specialize the extern serialize<T>. Every translation unit that serializes one of these types includes this header.

template <>
inline bool serialize<rsx::rsx_state>(utils::serial& ar, rsx::rsx_state& o)
{
	ar(o.transform_program);

	// Work around for old RSX captures.
	// RSX capture and savestates both call this method.
	// We do not want to grab transform constants if it is not savestate capture.
	const bool is_savestate_capture = thread_ctrl::get_current() && thread_ctrl::get_name() == "Emu State Capture Thread";
	if (GET_SERIALIZATION_VERSION(global_version) || is_savestate_capture)
	{
		ar(o.transform_constants);
	}

	return ar(o.registers);
}

template <>
inline bool serialize<rsx::frame_capture_data>(utils::serial& ar, rsx::frame_capture_data& o)
{
	ar(o.magic, o.version, o.LE_format);

	if (o.magic != rsx::c_fc_magic || o.version != rsx::c_fc_version || o.LE_format != u32{std::endian::little == std::endian::native})
	{
		return false;
	}

	return ar(o.tile_map, o.memory_map, o.memory_data_map, o.display_buffers_map, o.replay_commands, o.reg_state);
}

template <>
inline bool serialize<rsx::frame_capture_data::memory_block_data>(utils::serial& ar, rsx::frame_capture_data::memory_block_data& o)
{
	return ar(o.data);
}

template <>
inline bool serialize<rsx::frame_capture_data::replay_command>(utils::serial& ar, rsx::frame_capture_data::replay_command& o)
{
	return ar(o.rsx_command, o.memory_state, o.tile_state, o.display_buffer_state);
}

template <>
inline bool serialize<rsx::rsx_iomap_table>(utils::serial& ar, rsx::rsx_iomap_table& o)
{
	// We do not need more than that
	ar(std::span(o.ea.data(), 512));

	if (!ar.is_writing())
	{
		// Populate o.io
		for (const atomic_t<u32>& ea_addr : o.ea)
		{
			const u32& addr = ea_addr.raw();

			if (addr != umax)
			{
				o.io[addr >> 20].raw() = static_cast<u32>(&ea_addr - o.ea.data()) << 20;
			}
		}
	}

	return true;
}
