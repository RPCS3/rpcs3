#include "stdafx.h"
#include "stdafx_d3d12.h"
#ifdef _MSC_VER
#include "D3D12PipelineState.h"
#include "D3D12GSRender.h"
#include "Emu/state.h"
#include "D3D12Formats.h"

#define TO_STRING(x) #x

extern pD3DCompile wrapD3DCompile;

void Shader::Compile(const std::string &code, SHADER_TYPE st)
{
	content = code;
	HRESULT hr;
	ComPtr<ID3DBlob> errorBlob;
	UINT compileFlags;
	if (rpcs3::config.rsx.d3d12.debug_output.value())
		compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
	else
		compileFlags = 0;
	switch (st)
	{
	case SHADER_TYPE::SHADER_TYPE_VERTEX:
		hr = wrapD3DCompile(code.c_str(), code.size(), "VertexProgram.hlsl", nullptr, nullptr, "main", "vs_5_0", compileFlags, 0, &bytecode, errorBlob.GetAddressOf());
		if (hr != S_OK)
			LOG_ERROR(RSX, "VS build failed:%s", errorBlob->GetBufferPointer());
		break;
	case SHADER_TYPE::SHADER_TYPE_FRAGMENT:
		hr = wrapD3DCompile(code.c_str(), code.size(), "FragmentProgram.hlsl", nullptr, nullptr, "main", "ps_5_0", compileFlags, 0, &bytecode, errorBlob.GetAddressOf());
		if (hr != S_OK)
			LOG_ERROR(RSX, "FS build failed:%s", errorBlob->GetBufferPointer());
		break;
	}
}

bool D3D12GSRender::load_program()
{
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
	fragment_program.texture_dimensions.clear();

	for (u32 i = 0; i < rsx::limits::textures_count; ++i)
	{
		if (!textures[i].enabled())
			fragment_program.texture_dimensions.push_back(texture_dimension::texture_dimension_2d);
		else if (textures[i].cubemap())
			fragment_program.texture_dimensions.push_back(texture_dimension::texture_dimension_cubemap);
		else
			fragment_program.texture_dimensions.push_back(texture_dimension::texture_dimension_2d);
	}

	D3D12PipelineProperties prop = {};
	prop.Topology = get_primitive_topology_type(draw_mode);

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

		prop.Blend.RenderTarget[0].BlendOp = get_blend_op(rsx::method_registers[NV4097_SET_BLEND_EQUATION] & 0xFFFF);
		prop.Blend.RenderTarget[0].BlendOpAlpha = get_blend_op(rsx::method_registers[NV4097_SET_BLEND_EQUATION] >> 16);

		if (rsx::method_registers[NV4097_SET_BLEND_ENABLE_MRT] & 0x2)
		{
			prop.Blend.RenderTarget[1].BlendOp = get_blend_op(rsx::method_registers[NV4097_SET_BLEND_EQUATION] & 0xFFFF);
			prop.Blend.RenderTarget[1].BlendOpAlpha = get_blend_op(rsx::method_registers[NV4097_SET_BLEND_EQUATION] >> 16);
		}

		if (rsx::method_registers[NV4097_SET_BLEND_ENABLE_MRT] & 0x4)
		{
			prop.Blend.RenderTarget[2].BlendOp = get_blend_op(rsx::method_registers[NV4097_SET_BLEND_EQUATION] & 0xFFFF);
			prop.Blend.RenderTarget[2].BlendOpAlpha = get_blend_op(rsx::method_registers[NV4097_SET_BLEND_EQUATION] >> 16);
		}

		if (rsx::method_registers[NV4097_SET_BLEND_ENABLE_MRT] & 0x8)
		{
			prop.Blend.RenderTarget[3].BlendOp = get_blend_op(rsx::method_registers[NV4097_SET_BLEND_EQUATION] & 0xFFFF);
			prop.Blend.RenderTarget[3].BlendOpAlpha = get_blend_op(rsx::method_registers[NV4097_SET_BLEND_EQUATION] >> 16);
		}

		prop.Blend.RenderTarget[0].SrcBlend = get_blend_factor(rsx::method_registers[NV4097_SET_BLEND_FUNC_SFACTOR] & 0xFFFF);
		prop.Blend.RenderTarget[0].DestBlend = get_blend_factor(rsx::method_registers[NV4097_SET_BLEND_FUNC_DFACTOR] & 0xFFFF);
		prop.Blend.RenderTarget[0].SrcBlendAlpha = get_blend_factor_alpha(rsx::method_registers[NV4097_SET_BLEND_FUNC_SFACTOR] >> 16);
		prop.Blend.RenderTarget[0].DestBlendAlpha = get_blend_factor_alpha(rsx::method_registers[NV4097_SET_BLEND_FUNC_DFACTOR] >> 16);

		if (rsx::method_registers[NV4097_SET_BLEND_ENABLE_MRT] & 0x2)
		{
			prop.Blend.RenderTarget[1].SrcBlend = get_blend_factor(rsx::method_registers[NV4097_SET_BLEND_FUNC_SFACTOR] & 0xFFFF);
			prop.Blend.RenderTarget[1].DestBlend = get_blend_factor(rsx::method_registers[NV4097_SET_BLEND_FUNC_DFACTOR] & 0xFFFF);
			prop.Blend.RenderTarget[1].SrcBlendAlpha = get_blend_factor_alpha(rsx::method_registers[NV4097_SET_BLEND_FUNC_SFACTOR] >> 16);
			prop.Blend.RenderTarget[1].DestBlendAlpha = get_blend_factor_alpha(rsx::method_registers[NV4097_SET_BLEND_FUNC_DFACTOR] >> 16);
		}

		if (rsx::method_registers[NV4097_SET_BLEND_ENABLE_MRT] & 0x4)
		{
			prop.Blend.RenderTarget[2].SrcBlend = get_blend_factor(rsx::method_registers[NV4097_SET_BLEND_FUNC_SFACTOR] & 0xFFFF);
			prop.Blend.RenderTarget[2].DestBlend = get_blend_factor(rsx::method_registers[NV4097_SET_BLEND_FUNC_DFACTOR] & 0xFFFF);
			prop.Blend.RenderTarget[2].SrcBlendAlpha = get_blend_factor_alpha(rsx::method_registers[NV4097_SET_BLEND_FUNC_SFACTOR] >> 16);
			prop.Blend.RenderTarget[2].DestBlendAlpha = get_blend_factor_alpha(rsx::method_registers[NV4097_SET_BLEND_FUNC_DFACTOR] >> 16);
		}

		if (rsx::method_registers[NV4097_SET_BLEND_ENABLE_MRT] & 0x8)
		{
			prop.Blend.RenderTarget[3].SrcBlend = get_blend_factor(rsx::method_registers[NV4097_SET_BLEND_FUNC_SFACTOR] & 0xFFFF);
			prop.Blend.RenderTarget[3].DestBlend = get_blend_factor(rsx::method_registers[NV4097_SET_BLEND_FUNC_DFACTOR] & 0xFFFF);
			prop.Blend.RenderTarget[3].SrcBlendAlpha = get_blend_factor_alpha(rsx::method_registers[NV4097_SET_BLEND_FUNC_SFACTOR] >> 16);
			prop.Blend.RenderTarget[3].DestBlendAlpha = get_blend_factor_alpha(rsx::method_registers[NV4097_SET_BLEND_FUNC_DFACTOR] >> 16);
		}
	}

	if (rsx::method_registers[NV4097_SET_LOGIC_OP_ENABLE])
	{
		prop.Blend.RenderTarget[0].LogicOpEnable = true;
		prop.Blend.RenderTarget[0].LogicOp = get_logic_op(rsx::method_registers[NV4097_SET_LOGIC_OP]);
	}

//	if (m_set_blend_color)
	{
		// glBlendColor(m_blend_color_r, m_blend_color_g, m_blend_color_b, m_blend_color_a);
		// checkForGlError("glBlendColor");
	}
	prop.DepthStencilFormat = get_depth_stencil_surface_format(m_surface.depth_format);
	prop.RenderTargetsFormat = get_color_surface_format(m_surface.color_format);

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
		break;
	}

	prop.DepthStencil.DepthEnable = !!(rsx::method_registers[NV4097_SET_DEPTH_TEST_ENABLE]);
	prop.DepthStencil.DepthWriteMask = !!(rsx::method_registers[NV4097_SET_DEPTH_MASK]) ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	prop.DepthStencil.DepthFunc = get_compare_func(rsx::method_registers[NV4097_SET_DEPTH_FUNC]);
	prop.DepthStencil.StencilEnable = !!(rsx::method_registers[NV4097_SET_STENCIL_TEST_ENABLE]);
	prop.DepthStencil.StencilReadMask = rsx::method_registers[NV4097_SET_STENCIL_FUNC_MASK];
	prop.DepthStencil.StencilWriteMask = rsx::method_registers[NV4097_SET_STENCIL_MASK];
	prop.DepthStencil.FrontFace.StencilPassOp = get_stencil_op(rsx::method_registers[NV4097_SET_STENCIL_OP_ZPASS]);
	prop.DepthStencil.FrontFace.StencilDepthFailOp = get_stencil_op(rsx::method_registers[NV4097_SET_STENCIL_OP_ZFAIL]);
	prop.DepthStencil.FrontFace.StencilFailOp = get_stencil_op(rsx::method_registers[NV4097_SET_STENCIL_OP_FAIL]);
	prop.DepthStencil.FrontFace.StencilFunc = get_compare_func(rsx::method_registers[NV4097_SET_STENCIL_FUNC]);

	if (rsx::method_registers[NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE])
	{
		prop.DepthStencil.BackFace.StencilFailOp = get_stencil_op(rsx::method_registers[NV4097_SET_BACK_STENCIL_OP_FAIL]);
		prop.DepthStencil.BackFace.StencilFunc = get_compare_func(rsx::method_registers[NV4097_SET_BACK_STENCIL_FUNC]);
		prop.DepthStencil.BackFace.StencilPassOp = get_stencil_op(rsx::method_registers[NV4097_SET_BACK_STENCIL_OP_ZPASS]);
		prop.DepthStencil.BackFace.StencilDepthFailOp = get_stencil_op(rsx::method_registers[NV4097_SET_BACK_STENCIL_OP_ZFAIL]);
	}
	else
	{
		prop.DepthStencil.BackFace.StencilPassOp = get_stencil_op(rsx::method_registers[NV4097_SET_STENCIL_OP_ZPASS]);
		prop.DepthStencil.BackFace.StencilDepthFailOp = get_stencil_op(rsx::method_registers[NV4097_SET_STENCIL_OP_ZFAIL]);
		prop.DepthStencil.BackFace.StencilFailOp = get_stencil_op(rsx::method_registers[NV4097_SET_STENCIL_OP_FAIL]);
		prop.DepthStencil.BackFace.StencilFunc = get_compare_func(rsx::method_registers[NV4097_SET_STENCIL_FUNC]);
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

	prop.Rasterization.FrontCounterClockwise = get_front_face_ccw(rsx::method_registers[NV4097_SET_FRONT_FACE]);

	UINT8 mask = 0;
	mask |= (rsx::method_registers[NV4097_SET_COLOR_MASK] >> 16) & 0xFF ? D3D12_COLOR_WRITE_ENABLE_RED : 0;
	mask |= (rsx::method_registers[NV4097_SET_COLOR_MASK] >> 8) & 0xFF ? D3D12_COLOR_WRITE_ENABLE_GREEN : 0;
	mask |= rsx::method_registers[NV4097_SET_COLOR_MASK] & 0xFF ? D3D12_COLOR_WRITE_ENABLE_BLUE : 0;
	mask |= (rsx::method_registers[NV4097_SET_COLOR_MASK] >> 24) & 0xFF ? D3D12_COLOR_WRITE_ENABLE_ALPHA : 0;
	for (unsigned i = 0; i < prop.numMRT; i++)
		prop.Blend.RenderTarget[i].RenderTargetWriteMask = mask;

	prop.IASet = m_IASet;
	if (!!rsx::method_registers[NV4097_SET_RESTART_INDEX_ENABLE])
		prop.CutValue = ((rsx::method_registers[NV4097_SET_INDEX_ARRAY_DMA] >> 4) == CELL_GCM_DRAW_INDEX_ARRAY_TYPE_32) ?
			D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF : D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF;

	m_current_pso = m_pso_cache.getGraphicPipelineState(&vertex_program, &fragment_program, prop, std::make_pair(m_device.Get(), m_root_signatures));
	return m_current_pso != nullptr;
}

std::pair<std::string, std::string> D3D12GSRender::get_programs() const
{
	return std::make_pair(m_pso_cache.get_transform_program(vertex_program)->content, m_pso_cache.get_shader_program(fragment_program)->content);
}
#endif
