#include "stdafx.h"
#include "program_util.h"

namespace rsx
{
	// Convert u16 to u32
	static u32 duplicate_and_extend(u16 bits)
	{
		u32 x = bits;

		x = (x | (x << 8)) & 0x00FF00FF;
		x = (x | (x << 4)) & 0x0F0F0F0F;
		x = (x | (x << 2)) & 0x33333333;
		x = (x | (x << 1)) & 0x55555555;

		return x | (x << 1);
	}

	void fragment_program_texture_config::masked_transfer(void* dst, const void* src, u16 mask)
	{
		// Try to optimize for the very common case (first 4 slots used)
		switch (mask)
		{
		case 0:
			return;
		case 1:
			std::memcpy(dst, src, sizeof(TIU_slot)); return;
		case 3:
			std::memcpy(dst, src, sizeof(TIU_slot) * 2); return;
		case 7:
			std::memcpy(dst, src, sizeof(TIU_slot) * 3); return;
		case 15:
			std::memcpy(dst, src, sizeof(TIU_slot) * 4); return;
		default:
			break;
		};

		const auto start = std::countr_zero(mask);
		const auto end = 16 - std::countl_zero(mask);
		const auto mem_offset = (start * sizeof(TIU_slot));
		const auto mem_size = (end - start) * sizeof(TIU_slot);
		std::memcpy(static_cast<u8*>(dst) + mem_offset, reinterpret_cast<const u8*>(src) + mem_offset, mem_size);
	}

	void fragment_program_texture_config::write_to(void* dst, u16 mask) const
	{
		masked_transfer(dst, slots_, mask);
	}

	void fragment_program_texture_config::load_from(const void* src, u16 mask)
	{
		masked_transfer(slots_, src, mask);
	}

	void fragment_program_texture_state::clear(u32 index)
	{
		const u16 clear_mask = ~(static_cast<u16>(1 << index));
		redirected_textures &= clear_mask;
		shadow_textures &= clear_mask;
		multisampled_textures &= clear_mask;
	}

	void fragment_program_texture_state::import(const fragment_program_texture_state& other, u16 mask)
	{
		redirected_textures = other.redirected_textures & mask;
		shadow_textures = other.shadow_textures & mask;
		multisampled_textures = other.multisampled_textures & mask;
		texture_dimensions = other.texture_dimensions & duplicate_and_extend(mask);
	}

	void fragment_program_texture_state::set_dimension(texture_dimension_extended type, u32 index)
	{
		const auto offset = (index * 2);
		const auto mask = 3 << offset;
		texture_dimensions &= ~mask;
		texture_dimensions |= static_cast<u32>(type) << offset;
	}

	bool fragment_program_texture_state::operator == (const fragment_program_texture_state& other) const
	{
		return texture_dimensions == other.texture_dimensions &&
			redirected_textures == other.redirected_textures &&
			shadow_textures == other.shadow_textures &&
			multisampled_textures == other.multisampled_textures;
	}

	void vertex_program_texture_state::clear(u32 index)
	{
		const u16 clear_mask = ~(static_cast<u16>(1 << index));
		multisampled_textures &= clear_mask;
	}

	void vertex_program_texture_state::import(const vertex_program_texture_state& other, u16 mask)
	{
		multisampled_textures = other.multisampled_textures & mask;
		texture_dimensions = other.texture_dimensions & duplicate_and_extend(mask);
	}

	void vertex_program_texture_state::set_dimension(texture_dimension_extended type, u32 index)
	{
		const auto offset = (index * 2);
		const auto mask = 3 << offset;
		texture_dimensions &= ~mask;
		texture_dimensions |= static_cast<u32>(type) << offset;
	}

	bool vertex_program_texture_state::operator == (const vertex_program_texture_state& other) const
	{
		return texture_dimensions == other.texture_dimensions &&
			multisampled_textures == other.multisampled_textures;
	}

	int VertexProgramBase::translate_constants_range(int first_index, int count) const
	{
		// The constant ids should be sorted, so just find the first one and check for continuity
		int index = -1;
		int next = first_index;
		int last = first_index + count - 1;

		// Early rejection test
		if (constant_ids.empty() || first_index > constant_ids.back() || last < first_index)
		{
			return -1;
		}

		for (size_t i = 0; i < constant_ids.size(); ++i)
		{
			if (constant_ids[i] > first_index && index < 0)
			{
				// No chance of a match
				return -1;
			}

			if (constant_ids[i] == next)
			{
				// Index matched
				if (index < 0)
				{
					index = static_cast<int>(i);
				}

				if (last == next++)
				{
					return index;
				}

				continue;
			}

			if (index >= 0)
			{
				// Previously matched but no more
				return -1;
			}
		}

		// OOB or partial match
		return -1;
	}

	bool VertexProgramBase::overlaps_constants_range(int first_index, int count) const
	{
		if (has_indexed_constants)
		{
			return true;
		}

		const int last_index = first_index + count - 1;

		// Early rejection test
		if (constant_ids.empty() || first_index > constant_ids.back() || last_index < first_index)
		{
			return false;
		}

		// Check for any hits
		for (auto& idx : constant_ids)
		{
			if (idx >= first_index && idx <= last_index)
			{
				return true;
			}
		}

		return false;
	}
}
