#include "stdafx.h"
#if defined (DX12_SUPPORT)

#include "D3D12PipelineState.h"
#include <d3dcompiler.h>
#include "D3D12GSRender.h"

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




bool D3D12GSRender::LoadProgram()
{
	if (!m_cur_fragment_prog)
	{
		LOG_WARNING(RSX, "LoadProgram: m_cur_shader_prog == NULL");
		return false;
	}

	m_cur_fragment_prog->ctrl = m_shader_ctrl;

	if (!m_cur_vertex_prog)
	{
		LOG_WARNING(RSX, "LoadProgram: m_cur_vertex_prog == NULL");
		return false;
	}

	D3D12PipelineProperties prop = {};
	switch (m_draw_mode - 1)
	{
	case GL_POINTS:
		prop.Topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		break;
	case GL_LINES:
	case GL_LINE_LOOP:
	case GL_LINE_STRIP:
		prop.Topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		break;
	case GL_TRIANGLES:
	case GL_TRIANGLE_STRIP:
	case GL_TRIANGLE_FAN:
		prop.Topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		break;
	case GL_QUADS:
	case GL_QUAD_STRIP:
	case GL_POLYGON:
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

	if (m_set_blend)
	{
		prop.Blend.RenderTarget[0].BlendEnable = true;

		if (m_set_blend_mrt1)
			prop.Blend.RenderTarget[1].BlendEnable = true;
		if (m_set_blend_mrt2)
			prop.Blend.RenderTarget[2].BlendEnable = true;
		if (m_set_blend_mrt3)
			prop.Blend.RenderTarget[3].BlendEnable = true;
	}

	if (m_set_blend_equation)
	{
		prop.Blend.RenderTarget[0].BlendOp = getBlendOp(m_blend_equation_rgb);
		prop.Blend.RenderTarget[0].BlendOpAlpha = getBlendOp(m_blend_equation_alpha);

		if (m_set_blend_mrt1)
		{
			prop.Blend.RenderTarget[1].BlendOp = getBlendOp(m_blend_equation_rgb);
			prop.Blend.RenderTarget[1].BlendOpAlpha = getBlendOp(m_blend_equation_alpha);
		}

		if (m_set_blend_mrt2)
		{
			prop.Blend.RenderTarget[2].BlendOp = getBlendOp(m_blend_equation_rgb);
			prop.Blend.RenderTarget[2].BlendOpAlpha = getBlendOp(m_blend_equation_alpha);
		}

		if (m_set_blend_mrt3)
		{
			prop.Blend.RenderTarget[3].BlendOp = getBlendOp(m_blend_equation_rgb);
			prop.Blend.RenderTarget[3].BlendOpAlpha = getBlendOp(m_blend_equation_alpha);
		}
	}

	if (m_set_blend_sfactor && m_set_blend_dfactor)
	{
		prop.Blend.RenderTarget[0].SrcBlend = getBlendFactor(m_blend_sfactor_rgb);
		prop.Blend.RenderTarget[0].DestBlend = getBlendFactor(m_blend_dfactor_rgb);
		prop.Blend.RenderTarget[0].SrcBlendAlpha = getBlendFactor(m_blend_sfactor_alpha);
		prop.Blend.RenderTarget[0].DestBlendAlpha = getBlendFactor(m_blend_dfactor_alpha);

		if (m_set_blend_mrt1)
		{
			prop.Blend.RenderTarget[1].SrcBlend = getBlendFactor(m_blend_sfactor_rgb);
			prop.Blend.RenderTarget[1].DestBlend = getBlendFactor(m_blend_dfactor_rgb);
			prop.Blend.RenderTarget[1].SrcBlendAlpha = getBlendFactor(m_blend_sfactor_alpha);
			prop.Blend.RenderTarget[1].DestBlendAlpha = getBlendFactor(m_blend_dfactor_alpha);
		}

		if (m_set_blend_mrt2)
		{
			prop.Blend.RenderTarget[2].SrcBlend = getBlendFactor(m_blend_sfactor_rgb);
			prop.Blend.RenderTarget[2].DestBlend = getBlendFactor(m_blend_dfactor_rgb);
			prop.Blend.RenderTarget[2].SrcBlendAlpha = getBlendFactor(m_blend_sfactor_alpha);
			prop.Blend.RenderTarget[2].DestBlendAlpha = getBlendFactor(m_blend_dfactor_alpha);
		}

		if (m_set_blend_mrt3)
		{
			prop.Blend.RenderTarget[3].SrcBlend = getBlendFactor(m_blend_sfactor_rgb);
			prop.Blend.RenderTarget[3].DestBlend = getBlendFactor(m_blend_dfactor_rgb);
			prop.Blend.RenderTarget[3].SrcBlendAlpha = getBlendFactor(m_blend_sfactor_alpha);
			prop.Blend.RenderTarget[3].DestBlendAlpha = getBlendFactor(m_blend_dfactor_alpha);
		}
	}

	if (m_set_logic_op)
	{
		prop.Blend.RenderTarget[0].LogicOpEnable = true;
		prop.Blend.RenderTarget[0].LogicOp = getLogicOp(m_logic_op);
	}

	if (m_set_blend_color)
	{
		//		glBlendColor(m_blend_color_r, m_blend_color_g, m_blend_color_b, m_blend_color_a);
		//		checkForGlError("glBlendColor");
	}

	switch (m_surface_depth_format)
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
		LOG_ERROR(RSX, "Bad depth format! (%d)", m_surface_depth_format);
		assert(0);
	}

	switch (m_surface_color_format)
	{
	case CELL_GCM_SURFACE_A8R8G8B8:
		prop.RenderTargetsFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	case CELL_GCM_SURFACE_F_W16Z16Y16X16:
		prop.RenderTargetsFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
		break;
	}

	switch (m_surface_color_target)
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
		LOG_ERROR(RSX, "Bad surface color target: %d", m_surface_color_target);
	}

	prop.DepthStencil.DepthEnable = m_set_depth_test;
	prop.DepthStencil.DepthWriteMask = m_depth_mask ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	prop.DepthStencil.DepthFunc = getCompareFunc(m_depth_func);
	prop.DepthStencil.StencilEnable = m_set_stencil_test;
	prop.DepthStencil.StencilReadMask = m_stencil_func_mask;
	prop.DepthStencil.StencilWriteMask = m_stencil_mask;
	prop.DepthStencil.FrontFace.StencilPassOp = getStencilOp(m_stencil_zpass);
	prop.DepthStencil.FrontFace.StencilDepthFailOp = getStencilOp(m_stencil_zfail);
	prop.DepthStencil.FrontFace.StencilFailOp = getStencilOp(m_stencil_fail);
	prop.DepthStencil.FrontFace.StencilFunc = getCompareFunc(m_stencil_func);

	if (m_set_two_sided_stencil_test_enable)
	{
		prop.DepthStencil.BackFace.StencilFailOp = getStencilOp(m_back_stencil_fail);
		prop.DepthStencil.BackFace.StencilFunc = getCompareFunc(m_back_stencil_func);
		prop.DepthStencil.BackFace.StencilPassOp = getStencilOp(m_back_stencil_zpass);
		prop.DepthStencil.BackFace.StencilDepthFailOp = getStencilOp(m_back_stencil_zfail);
	}
	else
	{
		prop.DepthStencil.BackFace.StencilPassOp = getStencilOp(m_stencil_zpass);
		prop.DepthStencil.BackFace.StencilDepthFailOp = getStencilOp(m_stencil_zfail);
		prop.DepthStencil.BackFace.StencilFailOp = getStencilOp(m_stencil_fail);
		prop.DepthStencil.BackFace.StencilFunc = getCompareFunc(m_stencil_func);
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
	switch (m_set_cull_face)
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

	switch (m_front_face)
	{
	case CELL_GCM_CW:
		prop.Rasterization.FrontCounterClockwise = FALSE;
		break;
	case CELL_GCM_CCW:
		prop.Rasterization.FrontCounterClockwise = TRUE;
		break;
	}

	if (m_set_color_mask)
		prop.SampleMask = m_color_mask_r | (m_color_mask_g << 1) | (m_color_mask_b << 2) | (m_color_mask_a << 3);
	else
		prop.SampleMask = UINT_MAX;

	prop.IASet = m_IASet;

	m_PSO = m_cachePSO.getGraphicPipelineState(m_cur_vertex_prog, m_cur_fragment_prog, prop, std::make_pair(m_device, m_rootSignatures));
	return m_PSO != nullptr;
}


#endif