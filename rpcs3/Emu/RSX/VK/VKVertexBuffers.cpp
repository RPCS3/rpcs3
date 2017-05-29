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
		const VkFormat vec1_types[] = { VK_FORMAT_R16_SNORM, VK_FORMAT_R32_SFLOAT, VK_FORMAT_R16_SFLOAT, VK_FORMAT_R8_UNORM, VK_FORMAT_R16_SINT, VK_FORMAT_R16G16B16A16_SNORM, VK_FORMAT_R8_UINT };
		const VkFormat vec2_types[] = { VK_FORMAT_R16G16_SNORM, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R16G16_SFLOAT, VK_FORMAT_R8G8_UNORM, VK_FORMAT_R16G16_SINT, VK_FORMAT_R16G16B16A16_SNORM, VK_FORMAT_R8G8_UINT };
		const VkFormat vec3_types[] = { VK_FORMAT_R16G16B16A16_SNORM, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R16G16B16A16_SINT, VK_FORMAT_R16G16B16A16_SNORM, VK_FORMAT_R8G8B8A8_UINT };	//VEC3 COMPONENTS NOT SUPPORTED!
		const VkFormat vec4_types[] = { VK_FORMAT_R16G16B16A16_SNORM, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R16G16B16A16_SINT, VK_FORMAT_R16G16B16A16_SNORM, VK_FORMAT_R8G8B8A8_UINT };

		const VkFormat* vec_selectors[] = { 0, vec1_types, vec2_types, vec3_types, vec4_types };

		if (type > rsx::vertex_base_type::ub256)
			fmt::throw_exception("VKGS error: unknown vertex base type 0x%x" HERE, (u32)type);

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
			fmt::throw_exception("Unsupported primitive topology 0x%x", (u8)mode);
		}
	}

	bool is_primitive_native(rsx::primitive_type& mode)
	{
		bool result;
		get_appropriate_topology(mode, result);
		return !result;
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
	static constexpr std::array<const char*, 16> s_reg_table =
	{
		"in_pos_buffer", "in_weight_buffer", "in_normal_buffer",
		"in_diff_color_buffer", "in_spec_color_buffer",
		"in_fog_buffer",
		"in_point_size_buffer", "in_7_buffer",
		"in_tc0_buffer", "in_tc1_buffer", "in_tc2_buffer", "in_tc3_buffer",
		"in_tc4_buffer", "in_tc5_buffer", "in_tc6_buffer", "in_tc7_buffer"
	};

	/**
	* Creates and fills an index buffer emulating unsupported primitive type.
	* Returns index_count and (offset_in_index_buffer, index_type)
	*/
	std::tuple<u32, std::tuple<VkDeviceSize, VkIndexType>> generate_emulating_index_buffer(
		const rsx::draw_clause& clause, u32 vertex_count,
		vk::vk_data_heap& m_index_buffer_ring_info)
	{
		u32 index_count = get_index_count(clause.primitive, vertex_count);
		u32 upload_size = index_count * sizeof(u16);

		VkDeviceSize offset_in_index_buffer = m_index_buffer_ring_info.alloc<256>(upload_size);
		void* buf = m_index_buffer_ring_info.map(offset_in_index_buffer, upload_size);

		write_index_array_for_non_indexed_non_native_primitive_to_buffer(
			reinterpret_cast<char*>(buf), clause.primitive, 0, vertex_count);

		m_index_buffer_ring_info.unmap();
		return std::make_tuple(
			index_count, std::make_tuple(offset_in_index_buffer, VK_INDEX_TYPE_UINT16));
	}

	struct vertex_buffer_visitor
	{
		vertex_buffer_visitor(u32 vtx_cnt, VkDevice dev, vk::vk_data_heap& heap,
			vk::glsl::program* prog, VkDescriptorSet desc_set,
			std::vector<std::unique_ptr<vk::buffer_view>>& buffer_view_to_clean)
			: vertex_count(vtx_cnt), m_attrib_ring_info(heap), device(dev), m_program(prog),
			  descriptor_sets(desc_set), m_buffer_view_to_clean(buffer_view_to_clean)
		{
		}

		void operator()(const rsx::vertex_array_buffer& vertex_array)
		{
			// Fill vertex_array
			u32 element_size = rsx::get_vertex_type_size_on_host(vertex_array.type, vertex_array.attribute_size);
			u32 real_element_size = vk::get_suitable_vk_size(vertex_array.type, vertex_array.attribute_size);

			u32 upload_size = real_element_size * vertex_count;

			VkDeviceSize offset_in_attrib_buffer = m_attrib_ring_info.alloc<256>(upload_size);
			void *dst = m_attrib_ring_info.map(offset_in_attrib_buffer, upload_size);
			
			gsl::span<gsl::byte> dest_span(static_cast<gsl::byte*>(dst), upload_size);
			write_vertex_array_data_to_buffer(dest_span, vertex_array.data, vertex_count, vertex_array.type, vertex_array.attribute_size, vertex_array.stride, real_element_size);

			//Padding the vertex buffer should be done after the writes have been done
			//write_vertex_data function may 'dirty' unused sections of the buffer as optimization
			vk::prepare_buffer_for_writing(dst, vertex_array.type, vertex_array.attribute_size, vertex_count);

			m_attrib_ring_info.unmap();
			const VkFormat format = vk::get_suitable_vk_format(vertex_array.type, vertex_array.attribute_size);

			m_buffer_view_to_clean.push_back(std::make_unique<vk::buffer_view>(device, m_attrib_ring_info.heap->value, format, offset_in_attrib_buffer, upload_size));
			m_program->bind_uniform(m_buffer_view_to_clean.back()->value, s_reg_table[vertex_array.index], descriptor_sets);
		}

		void operator()(const rsx::vertex_array_register& vertex_register)
		{
			size_t data_size = rsx::get_vertex_type_size_on_host(vertex_register.type, vertex_register.attribute_size);
			const VkFormat format = vk::get_suitable_vk_format(vertex_register.type, vertex_register.attribute_size);

			u32 offset_in_attrib_buffer = 0;

			if (vk::requires_component_expansion(vertex_register.type, vertex_register.attribute_size))
			{
				const u32 num_stored_verts = static_cast<u32>(
					data_size / (sizeof(float) * vertex_register.attribute_size));
				const u32 real_element_size = vk::get_suitable_vk_size(vertex_register.type, vertex_register.attribute_size);

				data_size = real_element_size * num_stored_verts;
				offset_in_attrib_buffer = m_attrib_ring_info.alloc<256>(data_size);
				void *dst = m_attrib_ring_info.map(offset_in_attrib_buffer, data_size);

				vk::expand_array_components<float, 3, 4, 1>(reinterpret_cast<const float*>(vertex_register.data.data()), dst, num_stored_verts);
				m_attrib_ring_info.unmap();
			}
			else
			{
				offset_in_attrib_buffer = m_attrib_ring_info.alloc<256>(data_size);
				void *dst = m_attrib_ring_info.map(offset_in_attrib_buffer, data_size);
				memcpy(dst, vertex_register.data.data(), data_size);
				m_attrib_ring_info.unmap();
			}

			m_buffer_view_to_clean.push_back(std::make_unique<vk::buffer_view>(device, m_attrib_ring_info.heap->value, format, offset_in_attrib_buffer, data_size));
			m_program->bind_uniform(m_buffer_view_to_clean.back()->value, s_reg_table[vertex_register.index], descriptor_sets);
		}

		void operator()(const rsx::empty_vertex_array& vbo)
		{
			u32 offset_in_attrib_buffer = m_attrib_ring_info.alloc<256>(32);
			void *dst = m_attrib_ring_info.map(offset_in_attrib_buffer, 32);
			memset(dst, 0, 32);
			m_attrib_ring_info.unmap();
			m_buffer_view_to_clean.push_back(std::make_unique<vk::buffer_view>(device, m_attrib_ring_info.heap->value, VK_FORMAT_R32_SFLOAT, offset_in_attrib_buffer, 32));
			m_program->bind_uniform(m_buffer_view_to_clean.back()->value, s_reg_table[vbo.index], descriptor_sets);
		}

	protected:
		VkDevice device;
		u32 vertex_count;
		vk::vk_data_heap& m_attrib_ring_info;
		vk::glsl::program* m_program;
		VkDescriptorSet descriptor_sets;
		std::vector<std::unique_ptr<vk::buffer_view>>& m_buffer_view_to_clean;
	};

	using attribute_storage = std::vector<std::variant<rsx::vertex_array_buffer,
		rsx::vertex_array_register, rsx::empty_vertex_array>>;

	struct draw_command_visitor
	{
		using result_type = std::tuple<VkPrimitiveTopology, u32,
			std::optional<std::tuple<VkDeviceSize, VkIndexType>>>;

		draw_command_visitor(VkDevice device, vk::vk_data_heap& index_buffer_ring_info,
			vk::vk_data_heap& attrib_ring_info, vk::glsl::program* program,
			VkDescriptorSet descriptor_sets,
			std::vector<std::unique_ptr<vk::buffer_view>>& buffer_view_to_clean,
			std::function<attribute_storage(
				const rsx::rsx_state&, const std::vector<std::pair<u32, u32>>&)>
				get_vertex_buffers_f)
			: m_device(device), m_index_buffer_ring_info(index_buffer_ring_info),
			  m_attrib_ring_info(attrib_ring_info), m_program(program),
			  m_descriptor_sets(descriptor_sets), m_buffer_view_to_clean(buffer_view_to_clean),
			  get_vertex_buffers(get_vertex_buffers_f)
		{
		}

		result_type operator()(const rsx::draw_array_command& command)
		{
			bool primitives_emulated = false;
			VkPrimitiveTopology prims = vk::get_appropriate_topology(
				rsx::method_registers.current_draw_clause.primitive, primitives_emulated);
			u32 index_count = 0;
			std::optional<std::tuple<VkDeviceSize, VkIndexType>> index_info;

			u32 min_index =
				rsx::method_registers.current_draw_clause.first_count_commands.front().first;
			u32 max_index =
				rsx::method_registers.current_draw_clause.get_elements_count() + min_index - 1;

			if (primitives_emulated) {
				std::tie(index_count, index_info) =
					generate_emulating_index_buffer(rsx::method_registers.current_draw_clause,
						max_index - min_index + 1, m_index_buffer_ring_info);
			}
			else
			{
				index_count = rsx::method_registers.current_draw_clause.get_elements_count();
			}

			upload_vertex_buffers(min_index, max_index);
			return std::make_tuple(prims, index_count, index_info);
		}

		result_type operator()(const rsx::draw_indexed_array_command& command)
		{
			bool primitives_emulated = false;
			VkPrimitiveTopology prims = vk::get_appropriate_topology(
				rsx::method_registers.current_draw_clause.primitive, primitives_emulated);

			rsx::index_array_type index_type = rsx::method_registers.current_draw_clause.is_immediate_draw ?
				rsx::index_array_type::u32 :
				rsx::method_registers.index_type();

			u32 type_size = gsl::narrow<u32>(get_index_type_size(index_type));

			u32 index_count = rsx::method_registers.current_draw_clause.get_elements_count();
			if (primitives_emulated)
				index_count = get_index_count(rsx::method_registers.current_draw_clause.primitive, index_count);
			u32 upload_size = index_count * type_size;

			VkDeviceSize offset_in_index_buffer = m_index_buffer_ring_info.alloc<256>(upload_size);
			void* buf = m_index_buffer_ring_info.map(offset_in_index_buffer, upload_size);

			/**
			* Upload index (and expands it if primitive type is not natively supported).
			*/
			u32 min_index, max_index;
			std::tie(min_index, max_index) = write_index_array_data_to_buffer(
				gsl::span<gsl::byte>(static_cast<gsl::byte*>(buf), index_count * type_size),
				command.raw_index_buffer, index_type,
				rsx::method_registers.current_draw_clause.primitive,
				rsx::method_registers.restart_index_enabled(),
				rsx::method_registers.restart_index(), command.ranges_to_fetch_in_index_buffer,
				[](auto prim) { return !vk::is_primitive_native(prim); });

			m_index_buffer_ring_info.unmap();

			std::optional<std::tuple<VkDeviceSize, VkIndexType>> index_info =
				std::make_tuple(offset_in_index_buffer, vk::get_index_type(index_type));

			upload_vertex_buffers(0, max_index);
			return std::make_tuple(prims, index_count, index_info);
		}

		result_type operator()(const rsx::draw_inlined_array& command)
		{
			bool primitives_emulated = false;
			VkPrimitiveTopology prims = vk::get_appropriate_topology(
				rsx::method_registers.current_draw_clause.primitive, primitives_emulated);
			u32 index_count = upload_inlined_array();

			if (!primitives_emulated) {
				return std::make_tuple(prims, index_count, std::nullopt);
			}

			std::optional<std::tuple<VkDeviceSize, VkIndexType>> index_info;
			std::tie(index_count, index_info) = generate_emulating_index_buffer(
				rsx::method_registers.current_draw_clause, index_count, m_index_buffer_ring_info);
			return std::make_tuple(prims, index_count, index_info);
		}

	private:
		vk::vk_data_heap& m_index_buffer_ring_info;
		VkDevice m_device;
		vk::vk_data_heap& m_attrib_ring_info;
		vk::glsl::program* m_program;
		VkDescriptorSet m_descriptor_sets;
		std::vector<std::unique_ptr<vk::buffer_view>>& m_buffer_view_to_clean;
		std::function<attribute_storage(
			const rsx::rsx_state&, const std::vector<std::pair<u32, u32>>&)>
			get_vertex_buffers;

		void upload_vertex_buffers(u32 min_index, u32 vertex_max_index)
		{
			vertex_buffer_visitor visitor(vertex_max_index - min_index + 1, m_device,
				m_attrib_ring_info, m_program, m_descriptor_sets, m_buffer_view_to_clean);
			const auto& vertex_buffers = get_vertex_buffers(
				rsx::method_registers, {{min_index, vertex_max_index - min_index + 1}});
			for (const auto& vbo : vertex_buffers) std::apply_visitor(visitor, vbo);
		}

		u32 upload_inlined_array()
		{
			u32 stride = 0;
			u32 offsets[rsx::limits::vertex_count] = {0};

			for (u32 i = 0; i < rsx::limits::vertex_count; ++i) {
				const auto& info = rsx::method_registers.vertex_arrays_info[i];
				if (!info.size()) continue;

				offsets[i] = stride;
				stride += rsx::get_vertex_type_size_on_host(info.type(), info.size());
			}

			u32 vertex_draw_count =
				(u32)(rsx::method_registers.current_draw_clause.inline_vertex_array.size() *
					  sizeof(u32)) /
				stride;

			for (int index = 0; index < rsx::limits::vertex_count; ++index) {
				auto& vertex_info = rsx::method_registers.vertex_arrays_info[index];

				if (!m_program->has_uniform(s_reg_table[index])) continue;

				if (!vertex_info.size()) // disabled
				{
					continue;
				}

				const u32 element_size =
					vk::get_suitable_vk_size(vertex_info.type(), vertex_info.size());
				const u32 data_size = element_size * vertex_draw_count;
				const VkFormat format =
					vk::get_suitable_vk_format(vertex_info.type(), vertex_info.size());

				u32 offset_in_attrib_buffer = m_attrib_ring_info.alloc<256>(data_size);
				u8* src = reinterpret_cast<u8*>(
					rsx::method_registers.current_draw_clause.inline_vertex_array.data());
				u8* dst =
					static_cast<u8*>(m_attrib_ring_info.map(offset_in_attrib_buffer, data_size));

				src += offsets[index];
				u8 opt_size = vertex_info.size();

				if (vertex_info.size() == 3) opt_size = 4;

				// TODO: properly handle cmp type
				if (vertex_info.type() == rsx::vertex_base_type::cmp)
					LOG_ERROR(
						RSX, "Compressed vertex attributes not supported for inlined arrays yet");

				switch (vertex_info.type())
				{
				case rsx::vertex_base_type::f:
					vk::copy_inlined_data_to_buffer<float, 1>(src, dst, vertex_draw_count,
						vertex_info.type(), vertex_info.size(), opt_size, element_size, stride);
					break;
				case rsx::vertex_base_type::sf:
					vk::copy_inlined_data_to_buffer<u16, 0x3c00>(src, dst, vertex_draw_count,
						vertex_info.type(), vertex_info.size(), opt_size, element_size, stride);
					break;
				case rsx::vertex_base_type::s1:
				case rsx::vertex_base_type::ub:
				case rsx::vertex_base_type::ub256:
					vk::copy_inlined_data_to_buffer<u8, 1>(src, dst, vertex_draw_count,
						vertex_info.type(), vertex_info.size(), opt_size, element_size, stride);
					break;
				case rsx::vertex_base_type::s32k:
				case rsx::vertex_base_type::cmp:
					vk::copy_inlined_data_to_buffer<u16, 1>(src, dst, vertex_draw_count,
						vertex_info.type(), vertex_info.size(), opt_size, element_size, stride);
					break;
				default: fmt::throw_exception("Unknown base type %d" HERE, (u32)vertex_info.type());
				}

				m_attrib_ring_info.unmap();
				m_buffer_view_to_clean.push_back(std::make_unique<vk::buffer_view>(m_device,
					m_attrib_ring_info.heap->value, format, offset_in_attrib_buffer, data_size));
				m_program->bind_uniform(
					m_buffer_view_to_clean.back()->value, s_reg_table[index], m_descriptor_sets);
			}

			return vertex_draw_count;
		}
	};
}

std::tuple<VkPrimitiveTopology, u32, std::optional<std::tuple<VkDeviceSize, VkIndexType>>>
VKGSRender::upload_vertex_data()
{
	draw_command_visitor visitor(*m_device, m_index_buffer_ring_info, m_attrib_ring_info, m_program,
		descriptor_sets, m_buffer_view_to_clean,
		[this](const auto& state, const auto& range) { return this->get_vertex_buffers(state, range); });
	return std::apply_visitor(visitor, get_draw_command(rsx::method_registers));
}
