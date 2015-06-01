#include "stdafx.h"
#if defined(DX12_SUPPORT)
#include "D3D12Buffer.h"
#include "Utilities/Log.h"

#include "D3D12GSRender.h"

const int g_vertexCount = 32;

// Where are these type defined ???
static
DXGI_FORMAT getFormat(u8 type, u8 size)
{
	/*static const u32 gl_types[] =
	{
	GL_SHORT,
	GL_FLOAT,
	GL_HALF_FLOAT,
	GL_UNSIGNED_BYTE,
	GL_SHORT,
	GL_FLOAT, // Needs conversion
	GL_UNSIGNED_BYTE,
	};

	static const bool gl_normalized[] =
	{
	GL_TRUE,
	GL_FALSE,
	GL_FALSE,
	GL_TRUE,
	GL_FALSE,
	GL_TRUE,
	GL_FALSE,
	};*/
	static const DXGI_FORMAT typeX1[] =
	{
		DXGI_FORMAT_R16_SNORM,
		DXGI_FORMAT_R32_FLOAT,
		DXGI_FORMAT_R16_FLOAT,
		DXGI_FORMAT_R8_UNORM,
		DXGI_FORMAT_R16_SINT,
		DXGI_FORMAT_R32_FLOAT,
		DXGI_FORMAT_R8_UINT
	};
	static const DXGI_FORMAT typeX2[] =
	{
		DXGI_FORMAT_R16G16_SNORM,
		DXGI_FORMAT_R32G32_FLOAT,
		DXGI_FORMAT_R16G16_FLOAT,
		DXGI_FORMAT_R8G8_UNORM,
		DXGI_FORMAT_R16G16_SINT,
		DXGI_FORMAT_R32G32_FLOAT,
		DXGI_FORMAT_R8G8_UINT
	};
	static const DXGI_FORMAT typeX3[] =
	{
		DXGI_FORMAT_R16G16B16A16_SNORM,
		DXGI_FORMAT_R32G32B32_FLOAT,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT_R16G16B16A16_SINT,
		DXGI_FORMAT_R32G32B32_FLOAT,
		DXGI_FORMAT_R8G8B8A8_UINT
	};
	static const DXGI_FORMAT typeX4[] =
	{
		DXGI_FORMAT_R16G16B16A16_SNORM,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT_R16G16B16A16_SINT,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		DXGI_FORMAT_R8G8B8A8_UINT
	};

	switch (size)
	{
	case 1:
		return typeX1[type];
	case 2:
		return typeX2[type];
	case 3:
		return typeX3[type];
	case 4:
		return typeX4[type];
	}
}

struct VertexBufferFormat
{
	std::pair<size_t, size_t> range;
	std::vector<size_t> attributeId;
	size_t elementCount;
	size_t stride;
};

std::vector<D3D12_INPUT_ELEMENT_DESC> getIALayout(ID3D12Device *device, const std::vector<VertexBufferFormat> &vertexBufferFormat, const RSXVertexData *vertexData)
{
	std::vector<D3D12_INPUT_ELEMENT_DESC> result;

	for (size_t inputSlot = 0; inputSlot < vertexBufferFormat.size(); inputSlot++)
	{
		for (size_t attributeId : vertexBufferFormat[inputSlot].attributeId)
		{
			D3D12_INPUT_ELEMENT_DESC IAElement = {};
			IAElement.SemanticName = "TEXCOORD";
			IAElement.SemanticIndex = (UINT)attributeId;
			IAElement.InputSlot = (UINT)inputSlot;
			IAElement.Format = getFormat(vertexData[attributeId].type - 1, vertexData[attributeId].size);
			IAElement.AlignedByteOffset = (UINT)(vertexData[attributeId].addr - vertexBufferFormat[inputSlot].range.first);
			result.push_back(IAElement);
		}
	}
	return result;
}

template<typename IndexType, typename DstType, typename SrcType>
void expandIndexedQuads(DstType *dst, const SrcType *src, size_t indexCount)
{
	IndexType *typedDst = reinterpret_cast<IndexType *>(dst);
	const IndexType *typedSrc = reinterpret_cast<const IndexType *>(src);
	for (unsigned i = 0; i < indexCount / 4; i++)
	{
		// First triangle
		typedDst[6 * i] = typedSrc[4 * i];
		typedDst[6 * i + 1] = typedSrc[4 * i + 1];
		typedDst[6 * i + 2] = typedSrc[4 * i + 2];
		// Second triangle
		typedDst[6 * i + 3] = typedSrc[4 * i + 2];
		typedDst[6 * i + 4] = typedSrc[4 * i + 3];
		typedDst[6 * i + 5] = typedSrc[4 * i];
	}
}



// D3D12GS member handling buffers



#define MIN2(x, y) ((x) < (y)) ? (x) : (y)
#define MAX2(x, y) ((x) > (y)) ? (x) : (y)

static
bool overlaps(const std::pair<size_t, size_t> &range1, const std::pair<size_t, size_t> &range2)
{
	return !(range1.second < range2.first || range2.second < range1.first);
}

static
std::vector<VertexBufferFormat> FormatVertexData(RSXVertexData *m_vertex_data)
{
	std::vector<VertexBufferFormat> Result;
	for (size_t i = 0; i < 32; ++i)
	{
		if (!m_vertex_data[i].IsEnabled()) continue;
		size_t elementCount = m_vertex_data[i].data.size() / (m_vertex_data[i].size * m_vertex_data[i].GetTypeSize());
		// If there is a single element, stride is 0, use the size of element instead
		size_t stride = m_vertex_data[i].stride;
		size_t elementSize = m_vertex_data[i].GetTypeSize();
		std::pair<size_t, size_t> range = std::make_pair(m_vertex_data[i].addr, m_vertex_data[i].addr + elementSize + elementCount * stride);
		bool isMerged = false;

		for (VertexBufferFormat &vbf : Result)
		{
			if (overlaps(vbf.range, range) && vbf.stride == stride)
			{
				// Extend buffer if necessary
				vbf.range.first = MIN2(vbf.range.first, range.first);
				vbf.range.second = MAX2(vbf.range.second, range.second);
				vbf.elementCount = MAX2(vbf.elementCount, elementCount);

				vbf.attributeId.push_back(i);
				isMerged = true;
				break;
			}
		}
		if (isMerged)
			continue;
		VertexBufferFormat newRange = { range, std::vector<size_t>{ i }, elementCount, stride };
		Result.emplace_back(newRange);
	}
	return Result;
}

std::pair<std::vector<D3D12_VERTEX_BUFFER_VIEW>, D3D12_INDEX_BUFFER_VIEW> D3D12GSRender::EnableVertexData(bool indexed_draw)
{
	std::pair<std::vector<D3D12_VERTEX_BUFFER_VIEW>, D3D12_INDEX_BUFFER_VIEW> result;
	const std::vector<VertexBufferFormat> &vertexBufferFormat = FormatVertexData(m_vertex_data);
	m_IASet = getIALayout(m_device, vertexBufferFormat, m_vertex_data);

	const u32 data_offset = indexed_draw ? 0 : m_draw_array_first;

	for (size_t buffer = 0; buffer < vertexBufferFormat.size(); buffer++)
	{
		const VertexBufferFormat &vbf = vertexBufferFormat[buffer];
		// 65536 alignment
		size_t bufferHeapOffset = m_perFrameStorage.m_vertexIndexBuffersHeapFreeSpace;
		bufferHeapOffset = (bufferHeapOffset + 65536 - 1) & ~65535;
		size_t subBufferSize = vbf.range.second - vbf.range.first;

		ID3D12Resource *vertexBuffer;
		check(m_device->CreatePlacedResource(
			m_perFrameStorage.m_vertexIndexBuffersHeap,
			bufferHeapOffset,
			&getBufferResourceDesc(subBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&vertexBuffer)
			));
		void *bufferMap;
		check(vertexBuffer->Map(0, nullptr, (void**)&bufferMap));

		for (unsigned vertex = 0; vertex < vbf.elementCount; vertex++)
		{
			for (size_t attributeId : vbf.attributeId)
			{
				if (!m_vertex_data[attributeId].addr) continue;
				size_t baseOffset = m_vertex_data[attributeId].addr - vbf.range.first;
				size_t tsize = m_vertex_data[attributeId].GetTypeSize();
				size_t size = m_vertex_data[attributeId].size;
				auto src = vm::get_ptr<const u8>(m_vertex_data[attributeId].addr + vbf.stride * vertex);
				char* dst = (char*)bufferMap + baseOffset + vbf.stride * vertex;

				switch (tsize)
				{
				case 1:
				{
					memcpy(dst, src, size);
					break;
				}

				case 2:
				{
					const u16* c_src = (const u16*)src;
					u16* c_dst = (u16*)dst;
					for (u32 j = 0; j < size; ++j) *c_dst++ = re16(*c_src++);
					break;
				}

				case 4:
				{
					const u32* c_src = (const u32*)src;
					u32* c_dst = (u32*)dst;
					for (u32 j = 0; j < size; ++j) *c_dst++ = re32(*c_src++);
					break;
				}
				}
			}
		}

		vertexBuffer->Unmap(0, nullptr);
		m_perFrameStorage.m_inflightResources.push_back(vertexBuffer);

		D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
		vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
		vertexBufferView.SizeInBytes = (UINT)subBufferSize;
		vertexBufferView.StrideInBytes = (UINT)vbf.stride;
		result.first.push_back(vertexBufferView);
		m_perFrameStorage.m_vertexIndexBuffersHeapFreeSpace = bufferHeapOffset + subBufferSize;
	}

	// Only handle quads now
	switch (m_draw_mode - 1)
	{
	default:
	case GL_POINTS:
	case GL_LINES:
	case GL_LINE_LOOP:
	case GL_LINE_STRIP:
	case GL_TRIANGLES:
	case GL_TRIANGLE_STRIP:
	case GL_TRIANGLE_FAN:
	case GL_QUAD_STRIP:
	case GL_POLYGON:
		m_forcedIndexBuffer = false;
		break;
	case GL_QUADS:
		m_forcedIndexBuffer = true;
		break;
	}

	if (indexed_draw || m_forcedIndexBuffer)
	{
		D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
		size_t indexSize;

		if (!indexed_draw)
		{
			indexBufferView.Format = DXGI_FORMAT_R16_UINT;
			indexSize = 2;
		}
		else
		{
			switch (m_indexed_array.m_type)
			{
			default: abort();
			case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16:
				indexBufferView.Format = DXGI_FORMAT_R16_UINT;
				indexSize = 2;
				break;
			case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_32:
				indexBufferView.Format = DXGI_FORMAT_R32_UINT;
				indexSize = 4;
				break;
			}
		}

		if (indexed_draw && !m_forcedIndexBuffer)
			indexCount = m_indexed_array.m_data.size() / indexSize;
		else if (indexed_draw && m_forcedIndexBuffer)
			indexCount = 6 * m_indexed_array.m_data.size() / (4 * indexSize);
		else
			indexCount = m_draw_array_count * 6 / 4;
		size_t subBufferSize = powerOf2Align(indexCount * indexSize, 64);
		// 65536 alignment
		size_t bufferHeapOffset = m_perFrameStorage.m_vertexIndexBuffersHeapFreeSpace;
		bufferHeapOffset = (bufferHeapOffset + 65536 - 1) & ~65535;

		ID3D12Resource *indexBuffer;
		check(m_device->CreatePlacedResource(
			m_perFrameStorage.m_vertexIndexBuffersHeap,
			bufferHeapOffset,
			&getBufferResourceDesc(subBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&indexBuffer)
			));

		void *bufferMap;
		check(indexBuffer->Map(0, nullptr, (void**)&bufferMap));
		if (indexed_draw && !m_forcedIndexBuffer)
			streamBuffer(bufferMap, m_indexed_array.m_data.data(), subBufferSize);
		else if (indexed_draw && m_forcedIndexBuffer)
		{
			switch (m_indexed_array.m_type)
			{
			case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_32:
				expandIndexedQuads<unsigned int>(bufferMap, m_indexed_array.m_data.data(), m_indexed_array.m_data.size() / 4);
				break;
			case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16:
				expandIndexedQuads<unsigned short>(bufferMap, m_indexed_array.m_data.data(), m_indexed_array.m_data.size() / 2);
				break;
			}
		}
		else
		{
			unsigned short *typedDst = static_cast<unsigned short *>(bufferMap);
			for (unsigned i = 0; i < m_draw_array_count / 4; i++)
			{
				// First triangle
				typedDst[6 * i] = 4 * i;
				typedDst[6 * i + 1] = 4 * i + 1;
				typedDst[6 * i + 2] = 4 * i + 2;
				// Second triangle
				typedDst[6 * i + 3] = 4 * i + 2;
				typedDst[6 * i + 4] = 4 * i + 3;
				typedDst[6 * i + 5] = 4 * i;
			}
		}
		indexBuffer->Unmap(0, nullptr);
		m_perFrameStorage.m_inflightResources.push_back(indexBuffer);
		m_perFrameStorage.m_vertexIndexBuffersHeapFreeSpace = bufferHeapOffset + subBufferSize;


		indexBufferView.SizeInBytes = (UINT)subBufferSize;
		indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();

		result.second = indexBufferView;
	}
	return result;
}

void D3D12GSRender::setScaleOffset()
{
	float scaleOffsetMat[16] =
	{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	// Scale
	scaleOffsetMat[0] *= (float&)methodRegisters[NV4097_SET_VIEWPORT_SCALE + (0x4 * 0)] / (RSXThread::m_width / RSXThread::m_width_scale);
	scaleOffsetMat[5] *= (float&)methodRegisters[NV4097_SET_VIEWPORT_SCALE + (0x4 * 1)] / (RSXThread::m_height / RSXThread::m_height_scale);
	scaleOffsetMat[10] = (float&)methodRegisters[NV4097_SET_VIEWPORT_SCALE + (0x4 * 2)];

	// Offset
	scaleOffsetMat[3] = (float&)methodRegisters[NV4097_SET_VIEWPORT_OFFSET + (0x4 * 0)] - (RSXThread::m_width / RSXThread::m_width_scale);
	scaleOffsetMat[7] = -((float&)methodRegisters[NV4097_SET_VIEWPORT_OFFSET + (0x4 * 1)] - (RSXThread::m_height / RSXThread::m_height_scale));
	scaleOffsetMat[11] = (float&)methodRegisters[NV4097_SET_VIEWPORT_OFFSET + (0x4 * 2)];

	scaleOffsetMat[3] /= RSXThread::m_width / RSXThread::m_width_scale;
	scaleOffsetMat[7] /= RSXThread::m_height / RSXThread::m_height_scale;

	assert(m_constantsData.canAlloc(256));
	size_t heapOffset = m_constantsData.alloc(256);

	// Scale offset buffer
	// Separate constant buffer
	ID3D12Resource *scaleOffsetBuffer;
	check(m_device->CreatePlacedResource(
		m_constantsData.m_heap,
		heapOffset,
		&getBufferResourceDesc(256),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&scaleOffsetBuffer)
		));

	void *scaleOffsetMap;
	check(scaleOffsetBuffer->Map(0, nullptr, &scaleOffsetMap));
	streamToBuffer(scaleOffsetMap, scaleOffsetMat, 16 * sizeof(float));
	scaleOffsetBuffer->Unmap(0, nullptr);

	D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferViewDesc = {};
	constantBufferViewDesc.BufferLocation = scaleOffsetBuffer->GetGPUVirtualAddress();
	constantBufferViewDesc.SizeInBytes = (UINT)256;
	D3D12_CPU_DESCRIPTOR_HANDLE Handle = m_perFrameStorage.m_scaleOffsetDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	Handle.ptr += m_perFrameStorage.m_currentScaleOffsetBufferIndex * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_device->CreateConstantBufferView(&constantBufferViewDesc, Handle);
	m_constantsData.m_resourceStoredSinceLastSync.push_back(std::make_tuple(heapOffset, 256, scaleOffsetBuffer));
}

void D3D12GSRender::FillVertexShaderConstantsBuffer()
{
	for (const RSXTransformConstant& c : m_transform_constants)
	{
		size_t offset = c.id * 4 * sizeof(float);
		float vector[] = { c.x, c.y, c.z, c.w };
		memcpy((char*)vertexConstantShadowCopy + offset, vector, 4 * sizeof(float));
	}

	size_t constantBuffersHeapOffset = m_perFrameStorage.m_constantsBuffersHeapFreeSpace;
	// 65536 alignment
	constantBuffersHeapOffset = (constantBuffersHeapOffset + 65536 - 1) & ~65535;

	ID3D12Resource *constantsBuffer;
	check(m_device->CreatePlacedResource(
		m_perFrameStorage.m_constantsBuffersHeap,
		constantBuffersHeapOffset,
		&getBufferResourceDesc(512 * 4 * sizeof(float)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constantsBuffer)
		));

	void *constantsBufferMap;
	check(constantsBuffer->Map(0, nullptr, &constantsBufferMap));
	streamBuffer(constantsBufferMap, vertexConstantShadowCopy, 512 * 4 * sizeof(float));
	constantsBuffer->Unmap(0, nullptr);

	D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferViewDesc = {};
	constantBufferViewDesc.BufferLocation = constantsBuffer->GetGPUVirtualAddress();
	constantBufferViewDesc.SizeInBytes = 512 * 4 * sizeof(float);
	D3D12_CPU_DESCRIPTOR_HANDLE Handle = m_perFrameStorage.m_constantsBufferDescriptorsHeap->GetCPUDescriptorHandleForHeapStart();
	Handle.ptr += m_perFrameStorage.m_constantsBufferIndex * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_device->CreateConstantBufferView(&constantBufferViewDesc, Handle);
	m_perFrameStorage.m_constantsBuffersHeapFreeSpace = constantBuffersHeapOffset + 512 * 4 * sizeof(float);
	m_perFrameStorage.m_inflightResources.push_back(constantsBuffer);
}

void D3D12GSRender::FillPixelShaderConstantsBuffer()
{
	// Get constant from fragment program
	const std::vector<size_t> &fragmentOffset = m_cachePSO.getFragmentConstantOffsetsCache(m_cur_fragment_prog);
	size_t bufferSize = fragmentOffset.size() * 4 * sizeof(float) + 1;
	// Multiple of 256 never 0
	bufferSize = (bufferSize + 255) & ~255;

	size_t constantBuffersHeapOffset = m_perFrameStorage.m_constantsBuffersHeapFreeSpace;
	// 65536 alignment
	constantBuffersHeapOffset = (constantBuffersHeapOffset + 65536 - 1) & ~65535;

	ID3D12Resource *constantsBuffer;
	check(m_device->CreatePlacedResource(
		m_perFrameStorage.m_constantsBuffersHeap,
		constantBuffersHeapOffset,
		&getBufferResourceDesc(bufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constantsBuffer)
		));

	size_t offset = 0;
	void *constantsBufferMap;
	check(constantsBuffer->Map(0, nullptr, &constantsBufferMap));
	for (size_t offsetInFP : fragmentOffset)
	{
		u32 vector[4];
		// Is it assigned by color register in command buffer ?
		// TODO : we loop every iteration, we might do better...
		bool isCommandBufferSetConstant = false;
		for (const RSXTransformConstant& c : m_fragment_constants)
		{
			size_t fragmentId = c.id - m_cur_fragment_prog->offset;
			if (fragmentId == offsetInFP)
			{
				isCommandBufferSetConstant = true;
				vector[0] = (u32&)c.x;
				vector[1] = (u32&)c.y;
				vector[2] = (u32&)c.z;
				vector[3] = (u32&)c.w;
				break;
			}
		}
		if (!isCommandBufferSetConstant)
		{
			auto data = vm::ptr<u32>::make(m_cur_fragment_prog->addr + (u32)offsetInFP);

			u32 c0 = (data[0] >> 16 | data[0] << 16);
			u32 c1 = (data[1] >> 16 | data[1] << 16);
			u32 c2 = (data[2] >> 16 | data[2] << 16);
			u32 c3 = (data[3] >> 16 | data[3] << 16);

			vector[0] = c0;
			vector[1] = c1;
			vector[2] = c2;
			vector[3] = c3;
		}

		streamToBuffer((char*)constantsBufferMap + offset, vector, 4 * sizeof(u32));
		offset += 4 * sizeof(u32);
	}

	constantsBuffer->Unmap(0, nullptr);

	D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferViewDesc = {};
	constantBufferViewDesc.BufferLocation = constantsBuffer->GetGPUVirtualAddress();
	constantBufferViewDesc.SizeInBytes = (UINT)bufferSize;
	D3D12_CPU_DESCRIPTOR_HANDLE Handle = m_perFrameStorage.m_constantsBufferDescriptorsHeap->GetCPUDescriptorHandleForHeapStart();
	Handle.ptr += m_perFrameStorage.m_constantsBufferIndex * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_device->CreateConstantBufferView(&constantBufferViewDesc, Handle);
	m_perFrameStorage.m_constantsBuffersHeapFreeSpace = constantBuffersHeapOffset + bufferSize;
	m_perFrameStorage.m_inflightResources.push_back(constantsBuffer);
}


#endif