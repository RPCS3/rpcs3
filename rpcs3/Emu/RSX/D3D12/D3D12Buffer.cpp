#ifdef _MSC_VER
#include "stdafx.h"
#include "stdafx_d3d12.h"

#include "D3D12GSRender.h"
#include "d3dx12.h"
#include "../Common/BufferUtils.h"
#include "D3D12Formats.h"
#include "../rsx_methods.h"

namespace
{
	UINT get_component_mapping_from_vector_size(u8 size)
	{
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
		throw EXCEPTION("Wrong vector size %d", size);
	}

	u32 get_vertex_count(const std::vector<std::pair<u32, u32> > first_count_commands)
	{
		u32 vertex_count = 0;
		for (const auto &pair : first_count_commands)
			vertex_count += pair.second;
		return vertex_count;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC get_vertex_attribute_srv(const rsx::data_array_format_info &info, UINT64 offset_in_vertex_buffers_buffer, UINT buffer_size)
	{
		u32 element_size = rsx::get_vertex_type_size_on_host(info.type, info.size);
		D3D12_SHADER_RESOURCE_VIEW_DESC vertex_buffer_view = {
			get_vertex_attribute_format(info.type, info.size),
			D3D12_SRV_DIMENSION_BUFFER,
			get_component_mapping_from_vector_size(info.size)
		};
		vertex_buffer_view.Buffer.FirstElement = offset_in_vertex_buffers_buffer / element_size;
		vertex_buffer_view.Buffer.NumElements = buffer_size / element_size;
		return vertex_buffer_view;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC get_vertex_attribute_srv(const rsx::vertex_base_type type, u8 size, UINT64 offset_in_vertex_buffers_buffer, UINT buffer_size)
	{
		u32 element_size = rsx::get_vertex_type_size_on_host(type, size);
		D3D12_SHADER_RESOURCE_VIEW_DESC vertex_buffer_view = {
			get_vertex_attribute_format(type, size),
			D3D12_SRV_DIMENSION_BUFFER,
			get_component_mapping_from_vector_size(size)
		};
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


std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> D3D12GSRender::upload_vertex_attributes(
	const std::vector<std::pair<u32, u32> > &vertex_ranges,
	gsl::not_null<ID3D12GraphicsCommandList*> command_list)
{
	std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> vertex_buffer_views;
	command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertex_buffer_data.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST));

	u32 vertex_count = get_vertex_count(vertex_ranges);
	size_t offset_in_vertex_buffers_buffer = 0;
	u32 input_mask = rsx::method_registers.vertex_attrib_input_mask();
	EXPECTS(rsx::method_registers.vertex_data_base_index() == 0);

	for (int index = 0; index < rsx::limits::vertex_count; ++index)
	{
		bool enabled = !!(input_mask & (1 << index));
		if (!enabled)
			continue;

		if (rsx::method_registers.vertex_arrays_info[index].size > 0)
		{
			// Active vertex array
			const rsx::data_array_format_info &info = rsx::method_registers.vertex_arrays_info[index];

			u32 element_size = rsx::get_vertex_type_size_on_host(info.type, info.size);
			UINT buffer_size = element_size * vertex_count;
			size_t heap_offset = m_buffer_data.alloc<D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT>(buffer_size);

			u32 base_offset = rsx::method_registers.vertex_data_base_offset();
			u32 offset = rsx::method_registers.vertex_arrays_info[index].offset();
			u32 address = base_offset + rsx::get_address(offset & 0x7fffffff, offset >> 31);
			const gsl::byte *src_ptr = gsl::narrow_cast<const gsl::byte*>(vm::base(address));

			void *mapped_buffer = m_buffer_data.map<void>(CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));
			for (const auto &range : vertex_ranges)
			{
				gsl::span<gsl::byte> mapped_buffer_span = { (gsl::byte*)mapped_buffer, gsl::narrow_cast<int>(buffer_size) };
				write_vertex_array_data_to_buffer(mapped_buffer_span, src_ptr, range.first, range.second, info.type, info.size, info.stride, element_size);
				mapped_buffer = (char*)mapped_buffer + range.second * element_size;
			}
			m_buffer_data.unmap(CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));

			command_list->CopyBufferRegion(m_vertex_buffer_data.Get(), offset_in_vertex_buffers_buffer, m_buffer_data.get_heap(), heap_offset, buffer_size);

			vertex_buffer_views.emplace_back(get_vertex_attribute_srv(info, offset_in_vertex_buffers_buffer, buffer_size));
			offset_in_vertex_buffers_buffer = get_next_multiple_of<48>(offset_in_vertex_buffers_buffer + buffer_size); // 48 is multiple of 2, 4, 6, 8, 12, 16

			m_timers.buffer_upload_size += buffer_size;

		}
		else if (rsx::method_registers.register_vertex_info[index].size > 0)
		{
			// In register vertex attribute
			const rsx::register_vertex_data_info &info = rsx::method_registers.register_vertex_info[index];

			u32 element_size = rsx::get_vertex_type_size_on_host(info.type, info.size);
			UINT buffer_size = element_size;
			size_t heap_offset = m_buffer_data.alloc<D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT>(buffer_size);

			void *mapped_buffer = m_buffer_data.map<void>(CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));
			memcpy(mapped_buffer, info.data.data(), buffer_size);
			m_buffer_data.unmap(CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));

			command_list->CopyBufferRegion(m_vertex_buffer_data.Get(), offset_in_vertex_buffers_buffer, m_buffer_data.get_heap(), heap_offset, buffer_size);

			vertex_buffer_views.emplace_back(get_vertex_attribute_srv(info.type, info.size, offset_in_vertex_buffers_buffer, buffer_size));
			offset_in_vertex_buffers_buffer = get_next_multiple_of<48>(offset_in_vertex_buffers_buffer + buffer_size); // 48 is multiple of 2, 4, 6, 8, 12, 16
		}
	}
	command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertex_buffer_data.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
	return vertex_buffer_views;
}

namespace
{
std::tuple<std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC>, size_t> upload_inlined_vertex_array(
	gsl::span<const rsx::data_array_format_info, 16> vertex_attribute_infos,
	gsl::span<const gsl::byte> inlined_array_raw_data,
	d3d12_data_heap& ring_buffer_data,
	ID3D12Resource* vertex_buffer_placement,
	ID3D12GraphicsCommandList* command_list
	)
{
	// We can't rely on vertex_attribute_infos strides here so compute it
	// assuming all attributes are packed
	u32 stride = 0;
	u32 initial_offsets[rsx::limits::vertex_count];
	u8  index = 0;
	for (const auto &info : vertex_attribute_infos)
	{
		initial_offsets[index++] = stride;

		if (!info.size) // disabled
			continue;

		stride += rsx::get_vertex_type_size_on_host(info.type, info.size);
	}

	u32 element_count = gsl::narrow<u32>(inlined_array_raw_data.size_bytes()) / stride;
	std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> result;

	UINT64 vertex_buffer_offset = 0;
	index = 0;
	for (const auto &info : vertex_attribute_infos)
	{
		if (!info.size)
		{
			index++;
			continue;
		}

		u32 element_size = rsx::get_vertex_type_size_on_host(info.type, info.size);
		UINT buffer_size = element_size * element_count;
		size_t heap_offset = ring_buffer_data.alloc<D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT>(buffer_size);

		void *mapped_buffer = ring_buffer_data.map<void>(CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));
		gsl::span<gsl::byte> dst = { (gsl::byte*)mapped_buffer, buffer_size };

		for (u32 i = 0; i < element_count; i++)
		{
			auto subdst = dst.subspan(i * element_size, element_size);
			auto subsrc = inlined_array_raw_data.subspan(initial_offsets[index] + (i * stride), element_size);
			if (info.type == rsx::vertex_base_type::ub && info.size == 4)
			{
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

		command_list->CopyBufferRegion(vertex_buffer_placement, vertex_buffer_offset, ring_buffer_data.get_heap(), heap_offset, buffer_size);

		result.emplace_back(get_vertex_attribute_srv(info, vertex_buffer_offset, buffer_size));
		vertex_buffer_offset = get_next_multiple_of<48>(vertex_buffer_offset + buffer_size); // 48 is multiple of 2, 4, 6, 8, 12, 16
		index++;
	}

	return std::make_tuple(result, element_count);
}
}

void D3D12GSRender::upload_and_bind_scale_offset_matrix(size_t descriptorIndex)
{
	size_t heap_offset = m_buffer_data.alloc<D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT>(256);

	// Scale offset buffer
	// Separate constant buffer
	void *mapped_buffer = m_buffer_data.map<void>(CD3DX12_RANGE(heap_offset, heap_offset + 256));
	fill_scale_offset_data(mapped_buffer);
	int is_alpha_tested = rsx::method_registers.alpha_test_enabled();
	u8 alpha_ref_raw = rsx::method_registers.alpha_ref();
	float alpha_ref = alpha_ref_raw / 255.f;
	memcpy((char*)mapped_buffer + 16 * sizeof(float), &is_alpha_tested, sizeof(int));
	memcpy((char*)mapped_buffer + 17 * sizeof(float), &alpha_ref, sizeof(float));
	f32 fogp0 = rsx::method_registers.fog_params_0();
	f32 fogp1 = rsx::method_registers.fog_params_1();
	memcpy((char*)mapped_buffer + 18 * sizeof(float), &fogp0, sizeof(float));
	memcpy((char*)mapped_buffer + 19 * sizeof(float), &fogp1, sizeof(float));

	m_buffer_data.unmap(CD3DX12_RANGE(heap_offset, heap_offset + 256));

	D3D12_CONSTANT_BUFFER_VIEW_DESC constant_buffer_view_desc = {
		m_buffer_data.get_heap()->GetGPUVirtualAddress() + heap_offset,
		256
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
	size_t buffer_size = m_pso_cache.get_fragment_constants_buffer_size(m_fragment_program);
	// Multiple of 256 never 0
	buffer_size = (buffer_size + 255) & ~255;

	size_t heap_offset = m_buffer_data.alloc<D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT>(buffer_size);

	size_t offset = 0;
	float *mapped_buffer = m_buffer_data.map<float>(CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));
	m_pso_cache.fill_fragment_constans_buffer({ mapped_buffer, gsl::narrow<int>(buffer_size) }, m_fragment_program);
	m_buffer_data.unmap(CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));

	return {
		m_buffer_data.get_heap()->GetGPUVirtualAddress() + heap_offset,
		(UINT)buffer_size
	};
}




std::tuple<D3D12_INDEX_BUFFER_VIEW, size_t> D3D12GSRender::generate_index_buffer_for_emulated_primitives_array(const std::vector<std::pair<u32, u32> > &vertex_ranges)
{
	size_t index_count = 0;
	for (const auto &pair : vertex_ranges)
		index_count += get_index_count(rsx::method_registers.current_draw_clause.primitive, pair.second);

	// Alloc
	size_t buffer_size = align(index_count * sizeof(u16), 64);
	size_t heap_offset = m_buffer_data.alloc<D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT>(buffer_size);

	void *mapped_buffer = m_buffer_data.map<void>(CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));
	size_t first = 0;
	for (const auto &pair : vertex_ranges)
	{
		size_t element_count = get_index_count(rsx::method_registers.current_draw_clause.primitive, pair.second);
		write_index_array_for_non_indexed_non_native_primitive_to_buffer((char*)mapped_buffer, rsx::method_registers.current_draw_clause.primitive, (u32)first, (u32)pair.second);
		mapped_buffer = (char*)mapped_buffer + element_count * sizeof(u16);
		first += pair.second;
	}
	m_buffer_data.unmap(CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));
	D3D12_INDEX_BUFFER_VIEW index_buffer_view = {
		m_buffer_data.get_heap()->GetGPUVirtualAddress() + heap_offset,
		(UINT)buffer_size,
		DXGI_FORMAT_R16_UINT
	};

	return std::make_tuple(index_buffer_view, index_count);
}

std::tuple<bool, size_t, std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC>> D3D12GSRender::upload_and_set_vertex_index_data(ID3D12GraphicsCommandList *command_list)
{
	if (rsx::method_registers.current_draw_clause.command == rsx::draw_command::inlined_array)
	{
		size_t vertex_count;
		std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> vertex_buffer_view;
		std::tie(vertex_buffer_view, vertex_count) = upload_inlined_vertex_array(
			rsx::method_registers.vertex_arrays_info,
			{ (const gsl::byte*) inline_vertex_array.data(), gsl::narrow<int>(inline_vertex_array.size() * sizeof(uint)) },
			m_buffer_data, m_vertex_buffer_data.Get(), command_list);

		if (is_primitive_native(rsx::method_registers.current_draw_clause.primitive))
			return std::make_tuple(false, vertex_count, vertex_buffer_view);

		D3D12_INDEX_BUFFER_VIEW index_buffer_view;
		size_t index_count;
		std::tie(index_buffer_view, index_count) = generate_index_buffer_for_emulated_primitives_array({ { 0, (u32)vertex_count } });
		command_list->IASetIndexBuffer(&index_buffer_view);
		return std::make_tuple(true, index_count, vertex_buffer_view);
	}

	if (rsx::method_registers.current_draw_clause.command == rsx::draw_command::array)
	{
		if (is_primitive_native(rsx::method_registers.current_draw_clause.primitive))
		{
			size_t vertex_count = get_vertex_count(rsx::method_registers.current_draw_clause.first_count_commands);
			return std::make_tuple(false, vertex_count, upload_vertex_attributes(rsx::method_registers.current_draw_clause.first_count_commands, command_list));
		}

		D3D12_INDEX_BUFFER_VIEW index_buffer_view;
		size_t index_count;
		std::tie(index_buffer_view, index_count) = generate_index_buffer_for_emulated_primitives_array(rsx::method_registers.current_draw_clause.first_count_commands);
		command_list->IASetIndexBuffer(&index_buffer_view);
		return std::make_tuple(true, index_count, upload_vertex_attributes(rsx::method_registers.current_draw_clause.first_count_commands, command_list));
	}

	EXPECTS(rsx::method_registers.current_draw_clause.command == rsx::draw_command::indexed);

	// Index count
	size_t index_count = get_index_count(rsx::method_registers.current_draw_clause.primitive, gsl::narrow<int>(get_vertex_count(rsx::method_registers.current_draw_clause.first_count_commands)));

	rsx::index_array_type indexed_type = rsx::method_registers.index_type();
	size_t index_size = get_index_type_size(indexed_type);

	// Alloc
	size_t buffer_size = align(index_count * index_size, 64);
	size_t heap_offset = m_buffer_data.alloc<D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT>(buffer_size);

	void *mapped_buffer = m_buffer_data.map<void>(CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));
	u32 min_index, max_index;
	gsl::span<gsl::byte> dst{ reinterpret_cast<gsl::byte*>(mapped_buffer), gsl::narrow<u32>(buffer_size) };

	std::tie(min_index, max_index) = write_index_array_data_to_buffer(dst, get_raw_index_array(rsx::method_registers.current_draw_clause.first_count_commands),
		indexed_type, rsx::method_registers.current_draw_clause.primitive, rsx::method_registers.restart_index_enabled(), rsx::method_registers.restart_index(), rsx::method_registers.current_draw_clause.first_count_commands,
		[](auto prim) { return !is_primitive_native(prim); });

	m_buffer_data.unmap(CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));
	D3D12_INDEX_BUFFER_VIEW index_buffer_view = {
		m_buffer_data.get_heap()->GetGPUVirtualAddress() + heap_offset,
		(UINT)buffer_size,
		get_index_type(indexed_type)
	};
	m_timers.buffer_upload_size += buffer_size;
	command_list->IASetIndexBuffer(&index_buffer_view);

	return std::make_tuple(true, index_count, upload_vertex_attributes({ std::make_pair(0, max_index + 1) }, command_list));
}

#endif
