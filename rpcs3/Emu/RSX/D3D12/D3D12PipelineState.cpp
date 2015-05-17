#include "stdafx.h"
#if defined (DX12_SUPPORT)

#include "D3D12PipelineState.h"
#include <d3dcompiler.h>

#pragma comment (lib, "d3dcompiler.lib")

#define TO_STRING(x) #x

void Shader::Compile(const std::string &code, SHADER_TYPE st)
{	HRESULT hr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
	switch (st)
	{
	case SHADER_TYPE::SHADER_TYPE_VERTEX:
		hr = D3DCompile(code.c_str(), code.size(), "VertexProgram.hlsl", nullptr, nullptr, "main", "vs_5_0", 0, 0, &bytecode, errorBlob.GetAddressOf());
		if (hr != S_OK)
			LOG_ERROR(RSX, "VS build failed:%s", errorBlob->GetBufferPointer());
		break;
	case SHADER_TYPE::SHADER_TYPE_FRAGMENT:
		hr = D3DCompile(code.c_str(), code.size(), "FragmentProgram.hlsl", nullptr, nullptr, "main", "ps_5_0", 0, 0, &bytecode, errorBlob.GetAddressOf());
		if (hr != S_OK)
			LOG_ERROR(RSX, "FS build failed:%s", errorBlob->GetBufferPointer());
		break;
	}
}


#endif