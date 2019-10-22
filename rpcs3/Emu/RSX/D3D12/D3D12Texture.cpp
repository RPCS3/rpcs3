#ifdef _MSC_VER
#include "stdafx.h"
#include "stdafx_d3d12.h"
#include "D3D12GSRender.h"
#include "d3dx12.h"
#include "../Common/TextureUtils.h"
// For clarity this code deals with texture but belongs to D3D12GSRender class
#include "D3D12Formats.h"
#include "../rsx_methods.h"

bool is_dxtc_format(u32 texture_format)
{
	switch (texture_format)
	{
	case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
	case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
	case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
		return true;
	default:
		return false;
	}
}

namespace
{
D3D12_COMPARISON_FUNC get_sampler_compare_func[] =
{
	D3D12_COMPARISON_FUNC_NEVER,
	D3D12_COMPARISON_FUNC_LESS,
	D3D12_COMPARISON_FUNC_EQUAL,
	D3D12_COMPARISON_FUNC_LESS_EQUAL,
	D3D12_COMPARISON_FUNC_GREATER,
	D3D12_COMPARISON_FUNC_NOT_EQUAL,
	D3D12_COMPARISON_FUNC_GREATER_EQUAL,
	D3D12_COMPARISON_FUNC_ALWAYS
};

D3D12_SAMPLER_DESC get_sampler_desc(const rsx::fragment_texture &texture)
{
	D3D12_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = get_texture_filter(texture.min_filter(), texture.mag_filter());
	samplerDesc.AddressU = get_texture_wrap_mode(texture.wrap_s());
	samplerDesc.AddressV = get_texture_wrap_mode(texture.wrap_t());
	samplerDesc.AddressW = get_texture_wrap_mode(texture.wrap_r());
	samplerDesc.ComparisonFunc = get_sampler_compare_func[static_cast<u8>(texture.zfunc())];
	samplerDesc.MaxAnisotropy = get_texture_max_aniso(texture.max_aniso());
	samplerDesc.MipLODBias = texture.bias();
	samplerDesc.BorderColor[0] = (FLOAT)texture.border_color();
	samplerDesc.BorderColor[1] = (FLOAT)texture.border_color();
	samplerDesc.BorderColor[2] = (FLOAT)texture.border_color();
	samplerDesc.BorderColor[3] = (FLOAT)texture.border_color();
	samplerDesc.MinLOD = (FLOAT)(texture.min_lod());
	samplerDesc.MaxLOD = (FLOAT)(texture.max_lod());
	return samplerDesc;
}

namespace
{
	CD3DX12_RESOURCE_DESC get_texture_description(const rsx::fragment_texture &texture)
	{
		const u8 format = texture.format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
		DXGI_FORMAT dxgi_format = get_texture_format(format);
		u16 width = texture.width();
		u16 height = texture.height();
		u16 depth = texture.depth();
		u16 miplevels = texture.get_exact_mipmap_count();

		// DXTC uses 4x4 block texture and align to multiple of 4.
		if (is_dxtc_format(format))
		{
			width = align(width, 4);
			height = align(height, 4);
		}

		switch (texture.get_extended_texture_dimension())
		{
		case rsx::texture_dimension_extended::texture_dimension_1d:
			return CD3DX12_RESOURCE_DESC::Tex1D(dxgi_format, width, 1, miplevels);
		case rsx::texture_dimension_extended::texture_dimension_2d:
			return CD3DX12_RESOURCE_DESC::Tex2D(dxgi_format, width, height, 1, miplevels);
		case rsx::texture_dimension_extended::texture_dimension_cubemap:
			return CD3DX12_RESOURCE_DESC::Tex2D(dxgi_format, width, height, 6, miplevels);
		case rsx::texture_dimension_extended::texture_dimension_3d:
			return CD3DX12_RESOURCE_DESC::Tex3D(dxgi_format, width, height, depth, miplevels);
		}
		fmt::throw_exception("Unknown texture dimension" HERE);
	}
}

namespace {
	/**
	 * Allocate buffer in texture_buffer_heap big enough and upload data into existing_texture which should be in COPY_DEST state
	 */
	void update_existing_texture(
		const rsx::fragment_texture &texture,
		ID3D12GraphicsCommandList *command_list,
		d3d12_data_heap &texture_buffer_heap,
		ID3D12Resource *existing_texture)
	{
		size_t w = texture.width(), h = texture.height();

		const u8 format = texture.format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
		DXGI_FORMAT dxgi_format = get_texture_format(format);

		size_t buffer_size = get_placed_texture_storage_size(texture, 256);
		size_t heap_offset = texture_buffer_heap.alloc<D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT>(buffer_size);
		size_t mip_level = 0;

		void *mapped_buffer_ptr = texture_buffer_heap.map<void>(CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));
		gsl::span<gsl::byte> mapped_buffer{ (gsl::byte*)mapped_buffer_ptr, ::narrow<int>(buffer_size) };
		std::vector<rsx_subresource_layout> input_layouts = get_subresources_layout(texture);
		u8 block_size_in_bytes = get_format_block_size_in_bytes(format);
		u8 block_size_in_texel = get_format_block_size_in_texel(format);
		bool is_swizzled = !(texture.format() & CELL_GCM_TEXTURE_LN);
		size_t offset_in_buffer = 0;
		for (const rsx_subresource_layout &layout : input_layouts)
		{
			texture_uploader_capabilities caps{ false, false, 256 };
			upload_texture_subresource(mapped_buffer.subspan(offset_in_buffer), layout, format, is_swizzled, caps);
			UINT row_pitch = align(layout.width_in_block * block_size_in_bytes, 256);
			command_list->CopyTextureRegion(&CD3DX12_TEXTURE_COPY_LOCATION(existing_texture, (UINT)mip_level), 0, 0, 0,
				&CD3DX12_TEXTURE_COPY_LOCATION(texture_buffer_heap.get_heap(),
				{ heap_offset + offset_in_buffer,
				{
					dxgi_format,
					(UINT)layout.width_in_block * block_size_in_texel,
					(UINT)layout.height_in_block * block_size_in_texel,
					(UINT)layout.depth,
					row_pitch
				}
				}), nullptr);

			offset_in_buffer += row_pitch * layout.height_in_block * layout.depth;
			offset_in_buffer = align(offset_in_buffer, 512);
			mip_level++;
		}
		texture_buffer_heap.unmap(CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));

		command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(existing_texture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
	}
}


/**
 * Create a texture residing in default heap and generate uploads commands in commandList,
 * using a temporary texture buffer.
 */
ComPtr<ID3D12Resource> upload_single_texture(
	const rsx::fragment_texture &texture,
	ID3D12Device *device,
	ID3D12GraphicsCommandList *command_list,
	d3d12_data_heap &texture_buffer_heap)
{
	ComPtr<ID3D12Resource> result;
	CHECK_HRESULT(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&get_texture_description(texture),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(result.GetAddressOf())
		));

	update_existing_texture(texture, command_list, texture_buffer_heap, result.Get());
	return result;
}


D3D12_SHADER_RESOURCE_VIEW_DESC get_srv_descriptor_with_dimensions(const rsx::fragment_texture &tex)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC shared_resource_view_desc = {};
	switch (tex.get_extended_texture_dimension())
	{
	case rsx::texture_dimension_extended::texture_dimension_1d:
		shared_resource_view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
		shared_resource_view_desc.Texture1D.MipLevels = tex.get_exact_mipmap_count();
		return shared_resource_view_desc;
	case rsx::texture_dimension_extended::texture_dimension_2d:
		shared_resource_view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		shared_resource_view_desc.Texture2D.MipLevels = tex.get_exact_mipmap_count();
		return shared_resource_view_desc;
	case rsx::texture_dimension_extended::texture_dimension_cubemap:
		shared_resource_view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		shared_resource_view_desc.TextureCube.MipLevels = tex.get_exact_mipmap_count();
		return shared_resource_view_desc;
	case rsx::texture_dimension_extended::texture_dimension_3d:
		shared_resource_view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
		shared_resource_view_desc.Texture3D.MipLevels = tex.get_exact_mipmap_count();
		return shared_resource_view_desc;
	}
	fmt::throw_exception("Wrong texture dimension" HERE);
}
}

void D3D12GSRender::upload_textures(ID3D12GraphicsCommandList *command_list, size_t texture_count)
{
	for (u32 i = 0; i < texture_count; ++i)
	{
		if (!m_textures_dirty[i])
			continue;
		m_textures_dirty[i] = false;

		if (!rsx::method_registers.fragment_textures[i].enabled())
		{
			// Now fill remaining texture slots with dummy texture/sampler

			D3D12_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc = {};
			shader_resource_view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			shader_resource_view_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			shader_resource_view_desc.Texture2D.MipLevels = 1;
			shader_resource_view_desc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
				D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0,
				D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0,
				D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0,
				D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0);

			m_device->CreateShaderResourceView(m_dummy_texture, &shader_resource_view_desc,
				CD3DX12_CPU_DESCRIPTOR_HANDLE(m_current_texture_descriptors->GetCPUDescriptorHandleForHeapStart())
				.Offset((UINT)i, m_descriptor_stride_srv_cbv_uav)
				);

			D3D12_SAMPLER_DESC sampler_desc = {};
			sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
			sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;

			m_device->CreateSampler(&sampler_desc,
				CD3DX12_CPU_DESCRIPTOR_HANDLE(m_current_sampler_descriptors->GetCPUDescriptorHandleForHeapStart())
				.Offset((UINT)i, m_descriptor_stride_samplers));

			continue;
		}
		size_t w = rsx::method_registers.fragment_textures[i].width(), h = rsx::method_registers.fragment_textures[i].height();
		
		if (!w || !h)
		{
			LOG_ERROR(RSX, "Texture upload requested but invalid texture dimensions passed");
			continue;
		}

		const u32 texaddr = rsx::get_address(rsx::method_registers.fragment_textures[i].offset(), rsx::method_registers.fragment_textures[i].location());

		const u8 format = rsx::method_registers.fragment_textures[i].format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
		bool is_swizzled = !(rsx::method_registers.fragment_textures[i].format() & CELL_GCM_TEXTURE_LN);

		ID3D12Resource *vram_texture;
		std::pair<texture_entry, ComPtr<ID3D12Resource> > *cached_texture = m_texture_cache.find_data_if_available(texaddr);
		bool is_render_target = false, is_depth_stencil_texture = false;

		if (vram_texture = m_rtts.get_texture_from_render_target_if_applicable(texaddr))
		{
			is_render_target = true;
		}
		else if (vram_texture = m_rtts.get_texture_from_depth_stencil_if_applicable(texaddr))
		{
			is_depth_stencil_texture = true;
		}
		else if (cached_texture != nullptr && (cached_texture->first == texture_entry(format, w, h, rsx::method_registers.fragment_textures[i].depth(), rsx::method_registers.fragment_textures[i].get_exact_mipmap_count())))
		{
			if (cached_texture->first.m_is_dirty)
			{
				command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(cached_texture->second.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST));
				update_existing_texture(rsx::method_registers.fragment_textures[i], command_list, m_buffer_data, cached_texture->second.Get());
				m_texture_cache.protect_data(texaddr, texaddr, get_texture_size(rsx::method_registers.fragment_textures[i]));
			}
			vram_texture = cached_texture->second.Get();
		}
		else
		{
			if (cached_texture != nullptr)
				get_current_resource_storage().dirty_textures.push_back(m_texture_cache.remove_from_cache(texaddr));
			ComPtr<ID3D12Resource> tex = upload_single_texture(rsx::method_registers.fragment_textures[i], m_device.Get(), command_list, m_buffer_data);
			std::wstring name = L"texture_@" + std::to_wstring(texaddr);
			tex->SetName(name.c_str());
			vram_texture = tex.Get();
			m_texture_cache.store_and_protect_data(texaddr, texaddr, get_texture_size(rsx::method_registers.fragment_textures[i]), format, w, h, rsx::method_registers.fragment_textures[i].depth(), rsx::method_registers.fragment_textures[i].get_exact_mipmap_count(), tex);
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC shared_resource_view_desc = get_srv_descriptor_with_dimensions(rsx::method_registers.fragment_textures[i]);
		shared_resource_view_desc.Format = get_texture_format(format);

		bool requires_remap = false;
		std::array<INT, 4> channel_mapping = {};

		switch (format)
		{
		default:
			LOG_ERROR(RSX, "Unimplemented mapping for texture format: 0x%x", format);
			break;
		
		case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
		case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
		case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
		case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
		case CELL_GCM_TEXTURE_DEPTH24_D8:
		case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
		case CELL_GCM_TEXTURE_DEPTH16:
		case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
		case CELL_GCM_TEXTURE_X32_FLOAT:
		case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
		case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
		case CELL_GCM_TEXTURE_R5G5B5A1:
		case CELL_GCM_TEXTURE_D1R5G5B5:
		case CELL_GCM_TEXTURE_A1R5G5B5:
		case CELL_GCM_TEXTURE_A4R4G4B4:
		case CELL_GCM_TEXTURE_R5G6B5:
			shared_resource_view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			break;

		case CELL_GCM_TEXTURE_B8:
			shared_resource_view_desc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0);
			break;

		case CELL_GCM_TEXTURE_G8B8:
		{
			u8 remap_a = rsx::method_registers.fragment_textures[i].remap() & 0x3;
			u8 remap_r = (rsx::method_registers.fragment_textures[i].remap() >> 2) & 0x3;
			u8 remap_g = (rsx::method_registers.fragment_textures[i].remap() >> 4) & 0x3;
			u8 remap_b = (rsx::method_registers.fragment_textures[i].remap() >> 6) & 0x3;

			if (is_render_target)
			{
				// ARGB format
				// Data comes from RTT, stored as RGBA already
				const int RemapValue[4] =
				{
					D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
					D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2,
					D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
					D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2
				};

				channel_mapping = { RemapValue[remap_r], RemapValue[remap_g], RemapValue[remap_b], RemapValue[remap_a] };
				requires_remap = true;
			}
			else
			{
				// ARGB format
				// Data comes from RSX mem, stored as ARGB already
				const int RemapValue[4] =
				{
					D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2,
					D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
					D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2,
					D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1
				};

				channel_mapping = { RemapValue[remap_r], RemapValue[remap_g], RemapValue[remap_b], RemapValue[remap_a] };
				requires_remap = true;
			}

			break;
		}

		case CELL_GCM_TEXTURE_R6G5B5: // TODO: Remap it to another format here, so it's not glitched out
			shared_resource_view_desc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2,
				D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0);
			break;

		case CELL_GCM_TEXTURE_X16:
			shared_resource_view_desc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
				D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
				D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0);
			break;

		case CELL_GCM_TEXTURE_Y16_X16:
		case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
			shared_resource_view_desc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0);
			break;

		case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
			shared_resource_view_desc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1);
			break;

		case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
		case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
			shared_resource_view_desc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
				D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0);
			break;
				
		case CELL_GCM_TEXTURE_D8R8G8B8:	
			shared_resource_view_desc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3,
				D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1);
			break;
			
		case CELL_GCM_TEXTURE_A8R8G8B8:
		{
			u8 remap_a = rsx::method_registers.fragment_textures[i].remap() & 0x3;
			u8 remap_r = (rsx::method_registers.fragment_textures[i].remap() >> 2) & 0x3;
			u8 remap_g = (rsx::method_registers.fragment_textures[i].remap() >> 4) & 0x3;
			u8 remap_b = (rsx::method_registers.fragment_textures[i].remap() >> 6) & 0x3;

			if (is_render_target)
			{
				// ARGB format
				// Data comes from RTT, stored as RGBA already
				const int RemapValue[4] =
				{
					D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3,
					D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
					D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
					D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2
				};

				channel_mapping = { RemapValue[remap_r], RemapValue[remap_g], RemapValue[remap_b], RemapValue[remap_a] };
				requires_remap = true;
			}
			else if (is_depth_stencil_texture)
			{
				shared_resource_view_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
				shared_resource_view_desc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
					D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
					D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
					D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
					D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0);
			}
			else
			{
				// ARGB format
				// Data comes from RSX mem, stored as ARGB already
				const int RemapValue[4] =
				{
					D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
					D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
					D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2,
					D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3
				};

				channel_mapping = { RemapValue[remap_r], RemapValue[remap_g], RemapValue[remap_b], RemapValue[remap_a] };
				requires_remap = true;
			}

			break;
		}
		}

		if (requires_remap)
		{
			auto decoded_remap = rsx::method_registers.fragment_textures[i].decoded_remap();
			u8 remapped_inputs[] = { decoded_remap.second[1], decoded_remap.second[2], decoded_remap.second[3], decoded_remap.second[0] };

			for (u8 channel = 0; channel < 4; channel++)
			{
				switch (remapped_inputs[channel])
				{
				default:
				case CELL_GCM_TEXTURE_REMAP_REMAP:
					break;
				case CELL_GCM_TEXTURE_REMAP_ONE:
					channel_mapping[channel] = D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1;
					break;
				case CELL_GCM_TEXTURE_REMAP_ZERO:
					channel_mapping[channel] = D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0;
				}
			}

			shared_resource_view_desc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
				channel_mapping[0],
				channel_mapping[1],
				channel_mapping[2],
				channel_mapping[3]);
		}

		m_device->CreateShaderResourceView(vram_texture, &shared_resource_view_desc,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(m_current_texture_descriptors->GetCPUDescriptorHandleForHeapStart())
			.Offset((UINT)i, m_descriptor_stride_srv_cbv_uav)
			);

		m_device->CreateSampler(&get_sampler_desc(rsx::method_registers.fragment_textures[i]),
			CD3DX12_CPU_DESCRIPTOR_HANDLE(m_current_sampler_descriptors->GetCPUDescriptorHandleForHeapStart())
			.Offset((UINT)i, m_descriptor_stride_samplers));
	}
}
#endif
