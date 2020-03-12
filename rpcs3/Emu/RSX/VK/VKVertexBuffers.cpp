#include "stdafx.h"
#include "VKGSRender.h"
#include "../rsx_methods.h"
#include "../Common/BufferUtils.h"

namespace vk
{
	VkPrimitiveTopology get_appropriate_topology(rsx::primitive_type mode, bool &requires_modification)
	{
		requires_modification = false;

		switch (mode)
		{
		case rsx::primitive_type::lines:
			return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		case rsx::primitive_type::line_loop:
			requires_modification = true;
			return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
		case rsx::primitive_type::line_strip:
			return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
		case rsx::primitive_type::points:
			return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		case rsx::primitive_type::triangles:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		case rsx::primitive_type::triangle_strip:
		case rsx::primitive_type::quad_strip:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		case rsx::primitive_type::triangle_fan:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
		case rsx::primitive_type::quads:
		case rsx::primitive_type::polygon:
			requires_modification = true;
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		default:
			fmt::throw_exception("Unsupported primitive topology 0x%x", static_cast<u8>(mode));
		}
	}

	bool is_primitive_native(rsx::primitive_type& mode)
	{
		bool result;
		get_appropriate_topology(mode, result);
		return !result;
	}

	VkIndexType get_index_type(rsx::index_array_type type)
	{
		switch (type)
		{
		case rsx::index_array_type::u32:
			return VK_INDEX_TYPE_UINT32;
		case rsx::index_array_type::u16:
			return VK_INDEX_TYPE_UINT16;
		}
		fmt::throw_exception("Invalid index array type (%u)", static_cast<u8>(type));
	}
}

namespace
{
	std::tuple<u32, std::tuple<VkDeviceSize, VkIndexType>> generate_emulating_index_buffer(
		const rsx::draw_clause& clause, u32 vertex_count,
		vk::data_heap& m_index_buffer_ring_info)
	{
		u32 index_count = get_index_count(clause.primitive, vertex_count);
		u32 upload_size = index_count * sizeof(u16);

		VkDeviceSize offset_in_index_buffer = m_index_buffer_ring_info.alloc<256>(upload_size);
		void* buf = m_index_buffer_ring_info.map(offset_in_index_buffer, upload_size);

		g_fxo->get<rsx::dma_manager>()->emulate_as_indexed(buf, clause.primitive, vertex_count);

		m_index_buffer_ring_info.unmap();
		return std::make_tuple(
			index_count, std::make_tuple(offset_in_index_buffer, VK_INDEX_TYPE_UINT16));
	}

	struct vertex_input_state
	{
		VkPrimitiveTopology native_primitive_type;
		bool index_rebase;
		u32 min_index;
		u32 max_index;
		u32 vertex_draw_count;
		u32 vertex_index_offset;
		std::optional<std::tuple<VkDeviceSize, VkIndexType>> index_info;
	};

	struct draw_command_visitor
	{
		draw_command_visitor(vk::data_heap& index_buffer_ring_info, rsx::vertex_input_layout& layout)
			: m_index_buffer_ring_info(index_buffer_ring_info)
			, m_vertex_layout(layout)
		{
		}

		vertex_input_state operator()(const rsx::draw_array_command& command)
		{
			bool primitives_emulated = false;
			VkPrimitiveTopology prims = vk::get_appropriate_topology(
				rsx::method_registers.current_draw_clause.primitive, primitives_emulated);

			const u32 vertex_count = rsx::method_registers.current_draw_clause.get_elements_count();
			const u32 min_index = rsx::method_registers.current_draw_clause.min_index();
			const u32 max_index = (min_index + vertex_count) - 1;

			if (primitives_emulated)
			{
				u32 index_count;
				std::optional<std::tuple<VkDeviceSize, VkIndexType>> index_info;

				std::tie(index_count, index_info) =
					generate_emulating_index_buffer(rsx::method_registers.current_draw_clause,
						vertex_count, m_index_buffer_ring_info);

				return{ prims, false, min_index, max_index, index_count, 0, index_info };
			}

			return{ prims, false, min_index, max_index, vertex_count, 0, {} };
		}

		vertex_input_state operator()(const rsx::draw_indexed_array_command& command)
		{
			bool primitives_emulated = false;
			auto primitive = rsx::method_registers.current_draw_clause.primitive;
			const VkPrimitiveTopology prims = vk::get_appropriate_topology(primitive, primitives_emulated);
			const bool emulate_restart = rsx::method_registers.restart_index_enabled() && vk::emulate_primitive_restart(primitive);

			rsx::index_array_type index_type = rsx::method_registers.current_draw_clause.is_immediate_draw ?
				rsx::index_array_type::u32 :
				rsx::method_registers.index_type();

			u32 type_size = get_index_type_size(index_type);

			u32 index_count = rsx::method_registers.current_draw_clause.get_elements_count();
			if (primitives_emulated)
				index_count = get_index_count(rsx::method_registers.current_draw_clause.primitive, index_count);
			u32 upload_size = index_count * type_size;

			if (emulate_restart) upload_size *= 2;

			VkDeviceSize offset_in_index_buffer = m_index_buffer_ring_info.alloc<4>(upload_size);
			void* buf = m_index_buffer_ring_info.map(offset_in_index_buffer, upload_size);

			gsl::span<std::byte> dst;
			std::vector<std::byte> tmp;
			if (emulate_restart)
			{
				tmp.resize(upload_size);
				dst = tmp;
			}
			else
			{
				dst = gsl::span<std::byte>(static_cast<std::byte*>(buf), upload_size);
			}

			/**
			* Upload index (and expands it if primitive type is not natively supported).
			*/
			u32 min_index, max_index;
			std::tie(min_index, max_index, index_count) = write_index_array_data_to_buffer(
				dst,
				command.raw_index_buffer, index_type,
				rsx::method_registers.current_draw_clause.primitive,
				rsx::method_registers.restart_index_enabled(),
				rsx::method_registers.restart_index(),
				[](auto prim) { return !vk::is_primitive_native(prim); });

			if (min_index >= max_index)
			{
				//empty set, do not draw
				m_index_buffer_ring_info.unmap();
				return{ prims, false, 0, 0, 0, 0, {} };
			}

			if (emulate_restart)
			{
				if (index_type == rsx::index_array_type::u16)
				{
					index_count = rsx::remove_restart_index(static_cast<u16*>(buf), reinterpret_cast<u16*>(tmp.data()), index_count, u16{UINT16_MAX});
				}
				else
				{
					index_count = rsx::remove_restart_index(static_cast<u32*>(buf), reinterpret_cast<u32*>(tmp.data()), index_count, u32{UINT32_MAX});
				}
			}

			m_index_buffer_ring_info.unmap();

			std::optional<std::tuple<VkDeviceSize, VkIndexType>> index_info =
				std::make_tuple(offset_in_index_buffer, vk::get_index_type(index_type));

			const auto index_offset = rsx::method_registers.vertex_data_base_index();
			return {prims, true, min_index, max_index, index_count, index_offset, index_info};
		}

		vertex_input_state operator()(const rsx::draw_inlined_array& command)
		{
			bool primitives_emulated = false;
			auto &draw_clause = rsx::method_registers.current_draw_clause;
			VkPrimitiveTopology prims = vk::get_appropriate_topology(draw_clause.primitive, primitives_emulated);

			const auto stream_length = rsx::method_registers.current_draw_clause.inline_vertex_array.size();
			const u32 vertex_count = u32(stream_length * sizeof(u32)) / m_vertex_layout.interleaved_blocks[0].attribute_stride;

			if (!primitives_emulated)
			{
				return{ prims, false, 0, vertex_count - 1, vertex_count, 0, {} };
			}

			u32 index_count;
			std::optional<std::tuple<VkDeviceSize, VkIndexType>> index_info;
			std::tie(index_count, index_info) = generate_emulating_index_buffer(draw_clause, vertex_count, m_index_buffer_ring_info);
			return{ prims, false, 0, vertex_count - 1, index_count, 0, index_info };
		}

	private:
		vk::data_heap& m_index_buffer_ring_info;
		rsx::vertex_input_layout& m_vertex_layout;
	};
}

vk::vertex_upload_info VKGSRender::upload_vertex_data()
{
	draw_command_visitor visitor(m_index_buffer_ring_info, m_vertex_layout);
	auto result = std::visit(visitor, get_draw_command(rsx::method_registers));

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
	u32 persistent_range_base = UINT32_MAX, volatile_range_base = UINT32_MAX;
	size_t persistent_offset = UINT64_MAX, volatile_offset = UINT64_MAX;

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
			const auto data_offset = (vertex_base * m_vertex_layout.interleaved_blocks[0].attribute_stride);
			storage_address = m_vertex_layout.interleaved_blocks[0].real_offset_address + data_offset;

			if (auto cached = m_vertex_cache->find_vertex_range(storage_address, VK_FORMAT_R8_UINT, required.first))
			{
				verify(HERE), cached->local_address == storage_address;

				in_cache = true;
				persistent_range_base = cached->offset_in_heap;
			}
			else
			{
				to_store = true;
			}
		}

		if (!in_cache)
		{
			persistent_offset = static_cast<u32>(m_attrib_ring_info.alloc<256>(required.first));
			persistent_range_base = static_cast<u32>(persistent_offset);

			if (to_store)
			{
				//store ref in vertex cache
				m_vertex_cache->store_range(storage_address, VK_FORMAT_R8_UINT, required.first, static_cast<u32>(persistent_offset));
			}
		}
	}

	if (required.second > 0)
	{
		volatile_offset = static_cast<u32>(m_attrib_ring_info.alloc<256>(required.second));
		volatile_range_base = static_cast<u32>(volatile_offset);
	}

	//Write all the data once if possible
	if (required.first && required.second && volatile_offset > persistent_offset)
	{
		//Do this once for both to save time on map/unmap cycles
		const size_t block_end = (volatile_offset + required.second);
		const size_t block_size = block_end - persistent_offset;
		const size_t volatile_offset_in_block = volatile_offset - persistent_offset;

		void *block_mapping = m_attrib_ring_info.map(persistent_offset, block_size);
		write_vertex_data_to_memory(m_vertex_layout, vertex_base, vertex_count, block_mapping, static_cast<char*>(block_mapping) + volatile_offset_in_block);
		m_attrib_ring_info.unmap();
	}
	else
	{
		if (required.first > 0 && persistent_offset != UINT64_MAX)
		{
			void *persistent_mapping = m_attrib_ring_info.map(persistent_offset, required.first);
			write_vertex_data_to_memory(m_vertex_layout, vertex_base, vertex_count, persistent_mapping, nullptr);
			m_attrib_ring_info.unmap();
		}

		if (required.second > 0)
		{
			void *volatile_mapping = m_attrib_ring_info.map(volatile_offset, required.second);
			write_vertex_data_to_memory(m_vertex_layout, vertex_base, vertex_count, nullptr, volatile_mapping);
			m_attrib_ring_info.unmap();
		}
	}

	if (vk::test_status_interrupt(vk::heap_changed))
	{
		// Check for validity
		if (m_persistent_attribute_storage &&
			m_persistent_attribute_storage->info.buffer != m_attrib_ring_info.heap->value)
		{
			m_current_frame->buffer_views_to_clean.push_back(std::move(m_persistent_attribute_storage));
		}

		if (m_volatile_attribute_storage &&
			m_volatile_attribute_storage->info.buffer != m_attrib_ring_info.heap->value)
		{
			m_current_frame->buffer_views_to_clean.push_back(std::move(m_volatile_attribute_storage));
		}

		vk::clear_status_interrupt(vk::heap_changed);
	}

	if (persistent_range_base != UINT32_MAX)
	{
		if (!m_persistent_attribute_storage || !m_persistent_attribute_storage->in_range(persistent_range_base, required.first, persistent_range_base))
		{
			verify("Incompatible driver (MacOS?)" HERE), m_texbuffer_view_size >= required.first;

			if (m_persistent_attribute_storage)
				m_current_frame->buffer_views_to_clean.push_back(std::move(m_persistent_attribute_storage));

			//View 64M blocks at a time (different drivers will only allow a fixed viewable heap size, 64M should be safe)
			const size_t view_size = (persistent_range_base + m_texbuffer_view_size) > m_attrib_ring_info.size() ? m_attrib_ring_info.size() - persistent_range_base : m_texbuffer_view_size;
			m_persistent_attribute_storage = std::make_unique<vk::buffer_view>(*m_device, m_attrib_ring_info.heap->value, VK_FORMAT_R8_UINT, persistent_range_base, view_size);
			persistent_range_base = 0;
		}
	}

	if (volatile_range_base != UINT32_MAX)
	{
		if (!m_volatile_attribute_storage || !m_volatile_attribute_storage->in_range(volatile_range_base, required.second, volatile_range_base))
		{
			verify("Incompatible driver (MacOS?)" HERE), m_texbuffer_view_size >= required.second;

			if (m_volatile_attribute_storage)
				m_current_frame->buffer_views_to_clean.push_back(std::move(m_volatile_attribute_storage));

			const size_t view_size = (volatile_range_base + m_texbuffer_view_size) > m_attrib_ring_info.size() ? m_attrib_ring_info.size() - volatile_range_base : m_texbuffer_view_size;
			m_volatile_attribute_storage = std::make_unique<vk::buffer_view>(*m_device, m_attrib_ring_info.heap->value, VK_FORMAT_R8_UINT, volatile_range_base, view_size);
			volatile_range_base = 0;
		}
	}

	return{ result.native_primitive_type,                 // Primitive
			result.vertex_draw_count,                     // Vertex count
			vertex_count,                                 // Allocated vertex count
			vertex_base,                                  // First vertex in stream
			index_base,                                   // Index of vertex at data location 0
			result.vertex_index_offset,                   // Index offset
			persistent_range_base, volatile_range_base,   // Binding range
			result.index_info };                          // Index buffer info
}
