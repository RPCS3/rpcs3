#include "Emu/RSX/VK/vkutils/descriptors.h"
#include "stdafx.h"
#include "../Overlays/overlay_compile_notification.h"
#include "../Overlays/Shaders/shader_loading_dialog_native.h"

#include "VKAsyncScheduler.h"
#include "VKCommandStream.h"
#include "VKCommonPipelineLayout.h"
#include "VKCompute.h"
#include "VKGSRender.h"
#include "VKHelpers.h"
#include "VKRenderPass.h"
#include "VKResourceManager.h"

#include "vkutils/buffer_object.h"
#include "vkutils/scratch.h"

#include "Emu/RSX/rsx_methods.h"
#include "Emu/RSX/Host/MM.h"
#include "Emu/RSX/Host/RSXDMAWriter.h"
#include "Emu/RSX/NV47/HW/context_accessors.define.h"
#include "Emu/Memory/vm_locking.h"

#include "../Program/SPIRVCommon.h"

#include "util/asm.hpp"
#include <vulkan/vulkan_core.h>

namespace vk
{
	VkCompareOp get_compare_func(rsx::comparison_function op, bool reverse_direction = false);

	std::pair<VkFormat, VkComponentMapping> get_compatible_surface_format(rsx::surface_color_format color_format)
	{
		const VkComponentMapping o_rgb = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_ONE };
		const VkComponentMapping z_rgb = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_ZERO };

		switch (color_format)
		{
#ifndef __APPLE__
		case rsx::surface_color_format::r5g6b5:
			return std::make_pair(VK_FORMAT_R5G6B5_UNORM_PACK16, vk::default_component_map);

		case rsx::surface_color_format::x1r5g5b5_o1r5g5b5:
			return std::make_pair(VK_FORMAT_A1R5G5B5_UNORM_PACK16, o_rgb);

		case rsx::surface_color_format::x1r5g5b5_z1r5g5b5:
			return std::make_pair(VK_FORMAT_A1R5G5B5_UNORM_PACK16, z_rgb);
#else
		// assign B8G8R8A8_UNORM to formats that are not supported by Metal
		case rsx::surface_color_format::r5g6b5:
			return std::make_pair(VK_FORMAT_B8G8R8A8_UNORM, vk::default_component_map);

		case rsx::surface_color_format::x1r5g5b5_o1r5g5b5:
			return std::make_pair(VK_FORMAT_B8G8R8A8_UNORM, o_rgb);

		case rsx::surface_color_format::x1r5g5b5_z1r5g5b5:
			return std::make_pair(VK_FORMAT_B8G8R8A8_UNORM, z_rgb);
#endif
		case rsx::surface_color_format::a8r8g8b8:
			return std::make_pair(VK_FORMAT_B8G8R8A8_UNORM, vk::default_component_map);

		case rsx::surface_color_format::a8b8g8r8:
			return std::make_pair(VK_FORMAT_R8G8B8A8_UNORM, vk::default_component_map);

		case rsx::surface_color_format::x8b8g8r8_o8b8g8r8:
			return std::make_pair(VK_FORMAT_R8G8B8A8_UNORM, o_rgb);

		case rsx::surface_color_format::x8b8g8r8_z8b8g8r8:
			return std::make_pair(VK_FORMAT_R8G8B8A8_UNORM, z_rgb);

		case rsx::surface_color_format::x8r8g8b8_z8r8g8b8:
			return std::make_pair(VK_FORMAT_B8G8R8A8_UNORM, z_rgb);

		case rsx::surface_color_format::x8r8g8b8_o8r8g8b8:
			return std::make_pair(VK_FORMAT_B8G8R8A8_UNORM, o_rgb);

		case rsx::surface_color_format::w16z16y16x16:
			return std::make_pair(VK_FORMAT_R16G16B16A16_SFLOAT, vk::default_component_map);

		case rsx::surface_color_format::w32z32y32x32:
			return std::make_pair(VK_FORMAT_R32G32B32A32_SFLOAT, vk::default_component_map);

		case rsx::surface_color_format::b8:
		{
			const VkComponentMapping no_alpha = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_ONE };
			return std::make_pair(VK_FORMAT_R8_UNORM, no_alpha);
		}

		case rsx::surface_color_format::g8b8:
		{
			const VkComponentMapping gb_rg = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G };
			return std::make_pair(VK_FORMAT_R8G8_UNORM, gb_rg);
		}

		case rsx::surface_color_format::x32:
		{
			const VkComponentMapping rrrr = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R };
			return std::make_pair(VK_FORMAT_R32_SFLOAT, rrrr);
		}

		default:
			rsx_log.error("Surface color buffer: Unsupported surface color format (0x%x)", static_cast<u32>(color_format));
			return std::make_pair(VK_FORMAT_B8G8R8A8_UNORM, vk::default_component_map);
		}
	}

	VkLogicOp get_logic_op(rsx::logic_op op)
	{
		switch (op)
		{
		case rsx::logic_op::logic_clear: return VK_LOGIC_OP_CLEAR;
		case rsx::logic_op::logic_and: return VK_LOGIC_OP_AND;
		case rsx::logic_op::logic_and_reverse: return VK_LOGIC_OP_AND_REVERSE;
		case rsx::logic_op::logic_copy: return VK_LOGIC_OP_COPY;
		case rsx::logic_op::logic_and_inverted: return VK_LOGIC_OP_AND_INVERTED;
		case rsx::logic_op::logic_noop: return VK_LOGIC_OP_NO_OP;
		case rsx::logic_op::logic_xor: return VK_LOGIC_OP_XOR;
		case rsx::logic_op::logic_or : return VK_LOGIC_OP_OR;
		case rsx::logic_op::logic_nor: return VK_LOGIC_OP_NOR;
		case rsx::logic_op::logic_equiv: return VK_LOGIC_OP_EQUIVALENT;
		case rsx::logic_op::logic_invert: return VK_LOGIC_OP_INVERT;
		case rsx::logic_op::logic_or_reverse: return VK_LOGIC_OP_OR_REVERSE;
		case rsx::logic_op::logic_copy_inverted: return VK_LOGIC_OP_COPY_INVERTED;
		case rsx::logic_op::logic_or_inverted: return VK_LOGIC_OP_OR_INVERTED;
		case rsx::logic_op::logic_nand: return VK_LOGIC_OP_NAND;
		case rsx::logic_op::logic_set: return VK_LOGIC_OP_SET;
		default:
			fmt::throw_exception("Unknown logic op 0x%x", static_cast<u32>(op));
		}
	}

	VkBlendFactor get_blend_factor(rsx::blend_factor factor)
	{
		switch (factor)
		{
		case rsx::blend_factor::one: return VK_BLEND_FACTOR_ONE;
		case rsx::blend_factor::zero: return VK_BLEND_FACTOR_ZERO;
		case rsx::blend_factor::src_alpha: return VK_BLEND_FACTOR_SRC_ALPHA;
		case rsx::blend_factor::dst_alpha: return VK_BLEND_FACTOR_DST_ALPHA;
		case rsx::blend_factor::src_color: return VK_BLEND_FACTOR_SRC_COLOR;
		case rsx::blend_factor::dst_color: return VK_BLEND_FACTOR_DST_COLOR;
		case rsx::blend_factor::constant_color: return VK_BLEND_FACTOR_CONSTANT_COLOR;
		case rsx::blend_factor::constant_alpha: return VK_BLEND_FACTOR_CONSTANT_ALPHA;
		case rsx::blend_factor::one_minus_src_color: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		case rsx::blend_factor::one_minus_dst_color: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		case rsx::blend_factor::one_minus_src_alpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		case rsx::blend_factor::one_minus_dst_alpha: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		case rsx::blend_factor::one_minus_constant_alpha: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
		case rsx::blend_factor::one_minus_constant_color: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
		case rsx::blend_factor::src_alpha_saturate: return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
		default:
			fmt::throw_exception("Unknown blend factor 0x%x", static_cast<u32>(factor));
		}
	}

	VkBlendOp get_blend_op(rsx::blend_equation op)
	{
		switch (op)
		{
		case rsx::blend_equation::add_signed:
			rsx_log.trace("blend equation add_signed used. Emulating using FUNC_ADD");
			[[fallthrough]];
		case rsx::blend_equation::add:
			return VK_BLEND_OP_ADD;
		case rsx::blend_equation::subtract: return VK_BLEND_OP_SUBTRACT;
		case rsx::blend_equation::reverse_subtract_signed:
			rsx_log.trace("blend equation reverse_subtract_signed used. Emulating using FUNC_REVERSE_SUBTRACT");
			[[fallthrough]];
		case rsx::blend_equation::reverse_subtract: return VK_BLEND_OP_REVERSE_SUBTRACT;
		case rsx::blend_equation::min: return VK_BLEND_OP_MIN;
		case rsx::blend_equation::max: return VK_BLEND_OP_MAX;
		default:
			fmt::throw_exception("Unknown blend op: 0x%x", static_cast<u32>(op));
		}
	}

	VkStencilOp get_stencil_op(rsx::stencil_op op)
	{
		switch (op)
		{
		case rsx::stencil_op::keep: return VK_STENCIL_OP_KEEP;
		case rsx::stencil_op::zero: return VK_STENCIL_OP_ZERO;
		case rsx::stencil_op::replace: return VK_STENCIL_OP_REPLACE;
		case rsx::stencil_op::incr: return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
		case rsx::stencil_op::decr: return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
		case rsx::stencil_op::invert: return VK_STENCIL_OP_INVERT;
		case rsx::stencil_op::incr_wrap: return VK_STENCIL_OP_INCREMENT_AND_WRAP;
		case rsx::stencil_op::decr_wrap: return VK_STENCIL_OP_DECREMENT_AND_WRAP;
		default:
			fmt::throw_exception("Unknown stencil op: 0x%x", static_cast<u32>(op));
		}
	}

	VkFrontFace get_front_face(rsx::front_face ffv)
	{
		switch (ffv)
		{
		case rsx::front_face::cw: return VK_FRONT_FACE_CLOCKWISE;
		case rsx::front_face::ccw: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
		default:
			fmt::throw_exception("Unknown front face value: 0x%x", static_cast<u32>(ffv));
		}
	}

	VkCullModeFlags get_cull_face(rsx::cull_face cfv)
	{
		switch (cfv)
		{
		case rsx::cull_face::back: return VK_CULL_MODE_BACK_BIT;
		case rsx::cull_face::front: return VK_CULL_MODE_FRONT_BIT;
		case rsx::cull_face::front_and_back: return VK_CULL_MODE_FRONT_AND_BACK;
		default:
			fmt::throw_exception("Unknown cull face value: 0x%x", static_cast<u32>(cfv));
		}
	}

	struct vertex_input_assembly_state
	{
		VkPrimitiveTopology primitive;
		VkBool32 restart_index_enabled;
	};

	vertex_input_assembly_state decode_vertex_input_assembly_state()
	{
		vertex_input_assembly_state state{};
		const auto& current_draw = rsx::method_registers.current_draw_clause;
		const auto [primitive, emulated_primitive] = vk::get_appropriate_topology(current_draw.primitive);

		if (rsx::method_registers.restart_index_enabled() &&
			!current_draw.is_disjoint_primitive &&
			current_draw.command == rsx::draw_command::indexed &&
			!emulated_primitive &&
			!vk::emulate_primitive_restart(current_draw.primitive))
		{
			state.restart_index_enabled = VK_TRUE;
		}

		state.primitive = primitive;
		return state;
	}

	// TODO: This should be deprecated soon (kd)
	vk::pipeline_props decode_rsx_state(
		const vertex_input_assembly_state& vertex_input,
		vk::render_target* ds,
		const rsx::backend_configuration& backend_config,
		u8 num_draw_buffers,
		u8 num_rasterization_samples,
		bool depth_bounds_support)
	{
		vk::pipeline_props properties{};

		// Input assembly
		properties.state.set_primitive_type(vertex_input.primitive);
		properties.state.enable_primitive_restart(vertex_input.restart_index_enabled);

		// Rasterizer state
		properties.state.set_attachment_count(num_draw_buffers);
		properties.state.set_front_face(vk::get_front_face(rsx::method_registers.front_face_mode()));
		properties.state.enable_depth_clamp(rsx::method_registers.depth_clamp_enabled() || !rsx::method_registers.depth_clip_enabled());
		properties.state.enable_depth_bias(true);
		properties.state.enable_depth_bounds_test(depth_bounds_support);

		if (rsx::method_registers.depth_test_enabled())
		{
			//NOTE: Like stencil, depth write is meaningless without depth test
			properties.state.set_depth_mask(rsx::method_registers.depth_write_enabled());
			properties.state.enable_depth_test(vk::get_compare_func(rsx::method_registers.depth_func()));
		}

		if (rsx::method_registers.cull_face_enabled())
		{
			properties.state.enable_cull_face(vk::get_cull_face(rsx::method_registers.cull_face_mode()));
		}

		const auto host_write_mask = rsx::get_write_output_mask(rsx::method_registers.surface_color());
		for (uint index = 0; index < num_draw_buffers; ++index)
		{
			bool color_mask_b = rsx::method_registers.color_mask_b(index);
			bool color_mask_g = rsx::method_registers.color_mask_g(index);
			bool color_mask_r = rsx::method_registers.color_mask_r(index);
			bool color_mask_a = rsx::method_registers.color_mask_a(index);

			switch (rsx::method_registers.surface_color())
			{
			case rsx::surface_color_format::b8:
				rsx::get_b8_colormask(color_mask_r, color_mask_g, color_mask_b, color_mask_a);
				break;
			case rsx::surface_color_format::g8b8:
				rsx::get_g8b8_r8g8_colormask(color_mask_r, color_mask_g, color_mask_b, color_mask_a);
				break;
			default:
				break;
			}

			properties.state.set_color_mask(
				index,
				color_mask_r && host_write_mask[0],
				color_mask_g && host_write_mask[1],
				color_mask_b && host_write_mask[2],
				color_mask_a && host_write_mask[3]);
		}

		// LogicOp and Blend are mutually exclusive. If both are enabled, LogicOp takes precedence.
		if (rsx::method_registers.logic_op_enabled())
		{
			properties.state.enable_logic_op(vk::get_logic_op(rsx::method_registers.logic_operation()));
		}
		else
		{
			bool mrt_blend_enabled[] =
			{
				rsx::method_registers.blend_enabled(),
				rsx::method_registers.blend_enabled_surface_1(),
				rsx::method_registers.blend_enabled_surface_2(),
				rsx::method_registers.blend_enabled_surface_3()
			};

			VkBlendFactor sfactor_rgb, sfactor_a, dfactor_rgb, dfactor_a;
			VkBlendOp equation_rgb, equation_a;

			if (mrt_blend_enabled[0] || mrt_blend_enabled[1] || mrt_blend_enabled[2] || mrt_blend_enabled[3])
			{
				sfactor_rgb = vk::get_blend_factor(rsx::method_registers.blend_func_sfactor_rgb());
				sfactor_a = vk::get_blend_factor(rsx::method_registers.blend_func_sfactor_a());
				dfactor_rgb = vk::get_blend_factor(rsx::method_registers.blend_func_dfactor_rgb());
				dfactor_a = vk::get_blend_factor(rsx::method_registers.blend_func_dfactor_a());
				equation_rgb = vk::get_blend_op(rsx::method_registers.blend_equation_rgb());
				equation_a = vk::get_blend_op(rsx::method_registers.blend_equation_a());

				for (u8 idx = 0; idx < num_draw_buffers; ++idx)
				{
					if (mrt_blend_enabled[idx])
					{
						properties.state.enable_blend(idx, sfactor_rgb, sfactor_a, dfactor_rgb, dfactor_a, equation_rgb, equation_a);
					}
				}
			}
		}

		if (rsx::method_registers.stencil_test_enabled())
		{
			if (!rsx::method_registers.two_sided_stencil_test_enabled())
			{
				properties.state.enable_stencil_test(
					vk::get_stencil_op(rsx::method_registers.stencil_op_fail()),
					vk::get_stencil_op(rsx::method_registers.stencil_op_zfail()),
					vk::get_stencil_op(rsx::method_registers.stencil_op_zpass()),
					vk::get_compare_func(rsx::method_registers.stencil_func()),
					0xFF, 0xFF); //write mask, func_mask, ref are dynamic
			}
			else
			{
				properties.state.enable_stencil_test_separate(0,
					vk::get_stencil_op(rsx::method_registers.stencil_op_fail()),
					vk::get_stencil_op(rsx::method_registers.stencil_op_zfail()),
					vk::get_stencil_op(rsx::method_registers.stencil_op_zpass()),
					vk::get_compare_func(rsx::method_registers.stencil_func()),
					0xFF, 0xFF); //write mask, func_mask, ref are dynamic

				properties.state.enable_stencil_test_separate(1,
					vk::get_stencil_op(rsx::method_registers.back_stencil_op_fail()),
					vk::get_stencil_op(rsx::method_registers.back_stencil_op_zfail()),
					vk::get_stencil_op(rsx::method_registers.back_stencil_op_zpass()),
					vk::get_compare_func(rsx::method_registers.back_stencil_func()),
					0xFF, 0xFF); //write mask, func_mask, ref are dynamic
			}

			if (ds && ds->samples() > 1 && !(ds->stencil_init_flags & 0xFF00))
			{
				if (properties.state.ds.front.failOp != VK_STENCIL_OP_KEEP ||
					properties.state.ds.front.depthFailOp != VK_STENCIL_OP_KEEP ||
					properties.state.ds.front.passOp != VK_STENCIL_OP_KEEP ||
					properties.state.ds.back.failOp != VK_STENCIL_OP_KEEP ||
					properties.state.ds.back.depthFailOp != VK_STENCIL_OP_KEEP ||
					properties.state.ds.back.passOp != VK_STENCIL_OP_KEEP)
				{
					// Toggle bit 9 to signal require full bit-wise transfer
					ds->stencil_init_flags |= (1 << 8);
				}
			}
		}

		if (backend_config.supports_hw_a2c || num_rasterization_samples > 1)
		{
			const bool alpha_to_one_enable = rsx::method_registers.msaa_alpha_to_one_enabled() && backend_config.supports_hw_a2one;

			properties.state.set_multisample_state(
				num_rasterization_samples,
				rsx::method_registers.msaa_sample_mask(),
				rsx::method_registers.msaa_enabled(),
				rsx::method_registers.msaa_alpha_to_coverage_enabled(),
				alpha_to_one_enable);

			// A problem observed on multiple GPUs is that interior geometry edges can resolve 0 samples unless we force shading rate of 1.
			// For whatever reason, the way MSAA images are 'resolved' on PS3 bypasses this issue.
			// NOTE: We do not do image resolve at all, the output is merely 'exploded' and the guest application is responsible for doing the resolve in software as it is on real hardware.
			properties.state.set_multisample_shading_rate(1.f);
		}

		return properties;
	}
}

u64 VKGSRender::get_cycles()
{
	return thread_ctrl::get_cycles(static_cast<named_thread<VKGSRender>&>(*this));
}

VKGSRender::VKGSRender(utils::serial* ar) noexcept : GSRender(ar)
{
	// Initialize dependencies
	g_fxo->need<rsx::dma_manager>();

	if (!m_instance.create("RPCS3"))
	{
		rsx_log.fatal("Could not find a Vulkan compatible GPU driver. Your GPU(s) may not support Vulkan, or you need to install the Vulkan runtime and drivers");
		m_device = VK_NULL_HANDLE;
		return;
	}

	m_instance.bind();

	std::vector<vk::physical_device>& gpus = m_instance.enumerate_devices();

	// Actually confirm  that the loader found at least one compatible device
	// This should not happen unless something is wrong with the driver setup on the target system
	if (gpus.empty())
	{
		//We can't throw in Emulator::Load, so we show error and return
		rsx_log.fatal("No compatible GPU devices found");
		m_device = VK_NULL_HANDLE;
		return;
	}

	bool gpu_found = false;
	std::string adapter_name = g_cfg.video.vk.adapter;

	display_handle_t display = m_frame->handle();

#ifdef HAVE_X11
	std::visit([this](auto&& p) {
		using T = std::decay_t<decltype(p)>;
		if constexpr (std::is_same_v<T, std::pair<Display*, Window>>)
		{
			m_display_handle = p.first; XFlush(m_display_handle);
		}
	}, display);
#endif

	for (auto &gpu : gpus)
	{
		if (gpu.get_name() == adapter_name)
		{
			m_swapchain.reset(m_instance.create_swapchain(display, gpu));
			gpu_found = true;
			break;
		}
	}

	if (!gpu_found || adapter_name.empty())
	{
		m_swapchain.reset(m_instance.create_swapchain(display, gpus[0]));
	}

	if (!m_swapchain)
	{
		m_device = VK_NULL_HANDLE;
		rsx_log.fatal("Could not successfully initialize a swapchain");
		return;
	}

	m_device = const_cast<vk::render_device*>(&m_swapchain->get_device());
	vk::set_current_renderer(m_swapchain->get_device());
	vk::init();

	m_swapchain_dims.width = m_frame->client_width();
	m_swapchain_dims.height = m_frame->client_height();

	if (!m_swapchain->init(m_swapchain_dims.width, m_swapchain_dims.height))
	{
		swapchain_unavailable = true;
	}

	// create command buffer...
	m_command_buffer_pool.create((*m_device), m_device->get_graphics_queue_family());
	m_primary_cb_list.create(m_command_buffer_pool, vk::command_buffer::access_type_hint::flush_only);
	m_current_command_buffer = m_primary_cb_list.get();
	m_current_command_buffer->begin();

	// Create secondary command_buffer for parallel operations
	m_secondary_command_buffer_pool.create((*m_device), m_device->get_graphics_queue_family());
	m_secondary_cb_list.create(m_secondary_command_buffer_pool, vk::command_buffer::access_type_hint::all);

	//Occlusion
	m_occlusion_query_manager = std::make_unique<vk::query_pool_manager>(*m_device, VK_QUERY_TYPE_OCCLUSION, OCCLUSION_MAX_POOL_SIZE);
	m_occlusion_map.resize(rsx::reports::occlusion_query_count);

	for (u32 n = 0; n < rsx::reports::occlusion_query_count; ++n)
		m_occlusion_query_data[n].driver_handle = n;

	if (g_cfg.video.precise_zpass_count)
	{
		m_occlusion_query_manager->set_control_flags(VK_QUERY_CONTROL_PRECISE_BIT, 0);
	}

	VkSemaphoreCreateInfo semaphore_info = {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	// VRAM allocation
	m_attrib_ring_info.create(VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, VK_ATTRIB_RING_BUFFER_SIZE_M * 0x100000, "attrib buffer", 0x400000, VK_TRUE);
	m_fragment_env_ring_info.create(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_UBO_RING_BUFFER_SIZE_M * 0x100000, "fragment env buffer");
	m_vertex_env_ring_info.create(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_UBO_RING_BUFFER_SIZE_M * 0x100000, "vertex env buffer");
	m_fragment_texture_params_ring_info.create(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_UBO_RING_BUFFER_SIZE_M * 0x100000, "fragment texture params buffer");
	m_vertex_layout_ring_info.create(VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, VK_UBO_RING_BUFFER_SIZE_M * 0x100000, "vertex layout buffer", 0x10000, VK_TRUE);
	m_fragment_constants_ring_info.create(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_UBO_RING_BUFFER_SIZE_M * 0x100000, "fragment constants buffer");
	m_transform_constants_ring_info.create(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_TRANSFORM_CONSTANTS_BUFFER_SIZE_M * 0x100000, "transform constants buffer");
	m_index_buffer_ring_info.create(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_INDEX_RING_BUFFER_SIZE_M * 0x100000, "index buffer");
	m_texture_upload_buffer_ring_info.create(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_TEXTURE_UPLOAD_RING_BUFFER_SIZE_M * 0x100000, "texture upload buffer", 32 * 0x100000);
	m_raster_env_ring_info.create(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_UBO_RING_BUFFER_SIZE_M * 0x100000, "raster env buffer");
	m_instancing_buffer_ring_info.create(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_TRANSFORM_CONSTANTS_BUFFER_SIZE_M * 0x100000, "instancing data buffer");

	vk::data_heap_manager::register_ring_buffers
	({
		std::ref(m_attrib_ring_info),
		std::ref(m_fragment_env_ring_info),
		std::ref(m_vertex_env_ring_info),
		std::ref(m_fragment_texture_params_ring_info),
		std::ref(m_vertex_layout_ring_info),
		std::ref(m_fragment_constants_ring_info),
		std::ref(m_transform_constants_ring_info),
		std::ref(m_index_buffer_ring_info),
		std::ref(m_texture_upload_buffer_ring_info),
		std::ref(m_raster_env_ring_info),
		std::ref(m_instancing_buffer_ring_info)
	});

	const auto shadermode = g_cfg.video.shadermode.get();

	if (shadermode == shader_mode::async_with_interpreter || shadermode == shader_mode::interpreter_only)
	{
		m_vertex_instructions_buffer.create(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 64 * 0x100000, "vertex instructions buffer", 512 * 16);
		m_fragment_instructions_buffer.create(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 64 * 0x100000, "fragment instructions buffer", 2048);

		vk::data_heap_manager::register_ring_buffers
		({
			std::ref(m_vertex_instructions_buffer),
			std::ref(m_fragment_instructions_buffer)
		});
	}

	// Initialize optional allocation information with placeholders
	m_vertex_env_buffer_info = { m_vertex_env_ring_info.heap->value, 0, 16 };
	m_vertex_constants_buffer_info = { m_transform_constants_ring_info.heap->value, 0, 16 };
	m_fragment_env_buffer_info = { m_fragment_env_ring_info.heap->value, 0, 16 };
	m_fragment_texture_params_buffer_info = { m_fragment_texture_params_ring_info.heap->value, 0, 16 };
	m_raster_env_buffer_info = { m_raster_env_ring_info.heap->value, 0, 128 };

	const auto& limits = m_device->gpu().get_limits();
	m_texbuffer_view_size = std::min(limits.maxTexelBufferElements, VK_ATTRIB_RING_BUFFER_SIZE_M * 0x100000u);

	if (m_texbuffer_view_size < 0x800000)
	{
		// Warn, only possibly expected on macOS
		rsx_log.warning("Current driver may crash due to memory limitations (%uk)", m_texbuffer_view_size / 1024);
	}

	for (auto &ctx : frame_context_storage)
	{
		vkCreateSemaphore((*m_device), &semaphore_info, nullptr, &ctx.present_wait_semaphore);
		vkCreateSemaphore((*m_device), &semaphore_info, nullptr, &ctx.acquire_signal_semaphore);
	}

	const auto& memory_map = m_device->get_memory_mapping();
	null_buffer = std::make_unique<vk::buffer>(*m_device, 32, memory_map.device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, 0, VMM_ALLOCATION_POOL_UNDEFINED);
	null_buffer_view = std::make_unique<vk::buffer_view>(*m_device, null_buffer->value, VK_FORMAT_R8_UINT, 0, 32);

	spirv::initialize_compiler_context();
	vk::initialize_pipe_compiler(g_cfg.video.shader_compiler_threads_count);

	m_prog_buffer = std::make_unique<vk::program_cache>
	(
		[this](const vk::pipeline_props& props, const RSXVertexProgram& vp, const RSXFragmentProgram& fp)
		{
			// Program was linked or queued for linking
			m_shaders_cache->store(props, vp, fp);
		}
	);

	if (g_cfg.video.disable_vertex_cache)
		m_vertex_cache = std::make_unique<vk::null_vertex_cache>();
	else
		m_vertex_cache = std::make_unique<vk::weak_vertex_cache>();

	m_shaders_cache = std::make_unique<vk::shader_cache>(*m_prog_buffer, "vulkan", "v1.95");

	for (u32 i = 0; i < m_swapchain->get_swap_image_count(); ++i)
	{
		const auto target_layout = m_swapchain->get_optimal_present_layout();
		const auto target_image = m_swapchain->get_image(i);
		VkClearColorValue clear_color{};
		VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		vk::change_image_layout(*m_current_command_buffer, target_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range);
		vkCmdClearColorImage(*m_current_command_buffer, target_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color, 1, &range);
		vk::change_image_layout(*m_current_command_buffer, target_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, target_layout, range);

	}

	m_current_frame = &frame_context_storage[0];

	m_texture_cache.initialize((*m_device), m_device->get_graphics_queue(),
			m_texture_upload_buffer_ring_info);

	vk::get_overlay_pass<vk::ui_overlay_renderer>()->init(*m_current_command_buffer, m_texture_upload_buffer_ring_info);

	if (shadermode == shader_mode::async_with_interpreter || shadermode == shader_mode::interpreter_only)
	{
		m_shader_interpreter.init(*m_device);
	}

	backend_config.supports_multidraw = true;

	// NVIDIA has broken attribute interpolation
	backend_config.supports_normalized_barycentrics = (
		!vk::is_NVIDIA(vk::get_driver_vendor()) ||
		!m_device->get_barycoords_support() ||
		g_cfg.video.shader_precision == gpu_preset_level::low);

	// NOTE: We do not actually need multiple sample support for A2C to work
	// This is here for visual consistency - will be removed when AA problems due to mipmaps are fixed
	if (g_cfg.video.antialiasing_level != msaa_level::none)
	{
		backend_config.supports_hw_msaa = true;
		backend_config.supports_hw_a2c = true;
		backend_config.supports_hw_a2c_1spp = true;
		backend_config.supports_hw_a2one = m_device->get_alpha_to_one_support();
	}

	// NOTE: On NVIDIA cards going back decades (including the PS3) there is a slight normalization inaccuracy in compressed formats.
	// Confirmed in BLES01916 (The Evil Within) which uses RGB565 for some virtual texturing data.
	backend_config.supports_hw_renormalization = vk::is_NVIDIA(vk::get_driver_vendor());

	// Conditional rendering support
	// Do not use on MVK due to a speedhack we rely on (streaming results without stopping the current renderpass)
	// If we break the renderpasses, MVK loses around 75% of its performance in troublesome spots compared to just doing a CPU sync
	backend_config.supports_hw_conditional_render = (vk::get_driver_vendor() != vk::driver_vendor::MVK);

	// Passthrough DMA
	backend_config.supports_passthrough_dma = m_device->get_external_memory_host_support();

	// Host sync
	backend_config.supports_host_gpu_labels = !!g_cfg.video.host_label_synchronization;

	// Async compute and related operations
	if (g_cfg.video.vk.asynchronous_texture_streaming)
	{
		// Optimistic, enable async compute
		backend_config.supports_asynchronous_compute = true;

		if (m_device->get_graphics_queue() == m_device->get_transfer_queue())
		{
			rsx_log.error("Cannot run graphics and async transfer in the same queue. Async uploads are disabled. This is a limitation of your GPU");
			backend_config.supports_asynchronous_compute = false;
		}
	}

	// Sanity checks
	switch (vk::get_driver_vendor())
	{
	case vk::driver_vendor::NVIDIA:
		if (backend_config.supports_asynchronous_compute)
		{
			if (auto chip_family = vk::get_chip_family();
				chip_family == vk::chip_class::NV_kepler || chip_family == vk::chip_class::NV_maxwell)
			{
				rsx_log.warning("Older NVIDIA cards do not meet requirements for true asynchronous compute due to some driver fakery.");
			}

			rsx_log.notice("Forcing safe async compute for NVIDIA device to avoid crashing.");
			g_cfg.video.vk.asynchronous_scheduler.set(vk_gpu_scheduler_mode::safe);
		}
		break;
	case vk::driver_vendor::NVK:
		// TODO: Verify if this driver crashes or not
		rsx_log.warning("NVK behavior with passthrough DMA is unknown. Proceed with caution.");
		break;
#if !defined(_WIN32)
		// Anything running on AMDGPU kernel driver will not work due to the check for fd-backed memory allocations
	case vk::driver_vendor::RADV:
	case vk::driver_vendor::AMD:
#if !defined(__linux__)
		// Intel chipsets would fail on BSD in most cases and DRM_IOCTL_i915_GEM_USERPTR unimplemented
	case vk::driver_vendor::ANV:
#endif
		if (backend_config.supports_passthrough_dma)
		{
			rsx_log.error("AMDGPU kernel driver on Linux and INTEL driver on some platforms cannot support passthrough DMA buffers.");
			backend_config.supports_passthrough_dma = false;
		}
		break;
#endif
	case vk::driver_vendor::MVK:
		// Async compute crashes immediately on Apple GPUs
		rsx_log.error("Apple GPUs are incompatible with the current implementation of asynchronous texture decoding.");
		backend_config.supports_asynchronous_compute = false;
		break;
	case vk::driver_vendor::INTEL:
		// As expected host allocations won't work on INTEL despite the extension being present
		if (backend_config.supports_passthrough_dma)
		{
			rsx_log.error("INTEL driver does not support passthrough DMA buffers");
			backend_config.supports_passthrough_dma = false;
		}
		break;
	default: break;
	}

	if (backend_config.supports_asynchronous_compute)
	{
		m_async_compute_memory_barrier =
		{
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2_KHR,
			.pNext = nullptr,
			.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR,
			.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT_KHR,
			.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT_KHR | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT_KHR,
			.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT_KHR
		};

		m_async_compute_dependency_info =
		{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
			.memoryBarrierCount = 1,
			.pMemoryBarriers = &m_async_compute_memory_barrier
		};

		// Run only if async compute can be used.
		g_fxo->init<vk::AsyncTaskScheduler>(g_cfg.video.vk.asynchronous_scheduler, m_async_compute_dependency_info);
	}

	if (backend_config.supports_host_gpu_labels)
	{
		if (backend_config.supports_passthrough_dma)
		{
			m_host_object_data = std::make_unique<vk::buffer>(*m_device,
				0x10000,
				memory_map.host_visible_coherent, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0,
				VMM_ALLOCATION_POOL_SYSTEM);

			m_host_dma_ctrl = std::make_unique<rsx::RSXDMAWriter>(m_host_object_data->map(0, 0x10000));
		}
		else
		{
			rsx_log.error("Your GPU/driver does not support extensions required to enable passthrough DMA emulation. Host GPU labels will be disabled.");
			backend_config.supports_host_gpu_labels = false;
		}
	}

	if (!backend_config.supports_host_gpu_labels &&
		!backend_config.supports_asynchronous_compute)
	{
		// Disable passthrough DMA unless we enable a feature that requires it.
		// I'm avoiding an explicit checkbox for this until I figure out why host labels don't fix all problems with passthrough.
		backend_config.supports_passthrough_dma = false;
	}
}

VKGSRender::~VKGSRender()
{
	if (m_device == VK_NULL_HANDLE)
	{
		//Initialization failed
		return;
	}

	// Flush DMA queue
	while (!g_fxo->get<rsx::dma_manager>().sync())
	{
		do_local_task(rsx::FIFO::state::lock_wait);
	}

	//Wait for device to finish up with resources
	vkDeviceWaitIdle(*m_device);

	// Globals. TODO: Refactor lifetime management
	if (auto async_scheduler = g_fxo->try_get<vk::AsyncTaskScheduler>())
	{
		async_scheduler->destroy();
	}

	// GC cleanup
	vk::get_resource_manager()->flush();

	// Host data
	if (m_host_object_data)
	{
		m_host_object_data->unmap();
		m_host_object_data.reset();
	}

	// Clear flush requests
	m_flush_requests.clear_pending_flag();

	// Shaders
	vk::destroy_pipe_compiler();        // Ensure no pending shaders being compiled
	spirv::finalize_compiler_context(); // Shut down the glslang compiler
	m_prog_buffer->clear();             // Delete shader objects
	m_shader_interpreter.destroy();

	m_persistent_attribute_storage.reset();
	m_volatile_attribute_storage.reset();
	m_vertex_layout_storage.reset();

	// Upscaler (references some global resources)
	m_upscaler.reset();

	// Heaps
	vk::data_heap_manager::reset();

	// Fallback bindables
	null_buffer.reset();
	null_buffer_view.reset();

	if (m_current_frame == &m_aux_frame_context)
	{
		// Return resources back to the owner
		m_current_frame = &frame_context_storage[m_current_queue_index];
		m_current_frame->grab_resources(m_aux_frame_context);
	}

	// NOTE: aux_context uses descriptor pools borrowed from the main queues and any allocations will be automatically freed when pool is destroyed
	for (auto &ctx : frame_context_storage)
	{
		vkDestroySemaphore((*m_device), ctx.present_wait_semaphore, nullptr);
		vkDestroySemaphore((*m_device), ctx.acquire_signal_semaphore, nullptr);
	}

	// Textures
	m_rtts.destroy();
	m_texture_cache.destroy();

	m_stencil_mirror_sampler.reset();

	// Queries
	m_occlusion_query_manager.reset();
	m_cond_render_buffer.reset();

	// Command buffer
	m_primary_cb_list.destroy();
	m_secondary_cb_list.destroy();

	m_command_buffer_pool.destroy();
	m_secondary_command_buffer_pool.destroy();

	// Descriptors
	vk::descriptors::flush();

	// Global resources
	vk::destroy_global_resources();

	// Device handles/contexts
	m_swapchain->destroy();
	m_instance.destroy();

#if defined(HAVE_X11) && defined(HAVE_VULKAN)
	if (m_display_handle)
		XCloseDisplay(m_display_handle);
#endif
}

bool VKGSRender::on_access_violation(u32 address, bool is_writing)
{
	rsx::mm_flush(address);

	vk::texture_cache::thrashed_set result;
	{
		const rsx::invalidation_cause cause = is_writing ? rsx::invalidation_cause::deferred_write : rsx::invalidation_cause::deferred_read;
		result = m_texture_cache.invalidate_address(*m_secondary_cb_list.get(), address, cause);
	}

	if (result.invalidate_samplers)
	{
		std::lock_guard lock(m_sampler_mutex);
		m_samplers_dirty.store(true);
	}

	if (!result.violation_handled)
	{
		return zcull_ctrl->on_access_violation(address);
	}

	if (result.num_flushable > 0)
	{
		if (g_fxo->get<rsx::dma_manager>().is_current_thread())
		{
			// The offloader thread cannot handle flush requests
			ensure(!(m_queue_status & flush_queue_state::deadlock));

			m_offloader_fault_range = g_fxo->get<rsx::dma_manager>().get_fault_range(is_writing);
			m_offloader_fault_cause = (is_writing) ? rsx::invalidation_cause::write : rsx::invalidation_cause::read;

			g_fxo->get<rsx::dma_manager>().set_mem_fault_flag();
			m_queue_status |= flush_queue_state::deadlock;
			m_eng_interrupt_mask |= rsx::backend_interrupt;

			// Wait for deadlock to clear
			while (m_queue_status & flush_queue_state::deadlock)
			{
				utils::pause();
			}

			g_fxo->get<rsx::dma_manager>().clear_mem_fault_flag();
			return true;
		}

		bool has_queue_ref = false;
		std::function<void()> data_transfer_completed_callback{};

		if (!is_current_thread()) [[likely]]
		{
			// Always submit primary cb to ensure state consistency (flush pending changes such as image transitions)
			vm::temporary_unlock();

			std::lock_guard lock(m_flush_queue_mutex);

			m_flush_requests.post(false);
			m_eng_interrupt_mask |= rsx::backend_interrupt;
			has_queue_ref = true;
		}
		else
		{
			if (vk::is_uninterruptible())
			{
				rsx_log.error("Fault in uninterruptible code!");
			}

			// Flush primary cb queue to sync pending changes (e.g image transitions!)
			flush_command_queue();
		}

		if (has_queue_ref)
		{
			// Wait for the RSX thread to process request if it hasn't already
			m_flush_requests.producer_wait();

			data_transfer_completed_callback = [&]()
			{
				m_flush_requests.remove_one();
				has_queue_ref = false;
			};
		}

		m_texture_cache.flush_all(*m_secondary_cb_list.next(), result, data_transfer_completed_callback);

		if (has_queue_ref)
		{
			// Release RSX thread if it's still locked
			m_flush_requests.remove_one();
		}
	}

	return true;
}

void VKGSRender::on_invalidate_memory_range(const utils::address_range32 &range, rsx::invalidation_cause cause)
{
	std::lock_guard lock(m_secondary_cb_guard);

	auto data = m_texture_cache.invalidate_range(*m_secondary_cb_list.next(), range, cause);
	AUDIT(data.empty());

	if (cause == rsx::invalidation_cause::unmap)
	{
		if (data.violation_handled)
		{
			m_texture_cache.purge_unreleased_sections();
			{
				std::lock_guard lock(m_sampler_mutex);
				m_samplers_dirty.store(true);
			}
		}

		vk::unmap_dma(range.start, range.length());
	}
}

void VKGSRender::on_semaphore_acquire_wait()
{
	if (m_flush_requests.pending() ||
		(async_flip_requested & flip_request::emu_requested) ||
		(m_queue_status & flush_queue_state::deadlock))
	{
		do_local_task(rsx::FIFO::state::lock_wait);
	}
}

bool VKGSRender::on_vram_exhausted(rsx::problem_severity severity)
{
	ensure(!vk::is_uninterruptible() && rsx::get_current_renderer()->is_current_thread());

	bool texture_cache_relieved = false;
	if (severity >= rsx::problem_severity::fatal)
	{
		// Hard sync before trying to evict anything. This guarantees no UAF crashes in the driver.
		// As a bonus, we also get a free gc pass
		flush_command_queue(true, true);

		if (m_texture_cache.is_overallocated())
		{
			// Evict some unused textures. Do not evict any active references
			std::set<u32> exclusion_list;
			auto scan_array = [&](const auto& texture_array)
			{
				for (auto i = 0ull; i < texture_array.size(); ++i)
				{
					const auto& tex = texture_array[i];
					const auto addr = rsx::get_address(tex.offset(), tex.location());
					exclusion_list.insert(addr);
				}
			};

			scan_array(rsx::method_registers.fragment_textures);
			scan_array(rsx::method_registers.vertex_textures);

			// Hold the secondary lock guard to prevent threads from trying to touch access violation handler stuff
			std::lock_guard lock(m_secondary_cb_guard);

			rsx_log.warning("Texture cache is overallocated. Will evict unnecessary textures.");
			texture_cache_relieved = m_texture_cache.evict_unused(exclusion_list);
		}
	}

	texture_cache_relieved |= m_texture_cache.handle_memory_pressure(severity);
	if (severity == rsx::problem_severity::low)
	{
		// Low severity only handles invalidating unused textures
		return texture_cache_relieved;
	}

	bool surface_cache_relieved = false;
	const auto mem_info = m_device->get_memory_mapping();

	// Check if we need to spill
	if (severity >= rsx::problem_severity::fatal &&                // Only spill for fatal errors
		mem_info.device_local != mem_info.host_visible_coherent && // Do not spill if it is an IGP, there is nowhere to spill to
		m_rtts.is_overallocated())                                 // Surface cache must be over-allocated by the design quota
	{
		// Queue a VRAM spill operation.
		m_rtts.spill_unused_memory();
	}

	// Moderate severity and higher also starts removing stale render target objects
	if (m_rtts.handle_memory_pressure(*m_current_command_buffer, severity))
	{
		surface_cache_relieved = true;
		m_rtts.trim(*m_current_command_buffer, severity);
	}

	const bool any_cache_relieved = (texture_cache_relieved || surface_cache_relieved);
	if (severity < rsx::problem_severity::fatal)
	{
		return any_cache_relieved;
	}

	if (surface_cache_relieved && !m_samplers_dirty)
	{
		// If surface cache was modified destructively, then we must reload samplers touching the surface cache.
		bool invalidate_samplers = false;
		auto scan_array = [&](const auto& texture_array, const auto& sampler_states)
		{
			if (invalidate_samplers)
			{
				return;
			}

			for (auto i = 0ull; i < texture_array.size(); ++i)
			{
				if (texture_array[i].enabled() &&
					sampler_states[i] &&
					sampler_states[i]->upload_context == rsx::texture_upload_context::framebuffer_storage)
				{
					invalidate_samplers = true;
					break;
				}
			}
		};

		scan_array(rsx::method_registers.fragment_textures, fs_sampler_state);
		scan_array(rsx::method_registers.vertex_textures, vs_sampler_state);

		if (invalidate_samplers)
		{
			m_samplers_dirty.store(true);
		}
	}

	// Imminent crash, full GPU sync is the least of our problems
	flush_command_queue(true, true);

	return any_cache_relieved;
}

void VKGSRender::on_descriptor_pool_fragmentation(bool is_fatal)
{
	if (!is_fatal)
	{
		// It is very likely that the release is simply in progress (enqueued)
		m_primary_cb_list.wait_all();
		return;
	}

	// Just flush everything. Unless the hardware is very deficient, this should happen very rarely.
	flush_command_queue(true, true);
}

void VKGSRender::notify_tile_unbound(u32 tile)
{
	//TODO: Handle texture writeback
	if (false)
	{
		u32 addr = rsx::get_address(tiles[tile].offset, tiles[tile].location);
		on_notify_pre_memory_unmapped(addr, tiles[tile].size, *std::make_unique<std::vector<std::pair<u64, u64>>>());
		m_rtts.invalidate_surface_address(addr, false);
	}

	{
		std::lock_guard lock(m_sampler_mutex);
		m_samplers_dirty.store(true);
	}
}

void VKGSRender::check_present_status()
{
	while (!m_queued_frames.empty())
	{
		auto ctx = m_queued_frames.front();
		if (!ctx->swap_command_buffer->poke())
		{
			return;
		}

		frame_context_cleanup(ctx);
	}
}

void VKGSRender::set_viewport()
{
	const auto [clip_width, clip_height] = rsx::apply_resolution_scale<true>(
		rsx::method_registers.surface_clip_width(), rsx::method_registers.surface_clip_height());

	const auto zclip_near = rsx::method_registers.clip_min();
	const auto zclip_far = rsx::method_registers.clip_max();

	//NOTE: The scale_offset matrix already has viewport matrix factored in
	m_viewport.x = 0;
	m_viewport.y = 0;
	m_viewport.width = clip_width;
	m_viewport.height = clip_height;

	if (m_device->get_unrestricted_depth_range_support())
	{
		m_viewport.minDepth = zclip_near;
		m_viewport.maxDepth = zclip_far;
	}
	else
	{
		m_viewport.minDepth = 0.f;
		m_viewport.maxDepth = 1.f;
	}

	m_current_command_buffer->flags |= vk::command_buffer::cb_reload_dynamic_state;
	m_graphics_state.clear(rsx::pipeline_state::zclip_config_state_dirty);
}

void VKGSRender::set_scissor(bool clip_viewport)
{
	areau scissor;
	if (get_scissor(scissor, clip_viewport))
	{
		m_scissor.extent.height = scissor.height();
		m_scissor.extent.width = scissor.width();
		m_scissor.offset.x = scissor.x1;
		m_scissor.offset.y = scissor.y1;

		m_current_command_buffer->flags |= vk::command_buffer::cb_reload_dynamic_state;
	}
}

void VKGSRender::bind_viewport()
{
	if (m_graphics_state & rsx::pipeline_state::zclip_config_state_dirty)
	{
		if (m_device->get_unrestricted_depth_range_support())
		{
			m_viewport.minDepth = rsx::method_registers.clip_min();
			m_viewport.maxDepth = rsx::method_registers.clip_max();
		}

		m_graphics_state.clear(rsx::pipeline_state::zclip_config_state_dirty);
	}

	vkCmdSetViewport(*m_current_command_buffer, 0, 1, &m_viewport);
	vkCmdSetScissor(*m_current_command_buffer, 0, 1, &m_scissor);
}

void VKGSRender::on_init_thread()
{
	if (m_device == VK_NULL_HANDLE)
	{
		fmt::throw_exception("No Vulkan device was created");
	}

	GSRender::on_init_thread();
	zcull_ctrl.reset(static_cast<::rsx::reports::ZCULL_control*>(this));

	if (!m_overlay_manager)
	{
		m_frame->hide();
		m_shaders_cache->load(nullptr);
		m_frame->show();
	}
	else
	{
		rsx::shader_loading_dialog_native dlg(this);

		// TODO: Handle window resize messages during loading on GPUs without OUT_OF_DATE_KHR support
		m_shaders_cache->load(&dlg);
	}
}

void VKGSRender::on_exit()
{
	GSRender::on_exit();
	vk::destroy_pipe_compiler(); // Ensure no pending shaders being compiled
	zcull_ctrl.release();
}

void VKGSRender::clear_surface(u32 mask)
{
	if (skip_current_frame || swapchain_unavailable) return;

	// If stencil write mask is disabled, remove clear_stencil bit
	if (!rsx::method_registers.stencil_mask()) mask &= ~RSX_GCM_CLEAR_STENCIL_BIT;

	// Ignore invalid clear flags
	if (!(mask & RSX_GCM_CLEAR_ANY_MASK)) return;

	u8 ctx = rsx::framebuffer_creation_context::context_draw;
	if (mask & RSX_GCM_CLEAR_COLOR_RGBA_MASK) ctx |= rsx::framebuffer_creation_context::context_clear_color;
	if (mask & RSX_GCM_CLEAR_DEPTH_STENCIL_MASK) ctx |= rsx::framebuffer_creation_context::context_clear_depth;
	init_buffers(rsx::framebuffer_creation_context{ctx});

	if (!m_graphics_state.test(rsx::rtt_config_valid))
	{
		return;
	}

	//float depth_clear = 1.f;
	u32   stencil_clear = 0;
	u32   depth_stencil_mask = 0;

	std::vector<VkClearAttachment> clear_descriptors;
	VkClearValue depth_stencil_clear_values = {}, color_clear_values = {};

	u16 scissor_x = static_cast<u16>(m_scissor.offset.x);
	u16 scissor_w = static_cast<u16>(m_scissor.extent.width);
	u16 scissor_y = static_cast<u16>(m_scissor.offset.y);
	u16 scissor_h = static_cast<u16>(m_scissor.extent.height);

	const u16 fb_width = m_draw_fbo->width();
	const u16 fb_height = m_draw_fbo->height();

	//clip region
	std::tie(scissor_x, scissor_y, scissor_w, scissor_h) = rsx::clip_region<u16>(fb_width, fb_height, scissor_x, scissor_y, scissor_w, scissor_h, true);
	VkClearRect region = { { { scissor_x, scissor_y }, { scissor_w, scissor_h } }, 0, 1 };

	const bool full_frame = (scissor_w == fb_width && scissor_h == fb_height);
	bool update_color = false, update_z = false;
	auto surface_depth_format = rsx::method_registers.surface_depth_fmt();

	if (auto ds = std::get<1>(m_rtts.m_bound_depth_stencil); mask & RSX_GCM_CLEAR_DEPTH_STENCIL_MASK)
	{
		if (mask & RSX_GCM_CLEAR_DEPTH_BIT)
		{
			u32 max_depth_value = get_max_depth_value(surface_depth_format);

			u32 clear_depth = rsx::method_registers.z_clear_value(is_depth_stencil_format(surface_depth_format));
			float depth_clear = static_cast<float>(clear_depth) / max_depth_value;

			depth_stencil_clear_values.depthStencil.depth = depth_clear;
			depth_stencil_clear_values.depthStencil.stencil = stencil_clear;

			depth_stencil_mask |= VK_IMAGE_ASPECT_DEPTH_BIT;
		}

		if (is_depth_stencil_format(surface_depth_format))
		{
			if (mask & RSX_GCM_CLEAR_STENCIL_BIT)
			{
				u8 clear_stencil = rsx::method_registers.stencil_clear_value();
				depth_stencil_clear_values.depthStencil.stencil = clear_stencil;

				depth_stencil_mask |= VK_IMAGE_ASPECT_STENCIL_BIT;

				if (ds->samples() > 1)
				{
					if (full_frame) ds->stencil_init_flags &= 0xFF;
					ds->stencil_init_flags |= clear_stencil;
				}
			}
		}

		if ((depth_stencil_mask && depth_stencil_mask != ds->aspect()) || !full_frame)
		{
			// At least one aspect is not being cleared or the clear does not cover the full frame
			// Steps to initialize memory are required

			if (ds->state_flags & rsx::surface_state_flags::erase_bkgnd &&  // Needs initialization
				ds->old_contents.empty() && !g_cfg.video.read_depth_buffer) // No way to load data from memory, so no initialization given
			{
				// Only one aspect was cleared. Make sure to memory initialize the other before removing dirty flag
				const auto ds_mask = (mask & RSX_GCM_CLEAR_DEPTH_STENCIL_MASK);
				if (ds_mask == RSX_GCM_CLEAR_DEPTH_BIT && (ds->aspect() & VK_IMAGE_ASPECT_STENCIL_BIT))
				{
					// Depth was cleared, initialize stencil
					depth_stencil_clear_values.depthStencil.stencil = 0xFF;
					depth_stencil_mask |= VK_IMAGE_ASPECT_STENCIL_BIT;
				}
				else if (ds_mask == RSX_GCM_CLEAR_STENCIL_BIT)
				{
					// Stencil was cleared, initialize depth
					depth_stencil_clear_values.depthStencil.depth = 1.f;
					depth_stencil_mask |= VK_IMAGE_ASPECT_DEPTH_BIT;
				}
			}
			else
			{
				// Barrier required before any writes
				ds->write_barrier(*m_current_command_buffer);
			}
		}
	}

	if (auto colormask = (mask & RSX_GCM_CLEAR_COLOR_RGBA_MASK))
	{
		if (!m_draw_buffers.empty())
		{
			bool use_fast_clear = (colormask == RSX_GCM_CLEAR_COLOR_RGBA_MASK);;
			u8 clear_a = rsx::method_registers.clear_color_a();
			u8 clear_r = rsx::method_registers.clear_color_r();
			u8 clear_g = rsx::method_registers.clear_color_g();
			u8 clear_b = rsx::method_registers.clear_color_b();

			switch (rsx::method_registers.surface_color())
			{
			case rsx::surface_color_format::x32:
			case rsx::surface_color_format::w16z16y16x16:
			case rsx::surface_color_format::w32z32y32x32:
			{
				//NOP
				colormask = 0;
				break;
			}
			case rsx::surface_color_format::b8:
			{
				rsx::get_b8_clear_color(clear_r, clear_g, clear_b, clear_a);
				colormask = rsx::get_b8_clearmask(colormask);
				use_fast_clear = (colormask & RSX_GCM_CLEAR_RED_BIT);
				break;
			}
			case rsx::surface_color_format::g8b8:
			{
				rsx::get_g8b8_clear_color(clear_r, clear_g, clear_b, clear_a);
				colormask = rsx::get_g8b8_r8g8_clearmask(colormask);
				use_fast_clear = ((colormask & RSX_GCM_CLEAR_COLOR_RG_MASK) == RSX_GCM_CLEAR_COLOR_RG_MASK);
				break;
			}
			case rsx::surface_color_format::r5g6b5:
			{
				rsx::get_rgb565_clear_color(clear_r, clear_g, clear_b, clear_a);
				use_fast_clear = ((colormask & RSX_GCM_CLEAR_COLOR_RGB_MASK) == RSX_GCM_CLEAR_COLOR_RGB_MASK);
				break;
			}
			case rsx::surface_color_format::x1r5g5b5_o1r5g5b5:
			{
				rsx::get_a1rgb555_clear_color(clear_r, clear_g, clear_b, clear_a, 255);
				break;
			}
			case rsx::surface_color_format::x1r5g5b5_z1r5g5b5:
			{
				rsx::get_a1rgb555_clear_color(clear_r, clear_g, clear_b, clear_a, 0);
				break;
			}
			case rsx::surface_color_format::a8b8g8r8:
			case rsx::surface_color_format::x8b8g8r8_o8b8g8r8:
			case rsx::surface_color_format::x8b8g8r8_z8b8g8r8:
			{
				rsx::get_abgr8_clear_color(clear_r, clear_g, clear_b, clear_a);
				colormask = rsx::get_abgr8_clearmask(colormask);
				break;
			}
			default:
			{
				break;
			}
			}

			if (colormask)
			{
				if (!use_fast_clear || !full_frame)
				{
					// If we're not clobber all the memory, a barrier is required
					for (const auto& index : m_rtts.m_bound_render_target_ids)
					{
						m_rtts.m_bound_render_targets[index].second->write_barrier(*m_current_command_buffer);
					}
				}

				color_clear_values.color.float32[0] = static_cast<float>(clear_r) / 255;
				color_clear_values.color.float32[1] = static_cast<float>(clear_g) / 255;
				color_clear_values.color.float32[2] = static_cast<float>(clear_b) / 255;
				color_clear_values.color.float32[3] = static_cast<float>(clear_a) / 255;

				if (use_fast_clear)
				{
					for (u32 index = 0; index < m_draw_buffers.size(); ++index)
					{
						clear_descriptors.push_back({ VK_IMAGE_ASPECT_COLOR_BIT, index, color_clear_values });
					}
				}
				else
				{
					color4f clear_color =
					{
						color_clear_values.color.float32[0],
						color_clear_values.color.float32[1],
						color_clear_values.color.float32[2],
						color_clear_values.color.float32[3]
					};

					auto attachment_clear_pass = vk::get_overlay_pass<vk::attachment_clear_pass>();
					attachment_clear_pass->run(*m_current_command_buffer, m_draw_fbo, region.rect, colormask, clear_color, get_render_pass());
				}

				update_color = true;
			}
		}
	}

	if (depth_stencil_mask)
	{
		if ((depth_stencil_mask & VK_IMAGE_ASPECT_STENCIL_BIT) &&
			rsx::method_registers.stencil_mask() != 0xff)
		{
			// Partial stencil clear. Disables fast stencil clear
			auto ds = std::get<1>(m_rtts.m_bound_depth_stencil);
			auto key = vk::get_renderpass_key({ ds });
			auto renderpass = vk::get_renderpass(*m_device, key);

			vk::get_overlay_pass<vk::stencil_clear_pass>()->run(
				*m_current_command_buffer, ds, region.rect,
				depth_stencil_clear_values.depthStencil.stencil,
				rsx::method_registers.stencil_mask(), renderpass);

			depth_stencil_mask &= ~VK_IMAGE_ASPECT_STENCIL_BIT;
		}

		if (depth_stencil_mask)
		{
			clear_descriptors.push_back({ static_cast<VkImageAspectFlags>(depth_stencil_mask), 0, depth_stencil_clear_values });
		}

		update_z = true;
	}

	if (update_color || update_z)
	{
		m_rtts.on_write({ update_color, update_color, update_color, update_color }, update_z);
	}

	if (!clear_descriptors.empty())
	{
		begin_render_pass();
		vkCmdClearAttachments(*m_current_command_buffer, ::size32(clear_descriptors), clear_descriptors.data(), 1, &region);
	}
}

void VKGSRender::flush_command_queue(bool hard_sync, bool do_not_switch)
{
	close_and_submit_command_buffer();

	if (hard_sync)
	{
		// wait for the latest instruction to execute
		m_current_command_buffer->reset();

		// Clear all command buffer statuses
		m_primary_cb_list.poke_all();

		// Drain present queue
		while (!m_queued_frames.empty())
		{
			check_present_status();
		}

		m_flush_requests.clear_pending_flag();
	}

	if (!do_not_switch)
	{
		// Grab next cb in line and make it usable
		// NOTE: Even in the case of a hard sync, this is required to free any waiters on the CB (ZCULL)
		m_current_command_buffer = m_primary_cb_list.next();
		m_current_command_buffer->reset();
	}
	else
	{
		// Special hard-sync where we must preserve the CB. This can happen when an emergency event handler is invoked and needs to flush to hw.
		ensure(hard_sync);
	}

	// Just in case a queued frame holds a ref to this cb, drain the present queue
	check_present_status();

	if (m_occlusion_query_active)
	{
		m_current_command_buffer->flags |= vk::command_buffer::cb_load_occluson_task;
	}

	m_current_command_buffer->begin();
}

std::pair<volatile vk::host_data_t*, VkBuffer> VKGSRender::map_host_object_data() const
{
	return { m_host_dma_ctrl->host_ctx(), m_host_object_data->value };
}

bool VKGSRender::release_GCM_label(u32 address, u32 args)
{
	if (!backend_config.supports_host_gpu_labels)
	{
		return false;
	}

	auto host_ctx = ensure(m_host_dma_ctrl->host_ctx());

	if (host_ctx->texture_loads_completed())
	{
		// All texture loads already seen by the host GPU
		// Wait for all previously submitted labels to be flushed
		m_host_dma_ctrl->drain_label_queue();
		return false;
	}

	const auto mapping = vk::map_dma(address, 4);
	const auto write_data = std::bit_cast<u32, be_t<u32>>(args);

	if (!dynamic_cast<vk::memory_block_host*>(mapping.second->memory.get()))
	{
		// NVIDIA GPUs can disappoint when DMA blocks straddle VirtualAlloc boundaries.
		// Take the L and try the fallback.
		rsx_log.warning("Host label update at 0x%x was not possible.", address);
		m_host_dma_ctrl->drain_label_queue();
		return false;
	}

	const auto release_event_id = host_ctx->on_label_acquire();

	if (host_ctx->has_unflushed_texture_loads())
	{
		if (vk::is_renderpass_open(*m_current_command_buffer))
		{
			vk::end_renderpass(*m_current_command_buffer);
		}

		vkCmdUpdateBuffer(*m_current_command_buffer, mapping.second->value, mapping.first, 4, &write_data);
		flush_command_queue();
	}
	else
	{
		auto cmd = m_secondary_cb_list.next();
		cmd->begin();
		vkCmdUpdateBuffer(*cmd, mapping.second->value, mapping.first, 4, &write_data);
		vkCmdUpdateBuffer(*cmd, m_host_object_data->value, ::offset32(&vk::host_data_t::commands_complete_event), 8, &release_event_id);
		cmd->end();

		vk::queue_submit_t submit_info = { m_device->get_graphics_queue(), nullptr };
		cmd->submit(submit_info);

		host_ctx->on_label_release();
	}

	return true;
}

void VKGSRender::on_guest_texture_read(const vk::command_buffer& cmd)
{
	if (!backend_config.supports_host_gpu_labels)
	{
		return;
	}

	// Queue a sync update on the CB doing the load
	auto host_ctx = ensure(m_host_dma_ctrl->host_ctx());
	const auto event_id = host_ctx->on_texture_load_acquire();
	vkCmdUpdateBuffer(cmd, m_host_object_data->value, ::offset32(&vk::host_data_t::texture_load_complete_event), sizeof(u64), &event_id);
}

void VKGSRender::sync_hint(rsx::FIFO::interrupt_hint hint, rsx::reports::sync_hint_payload_t payload)
{
	rsx::thread::sync_hint(hint, payload);

	if (!(m_current_command_buffer->flags & vk::command_buffer::cb_has_occlusion_task))
	{
		// Occlusion queries not enabled, do nothing
		return;
	}

	// Occlusion test result evaluation is coming up, avoid a hard sync
	switch (hint)
	{
	case rsx::FIFO::interrupt_hint::conditional_render_eval:
	{
		// If a flush request is already enqueued, do nothing
		if (m_flush_requests.pending())
		{
			return;
		}

		// If the result is not going to be read by CELL, do nothing
		const auto ref_addr = static_cast<u32>(payload.address);
		if (!zcull_ctrl->is_query_result_urgent(ref_addr))
		{
			// No effect on CELL behaviour, it will be faster to handle this in RSX code
			return;
		}

		// OK, cell will be accessing the results, probably.
		// Try to avoid flush spam, it is more costly to flush the CB than it is to just upload the vertex data
		// This is supposed to be an optimization afterall.
		const auto now = get_system_time();
		if ((now - m_last_cond_render_eval_hint) > 50)
		{
			// Schedule a sync on the next loop iteration
			m_flush_requests.post(false);
			m_flush_requests.remove_one();
		}

		m_last_cond_render_eval_hint = now;
		break;
	}
	case rsx::FIFO::interrupt_hint::zcull_sync:
	{
		// Check if the required report is synced to this CB
		auto& data = m_occlusion_map[payload.query->driver_handle];

		// NOTE: Currently, a special condition exists where the indices can be empty even with active draw count.
		// This is caused by async compiler and should be removed when ubershaders are added in
		if (!data.is_current(m_current_command_buffer) || data.indices.empty())
		{
			return;
		}

		// Unavoidable hard sync coming up, flush immediately
		// This heavyweight hint should be used with caution
		std::lock_guard lock(m_flush_queue_mutex);
		flush_command_queue();

		if (m_flush_requests.pending())
		{
			// Clear without wait
			m_flush_requests.clear_pending_flag();
		}
		break;
	}
	}
}

void VKGSRender::do_local_task(rsx::FIFO::state state)
{
	if (m_queue_status & flush_queue_state::deadlock)
	{
		// Clear offloader deadlock
		// NOTE: It is not possible to handle regular flush requests before this is cleared
		// NOTE: This may cause graphics corruption due to unsynchronized modification
		on_invalidate_memory_range(m_offloader_fault_range, m_offloader_fault_cause);
		m_queue_status.clear(flush_queue_state::deadlock);
	}

	if (m_queue_status & flush_queue_state::flushing)
	{
		// Abort recursive CB submit requests.
		// When flushing flag is already set, only deadlock events may be processed.
		return;
	}
	else if (m_flush_requests.pending())
	{
		if (m_flush_queue_mutex.try_lock())
		{
			// TODO: Determine if a hard sync is necessary
			// Pipeline barriers later may do a better job synchronizing than wholly stalling the pipeline
			flush_command_queue();

			m_flush_requests.clear_pending_flag();
			m_flush_requests.consumer_wait();
			m_flush_queue_mutex.unlock();
		}
	}
	else if (!in_begin_end && state != rsx::FIFO::state::lock_wait)
	{
		if (m_graphics_state & rsx::pipeline_state::framebuffer_reads_dirty)
		{
			//This will re-engage locks and break the texture cache if another thread is waiting in access violation handler!
			//Only call when there are no waiters
			m_texture_cache.do_update();
			m_graphics_state.clear(rsx::pipeline_state::framebuffer_reads_dirty);
		}
	}

	rsx::thread::do_local_task(state);

	switch (state)
	{
	case rsx::FIFO::state::lock_wait:
		// Critical check finished
		return;
	//case rsx::FIFO::state::spinning:
	//case rsx::FIFO::state::empty:
		// We have some time, check the present queue
		//check_present_status();
		//break;
	default:
		break;
	}

	if (m_overlay_manager)
	{
		const auto should_ignore = in_begin_end && state != rsx::FIFO::state::empty;
		if ((async_flip_requested & flip_request::native_ui) && !should_ignore && !is_stopped())
		{
			flush_command_queue(true);
			rsx::display_flip_info_t info{};
			info.buffer = current_display_buffer;
			flip(info);
		}
	}
}

bool VKGSRender::load_program()
{
	const auto shadermode = g_cfg.video.shadermode.get();

	// TODO: EXT_dynamic_state should get rid of this sillyness soon (kd)
	const auto vertex_state = vk::decode_vertex_input_assembly_state();

	if (m_graphics_state & rsx::pipeline_state::invalidate_pipeline_bits)
	{
		get_current_fragment_program(fs_sampler_state);
		ensure(current_fragment_program.valid);

		get_current_vertex_program(vs_sampler_state);

		m_graphics_state.clear(rsx::pipeline_state::invalidate_pipeline_bits);
	}
	else if (!(m_graphics_state & rsx::pipeline_state::pipeline_config_dirty) &&
		m_program &&
		m_pipeline_properties.state.ia.topology == vertex_state.primitive &&
		m_pipeline_properties.state.ia.primitiveRestartEnable == vertex_state.restart_index_enabled)
	{
		if (!m_shader_interpreter.is_interpreter(m_program)) [[ likely ]]
		{
			return true;
		}

		if (shadermode == shader_mode::interpreter_only)
		{
			m_program = m_shader_interpreter.get(
				m_pipeline_properties,
				current_fp_metadata,
				current_vp_metadata,
				current_vertex_program.ctrl,
				current_fragment_program.ctrl);

			std::tie(m_vs_binding_table, m_fs_binding_table) = get_binding_table();
			return true;
		}
	}

	auto &vertex_program = current_vertex_program;
	auto &fragment_program = current_fragment_program;

	if (m_graphics_state & rsx::pipeline_state::pipeline_config_dirty)
	{
		vk::pipeline_props properties = vk::decode_rsx_state(
			vertex_state,
			m_rtts.m_bound_depth_stencil.second,
			backend_config,
			static_cast<u8>(m_draw_buffers.size()),
			u8((m_current_renderpass_key >> 16) & 0xF),
			m_device->get_depth_bounds_support()
		);

		properties.renderpass_key = m_current_renderpass_key;
		if (m_program &&
			!m_shader_interpreter.is_interpreter(m_program) &&
			m_pipeline_properties == properties)
		{
			// Nothing changed
			return true;
		}

		// Fallthrough
		m_pipeline_properties = properties;
		m_graphics_state.clear(rsx::pipeline_state::pipeline_config_dirty);
	}
	else
	{
		// Update primitive type and restart index. Note that this is not needed with EXT_dynamic_state
		m_pipeline_properties.state.set_primitive_type(vertex_state.primitive);
		m_pipeline_properties.state.enable_primitive_restart(vertex_state.restart_index_enabled);
		m_pipeline_properties.renderpass_key = m_current_renderpass_key;
	}

	m_vertex_prog = nullptr;
	m_fragment_prog = nullptr;

	if (shadermode != shader_mode::interpreter_only) [[likely]]
	{
		vk::enter_uninterruptible();

		if (g_cfg.video.debug_overlay)
		{
			m_frame_stats.program_cache_lookups_total += 2;
			if (m_program_cache_hint.has_fragment_program())
			{
				m_frame_stats.program_cache_lookups_ellided++;
			}
			if (m_program_cache_hint.has_vertex_program())
			{
				m_frame_stats.program_cache_lookups_ellided++;
			}
		}

		// Load current program from cache
		std::tie(m_program, m_vertex_prog, m_fragment_prog) = m_prog_buffer->get_graphics_pipeline(
			&m_program_cache_hint,
			vertex_program,
			fragment_program,
			m_pipeline_properties,
			shadermode != shader_mode::recompiler, true);

		vk::leave_uninterruptible();

		if (m_prog_buffer->check_cache_missed())
		{
			// Notify the user with HUD notification
			if (g_cfg.misc.show_shader_compilation_hint)
			{
				if (m_overlay_manager)
				{
					rsx::overlays::show_shader_compile_notification();
				}
			}
		}
	}
	else
	{
		m_program = nullptr;
	}

	if (shadermode == shader_mode::async_with_interpreter || shadermode == shader_mode::interpreter_only)
	{
		const bool is_interpreter = !m_program;
		const bool was_interpreter = m_shader_interpreter.is_interpreter(m_prev_program);

		// First load the next program if not available
		if (!m_program)
		{
			m_program = m_shader_interpreter.get(
				m_pipeline_properties,
				current_fp_metadata,
				current_vp_metadata,
				current_vertex_program.ctrl,
				current_fragment_program.ctrl);

			// Program has changed, reupload
			m_interpreter_state = rsx::invalidate_pipeline_bits;
		}

		// If swapping between interpreter and recompiler, we need to adjust some flags to reupload data as needed.
		if (is_interpreter != was_interpreter)
		{
			// Always reupload transform constants when going between interpreter and recompiler
			m_graphics_state |= rsx::transform_constants_dirty;

			// Always reload fragment constansts when moving from interpreter back to recompiler.
			if (was_interpreter)
			{
				m_graphics_state |= rsx::fragment_constants_dirty;
			}
		}
	}

	if (m_program)
	{
		std::tie(m_vs_binding_table, m_fs_binding_table) = get_binding_table();
	}
	else
	{
		m_vs_binding_table = nullptr;
		m_fs_binding_table = nullptr;
	}

	return m_program != nullptr;
}

void VKGSRender::load_program_env()
{
	if (!m_program)
	{
		fmt::throw_exception("Unreachable right now");
	}

	const u32 fragment_constants_size = current_fp_metadata.program_constants_buffer_length;
	const bool is_interpreter = m_shader_interpreter.is_interpreter(m_program);

	const bool update_transform_constants = !!(m_graphics_state & rsx::pipeline_state::transform_constants_dirty);
	const bool update_fragment_constants = !!(m_graphics_state & rsx::pipeline_state::fragment_constants_dirty);
	const bool update_vertex_env = !!(m_graphics_state & rsx::pipeline_state::vertex_state_dirty);
	const bool update_fragment_env = !!(m_graphics_state & rsx::pipeline_state::fragment_state_dirty);
	const bool update_fragment_texture_env = !!(m_graphics_state & rsx::pipeline_state::fragment_texture_state_dirty);
	const bool update_instruction_buffers = (!!m_interpreter_state && is_interpreter);
	const bool update_raster_env = (rsx::method_registers.polygon_stipple_enabled() && !!(m_graphics_state & rsx::pipeline_state::polygon_stipple_pattern_dirty));
	const bool update_instancing_data = rsx::method_registers.current_draw_clause.is_trivial_instanced_draw;

	if (update_vertex_env)
	{
		// Vertex state
		const auto mem = m_vertex_env_ring_info.static_alloc<256>();
		auto buf = static_cast<u8*>(m_vertex_env_ring_info.map(mem, 148));

		m_draw_processor.fill_scale_offset_data(buf, false);
		m_draw_processor.fill_user_clip_data(buf + 64);
		*(reinterpret_cast<u32*>(buf + 128)) = rsx::method_registers.transform_branch_bits();
		*(reinterpret_cast<f32*>(buf + 132)) = rsx::method_registers.point_size() * rsx::get_resolution_scale();
		*(reinterpret_cast<f32*>(buf + 136)) = rsx::method_registers.clip_min();
		*(reinterpret_cast<f32*>(buf + 140)) = rsx::method_registers.clip_max();

		m_vertex_env_ring_info.unmap();
		m_vertex_env_buffer_info = { m_vertex_env_ring_info.heap->value, mem, 144 };
	}

	if (update_instancing_data)
	{
		// Combines transform load + instancing lookup table
		const auto alignment = m_device->gpu().get_limits().minStorageBufferOffsetAlignment;
		usz indirection_table_offset = 0;
		usz constants_data_table_offset = 0;

		rsx::io_buffer indirection_table_buf([&](usz size) -> std::pair<void*, usz>
		{
			indirection_table_offset = m_instancing_buffer_ring_info.alloc<1>(utils::align(size, alignment));
			return std::make_pair(m_instancing_buffer_ring_info.map(indirection_table_offset, size), size);
		});

		rsx::io_buffer constants_array_buf([&](usz size) -> std::pair<void*, usz>
		{
			constants_data_table_offset = m_instancing_buffer_ring_info.alloc<1>(utils::align(size, alignment));
			return std::make_pair(m_instancing_buffer_ring_info.map(constants_data_table_offset, size), size);
		});

		const auto bound_vertex_prog = m_shader_interpreter.is_interpreter(m_program) ? nullptr : m_vertex_prog;
		m_draw_processor.fill_constants_instancing_buffer(indirection_table_buf, constants_array_buf, bound_vertex_prog);
		m_instancing_buffer_ring_info.unmap();

		m_instancing_indirection_buffer_info = { m_instancing_buffer_ring_info.heap->value, indirection_table_offset, indirection_table_buf.size() };
		m_instancing_constants_array_buffer_info = { m_instancing_buffer_ring_info.heap->value, constants_data_table_offset, constants_array_buf.size() };
	}
	else if (update_transform_constants)
	{
		// Transform constants
		usz mem_offset = 0;
		auto alloc_storage = [&](usz size) -> std::pair<void*, usz>
		{
			const auto alignment = m_device->gpu().get_limits().minStorageBufferOffsetAlignment;
			mem_offset = m_transform_constants_ring_info.alloc<1>(utils::align(size, alignment));
			return std::make_pair(m_transform_constants_ring_info.map(mem_offset, size), size);
		};

		auto io_buf = rsx::io_buffer(alloc_storage);
		upload_transform_constants(io_buf);

		if (!io_buf.empty())
		{
			m_transform_constants_ring_info.unmap();
			m_vertex_constants_buffer_info = { m_transform_constants_ring_info.heap->value, 0, VK_WHOLE_SIZE };
			m_xform_constants_dynamic_offset = mem_offset;
		}
	}

	if (update_fragment_constants && !m_shader_interpreter.is_interpreter(m_program))
	{
		// Fragment constants
		if (fragment_constants_size)
		{
			auto mem = m_fragment_constants_ring_info.alloc<256>(fragment_constants_size);
			auto buf = m_fragment_constants_ring_info.map(mem, fragment_constants_size);

			m_prog_buffer->fill_fragment_constants_buffer({ reinterpret_cast<float*>(buf), fragment_constants_size },
				*ensure(m_fragment_prog), current_fragment_program, true);

			m_fragment_constants_ring_info.unmap();
			m_fragment_constants_buffer_info = { m_fragment_constants_ring_info.heap->value, mem, fragment_constants_size };
		}
		else
		{
			m_fragment_constants_buffer_info = { m_fragment_constants_ring_info.heap->value, 0, 32 };
		}
	}

	if (update_fragment_env)
	{
		auto mem = m_fragment_env_ring_info.static_alloc<256>();
		auto buf = m_fragment_env_ring_info.map(mem, 32);

		m_draw_processor.fill_fragment_state_buffer(buf, current_fragment_program);
		m_fragment_env_ring_info.unmap();
		m_fragment_env_buffer_info = { m_fragment_env_ring_info.heap->value, mem, 32 };
	}

	if (update_fragment_texture_env)
	{
		auto mem = m_fragment_texture_params_ring_info.static_alloc<256, 768>();
		auto buf = m_fragment_texture_params_ring_info.map(mem, 768);

		current_fragment_program.texture_params.write_to(buf, current_fp_metadata.referenced_textures_mask);
		m_fragment_texture_params_ring_info.unmap();
		m_fragment_texture_params_buffer_info = { m_fragment_texture_params_ring_info.heap->value, mem, 768 };
	}

	if (update_raster_env)
	{
		auto mem = m_raster_env_ring_info.static_alloc<256>();
		auto buf = m_raster_env_ring_info.map(mem, 128);

		std::memcpy(buf, rsx::method_registers.polygon_stipple_pattern(), 128);
		m_raster_env_ring_info.unmap();
		m_raster_env_buffer_info = { m_raster_env_ring_info.heap->value, mem, 128 };

		m_graphics_state.clear(rsx::pipeline_state::polygon_stipple_pattern_dirty);
	}

	if (update_instruction_buffers)
	{
		if (m_interpreter_state & rsx::vertex_program_dirty)
		{
			// Attach vertex buffer data
			const auto vp_block_length = current_vp_metadata.ucode_length + 16;
			auto vp_mapping = m_vertex_instructions_buffer.alloc<256>(vp_block_length);
			auto vp_buf = static_cast<u8*>(m_vertex_instructions_buffer.map(vp_mapping, vp_block_length));

			auto vp_config = reinterpret_cast<u32*>(vp_buf);
			vp_config[0] = current_vertex_program.base_address;
			vp_config[1] = current_vertex_program.entry;
			vp_config[2] = current_vertex_program.output_mask;
			vp_config[3] = rsx::method_registers.two_side_light_en()? 1u: 0u;

			std::memcpy(vp_buf + 16, current_vertex_program.data.data(), current_vp_metadata.ucode_length);
			m_vertex_instructions_buffer.unmap();

			m_vertex_instructions_buffer_info = { m_vertex_instructions_buffer.heap->value, vp_mapping, vp_block_length };
		}

		if (m_interpreter_state & rsx::fragment_program_dirty)
		{
			// Attach fragment buffer data
			const auto fp_block_length = current_fp_metadata.program_ucode_length + 16;
			auto fp_mapping = m_fragment_instructions_buffer.alloc<256>(fp_block_length);
			auto fp_buf = static_cast<u8*>(m_fragment_instructions_buffer.map(fp_mapping, fp_block_length));

			// Control mask
			const auto control_masks = reinterpret_cast<u32*>(fp_buf);
			control_masks[0] = rsx::method_registers.shader_control();
			control_masks[1] = current_fragment_program.texture_state.texture_dimensions;

			std::memcpy(fp_buf + 16, current_fragment_program.get_data(), current_fragment_program.ucode_length);
			m_fragment_instructions_buffer.unmap();

			m_fragment_instructions_buffer_info = { m_fragment_instructions_buffer.heap->value, fp_mapping, fp_block_length };
		}
	}

	m_program->bind_uniform(m_vertex_env_buffer_info, vk::glsl::binding_set_index_vertex, m_vs_binding_table->context_buffer_location);
	m_program->bind_uniform(m_fragment_env_buffer_info, vk::glsl::binding_set_index_fragment, m_fs_binding_table->context_buffer_location);
	m_program->bind_uniform(m_fragment_texture_params_buffer_info, vk::glsl::binding_set_index_fragment, m_fs_binding_table->tex_param_location);
	m_program->bind_uniform(m_raster_env_buffer_info, vk::glsl::binding_set_index_fragment, m_fs_binding_table->polygon_stipple_params_location);

	if (m_vs_binding_table->cbuf_location != umax)
	{
		m_program->bind_uniform(m_vertex_constants_buffer_info, vk::glsl::binding_set_index_vertex, m_vs_binding_table->cbuf_location);
	}

	if (m_shader_interpreter.is_interpreter(m_program))
	{
		m_program->bind_uniform(m_vertex_instructions_buffer_info, vk::glsl::binding_set_index_vertex, m_shader_interpreter.get_vertex_instruction_location());
		m_program->bind_uniform(m_fragment_instructions_buffer_info, vk::glsl::binding_set_index_fragment, m_shader_interpreter.get_fragment_instruction_location());
	}
	else if (m_fs_binding_table->cbuf_location != umax)
	{
		m_program->bind_uniform(m_fragment_constants_buffer_info, vk::glsl::binding_set_index_fragment, m_fs_binding_table->cbuf_location);
	}

	if (vk::emulate_conditional_rendering())
	{
		auto predicate = m_cond_render_buffer ? m_cond_render_buffer->value : vk::get_scratch_buffer(*m_current_command_buffer, 4)->value;
		m_program->bind_uniform({ predicate, 0, 4 }, vk::glsl::binding_set_index_vertex, m_vs_binding_table->cr_pred_buffer_location);
	}

	if (current_vertex_program.ctrl & RSX_SHADER_CONTROL_INSTANCED_CONSTANTS)
	{
		m_program->bind_uniform(m_instancing_indirection_buffer_info, vk::glsl::binding_set_index_vertex, m_vs_binding_table->instanced_lut_buffer_location);
		m_program->bind_uniform(m_instancing_constants_array_buffer_info, vk::glsl::binding_set_index_vertex, m_vs_binding_table->instanced_cbuf_location);
	}

	// Clear flags
	rsx::flags32_t handled_flags =
		rsx::pipeline_state::fragment_state_dirty |
		rsx::pipeline_state::vertex_state_dirty |
		rsx::pipeline_state::fragment_texture_state_dirty;

	if (!update_instancing_data)
	{
		handled_flags |= rsx::pipeline_state::transform_constants_dirty;
	}

	if (update_fragment_constants && !m_shader_interpreter.is_interpreter(m_program))
	{
		handled_flags |= rsx::pipeline_state::fragment_constants_dirty;
	}

	if (m_shader_interpreter.is_interpreter(m_program))
	{
		ensure(m_vertex_constants_buffer_info.range >= 468 * 16);
	}

	m_graphics_state.clear(handled_flags);
}

std::pair<const vs_binding_table_t*, const fs_binding_table_t*> VKGSRender::get_binding_table() const
{
	ensure(m_program);

	if (!m_shader_interpreter.is_interpreter(m_program))
	{
		return { &m_vertex_prog->binding_table, &m_fragment_prog->binding_table };
	}

	const auto& [vs, fs] = m_shader_interpreter.get_shaders();
	return { &vs->binding_table, &fs->binding_table };
}

bool VKGSRender::is_current_program_interpreted() const
{
	return m_program && m_shader_interpreter.is_interpreter(m_program);
}

void VKGSRender::upload_transform_constants(const rsx::io_buffer& buffer)
{
	const bool is_interpreter = m_shader_interpreter.is_interpreter(m_program);
	const usz transform_constants_size = (is_interpreter || m_vertex_prog->has_indexed_constants) ? 8192 : m_vertex_prog->constant_ids.size() * 16;

	if (transform_constants_size)
	{
		buffer.reserve(transform_constants_size);
		auto buf = buffer.data();

		const auto constant_ids = (transform_constants_size == 8192)
			? std::span<const u16>{}
			: std::span<const u16>(m_vertex_prog->constant_ids);
		m_draw_processor.fill_vertex_program_constants_data(buf, constant_ids);
	}
}

void VKGSRender::update_vertex_env(u32 id, const vk::vertex_upload_info& vertex_info)
{
	// Actual allocation must have been done previously
	u32 base_offset;
	const u32 offset32 = static_cast<u32>(m_vertex_layout_stream_info.offset);
	const u32 range32 = static_cast<u32>(m_vertex_layout_stream_info.range);

	if (!m_vertex_layout_storage || !m_vertex_layout_storage->in_range(offset32, range32, base_offset))
	{
		ensure(m_texbuffer_view_size >= m_vertex_layout_stream_info.range);
		vk::get_resource_manager()->dispose(m_vertex_layout_storage);

		const usz alloc_addr = m_vertex_layout_stream_info.offset;
		const usz view_size = (alloc_addr + m_texbuffer_view_size) > m_vertex_layout_ring_info.size() ? m_vertex_layout_ring_info.size() - alloc_addr : m_texbuffer_view_size;
		m_vertex_layout_storage = std::make_unique<vk::buffer_view>(*m_device, m_vertex_layout_ring_info.heap->value, VK_FORMAT_R32G32_UINT, alloc_addr, view_size);
		base_offset = 0;
	}

	const u32 vertex_layout_offset = (id * 16) + (base_offset / 8);
	const u32 constant_id_offset = static_cast<u32>(m_xform_constants_dynamic_offset) / 16u;

	u32 push_constants[6];
	u32 data_length = 20;

	push_constants[0] = vertex_info.vertex_index_base;
	push_constants[1] = vertex_info.vertex_index_offset;
	push_constants[2] = id;
	push_constants[3] = vertex_layout_offset;
	push_constants[4] = constant_id_offset;

	if (vk::emulate_conditional_rendering())
	{
		push_constants[5] = cond_render_ctrl.hw_cond_active ? 1 : 0;
		data_length += 4;
	}

	vkCmdPushConstants(
		*m_current_command_buffer,
		m_program->layout(),
		VK_SHADER_STAGE_VERTEX_BIT,
		0,
		data_length,
		push_constants);

	const usz data_offset = (id * 128) + m_vertex_layout_stream_info.offset;
	auto dst = m_vertex_layout_ring_info.map(data_offset, 128);

	m_draw_processor.fill_vertex_layout_state(
		m_vertex_layout,
		current_vp_metadata,
		vertex_info.first_vertex,
		vertex_info.allocated_vertex_count,
		static_cast<s32*>(dst),
		vertex_info.persistent_window_offset,
		vertex_info.volatile_window_offset);

	m_vertex_layout_ring_info.unmap();
}

void VKGSRender::patch_transform_constants(rsx::context* /*ctx*/, u32 index, u32 count)
{
	if (!m_program || !m_vertex_prog)
	{
		// Shouldn't be reachable, but handle it correctly anyway
		m_graphics_state |= rsx::pipeline_state::transform_constants_dirty;
		return;
	}

	if (!m_vertex_prog->overlaps_constants_range(index, count))
	{
		// Nothing meaningful to us
		return;
	}

	// Buffer updates mid-pass violate the spec and destroy performance on NVIDIA
	auto allocate_mem = [&](usz size) -> std::pair<void*, usz>
	{
		const usz alignment = m_device->gpu().get_limits().minStorageBufferOffsetAlignment;
		m_xform_constants_dynamic_offset = m_transform_constants_ring_info.alloc<1>(utils::align(size, alignment));
		return std::make_pair(m_transform_constants_ring_info.map(m_xform_constants_dynamic_offset, size), size);
	};

	rsx::io_buffer iobuf(allocate_mem);
	upload_transform_constants(iobuf);
}

void VKGSRender::init_buffers(rsx::framebuffer_creation_context context, bool)
{
	prepare_rtts(context);
}

void VKGSRender::close_and_submit_command_buffer(vk::fence* pFence, VkSemaphore wait_semaphore, VkSemaphore signal_semaphore, VkPipelineStageFlags pipeline_stage_flags)
{
	ensure(!m_queue_status.test_and_set(flush_queue_state::flushing));

	// Host MM sync before executing anything on the GPU
	rsx::mm_flush();

	// Workaround for deadlock occuring during RSX offloader fault
	// TODO: Restructure command submission infrastructure to avoid this condition
	const bool sync_success = g_fxo->get<rsx::dma_manager>().sync();
	const VkBool32 force_flush = !sync_success;

	if (vk::test_status_interrupt(vk::heap_dirty))
	{
		if (m_attrib_ring_info.is_dirty() ||
			m_fragment_env_ring_info.is_dirty() ||
			m_vertex_env_ring_info.is_dirty() ||
			m_fragment_texture_params_ring_info.is_dirty() ||
			m_vertex_layout_ring_info.is_dirty() ||
			m_fragment_constants_ring_info.is_dirty() ||
			m_index_buffer_ring_info.is_dirty() ||
			m_transform_constants_ring_info.is_dirty() ||
			m_texture_upload_buffer_ring_info.is_dirty() ||
			m_raster_env_ring_info.is_dirty() ||
			m_instancing_buffer_ring_info.is_dirty())
		{
			auto secondary_command_buffer = m_secondary_cb_list.next();
			secondary_command_buffer->begin();

			m_attrib_ring_info.sync(*secondary_command_buffer);
			m_fragment_env_ring_info.sync(*secondary_command_buffer);
			m_vertex_env_ring_info.sync(*secondary_command_buffer);
			m_fragment_texture_params_ring_info.sync(*secondary_command_buffer);
			m_vertex_layout_ring_info.sync(*secondary_command_buffer);
			m_fragment_constants_ring_info.sync(*secondary_command_buffer);
			m_index_buffer_ring_info.sync(*secondary_command_buffer);
			m_transform_constants_ring_info.sync(*secondary_command_buffer);
			m_texture_upload_buffer_ring_info.sync(*secondary_command_buffer);
			m_raster_env_ring_info.sync(*secondary_command_buffer);
			m_instancing_buffer_ring_info.sync(*secondary_command_buffer);

			secondary_command_buffer->end();

			vk::queue_submit_t submit_info{ m_device->get_graphics_queue(), nullptr };
			secondary_command_buffer->submit(submit_info, force_flush);
		}

		vk::clear_status_interrupt(vk::heap_dirty);
	}

#if 0 // Currently unreachable
	if (m_current_command_buffer->flags & vk::command_buffer::cb_has_conditional_render)
	{
		ensure(m_render_pass_open);
		_vkCmdEndConditionalRenderingEXT(*m_current_command_buffer);
	}
#endif

	// End any active renderpasses; the caller should handle reopening
	if (vk::is_renderpass_open(*m_current_command_buffer))
	{
		close_render_pass();
	}

	// End open queries. Flags will be automatically reset by the submit routine
	if (m_current_command_buffer->flags & vk::command_buffer::cb_has_open_query)
	{
		auto open_query = m_occlusion_map[m_active_query_info->driver_handle].indices.back();
		m_occlusion_query_manager->end_query(*m_current_command_buffer, open_query);
		m_current_command_buffer->flags &= ~vk::command_buffer::cb_has_open_query;
	}

	if (m_host_dma_ctrl && m_host_dma_ctrl->host_ctx()->needs_label_release())
	{
		vkCmdUpdateBuffer(*m_current_command_buffer,
			m_host_object_data->value,
			::offset32(&vk::host_data_t::commands_complete_event),
			sizeof(u64),
			const_cast<u64*>(&m_host_dma_ctrl->host_ctx()->last_label_acquire_event));

		m_host_dma_ctrl->host_ctx()->on_label_release();
	}

	m_current_command_buffer->end();
	m_current_command_buffer->tag();

	// Supporting concurrent access vastly simplifies this logic.
	// Instead of doing CB slice injection, we can just chain these together logically with the async stream going first
	vk::queue_submit_t primary_submit_info{ m_device->get_graphics_queue(), pFence };
	vk::queue_submit_t secondary_submit_info{};

	if (wait_semaphore)
	{
		primary_submit_info.wait_on(wait_semaphore, pipeline_stage_flags);
	}

	if (auto async_scheduler = g_fxo->try_get<vk::AsyncTaskScheduler>();
		async_scheduler && async_scheduler->is_recording())
	{
		if (async_scheduler->is_host_mode())
		{
			const VkSemaphore async_sema = *async_scheduler->get_sema();
			secondary_submit_info.queue_signal(async_sema);
			primary_submit_info.wait_on(async_sema, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

			// Delay object destruction by one cycle
			vk::get_resource_manager()->push_down_current_scope();
		}

		async_scheduler->flush(secondary_submit_info, force_flush);
	}

	if (signal_semaphore)
	{
		primary_submit_info.queue_signal(signal_semaphore);
	}

	m_current_command_buffer->submit(primary_submit_info, force_flush);

	m_queue_status.clear(flush_queue_state::flushing);
}

void VKGSRender::prepare_rtts(rsx::framebuffer_creation_context context)
{
	const bool clipped_scissor = (context == rsx::framebuffer_creation_context::context_draw);
	if (m_current_framebuffer_context == context && !m_graphics_state.test(rsx::rtt_config_dirty) && m_draw_fbo)
	{
		// Fast path
		// Framebuffer usage has not changed, framebuffer exists and config regs have not changed
		set_scissor(clipped_scissor);
		return;
	}

	m_graphics_state.clear(
		rsx::rtt_config_dirty |
		rsx::rtt_config_contested |
		rsx::rtt_config_valid |
		rsx::rtt_cache_state_dirty);

	get_framebuffer_layout(context, m_framebuffer_layout);
	if (!m_graphics_state.test(rsx::rtt_config_valid))
	{
		return;
	}

	if (m_draw_fbo && m_framebuffer_layout.ignore_change)
	{
		// Nothing has changed, we're still using the same framebuffer
		// Update flags to match current
		set_scissor(clipped_scissor);
		return;
	}

	m_rtts.prepare_render_target(*m_current_command_buffer,
		m_framebuffer_layout.color_format, m_framebuffer_layout.depth_format,
		m_framebuffer_layout.width, m_framebuffer_layout.height,
		m_framebuffer_layout.target, m_framebuffer_layout.aa_mode, m_framebuffer_layout.raster_type,
		m_framebuffer_layout.color_addresses, m_framebuffer_layout.zeta_address,
		m_framebuffer_layout.actual_color_pitch, m_framebuffer_layout.actual_zeta_pitch,
		(*m_device), *m_current_command_buffer);

	// Reset framebuffer information
	const auto color_bpp = get_format_block_size_in_bytes(m_framebuffer_layout.color_format);
	const auto samples = get_format_sample_count(m_framebuffer_layout.aa_mode);

	for (u8 i = 0; i < rsx::limits::color_buffers_count; ++i)
	{
		// Flush old address if we keep missing it
		if (m_surface_info[i].pitch && g_cfg.video.write_color_buffers)
		{
			const utils::address_range32 rsx_range = m_surface_info[i].get_memory_range();
			m_texture_cache.set_memory_read_flags(rsx_range, rsx::memory_read_flags::flush_once);
			m_texture_cache.flush_if_cache_miss_likely(*m_current_command_buffer, rsx_range);
		}

		m_surface_info[i].address = m_surface_info[i].pitch = 0;
		m_surface_info[i].width = m_framebuffer_layout.width;
		m_surface_info[i].height = m_framebuffer_layout.height;
		m_surface_info[i].color_format = m_framebuffer_layout.color_format;
		m_surface_info[i].bpp = color_bpp;
		m_surface_info[i].samples = samples;
	}

	//Process depth surface as well
	{
		if (m_depth_surface_info.pitch && g_cfg.video.write_depth_buffer)
		{
			const utils::address_range32 surface_range = m_depth_surface_info.get_memory_range();
			m_texture_cache.set_memory_read_flags(surface_range, rsx::memory_read_flags::flush_once);
			m_texture_cache.flush_if_cache_miss_likely(*m_current_command_buffer, surface_range);
		}

		m_depth_surface_info.address = m_depth_surface_info.pitch = 0;
		m_depth_surface_info.width = m_framebuffer_layout.width;
		m_depth_surface_info.height = m_framebuffer_layout.height;
		m_depth_surface_info.depth_format = m_framebuffer_layout.depth_format;
		m_depth_surface_info.bpp = get_format_block_size_in_bytes(m_framebuffer_layout.depth_format);
		m_depth_surface_info.samples = samples;
	}

	//Bind created rtts as current fbo...
	const auto draw_buffers = rsx::utility::get_rtt_indexes(m_framebuffer_layout.target);
	m_draw_buffers.clear();
	m_fbo_images.clear();

	for (u8 index : draw_buffers)
	{
		if (auto surface = std::get<1>(m_rtts.m_bound_render_targets[index]))
		{
			m_fbo_images.push_back(surface);

			m_surface_info[index].address = m_framebuffer_layout.color_addresses[index];
			m_surface_info[index].pitch = m_framebuffer_layout.actual_color_pitch[index];
			ensure(surface->rsx_pitch == m_framebuffer_layout.actual_color_pitch[index]);

			m_texture_cache.notify_surface_changed(m_surface_info[index].get_memory_range(m_framebuffer_layout.aa_factors));
			m_draw_buffers.push_back(index);
		}
	}

	if (std::get<0>(m_rtts.m_bound_depth_stencil) != 0)
	{
		auto ds = std::get<1>(m_rtts.m_bound_depth_stencil);
		m_fbo_images.push_back(ds);

		m_depth_surface_info.address = m_framebuffer_layout.zeta_address;
		m_depth_surface_info.pitch = m_framebuffer_layout.actual_zeta_pitch;
		ensure(ds->rsx_pitch == m_framebuffer_layout.actual_zeta_pitch);

		m_texture_cache.notify_surface_changed(m_depth_surface_info.get_memory_range(m_framebuffer_layout.aa_factors));
	}

	// Before messing with memory properties, flush command queue if there are dma transfers queued up
	if (m_current_command_buffer->flags & vk::command_buffer::cb_has_dma_transfer)
	{
		flush_command_queue();
	}

	if (!m_rtts.superseded_surfaces.empty())
	{
		for (auto& surface : m_rtts.superseded_surfaces)
		{
			m_texture_cache.discard_framebuffer_memory_region(*m_current_command_buffer, surface->get_memory_range());
		}

		m_rtts.superseded_surfaces.clear();
	}

	if (!m_rtts.orphaned_surfaces.empty())
	{
		u32 gcm_format;
		bool swap_bytes;

		for (auto& [base_addr, surface] : m_rtts.orphaned_surfaces)
		{
			bool lock = surface->is_depth_surface() ? !!g_cfg.video.write_depth_buffer :
				!!g_cfg.video.write_color_buffers;

			if (lock &&
#ifdef TEXTURE_CACHE_DEBUG
				!m_texture_cache.is_protected(
					base_addr,
					surface->get_memory_range(),
					rsx::texture_upload_context::framebuffer_storage)
#else
				!surface->is_locked()
#endif
				)
			{
				lock = false;
			}

			if (!lock) [[likely]]
			{
				m_texture_cache.commit_framebuffer_memory_region(*m_current_command_buffer, surface->get_memory_range());
				continue;
			}

			if (surface->is_depth_surface())
			{
				gcm_format = (surface->get_surface_depth_format() != rsx::surface_depth_format::z16) ? CELL_GCM_TEXTURE_DEPTH16 : CELL_GCM_TEXTURE_DEPTH24_D8;
				swap_bytes = true;
			}
			else
			{
				auto info = get_compatible_gcm_format(surface->get_surface_color_format());
				gcm_format = info.first;
				swap_bytes = info.second;
			}

			m_texture_cache.lock_memory_region(
				*m_current_command_buffer, surface, surface->get_memory_range(), false,
				surface->get_surface_width<rsx::surface_metrics::pixels>(), surface->get_surface_height<rsx::surface_metrics::pixels>(), surface->get_rsx_pitch(),
				gcm_format, swap_bytes);
		}

		m_rtts.orphaned_surfaces.clear();
	}

	const auto color_fmt_info = get_compatible_gcm_format(m_framebuffer_layout.color_format);
	for (u8 index : m_draw_buffers)
	{
		if (!m_surface_info[index].address || !m_surface_info[index].pitch) continue;

		const utils::address_range32 surface_range = m_surface_info[index].get_memory_range();
		if (g_cfg.video.write_color_buffers)
		{
			m_texture_cache.lock_memory_region(
				*m_current_command_buffer, m_rtts.m_bound_render_targets[index].second, surface_range, true,
				m_surface_info[index].width, m_surface_info[index].height, m_framebuffer_layout.actual_color_pitch[index],
				color_fmt_info.first, color_fmt_info.second);
		}
		else
		{
			m_texture_cache.commit_framebuffer_memory_region(*m_current_command_buffer, surface_range);
		}
	}

	if (m_depth_surface_info.address && m_depth_surface_info.pitch)
	{
		const utils::address_range32 surface_range = m_depth_surface_info.get_memory_range();
		if (g_cfg.video.write_depth_buffer)
		{
			const u32 gcm_format = (m_depth_surface_info.depth_format == rsx::surface_depth_format::z16) ? CELL_GCM_TEXTURE_DEPTH16 : CELL_GCM_TEXTURE_DEPTH24_D8;
			m_texture_cache.lock_memory_region(
				*m_current_command_buffer, m_rtts.m_bound_depth_stencil.second, surface_range, true,
				m_depth_surface_info.width, m_depth_surface_info.height, m_framebuffer_layout.actual_zeta_pitch, gcm_format, true);
		}
		else
		{
			m_texture_cache.commit_framebuffer_memory_region(*m_current_command_buffer, surface_range);
		}
	}

	m_current_renderpass_key = vk::get_renderpass_key(m_fbo_images);
	m_cached_renderpass = vk::get_renderpass(*m_device, m_current_renderpass_key);

	// Search old framebuffers for this same configuration
	const auto [fbo_width, fbo_height] = rsx::apply_resolution_scale<true>(m_framebuffer_layout.width, m_framebuffer_layout.height);

	if (m_draw_fbo)
	{
		// Release old ref
		m_draw_fbo->release();
	}

	m_draw_fbo = vk::get_framebuffer(*m_device, fbo_width, fbo_height, VK_FALSE, m_cached_renderpass, m_fbo_images);
	m_draw_fbo->add_ref();

	set_viewport();
	set_scissor(clipped_scissor);

	check_zcull_status(true);
}

void VKGSRender::renderctl(u32 request_code, void* args)
{
	switch (request_code)
	{
	case vk::rctrl_queue_submit:
	{
		const auto packet = reinterpret_cast<vk::queue_submit_t*>(args);
		vk::queue_submit(packet);
		free(packet);
		break;
	}
	case vk::rctrl_run_gc:
	{
		auto eid = reinterpret_cast<u64>(args);
		vk::on_event_completed(eid, true);
		break;
	}
	default:
		rsx::thread::renderctl(request_code, args);
	}
}

bool VKGSRender::scaled_image_from_memory(const rsx::blit_src_info& src, const rsx::blit_dst_info& dst, bool interpolate)
{
	if (swapchain_unavailable)
		return false;

	if (m_texture_cache.blit(src, dst, interpolate, m_rtts, *m_current_command_buffer))
	{
		m_samplers_dirty.store(true);
		m_current_command_buffer->set_flag(vk::command_buffer::cb_has_blit_transfer);

		if (m_current_command_buffer->flags & vk::command_buffer::cb_has_dma_transfer)
		{
			// A dma transfer has been queued onto this cb
			// This likely means that we're done with the tranfers to the target (writes_likely_completed=1)
			flush_command_queue();
		}
		return true;
	}

	return false;
}

void VKGSRender::begin_occlusion_query(rsx::reports::occlusion_query_info* query)
{
	ensure(!m_occlusion_query_active);

	query->result = 0;
	//query->sync_timestamp = get_system_time();
	m_active_query_info = query;
	m_occlusion_query_active = true;
	m_current_command_buffer->flags |= vk::command_buffer::cb_load_occluson_task;
}

void VKGSRender::end_occlusion_query(rsx::reports::occlusion_query_info* query)
{
	ensure(query == m_active_query_info);

	// NOTE: flushing the queue is very expensive, do not flush just because query stopped
	if (m_current_command_buffer->flags & vk::command_buffer::cb_has_open_query)
	{
		// VK_1_0 rules dictate that query must match subpass behavior on begin/end query.
		// This is slow, so only do this for drivers that care.
		if (vk::use_strict_query_scopes() &&
			vk::is_renderpass_open(*m_current_command_buffer))
		{
			vk::end_renderpass(*m_current_command_buffer);
		}

		// End query
		auto open_query = m_occlusion_map[m_active_query_info->driver_handle].indices.back();
		m_occlusion_query_manager->end_query(*m_current_command_buffer, open_query);
		m_current_command_buffer->flags &= ~vk::command_buffer::cb_has_open_query;
	}

	// Clear occlusion load flag
	m_current_command_buffer->flags &= ~vk::command_buffer::cb_load_occluson_task;

	m_occlusion_query_active = false;
	m_active_query_info = nullptr;
}

bool VKGSRender::check_occlusion_query_status(rsx::reports::occlusion_query_info* query)
{
	if (!query->num_draws)
		return true;

	auto &data = m_occlusion_map[query->driver_handle];
	if (data.indices.empty())
		return true;

	if (data.is_current(m_current_command_buffer))
		return false;

	const u32 oldest = data.indices.front();
	return m_occlusion_query_manager->check_query_status(oldest);
}

void VKGSRender::get_occlusion_query_result(rsx::reports::occlusion_query_info* query)
{
	auto &data = m_occlusion_map[query->driver_handle];
	if (data.indices.empty())
		return;

	if (query->num_draws)
	{
		if (data.is_current(m_current_command_buffer))
		{
			std::lock_guard lock(m_flush_queue_mutex);
			flush_command_queue();

			if (m_flush_requests.pending())
			{
				m_flush_requests.clear_pending_flag();
			}

			rsx_log.warning("[Performance warning] Unexpected ZCULL read caused a hard sync");
			busy_wait();
		}

		data.sync();

		// Gather data
		for (const auto occlusion_id : data.indices)
		{
			query->result += m_occlusion_query_manager->get_query_result(occlusion_id);
			if (query->result && !g_cfg.video.precise_zpass_count)
			{
				// We only need one hit unless precise zcull is requested
				break;
			}
		}
	}

	m_occlusion_query_manager->free_queries(*m_current_command_buffer, data.indices);
	data.indices.clear();
}

void VKGSRender::discard_occlusion_query(rsx::reports::occlusion_query_info* query)
{
	if (m_active_query_info == query)
	{
		end_occlusion_query(query);
	}

	auto &data = m_occlusion_map[query->driver_handle];
	if (data.indices.empty())
		return;

	m_occlusion_query_manager->free_queries(*m_current_command_buffer, data.indices);
	data.indices.clear();
}

void VKGSRender::emergency_query_cleanup(vk::command_buffer* commands)
{
	ensure(commands == static_cast<vk::command_buffer*>(m_current_command_buffer));

	if (m_current_command_buffer->flags & vk::command_buffer::cb_has_open_query)
	{
		auto open_query = m_occlusion_map[m_active_query_info->driver_handle].indices.back();
		m_occlusion_query_manager->end_query(*m_current_command_buffer, open_query);
		m_current_command_buffer->flags &= ~vk::command_buffer::cb_has_open_query;
	}
}

void VKGSRender::begin_conditional_rendering(const std::vector<rsx::reports::occlusion_query_info*>& sources)
{
	ensure(!sources.empty());

	// Flag check whether to calculate all entries or only one
	bool partial_eval;

	// Try and avoid regenerating the data if its a repeat/spam
	// NOTE: The incoming list is reversed with the first entry being the newest
	if (m_cond_render_sync_tag == sources.front()->sync_tag)
	{
		// Already synched, check subdraw which is possible if last sync happened while query was active
		if (!m_active_query_info || m_active_query_info != sources.front())
		{
			rsx::thread::begin_conditional_rendering(sources);
			return;
		}

		// Partial evaluation only
		partial_eval = true;
	}
	else
	{
		m_cond_render_sync_tag = sources.front()->sync_tag;
		partial_eval = false;
	}

	// Time to aggregate
	if (!m_cond_render_buffer)
	{
		auto& memory_props = m_device->get_memory_mapping();
		auto usage_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		if (m_device->get_conditional_render_support())
		{
			usage_flags |= VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT;
		}

		m_cond_render_buffer = std::make_unique<vk::buffer>(
			*m_device, 4,
			memory_props.device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			usage_flags, 0, VMM_ALLOCATION_POOL_UNDEFINED);
	}

	VkPipelineStageFlags dst_stage;
	VkAccessFlags dst_access;
	u32 dst_offset = 0;
	u32 num_hw_queries = 0;
	usz first = 0;
	usz last = (!partial_eval) ? sources.size() : 1;

	// Count number of queries available. This is an "opening" evaluation, if there is only one source, read it as-is.
	// The idea is to avoid scheduling a compute task unless we have to.
	for (usz i = first; i < last; ++i)
	{
		auto& query_info = m_occlusion_map[sources[i]->driver_handle];
		num_hw_queries += ::size32(query_info.indices);
	}

	if (m_device->get_conditional_render_support())
	{
		dst_stage = VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT;
		dst_access = VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT;
	}
	else
	{
		dst_stage = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
		dst_access = VK_ACCESS_SHADER_READ_BIT;
	}

	if (num_hw_queries == 1 && !partial_eval) [[ likely ]]
	{
		// Accept the first available query handle as the source of truth. No aggregation is required.
		for (usz i = first; i < last; ++i)
		{
			auto& query_info = m_occlusion_map[sources[i]->driver_handle];
			if (!query_info.indices.empty())
			{
				const auto& index = query_info.indices.front();
				m_occlusion_query_manager->get_query_result_indirect(*m_current_command_buffer, index, 1, m_cond_render_buffer->value, 0);

				vk::insert_buffer_memory_barrier(*m_current_command_buffer, m_cond_render_buffer->value, 0, 4,
					VK_PIPELINE_STAGE_TRANSFER_BIT, dst_stage,
					VK_ACCESS_TRANSFER_WRITE_BIT, dst_access);

				rsx::thread::begin_conditional_rendering(sources);
				return;
			}
		}

		// This is unreachable unless something went horribly wrong
		fmt::throw_exception("Unreachable");
	}
	else if (num_hw_queries > 0)
	{
		// We'll need to do some result aggregation using a compute shader.
		auto scratch = vk::get_scratch_buffer(*m_current_command_buffer, num_hw_queries * 4);

		// Range latching. Because of how the query pool manages allocations using a stack, we get an inverse sequential set of handles/indices that we can easily group together.
		// This drastically boosts performance on some drivers like the NVIDIA proprietary one that seems to have a rather high cost for every individual query transer command.
		struct { u32 first, last; } query_range = { umax, 0 };

		auto copy_query_range_impl = [&]()
		{
			const auto count = (query_range.last - query_range.first + 1);
			m_occlusion_query_manager->get_query_result_indirect(*m_current_command_buffer, query_range.first, count, scratch->value, dst_offset);
			dst_offset += count * 4;
		};

		for (usz i = first; i < last; ++i)
		{
			auto& query_info = m_occlusion_map[sources[i]->driver_handle];
			for (const auto& index : query_info.indices)
			{
				// First iteration?
				if (query_range.first == umax)
				{
					query_range = { index, index };
					continue;
				}

				// Head?
				if ((query_range.first - 1) == index)
				{
					query_range.first = index;
					continue;
				}

				// Tail?
				if ((query_range.last + 1) == index)
				{
					query_range.last = index;
					continue;
				}

				// Flush pending queue. In practice, this is never reached and we fall out to the spill block outside the loops
				copy_query_range_impl();

				// Start a new range for the current index
				query_range = { index, index };
			}
		}

		if (query_range.first != umax)
		{
			// Dangling queries, flush
			copy_query_range_impl();
		}

		// Sanity check
		ensure(dst_offset <= scratch->size());

		if (!partial_eval)
		{
			// Fast path should have been caught above
			ensure(dst_offset > 4);

			// Clear result to zero
			vkCmdFillBuffer(*m_current_command_buffer, m_cond_render_buffer->value, 0, 4, 0);

			vk::insert_buffer_memory_barrier(*m_current_command_buffer, m_cond_render_buffer->value, 0, 4,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT);
		}

		vk::insert_buffer_memory_barrier(*m_current_command_buffer, scratch->value, 0, dst_offset,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

		vk::get_compute_task<vk::cs_aggregator>()->run(*m_current_command_buffer, m_cond_render_buffer.get(), scratch, dst_offset / 4);

		vk::insert_buffer_memory_barrier(*m_current_command_buffer, m_cond_render_buffer->value, 0, 4,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, dst_stage,
			VK_ACCESS_SHADER_WRITE_BIT, dst_access);
	}
	else if (m_program)
	{
		// This can sometimes happen when shaders are compiling, only log if there is a program hit
		rsx_log.warning("Dubious query data pushed to cond render! Please report to developers(q.pending=%d)", sources.front()->pending);
	}

	rsx::thread::begin_conditional_rendering(sources);
}

void VKGSRender::end_conditional_rendering()
{
	thread::end_conditional_rendering();
}
