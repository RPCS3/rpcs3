#pragma once

#include <util/types.hpp>

#include "Emu/RSX/Common/simple_array.hpp"
#include "Emu/RSX/Core/RSXVertexTypes.h"
#include "Emu/RSX/NV47/FW/draw_call.hpp"
#include "Emu/RSX/Program/ProgramStateCache.h"
#include "Emu/RSX/rsx_vertex_data.h"

#include <span>
#include <variant>

namespace rsx
{
	struct rsx_state;
	struct context;
	class io_buffer;

	class draw_command_processor
	{
		using vertex_program_metadata_t = program_hash_util::vertex_program_utils::vertex_program_metadata;

		context* m_ctx = nullptr;

	protected:
		friend class thread;

		std::array<push_buffer_vertex_info, 16> m_vertex_push_buffers;
		rsx::simple_array<u32> m_element_push_buffer;

		struct
		{
			rsx::simple_array<u32> u32buf;
			rsx::simple_array<u128> u128buf;
		} mutable m_scratch_buffers;

	public:
		draw_command_processor() = default;

		void init(context* ctx)
		{
			m_ctx = ctx;

			for (auto& buffer : m_vertex_push_buffers)
			{
				buffer.ctx = ctx;
			}
		}

		// Analyze vertex inputs and group all interleaved blocks
		void analyse_inputs_interleaved(vertex_input_layout& layout, const vertex_program_metadata_t& vp_metadata);

		/**
		 * Computes VRAM requirements needed to upload raw vertex streams
		 * result.first contains persistent memory requirements
		 * result.second contains volatile memory requirements
		 */
		std::pair<u32, u32> calculate_memory_requirements(const vertex_input_layout& layout, u32 first_vertex, u32 vertex_count) const;

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

		/**
		 * Fill buffer with 4x4 scale offset matrix.
		 * Vertex shader's position is to be multiplied by this matrix.
		 * if flip_y is set, the matrix is modified to use d3d convention.
		 */
		void fill_scale_offset_data(void* buffer, bool flip_y) const;

		/**
		 * Fill buffer with user clip information
		 */
		void fill_user_clip_data(void* buffer) const;

		/**
		 * Fill the 96-byte vertex environment block: scale/offset matrix, user clip planes, transform branch bits,
		 * point size and depth clip range. point_scale is the host's resolution scale for the point size.
		 */
		void fill_vertex_env(void* buffer, f32 point_scale) const;

		/**
		 * Fill the 16-byte headers in front of the shader interpreter's instruction blocks.
		 * Vertex: base address, entry, output mask, two-sided lighting. Fragment: shader control, texture dimensions, referenced textures.
		 */
		void fill_interpreter_vertex_header(void* buffer) const;
		void fill_interpreter_fragment_header(void* buffer) const;

		/**
		* Fill buffer with vertex program constants.
		* Relocation table allows to do a partial fill with only selected registers.
		*/
		void fill_vertex_program_constants_data(void* buffer, const std::span<const u16>& reloc_table) const;

		/**
		 * Fill buffer with fragment rasterization state.
		 * Fills current fog values, alpha test parameters and texture scaling parameters
		 */
		void fill_fragment_state_buffer(void* buffer, const RSXFragmentProgram& fragment_program) const;

		// Fill instancing buffers. A single iobuf is used for both. 256byte alignment enforced to allow global bind
		// Returns offsets to the index redirection lookup table and constants field array
		void fill_constants_instancing_buffer(rsx::io_buffer& indirection_table_buf, rsx::io_buffer& constants_data_array_buffer, const VertexProgramBase* prog) const;
	};
}
