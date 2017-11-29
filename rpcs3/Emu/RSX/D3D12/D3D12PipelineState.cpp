#ifdef _MSC_VER
#include "stdafx.h"
#include "stdafx_d3d12.h"
#include "D3D12PipelineState.h"
#include "D3D12GSRender.h"
#include "D3D12Formats.h"
#include "../rsx_methods.h"
#include "../rsx_utils.h"

#define TO_STRING(x) #x

extern pD3DCompile wrapD3DCompile;

void Shader::Compile(const std::string &code, SHADER_TYPE st)
{
	content = code;
	HRESULT hr;
	ComPtr<ID3DBlob> errorBlob;
	UINT compileFlags;
	if (g_cfg.video.debug_output)
		compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
	else
		compileFlags = 0;
	switch (st)
	{
	case SHADER_TYPE::SHADER_TYPE_VERTEX:
		hr = wrapD3DCompile(code.c_str(), code.size(), "VertexProgram.hlsl", nullptr, nullptr, "main", "vs_5_0", compileFlags, 0, &bytecode, errorBlob.GetAddressOf());
		if (hr != S_OK)
			LOG_ERROR(RSX, "VS build failed:%s", (char*)errorBlob->GetBufferPointer());
		break;
	case SHADER_TYPE::SHADER_TYPE_FRAGMENT:
		hr = wrapD3DCompile(code.c_str(), code.size(), "FragmentProgram.hlsl", nullptr, nullptr, "main", "ps_5_0", compileFlags, 0, &bytecode, errorBlob.GetAddressOf());
		if (hr != S_OK)
			LOG_ERROR(RSX, "FS build failed:%s", (char*)errorBlob->GetBufferPointer());
		break;
	}
}

void D3D12GSRender::load_program()
{
	auto rtt_lookup_func = [this](u32 texaddr, rsx::fragment_texture&, bool is_depth) -> std::tuple<bool, u16>
	{
		ID3D12Resource *surface = nullptr;
		if (!is_depth)
			surface = m_rtts.get_texture_from_render_target_if_applicable(texaddr);
		else
			surface = m_rtts.get_texture_from_depth_stencil_if_applicable(texaddr);

		if (!surface) return std::make_tuple(false, 0);
		
		D3D12_RESOURCE_DESC desc = surface->GetDesc();
		u16 native_pitch = get_dxgi_texel_size(desc.Format) * (u16)desc.Width;
		return std::make_tuple(true, native_pitch);
	};

	get_current_vertex_program();
	get_current_fragment_program_legacy(rtt_lookup_func);

	if (!current_fragment_program.valid)
		return;

	D3D12PipelineProperties prop = {};
	prop.Topology = get_primitive_topology_type(rsx::method_registers.current_draw_clause.primitive);

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

	if (rsx::method_registers.blend_enabled())
	{
		//We can use the d3d blend factor as long as !(rgb_factor == alpha && a_factor == color)
		rsx::blend_factor sfactor_rgb = rsx::method_registers.blend_func_sfactor_rgb();
		rsx::blend_factor dfactor_rgb = rsx::method_registers.blend_func_dfactor_rgb();
		rsx::blend_factor sfactor_a = rsx::method_registers.blend_func_sfactor_a();
		rsx::blend_factor dfactor_a = rsx::method_registers.blend_func_dfactor_a();

		D3D12_BLEND d3d_sfactor_rgb = get_blend_factor(sfactor_rgb);
		D3D12_BLEND d3d_dfactor_rgb = get_blend_factor(dfactor_rgb);;
		D3D12_BLEND d3d_sfactor_alpha = get_blend_factor_alpha(sfactor_a);
		D3D12_BLEND d3d_dfactor_alpha = get_blend_factor_alpha(dfactor_a);
		
		auto BlendColor = rsx::get_constant_blend_colors();
		bool color_blend_possible = true;

		if (sfactor_rgb == rsx::blend_factor::constant_alpha ||
			dfactor_rgb == rsx::blend_factor::constant_alpha)
		{
			if (sfactor_rgb == rsx::blend_factor::constant_color ||
				dfactor_rgb == rsx::blend_factor::constant_color)
			{
				//Color information will be destroyed
				color_blend_possible = false;
			}
			else
			{
				//All components are alpha.
				//If an alpha factor refers to constant_color, it only refers to the alpha component, so no need to replace it
				BlendColor[0] = BlendColor[1] = BlendColor[2] = BlendColor[3];
			}
		}

		if (!color_blend_possible)
		{
			LOG_ERROR(RSX, "The constant_color blend factor combination defined is not supported");
			
			auto flatten_d3d12_factor = [](D3D12_BLEND in) -> D3D12_BLEND
			{
				switch (in)
				{
				case D3D12_BLEND_BLEND_FACTOR:
					return D3D12_BLEND_ONE;
				case D3D12_BLEND_INV_BLEND_FACTOR:
					return D3D12_BLEND_ZERO;
				}

				LOG_ERROR(RSX, "No suitable conversion defined for blend factor 0x%X" HERE, (u32)in);
				return in;
			};

			d3d_sfactor_rgb = flatten_d3d12_factor(d3d_sfactor_rgb);
			d3d_dfactor_rgb = flatten_d3d12_factor(d3d_dfactor_rgb);;
			d3d_sfactor_alpha = flatten_d3d12_factor(d3d_sfactor_alpha);
			d3d_dfactor_alpha = flatten_d3d12_factor(d3d_dfactor_alpha);
		}
		else
		{
			get_current_resource_storage().command_list->OMSetBlendFactor(BlendColor.data());
		}

		prop.Blend.RenderTarget[0].BlendEnable = true;

		if (rsx::method_registers.blend_enabled_surface_1())
			prop.Blend.RenderTarget[1].BlendEnable = true;
		if (rsx::method_registers.blend_enabled_surface_2())
			prop.Blend.RenderTarget[2].BlendEnable = true;
		if (rsx::method_registers.blend_enabled_surface_3())
			prop.Blend.RenderTarget[3].BlendEnable = true;

		prop.Blend.RenderTarget[0].BlendOp = get_blend_op(rsx::method_registers.blend_equation_rgb());
		prop.Blend.RenderTarget[0].BlendOpAlpha = get_blend_op(rsx::method_registers.blend_equation_a());

		if (rsx::method_registers.blend_enabled_surface_1())
		{
			prop.Blend.RenderTarget[1].BlendOp = get_blend_op(rsx::method_registers.blend_equation_rgb());
			prop.Blend.RenderTarget[1].BlendOpAlpha = get_blend_op(rsx::method_registers.blend_equation_a());
		}

		if (rsx::method_registers.blend_enabled_surface_2())
		{
			prop.Blend.RenderTarget[2].BlendOp = get_blend_op(rsx::method_registers.blend_equation_rgb());
			prop.Blend.RenderTarget[2].BlendOpAlpha = get_blend_op(rsx::method_registers.blend_equation_a());
		}

		if (rsx::method_registers.blend_enabled_surface_3())
		{
			prop.Blend.RenderTarget[3].BlendOp = get_blend_op(rsx::method_registers.blend_equation_rgb());
			prop.Blend.RenderTarget[3].BlendOpAlpha = get_blend_op(rsx::method_registers.blend_equation_a());
		}

		prop.Blend.RenderTarget[0].SrcBlend = d3d_sfactor_rgb;
		prop.Blend.RenderTarget[0].DestBlend = d3d_dfactor_rgb;
		prop.Blend.RenderTarget[0].SrcBlendAlpha = d3d_sfactor_alpha;
		prop.Blend.RenderTarget[0].DestBlendAlpha = d3d_dfactor_alpha;

		if (rsx::method_registers.blend_enabled_surface_1())
		{
			prop.Blend.RenderTarget[1].SrcBlend = d3d_sfactor_rgb;
			prop.Blend.RenderTarget[1].DestBlend = d3d_dfactor_rgb;
			prop.Blend.RenderTarget[1].SrcBlendAlpha = d3d_sfactor_alpha;
			prop.Blend.RenderTarget[1].DestBlendAlpha = d3d_dfactor_alpha;
		}

		if (rsx::method_registers.blend_enabled_surface_2())
		{
			prop.Blend.RenderTarget[2].SrcBlend = d3d_sfactor_rgb;
			prop.Blend.RenderTarget[2].DestBlend = d3d_dfactor_rgb;
			prop.Blend.RenderTarget[2].SrcBlendAlpha = d3d_sfactor_alpha;
			prop.Blend.RenderTarget[2].DestBlendAlpha = d3d_dfactor_alpha;
		}

		if (rsx::method_registers.blend_enabled_surface_3())
		{
			prop.Blend.RenderTarget[3].SrcBlend = d3d_sfactor_rgb;
			prop.Blend.RenderTarget[3].DestBlend = d3d_dfactor_rgb;
			prop.Blend.RenderTarget[3].SrcBlendAlpha = d3d_sfactor_alpha;
			prop.Blend.RenderTarget[3].DestBlendAlpha = d3d_dfactor_alpha;
		}
	}

	if (rsx::method_registers.logic_op_enabled())
	{
		prop.Blend.RenderTarget[0].LogicOpEnable = true;
		prop.Blend.RenderTarget[0].LogicOp = get_logic_op(rsx::method_registers.logic_operation());
	}

	prop.DepthStencilFormat = get_depth_stencil_surface_format(rsx::method_registers.surface_depth_fmt());
	prop.RenderTargetsFormat = get_color_surface_format(rsx::method_registers.surface_color());

	switch (rsx::method_registers.surface_color_target())
	{
	case rsx::surface_target::surface_a:
	case rsx::surface_target::surface_b:
		prop.numMRT = 1;
		break;
	case rsx::surface_target::surfaces_a_b:
		prop.numMRT = 2;
		break;
	case rsx::surface_target::surfaces_a_b_c:
		prop.numMRT = 3;
		break;
	case rsx::surface_target::surfaces_a_b_c_d:
		prop.numMRT = 4;
		break;
	default:
		break;
	}
	if (rsx::method_registers.depth_test_enabled())
	{
		prop.DepthStencil.DepthEnable = TRUE;
		prop.DepthStencil.DepthFunc = get_compare_func(rsx::method_registers.depth_func());
	}
	else
		prop.DepthStencil.DepthEnable = FALSE;

	prop.DepthStencil.DepthWriteMask = rsx::method_registers.depth_write_enabled() ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;

	if (rsx::method_registers.stencil_test_enabled())
	{
		prop.DepthStencil.StencilEnable = TRUE;
		prop.DepthStencil.StencilReadMask = rsx::method_registers.stencil_func_mask();
		prop.DepthStencil.StencilWriteMask = rsx::method_registers.stencil_mask();
		prop.DepthStencil.FrontFace.StencilPassOp = get_stencil_op(rsx::method_registers.stencil_op_zpass());
		prop.DepthStencil.FrontFace.StencilDepthFailOp = get_stencil_op(rsx::method_registers.stencil_op_zfail());
		prop.DepthStencil.FrontFace.StencilFailOp = get_stencil_op(rsx::method_registers.stencil_op_fail());
		prop.DepthStencil.FrontFace.StencilFunc = get_compare_func(rsx::method_registers.stencil_func());

		if (rsx::method_registers.two_sided_stencil_test_enabled())
		{
			prop.DepthStencil.BackFace.StencilFailOp = get_stencil_op(rsx::method_registers.back_stencil_op_fail());
			prop.DepthStencil.BackFace.StencilFunc = get_compare_func(rsx::method_registers.back_stencil_func());
			prop.DepthStencil.BackFace.StencilPassOp = get_stencil_op(rsx::method_registers.back_stencil_op_zpass());
			prop.DepthStencil.BackFace.StencilDepthFailOp = get_stencil_op(rsx::method_registers.back_stencil_op_zfail());
		}
		else
		{
			prop.DepthStencil.BackFace.StencilPassOp = get_stencil_op(rsx::method_registers.stencil_op_zpass());
			prop.DepthStencil.BackFace.StencilDepthFailOp = get_stencil_op(rsx::method_registers.stencil_op_zfail());
			prop.DepthStencil.BackFace.StencilFailOp = get_stencil_op(rsx::method_registers.stencil_op_fail());
			prop.DepthStencil.BackFace.StencilFunc = get_compare_func(rsx::method_registers.stencil_func());
		}
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

	if (rsx::method_registers.cull_face_enabled())
	{
		prop.Rasterization.CullMode = get_cull_face(rsx::method_registers.cull_face_mode());
	}

	prop.Rasterization.FrontCounterClockwise = get_front_face_ccw(rsx::method_registers.front_face_mode());

	UINT8 mask = 0;
	mask |= rsx::method_registers.color_mask_r() ? D3D12_COLOR_WRITE_ENABLE_RED : 0;
	mask |= rsx::method_registers.color_mask_g() ? D3D12_COLOR_WRITE_ENABLE_GREEN : 0;
	mask |= rsx::method_registers.color_mask_b() ? D3D12_COLOR_WRITE_ENABLE_BLUE : 0;
	mask |= rsx::method_registers.color_mask_a() ? D3D12_COLOR_WRITE_ENABLE_ALPHA : 0;
	for (unsigned i = 0; i < prop.numMRT; i++)
		prop.Blend.RenderTarget[i].RenderTargetWriteMask = mask;

	if (rsx::method_registers.restart_index_enabled())
	{
		rsx::index_array_type index_type = rsx::method_registers.current_draw_clause.is_immediate_draw?
			rsx::index_array_type::u32:
			rsx::method_registers.index_type();

		if (index_type == rsx::index_array_type::u32)
		{
			prop.CutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF;
		}
		if (index_type == rsx::index_array_type::u16)
		{
			prop.CutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF;
		}
	}

	m_current_pso = m_pso_cache.getGraphicPipelineState(current_vertex_program, current_fragment_program, prop, m_device.Get(), m_shared_root_signature.Get());
	return;
}

std::pair<std::string, std::string> D3D12GSRender::get_programs() const
{
	return std::make_pair(m_pso_cache.get_transform_program(current_vertex_program).content, m_pso_cache.get_shader_program(current_fragment_program).content);
}

void D3D12GSRender::notify_tile_unbound(u32 tile)
{
	u32 addr = rsx::get_address(tiles[tile].offset, tiles[tile].location);
	m_rtts.invalidate_surface_address(addr, false);
}
#endif
