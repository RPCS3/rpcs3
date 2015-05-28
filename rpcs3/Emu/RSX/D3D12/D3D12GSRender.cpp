#include "stdafx.h"
#if defined(DX12_SUPPORT)
#include "D3D12GSRender.h"
#include <wrl/client.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <thread>
#include <chrono>

GetGSFrameCb2 GetGSFrame = nullptr;

void SetGetD3DGSFrameCallback(GetGSFrameCb2 value)
{
	GetGSFrame = value;
}

void D3D12GSRender::ResourceStorage::Reset()
{
	m_vertexIndexBuffersHeapFreeSpace = 0;
	m_constantsBufferIndex = 0;
	m_currentScaleOffsetBufferIndex = 0;
	m_constantsBuffersHeapFreeSpace = 0;
	m_currentStorageOffset = 0;
	m_currentTextureIndex = 0;

	m_commandAllocator->Reset();
	m_textureUploadCommandAllocator->Reset();
	m_downloadCommandAllocator->Reset();
	for (ID3D12GraphicsCommandList *gfxCommandList : m_inflightCommandList)
		gfxCommandList->Release();
	m_inflightCommandList.clear();
	for (ID3D12Resource *vertexBuffer : m_inflightResources)
		vertexBuffer->Release();
	m_inflightResources.clear();
}

void D3D12GSRender::ResourceStorage::Init(ID3D12Device *device)
{
	m_queueCompletion = 0;
	// Create a global command allocator
	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator));
	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_textureUploadCommandAllocator));
	check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&m_downloadCommandAllocator)));

	// Create heap for vertex and constants buffers
	D3D12_HEAP_DESC vertexBufferHeapDesc = {};
	// 16 MB wide
	vertexBufferHeapDesc.SizeInBytes = 1024 * 1024 * 128;
	vertexBufferHeapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
	vertexBufferHeapDesc.Properties.Type = D3D12_HEAP_TYPE_UPLOAD;
	check(device->CreateHeap(&vertexBufferHeapDesc, IID_PPV_ARGS(&m_vertexIndexBuffersHeap)));
	check(device->CreateHeap(&vertexBufferHeapDesc, IID_PPV_ARGS(&m_constantsBuffersHeap)));

	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descriptorHeapDesc.NumDescriptors = 10000; // For safety
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	check(device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_constantsBufferDescriptorsHeap)));


	descriptorHeapDesc = {};
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descriptorHeapDesc.NumDescriptors = 10000; // For safety
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	check(device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_scaleOffsetDescriptorHeap)));

	// Texture
	D3D12_HEAP_DESC heapDescription = {};
	heapDescription.SizeInBytes = 1024 * 1024 * 512;
	heapDescription.Properties.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapDescription.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
	check(device->CreateHeap(&heapDescription, IID_PPV_ARGS(&m_uploadTextureHeap)));

	heapDescription.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapDescription.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
	check(device->CreateHeap(&heapDescription, IID_PPV_ARGS(&m_textureStorage)));

	D3D12_DESCRIPTOR_HEAP_DESC textureDescriptorDesc = {};
	textureDescriptorDesc.NumDescriptors = 1024; // For safety
	textureDescriptorDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	textureDescriptorDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	check(device->CreateDescriptorHeap(&textureDescriptorDesc, IID_PPV_ARGS(&m_textureDescriptorsHeap)));

	textureDescriptorDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	check(device->CreateDescriptorHeap(&textureDescriptorDesc, IID_PPV_ARGS(&m_samplerDescriptorHeap)));
}

void D3D12GSRender::ResourceStorage::Release()
{
	// NOTE: Should be released only if no command are in flight !

	m_constantsBufferDescriptorsHeap->Release();
	m_scaleOffsetDescriptorHeap->Release();
	m_constantsBuffersHeap->Release();
	m_vertexIndexBuffersHeap->Release();
	for (auto tmp : m_inflightResources)
		tmp->Release();
	m_textureDescriptorsHeap->Release();
	m_textureStorage->Release();
	m_uploadTextureHeap->Release();
	m_samplerDescriptorHeap->Release();
	for (auto tmp : m_inflightCommandList)
		tmp->Release();
	m_commandAllocator->Release();
	m_textureUploadCommandAllocator->Release();
	m_downloadCommandAllocator->Release();
}

// 32 bits float to U8 unorm CS
#define STRINGIFY(x) #x
const char *shaderCode = STRINGIFY(
	Texture2D<float> InputTexture : register(t0); \n
	RWTexture2D<float> OutputTexture : register(u0);\n

	[numthreads(8, 8, 1)]\n
	void main(uint3 Id : SV_DispatchThreadID)\n
{ \n
	OutputTexture[Id.xy] = InputTexture.Load(uint3(Id.xy, 0));\n
}
);

/**
 * returns bytecode and root signature of a Compute Shader converting texture from 
 * one format to another
 */
static
std::pair<ID3DBlob *, ID3DBlob *> compileF32toU8CS()
{
	ID3DBlob *bytecode;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
	HRESULT hr = D3DCompile(shaderCode, strlen(shaderCode), "test", nullptr, nullptr, "main", "cs_5_0", 0, 0, &bytecode, errorBlob.GetAddressOf());
	if (hr != S_OK)
	{
		const char *tmp = (const char*)errorBlob->GetBufferPointer();
		LOG_ERROR(RSX, tmp);
	}
	D3D12_DESCRIPTOR_RANGE descriptorRange[2] = {};
	// Textures
	descriptorRange[0].BaseShaderRegister = 0;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[1].BaseShaderRegister = 0;
	descriptorRange[1].NumDescriptors = 1;
	descriptorRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	descriptorRange[1].OffsetInDescriptorsFromTableStart = 1;
	D3D12_ROOT_PARAMETER RP[2] = {};
	RP[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	RP[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	RP[0].DescriptorTable.pDescriptorRanges = &descriptorRange[0];
	RP[0].DescriptorTable.NumDescriptorRanges = 2;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.NumParameters = 1;
	rootSignatureDesc.pParameters = RP;

	ID3DBlob *rootSignatureBlob;

	hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, &errorBlob);
	if (hr != S_OK)
	{
		const char *tmp = (const char*)errorBlob->GetBufferPointer();
		LOG_ERROR(RSX, tmp);
	}

	return std::make_pair(bytecode, rootSignatureBlob);
}

D3D12GSRender::D3D12GSRender()
	: GSRender(), m_fbo(nullptr), m_PSO(nullptr)
{
	if (Ini.GSDebugOutputEnable.GetValue())
	{
		Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;
		D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
		debugInterface->EnableDebugLayer();
	}

	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
	check(CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory)));
	// Create adapter
	IDXGIAdapter* adaptater = nullptr;
	switch (Ini.GSD3DAdaptater.GetValue())
	{
	case 0: // WARP
		check(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&adaptater)));
		break;
	case 1: // Default
		dxgiFactory->EnumAdapters(0, &adaptater);
		break;
	default: // Adaptater 0, 1, ...
		dxgiFactory->EnumAdapters(Ini.GSD3DAdaptater.GetValue() - 2,&adaptater);
		break;
	}
	check(D3D12CreateDevice(adaptater, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));

	// Queues
	D3D12_COMMAND_QUEUE_DESC copyQueueDesc = {}, graphicQueueDesc = {};
	copyQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	graphicQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	check(m_device->CreateCommandQueue(&copyQueueDesc, IID_PPV_ARGS(&m_commandQueueCopy)));
	check(m_device->CreateCommandQueue(&graphicQueueDesc, IID_PPV_ARGS(&m_commandQueueGraphic)));

	m_frame = GetGSFrame();
	DXGI_ADAPTER_DESC adaptaterDesc;
	adaptater->GetDesc(&adaptaterDesc);
	m_frame->SetAdaptaterName(adaptaterDesc.Description);

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

	// Common root signature
	D3D12_DESCRIPTOR_RANGE descriptorRange[4] = {};
	// Scale Offset data
	descriptorRange[0].BaseShaderRegister = 0;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	// Constants
	descriptorRange[1].BaseShaderRegister = 1;
	descriptorRange[1].NumDescriptors = 2;
	descriptorRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	// Textures
	descriptorRange[2].BaseShaderRegister = 0;
	descriptorRange[2].NumDescriptors = 16;
	descriptorRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	// Samplers
	descriptorRange[3].BaseShaderRegister = 0;
	descriptorRange[3].NumDescriptors = 16;
	descriptorRange[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	D3D12_ROOT_PARAMETER RP[4] = {};
	RP[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	RP[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	RP[0].DescriptorTable.pDescriptorRanges = &descriptorRange[0];
	RP[0].DescriptorTable.NumDescriptorRanges = 1;
	RP[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	RP[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	RP[1].DescriptorTable.pDescriptorRanges = &descriptorRange[1];
	RP[1].DescriptorTable.NumDescriptorRanges = 1;
	RP[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	RP[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	RP[2].DescriptorTable.pDescriptorRanges = &descriptorRange[2];
	RP[2].DescriptorTable.NumDescriptorRanges = 1;
	RP[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	RP[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	RP[3].DescriptorTable.pDescriptorRanges = &descriptorRange[3];
	RP[3].DescriptorTable.NumDescriptorRanges = 1;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.NumParameters = 4;
	rootSignatureDesc.pParameters = RP;

	Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
	check(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, &errorBlob));

	m_device->CreateRootSignature(0,
		rootSignatureBlob->GetBufferPointer(),
		rootSignatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&m_rootSignature));

	m_perFrameStorage.Init(m_device);
	m_perFrameStorage.Reset();

	vertexConstantShadowCopy = new float[512 * 4];

	// Convert shader
	auto p = compileF32toU8CS();
	check(
		m_device->CreateRootSignature(0, p.second->GetBufferPointer(), p.second->GetBufferSize(), IID_PPV_ARGS(&m_convertRootSignature))
		);

	D3D12_COMPUTE_PIPELINE_STATE_DESC computePipelineStateDesc = {};
	computePipelineStateDesc.CS.BytecodeLength = p.first->GetBufferSize();
	computePipelineStateDesc.CS.pShaderBytecode = p.first->GetBufferPointer();
	computePipelineStateDesc.pRootSignature = m_convertRootSignature;

	check(
		m_device->CreateComputePipelineState(&computePipelineStateDesc, IID_PPV_ARGS(&m_convertPSO))
		);

	p.first->Release();
	p.second->Release();

	D3D12_HEAP_PROPERTIES hp = {};
	hp.Type = D3D12_HEAP_TYPE_DEFAULT;
	check(
		m_device->CreateCommittedResource(
			&hp,
			D3D12_HEAP_FLAG_NONE,
			&getTexture2DResourceDesc(2, 2, DXGI_FORMAT_R8G8B8A8_UNORM),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_dummyTexture))
			);
}

D3D12GSRender::~D3D12GSRender()
{
	m_dummyTexture->Release();
	m_convertPSO->Release();
	m_convertRootSignature->Release();
	m_perFrameStorage.Release();
	m_commandQueueGraphic->Release();
	m_commandQueueCopy->Release();
	m_backbufferAsRendertarget[0]->Release();
	m_backBuffer[0]->Release();
	m_backbufferAsRendertarget[1]->Release();
	m_backBuffer[1]->Release();
	if (m_fbo)
		delete m_fbo;
	m_rootSignature->Release();
	m_swapChain->Release();
	m_device->Release();
	delete[] vertexConstantShadowCopy;
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
		float clearColor[] =
		{
			m_clear_surface_color_r / 255.0f,
			m_clear_surface_color_g / 255.0f,
			m_clear_surface_color_b / 255.0f,
			m_clear_surface_color_a / 255.0f
		};

		m_fbo = new D3D12RenderTargetSets(m_device, (u8)m_lastDepth, m_lastWidth, m_lastHeight, clearColor, 1.f);
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
	check(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_perFrameStorage.m_commandAllocator, nullptr, IID_PPV_ARGS(&commandList)));
	m_perFrameStorage.m_inflightCommandList.push_back(commandList);

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

static
D3D12_BLEND_OP getBlendOp()
{
	return D3D12_BLEND_OP_ADD;
}

static
D3D12_BLEND getBlendFactor(u16 glFactor)
{
	switch (glFactor)
	{
	default: LOG_WARNING(RSX, "Unsupported Blend Op %d", glFactor);
	case GL_ZERO: return D3D12_BLEND_ZERO;
	case GL_ONE: return D3D12_BLEND_ONE;
	case GL_SRC_COLOR: return D3D12_BLEND_SRC_COLOR;
	case GL_ONE_MINUS_SRC_COLOR: return D3D12_BLEND_INV_SRC_COLOR;
	case GL_DST_COLOR: return D3D12_BLEND_DEST_COLOR;
	case GL_ONE_MINUS_DST_COLOR: D3D12_BLEND_INV_DEST_COLOR;
	case GL_SRC_ALPHA: return D3D12_BLEND_SRC_ALPHA;
	case GL_ONE_MINUS_SRC_ALPHA: return D3D12_BLEND_INV_SRC_ALPHA;
	case GL_DST_ALPHA: return D3D12_BLEND_DEST_ALPHA;
	case GL_ONE_MINUS_DST_ALPHA: return D3D12_BLEND_INV_DEST_ALPHA;
	case GL_SRC_ALPHA_SATURATE: return D3D12_BLEND_SRC_ALPHA_SAT;
	}
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

	D3D12PipelineProperties prop = {};
	switch (m_draw_mode - 1)
	{
	case GL_POINTS:
		prop.Topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		break;
	case GL_LINES:
	case GL_LINE_LOOP:
	case GL_LINE_STRIP:
		prop.Topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		break;
	case GL_TRIANGLES:
	case GL_TRIANGLE_STRIP:
	case GL_TRIANGLE_FAN:
		prop.Topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		break;
	case GL_QUADS:
	case GL_QUAD_STRIP:
	case GL_POLYGON:
	default:
//		LOG_ERROR(RSX, "Unsupported primitive type");
		prop.Topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		break;
	}

	static D3D12_BLEND_DESC CD3D12_BLEND_DESC =
	{
		FALSE,
		FALSE,
		{
			FALSE,FALSE,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP,
		D3D12_COLOR_WRITE_ENABLE_ALL,
		}
	};
	prop.Blend = CD3D12_BLEND_DESC;

	if (m_set_blend_equation)
	{
//		glBlendEquationSeparate(m_blend_equation_rgb, m_blend_equation_alpha);
//		checkForGlError("glBlendEquationSeparate");
	}

	if (m_set_blend_sfactor && m_set_blend_dfactor)
	{
		prop.Blend.RenderTarget[0].BlendEnable = true;
		prop.Blend.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		prop.Blend.RenderTarget[0].SrcBlend = getBlendFactor(m_blend_sfactor_rgb);
		prop.Blend.RenderTarget[0].DestBlend = getBlendFactor(m_blend_dfactor_rgb);
		prop.Blend.RenderTarget[0].SrcBlendAlpha = getBlendFactor(m_blend_sfactor_alpha);
		prop.Blend.RenderTarget[0].DestBlendAlpha = getBlendFactor(m_blend_dfactor_alpha);
		prop.Blend.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	}

	if (m_set_blend_color)
	{
//		glBlendColor(m_blend_color_r, m_blend_color_g, m_blend_color_b, m_blend_color_a);
//		checkForGlError("glBlendColor");
	}

	switch (m_surface_depth_format)
	{
	case 0:
		break;
	case CELL_GCM_SURFACE_Z16:
		prop.DepthStencilFormat = DXGI_FORMAT_D16_UNORM;
		break;
	case CELL_GCM_SURFACE_Z24S8:
		prop.DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		break;
	default:
		LOG_ERROR(RSX, "Bad depth format! (%d)", m_surface_depth_format);
		assert(0);
	}

	switch (m_surface_color_target)
	{
	case CELL_GCM_SURFACE_TARGET_0:
	case CELL_GCM_SURFACE_TARGET_1:
		prop.numMRT = 1;
		break;
	case CELL_GCM_SURFACE_TARGET_MRT1:
		prop.numMRT = 2;
		break;
	case CELL_GCM_SURFACE_TARGET_MRT2:
		prop.numMRT = 3;
		break;
	case CELL_GCM_SURFACE_TARGET_MRT3:
		prop.numMRT = 4;
		break;
	default:
		LOG_ERROR(RSX, "Bad surface color target: %d", m_surface_color_target);
	}

	prop.depthEnabled = m_set_depth_test;

	prop.IASet = m_IASet;

	m_PSO = m_cachePSO.getGraphicPipelineState(m_cur_vertex_prog, m_cur_fragment_prog, prop, std::make_pair(m_device, m_rootSignature));
	return m_PSO != nullptr;
}

void D3D12GSRender::ExecCMD()
{
	ID3D12GraphicsCommandList *commandList;
	m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_perFrameStorage.m_commandAllocator, nullptr, IID_PPV_ARGS(&commandList));
	m_perFrameStorage.m_inflightCommandList.push_back(commandList);

	commandList->SetGraphicsRootSignature(m_rootSignature);

	if (m_indexed_array.m_count)
		LoadVertexData(m_indexed_array.index_min, m_indexed_array.index_max - m_indexed_array.index_min + 1);

	if (m_indexed_array.m_count || m_draw_array_count)
	{
		const std::pair<std::vector<D3D12_VERTEX_BUFFER_VIEW>, D3D12_INDEX_BUFFER_VIEW> &vertexIndexBufferViews = EnableVertexData(m_indexed_array.m_count ? true : false);
		commandList->IASetVertexBuffers(0, (UINT)vertexIndexBufferViews.first.size(), vertexIndexBufferViews.first.data());
		if (m_forcedIndexBuffer || m_indexed_array.m_count)
			commandList->IASetIndexBuffer(&vertexIndexBufferViews.second);
	}

	if (!LoadProgram())
	{
		LOG_ERROR(RSX, "LoadProgram failed.");
		Emu.Pause();
		return;
	}

	// Constants
	setScaleOffset();
	commandList->SetDescriptorHeaps(1, &m_perFrameStorage.m_scaleOffsetDescriptorHeap);
	D3D12_GPU_DESCRIPTOR_HANDLE Handle = m_perFrameStorage.m_scaleOffsetDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	Handle.ptr += m_perFrameStorage.m_currentScaleOffsetBufferIndex * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	commandList->SetGraphicsRootDescriptorTable(0, Handle);
	m_perFrameStorage.m_currentScaleOffsetBufferIndex++;

	size_t currentBufferIndex = m_perFrameStorage.m_constantsBufferIndex;
	FillVertexShaderConstantsBuffer();
	m_perFrameStorage.m_constantsBufferIndex++;
	FillPixelShaderConstantsBuffer();
	m_perFrameStorage.m_constantsBufferIndex++;

	commandList->SetDescriptorHeaps(1, &m_perFrameStorage.m_constantsBufferDescriptorsHeap);
	Handle = m_perFrameStorage.m_constantsBufferDescriptorsHeap->GetGPUDescriptorHandleForHeapStart();
	Handle.ptr += currentBufferIndex * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	commandList->SetGraphicsRootDescriptorTable(1, Handle);
	commandList->SetPipelineState(m_PSO);

	size_t usedTexture = UploadTextures();
	// Drivers don't like undefined texture descriptors
	for (; usedTexture < 16; usedTexture++)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		D3D12_CPU_DESCRIPTOR_HANDLE Handle = m_perFrameStorage.m_textureDescriptorsHeap->GetCPUDescriptorHandleForHeapStart();
		Handle.ptr += (m_perFrameStorage.m_currentTextureIndex + usedTexture) * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_device->CreateShaderResourceView(m_dummyTexture, &srvDesc, Handle);

		D3D12_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		Handle = m_perFrameStorage.m_samplerDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		Handle.ptr += (m_perFrameStorage.m_currentTextureIndex + usedTexture) * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_device->CreateSampler(&samplerDesc, Handle);
	}

	Handle = m_perFrameStorage.m_textureDescriptorsHeap->GetGPUDescriptorHandleForHeapStart();
	Handle.ptr += m_perFrameStorage.m_currentTextureIndex * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	commandList->SetDescriptorHeaps(1, &m_perFrameStorage.m_textureDescriptorsHeap);
	commandList->SetGraphicsRootDescriptorTable(2, Handle);

	Handle = m_perFrameStorage.m_samplerDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	Handle.ptr += m_perFrameStorage.m_currentTextureIndex * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	commandList->SetDescriptorHeaps(1, &m_perFrameStorage.m_samplerDescriptorHeap);
	commandList->SetGraphicsRootDescriptorTable(3, Handle);

	m_perFrameStorage.m_currentTextureIndex += 16;

	InitDrawBuffers();

	D3D12_CPU_DESCRIPTOR_HANDLE *DepthStencilHandle = &m_fbo->getDSVCPUHandle();
	switch (m_surface_color_target)
	{
	case CELL_GCM_SURFACE_TARGET_NONE: break;
	case CELL_GCM_SURFACE_TARGET_0:
		commandList->OMSetRenderTargets(1, &m_fbo->getRTTCPUHandle(0), true, DepthStencilHandle);
		break;
	case CELL_GCM_SURFACE_TARGET_1:
		commandList->OMSetRenderTargets(1, &m_fbo->getRTTCPUHandle(1), true, DepthStencilHandle);
		break;
	case CELL_GCM_SURFACE_TARGET_MRT1:
		commandList->OMSetRenderTargets(2, &m_fbo->getRTTCPUHandle(0), true, DepthStencilHandle);
		break;
	case CELL_GCM_SURFACE_TARGET_MRT2:
		commandList->OMSetRenderTargets(3, &m_fbo->getRTTCPUHandle(0), true, DepthStencilHandle);
		break;
	case CELL_GCM_SURFACE_TARGET_MRT3:
		commandList->OMSetRenderTargets(4, &m_fbo->getRTTCPUHandle(0), true, DepthStencilHandle);
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

	bool requireIndexBuffer = false;
	switch (m_draw_mode - 1)
	{
	case GL_POINTS:
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
		break;
	case GL_LINES:
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		break;
	case GL_LINE_LOOP:
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ);
		break;
	case GL_LINE_STRIP:
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);
		break;
	case GL_TRIANGLES:
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		break;
	case GL_TRIANGLE_STRIP:
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		break;
	case GL_TRIANGLE_FAN:
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ);
		break;
	case GL_QUADS:
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		requireIndexBuffer = true;
	case GL_QUAD_STRIP:
	case GL_POLYGON:
	default:
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//		LOG_ERROR(RSX, "Unsupported primitive type");
		break;
	}

	// Indexed quad
	if (m_forcedIndexBuffer && m_indexed_array.m_count)
		commandList->DrawIndexedInstanced((UINT)indexCount, 1, 0, 0, 0);
	// Non indexed quad
	else if (m_forcedIndexBuffer && !m_indexed_array.m_count)
		commandList->DrawIndexedInstanced((UINT)indexCount, 1, 0, (UINT)m_draw_array_first, 0);
	// Indexed triangles
	else if (m_indexed_array.m_count)
		commandList->DrawIndexedInstanced((UINT)m_indexed_array.m_data.size() / 4, 1, 0, (UINT)m_draw_array_first, 0);
	else if (m_draw_array_count)
		commandList->DrawInstanced(m_draw_array_count, 1, m_draw_array_first, 0);

	check(commandList->Close());
	m_commandQueueGraphic->ExecuteCommandLists(1, (ID3D12CommandList**)&commandList);
	m_indexed_array.Reset();
	WriteDepthBuffer();

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
	m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_perFrameStorage.m_commandAllocator, nullptr, IID_PPV_ARGS(&commandList));
	m_perFrameStorage.m_inflightCommandList.push_back(commandList);

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

	check(m_swapChain->Present(Ini.GSVSyncEnable.GetValue() ? 1 : 0, 0));
	// Add an event signaling queue completion
	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
	m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	HANDLE handle = CreateEvent(0, 0, 0, 0);
	fence->SetEventOnCompletion(1, handle);
	m_commandQueueGraphic->Signal(fence.Get(), 1);
	WaitForSingleObject(handle, INFINITE);
	CloseHandle(handle);
	m_perFrameStorage.Reset();

	m_frame->Flip(nullptr);
}


void D3D12GSRender::WriteDepthBuffer()
{
}


void D3D12GSRender::semaphorePGRAPHBackendRelease(u32 offset, u32 value)
{
	// Add all buffer write
	// Cell can't make any assumption about readyness of color/depth buffer
	// Except when a semaphore is written by RSX


/*	if (!Ini.GSDumpDepthBuffer.GetValue())
		return;*/

	ID3D12Fence *fence;
	check(
		m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))
		);
	HANDLE handle = CreateEvent(0, FALSE, FALSE, 0);
	fence->SetEventOnCompletion(1, handle);

	ID3D12Resource *writeDest, *depthConverted;
	ID3D12GraphicsCommandList *convertCommandList, *downloadCommandList;
	ID3D12DescriptorHeap *descriptorHeap;
	size_t rowPitch = RSXThread::m_width;
	rowPitch = (rowPitch + 255) & ~255;
	if (m_set_context_dma_z)
	{
		D3D12_HEAP_PROPERTIES heapProp = {};
		heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
		D3D12_RESOURCE_DESC resdesc = {};
		resdesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resdesc.Width = RSXThread::m_width;
		resdesc.Height = RSXThread::m_height;
		resdesc.DepthOrArraySize = 1;
		resdesc.SampleDesc.Count = 1;
		resdesc.MipLevels = 1;
		resdesc.Format = DXGI_FORMAT_R8_UNORM;
		resdesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		check(
			m_device->CreateCommittedResource(
				&heapProp,
				D3D12_HEAP_FLAG_NONE,
				&resdesc,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				nullptr,
				IID_PPV_ARGS(&depthConverted)
				)
			);

		heapProp = {};
		heapProp.Type = D3D12_HEAP_TYPE_READBACK;
		resdesc = {};
		resdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resdesc.Width = RSXThread::m_width * RSXThread::m_height * 2; // * 2 for safety
		resdesc.Height = 1;
		resdesc.DepthOrArraySize = 1;
		resdesc.SampleDesc.Count = 1;
		resdesc.MipLevels = 1;
		resdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		check(
			m_device->CreateCommittedResource(
				&heapProp,
				D3D12_HEAP_FLAG_NONE,
				&resdesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&writeDest)
				)
			);

		check(
			m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_perFrameStorage.m_commandAllocator, nullptr, IID_PPV_ARGS(&convertCommandList))
			);

		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
		descriptorHeapDesc.NumDescriptors = 2;
		descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		check(
			m_device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap))
			);
		D3D12_CPU_DESCRIPTOR_HANDLE Handle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		switch (m_surface_depth_format)
		{
		case 0:
			break;
		case CELL_GCM_SURFACE_Z16:
			srvDesc.Format = DXGI_FORMAT_R16_UNORM;
			break;
		case CELL_GCM_SURFACE_Z24S8:
			srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			break;
		default:
			LOG_ERROR(RSX, "Bad depth format! (%d)", m_surface_depth_format);
			assert(0);
		}
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		m_device->CreateShaderResourceView(m_fbo->getDepthStencilTexture(), &srvDesc, Handle);
		Handle.ptr += m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_R8_UNORM;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		m_device->CreateUnorderedAccessView(depthConverted, nullptr, &uavDesc, Handle);


		// Convert
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = m_fbo->getDepthStencilTexture();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
		convertCommandList->ResourceBarrier(1, &barrier);

		convertCommandList->SetPipelineState(m_convertPSO);
		convertCommandList->SetComputeRootSignature(m_convertRootSignature);
		convertCommandList->SetDescriptorHeaps(1, &descriptorHeap);
		convertCommandList->SetComputeRootDescriptorTable(0, descriptorHeap->GetGPUDescriptorHandleForHeapStart());
		convertCommandList->Dispatch(RSXThread::m_width / 8, RSXThread::m_height / 8, 1);

		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_GENERIC_READ;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		// Flush UAV
		D3D12_RESOURCE_BARRIER uavbarrier = {};
		uavbarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uavbarrier.UAV.pResource = depthConverted;

		D3D12_RESOURCE_BARRIER barriers[] =
		{
			barrier,
			uavbarrier,
		};
		convertCommandList->ResourceBarrier(2, barriers);

		barrier.Transition.pResource = depthConverted;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
		convertCommandList->ResourceBarrier(1, &barrier);

		convertCommandList->Close();
		m_commandQueueGraphic->ExecuteCommandLists(1, (ID3D12CommandList**)&convertCommandList);

		ID3D12Fence *convertDownloadFence;
		check(
			m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&convertDownloadFence))
			);
		m_commandQueueGraphic->Signal(convertDownloadFence, 1);

		check(
			m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, m_perFrameStorage.m_downloadCommandAllocator, nullptr, IID_PPV_ARGS(&downloadCommandList))
			);

		// Copy
		D3D12_TEXTURE_COPY_LOCATION dst = {}, src = {};
		src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		src.pResource = depthConverted;
		dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		dst.pResource = writeDest;
		dst.PlacedFootprint.Offset = 0;
		dst.PlacedFootprint.Footprint.Depth = 1;
		dst.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8_UNORM;
		dst.PlacedFootprint.Footprint.Height = RSXThread::m_height;
		dst.PlacedFootprint.Footprint.Width = RSXThread::m_width;
		dst.PlacedFootprint.Footprint.RowPitch = (UINT)rowPitch;
		downloadCommandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

		downloadCommandList->Close();
		m_commandQueueCopy->Wait(convertDownloadFence, 1);
		m_commandQueueCopy->ExecuteCommandLists(1, (ID3D12CommandList**)&downloadCommandList);
		//Wait for result
		m_commandQueueCopy->Signal(fence, 1);
		convertDownloadFence->Release();
	}
	else
		m_commandQueueGraphic->Signal(fence, 1);

	std::thread valueChangerThread([=]() {
		WaitForSingleObject(handle, INFINITE);
		CloseHandle(handle);
		fence->Release();

		if (m_set_context_dma_z)
		{
			u32 address = GetAddress(m_surface_offset_z, m_context_dma_z - 0xfeed0000);
			auto ptr = vm::get_ptr<void>(address);
			char *ptrAsChar = (char*)ptr;
			unsigned char *writeDestPtr;
			check(writeDest->Map(0, nullptr, (void**)&writeDestPtr));
			// TODO : this should be done by the gpu
			for (unsigned row = 0; row < RSXThread::m_height; row++)
			{
				for (unsigned i = 0; i < RSXThread::m_width; i++)
				{
					unsigned char c = writeDestPtr[row * rowPitch + i];
					ptrAsChar[4 * (row * RSXThread::m_width + i)] = c;
					ptrAsChar[4 * (row * RSXThread::m_width + i) + 1] = c;
					ptrAsChar[4 * (row * RSXThread::m_width + i) + 2] = c;
					ptrAsChar[4 * (row * RSXThread::m_width + i) + 3] = c;
				}
			}
			writeDest->Release();
			depthConverted->Release();
			descriptorHeap->Release();
			downloadCommandList->Release();
			convertCommandList->Release();
		}

		vm::write32(m_label_addr + offset, value);
	});
	valueChangerThread.detach();
}

void D3D12GSRender::semaphorePFIFOAcquire(u32 offset, u32 value)
{
	ID3D12Fence *fence;
	check(
		m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))
		);
	m_commandQueueGraphic->Wait(fence, 1);

	std::thread valueChangerThread([=]() {
		while (true)
		{
			u32 val = vm::read32(m_label_addr + offset);
			if (val == value) break;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		fence->Signal(1);
		fence->Release();
	}
	);
	valueChangerThread.detach();
}
#endif