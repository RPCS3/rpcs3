#include "stdafx.h"
#if defined(DX12_SUPPORT)
#include "D3D12GSRender.h"
#include "d3dx12.h"
#include "../Common/TextureUtils.h"
// For clarity this code deals with texture but belongs to D3D12GSRender class

static
D3D12_COMPARISON_FUNC getSamplerCompFunc[] =
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

static
size_t getSamplerMaxAniso(size_t aniso)
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

static
D3D12_TEXTURE_ADDRESS_MODE getSamplerWrap(size_t wrap)
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

static
D3D12_FILTER getSamplerFilter(u32 minFilter, u32 magFilter)
{
	D3D12_FILTER_TYPE min, mag, mip;
	switch (minFilter)
	{
	case CELL_GCM_TEXTURE_NEAREST:
		min = D3D12_FILTER_TYPE_POINT;
		mip = D3D12_FILTER_TYPE_POINT;
		break;
	case CELL_GCM_TEXTURE_LINEAR:
		min = D3D12_FILTER_TYPE_LINEAR;
		mip = D3D12_FILTER_TYPE_POINT;
		break;
	case CELL_GCM_TEXTURE_NEAREST_NEAREST:
		min = D3D12_FILTER_TYPE_POINT;
		mip = D3D12_FILTER_TYPE_POINT;
		break;
	case CELL_GCM_TEXTURE_LINEAR_NEAREST:
		min = D3D12_FILTER_TYPE_LINEAR;
		mip = D3D12_FILTER_TYPE_POINT;
		break;
	case CELL_GCM_TEXTURE_NEAREST_LINEAR:
		min = D3D12_FILTER_TYPE_POINT;
		mip = D3D12_FILTER_TYPE_LINEAR;
		break;
	case CELL_GCM_TEXTURE_LINEAR_LINEAR:
		min = D3D12_FILTER_TYPE_LINEAR;
		mip = D3D12_FILTER_TYPE_LINEAR;
		break;
	case CELL_GCM_TEXTURE_CONVOLUTION_MIN:
	default:
		LOG_ERROR(RSX, "Unknow min filter %x", minFilter);
	}

	switch (magFilter)
	{
	case CELL_GCM_TEXTURE_NEAREST:
		mag = D3D12_FILTER_TYPE_POINT;
		break;
	case CELL_GCM_TEXTURE_LINEAR:
		mag = D3D12_FILTER_TYPE_LINEAR;
		break;
	default:
		LOG_ERROR(RSX, "Unknow mag filter %x", magFilter);
	}

	return D3D12_ENCODE_BASIC_FILTER(min, mag, mip, D3D12_FILTER_REDUCTION_TYPE_STANDARD);
}

static
D3D12_SAMPLER_DESC getSamplerDesc(const rsx::texture &texture)
{
	D3D12_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = getSamplerFilter(texture.min_filter(), texture.mag_filter());
	samplerDesc.AddressU = getSamplerWrap(texture.wrap_s());
	samplerDesc.AddressV = getSamplerWrap(texture.wrap_t());
	samplerDesc.AddressW = getSamplerWrap(texture.wrap_r());
	samplerDesc.ComparisonFunc = getSamplerCompFunc[texture.zfunc()];
	samplerDesc.MaxAnisotropy = (UINT)getSamplerMaxAniso(texture.max_aniso());
	samplerDesc.MipLODBias = texture.bias();
	samplerDesc.BorderColor[4] = (FLOAT)texture.border_color();
	samplerDesc.MinLOD = (FLOAT)(texture.min_lod() >> 8);
	samplerDesc.MaxLOD = (FLOAT)(texture.max_lod() >> 8);
	return samplerDesc;
}


/**
 * Create a texture residing in default heap and generate uploads commands in commandList,
 * using a temporary texture buffer.
 */
static
ComPtr<ID3D12Resource> uploadSingleTexture(
	const rsx::texture &texture,
	ID3D12Device *device,
	ID3D12GraphicsCommandList *commandList,
	DataHeap<ID3D12Resource, 65536> &textureBuffersHeap)
{
	ComPtr<ID3D12Resource> vramTexture;
	size_t w = texture.width(), h = texture.height();

	int format = texture.format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
	DXGI_FORMAT dxgiFormat = getTextureDXGIFormat(format);

	size_t textureSize = getPlacedTextureStorageSpace(texture, 256);
	assert(textureBuffersHeap.canAlloc(textureSize));
	size_t heapOffset = textureBuffersHeap.alloc(textureSize);

	void *buffer;
	ThrowIfFailed(textureBuffersHeap.m_heap->Map(0, &CD3DX12_RANGE(heapOffset, heapOffset + textureSize), &buffer));
	void *textureData = (char*)buffer + heapOffset;
	std::vector<MipmapLevelInfo> mipInfos = uploadPlacedTexture(texture, 256, textureData);
	textureBuffersHeap.m_heap->Unmap(0, &CD3DX12_RANGE(heapOffset, heapOffset + textureSize));

	D3D12_RESOURCE_DESC texturedesc = CD3DX12_RESOURCE_DESC::Tex2D(dxgiFormat, (UINT)w, (UINT)h, 1, texture.mipmap());
	textureSize = device->GetResourceAllocationInfo(0, 1, &texturedesc).SizeInBytes;

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texturedesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(vramTexture.GetAddressOf())
		));

	size_t miplevel = 0;
	for (const MipmapLevelInfo mli : mipInfos)
	{
		commandList->CopyTextureRegion(&CD3DX12_TEXTURE_COPY_LOCATION(vramTexture.Get(), (UINT)miplevel), 0, 0, 0,
			&CD3DX12_TEXTURE_COPY_LOCATION(textureBuffersHeap.m_heap, { heapOffset + mli.offset, { dxgiFormat, (UINT)mli.width, (UINT)mli.height, 1, (UINT)mli.rowPitch } }), nullptr);
		miplevel++;
	}

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vramTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
	return vramTexture;
}



/**
*
*/
static
void updateExistingTexture(
	const rsx::texture &texture,
	ID3D12GraphicsCommandList *commandList,
	DataHeap<ID3D12Resource, 65536> &textureBuffersHeap,
	ID3D12Resource *existingTexture)
{
	size_t w = texture.width(), h = texture.height();

	int format = texture.format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
	DXGI_FORMAT dxgiFormat = getTextureDXGIFormat(format);

	size_t textureSize = getPlacedTextureStorageSpace(texture, 256);
	assert(textureBuffersHeap.canAlloc(textureSize));
	size_t heapOffset = textureBuffersHeap.alloc(textureSize);

	void *buffer;
	ThrowIfFailed(textureBuffersHeap.m_heap->Map(0, &CD3DX12_RANGE(heapOffset, heapOffset + textureSize), &buffer));
	void *textureData = (char*)buffer + heapOffset;
	std::vector<MipmapLevelInfo> mipInfos = uploadPlacedTexture(texture, 256, textureData);
	textureBuffersHeap.m_heap->Unmap(0, &CD3DX12_RANGE(heapOffset, heapOffset + textureSize));

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(existingTexture, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST));
	size_t miplevel = 0;
	for (const MipmapLevelInfo mli : mipInfos)
	{
		commandList->CopyTextureRegion(&CD3DX12_TEXTURE_COPY_LOCATION(existingTexture, (UINT)miplevel), 0, 0, 0,
			&CD3DX12_TEXTURE_COPY_LOCATION(textureBuffersHeap.m_heap, { heapOffset + mli.offset,{ dxgiFormat, (UINT)mli.width, (UINT)mli.height, 1, (UINT)mli.rowPitch } }), nullptr);
		miplevel++;
	}

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(existingTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
}


/**
 * Get number of bytes occupied by texture in RSX mem
 */
static
size_t getTextureSize(const rsx::texture &texture)
{
	size_t w = texture.width(), h = texture.height();

	int format = texture.format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
	// TODO: Take mipmaps into account
	switch (format)
	{
	case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
	case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
	default:
		LOG_ERROR(RSX, "Unimplemented Texture format : %x", format);
		return 0;
	case CELL_GCM_TEXTURE_B8:
		return w * h;
	case CELL_GCM_TEXTURE_A1R5G5B5:
		return w * h * 2;
	case CELL_GCM_TEXTURE_A4R4G4B4:
		return w * h * 2;
	case CELL_GCM_TEXTURE_R5G6B5:
		return w * h * 2;
	case CELL_GCM_TEXTURE_A8R8G8B8:
		return w * h * 4;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
		return w * h / 6;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
		return w * h / 4;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
		return w * h / 4;
	case CELL_GCM_TEXTURE_G8B8:
		return w * h * 2;
	case CELL_GCM_TEXTURE_R6G5B5:
		return w * h * 2;
	case CELL_GCM_TEXTURE_DEPTH24_D8:
		return w * h * 4;
	case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
		return w * h * 4;
	case CELL_GCM_TEXTURE_DEPTH16:
		return w * h * 2;
	case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
		return w * h * 2;
	case CELL_GCM_TEXTURE_X16:
		return w * h * 2;
	case CELL_GCM_TEXTURE_Y16_X16:
		return w * h * 4;
	case CELL_GCM_TEXTURE_R5G5B5A1:
		return w * h * 2;
	case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
		return w * h * 8;
	case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
		return w * h * 16;
	case CELL_GCM_TEXTURE_X32_FLOAT:
		return w * h * 4;
	case CELL_GCM_TEXTURE_D1R5G5B5:
		return w * h * 2;
	case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
		return w * h * 4;
	case CELL_GCM_TEXTURE_D8R8G8B8:
		return w * h * 4;
	case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
		return w * h * 4;
	case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
		return w * h * 4;
	}
}

size_t D3D12GSRender::UploadTextures(ID3D12GraphicsCommandList *cmdlist, size_t descriptorIndex)
{
	size_t usedTexture = 0;

	for (u32 i = 0; i < rsx::limits::textures_count; ++i)
	{
		if (!textures[i].enabled()) continue;
		size_t w = textures[i].width(), h = textures[i].height();
		if (!w || !h) continue;

		const u32 texaddr = rsx::get_address(textures[i].offset(), textures[i].location());

		int format = textures[i].format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
		DXGI_FORMAT dxgiFormat = getTextureDXGIFormat(format);
		bool is_swizzled = !(textures[i].format() & CELL_GCM_TEXTURE_LN);

		ID3D12Resource *vramTexture;
		std::unordered_map<u32, ID3D12Resource* >::const_iterator ItRTT = m_rtts.m_renderTargets.find(texaddr);
		std::pair<TextureEntry, ComPtr<ID3D12Resource> > *cachedTex = m_textureCache.findDataIfAvailable(texaddr);
		bool isRenderTarget = false;
		if (ItRTT != m_rtts.m_renderTargets.end())
		{
			vramTexture = ItRTT->second;
			isRenderTarget = true;
		}
		else if (cachedTex != nullptr && (cachedTex->first == TextureEntry(format, w, h, textures[i].mipmap())))
		{
			if (cachedTex->first.m_isDirty)
			{
				updateExistingTexture(textures[i], cmdlist, m_textureUploadData, cachedTex->second.Get());
				m_textureCache.protectData(texaddr, texaddr, getTextureSize(textures[i]));
			}
			vramTexture = cachedTex->second.Get();
		}
		else
		{
			if (cachedTex != nullptr)
				getCurrentResourceStorage().m_dirtyTextures.push_back(m_textureCache.removeFromCache(texaddr));
			ComPtr<ID3D12Resource> tex = uploadSingleTexture(textures[i], m_device.Get(), cmdlist, m_textureUploadData);
			vramTexture = tex.Get();
			m_textureCache.storeAndProtectData(texaddr, texaddr, getTextureSize(textures[i]), format, w, h, textures[i].mipmap(), tex);
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Format = dxgiFormat;
		srvDesc.Texture2D.MipLevels = textures[i].mipmap();

		switch (format)
		{
		case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
		case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
		case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
		case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
		default:
			LOG_ERROR(RSX, "Unimplemented Texture format : %x", format);
			break;
		case CELL_GCM_TEXTURE_B8:
			srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0);
			break;
		case CELL_GCM_TEXTURE_A1R5G5B5:
		case CELL_GCM_TEXTURE_A4R4G4B4:
		case CELL_GCM_TEXTURE_R5G6B5:
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			break;
		case CELL_GCM_TEXTURE_A8R8G8B8:
		{


			u8 remap_a = textures[i].remap() & 0x3;
			u8 remap_r = (textures[i].remap() >> 2) & 0x3;
			u8 remap_g = (textures[i].remap() >> 4) & 0x3;
			u8 remap_b = (textures[i].remap() >> 6) & 0x3;
			if (isRenderTarget)
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

				srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
					RemapValue[remap_r],
					RemapValue[remap_g],
					RemapValue[remap_b],
					RemapValue[remap_a]);
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

				srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
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
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			break;
		case CELL_GCM_TEXTURE_D8R8G8B8:
		{
			const int RemapValue[4] =
			{
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3,
				D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1
			};

			u8 remap_a = textures[i].remap() & 0x3;
			u8 remap_r = (textures[i].remap() >> 2) & 0x3;
			u8 remap_g = (textures[i].remap() >> 4) & 0x3;
			u8 remap_b = (textures[i].remap() >> 6) & 0x3;

			srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
				RemapValue[remap_a],
				RemapValue[remap_r],
				RemapValue[remap_g],
				RemapValue[remap_b]);
			break;
		}
		case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			break;
		case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			break;
		case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			break;
		}

		m_device->CreateShaderResourceView(vramTexture, &srvDesc,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(getCurrentResourceStorage().m_descriptorsHeap->GetCPUDescriptorHandleForHeapStart())
			.Offset((UINT)descriptorIndex + (UINT)usedTexture, g_descriptorStrideSRVCBVUAV));

		if (getCurrentResourceStorage().m_currentSamplerIndex + 16 > 2048)
		{
			getCurrentResourceStorage().m_samplerDescriptorHeapIndex = 1;
			getCurrentResourceStorage().m_currentSamplerIndex = 0;
		}
		m_device->CreateSampler(&getSamplerDesc(textures[i]),
			CD3DX12_CPU_DESCRIPTOR_HANDLE(getCurrentResourceStorage().m_samplerDescriptorHeap[getCurrentResourceStorage().m_samplerDescriptorHeapIndex]->GetCPUDescriptorHandleForHeapStart())
			.Offset((UINT)getCurrentResourceStorage().m_currentSamplerIndex + (UINT)usedTexture, g_descriptorStrideSamplers));

		usedTexture++;
	}

	return usedTexture;
}

#endif