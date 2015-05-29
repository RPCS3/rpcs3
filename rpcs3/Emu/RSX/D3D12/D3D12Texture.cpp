#include "stdafx.h"
#if defined(DX12_SUPPORT)
#include "D3D12GSRender.h"
// For clarity this code deals with texture but belongs to D3D12GSRender class


static
u32 LinearToSwizzleAddress(u32 x, u32 y, u32 z, u32 log2_width, u32 log2_height, u32 log2_depth)
{
	u32 offset = 0;
	u32 shift_count = 0;
	while (log2_width | log2_height | log2_depth) {
		if (log2_width)
		{
			offset |= (x & 0x01) << shift_count;
			x >>= 1;
			++shift_count;
			--log2_width;
		}
		if (log2_height)
		{
			offset |= (y & 0x01) << shift_count;
			y >>= 1;
			++shift_count;
			--log2_height;
		}
		if (log2_depth)
		{
			offset |= (z & 0x01) << shift_count;
			z >>= 1;
			++shift_count;
			--log2_depth;
		}
	}
	return offset;
}

static D3D12_COMPARISON_FUNC ComparisonFunc[] =
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

size_t D3D12GSRender::GetMaxAniso(size_t aniso)
{
	switch (aniso)
	{
	case CELL_GCM_TEXTURE_MAX_ANISO_1: return 1;
	case CELL_GCM_TEXTURE_MAX_ANISO_2: return 2;
	case CELL_GCM_TEXTURE_MAX_ANISO_4: return 4;
	case CELL_GCM_TEXTURE_MAX_ANISO_6: return 6;
	case CELL_GCM_TEXTURE_MAX_ANISO_8: return 8;
	case CELL_GCM_TEXTURE_MAX_ANISO_10: return 10;
	case CELL_GCM_TEXTURE_MAX_ANISO_12: return 12;
	case CELL_GCM_TEXTURE_MAX_ANISO_16: return 16;
	}

	return 1;
}

D3D12_TEXTURE_ADDRESS_MODE D3D12GSRender::GetWrap(size_t wrap)
{
	switch (wrap)
	{
	case CELL_GCM_TEXTURE_WRAP: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	case CELL_GCM_TEXTURE_MIRROR: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	case CELL_GCM_TEXTURE_CLAMP_TO_EDGE: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	case CELL_GCM_TEXTURE_BORDER: return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	case CELL_GCM_TEXTURE_CLAMP: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	case CELL_GCM_TEXTURE_MIRROR_ONCE_CLAMP_TO_EDGE: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
	case CELL_GCM_TEXTURE_MIRROR_ONCE_BORDER: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
	case CELL_GCM_TEXTURE_MIRROR_ONCE_CLAMP: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
	}

	return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
}

size_t D3D12GSRender::UploadTextures()
{
	size_t usedTexture = 0;

	for (u32 i = 0; i < m_textures_count; ++i)
	{
		if (!m_textures[i].IsEnabled()) continue;
		size_t w = m_textures[i].GetWidth(), h = m_textures[i].GetHeight();

		const u32 texaddr = GetAddress(m_textures[i].GetOffset(), m_textures[i].GetLocation());

		DXGI_FORMAT dxgiFormat;
		size_t blockSizeInByte, blockWidthInPixel, blockHeightInPixel;
		int format = m_textures[i].GetFormat() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);

		bool is_swizzled = !(m_textures[i].GetFormat() & CELL_GCM_TEXTURE_LN);
		switch (format)
		{
		case CELL_GCM_TEXTURE_A1R5G5B5:
		case CELL_GCM_TEXTURE_A4R4G4B4:
		case CELL_GCM_TEXTURE_R5G6B5:
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
		case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
		case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
		case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
		default:
			LOG_ERROR(RSX, "Unimplemented Texture format");
			break;
		case CELL_GCM_TEXTURE_D8R8G8B8:
			dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
			blockSizeInByte = 4;
			blockWidthInPixel = 1, blockHeightInPixel = 1;
			break;
		case CELL_GCM_TEXTURE_A8R8G8B8:
			dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
			blockSizeInByte = 4;
			blockWidthInPixel = 1, blockHeightInPixel = 1;
			break;
		case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
			dxgiFormat = DXGI_FORMAT_BC1_UNORM;
			blockSizeInByte = 8;
			blockWidthInPixel = 4, blockHeightInPixel = 4;
			break;
		case CELL_GCM_TEXTURE_B8:
			dxgiFormat = DXGI_FORMAT_R8_UNORM;
			blockSizeInByte = 1;
			blockWidthInPixel = 1, blockHeightInPixel = 1;
			break;
		}

		ID3D12Resource *vramTexture;
		std::unordered_map<u32, Microsoft::WRL::ComPtr<ID3D12Resource> >::const_iterator ItRTT = m_texturesRTTs.find(texaddr);
		std::unordered_map<u32, ID3D12Resource* >::const_iterator ItCache = m_texturesCache.find(texaddr);
		if (ItRTT != m_texturesRTTs.end())
		{
			vramTexture = ItRTT->second.Get();
		}
		else if (ItCache != m_texturesCache.end())
		{
			vramTexture = ItCache->second;
		}
		else
		{
			// Upload at each iteration to take advantage of overlapping transfer
			ID3D12GraphicsCommandList *commandList;
			check(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_perFrameStorage.m_textureUploadCommandAllocator, nullptr, IID_PPV_ARGS(&commandList)));

			size_t heightInBlocks = (m_textures[i].GetHeight() + blockHeightInPixel - 1) / blockHeightInPixel;
			size_t widthInBlocks = (m_textures[i].GetWidth() + blockWidthInPixel - 1) / blockWidthInPixel;
			// Multiple of 256
			size_t rowPitch = blockSizeInByte * widthInBlocks;
			rowPitch = (rowPitch + 255) & ~255;

			ID3D12Resource *Texture;
			size_t textureSize = rowPitch * heightInBlocks;

			check(m_device->CreatePlacedResource(
				m_perFrameStorage.m_uploadTextureHeap,
				m_perFrameStorage.m_currentStorageOffset,
				&getBufferResourceDesc(textureSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&Texture)
				));

			auto pixels = vm::get_ptr<const u8>(texaddr);
			void *textureData;
			check(Texture->Map(0, nullptr, (void**)&textureData));

			// Upload with correct rowpitch
			for (unsigned row = 0; row < heightInBlocks; row++)
			{
				size_t m_texture_pitch = m_textures[i].m_pitch;
				if (!m_texture_pitch) m_texture_pitch = rowPitch;
				if (format == CELL_GCM_TEXTURE_A8R8G8B8 && is_swizzled)
				{
					u32 *src, *dst;
					u32 log2width, log2height;

					src = (u32*)pixels;
					dst = (u32*)textureData;

					log2width = (u32)(logf(m_textures[i].GetWidth()) / logf(2.f));
					log2height = (u32)(logf(m_textures[i].GetHeight()) / logf(2.f));

					for (int j = 0; j < m_textures[i].GetWidth(); j++)
					{
						dst[(row * rowPitch / 4) + j] = src[LinearToSwizzleAddress(j, i, 0, log2width, log2height, 0)];
					}
				}
				else
					streamToBuffer((char*)textureData + row * rowPitch, (char*)pixels + row * m_texture_pitch, m_texture_pitch);
			}
			Texture->Unmap(0, nullptr);

			check(m_device->CreatePlacedResource(
				m_perFrameStorage.m_textureStorage,
				m_perFrameStorage.m_currentStorageOffset,
				&getTexture2DResourceDesc(m_textures[i].GetWidth(), m_textures[i].GetHeight(), dxgiFormat),
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&vramTexture)
				));

			m_perFrameStorage.m_currentStorageOffset += textureSize;
			m_perFrameStorage.m_currentStorageOffset = (m_perFrameStorage.m_currentStorageOffset + 65536 - 1) & ~65535;
			m_perFrameStorage.m_inflightResources.push_back(Texture);
			m_perFrameStorage.m_inflightResources.push_back(vramTexture);

			D3D12_TEXTURE_COPY_LOCATION dst = {}, src = {};
			dst.pResource = vramTexture;
			dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			src.pResource = Texture;
			src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			src.PlacedFootprint.Footprint.Depth = 1;
			src.PlacedFootprint.Footprint.Width = m_textures[i].GetWidth();
			src.PlacedFootprint.Footprint.Height = m_textures[i].GetHeight();
			src.PlacedFootprint.Footprint.RowPitch = (UINT)rowPitch;
			src.PlacedFootprint.Footprint.Format = dxgiFormat;

			commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Transition.pResource = vramTexture;
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
			commandList->ResourceBarrier(1, &barrier);

			commandList->Close();
			m_commandQueueGraphic->ExecuteCommandLists(1, (ID3D12CommandList**)&commandList);
			m_perFrameStorage.m_inflightCommandList.push_back(commandList);
			m_texturesCache[texaddr] = vramTexture;
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Format = dxgiFormat;
		srvDesc.Texture2D.MipLevels = 1;
		const int RemapValue[4] =
		{
			D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
			D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2,
			D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3,
			D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0
		};
		if (format != CELL_GCM_TEXTURE_B8 && format != CELL_GCM_TEXTURE_X16 && format != CELL_GCM_TEXTURE_X32_FLOAT)
		{
			u8 remap_a = m_textures[i].GetRemap() & 0x3;
			u8 remap_r = (m_textures[i].GetRemap() >> 2) & 0x3;
			u8 remap_g = (m_textures[i].GetRemap() >> 4) & 0x3;
			u8 remap_b = (m_textures[i].GetRemap() >> 6) & 0x3;

			srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(RemapValue[remap_a], RemapValue[remap_r], RemapValue[remap_g], RemapValue[remap_b]);
		}
		else
			srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(RemapValue[0], RemapValue[1], RemapValue[2], RemapValue[3]);

		D3D12_CPU_DESCRIPTOR_HANDLE Handle = m_perFrameStorage.m_textureDescriptorsHeap->GetCPUDescriptorHandleForHeapStart();
		Handle.ptr += (m_perFrameStorage.m_currentTextureIndex + usedTexture) * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_device->CreateShaderResourceView(vramTexture, &srvDesc, Handle);

		// TODO : Correctly define sampler
		D3D12_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		samplerDesc.AddressU = GetWrap(m_textures[i].GetWrapS());
		samplerDesc.AddressV = GetWrap(m_textures[i].GetWrapT());
		samplerDesc.AddressW = GetWrap(m_textures[i].GetWrapR());
		samplerDesc.ComparisonFunc = ComparisonFunc[m_textures[i].GetZfunc()];
		samplerDesc.MaxAnisotropy = GetMaxAniso(m_textures[i].GetMaxAniso());
		samplerDesc.MipLODBias = m_textures[i].GetBias();
		samplerDesc.BorderColor[4] = m_textures[i].GetBorderColor();
		samplerDesc.MinLOD = m_textures[i].GetMinLOD() >> 8;
		samplerDesc.MaxLOD = m_textures[i].GetMaxLOD() >> 8;
		Handle = m_perFrameStorage.m_samplerDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		Handle.ptr += (m_perFrameStorage.m_currentTextureIndex + usedTexture) * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		m_device->CreateSampler(&samplerDesc, Handle);

		usedTexture++;
	}

	return usedTexture;
}

#endif