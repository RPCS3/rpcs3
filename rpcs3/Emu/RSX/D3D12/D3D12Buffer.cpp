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

std::vector<D3D12_INPUT_ELEMENT_DESC> getIALayout(ID3D12Device *device, bool indexedDraw, const RSXVertexData *vertexData)
{
	std::vector<D3D12_INPUT_ELEMENT_DESC> result;
	u32 offset_list[g_vertexCount];
	u32 cur_offset = 0;

	for (u32 i = 0; i < g_vertexCount; ++i)
	{
		if (!vertexData[i].IsEnabled()) continue;
		const size_t item_size = vertexData[i].GetTypeSize() * vertexData[i].size;
		offset_list[i] = (u32)item_size;
	}

#if	DUMP_VERTEX_DATA
	rFile dump("VertexDataArray.dump", rFile::write);
#endif

	size_t inputSlot = 0;
	for (u32 i = 0; i < g_vertexCount; ++i)
	{
		if (!vertexData[i].IsEnabled()) continue;

#if	DUMP_VERTEX_DATA
		dump.Write(wxString::Format("VertexData[%d]:\n", i));
		switch (m_vertex_data[i].type)
		{
		case CELL_GCM_VERTEX_S1:
			for (u32 j = 0; j < m_vertex_data[i].data.size(); j += 2)
			{
				dump.Write(wxString::Format("%d\n", *(u16*)&m_vertex_data[i].data[j]));
				if (!(((j + 2) / 2) % m_vertex_data[i].size)) dump.Write("\n");
			}
			break;

		case CELL_GCM_VERTEX_F:
			for (u32 j = 0; j < m_vertex_data[i].data.size(); j += 4)
			{
				dump.Write(wxString::Format("%.01f\n", *(float*)&m_vertex_data[i].data[j]));
				if (!(((j + 4) / 4) % m_vertex_data[i].size)) dump.Write("\n");
			}
			break;

		case CELL_GCM_VERTEX_SF:
			for (u32 j = 0; j < m_vertex_data[i].data.size(); j += 2)
			{
				dump.Write(wxString::Format("%.01f\n", *(float*)&m_vertex_data[i].data[j]));
				if (!(((j + 2) / 2) % m_vertex_data[i].size)) dump.Write("\n");
			}
			break;

		case CELL_GCM_VERTEX_UB:
			for (u32 j = 0; j < m_vertex_data[i].data.size(); ++j)
			{
				dump.Write(wxString::Format("%d\n", m_vertex_data[i].data[j]));
				if (!((j + 1) % m_vertex_data[i].size)) dump.Write("\n");
			}
			break;

		case CELL_GCM_VERTEX_S32K:
			for (u32 j = 0; j < m_vertex_data[i].data.size(); j += 2)
			{
				dump.Write(wxString::Format("%d\n", *(u16*)&m_vertex_data[i].data[j]));
				if (!(((j + 2) / 2) % m_vertex_data[i].size)) dump.Write("\n");
			}
			break;

			// case CELL_GCM_VERTEX_CMP:

		case CELL_GCM_VERTEX_UB256:
			for (u32 j = 0; j < m_vertex_data[i].data.size(); ++j)
			{
				dump.Write(wxString::Format("%d\n", m_vertex_data[i].data[j]));
				if (!((j + 1) % m_vertex_data[i].size)) dump.Write("\n");
			}
			break;

		default:
			LOG_ERROR(HLE, "Bad cv type! %d", m_vertex_data[i].type);
			return;
		}

		dump.Write("\n");
#endif

		if (vertexData[i].type < 1 || vertexData[i].type > 7)
		{
			LOG_ERROR(RSX, "GLGSRender::EnableVertexData: Bad vertex data type (%d)!", vertexData[i].type);
		}

		D3D12_INPUT_ELEMENT_DESC IAElement = {};
		/*		if (!m_vertex_data[i].addr)
		{
		switch (m_vertex_data[i].type)
		{
		case CELL_GCM_VERTEX_S32K:
		case CELL_GCM_VERTEX_S1:
		switch (m_vertex_data[i].size)
		{
		case 1: glVertexAttrib1s(i, (GLshort&)m_vertex_data[i].data[0]); break;
		case 2: glVertexAttrib2sv(i, (GLshort*)&m_vertex_data[i].data[0]); break;
		case 3: glVertexAttrib3sv(i, (GLshort*)&m_vertex_data[i].data[0]); break;
		case 4: glVertexAttrib4sv(i, (GLshort*)&m_vertex_data[i].data[0]); break;
		}
		break;

		case CELL_GCM_VERTEX_F:
		switch (m_vertex_data[i].size)
		{
		case 1: glVertexAttrib1f(i, (GLfloat&)m_vertex_data[i].data[0]); break;
		case 2: glVertexAttrib2fv(i, (GLfloat*)&m_vertex_data[i].data[0]); break;
		case 3: glVertexAttrib3fv(i, (GLfloat*)&m_vertex_data[i].data[0]); break;
		case 4: glVertexAttrib4fv(i, (GLfloat*)&m_vertex_data[i].data[0]); break;
		}
		break;

		case CELL_GCM_VERTEX_CMP:
		case CELL_GCM_VERTEX_UB:
		glVertexAttrib4ubv(i, (GLubyte*)&m_vertex_data[i].data[0]);
		break;
		}

		checkForGlError("glVertexAttrib");
		}
		else*/
		{
			IAElement.SemanticName = "TEXCOORD";
			IAElement.SemanticIndex = i;
			IAElement.InputSlot = (UINT)inputSlot;
			IAElement.Format = getFormat(vertexData[i].type - 1, vertexData[i].size);
			inputSlot++;
		}
		result.push_back(IAElement);
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

static
D3D12_RESOURCE_DESC getBufferResourceDesc(size_t sizeInByte)
{
	D3D12_RESOURCE_DESC BufferDesc = {};
	BufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	BufferDesc.Width = (UINT)sizeInByte;
	BufferDesc.Height = 1;
	BufferDesc.DepthOrArraySize = 1;
	BufferDesc.SampleDesc.Count = 1;
	BufferDesc.MipLevels = 1;
	BufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	return BufferDesc;
}

// D3D12GS member handling buffers

std::pair<std::vector<D3D12_VERTEX_BUFFER_VIEW>, D3D12_INDEX_BUFFER_VIEW> D3D12GSRender::EnableVertexData(bool indexed_draw)
{
	std::pair<std::vector<D3D12_VERTEX_BUFFER_VIEW>, D3D12_INDEX_BUFFER_VIEW> result;
	m_IASet = getIALayout(m_device, indexed_draw, m_vertex_data);

	const u32 data_offset = indexed_draw ? 0 : m_draw_array_first;

	for (u32 i = 0; i < m_vertex_count; ++i)
	{
		if (!m_vertex_data[i].IsEnabled()) continue;
		const size_t item_size = m_vertex_data[i].GetTypeSize() * m_vertex_data[i].size;
		const size_t data_size = m_vertex_data[i].data.size() - data_offset * item_size;
		size_t subBufferSize = (data_offset + data_size) * item_size;
		// 65536 alignment
		size_t bufferHeapOffset = getCurrentResourceStorage().m_vertexIndexBuffersHeapFreeSpace;
		bufferHeapOffset = (bufferHeapOffset + 65536 - 1) & ~65535;

		ID3D12Resource *vertexBuffer;
		check(m_device->CreatePlacedResource(
			getCurrentResourceStorage().m_vertexIndexBuffersHeap,
			bufferHeapOffset,
			&getBufferResourceDesc(subBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&vertexBuffer)
			));
		void *bufferMap;
		check(vertexBuffer->Map(0, nullptr, (void**)&bufferMap));
		memcpy((char*)bufferMap + data_offset * item_size, &m_vertex_data[i].data[data_offset * item_size], data_size);
		vertexBuffer->Unmap(0, nullptr);
		getCurrentResourceStorage().m_inflightResources.push_back(vertexBuffer);

		D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
		vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
		vertexBufferView.SizeInBytes = (UINT)subBufferSize;
		vertexBufferView.StrideInBytes = (UINT)item_size;
		result.first.push_back(vertexBufferView);
		getCurrentResourceStorage().m_vertexIndexBuffersHeapFreeSpace = bufferHeapOffset + subBufferSize;
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
		switch (m_indexed_array.m_type)
		{
		default: // If it's not indexed draw, use 16 bits unsigned short
		case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16:
			indexBufferView.Format = DXGI_FORMAT_R16_UINT;
			indexSize = 2;
			break;
		case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_32:
			indexBufferView.Format = DXGI_FORMAT_R32_UINT;
			indexSize = 4;
			break;
		}

		if (indexed_draw && !m_forcedIndexBuffer)
			indexCount = m_indexed_array.m_data.size() / indexSize;
		else if (indexed_draw && m_forcedIndexBuffer)
			indexCount = 6 * m_indexed_array.m_data.size() / (4 * indexSize);
		else
			indexCount = m_draw_array_count * 6 / 4;
		size_t subBufferSize = indexCount * indexSize;
		// 65536 alignment
		size_t bufferHeapOffset = getCurrentResourceStorage().m_vertexIndexBuffersHeapFreeSpace;
		bufferHeapOffset = (bufferHeapOffset + 65536 - 1) & ~65535;

		ID3D12Resource *indexBuffer;
		check(m_device->CreatePlacedResource(
			getCurrentResourceStorage().m_vertexIndexBuffersHeap,
			D3D12_HEAP_FLAG_NONE,
			&getBufferResourceDesc(subBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&indexBuffer)
			));

		void *bufferMap;
		check(indexBuffer->Map(0, nullptr, (void**)&bufferMap));
		if (indexed_draw && !m_forcedIndexBuffer)
			memcpy(bufferMap, m_indexed_array.m_data.data(), subBufferSize);
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
		getCurrentResourceStorage().m_inflightResources.push_back(indexBuffer);
		getCurrentResourceStorage().m_vertexIndexBuffersHeapFreeSpace = bufferHeapOffset + subBufferSize;


		indexBufferView.SizeInBytes = (UINT)subBufferSize;
		indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();

		if (m_forcedIndexBuffer)
			indexBufferView.Format = DXGI_FORMAT_R16_UINT;

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
	scaleOffsetMat[7] = (float&)methodRegisters[NV4097_SET_VIEWPORT_OFFSET + (0x4 * 1)] - (RSXThread::m_height / RSXThread::m_height_scale);
	scaleOffsetMat[11] = (float&)methodRegisters[NV4097_SET_VIEWPORT_OFFSET + (0x4 * 2)];

	scaleOffsetMat[3] /= RSXThread::m_width / RSXThread::m_width_scale;
	scaleOffsetMat[7] /= RSXThread::m_height / RSXThread::m_height_scale;

	size_t constantBuffersHeapOffset = getCurrentResourceStorage().m_constantsBuffersHeapFreeSpace;
	// 65536 alignment
	constantBuffersHeapOffset = (constantBuffersHeapOffset + 65536 - 1) & ~65535;

	// Scale offset buffer
	// Separate constant buffer
	ID3D12Resource *scaleOffsetBuffer;
	check(m_device->CreatePlacedResource(
		getCurrentResourceStorage().m_constantsBuffersHeap,
		constantBuffersHeapOffset,
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
	D3D12_CPU_DESCRIPTOR_HANDLE Handle = getCurrentResourceStorage().m_scaleOffsetDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	Handle.ptr += getCurrentResourceStorage().m_currentScaleOffsetBufferIndex * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_device->CreateConstantBufferView(&constantBufferViewDesc, Handle);
	getCurrentResourceStorage().m_constantsBuffersHeapFreeSpace = constantBuffersHeapOffset + 256;
	getCurrentResourceStorage().m_inflightResources.push_back(scaleOffsetBuffer);
}

void D3D12GSRender::FillVertexShaderConstantsBuffer()
{
	for (const RSXTransformConstant& c : m_transform_constants)
	{
		size_t offset = c.id * 4 * sizeof(float);
		float vector[] = { c.x, c.y, c.z, c.w };
		memcpy((char*)vertexConstantShadowCopy + offset, vector, 4 * sizeof(float));
	}

	size_t constantBuffersHeapOffset = getCurrentResourceStorage().m_constantsBuffersHeapFreeSpace;
	// 65536 alignment
	constantBuffersHeapOffset = (constantBuffersHeapOffset + 65536 - 1) & ~65535;

	ID3D12Resource *constantsBuffer;
	check(m_device->CreatePlacedResource(
		getCurrentResourceStorage().m_constantsBuffersHeap,
		constantBuffersHeapOffset,
		&getBufferResourceDesc(512 * 4 * sizeof(float)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constantsBuffer)
		));

	void *constantsBufferMap;
	check(constantsBuffer->Map(0, nullptr, &constantsBufferMap));
	streamToBuffer(constantsBufferMap, vertexConstantShadowCopy, 512 * 4 * sizeof(float));
	constantsBuffer->Unmap(0, nullptr);

	D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferViewDesc = {};
	constantBufferViewDesc.BufferLocation = constantsBuffer->GetGPUVirtualAddress();
	constantBufferViewDesc.SizeInBytes = 512 * 4 * sizeof(float);
	D3D12_CPU_DESCRIPTOR_HANDLE Handle = getCurrentResourceStorage().m_constantsBufferDescriptorsHeap->GetCPUDescriptorHandleForHeapStart();
	Handle.ptr += getCurrentResourceStorage().m_constantsBufferIndex * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_device->CreateConstantBufferView(&constantBufferViewDesc, Handle);
	getCurrentResourceStorage().m_constantsBuffersHeapFreeSpace = constantBuffersHeapOffset + 512 * 4 * sizeof(float);
	getCurrentResourceStorage().m_inflightResources.push_back(constantsBuffer);
}

void D3D12GSRender::FillPixelShaderConstantsBuffer()
{
	// Get constant from fragment program
	const std::vector<size_t> &fragmentOffset = m_cachePSO.getFragmentConstantOffsetsCache(m_cur_fragment_prog);
	size_t bufferSize = fragmentOffset.size() * 4 * sizeof(float) + 1;
	// Multiple of 256 never 0
	bufferSize = (bufferSize + 255) & ~255;

	size_t constantBuffersHeapOffset = getCurrentResourceStorage().m_constantsBuffersHeapFreeSpace;
	// 65536 alignment
	constantBuffersHeapOffset = (constantBuffersHeapOffset + 65536 - 1) & ~65535;

	ID3D12Resource *constantsBuffer;
	check(m_device->CreatePlacedResource(
		getCurrentResourceStorage().m_constantsBuffersHeap,
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
		if (!m_fragment_constants.empty() && offsetInFP == m_fragment_constants.front().id - m_cur_fragment_prog->offset)
		{
			const RSXTransformConstant& c = m_fragment_constants.front();
			vector[0] = (u32&)c.x;
			vector[1] = (u32&)c.y;
			vector[2] = (u32&)c.z;
			vector[3] = (u32&)c.w;
		}
		else
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
	D3D12_CPU_DESCRIPTOR_HANDLE Handle = getCurrentResourceStorage().m_constantsBufferDescriptorsHeap->GetCPUDescriptorHandleForHeapStart();
	Handle.ptr += getCurrentResourceStorage().m_constantsBufferIndex * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_device->CreateConstantBufferView(&constantBufferViewDesc, Handle);
	getCurrentResourceStorage().m_constantsBuffersHeapFreeSpace = constantBuffersHeapOffset + bufferSize;
	getCurrentResourceStorage().m_inflightResources.push_back(constantsBuffer);
}


#endif