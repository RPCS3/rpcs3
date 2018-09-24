#include "stdafx.h"
#include "Emu/Memory/vm.h"
#include "Emu/System.h"
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
			fmt::throw_exception("Unsupported primitive topology 0x%x", (u8)mode);
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
		throw;
	}
}

namespace
{

	std::tuple<u32, std::tuple<VkDeviceSize, VkIndexType>> generate_emulating_index_buffer(
		const rsx::draw_clause& clause, u32 vertex_count,
		vk::vk_data_heap& m_index_buffer_ring_info)
	{
		u32 index_count = get_index_count(clause.primitive, vertex_count);
		u32 upload_size = index_count * sizeof(u16);

		VkDeviceSize offset_in_index_buffer = m_index_buffer_ring_info.alloc<256>(upload_size);
		void* buf = m_index_buffer_ring_info.map(offset_in_index_buffer, upload_size);

		write_index_array_for_non_indexed_non_native_primitive_to_buffer(
			reinterpret_cast<char*>(buf), clause.primitive, vertex_count);

		m_index_buffer_ring_info.unmap();
		return std::make_tuple(
			index_count, std::make_tuple(offset_in_index_buffer, VK_INDEX_TYPE_UINT16));
	}

	struct vertex_input_state
	{
		VkPrimitiveTopology native_primitive_type;
		u32 vertex_draw_count;
		u32 allocated_vertex_count;
		u32 vertex_data_base;
		u32 vertex_index_base;
		std::optional<std::tuple<VkDeviceSize, VkIndexType>> index_info;
	};

	struct draw_command_visitor
	{
		draw_command_visitor(vk::vk_data_heap& index_buffer_ring_info, rsx::vertex_input_layout& layout)
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

			if (primitives_emulated)
			{
				u32 index_count;
				std::optional<std::tuple<VkDeviceSize, VkIndexType>> index_info;

				std::tie(index_count, index_info) =
					generate_emulating_index_buffer(rsx::method_registers.current_draw_clause,
						vertex_count, m_index_buffer_ring_info);

				return{ prims, index_count, vertex_count, min_index, 0, index_info };
			}

			return{ prims, vertex_count, vertex_count, min_index, 0, {} };
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

			constexpr u32 type_size = sizeof(u32); // Force u32 index size dest to avoid overflows when adding vertex base index

			u32 index_count = rsx::method_registers.current_draw_clause.get_elements_count();
			if (primitives_emulated)
				index_count = get_index_count(rsx::method_registers.current_draw_clause.primitive, index_count);
			u32 upload_size = index_count * type_size;

			if (emulate_restart) upload_size *= 2;

			VkDeviceSize offset_in_index_buffer = m_index_buffer_ring_info.alloc<4>(upload_size);
			void* buf = m_index_buffer_ring_info.map(offset_in_index_buffer, upload_size);

			gsl::span<gsl::byte> dst;
			std::vector<gsl::byte> tmp;
			if (emulate_restart)
			{
				tmp.resize(upload_size);
				dst = tmp;
			}
			else
			{
				dst = gsl::span<gsl::byte>(static_cast<gsl::byte*>(buf), upload_size);
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
				rsx::method_registers.restart_index(), rsx::method_registers.current_draw_clause.draw_command_ranges,
				rsx::method_registers.vertex_data_base_index(), [](auto prim) { return !vk::is_primitive_native(prim); });

			if (min_index >= max_index)
			{
				//empty set, do not draw
				m_index_buffer_ring_info.unmap();
				return{ prims, 0, 0, 0, 0, {} };
			}

			if (emulate_restart)
			{
				index_count = rsx::remove_restart_index((u32*)buf, (u32*)tmp.data(), index_count, (u32)UINT32_MAX);
			}

			m_index_buffer_ring_info.unmap();

			std::optional<std::tuple<VkDeviceSize, VkIndexType>> index_info =
				std::make_tuple(offset_in_index_buffer, VK_INDEX_TYPE_UINT32);

			//check for vertex arrays with frequency modifiers
			for (auto &block : m_vertex_layout.interleaved_blocks)
			{
				if (block.min_divisor > 1)
				{
					//Ignore base offsets and return real results
					//The upload function will optimize the uploaded range anyway
					return{ prims, index_count, max_index, 0, 0, index_info };
				}
			}

			return {prims, index_count, (max_index - min_index + 1), min_index, min_index, index_info};
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
				return{ prims, vertex_count, vertex_count, 0, 0, {} };
			}

			u32 index_count;
			std::optional<std::tuple<VkDeviceSize, VkIndexType>> index_info;
			std::tie(index_count, index_info) = generate_emulating_index_buffer(draw_clause, vertex_count, m_index_buffer_ring_info);
			return{ prims, index_count, vertex_count, 0, 0, index_info };
		}

	private:
		vk::vk_data_heap& m_index_buffer_ring_info;
		rsx::vertex_input_layout& m_vertex_layout;
	};
}

vk::vertex_upload_info VKGSRender::upload_vertex_data()
{
	m_vertex_layout = analyse_inputs_interleaved();

	if (!m_vertex_layout.validate())
		return {};

	draw_command_visitor visitor(m_index_buffer_ring_info, m_vertex_layout);
	auto result = std::visit(visitor, get_draw_command(rsx::method_registers));

	auto &vertex_count = result.allocated_vertex_count;
	auto &vertex_base = result.vertex_data_base;

	//Do actual vertex upload
	auto required = calculate_memory_requirements(m_vertex_layout, vertex_count);
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
			storage_address = m_vertex_layout.interleaved_blocks[0].real_offset_address + vertex_base;
			if (auto cached = m_vertex_cache->find_vertex_range(storage_address, VK_FORMAT_R8_UINT, required.first))
			{
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
			persistent_offset = (u32)m_attrib_ring_info.alloc<256>(required.first);
			persistent_range_base = (u32)persistent_offset;

			if (to_store)
			{
				//store ref in vertex cache
				m_vertex_cache->store_range(storage_address, VK_FORMAT_R8_UINT, required.first, (u32)persistent_offset);
			}
		}
	}

	if (required.second > 0)
	{
		volatile_offset = (u32)m_attrib_ring_info.alloc<256>(required.second);
		volatile_range_base = (u32)volatile_offset;
	}

	//Write all the data once if possible
	if (required.first && required.second && volatile_offset > persistent_offset)
	{
		//Do this once for both to save time on map/unmap cycles
		const size_t block_end = (volatile_offset + required.second);
		const size_t block_size = block_end - persistent_offset;
		const size_t volatile_offset_in_block = volatile_offset - persistent_offset;

		void *block_mapping = m_attrib_ring_info.map(persistent_offset, block_size);
		write_vertex_data_to_memory(m_vertex_layout, vertex_base, vertex_count, block_mapping, (char*)block_mapping + volatile_offset_in_block);
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

	return{ result.native_primitive_type, result.vertex_draw_count, result.allocated_vertex_count, result.vertex_index_base, persistent_range_base, volatile_range_base, result.index_info };
}
