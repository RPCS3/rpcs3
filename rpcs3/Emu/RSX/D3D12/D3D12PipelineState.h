#pragma once
#if defined (DX12_SUPPORT)

#include <d3d12.h>
#include "Emu/RSX/RSXFragmentProgram.h"
#include "Emu/RSX/RSXVertexProgram.h"

class D3D12PipelineState
{
	ID3D12PipelineState *m_pipelineStateObject;
	ID3D12RootSignature *m_rootSignature;
public:
	D3D12PipelineState(ID3D12Device *device, RSXVertexProgram *vertexShader, RSXFragmentProgram *fragmentShader);
	~D3D12PipelineState();
};

#endif