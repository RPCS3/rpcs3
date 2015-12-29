#include "stdafx.h"
#include "stdafx_d3d12.h"
#ifdef _MSC_VER
#include "D3D12RenderTargetSets.h"
#include "Utilities/rPlatform.h" // only for rImage
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/state.h"
#include "Emu/RSX/GSRender.h"

#include "D3D12.h"
#include "D3D12GSRender.h"
#include "D3D12Formats.h"


namespace
{
	UINT get_num_rtt(u8 color_target)
	{
		switch (color_target)
		{
		case CELL_GCM_SURFACE_TARGET_NONE: return 0;
		case CELL_GCM_SURFACE_TARGET_0:
		case CELL_GCM_SURFACE_TARGET_1: return 1;
		case CELL_GCM_SURFACE_TARGET_MRT1: return 2;
		case CELL_GCM_SURFACE_TARGET_MRT2: return 3;
		case CELL_GCM_SURFACE_TARGET_MRT3: return 4;
		}
		throw EXCEPTION("Wrong color_target (%d)", color_target);
	}

	std::vector<u8> get_rtt_indexes(u8 color_target)
	{
		switch (color_target)
		{
		case CELL_GCM_SURFACE_TARGET_NONE: return{};
		case CELL_GCM_SURFACE_TARGET_0: return{ 0 };
		case CELL_GCM_SURFACE_TARGET_1: return{ 1 };
		case CELL_GCM_SURFACE_TARGET_MRT1: return{ 0, 1 };
		case CELL_GCM_SURFACE_TARGET_MRT2: return{ 0, 1, 2 };
		case CELL_GCM_SURFACE_TARGET_MRT3: return{ 0, 1, 2, 3 };
		}
		throw EXCEPTION("Wrong color_target (%d)", color_target);
	}

	std::array<float, 4> get_clear_color(u32 clear_color)
	{
		u8 clear_a = clear_color >> 24;
		u8 clear_r = clear_color >> 16;
		u8 clear_g = clear_color >> 8;
		u8 clear_b = clear_color;
		return
		{
			clear_r / 255.0f,
			clear_g / 255.0f,
			clear_b / 255.0f,
			clear_a / 255.0f
		};
	}

	u8 get_clear_stencil(u32 register_value)
	{
		return register_value & 0xff;
	}
}

void D3D12GSRender::clear_surface(u32 arg)
{
	std::chrono::time_point<std::chrono::system_clock> start_duration = std::chrono::system_clock::now();

	std::chrono::time_point<std::chrono::system_clock> rtt_duration_start = std::chrono::system_clock::now();
	prepare_render_targets(get_current_resource_storage().command_list.Get());

	std::chrono::time_point<std::chrono::system_clock> rtt_duration_end = std::chrono::system_clock::now();
	m_timers.m_prepare_rtt_duration += std::chrono::duration_cast<std::chrono::microseconds>(rtt_duration_end - rtt_duration_start).count();

	if (arg & 0x1 || arg & 0x2)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(get_current_resource_storage().depth_stencil_descriptor_heap->GetCPUDescriptorHandleForHeapStart())
			.Offset((INT)get_current_resource_storage().depth_stencil_descriptor_heap_index * g_descriptor_stride_rtv);
		m_rtts.bind_depth_stencil(m_device.Get(), m_surface.depth_format, handle);
		get_current_resource_storage().depth_stencil_descriptor_heap_index++;

		if (arg & 0x1)
		{
			u32 clear_depth = rsx::method_registers[NV4097_SET_ZSTENCIL_CLEAR_VALUE] >> 8;
			u32 max_depth_value = m_surface.depth_format == CELL_GCM_SURFACE_Z16 ? 0x0000ffff : 0x00ffffff;
			get_current_resource_storage().command_list->ClearDepthStencilView(handle, D3D12_CLEAR_FLAG_DEPTH, clear_depth / (float)max_depth_value, 0,
				1, &get_scissor(rsx::method_registers[NV4097_SET_SCISSOR_HORIZONTAL], rsx::method_registers[NV4097_SET_SCISSOR_VERTICAL]));
		}

		if (arg & 0x2)
			get_current_resource_storage().command_list->ClearDepthStencilView(handle, D3D12_CLEAR_FLAG_STENCIL, 0.f, get_clear_stencil(rsx::method_registers[NV4097_SET_ZSTENCIL_CLEAR_VALUE]),
				1, &get_scissor(rsx::method_registers[NV4097_SET_SCISSOR_HORIZONTAL], rsx::method_registers[NV4097_SET_SCISSOR_VERTICAL]));
	}

	if (arg & 0xF0)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(get_current_resource_storage().render_targets_descriptors_heap->GetCPUDescriptorHandleForHeapStart())
			.Offset((INT)get_current_resource_storage().render_targets_descriptors_heap_index * g_descriptor_stride_rtv);
		size_t rtt_index = m_rtts.bind_render_targets(m_device.Get(), m_surface.color_format, handle);
		get_current_resource_storage().render_targets_descriptors_heap_index += rtt_index;
		for (unsigned i = 0; i < rtt_index; i++)
			get_current_resource_storage().command_list->ClearRenderTargetView(handle.Offset(i, g_descriptor_stride_rtv), get_clear_color(rsx::method_registers[NV4097_SET_COLOR_CLEAR_VALUE]).data(),
				1, &get_scissor(rsx::method_registers[NV4097_SET_SCISSOR_HORIZONTAL], rsx::method_registers[NV4097_SET_SCISSOR_VERTICAL]));
	}

	std::chrono::time_point<std::chrono::system_clock> end_duration = std::chrono::system_clock::now();
	m_timers.m_draw_calls_duration += std::chrono::duration_cast<std::chrono::microseconds>(end_duration - start_duration).count();
	m_timers.m_draw_calls_count++;

	if (rpcs3::config.rsx.d3d12.debug_output.value())
	{
		CHECK_HRESULT(get_current_resource_storage().command_list->Close());
		m_command_queue->ExecuteCommandLists(1, (ID3D12CommandList**)get_current_resource_storage().command_list.GetAddressOf());
		get_current_resource_storage().set_new_command_list();
	}
}

void D3D12GSRender::prepare_render_targets(ID3D12GraphicsCommandList *copycmdlist)
{
	u32 surface_format = rsx::method_registers[NV4097_SET_SURFACE_FORMAT];

	u32 clip_horizontal = rsx::method_registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL];
	u32 clip_vertical = rsx::method_registers[NV4097_SET_SURFACE_CLIP_VERTICAL];

	u32 clip_width = clip_horizontal >> 16;
	u32 clip_height = clip_vertical >> 16;
	u32 clip_x = clip_horizontal;
	u32 clip_y = clip_vertical;

	u32 context_dma_color[] =
	{
		rsx::method_registers[NV4097_SET_CONTEXT_DMA_COLOR_A],
		rsx::method_registers[NV4097_SET_CONTEXT_DMA_COLOR_B],
		rsx::method_registers[NV4097_SET_CONTEXT_DMA_COLOR_C],
		rsx::method_registers[NV4097_SET_CONTEXT_DMA_COLOR_D]
	};
	u32 m_context_dma_z = rsx::method_registers[NV4097_SET_CONTEXT_DMA_ZETA];

	u32 offset_color[] =
	{
		rsx::method_registers[NV4097_SET_SURFACE_COLOR_AOFFSET],
		rsx::method_registers[NV4097_SET_SURFACE_COLOR_BOFFSET],
		rsx::method_registers[NV4097_SET_SURFACE_COLOR_COFFSET],
		rsx::method_registers[NV4097_SET_SURFACE_COLOR_DOFFSET]
	};
	u32 offset_zeta = rsx::method_registers[NV4097_SET_SURFACE_ZETA_OFFSET];

	// FBO location has changed, previous data might be copied
	u32 address_color[] =
	{
		rsx::get_address(offset_color[0], context_dma_color[0]),
		rsx::get_address(offset_color[1], context_dma_color[1]),
		rsx::get_address(offset_color[2], context_dma_color[2]),
		rsx::get_address(offset_color[3], context_dma_color[3]),
	};
	u32 address_z = rsx::get_address(offset_zeta, m_context_dma_z);

	// Exit early if there is no rtt changes
	if (m_previous_address_a == address_color[0] &&
		m_previous_address_b == address_color[1] &&
		m_previous_address_c == address_color[2] &&
		m_previous_address_d == address_color[3] &&
		m_previous_address_z == address_z &&
		m_surface.format == surface_format)
		return;

	m_previous_address_a = address_color[0];
	m_previous_address_b = address_color[1];
	m_previous_address_c = address_color[2];
	m_previous_address_d = address_color[3];
	m_previous_address_z = address_z;

	if (m_surface.format != surface_format)
	{
		m_surface.unpack(surface_format);
		m_surface.width = clip_width;
		m_surface.height = clip_height;
	}

	// Make previous RTTs sampleable
	for (unsigned i = 0; i < 4; i++)
	{
		if (m_rtts.bound_render_targets[i] == nullptr)
			continue;
		copycmdlist->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_rtts.bound_render_targets[i], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
	}
	// Reset bound data
	memset(m_rtts.bound_render_targets_address, 0, 4 * sizeof(u32));
	memset(m_rtts.bound_render_targets, 0, 4 * sizeof(ID3D12Resource *));


	// Create/Reuse requested rtts
	std::array<float, 4> clear_color = get_clear_color(rsx::method_registers[NV4097_SET_COLOR_CLEAR_VALUE]);
	for (u8 i : get_rtt_indexes(rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET]))
	{
		ComPtr<ID3D12Resource> old_render_target_resource;
		m_rtts.bound_render_targets[i] = m_rtts.bind_address_as_render_targets(m_device.Get(), copycmdlist, address_color[i], clip_width, clip_height, m_surface.color_format,
			clear_color, old_render_target_resource);
		if (old_render_target_resource)
			get_current_resource_storage().dirty_textures.push_back(old_render_target_resource);
		m_rtts.bound_render_targets_address[i] = address_color[i];
	}

	// Same for depth buffer
	if (m_rtts.bound_depth_stencil != nullptr)
		copycmdlist->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_rtts.bound_depth_stencil, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
	m_rtts.bound_depth_stencil = nullptr;
	m_rtts.bound_depth_stencil_address = 0;
	if (!address_z)
		return;
	ComPtr<ID3D12Resource> old_depth_stencil_resource;
	ID3D12Resource *ds = m_rtts.bind_address_as_depth_stencil(m_device.Get(), copycmdlist, address_z, clip_width, clip_height, m_surface.depth_format, 1., 0, old_depth_stencil_resource);
	if (old_depth_stencil_resource)
		get_current_resource_storage().dirty_textures.push_back(old_depth_stencil_resource);
	m_rtts.bound_depth_stencil_address = address_z;
	m_rtts.bound_depth_stencil = ds;
}

size_t render_targets::bind_render_targets(ID3D12Device *device, u32 color_format, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	DXGI_FORMAT dxgi_format = get_color_surface_format(color_format);
	D3D12_RENDER_TARGET_VIEW_DESC rtt_view_desc = {};
	rtt_view_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtt_view_desc.Format = dxgi_format;

	size_t rtt_index = 0;
	for (u8 i : get_rtt_indexes(rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET]))
	{
		if (bound_render_targets[i] == nullptr)
			continue;
		device->CreateRenderTargetView(bound_render_targets[i], &rtt_view_desc,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(handle).Offset((INT)rtt_index * g_descriptor_stride_rtv));
		rtt_index++;
	}
	return rtt_index;
}

size_t render_targets::bind_depth_stencil(ID3D12Device *device, u32 depth_format, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	if (!bound_depth_stencil)
		return 0;
	D3D12_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc = {};
	depth_stencil_view_desc.Format = get_depth_stencil_surface_format(depth_format);
	depth_stencil_view_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	device->CreateDepthStencilView(bound_depth_stencil, &depth_stencil_view_desc, handle);
	return 1;
}

void D3D12GSRender::set_rtt_and_ds(ID3D12GraphicsCommandList *command_list)
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(get_current_resource_storage().render_targets_descriptors_heap->GetCPUDescriptorHandleForHeapStart())
		.Offset((INT)get_current_resource_storage().render_targets_descriptors_heap_index * g_descriptor_stride_rtv);
	size_t num_rtt = m_rtts.bind_render_targets(m_device.Get(), m_surface.color_format, handle);
	get_current_resource_storage().render_targets_descriptors_heap_index += num_rtt;
	CD3DX12_CPU_DESCRIPTOR_HANDLE depth_stencil_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(get_current_resource_storage().depth_stencil_descriptor_heap->GetCPUDescriptorHandleForHeapStart())
		.Offset((INT)get_current_resource_storage().depth_stencil_descriptor_heap_index * g_descriptor_stride_rtv);
	size_t num_ds = m_rtts.bind_depth_stencil(m_device.Get(), m_surface.depth_format, depth_stencil_handle);
	get_current_resource_storage().depth_stencil_descriptor_heap_index += num_ds;
	command_list->OMSetRenderTargets((UINT)num_rtt, num_rtt > 0 ? &handle : nullptr, !!num_rtt,
		num_ds > 0 ? &depth_stencil_handle : nullptr);
}

ID3D12Resource *render_targets::bind_address_as_render_targets(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList, u32 address,
	size_t width, size_t height, u8 surfaceColorFormat, const std::array<float, 4> &clear_color, ComPtr<ID3D12Resource> &dirtyRTT)
{
	DXGI_FORMAT dxgi_format = get_color_surface_format(surfaceColorFormat);
	auto It = render_targets_storage.find(address);
	// TODO: Check if format and size match
	if (It != render_targets_storage.end())
	{
		ComPtr<ID3D12Resource> rtt;
		rtt = It->second.Get();
		if (rtt->GetDesc().Format == dxgi_format && rtt->GetDesc().Width == width && rtt->GetDesc().Height == height)
		{
			cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rtt.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
			return rtt.Get();
		}
		render_targets_storage.erase(address);
		dirtyRTT = rtt;
	}
	ComPtr<ID3D12Resource> rtt;
		LOG_WARNING(RSX, "Creating RTT");

	D3D12_CLEAR_VALUE clear_color_value = {};
	clear_color_value.Format = dxgi_format;
	clear_color_value.Color[0] = clear_color[0];
	clear_color_value.Color[1] = clear_color[1];
	clear_color_value.Color[2] = clear_color[2];
	clear_color_value.Color[3] = clear_color[3];

	device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(dxgi_format, (UINT)width, (UINT)height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		&clear_color_value,
		IID_PPV_ARGS(rtt.GetAddressOf())
		);
	render_targets_storage[address] = rtt;
	std::wstring name = L"rtt_@" + std::to_wstring(address);
	rtt->SetName(name.c_str());

	return rtt.Get();
}

ID3D12Resource * render_targets::bind_address_as_depth_stencil(ID3D12Device * device, ID3D12GraphicsCommandList * cmdList, u32 address, size_t width, size_t height, u8 surfaceDepthFormat, float depthClear, u8 stencilClear, ComPtr<ID3D12Resource> &dirtyDS)
{
	auto It = depth_stencil_storage.find(address);

	// TODO: Check if surface depth format match

	if (It != depth_stencil_storage.end())
	{
		ComPtr<ID3D12Resource> ds = It->second;
		if (ds->GetDesc().Width == width && ds->GetDesc().Height == height)
		{
			// set the resource as depth write
			cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(ds.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));
			return ds.Get();
		}
		// If size doesn't match, remove ds from cache
		depth_stencil_storage.erase(address);
		dirtyDS = ds;
	}

	D3D12_CLEAR_VALUE clear_depth_value = {};
	clear_depth_value.DepthStencil.Depth = depthClear;

	DXGI_FORMAT dxgi_format = get_depth_stencil_typeless_surface_format(surfaceDepthFormat);
	clear_depth_value.Format = get_depth_stencil_surface_clear_format(surfaceDepthFormat);

	ComPtr<ID3D12Resource> new_depth_stencil;
	device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(dxgi_format, (UINT)width, (UINT)height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clear_depth_value,
		IID_PPV_ARGS(new_depth_stencil.GetAddressOf())
		);
	depth_stencil_storage[address] = new_depth_stencil;
	std::wstring name = L"ds_@" + std::to_wstring(address);
	new_depth_stencil->SetName(name.c_str());

	return new_depth_stencil.Get();
}

void render_targets::init(ID3D12Device *device)//, u8 surfaceDepthFormat, size_t width, size_t height, float clearColor[4], float clearDepth)
{
	memset(bound_render_targets_address, 0, 4 * sizeof(u32));
	memset(bound_render_targets, 0, 4 * sizeof(ID3D12Resource*));
	bound_depth_stencil = nullptr;
	bound_depth_stencil_address = 0;
	g_descriptor_stride_rtv = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
}


namespace
{
	/**
	 * Populate command_list with copy command with color_surface data and return offset in readback buffer
	 */
	size_t download_to_readback_buffer(
		ID3D12Device *device,
		ID3D12GraphicsCommandList * command_list,
		data_heap<ID3D12Resource, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT> &readback_heap,
		ID3D12Resource * color_surface,
		int color_surface_format
		)
	{
		int clip_w = rsx::method_registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL] >> 16;
		int clip_h = rsx::method_registers[NV4097_SET_SURFACE_CLIP_VERTICAL] >> 16;

		DXGI_FORMAT dxgi_format = get_color_surface_format(color_surface_format);
		size_t row_pitch;
		switch (color_surface_format)
		{
		case CELL_GCM_SURFACE_R5G6B5:
			row_pitch = align(clip_w * 2, 256);
			break;
		case CELL_GCM_SURFACE_A8R8G8B8:
			row_pitch = align(clip_w * 4, 256);
			break;
		case CELL_GCM_SURFACE_F_W16Z16Y16X16:
			row_pitch = align(clip_w * 8, 256);
			break;
		}

		size_t buffer_size = row_pitch * clip_h;
		assert(readback_heap.can_alloc(buffer_size));
		size_t heap_offset = readback_heap.alloc(buffer_size);

		command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(color_surface, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));

		command_list->CopyTextureRegion(&CD3DX12_TEXTURE_COPY_LOCATION(readback_heap.m_heap, { heap_offset, { dxgi_format, (UINT)clip_w, (UINT)clip_h, 1, (UINT)row_pitch } }), 0, 0, 0,
			&CD3DX12_TEXTURE_COPY_LOCATION(color_surface, 0), nullptr);
		command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(color_surface, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
		return heap_offset;
	}

	void copy_readback_buffer_to_dest(void *dest, data_heap<ID3D12Resource, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT> &readback_heap, size_t offset_in_heap, size_t dst_pitch, size_t src_pitch, size_t height)
	{
		void *buffer;
		// TODO: Use exact range
		CHECK_HRESULT(readback_heap.m_heap->Map(0, nullptr, &buffer));
		void *mapped_buffer = (char*)buffer + offset_in_heap;
		for (unsigned row = 0; row < height; row++)
		{
			u32 *casted_dest = (u32*)((char*)dest + row * dst_pitch);
			u32 *casted_src = (u32*)((char*)mapped_buffer + row * src_pitch);
			for (unsigned col = 0; col < src_pitch / 4; col++)
				*casted_dest++ = se_storage<u32>::swap(*casted_src++);
		}
		readback_heap.m_heap->Unmap(0, nullptr);
	}

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

void D3D12GSRender::copy_render_target_to_dma_location()
{
	// Add all buffer write
	// Cell can't make any assumption about readyness of color/depth buffer
	// Except when a semaphore is written by RSX
	int clip_w = rsx::method_registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL] >> 16;
	int clip_h = rsx::method_registers[NV4097_SET_SURFACE_CLIP_VERTICAL] >> 16;

	ComPtr<ID3D12Resource> depth_format_conversion_buffer;
	ComPtr<ID3D12DescriptorHeap> descriptor_heap;
	size_t depth_row_pitch = align(clip_w, 256);
	size_t depth_buffer_offset_in_heap = 0;

	u32 context_dma_color[] =
	{
		rsx::method_registers[NV4097_SET_CONTEXT_DMA_COLOR_A],
		rsx::method_registers[NV4097_SET_CONTEXT_DMA_COLOR_B],
		rsx::method_registers[NV4097_SET_CONTEXT_DMA_COLOR_C],
		rsx::method_registers[NV4097_SET_CONTEXT_DMA_COLOR_D],
	};
	u32 m_context_dma_z = rsx::method_registers[NV4097_SET_CONTEXT_DMA_ZETA];

	u32 offset_color[] =
	{
		rsx::method_registers[NV4097_SET_SURFACE_COLOR_AOFFSET],
		rsx::method_registers[NV4097_SET_SURFACE_COLOR_BOFFSET],
		rsx::method_registers[NV4097_SET_SURFACE_COLOR_COFFSET],
		rsx::method_registers[NV4097_SET_SURFACE_COLOR_DOFFSET]
	};
	u32 offset_zeta = rsx::method_registers[NV4097_SET_SURFACE_ZETA_OFFSET];

	u32 address_color[] =
	{
		rsx::get_address(offset_color[0], context_dma_color[0]),
		rsx::get_address(offset_color[1], context_dma_color[1]),
		rsx::get_address(offset_color[2], context_dma_color[2]),
		rsx::get_address(offset_color[3], context_dma_color[3]),
	};
	u32 address_z = rsx::get_address(offset_zeta, m_context_dma_z);

	bool need_transfer = false;

	if (m_context_dma_z && rpcs3::state.config.rsx.opengl.write_depth_buffer)
	{
		size_t uav_size = clip_w * clip_h * 2;
		assert(m_uav_heap.can_alloc(uav_size));
		size_t heap_offset = m_uav_heap.alloc(uav_size);

		CHECK_HRESULT(
			m_device->CreatePlacedResource(
				m_uav_heap.m_heap,
				heap_offset,
				&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8_UNORM, clip_w, clip_h, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				nullptr,
				IID_PPV_ARGS(depth_format_conversion_buffer.GetAddressOf())
				)
			);

		D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV , 2, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE };
		CHECK_HRESULT(
			m_device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(descriptor_heap.GetAddressOf()))
			);
		D3D12_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc = {};
		shader_resource_view_desc.Format = get_depth_samplable_surface_format(m_surface.depth_format);
		shader_resource_view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		shader_resource_view_desc.Texture2D.MipLevels = 1;
		shader_resource_view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		m_device->CreateShaderResourceView(m_rtts.bound_depth_stencil, &shader_resource_view_desc,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(descriptor_heap->GetCPUDescriptorHandleForHeapStart()));
		D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
		uav_desc.Format = DXGI_FORMAT_R8_UNORM;
		uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		m_device->CreateUnorderedAccessView(depth_format_conversion_buffer.Get(), nullptr, &uav_desc,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(descriptor_heap->GetCPUDescriptorHandleForHeapStart()).Offset(1, g_descriptor_stride_srv_cbv_uav));

		// Convert
		get_current_resource_storage().command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_rtts.bound_depth_stencil, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));

		get_current_resource_storage().command_list->SetPipelineState(m_convertPSO);
		get_current_resource_storage().command_list->SetComputeRootSignature(m_convertRootSignature);
		get_current_resource_storage().command_list->SetDescriptorHeaps(1, descriptor_heap.GetAddressOf());
		get_current_resource_storage().command_list->SetComputeRootDescriptorTable(0, descriptor_heap->GetGPUDescriptorHandleForHeapStart());
		get_current_resource_storage().command_list->Dispatch(clip_w / 8, clip_h / 8, 1);

		D3D12_RESOURCE_BARRIER barriers[] =
		{
			CD3DX12_RESOURCE_BARRIER::Transition(m_rtts.bound_depth_stencil, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE),
			CD3DX12_RESOURCE_BARRIER::UAV(depth_format_conversion_buffer.Get()),
		};
		get_current_resource_storage().command_list->ResourceBarrier(2, barriers);
		get_current_resource_storage().command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(depth_format_conversion_buffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));
		get_current_resource_storage().command_list->CopyTextureRegion(&CD3DX12_TEXTURE_COPY_LOCATION(m_readback_resources.m_heap, { depth_buffer_offset_in_heap,{ DXGI_FORMAT_R8_UNORM, (UINT)clip_w, (UINT)clip_h, 1, (UINT)depth_row_pitch } }), 0, 0, 0,
			&CD3DX12_TEXTURE_COPY_LOCATION(depth_format_conversion_buffer.Get(), 0), nullptr);

		invalidate_address(address_z);

		need_transfer = true;
	}

	size_t color_buffer_offset_in_heap[4];
	if (rpcs3::state.config.rsx.opengl.write_color_buffers)
	{
		for (u8 i : get_rtt_indexes(rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET]))
		{
			if (!address_color[i])
				continue;
			color_buffer_offset_in_heap[i] = download_to_readback_buffer(m_device.Get(), get_current_resource_storage().command_list.Get(), m_readback_resources, m_rtts.bound_render_targets[i], m_surface.color_format);
			invalidate_address(address_color[i]);
			need_transfer = true;
		}
	}
	if (need_transfer)
	{
		CHECK_HRESULT(get_current_resource_storage().command_list->Close());
		m_command_queue->ExecuteCommandLists(1, (ID3D12CommandList**)get_current_resource_storage().command_list.GetAddressOf());
		get_current_resource_storage().set_new_command_list();
	}

	//Wait for result
	wait_for_command_queue(m_device.Get(), m_command_queue.Get());

	if (address_z && rpcs3::state.config.rsx.opengl.write_depth_buffer)
	{
		auto ptr = vm::base(address_z);
		char *depth_buffer = (char*)ptr;
		void *buffer;
		// TODO: Use exact range
		CHECK_HRESULT(m_readback_resources.m_heap->Map(0, nullptr, &buffer));
		unsigned char *mapped_buffer = (unsigned char*)buffer + depth_buffer_offset_in_heap;

		for (unsigned row = 0; row < (unsigned)clip_h; row++)
		{
			for (unsigned i = 0; i < (unsigned)clip_w; i++)
			{
				unsigned char c = mapped_buffer[row * depth_row_pitch + i];
				depth_buffer[4 * (row * clip_w + i)] = c;
				depth_buffer[4 * (row * clip_w + i) + 1] = c;
				depth_buffer[4 * (row * clip_w + i) + 2] = c;
				depth_buffer[4 * (row * clip_w + i) + 3] = c;
			}
		}
		m_readback_resources.m_heap->Unmap(0, nullptr);
	}

	size_t srcPitch, dstPitch;
	switch (m_surface.color_format)
	{
	case CELL_GCM_SURFACE_R5G6B5:
		srcPitch = align(clip_w * 2, 256);
		dstPitch = clip_w * 2;
		break;
	case CELL_GCM_SURFACE_A8R8G8B8:
		srcPitch = align(clip_w * 4, 256);
		dstPitch = clip_w * 4;
		break;
	case CELL_GCM_SURFACE_F_W16Z16Y16X16:
		srcPitch = align(clip_w * 8, 256);
		dstPitch = clip_w * 8;
		break;
	}

	if (rpcs3::state.config.rsx.opengl.write_color_buffers)
	{
		void *dest_buffer[] =
		{
			vm::base(address_color[0]),
			vm::base(address_color[1]),
			vm::base(address_color[2]),
			vm::base(address_color[3]),
		};

		for (u8 i : get_rtt_indexes(rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET]))
		{
			if (!address_color[i])
				continue;
			copy_readback_buffer_to_dest(dest_buffer[i], m_readback_resources, color_buffer_offset_in_heap[i], srcPitch, dstPitch, clip_h);
		}
	}
}


void D3D12GSRender::copy_render_targets_to_memory(void *buffer, u8 rtt)
{
	size_t heap_offset = download_to_readback_buffer(m_device.Get(), get_current_resource_storage().command_list.Get(), m_readback_resources, m_rtts.bound_render_targets[rtt], m_surface.color_format);

	CHECK_HRESULT(get_current_resource_storage().command_list->Close());
	m_command_queue->ExecuteCommandLists(1, (ID3D12CommandList**)get_current_resource_storage().command_list.GetAddressOf());
	get_current_resource_storage().set_new_command_list();

	wait_for_command_queue(m_device.Get(), m_command_queue.Get());
	m_readback_resources.m_get_pos = m_readback_resources.get_current_put_pos_minus_one();

	int clip_w = rsx::method_registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL] >> 16;
	int clip_h = rsx::method_registers[NV4097_SET_SURFACE_CLIP_VERTICAL] >> 16;
	size_t srcPitch, dstPitch;
	switch (m_surface.color_format)
	{
	case CELL_GCM_SURFACE_R5G6B5:
		srcPitch = align(clip_w * 2, 256);
		dstPitch = clip_w * 2;
		break;
	case CELL_GCM_SURFACE_A8R8G8B8:
		srcPitch = align(clip_w * 4, 256);
		dstPitch = clip_w * 4;
		break;
	case CELL_GCM_SURFACE_F_W16Z16Y16X16:
		srcPitch = align(clip_w * 8, 256);
		dstPitch = clip_w * 8;
		break;
	}
	copy_readback_buffer_to_dest(buffer, m_readback_resources, heap_offset, srcPitch, dstPitch, clip_h);
}

void D3D12GSRender::copy_depth_buffer_to_memory(void *buffer)
{
	unsigned clip_w = rsx::method_registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL] >> 16;
	unsigned clip_h = rsx::method_registers[NV4097_SET_SURFACE_CLIP_VERTICAL] >> 16;

	size_t row_pitch = align(clip_w * 4, 256);

	size_t buffer_size = row_pitch * clip_h;
	assert(m_readback_resources.can_alloc(buffer_size));
	size_t heap_offset = m_readback_resources.alloc(buffer_size);

	get_current_resource_storage().command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_rtts.bound_depth_stencil, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COPY_SOURCE));

	get_current_resource_storage().command_list->CopyTextureRegion(&CD3DX12_TEXTURE_COPY_LOCATION(m_readback_resources.m_heap, { heap_offset,{ DXGI_FORMAT_R32_TYPELESS, (UINT)clip_w, (UINT)clip_h, 1, (UINT)row_pitch } }), 0, 0, 0,
		&CD3DX12_TEXTURE_COPY_LOCATION(m_rtts.bound_depth_stencil, 0), nullptr);
	get_current_resource_storage().command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_rtts.bound_depth_stencil, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	CHECK_HRESULT(get_current_resource_storage().command_list->Close());
	m_command_queue->ExecuteCommandLists(1, (ID3D12CommandList**)get_current_resource_storage().command_list.GetAddressOf());
	get_current_resource_storage().set_new_command_list();

	wait_for_command_queue(m_device.Get(), m_command_queue.Get());
	m_readback_resources.m_get_pos = m_readback_resources.get_current_put_pos_minus_one();

	void *temp_buffer;
	CHECK_HRESULT(m_readback_resources.m_heap->Map(0, nullptr, &temp_buffer));
	void *mapped_buffer = (char*)temp_buffer + heap_offset;
	for (unsigned row = 0; row < clip_h; row++)
	{
		u32 *casted_dest = (u32*)((char*)buffer + row * clip_w * 4);
		u32 *casted_src = (u32*)((char*)mapped_buffer + row * row_pitch);
		for (unsigned col = 0; col < row_pitch / 4; col++)
			*casted_dest++ = *casted_src++;
	}
	m_readback_resources.m_heap->Unmap(0, nullptr);
}


void D3D12GSRender::copy_stencil_buffer_to_memory(void *buffer)
{
	unsigned clip_w = rsx::method_registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL] >> 16;
	unsigned clip_h = rsx::method_registers[NV4097_SET_SURFACE_CLIP_VERTICAL] >> 16;

	size_t row_pitch = align(clip_w * 4, 256);

	size_t buffer_size = row_pitch * clip_h;
	assert(m_readback_resources.can_alloc(buffer_size));
	size_t heap_offset = m_readback_resources.alloc(buffer_size);

	get_current_resource_storage().command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_rtts.bound_depth_stencil, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COPY_SOURCE));

	get_current_resource_storage().command_list->CopyTextureRegion(&CD3DX12_TEXTURE_COPY_LOCATION(m_readback_resources.m_heap, { heap_offset, { DXGI_FORMAT_R8_TYPELESS, (UINT)clip_w, (UINT)clip_h, 1, (UINT)row_pitch } }), 0, 0, 0,
		&CD3DX12_TEXTURE_COPY_LOCATION(m_rtts.bound_depth_stencil, 1), nullptr);
	get_current_resource_storage().command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_rtts.bound_depth_stencil, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	CHECK_HRESULT(get_current_resource_storage().command_list->Close());
	m_command_queue->ExecuteCommandLists(1, (ID3D12CommandList**)get_current_resource_storage().command_list.GetAddressOf());
	get_current_resource_storage().set_new_command_list();

	wait_for_command_queue(m_device.Get(), m_command_queue.Get());
	m_readback_resources.m_get_pos = m_readback_resources.get_current_put_pos_minus_one();

	void *temp_buffer;
	CHECK_HRESULT(m_readback_resources.m_heap->Map(0, nullptr, &temp_buffer));
	void *mapped_buffer = (char*)temp_buffer + heap_offset;
	for (unsigned row = 0; row < clip_h; row++)
	{
		char *casted_dest = (char*)buffer + row * clip_w;
		char *casted_src = (char*)mapped_buffer + row * row_pitch;
		for (unsigned col = 0; col < row_pitch; col++)
			*casted_dest++ = *casted_src++;
	}
	m_readback_resources.m_heap->Unmap(0, nullptr);
}

#endif
