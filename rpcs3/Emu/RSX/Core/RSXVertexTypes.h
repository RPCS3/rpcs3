#pragma once

#include <util/types.hpp>
#include "../Common/simple_array.hpp"
#include "../gcm_enums.h"

#include <span>

namespace rsx
{
	struct vertex_array_buffer
	{
		rsx::vertex_base_type type;
		u8 attribute_size;
		u8 stride;
		std::span<const std::byte> data;
		u8 index;
		bool is_be;
	};

	struct vertex_array_register
	{
		rsx::vertex_base_type type;
		u8 attribute_size;
		std::array<u32, 4> data;
		u8 index;
	};

	struct empty_vertex_array
	{
		u8 index;
	};

	struct draw_array_command
	{
		u32 __dummy;
	};

	struct draw_indexed_array_command
	{
		std::span<const std::byte> raw_index_buffer;
	};

	struct draw_inlined_array
	{
		u32 __dummy;
		u32 __dummy2;
	};

	struct interleaved_attribute_t
	{
		u8 index;
		bool modulo;
		u16 frequency;
	};

	struct interleaved_range_info
	{
		bool interleaved = false;
		bool single_vertex = false;
		u32  base_offset = 0;
		u32  real_offset_address = 0;
		u8   memory_location = 0;
		u8   attribute_stride = 0;
		std::pair<u32, u32> vertex_range{};

		rsx::simple_array<interleaved_attribute_t> locations;

		// Check if we need to upload a full unoptimized range, i.e [0-max_index]
		std::pair<u32, u32> calculate_required_range(u32 first, u32 count);
	};

	enum attribute_buffer_placement : u8
	{
		none = 0,
		persistent = 1,
		transient = 2
	};

	class vertex_input_layout
	{
		int m_num_used_blocks = 0;
		std::array<interleaved_range_info, 16> m_blocks_data{};

	public:
		rsx::simple_array<interleaved_range_info*> interleaved_blocks{};  // Interleaved blocks to be uploaded as-is
		std::vector<std::pair<u8, u32>> volatile_blocks{};                // Volatile data blocks (immediate draw vertex data for example)
		rsx::simple_array<u8> referenced_registers{};                     // Volatile register data
		u16 attribute_mask = 0;                                           // ATTRn mask

		std::array<attribute_buffer_placement, 16> attribute_placement = fill_array(attribute_buffer_placement::none);

		vertex_input_layout() = default;

		interleaved_range_info* alloc_interleaved_block()
		{
			auto result = &m_blocks_data[m_num_used_blocks++];
			result->attribute_stride = 0;
			result->base_offset = 0;
			result->memory_location = 0;
			result->real_offset_address = 0;
			result->single_vertex = false;
			result->locations.clear();
			result->interleaved = true;
			result->vertex_range.second = 0;
			return result;
		}

		void clear()
		{
			m_num_used_blocks = 0;
			attribute_mask = 0;
			interleaved_blocks.clear();
			volatile_blocks.clear();
			referenced_registers.clear();
		}

		bool validate() const
		{
			// Criteria: At least one array stream has to be defined to feed vertex positions
			// This stream cannot be a const register as the vertices cannot create a zero-area primitive
			if (!interleaved_blocks.empty() && interleaved_blocks[0]->attribute_stride != 0)
				return true;

			if (!volatile_blocks.empty())
				return true;

			for (u16 ref_mask = attribute_mask, index = 0; ref_mask; ++index, ref_mask >>= 1)
			{
				if (!(ref_mask & 1))
				{
					// Disabled
					continue;
				}

				switch (attribute_placement[index])
				{
				case attribute_buffer_placement::transient:
				{
					// Ignore register reference
					if (std::find(referenced_registers.begin(), referenced_registers.end(), index) != referenced_registers.end())
						continue;

					// The source is inline array or immediate draw push buffer
					return true;
				}
				case attribute_buffer_placement::persistent:
				{
					return true;
				}
				case attribute_buffer_placement::none:
				{
					continue;
				}
				default:
				{
					fmt::throw_exception("Unreachable");
				}
				}
			}

			return false;
		}

		u32 calculate_interleaved_memory_requirements(u32 first_vertex, u32 vertex_count) const
		{
			u32 mem = 0;
			for (auto& block : interleaved_blocks)
			{
				const auto range = block->calculate_required_range(first_vertex, vertex_count);
				mem += range.second * block->attribute_stride;
			}

			return mem;
		}
	};
}
