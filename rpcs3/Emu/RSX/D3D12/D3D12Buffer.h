#pragma once
#if defined(DX12_SUPPORT)
#include <d3d12.h>
#include "Emu/Memory/vm.h"
#include "Emu/RSX/RSXThread.h"

std::vector<D3D12_INPUT_ELEMENT_DESC> getIALayout(ID3D12Device *device, bool indexedDraw, const RSXVertexData *vertexData);

#endif