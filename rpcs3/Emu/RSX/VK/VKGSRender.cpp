﻿#include "stdafx.h"
#include "Emu/Memory/vm.h"
#include "Emu/System.h"
#include "VKGSRender.h"
#include "../rsx_methods.h"
#include "../rsx_utils.h"
#include "../Common/BufferUtils.h"
#include "VKFormats.h"
#include "VKCommonDecompiler.h"
#include "VKRenderPass.h"

namespace
{
	u32 get_max_depth_value(rsx::surface_depth_format format)
	{
		switch (format)
		{
		case rsx::surface_depth_format::z16: return 0xFFFF;
		case rsx::surface_depth_format::z24s8: return 0xFFFFFF;
		}
		fmt::throw_exception("Unknown depth format" HERE);
	}

	u8 get_pixel_size(rsx::surface_depth_format format)
	{
		switch (format)
		{
		case rsx::surface_depth_format::z16: return 2;
		case rsx::surface_depth_format::z24s8: return 4;
		}
		fmt::throw_exception("Unknown depth format" HERE);
	}
}

namespace vk
{
	VkCompareOp get_compare_func(rsx::comparison_function op, bool reverse_direction = false)
	{
		switch (op)
		{
		case rsx::comparison_function::never: return VK_COMPARE_OP_NEVER;
		case rsx::comparison_function::greater: return reverse_direction ? VK_COMPARE_OP_LESS: VK_COMPARE_OP_GREATER;
		case rsx::comparison_function::less: return reverse_direction ? VK_COMPARE_OP_GREATER: VK_COMPARE_OP_LESS;
		case rsx::comparison_function::less_or_equal: return reverse_direction ? VK_COMPARE_OP_GREATER_OR_EQUAL: VK_COMPARE_OP_LESS_OR_EQUAL;
		case rsx::comparison_function::greater_or_equal: return reverse_direction ? VK_COMPARE_OP_LESS_OR_EQUAL: VK_COMPARE_OP_GREATER_OR_EQUAL;
		case rsx::comparison_function::equal: return VK_COMPARE_OP_EQUAL;
		case rsx::comparison_function::not_equal: return VK_COMPARE_OP_NOT_EQUAL;
		case rsx::comparison_function::always: return VK_COMPARE_OP_ALWAYS;
		default:
			fmt::throw_exception("Unknown compare op: 0x%x" HERE, (u32)op);
		}
	}

	std::pair<VkFormat, VkComponentMapping> get_compatible_surface_format(rsx::surface_color_format color_format)
	{
		const VkComponentMapping abgr = { VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_A };
		const VkComponentMapping o_rgb = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_ONE };
		const VkComponentMapping z_rgb = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_ZERO };
		const VkComponentMapping o_bgr = { VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_ONE };
		const VkComponentMapping z_bgr = { VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_ZERO };

		switch (color_format)
		{
		case rsx::surface_color_format::r5g6b5:
			return std::make_pair(VK_FORMAT_R5G6B5_UNORM_PACK16, vk::default_component_map());

		case rsx::surface_color_format::a8r8g8b8:
			return std::make_pair(VK_FORMAT_B8G8R8A8_UNORM, vk::default_component_map());

		case rsx::surface_color_format::a8b8g8r8:
			return std::make_pair(VK_FORMAT_B8G8R8A8_UNORM, abgr);

		case rsx::surface_color_format::x8b8g8r8_o8b8g8r8:
			return std::make_pair(VK_FORMAT_B8G8R8A8_UNORM, o_bgr);

		case rsx::surface_color_format::x8b8g8r8_z8b8g8r8:
			return std::make_pair(VK_FORMAT_B8G8R8A8_UNORM, z_bgr);

		case rsx::surface_color_format::x8r8g8b8_z8r8g8b8:
			return std::make_pair(VK_FORMAT_B8G8R8A8_UNORM, z_rgb);

		case rsx::surface_color_format::x8r8g8b8_o8r8g8b8:
			return std::make_pair(VK_FORMAT_B8G8R8A8_UNORM, o_rgb);

		case rsx::surface_color_format::w16z16y16x16:
			return std::make_pair(VK_FORMAT_R16G16B16A16_SFLOAT, vk::default_component_map());

		case rsx::surface_color_format::w32z32y32x32:
			return std::make_pair(VK_FORMAT_R32G32B32A32_SFLOAT, vk::default_component_map());

		case rsx::surface_color_format::x1r5g5b5_o1r5g5b5:
			return std::make_pair(VK_FORMAT_B8G8R8A8_UNORM, o_rgb);

		case rsx::surface_color_format::x1r5g5b5_z1r5g5b5:
			return std::make_pair(VK_FORMAT_B8G8R8A8_UNORM, z_rgb);

		case rsx::surface_color_format::b8:
		{
			VkComponentMapping no_alpha = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_ONE };
			return std::make_pair(VK_FORMAT_R8_UNORM, no_alpha);
		}

		case rsx::surface_color_format::g8b8:
		{
			VkComponentMapping gb_rg = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G };
			return std::make_pair(VK_FORMAT_R8G8_UNORM, gb_rg);
		}

		case rsx::surface_color_format::x32:
			return std::make_pair(VK_FORMAT_R32_SFLOAT, vk::default_component_map());

		default:
			LOG_ERROR(RSX, "Surface color buffer: Unsupported surface color format (0x%x)", (u32)color_format);
			return std::make_pair(VK_FORMAT_B8G8R8A8_UNORM, vk::default_component_map());
		}
	}

	std::pair<u32, bool> get_compatible_gcm_format(rsx::surface_color_format color_format)
	{
		switch (color_format)
		{
		case rsx::surface_color_format::r5g6b5:
			return{ CELL_GCM_TEXTURE_R5G6B5, false };

		case rsx::surface_color_format::a8r8g8b8:
			return{ CELL_GCM_TEXTURE_A8R8G8B8, true }; //verified

		case rsx::surface_color_format::a8b8g8r8:
			return{ CELL_GCM_TEXTURE_A8R8G8B8, false };

		case rsx::surface_color_format::x8b8g8r8_o8b8g8r8:
		case rsx::surface_color_format::x8b8g8r8_z8b8g8r8:
			return{ CELL_GCM_TEXTURE_A8R8G8B8, true };

		case rsx::surface_color_format::x8r8g8b8_z8r8g8b8:
		case rsx::surface_color_format::x8r8g8b8_o8r8g8b8:
			return{ CELL_GCM_TEXTURE_A8R8G8B8, false };

		case rsx::surface_color_format::w16z16y16x16:
			return{ CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT, true };

		case rsx::surface_color_format::w32z32y32x32:
			return{ CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT, true };

		case rsx::surface_color_format::x1r5g5b5_o1r5g5b5:
		case rsx::surface_color_format::x1r5g5b5_z1r5g5b5:
			return{ CELL_GCM_TEXTURE_A1R5G5B5, false };

		case rsx::surface_color_format::b8:
			return{ CELL_GCM_TEXTURE_B8, false };

		case rsx::surface_color_format::g8b8:
			return{ CELL_GCM_TEXTURE_G8B8, true };

		case rsx::surface_color_format::x32:
			return{ CELL_GCM_TEXTURE_X32_FLOAT, true }; //verified

		default:
			return{ CELL_GCM_TEXTURE_A8R8G8B8, false };
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
			fmt::throw_exception("Unknown logic op 0x%x" HERE, (u32)op);
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
			fmt::throw_exception("Unknown blend factor 0x%x" HERE, (u32)factor);
		}
	}

	VkBlendOp get_blend_op(rsx::blend_equation op)
	{
		switch (op)
		{
		case rsx::blend_equation::add_signed:
			LOG_TRACE(RSX, "blend equation add_signed used. Emulating using FUNC_ADD");
		case rsx::blend_equation::add:
			return VK_BLEND_OP_ADD;
		case rsx::blend_equation::substract: return VK_BLEND_OP_SUBTRACT;
		case rsx::blend_equation::reverse_substract_signed:
			LOG_TRACE(RSX, "blend equation reverse_subtract_signed used. Emulating using FUNC_REVERSE_SUBTRACT");
		case rsx::blend_equation::reverse_substract: return VK_BLEND_OP_REVERSE_SUBTRACT;
		case rsx::blend_equation::min: return VK_BLEND_OP_MIN;
		case rsx::blend_equation::max: return VK_BLEND_OP_MAX;
		default:
			fmt::throw_exception("Unknown blend op: 0x%x" HERE, (u32)op);
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
			fmt::throw_exception("Unknown stencil op: 0x%x" HERE, (u32)op);
		}
	}

	VkFrontFace get_front_face(rsx::front_face ffv)
	{
		switch (ffv)
		{
		case rsx::front_face::cw: return VK_FRONT_FACE_CLOCKWISE;
		case rsx::front_face::ccw: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
		default:
			fmt::throw_exception("Unknown front face value: 0x%x" HERE, (u32)ffv);
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
			fmt::throw_exception("Unknown cull face value: 0x%x" HERE, (u32)cfv);
		}
	}
}

namespace
{
	std::tuple<VkPipelineLayout, VkDescriptorSetLayout> get_shared_pipeline_layout(VkDevice dev)
	{
		std::array<VkDescriptorSetLayoutBinding, VK_NUM_DESCRIPTOR_BINDINGS> bindings = {};

		size_t idx = 0;

		// Vertex stream, one stream for cacheable data, one stream for transient data
		for (int i = 0; i < 2; i++)
		{
			bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
			bindings[idx].descriptorCount = 1;
			bindings[idx].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			bindings[idx].binding = VERTEX_BUFFERS_FIRST_BIND_SLOT + i;
			idx++;
		}

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[idx].descriptorCount = 1;
		bindings[idx].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[idx].binding = FRAGMENT_CONSTANT_BUFFERS_BIND_SLOT;

		idx++;

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[idx].descriptorCount = 1;
		bindings[idx].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[idx].binding = FRAGMENT_STATE_BIND_SLOT;

		idx++;

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[idx].descriptorCount = 1;
		bindings[idx].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[idx].binding = FRAGMENT_TEXTURE_PARAMS_BIND_SLOT;

		idx++;

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[idx].descriptorCount = 1;
		bindings[idx].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		bindings[idx].binding = VERTEX_CONSTANT_BUFFERS_BIND_SLOT;

		idx++;

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[idx].descriptorCount = 1;
		bindings[idx].stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
		bindings[idx].binding = VERTEX_LAYOUT_BIND_SLOT;

		idx++;

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[idx].descriptorCount = 1;
		bindings[idx].stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
		bindings[idx].binding = VERTEX_PARAMS_BIND_SLOT;

		idx++;

		for (int i = 0; i < rsx::limits::fragment_textures_count; i++)
		{
			bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			bindings[idx].descriptorCount = 1;
			bindings[idx].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			bindings[idx].binding = TEXTURES_FIRST_BIND_SLOT + i;
			idx++;
		}

		for (int i = 0; i < rsx::limits::vertex_textures_count; i++)
		{
			bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			bindings[idx].descriptorCount = 1;
			bindings[idx].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			bindings[idx].binding = VERTEX_TEXTURES_FIRST_BIND_SLOT + i;
			idx++;
		}

		verify(HERE), idx == VK_NUM_DESCRIPTOR_BINDINGS;

		VkDescriptorSetLayoutCreateInfo infos = {};
		infos.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		infos.pBindings = bindings.data();
		infos.bindingCount = static_cast<uint32_t>(bindings.size());

		VkDescriptorSetLayout set_layout;
		CHECK_RESULT(vkCreateDescriptorSetLayout(dev, &infos, nullptr, &set_layout));

		VkPipelineLayoutCreateInfo layout_info = {};
		layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layout_info.setLayoutCount = 1;
		layout_info.pSetLayouts = &set_layout;

		VkPipelineLayout result;
		CHECK_RESULT(vkCreatePipelineLayout(dev, &layout_info, nullptr, &result));
		return std::make_tuple(result, set_layout);
	}
}

u64 VKGSRender::get_cycles()
{
	return thread_ctrl::get_cycles(static_cast<named_thread<VKGSRender>&>(*this));
}

VKGSRender::VKGSRender() : GSRender()
{
	u32 instance_handle = m_thread_context.createInstance("RPCS3");

	if (instance_handle > 0)
	{
		m_thread_context.makeCurrentInstance(instance_handle);
		m_thread_context.enable_debugging();
	}
	else
	{
		LOG_FATAL(RSX, "Could not find a vulkan compatible GPU driver. Your GPU(s) may not support Vulkan, or you need to install the vulkan runtime and drivers");
		m_device = VK_NULL_HANDLE;
		return;
	}

	std::vector<vk::physical_device>& gpus = m_thread_context.enumerateDevices();

	//Actually confirm  that the loader found at least one compatible device
	//This should not happen unless something is wrong with the driver setup on the target system
	if (gpus.empty())
	{
		//We can't throw in Emulator::Load, so we show error and return
		LOG_FATAL(RSX, "No compatible GPU devices found");
		m_device = VK_NULL_HANDLE;
		return;
	}

	bool gpu_found = false;
	std::string adapter_name = g_cfg.video.vk.adapter;

	display_handle_t display = m_frame->handle();

#if !defined(_WIN32) && !defined(__APPLE__)
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
			m_swapchain.reset(m_thread_context.createSwapChain(display, gpu));
			gpu_found = true;
			break;
		}
	}

	if (!gpu_found || adapter_name.empty())
	{
		m_swapchain.reset(m_thread_context.createSwapChain(display, gpus[0]));
	}

	if (!m_swapchain)
	{
		m_device = VK_NULL_HANDLE;
		LOG_FATAL(RSX, "Could not successfully initialize a swapchain");
		return;
	}

	m_device = (vk::render_device*)(&m_swapchain->get_device());

	vk::set_current_thread_ctx(m_thread_context);
	vk::set_current_renderer(m_swapchain->get_device());

	m_client_width = m_frame->client_width();
	m_client_height = m_frame->client_height();
	if (!m_swapchain->init(m_client_width, m_client_height))
		present_surface_dirty_flag = true;

	//create command buffer...
	m_command_buffer_pool.create((*m_device));

	for (auto &cb : m_primary_cb_list)
	{
		cb.create(m_command_buffer_pool);
		cb.init_fence(*m_device);
	}

	m_current_command_buffer = &m_primary_cb_list[0];

	//Create secondary command_buffer for parallel operations
	m_secondary_command_buffer_pool.create((*m_device));
	m_secondary_command_buffer.create(m_secondary_command_buffer_pool, true);
	m_secondary_command_buffer.access_hint = vk::command_buffer::access_type_hint::all;

	//Precalculated stuff
	std::tie(pipeline_layout, descriptor_layouts) = get_shared_pipeline_layout(*m_device);

	//Occlusion
	m_occlusion_query_pool.create((*m_device), OCCLUSION_MAX_POOL_SIZE);
	for (int n = 0; n < 128; ++n)
		m_occlusion_query_data[n].driver_handle = n;

	//Generate frame contexts
	VkDescriptorPoolSize uniform_buffer_pool = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 6 * DESCRIPTOR_MAX_DRAW_CALLS };
	VkDescriptorPoolSize uniform_texel_pool = { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER , 2 * DESCRIPTOR_MAX_DRAW_CALLS };
	VkDescriptorPoolSize texture_pool = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 20 * DESCRIPTOR_MAX_DRAW_CALLS };

	std::vector<VkDescriptorPoolSize> sizes{ uniform_buffer_pool, uniform_texel_pool, texture_pool };

	VkSemaphoreCreateInfo semaphore_info = {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	//VRAM allocation
	m_attrib_ring_info.create(VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, VK_ATTRIB_RING_BUFFER_SIZE_M * 0x100000, "attrib buffer", 0x400000);
	m_fragment_env_ring_info.create(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_UBO_RING_BUFFER_SIZE_M * 0x100000, "fragment env buffer");
	m_vertex_env_ring_info.create(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_UBO_RING_BUFFER_SIZE_M * 0x100000, "vertex env buffer");
	m_fragment_texture_params_ring_info.create(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_UBO_RING_BUFFER_SIZE_M * 0x100000, "fragment texture params buffer");
	m_vertex_layout_ring_info.create(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_UBO_RING_BUFFER_SIZE_M * 0x100000, "vertex layout buffer");
	m_fragment_constants_ring_info.create(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_UBO_RING_BUFFER_SIZE_M * 0x100000, "fragment constants buffer");
	m_transform_constants_ring_info.create(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_TRANSFORM_CONSTANTS_BUFFER_SIZE_M * 0x100000, "transform constants buffer");
	m_index_buffer_ring_info.create(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_INDEX_RING_BUFFER_SIZE_M * 0x100000, "index buffer");
	m_texture_upload_buffer_ring_info.create(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_TEXTURE_UPLOAD_RING_BUFFER_SIZE_M * 0x100000, "texture upload buffer", 32 * 0x100000);

	const auto limits = m_device->gpu().get_limits();
	m_texbuffer_view_size = std::min(limits.maxTexelBufferElements, VK_ATTRIB_RING_BUFFER_SIZE_M * 0x100000u);

	if (m_texbuffer_view_size < 0x800000)
	{
		// Warn, only possibly expected on macOS
		LOG_WARNING(RSX, "Current driver may crash due to memory limitations (%uk)", m_texbuffer_view_size / 1024);
	}

	for (auto &ctx : frame_context_storage)
	{
		vkCreateSemaphore((*m_device), &semaphore_info, nullptr, &ctx.present_semaphore);
		ctx.descriptor_pool.create(*m_device, sizes.data(), static_cast<uint32_t>(sizes.size()), DESCRIPTOR_MAX_DRAW_CALLS, 1);
	}

	const auto& memory_map = m_device->get_memory_mapping();
	null_buffer = std::make_unique<vk::buffer>(*m_device, 32, memory_map.device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, 0);
	null_buffer_view = std::make_unique<vk::buffer_view>(*m_device, null_buffer->value, VK_FORMAT_R8_UINT, 0, 32);

	vk::initialize_compiler_context();

	if (g_cfg.video.overlay)
	{
		auto key = vk::get_renderpass_key(m_swapchain->get_surface_format());
		m_text_writer.reset(new vk::text_writer());
		m_text_writer->init(*m_device, vk::get_renderpass(*m_device, key));
	}

	m_depth_converter.reset(new vk::depth_convert_pass());
	m_depth_converter->create(*m_device);

	m_attachment_clear_pass.reset(new vk::attachment_clear_pass());
	m_attachment_clear_pass->create(*m_device);

	m_prog_buffer.reset(new VKProgramBuffer());

	if (g_cfg.video.disable_vertex_cache)
		m_vertex_cache.reset(new vk::null_vertex_cache());
	else
		m_vertex_cache.reset(new vk::weak_vertex_cache());

	m_shaders_cache.reset(new vk::shader_cache(*m_prog_buffer.get(), "vulkan", "v1.7"));

	open_command_buffer();

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

	m_texture_cache.initialize((*m_device), m_swapchain->get_graphics_queue(),
			m_texture_upload_buffer_ring_info);

	m_ui_renderer.reset(new vk::ui_overlay_renderer());
	m_ui_renderer->create(*m_current_command_buffer, m_texture_upload_buffer_ring_info);

	supports_multidraw = true;
	supports_native_ui = (bool)g_cfg.misc.use_native_interface;
}

VKGSRender::~VKGSRender()
{
	if (m_device == VK_NULL_HANDLE)
	{
		//Initialization failed
		return;
	}

	//Wait for device to finish up with resources
	vkDeviceWaitIdle(*m_device);

	//Texture cache
	m_texture_cache.destroy();

	//Shaders
	vk::finalize_compiler_context();
	m_prog_buffer->clear();

	m_persistent_attribute_storage.reset();
	m_volatile_attribute_storage.reset();

	//Global resources
	vk::destroy_global_resources();

	//Heaps
	m_attrib_ring_info.destroy();
	m_fragment_env_ring_info.destroy();
	m_vertex_env_ring_info.destroy();
	m_fragment_texture_params_ring_info.destroy();
	m_vertex_layout_ring_info.destroy();
	m_fragment_constants_ring_info.destroy();
	m_transform_constants_ring_info.destroy();
	m_index_buffer_ring_info.destroy();
	m_texture_upload_buffer_ring_info.destroy();

	//Fallback bindables
	null_buffer.reset();
	null_buffer_view.reset();

	if (m_current_frame == &m_aux_frame_context)
	{
		//Return resources back to the owner
		m_current_frame = &frame_context_storage[m_current_queue_index];
		m_current_frame->swap_storage(m_aux_frame_context);
		m_current_frame->grab_resources(m_aux_frame_context);
	}

	m_aux_frame_context.buffer_views_to_clean.clear();

	//NOTE: aux_context uses descriptor pools borrowed from the main queues and any allocations will be automatically freed when pool is destroyed
	for (auto &ctx : frame_context_storage)
	{
		vkDestroySemaphore((*m_device), ctx.present_semaphore, nullptr);
		ctx.descriptor_pool.destroy();

		ctx.buffer_views_to_clean.clear();
	}

	//Textures
	m_rtts.destroy();
	m_texture_cache.destroy();

	m_resource_manager.destroy();
	m_stencil_mirror_sampler.reset();

	//Overlay text handler
	m_text_writer.reset();

	//Overlay UI renderer
	m_ui_renderer->destroy();
	m_ui_renderer.reset();

	//RGBA->depth cast helper
	m_depth_converter->destroy();
	m_depth_converter.reset();

	//Attachment clear helper
	m_attachment_clear_pass->destroy();
	m_attachment_clear_pass.reset();

	//Pipeline descriptors
	vkDestroyPipelineLayout(*m_device, pipeline_layout, nullptr);
	vkDestroyDescriptorSetLayout(*m_device, descriptor_layouts, nullptr);

	//Queries
	m_occlusion_query_pool.destroy();

	//Command buffer
	for (auto &cb : m_primary_cb_list)
		cb.destroy();

	m_command_buffer_pool.destroy();

	m_secondary_command_buffer.destroy();
	m_secondary_command_buffer_pool.destroy();

	//Device handles/contexts
	m_swapchain->destroy();
	m_thread_context.close();

#if !defined(_WIN32) && !defined(__APPLE__) && defined(HAVE_VULKAN)
	if (m_display_handle)
		XCloseDisplay(m_display_handle);
#endif
}

bool VKGSRender::on_access_violation(u32 address, bool is_writing)
{
	vk::texture_cache::thrashed_set result;
	{
		std::lock_guard lock(m_secondary_cb_guard);

		const rsx::invalidation_cause cause = is_writing ? rsx::invalidation_cause::deferred_write : rsx::invalidation_cause::deferred_read;
		result = std::move(m_texture_cache.invalidate_address(m_secondary_command_buffer, address, cause));
	}

	if (!result.violation_handled)
		return false;

	{
		std::lock_guard lock(m_sampler_mutex);
		m_samplers_dirty.store(true);
	}

	if (result.num_flushable > 0)
	{
		const bool is_rsxthr = std::this_thread::get_id() == m_rsx_thread;
		bool has_queue_ref = false;

		if (!is_rsxthr)
		{
			//Always submit primary cb to ensure state consistency (flush pending changes such as image transitions)
			vm::temporary_unlock();

			std::lock_guard lock(m_flush_queue_mutex);

			m_flush_requests.post(false);
			has_queue_ref = true;
		}
		else if (!vk::is_uninterruptible())
		{
			//Flush primary cb queue to sync pending changes (e.g image transitions!)
			flush_command_queue();
		}
		else
		{
			//LOG_ERROR(RSX, "Fault in uninterruptible code!");
		}

		if (has_queue_ref)
		{
			//Wait for the RSX thread to process request if it hasn't already
			m_flush_requests.producer_wait();
		}

		m_texture_cache.flush_all(m_secondary_command_buffer, result);

		if (has_queue_ref)
		{
			//Release RSX thread
			m_flush_requests.remove_one();
		}
	}

	return true;
}

void VKGSRender::on_invalidate_memory_range(const utils::address_range &range)
{
	std::lock_guard lock(m_secondary_cb_guard);

	auto data = std::move(m_texture_cache.invalidate_range(m_secondary_command_buffer, range, rsx::invalidation_cause::unmap));
	AUDIT(data.empty());

	if (data.violation_handled)
	{
		m_texture_cache.purge_unreleased_sections();
		{
			std::lock_guard lock(m_sampler_mutex);
			m_samplers_dirty.store(true);
		}
	}
}

void VKGSRender::notify_tile_unbound(u32 tile)
{
	//TODO: Handle texture writeback
	//u32 addr = rsx::get_address(tiles[tile].offset, tiles[tile].location);
	//on_notify_memory_unmapped(addr, tiles[tile].size);
	//m_rtts.invalidate_surface_address(addr, false);

	{
		std::lock_guard lock(m_sampler_mutex);
		m_samplers_dirty.store(true);
	}
}

void VKGSRender::check_heap_status(u32 flags)
{
	bool heap_critical;
	if (flags == VK_HEAP_CHECK_ALL)
	{
		heap_critical = m_attrib_ring_info.is_critical() ||
			m_texture_upload_buffer_ring_info.is_critical() ||
			m_fragment_env_ring_info.is_critical() ||
			m_vertex_env_ring_info.is_critical() ||
			m_fragment_texture_params_ring_info.is_critical() ||
			m_vertex_layout_ring_info.is_critical() ||
			m_fragment_constants_ring_info.is_critical() ||
			m_transform_constants_ring_info.is_critical() ||
			m_index_buffer_ring_info.is_critical();
	}
	else if (flags)
	{
		heap_critical = false;
		u32 test = 1 << utils::cnttz32(flags, true);

		do
		{
			switch (flags & test)
			{
			case 0:
				break;
			case VK_HEAP_CHECK_TEXTURE_UPLOAD_STORAGE:
				heap_critical = m_texture_upload_buffer_ring_info.is_critical();
				break;
			case VK_HEAP_CHECK_VERTEX_STORAGE:
				heap_critical = m_attrib_ring_info.is_critical() || m_index_buffer_ring_info.is_critical();
				break;
			case VK_HEAP_CHECK_VERTEX_ENV_STORAGE:
				heap_critical = m_vertex_env_ring_info.is_critical();
				break;
			case VK_HEAP_CHECK_FRAGMENT_ENV_STORAGE:
				heap_critical = m_fragment_env_ring_info.is_critical();
				break;
			case VK_HEAP_CHECK_TEXTURE_ENV_STORAGE:
				heap_critical = m_fragment_texture_params_ring_info.is_critical();
				break;
			case VK_HEAP_CHECK_VERTEX_LAYOUT_STORAGE:
				heap_critical = m_vertex_layout_ring_info.is_critical();
				break;
			case VK_HEAP_CHECK_TRANSFORM_CONSTANTS_STORAGE:
				heap_critical = m_transform_constants_ring_info.is_critical();
				break;
			case VK_HEAP_CHECK_FRAGMENT_CONSTANTS_STORAGE:
				heap_critical = m_fragment_constants_ring_info.is_critical();
				break;
			default:
				fmt::throw_exception("Unexpected heap flag set! (0x%X)", test);
			}

			flags &= ~test;
			test <<= 1;
		}
		while (flags && !heap_critical);
	}

	if (heap_critical)
	{
		std::chrono::time_point<steady_clock> submit_start = steady_clock::now();

		frame_context_t *target_frame = nullptr;
		u64 earliest_sync_time = UINT64_MAX;
		for (s32 i = 0; i < VK_MAX_ASYNC_FRAMES; ++i)
		{
			auto ctx = &frame_context_storage[i];
			if (ctx->swap_command_buffer)
			{
				if (ctx->last_frame_sync_time > m_last_heap_sync_time &&
					ctx->last_frame_sync_time < earliest_sync_time)
					target_frame = ctx;
			}
		}

		if (target_frame == nullptr)
		{
			flush_command_queue(true);
			m_vertex_cache->purge();

			m_index_buffer_ring_info.reset_allocation_stats();
			m_fragment_env_ring_info.reset_allocation_stats();
			m_vertex_env_ring_info.reset_allocation_stats();
			m_fragment_texture_params_ring_info.reset_allocation_stats();
			m_vertex_layout_ring_info.reset_allocation_stats();
			m_fragment_constants_ring_info.reset_allocation_stats();
			m_transform_constants_ring_info.reset_allocation_stats();
			m_attrib_ring_info.reset_allocation_stats();
			m_texture_upload_buffer_ring_info.reset_allocation_stats();
			m_current_frame->reset_heap_ptrs();
			m_last_heap_sync_time = get_system_time();
		}
		else
		{
			target_frame->swap_command_buffer->poke();
			while (target_frame->swap_command_buffer->pending)
			{
				if (!target_frame->swap_command_buffer->poke())
					std::this_thread::yield();
			}

			process_swap_request(target_frame, true);
		}

		std::chrono::time_point<steady_clock> submit_end = steady_clock::now();
		m_flip_time += std::chrono::duration_cast<std::chrono::microseconds>(submit_end - submit_start).count();
	}
}

void VKGSRender::check_present_status()
{
	if (!m_queued_frames.empty())
	{
		auto ctx = m_queued_frames.front();
		if (ctx->swap_command_buffer->pending)
		{
			if (!ctx->swap_command_buffer->poke())
			{
				return;
			}
		}

		process_swap_request(ctx, true);
	}
}

void VKGSRender::check_descriptors()
{
	// Ease resource pressure if the number of draw calls becomes too high or we are running low on memory resources
	const auto required_descriptors = rsx::method_registers.current_draw_clause.pass_count();
	verify(HERE), required_descriptors < DESCRIPTOR_MAX_DRAW_CALLS;
	if ((required_descriptors + m_current_frame->used_descriptors) > DESCRIPTOR_MAX_DRAW_CALLS)
	{
		// Should hard sync before resetting descriptors for spec compliance
		flush_command_queue(true);

		m_current_frame->descriptor_pool.reset(0);
		m_current_frame->used_descriptors = 0;
	}
}

void VKGSRender::check_window_status()
{
	if (m_swapchain->supports_automatic_wm_reports())
	{
		// This driver will report window events as VK_ERROR_OUT_OF_DATE_KHR
		m_frame->clear_wm_events();
		return;
	}

#ifdef _WIN32

	if (LIKELY(!m_frame->has_wm_events()))
	{
		return;
	}

	while (const auto _event = m_frame->get_wm_event())
	{
		switch (_event)
		{
		case wm_event::toggle_fullscreen:
		{
			renderer_unavailable = true;
			m_frame->enable_wm_fullscreen();
			m_frame->toggle_fullscreen();
			m_frame->disable_wm_fullscreen();
			break;
		}
		case wm_event::geometry_change_notice:
		{
			// Stall until finish notification is received. Also, raise surface dirty flag
			u32 timeout = 1000;
			bool handled = false;

			while (timeout)
			{
				switch (m_frame->get_wm_event())
				{
				default:
					break;
				case wm_event::window_resized:
					handled = true;
					present_surface_dirty_flag = true;
					break;
				case wm_event::geometry_change_in_progress:
					timeout += 10; // Extend timeout to wait for user to finish resizing
					break;
				case wm_event::window_restored:
				case wm_event::window_visibility_changed:
				case wm_event::window_minimized:
				case wm_event::window_moved:
					handled = true; // Ignore these events as they do not alter client area
					break;
				}

				if (handled)
				{
					break;
				}
				else
				{
					// Wait for window manager event
					std::this_thread::sleep_for(1ms);
					timeout --;
				}
			}

			if (!timeout)
			{
				LOG_ERROR(RSX, "wm event handler timed out");
			}

			// Reset renderer availability if something has changed about the window
			renderer_unavailable = false;
			break;
		}
		case wm_event::window_resized:
		{
			LOG_ERROR(RSX, "wm_event::window_resized received without corresponding wm_event::geometry_change_notice!");
			std::this_thread::sleep_for(100ms);
			renderer_unavailable = false;
			break;
		}
		}
	}

#else

	// If the queue is in use, it should be properly consumed
	verify(HERE), !m_frame->has_wm_events();

	const auto frame_width = m_frame->client_width();
	const auto frame_height = m_frame->client_height();

	if (m_client_height != frame_height ||
		m_client_width != frame_width)
	{
		if (!!frame_width && !!frame_height)
		{
			present_surface_dirty_flag = true;
			renderer_unavailable = false;
		}
	}

#endif
}

VkDescriptorSet VKGSRender::allocate_descriptor_set()
{
	verify(HERE), m_current_frame->used_descriptors < DESCRIPTOR_MAX_DRAW_CALLS;

	VkDescriptorSetAllocateInfo alloc_info = {};
	alloc_info.descriptorPool = m_current_frame->descriptor_pool;
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts = &descriptor_layouts;
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

	VkDescriptorSet new_descriptor_set;
	CHECK_RESULT(vkAllocateDescriptorSets(*m_device, &alloc_info, &new_descriptor_set));
	m_current_frame->used_descriptors++;

	return new_descriptor_set;
}

void VKGSRender::begin()
{
	rsx::thread::begin();

	if (skip_frame || renderer_unavailable ||
		(conditional_render_enabled && conditional_render_test_failed))
		return;

	init_buffers(rsx::framebuffer_creation_context::context_draw);

	if (!framebuffer_status_valid)
		return;

	if (m_current_frame->flags & frame_context_state::dirty)
	{
		check_present_status();

		if (m_current_frame->swap_command_buffer)
		{
			// Borrow time by using the auxilliary context
			m_aux_frame_context.grab_resources(*m_current_frame);
			m_current_frame = &m_aux_frame_context;
		}
		else if (m_current_frame->used_descriptors)
		{
			m_current_frame->descriptor_pool.reset(0);
			m_current_frame->used_descriptors = 0;
		}

		verify(HERE), !m_current_frame->swap_command_buffer;

		m_current_frame->flags &= ~frame_context_state::dirty;
	}
	else
	{
		check_present_status();
	}
}

void VKGSRender::update_draw_state()
{
	std::chrono::time_point<steady_clock> start = steady_clock::now();

	float actual_line_width = rsx::method_registers.line_width();
	vkCmdSetLineWidth(*m_current_command_buffer, actual_line_width);

	if (rsx::method_registers.poly_offset_fill_enabled())
	{
		//offset_bias is the constant factor, multiplied by the implementation factor R
		//offst_scale is the slope factor, multiplied by the triangle slope factor M
		vkCmdSetDepthBias(*m_current_command_buffer, rsx::method_registers.poly_offset_bias(), 0.f, rsx::method_registers.poly_offset_scale());
	}
	else
	{
		//Zero bias value - disables depth bias
		vkCmdSetDepthBias(*m_current_command_buffer, 0.f, 0.f, 0.f);
	}

	//Update dynamic state
	if (rsx::method_registers.blend_enabled())
	{
		//Update blend constants
		auto blend_colors = rsx::get_constant_blend_colors();
		vkCmdSetBlendConstants(*m_current_command_buffer, blend_colors.data());
	}

	if (rsx::method_registers.stencil_test_enabled())
	{
		const bool two_sided_stencil = rsx::method_registers.two_sided_stencil_test_enabled();
		VkStencilFaceFlags face_flag = (two_sided_stencil) ? VK_STENCIL_FACE_FRONT_BIT : VK_STENCIL_FRONT_AND_BACK;

		vkCmdSetStencilWriteMask(*m_current_command_buffer, face_flag, rsx::method_registers.stencil_mask());
		vkCmdSetStencilCompareMask(*m_current_command_buffer, face_flag, rsx::method_registers.stencil_func_mask());
		vkCmdSetStencilReference(*m_current_command_buffer, face_flag, rsx::method_registers.stencil_func_ref());

		if (two_sided_stencil)
		{
			vkCmdSetStencilWriteMask(*m_current_command_buffer, VK_STENCIL_FACE_BACK_BIT, rsx::method_registers.back_stencil_mask());
			vkCmdSetStencilCompareMask(*m_current_command_buffer, VK_STENCIL_FACE_BACK_BIT, rsx::method_registers.back_stencil_func_mask());
			vkCmdSetStencilReference(*m_current_command_buffer, VK_STENCIL_FACE_BACK_BIT, rsx::method_registers.back_stencil_func_ref());
		}
	}

	if (rsx::method_registers.depth_bounds_test_enabled())
	{
		//Update depth bounds min/max
		vkCmdSetDepthBounds(*m_current_command_buffer, rsx::method_registers.depth_bounds_min(), rsx::method_registers.depth_bounds_max());
	}
	else
	{
		vkCmdSetDepthBounds(*m_current_command_buffer, 0.f, 1.f);
	}

	bind_viewport();

	//TODO: Set up other render-state parameters into the program pipeline

	std::chrono::time_point<steady_clock> stop = steady_clock::now();
	m_setup_time += std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
}

void VKGSRender::begin_render_pass()
{
	if (m_render_pass_open)
		return;

	const auto renderpass = (m_cached_renderpass)? m_cached_renderpass : vk::get_renderpass(*m_device, m_current_renderpass_key);

	VkRenderPassBeginInfo rp_begin = {};
	rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rp_begin.renderPass = renderpass;
	rp_begin.framebuffer = m_draw_fbo->value;
	rp_begin.renderArea.offset.x = 0;
	rp_begin.renderArea.offset.y = 0;
	rp_begin.renderArea.extent.width = m_draw_fbo->width();
	rp_begin.renderArea.extent.height = m_draw_fbo->height();

	vkCmdBeginRenderPass(*m_current_command_buffer, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
	m_render_pass_open = true;
}

void VKGSRender::close_render_pass()
{
	if (!m_render_pass_open)
		return;

	vkCmdEndRenderPass(*m_current_command_buffer);
	m_render_pass_open = false;
}

void VKGSRender::emit_geometry(u32 sub_index)
{
	auto &draw_call = rsx::method_registers.current_draw_clause;
	//std::chrono::time_point<steady_clock> vertex_start = steady_clock::now();

	if (sub_index == 0)
	{
		analyse_inputs_interleaved(m_vertex_layout);
	}

	if (!m_vertex_layout.validate())
	{
		// No vertex inputs enabled
		// Execute remainining pipeline barriers with NOP draw
		do
		{
			draw_call.execute_pipeline_dependencies();
		}
		while (draw_call.next());

		draw_call.end();
		return;
	}

	if (sub_index > 0 && draw_call.execute_pipeline_dependencies() & rsx::vertex_base_changed)
	{
		// Rebase vertex bases instead of 
		for (auto &info : m_vertex_layout.interleaved_blocks)
		{
			const auto vertex_base_offset = rsx::method_registers.vertex_data_base_offset();
			info.real_offset_address = rsx::get_address(rsx::get_vertex_offset_from_base(vertex_base_offset, info.base_offset), info.memory_location);
		}
	}

	const auto old_persistent_buffer = m_persistent_attribute_storage ? m_persistent_attribute_storage->value : null_buffer_view->value;
	const auto old_volatile_buffer = m_volatile_attribute_storage ? m_volatile_attribute_storage->value : null_buffer_view->value;

	// Programs data is dependent on vertex state
	auto upload_info = upload_vertex_data();
	if (!upload_info.vertex_draw_count)
	{
		// Malformed vertex setup; abort
		return;
	}

	//std::chrono::time_point<steady_clock> vertex_end = steady_clock::now();
	//m_vertex_upload_time += std::chrono::duration_cast<std::chrono::microseconds>(vertex_end - vertex_start).count();

	auto persistent_buffer = m_persistent_attribute_storage ? m_persistent_attribute_storage->value : null_buffer_view->value;
	auto volatile_buffer = m_volatile_attribute_storage ? m_volatile_attribute_storage->value : null_buffer_view->value;
	bool update_descriptors = false;

	if (sub_index == 0)
	{
		update_descriptors = true;
	}
	else
	{
		// Vertex env update will change information in the descriptor set
		// Make a copy for the next draw call
		// TODO: Restructure program to allow use of push constants when possible
		// NOTE: AMD has insuffecient push constants buffer memory which is unfortunate
		VkDescriptorSet new_descriptor_set = allocate_descriptor_set();
		std::array<VkCopyDescriptorSet, VK_NUM_DESCRIPTOR_BINDINGS> copy_set;

		for (u32 n = 0; n < VK_NUM_DESCRIPTOR_BINDINGS; ++n)
		{
			copy_set[n] =
			{
				VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET,   // sType
				nullptr,                                 // pNext
				m_current_frame->descriptor_set,         // srcSet
				n,                                       // srcBinding
				0u,                                      // srcArrayElement
				new_descriptor_set,                      // dstSet
				n,                                       // dstBinding
				0u,                                      // dstArrayElement
				1u                                       // descriptorCount
			};
		}

		vkUpdateDescriptorSets(*m_device, 0, 0, VK_NUM_DESCRIPTOR_BINDINGS, copy_set.data());
		m_current_frame->descriptor_set = new_descriptor_set;

		if (persistent_buffer != old_persistent_buffer || volatile_buffer != old_volatile_buffer)
		{
			// Rare event, we need to actually change the descriptors
			update_descriptors = true;
		}
	}

	// Update vertex fetch parameters
	update_vertex_env(upload_info);

	if (update_descriptors)
	{
		m_program->bind_uniform(persistent_buffer, vk::glsl::program_input_type::input_type_texel_buffer, "persistent_input_stream", m_current_frame->descriptor_set);
		m_program->bind_uniform(volatile_buffer, vk::glsl::program_input_type::input_type_texel_buffer, "volatile_input_stream", m_current_frame->descriptor_set);
	}

	// Bind the new set of descriptors for use with this draw call
	vkCmdBindDescriptorSets(*m_current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &m_current_frame->descriptor_set, 0, nullptr);

	//std::chrono::time_point<steady_clock> draw_start = steady_clock::now();
	//m_setup_time += std::chrono::duration_cast<std::chrono::microseconds>(draw_start - vertex_end).count();

	if (!upload_info.index_info)
	{
		if (draw_call.is_single_draw())
		{
			vkCmdDraw(*m_current_command_buffer, upload_info.vertex_draw_count, 1, 0, 0);
		}
		else
		{
			u32 vertex_offset = 0;
			const auto subranges = draw_call.get_subranges();
			for (const auto &range : subranges)
			{
				vkCmdDraw(*m_current_command_buffer, range.count, 1, vertex_offset, 0);
				vertex_offset += range.count;
			}
		}
	}
	else
	{
		const VkIndexType index_type = std::get<1>(*upload_info.index_info);
		const VkDeviceSize offset = std::get<0>(*upload_info.index_info);

		vkCmdBindIndexBuffer(*m_current_command_buffer, m_index_buffer_ring_info.heap->value, offset, index_type);

		if (rsx::method_registers.current_draw_clause.is_single_draw())
		{
			const u32 index_count = upload_info.vertex_draw_count;
			vkCmdDrawIndexed(*m_current_command_buffer, index_count, 1, 0, 0, 0);
		}
		else
		{
			u32 vertex_offset = 0;
			const auto subranges = draw_call.get_subranges();
			for (const auto &range : subranges)
			{
				const auto count = get_index_count(draw_call.primitive, range.count);
				vkCmdDrawIndexed(*m_current_command_buffer, count, 1, vertex_offset, 0, 0);
				vertex_offset += count;
			}
		}
	}

	//std::chrono::time_point<steady_clock> draw_end = steady_clock::now();
	//m_draw_time += std::chrono::duration_cast<std::chrono::microseconds>(draw_end - draw_start).count();
}

void VKGSRender::end()
{
	if (skip_frame || !framebuffer_status_valid || renderer_unavailable ||
		(conditional_render_enabled && conditional_render_test_failed))
	{
		execute_nop_draw();
		rsx::thread::end();
		return;
	}

	std::chrono::time_point<steady_clock> textures_start = steady_clock::now();

	// Check for data casts
	auto ds = std::get<1>(m_rtts.m_bound_depth_stencil);
	if (ds && ds->old_contents &&
		ds->old_contents.source->info.format == VK_FORMAT_B8G8R8A8_UNORM &&
		rsx::pitch_compatible(ds, vk::as_rtt(ds->old_contents.source)))
	{
		auto key = vk::get_renderpass_key(ds->info.format);
		auto render_pass = vk::get_renderpass(*m_device, key);
		verify("Usupported renderpass configuration" HERE), render_pass != VK_NULL_HANDLE;

		VkClearDepthStencilValue clear = { 1.f, 0xFF };
		VkImageSubresourceRange range = { VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 };

		// Clear explicitly before starting the inheritance transfer
		const bool preinitialized = (ds->current_layout == VK_IMAGE_LAYOUT_GENERAL);
		if (!preinitialized) ds->push_layout(*m_current_command_buffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		vkCmdClearDepthStencilImage(*m_current_command_buffer, ds->value, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear, 1, &range);
		if (!preinitialized) ds->pop_layout(*m_current_command_buffer);

		// TODO: Stencil transfer
		ds->old_contents.init_transfer(ds);
		m_depth_converter->run(*m_current_command_buffer,
			ds->old_contents.src_rect(),
			ds->old_contents.dst_rect(),
			vk::as_rtt(ds->old_contents.source)->get_view(0xAAE4, rsx::default_remap_vector),
			ds, render_pass);

		// TODO: Flush management to avoid pass running out of ubo space (very unlikely)
		ds->on_write();
	}

	//Load textures
	{
		std::lock_guard lock(m_sampler_mutex);
		bool update_framebuffer_sourced = false;
		bool check_for_cyclic_refs = false;

		if (UNLIKELY(surface_store_tag != m_rtts.cache_tag))
		{
			update_framebuffer_sourced = true;
			surface_store_tag = m_rtts.cache_tag;
		}

		for (int i = 0; i < rsx::limits::fragment_textures_count; ++i)
		{
			if (!fs_sampler_state[i])
				fs_sampler_state[i] = std::make_unique<vk::texture_cache::sampled_image_descriptor>();

			if (m_samplers_dirty || m_textures_dirty[i] ||
				(update_framebuffer_sourced && fs_sampler_state[i]->upload_context == rsx::texture_upload_context::framebuffer_storage))
			{
				auto sampler_state = static_cast<vk::texture_cache::sampled_image_descriptor*>(fs_sampler_state[i].get());

				if (rsx::method_registers.fragment_textures[i].enabled())
				{
					check_heap_status(VK_HEAP_CHECK_TEXTURE_UPLOAD_STORAGE);
					*sampler_state = m_texture_cache.upload_texture(*m_current_command_buffer, rsx::method_registers.fragment_textures[i], m_rtts);

					if (sampler_state->is_cyclic_reference)
					{
						check_for_cyclic_refs |= true;
					}

					bool replace = !fs_sampler_handles[i];
					VkFilter min_filter, mag_filter;
					VkSamplerMipmapMode mip_mode;
					f32 min_lod = 0.f, max_lod = 0.f;
					f32 lod_bias = 0.f;

					const u32 texture_format = rsx::method_registers.fragment_textures[i].format() & ~(CELL_GCM_TEXTURE_UN | CELL_GCM_TEXTURE_LN);
					VkBool32 compare_enabled = VK_FALSE;
					VkCompareOp depth_compare_mode = VK_COMPARE_OP_NEVER;

					if (texture_format >= CELL_GCM_TEXTURE_DEPTH24_D8 && texture_format <= CELL_GCM_TEXTURE_DEPTH16_FLOAT)
					{
						if (m_device->get_formats_support().d24_unorm_s8)
						{
							// NOTE:
							// The nvidia-specific format D24S8 has a special way of doing depth comparison that matches the PS3
							// In case of projected shadow lookup the result of the divide operation has its Z clamped to [0-1] before comparison
							// Most other wide formats (Z bits > 16) do not behave this way and depth greater than 1 is possible due to the use of floating point as storage
							// Compare operations for these formats (such as D32_SFLOAT) are therefore emulated for correct results

							// NOTE2:
							// To improve reusability, DEPTH16 shadow ops are also emulated if D24S8 support is not available

							compare_enabled = VK_TRUE;
							depth_compare_mode = vk::get_compare_func(rsx::method_registers.fragment_textures[i].zfunc(), true);
						}
					}

					const bool aniso_override = !g_cfg.video.strict_rendering_mode && g_cfg.video.anisotropic_level_override > 0;
					const f32 af_level = aniso_override ? g_cfg.video.anisotropic_level_override : vk::max_aniso(rsx::method_registers.fragment_textures[i].max_aniso());
					const auto wrap_s = vk::vk_wrap_mode(rsx::method_registers.fragment_textures[i].wrap_s());
					const auto wrap_t = vk::vk_wrap_mode(rsx::method_registers.fragment_textures[i].wrap_t());
					const auto wrap_r = vk::vk_wrap_mode(rsx::method_registers.fragment_textures[i].wrap_r());
					const auto border_color = vk::get_border_color(rsx::method_registers.fragment_textures[i].border_color());

					// Check if non-point filtering can even be used on this format
					bool can_sample_linear;
					if (LIKELY(!sampler_state->is_depth_texture))
					{
						// Most PS3-like formats can be linearly filtered without problem
						can_sample_linear = true;
					}
					else
					{
						// Not all GPUs support linear filtering of depth formats
						const auto vk_format = sampler_state->image_handle ? sampler_state->image_handle->image()->format() :
							vk::get_compatible_sampler_format(m_device->get_formats_support(), sampler_state->external_subresource_desc.gcm_format);

						can_sample_linear = m_device->get_format_properties(vk_format).optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
					}

					if (can_sample_linear)
					{
						mag_filter = vk::get_mag_filter(rsx::method_registers.fragment_textures[i].mag_filter());
						std::tie(min_filter, mip_mode) = vk::get_min_filter_and_mip(rsx::method_registers.fragment_textures[i].min_filter());
					}
					else
					{
						mag_filter = min_filter = VK_FILTER_NEAREST;
						mip_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
					}

					if (sampler_state->upload_context == rsx::texture_upload_context::shader_read &&
						rsx::method_registers.fragment_textures[i].get_exact_mipmap_count() > 1)
					{
						min_lod = (float)(rsx::method_registers.fragment_textures[i].min_lod() >> 8);
						max_lod = (float)(rsx::method_registers.fragment_textures[i].max_lod() >> 8);
						lod_bias = rsx::method_registers.fragment_textures[i].bias();
					}
					else
					{
						mip_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
					}

					if (fs_sampler_handles[i] && m_textures_dirty[i])
					{
						if (!fs_sampler_handles[i]->matches(wrap_s, wrap_t, wrap_r, false, lod_bias, af_level, min_lod, max_lod,
							min_filter, mag_filter, mip_mode, border_color, compare_enabled, depth_compare_mode))
						{
							replace = true;
						}
					}

					if (replace)
					{
						fs_sampler_handles[i] = m_resource_manager.find_sampler(*m_device, wrap_s, wrap_t, wrap_r, false, lod_bias, af_level, min_lod, max_lod,
							min_filter, mag_filter, mip_mode, border_color, compare_enabled, depth_compare_mode);
					}
				}
				else
				{
					*sampler_state = {};
				}

				m_textures_dirty[i] = false;
			}
		}

		for (int i = 0; i < rsx::limits::vertex_textures_count; ++i)
		{
			if (!vs_sampler_state[i])
				vs_sampler_state[i] = std::make_unique<vk::texture_cache::sampled_image_descriptor>();

			if (m_samplers_dirty || m_vertex_textures_dirty[i] ||
				(update_framebuffer_sourced && vs_sampler_state[i]->upload_context == rsx::texture_upload_context::framebuffer_storage))
			{
				auto sampler_state = static_cast<vk::texture_cache::sampled_image_descriptor*>(vs_sampler_state[i].get());

				if (rsx::method_registers.vertex_textures[i].enabled())
				{
					check_heap_status(VK_HEAP_CHECK_TEXTURE_UPLOAD_STORAGE);
					*sampler_state = m_texture_cache.upload_texture(*m_current_command_buffer, rsx::method_registers.vertex_textures[i], m_rtts);

					if (sampler_state->is_cyclic_reference)
					{
						check_for_cyclic_refs |= true;
					}

					bool replace = !vs_sampler_handles[i];
					const VkBool32 unnormalized_coords = !!(rsx::method_registers.vertex_textures[i].format() & CELL_GCM_TEXTURE_UN);
					const auto min_lod = (f32)rsx::method_registers.vertex_textures[i].min_lod();
					const auto max_lod = (f32)rsx::method_registers.vertex_textures[i].max_lod();
					const auto border_color = vk::get_border_color(rsx::method_registers.vertex_textures[i].border_color());

					if (vs_sampler_handles[i])
					{
						if (!vs_sampler_handles[i]->matches(VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT,
							unnormalized_coords, 0.f, 1.f, min_lod, max_lod, VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST, border_color))
						{
							replace = true;
						}
					}

					if (replace)
					{
						vs_sampler_handles[i] = m_resource_manager.find_sampler(
							*m_device,
							VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT,
							unnormalized_coords,
							0.f, 1.f, min_lod, max_lod,
							VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST, border_color);
					}
				}
				else
					*sampler_state = {};

				m_vertex_textures_dirty[i] = false;
			}
		}

		m_samplers_dirty.store(false);

		if (check_for_cyclic_refs)
		{
			// Regenerate renderpass key
			if (const auto key = vk::get_renderpass_key(m_fbo_images, m_current_renderpass_key);
				key != m_current_renderpass_key)
			{
				m_current_renderpass_key = key;
				m_cached_renderpass = VK_NULL_HANDLE;
			}
		}
	}

	std::chrono::time_point<steady_clock> textures_end = steady_clock::now();
	m_textures_upload_time += (u32)std::chrono::duration_cast<std::chrono::microseconds>(textures_end - textures_start).count();

	std::chrono::time_point<steady_clock> program_start = textures_end;
	if (!load_program())
	{
		// Program is not ready, skip drawing this
		std::this_thread::yield();
		execute_nop_draw();
		// m_rtts.on_write(); - breaks games for obvious reasons
		rsx::thread::end();
		return;
	}

	// Allocate descriptor set
	check_descriptors();
	m_current_frame->descriptor_set = allocate_descriptor_set();

	// Load program execution environment
	load_program_env();

	std::chrono::time_point<steady_clock> program_end = steady_clock::now();
	m_setup_time += std::chrono::duration_cast<std::chrono::microseconds>(program_end - program_start).count();

	textures_start = program_end;

	for (int i = 0; i < rsx::limits::fragment_textures_count; ++i)
	{
		if (current_fp_metadata.referenced_textures_mask & (1 << i))
		{
			vk::image_view* view = nullptr;
			auto sampler_state = static_cast<vk::texture_cache::sampled_image_descriptor*>(fs_sampler_state[i].get());

			if (rsx::method_registers.fragment_textures[i].enabled() &&
				sampler_state->validate())
			{
				if (view = sampler_state->image_handle; !view)
				{
					//Requires update, copy subresource
					view = m_texture_cache.create_temporary_subresource(*m_current_command_buffer, sampler_state->external_subresource_desc);
				}
				else
				{
					switch (view->image()->current_layout)
					{
					default:
					//case VK_IMAGE_LAYOUT_GENERAL:
					//case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
						break;
					case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
						verify(HERE), sampler_state->upload_context == rsx::texture_upload_context::blit_engine_dst;
						view->image()->change_layout(*m_current_command_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
						break;
					}
				}
			}

			if (LIKELY(view))
			{
				m_program->bind_uniform({ fs_sampler_handles[i]->value, view->value, view->image()->current_layout },
					i,
					::glsl::program_domain::glsl_fragment_program,
					m_current_frame->descriptor_set);

				if (current_fragment_program.redirected_textures & (1 << i))
				{
					// Stencil mirror required
					auto root_image = static_cast<vk::viewable_image*>(view->image());
					auto stencil_view = root_image->get_view(0xAAE4, rsx::default_remap_vector, VK_IMAGE_ASPECT_STENCIL_BIT);

					if (!m_stencil_mirror_sampler)
					{
						m_stencil_mirror_sampler = std::make_unique<vk::sampler>(*m_device,
							VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
							VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
							VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
							VK_FALSE, 0.f, 1.f, 0.f, 0.f,
							VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST,
							VK_BORDER_COLOR_INT_OPAQUE_BLACK);
					}

					m_program->bind_uniform({ m_stencil_mirror_sampler->value, stencil_view->value, stencil_view->image()->current_layout },
						i,
						::glsl::program_domain::glsl_fragment_program,
						m_current_frame->descriptor_set,
						true);
				}
			}
			else
			{
				m_program->bind_uniform({ vk::null_sampler(), vk::null_image_view(*m_current_command_buffer)->value, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
					i,
					::glsl::program_domain::glsl_fragment_program,
					m_current_frame->descriptor_set);

				if (current_fragment_program.redirected_textures & (1 << i))
				{
					m_program->bind_uniform({ vk::null_sampler(), vk::null_image_view(*m_current_command_buffer)->value, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
						i,
						::glsl::program_domain::glsl_fragment_program,
						m_current_frame->descriptor_set,
						true);
				}
			}
		}
	}

	for (int i = 0; i < rsx::limits::vertex_textures_count; ++i)
	{
		if (current_vp_metadata.referenced_textures_mask & (1 << i))
		{
			if (!rsx::method_registers.vertex_textures[i].enabled())
			{
				m_program->bind_uniform({ vk::null_sampler(), vk::null_image_view(*m_current_command_buffer)->value, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
					i,
					::glsl::program_domain::glsl_vertex_program,
					m_current_frame->descriptor_set);

				continue;
			}

			auto sampler_state = static_cast<vk::texture_cache::sampled_image_descriptor*>(vs_sampler_state[i].get());
			auto image_ptr = sampler_state->image_handle;

			if (!image_ptr && sampler_state->validate())
			{
				image_ptr = m_texture_cache.create_temporary_subresource(*m_current_command_buffer, sampler_state->external_subresource_desc);
				m_vertex_textures_dirty[i] = true;
			}

			if (!image_ptr)
			{
				LOG_ERROR(RSX, "Texture upload failed to vtexture index %d. Binding null sampler.", i);
				m_program->bind_uniform({ vk::null_sampler(), vk::null_image_view(*m_current_command_buffer)->value, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, rsx::constants::vertex_texture_names[i], m_current_frame->descriptor_set);
				continue;
			}

			switch (image_ptr->image()->current_layout)
			{
			default:
			//case VK_IMAGE_LAYOUT_GENERAL:
			//case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				break;
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				verify(HERE), sampler_state->upload_context == rsx::texture_upload_context::blit_engine_dst;
				image_ptr->image()->change_layout(*m_current_command_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				break;
			}

			m_program->bind_uniform({ vs_sampler_handles[i]->value, image_ptr->value, image_ptr->image()->current_layout },
				i,
				::glsl::program_domain::glsl_vertex_program,
				m_current_frame->descriptor_set);
		}
	}

	textures_end = steady_clock::now();
	m_textures_upload_time += std::chrono::duration_cast<std::chrono::microseconds>(textures_end - textures_start).count();

	u32 occlusion_id = 0;
	if (m_occlusion_query_active)
	{
		occlusion_id = m_occlusion_query_pool.find_free_slot();
		if (occlusion_id == UINT32_MAX)
		{
			m_tsc += 100;
			update(this);

			occlusion_id = m_occlusion_query_pool.find_free_slot();
			if (occlusion_id == UINT32_MAX)
			{
				//LOG_ERROR(RSX, "Occlusion pool overflow");
				if (m_current_task) m_current_task->result = 1;
			}
		}
	}

	bool primitive_emulated = false;
	vk::get_appropriate_topology(rsx::method_registers.current_draw_clause.primitive, primitive_emulated);

	if (m_occlusion_query_active && (occlusion_id != UINT32_MAX))
	{
		//Begin query
		m_occlusion_query_pool.begin_query(*m_current_command_buffer, occlusion_id);
		m_occlusion_map[m_active_query_info->driver_handle].indices.push_back(occlusion_id);
		m_occlusion_map[m_active_query_info->driver_handle].command_buffer_to_wait = m_current_command_buffer;

		m_current_command_buffer->flags |= vk::command_buffer::cb_has_occlusion_task;
	}

	// Final heap check...
	check_heap_status(VK_HEAP_CHECK_VERTEX_STORAGE | VK_HEAP_CHECK_VERTEX_LAYOUT_STORAGE);

	// While vertex upload is an interruptible process, if we made it this far, there's no need to sync anything that occurs past this point
	// Only textures are synchronized tightly with the GPU and they have been read back above
	vk::enter_uninterruptible();

	vkCmdBindPipeline(*m_current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_program->pipeline);
	update_draw_state();

	// Apply write memory barriers
	if (1)//g_cfg.video.strict_rendering_mode)
	{
		if (ds) ds->write_barrier(*m_current_command_buffer);

		for (auto &rtt : m_rtts.m_bound_render_targets)
		{
			if (auto surface = std::get<1>(rtt))
			{
				surface->write_barrier(*m_current_command_buffer);
			}
		}

		begin_render_pass();
	}
	else
	{
		begin_render_pass();

		// Clear any 'dirty' surfaces - possible is a recycled cache surface is used
		rsx::simple_array<VkClearAttachment> buffers_to_clear;

		if (ds && ds->dirty())
		{
			// Clear this surface before drawing on it
			VkClearValue clear_value = {};
			clear_value.depthStencil = { 1.f, 255 };
			buffers_to_clear.push_back({ vk::get_aspect_flags(ds->info.format), 0, clear_value });
		}

		for (u32 index = 0; index < m_draw_buffers.size(); ++index)
		{
			if (auto rtt = std::get<1>(m_rtts.m_bound_render_targets[index]))
			{
				if (rtt->dirty())
				{
					buffers_to_clear.push_back({ VK_IMAGE_ASPECT_COLOR_BIT, index, {} });
				}
			}
		}

		if (UNLIKELY(!buffers_to_clear.empty()))
		{
			VkClearRect rect = { {{0, 0}, {m_draw_fbo->width(), m_draw_fbo->height()}}, 0, 1 };
			vkCmdClearAttachments(*m_current_command_buffer, buffers_to_clear.size(),
				buffers_to_clear.data(), 1, &rect);
		}
	}

	u32 sub_index = 0;
	rsx::method_registers.current_draw_clause.begin();
	do
	{
		emit_geometry(sub_index++);
	}
	while (rsx::method_registers.current_draw_clause.next());

	close_render_pass();
	vk::leave_uninterruptible();

	if (m_occlusion_query_active && (occlusion_id != UINT32_MAX))
	{
		//End query
		m_occlusion_query_pool.end_query(*m_current_command_buffer, occlusion_id);
	}

	m_rtts.on_write();

	rsx::thread::end();
}

void VKGSRender::set_viewport()
{
	const auto clip_width = rsx::apply_resolution_scale(rsx::method_registers.surface_clip_width(), true);
	const auto clip_height = rsx::apply_resolution_scale(rsx::method_registers.surface_clip_height(), true);

	//NOTE: The scale_offset matrix already has viewport matrix factored in
	m_viewport.x = 0;
	m_viewport.y = 0;
	m_viewport.width = clip_width;
	m_viewport.height = clip_height;
	m_viewport.minDepth = 0.f;
	m_viewport.maxDepth = 1.f;
}

void VKGSRender::set_scissor()
{
	if (m_graphics_state & rsx::pipeline_state::scissor_config_state_dirty)
	{
		// Optimistic that the new config will allow us to render
		framebuffer_status_valid = true;
	}
	else if (!(m_graphics_state & rsx::pipeline_state::scissor_config_state_dirty))
	{
		// Nothing to do
		return;
	}

	m_graphics_state &= ~(rsx::pipeline_state::scissor_config_state_dirty | rsx::pipeline_state::scissor_setup_invalid);

	u16 scissor_x = rsx::apply_resolution_scale(rsx::method_registers.scissor_origin_x(), false);
	u16 scissor_w = rsx::apply_resolution_scale(rsx::method_registers.scissor_width(), true);
	u16 scissor_y = rsx::apply_resolution_scale(rsx::method_registers.scissor_origin_y(), false);
	u16 scissor_h = rsx::apply_resolution_scale(rsx::method_registers.scissor_height(), true);

	m_scissor.extent.height = scissor_h;
	m_scissor.extent.width = scissor_w;
	m_scissor.offset.x = scissor_x;
	m_scissor.offset.y = scissor_y;

	if (scissor_x >= m_viewport.width || scissor_y >= m_viewport.height || scissor_w == 0 || scissor_h == 0)
	{
		if (!g_cfg.video.strict_rendering_mode)
		{
			m_graphics_state |= rsx::pipeline_state::scissor_setup_invalid;
			framebuffer_status_valid = false;
			return;
		}
	}
}

void VKGSRender::bind_viewport()
{
	vkCmdSetViewport(*m_current_command_buffer, 0, 1, &m_viewport);
	vkCmdSetScissor(*m_current_command_buffer, 0, 1, &m_scissor);
}

void VKGSRender::on_init_thread()
{
	if (m_device == VK_NULL_HANDLE)
	{
		fmt::throw_exception("No vulkan device was created");
	}

	GSRender::on_init_thread();
	zcull_ctrl.reset(static_cast<::rsx::reports::ZCULL_control*>(this));

	if (!supports_native_ui)
	{
		m_frame->disable_wm_event_queue();
		m_frame->hide();
		m_shaders_cache->load(nullptr, *m_device, pipeline_layout);
		m_frame->enable_wm_event_queue();
		m_frame->show();
	}
	else
	{
		struct native_helper : vk::shader_cache::progress_dialog_helper
		{
			rsx::thread *owner = nullptr;
			std::shared_ptr<rsx::overlays::message_dialog> dlg;

			native_helper(VKGSRender *ptr) :
				owner(ptr) {}

			void create() override
			{
				MsgDialogType type = {};
				type.disable_cancel = true;
				type.progress_bar_count = 2;

				dlg = fxm::get<rsx::overlays::display_manager>()->create<rsx::overlays::message_dialog>((bool)g_cfg.video.shader_preloading_dialog.use_custom_background);
				dlg->progress_bar_set_taskbar_index(-1);
				dlg->show("Loading precompiled shaders from disk...", type, [](s32 status)
				{
					if (status != CELL_OK)
						Emu.Stop();
				});
			}

			void update_msg(u32 index, u32 processed, u32 entry_count) override
			{
				const char *text = index == 0 ? "Loading pipeline object %u of %u" : "Compiling pipeline object %u of %u";
				dlg->progress_bar_set_message(index, fmt::format(text, processed, entry_count));
				owner->flip(0);
			}

			void inc_value(u32 index, u32 value) override
			{
				dlg->progress_bar_increment(index, (f32)value);
				owner->flip(0);
			}

			void set_limit(u32 index, u32 limit) override
			{
				dlg->progress_bar_set_limit(index, limit);
				owner->flip(0);
			}

			void refresh() override
			{
				dlg->refresh();
			}

			void close() override
			{
				dlg->return_code = CELL_OK;
				dlg->close();
			}
		}
		helper(this);

		//TODO: Handle window resize messages during loading on GPUs without OUT_OF_DATE_KHR support
		m_frame->disable_wm_event_queue();
		m_shaders_cache->load(&helper, *m_device, pipeline_layout);
		m_frame->enable_wm_event_queue();

#ifdef _WIN32
		if (!m_swapchain->supports_automatic_wm_reports())
		{
			// If the renderer does not handle WM events itself, switching to fullscreen is done by the renderer, not the UI
			m_frame->disable_wm_fullscreen();
		}
#endif
	}
}

void VKGSRender::on_exit()
{
	zcull_ctrl.release();
	GSRender::on_exit();
}

void VKGSRender::clear_surface(u32 mask)
{
	if (skip_frame || renderer_unavailable) return;

	// If stencil write mask is disabled, remove clear_stencil bit
	if (!rsx::method_registers.stencil_mask()) mask &= ~0x2u;

	// Ignore invalid clear flags
	if (!(mask & 0xF3)) return;

	u8 ctx = rsx::framebuffer_creation_context::context_draw;
	if (mask & 0xF0) ctx |= rsx::framebuffer_creation_context::context_clear_color;
	if (mask & 0x3) ctx |= rsx::framebuffer_creation_context::context_clear_depth;
	init_buffers((rsx::framebuffer_creation_context)ctx);

	if (!framebuffer_status_valid) return;

	float depth_clear = 1.f;
	u32   stencil_clear = 0;
	u32   depth_stencil_mask = 0;

	std::vector<VkClearAttachment> clear_descriptors;
	VkClearValue depth_stencil_clear_values = {}, color_clear_values = {};

	u16 scissor_x = (u16)m_scissor.offset.x;
	u16 scissor_w = (u16)m_scissor.extent.width;
	u16 scissor_y = (u16)m_scissor.offset.y;
	u16 scissor_h = (u16)m_scissor.extent.height;

	const u16 fb_width = m_draw_fbo->width();
	const u16 fb_height = m_draw_fbo->height();

	//clip region
	std::tie(scissor_x, scissor_y, scissor_w, scissor_h) = rsx::clip_region<u16>(fb_width, fb_height, scissor_x, scissor_y, scissor_w, scissor_h, true);
	VkClearRect region = { { { scissor_x, scissor_y },{ scissor_w, scissor_h } }, 0, 1 };

	const bool require_mem_load = (scissor_w * scissor_h) < (fb_width * fb_height);
	auto surface_depth_format = rsx::method_registers.surface_depth_fmt();

	if (auto ds = std::get<1>(m_rtts.m_bound_depth_stencil); mask & 0x3)
	{
		if (mask & 0x1)
		{
			u32 max_depth_value = get_max_depth_value(surface_depth_format);

			u32 clear_depth = rsx::method_registers.z_clear_value(surface_depth_format == rsx::surface_depth_format::z24s8);
			float depth_clear = (float)clear_depth / max_depth_value;

			depth_stencil_clear_values.depthStencil.depth = depth_clear;
			depth_stencil_clear_values.depthStencil.stencil = stencil_clear;

			depth_stencil_mask |= VK_IMAGE_ASPECT_DEPTH_BIT;
		}

		if (surface_depth_format == rsx::surface_depth_format::z24s8)
		{
			if (mask & 0x2)
			{
				u8 clear_stencil = rsx::method_registers.stencil_clear_value();
				depth_stencil_clear_values.depthStencil.stencil = clear_stencil;

				depth_stencil_mask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}

			if ((mask & 0x3) != 0x3 && !require_mem_load && ds->state_flags & rsx::surface_state_flags::erase_bkgnd)
			{
				verify(HERE), depth_stencil_mask;

				// Only one aspect was cleared. Make sure to memory intialize the other before removing dirty flag
				if (mask == 1)
				{
					// Depth was cleared, initialize stencil
					depth_stencil_clear_values.depthStencil.stencil = 0xFF;
					depth_stencil_mask |= VK_IMAGE_ASPECT_STENCIL_BIT;
				}
				else
				{
					// Stencil was cleared, initialize depth
					depth_stencil_clear_values.depthStencil.depth = 1.f;
					depth_stencil_mask |= VK_IMAGE_ASPECT_DEPTH_BIT;
				}
			}
		}
	}

	if (auto colormask = (mask & 0xF0))
	{
		if (!m_draw_buffers.empty())
		{
			bool use_fast_clear = false;
			bool ignore_clear = false;
			switch (rsx::method_registers.surface_color())
			{
			case rsx::surface_color_format::x32:
			case rsx::surface_color_format::w16z16y16x16:
			case rsx::surface_color_format::w32z32y32x32:
				//NOP
				ignore_clear = true;
				break;
			case rsx::surface_color_format::g8b8:
				colormask = rsx::get_g8b8_r8g8_colormask(colormask);
				use_fast_clear = (colormask == (0x10 | 0x20));
				ignore_clear = (colormask == 0);
				colormask |= (0x40 | 0x80);
				break;
			default:
				use_fast_clear = (colormask == (0x10 | 0x20 | 0x40 | 0x80));
				break;
			}

			if (!ignore_clear)
			{
				u8 clear_a = rsx::method_registers.clear_color_a();
				u8 clear_r = rsx::method_registers.clear_color_r();
				u8 clear_g = rsx::method_registers.clear_color_g();
				u8 clear_b = rsx::method_registers.clear_color_b();

				color_clear_values.color.float32[0] = (float)clear_r / 255;
				color_clear_values.color.float32[1] = (float)clear_g / 255;
				color_clear_values.color.float32[2] = (float)clear_b / 255;
				color_clear_values.color.float32[3] = (float)clear_a / 255;

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

					VkRenderPass renderpass = VK_NULL_HANDLE;
					m_attachment_clear_pass->update_config(colormask, clear_color);

					for (const auto &index : m_draw_buffers)
					{
						if (auto rtt = m_rtts.m_bound_render_targets[index].second)
						{
							if (require_mem_load) rtt->write_barrier(*m_current_command_buffer);

							// Add a barrier to ensure previous writes are visible; also transitions into GENERAL layout
							const auto old_layout = rtt->current_layout;
							vk::insert_texture_barrier(*m_current_command_buffer, rtt, VK_IMAGE_LAYOUT_GENERAL);

							if (!renderpass)
							{
								std::vector<vk::image*> surfaces = { rtt };
								const auto key = vk::get_renderpass_key(surfaces);
								renderpass = vk::get_renderpass(*m_device, key);
							}

							m_attachment_clear_pass->run(*m_current_command_buffer, rtt,
								region.rect, renderpass);

							rtt->change_layout(*m_current_command_buffer, old_layout);
						}
						else
							fmt::throw_exception("Unreachable" HERE);
					}

					//Fush unconditionally - parameters might not persist
					//TODO: Better parameter management for overlay passes
					flush_command_queue();
				}

				for (auto &rtt : m_rtts.m_bound_render_targets)
				{
					if (const auto address = rtt.first)
					{
						if (require_mem_load) rtt.second->write_barrier(*m_current_command_buffer);
						m_rtts.on_write(address);
					}
				}
			}
		}
	}

	if (depth_stencil_mask)
	{
		if (const auto address = m_rtts.m_bound_depth_stencil.first)
		{
			if (require_mem_load) m_rtts.m_bound_depth_stencil.second->write_barrier(*m_current_command_buffer);
			m_rtts.on_write(address);
			clear_descriptors.push_back({ (VkImageAspectFlags)depth_stencil_mask, 0, depth_stencil_clear_values });
		}
	}

	if (!clear_descriptors.empty())
	{
		begin_render_pass();
		vkCmdClearAttachments(*m_current_command_buffer, (u32)clear_descriptors.size(), clear_descriptors.data(), 1, &region);
		close_render_pass();
	}
}

void VKGSRender::flush_command_queue(bool hard_sync)
{
	close_and_submit_command_buffer({}, m_current_command_buffer->submit_fence);

	if (hard_sync)
	{
		// swap handler checks the pending flag, so call it here
		process_swap_request(m_current_frame);

		//wait for the latest instruction to execute
		m_current_command_buffer->pending = true;
		m_current_command_buffer->reset();

		//Clear all command buffer statuses
		for (auto &cb : m_primary_cb_list)
		{
			if (cb.pending)
				cb.poke();
		}

		m_flush_requests.clear_pending_flag();
	}
	else
	{
		// Mark this queue as pending
		m_current_command_buffer->pending = true;

		// Grab next cb in line and make it usable
		m_current_cb_index = (m_current_cb_index + 1) % VK_MAX_ASYNC_CB_COUNT;
		m_current_command_buffer = &m_primary_cb_list[m_current_cb_index];

		// Soft sync if a present has not yet occured before consuming the wait event
		for (auto &ctx : frame_context_storage)
		{
			if (ctx.swap_command_buffer == m_current_command_buffer)
				process_swap_request(&ctx, true);
		}

		m_current_command_buffer->reset();
	}

	open_command_buffer();
}

void VKGSRender::sync_hint(rsx::FIFO_hint hint)
{
	if (hint == rsx::FIFO_hint::hint_conditional_render_eval)
	{
		if (m_current_command_buffer->flags & vk::command_buffer::cb_has_occlusion_task)
		{
			// Occlusion test result evaluation is coming up, avoid a hard sync
			if (!m_flush_requests.pending())
			{
				m_flush_requests.post(false);
				m_flush_requests.remove_one();
			}
		}
	}
}

void VKGSRender::advance_queued_frames()
{
	// Check all other frames for completion and clear resources
	check_present_status();

	//m_rtts storage is double buffered and should be safe to tag on frame boundary
	m_rtts.free_invalidated();

	//texture cache is also double buffered to prevent use-after-free
	m_texture_cache.on_frame_end();
	m_samplers_dirty.store(true);

	vk::remove_unused_framebuffers();

	m_vertex_cache->purge();
	m_current_frame->tag_frame_end(m_attrib_ring_info.get_current_put_pos_minus_one(),
		m_vertex_env_ring_info.get_current_put_pos_minus_one(),
		m_fragment_env_ring_info.get_current_put_pos_minus_one(),
		m_vertex_layout_ring_info.get_current_put_pos_minus_one(),
		m_fragment_texture_params_ring_info.get_current_put_pos_minus_one(),
		m_fragment_constants_ring_info.get_current_put_pos_minus_one(),
		m_transform_constants_ring_info.get_current_put_pos_minus_one(),
		m_index_buffer_ring_info.get_current_put_pos_minus_one(),
		m_texture_upload_buffer_ring_info.get_current_put_pos_minus_one());

	m_queued_frames.push_back(m_current_frame);
	verify(HERE), m_queued_frames.size() <= VK_MAX_ASYNC_FRAMES;

	m_current_queue_index = (m_current_queue_index + 1) % VK_MAX_ASYNC_FRAMES;
	m_current_frame = &frame_context_storage[m_current_queue_index];
	m_current_frame->flags |= frame_context_state::dirty;

	vk::advance_frame_counter();
}

void VKGSRender::present(frame_context_t *ctx)
{
	verify(HERE), ctx->present_image != UINT32_MAX;

	if (!present_surface_dirty_flag)
	{
		switch (VkResult error = m_swapchain->present(ctx->present_image))
		{
		case VK_SUCCESS:
			break;
		case VK_SUBOPTIMAL_KHR:
			break;
		case VK_ERROR_OUT_OF_DATE_KHR:
			present_surface_dirty_flag = true;
			break;
		default:
			vk::die_with_error(HERE, error);
		}
	}

	// Presentation image released; reset value
	ctx->present_image = UINT32_MAX;

	// Remove from queued list
	while (!m_queued_frames.empty())
	{
		auto frame = m_queued_frames.front();
		m_queued_frames.pop_front();

		if (frame == ctx)
		{
			break;
		}
	}

	vk::advance_completed_frame_counter();
}

void VKGSRender::queue_swap_request()
{
	// Buffer the swap request and return
	if (m_current_frame->swap_command_buffer &&
		m_current_frame->swap_command_buffer->pending)
	{
		// Its probable that no actual drawing took place
		process_swap_request(m_current_frame);
	}

	m_current_frame->swap_command_buffer = m_current_command_buffer;

	if (m_swapchain->is_headless())
	{
		m_swapchain->end_frame(*m_current_command_buffer, m_current_frame->present_image);
		close_and_submit_command_buffer({}, m_current_command_buffer->submit_fence);
	}
	else
	{
		close_and_submit_command_buffer({ m_current_frame->present_semaphore },
			m_current_command_buffer->submit_fence,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT);
	}

	m_current_frame->swap_command_buffer->pending = true;

	//Grab next cb in line and make it usable
	m_current_cb_index = (m_current_cb_index + 1) % VK_MAX_ASYNC_CB_COUNT;
	m_current_command_buffer = &m_primary_cb_list[m_current_cb_index];
	m_current_command_buffer->reset();

	//Set up new pointers for the next frame
	advance_queued_frames();
	open_command_buffer();
}

void VKGSRender::process_swap_request(frame_context_t *ctx, bool free_resources)
{
	if (!ctx->swap_command_buffer)
		return;

	if (ctx->swap_command_buffer->pending)
	{
		// Perform hard swap here
		if (ctx->swap_command_buffer->wait(FRAME_PRESENT_TIMEOUT) != VK_SUCCESS)
		{
			// Lost surface, release renderer
			present_surface_dirty_flag = true;
			renderer_unavailable = true;
		}

		free_resources = true;
	}

	//Always present
	present(ctx);

	if (free_resources)
	{
		if (g_cfg.video.overlay)
		{
			m_text_writer->reset_descriptors();
		}

		if (m_overlay_manager && m_overlay_manager->has_dirty())
		{
			m_overlay_manager->lock();

			std::vector<u32> uids_to_dispose;
			uids_to_dispose.reserve(m_overlay_manager->get_dirty().size());

			for (const auto& view : m_overlay_manager->get_dirty())
			{
				m_ui_renderer->remove_temp_resources(view->uid);
				uids_to_dispose.push_back(view->uid);
			}

			m_overlay_manager->unlock();
			m_overlay_manager->dispose(uids_to_dispose);
		}

		vk::reset_compute_tasks();

		m_attachment_clear_pass->free_resources();
		m_depth_converter->free_resources();
		m_ui_renderer->free_resources();

		ctx->buffer_views_to_clean.clear();

		if (ctx->last_frame_sync_time > m_last_heap_sync_time)
		{
			m_last_heap_sync_time = ctx->last_frame_sync_time;

			//Heap cleanup; deallocates memory consumed by the frame if it is still held
			m_attrib_ring_info.m_get_pos = ctx->attrib_heap_ptr;
			m_vertex_env_ring_info.m_get_pos = ctx->vtx_env_heap_ptr;
			m_fragment_env_ring_info.m_get_pos = ctx->frag_env_heap_ptr;
			m_fragment_constants_ring_info.m_get_pos = ctx->frag_const_heap_ptr;
			m_transform_constants_ring_info.m_get_pos = ctx->vtx_const_heap_ptr;
			m_vertex_layout_ring_info.m_get_pos = ctx->vtx_layout_heap_ptr;
			m_fragment_texture_params_ring_info.m_get_pos = ctx->frag_texparam_heap_ptr;
			m_index_buffer_ring_info.m_get_pos = ctx->index_heap_ptr;
			m_texture_upload_buffer_ring_info.m_get_pos = ctx->texture_upload_heap_ptr;

			m_attrib_ring_info.notify();
			m_vertex_env_ring_info.notify();
			m_fragment_env_ring_info.notify();
			m_fragment_constants_ring_info.notify();
			m_transform_constants_ring_info.notify();
			m_vertex_layout_ring_info.notify();
			m_fragment_texture_params_ring_info.notify();
			m_index_buffer_ring_info.notify();
			m_texture_upload_buffer_ring_info.notify();
		}
	}

	ctx->swap_command_buffer = nullptr;
}

void VKGSRender::do_local_task(rsx::FIFO_state state)
{
	if (m_flush_requests.pending())
	{
		std::lock_guard lock(m_flush_queue_mutex);

		//TODO: Determine if a hard sync is necessary
		//Pipeline barriers later may do a better job synchronizing than wholly stalling the pipeline
		flush_command_queue();

		m_flush_requests.clear_pending_flag();
		m_flush_requests.consumer_wait();
	}
	else if (!in_begin_end && state != rsx::FIFO_state::lock_wait)
	{
		if (m_graphics_state & rsx::pipeline_state::framebuffer_reads_dirty)
		{
			//This will re-engage locks and break the texture cache if another thread is waiting in access violation handler!
			//Only call when there are no waiters
			m_texture_cache.do_update();
			m_graphics_state &= ~rsx::pipeline_state::framebuffer_reads_dirty;
		}
	}

	rsx::thread::do_local_task(state);

	switch (state)
	{
	case rsx::FIFO_state::lock_wait:
		// Critical check finished
		return;
	//case rsx::FIFO_state::spinning:
	//case rsx::FIFO_state::empty:
		// We have some time, check the present queue
		//check_present_status();
		//break;
	default:
		break;
	}

	check_window_status();

	if (m_overlay_manager)
	{
		if (!in_begin_end && async_flip_requested & flip_request::native_ui)
		{
			flush_command_queue(true);
			flip((s32)current_display_buffer, false);
		}
	}
}

bool VKGSRender::do_method(u32 cmd, u32 arg)
{
	switch (cmd)
	{
	case NV4097_CLEAR_SURFACE:
		clear_surface(arg);
		return true;
	case NV4097_TEXTURE_READ_SEMAPHORE_RELEASE:
		// Texture barrier, seemingly not very useful
		return true;
	case NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE:
		//sync_at_semaphore_release();
		return true;
	default:
		return false;
	}
}

bool VKGSRender::load_program()
{
	if (m_graphics_state & rsx::pipeline_state::invalidate_pipeline_bits)
	{
		get_current_fragment_program(fs_sampler_state);
		verify(HERE), current_fragment_program.valid;

		get_current_vertex_program(vs_sampler_state);

		m_graphics_state &= ~rsx::pipeline_state::invalidate_pipeline_bits;
	}

	auto &vertex_program = current_vertex_program;
	auto &fragment_program = current_fragment_program;
	auto old_program = m_program;

	vk::pipeline_props properties{};

	// Input assembly
	bool emulated_primitive_type;
	properties.state.set_primitive_type(vk::get_appropriate_topology(rsx::method_registers.current_draw_clause.primitive, emulated_primitive_type));

	const bool restarts_valid = rsx::method_registers.current_draw_clause.command == rsx::draw_command::indexed && !emulated_primitive_type && !rsx::method_registers.current_draw_clause.is_disjoint_primitive;
	if (rsx::method_registers.restart_index_enabled() && !vk::emulate_primitive_restart(rsx::method_registers.current_draw_clause.primitive) && restarts_valid)
		properties.state.enable_primitive_restart();

	// Rasterizer state
	properties.state.set_attachment_count((u32)m_draw_buffers.size());
	properties.state.set_front_face(vk::get_front_face(rsx::method_registers.front_face_mode()));
	properties.state.enable_depth_clamp(rsx::method_registers.depth_clamp_enabled() || !rsx::method_registers.depth_clip_enabled());
	properties.state.enable_depth_bias(true);
	properties.state.enable_depth_bounds_test(true);

	if (rsx::method_registers.depth_test_enabled())
	{
		//NOTE: Like stencil, depth write is meaningless without depth test
		properties.state.set_depth_mask(rsx::method_registers.depth_write_enabled());
		properties.state.enable_depth_test(vk::get_compare_func(rsx::method_registers.depth_func()));
	}

	if (rsx::method_registers.logic_op_enabled())
		properties.state.enable_logic_op(vk::get_logic_op(rsx::method_registers.logic_operation()));

	if (rsx::method_registers.cull_face_enabled())
		properties.state.enable_cull_face(vk::get_cull_face(rsx::method_registers.cull_face_mode()));

	bool color_mask_b = rsx::method_registers.color_mask_b();
	bool color_mask_g = rsx::method_registers.color_mask_g();
	bool color_mask_r = rsx::method_registers.color_mask_r();
	bool color_mask_a = rsx::method_registers.color_mask_a();

	if (rsx::method_registers.surface_color() == rsx::surface_color_format::g8b8)
		rsx::get_g8b8_r8g8_colormask(color_mask_r, color_mask_g, color_mask_b, color_mask_a);

	properties.state.set_color_mask(color_mask_r, color_mask_g, color_mask_b, color_mask_a);

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

		for (u8 idx = 0; idx < m_draw_buffers.size(); ++idx)
		{
			if (mrt_blend_enabled[idx])
			{
				properties.state.enable_blend(idx, sfactor_rgb, sfactor_a, dfactor_rgb, dfactor_a, equation_rgb, equation_a);
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
	}

	properties.renderpass_key = m_current_renderpass_key;
	properties.num_targets = (u32)m_draw_buffers.size();

	vk::enter_uninterruptible();

	//Load current program from buffer
	vertex_program.skip_vertex_input_check = true;
	fragment_program.unnormalized_coords = 0;
	m_program = m_prog_buffer->get_graphics_pipeline(vertex_program, fragment_program, properties,
			!g_cfg.video.disable_asynchronous_shader_compiler, *m_device, pipeline_layout).get();

	vk::leave_uninterruptible();

	if (m_prog_buffer->check_cache_missed())
	{
		if (m_prog_buffer->check_program_linked_flag())
		{
			// Program was linked or queued for linking
			m_shaders_cache->store(properties, vertex_program, fragment_program);
		}

		// Notify the user with HUD notification
		if (g_cfg.misc.show_shader_compilation_hint)
		{
			if (m_overlay_manager)
			{
				if (auto dlg = m_overlay_manager->get<rsx::overlays::shader_compile_notification>())
				{
					// Extend duration
					dlg->touch();
				}
				else
				{
					// Create dialog but do not show immediately
					m_overlay_manager->create<rsx::overlays::shader_compile_notification>();
				}
			}
		}
	}

	return m_program != nullptr;
}

void VKGSRender::load_program_env()
{
	if (!m_program)
	{
		fmt::throw_exception("Unreachable right now" HERE);
	}

	const u32 fragment_constants_size = current_fp_metadata.program_constants_buffer_length;

	const bool update_transform_constants = !!(m_graphics_state & rsx::pipeline_state::transform_constants_dirty);
	const bool update_fragment_constants = !!(m_graphics_state & rsx::pipeline_state::fragment_constants_dirty);
	const bool update_vertex_env = !!(m_graphics_state & rsx::pipeline_state::vertex_state_dirty);
	const bool update_fragment_env = !!(m_graphics_state & rsx::pipeline_state::fragment_state_dirty);
	const bool update_fragment_texture_env = !!(m_graphics_state & rsx::pipeline_state::fragment_texture_state_dirty);

	if (update_vertex_env)
	{
		check_heap_status(VK_HEAP_CHECK_VERTEX_ENV_STORAGE);

		// Vertex state
		const auto mem = m_vertex_env_ring_info.alloc<256>(256);
		auto buf = (u8*)m_vertex_env_ring_info.map(mem, 144);

		fill_scale_offset_data(buf, false);
		fill_user_clip_data(buf + 64);
		*(reinterpret_cast<u32*>(buf + 128)) = rsx::method_registers.transform_branch_bits();
		*(reinterpret_cast<f32*>(buf + 132)) = rsx::method_registers.point_size();
		*(reinterpret_cast<f32*>(buf + 136)) = rsx::method_registers.clip_min();
		*(reinterpret_cast<f32*>(buf + 140)) = rsx::method_registers.clip_max();

		m_vertex_env_ring_info.unmap();
		m_vertex_env_buffer_info = { m_vertex_env_ring_info.heap->value, mem, 144 };
	}

	if (update_transform_constants)
	{
		check_heap_status(VK_HEAP_CHECK_TRANSFORM_CONSTANTS_STORAGE);

		// Transform constants
		auto mem = m_transform_constants_ring_info.alloc<256>(8192);
		auto buf = m_transform_constants_ring_info.map(mem, 8192);

		fill_vertex_program_constants_data(buf);
		m_transform_constants_ring_info.unmap();
		m_vertex_constants_buffer_info = { m_transform_constants_ring_info.heap->value, mem, 8192 };
	}

	if (update_fragment_constants)
	{
		check_heap_status(VK_HEAP_CHECK_FRAGMENT_CONSTANTS_STORAGE);

		// Fragment constants
		if (fragment_constants_size)
		{
			auto mem = m_fragment_constants_ring_info.alloc<256>(fragment_constants_size);
			auto buf = m_fragment_constants_ring_info.map(mem, fragment_constants_size);

			m_prog_buffer->fill_fragment_constants_buffer({ reinterpret_cast<float*>(buf), ::narrow<int>(fragment_constants_size) },
				current_fragment_program, vk::sanitize_fp_values());

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
		check_heap_status(VK_HEAP_CHECK_FRAGMENT_ENV_STORAGE);

		auto mem = m_fragment_env_ring_info.alloc<256>(256);
		auto buf = m_fragment_env_ring_info.map(mem, 32);

		fill_fragment_state_buffer(buf, current_fragment_program);
		m_fragment_env_ring_info.unmap();
		m_fragment_env_buffer_info = { m_fragment_env_ring_info.heap->value, mem, 32 };
	}

	if (update_fragment_texture_env)
	{
		check_heap_status(VK_HEAP_CHECK_TEXTURE_ENV_STORAGE);

		auto mem = m_fragment_texture_params_ring_info.alloc<256>(256);
		auto buf = m_fragment_texture_params_ring_info.map(mem, 256);

		fill_fragment_texture_parameters(buf, current_fragment_program);
		m_fragment_texture_params_ring_info.unmap();
		m_fragment_texture_params_buffer_info = { m_fragment_texture_params_ring_info.heap->value, mem, 256 };
	}

	//if (1)
	{
		m_program->bind_uniform(m_vertex_env_buffer_info, VERTEX_PARAMS_BIND_SLOT, m_current_frame->descriptor_set);
		m_program->bind_uniform(m_vertex_constants_buffer_info, VERTEX_CONSTANT_BUFFERS_BIND_SLOT, m_current_frame->descriptor_set);
		m_program->bind_uniform(m_fragment_constants_buffer_info, FRAGMENT_CONSTANT_BUFFERS_BIND_SLOT, m_current_frame->descriptor_set);
		m_program->bind_uniform(m_fragment_env_buffer_info, FRAGMENT_STATE_BIND_SLOT, m_current_frame->descriptor_set);
		m_program->bind_uniform(m_fragment_texture_params_buffer_info, FRAGMENT_TEXTURE_PARAMS_BIND_SLOT, m_current_frame->descriptor_set);
	}

	//Clear flags
	const u32 handled_flags = (rsx::pipeline_state::fragment_state_dirty | rsx::pipeline_state::vertex_state_dirty | rsx::pipeline_state::transform_constants_dirty | rsx::pipeline_state::fragment_constants_dirty | rsx::pipeline_state::fragment_texture_state_dirty);
	m_graphics_state &= ~handled_flags;
}

void VKGSRender::update_vertex_env(const vk::vertex_upload_info& vertex_info)
{
	auto mem = m_vertex_layout_ring_info.alloc<256>(256);
	auto buf = (u32*)m_vertex_layout_ring_info.map(mem, 128 + 16);

	buf[0] = vertex_info.vertex_index_base;
	buf[1] = vertex_info.vertex_index_offset;
	buf += 4;

	fill_vertex_layout_state(m_vertex_layout, vertex_info.first_vertex, vertex_info.allocated_vertex_count, (s32*)buf,
		vertex_info.persistent_window_offset, vertex_info.volatile_window_offset);

	m_vertex_layout_ring_info.unmap();
	m_vertex_layout_buffer_info = { m_vertex_layout_ring_info.heap->value, mem, 128 + 16 };

	m_program->bind_uniform(m_vertex_layout_buffer_info, VERTEX_LAYOUT_BIND_SLOT, m_current_frame->descriptor_set);
}

void VKGSRender::init_buffers(rsx::framebuffer_creation_context context, bool skip_reading)
{
	prepare_rtts(context);

	if (!skip_reading)
	{
		read_buffers();
	}
}

void VKGSRender::read_buffers()
{
}

void VKGSRender::write_buffers()
{
}

void VKGSRender::close_and_submit_command_buffer(const std::vector<VkSemaphore> &semaphores, VkFence fence, VkPipelineStageFlags pipeline_stage_flags)
{
	if (m_attrib_ring_info.dirty() ||
		m_fragment_env_ring_info.dirty() ||
		m_vertex_env_ring_info.dirty() ||
		m_fragment_texture_params_ring_info.dirty() ||
		m_vertex_layout_ring_info.dirty() ||
		m_fragment_constants_ring_info.dirty() ||
		m_index_buffer_ring_info.dirty() ||
		m_transform_constants_ring_info.dirty() ||
		m_texture_upload_buffer_ring_info.dirty())
	{
		std::lock_guard lock(m_secondary_cb_guard);
		m_secondary_command_buffer.begin();

		m_attrib_ring_info.sync(m_secondary_command_buffer);
		m_fragment_env_ring_info.sync(m_secondary_command_buffer);
		m_vertex_env_ring_info.sync(m_secondary_command_buffer);
		m_fragment_texture_params_ring_info.sync(m_secondary_command_buffer);
		m_vertex_layout_ring_info.sync(m_secondary_command_buffer);
		m_fragment_constants_ring_info.sync(m_secondary_command_buffer);
		m_index_buffer_ring_info.sync(m_secondary_command_buffer);
		m_transform_constants_ring_info.sync(m_secondary_command_buffer);
		m_texture_upload_buffer_ring_info.sync(m_secondary_command_buffer);

		m_secondary_command_buffer.end();
		m_secondary_command_buffer.submit(m_swapchain->get_graphics_queue(), {}, VK_NULL_HANDLE, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
	}

	m_current_command_buffer->end();
	m_current_command_buffer->tag();
	m_current_command_buffer->submit(m_swapchain->get_graphics_queue(), semaphores, fence, pipeline_stage_flags);
}

void VKGSRender::open_command_buffer()
{
	m_current_command_buffer->begin();
}

void VKGSRender::prepare_rtts(rsx::framebuffer_creation_context context)
{
	if (m_current_framebuffer_context == context && !m_rtts_dirty && m_draw_fbo)
	{
		// Fast path
		// Framebuffer usage has not changed, framebuffer exists and config regs have not changed
		set_scissor();
		return;
	}

	m_rtts_dirty = false;
	framebuffer_status_valid = false;
	m_framebuffer_state_contested = false;

	const auto layout = get_framebuffer_layout(context);
	if (!framebuffer_status_valid)
	{
		return;
	}

	if (m_draw_fbo && layout.ignore_change)
	{
		// Nothing has changed, we're still using the same framebuffer
		// Update flags to match current

		const auto aa_mode = rsx::method_registers.surface_antialias();

		for (u32 index = 0; index < 4; index++)
		{
			if (auto surface = std::get<1>(m_rtts.m_bound_render_targets[index]))
			{
				surface->write_aa_mode = layout.aa_mode;
			}
		}

		if (auto ds = std::get<1>(m_rtts.m_bound_depth_stencil))
		{
			ds->write_aa_mode = layout.aa_mode;
		}

		return;
	}

	m_rtts.prepare_render_target(*m_current_command_buffer,
		layout.color_format, layout.depth_format,
		layout.width, layout.height,
		layout.target, layout.aa_mode,
		layout.color_addresses, layout.zeta_address,
		layout.actual_color_pitch, layout.actual_zeta_pitch,
		(*m_device), *m_current_command_buffer);

	// Reset framebuffer information
	VkFormat old_format = VK_FORMAT_UNDEFINED;
	const auto color_bpp = get_format_block_size_in_bytes(layout.color_format);

	for (u8 i = 0; i < rsx::limits::color_buffers_count; ++i)
	{
		// Flush old address if we keep missing it
		if (m_surface_info[i].pitch && g_cfg.video.write_color_buffers)
		{
			if (old_format == VK_FORMAT_UNDEFINED)
				old_format = vk::get_compatible_surface_format(m_surface_info[i].color_format).first;

			const utils::address_range rsx_range = m_surface_info[i].get_memory_range();
			m_texture_cache.set_memory_read_flags(rsx_range, rsx::memory_read_flags::flush_once);
			m_texture_cache.flush_if_cache_miss_likely(*m_current_command_buffer, rsx_range);
		}

		m_surface_info[i].address = m_surface_info[i].pitch = 0;
		m_surface_info[i].width = layout.width;
		m_surface_info[i].height = layout.height;
		m_surface_info[i].color_format = layout.color_format;
		m_surface_info[i].bpp = color_bpp;
	}

	//Process depth surface as well
	{
		if (m_depth_surface_info.pitch && g_cfg.video.write_depth_buffer)
		{
			auto old_format = vk::get_compatible_depth_surface_format(m_device->get_formats_support(), m_depth_surface_info.depth_format);
			const utils::address_range surface_range = m_depth_surface_info.get_memory_range();
			m_texture_cache.set_memory_read_flags(surface_range, rsx::memory_read_flags::flush_once);
			m_texture_cache.flush_if_cache_miss_likely(*m_current_command_buffer, surface_range);
		}

		m_depth_surface_info.address = m_depth_surface_info.pitch = 0;
		m_depth_surface_info.width = layout.width;
		m_depth_surface_info.height = layout.height;
		m_depth_surface_info.depth_format = layout.depth_format;
		m_depth_surface_info.bpp = (layout.depth_format == rsx::surface_depth_format::z16? 2 : 4);
	}

	//Bind created rtts as current fbo...
	const auto draw_buffers = rsx::utility::get_rtt_indexes(layout.target);
	m_draw_buffers.clear();
	m_fbo_images.clear();

	for (u8 index : draw_buffers)
	{
		if (auto surface = std::get<1>(m_rtts.m_bound_render_targets[index]))
		{
			m_fbo_images.push_back(surface);

			m_surface_info[index].address = layout.color_addresses[index];
			m_surface_info[index].pitch = layout.actual_color_pitch[index];
			verify("Pitch mismatch!" HERE), surface->rsx_pitch == layout.actual_color_pitch[index];

			surface->write_aa_mode = layout.aa_mode;
			m_texture_cache.notify_surface_changed(m_surface_info[index].get_memory_range(layout.aa_factors));
			m_draw_buffers.push_back(index);
		}
	}

	if (std::get<0>(m_rtts.m_bound_depth_stencil) != 0)
	{
		auto ds = std::get<1>(m_rtts.m_bound_depth_stencil);
		m_fbo_images.push_back(ds);

		m_depth_surface_info.address = layout.zeta_address;
		m_depth_surface_info.pitch = layout.actual_zeta_pitch;
		verify("Pitch mismatch!" HERE), ds->rsx_pitch == layout.actual_zeta_pitch;

		ds->write_aa_mode = layout.aa_mode;
		m_texture_cache.notify_surface_changed(m_depth_surface_info.get_memory_range(layout.aa_factors));
	}

	// Before messing with memory properties, flush command queue if there are dma transfers queued up
	if (m_current_command_buffer->flags & vk::command_buffer::cb_has_dma_transfer)
	{
		flush_command_queue();
	}

	const auto color_fmt_info = vk::get_compatible_gcm_format(layout.color_format);
	for (u8 index : m_draw_buffers)
	{
		if (!m_surface_info[index].address || !m_surface_info[index].pitch) continue;

		const utils::address_range surface_range = m_surface_info[index].get_memory_range();
		if (g_cfg.video.write_color_buffers)
		{
			m_texture_cache.lock_memory_region(
				*m_current_command_buffer, m_rtts.m_bound_render_targets[index].second, surface_range, true,
				m_surface_info[index].width, m_surface_info[index].height, layout.actual_color_pitch[index],
				color_fmt_info.first, color_fmt_info.second);
		}
		else
		{
			m_texture_cache.commit_framebuffer_memory_region(*m_current_command_buffer, surface_range);
		}
	}

	if (m_depth_surface_info.address && m_depth_surface_info.pitch)
	{
		const utils::address_range surface_range = m_depth_surface_info.get_memory_range();
		if (g_cfg.video.write_depth_buffer)
		{
			const u32 gcm_format = (m_depth_surface_info.depth_format != rsx::surface_depth_format::z16) ? CELL_GCM_TEXTURE_DEPTH16 : CELL_GCM_TEXTURE_DEPTH24_D8;
			m_texture_cache.lock_memory_region(
				*m_current_command_buffer, m_rtts.m_bound_depth_stencil.second, surface_range, true,
				m_depth_surface_info.width, m_depth_surface_info.height, layout.actual_zeta_pitch, gcm_format, false);
		}
		else
		{
			m_texture_cache.commit_framebuffer_memory_region(*m_current_command_buffer, surface_range);
		}
	}

	if (!m_rtts.orphaned_surfaces.empty())
	{
		if (g_cfg.video.write_color_buffers || g_cfg.video.write_depth_buffer)
		{
			u32 gcm_format;
			bool swap_bytes;

			for (auto& surface : m_rtts.orphaned_surfaces)
			{
				if (surface->is_depth_surface())
				{
					if (!g_cfg.video.write_depth_buffer) continue;

					gcm_format = (surface->get_surface_depth_format() != rsx::surface_depth_format::z16) ? CELL_GCM_TEXTURE_DEPTH16 : CELL_GCM_TEXTURE_DEPTH24_D8;
					swap_bytes = true;
				}
				else
				{
					if (!g_cfg.video.write_color_buffers) continue;

					auto info = vk::get_compatible_gcm_format(surface->get_surface_color_format());
					gcm_format = info.first;
					swap_bytes = info.second;
				}

				m_texture_cache.lock_memory_region(
					*m_current_command_buffer, surface, surface->get_memory_range(), false,
					surface->get_surface_width(), surface->get_surface_height(), surface->get_rsx_pitch(),
					gcm_format, swap_bytes);
			}
		}

		m_rtts.orphaned_surfaces.clear();
	}

	m_current_renderpass_key = vk::get_renderpass_key(m_fbo_images);
	m_cached_renderpass = vk::get_renderpass(*m_device, m_current_renderpass_key);

	// Search old framebuffers for this same configuration
	const auto fbo_width = rsx::apply_resolution_scale(layout.width, true);
	const auto fbo_height = rsx::apply_resolution_scale(layout.height, true);

	if (m_draw_fbo)
	{
		// Release old ref
		m_draw_fbo->release();
	}

	m_draw_fbo = vk::get_framebuffer(*m_device, fbo_width, fbo_height, m_cached_renderpass, m_fbo_images);
	m_draw_fbo->add_ref();

	set_viewport();
	set_scissor();

	check_zcull_status(true);
}

void VKGSRender::reinitialize_swapchain()
{
	const auto new_width = m_frame->client_width();
	const auto new_height = m_frame->client_height();

	//Reject requests to acquire new swapchain if the window is minimized
	//The NVIDIA driver will spam VK_ERROR_OUT_OF_DATE_KHR if you try to acquire an image from the swapchain and the window is minimized
	//However, any attempt to actually renew the swapchain will crash the driver with VK_ERROR_DEVICE_LOST while the window is in this state
	if (new_width == 0 || new_height == 0)
		return;

	/**
	* Waiting for the commands to process does not work reliably as the fence can be signaled before swap images are released
	* and there are no explicit methods to ensure that the presentation engine is not using the images at all.
	*/

	//NOTE: This operation will create a hard sync point
	close_and_submit_command_buffer({}, m_current_command_buffer->submit_fence);
	m_current_command_buffer->pending = true;
	m_current_command_buffer->reset();

	for (auto &ctx : frame_context_storage)
	{
		if (ctx.present_image == UINT32_MAX)
			continue;

		// Release present image by presenting it
		ctx.swap_command_buffer->wait(FRAME_PRESENT_TIMEOUT);
		ctx.swap_command_buffer = nullptr;
		present(&ctx);
	}

	// Remove any old refs to the old images as they are about to be destroyed
	//m_framebuffers_to_clean;

	// Rebuild swapchain. Old swapchain destruction is handled by the init_swapchain call
	if (!m_swapchain->init(new_width, new_height))
	{
		LOG_WARNING(RSX, "Swapchain initialization failed. Request ignored [%dx%d]", new_width, new_height);
		present_surface_dirty_flag = true;
		renderer_unavailable = true;
		open_command_buffer();
		return;
	}

	m_client_width = new_width;
	m_client_height = new_height;

	//Prepare new swapchain images for use
	open_command_buffer();

	for (u32 i = 0; i < m_swapchain->get_swap_image_count(); ++i)
	{
		const auto target_layout = m_swapchain->get_optimal_present_layout();
		const auto target_image = m_swapchain->get_image(i);
		VkClearColorValue clear_color{};
		VkImageSubresourceRange range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

		vk::change_image_layout(*m_current_command_buffer, target_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range);
		vkCmdClearColorImage(*m_current_command_buffer, target_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color, 1, &range);
		vk::change_image_layout(*m_current_command_buffer, target_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, target_layout, range);
	}

	//Will have to block until rendering is completed
	VkFence resize_fence = VK_NULL_HANDLE;
	VkFenceCreateInfo infos = {};
	infos.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	vkCreateFence((*m_device), &infos, nullptr, &resize_fence);

	//Flush the command buffer
	close_and_submit_command_buffer({}, resize_fence);
	vk::wait_for_fence(resize_fence);
	vkDestroyFence((*m_device), resize_fence, nullptr);

	m_current_command_buffer->reset();
	open_command_buffer();

	present_surface_dirty_flag = false;
	renderer_unavailable = false;
}

void VKGSRender::flip(int buffer, bool emu_flip)
{
	if (skip_frame || renderer_unavailable)
	{
		m_frame->flip(m_context);
		rsx::thread::flip(buffer, emu_flip);

		if (!skip_frame)
		{
			m_draw_time = 0;
			m_setup_time = 0;
			m_vertex_upload_time = 0;
			m_textures_upload_time = 0;
		}

		return;
	}

	std::chrono::time_point<steady_clock> flip_start = steady_clock::now();

	if (m_current_frame == &m_aux_frame_context)
	{
		m_current_frame = &frame_context_storage[m_current_queue_index];
		if (m_current_frame->swap_command_buffer)
		{
			// Always present if pending swap is present.
			// Its possible this flip request is triggered by overlays and the flip queue is in undefined state
			process_swap_request(m_current_frame, true);
		}

		//swap aux storage and current frame; aux storage should always be ready for use at all times
		m_current_frame->swap_storage(m_aux_frame_context);
		m_current_frame->grab_resources(m_aux_frame_context);
	}
	else if (m_current_frame->swap_command_buffer)
	{
		if (m_draw_calls > 0)
		{
			// This can be 'legal' if the window was being resized and no polling happened because of renderer_unavailable flag
			LOG_ERROR(RSX, "Possible data corruption on frame context storage detected");
		}

		//There were no draws and back-to-back flips happened
		process_swap_request(m_current_frame, true);
	}

	if (present_surface_dirty_flag)
	{
		//Recreate swapchain and continue as usual
		reinitialize_swapchain();
	}

	if (renderer_unavailable)
		return;

	u32 buffer_width = display_buffers[buffer].width;
	u32 buffer_height = display_buffers[buffer].height;
	u32 buffer_pitch = display_buffers[buffer].pitch;

	u32 av_format;
	const auto avconfig = fxm::get<rsx::avconf>();

	if (avconfig)
	{
		av_format = avconfig->get_compatible_gcm_format();
		if (!buffer_pitch) buffer_pitch = buffer_width * avconfig->get_bpp();

		buffer_width = std::min(buffer_width, avconfig->resolution_x);
		buffer_height = std::min(buffer_height, avconfig->resolution_y);
	}
	else
	{
		av_format = CELL_GCM_TEXTURE_A8R8G8B8;
		if (!buffer_pitch) buffer_pitch = buffer_width * 4;
	}

	coordi aspect_ratio;

	sizei csize = { (s32)m_client_width, (s32)m_client_height };
	sizei new_size = csize;

	if (!g_cfg.video.stretch_to_display_area)
	{
		const double aq = (double)buffer_width / buffer_height;
		const double rq = (double)new_size.width / new_size.height;
		const double q = aq / rq;

		if (q > 1.0)
		{
			new_size.height = int(new_size.height / q);
			aspect_ratio.y = (csize.height - new_size.height) / 2;
		}
		else if (q < 1.0)
		{
			new_size.width = int(new_size.width * q);
			aspect_ratio.x = (csize.width - new_size.width) / 2;
		}
	}

	aspect_ratio.size = new_size;

	//Prepare surface for new frame. Set no timeout here so that we wait for the next image if need be
	verify(HERE), m_current_frame->present_image == UINT32_MAX;
	verify(HERE), m_current_frame->swap_command_buffer == nullptr;

	u64 timeout = m_swapchain->get_swap_image_count() <= VK_MAX_ASYNC_FRAMES? 0ull: 100000000ull;
	while (VkResult status = m_swapchain->acquire_next_swapchain_image(m_current_frame->present_semaphore, timeout, &m_current_frame->present_image))
	{
		switch (status)
		{
		case VK_TIMEOUT:
		case VK_NOT_READY:
		{
			//In some cases, after a fullscreen switch, the driver only allows N-1 images to be acquirable, where N = number of available swap images.
			//This means that any acquired images have to be released
			//before acquireNextImage can return successfully. This is despite the driver reporting 2 swap chain images available
			//This makes fullscreen performance slower than windowed performance as throughput is lowered due to losing one presentable image
			//Found on AMD Crimson 17.7.2

			//Whatever returned from status, this is now a spin
			timeout = 0ull;
			for (auto &ctx : frame_context_storage)
			{
				if (ctx.swap_command_buffer)
				{
					ctx.swap_command_buffer->poke();
					if (!ctx.swap_command_buffer->pending)
					{
						//Release in case there is competition for frame resources
						process_swap_request(&ctx, true);
					}
				}
			}

			continue;
		}
		case VK_ERROR_OUT_OF_DATE_KHR:
			LOG_WARNING(RSX, "vkAcquireNextImageKHR failed with VK_ERROR_OUT_OF_DATE_KHR. Flip request ignored until surface is recreated.");
			present_surface_dirty_flag = true;
			reinitialize_swapchain();
			return;
		default:
			vk::die_with_error(HERE, status);
		}
	}

	//Confirm that the driver did not silently fail
	verify(HERE), m_current_frame->present_image != UINT32_MAX;

	//Blit contents to screen..
	vk::image* image_to_flip = nullptr;

	if ((u32)buffer < display_buffers_count && buffer_width && buffer_height)
	{
		const u32 absolute_address = rsx::get_address(display_buffers[buffer].offset, CELL_GCM_LOCATION_LOCAL);

		if (auto render_target_texture = m_rtts.get_texture_from_render_target_if_applicable(absolute_address))
		{
			if (render_target_texture->last_use_tag == m_rtts.write_tag)
			{
				image_to_flip = render_target_texture;
			}
			else
			{
				const auto overlap_info = m_rtts.get_merged_texture_memory_region(*m_current_command_buffer, absolute_address, buffer_width, buffer_height, buffer_pitch, render_target_texture->get_bpp());
				if (!overlap_info.empty() && overlap_info.back().surface == render_target_texture)
				{
					// Confirmed to be the newest data source in that range
					image_to_flip = render_target_texture;
				}
			}

			if (image_to_flip)
			{
				buffer_width = rsx::apply_resolution_scale(buffer_width, true);
				buffer_height = rsx::apply_resolution_scale(buffer_height, true);

				if (buffer_width > render_target_texture->width() ||
					buffer_height > render_target_texture->height())
				{
					// TODO: Should emit only once to avoid flooding the log file
					// TODO: Take AA scaling into account
					LOG_WARNING(RSX, "Selected output image does not satisfy the video configuration. Display buffer resolution=%dx%d, avconf resolution=%dx%d, surface=%dx%d",
						display_buffers[buffer].width, display_buffers[buffer].height, avconfig? avconfig->resolution_x : 0, avconfig? avconfig->resolution_y : 0,
						render_target_texture->get_surface_width(), render_target_texture->get_surface_height());

					buffer_width = render_target_texture->width();
					buffer_height = render_target_texture->height();
				}
			}
		}
		else if (auto surface = m_texture_cache.find_texture_from_dimensions<true>(absolute_address, av_format, buffer_width, buffer_height))
		{
			//Hack - this should be the first location to check for output
			//The render might have been done offscreen or in software and a blit used to display
			image_to_flip = surface->get_raw_texture();
		}

		if (!image_to_flip)
		{
			// Read from cell
			const auto range = utils::address_range::start_length(absolute_address, buffer_pitch * buffer_height);
			const u32  lookup_mask = rsx::texture_upload_context::blit_engine_dst | rsx::texture_upload_context::framebuffer_storage;
			const auto overlap = m_texture_cache.find_texture_from_range<true>(range, 0, lookup_mask);

			for (const auto & section : overlap)
			{
				if (!section->is_synchronized())
				{
					section->copy_texture(*m_current_command_buffer, true);
				}
			}

			if (m_current_command_buffer->flags & vk::command_buffer::cb_has_dma_transfer)
			{
				// Submit for processing to lower hard fault penalty
				flush_command_queue();
			}

			m_texture_cache.invalidate_range(*m_current_command_buffer, range, rsx::invalidation_cause::read);
			image_to_flip = m_texture_cache.upload_image_simple(*m_current_command_buffer, absolute_address, buffer_width, buffer_height);
		}
	}

	VkImage target_image = m_swapchain->get_image(m_current_frame->present_image);
	const auto present_layout = m_swapchain->get_optimal_present_layout();

	if (image_to_flip)
	{
		VkImageLayout target_layout = present_layout;
		VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		if (aspect_ratio.x || aspect_ratio.y)
		{
			VkClearColorValue clear_black {};
			vk::change_image_layout(*m_current_command_buffer, target_image, present_layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range);
			vkCmdClearColorImage(*m_current_command_buffer, target_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_black, 1, &range);

			target_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		}

		vk::copy_scaled_image(*m_current_command_buffer, image_to_flip->value, target_image, image_to_flip->current_layout, target_layout,
			{ 0, 0, (s32)buffer_width, (s32)buffer_height }, aspect_ratio, 1, VK_IMAGE_ASPECT_COLOR_BIT, false);

		if (target_layout != present_layout)
		{
			vk::change_image_layout(*m_current_command_buffer, target_image, target_layout, present_layout, range);
		}
	}
	else
	{
		//No draw call was issued!
		//TODO: Upload raw bytes from cpu for rendering
		VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		VkClearColorValue clear_black {};
		vk::change_image_layout(*m_current_command_buffer, target_image, present_layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range);
		vkCmdClearColorImage(*m_current_command_buffer, target_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_black, 1, &range);
		vk::change_image_layout(*m_current_command_buffer, target_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, present_layout, range);
	}

	const bool has_overlay = (m_overlay_manager && m_overlay_manager->has_visible());
	if (g_cfg.video.overlay || has_overlay)
	{
		//Change the image layout whilst setting up a dependency on waiting for the blit op to finish before we start writing
		VkImageSubresourceRange subres = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		barrier.oldLayout = present_layout;
		barrier.image = target_image;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange = subres;

		vkCmdPipelineBarrier(*m_current_command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &barrier);

		auto key = vk::get_renderpass_key(m_swapchain->get_surface_format());
		VkRenderPass single_target_pass = vk::get_renderpass(*m_device, key);
		verify("Usupported renderpass configuration" HERE), single_target_pass != VK_NULL_HANDLE;

		auto direct_fbo = vk::get_framebuffer(*m_device, m_client_width, m_client_height, single_target_pass, m_swapchain->get_surface_format(), target_image);
		direct_fbo->add_ref();

		if (has_overlay)
		{
			// Lock to avoid modification during run-update chain
			std::lock_guard lock(*m_overlay_manager);

			for (const auto& view : m_overlay_manager->get_views())
			{
				m_ui_renderer->run(*m_current_command_buffer, direct_fbo->width(), direct_fbo->height(), direct_fbo, single_target_pass, m_texture_upload_buffer_ring_info, *view.get());
			}
		}

		if (g_cfg.video.overlay)
		{
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0,   0, direct_fbo->width(), direct_fbo->height(), fmt::format("RSX Load:                 %3d%%", get_load()));
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0,  18, direct_fbo->width(), direct_fbo->height(), fmt::format("draw calls: %17d", m_draw_calls));
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0,  36, direct_fbo->width(), direct_fbo->height(), fmt::format("draw call setup: %12dus", m_setup_time));
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0,  54, direct_fbo->width(), direct_fbo->height(), fmt::format("vertex upload time: %9dus", m_vertex_upload_time));
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0,  72, direct_fbo->width(), direct_fbo->height(), fmt::format("texture upload time: %8dus", m_textures_upload_time));
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0,  90, direct_fbo->width(), direct_fbo->height(), fmt::format("draw call execution: %8dus", m_draw_time));
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0, 108, direct_fbo->width(), direct_fbo->height(), fmt::format("submit and flip: %12dus", m_flip_time));

			const auto num_dirty_textures = m_texture_cache.get_unreleased_textures_count();
			const auto texture_memory_size = m_texture_cache.get_texture_memory_in_use() / (1024 * 1024);
			const auto tmp_texture_memory_size = m_texture_cache.get_temporary_memory_in_use() / (1024 * 1024);
			const auto num_flushes = m_texture_cache.get_num_flush_requests();
			const auto num_mispredict = m_texture_cache.get_num_cache_mispredictions();
			const auto num_speculate = m_texture_cache.get_num_cache_speculative_writes();
			const auto num_misses = m_texture_cache.get_num_cache_misses();
			const auto num_unavoidable = m_texture_cache.get_num_unavoidable_hard_faults();
			const auto cache_miss_ratio = (u32)ceil(m_texture_cache.get_cache_miss_ratio() * 100);
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0, 144, direct_fbo->width(), direct_fbo->height(), fmt::format("Unreleased textures: %8d", num_dirty_textures));
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0, 162, direct_fbo->width(), direct_fbo->height(), fmt::format("Texture cache memory: %7dM", texture_memory_size));
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0, 180, direct_fbo->width(), direct_fbo->height(), fmt::format("Temporary texture memory: %3dM", tmp_texture_memory_size));
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0, 198, direct_fbo->width(), direct_fbo->height(), fmt::format("Flush requests: %13d  = %2d (%3d%%) hard faults, %2d unavoidable, %2d misprediction(s), %2d speculation(s)", num_flushes, num_misses, cache_miss_ratio, num_unavoidable, num_mispredict, num_speculate));
		}

		vk::change_image_layout(*m_current_command_buffer, target_image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, present_layout, subres);

		direct_fbo->release();
	}

	queue_swap_request();

	std::chrono::time_point<steady_clock> flip_end = steady_clock::now();
	m_flip_time = std::chrono::duration_cast<std::chrono::microseconds>(flip_end - flip_start).count();

	//NOTE:Resource destruction is handled within the real swap handler

	m_frame->flip(m_context);
	rsx::thread::flip(buffer, emu_flip);

	//Do not reset perf counters if we are skipping the next frame
	if (skip_frame) return;

	m_draw_time = 0;
	m_setup_time = 0;
	m_vertex_upload_time = 0;
	m_textures_upload_time = 0;
}

bool VKGSRender::scaled_image_from_memory(rsx::blit_src_info& src, rsx::blit_dst_info& dst, bool interpolate)
{
	if (renderer_unavailable)
		return false;

	// Verify enough memory exists before attempting to handle data transfer
	check_heap_status(VK_HEAP_CHECK_TEXTURE_UPLOAD_STORAGE);

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
	query->result = 0;
	//query->sync_timestamp = get_system_time();
	m_active_query_info = query;
	m_occlusion_query_active = true;
}

void VKGSRender::end_occlusion_query(rsx::reports::occlusion_query_info* query)
{
	m_occlusion_query_active = false;
	m_active_query_info = nullptr;

	// NOTE: flushing the queue is very expensive, do not flush just because query stopped
}

bool VKGSRender::check_occlusion_query_status(rsx::reports::occlusion_query_info* query)
{
	if (!query->num_draws)
		return true;

	auto found = m_occlusion_map.find(query->driver_handle);
	if (found == m_occlusion_map.end())
		return true;

	auto &data = found->second;
	if (data.indices.empty())
		return true;

	if (data.command_buffer_to_wait == m_current_command_buffer)
	{
		if (!m_flush_requests.pending())
		{
			//Likely to be read at some point in the near future, submit now to avoid stalling later
			m_flush_requests.post(false);
			m_flush_requests.remove_one();
		}

		return false;
	}

	if (data.command_buffer_to_wait->pending)
		//Don't bother poking the state, a flush later will likely do it for free
		return false;

	u32 oldest = data.indices.front();
	return m_occlusion_query_pool.check_query_status(oldest);
}

void VKGSRender::get_occlusion_query_result(rsx::reports::occlusion_query_info* query)
{
	auto found = m_occlusion_map.find(query->driver_handle);
	if (found == m_occlusion_map.end())
		return;

	auto &data = found->second;
	if (data.indices.empty())
		return;

	if (query->num_draws)
	{
		if (data.command_buffer_to_wait == m_current_command_buffer)
		{
			std::lock_guard lock(m_flush_queue_mutex);
			flush_command_queue();

			//Clear any deferred flush requests from previous call to get_query_status()
			if (m_flush_requests.pending())
			{
				m_flush_requests.clear_pending_flag();
				m_flush_requests.consumer_wait();
			}
		}

		if (data.command_buffer_to_wait->pending)
			data.command_buffer_to_wait->wait(GENERAL_WAIT_TIMEOUT);

		//Gather data
		for (const auto occlusion_id : data.indices)
		{
			//We only need one hit
			if (auto value = m_occlusion_query_pool.get_query_result(occlusion_id))
			{
				query->result = 1;
				break;
			}
		}
	}

	m_occlusion_query_pool.reset_queries(*m_current_command_buffer, data.indices);
	m_occlusion_map.erase(query->driver_handle);
}

void VKGSRender::discard_occlusion_query(rsx::reports::occlusion_query_info* query)
{
	if (m_active_query_info == query)
	{
		m_occlusion_query_active = false;
		m_active_query_info = nullptr;
	}

	auto found = m_occlusion_map.find(query->driver_handle);
	if (found == m_occlusion_map.end())
		return;

	auto &data = found->second;
	if (data.indices.empty())
		return;

	m_occlusion_query_pool.reset_queries(*m_current_command_buffer, data.indices);
	m_occlusion_map.erase(query->driver_handle);
}

bool VKGSRender::on_decompiler_task()
{
	return m_prog_buffer->async_update(8, *m_device, pipeline_layout).first;
}
