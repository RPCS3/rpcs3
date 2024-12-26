#pragma once

#include <util/types.hpp>

#include "Emu/RSX/Core/RSXVertexTypes.h"
#include "Emu/RSX/NV47/FW/draw_call.hpp"
#include "Emu/RSX/Program/ProgramStateCache.h"
#include "Emu/RSX/rsx_vertex_data.h"

#include <span>
#include <variant>

namespace rsx
{
	struct rsx_state;
	class thread;

	class draw_command_processor
	{
		using vertex_program_metadata_t = program_hash_util::vertex_program_utils::vertex_program_metadata;

		thread* m_thread = nullptr;

	protected:
		friend class thread;

		std::array<push_buffer_vertex_info, 16> m_vertex_push_buffers;
		rsx::simple_array<u32> m_element_push_buffer;

	public:
		draw_command_processor() = default;

		void init(thread* rsxthr)
		{
			m_thread = rsxthr;
		}

		// Analyze vertex inputs and group all interleaved blocks
		void analyse_inputs_interleaved(vertex_input_layout& layout, const vertex_program_metadata_t& vp_metadata);

		// Retrieve raw bytes for the index array (untyped)
		std::span<const std::byte> get_raw_index_array(const draw_clause& draw_indexed_clause) const;

		// Get compiled draw command for backend rendering
		std::variant<draw_array_command, draw_indexed_array_command, draw_inlined_array>
			get_draw_command(const rsx::rsx_state& state) const;

		// Push-buffers for immediate rendering (begin-end scopes)
		void append_to_push_buffer(u32 attribute, u32 size, u32 subreg_index, vertex_base_type type, u32 value);

		u32 get_push_buffer_vertex_count() const;

		void append_array_element(u32 index);

		u32 get_push_buffer_index_count() const;

		void clear_push_buffers();

		const std::span<const u32> element_push_buffer() const
		{
			return m_element_push_buffer;
		}

		// Host driver helpers
		void fill_vertex_layout_state(
			const vertex_input_layout& layout,
			const vertex_program_metadata_t& vp_metadata,
			u32 first_vertex,
			u32 vertex_count,
			s32* buffer,
			u32 persistent_offset_base,
			u32 volatile_offset_base) const;

		void write_vertex_data_to_memory(
			const vertex_input_layout& layout,
			u32 first_vertex,
			u32 vertex_count,
			void* persistent_data,
			void* volatile_data) const;
	};
}
