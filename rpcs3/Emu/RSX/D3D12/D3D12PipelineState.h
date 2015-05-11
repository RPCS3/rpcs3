#pragma once
#if defined (DX12_SUPPORT)

#include <d3d12.h>
#include "Emu/RSX/RSXFragmentProgram.h"
#include "Emu/RSX/RSXVertexProgram.h"

ID3D12PipelineState *getGraphicPipelineState(ID3D12Device *device, RSXVertexProgram *vertexShader, RSXFragmentProgram *fragmentShader);

#endif