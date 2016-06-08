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
	u32 expand_line_loop_array_to_strip(u32 vertex_draw_count, std::vector<u16>& indices)
	{
		u32 i = 0;
		indices.resize(vertex_draw_count + 1);

		for (; i < vertex_draw_count; ++i)
			indices[i] = i;

		indices[i] = 0;
		return static_cast<u32>(indices.size());
	}

	template<typename T>
	u32 expand_indexed_line_loop_to_strip(u32 original_count, const T* original_indices, std::vector<T>& indices)
	{
		indices.resize(original_count + 1);

		u32 i = 0;
		for (; i < original_count; ++i)
			indices[i] = original_indices[i];

		indices[i] = original_indices[0];
		return static_cast<u32>(indices.size());
	}

	/**
	 * Template: Expand any N-compoent vector to a larger X-component vector and pad unused slots with 1
	 */
	template<typename T, u8 src_components, u8 dst_components, u32 padding>
	void expand_array_components(const T* src_data, std::vector<u8>& dst_data, u32 vertex_count)
	{
		u32 dst_size = (vertex_count * dst_components * sizeof(T));
		dst_data.resize(dst_size);

		T* src = const_cast<T*>(src_data);
		T* dst = reinterpret_cast<T*>(dst_data.data());

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
}

namespace
{
	struct data_heap_alloc
	{
		static size_t alloc_and_copy(vk::vk_data_heap& data_heap_info, size_t size, std::function<void(gsl::span<gsl::byte> ptr)> copy_function)
		{
			size_t offset = data_heap_info.alloc<256>(size);
			void* buf = data_heap_info.map(offset, size);
			gsl::span<gsl::byte> mapped_span = { reinterpret_cast<gsl::byte*>(buf), gsl::narrow<int>(size) };
			copy_function(mapped_span);
			data_heap_info.unmap();
			return offset;
		}
	};
}

std::tuple<VkPrimitiveTopology, bool, u32, VkDeviceSize, VkIndexType>
VKGSRender::upload_vertex_data()
{
	//initialize vertex attributes
	std::vector<u8> vertex_arrays_data;

	const std::string reg_table[] =
	{
		"in_pos_buffer", "in_weight_buffer", "in_normal_buffer",
		"in_diff_color_buffer", "in_spec_color_buffer",
		"in_fog_buffer",
		"in_point_size_buffer", "in_7_buffer",
		"in_tc0_buffer", "in_tc1_buffer", "in_tc2_buffer", "in_tc3_buffer",
		"in_tc4_buffer", "in_tc5_buffer", "in_tc6_buffer", "in_tc7_buffer"
	};

	u32 input_mask = rsx::method_registers[NV4097_SET_VERTEX_ATTRIB_INPUT_MASK];

	std::vector<u8> vertex_index_array;
	vertex_draw_count = 0;
	u32 min_index, max_index;

	if (draw_command == rsx::draw_command::indexed)
	{
		rsx::index_array_type type = rsx::to_index_array_type(rsx::method_registers[NV4097_SET_INDEX_ARRAY_DMA] >> 4);
		u32 type_size = gsl::narrow<u32>(get_index_type_size(type));
		for (const auto& first_count : first_count_commands)
		{
			vertex_draw_count += first_count.second;
		}

		vertex_index_array.resize(vertex_draw_count * type_size);

		switch (type)
		{
		case rsx::index_array_type::u32:
			std::tie(min_index, max_index) = write_index_array_data_to_buffer_untouched(gsl::span<u32>((u32*)vertex_index_array.data(), vertex_draw_count), first_count_commands);
			break;
		case rsx::index_array_type::u16:
			std::tie(min_index, max_index) = write_index_array_data_to_buffer_untouched(gsl::span<u16>((u16*)vertex_index_array.data(), vertex_draw_count), first_count_commands);
			break;
		}
	}

	if (draw_command == rsx::draw_command::inlined_array)
	{
		u32 stride = 0;
		u32 offsets[rsx::limits::vertex_count] = { 0 };

		for (u32 i = 0; i < rsx::limits::vertex_count; ++i)
		{
			const auto &info = vertex_arrays_info[i];
			if (!info.size) continue;

			offsets[i] = stride;
			stride += rsx::get_vertex_type_size_on_host(info.type, info.size);
		}

		vertex_draw_count = (u32)(inline_vertex_array.size() * sizeof(u32)) / stride;

		for (int index = 0; index < rsx::limits::vertex_count; ++index)
		{
			auto &vertex_info = vertex_arrays_info[index];

			if (!m_program->has_uniform(reg_table[index]))
				continue;

			if (!vertex_info.size) // disabled
			{
				continue;
			}

			const u32 element_size = vk::get_suitable_vk_size(vertex_info.type, vertex_info.size);
			const u32 data_size = element_size * vertex_draw_count;
			const VkFormat format = vk::get_suitable_vk_format(vertex_info.type, vertex_info.size);

			vertex_arrays_data.resize(data_size);
			u8 *src = reinterpret_cast<u8*>(inline_vertex_array.data());
			u8 *dst = vertex_arrays_data.data();

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

			size_t offset_in_attrib_buffer = data_heap_alloc::alloc_and_copy(m_attrib_ring_info, data_size, [&vertex_arrays_data, data_size](gsl::span<gsl::byte> ptr) { memcpy(ptr.data(), vertex_arrays_data.data(), data_size); });

			m_buffer_view_to_clean.push_back(std::make_unique<vk::buffer_view>(*m_device, m_attrib_ring_info.heap->value, format, offset_in_attrib_buffer, data_size));
			m_program->bind_uniform(m_buffer_view_to_clean.back()->value, reg_table[index], descriptor_sets);
		}
	}

	if (draw_command == rsx::draw_command::array)
	{
		for (const auto &first_count : first_count_commands)
		{
			vertex_draw_count += first_count.second;
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

			if (vertex_arrays_info[index].size > 0)
			{
				auto &vertex_info = vertex_arrays_info[index];
				// Active vertex array
				std::vector<gsl::byte> vertex_array;

				// Fill vertex_array
				u32 element_size = rsx::get_vertex_type_size_on_host(vertex_info.type, vertex_info.size);
				vertex_array.resize(vertex_draw_count * element_size);

				// Get source pointer
				u32 base_offset = rsx::method_registers[NV4097_SET_VERTEX_DATA_BASE_OFFSET];
				u32 offset = rsx::method_registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + index];
				u32 address = base_offset + rsx::get_address(offset & 0x7fffffff, offset >> 31);
				const gsl::byte *src_ptr = gsl::narrow_cast<const gsl::byte*>(vm::base(address));

				u32 num_stored_verts = vertex_draw_count;

				if (draw_command == rsx::draw_command::array)
				{
					size_t offset = 0;
					gsl::span<gsl::byte> dest_span(vertex_array);
					vk::prepare_buffer_for_writing(vertex_array.data(), vertex_info.type, vertex_info.size, vertex_draw_count);

					for (const auto &first_count : first_count_commands)
					{
						write_vertex_array_data_to_buffer(dest_span.subspan(offset), src_ptr, first_count.first, first_count.second, vertex_info.type, vertex_info.size, vertex_info.stride, element_size);
						offset += first_count.second * element_size;
					}
				}
				if (draw_command == rsx::draw_command::indexed)
				{
					num_stored_verts = (max_index + 1);
					vertex_array.resize((max_index + 1) * element_size);
					gsl::span<gsl::byte> dest_span(vertex_array);
					vk::prepare_buffer_for_writing(vertex_array.data(), vertex_info.type, vertex_info.size, vertex_draw_count);

					write_vertex_array_data_to_buffer(dest_span, src_ptr, 0, max_index + 1, vertex_info.type, vertex_info.size, vertex_info.stride, element_size);
				}

				std::vector<u8> converted_buffer;
				void *data_ptr = vertex_array.data();

				if (vk::requires_component_expansion(vertex_info.type, vertex_info.size))
				{
					switch (vertex_info.type)
					{
					case rsx::vertex_base_type::f:
						vk::expand_array_components<float, 3, 4, 1>(reinterpret_cast<float*>(vertex_array.data()), converted_buffer, num_stored_verts);
						break;
					}

					data_ptr = static_cast<void*>(converted_buffer.data());
				}

				const VkFormat format = vk::get_suitable_vk_format(vertex_info.type, vertex_info.size);
				const u32 data_size = vk::get_suitable_vk_size(vertex_info.type, vertex_info.size) * num_stored_verts;

				size_t offset_in_attrib_buffer = data_heap_alloc::alloc_and_copy(m_attrib_ring_info, data_size, [data_ptr, data_size](gsl::span<gsl::byte> ptr) { memcpy(ptr.data(), data_ptr, data_size); });
				m_buffer_view_to_clean.push_back(std::make_unique<vk::buffer_view>(*m_device, m_attrib_ring_info.heap->value, format, offset_in_attrib_buffer, data_size));
				m_program->bind_uniform(m_buffer_view_to_clean.back()->value, reg_table[index], descriptor_sets);
			}
			else if (register_vertex_info[index].size > 0)
			{
				//Untested!
				auto &vertex_data = register_vertex_data[index];
				auto &vertex_info = register_vertex_info[index];

				switch (vertex_info.type)
				{
				case rsx::vertex_base_type::f:
				{
					size_t data_size = vertex_data.size();
					const VkFormat format = vk::get_suitable_vk_format(vertex_info.type, vertex_info.size);

					std::vector<u8> converted_buffer;
					void *data_ptr = vertex_data.data();

					if (vk::requires_component_expansion(vertex_info.type, vertex_info.size))
					{
						switch (vertex_info.type)
						{
						case rsx::vertex_base_type::f:
						{
							const u32 num_stored_verts = static_cast<u32>(data_size / (sizeof(float) * vertex_info.size));
							vk::expand_array_components<float, 3, 4, 1>(reinterpret_cast<float*>(vertex_data.data()), converted_buffer, num_stored_verts);
							break;
						}
						}

						data_ptr = static_cast<void*>(converted_buffer.data());
						data_size = converted_buffer.size();
					}


					size_t offset_in_attrib_buffer = data_heap_alloc::alloc_and_copy(m_attrib_ring_info, data_size, [data_ptr, data_size](gsl::span<gsl::byte> ptr) { memcpy(ptr.data(), data_ptr, data_size); });
					m_buffer_view_to_clean.push_back(std::make_unique<vk::buffer_view>(*m_device, m_attrib_ring_info.heap->value, format, offset_in_attrib_buffer, data_size));
					m_program->bind_uniform(m_buffer_view_to_clean.back()->value, reg_table[index], descriptor_sets);
					break;
				}
				default:
					LOG_ERROR(RSX, "bad non array vertex data format (type = %d, size = %d)", vertex_info.type, vertex_info.size);
					break;
				}
			}
		}
	}

	bool is_indexed_draw = (draw_command == rsx::draw_command::indexed);
	bool index_buffer_filled = false;
	bool primitives_emulated = false;
	u32  index_count = vertex_draw_count;

	VkIndexType index_format = VK_INDEX_TYPE_UINT16;
	VkPrimitiveTopology prims = vk::get_appropriate_topology(draw_mode, primitives_emulated);

	size_t offset_in_index_buffer = -1;

	if (primitives_emulated)
	{
		//Line loops are line-strips with loop-back; using line-strips-with-adj doesnt work for vulkan
		if (draw_mode == rsx::primitive_type::line_loop)
		{
			std::vector<u16> indices;

			if (!is_indexed_draw)
			{
				index_count = vk::expand_line_loop_array_to_strip(vertex_draw_count, indices);
				size_t upload_size = index_count * sizeof(u16);
				offset_in_index_buffer = m_index_buffer_ring_info.alloc<256>(upload_size);
				void* buf = m_index_buffer_ring_info.heap->map(offset_in_index_buffer, upload_size);
				memcpy(buf, indices.data(), upload_size);
				m_index_buffer_ring_info.heap->unmap();
			}
			else
			{
				rsx::index_array_type indexed_type = rsx::to_index_array_type(rsx::method_registers[NV4097_SET_INDEX_ARRAY_DMA] >> 4);
				if (indexed_type == rsx::index_array_type::u32)
				{
					index_format = VK_INDEX_TYPE_UINT32;
					std::vector<u32> indices32;

					index_count = vk::expand_indexed_line_loop_to_strip(vertex_draw_count, (u32*)vertex_index_array.data(), indices32);
					size_t upload_size = index_count * sizeof(u32);
					offset_in_index_buffer = m_index_buffer_ring_info.alloc<256>(upload_size);
					void* buf = m_index_buffer_ring_info.heap->map(offset_in_index_buffer, upload_size);
					memcpy(buf, indices32.data(), upload_size);
					m_index_buffer_ring_info.heap->unmap();
				}
				else
				{
					index_count = vk::expand_indexed_line_loop_to_strip(vertex_draw_count, (u16*)vertex_index_array.data(), indices);
					size_t upload_size = index_count * sizeof(u16);
					offset_in_index_buffer = m_index_buffer_ring_info.alloc<256>(upload_size);
					void* buf = m_index_buffer_ring_info.heap->map(offset_in_index_buffer, upload_size);
					memcpy(buf, indices.data(), upload_size);
					m_index_buffer_ring_info.heap->unmap();
				}
			}
		}
		else
		{
			index_count = static_cast<u32>(get_index_count(draw_mode, vertex_draw_count));
			std::vector<u16> indices(index_count);

			if (is_indexed_draw)
			{
				rsx::index_array_type indexed_type = rsx::to_index_array_type(rsx::method_registers[NV4097_SET_INDEX_ARRAY_DMA] >> 4);
				size_t index_size = get_index_type_size(indexed_type);

				std::vector<std::pair<u32, u32>> ranges;
				ranges.push_back(std::pair<u32, u32>(0, vertex_draw_count));

				gsl::span<gsl::byte> dst = { (gsl::byte*)indices.data(), gsl::narrow<int>(index_count * 2) };
				write_index_array_data_to_buffer(dst, rsx::index_array_type::u16, draw_mode, ranges);
			}
			else
			{
				write_index_array_for_non_indexed_non_native_primitive_to_buffer(reinterpret_cast<char*>(indices.data()), draw_mode, 0, vertex_draw_count);
			}

			size_t upload_size = index_count * sizeof(u16);
			offset_in_index_buffer = m_index_buffer_ring_info.alloc<256>(upload_size);
			void* buf = m_index_buffer_ring_info.heap->map(offset_in_index_buffer, upload_size);
			memcpy(buf, indices.data(), upload_size);
			m_index_buffer_ring_info.heap->unmap();
		}

		is_indexed_draw = true;
		index_buffer_filled = true;
	}

	if (!index_buffer_filled && is_indexed_draw)
	{
		rsx::index_array_type indexed_type = rsx::to_index_array_type(rsx::method_registers[NV4097_SET_INDEX_ARRAY_DMA] >> 4);
		index_format = VK_INDEX_TYPE_UINT16;
		VkFormat fmt = VK_FORMAT_R16_UINT;
		
		u32 elem_size = static_cast<u32>(get_index_type_size(indexed_type));

		if (indexed_type == rsx::index_array_type::u32)
		{
			index_format = VK_INDEX_TYPE_UINT32;
			fmt = VK_FORMAT_R32_UINT;
		}

		u32 index_sz = static_cast<u32>(vertex_index_array.size()) / elem_size;
		if (index_sz != vertex_draw_count)
			LOG_ERROR(RSX, "Vertex draw count mismatch!");

		size_t upload_size = vertex_index_array.size();
		offset_in_index_buffer = m_index_buffer_ring_info.alloc<256>(upload_size);
		void* buf = m_index_buffer_ring_info.heap->map(offset_in_index_buffer, upload_size);
		memcpy(buf, vertex_index_array.data(), upload_size);
		m_index_buffer_ring_info.heap->unmap();
	}

	return std::make_tuple(prims, is_indexed_draw, index_count, offset_in_index_buffer, index_format);
}

