#include "stdafx_d3d12.h"
#ifdef _WIN32
#include "D3D12Buffer.h"
#include "Utilities/Log.h"

#include "D3D12GSRender.h"
#include "d3dx12.h"
#include "../Common/BufferUtils.h"
#include "D3D12Formats.h"

const int g_vertexCount = 32;


// D3D12GS member handling buffers

/**
 * 
 */
static
D3D12_GPU_VIRTUAL_ADDRESS createVertexBuffer(const rsx::data_array_format_info &vertex_array_desc, const std::vector<u8> &vertex_data, ID3D12Device *device, DataHeap<ID3D12Resource, 65536> &vertexIndexHeap)
{
	size_t subBufferSize = vertex_data.size();
	assert(vertexIndexHeap.canAlloc(subBufferSize));
	size_t heapOffset = vertexIndexHeap.alloc(subBufferSize);

	void *buffer;
	ThrowIfFailed(vertexIndexHeap.m_heap->Map(0, &CD3DX12_RANGE(heapOffset, heapOffset + subBufferSize), (void**)&buffer));
	void *bufferMap = (char*)buffer + heapOffset;
	memcpy(bufferMap, vertex_data.data(), vertex_data.size());
	vertexIndexHeap.m_heap->Unmap(0, &CD3DX12_RANGE(heapOffset, heapOffset + subBufferSize));
	return vertexIndexHeap.m_heap->GetGPUVirtualAddress() + heapOffset;
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
	size_t inputSlot = 0;

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

		size_t subBufferSize = element_size * vertex_count;
		assert(m_vertexIndexData.canAlloc(subBufferSize));
		size_t heapOffset = m_vertexIndexData.alloc(subBufferSize);

		void *buffer;
		ThrowIfFailed(m_vertexIndexData.m_heap->Map(0, &CD3DX12_RANGE(heapOffset, heapOffset + subBufferSize), (void**)&buffer));
		void *bufferMap = (char*)buffer + heapOffset;
		for (const auto &range : vertex_ranges)
		{
			write_vertex_array_data_to_buffer(bufferMap, range.first, range.second, index, info);
			bufferMap = (char*)bufferMap + range.second * element_size;
		}
		m_vertexIndexData.m_heap->Unmap(0, &CD3DX12_RANGE(heapOffset, heapOffset + subBufferSize));

		D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
		vertexBufferView.BufferLocation = m_vertexIndexData.m_heap->GetGPUVirtualAddress() + heapOffset;
		vertexBufferView.SizeInBytes = (UINT)subBufferSize;
		vertexBufferView.StrideInBytes = (UINT)element_size;
		m_vertex_buffer_views.push_back(vertexBufferView);

		m_timers.m_bufferUploadSize += subBufferSize;

		D3D12_INPUT_ELEMENT_DESC IAElement = {};
		IAElement.SemanticName = "TEXCOORD";
		IAElement.SemanticIndex = (UINT)index;
		IAElement.InputSlot = (UINT)inputSlot++;
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

		size_t subBufferSize = data.size();
		assert(m_vertexIndexData.canAlloc(subBufferSize));
		size_t heapOffset = m_vertexIndexData.alloc(subBufferSize);

		void *buffer;
		ThrowIfFailed(m_vertexIndexData.m_heap->Map(0, &CD3DX12_RANGE(heapOffset, heapOffset + subBufferSize), (void**)&buffer));
		void *bufferMap = (char*)buffer + heapOffset;
		memcpy(bufferMap, data.data(), data.size());
		m_vertexIndexData.m_heap->Unmap(0, &CD3DX12_RANGE(heapOffset, heapOffset + subBufferSize));

		D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
		vertexBufferView.BufferLocation = m_vertexIndexData.m_heap->GetGPUVirtualAddress() + heapOffset;
		vertexBufferView.SizeInBytes = (UINT)subBufferSize;
		vertexBufferView.StrideInBytes = (UINT)element_size;
		m_vertex_buffer_views.push_back(vertexBufferView);

		D3D12_INPUT_ELEMENT_DESC IAElement = {};
		IAElement.SemanticName = "TEXCOORD";
		IAElement.SemanticIndex = (UINT)index;
		IAElement.InputSlot = (UINT)inputSlot++;
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
	float scaleOffsetMat[16] =
	{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	int clip_w = rsx::method_registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL] >> 16;
	int clip_h = rsx::method_registers[NV4097_SET_SURFACE_CLIP_VERTICAL] >> 16;

	// Scale
	scaleOffsetMat[0] *= (float&)rsx::method_registers[NV4097_SET_VIEWPORT_SCALE] / (clip_w / 2.f);
	scaleOffsetMat[5] *= (float&)rsx::method_registers[NV4097_SET_VIEWPORT_SCALE + 1] / (clip_h / 2.f);
	scaleOffsetMat[10] = (float&)rsx::method_registers[NV4097_SET_VIEWPORT_SCALE + 2];

	// Offset
	scaleOffsetMat[3] = (float&)rsx::method_registers[NV4097_SET_VIEWPORT_OFFSET] - (clip_w / 2.f);
	scaleOffsetMat[7] = -((float&)rsx::method_registers[NV4097_SET_VIEWPORT_OFFSET + 1] - (clip_h / 2.f));
	scaleOffsetMat[11] = (float&)rsx::method_registers[NV4097_SET_VIEWPORT_OFFSET + 2];

	scaleOffsetMat[3] /= clip_w / 2.f;
	scaleOffsetMat[7] /= clip_h / 2.f;

	assert(m_constantsData.canAlloc(256));
	size_t heapOffset = m_constantsData.alloc(256);

	// Scale offset buffer
	// Separate constant buffer
	void *scaleOffsetMap;
	ThrowIfFailed(m_constantsData.m_heap->Map(0, &CD3DX12_RANGE(heapOffset, heapOffset + 256), &scaleOffsetMap));
	streamToBuffer((char*)scaleOffsetMap + heapOffset, scaleOffsetMat, 16 * sizeof(float));
	int isAlphaTested = !!(rsx::method_registers[NV4097_SET_ALPHA_TEST_ENABLE]);
	float alpha_ref = (float&)rsx::method_registers[NV4097_SET_ALPHA_REF];
	memcpy((char*)scaleOffsetMap + heapOffset + 16 * sizeof(float), &isAlphaTested, sizeof(int));
	memcpy((char*)scaleOffsetMap + heapOffset + 17 * sizeof(float), &alpha_ref, sizeof(float));
	m_constantsData.m_heap->Unmap(0, &CD3DX12_RANGE(heapOffset, heapOffset + 256));

	D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferViewDesc = {};
	constantBufferViewDesc.BufferLocation = m_constantsData.m_heap->GetGPUVirtualAddress() + heapOffset;
	constantBufferViewDesc.SizeInBytes = (UINT)256;
	m_device->CreateConstantBufferView(&constantBufferViewDesc,
		CD3DX12_CPU_DESCRIPTOR_HANDLE(getCurrentResourceStorage().m_descriptorsHeap->GetCPUDescriptorHandleForHeapStart())
		.Offset((INT)descriptorIndex, g_descriptorStrideSRVCBVUAV));
}

void D3D12GSRender::FillVertexShaderConstantsBuffer(size_t descriptorIndex)
{
	for (const auto &entry : transform_constants)
		local_transform_constants[entry.first] = entry.second;

	size_t bufferSize = 512 * 4 * sizeof(float);

	assert(m_constantsData.canAlloc(bufferSize));
	size_t heapOffset = m_constantsData.alloc(bufferSize);

	void *constantsBufferMap;
	ThrowIfFailed(m_constantsData.m_heap->Map(0, &CD3DX12_RANGE(heapOffset, heapOffset + bufferSize), &constantsBufferMap));
	for (const auto &entry : local_transform_constants)
	{
		float data[4] = {
			entry.second.x,
			entry.second.y,
			entry.second.z,
			entry.second.w
		};
		streamToBuffer((char*)constantsBufferMap + heapOffset + entry.first * 4 * sizeof(float), data, 4 * sizeof(float));
	}
	m_constantsData.m_heap->Unmap(0, &CD3DX12_RANGE(heapOffset, heapOffset + bufferSize));

	D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferViewDesc = {};
	constantBufferViewDesc.BufferLocation = m_constantsData.m_heap->GetGPUVirtualAddress() + heapOffset;
	constantBufferViewDesc.SizeInBytes = (UINT)bufferSize;
	m_device->CreateConstantBufferView(&constantBufferViewDesc,
		CD3DX12_CPU_DESCRIPTOR_HANDLE(getCurrentResourceStorage().m_descriptorsHeap->GetCPUDescriptorHandleForHeapStart())
		.Offset((INT)descriptorIndex, g_descriptorStrideSRVCBVUAV));
}

void D3D12GSRender::FillPixelShaderConstantsBuffer(size_t descriptorIndex)
{
	// Get constant from fragment program
	const std::vector<size_t> &fragmentOffset = m_cachePSO.getFragmentConstantOffsetsCache(&fragment_program);
	size_t bufferSize = fragmentOffset.size() * 4 * sizeof(float) + 1;
	// Multiple of 256 never 0
	bufferSize = (bufferSize + 255) & ~255;

	assert(m_constantsData.canAlloc(bufferSize));
	size_t heapOffset = m_constantsData.alloc(bufferSize);

	size_t offset = 0;
	void *constantsBufferMap;
	ThrowIfFailed(m_constantsData.m_heap->Map(0, &CD3DX12_RANGE(heapOffset, heapOffset + bufferSize), &constantsBufferMap));
	for (size_t offsetInFP : fragmentOffset)
	{
		u32 vector[4];
		// Is it assigned by color register in command buffer ?
		// TODO : we loop every iteration, we might do better...
		bool isCommandBufferSetConstant = false;
/*		for (const auto& entry : fragment_constants)
		{
			size_t fragmentId = entry.first - fragment_program.offset;
			if (fragmentId == offsetInFP)
			{
				isCommandBufferSetConstant = true;
				vector[0] = (u32&)entry.second.x;
				vector[1] = (u32&)entry.second.y;
				vector[2] = (u32&)entry.second.z;
				vector[3] = (u32&)entry.second.w;
				break;
			}
		}*/
		if (!isCommandBufferSetConstant)
		{
			auto data = vm::ps3::ptr<u32>::make(fragment_program.addr + (u32)offsetInFP);

			u32 c0 = (data[0] >> 16 | data[0] << 16);
			u32 c1 = (data[1] >> 16 | data[1] << 16);
			u32 c2 = (data[2] >> 16 | data[2] << 16);
			u32 c3 = (data[3] >> 16 | data[3] << 16);

			vector[0] = c0;
			vector[1] = c1;
			vector[2] = c2;
			vector[3] = c3;
		}

		streamToBuffer((char*)constantsBufferMap + heapOffset + offset, vector, 4 * sizeof(u32));
		offset += 4 * sizeof(u32);
	}
	m_constantsData.m_heap->Unmap(0, &CD3DX12_RANGE(heapOffset, heapOffset + bufferSize));

	D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferViewDesc = {};
	constantBufferViewDesc.BufferLocation = m_constantsData.m_heap->GetGPUVirtualAddress() + heapOffset;
	constantBufferViewDesc.SizeInBytes = (UINT)bufferSize;
	m_device->CreateConstantBufferView(&constantBufferViewDesc,
		CD3DX12_CPU_DESCRIPTOR_HANDLE(getCurrentResourceStorage().m_descriptorsHeap->GetCPUDescriptorHandleForHeapStart())
		.Offset((INT)descriptorIndex, g_descriptorStrideSRVCBVUAV));
}

void D3D12GSRender::upload_vertex_index_data(ID3D12GraphicsCommandList *cmdlist)
{
	// Index count
	m_renderingInfo.m_count = 0;
	for (const auto &pair : m_first_count_pairs)
		m_renderingInfo.m_count += getIndexCount(draw_mode, pair.second);

	if (!m_renderingInfo.m_indexed)
	{
		// Non indexed
		upload_vertex_attributes(m_first_count_pairs);
		cmdlist->IASetVertexBuffers(0, (UINT)m_vertex_buffer_views.size(), m_vertex_buffer_views.data());
		if (isNativePrimitiveMode(draw_mode))
			return;
		// Handle non native primitive

		// Alloc
		size_t subBufferSize = align(m_renderingInfo.m_count * sizeof(u16), 64);
		assert(m_vertexIndexData.canAlloc(subBufferSize));
		size_t heapOffset = m_vertexIndexData.alloc(subBufferSize);

		void *buffer;
		ThrowIfFailed(m_vertexIndexData.m_heap->Map(0, &CD3DX12_RANGE(heapOffset, heapOffset + subBufferSize), (void**)&buffer));
		void *bufferMap = (char*)buffer + heapOffset;
		size_t first = 0;
		for (const auto &pair : m_first_count_pairs)
		{
			size_t element_count = getIndexCount(draw_mode, pair.second);
			write_index_array_for_non_indexed_non_native_primitive_to_buffer((char*)bufferMap, draw_mode, first, pair.second);
			bufferMap = (char*)bufferMap + element_count * sizeof(u16);
			first += pair.second;
		}
		m_vertexIndexData.m_heap->Unmap(0, &CD3DX12_RANGE(heapOffset, heapOffset + subBufferSize));
		D3D12_INDEX_BUFFER_VIEW indexBufferView = {
			m_vertexIndexData.m_heap->GetGPUVirtualAddress() + heapOffset,
			(UINT)subBufferSize,
			DXGI_FORMAT_R16_UINT
		};
		cmdlist->IASetIndexBuffer(&indexBufferView);
		m_renderingInfo.m_indexed = true;
	}
	else
	{
		u32 indexed_type = rsx::method_registers[NV4097_SET_INDEX_ARRAY_DMA] >> 4;

		// Index type
		size_t indexSize = (indexed_type == CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16) ? 2 : 4;

		// Alloc
		size_t subBufferSize = align(m_renderingInfo.m_count * indexSize, 64);
		assert(m_vertexIndexData.canAlloc(subBufferSize));
		size_t heapOffset = m_vertexIndexData.alloc(subBufferSize);

		void *buffer;
		ThrowIfFailed(m_vertexIndexData.m_heap->Map(0, &CD3DX12_RANGE(heapOffset, heapOffset + subBufferSize), (void**)&buffer));
		void *bufferMap = (char*)buffer + heapOffset;
		u32 min_index = (u32)-1, max_index = 0;
		for (const auto &pair : m_first_count_pairs)
		{
			size_t element_count = getIndexCount(draw_mode, pair.second);
			write_index_array_data_to_buffer((char*)bufferMap, draw_mode, pair.first, pair.second, min_index, max_index);
			bufferMap = (char*)bufferMap + element_count * indexSize;
		}
		m_vertexIndexData.m_heap->Unmap(0, &CD3DX12_RANGE(heapOffset, heapOffset + subBufferSize));
		D3D12_INDEX_BUFFER_VIEW indexBufferView = {
			m_vertexIndexData.m_heap->GetGPUVirtualAddress() + heapOffset,
			(UINT)subBufferSize,
			get_index_type(indexed_type)
		};
		m_timers.m_bufferUploadSize += subBufferSize;
		cmdlist->IASetIndexBuffer(&indexBufferView);
		m_renderingInfo.m_indexed = true;

		upload_vertex_attributes({ std::make_pair(0, max_index + 1) });
		cmdlist->IASetVertexBuffers(0, (UINT)m_vertex_buffer_views.size(), m_vertex_buffer_views.data());
	}
}

#endif
