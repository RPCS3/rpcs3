#include "stdafx.h"
#include "stdafx_d3d12.h"
#ifdef _MSC_VER
#include "D3D12GSRender.h"
#include "d3dx12.h"
#include "../Common/TextureUtils.h"
// For clarity this code deals with texture but belongs to D3D12GSRender class
#include "D3D12Formats.h"

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

D3D12_SAMPLER_DESC get_sampler_desc(const rsx::texture &texture)
{
	D3D12_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = get_texture_filter(texture.min_filter(), texture.mag_filter());
	samplerDesc.AddressU = get_texture_wrap_mode(texture.wrap_s());
	samplerDesc.AddressV = get_texture_wrap_mode(texture.wrap_t());
	samplerDesc.AddressW = get_texture_wrap_mode(texture.wrap_r());
	samplerDesc.ComparisonFunc = get_sampler_compare_func[texture.zfunc()];
	samplerDesc.MaxAnisotropy = get_texture_max_aniso(texture.max_aniso());
	samplerDesc.MipLODBias = texture.bias();
	samplerDesc.BorderColor[0] = (FLOAT)texture.border_color();
	samplerDesc.BorderColor[1] = (FLOAT)texture.border_color();
	samplerDesc.BorderColor[2] = (FLOAT)texture.border_color();
	samplerDesc.BorderColor[3] = (FLOAT)texture.border_color();
	samplerDesc.MinLOD = (FLOAT)(texture.min_lod() >> 8);
	samplerDesc.MaxLOD = (FLOAT)(texture.max_lod() >> 8);
	return samplerDesc;
}


/**
 * Create a texture residing in default heap and generate uploads commands in commandList,
 * using a temporary texture buffer.
 */
ComPtr<ID3D12Resource> upload_single_texture(
	const rsx::texture &texture,
	ID3D12Device *device,
	ID3D12GraphicsCommandList *command_list,
	data_heap &texture_buffer_heap)
{
	size_t w = texture.width(), h = texture.height();
	size_t depth = texture.depth();
	if (depth == 0) depth = 1;
	if (texture.cubemap()) depth *= 6;

	const u8 format = texture.format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
	DXGI_FORMAT dxgi_format = get_texture_format(format);

	size_t buffer_size = get_placed_texture_storage_size(texture, 256);
	size_t heap_offset = texture_buffer_heap.alloc<D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT>(buffer_size);

	void *mapped_buffer = texture_buffer_heap.map<void>(CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));
	std::vector<MipmapLevelInfo> mipInfos = upload_placed_texture(texture, 256, mapped_buffer);
	texture_buffer_heap.unmap(CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));

	ComPtr<ID3D12Resource> result;
	CHECK_HRESULT(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(dxgi_format, (UINT)w, (UINT)h, (UINT)depth, texture.mipmap()),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(result.GetAddressOf())
		));

	size_t mip_level = 0;
	for (const MipmapLevelInfo mli : mipInfos)
	{
		command_list->CopyTextureRegion(&CD3DX12_TEXTURE_COPY_LOCATION(result.Get(), (UINT)mip_level), 0, 0, 0,
			&CD3DX12_TEXTURE_COPY_LOCATION(texture_buffer_heap.get_heap(), { heap_offset + mli.offset, { dxgi_format, (UINT)mli.width, (UINT)mli.height, 1, (UINT)mli.rowPitch } }), nullptr);
		mip_level++;
	}

	command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(result.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
	return result;
}

/**
*
*/
void update_existing_texture(
	const rsx::texture &texture,
	ID3D12GraphicsCommandList *command_list,
	data_heap &texture_buffer_heap,
	ID3D12Resource *existing_texture)
{
	size_t w = texture.width(), h = texture.height();

	const u8 format = texture.format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
	DXGI_FORMAT dxgi_format = get_texture_format(format);

	size_t buffer_size = get_placed_texture_storage_size(texture, 256);
	size_t heap_offset = texture_buffer_heap.alloc<D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT>(buffer_size);

	void *mapped_buffer = texture_buffer_heap.map<void>(CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));
	std::vector<MipmapLevelInfo> mipInfos = upload_placed_texture(texture, 256, mapped_buffer);
	texture_buffer_heap.unmap(CD3DX12_RANGE(heap_offset, heap_offset + buffer_size));

	command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(existing_texture, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST));
	size_t miplevel = 0;
	for (const MipmapLevelInfo mli : mipInfos)
	{
		command_list->CopyTextureRegion(&CD3DX12_TEXTURE_COPY_LOCATION(existing_texture, (UINT)miplevel), 0, 0, 0,
			&CD3DX12_TEXTURE_COPY_LOCATION(texture_buffer_heap.get_heap(), { heap_offset + mli.offset,{ dxgi_format, (UINT)mli.width, (UINT)mli.height, 1, (UINT)mli.rowPitch } }), nullptr);
		miplevel++;
	}

	command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(existing_texture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
}
}

void D3D12GSRender::upload_and_bind_textures(ID3D12GraphicsCommandList *command_list, size_t descriptor_index, size_t texture_count)
{
	size_t used_texture = 0;

	for (u32 i = 0; i < rsx::limits::textures_count; ++i)
	{
		if (!textures[i].enabled())
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
				CD3DX12_CPU_DESCRIPTOR_HANDLE(get_current_resource_storage().descriptors_heap->GetCPUDescriptorHandleForHeapStart())
				.Offset((INT)descriptor_index + (INT)used_texture, g_descriptor_stride_srv_cbv_uav)
				);

			D3D12_SAMPLER_DESC sampler_desc = {};
			sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
			sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			m_device->CreateSampler(&sampler_desc,
				CD3DX12_CPU_DESCRIPTOR_HANDLE(get_current_resource_storage().sampler_descriptor_heap[get_current_resource_storage().sampler_descriptors_heap_index]->GetCPUDescriptorHandleForHeapStart())
				.Offset((INT)get_current_resource_storage().current_sampler_index + (INT)used_texture, g_descriptor_stride_samplers)
				);
			used_texture++;
			continue;
		}
		size_t w = textures[i].width(), h = textures[i].height();
//		if (!w || !h) continue;

		const u32 texaddr = rsx::get_address(textures[i].offset(), textures[i].location());

		const u8 format = textures[i].format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
		bool is_swizzled = !(textures[i].format() & CELL_GCM_TEXTURE_LN);

		ID3D12Resource *vram_texture;
		std::unordered_map<u32, ComPtr<ID3D12Resource> >::const_iterator ItRTT = m_rtts.render_targets_storage.find(texaddr);
		std::unordered_map<u32, ComPtr<ID3D12Resource> >::const_iterator ItDS = m_rtts.depth_stencil_storage.find(texaddr);
		std::pair<texture_entry, ComPtr<ID3D12Resource> > *cached_texture = m_texture_cache.find_data_if_available(texaddr);
		bool is_render_target = false, is_depth_stencil_texture = false;
		if (ItRTT != m_rtts.render_targets_storage.end())
		{
			vram_texture = ItRTT->second.Get();
			is_render_target = true;
		}
		else if (ItDS != m_rtts.depth_stencil_storage.end())
		{
			vram_texture = ItDS->second.Get();
			is_depth_stencil_texture = true;
		}
		else if (cached_texture != nullptr && (cached_texture->first == texture_entry(format, w, h, textures[i].mipmap())))
		{
			if (cached_texture->first.m_is_dirty)
			{
				update_existing_texture(textures[i], command_list, m_buffer_data, cached_texture->second.Get());
				m_texture_cache.protect_data(texaddr, texaddr, get_texture_size(textures[i]));
			}
			vram_texture = cached_texture->second.Get();
		}
		else
		{
			if (cached_texture != nullptr)
				get_current_resource_storage().dirty_textures.push_back(m_texture_cache.remove_from_cache(texaddr));
			ComPtr<ID3D12Resource> tex = upload_single_texture(textures[i], m_device.Get(), command_list, m_buffer_data);
			std::wstring name = L"texture_@" + std::to_wstring(texaddr);
			tex->SetName(name.c_str());
			vram_texture = tex.Get();
			m_texture_cache.store_and_protect_data(texaddr, texaddr, get_texture_size(textures[i]), format, w, h, textures[i].mipmap(), tex);
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC shared_resource_view_desc = {};
		if (textures[i].cubemap())
		{
			shared_resource_view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			shared_resource_view_desc.TextureCube.MipLevels = textures[i].mipmap();
		}
		else
		{
			shared_resource_view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			shared_resource_view_desc.Texture2D.MipLevels = textures[i].mipmap();
		}
		shared_resource_view_desc.Format = get_texture_format(format);

		switch (format)
		{
		case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
		case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
		case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
		case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
		default:
			LOG_ERROR(RSX, "Unimplemented Texture format : 0x%x", format);
			break;
		case CELL_GCM_TEXTURE_B8:
			shared_resource_view_desc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0);
			break;
		case CELL_GCM_TEXTURE_A1R5G5B5:
		case CELL_GCM_TEXTURE_A4R4G4B4:
		case CELL_GCM_TEXTURE_R5G6B5:
			shared_resource_view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			break;
		case CELL_GCM_TEXTURE_A8R8G8B8:
		case CELL_GCM_TEXTURE_D8R8G8B8:
		{


			u8 remap_a = textures[i].remap() & 0x3;
			u8 remap_r = (textures[i].remap() >> 2) & 0x3;
			u8 remap_g = (textures[i].remap() >> 4) & 0x3;
			u8 remap_b = (textures[i].remap() >> 6) & 0x3;
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

				shared_resource_view_desc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
					RemapValue[remap_r],
					RemapValue[remap_g],
					RemapValue[remap_b],
					RemapValue[remap_a]);
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

				shared_resource_view_desc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
					RemapValue[remap_r],
					RemapValue[remap_g],
					RemapValue[remap_b],
					RemapValue[remap_a]);
			}

			break;
		}
		case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
		case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
		case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
		case CELL_GCM_TEXTURE_G8B8:
		case CELL_GCM_TEXTURE_R6G5B5:
		case CELL_GCM_TEXTURE_DEPTH24_D8:
		case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
		case CELL_GCM_TEXTURE_DEPTH16:
		case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
		case CELL_GCM_TEXTURE_X16:
		case CELL_GCM_TEXTURE_Y16_X16:
		case CELL_GCM_TEXTURE_R5G5B5A1:
		case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
		case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
		case CELL_GCM_TEXTURE_X32_FLOAT:
		case CELL_GCM_TEXTURE_D1R5G5B5:
			shared_resource_view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			break;
		case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
			shared_resource_view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			break;
		case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
			shared_resource_view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			break;
		case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
			shared_resource_view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			break;
		}

		m_device->CreateShaderResourceView(vram_texture, &shared_resource_view_desc,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(get_current_resource_storage().descriptors_heap->GetCPUDescriptorHandleForHeapStart())
			.Offset((UINT)descriptor_index + (UINT)used_texture, g_descriptor_stride_srv_cbv_uav));

		if (get_current_resource_storage().current_sampler_index + 16 > 2048)
		{
			get_current_resource_storage().sampler_descriptors_heap_index = 1;
			get_current_resource_storage().current_sampler_index = 0;

			ID3D12DescriptorHeap *descriptors[] =
			{
				get_current_resource_storage().descriptors_heap.Get(),
				get_current_resource_storage().sampler_descriptor_heap[get_current_resource_storage().sampler_descriptors_heap_index].Get(),
			};
			command_list->SetDescriptorHeaps(2, descriptors);
		}
		m_device->CreateSampler(&get_sampler_desc(textures[i]),
			CD3DX12_CPU_DESCRIPTOR_HANDLE(get_current_resource_storage().sampler_descriptor_heap[get_current_resource_storage().sampler_descriptors_heap_index]->GetCPUDescriptorHandleForHeapStart())
			.Offset((UINT)get_current_resource_storage().current_sampler_index + (UINT)used_texture, g_descriptor_stride_samplers));

		used_texture++;
	}


}
#endif
