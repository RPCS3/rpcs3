#include "stdafx.h"
#include "../Common/BufferUtils.h"
#include "../rsx_methods.h"
#include "GLGSRender.h"
#include "GLHelpers.h"

namespace
{
	[[maybe_unused]] constexpr std::array<const char*, 16> s_reg_table =
	{
		"in_pos_buffer", "in_weight_buffer", "in_normal_buffer",
		"in_diff_color_buffer", "in_spec_color_buffer",
		"in_fog_buffer",
		"in_point_size_buffer", "in_7_buffer",
		"in_tc0_buffer", "in_tc1_buffer", "in_tc2_buffer", "in_tc3_buffer",
		"in_tc4_buffer", "in_tc5_buffer", "in_tc6_buffer", "in_tc7_buffer"
	};
}

namespace
{
	// return vertex count if primitive type is not native (empty array otherwise)
	std::tuple<u32, u32> get_index_array_for_emulated_non_indexed_draw(rsx::primitive_type primitive_mode, gl::ring_buffer &dst, u32 vertex_count)
	{
		// This is an emulated buffer, so our indices only range from 0->original_vertex_array_length
		const auto element_count = get_index_count(primitive_mode, vertex_count);
		ensure(!gl::is_primitive_native(primitive_mode));

		auto mapping = dst.alloc_from_heap(element_count * sizeof(u16), 256);
		auto mapped_buffer = static_cast<char*>(mapping.first);

		write_index_array_for_non_indexed_non_native_primitive_to_buffer(mapped_buffer, primitive_mode, vertex_count);
		return std::make_tuple(element_count, mapping.second);
	}
}

namespace
{
	GLenum get_index_type(rsx::index_array_type type)
	{
		switch (type)
		{
		case rsx::index_array_type::u16: return GL_UNSIGNED_SHORT;
		case rsx::index_array_type::u32: return GL_UNSIGNED_INT;
		}
		fmt::throw_exception("Invalid index array type (%u)", static_cast<u8>(type));
	}

	struct vertex_input_state
	{
		bool index_rebase;
		u32 min_index;
		u32 max_index;
		u32 vertex_draw_count;
		u32 vertex_index_offset;
		std::optional<std::tuple<GLenum, u32>> index_info;
	};

	struct draw_command_visitor
	{
		draw_command_visitor(gl::ring_buffer& index_ring_buffer, rsx::vertex_input_layout& vertex_layout)
			: m_index_ring_buffer(index_ring_buffer)
			, m_vertex_layout(vertex_layout)
		{}

		vertex_input_state operator()(const rsx::draw_array_command& /*command*/)
		{
			const u32 vertex_count = rsx::method_registers.current_draw_clause.get_elements_count();
			const u32 min_index    = rsx::method_registers.current_draw_clause.min_index();
			const u32 max_index    = (min_index + vertex_count) - 1;

			if (!gl::is_primitive_native(rsx::method_registers.current_draw_clause.primitive))
			{
				u32 index_count;
				u32 offset_in_index_buffer;
				std::tie(index_count, offset_in_index_buffer) = get_index_array_for_emulated_non_indexed_draw(
					rsx::method_registers.current_draw_clause.primitive, m_index_ring_buffer,
					rsx::method_registers.current_draw_clause.get_elements_count());

				return{ false, min_index, max_index, index_count, 0, std::make_tuple(static_cast<GLenum>(GL_UNSIGNED_SHORT), offset_in_index_buffer) };
			}

			return{ false, min_index, max_index, vertex_count, 0, std::optional<std::tuple<GLenum, u32>>() };
		}

		vertex_input_state operator()(const rsx::draw_indexed_array_command& command)
		{
			u32 min_index = 0, max_index = 0;

			rsx::index_array_type type = rsx::method_registers.current_draw_clause.is_immediate_draw?
				rsx::index_array_type::u32:
				rsx::method_registers.index_type();

			u32 type_size              = get_index_type_size(type);

			const u32 vertex_count = rsx::method_registers.current_draw_clause.get_elements_count();
			u32 index_count = vertex_count;

			if (!gl::is_primitive_native(rsx::method_registers.current_draw_clause.primitive))
				index_count = static_cast<u32>(get_index_count(rsx::method_registers.current_draw_clause.primitive, vertex_count));

			u32 max_size               = index_count * type_size;
			auto mapping               = m_index_ring_buffer.alloc_from_heap(max_size, 256);
			void* ptr                  = mapping.first;
			u32 offset_in_index_buffer = mapping.second;

			std::tie(min_index, max_index, index_count) = write_index_array_data_to_buffer(
				{ reinterpret_cast<std::byte*>(ptr), max_size },
				command.raw_index_buffer, type,
				rsx::method_registers.current_draw_clause.primitive,
				rsx::method_registers.restart_index_enabled(),
				rsx::method_registers.restart_index(),
				[](auto prim) { return !gl::is_primitive_native(prim); });

			if (min_index >= max_index)
			{
				//empty set, do not draw
				return{ false, 0, 0, 0, 0, std::make_tuple(get_index_type(type), offset_in_index_buffer) };
			}

			// Prefer only reading the vertices that are referenced in the index buffer itself
			// Offset data source by min_index verts, but also notify the shader to offset the vertexID (important for modulo op)
			const auto index_offset = rsx::method_registers.vertex_data_base_index();
			return{ true, min_index, max_index, index_count, index_offset, std::make_tuple(get_index_type(type), offset_in_index_buffer) };
		}

		vertex_input_state operator()(const rsx::draw_inlined_array& /*command*/)
		{
			const auto stream_length = rsx::method_registers.current_draw_clause.inline_vertex_array.size();
			const u32 vertex_count = u32(stream_length * sizeof(u32)) / m_vertex_layout.interleaved_blocks[0]->attribute_stride;

			if (!gl::is_primitive_native(rsx::method_registers.current_draw_clause.primitive))
			{
				u32 offset_in_index_buffer;
				u32 index_count;
				std::tie(index_count, offset_in_index_buffer) = get_index_array_for_emulated_non_indexed_draw(
					rsx::method_registers.current_draw_clause.primitive, m_index_ring_buffer, vertex_count);

				return{ false, 0, vertex_count, index_count, 0, std::make_tuple(static_cast<GLenum>(GL_UNSIGNED_SHORT), offset_in_index_buffer) };
			}

			return{ false, 0, vertex_count, vertex_count, 0, std::optional<std::tuple<GLenum, u32>>() };
		}

	private:
		gl::ring_buffer& m_index_ring_buffer;
		rsx::vertex_input_layout& m_vertex_layout;
	};
}

gl::vertex_upload_info GLGSRender::set_vertex_buffer()
{
	m_profiler.start();

	//Write index buffers and count verts
	auto result = std::visit(draw_command_visitor(*m_index_ring_buffer, m_vertex_layout), m_draw_processor.get_draw_command(rsx::method_registers));

	const u32 vertex_count = (result.max_index - result.min_index) + 1;
	u32 vertex_base = result.min_index;
	u32 index_base = 0;

	if (result.index_rebase)
	{
		vertex_base = rsx::get_index_from_base(vertex_base, rsx::method_registers.vertex_data_base_index());
		index_base = result.min_index;
	}

	//Do actual vertex upload
	auto required = calculate_memory_requirements(m_vertex_layout, vertex_base, vertex_count);

	std::pair<void*, u32> persistent_mapping = {}, volatile_mapping = {};
	gl::vertex_upload_info upload_info =
	{
		result.vertex_draw_count,                // Vertex count
		vertex_count,                            // Allocated vertex count
		vertex_base,                             // First vertex in block
		index_base,                              // Index of attribute at data location 0
		result.vertex_index_offset,              // Hw index offset
		0u, 0u,                                  // Mapping
		result.index_info                        // Index buffer info
	};

	if (required.first > 0)
	{
		//Check if cacheable
		//Only data in the 'persistent' block may be cached
		//TODO: make vertex cache keep local data beyond frame boundaries and hook notify command
		bool in_cache = false;
		bool to_store = false;
		u32  storage_address = -1;

		if (m_vertex_layout.interleaved_blocks.size() == 1 &&
			rsx::method_registers.current_draw_clause.command != rsx::draw_command::inlined_array)
		{
			const auto data_offset = (vertex_base * m_vertex_layout.interleaved_blocks[0]->attribute_stride);
			storage_address = m_vertex_layout.interleaved_blocks[0]->real_offset_address + data_offset;

			if (auto cached = m_vertex_cache->find_vertex_range(storage_address, required.first))
			{
				ensure(cached->local_address == storage_address);

				in_cache = true;
				upload_info.persistent_mapping_offset = cached->offset_in_heap;
			}
			else
			{
				to_store = true;
			}
		}

		if (!in_cache)
		{
			persistent_mapping = m_attrib_ring_buffer->alloc_from_heap(required.first, m_min_texbuffer_alignment);
			upload_info.persistent_mapping_offset = persistent_mapping.second;

			if (to_store)
			{
				//store ref in vertex cache
				m_vertex_cache->store_range(storage_address, required.first, persistent_mapping.second);
			}
		}

		if (!m_persistent_stream_view.in_range(upload_info.persistent_mapping_offset, required.first, upload_info.persistent_mapping_offset))
		{
			ensure(m_max_texbuffer_size < m_attrib_ring_buffer->size());
			const usz view_size = ((upload_info.persistent_mapping_offset + m_max_texbuffer_size) > m_attrib_ring_buffer->size()) ?
				(m_attrib_ring_buffer->size() - upload_info.persistent_mapping_offset) : m_max_texbuffer_size;

			m_persistent_stream_view.update(m_attrib_ring_buffer.get(), upload_info.persistent_mapping_offset, static_cast<u32>(view_size));
			m_gl_persistent_stream_buffer->copy_from(m_persistent_stream_view);
			upload_info.persistent_mapping_offset = 0;
		}
	}

	if (required.second > 0)
	{
		volatile_mapping = m_attrib_ring_buffer->alloc_from_heap(required.second, m_min_texbuffer_alignment);
		upload_info.volatile_mapping_offset = volatile_mapping.second;

		if (!m_volatile_stream_view.in_range(upload_info.volatile_mapping_offset, required.second, upload_info.volatile_mapping_offset))
		{
			ensure(m_max_texbuffer_size < m_attrib_ring_buffer->size());
			const usz view_size = ((upload_info.volatile_mapping_offset + m_max_texbuffer_size) > m_attrib_ring_buffer->size()) ?
				(m_attrib_ring_buffer->size() - upload_info.volatile_mapping_offset) : m_max_texbuffer_size;

			m_volatile_stream_view.update(m_attrib_ring_buffer.get(), upload_info.volatile_mapping_offset, static_cast<u32>(view_size));
			m_gl_volatile_stream_buffer->copy_from(m_volatile_stream_view);
			upload_info.volatile_mapping_offset = 0;
		}
	}

	//Write all the data
	m_draw_processor.write_vertex_data_to_memory(m_vertex_layout, vertex_base, vertex_count, persistent_mapping.first, volatile_mapping.first);

	m_frame_stats.vertex_upload_time += m_profiler.duration();
	return upload_info;
}
