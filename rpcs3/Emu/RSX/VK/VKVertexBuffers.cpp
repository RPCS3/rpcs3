#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "VKGSRender.h"
#include "../rsx_methods.h"
#include "../Common/BufferUtils.h"

namespace vk
{
	bool requires_component_expansion(rsx::vertex_base_type type, u32 size)
	{
		if (size == 3)
		{
			switch (type)
			{
			case rsx::vertex_base_type::f:
				return true;
			}
		}

		return false;
	}

	u32 get_suitable_vk_size(rsx::vertex_base_type type, u32 size)
	{
		if (size == 3)
		{
			switch (type)
			{
			case rsx::vertex_base_type::f:
				return 16;
			}
		}

		return rsx::get_vertex_type_size_on_host(type, size);
	}

	VkFormat get_suitable_vk_format(rsx::vertex_base_type type, u8 size)
	{
		/**
		* Set up buffer fetches to only work on 4-component access. This is hardware dependant so we use 4-component access to avoid branching based on IHV implementation
		* AMD GCN 1.0 for example does not support RGB32 formats for texel buffers
		*/
		const VkFormat vec1_types[] = { VK_FORMAT_R16_UNORM, VK_FORMAT_R32_SFLOAT, VK_FORMAT_R16_SFLOAT, VK_FORMAT_R8_UNORM, VK_FORMAT_R16_SINT, VK_FORMAT_R16_SFLOAT, VK_FORMAT_R8_UINT };
		const VkFormat vec2_types[] = { VK_FORMAT_R16G16_UNORM, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R16G16_SFLOAT, VK_FORMAT_R8G8_UNORM, VK_FORMAT_R16G16_SINT, VK_FORMAT_R16G16_SFLOAT, VK_FORMAT_R8G8_UINT };
		const VkFormat vec3_types[] = { VK_FORMAT_R16G16B16A16_UNORM, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R16G16B16A16_SINT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R8G8B8A8_UINT };	//VEC3 COMPONENTS NOT SUPPORTED!
		const VkFormat vec4_types[] = { VK_FORMAT_R16G16B16A16_UNORM, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R16G16B16A16_SINT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R8G8B8A8_UINT };

		const VkFormat* vec_selectors[] = { 0, vec1_types, vec2_types, vec3_types, vec4_types };

		if (type > rsx::vertex_base_type::ub256)
			throw EXCEPTION("VKGS error: unknown vertex base type 0x%X.", (u32)type);

		return vec_selectors[size][(int)type];
	}

	VkPrimitiveTopology get_appropriate_topology(rsx::primitive_type& mode, bool &requires_modification)
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
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		case rsx::primitive_type::triangle_fan:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
		case rsx::primitive_type::quads:
		case rsx::primitive_type::quad_strip:
		case rsx::primitive_type::polygon:
			requires_modification = true;
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		default:
			throw ("Unsupported primitive topology 0x%X", (u8)mode);
		}
	}

	/**
	* Expand line loop array to line strip array; simply loop back the last vertex to the first..
	*/
	void expand_line_loop_array_to_strip(u32 vertex_draw_count, u16* indices)
	{
		u32 i = 0;

		for (; i < vertex_draw_count; ++i)
			indices[i] = i;

		indices[i] = 0;
	}

	template <typename T, u32 padding>
	void copy_inlined_data_to_buffer(void *src_data, void *dst_data, u32 vertex_count, rsx::vertex_base_type type, u8 src_channels, u8 dst_channels, u16 element_size, u16 stride)
	{
		u8 *src = static_cast<u8*>(src_data);
		u8 *dst = static_cast<u8*>(dst_data);

		for (u32 i = 0; i < vertex_count; ++i)
		{
			T* src_ptr = reinterpret_cast<T*>(src);
			T* dst_ptr = reinterpret_cast<T*>(dst);

			switch (type)
			{
			case rsx::vertex_base_type::ub:
			{
				if (src_channels == 4)
				{
					dst[0] = src[3];
					dst[1] = src[2];
					dst[2] = src[1];
					dst[3] = src[0];

					break;
				}
			}
			default:
			{
				for (u8 ch = 0; ch < dst_channels; ++ch)
				{
					if (ch < src_channels)
					{
						*dst_ptr = *src_ptr;
						src_ptr++;
					}
					else
						*dst_ptr = (T)(padding);

					dst_ptr++;
				}
			}
			}

			src += stride;
			dst += element_size;
		}
	}

	void prepare_buffer_for_writing(void *data, rsx::vertex_base_type type, u8 vertex_size, u32 vertex_count)
	{
		switch (type)
		{
		case rsx::vertex_base_type::f:
		{
			if (vertex_size == 3)
			{
				float *dst = reinterpret_cast<float*>(data);
				for (u32 i = 0, idx = 3; i < vertex_count; ++i, idx += 4)
					dst[idx] = 1.f;
			}

			break;
		}
		case rsx::vertex_base_type::sf:
		{
			if (vertex_size == 3)
			{
				/**
				* Pad the 4th component for half-float arrays to 1, since texelfetch does not mask components
				*/
				u16 *dst = reinterpret_cast<u16*>(data);
				for (u32 i = 0, idx = 3; i < vertex_count; ++i, idx += 4)
					dst[idx] = 0x3c00;
			}

			break;
		}
		}
	}

	/**
	* Template: Expand any N-compoent vector to a larger X-component vector and pad unused slots with 1
	*/
	template<typename T, u8 src_components, u8 dst_components, u32 padding>
	void expand_array_components(const T* src_data, void *dst_ptr, u32 vertex_count)
	{
		T* src = const_cast<T*>(src_data);
		T* dst = static_cast<T*>(dst_ptr);

		for (u32 index = 0; index < vertex_count; ++index)
		{
			for (u8 channel = 0; channel < dst_components; channel++)
			{
				if (channel < src_components)
				{
					*dst = *src;

					dst++;
					src++;
				}
				else
				{
					*dst = (T)(padding);
					dst++;
				}
			}
		}
	}

	u32 get_emulated_index_array_size(rsx::primitive_type type, u32 vertex_count)
	{
		switch (type)
		{
		case rsx::primitive_type::line_loop:
			return vertex_count + 1;
		default:
			return static_cast<u32>(get_index_count(type, vertex_count));
		}
	}

	std::tuple<u32, u32, VkIndexType> upload_index_buffer(rsx::primitive_type type, rsx::index_array_type index_type, void *dst_ptr, bool indexed_draw, u32 vertex_count, u32 index_count, std::vector<std::pair<u32, u32>> first_count_commands)
	{
		bool emulated = false;
		get_appropriate_topology(type, emulated);

		u32 min_index, max_index;

		if (!emulated)
		{
			switch (index_type)
			{
			case rsx::index_array_type::u32:
				std::tie(min_index, max_index) = write_index_array_data_to_buffer_untouched(gsl::span<u32>((u32*)dst_ptr, vertex_count), first_count_commands);
				return std::make_tuple(min_index, max_index, VK_INDEX_TYPE_UINT32);
			case rsx::index_array_type::u16:
				std::tie(min_index, max_index) = write_index_array_data_to_buffer_untouched(gsl::span<u16>((u16*)dst_ptr, vertex_count), first_count_commands);
				return std::make_tuple(min_index, max_index, VK_INDEX_TYPE_UINT16);
			}
		}

		switch (type)
		{
		case rsx::primitive_type::line_loop:
		{
			if (!indexed_draw)
			{
				expand_line_loop_array_to_strip(vertex_count, static_cast<u16*>(dst_ptr));
				return std::make_tuple(0, vertex_count-1, VK_INDEX_TYPE_UINT16);
			}

			VkIndexType vk_index_type = VK_INDEX_TYPE_UINT16;

			switch (index_type)
			{
			case rsx::index_array_type::u32:
			{
				u32 *idx_ptr = static_cast<u32*>(dst_ptr);
				std::tie(min_index, max_index) = write_index_array_data_to_buffer_untouched(gsl::span<u32>(idx_ptr, vertex_count), first_count_commands);
				idx_ptr[vertex_count] = idx_ptr[0];
				vk_index_type = VK_INDEX_TYPE_UINT32;
				break;
			}
			case rsx::index_array_type::u16:
			{
				u16 *idx_ptr = static_cast<u16*>(dst_ptr);
				std::tie(min_index, max_index) = write_index_array_data_to_buffer_untouched(gsl::span<u16>(idx_ptr, vertex_count), first_count_commands);
				idx_ptr[vertex_count] = idx_ptr[0];
				break;
			}
			}

			return std::make_tuple(min_index, max_index, vk_index_type);
		}
		default:
		{
			if (indexed_draw)
			{
				std::tie(min_index, max_index) = write_index_array_data_to_buffer(gsl::span<gsl::byte>(static_cast<gsl::byte*>(dst_ptr), index_count * 2), rsx::index_array_type::u16, type, first_count_commands);
				return std::make_tuple(min_index, max_index, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				write_index_array_for_non_indexed_non_native_primitive_to_buffer(reinterpret_cast<char*>(dst_ptr), type, 0, vertex_count);
				return std::make_tuple(0, vertex_count-1, VK_INDEX_TYPE_UINT16);
			}
		}
		}
	}
}

std::tuple<VkPrimitiveTopology, bool, u32, VkDeviceSize, VkIndexType>
VKGSRender::upload_vertex_data()
{
	//initialize vertex attributes
	const std::string reg_table[] =
	{
		"in_pos_buffer", "in_weight_buffer", "in_normal_buffer",
		"in_diff_color_buffer", "in_spec_color_buffer",
		"in_fog_buffer",
		"in_point_size_buffer", "in_7_buffer",
		"in_tc0_buffer", "in_tc1_buffer", "in_tc2_buffer", "in_tc3_buffer",
		"in_tc4_buffer", "in_tc5_buffer", "in_tc6_buffer", "in_tc7_buffer"
	};

	u32 input_mask = rsx::method_registers.vertex_attrib_input_mask();

	size_t offset_in_index_buffer = -1;
	vertex_draw_count = 0;
	u32 min_index, max_index;

	bool is_indexed_draw = (draw_command == rsx::draw_command::indexed);
	bool primitives_emulated = false;
	u32  index_count = 0;

	VkIndexType index_format = VK_INDEX_TYPE_UINT16;
	VkPrimitiveTopology prims = vk::get_appropriate_topology(draw_mode, primitives_emulated);

	if (draw_command == rsx::draw_command::array)
	{
		for (const auto &first_count : first_count_commands)
		{
			vertex_draw_count += first_count.second;
		}
	}

	if (draw_command == rsx::draw_command::indexed || primitives_emulated)
	{
		rsx::index_array_type type = rsx::method_registers.index_type();
		u32 type_size = gsl::narrow<u32>(get_index_type_size(type));
		
		if (is_indexed_draw)	//Could be emulated or not, emulated array vertex count already computed above
		{
			for (const auto& first_count : first_count_commands)
			{
				vertex_draw_count += first_count.second;
			}
		}

		index_count = vertex_draw_count;
		u32 upload_size = vertex_draw_count * type_size;

		std::vector<std::pair<u32, u32>> ranges = first_count_commands;

		if (primitives_emulated)
		{
			index_count = vk::get_emulated_index_array_size(draw_mode, vertex_draw_count);
			upload_size = index_count * sizeof(u16);

			if (is_indexed_draw)
			{
				ranges.resize(0);
				ranges.push_back(std::pair<u32, u32>(0, vertex_draw_count));
			}
		}
		
		offset_in_index_buffer = m_index_buffer_ring_info.alloc<256>(upload_size);
		void* buf = m_index_buffer_ring_info.map(offset_in_index_buffer, upload_size);

		std::tie(min_index, max_index, index_format) = vk::upload_index_buffer(draw_mode, type, buf, is_indexed_draw, vertex_draw_count, index_count, ranges);

		m_index_buffer_ring_info.unmap();
		is_indexed_draw = true;
	}

	if (draw_command == rsx::draw_command::inlined_array)
	{
		u32 stride = 0;
		u32 offsets[rsx::limits::vertex_count] = { 0 };

		for (u32 i = 0; i < rsx::limits::vertex_count; ++i)
		{
			const auto &info = rsx::method_registers.vertex_arrays_info[i];
			if (!info.size) continue;

			offsets[i] = stride;
			stride += rsx::get_vertex_type_size_on_host(info.type, info.size);
		}

		vertex_draw_count = (u32)(inline_vertex_array.size() * sizeof(u32)) / stride;

		for (int index = 0; index < rsx::limits::vertex_count; ++index)
		{
			auto &vertex_info = rsx::method_registers.vertex_arrays_info[index];

			if (!m_program->has_uniform(reg_table[index]))
				continue;

			if (!vertex_info.size) // disabled
			{
				continue;
			}

			const u32 element_size = vk::get_suitable_vk_size(vertex_info.type, vertex_info.size);
			const u32 data_size = element_size * vertex_draw_count;
			const VkFormat format = vk::get_suitable_vk_format(vertex_info.type, vertex_info.size);

			u32 offset_in_attrib_buffer = m_attrib_ring_info.alloc<256>(data_size);
			u8 *src = reinterpret_cast<u8*>(inline_vertex_array.data());
			u8 *dst = static_cast<u8*>(m_attrib_ring_info.map(offset_in_attrib_buffer, data_size));

			src += offsets[index];
			u8 opt_size = vertex_info.size;

			if (vertex_info.size == 3)
				opt_size = 4;

			//TODO: properly handle cmp type
			if (vertex_info.type == rsx::vertex_base_type::cmp)
				LOG_ERROR(RSX, "Compressed vertex attributes not supported for inlined arrays yet");

			switch (vertex_info.type)
			{
			case rsx::vertex_base_type::f:
				vk::copy_inlined_data_to_buffer<float, 1>(src, dst, vertex_draw_count, vertex_info.type, vertex_info.size, opt_size, element_size, stride);
				break;
			case rsx::vertex_base_type::sf:
				vk::copy_inlined_data_to_buffer<u16, 0x3c00>(src, dst, vertex_draw_count, vertex_info.type, vertex_info.size, opt_size, element_size, stride);
				break;
			case rsx::vertex_base_type::s1:
			case rsx::vertex_base_type::ub:
			case rsx::vertex_base_type::ub256:
				vk::copy_inlined_data_to_buffer<u8, 1>(src, dst, vertex_draw_count, vertex_info.type, vertex_info.size, opt_size, element_size, stride);
				break;
			case rsx::vertex_base_type::s32k:
			case rsx::vertex_base_type::cmp:
				vk::copy_inlined_data_to_buffer<u16, 1>(src, dst, vertex_draw_count, vertex_info.type, vertex_info.size, opt_size, element_size, stride);
				break;
			default:
				throw EXCEPTION("Unknown base type %d", vertex_info.type);
			}

			m_attrib_ring_info.unmap();
			m_buffer_view_to_clean.push_back(std::make_unique<vk::buffer_view>(*m_device, m_attrib_ring_info.heap->value, format, offset_in_attrib_buffer, data_size));
			m_program->bind_uniform(m_buffer_view_to_clean.back()->value, reg_table[index], descriptor_sets);
		}
	}

	if (draw_command == rsx::draw_command::array || draw_command == rsx::draw_command::indexed)
	{
		for (int index = 0; index < rsx::limits::vertex_count; ++index)
		{
			bool enabled = !!(input_mask & (1 << index));

			if (!m_program->has_uniform(reg_table[index]))
				continue;

			if (!enabled)
			{
				continue;
			}

			if (rsx::method_registers.vertex_arrays_info[index].size > 0)
			{
				auto &vertex_info = rsx::method_registers.vertex_arrays_info[index];

				// Fill vertex_array
				u32 element_size = rsx::get_vertex_type_size_on_host(vertex_info.type, vertex_info.size);
				u32 real_element_size = vk::get_suitable_vk_size(vertex_info.type, vertex_info.size);

				u32 upload_size = real_element_size * vertex_draw_count;
				u32 offset_in_attrib_buffer = 0;
				bool requires_expansion = vk::requires_component_expansion(vertex_info.type, vertex_info.size);

				// Get source pointer
				u32 base_offset = rsx::method_registers.vertex_data_base_offset();
				u32 offset = rsx::method_registers.vertex_arrays_info[index].offset();
				u32 address = base_offset + rsx::get_address(offset & 0x7fffffff, offset >> 31);
				const gsl::byte *src_ptr = gsl::narrow_cast<const gsl::byte*>(vm::base(address));

				u32 num_stored_verts = vertex_draw_count;

				if (draw_command == rsx::draw_command::array)
				{
					size_t offset = 0;
					offset_in_attrib_buffer = m_attrib_ring_info.alloc<256>(upload_size);
					void *dst = m_attrib_ring_info.map(offset_in_attrib_buffer, upload_size);
					vk::prepare_buffer_for_writing(dst, vertex_info.type, vertex_info.size, vertex_draw_count);
					
					gsl::span<gsl::byte> dest_span(static_cast<gsl::byte*>(dst), upload_size);
					
					for (const auto &first_count : first_count_commands)
					{
						write_vertex_array_data_to_buffer(dest_span.subspan(offset), src_ptr, first_count.first, first_count.second, vertex_info.type, vertex_info.size, vertex_info.stride, real_element_size);
						offset += first_count.second * real_element_size;
					}

					m_attrib_ring_info.unmap();
				}

				if (draw_command == rsx::draw_command::indexed)
				{
					num_stored_verts = (max_index + 1);
					upload_size = real_element_size * num_stored_verts;
					offset_in_attrib_buffer = m_attrib_ring_info.alloc<256>(upload_size);
					void *dst = m_attrib_ring_info.map(offset_in_attrib_buffer, upload_size);
					
					gsl::span<gsl::byte> dest_span(static_cast<gsl::byte*>(dst), upload_size);
					vk::prepare_buffer_for_writing(dst, vertex_info.type, vertex_info.size, num_stored_verts);

					write_vertex_array_data_to_buffer(dest_span, src_ptr, 0, max_index + 1, vertex_info.type, vertex_info.size, vertex_info.stride, real_element_size);
					m_attrib_ring_info.unmap();
				}

				const VkFormat format = vk::get_suitable_vk_format(vertex_info.type, vertex_info.size);

				m_buffer_view_to_clean.push_back(std::make_unique<vk::buffer_view>(*m_device, m_attrib_ring_info.heap->value, format, offset_in_attrib_buffer, upload_size));
				m_program->bind_uniform(m_buffer_view_to_clean.back()->value, reg_table[index], descriptor_sets);
			}
			else if (rsx::method_registers.register_vertex_info[index].size > 0)
			{
				//Untested!
				auto &vertex_info = rsx::method_registers.register_vertex_info[index];

				switch (vertex_info.type)
				{
				case rsx::vertex_base_type::f:
				{
					size_t data_size = rsx::get_vertex_type_size_on_host(vertex_info.type, vertex_info.size);
					const VkFormat format = vk::get_suitable_vk_format(vertex_info.type, vertex_info.size);

					u32 offset_in_attrib_buffer = 0;
					void *data_ptr = vertex_info.data.data();

					if (vk::requires_component_expansion(vertex_info.type, vertex_info.size))
					{
						const u32 num_stored_verts = static_cast<u32>(data_size / (sizeof(float) * vertex_info.size));
						const u32 real_element_size = vk::get_suitable_vk_size(vertex_info.type, vertex_info.size);

						data_size = real_element_size * num_stored_verts;
						offset_in_attrib_buffer = m_attrib_ring_info.alloc<256>(data_size);
						void *dst = m_attrib_ring_info.map(offset_in_attrib_buffer, data_size);

						vk::expand_array_components<float, 3, 4, 1>(reinterpret_cast<float*>(vertex_info.data.data()), dst, num_stored_verts);
						m_attrib_ring_info.unmap();
					}
					else
					{
						offset_in_attrib_buffer = m_attrib_ring_info.alloc<256>(data_size);
						void *dst = m_attrib_ring_info.map(offset_in_attrib_buffer, data_size);
						memcpy(dst, vertex_info.data.data(), data_size);
						m_attrib_ring_info.unmap();
					}

					m_buffer_view_to_clean.push_back(std::make_unique<vk::buffer_view>(*m_device, m_attrib_ring_info.heap->value, format, offset_in_attrib_buffer, data_size));
					m_program->bind_uniform(m_buffer_view_to_clean.back()->value, reg_table[index], descriptor_sets);
					break;
				}
				default:
					LOG_ERROR(RSX, "bad non array vertex data format (type = %d, size = %d)", vertex_info.type, vertex_info.size);
					break;
				}
			}
			else
			{
				//This section should theoretically be unreachable (data stream without available data)
				//Variable is defined in the shaders but no data is available
				//Bind a buffer view to keep the driver from crashing if access is attempted.
				
				u32 offset_in_attrib_buffer = m_attrib_ring_info.alloc<256>(32);
				void *dst = m_attrib_ring_info.map(offset_in_attrib_buffer, 32);
				memset(dst, 0, 32);
				m_attrib_ring_info.unmap();

				m_buffer_view_to_clean.push_back(std::make_unique<vk::buffer_view>(*m_device, m_attrib_ring_info.heap->value, VK_FORMAT_R32_SFLOAT, offset_in_attrib_buffer, 32));
				m_program->bind_uniform(m_buffer_view_to_clean.back()->value, reg_table[index], descriptor_sets);
			}
		}
	}
	
	return std::make_tuple(prims, is_indexed_draw, index_count, offset_in_index_buffer, index_format);
}

