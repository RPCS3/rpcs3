#include "stdafx_d3d12.h"
#ifdef _WIN32
#include "Utilities/Log.h"

#include "D3D12GSRender.h"
#include "d3dx12.h"
#include "../Common/BufferUtils.h"
#include "D3D12Formats.h"

namespace
{
/**
 * 
 */
D3D12_GPU_VIRTUAL_ADDRESS createVertexBuffer(const rsx::data_array_format_info &vertex_array_desc, const std::vector<u8> &vertex_data, ID3D12Device *device, data_heap<ID3D12Resource, 65536> &vertex_index_heap)
{
	size_t buffer_size = vertex_data.size();
	assert(vertex_index_heap.can_alloc(buffer_size));
	size_t heap_offset = vertex_index_heap.alloc(buffer_size);

	void *buffer;
	ThrowIfFailed(vertex_index_heap.m_heap->Map(0, &CD3DX12_RANGE(heap_offset, heap_offset + buffer_size), (void**)&buffer));
	void *bufferMap = (char*)buffer + heap_offset;
	memcpy(bufferMap, vertex_data.data(), vertex_data.size());
	vertex_index_heap.m_heap->Unmap(0, &CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));
	return vertex_index_heap.m_heap->GetGPUVirtualAddress() + heap_offset;
}

}

void D3D12GSRender::load_vertex_data(u32 first, u32 count)
{
	m_first_count_pairs.emplace_back(std::make_pair(first, count));
	vertex_draw_count += count;
}

void D3D12GSRender::upload_vertex_attributes(const std::vector<std::pair<u32, u32> > &vertex_ranges)
{
	m_vertex_buffer_views.clear();
	m_IASet.clear();
	size_t input_slot = 0;

	size_t vertex_count = 0;

	for (const auto &pair : vertex_ranges)
		vertex_count += pair.second;

	// First array attribute
	for (int index = 0; index < rsx::limits::vertex_count; ++index)
	{
		const auto &info = vertex_arrays_info[index];

		if (!info.array) // disabled or not a vertex array
			continue;

		u32 type_size = rsx::get_vertex_type_size(info.type);
		u32 element_size = type_size * info.size;

		size_t buffer_size = element_size * vertex_count;
		assert(m_vertexIndexData.can_alloc(buffer_size));
		size_t heap_offset = m_vertexIndexData.alloc(buffer_size);

		void *buffer;
		ThrowIfFailed(m_vertexIndexData.m_heap->Map(0, &CD3DX12_RANGE(heap_offset, heap_offset + buffer_size), (void**)&buffer));
		void *mapped_buffer = (char*)buffer + heap_offset;
		for (const auto &range : vertex_ranges)
		{
			write_vertex_array_data_to_buffer(mapped_buffer, range.first, range.second, index, info);
			mapped_buffer = (char*)mapped_buffer + range.second * element_size;
		}
		m_vertexIndexData.m_heap->Unmap(0, &CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));

		D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view =
		{
			m_vertexIndexData.m_heap->GetGPUVirtualAddress() + heap_offset,
			(UINT)buffer_size,
			(UINT)element_size
		};
		m_vertex_buffer_views.push_back(vertex_buffer_view);

		m_timers.m_bufferUploadSize += buffer_size;

		D3D12_INPUT_ELEMENT_DESC IAElement = {};
		IAElement.SemanticName = "TEXCOORD";
		IAElement.SemanticIndex = (UINT)index;
		IAElement.InputSlot = (UINT)input_slot++;
		IAElement.Format = get_vertex_attribute_format(info.type, info.size);
		IAElement.AlignedByteOffset = 0;
		IAElement.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		IAElement.InstanceDataStepRate = 0;
		m_IASet.push_back(IAElement);
	}

	// Now immediate vertex buffer
	for (int index = 0; index < rsx::limits::vertex_count; ++index)
	{
		const auto &info = vertex_arrays_info[index];

		if (info.array)
			continue;
		if (!info.size) // disabled
			continue;

		auto &data = vertex_arrays[index];

		u32 type_size = rsx::get_vertex_type_size(info.type);
		u32 element_size = type_size * info.size;

		size_t buffer_size = data.size();
		assert(m_vertexIndexData.can_alloc(buffer_size));
		size_t heap_offset = m_vertexIndexData.alloc(buffer_size);

		void *buffer;
		ThrowIfFailed(m_vertexIndexData.m_heap->Map(0, &CD3DX12_RANGE(heap_offset, heap_offset + buffer_size), (void**)&buffer));
		void *mapped_buffer = (char*)buffer + heap_offset;
		memcpy(mapped_buffer, data.data(), data.size());
		m_vertexIndexData.m_heap->Unmap(0, &CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));

		D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view = {
			m_vertexIndexData.m_heap->GetGPUVirtualAddress() + heap_offset,
			(UINT)buffer_size,
			(UINT)element_size
		};
		m_vertex_buffer_views.push_back(vertex_buffer_view);

		D3D12_INPUT_ELEMENT_DESC IAElement = {};
		IAElement.SemanticName = "TEXCOORD";
		IAElement.SemanticIndex = (UINT)index;
		IAElement.InputSlot = (UINT)input_slot++;
		IAElement.Format = get_vertex_attribute_format(info.type, info.size);
		IAElement.AlignedByteOffset = 0;
		IAElement.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
		IAElement.InstanceDataStepRate = 1;
		m_IASet.push_back(IAElement);
	}
}

void D3D12GSRender::load_vertex_index_data(u32 first, u32 count)
{
	m_renderingInfo.m_indexed = true;
}

void D3D12GSRender::setScaleOffset(size_t descriptorIndex)
{
	float scale_offset_matrix[16] =
	{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	int clip_w = rsx::method_registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL] >> 16;
	int clip_h = rsx::method_registers[NV4097_SET_SURFACE_CLIP_VERTICAL] >> 16;

	// Scale
	scale_offset_matrix[0] *= (float&)rsx::method_registers[NV4097_SET_VIEWPORT_SCALE] / (clip_w / 2.f);
	scale_offset_matrix[5] *= (float&)rsx::method_registers[NV4097_SET_VIEWPORT_SCALE + 1] / (clip_h / 2.f);
	scale_offset_matrix[10] = (float&)rsx::method_registers[NV4097_SET_VIEWPORT_SCALE + 2];

	// Offset
	scale_offset_matrix[3] = (float&)rsx::method_registers[NV4097_SET_VIEWPORT_OFFSET] - (clip_w / 2.f);
	scale_offset_matrix[7] = -((float&)rsx::method_registers[NV4097_SET_VIEWPORT_OFFSET + 1] - (clip_h / 2.f));
	scale_offset_matrix[11] = (float&)rsx::method_registers[NV4097_SET_VIEWPORT_OFFSET + 2];

	scale_offset_matrix[3] /= clip_w / 2.f;
	scale_offset_matrix[7] /= clip_h / 2.f;

	assert(m_constantsData.can_alloc(256));
	size_t heap_offset = m_constantsData.alloc(256);

	// Scale offset buffer
	// Separate constant buffer
	void *mapped_buffer;
	ThrowIfFailed(m_constantsData.m_heap->Map(0, &CD3DX12_RANGE(heap_offset, heap_offset + 256), &mapped_buffer));
	streamToBuffer((char*)mapped_buffer + heap_offset, scale_offset_matrix, 16 * sizeof(float));
	int is_alpha_tested = !!(rsx::method_registers[NV4097_SET_ALPHA_TEST_ENABLE]);
	float alpha_ref = (float&)rsx::method_registers[NV4097_SET_ALPHA_REF];
	memcpy((char*)mapped_buffer + heap_offset + 16 * sizeof(float), &is_alpha_tested, sizeof(int));
	memcpy((char*)mapped_buffer + heap_offset + 17 * sizeof(float), &alpha_ref, sizeof(float));
	m_constantsData.m_heap->Unmap(0, &CD3DX12_RANGE(heap_offset, heap_offset + 256));

	D3D12_CONSTANT_BUFFER_VIEW_DESC constant_buffer_view_desc = {
		m_constantsData.m_heap->GetGPUVirtualAddress() + heap_offset,
		256
	};
	m_device->CreateConstantBufferView(&constant_buffer_view_desc,
		CD3DX12_CPU_DESCRIPTOR_HANDLE(getCurrentResourceStorage().descriptors_heap->GetCPUDescriptorHandleForHeapStart())
		.Offset((INT)descriptorIndex, g_descriptorStrideSRVCBVUAV));
}

void D3D12GSRender::FillVertexShaderConstantsBuffer(size_t descriptor_index)
{
	for (const auto &entry : transform_constants)
		local_transform_constants[entry.first] = entry.second;

	size_t buffer_size = 512 * 4 * sizeof(float);

	assert(m_constantsData.can_alloc(buffer_size));
	size_t heap_offset = m_constantsData.alloc(buffer_size);

	void *mapped_buffer;
	ThrowIfFailed(m_constantsData.m_heap->Map(0, &CD3DX12_RANGE(heap_offset, heap_offset + buffer_size), &mapped_buffer));
	for (const auto &entry : local_transform_constants)
	{
		float data[4] = {
			entry.second.x,
			entry.second.y,
			entry.second.z,
			entry.second.w
		};
		streamToBuffer((char*)mapped_buffer + heap_offset + entry.first * 4 * sizeof(float), data, 4 * sizeof(float));
	}
	m_constantsData.m_heap->Unmap(0, &CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));

	D3D12_CONSTANT_BUFFER_VIEW_DESC constant_buffer_view_desc = {
		m_constantsData.m_heap->GetGPUVirtualAddress() + heap_offset,
		(UINT)buffer_size
	};
	m_device->CreateConstantBufferView(&constant_buffer_view_desc,
		CD3DX12_CPU_DESCRIPTOR_HANDLE(getCurrentResourceStorage().descriptors_heap->GetCPUDescriptorHandleForHeapStart())
		.Offset((INT)descriptor_index, g_descriptorStrideSRVCBVUAV));
}

void D3D12GSRender::FillPixelShaderConstantsBuffer(size_t descriptor_index)
{
	// Get constant from fragment program
	const std::vector<size_t> &fragment_constant_offsets = m_cachePSO.getFragmentConstantOffsetsCache(&fragment_program);
	size_t buffer_size = fragment_constant_offsets.size() * 4 * sizeof(float) + 1;
	// Multiple of 256 never 0
	buffer_size = (buffer_size + 255) & ~255;

	assert(m_constantsData.can_alloc(buffer_size));
	size_t heap_offset = m_constantsData.alloc(buffer_size);

	size_t offset = 0;
	void *mapped_buffer;
	ThrowIfFailed(m_constantsData.m_heap->Map(0, &CD3DX12_RANGE(heap_offset, heap_offset + buffer_size), &mapped_buffer));
	for (size_t offset_in_fragment_program : fragment_constant_offsets)
	{
		u32 vector[4];
		auto data = vm::ps3::ptr<u32>::make(fragment_program.addr + (u32)offset_in_fragment_program);

		u32 c0 = (data[0] >> 16 | data[0] << 16);
		u32 c1 = (data[1] >> 16 | data[1] << 16);
		u32 c2 = (data[2] >> 16 | data[2] << 16);
		u32 c3 = (data[3] >> 16 | data[3] << 16);

		vector[0] = c0;
		vector[1] = c1;
		vector[2] = c2;
		vector[3] = c3;

		streamToBuffer((char*)mapped_buffer + heap_offset + offset, vector, 4 * sizeof(u32));
		offset += 4 * sizeof(u32);
	}
	m_constantsData.m_heap->Unmap(0, &CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));

	D3D12_CONSTANT_BUFFER_VIEW_DESC constant_buffer_view_desc = {
		m_constantsData.m_heap->GetGPUVirtualAddress() + heap_offset,
		(UINT)buffer_size
	};
	m_device->CreateConstantBufferView(&constant_buffer_view_desc,
		CD3DX12_CPU_DESCRIPTOR_HANDLE(getCurrentResourceStorage().descriptors_heap->GetCPUDescriptorHandleForHeapStart())
		.Offset((INT)descriptor_index, g_descriptorStrideSRVCBVUAV));
}

void D3D12GSRender::upload_vertex_index_data(ID3D12GraphicsCommandList *command_list)
{
	// Index count
	m_renderingInfo.m_count = 0;
	for (const auto &pair : m_first_count_pairs)
		m_renderingInfo.m_count += get_index_count(draw_mode, pair.second);

	if (!m_renderingInfo.m_indexed)
	{
		// Non indexed
		upload_vertex_attributes(m_first_count_pairs);
		command_list->IASetVertexBuffers(0, (UINT)m_vertex_buffer_views.size(), m_vertex_buffer_views.data());
		if (is_primitive_native(draw_mode))
			return;
		// Handle non native primitive

		// Alloc
		size_t buffer_size = align(m_renderingInfo.m_count * sizeof(u16), 64);
		assert(m_vertexIndexData.can_alloc(buffer_size));
		size_t heap_offset = m_vertexIndexData.alloc(buffer_size);

		void *buffer;
		ThrowIfFailed(m_vertexIndexData.m_heap->Map(0, &CD3DX12_RANGE(heap_offset, heap_offset + buffer_size), (void**)&buffer));
		void *mapped_buffer = (char*)buffer + heap_offset;
		size_t first = 0;
		for (const auto &pair : m_first_count_pairs)
		{
			size_t element_count = get_index_count(draw_mode, pair.second);
			write_index_array_for_non_indexed_non_native_primitive_to_buffer((char*)mapped_buffer, draw_mode, (u32)first, (u32)pair.second);
			mapped_buffer = (char*)mapped_buffer + element_count * sizeof(u16);
			first += pair.second;
		}
		m_vertexIndexData.m_heap->Unmap(0, &CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));
		D3D12_INDEX_BUFFER_VIEW index_buffer_view = {
			m_vertexIndexData.m_heap->GetGPUVirtualAddress() + heap_offset,
			(UINT)buffer_size,
			DXGI_FORMAT_R16_UINT
		};
		command_list->IASetIndexBuffer(&index_buffer_view);
		m_renderingInfo.m_indexed = true;
	}
	else
	{
		u32 indexed_type = rsx::method_registers[NV4097_SET_INDEX_ARRAY_DMA] >> 4;

		// Index type
		size_t index_size = get_index_type_size(indexed_type);

		// Alloc
		size_t buffer_size = align(m_renderingInfo.m_count * index_size, 64);
		assert(m_vertexIndexData.can_alloc(buffer_size));
		size_t heap_offset = m_vertexIndexData.alloc(buffer_size);

		void *buffer;
		ThrowIfFailed(m_vertexIndexData.m_heap->Map(0, &CD3DX12_RANGE(heap_offset, heap_offset + buffer_size), (void**)&buffer));
		void *mapped_buffer = (char*)buffer + heap_offset;
		u32 min_index = (u32)-1, max_index = 0;
		for (const auto &pair : m_first_count_pairs)
		{
			size_t element_count = get_index_count(draw_mode, pair.second);
			write_index_array_data_to_buffer((char*)mapped_buffer, draw_mode, pair.first, pair.second, min_index, max_index);
			mapped_buffer = (char*)mapped_buffer + element_count * index_size;
		}
		m_vertexIndexData.m_heap->Unmap(0, &CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));
		D3D12_INDEX_BUFFER_VIEW index_buffer_view = {
			m_vertexIndexData.m_heap->GetGPUVirtualAddress() + heap_offset,
			(UINT)buffer_size,
			get_index_type(indexed_type)
		};
		m_timers.m_bufferUploadSize += buffer_size;
		command_list->IASetIndexBuffer(&index_buffer_view);
		m_renderingInfo.m_indexed = true;

		upload_vertex_attributes({ std::make_pair(0, max_index + 1) });
		command_list->IASetVertexBuffers(0, (UINT)m_vertex_buffer_views.size(), m_vertex_buffer_views.data());
	}
}

#endif
