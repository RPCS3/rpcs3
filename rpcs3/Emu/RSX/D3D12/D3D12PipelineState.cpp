#include "stdafx_d3d12.h"
#ifdef _WIN32
#include <d3dcompiler.h>
#include "D3D12PipelineState.h"
#include "D3D12GSRender.h"

#pragma comment (lib, "d3dcompiler.lib")

#define TO_STRING(x) #x

void Shader::Compile(const std::string &code, SHADER_TYPE st)
{
	HRESULT hr;
	ComPtr<ID3DBlob> errorBlob;
	UINT compileFlags;
	if (Ini.GSDebugOutputEnable.GetValue())
		compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
	else
		compileFlags = 0;
	switch (st)
	{
	case SHADER_TYPE::SHADER_TYPE_VERTEX:
		hr = D3DCompile(code.c_str(), code.size(), "VertexProgram.hlsl", nullptr, nullptr, "main", "vs_5_0", compileFlags, 0, &bytecode, errorBlob.GetAddressOf());
		if (hr != S_OK)
			LOG_ERROR(RSX, "VS build failed:%s", errorBlob->GetBufferPointer());
		break;
	case SHADER_TYPE::SHADER_TYPE_FRAGMENT:
		hr = D3DCompile(code.c_str(), code.size(), "FragmentProgram.hlsl", nullptr, nullptr, "main", "ps_5_0", compileFlags, 0, &bytecode, errorBlob.GetAddressOf());
		if (hr != S_OK)
			LOG_ERROR(RSX, "FS build failed:%s", errorBlob->GetBufferPointer());
		break;
	}
}

bool D3D12GSRender::LoadProgram()
{
	RSXVertexProgram vertex_program;
	u32 transform_program_start = rsx::method_registers[NV4097_SET_TRANSFORM_PROGRAM_START];
	vertex_program.data.reserve((512 - transform_program_start) * 4);

	for (int i = transform_program_start; i < 512; ++i)
	{
		vertex_program.data.resize((i - transform_program_start) * 4 + 4);
		memcpy(vertex_program.data.data() + (i - transform_program_start) * 4, transform_program + i * 4, 4 * sizeof(u32));

		D3 d3;
		d3.HEX = transform_program[i * 4 + 3];

		if (d3.end)
			break;
	}

	u32 shader_program = rsx::method_registers[NV4097_SET_SHADER_PROGRAM];
	fragment_program.offset = shader_program & ~0x3;
	fragment_program.addr = rsx::get_address(fragment_program.offset, (shader_program & 0x3) - 1);
	fragment_program.ctrl = rsx::method_registers[NV4097_SET_SHADER_CONTROL];

	D3D12PipelineProperties prop = {};
	switch (draw_mode)
	{
	case CELL_GCM_PRIMITIVE_POINTS:
		prop.Topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		break;
	case CELL_GCM_PRIMITIVE_LINES:
	case CELL_GCM_PRIMITIVE_LINE_LOOP:
	case CELL_GCM_PRIMITIVE_LINE_STRIP:
		prop.Topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		break;
	case CELL_GCM_PRIMITIVE_TRIANGLES:
	case CELL_GCM_PRIMITIVE_TRIANGLE_STRIP:
	case CELL_GCM_PRIMITIVE_TRIANGLE_FAN:
		prop.Topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		break;
	case CELL_GCM_PRIMITIVE_QUADS:
	case CELL_GCM_PRIMITIVE_QUAD_STRIP:
	case CELL_GCM_PRIMITIVE_POLYGON:
	default:
		//		LOG_ERROR(RSX, "Unsupported primitive type");
		prop.Topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		break;
	}

	static D3D12_BLEND_DESC CD3D12_BLEND_DESC =
	{
		FALSE,
		FALSE,
		{
			FALSE,FALSE,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP,
		D3D12_COLOR_WRITE_ENABLE_ALL,
		}
	};
	prop.Blend = CD3D12_BLEND_DESC;

	if (rsx::method_registers[NV4097_SET_BLEND_ENABLE])
	{
		prop.Blend.RenderTarget[0].BlendEnable = true;

		if (rsx::method_registers[NV4097_SET_BLEND_ENABLE_MRT] & 0x2)
			prop.Blend.RenderTarget[1].BlendEnable = true;
		if (rsx::method_registers[NV4097_SET_BLEND_ENABLE_MRT] & 0x4)
			prop.Blend.RenderTarget[2].BlendEnable = true;
		if (rsx::method_registers[NV4097_SET_BLEND_ENABLE_MRT] & 0x8)
			prop.Blend.RenderTarget[3].BlendEnable = true;

		prop.Blend.RenderTarget[0].BlendOp = getBlendOp(rsx::method_registers[NV4097_SET_BLEND_EQUATION] & 0xFFFF);
		prop.Blend.RenderTarget[0].BlendOpAlpha = getBlendOp(rsx::method_registers[NV4097_SET_BLEND_EQUATION] >> 16);

		if (rsx::method_registers[NV4097_SET_BLEND_ENABLE_MRT] & 0x2)
		{
			prop.Blend.RenderTarget[1].BlendOp = getBlendOp(rsx::method_registers[NV4097_SET_BLEND_EQUATION] & 0xFFFF);
			prop.Blend.RenderTarget[1].BlendOpAlpha = getBlendOp(rsx::method_registers[NV4097_SET_BLEND_EQUATION] >> 16);
		}

		if (rsx::method_registers[NV4097_SET_BLEND_ENABLE_MRT] & 0x4)
		{
			prop.Blend.RenderTarget[2].BlendOp = getBlendOp(rsx::method_registers[NV4097_SET_BLEND_EQUATION] & 0xFFFF);
			prop.Blend.RenderTarget[2].BlendOpAlpha = getBlendOp(rsx::method_registers[NV4097_SET_BLEND_EQUATION] >> 16);
		}

		if (rsx::method_registers[NV4097_SET_BLEND_ENABLE_MRT] & 0x8)
		{
			prop.Blend.RenderTarget[3].BlendOp = getBlendOp(rsx::method_registers[NV4097_SET_BLEND_EQUATION] & 0xFFFF);
			prop.Blend.RenderTarget[3].BlendOpAlpha = getBlendOp(rsx::method_registers[NV4097_SET_BLEND_EQUATION] >> 16);
		}

		prop.Blend.RenderTarget[0].SrcBlend = getBlendFactor(rsx::method_registers[NV4097_SET_BLEND_FUNC_SFACTOR] & 0xFFFF);
		prop.Blend.RenderTarget[0].DestBlend = getBlendFactor(rsx::method_registers[NV4097_SET_BLEND_FUNC_DFACTOR] & 0xFFFF);
		prop.Blend.RenderTarget[0].SrcBlendAlpha = getBlendFactorAlpha(rsx::method_registers[NV4097_SET_BLEND_FUNC_SFACTOR] >> 16);
		prop.Blend.RenderTarget[0].DestBlendAlpha = getBlendFactorAlpha(rsx::method_registers[NV4097_SET_BLEND_FUNC_DFACTOR] >> 16);

		if (rsx::method_registers[NV4097_SET_BLEND_ENABLE_MRT] & 0x2)
		{
			prop.Blend.RenderTarget[1].SrcBlend = getBlendFactor(rsx::method_registers[NV4097_SET_BLEND_FUNC_SFACTOR] & 0xFFFF);
			prop.Blend.RenderTarget[1].DestBlend = getBlendFactor(rsx::method_registers[NV4097_SET_BLEND_FUNC_DFACTOR] & 0xFFFF);
			prop.Blend.RenderTarget[1].SrcBlendAlpha = getBlendFactorAlpha(rsx::method_registers[NV4097_SET_BLEND_FUNC_SFACTOR] >> 16);
			prop.Blend.RenderTarget[1].DestBlendAlpha = getBlendFactorAlpha(rsx::method_registers[NV4097_SET_BLEND_FUNC_DFACTOR] >> 16);
		}

		if (rsx::method_registers[NV4097_SET_BLEND_ENABLE_MRT] & 0x4)
		{
			prop.Blend.RenderTarget[2].SrcBlend = getBlendFactor(rsx::method_registers[NV4097_SET_BLEND_FUNC_SFACTOR] & 0xFFFF);
			prop.Blend.RenderTarget[2].DestBlend = getBlendFactor(rsx::method_registers[NV4097_SET_BLEND_FUNC_DFACTOR] & 0xFFFF);
			prop.Blend.RenderTarget[2].SrcBlendAlpha = getBlendFactorAlpha(rsx::method_registers[NV4097_SET_BLEND_FUNC_SFACTOR] >> 16);
			prop.Blend.RenderTarget[2].DestBlendAlpha = getBlendFactorAlpha(rsx::method_registers[NV4097_SET_BLEND_FUNC_DFACTOR] >> 16);
		}

		if (rsx::method_registers[NV4097_SET_BLEND_ENABLE_MRT] & 0x8)
		{
			prop.Blend.RenderTarget[3].SrcBlend = getBlendFactor(rsx::method_registers[NV4097_SET_BLEND_FUNC_SFACTOR] & 0xFFFF);
			prop.Blend.RenderTarget[3].DestBlend = getBlendFactor(rsx::method_registers[NV4097_SET_BLEND_FUNC_DFACTOR] & 0xFFFF);
			prop.Blend.RenderTarget[3].SrcBlendAlpha = getBlendFactorAlpha(rsx::method_registers[NV4097_SET_BLEND_FUNC_SFACTOR] >> 16);
			prop.Blend.RenderTarget[3].DestBlendAlpha = getBlendFactorAlpha(rsx::method_registers[NV4097_SET_BLEND_FUNC_DFACTOR] >> 16);
		}
	}

	if (rsx::method_registers[NV4097_SET_LOGIC_OP_ENABLE])
	{
		prop.Blend.RenderTarget[0].LogicOpEnable = true;
		prop.Blend.RenderTarget[0].LogicOp = getLogicOp(rsx::method_registers[NV4097_SET_LOGIC_OP]);
	}

//	if (m_set_blend_color)
	{
		// glBlendColor(m_blend_color_r, m_blend_color_g, m_blend_color_b, m_blend_color_a);
		// checkForGlError("glBlendColor");
	}

	switch (m_surface.depth_format)
	{
	case 0:
		break;
	case CELL_GCM_SURFACE_Z16:
		prop.DepthStencilFormat = DXGI_FORMAT_D16_UNORM;
		break;
	case CELL_GCM_SURFACE_Z24S8:
		prop.DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		break;
	default:
		LOG_ERROR(RSX, "Bad depth format! (%d)", m_surface.depth_format);
		assert(0);
	}

	switch (m_surface.color_format)
	{
	case CELL_GCM_SURFACE_A8R8G8B8:
		prop.RenderTargetsFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	case CELL_GCM_SURFACE_F_W16Z16Y16X16:
		prop.RenderTargetsFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
		break;
	}

	switch (u32 color_target = rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET])
	{
	case CELL_GCM_SURFACE_TARGET_0:
	case CELL_GCM_SURFACE_TARGET_1:
		prop.numMRT = 1;
		break;
	case CELL_GCM_SURFACE_TARGET_MRT1:
		prop.numMRT = 2;
		break;
	case CELL_GCM_SURFACE_TARGET_MRT2:
		prop.numMRT = 3;
		break;
	case CELL_GCM_SURFACE_TARGET_MRT3:
		prop.numMRT = 4;
		break;
	default:
		LOG_ERROR(RSX, "Bad surface color target: %d", color_target);
	}

	prop.DepthStencil.DepthEnable = !!(rsx::method_registers[NV4097_SET_DEPTH_TEST_ENABLE]);
	prop.DepthStencil.DepthWriteMask = !!(rsx::method_registers[NV4097_SET_DEPTH_MASK]) ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	prop.DepthStencil.DepthFunc = getCompareFunc(rsx::method_registers[NV4097_SET_DEPTH_FUNC]);
	prop.DepthStencil.StencilEnable = !!(rsx::method_registers[NV4097_SET_STENCIL_TEST_ENABLE]);
	prop.DepthStencil.StencilReadMask = rsx::method_registers[NV4097_SET_STENCIL_FUNC_MASK];
	prop.DepthStencil.StencilWriteMask = rsx::method_registers[NV4097_SET_STENCIL_MASK];
	prop.DepthStencil.FrontFace.StencilPassOp = getStencilOp(rsx::method_registers[NV4097_SET_STENCIL_OP_ZPASS]);
	prop.DepthStencil.FrontFace.StencilDepthFailOp = getStencilOp(rsx::method_registers[NV4097_SET_STENCIL_OP_ZFAIL]);
	prop.DepthStencil.FrontFace.StencilFailOp = getStencilOp(rsx::method_registers[NV4097_SET_STENCIL_OP_FAIL]);
	prop.DepthStencil.FrontFace.StencilFunc = getCompareFunc(rsx::method_registers[NV4097_SET_STENCIL_FUNC]);

	if (rsx::method_registers[NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE])
	{
		prop.DepthStencil.BackFace.StencilFailOp = getStencilOp(rsx::method_registers[NV4097_SET_BACK_STENCIL_OP_FAIL]);
		prop.DepthStencil.BackFace.StencilFunc = getCompareFunc(rsx::method_registers[NV4097_SET_BACK_STENCIL_FUNC]);
		prop.DepthStencil.BackFace.StencilPassOp = getStencilOp(rsx::method_registers[NV4097_SET_BACK_STENCIL_OP_ZPASS]);
		prop.DepthStencil.BackFace.StencilDepthFailOp = getStencilOp(rsx::method_registers[NV4097_SET_BACK_STENCIL_OP_ZFAIL]);
	}
	else
	{
		prop.DepthStencil.BackFace.StencilPassOp = getStencilOp(rsx::method_registers[NV4097_SET_STENCIL_OP_ZPASS]);
		prop.DepthStencil.BackFace.StencilDepthFailOp = getStencilOp(rsx::method_registers[NV4097_SET_STENCIL_OP_ZFAIL]);
		prop.DepthStencil.BackFace.StencilFailOp = getStencilOp(rsx::method_registers[NV4097_SET_STENCIL_OP_FAIL]);
		prop.DepthStencil.BackFace.StencilFunc = getCompareFunc(rsx::method_registers[NV4097_SET_STENCIL_FUNC]);
	}

	// Sensible default value
	static D3D12_RASTERIZER_DESC CD3D12_RASTERIZER_DESC =
	{
		D3D12_FILL_MODE_SOLID,
		D3D12_CULL_MODE_NONE,
		FALSE,
		D3D12_DEFAULT_DEPTH_BIAS,
		D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
		D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
		TRUE,
		FALSE,
		FALSE,
		0,
		D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
	};
	prop.Rasterization = CD3D12_RASTERIZER_DESC;
	if (rsx::method_registers[NV4097_SET_CULL_FACE_ENABLE])
	{
		switch (rsx::method_registers[NV4097_SET_CULL_FACE])
		{
		case CELL_GCM_FRONT:
			prop.Rasterization.CullMode = D3D12_CULL_MODE_FRONT;
			break;
		case CELL_GCM_BACK:
			prop.Rasterization.CullMode = D3D12_CULL_MODE_BACK;
			break;
		default:
			prop.Rasterization.CullMode = D3D12_CULL_MODE_NONE;
			break;
		}
	}
	else
		prop.Rasterization.CullMode = D3D12_CULL_MODE_NONE;

	switch (rsx::method_registers[NV4097_SET_FRONT_FACE])
	{
	case CELL_GCM_CW:
		prop.Rasterization.FrontCounterClockwise = FALSE;
		break;
	case CELL_GCM_CCW:
		prop.Rasterization.FrontCounterClockwise = TRUE;
		break;
	}

	UINT8 mask = 0;
	mask |= (rsx::method_registers[NV4097_SET_COLOR_MASK] >> 16) & 0xFF ? D3D12_COLOR_WRITE_ENABLE_RED : 0;
	mask |= (rsx::method_registers[NV4097_SET_COLOR_MASK] >> 8) & 0xFF ? D3D12_COLOR_WRITE_ENABLE_GREEN : 0;
	mask |= rsx::method_registers[NV4097_SET_COLOR_MASK] & 0xFF ? D3D12_COLOR_WRITE_ENABLE_BLUE : 0;
	mask |= (rsx::method_registers[NV4097_SET_COLOR_MASK] >> 24) & 0xFF ? D3D12_COLOR_WRITE_ENABLE_ALPHA : 0;
	for (unsigned i = 0; i < prop.numMRT; i++)
		prop.Blend.RenderTarget[i].RenderTargetWriteMask = mask;

	prop.IASet = m_IASet;

	m_PSO = m_cachePSO.getGraphicPipelineState(&vertex_program, &fragment_program, prop, std::make_pair(m_device.Get(), m_rootSignatures));
	return m_PSO != nullptr;
}
#endif