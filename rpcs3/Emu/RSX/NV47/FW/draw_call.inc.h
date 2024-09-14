#pragma once

#include <util/types.hpp>

namespace rsx
{
	enum class draw_command
	{
		none,
		array,
		inlined_array,
		indexed,
	};

	enum command_barrier_type : u32
	{
		primitive_restart_barrier,
		vertex_base_modifier_barrier,
		index_base_modifier_barrier,
		vertex_array_offset_modifier_barrier,
		transform_constant_load_modifier_barrier,
		transform_constant_update_barrier
	};

	enum command_execution_flags : u32
	{
		vertex_base_changed = (1 << 0),
		index_base_changed = (1 << 1),
		vertex_arrays_changed = (1 << 2),
		transform_constants_changed = (1 << 3)
	};

	enum class primitive_class
	{
		polygon,
		non_polygon
	};

	struct barrier_t
	{
		u32 draw_id;
		u64 timestamp;

		u32 address;
		u32 index;
		u32 arg0;
		u32 arg1;
		u32 flags;
		command_barrier_type type;

		bool operator < (const barrier_t& other) const
		{
			if (address != ~0u)
			{
				return address < other.address;
			}

			return timestamp < other.timestamp;
		}

		ENABLE_BITWISE_SERIALIZATION;
	};

	struct draw_range_t
	{
		u32 command_data_offset = 0;
		u32 first = 0;
		u32 count = 0;

		ENABLE_BITWISE_SERIALIZATION;
	};
}
