#include "stdafx.h"
#if defined(DX12_SUPPORT)
#include "D3D12GSRender.h"
// For clarity this code deals with texture but belongs to D3D12GSRender class

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

		// Upload at each iteration to take advantage of overlapping transfer
		ID3D12GraphicsCommandList *commandList;
		check(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_perFrameStorage.m_textureUploadCommandAllocator, nullptr, IID_PPV_ARGS(&commandList)));

		DXGI_FORMAT dxgiFormat;
		size_t pixelSize;
		int format = m_textures[i].GetFormat() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
		switch (format)
		{
		default:
			dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
			pixelSize = 4;
			break;
		case CELL_GCM_TEXTURE_B8:
			dxgiFormat = DXGI_FORMAT_R8_UNORM;
			pixelSize = 1;
			break;
		}

		ID3D12Resource *Texture, *vramTexture;
		size_t textureSize = w * h * 4;
		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		textureDesc.Width = textureSize;
		textureDesc.Height = 1;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.MipLevels = 1;
		textureDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		check(m_device->CreatePlacedResource(
			m_perFrameStorage.m_uploadTextureHeap,
			m_perFrameStorage.m_currentStorageOffset,
			&textureDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&Texture)
			));

		const u32 texaddr = GetAddress(m_textures[i].GetOffset(), m_textures[i].GetLocation());
		auto pixels = vm::get_ptr<const u8>(texaddr);
		void *textureData;
		check(Texture->Map(0, nullptr, (void**)&textureData));

		// Multiple of 256
		size_t rowPitch = m_textures[i].GetWidth() * pixelSize;
		rowPitch = (rowPitch + 255) & ~255;
		// Upload with correct rowpitch
		for (unsigned row = 0; row < m_textures[i].GetHeight(); row++)
			streamToBuffer((char*)textureData + row * rowPitch, (char*)pixels + row * m_textures[i].m_pitch, m_textures[i].m_pitch);
		Texture->Unmap(0, nullptr);

		D3D12_RESOURCE_DESC vramTextureDesc = {};
		vramTextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		vramTextureDesc.Width = m_textures[i].GetWidth();
		vramTextureDesc.Height = m_textures[i].GetHeight();
		vramTextureDesc.Format = dxgiFormat;
		vramTextureDesc.DepthOrArraySize = 1;
		vramTextureDesc.SampleDesc.Count = 1;
		vramTextureDesc.MipLevels = 1;
		check(m_device->CreatePlacedResource(
			m_perFrameStorage.m_textureStorage,
			m_perFrameStorage.m_currentStorageOffset,
			&vramTextureDesc,
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

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Format = dxgiFormat;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0);
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
		Handle.ptr += (m_perFrameStorage.m_currentTextureIndex + usedTexture) * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_device->CreateSampler(&samplerDesc, Handle);

		commandList->Close();
		m_commandQueueGraphic->ExecuteCommandLists(1, (ID3D12CommandList**)&commandList);
		m_perFrameStorage.m_inflightCommandList.push_back(commandList);

		usedTexture++;
	}

	return usedTexture;
}

#endif