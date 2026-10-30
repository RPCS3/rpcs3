#pragma once

#include <util/types.hpp>
#include "Emu/RSX/NV47/FW/draw_call.inc.h"
#include "Emu/RSX/Program/ProgramStateCache.h"

namespace rsx
{
	// Per-context shader program state: the current programs and the decoded state they were built from
	struct program_state
	{
		RSXVertexProgram vertex_program = {};
		RSXFragmentProgram fragment_program = {};

		vertex_program_texture_state vp_texture_state = {};
		fragment_program_texture_state fp_texture_state = {};

		program_hash_util::vertex_program_utils::vertex_program_metadata vp_metadata = {};
		program_hash_util::fragment_program_utils::fragment_program_metadata fp_metadata = {};

		program_cache_hint_t cache_hint;

		// Primitive class of the last draw. A change between polygons and lines/points invalidates the programs.
		primitive_class draw_mode = primitive_class::polygon;

		bool is_vertex_program_instanced() const { return !!(vertex_program.ctrl & RSX_SHADER_CONTROL_INSTANCED_CONSTANTS); }
	};
}
