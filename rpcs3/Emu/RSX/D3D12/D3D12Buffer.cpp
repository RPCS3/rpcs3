#ifdef _MSC_VER
#include "stdafx.h"
#include "stdafx_d3d12.h"

#include <variant>

#include "D3D12GSRender.h"
#include "d3dx12.h"
#include "../Common/BufferUtils.h"
#include "D3D12Formats.h"
#include "../rsx_methods.h"

namespace
{
	UINT get_component_mapping_from_vector_size(rsx::vertex_base_type type, u8 size)
	{
		if (type == rsx::vertex_base_type::cmp) return D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		switch (size)
		{
		case 1:
			return D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
				D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0,
				D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0,
				D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1);
		case 2:
			return D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
				D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0,
				D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1);
		case 3:
			return D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2,
				D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1);
		case 4:
			return D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		}
		fmt::throw_exception("Wrong vector size %d" HERE, size);
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC get_vertex_attribute_srv(const rsx::data_array_format_info &info, UINT64 offset_in_vertex_buffers_buffer, UINT buffer_size)
	{
		u32 element_size = rsx::get_vertex_type_size_on_host(info.type(), info.size());
		D3D12_SHADER_RESOURCE_VIEW_DESC vertex_buffer_view = {
			get_vertex_attribute_format(info.type(), info.size()), D3D12_SRV_DIMENSION_BUFFER,
			get_component_mapping_from_vector_size(info.type(), info.size())};
		vertex_buffer_view.Buffer.FirstElement = offset_in_vertex_buffers_buffer / element_size;
		vertex_buffer_view.Buffer.NumElements = buffer_size / element_size;
		return vertex_buffer_view;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC get_vertex_attribute_srv(const rsx::vertex_base_type type, u8 size, UINT64 offset_in_vertex_buffers_buffer, UINT buffer_size)
	{
		u32 element_size = rsx::get_vertex_type_size_on_host(type, size);
		D3D12_SHADER_RESOURCE_VIEW_DESC vertex_buffer_view = {get_vertex_attribute_format(type, size),
			D3D12_SRV_DIMENSION_BUFFER, get_component_mapping_from_vector_size(type, size)};
		vertex_buffer_view.Buffer.FirstElement = offset_in_vertex_buffers_buffer / element_size;
		vertex_buffer_view.Buffer.NumElements = buffer_size / element_size;
		return vertex_buffer_view;
	}

	template<int N>
	UINT64 get_next_multiple_of(UINT64 val)
	{
		UINT64 divided_val = (val + N - 1) / N;
		return divided_val * N;
	}
}


void D3D12GSRender::upload_and_bind_scale_offset_matrix(size_t descriptorIndex)
{
	size_t heap_offset = m_buffer_data.alloc<D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT>(512);

	// Scale offset buffer
	// Separate constant buffer
	void *mapped_buffer = m_buffer_data.map<void>(CD3DX12_RANGE(heap_offset, heap_offset + 512));
	fill_scale_offset_data(mapped_buffer, true);
	fill_user_clip_data((char*)mapped_buffer + 64);
	fill_fragment_state_buffer((char *)mapped_buffer + 128, current_fragment_program);
	m_buffer_data.unmap(CD3DX12_RANGE(heap_offset, heap_offset + 512));

	D3D12_CONSTANT_BUFFER_VIEW_DESC constant_buffer_view_desc = {
		m_buffer_data.get_heap()->GetGPUVirtualAddress() + heap_offset,
		512
	};
	m_device->CreateConstantBufferView(&constant_buffer_view_desc,
		CD3DX12_CPU_DESCRIPTOR_HANDLE(get_current_resource_storage().descriptors_heap->GetCPUDescriptorHandleForHeapStart())
		.Offset((INT)descriptorIndex, m_descriptor_stride_srv_cbv_uav));
}

void D3D12GSRender::upload_and_bind_vertex_shader_constants(size_t descriptor_index)
{
	size_t buffer_size = 512 * 4 * sizeof(float);

	size_t heap_offset = m_buffer_data.alloc<D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT>(buffer_size);

	void *mapped_buffer = m_buffer_data.map<void>(CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));
	fill_vertex_program_constants_data(mapped_buffer);
	*(reinterpret_cast<u32*>((char *)mapped_buffer + (468 * 4 * sizeof(float)))) = rsx::method_registers.transform_branch_bits();
	m_buffer_data.unmap(CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));

	D3D12_CONSTANT_BUFFER_VIEW_DESC constant_buffer_view_desc = {
		m_buffer_data.get_heap()->GetGPUVirtualAddress() + heap_offset,
		(UINT)buffer_size
	};
	m_device->CreateConstantBufferView(&constant_buffer_view_desc,
		CD3DX12_CPU_DESCRIPTOR_HANDLE(get_current_resource_storage().descriptors_heap->GetCPUDescriptorHandleForHeapStart())
		.Offset((INT)descriptor_index, m_descriptor_stride_srv_cbv_uav));
}

D3D12_CONSTANT_BUFFER_VIEW_DESC D3D12GSRender::upload_fragment_shader_constants()
{
	// Get constant from fragment program
	size_t buffer_size = m_pso_cache.get_fragment_constants_buffer_size(current_fragment_program);
	// Multiple of 256 never 0
	buffer_size = (buffer_size + 255) & ~255;

	size_t heap_offset = m_buffer_data.alloc<D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT>(buffer_size);

	size_t offset = 0;
	float *mapped_buffer = m_buffer_data.map<float>(CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));
	m_pso_cache.fill_fragment_constants_buffer({ mapped_buffer, ::narrow<int>(buffer_size) }, current_fragment_program);
	m_buffer_data.unmap(CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));

	return {
		m_buffer_data.get_heap()->GetGPUVirtualAddress() + heap_offset,
		(UINT)buffer_size
	};
}

namespace
{

	struct vertex_buffer_visitor
	{
		std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> vertex_buffer_views;

		vertex_buffer_visitor(u32 vtx_cnt, ID3D12GraphicsCommandList* cmdlst,
			ID3D12Resource* write_vertex_buffer, d3d12_data_heap& heap)
			: vertex_count(vtx_cnt), offset_in_vertex_buffers_buffer(0), m_buffer_data(heap),
			  command_list(cmdlst), m_vertex_buffer_data(write_vertex_buffer)
		{
		}

		void operator()(const rsx::vertex_array_buffer& vertex_array)
		{
			u32 element_size =
				rsx::get_vertex_type_size_on_host(vertex_array.type, vertex_array.attribute_size);
			UINT buffer_size = element_size * vertex_count;
			size_t heap_offset =
				m_buffer_data.alloc<D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT>(buffer_size);

			void* mapped_buffer =
				m_buffer_data.map<void>(CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));
			gsl::span<gsl::byte> mapped_buffer_span = {
				(gsl::byte*)mapped_buffer, gsl::narrow_cast<int>(buffer_size)};
			write_vertex_array_data_to_buffer(mapped_buffer_span, vertex_array.data, vertex_count,
				vertex_array.type, vertex_array.attribute_size, vertex_array.stride, element_size, vertex_array.is_be);

			m_buffer_data.unmap(CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));

			command_list->CopyBufferRegion(m_vertex_buffer_data, offset_in_vertex_buffers_buffer,
				m_buffer_data.get_heap(), heap_offset, buffer_size);

			vertex_buffer_views.emplace_back(get_vertex_attribute_srv(vertex_array.type,
				vertex_array.attribute_size, offset_in_vertex_buffers_buffer, buffer_size));
			offset_in_vertex_buffers_buffer =
				get_next_multiple_of<48>(offset_in_vertex_buffers_buffer +
										 buffer_size); // 48 is multiple of 2, 4, 6, 8, 12, 16

			// m_timers.buffer_upload_size += buffer_size;
		}

		void operator()(const rsx::vertex_array_register& vertex_register)
		{
			u32 element_size = rsx::get_vertex_type_size_on_host(
				vertex_register.type, vertex_register.attribute_size);
			UINT buffer_size = element_size;
			size_t heap_offset =
				m_buffer_data.alloc<D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT>(buffer_size);

			void* mapped_buffer =
				m_buffer_data.map<void>(CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));
			memcpy(mapped_buffer, vertex_register.data.data(), buffer_size);
			m_buffer_data.unmap(CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));

			command_list->CopyBufferRegion(m_vertex_buffer_data, offset_in_vertex_buffers_buffer,
				m_buffer_data.get_heap(), heap_offset, buffer_size);

			vertex_buffer_views.emplace_back(get_vertex_attribute_srv(vertex_register.type,
				vertex_register.attribute_size, offset_in_vertex_buffers_buffer, buffer_size));
			offset_in_vertex_buffers_buffer =
				get_next_multiple_of<48>(offset_in_vertex_buffers_buffer +
										 buffer_size); // 48 is multiple of 2, 4, 6, 8, 12, 16
		}

		void operator()(const rsx::empty_vertex_array& vbo)
		{
		}

	protected:
		u32 vertex_count;
		ID3D12GraphicsCommandList* command_list;
		ID3D12Resource* m_vertex_buffer_data;
		size_t offset_in_vertex_buffers_buffer;
		d3d12_data_heap& m_buffer_data;
	};

	std::tuple<D3D12_INDEX_BUFFER_VIEW, size_t> generate_index_buffer_for_emulated_primitives_array(
		u32 vertex_count, d3d12_data_heap& m_buffer_data)
	{
		size_t index_count = get_index_count(rsx::method_registers.current_draw_clause.primitive, vertex_count);

		// Alloc
		size_t buffer_size = align(index_count * sizeof(u16), 64);
		size_t heap_offset =
			m_buffer_data.alloc<D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT>(buffer_size);

		void* mapped_buffer =
			m_buffer_data.map<void>(CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));

		write_index_array_for_non_indexed_non_native_primitive_to_buffer((char *)mapped_buffer, rsx::method_registers.current_draw_clause.primitive, vertex_count);

		m_buffer_data.unmap(CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));
		D3D12_INDEX_BUFFER_VIEW index_buffer_view = {
			m_buffer_data.get_heap()->GetGPUVirtualAddress() + heap_offset, (UINT)buffer_size,
			DXGI_FORMAT_R16_UINT};

		return std::make_tuple(index_buffer_view, index_count);
	}

	using attribute_storage = std::vector<std::variant<rsx::vertex_array_buffer,
		rsx::vertex_array_register, rsx::empty_vertex_array>>;

	/**
	* Upload all enabled vertex attributes for vertex in ranges described by vertex_ranges.
	* A range in vertex_range is a pair whose first element is the index of the beginning of the
	* range, and whose second element is the number of vertex in this range.
	*/
	std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> upload_vertex_attributes(
		u32 vertex_count,
		std::function<attribute_storage()> get_vertex_buffers,
		ID3D12Resource* m_vertex_buffer_data, d3d12_data_heap& m_buffer_data,
		ID3D12GraphicsCommandList* command_list)
	{
		command_list->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(m_vertex_buffer_data,
				D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST));

		vertex_buffer_visitor visitor(
			vertex_count, command_list, m_vertex_buffer_data, m_buffer_data);
		const auto& vertex_buffers = get_vertex_buffers();

		for (const auto& vbo : vertex_buffers) std::visit(visitor, vbo);

		command_list->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(m_vertex_buffer_data,
				D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
		return visitor.vertex_buffer_views;
	}

	std::tuple<std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC>, size_t> upload_inlined_vertex_array(
		gsl::span<const rsx::data_array_format_info, 16> vertex_attribute_infos,
		gsl::span<const gsl::byte> inlined_array_raw_data, d3d12_data_heap& ring_buffer_data,
		ID3D12Resource* vertex_buffer_placement, ID3D12GraphicsCommandList* command_list)
	{
		// We can't rely on vertex_attribute_infos strides here so compute it
		// assuming all attributes are packed
		u32 stride = 0;
		u32 initial_offsets[rsx::limits::vertex_count];
		u8 index = 0;
		for (const auto& info : vertex_attribute_infos) {
			initial_offsets[index++] = stride;

			if (!info.size()) // disabled
				continue;

			stride += rsx::get_vertex_type_size_on_host(info.type(), info.size());
		}

		u32 element_count = ::narrow<u32>(inlined_array_raw_data.size_bytes()) / stride;
		std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> result;

		UINT64 vertex_buffer_offset = 0;
		index = 0;
		for (const auto& info : vertex_attribute_infos) {
			if (!info.size()) {
				index++;
				continue;
			}

			u32 element_size = rsx::get_vertex_type_size_on_host(info.type(), info.size());
			UINT buffer_size = element_size * element_count;
			size_t heap_offset =
				ring_buffer_data.alloc<D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT>(buffer_size);

			void* mapped_buffer =
				ring_buffer_data.map<void>(CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));
			gsl::span<gsl::byte> dst = {(gsl::byte*)mapped_buffer, buffer_size};

			for (u32 i = 0; i < element_count; i++) {
				auto subdst = dst.subspan(i * element_size, element_size);
				auto subsrc = inlined_array_raw_data.subspan(
					initial_offsets[index] + (i * stride), element_size);
				if (info.type() == rsx::vertex_base_type::ub && info.size() == 4) {
					subdst[0] = subsrc[3];
					subdst[1] = subsrc[2];
					subdst[2] = subsrc[1];
					subdst[3] = subsrc[0];
				}
				else
				{
					std::copy(subsrc.begin(), subsrc.end(), subdst.begin());
				}
			}

			ring_buffer_data.unmap(CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));

			command_list->CopyBufferRegion(vertex_buffer_placement, vertex_buffer_offset,
				ring_buffer_data.get_heap(), heap_offset, buffer_size);

			result.emplace_back(get_vertex_attribute_srv(info, vertex_buffer_offset, buffer_size));
			vertex_buffer_offset = get_next_multiple_of<48>(
				vertex_buffer_offset + buffer_size); // 48 is multiple of 2, 4, 6, 8, 12, 16
			index++;
		}

		return std::make_tuple(result, element_count);
	}

	struct draw_command_visitor
	{
		draw_command_visitor(ID3D12GraphicsCommandList* cmd_list, d3d12_data_heap& buffer_data,
			ID3D12Resource* vertex_buffer_data,
			std::function<attribute_storage()> get_vertex_info_lambda)
			: command_list(cmd_list), m_buffer_data(buffer_data),
			  m_vertex_buffer_data(vertex_buffer_data), get_vertex_buffers(get_vertex_info_lambda)
		{
		}

		std::tuple<bool, size_t, std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC>> operator()(
			const rsx::draw_array_command& command)
		{
			const auto vertex_count = rsx::method_registers.current_draw_clause.get_elements_count();
			if (is_primitive_native(rsx::method_registers.current_draw_clause.primitive)) {
				return std::make_tuple(false, vertex_count,
					upload_vertex_attributes(vertex_count,
						get_vertex_buffers,
						m_vertex_buffer_data, m_buffer_data, command_list));
			}

			D3D12_INDEX_BUFFER_VIEW index_buffer_view;
			size_t index_count;
			std::tie(index_buffer_view, index_count) =
				generate_index_buffer_for_emulated_primitives_array(
					vertex_count, m_buffer_data);
			command_list->IASetIndexBuffer(&index_buffer_view);
			return std::make_tuple(true, index_count,
				upload_vertex_attributes(vertex_count,
					get_vertex_buffers,
					m_vertex_buffer_data, m_buffer_data, command_list));
		}

		std::tuple<bool, size_t, std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC>> operator()(
			const rsx::draw_indexed_array_command& command)
		{
			// Index count
			size_t index_count =
				get_index_count(rsx::method_registers.current_draw_clause.primitive,
					rsx::method_registers.current_draw_clause.get_elements_count());

			rsx::index_array_type indexed_type = rsx::method_registers.current_draw_clause.is_immediate_draw?
				rsx::index_array_type::u32:
				rsx::method_registers.index_type();

			constexpr size_t index_size = sizeof(u32); // Force u32 destination to avoid overflows when adding base

			// Alloc
			size_t buffer_size = align(index_count * index_size, 64);
			size_t heap_offset =
				m_buffer_data.alloc<D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT>(buffer_size);

			void* mapped_buffer =
				m_buffer_data.map<void>(CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));
			u32 min_index, max_index;
			gsl::span<gsl::byte> dst{
				reinterpret_cast<gsl::byte*>(mapped_buffer), ::narrow<u32>(buffer_size)};

			std::tie(min_index, max_index, index_count) =
				write_index_array_data_to_buffer(dst, command.raw_index_buffer, indexed_type,
					rsx::method_registers.current_draw_clause.primitive,
					rsx::method_registers.restart_index_enabled(),
					rsx::method_registers.restart_index(),
					rsx::method_registers.vertex_data_base_index(), [](auto prim) { return !is_primitive_native(prim); });

			m_buffer_data.unmap(CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));
			D3D12_INDEX_BUFFER_VIEW index_buffer_view = {
				m_buffer_data.get_heap()->GetGPUVirtualAddress() + heap_offset, (UINT)buffer_size,
				DXGI_FORMAT_R32_UINT};
			// m_timers.buffer_upload_size += buffer_size;
			command_list->IASetIndexBuffer(&index_buffer_view);

			return std::make_tuple(true, index_count,
				upload_vertex_attributes(max_index + 1, get_vertex_buffers,
					m_vertex_buffer_data, m_buffer_data, command_list));
		}

		std::tuple<bool, size_t, std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC>> operator()(
			const rsx::draw_inlined_array& command)
		{
			size_t vertex_count;
			std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> vertex_buffer_view;
			std::tie(vertex_buffer_view, vertex_count) =
				upload_inlined_vertex_array(rsx::method_registers.vertex_arrays_info,
					{(const gsl::byte*)rsx::method_registers.current_draw_clause.inline_vertex_array.data(),
						::narrow<int>(rsx::method_registers.current_draw_clause.inline_vertex_array.size() * sizeof(uint))},
					m_buffer_data, m_vertex_buffer_data, command_list);

			if (is_primitive_native(rsx::method_registers.current_draw_clause.primitive))
				return std::make_tuple(false, vertex_count, vertex_buffer_view);

			D3D12_INDEX_BUFFER_VIEW index_buffer_view;
			size_t index_count;
			std::tie(index_buffer_view, index_count) =
				generate_index_buffer_for_emulated_primitives_array(
					vertex_count, m_buffer_data);
			command_list->IASetIndexBuffer(&index_buffer_view);
			return std::make_tuple(true, index_count, vertex_buffer_view);
		}

	private:
		ID3D12GraphicsCommandList* command_list;
		d3d12_data_heap& m_buffer_data;
		std::function<attribute_storage()> get_vertex_buffers;
		ID3D12Resource* m_vertex_buffer_data;
	};
} // End anonymous namespace

std::tuple<bool, size_t, std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC>>
D3D12GSRender::upload_and_set_vertex_index_data(ID3D12GraphicsCommandList* command_list)
{
	return std::visit(
		draw_command_visitor(command_list, m_buffer_data, m_vertex_buffer_data.Get(),
			[this]() { return get_vertex_buffers(rsx::method_registers, 0); }),
		get_draw_command(rsx::method_registers));
}

#endif
