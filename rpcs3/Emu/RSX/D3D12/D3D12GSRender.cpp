#include "stdafx.h"
#include "stdafx_d3d12.h"
#ifdef _MSC_VER
#include "D3D12GSRender.h"
#include <wrl/client.h>
#include <dxgi1_4.h>
#include <thread>
#include <chrono>
#include "d3dx12.h"
#include <d3d11on12.h>
#include "Emu/state.h"
#include "D3D12Formats.h"

PFN_D3D12_CREATE_DEVICE wrapD3D12CreateDevice;
PFN_D3D12_GET_DEBUG_INTERFACE wrapD3D12GetDebugInterface;
PFN_D3D12_SERIALIZE_ROOT_SIGNATURE wrapD3D12SerializeRootSignature;
PFN_D3D11ON12_CREATE_DEVICE wrapD3D11On12CreateDevice;
pD3DCompile wrapD3DCompile;

namespace
{
HMODULE D3D12Module;
HMODULE D3D11Module;
HMODULE D3DCompiler;

void loadD3D12FunctionPointers()
{
	CHECK_ASSERTION(D3D12Module = LoadLibrary(L"d3d12.dll"));
	wrapD3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(D3D12Module, "D3D12CreateDevice");
	wrapD3D12GetDebugInterface = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(D3D12Module, "D3D12GetDebugInterface");
	wrapD3D12SerializeRootSignature = (PFN_D3D12_SERIALIZE_ROOT_SIGNATURE)GetProcAddress(D3D12Module, "D3D12SerializeRootSignature");
	CHECK_ASSERTION(D3D11Module = LoadLibrary(L"d3d11.dll"));
	wrapD3D11On12CreateDevice = (PFN_D3D11ON12_CREATE_DEVICE)GetProcAddress(D3D11Module, "D3D11On12CreateDevice");
	CHECK_ASSERTION(D3DCompiler = LoadLibrary(L"d3dcompiler_47.dll"));
	wrapD3DCompile = (pD3DCompile)GetProcAddress(D3DCompiler, "D3DCompile");
}

void unloadD3D12FunctionPointers()
{
	FreeLibrary(D3D12Module);
	FreeLibrary(D3D11Module);
	FreeLibrary(D3DCompiler);
}

/**
 * Wait until command queue has completed all task.
 */ 
void wait_for_command_queue(ID3D12Device *device, ID3D12CommandQueue *command_queue)
{
	ComPtr<ID3D12Fence> fence;
	CHECK_HRESULT(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf())));
	HANDLE handle = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
	fence->SetEventOnCompletion(1, handle);
	command_queue->Signal(fence.Get(), 1);
	WaitForSingleObjectEx(handle, INFINITE, FALSE);
	CloseHandle(handle);
}
}

void D3D12GSRender::Shader::Release()
{
	m_PSO->Release();
	m_rootSignature->Release();
	m_vertexBuffer->Release();
	m_textureDescriptorHeap->Release();
	m_samplerDescriptorHeap->Release();
}

extern std::function<bool(u32 addr)> gfxHandler;

bool D3D12GSRender::invalidate_address(u32 addr)
{
	bool result = false;
	result |= m_texture_cache.invalidate_address(addr);
	return result;
}

D3D12DLLManagement::D3D12DLLManagement()
{
	loadD3D12FunctionPointers();
}

D3D12DLLManagement::~D3D12DLLManagement()
{
	unloadD3D12FunctionPointers();
}

D3D12GSRender::D3D12GSRender()
	: GSRender(frame_type::DX12), m_d3d12_lib(), m_current_pso(nullptr)
{
	m_previous_address_a = 0;
	m_previous_address_b = 0;
	m_previous_address_c = 0;
	m_previous_address_d = 0;
	m_previous_address_z = 0;
	gfxHandler = [this](u32 addr) {
		bool result = invalidate_address(addr);
		if (result)
				LOG_WARNING(RSX, "Reporting Cell writing to 0x%x", addr);
		return result;
	};
	if (rpcs3::config.rsx.d3d12.debug_output.value())
	{
		Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;
		wrapD3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
		debugInterface->EnableDebugLayer();
	}

	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgi_factory;
	CHECK_HRESULT(CreateDXGIFactory(IID_PPV_ARGS(&dxgi_factory)));
	// Create adapter
	ComPtr<IDXGIAdapter> adaptater = nullptr;
	CHECK_HRESULT(dxgi_factory->EnumAdapters(rpcs3::state.config.rsx.d3d12.adaptater.value(), adaptater.GetAddressOf()));
	CHECK_HRESULT(wrapD3D12CreateDevice(adaptater.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));

	// Queues
	D3D12_COMMAND_QUEUE_DESC graphic_queue_desc = { D3D12_COMMAND_LIST_TYPE_DIRECT };
	CHECK_HRESULT(m_device->CreateCommandQueue(&graphic_queue_desc, IID_PPV_ARGS(m_command_queue.GetAddressOf())));

	g_descriptor_stride_srv_cbv_uav = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	g_descriptor_stride_dsv = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	g_descriptor_stride_rtv = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	g_descriptor_stride_samplers = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	// Create swap chain and put them in a descriptor heap as rendertarget
	DXGI_SWAP_CHAIN_DESC swap_chain = {};
	swap_chain.BufferCount = 2;
	swap_chain.Windowed = true;
	swap_chain.OutputWindow = (HWND)m_frame->handle();
	swap_chain.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swap_chain.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain.SampleDesc.Count = 1;
	swap_chain.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swap_chain.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

	CHECK_HRESULT(dxgi_factory->CreateSwapChain(m_command_queue.Get(), &swap_chain, (IDXGISwapChain**)m_swap_chain.GetAddressOf()));
	m_swap_chain->GetBuffer(0, IID_PPV_ARGS(&m_backbuffer[0]));
	m_swap_chain->GetBuffer(1, IID_PPV_ARGS(&m_backbuffer[1]));

	D3D12_DESCRIPTOR_HEAP_DESC render_target_descriptor_heap_desc = { D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1};
	D3D12_RENDER_TARGET_VIEW_DESC renter_target_view_desc = {};
	renter_target_view_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	renter_target_view_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	m_device->CreateDescriptorHeap(&render_target_descriptor_heap_desc, IID_PPV_ARGS(&m_backbuffer_descriptor_heap[0]));
	m_device->CreateRenderTargetView(m_backbuffer[0].Get(), &renter_target_view_desc, m_backbuffer_descriptor_heap[0]->GetCPUDescriptorHandleForHeapStart());
	m_device->CreateDescriptorHeap(&render_target_descriptor_heap_desc, IID_PPV_ARGS(&m_backbuffer_descriptor_heap[1]));
	m_device->CreateRenderTargetView(m_backbuffer[1].Get(), &renter_target_view_desc, m_backbuffer_descriptor_heap[1]->GetCPUDescriptorHandleForHeapStart());

	// Common root signatures
	for (unsigned texture_count = 0; texture_count < 17; texture_count++)
	{
		CD3DX12_DESCRIPTOR_RANGE descriptorRange[] =
		{
			// Scale Offset data
			CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0),
			// Constants
			CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 2, 1),
			// Textures
			CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, texture_count, 0),
			// Samplers
			CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, texture_count, 0),
		};
		CD3DX12_ROOT_PARAMETER RP[2];
		RP[0].InitAsDescriptorTable((texture_count > 0) ? 3 : 2, &descriptorRange[0]);
		RP[1].InitAsDescriptorTable(1, &descriptorRange[3]);

		Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
		CHECK_HRESULT(wrapD3D12SerializeRootSignature(
			&CD3DX12_ROOT_SIGNATURE_DESC((texture_count > 0) ? 2 : 1, RP, 0, 0, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),
			D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, &errorBlob));

		m_device->CreateRootSignature(0,
			rootSignatureBlob->GetBufferPointer(),
			rootSignatureBlob->GetBufferSize(),
			IID_PPV_ARGS(m_root_signatures[texture_count].GetAddressOf()));
	}

	m_per_frame_storage[0].init(m_device.Get());
	m_per_frame_storage[0].reset();
	m_per_frame_storage[1].init(m_device.Get());
	m_per_frame_storage[1].reset();

	initConvertShader();
	m_output_scaling_pass.Init(m_device.Get(), m_command_queue.Get());

	CHECK_HRESULT(
		m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 2, 2, 1, 1),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_dummy_texture))
			);

	m_readback_resources.init(m_device.Get(), 1024 * 1024 * 128, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST);
	m_uav_heap.init(m_device.Get(), 1024 * 1024 * 128, D3D12_HEAP_TYPE_DEFAULT, D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES);

	m_rtts.init(m_device.Get());

	m_constants_data.init(m_device.Get(), 1024 * 1024 * 64, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
	m_vertex_index_data.init(m_device.Get(), 1024 * 1024 * 384, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
	m_texture_upload_data.init(m_device.Get(), 1024 * 1024 * 512, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);

	if (rpcs3::config.rsx.d3d12.overlay.value())
		init_d2d_structures();
}

D3D12GSRender::~D3D12GSRender()
{
	wait_for_command_queue(m_device.Get(), m_command_queue.Get());

	m_texture_cache.unprotect_all();

	gfxHandler = [this](u32) { return false; };
	m_constants_data.release();
	m_vertex_index_data.release();
	m_texture_upload_data.release();
	m_uav_heap.m_heap->Release();
	m_readback_resources.m_heap->Release();
	m_dummy_texture->Release();
	m_convertPSO->Release();
	m_convertRootSignature->Release();
	m_per_frame_storage[0].release();
	m_per_frame_storage[1].release();
	m_output_scaling_pass.Release();

	release_d2d_structures();
}

void D3D12GSRender::on_exit()
{
}

bool D3D12GSRender::do_method(u32 cmd, u32 arg)
{
	switch (cmd)
	{
	case NV4097_CLEAR_SURFACE:
		clear_surface(arg);
		return true;
	case NV4097_TEXTURE_READ_SEMAPHORE_RELEASE:
		copy_render_target_to_dma_location();
		return false; //call rsx::thread method implementation
	case NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE:
		copy_render_target_to_dma_location();
		return false; //call rsx::thread method implementation

	default:
		return false;
	}
}

void D3D12GSRender::end()
{
	std::chrono::time_point<std::chrono::system_clock> start_duration = std::chrono::system_clock::now();

	std::chrono::time_point<std::chrono::system_clock> rtt_duration_start = std::chrono::system_clock::now();
	prepare_render_targets(get_current_resource_storage().command_list.Get());

	std::chrono::time_point<std::chrono::system_clock> rtt_duration_end = std::chrono::system_clock::now();
	m_timers.m_prepare_rtt_duration += std::chrono::duration_cast<std::chrono::microseconds>(rtt_duration_end - rtt_duration_start).count();

	std::chrono::time_point<std::chrono::system_clock> vertex_index_duration_start = std::chrono::system_clock::now();

	if (!vertex_index_array.empty() || vertex_draw_count)
		upload_and_set_vertex_index_data(get_current_resource_storage().command_list.Get());

	std::chrono::time_point<std::chrono::system_clock> vertex_index_duration_end = std::chrono::system_clock::now();
	m_timers.m_vertex_index_duration += std::chrono::duration_cast<std::chrono::microseconds>(vertex_index_duration_end - vertex_index_duration_start).count();

	std::chrono::time_point<std::chrono::system_clock> program_load_start = std::chrono::system_clock::now();
	if (!load_program())
	{
		LOG_ERROR(RSX, "LoadProgram failed.");
		Emu.Pause();
		return;
	}
	std::chrono::time_point<std::chrono::system_clock> program_load_end = std::chrono::system_clock::now();
	m_timers.m_program_load_duration += std::chrono::duration_cast<std::chrono::microseconds>(program_load_end - program_load_start).count();

	get_current_resource_storage().command_list->SetGraphicsRootSignature(m_root_signatures[std::get<2>(*m_current_pso)].Get());
	get_current_resource_storage().command_list->OMSetStencilRef(rsx::method_registers[NV4097_SET_STENCIL_FUNC_REF]);

	std::chrono::time_point<std::chrono::system_clock> constants_duration_start = std::chrono::system_clock::now();

	size_t currentDescriptorIndex = get_current_resource_storage().descriptors_heap_index;
	// Constants
	upload_and_bind_scale_offset_matrix(currentDescriptorIndex);
	upload_and_bind_vertex_shader_constants(currentDescriptorIndex + 1);
	upload_and_bind_fragment_shader_constants(currentDescriptorIndex + 2);

	std::chrono::time_point<std::chrono::system_clock> constants_duration_end = std::chrono::system_clock::now();
	m_timers.m_constants_duration += std::chrono::duration_cast<std::chrono::microseconds>(constants_duration_end - constants_duration_start).count();

	get_current_resource_storage().command_list->SetPipelineState(std::get<0>(*m_current_pso));

	std::chrono::time_point<std::chrono::system_clock> texture_duration_start = std::chrono::system_clock::now();
	if (std::get<2>(*m_current_pso) > 0)
	{
		upload_and_bind_textures(get_current_resource_storage().command_list.Get(), currentDescriptorIndex + 3, std::get<2>(*m_current_pso) > 0);


		get_current_resource_storage().command_list->SetGraphicsRootDescriptorTable(0,
			CD3DX12_GPU_DESCRIPTOR_HANDLE(get_current_resource_storage().descriptors_heap->GetGPUDescriptorHandleForHeapStart())
			.Offset((INT)currentDescriptorIndex, g_descriptor_stride_srv_cbv_uav)
			);
		get_current_resource_storage().command_list->SetGraphicsRootDescriptorTable(1,
			CD3DX12_GPU_DESCRIPTOR_HANDLE(get_current_resource_storage().sampler_descriptor_heap[get_current_resource_storage().sampler_descriptors_heap_index]->GetGPUDescriptorHandleForHeapStart())
			.Offset((INT)get_current_resource_storage().current_sampler_index, g_descriptor_stride_samplers)
			);

		get_current_resource_storage().current_sampler_index += std::get<2>(*m_current_pso);
		get_current_resource_storage().descriptors_heap_index += std::get<2>(*m_current_pso) + 3;
	}
	else
	{
		get_current_resource_storage().command_list->SetDescriptorHeaps(1, get_current_resource_storage().descriptors_heap.GetAddressOf());
		get_current_resource_storage().command_list->SetGraphicsRootDescriptorTable(0,
			CD3DX12_GPU_DESCRIPTOR_HANDLE(get_current_resource_storage().descriptors_heap->GetGPUDescriptorHandleForHeapStart())
			.Offset((INT)currentDescriptorIndex, g_descriptor_stride_srv_cbv_uav)
			);
		get_current_resource_storage().descriptors_heap_index += 3;
	}

	std::chrono::time_point<std::chrono::system_clock> texture_duration_end = std::chrono::system_clock::now();
	m_timers.m_texture_duration += std::chrono::duration_cast<std::chrono::microseconds>(texture_duration_end - texture_duration_start).count();
	set_rtt_and_ds(get_current_resource_storage().command_list.Get());

	int clip_w = rsx::method_registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL] >> 16;
	int clip_h = rsx::method_registers[NV4097_SET_SURFACE_CLIP_VERTICAL] >> 16;

	D3D12_VIEWPORT viewport =
	{
		0.f,
		0.f,
		(float)clip_w,
		(float)clip_h,
		(f32&)rsx::method_registers[NV4097_SET_CLIP_MIN],
		(f32&)rsx::method_registers[NV4097_SET_CLIP_MAX]
	};
	get_current_resource_storage().command_list->RSSetViewports(1, &viewport);

	get_current_resource_storage().command_list->RSSetScissorRects(1, &get_scissor(rsx::method_registers[NV4097_SET_SCISSOR_HORIZONTAL], rsx::method_registers[NV4097_SET_SCISSOR_VERTICAL]));

	get_current_resource_storage().command_list->IASetPrimitiveTopology(get_primitive_topology(draw_mode));

	if (m_rendering_info.m_indexed)
		get_current_resource_storage().command_list->DrawIndexedInstanced((UINT)m_rendering_info.m_count, 1, 0, 0, 0);
	else
		get_current_resource_storage().command_list->DrawInstanced((UINT)m_rendering_info.m_count, 1, 0, 0);

	vertex_index_array.clear();
	std::chrono::time_point<std::chrono::system_clock> end_duration = std::chrono::system_clock::now();
	m_timers.m_draw_calls_duration += std::chrono::duration_cast<std::chrono::microseconds>(end_duration - start_duration).count();
	m_timers.m_draw_calls_count++;

	if (rpcs3::config.rsx.d3d12.debug_output.value())
	{
		CHECK_HRESULT(get_current_resource_storage().command_list->Close());
		m_command_queue->ExecuteCommandLists(1, (ID3D12CommandList**)get_current_resource_storage().command_list.GetAddressOf());
		get_current_resource_storage().set_new_command_list();
	}
	m_first_count_pairs.clear();
	m_rendering_info.m_indexed = false;
	thread::end();
}

namespace
{
bool is_flip_surface_in_global_memory(u32 color_target)
{
	switch (color_target)
	{
	case CELL_GCM_SURFACE_TARGET_0:
	case CELL_GCM_SURFACE_TARGET_1:
	case CELL_GCM_SURFACE_TARGET_MRT1:
	case CELL_GCM_SURFACE_TARGET_MRT2:
	case CELL_GCM_SURFACE_TARGET_MRT3:
		return true;
	case CELL_GCM_SURFACE_TARGET_NONE:
		return false;
	}
	throw EXCEPTION("Wrong color_target (%u)", color_target);
}
}

void D3D12GSRender::flip(int buffer)
{
	ID3D12Resource *resource_to_flip;
	float viewport_w, viewport_h;

	if (!is_flip_surface_in_global_memory(rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET]))
	{
		resource_storage &storage = get_current_resource_storage();
		assert(storage.ram_framebuffer == nullptr);

		size_t w = 0, h = 0, row_pitch = 0;

		size_t offset = 0;
		if (false)
		{
			CellGcmDisplayInfo* buffers = nullptr;// = vm::ps3::_ptr<CellGcmDisplayInfo>(m_gcm_buffers_addr);
			u32 addr = rsx::get_address(gcm_buffers[gcm_current_buffer].offset, CELL_GCM_LOCATION_LOCAL);
			w = gcm_buffers[gcm_current_buffer].width;
			h = gcm_buffers[gcm_current_buffer].height;
			u8 *src_buffer = vm::ps3::_ptr<u8>(addr);

			row_pitch = align(w * 4, 256);
			size_t texture_size = row_pitch * h; // * 4 for mipmap levels
			assert(m_texture_upload_data.can_alloc(texture_size));
			size_t heap_offset = m_texture_upload_data.alloc(texture_size);

			void *buffer;
			CHECK_HRESULT(m_texture_upload_data.m_heap->Map(0, &CD3DX12_RANGE(heap_offset, heap_offset + texture_size), &buffer));
			void *mapped_buffer = (char*)buffer + heap_offset;
			for (unsigned row = 0; row < h; row++)
				memcpy((char*)mapped_buffer + row * row_pitch, (char*)src_buffer + row * w * 4, w * 4);
			m_texture_upload_data.m_heap->Unmap(0, &CD3DX12_RANGE(heap_offset, heap_offset + texture_size));
			offset = heap_offset;
		}

		CHECK_HRESULT(
			m_device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, (UINT)w, (UINT)h, 1, 1),
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(storage.ram_framebuffer.GetAddressOf())
				)
			);
		get_current_resource_storage().command_list->CopyTextureRegion(&CD3DX12_TEXTURE_COPY_LOCATION(storage.ram_framebuffer.Get(), 0), 0, 0, 0,
			&CD3DX12_TEXTURE_COPY_LOCATION(m_texture_upload_data.m_heap, { offset, { DXGI_FORMAT_R8G8B8A8_UNORM, (UINT)w, (UINT)h, 1, (UINT)row_pitch } }), nullptr);

		get_current_resource_storage().command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(storage.ram_framebuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
		resource_to_flip = storage.ram_framebuffer.Get();
		viewport_w = (float)w, viewport_h = (float)h;
	}
	else
	{
		if (m_rtts.bound_render_targets[0] != nullptr)
		{
			get_current_resource_storage().command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_rtts.bound_render_targets[0], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
			resource_to_flip = m_rtts.bound_render_targets[0];
		}
		else if (m_rtts.bound_render_targets[1] != nullptr)
		{
			get_current_resource_storage().command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_rtts.bound_render_targets[1], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
			resource_to_flip = m_rtts.bound_render_targets[1];
		}
		else
			resource_to_flip = nullptr;
	}

	get_current_resource_storage().command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_backbuffer[m_swap_chain->GetCurrentBackBufferIndex()].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	D3D12_VIEWPORT viewport =
	{
		0.f,
		0.f,
		(float)m_backbuffer[m_swap_chain->GetCurrentBackBufferIndex()]->GetDesc().Width,
		(float)m_backbuffer[m_swap_chain->GetCurrentBackBufferIndex()]->GetDesc().Height,
		0.f,
		1.f
	};
	get_current_resource_storage().command_list->RSSetViewports(1, &viewport);

	D3D12_RECT box =
	{
		0,
		0,
		(LONG)m_backbuffer[m_swap_chain->GetCurrentBackBufferIndex()]->GetDesc().Width,
		(LONG)m_backbuffer[m_swap_chain->GetCurrentBackBufferIndex()]->GetDesc().Height,
	};
	get_current_resource_storage().command_list->RSSetScissorRects(1, &box);
	get_current_resource_storage().command_list->SetGraphicsRootSignature(m_output_scaling_pass.m_rootSignature);
	get_current_resource_storage().command_list->SetPipelineState(m_output_scaling_pass.m_PSO);

	D3D12_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc = {};
	// FIXME: Not always true
	shader_resource_view_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	shader_resource_view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	shader_resource_view_desc.Texture2D.MipLevels = 1;
	if (is_flip_surface_in_global_memory(rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET]))
		shader_resource_view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	else
		shader_resource_view_desc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
			D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
			D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2,
			D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3,
			D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0
			);
	m_device->CreateShaderResourceView(resource_to_flip, &shader_resource_view_desc,
		CD3DX12_CPU_DESCRIPTOR_HANDLE(m_output_scaling_pass.m_textureDescriptorHeap->GetCPUDescriptorHandleForHeapStart()).Offset(m_swap_chain->GetCurrentBackBufferIndex(), g_descriptor_stride_srv_cbv_uav));

	D3D12_SAMPLER_DESC sampler_desc = {};
	sampler_desc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	m_device->CreateSampler(&sampler_desc,
		CD3DX12_CPU_DESCRIPTOR_HANDLE(m_output_scaling_pass.m_samplerDescriptorHeap->GetCPUDescriptorHandleForHeapStart()).Offset(m_swap_chain->GetCurrentBackBufferIndex(), g_descriptor_stride_samplers));

	ID3D12DescriptorHeap *descriptors_heaps[] =
	{
		m_output_scaling_pass.m_textureDescriptorHeap,
		m_output_scaling_pass.m_samplerDescriptorHeap
	};
	get_current_resource_storage().command_list->SetDescriptorHeaps(2, descriptors_heaps);
	get_current_resource_storage().command_list->SetGraphicsRootDescriptorTable(0,
		CD3DX12_GPU_DESCRIPTOR_HANDLE(m_output_scaling_pass.m_textureDescriptorHeap->GetGPUDescriptorHandleForHeapStart()).Offset(m_swap_chain->GetCurrentBackBufferIndex(), g_descriptor_stride_srv_cbv_uav));
	get_current_resource_storage().command_list->SetGraphicsRootDescriptorTable(1,
		CD3DX12_GPU_DESCRIPTOR_HANDLE(m_output_scaling_pass.m_samplerDescriptorHeap->GetGPUDescriptorHandleForHeapStart()).Offset(m_swap_chain->GetCurrentBackBufferIndex(), g_descriptor_stride_samplers));

	get_current_resource_storage().command_list->OMSetRenderTargets(1,
		&CD3DX12_CPU_DESCRIPTOR_HANDLE(m_backbuffer_descriptor_heap[m_swap_chain->GetCurrentBackBufferIndex()]->GetCPUDescriptorHandleForHeapStart()),
		true, nullptr);
	D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view = {};
	vertex_buffer_view.BufferLocation = m_output_scaling_pass.m_vertexBuffer->GetGPUVirtualAddress();
	vertex_buffer_view.StrideInBytes = 4 * sizeof(float);
	vertex_buffer_view.SizeInBytes = 16 * sizeof(float);
	get_current_resource_storage().command_list->IASetVertexBuffers(0, 1, &vertex_buffer_view);
	get_current_resource_storage().command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	if (resource_to_flip)
		get_current_resource_storage().command_list->DrawInstanced(4, 1, 0, 0);

	if (!rpcs3::config.rsx.d3d12.overlay.value())
		get_current_resource_storage().command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_backbuffer[m_swap_chain->GetCurrentBackBufferIndex()].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	if (is_flip_surface_in_global_memory(rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET]) && resource_to_flip != nullptr)
		get_current_resource_storage().command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(resource_to_flip, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
	CHECK_HRESULT(get_current_resource_storage().command_list->Close());
	m_command_queue->ExecuteCommandLists(1, (ID3D12CommandList**)get_current_resource_storage().command_list.GetAddressOf());

	if(rpcs3::config.rsx.d3d12.overlay.value())
		render_overlay();

	reset_timer();

	std::chrono::time_point<std::chrono::system_clock> flip_start = std::chrono::system_clock::now();

	CHECK_HRESULT(m_swap_chain->Present(rpcs3::state.config.rsx.vsync.value() ? 1 : 0, 0));
	// Add an event signaling queue completion

	resource_storage &storage = get_non_current_resource_storage();

	m_command_queue->Signal(storage.frame_finished_fence.Get(), storage.fence_value);
	storage.frame_finished_fence->SetEventOnCompletion(storage.fence_value, storage.frame_finished_handle);
	storage.fence_value++;

	storage.in_use = true;

	// Get the put pos - 1. This way after cleaning we can set the get ptr to
	// this value, allowing heap to proceed even if we cleant before allocating
	// a new value (that's the reason of the -1)
	storage.constants_heap_get_pos = m_constants_data.get_current_put_pos_minus_one();
	storage.vertex_index_heap_get_pos = m_vertex_index_data.get_current_put_pos_minus_one();
	storage.texture_upload_heap_get_pos = m_texture_upload_data.get_current_put_pos_minus_one();
	storage.readback_heap_get_pos = m_readback_resources.get_current_put_pos_minus_one();
	storage.uav_heap_get_pos = m_uav_heap.get_current_put_pos_minus_one();

	// Now get ready for next frame
	resource_storage &new_storage = get_current_resource_storage();

	new_storage.wait_and_clean();
	if (new_storage.in_use)
	{
		m_constants_data.m_get_pos = new_storage.constants_heap_get_pos;
		m_vertex_index_data.m_get_pos = new_storage.vertex_index_heap_get_pos;
		m_texture_upload_data.m_get_pos = new_storage.texture_upload_heap_get_pos;
		m_readback_resources.m_get_pos = new_storage.readback_heap_get_pos;
		m_uav_heap.m_get_pos = new_storage.uav_heap_get_pos;
	}

	m_frame->flip(nullptr);


	std::chrono::time_point<std::chrono::system_clock> flip_end = std::chrono::system_clock::now();
	m_timers.m_flip_duration += std::chrono::duration_cast<std::chrono::microseconds>(flip_end - flip_start).count();
}

void D3D12GSRender::reset_timer()
{
	m_timers.m_draw_calls_count = 0;
	m_timers.m_draw_calls_duration = 0;
	m_timers.m_prepare_rtt_duration = 0;
	m_timers.m_vertex_index_duration = 0;
	m_timers.m_buffer_upload_size = 0;
	m_timers.m_program_load_duration = 0;
	m_timers.m_constants_duration = 0;
	m_timers.m_texture_duration = 0;
	m_timers.m_flip_duration = 0;
}

resource_storage& D3D12GSRender::get_current_resource_storage()
{
	return m_per_frame_storage[m_swap_chain->GetCurrentBackBufferIndex()];
}

resource_storage& D3D12GSRender::get_non_current_resource_storage()
{
	return m_per_frame_storage[1 - m_swap_chain->GetCurrentBackBufferIndex()];
}
#endif
