#include "stdafx.h"
#if defined(DX12_SUPPORT)
#include "D3D12GSRender.h"
#include <wrl/client.h>
#include <dxgi1_4.h>

GetGSFrameCb2 GetGSFrame = nullptr;

void SetGetD3DGSFrameCallback(GetGSFrameCb2 value)
{
	GetGSFrame = value;
}

static void check(HRESULT hr)
{
	if (hr != 0)
		abort();
}

D3D12GSRender::D3D12GSRender()
	: GSRender(), m_fbo(nullptr), m_PSO(nullptr)
{
	memset(m_vertexBufferSize, 0, sizeof(m_vertexBufferSize));
	m_constantsBufferSize = 0;
	m_constantsBufferIndex = 0;
	m_currentScaleOffsetBufferIndex = 0;
	constantsFragmentSize = 0;
	// Enable d3d debug layer
	Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;
	D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
	debugInterface->EnableDebugLayer();

	// Create adapter
	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
	check(CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory)));
	IDXGIAdapter* warpAdapter;
	check(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));
	check(D3D12CreateDevice(warpAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));

	// Queues
	D3D12_COMMAND_QUEUE_DESC copyQueueDesc = {}, graphicQueueDesc = {};
	copyQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	graphicQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	check(m_device->CreateCommandQueue(&copyQueueDesc, IID_PPV_ARGS(&m_commandQueueCopy)));
	check(m_device->CreateCommandQueue(&graphicQueueDesc, IID_PPV_ARGS(&m_commandQueueGraphic)));

	// Create a global command allocator
	m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator));

	m_frame = GetGSFrame();

	// Create swap chain and put them in a descriptor heap as rendertarget
	DXGI_SWAP_CHAIN_DESC swapChain = {};
	swapChain.BufferCount = 2;
	swapChain.Windowed = true;
	swapChain.OutputWindow = m_frame->getHandle();
	swapChain.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChain.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChain.SampleDesc.Count = 1;
	swapChain.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swapChain.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

	check(dxgiFactory->CreateSwapChain(m_commandQueueGraphic, &swapChain, (IDXGISwapChain**)&m_swapChain));
	m_swapChain->GetBuffer(0, IID_PPV_ARGS(&m_backBuffer[0]));
	m_swapChain->GetBuffer(1, IID_PPV_ARGS(&m_backBuffer[1]));

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 1;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	D3D12_RENDER_TARGET_VIEW_DESC rttDesc = {};
	rttDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rttDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_backbufferAsRendertarget[0]));
	m_device->CreateRenderTargetView(m_backBuffer[0], &rttDesc, m_backbufferAsRendertarget[0]->GetCPUDescriptorHandleForHeapStart());
	m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_backbufferAsRendertarget[1]));
	m_device->CreateRenderTargetView(m_backBuffer[1], &rttDesc, m_backbufferAsRendertarget[1]->GetCPUDescriptorHandleForHeapStart());

	// Create global vertex buffers (1 MB, hopefully big enough...)
	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Width = (UINT)1024 * 1024;
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.MipLevels = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	for (unsigned i = 0; i < m_vertex_count; i++)
	{
		check(m_device->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vertexBuffer[i])
			));
	}

	check(m_device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_constantsVertexBuffer)
		));

	check(m_device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_constantsFragmentBuffer)
		));

	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descriptorHeapDesc.NumDescriptors = 1000; // For safety
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	check(m_device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_constantsBufferDescriptorsHeap)));

	// Scale offset buffer
	// Separate constant buffer
	check(m_device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_scaleOffsetBuffer)
		));
	descriptorHeapDesc = {};
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descriptorHeapDesc.NumDescriptors = 1000; // For safety
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	check(m_device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_scaleOffsetDescriptorHeap)));


	// Common root signature
	D3D12_DESCRIPTOR_RANGE descriptorRange[2] = {};
	// Scale Offset data
	descriptorRange[0].BaseShaderRegister = 0;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	// Constants
	descriptorRange[1].BaseShaderRegister = 1;
	descriptorRange[1].NumDescriptors = 2;
	descriptorRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	D3D12_ROOT_PARAMETER RP[2] = {};
	RP[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	RP[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	RP[0].DescriptorTable.pDescriptorRanges = &descriptorRange[0];
	RP[0].DescriptorTable.NumDescriptorRanges = 1;
	RP[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	RP[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	RP[1].DescriptorTable.pDescriptorRanges = &descriptorRange[1];
	RP[1].DescriptorTable.NumDescriptorRanges = 1;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.NumParameters = 2;
	rootSignatureDesc.pParameters = RP;

	Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
	check(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, &errorBlob));

	m_device->CreateRootSignature(0,
		rootSignatureBlob->GetBufferPointer(),
		rootSignatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&m_rootSignature));
}

D3D12GSRender::~D3D12GSRender()
{
	// NOTE: Should be released only if no command are in flight !
	m_commandAllocator->Release();
	m_commandQueueGraphic->Release();
	m_commandQueueCopy->Release();
	m_backbufferAsRendertarget[0]->Release();
	m_backbufferAsRendertarget[1]->Release();
	m_constantsBufferDescriptorsHeap->Release();
	m_scaleOffsetDescriptorHeap->Release();
	m_constantsVertexBuffer->Release();
	m_constantsFragmentBuffer->Release();
	m_scaleOffsetBuffer->Release();
	for (unsigned i = 0; i < 32; i++)
		m_vertexBuffer[i]->Release();
	if (m_fbo)
		delete m_fbo;
	m_rootSignature->Release();
	m_backBuffer[0]->Release();
	m_backBuffer[1]->Release();
	m_swapChain->Release();
	m_device->Release();
}

void D3D12GSRender::Close()
{
	Stop();
	m_frame->Hide();
}

void D3D12GSRender::InitDrawBuffers()
{
	if (m_fbo == nullptr || RSXThread::m_width != m_lastWidth || RSXThread::m_height != m_lastHeight || m_lastDepth != m_surface_depth_format)
	{
		
		LOG_WARNING(RSX, "New FBO (%dx%d)", RSXThread::m_width, RSXThread::m_height);
		m_lastWidth = RSXThread::m_width;
		m_lastHeight = RSXThread::m_height;
		m_lastDepth = m_surface_depth_format;

		m_fbo = new D3D12RenderTargetSets(m_device, (u8)m_lastDepth, m_lastWidth, m_lastHeight);
	}
}

void D3D12GSRender::OnInit()
{
	m_frame->Show();
}

void D3D12GSRender::OnInitThread()
{
}

void D3D12GSRender::OnExitThread()
{
}

void D3D12GSRender::OnReset()
{
}

void D3D12GSRender::ExecCMD(u32 cmd)
{
	assert(cmd == NV4097_CLEAR_SURFACE);

	InitDrawBuffers();

	ID3D12GraphicsCommandList *commandList;
	check(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator, nullptr, IID_PPV_ARGS(&commandList)));
	m_inflightCommandList.push_back(commandList);

/*	if (m_set_color_mask)
	{
		glColorMask(m_color_mask_r, m_color_mask_g, m_color_mask_b, m_color_mask_a);
		checkForGlError("glColorMask");
	}

	if (m_set_scissor_horizontal && m_set_scissor_vertical)
	{
		glScissor(m_scissor_x, m_scissor_y, m_scissor_w, m_scissor_h);
		checkForGlError("glScissor");
	}*/

	// TODO: Merge depth and stencil clear when possible
	if (m_clear_surface_mask & 0x1)
		commandList->ClearDepthStencilView(m_fbo->getDSVCPUHandle(), D3D12_CLEAR_FLAG_DEPTH, m_clear_surface_z / (float)0xffffff, 0, 0, nullptr);

	if (m_clear_surface_mask & 0x2)
		commandList->ClearDepthStencilView(m_fbo->getDSVCPUHandle(), D3D12_CLEAR_FLAG_STENCIL, 0.f, m_clear_surface_s, 0, nullptr);

	if (m_clear_surface_mask & 0xF0)
	{
		float clearColor[] =
		{
			m_clear_surface_color_r / 255.0f,
			m_clear_surface_color_g / 255.0f,
			m_clear_surface_color_b / 255.0f,
			m_clear_surface_color_a / 255.0f
		};
		switch (m_surface_color_target)
		{
			case CELL_GCM_SURFACE_TARGET_NONE: break;

			case CELL_GCM_SURFACE_TARGET_0:
				commandList->ClearRenderTargetView(m_fbo->getRTTCPUHandle(0), clearColor, 0, nullptr);
				break;
			case CELL_GCM_SURFACE_TARGET_1:
				commandList->ClearRenderTargetView(m_fbo->getRTTCPUHandle(1), clearColor, 0, nullptr);
				break;
			case CELL_GCM_SURFACE_TARGET_MRT1:
				commandList->ClearRenderTargetView(m_fbo->getRTTCPUHandle(0), clearColor, 0, nullptr);
				commandList->ClearRenderTargetView(m_fbo->getRTTCPUHandle(1), clearColor, 0, nullptr);
				break;
			case CELL_GCM_SURFACE_TARGET_MRT2:
				commandList->ClearRenderTargetView(m_fbo->getRTTCPUHandle(0), clearColor, 0, nullptr);
				commandList->ClearRenderTargetView(m_fbo->getRTTCPUHandle(1), clearColor, 0, nullptr);
				commandList->ClearRenderTargetView(m_fbo->getRTTCPUHandle(2), clearColor, 0, nullptr);
				break;
			case CELL_GCM_SURFACE_TARGET_MRT3:
				commandList->ClearRenderTargetView(m_fbo->getRTTCPUHandle(0), clearColor, 0, nullptr);
				commandList->ClearRenderTargetView(m_fbo->getRTTCPUHandle(1), clearColor, 0, nullptr);
				commandList->ClearRenderTargetView(m_fbo->getRTTCPUHandle(2), clearColor, 0, nullptr);
				commandList->ClearRenderTargetView(m_fbo->getRTTCPUHandle(3), clearColor, 0, nullptr);
				break;
			default:
				LOG_ERROR(RSX, "Bad surface color target: %d", m_surface_color_target);
		}
	}

	check(commandList->Close());
	m_commandQueueGraphic->ExecuteCommandLists(1, (ID3D12CommandList**) &commandList);
}

void D3D12GSRender::EnableVertexData(bool indexed_draw)
{
	m_IASet = getIALayout(m_device, indexed_draw, m_vertex_data);

	const u32 data_offset = indexed_draw ? 0 : m_draw_array_first;

	for (u32 i = 0; i < m_vertex_count; ++i)
	{
		if (!m_vertex_data[i].IsEnabled()) continue;
		const size_t item_size = m_vertex_data[i].GetTypeSize() * m_vertex_data[i].size;
		const size_t data_size = m_vertex_data[i].data.size() - data_offset * item_size;

		// TODO: Use default heap and upload data
		void *bufferMap;
		check(m_vertexBuffer[i]->Map(0, nullptr, (void**)&bufferMap));
		memcpy((char*)bufferMap + data_offset * item_size, &m_vertex_data[i].data[data_offset * item_size], data_size);
		m_vertexBuffer[i]->Unmap(0, nullptr);
		size_t newOffset = (data_offset + data_size) * item_size;
		m_vertexBufferSize[i] = newOffset > m_vertexBufferSize[i] ? newOffset : m_vertexBufferSize[i];
	}

	if (indexed_draw)
	{
/*		D3D12_RESOURCE_DESC resDesc = {};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resDesc.Width = (UINT)m_indexed_array.m_data.size();
		resDesc.Height = 1;
		resDesc.DepthOrArraySize = 1;
		resDesc.SampleDesc.Count = 1;
		resDesc.MipLevels = 1;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		check(m_device->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_indexBuffer)
			));

		check(m_indexBuffer->Map(0, nullptr, (void**)&bufferMap));
		memcpy(bufferMap, m_indexed_array.m_data.data(), m_indexed_array.m_data.size());
		m_indexBuffer->Unmap(0, nullptr);

		D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
		indexBufferView.SizeInBytes = (UINT)m_indexed_array.m_data.size();
		indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();*/
	}
}

void D3D12GSRender::setScaleOffset()
{
	float scaleOffsetMat[16] =
	{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	// Scale
	scaleOffsetMat[0] = (float&)methodRegisters[NV4097_SET_VIEWPORT_SCALE + (0x4 * 0)] / (RSXThread::m_width / RSXThread::m_width_scale);
	scaleOffsetMat[5] = (float&)methodRegisters[NV4097_SET_VIEWPORT_SCALE + (0x4 * 1)] / (RSXThread::m_height / RSXThread::m_height_scale);
	scaleOffsetMat[10] = (float&)methodRegisters[NV4097_SET_VIEWPORT_SCALE + (0x4 * 2)];

	// Offset
	scaleOffsetMat[3] = (float&)methodRegisters[NV4097_SET_VIEWPORT_OFFSET + (0x4 * 0)] - (RSXThread::m_width / RSXThread::m_width_scale);
	scaleOffsetMat[7] = (float&)methodRegisters[NV4097_SET_VIEWPORT_OFFSET + (0x4 * 1)] - (RSXThread::m_height / RSXThread::m_height_scale);
	scaleOffsetMat[11] = (float&)methodRegisters[NV4097_SET_VIEWPORT_OFFSET + (0x4 * 2)] - 1 / 2.0f;

	scaleOffsetMat[3] /= RSXThread::m_width / RSXThread::m_width_scale;
	scaleOffsetMat[7] /= RSXThread::m_height / RSXThread::m_height_scale;

	void *scaleOffsetMap;
	size_t offset = m_currentScaleOffsetBufferIndex * 256;
	D3D12_RANGE range = {
		offset,
		1024 * 1024 - offset
	};
	check(m_scaleOffsetBuffer->Map(0, &range, &scaleOffsetMap));
	memcpy((char*)scaleOffsetMap + offset, scaleOffsetMat, 16 * sizeof(float));
	m_scaleOffsetBuffer->Unmap(0, &range);

	D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferViewDesc = {};
	constantBufferViewDesc.BufferLocation = m_scaleOffsetBuffer->GetGPUVirtualAddress() + offset;
	constantBufferViewDesc.SizeInBytes = (UINT)256;
	D3D12_CPU_DESCRIPTOR_HANDLE Handle = m_scaleOffsetDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	Handle.ptr += m_currentScaleOffsetBufferIndex * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_device->CreateConstantBufferView(&constantBufferViewDesc, Handle);
}

void D3D12GSRender::FillVertexShaderConstantsBuffer()
{
	void *constantsBufferMap;
	check(m_constantsVertexBuffer->Map(0, nullptr, &constantsBufferMap));

	for (const RSXTransformConstant& c : m_transform_constants)
	{
		size_t offset = c.id * 4 * sizeof(float);
		float vector[] = { c.x, c.y, c.z, c.w };
		memcpy((char*)constantsBufferMap + offset, vector, 4 * sizeof(float));
		size_t bufferSizeCandidate = offset + 4 * sizeof(float);
		m_constantsBufferSize = bufferSizeCandidate > m_constantsBufferSize ? bufferSizeCandidate : m_constantsBufferSize;
	}
	m_constantsVertexBuffer->Unmap(0, nullptr);
	// make it multiple of 256 bytes
	m_constantsBufferSize = (m_constantsBufferSize + 255) & ~255;

	D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferViewDesc = {};
	constantBufferViewDesc.BufferLocation = m_constantsVertexBuffer->GetGPUVirtualAddress();
	constantBufferViewDesc.SizeInBytes = (UINT)m_constantsBufferSize;
	D3D12_CPU_DESCRIPTOR_HANDLE Handle = m_constantsBufferDescriptorsHeap->GetCPUDescriptorHandleForHeapStart();
	Handle.ptr += m_constantsBufferIndex * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_device->CreateConstantBufferView(&constantBufferViewDesc, Handle);
}

void D3D12GSRender::FillPixelShaderConstantsBuffer()
{
	size_t offset = 0;
	void *constantsBufferMap;
	check(m_constantsFragmentBuffer->Map(0, nullptr, &constantsBufferMap));
	for (const RSXTransformConstant& c : m_fragment_constants)
	{
		u32 id = c.id - m_cur_fragment_prog->offset;
		float vector[] = { c.x, c.y, c.z, c.w };
		memcpy((char*)constantsBufferMap + constantsFragmentSize + offset, vector, 4 * sizeof(float));
		offset += 4 * sizeof(float);
	}
	m_constantsFragmentBuffer->Unmap(0, nullptr);
	// Multiple of 256
	offset = (offset + 255) & ~255;

	D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferViewDesc = {};
	constantBufferViewDesc.BufferLocation = m_constantsFragmentBuffer->GetGPUVirtualAddress() + constantsFragmentSize;
	constantBufferViewDesc.SizeInBytes = (UINT)offset;
	D3D12_CPU_DESCRIPTOR_HANDLE Handle = m_constantsBufferDescriptorsHeap->GetCPUDescriptorHandleForHeapStart();
	Handle.ptr += m_constantsBufferIndex * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_device->CreateConstantBufferView(&constantBufferViewDesc, Handle);
	constantsFragmentSize += offset;
}


bool D3D12GSRender::LoadProgram()
{
	if (!m_cur_fragment_prog)
	{
		LOG_WARNING(RSX, "LoadProgram: m_cur_shader_prog == NULL");
		return false;
	}

	m_cur_fragment_prog->ctrl = m_shader_ctrl;

	if (!m_cur_vertex_prog)
	{
		LOG_WARNING(RSX, "LoadProgram: m_cur_vertex_prog == NULL");
		return false;
	}

	PipelineProperties prop = {};
	/*
	#define GL_POINTS                         0x0000
	#define GL_LINES                          0x0001
	#define GL_LINE_LOOP                      0x0002
	#define GL_LINE_STRIP                     0x0003
	#define GL_TRIANGLES                      0x0004
	#define GL_TRIANGLE_STRIP                 0x0005
	#define GL_TRIANGLE_FAN                   0x0006
	#define GL_QUADS                          0x0007
	#define GL_QUAD_STRIP                     0x0008
	#define GL_POLYGON                        0x0009
	*/
	switch (m_draw_mode - 1)
	{
	case 0:
		prop.Topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		break;
	case 1:
	case 2:
	case 3:
		prop.Topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		break;
	case 4:
	case 5:
	case 6:
		prop.Topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		break;
	default:
//		LOG_ERROR(RSX, "Unsupported primitive type");
		prop.Topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		break;
	}

	m_PSO = m_cachePSO.getGraphicPipelineState(m_device, m_rootSignature, m_cur_vertex_prog, m_cur_fragment_prog, prop, m_IASet);
	return m_PSO != nullptr;
}

void D3D12GSRender::ExecCMD()
{
	ID3D12GraphicsCommandList *commandList;
	m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator, nullptr, IID_PPV_ARGS(&commandList));
	m_inflightCommandList.push_back(commandList);

	commandList->SetGraphicsRootSignature(m_rootSignature);

	if (m_indexed_array.m_count)
	{
		//		LoadVertexData(m_indexed_array.index_min, m_indexed_array.index_max - m_indexed_array.index_min + 1);
	}

	if (m_indexed_array.m_count || m_draw_array_count)
	{
		EnableVertexData(m_indexed_array.m_count ? true : false);
		std::vector<D3D12_VERTEX_BUFFER_VIEW> vertexBufferViews;
		for (u32 i = 0; i < m_vertex_count; ++i)
		{
			if (!m_vertex_data[i].IsEnabled()) continue;
			const size_t item_size = m_vertex_data[i].GetTypeSize() * m_vertex_data[i].size;
			D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};

			vertexBufferView.BufferLocation = m_vertexBuffer[i]->GetGPUVirtualAddress();
			vertexBufferView.SizeInBytes = (UINT)m_vertexBufferSize[i];
			vertexBufferView.StrideInBytes = (UINT)item_size;
			vertexBufferViews.push_back(vertexBufferView);

			assert((m_draw_array_first + m_draw_array_count) * item_size <= m_vertexBufferSize[i]);
		}
		commandList->IASetVertexBuffers(0, (UINT)vertexBufferViews.size(), vertexBufferViews.data());

		setScaleOffset();
		commandList->SetDescriptorHeaps(1, &m_scaleOffsetDescriptorHeap);
		D3D12_GPU_DESCRIPTOR_HANDLE Handle = m_scaleOffsetDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		Handle.ptr += m_currentScaleOffsetBufferIndex * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		commandList->SetGraphicsRootDescriptorTable(0, Handle);
		m_currentScaleOffsetBufferIndex++;

		size_t currentBufferIndex = m_constantsBufferIndex;
		FillVertexShaderConstantsBuffer();
		m_constantsBufferIndex++;
		FillPixelShaderConstantsBuffer();
		m_constantsBufferIndex++;

		commandList->SetDescriptorHeaps(1, &m_constantsBufferDescriptorsHeap);
		Handle = m_constantsBufferDescriptorsHeap->GetGPUDescriptorHandleForHeapStart();
		Handle.ptr += currentBufferIndex * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		commandList->SetGraphicsRootDescriptorTable(1, Handle);
	}

	if (!LoadProgram())
	{
		LOG_ERROR(RSX, "LoadProgram failed.");
		Emu.Pause();
		return;
	}
	commandList->SetPipelineState(m_PSO);

	InitDrawBuffers();
	switch (m_surface_color_target)
	{
	case CELL_GCM_SURFACE_TARGET_NONE: break;
	case CELL_GCM_SURFACE_TARGET_0:
		commandList->OMSetRenderTargets(1, &m_fbo->getRTTCPUHandle(0), true, nullptr);
		break;
	case CELL_GCM_SURFACE_TARGET_1:
		commandList->OMSetRenderTargets(1, &m_fbo->getRTTCPUHandle(1), true, nullptr);
		break;
	case CELL_GCM_SURFACE_TARGET_MRT1:
		commandList->OMSetRenderTargets(2, &m_fbo->getRTTCPUHandle(0), true, nullptr);
		break;
	case CELL_GCM_SURFACE_TARGET_MRT2:
		commandList->OMSetRenderTargets(3, &m_fbo->getRTTCPUHandle(0), true, nullptr);
		break;
	case CELL_GCM_SURFACE_TARGET_MRT3:
		commandList->OMSetRenderTargets(4, &m_fbo->getRTTCPUHandle(0), true, nullptr);
		break;
	default:
		LOG_ERROR(RSX, "Bad surface color target: %d", m_surface_color_target);
	}
	D3D12_VIEWPORT viewport =
	{
		0.f,
		0.f,
		(float)RSXThread::m_width,
		(float)RSXThread::m_height,
		-1.f,
		1.f
	};
	commandList->RSSetViewports(1, &viewport);
	D3D12_RECT box =
	{
		0, 0,
		(LONG)RSXThread::m_width, (LONG)RSXThread::m_height,
	};
	commandList->RSSetScissorRects(1, &box);

	/*
	#define GL_POINTS                         0x0000
	#define GL_LINES                          0x0001
	#define GL_LINE_LOOP                      0x0002
	#define GL_LINE_STRIP                     0x0003
	#define GL_TRIANGLES                      0x0004
	#define GL_TRIANGLE_STRIP                 0x0005
	#define GL_TRIANGLE_FAN                   0x0006
	#define GL_QUADS                          0x0007
	#define GL_QUAD_STRIP                     0x0008
	#define GL_POLYGON                        0x0009
	*/
	switch (m_draw_mode - 1)
	{
	case 0:
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
		break;
	case 1:
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		break;
	case 2:
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ);
		break;
	case 3:
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);
		break;
	case 4:
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		break;
	case 5:
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		break;
	case 6:
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ);
		break;
	default:
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ);
//		LOG_ERROR(RSX, "Unsupported primitive type");
		break;
	}

	if (m_indexed_array.m_count)
	{
/*		switch (m_indexed_array.m_type)
		{
		case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_32:
			commandList->DrawIndexedInstanced
			glDrawElements(m_draw_mode - 1, m_indexed_array.m_count, GL_UNSIGNED_INT, nullptr);
			checkForGlError("glDrawElements #4");
			break;

		case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16:
			glDrawElements(m_draw_mode - 1, m_indexed_array.m_count, GL_UNSIGNED_SHORT, nullptr);
			checkForGlError("glDrawElements #2");
			break;

		default:
			LOG_ERROR(RSX, "Bad indexed array type (%d)", m_indexed_array.m_type);
			break;
		}

		DisableVertexData();
		m_indexed_array.Reset();*/
	}

	if (m_draw_array_count)
	{
		//LOG_WARNING(RSX,"glDrawArrays(%d,%d,%d)", m_draw_mode - 1, m_draw_array_first, m_draw_array_count);
		commandList->DrawInstanced(m_draw_array_count, 1, m_draw_array_first, 0);
	}
	check(commandList->Close());
	m_commandQueueGraphic->ExecuteCommandLists(1, (ID3D12CommandList**)&commandList);

/*	if (m_set_color_mask)
	{
		glColorMask(m_color_mask_r, m_color_mask_g, m_color_mask_b, m_color_mask_a);
		checkForGlError("glColorMask");
	}

	if (!m_indexed_array.m_count && !m_draw_array_count)
	{
		u32 min_vertex_size = ~0;
		for (auto &i : m_vertex_data)
		{
			if (!i.size)
				continue;

			u32 vertex_size = i.data.size() / (i.size * i.GetTypeSize());

			if (min_vertex_size > vertex_size)
				min_vertex_size = vertex_size;
		}

		m_draw_array_count = min_vertex_size;
		m_draw_array_first = 0;
	}

	Enable(m_set_depth_test, GL_DEPTH_TEST);
	Enable(m_set_alpha_test, GL_ALPHA_TEST);
	Enable(m_set_blend || m_set_blend_mrt1 || m_set_blend_mrt2 || m_set_blend_mrt3, GL_BLEND);
	Enable(m_set_scissor_horizontal && m_set_scissor_vertical, GL_SCISSOR_TEST);
	Enable(m_set_logic_op, GL_LOGIC_OP);
	Enable(m_set_cull_face, GL_CULL_FACE);
	Enable(m_set_dither, GL_DITHER);
	Enable(m_set_stencil_test, GL_STENCIL_TEST);
	Enable(m_set_line_smooth, GL_LINE_SMOOTH);
	Enable(m_set_poly_smooth, GL_POLYGON_SMOOTH);
	Enable(m_set_point_sprite_control, GL_POINT_SPRITE);
	Enable(m_set_specular, GL_LIGHTING);
	Enable(m_set_poly_offset_fill, GL_POLYGON_OFFSET_FILL);
	Enable(m_set_poly_offset_line, GL_POLYGON_OFFSET_LINE);
	Enable(m_set_poly_offset_point, GL_POLYGON_OFFSET_POINT);
	Enable(m_set_restart_index, GL_PRIMITIVE_RESTART);
	Enable(m_set_line_stipple, GL_LINE_STIPPLE);
	Enable(m_set_polygon_stipple, GL_POLYGON_STIPPLE);

	if (m_set_clip_plane)
	{
		Enable(m_clip_plane_0, GL_CLIP_PLANE0);
		Enable(m_clip_plane_1, GL_CLIP_PLANE1);
		Enable(m_clip_plane_2, GL_CLIP_PLANE2);
		Enable(m_clip_plane_3, GL_CLIP_PLANE3);
		Enable(m_clip_plane_4, GL_CLIP_PLANE4);
		Enable(m_clip_plane_5, GL_CLIP_PLANE5);

		checkForGlError("m_set_clip_plane");
	}

	checkForGlError("glEnable");

	if (m_set_front_polygon_mode)
	{
		glPolygonMode(GL_FRONT, m_front_polygon_mode);
		checkForGlError("glPolygonMode(Front)");
	}

	if (m_set_back_polygon_mode)
	{
		glPolygonMode(GL_BACK, m_back_polygon_mode);
		checkForGlError("glPolygonMode(Back)");
	}

	if (m_set_point_size)
	{
		glPointSize(m_point_size);
		checkForGlError("glPointSize");
	}

	if (m_set_poly_offset_mode)
	{
		glPolygonOffset(m_poly_offset_scale_factor, m_poly_offset_bias);
		checkForGlError("glPolygonOffset");
	}

	if (m_set_logic_op)
	{
		glLogicOp(m_logic_op);
		checkForGlError("glLogicOp");
	}

	if (m_set_scissor_horizontal && m_set_scissor_vertical)
	{
		glScissor(m_scissor_x, m_scissor_y, m_scissor_w, m_scissor_h);
		checkForGlError("glScissor");
	}

	if (m_set_two_sided_stencil_test_enable)
	{
		if (m_set_stencil_fail && m_set_stencil_zfail && m_set_stencil_zpass)
		{
			glStencilOpSeparate(GL_FRONT, m_stencil_fail, m_stencil_zfail, m_stencil_zpass);
			checkForGlError("glStencilOpSeparate");
		}

		if (m_set_stencil_mask)
		{
			glStencilMaskSeparate(GL_FRONT, m_stencil_mask);
			checkForGlError("glStencilMaskSeparate");
		}

		if (m_set_stencil_func && m_set_stencil_func_ref && m_set_stencil_func_mask)
		{
			glStencilFuncSeparate(GL_FRONT, m_stencil_func, m_stencil_func_ref, m_stencil_func_mask);
			checkForGlError("glStencilFuncSeparate");
		}

		if (m_set_back_stencil_fail && m_set_back_stencil_zfail && m_set_back_stencil_zpass)
		{
			glStencilOpSeparate(GL_BACK, m_back_stencil_fail, m_back_stencil_zfail, m_back_stencil_zpass);
			checkForGlError("glStencilOpSeparate(GL_BACK)");
		}

		if (m_set_back_stencil_mask)
		{
			glStencilMaskSeparate(GL_BACK, m_back_stencil_mask);
			checkForGlError("glStencilMaskSeparate(GL_BACK)");
		}

		if (m_set_back_stencil_func && m_set_back_stencil_func_ref && m_set_back_stencil_func_mask)
		{
			glStencilFuncSeparate(GL_BACK, m_back_stencil_func, m_back_stencil_func_ref, m_back_stencil_func_mask);
			checkForGlError("glStencilFuncSeparate(GL_BACK)");
		}
	}
	else
	{
		if (m_set_stencil_fail && m_set_stencil_zfail && m_set_stencil_zpass)
		{
			glStencilOp(m_stencil_fail, m_stencil_zfail, m_stencil_zpass);
			checkForGlError("glStencilOp");
		}

		if (m_set_stencil_mask)
		{
			glStencilMask(m_stencil_mask);
			checkForGlError("glStencilMask");
		}

		if (m_set_stencil_func && m_set_stencil_func_ref && m_set_stencil_func_mask)
		{
			glStencilFunc(m_stencil_func, m_stencil_func_ref, m_stencil_func_mask);
			checkForGlError("glStencilFunc");
		}
	}

	// TODO: Use other glLightModel functions?
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, m_set_two_side_light_enable ? GL_TRUE : GL_FALSE);
	checkForGlError("glLightModeli");

	if (m_set_shade_mode)
	{
		glShadeModel(m_shade_mode);
		checkForGlError("glShadeModel");
	}

	if (m_set_depth_mask)
	{
		glDepthMask(m_depth_mask);
		checkForGlError("glDepthMask");
	}

	if (m_set_depth_func)
	{
		glDepthFunc(m_depth_func);
		checkForGlError("glDepthFunc");
	}

	if (m_set_depth_bounds && !is_intel_vendor)
	{
		glDepthBoundsEXT(m_depth_bounds_min, m_depth_bounds_max);
		checkForGlError("glDepthBounds");
	}

	if (m_set_clip)
	{
		glDepthRangef(m_clip_min, m_clip_max);
		checkForGlError("glDepthRangef");
	}

	if (m_set_line_width)
	{
		glLineWidth(m_line_width);
		checkForGlError("glLineWidth");
	}

	if (m_set_line_stipple)
	{
		glLineStipple(m_line_stipple_factor, m_line_stipple_pattern);
		checkForGlError("glLineStipple");
	}

	if (m_set_polygon_stipple)
	{
		glPolygonStipple((const GLubyte*)m_polygon_stipple_pattern);
		checkForGlError("glPolygonStipple");
	}

	if (m_set_blend_equation)
	{
		glBlendEquationSeparate(m_blend_equation_rgb, m_blend_equation_alpha);
		checkForGlError("glBlendEquationSeparate");
	}

	if (m_set_blend_sfactor && m_set_blend_dfactor)
	{
		glBlendFuncSeparate(m_blend_sfactor_rgb, m_blend_dfactor_rgb, m_blend_sfactor_alpha, m_blend_dfactor_alpha);
		checkForGlError("glBlendFuncSeparate");
	}

	if (m_set_blend_color)
	{
		glBlendColor(m_blend_color_r, m_blend_color_g, m_blend_color_b, m_blend_color_a);
		checkForGlError("glBlendColor");
	}

	if (m_set_cull_face)
	{
		glCullFace(m_cull_face);
		checkForGlError("glCullFace");
	}

	if (m_set_front_face)
	{
		glFrontFace(m_front_face);
		checkForGlError("glFrontFace");
	}

	if (m_set_alpha_func && m_set_alpha_ref)
	{
		glAlphaFunc(m_alpha_func, m_alpha_ref);
		checkForGlError("glAlphaFunc");
	}

	if (m_set_fog_mode)
	{
		glFogi(GL_FOG_MODE, m_fog_mode);
		checkForGlError("glFogi(GL_FOG_MODE)");
	}

	if (m_set_fog_params)
	{
		glFogf(GL_FOG_START, m_fog_param0);
		checkForGlError("glFogf(GL_FOG_START)");
		glFogf(GL_FOG_END, m_fog_param1);
		checkForGlError("glFogf(GL_FOG_END)");
	}

	if (m_set_restart_index)
	{
		glPrimitiveRestartIndex(m_restart_index);
		checkForGlError("glPrimitiveRestartIndex");
	}

	if (m_indexed_array.m_count && m_draw_array_count)
	{
		LOG_WARNING(RSX, "m_indexed_array.m_count && draw_array_count");
	}

	for (u32 i = 0; i < m_textures_count; ++i)
	{
		if (!m_textures[i].IsEnabled()) continue;

		glActiveTexture(GL_TEXTURE0 + i);
		checkForGlError("glActiveTexture");
		m_gl_textures[i].Create();
		m_gl_textures[i].Bind();
		checkForGlError(fmt::Format("m_gl_textures[%d].Bind", i));
		m_program.SetTex(i);
		m_gl_textures[i].Init(m_textures[i]);
		checkForGlError(fmt::Format("m_gl_textures[%d].Init", i));
	}

	for (u32 i = 0; i < m_textures_count; ++i)
	{
		if (!m_vertex_textures[i].IsEnabled()) continue;

		glActiveTexture(GL_TEXTURE0 + m_textures_count + i);
		checkForGlError("glActiveTexture");
		m_gl_vertex_textures[i].Create();
		m_gl_vertex_textures[i].Bind();
		checkForGlError(fmt::Format("m_gl_vertex_textures[%d].Bind", i));
		m_program.SetVTex(i);
		m_gl_vertex_textures[i].Init(m_vertex_textures[i]);
		checkForGlError(fmt::Format("m_gl_vertex_textures[%d].Init", i));
	}*/

//	WriteBuffers();
}

void D3D12GSRender::Flip()
{
	ID3D12GraphicsCommandList *commandList;
	m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator, nullptr, IID_PPV_ARGS(&commandList));
	m_inflightCommandList.push_back(commandList);

	switch (m_surface_color_target)
	{
	case CELL_GCM_SURFACE_TARGET_0:
	case CELL_GCM_SURFACE_TARGET_1:
	case CELL_GCM_SURFACE_TARGET_MRT1:
	case CELL_GCM_SURFACE_TARGET_MRT2:
	case CELL_GCM_SURFACE_TARGET_MRT3:
	{
		D3D12_RESOURCE_BARRIER barriers[2] = {};
		barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barriers[0].Transition.pResource = m_backBuffer[m_swapChain->GetCurrentBackBufferIndex()];
		barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;

		barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barriers[1].Transition.pResource = m_fbo->getRenderTargetTexture(0);
		barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;

		commandList->ResourceBarrier(2, barriers);
		D3D12_TEXTURE_COPY_LOCATION src = {}, dst = {};
		src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		src.SubresourceIndex = 0, dst.SubresourceIndex = 0;
		src.pResource = m_fbo->getRenderTargetTexture(0), dst.pResource = m_backBuffer[m_swapChain->GetCurrentBackBufferIndex()];
		D3D12_BOX box = { 0, 0, 0, RSXThread::m_width, RSXThread::m_height, 1 };
		commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, &box);

		barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
		barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		commandList->ResourceBarrier(2, barriers);
		commandList->Close();
		m_commandQueueGraphic->ExecuteCommandLists(1, (ID3D12CommandList**)&commandList);
	}
	}

	check(m_swapChain->Present(1, 0));
	// Wait execution is over
	// TODO: It's suboptimal, we should use 2 command allocator
	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
	m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	HANDLE gfxqueuecompletion = CreateEvent(0, 0, 0, 0);
	fence->SetEventOnCompletion(1, gfxqueuecompletion);
	m_commandQueueGraphic->Signal(fence.Get(), 1);
	WaitForSingleObject(gfxqueuecompletion, INFINITE);
	CloseHandle(gfxqueuecompletion);
	m_commandAllocator->Reset();
	for (ID3D12GraphicsCommandList *gfxCommandList : m_inflightCommandList)
		gfxCommandList->Release();
	m_inflightCommandList.clear();
	memset(m_vertexBufferSize, 0, sizeof(m_vertexBufferSize));
	m_constantsBufferSize = 0;
	m_constantsBufferIndex = 0;
	m_currentScaleOffsetBufferIndex = 0;
	constantsFragmentSize = 0;
	m_frame->Flip(nullptr);
}
#endif