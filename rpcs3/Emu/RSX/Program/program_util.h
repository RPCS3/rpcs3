#pragma once

#include "util/types.hpp"
#include "../gcm_enums.h"

namespace rsx
{
	enum program_limits
	{
		max_vertex_program_instructions = 544
	};

#pragma pack(push, 1)
	// NOTE: This structure must be packed to match GPU layout (std140).
	struct fragment_program_texture_config
	{
		struct TIU_slot
		{
			float scale[3];
			float bias[3];
			float clamp_min[2];
			float clamp_max[2];
			u32 remap;
			u32 control;
		}
		slots_[16]; // QT headers will collide with any variable named 'slots' because reasons

		TIU_slot& operator[](u32 index) { return slots_[index]; }

		void write_to(void* dst, u16 mask) const;
		void load_from(const void* src, u16 mask);

		static void masked_transfer(void* dst, const void* src, u16 mask);
	};
#pragma pack(pop)

	struct fragment_program_texture_state
	{
		u32 texture_dimensions = 0;
		u16 redirected_textures = 0;
		u16 shadow_textures = 0;
		u16 multisampled_textures = 0;

		void clear(u32 index);
		void import(const fragment_program_texture_state& other, u16 mask);
		void set_dimension(texture_dimension_extended type, u32 index);
		bool operator == (const fragment_program_texture_state& other) const;
	};

	struct vertex_program_texture_state
	{
		u32 texture_dimensions = 0;
		u16 multisampled_textures = 0;

		void clear(u32 index);
		void import(const vertex_program_texture_state& other, u16 mask);
		void set_dimension(texture_dimension_extended type, u32 index);
		bool operator == (const vertex_program_texture_state& other) const;
	};

	struct VertexProgramBase
	{
		u32 id = 0;
		std::vector<u16> constant_ids;
		bool has_indexed_constants = false;

		// Translates an incoming range of constants against our mapping.
		// If there is no linear mapping available, return -1, otherwise returns the translated index of the first slot
		// TODO: Move this somewhere else during refactor
		int translate_constants_range(int first_index, int count) const;

		// Returns true if this program consumes any constants in the range [first, first + count - 1]
		bool overlaps_constants_range(int first_index, int count) const;
	};
}
