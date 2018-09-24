#include "stdafx.h"
#include "GLGSRender.h"
#include "../rsx_methods.h"
#include "../Common/BufferUtils.h"
#include "GLHelpers.h"

namespace
{
	static constexpr std::array<const char*, 16> s_reg_table =
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
	std::tuple<u32, u32> get_index_array_for_emulated_non_indexed_draw(const std::vector<rsx::draw_range_t> &first_count_commands, rsx::primitive_type primitive_mode, gl::ring_buffer &dst)
	{
		//This is an emulated buffer, so our indices only range from 0->original_vertex_array_length
		u32 vertex_count = 0;
		u32 element_count = 0;
		verify(HERE), !gl::is_primitive_native(primitive_mode);

		for (const auto &range : first_count_commands)
		{
			element_count += (u32)get_index_count(primitive_mode, range.count);
			vertex_count += range.count;
		}

		auto mapping = dst.alloc_from_heap(element_count * sizeof(u16), 256);
		char *mapped_buffer = (char *)mapping.first;

		write_index_array_for_non_indexed_non_native_primitive_to_buffer(mapped_buffer, primitive_mode, vertex_count);
		return std::make_tuple(element_count, mapping.second);
	}

	std::tuple<u32, u32, u32> upload_index_buffer(gsl::span<const gsl::byte> raw_index_buffer, void *ptr, rsx::index_array_type type, rsx::primitive_type draw_mode, const std::vector<rsx::draw_range_t>& first_count_commands, u32 initial_vertex_count)
	{
		u32 min_index, max_index, vertex_draw_count = initial_vertex_count;

		if (!gl::is_primitive_native(draw_mode))
			vertex_draw_count = (u32)get_index_count(draw_mode, ::narrow<int>(vertex_draw_count));

		u32 block_sz = vertex_draw_count * sizeof(u32); // Force u32 index size dest to avoid overflows when adding vertex base index

		gsl::span<gsl::byte> dst{ reinterpret_cast<gsl::byte*>(ptr), ::narrow<u32>(block_sz) };
		std::tie(min_index, max_index, vertex_draw_count) = write_index_array_data_to_buffer(dst, raw_index_buffer,
			type, draw_mode, rsx::method_registers.restart_index_enabled(), rsx::method_registers.restart_index(), first_count_commands,
			rsx::method_registers.vertex_data_base_index(), [](auto prim) { return !gl::is_primitive_native(prim); });

		return std::make_tuple(min_index, max_index, vertex_draw_count);
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
		throw;
	}

	struct vertex_input_state
	{
		u32 vertex_draw_count;
		u32 allocated_vertex_count;
		u32 vertex_data_base;
		u32 vertex_index_base;
		std::optional<std::tuple<GLenum, u32>> index_info;
	};

	struct draw_command_visitor
	{
		using attribute_storage = std::vector<
		    std::variant<rsx::vertex_array_buffer, rsx::vertex_array_register, rsx::empty_vertex_array>>;

		draw_command_visitor(gl::ring_buffer& index_ring_buffer, rsx::vertex_input_layout& vertex_layout)
		    : m_index_ring_buffer(index_ring_buffer)
			, m_vertex_layout(vertex_layout)
		{}

		vertex_input_state operator()(const rsx::draw_array_command& command)
		{
			const u32 vertex_count = rsx::method_registers.current_draw_clause.get_elements_count();
			const u32 min_index    = rsx::method_registers.current_draw_clause.min_index();

			if (!gl::is_primitive_native(rsx::method_registers.current_draw_clause.primitive))
			{
				u32 index_count;
				u32 offset_in_index_buffer;
				std::tie(index_count, offset_in_index_buffer) = get_index_array_for_emulated_non_indexed_draw(
				    rsx::method_registers.current_draw_clause.draw_command_ranges,
				    rsx::method_registers.current_draw_clause.primitive, m_index_ring_buffer);

				return{ index_count, vertex_count, min_index, 0, std::make_tuple(GL_UNSIGNED_SHORT, offset_in_index_buffer) };
			}

			return{ vertex_count, vertex_count, min_index, 0, std::optional<std::tuple<GLenum, u32>>() };
		}

		vertex_input_state operator()(const rsx::draw_indexed_array_command& command)
		{
			u32 min_index = 0, max_index = 0;

			rsx::index_array_type type = rsx::method_registers.current_draw_clause.is_immediate_draw?
				rsx::index_array_type::u32:
				rsx::method_registers.index_type();

			const u32 vertex_count = rsx::method_registers.current_draw_clause.get_elements_count();
			u32 index_count = vertex_count;
			
			if (!gl::is_primitive_native(rsx::method_registers.current_draw_clause.primitive))
				index_count = (u32)get_index_count(rsx::method_registers.current_draw_clause.primitive, vertex_count);

			u32 max_size               = index_count * sizeof(u32);
			auto mapping               = m_index_ring_buffer.alloc_from_heap(max_size, 256);
			void* ptr                  = mapping.first;
			u32 offset_in_index_buffer = mapping.second;

			std::tie(min_index, max_index, index_count) = upload_index_buffer(
			    command.raw_index_buffer, ptr, type, rsx::method_registers.current_draw_clause.primitive,
			    rsx::method_registers.current_draw_clause.draw_command_ranges, vertex_count);

			if (min_index >= max_index)
			{
				//empty set, do not draw
				return{ 0, 0, 0, 0, std::make_tuple(GL_UNSIGNED_INT, offset_in_index_buffer) };
			}

			//check for vertex arrays with frequency modifiers
			for (auto &block : m_vertex_layout.interleaved_blocks)
			{
				if (block.min_divisor > 1)
				{
					//Ignore base offsets and return real results
					//The upload function will optimize the uploaded range anyway
					return{ index_count, max_index, 0, 0, std::make_tuple(GL_UNSIGNED_INT, offset_in_index_buffer) };
				}
			}

			//Prefer only reading the vertices that are referenced in the index buffer itself
			//Offset data source by min_index verts, but also notify the shader to offset the vertexID
			return{ index_count, (max_index - min_index + 1), min_index, min_index, std::make_tuple(GL_UNSIGNED_INT, offset_in_index_buffer) };
		}

		vertex_input_state operator()(const rsx::draw_inlined_array& command)
		{
			const auto stream_length = rsx::method_registers.current_draw_clause.inline_vertex_array.size();
			const u32 vertex_count = u32(stream_length * sizeof(u32)) / m_vertex_layout.interleaved_blocks[0].attribute_stride;

			if (!gl::is_primitive_native(rsx::method_registers.current_draw_clause.primitive))
			{
				u32 offset_in_index_buffer;
				u32 index_count;
				std::tie(index_count, offset_in_index_buffer) = get_index_array_for_emulated_non_indexed_draw(
					{ { 0, 0, vertex_count } },
					rsx::method_registers.current_draw_clause.primitive, m_index_ring_buffer);

				return{ index_count, vertex_count, 0, 0, std::make_tuple(GL_UNSIGNED_SHORT, offset_in_index_buffer) };
			}

			return{ vertex_count, vertex_count, 0, 0, std::optional<std::tuple<GLenum, u32>>() };
		}

	private:
		gl::ring_buffer& m_index_ring_buffer;
		rsx::vertex_input_layout& m_vertex_layout;
	};
}

gl::vertex_upload_info GLGSRender::set_vertex_buffer()
{
	std::chrono::time_point<steady_clock> then = steady_clock::now();

	m_vertex_layout = analyse_inputs_interleaved();

	if (!m_vertex_layout.validate())
		return {};

	//Write index buffers and count verts
	auto result = std::visit(draw_command_visitor(*m_index_ring_buffer, m_vertex_layout), get_draw_command(rsx::method_registers));

	auto &vertex_count = result.allocated_vertex_count;
	auto &vertex_base = result.vertex_data_base;

	//Do actual vertex upload
	auto required = calculate_memory_requirements(m_vertex_layout, vertex_count);

	std::pair<void*, u32> persistent_mapping = {}, volatile_mapping = {};
	gl::vertex_upload_info upload_info = { result.vertex_draw_count, result.allocated_vertex_count, result.vertex_index_base, 0u, 0u, result.index_info };

	if (required.first > 0)
	{
		//Check if cacheable
		//Only data in the 'persistent' block may be cached
		//TODO: make vertex cache keep local data beyond frame boundaries and hook notify command
		bool in_cache = false;
		bool to_store = false;
		u32  storage_address = UINT32_MAX;

		if (m_vertex_layout.interleaved_blocks.size() == 1 &&
			rsx::method_registers.current_draw_clause.command != rsx::draw_command::inlined_array)
		{
			storage_address = m_vertex_layout.interleaved_blocks[0].real_offset_address + vertex_base;
			if (auto cached = m_vertex_cache->find_vertex_range(storage_address, GL_R8UI, required.first))
			{
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
				m_vertex_cache->store_range(storage_address, GL_R8UI, required.first, persistent_mapping.second);
			}
		}

		if (!m_persistent_stream_view.in_range(upload_info.persistent_mapping_offset, required.first, upload_info.persistent_mapping_offset))
		{
			verify(HERE), m_max_texbuffer_size < m_attrib_ring_buffer->size();
			const size_t view_size = ((upload_info.persistent_mapping_offset + m_max_texbuffer_size) > m_attrib_ring_buffer->size()) ?
				(m_attrib_ring_buffer->size() - upload_info.persistent_mapping_offset) : m_max_texbuffer_size;

			m_persistent_stream_view.update(m_attrib_ring_buffer.get(), upload_info.persistent_mapping_offset, (u32)view_size);
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
			verify(HERE), m_max_texbuffer_size < m_attrib_ring_buffer->size();
			const size_t view_size = ((upload_info.volatile_mapping_offset + m_max_texbuffer_size) > m_attrib_ring_buffer->size()) ?
				(m_attrib_ring_buffer->size() - upload_info.volatile_mapping_offset) : m_max_texbuffer_size;

			m_volatile_stream_view.update(m_attrib_ring_buffer.get(), upload_info.volatile_mapping_offset, (u32)view_size);
			m_gl_volatile_stream_buffer->copy_from(m_volatile_stream_view);
			upload_info.volatile_mapping_offset = 0;
		}
	}

	//Write all the data
	write_vertex_data_to_memory(m_vertex_layout, vertex_base, vertex_count, persistent_mapping.first, volatile_mapping.first);

	std::chrono::time_point<steady_clock> now = steady_clock::now();
	m_vertex_upload_time += std::chrono::duration_cast<std::chrono::microseconds>(now - then).count();
	return upload_info;
}
