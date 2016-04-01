#include "stdafx.h"
#include "stdafx_d3d12.h"
#ifdef _MSC_VER
#include "D3D12RenderTargetSets.h"
#include "Utilities/rPlatform.h" // only for rImage
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/state.h"
#include "Emu/RSX/GSRender.h"
#include "../rsx_methods.h"

#include <D3D12.h>
#include "D3D12GSRender.h"
#include "D3D12Formats.h"

namespace
{
	u32 get_max_depth_value(rsx::surface_depth_format format)
	{
		switch (format)
		{
		case rsx::surface_depth_format::z16: return 0xFFFF;
		case rsx::surface_depth_format::z24s8: return 0xFFFFFF;
		}
		throw EXCEPTION("Unknow depth format");
	}

	UINT get_num_rtt(rsx::surface_target color_target)
	{
		switch (color_target)
		{
		case rsx::surface_target::none: return 0;
		case rsx::surface_target::surface_a:
		case rsx::surface_target::surface_b: return 1;
		case rsx::surface_target::surfaces_a_b: return 2;
		case rsx::surface_target::surfaces_a_b_c: return 3;
		case rsx::surface_target::surfaces_a_b_c_d: return 4;
		}
		throw EXCEPTION("Wrong color_target (%d)", color_target);
	}

	std::vector<u8> get_rtt_indexes(rsx::surface_target color_target)
	{
		switch (color_target)
		{
		case rsx::surface_target::none: return{};
		case rsx::surface_target::surface_a: return{ 0 };
		case rsx::surface_target::surface_b: return{ 1 };
		case rsx::surface_target::surfaces_a_b: return{ 0, 1 };
		case rsx::surface_target::surfaces_a_b_c: return{ 0, 1, 2 };
		case rsx::surface_target::surfaces_a_b_c_d: return{ 0, 1, 2, 3 };
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

	size_t get_aligned_pitch(rsx::surface_color_format format, u32 width)
	{
		switch (format)
		{
		case rsx::surface_color_format::b8: return align(width, 256);
		case rsx::surface_color_format::g8b8:
		case rsx::surface_color_format::x1r5g5b5_o1r5g5b5:
		case rsx::surface_color_format::x1r5g5b5_z1r5g5b5:
		case rsx::surface_color_format::r5g6b5: return align(width * 2, 256);
		case rsx::surface_color_format::a8b8g8r8:
		case rsx::surface_color_format::x8b8g8r8_o8b8g8r8:
		case rsx::surface_color_format::x8b8g8r8_z8b8g8r8:
		case rsx::surface_color_format::x8r8g8b8_o8r8g8b8:
		case rsx::surface_color_format::x8r8g8b8_z8r8g8b8:
		case rsx::surface_color_format::x32:
		case rsx::surface_color_format::a8r8g8b8: return align(width * 4, 256);
		case rsx::surface_color_format::w16z16y16x16: return align(width * 8, 256);
		case rsx::surface_color_format::w32z32y32x32: return align(width * 16, 256);
		}
		throw EXCEPTION("Unknow color surface format");
	}

	size_t get_packed_pitch(rsx::surface_color_format format, u32 width)
	{
		switch (format)
		{
		case rsx::surface_color_format::b8: return width;
		case rsx::surface_color_format::g8b8:
		case rsx::surface_color_format::x1r5g5b5_o1r5g5b5:
		case rsx::surface_color_format::x1r5g5b5_z1r5g5b5:
		case rsx::surface_color_format::r5g6b5: return width * 2;
		case rsx::surface_color_format::a8b8g8r8:
		case rsx::surface_color_format::x8b8g8r8_o8b8g8r8:
		case rsx::surface_color_format::x8b8g8r8_z8b8g8r8:
		case rsx::surface_color_format::x8r8g8b8_o8r8g8b8:
		case rsx::surface_color_format::x8r8g8b8_z8r8g8b8:
		case rsx::surface_color_format::x32:
		case rsx::surface_color_format::a8r8g8b8: return width * 4;
		case rsx::surface_color_format::w16z16y16x16: return width * 8;
		case rsx::surface_color_format::w32z32y32x32: return width * 16;
		}
		throw EXCEPTION("Unknow color surface format");
	}
}

void D3D12GSRender::clear_surface(u32 arg)
{
	std::chrono::time_point<std::chrono::system_clock> start_duration = std::chrono::system_clock::now();

	std::chrono::time_point<std::chrono::system_clock> rtt_duration_start = std::chrono::system_clock::now();
	prepare_render_targets(get_current_resource_storage().command_list.Get());

	std::chrono::time_point<std::chrono::system_clock> rtt_duration_end = std::chrono::system_clock::now();
	m_timers.prepare_rtt_duration += std::chrono::duration_cast<std::chrono::microseconds>(rtt_duration_end - rtt_duration_start).count();

	if (arg & 0x1 || arg & 0x2)
	{
		get_current_resource_storage().depth_stencil_descriptor_heap_index++;

		if (arg & 0x1)
		{
			u32 clear_depth = rsx::method_registers[NV4097_SET_ZSTENCIL_CLEAR_VALUE] >> 8;
			u32 max_depth_value = get_max_depth_value(m_surface.depth_format);
			get_current_resource_storage().command_list->ClearDepthStencilView(m_rtts.current_ds_handle, D3D12_CLEAR_FLAG_DEPTH, clear_depth / (float)max_depth_value, 0,
				1, &get_scissor(rsx::method_registers[NV4097_SET_SCISSOR_HORIZONTAL], rsx::method_registers[NV4097_SET_SCISSOR_VERTICAL]));
		}

		if (arg & 0x2)
			get_current_resource_storage().command_list->ClearDepthStencilView(m_rtts.current_ds_handle, D3D12_CLEAR_FLAG_STENCIL, 0.f, get_clear_stencil(rsx::method_registers[NV4097_SET_ZSTENCIL_CLEAR_VALUE]),
				1, &get_scissor(rsx::method_registers[NV4097_SET_SCISSOR_HORIZONTAL], rsx::method_registers[NV4097_SET_SCISSOR_VERTICAL]));
	}

	if (arg & 0xF0)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtts.current_rtts_handle);
		size_t rtt_index = get_num_rtt(rsx::to_surface_target(rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET]));
		get_current_resource_storage().render_targets_descriptors_heap_index += rtt_index;
		for (unsigned i = 0; i < rtt_index; i++)
			get_current_resource_storage().command_list->ClearRenderTargetView(handle.Offset(i, m_descriptor_stride_rtv), get_clear_color(rsx::method_registers[NV4097_SET_COLOR_CLEAR_VALUE]).data(),
				1, &get_scissor(rsx::method_registers[NV4097_SET_SCISSOR_HORIZONTAL], rsx::method_registers[NV4097_SET_SCISSOR_VERTICAL]));
	}

	std::chrono::time_point<std::chrono::system_clock> end_duration = std::chrono::system_clock::now();
	m_timers.draw_calls_duration += std::chrono::duration_cast<std::chrono::microseconds>(end_duration - start_duration).count();
	m_timers.draw_calls_count++;

	if (rpcs3::config.rsx.d3d12.debug_output.value())
	{
		CHECK_HRESULT(get_current_resource_storage().command_list->Close());
		m_command_queue->ExecuteCommandLists(1, (ID3D12CommandList**)get_current_resource_storage().command_list.GetAddressOf());
		get_current_resource_storage().set_new_command_list();
	}
}

void D3D12GSRender::prepare_render_targets(ID3D12GraphicsCommandList *copycmdlist)
{
	// check if something has changed
	u32 surface_format = rsx::method_registers[NV4097_SET_SURFACE_FORMAT];

	// Exit early if there is no rtt changes
	if (!m_rtts_dirty)
		return;
	m_rtts_dirty = false;


	if (m_surface.format != surface_format)
		m_surface.unpack(surface_format);

	std::array<float, 4> clear_color = get_clear_color(rsx::method_registers[NV4097_SET_COLOR_CLEAR_VALUE]);
	m_rtts.prepare_render_target(copycmdlist,
		rsx::method_registers[NV4097_SET_SURFACE_FORMAT],
		rsx::method_registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL], rsx::method_registers[NV4097_SET_SURFACE_CLIP_VERTICAL],
		rsx::to_surface_target(rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET]),
		get_color_surface_addresses(), get_zeta_surface_address(),
		m_device.Get(), clear_color, 1.f, 0);

	// write descriptors
	DXGI_FORMAT dxgi_format = get_color_surface_format(m_surface.color_format);
	D3D12_RENDER_TARGET_VIEW_DESC rtt_view_desc = {};
	rtt_view_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtt_view_desc.Format = dxgi_format;

	m_rtts.current_rtts_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(get_current_resource_storage().render_targets_descriptors_heap->GetCPUDescriptorHandleForHeapStart())
		.Offset((INT)get_current_resource_storage().render_targets_descriptors_heap_index * m_descriptor_stride_rtv);
	size_t rtt_index = 0;
	for (u8 i : get_rtt_indexes(rsx::to_surface_target(rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET])))
	{
		if (std::get<1>(m_rtts.m_bound_render_targets[i]) == nullptr)
			continue;
		m_device->CreateRenderTargetView(std::get<1>(m_rtts.m_bound_render_targets[i]), &rtt_view_desc,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtts.current_rtts_handle).Offset((INT)rtt_index * m_descriptor_stride_rtv));
		rtt_index++;
	}
	get_current_resource_storage().render_targets_descriptors_heap_index += rtt_index;

	if (std::get<1>(m_rtts.m_bound_depth_stencil) == nullptr)
		return;
	m_rtts.current_ds_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(get_current_resource_storage().depth_stencil_descriptor_heap->GetCPUDescriptorHandleForHeapStart())
		.Offset((INT)get_current_resource_storage().depth_stencil_descriptor_heap_index * m_descriptor_stride_dsv);
	get_current_resource_storage().depth_stencil_descriptor_heap_index += 1;
	D3D12_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc = {};
	depth_stencil_view_desc.Format = get_depth_stencil_surface_format(m_surface.depth_format);
	depth_stencil_view_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	m_device->CreateDepthStencilView(std::get<1>(m_rtts.m_bound_depth_stencil), &depth_stencil_view_desc, m_rtts.current_ds_handle);
}

void D3D12GSRender::set_rtt_and_ds(ID3D12GraphicsCommandList *command_list)
{
	UINT num_rtt = get_num_rtt(rsx::to_surface_target(rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET]));
	D3D12_CPU_DESCRIPTOR_HANDLE* ds_handle = (std::get<1>(m_rtts.m_bound_depth_stencil) != nullptr) ? &m_rtts.current_ds_handle : nullptr;
	command_list->OMSetRenderTargets((UINT)num_rtt, &m_rtts.current_rtts_handle, true, ds_handle);
}

void rsx::render_targets::init(ID3D12Device *device)
{
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
		d3d12_data_heap &readback_heap,
		ID3D12Resource * color_surface,
		rsx::surface_color_format color_surface_format
		)
	{
		int clip_w = rsx::method_registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL] >> 16;
		int clip_h = rsx::method_registers[NV4097_SET_SURFACE_CLIP_VERTICAL] >> 16;

		DXGI_FORMAT dxgi_format = get_color_surface_format(color_surface_format);
		size_t row_pitch = get_aligned_pitch(color_surface_format, clip_w);

		size_t buffer_size = row_pitch * clip_h;
		size_t heap_offset = readback_heap.alloc<D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT>(buffer_size);

		command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(color_surface, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));

		command_list->CopyTextureRegion(&CD3DX12_TEXTURE_COPY_LOCATION(readback_heap.get_heap(), { heap_offset, { dxgi_format, (UINT)clip_w, (UINT)clip_h, 1, (UINT)row_pitch } }), 0, 0, 0,
			&CD3DX12_TEXTURE_COPY_LOCATION(color_surface, 0), nullptr);
		command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(color_surface, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
		return heap_offset;
	}

	void copy_readback_buffer_to_dest(void *dest, d3d12_data_heap &readback_heap, size_t offset_in_heap, size_t dst_pitch, size_t src_pitch, size_t height)
	{
		// TODO: Use exact range
		void *mapped_buffer = readback_heap.map<void>(offset_in_heap);
		for (unsigned row = 0; row < height; row++)
		{
			u32 *casted_dest = (u32*)((char*)dest + row * dst_pitch);
			u32 *casted_src = (u32*)((char*)mapped_buffer + row * src_pitch);
			for (unsigned col = 0; col < src_pitch / 4; col++)
				*casted_dest++ = se_storage<u32>::swap(*casted_src++);
		}
		readback_heap.unmap();
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

	size_t depth_row_pitch = align(clip_w * 4, 256);
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
		get_current_resource_storage().command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(std::get<1>(m_rtts.m_bound_depth_stencil), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COPY_SOURCE));
		get_current_resource_storage().command_list->CopyTextureRegion(&CD3DX12_TEXTURE_COPY_LOCATION(m_readback_resources.get_heap(), { depth_buffer_offset_in_heap,{ DXGI_FORMAT_R32_TYPELESS, (UINT)clip_w, (UINT)clip_h, 1, (UINT)depth_row_pitch } }), 0, 0, 0,
			&CD3DX12_TEXTURE_COPY_LOCATION(std::get<1>(m_rtts.m_bound_depth_stencil), 0), nullptr);
		get_current_resource_storage().command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(std::get<1>(m_rtts.m_bound_depth_stencil), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE));
		invalidate_address(address_z);

		need_transfer = true;
	}

	size_t color_buffer_offset_in_heap[4];
	if (rpcs3::state.config.rsx.opengl.write_color_buffers)
	{
		for (u8 i : get_rtt_indexes(rsx::to_surface_target(rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET])))
		{
			if (!address_color[i])
				continue;
			color_buffer_offset_in_heap[i] = download_to_readback_buffer(m_device.Get(), get_current_resource_storage().command_list.Get(), m_readback_resources, std::get<1>(m_rtts.m_bound_render_targets[i]), m_surface.color_format);
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
		u8 *mapped_buffer = m_readback_resources.map<u8>(depth_buffer_offset_in_heap);

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
		m_readback_resources.unmap();
	}

	if (rpcs3::state.config.rsx.opengl.write_color_buffers)
	{
		size_t srcPitch = get_aligned_pitch(m_surface.color_format, clip_w);
		size_t dstPitch = get_packed_pitch(m_surface.color_format, clip_w);

		void *dest_buffer[] =
		{
			vm::base(address_color[0]),
			vm::base(address_color[1]),
			vm::base(address_color[2]),
			vm::base(address_color[3]),
		};

		for (u8 i : get_rtt_indexes(rsx::to_surface_target(rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET])))
		{
			if (!address_color[i])
				continue;
			copy_readback_buffer_to_dest(dest_buffer[i], m_readback_resources, color_buffer_offset_in_heap[i], srcPitch, dstPitch, clip_h);
		}
	}
}


std::array<std::vector<gsl::byte>, 4> D3D12GSRender::copy_render_targets_to_memory()
{
	int clip_w = rsx::method_registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL] >> 16;
	int clip_h = rsx::method_registers[NV4097_SET_SURFACE_CLIP_VERTICAL] >> 16;
	rsx::surface_info surface = {};
	surface.unpack(rsx::method_registers[NV4097_SET_SURFACE_FORMAT]);
	return m_rtts.get_render_targets_data(surface.color_format, clip_w, clip_h, m_device.Get(), m_command_queue.Get(), m_readback_resources, get_current_resource_storage());
}

std::array<std::vector<gsl::byte>, 2> D3D12GSRender::copy_depth_stencil_buffer_to_memory()
{
	int clip_w = rsx::method_registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL] >> 16;
	int clip_h = rsx::method_registers[NV4097_SET_SURFACE_CLIP_VERTICAL] >> 16;
	rsx::surface_info surface = {};
	surface.unpack(rsx::method_registers[NV4097_SET_SURFACE_FORMAT]);
	return m_rtts.get_depth_stencil_data(surface.depth_format, clip_w, clip_h, m_device.Get(), m_command_queue.Get(), m_readback_resources, get_current_resource_storage());
}
#endif
