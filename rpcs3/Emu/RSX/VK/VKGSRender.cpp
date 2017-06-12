#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "VKGSRender.h"
#include "../rsx_methods.h"
#include "../rsx_utils.h"
#include "../Common/BufferUtils.h"
#include "VKFormats.h"

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
	VkCompareOp get_compare_func(rsx::comparison_function op)
	{
		switch (op)
		{
		case rsx::comparison_function::never: return VK_COMPARE_OP_NEVER;
		case rsx::comparison_function::greater: return VK_COMPARE_OP_GREATER;
		case rsx::comparison_function::less: return VK_COMPARE_OP_LESS;
		case rsx::comparison_function::less_or_equal: return VK_COMPARE_OP_LESS_OR_EQUAL;
		case rsx::comparison_function::greater_or_equal: return VK_COMPARE_OP_GREATER_OR_EQUAL;
		case rsx::comparison_function::equal: return VK_COMPARE_OP_EQUAL;
		case rsx::comparison_function::not_equal: return VK_COMPARE_OP_NOT_EQUAL;
		case rsx::comparison_function::always: return VK_COMPARE_OP_ALWAYS;
		default:
			fmt::throw_exception("Unknown compare op: 0x%x" HERE, (u32)op);
		}
	}

	std::pair<VkFormat, VkComponentMapping> get_compatible_surface_format(rsx::surface_color_format color_format)
	{
		switch (color_format)
		{
		case rsx::surface_color_format::r5g6b5:
			return std::make_pair(VK_FORMAT_R5G6B5_UNORM_PACK16, vk::default_component_map());

		case rsx::surface_color_format::a8r8g8b8:
		case rsx::surface_color_format::a8b8g8r8:
			return std::make_pair(VK_FORMAT_B8G8R8A8_UNORM, vk::default_component_map());

		case rsx::surface_color_format::x8b8g8r8_o8b8g8r8:
		case rsx::surface_color_format::x8b8g8r8_z8b8g8r8:
		case rsx::surface_color_format::x8r8g8b8_z8r8g8b8:
		case rsx::surface_color_format::x8r8g8b8_o8r8g8b8:
		{
			VkComponentMapping no_alpha = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_ONE };
			return std::make_pair(VK_FORMAT_B8G8R8A8_UNORM, no_alpha);
		}

		case rsx::surface_color_format::w16z16y16x16:
			return std::make_pair(VK_FORMAT_R16G16B16A16_SFLOAT, vk::default_component_map());

		case rsx::surface_color_format::w32z32y32x32:
			return std::make_pair(VK_FORMAT_R32G32B32A32_SFLOAT, vk::default_component_map());

		case rsx::surface_color_format::x1r5g5b5_o1r5g5b5:
		case rsx::surface_color_format::x1r5g5b5_z1r5g5b5:
		{
			VkComponentMapping no_alpha = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_ONE };
			return std::make_pair(VK_FORMAT_B8G8R8A8_UNORM, no_alpha);
		}

		case rsx::surface_color_format::b8:
			return std::make_pair(VK_FORMAT_R8_UNORM, vk::default_component_map());
		
		case rsx::surface_color_format::g8b8:
			return std::make_pair(VK_FORMAT_R8G8_UNORM, vk::default_component_map());

		case rsx::surface_color_format::x32:
			return std::make_pair(VK_FORMAT_R32_SFLOAT, vk::default_component_map());

		default:
			LOG_ERROR(RSX, "Surface color buffer: Unsupported surface color format (0x%x)", (u32)color_format);
			return std::make_pair(VK_FORMAT_B8G8R8A8_UNORM, vk::default_component_map());
		}
	}

	/** Maps color_format, depth_stencil_format and color count to an int as below :
	 * idx = color_count + 5 * depth_stencil_idx + 15 * color_format_idx
	 * This should perform a 1:1 mapping
	 */

	size_t get_render_pass_location(VkFormat color_format, VkFormat depth_stencil_format, u8 color_count)
	{
		size_t color_format_idx = 0;
		size_t depth_format_idx = 0;

		verify(HERE), color_count < 5;

		switch (color_format)
		{
		case VK_FORMAT_R5G6B5_UNORM_PACK16:
			color_format_idx = 0;
			break;
		case VK_FORMAT_B8G8R8A8_UNORM:
			color_format_idx = 1;
			break;
		case VK_FORMAT_R16G16B16A16_SFLOAT:
			color_format_idx = 2;
			break;
		case VK_FORMAT_R32G32B32A32_SFLOAT:
			color_format_idx = 3;
			break;
		case VK_FORMAT_R8_UINT:
			color_format_idx = 4;
			break;
		case VK_FORMAT_R8G8_UINT:
			color_format_idx = 5;
			break;
		case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
			color_format_idx = 6;
			break;
		case VK_FORMAT_R32_SFLOAT:
			color_format_idx = 7;
			break;
		}

		switch (depth_stencil_format)
		{
		case VK_FORMAT_D16_UNORM:
			depth_format_idx = 0;
			break;
		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			depth_format_idx = 1;
			break;
		case VK_FORMAT_UNDEFINED:
			depth_format_idx = 2;
			break;
		}

		return color_count + 5 * depth_format_idx + 5 * 3 * color_format_idx;
	}

	std::vector<u8> get_draw_buffers(rsx::surface_target fmt)
	{
		switch (fmt)
		{
		case rsx::surface_target::none:
			return{};
		case rsx::surface_target::surface_a:
			return{ 0 };
		case rsx::surface_target::surface_b:
			return{ 1 };
		case rsx::surface_target::surfaces_a_b:
			return{ 0, 1 };
		case rsx::surface_target::surfaces_a_b_c:
			return{ 0, 1, 2 };
		case rsx::surface_target::surfaces_a_b_c_d:
			return{ 0, 1, 2, 3 };
		default:
			LOG_ERROR(RSX, "Bad surface color target: %d", (u32)fmt);
			return{};
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
	};

	VkBlendOp get_blend_op(rsx::blend_equation op)
	{
		switch (op)
		{
		case rsx::blend_equation::add: return VK_BLEND_OP_ADD;
		case rsx::blend_equation::substract: return VK_BLEND_OP_SUBTRACT;
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
	VkRenderPass precompute_render_pass(VkDevice dev, VkFormat color_format, u8 number_of_color_surface, VkFormat depth_format)
	{
		// Some driver crashes when using empty render pass
		if (number_of_color_surface == 0 && depth_format == VK_FORMAT_UNDEFINED)
			return nullptr;
		/* Describe a render pass and framebuffer attachments */
		std::vector<VkAttachmentDescription> attachments = {};
		std::vector<VkAttachmentReference> attachment_references;

		VkAttachmentDescription color_attachement_description = {};
		color_attachement_description.format = color_format;
		color_attachement_description.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachement_description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		color_attachement_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachement_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachement_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachement_description.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		color_attachement_description.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		for (u32 i = 0; i < number_of_color_surface; ++i)
		{
			attachments.push_back(color_attachement_description);
			attachment_references.push_back({ i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
		}

		if (depth_format != VK_FORMAT_UNDEFINED)
		{
			VkAttachmentDescription depth_attachement_description = {};
			depth_attachement_description.format = depth_format;
			depth_attachement_description.samples = VK_SAMPLE_COUNT_1_BIT;
			depth_attachement_description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			depth_attachement_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			depth_attachement_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			depth_attachement_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
			depth_attachement_description.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			depth_attachement_description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			attachments.push_back(depth_attachement_description);

			attachment_references.push_back({ number_of_color_surface, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL });
		}

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = number_of_color_surface;
		subpass.pColorAttachments = number_of_color_surface > 0 ? attachment_references.data() : nullptr;
		subpass.pDepthStencilAttachment = depth_format != VK_FORMAT_UNDEFINED ? &attachment_references.back() : nullptr;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstSubpass = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo rp_info = {};
		rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		rp_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		rp_info.pAttachments = attachments.data();
		rp_info.subpassCount = 1;
		rp_info.pSubpasses = &subpass;
		rp_info.pDependencies = &dependency;
		rp_info.dependencyCount = 1;

		VkRenderPass result;
		CHECK_RESULT(vkCreateRenderPass(dev, &rp_info, NULL, &result));
		return result;
	}

	std::array<VkRenderPass, 120> get_precomputed_render_passes(VkDevice dev, const vk::gpu_formats_support &gpu_format_support)
	{
		std::array<VkRenderPass, 120> result = {};

		const std::array<VkFormat, 3> depth_format_list = { VK_FORMAT_UNDEFINED, VK_FORMAT_D16_UNORM, gpu_format_support.d24_unorm_s8 ? VK_FORMAT_D24_UNORM_S8_UINT : VK_FORMAT_D32_SFLOAT_S8_UINT };
		const std::array<VkFormat, 8> color_format_list = { VK_FORMAT_R5G6B5_UNORM_PACK16, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R8_UINT, VK_FORMAT_R8G8_UINT, VK_FORMAT_A1R5G5B5_UNORM_PACK16, VK_FORMAT_R32_SFLOAT };


		for (const VkFormat &color_format : color_format_list)
		{
			for (const VkFormat &depth_stencil_format : depth_format_list)
			{
				for (u8 number_of_draw_buffer = 0; number_of_draw_buffer <= 4; number_of_draw_buffer++)
				{
					size_t idx = vk::get_render_pass_location(color_format, depth_stencil_format, number_of_draw_buffer);
					result[idx] = precompute_render_pass(dev, color_format, number_of_draw_buffer, depth_stencil_format);
				}
			}
		}
		return result;
	}

	std::tuple<VkPipelineLayout, VkDescriptorSetLayout> get_shared_pipeline_layout(VkDevice dev)
	{
		std::array<VkDescriptorSetLayoutBinding, 39> bindings = {};

		size_t idx = 0;
		// Vertex buffer
		for (int i = 0; i < 16; i++)
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
		bindings[idx].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		bindings[idx].binding = VERTEX_CONSTANT_BUFFERS_BIND_SLOT;

		idx++;

		for (int i = 0; i < 16; i++)
		{
			bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			bindings[idx].descriptorCount = 1;
			bindings[idx].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			bindings[idx].binding = TEXTURES_FIRST_BIND_SLOT + i;
			idx++;
		}

		for (int i = 0; i < 4; i++)
		{
			bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			bindings[idx].descriptorCount = 1;
			bindings[idx].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			bindings[idx].binding = VERTEX_TEXTURES_FIRST_BIND_SLOT + i;
			idx++;
		}

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[idx].descriptorCount = 1;
		bindings[idx].stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
		bindings[idx].binding = SCALE_OFFSET_BIND_SLOT;

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

VKGSRender::VKGSRender() : GSRender()
{
	//shaders_cache.load(rsx::old_shaders_cache::shader_language::glsl);

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

#ifdef _WIN32
	HINSTANCE hInstance = NULL;
	HWND hWnd = (HWND)m_frame->handle();

	std::vector<vk::physical_device>& gpus = m_thread_context.enumerateDevices();	
	
	//Actually confirm  that the loader found at least one compatible device
	//This should not happen unless something is wrong with the driver setup on the target system
	if (gpus.size() == 0)
	{
		//We can't throw in Emulator::Load, so we show error and return
		LOG_FATAL(RSX, "No compatible GPU devices found");
		m_device = VK_NULL_HANDLE;
		return;
	}

	bool gpu_found = false;
	std::string adapter_name = g_cfg.video.vk.adapter;
	for (auto &gpu : gpus)
	{
		if (gpu.name() == adapter_name)
		{
			m_swap_chain = m_thread_context.createSwapChain(hInstance, hWnd, gpu);
			gpu_found = true;
			break;
		}
	}

	if (!gpu_found || adapter_name.empty())
	{
		m_swap_chain = m_thread_context.createSwapChain(hInstance, hWnd, gpus[0]);
	}
	
#elif __linux__

	Window window = (Window)m_frame->handle();
	Display *display = XOpenDisplay(0);

	std::vector<vk::physical_device>& gpus = m_thread_context.enumerateDevices();	
	
	//Actually confirm  that the loader found at least one compatible device
	//This should not happen unless something is wrong with the driver setup on the target system
	if (gpus.size() == 0)
	{
		//We can't throw in Emulator::Load, so we show error and return
		LOG_FATAL(RSX, "No compatible GPU devices found");
		m_device = VK_NULL_HANDLE;
		return;
	}

	XFlush(display);

	bool gpu_found = false;
	std::string adapter_name = g_cfg.video.vk.adapter;
	for (auto &gpu : gpus)
	{
		if (gpu.name() == adapter_name)
		{
			m_swap_chain = m_thread_context.createSwapChain(display, window, gpu);
			gpu_found = true;
			break;
		}
	}

	if (!gpu_found || adapter_name.empty())
	{
		m_swap_chain = m_thread_context.createSwapChain(display, window, gpus[0]);
	}
	
	m_display_handle = display;

#endif

	m_device = (vk::render_device *)(&m_swap_chain->get_device());

	m_memory_type_mapping = get_memory_mapping(m_device->gpu());

	m_optimal_tiling_supported_formats = vk::get_optimal_tiling_supported_formats(m_device->gpu());

	vk::set_current_thread_ctx(m_thread_context);
	vk::set_current_renderer(m_swap_chain->get_device());

	m_client_width = m_frame->client_width();
	m_client_height = m_frame->client_height();
	m_swap_chain->init_swapchain(m_client_width, m_client_height);

	//create command buffer...
	m_command_buffer_pool.create((*m_device));

	for (auto &cb : m_primary_cb_list)
	{
		cb.create(m_command_buffer_pool);
		cb.init_fence(*m_device);
	}

	m_current_command_buffer = &m_primary_cb_list[0];
	
	//Create secondar command_buffer for parallel operations
	m_secondary_command_buffer_pool.create((*m_device));
	m_secondary_command_buffer.create(m_secondary_command_buffer_pool);
	
	open_command_buffer();

	for (u32 i = 0; i < m_swap_chain->get_swap_image_count(); ++i)
	{
		vk::change_image_layout(*m_current_command_buffer, m_swap_chain->get_swap_chain_image(i),
								VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
								vk::get_image_subresource_range(0, 0, 1, 1, VK_IMAGE_ASPECT_COLOR_BIT));

		VkClearColorValue clear_color{};
		auto range = vk::get_image_subresource_range(0, 0, 1, 1, VK_IMAGE_ASPECT_COLOR_BIT);
		vkCmdClearColorImage(*m_current_command_buffer, m_swap_chain->get_swap_chain_image(i), VK_IMAGE_LAYOUT_GENERAL, &clear_color, 1, &range);
		vk::change_image_layout(*m_current_command_buffer, m_swap_chain->get_swap_chain_image(i),
			VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			vk::get_image_subresource_range(0, 0, 1, 1, VK_IMAGE_ASPECT_COLOR_BIT));

	}

	m_attrib_ring_info.init(VK_ATTRIB_RING_BUFFER_SIZE_M * 0x100000);
	m_attrib_ring_info.heap.reset(new vk::buffer(*m_device, VK_ATTRIB_RING_BUFFER_SIZE_M * 0x100000, m_memory_type_mapping.host_visible_coherent, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, 0));
	m_uniform_buffer_ring_info.init(VK_UBO_RING_BUFFER_SIZE_M * 0x100000);
	m_uniform_buffer_ring_info.heap.reset(new vk::buffer(*m_device, VK_UBO_RING_BUFFER_SIZE_M * 0x100000, m_memory_type_mapping.host_visible_coherent, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 0));
	m_index_buffer_ring_info.init(VK_INDEX_RING_BUFFER_SIZE_M * 0x100000);
	m_index_buffer_ring_info.heap.reset(new vk::buffer(*m_device, VK_INDEX_RING_BUFFER_SIZE_M * 0x100000, m_memory_type_mapping.host_visible_coherent, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 0));
	m_texture_upload_buffer_ring_info.init(VK_TEXTURE_UPLOAD_RING_BUFFER_SIZE_M * 0x100000);
	m_texture_upload_buffer_ring_info.heap.reset(new vk::buffer(*m_device, VK_TEXTURE_UPLOAD_RING_BUFFER_SIZE_M * 0x100000, m_memory_type_mapping.host_visible_coherent, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 0));

	m_render_passes = get_precomputed_render_passes(*m_device, m_optimal_tiling_supported_formats);

	std::tie(pipeline_layout, descriptor_layouts) = get_shared_pipeline_layout(*m_device);

	VkDescriptorPoolSize uniform_buffer_pool = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 3 * DESCRIPTOR_MAX_DRAW_CALLS };
	VkDescriptorPoolSize uniform_texel_pool = { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER , 16 * DESCRIPTOR_MAX_DRAW_CALLS };
	VkDescriptorPoolSize texture_pool = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 20 * DESCRIPTOR_MAX_DRAW_CALLS };

	std::vector<VkDescriptorPoolSize> sizes{ uniform_buffer_pool, uniform_texel_pool, texture_pool };

	descriptor_pool.create(*m_device, sizes.data(), static_cast<uint32_t>(sizes.size()));


	null_buffer = std::make_unique<vk::buffer>(*m_device, 32, m_memory_type_mapping.host_visible_coherent, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, 0);
	null_buffer_view = std::make_unique<vk::buffer_view>(*m_device, null_buffer->value, VK_FORMAT_R32_SFLOAT, 0, 32);

	VkFenceCreateInfo fence_info = {};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	VkSemaphoreCreateInfo semaphore_info = {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	vkCreateSemaphore((*m_device), &semaphore_info, nullptr, &m_present_semaphore);

	if (g_cfg.video.overlay)
	{
		size_t idx = vk::get_render_pass_location( m_swap_chain->get_surface_format(), VK_FORMAT_UNDEFINED, 1);
		m_text_writer.reset(new vk::text_writer());
		m_text_writer->init(*m_device, m_memory_type_mapping, m_render_passes[idx]);
	}
}

VKGSRender::~VKGSRender()
{
	if (m_device == VK_NULL_HANDLE)
	{
		//Initialization failed
		return;
	}

	m_current_command_buffer->reset();

	//Wait for device to finish up with resources
	vkDeviceWaitIdle(*m_device);

	//Sync objects
	if (m_present_semaphore)
	{
		vkDestroySemaphore((*m_device), m_present_semaphore, nullptr);
		m_present_semaphore = nullptr;
	}

	//Shaders
	m_prog_buffer.clear();

	//Global resources
	vk::destroy_global_resources();

	//Data heaps/buffers
	m_index_buffer_ring_info.heap.reset();
	m_uniform_buffer_ring_info.heap.reset();
	m_attrib_ring_info.heap.reset();
	m_texture_upload_buffer_ring_info.heap.reset();

	//Fallback bindables
	null_buffer.reset();
	null_buffer_view.reset();

	//Temporary objects
	m_buffer_view_to_clean.clear();
	m_sampler_to_clean.clear();
	m_framebuffer_to_clean.clear();

	//Render passes
	for (auto &render_pass : m_render_passes)
		if (render_pass)
			vkDestroyRenderPass(*m_device, render_pass, nullptr);

	//Textures
	m_rtts.destroy();
	m_texture_cache.destroy();

	m_text_writer.reset();

	//Pipeline descriptors
	vkDestroyPipelineLayout(*m_device, pipeline_layout, nullptr);
	vkDestroyDescriptorSetLayout(*m_device, descriptor_layouts, nullptr);

	descriptor_pool.destroy();

	//Command buffer
	for (auto &cb : m_primary_cb_list)
		cb.destroy();

	m_command_buffer_pool.destroy();

	m_secondary_command_buffer.destroy();
	m_secondary_command_buffer_pool.destroy();

	//Device handles/contexts
	m_swap_chain->destroy();
	m_thread_context.close();

	delete m_swap_chain;
	
#ifdef __linux__
	if (m_display_handle)
		XCloseDisplay(m_display_handle);
#endif
}

bool VKGSRender::on_access_violation(u32 address, bool is_writing)
{
	if (is_writing)
		return m_texture_cache.invalidate_address(address);
	else
	{
		if (g_cfg.video.write_color_buffers || g_cfg.video.write_depth_buffer)
		{
			bool flushable, synchronized;
			std::tie(flushable, synchronized) = m_texture_cache.address_is_flushable(address);
			
			if (!flushable)
				return false;

			if (synchronized)
			{
				if (m_last_flushable_cb >= 0)
				{
					if (m_primary_cb_list[m_last_flushable_cb].pending)
						m_primary_cb_list[m_last_flushable_cb].wait();
				}

				m_last_flushable_cb = -1;
			}
			else
			{
				//This region is buffered, but no previous sync point has been put in place to start sync efforts
				//Just stall and get what we have at this point
				if (std::this_thread::get_id() != rsx_thread)
				{
					{
						std::lock_guard<std::mutex> lock(m_flush_queue_mutex);

						m_flush_commands = true;
						m_queued_threads++;
					}

					//This is awful!
					while (m_flush_commands) _mm_pause();

					std::lock_guard<std::mutex> lock(m_secondary_cb_guard);
					bool status = m_texture_cache.flush_address(address, *m_device, m_secondary_command_buffer, m_memory_type_mapping, m_swap_chain->get_present_queue());

					m_queued_threads--;
					return status;
				}
				else
				{
					//NOTE: If the rsx::thread is trampling its own data, we have an operation that should be moved to the GPU
					//We should never interrupt our own cb recording since some operations are not interruptible
					if (!vk::is_uninterruptible())
						//TODO: Investigate driver behaviour to determine if we need a hard sync or a soft flush
						flush_command_queue();
				}
			}
		}
		else
		{
			//If we aren't managing buffer sync, dont bother checking the cache
			return false;
		}

		std::lock_guard<std::mutex> lock(m_secondary_cb_guard);
		return m_texture_cache.flush_address(address, *m_device, m_secondary_command_buffer, m_memory_type_mapping, m_swap_chain->get_present_queue());
	}

	return false;
}

void VKGSRender::begin()
{
	rsx::thread::begin();

	//Ease resource pressure if the number of draw calls becomes too high or we are running low on memory resources
	if (m_used_descriptors >= DESCRIPTOR_MAX_DRAW_CALLS ||
		m_attrib_ring_info.is_critical() ||
		m_texture_upload_buffer_ring_info.is_critical() ||
		m_uniform_buffer_ring_info.is_critical() ||
		m_index_buffer_ring_info.is_critical())
	{
		std::chrono::time_point<steady_clock> submit_start = steady_clock::now();

		flush_command_queue(true);

		CHECK_RESULT(vkResetDescriptorPool(*m_device, descriptor_pool, 0));
		m_used_descriptors = 0;

		m_uniform_buffer_ring_info.reset_allocation_stats();
		m_index_buffer_ring_info.reset_allocation_stats();
		m_attrib_ring_info.reset_allocation_stats();
		m_texture_upload_buffer_ring_info.reset_allocation_stats();

		std::chrono::time_point<steady_clock> submit_end = steady_clock::now();
		m_flip_time += std::chrono::duration_cast<std::chrono::microseconds>(submit_end - submit_start).count();
	}

	std::chrono::time_point<steady_clock> start = steady_clock::now();

	VkDescriptorSetAllocateInfo alloc_info = {};
	alloc_info.descriptorPool = descriptor_pool;
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts = &descriptor_layouts;
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

	VkDescriptorSet new_descriptor_set;
	CHECK_RESULT(vkAllocateDescriptorSets(*m_device, &alloc_info, &new_descriptor_set));

	descriptor_sets = new_descriptor_set;

	init_buffers();

	float actual_line_width = rsx::method_registers.line_width();

	vkCmdSetLineWidth(*m_current_command_buffer, actual_line_width);

	//TODO: Set up other render-state parameters into the program pipeline

	std::chrono::time_point<steady_clock> stop = steady_clock::now();
	m_setup_time += std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();

	m_used_descriptors++;
}

void VKGSRender::end()
{
	size_t idx = vk::get_render_pass_location(
		vk::get_compatible_surface_format(rsx::method_registers.surface_color()).first,
		vk::get_compatible_depth_surface_format(m_optimal_tiling_supported_formats, rsx::method_registers.surface_depth_fmt()),
		(u8)vk::get_draw_buffers(rsx::method_registers.surface_color_target()).size());
	VkRenderPass current_render_pass = m_render_passes[idx];

	std::chrono::time_point<steady_clock> program_start = steady_clock::now();

	//Load program here since it is dependent on vertex state
	load_program();

	std::chrono::time_point<steady_clock> program_stop = steady_clock::now();
	m_setup_time += (u32)std::chrono::duration_cast<std::chrono::microseconds>(program_stop - program_start).count();

	std::chrono::time_point<steady_clock> textures_start = steady_clock::now();

	for (int i = 0; i < rsx::limits::fragment_textures_count; ++i)
	{
		if (m_program->has_uniform("tex" + std::to_string(i)))
		{
			if (!rsx::method_registers.fragment_textures[i].enabled())
			{
				m_program->bind_uniform({ vk::null_sampler(), vk::null_image_view(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, "tex" + std::to_string(i), descriptor_sets);
				continue;
			}

			vk::image_view *texture0 = m_texture_cache.upload_texture(*m_current_command_buffer, rsx::method_registers.fragment_textures[i], m_rtts, m_memory_type_mapping, m_texture_upload_buffer_ring_info, m_texture_upload_buffer_ring_info.heap.get());

			if (!texture0)
			{
				LOG_ERROR(RSX, "Texture upload failed to texture index %d. Binding null sampler.", i);
				m_program->bind_uniform({ vk::null_sampler(), vk::null_image_view(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, "tex" + std::to_string(i), descriptor_sets);
				continue;
			}

			VkFilter min_filter;
			VkSamplerMipmapMode mip_mode;
			float min_lod = 0.f, max_lod = 0.f;
			float lod_bias = 0.f;

			std::tie(min_filter, mip_mode) = vk::get_min_filter_and_mip(rsx::method_registers.fragment_textures[i].min_filter());
			
			if (rsx::method_registers.fragment_textures[i].get_exact_mipmap_count() > 1)
			{
				min_lod = (float)(rsx::method_registers.fragment_textures[i].min_lod() >> 8);
				max_lod = (float)(rsx::method_registers.fragment_textures[i].max_lod() >> 8);
				lod_bias = rsx::method_registers.fragment_textures[i].bias();
			}
			else
			{
				mip_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			}
			
			m_sampler_to_clean.push_back(std::make_unique<vk::sampler>(
				*m_device,
				vk::vk_wrap_mode(rsx::method_registers.fragment_textures[i].wrap_s()), vk::vk_wrap_mode(rsx::method_registers.fragment_textures[i].wrap_t()), vk::vk_wrap_mode(rsx::method_registers.fragment_textures[i].wrap_r()),
				!!(rsx::method_registers.fragment_textures[i].format() & CELL_GCM_TEXTURE_UN),
				lod_bias, vk::max_aniso(rsx::method_registers.fragment_textures[i].max_aniso()), min_lod, max_lod,
				min_filter, vk::get_mag_filter(rsx::method_registers.fragment_textures[i].mag_filter()), mip_mode, vk::get_border_color(rsx::method_registers.fragment_textures[i].border_color())
				));

			m_program->bind_uniform({ m_sampler_to_clean.back()->value, texture0->value, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, "tex" + std::to_string(i), descriptor_sets);
		}
	}
	
	for (int i = 0; i < rsx::limits::vertex_textures_count; ++i)
	{
		if (m_program->has_uniform("vtex" + std::to_string(i)))
		{
			if (!rsx::method_registers.vertex_textures[i].enabled())
			{
				m_program->bind_uniform({ vk::null_sampler(), vk::null_image_view(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, "vtex" + std::to_string(i), descriptor_sets);
				continue;
			}

			vk::image_view *texture0 = m_texture_cache.upload_texture(*m_current_command_buffer, rsx::method_registers.vertex_textures[i], m_rtts, m_memory_type_mapping, m_texture_upload_buffer_ring_info, m_texture_upload_buffer_ring_info.heap.get());

			if (!texture0)
			{
				LOG_ERROR(RSX, "Texture upload failed to vtexture index %d. Binding null sampler.", i);
				m_program->bind_uniform({ vk::null_sampler(), vk::null_image_view(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, "vtex" + std::to_string(i), descriptor_sets);
				continue;
			}

			m_sampler_to_clean.push_back(std::make_unique<vk::sampler>(
				*m_device,
				VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT,
				!!(rsx::method_registers.vertex_textures[i].format() & CELL_GCM_TEXTURE_UN),
				0, 1.f, rsx::method_registers.vertex_textures[i].min_lod(), rsx::method_registers.vertex_textures[i].max_lod(),
				VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST, vk::get_border_color(rsx::method_registers.vertex_textures[i].border_color())
				));

			m_program->bind_uniform({ m_sampler_to_clean.back()->value, texture0->value, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, "vtex" + std::to_string(i), descriptor_sets);
		}
	}

	std::chrono::time_point<steady_clock> textures_end = steady_clock::now();
	m_textures_upload_time += std::chrono::duration_cast<std::chrono::microseconds>(textures_end - textures_start).count();

	//While vertex upload is an interruptible process, if we made it this far, there's no need to sync anything that occurs past this point
	//Only textures are synchronized tightly with the GPU and they have been read back above
	vk::enter_uninterruptible();

	auto upload_info = upload_vertex_data();

	std::chrono::time_point<steady_clock> vertex_end = steady_clock::now();
	m_vertex_upload_time += std::chrono::duration_cast<std::chrono::microseconds>(vertex_end - textures_end).count();

	VkRenderPassBeginInfo rp_begin = {};
	rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rp_begin.renderPass = current_render_pass;
	rp_begin.framebuffer = m_framebuffer_to_clean.back()->value;
	rp_begin.renderArea.offset.x = 0;
	rp_begin.renderArea.offset.y = 0;
	rp_begin.renderArea.extent.width = m_framebuffer_to_clean.back()->width();
	rp_begin.renderArea.extent.height = m_framebuffer_to_clean.back()->height();

	vkCmdBeginRenderPass(*m_current_command_buffer, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(*m_current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_program->pipeline);
	vkCmdBindDescriptorSets(*m_current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets, 0, nullptr);

	std::optional<std::tuple<VkDeviceSize, VkIndexType> > index_info = std::get<2>(upload_info);

	if (!index_info)
		vkCmdDraw(*m_current_command_buffer, std::get<1>(upload_info), 1, 0, 0);
	else
	{
		VkIndexType index_type;
		u32 index_count = std::get<1>(upload_info);
		VkDeviceSize offset;

		std::tie(offset, index_type) = index_info.value();

		vkCmdBindIndexBuffer(*m_current_command_buffer, m_index_buffer_ring_info.heap->value, offset, index_type);
		vkCmdDrawIndexed(*m_current_command_buffer, index_count, 1, 0, 0, 0);
	}

	vkCmdEndRenderPass(*m_current_command_buffer);

	vk::leave_uninterruptible();

	std::chrono::time_point<steady_clock> draw_end = steady_clock::now();
	m_draw_time += std::chrono::duration_cast<std::chrono::microseconds>(draw_end - vertex_end).count();

	copy_render_targets_to_dma_location();
	m_draw_calls++;

	rsx::thread::end();
}

void VKGSRender::set_viewport()
{
	u16 scissor_x = rsx::method_registers.scissor_origin_x();
	u16 scissor_w = rsx::method_registers.scissor_width();
	u16 scissor_y = rsx::method_registers.scissor_origin_y();
	u16 scissor_h = rsx::method_registers.scissor_height();

	//NOTE: The scale_offset matrix already has viewport matrix factored in
	VkViewport viewport = {};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = rsx::method_registers.surface_clip_width();
	viewport.height = rsx::method_registers.surface_clip_height();
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	vkCmdSetViewport(*m_current_command_buffer, 0, 1, &viewport);

	VkRect2D scissor = {};
	scissor.extent.height = scissor_h;
	scissor.extent.width = scissor_w;
	scissor.offset.x = scissor_x;
	scissor.offset.y = scissor_y;

	vkCmdSetScissor(*m_current_command_buffer, 0, 1, &scissor);
}

void VKGSRender::on_init_thread()
{
	if (m_device == VK_NULL_HANDLE)
	{
		fmt::throw_exception("No vulkan device was created");
	}

	GSRender::on_init_thread();
	rsx_thread = std::this_thread::get_id();
}

void VKGSRender::on_exit()
{
	m_texture_cache.destroy();

	return GSRender::on_exit();
}

void VKGSRender::clear_surface(u32 mask)
{
	// Ignore clear if surface target is set to CELL_GCM_SURFACE_TARGET_NONE
	if (rsx::method_registers.surface_color_target() == rsx::surface_target::none) return;

	if (!(mask & 0xF3)) return;
	if (m_current_present_image == 0xFFFF) return;

	init_buffers();
	copy_render_targets_to_dma_location();

	float depth_clear = 1.f;
	u32   stencil_clear = 0;
	u32   depth_stencil_mask = 0;

	std::vector<VkClearAttachment> clear_descriptors;
	std::vector<VkClearRect> clear_regions;

	VkClearValue depth_stencil_clear_values, color_clear_values;

	u16 scissor_x = rsx::method_registers.scissor_origin_x();
	u16 scissor_w = rsx::method_registers.scissor_width();
	u16 scissor_y = rsx::method_registers.scissor_origin_y();
	u16 scissor_h = rsx::method_registers.scissor_height();

	const u32 fb_width = m_framebuffer_to_clean.back()->width();
	const u32 fb_height = m_framebuffer_to_clean.back()->height();

	//clip region
	std::tie(scissor_x, scissor_y, scissor_w, scissor_h) = rsx::clip_region<u16>(fb_width, fb_height, scissor_x, scissor_y, scissor_w, scissor_h, true);
	VkClearRect region = { { { scissor_x, scissor_y },{ scissor_w, scissor_h } }, 0, 1 };

	auto targets = vk::get_draw_buffers(rsx::method_registers.surface_color_target());
	auto surface_depth_format = rsx::method_registers.surface_depth_fmt();

	if (mask & 0x1)
	{
		u32 max_depth_value = get_max_depth_value(surface_depth_format);

		u32 clear_depth = rsx::method_registers.z_clear_value(surface_depth_format == rsx::surface_depth_format::z24s8);
		float depth_clear = (float)clear_depth / max_depth_value;

		depth_stencil_clear_values.depthStencil.depth = depth_clear;
		depth_stencil_clear_values.depthStencil.stencil = stencil_clear;

		depth_stencil_mask |= VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	if (mask & 0x2)
	{
		if (surface_depth_format == rsx::surface_depth_format::z24s8)
		{
			u8 clear_stencil = rsx::method_registers.stencil_clear_value();

			depth_stencil_clear_values.depthStencil.stencil = clear_stencil;

			depth_stencil_mask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}

	if (mask & 0xF0)
	{
		u8 clear_a = rsx::method_registers.clear_color_a();
		u8 clear_r = rsx::method_registers.clear_color_r();
		u8 clear_g = rsx::method_registers.clear_color_g();
		u8 clear_b = rsx::method_registers.clear_color_b();

		color_clear_values.color.float32[0] = (float)clear_r / 255;
		color_clear_values.color.float32[1] = (float)clear_g / 255;
		color_clear_values.color.float32[2] = (float)clear_b / 255;
		color_clear_values.color.float32[3] = (float)clear_a / 255;

		for (int index = 0; index < targets.size(); ++index)
		{
			clear_descriptors.push_back({ VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)index, color_clear_values });
			clear_regions.push_back(region);
		}
	}

	if (mask & 0x3)
	{
		clear_descriptors.push_back({ (VkImageAspectFlags)depth_stencil_mask, 0, depth_stencil_clear_values });
		clear_regions.push_back(region);
	}

	size_t idx = vk::get_render_pass_location(
		vk::get_compatible_surface_format(rsx::method_registers.surface_color()).first,
		vk::get_compatible_depth_surface_format(m_optimal_tiling_supported_formats, surface_depth_format),
		(u8)targets.size());
	VkRenderPass current_render_pass = m_render_passes[idx];

	VkRenderPassBeginInfo rp_begin = {};
	rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rp_begin.renderPass = current_render_pass;
	rp_begin.framebuffer = m_framebuffer_to_clean.back()->value;
	rp_begin.renderArea.offset.x = 0;
	rp_begin.renderArea.offset.y = 0;
	rp_begin.renderArea.extent.width = m_framebuffer_to_clean.back()->width();
	rp_begin.renderArea.extent.height = m_framebuffer_to_clean.back()->height();

	vkCmdBeginRenderPass(*m_current_command_buffer, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdClearAttachments(*m_current_command_buffer, clear_descriptors.size(), clear_descriptors.data(), clear_regions.size(), clear_regions.data());

	vkCmdEndRenderPass(*m_current_command_buffer);
}

void VKGSRender::sync_at_semaphore_release()
{
	m_flush_draw_buffers = true;
}

void VKGSRender::copy_render_targets_to_dma_location()
{
	if (!m_flush_draw_buffers)
		return;

	if (!g_cfg.video.write_color_buffers && !g_cfg.video.write_depth_buffer)
		return;

	//TODO: Make this asynchronous. Should be similar to a glFlush() but in this case its similar to glFinish
	//This is due to all the hard waits for fences
	//TODO: Use a command buffer array to allow explicit draw command tracking

	vk::enter_uninterruptible();

	if (g_cfg.video.write_color_buffers)
	{
		for (u8 index = 0; index < rsx::limits::color_buffers_count; index++)
		{
			if (!m_surface_info[index].pitch)
				continue;

			m_texture_cache.flush_memory_to_cache(m_surface_info[index].address, m_surface_info[index].pitch * m_surface_info[index].height,
					*m_current_command_buffer, m_memory_type_mapping, m_swap_chain->get_present_queue());
		}
	}

	if (g_cfg.video.write_depth_buffer)
	{
		if (m_depth_surface_info.pitch)
		{
			m_texture_cache.flush_memory_to_cache(m_depth_surface_info.address, m_depth_surface_info.pitch * m_depth_surface_info.height,
				*m_current_command_buffer, m_memory_type_mapping, m_swap_chain->get_present_queue());
		}
	}

	vk::leave_uninterruptible();

	m_last_flushable_cb = m_current_cb_index;
	flush_command_queue();

	m_flush_draw_buffers = false;
}

void VKGSRender::flush_command_queue(bool hard_sync)
{
	close_and_submit_command_buffer({}, m_current_command_buffer->submit_fence);

	if (hard_sync)
	{
		//swap handler checks the pending flag, so call it here
		process_swap_request();

		//wait for the latest intruction to execute
		m_current_command_buffer->pending = true;
		m_current_command_buffer->wait();

		//Clear all command buffer statuses
		for (auto &cb : m_primary_cb_list)
			cb.poke();

		m_last_flushable_cb = -1;
		m_flush_commands = false;
	}
	else
	{
		//Mark this queue as pending
		m_current_command_buffer->pending = true;
		
		//Grab next cb in line and make it usable
		m_current_cb_index = (m_current_cb_index + 1) % VK_MAX_ASYNC_CB_COUNT;
		m_current_command_buffer = &m_primary_cb_list[m_current_cb_index];
		m_current_command_buffer->reset();
	}

	open_command_buffer();
}

void VKGSRender::queue_swap_request()
{
	//buffer the swap request and return
	if (m_swap_command_buffer && m_swap_command_buffer->pending)
	{
		//Its probable that no actual drawing took place
		process_swap_request();
	}

	m_swap_command_buffer = m_current_command_buffer;
	close_and_submit_command_buffer({ m_present_semaphore }, m_current_command_buffer->submit_fence);

	//Grab next cb in line and make it usable
	m_current_cb_index = (m_current_cb_index + 1) % VK_MAX_ASYNC_CB_COUNT;
	m_current_command_buffer = &m_primary_cb_list[m_current_cb_index];
	m_current_command_buffer->reset();

	m_swap_command_buffer->pending = true;
	open_command_buffer();
}

void VKGSRender::process_swap_request()
{
	if (!m_swap_command_buffer)
		return;

	if (m_swap_command_buffer->pending)
	{
		//Perform hard swap here
		m_swap_command_buffer->wait();

		VkSwapchainKHR swap_chain = (VkSwapchainKHR)(*m_swap_chain);

		VkPresentInfoKHR present = {};
		present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present.pNext = nullptr;
		present.swapchainCount = 1;
		present.pSwapchains = &swap_chain;
		present.pImageIndices = &m_current_present_image;
		CHECK_RESULT(m_swap_chain->queuePresentKHR(m_swap_chain->get_present_queue(), &present));
	}

	//Clean up all the resources from the last frame

	//Feed back damaged resources to the main texture cache for management...
	//m_texture_cache.merge_dirty_textures(m_rtts.invalidated_resources);
	
	m_rtts.invalidated_resources.clear();
	m_texture_cache.flush();

	m_buffer_view_to_clean.clear();
	m_sampler_to_clean.clear();
	m_framebuffer_to_clean.clear();

	if (g_cfg.video.overlay)
	{
		m_text_writer->reset_descriptors();
	}

	m_swap_command_buffer = nullptr;
}

void VKGSRender::do_local_task()
{
	if (m_flush_commands)
	{
		std::lock_guard<std::mutex> lock(m_flush_queue_mutex);

		//TODO: Determine if a hard sync is necessary
		//Pipeline barriers later may do a better job synchronizing than wholly stalling the pipeline
		flush_command_queue();

		m_flush_commands = false;
		while (m_queued_threads) _mm_pause();
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
	case NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE:
		sync_at_semaphore_release();
		return false; //call rsx::thread method implementation
	default:
		return false;
	}
}

bool VKGSRender::load_program()
{
	auto rtt_lookup_func = [this](u32 texaddr, rsx::fragment_texture&, bool is_depth) -> std::tuple<bool, u16>
	{
		vk::render_target *surface = nullptr;
		if (!is_depth)
			surface = m_rtts.get_texture_from_render_target_if_applicable(texaddr);
		else
			surface = m_rtts.get_texture_from_depth_stencil_if_applicable(texaddr);

		if (!surface) return std::make_tuple(false, 0);

		return std::make_tuple(true, surface->native_pitch);
	};

	RSXVertexProgram vertex_program = get_current_vertex_program();
	RSXFragmentProgram fragment_program = get_current_fragment_program(rtt_lookup_func);

	vk::pipeline_props properties = {};

	properties.ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	bool unused;
	properties.ia.topology = vk::get_appropriate_topology(rsx::method_registers.current_draw_clause.primitive, unused);

	if (rsx::method_registers.restart_index_enabled())
	{
		properties.ia.primitiveRestartEnable = VK_TRUE;
	}
	else
		properties.ia.primitiveRestartEnable = VK_FALSE;


	for (int i = 0; i < 4; ++i)
	{
		properties.att_state[i].colorWriteMask = 0xf;
		properties.att_state[i].blendEnable = VK_FALSE;
	}

	VkColorComponentFlags mask = 0;
	if (rsx::method_registers.color_mask_a()) mask |= VK_COLOR_COMPONENT_A_BIT;
	if (rsx::method_registers.color_mask_b()) mask |= VK_COLOR_COMPONENT_B_BIT;
	if (rsx::method_registers.color_mask_g()) mask |= VK_COLOR_COMPONENT_G_BIT;
	if (rsx::method_registers.color_mask_r()) mask |= VK_COLOR_COMPONENT_R_BIT;

	VkColorComponentFlags color_masks[4] = { mask };

	u8 render_targets[] = { 0, 1, 2, 3 };

	for (u8 idx = 0; idx < m_draw_buffers_count; ++idx)
	{
		properties.att_state[render_targets[idx]].colorWriteMask = mask;
	}

	if (rsx::method_registers.blend_enabled())
	{
		VkBlendFactor sfactor_rgb = vk::get_blend_factor(rsx::method_registers.blend_func_sfactor_rgb());
		VkBlendFactor sfactor_a = vk::get_blend_factor(rsx::method_registers.blend_func_sfactor_a());
		VkBlendFactor dfactor_rgb = vk::get_blend_factor(rsx::method_registers.blend_func_dfactor_rgb());
		VkBlendFactor dfactor_a = vk::get_blend_factor(rsx::method_registers.blend_func_dfactor_a());

		VkBlendOp equation_rgb = vk::get_blend_op(rsx::method_registers.blend_equation_rgb());
		VkBlendOp equation_a = vk::get_blend_op(rsx::method_registers.blend_equation_a());

		for (u8 idx = 0; idx < m_draw_buffers_count; ++idx)
		{
			properties.att_state[render_targets[idx]].blendEnable = VK_TRUE;
			properties.att_state[render_targets[idx]].srcColorBlendFactor = sfactor_rgb;
			properties.att_state[render_targets[idx]].dstColorBlendFactor = dfactor_rgb;
			properties.att_state[render_targets[idx]].srcAlphaBlendFactor = sfactor_a;
			properties.att_state[render_targets[idx]].dstAlphaBlendFactor = dfactor_a;
			properties.att_state[render_targets[idx]].colorBlendOp = equation_rgb;
			properties.att_state[render_targets[idx]].alphaBlendOp = equation_a;
		}
		
		auto blend_colors = rsx::get_constant_blend_colors();
		properties.cs.blendConstants[0] = blend_colors[0];
		properties.cs.blendConstants[1] = blend_colors[1];
		properties.cs.blendConstants[2] = blend_colors[2];
		properties.cs.blendConstants[3] = blend_colors[3];
	}
	else
	{
		for (u8 idx = 0; idx < m_draw_buffers_count; ++idx)
		{
			properties.att_state[render_targets[idx]].blendEnable = VK_FALSE;
		}
	}
	
	properties.cs.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	properties.cs.attachmentCount = m_draw_buffers_count;
	properties.cs.pAttachments = properties.att_state;
	
	if (rsx::method_registers.logic_op_enabled())
	{
		properties.cs.logicOpEnable = true;
		properties.cs.logicOp = vk::get_logic_op(rsx::method_registers.logic_operation());
	}

	properties.ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	properties.ds.depthWriteEnable = rsx::method_registers.depth_write_enabled() ? VK_TRUE : VK_FALSE;

	if (rsx::method_registers.depth_bounds_test_enabled())
	{
		properties.ds.depthBoundsTestEnable = VK_TRUE;
		properties.ds.minDepthBounds = rsx::method_registers.depth_bounds_min();
		properties.ds.maxDepthBounds = rsx::method_registers.depth_bounds_max();
	}
	else
		properties.ds.depthBoundsTestEnable = VK_FALSE;

	if (rsx::method_registers.stencil_test_enabled())
	{
		properties.ds.stencilTestEnable = VK_TRUE;
		properties.ds.front.writeMask = rsx::method_registers.stencil_mask();
		properties.ds.front.compareMask = rsx::method_registers.stencil_func_mask();
		properties.ds.front.reference = rsx::method_registers.stencil_func_ref();
		properties.ds.front.failOp = vk::get_stencil_op(rsx::method_registers.stencil_op_fail());
		properties.ds.front.passOp = vk::get_stencil_op(rsx::method_registers.stencil_op_zpass());
		properties.ds.front.depthFailOp = vk::get_stencil_op(rsx::method_registers.stencil_op_zfail());
		properties.ds.front.compareOp = vk::get_compare_func(rsx::method_registers.stencil_func());

		if (rsx::method_registers.two_sided_stencil_test_enabled())
		{
			properties.ds.back.failOp = vk::get_stencil_op(rsx::method_registers.back_stencil_op_fail());
			properties.ds.back.passOp = vk::get_stencil_op(rsx::method_registers.back_stencil_op_zpass());
			properties.ds.back.depthFailOp = vk::get_stencil_op(rsx::method_registers.back_stencil_op_zfail());
			properties.ds.back.compareOp = vk::get_compare_func(rsx::method_registers.back_stencil_func());
		}
		else
			properties.ds.back = properties.ds.front;
	}
	else
		properties.ds.stencilTestEnable = VK_FALSE;

	if (rsx::method_registers.depth_test_enabled())
	{
		properties.ds.depthTestEnable = VK_TRUE;
		properties.ds.depthCompareOp = vk::get_compare_func(rsx::method_registers.depth_func());
	}
	else
		properties.ds.depthTestEnable = VK_FALSE;

	properties.rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	properties.rs.polygonMode = VK_POLYGON_MODE_FILL;
	properties.rs.depthClampEnable = VK_FALSE;
	properties.rs.rasterizerDiscardEnable = VK_FALSE;
	properties.rs.depthBiasEnable = VK_FALSE;

	if (rsx::method_registers.cull_face_enabled())
		properties.rs.cullMode = vk::get_cull_face(rsx::method_registers.cull_face_mode());
	else
		properties.rs.cullMode = VK_CULL_MODE_NONE;

	properties.rs.frontFace = vk::get_front_face(rsx::method_registers.front_face_mode());

	size_t idx = vk::get_render_pass_location(
		vk::get_compatible_surface_format(rsx::method_registers.surface_color()).first,
		vk::get_compatible_depth_surface_format(m_optimal_tiling_supported_formats, rsx::method_registers.surface_depth_fmt()),
		(u8)vk::get_draw_buffers(rsx::method_registers.surface_color_target()).size());
	
	properties.render_pass = m_render_passes[idx];

	properties.num_targets = m_draw_buffers_count;

	vk::enter_uninterruptible();

	//Load current program from buffer
	m_program = m_prog_buffer.getGraphicPipelineState(vertex_program, fragment_program, properties, *m_device, pipeline_layout).get();

	//TODO: Update constant buffers..
	//1. Update scale-offset matrix
	//2. Update vertex constants
	//3. Update fragment constants
	const size_t scale_offset_offset = m_uniform_buffer_ring_info.alloc<256>(256);

	u8 *buf = (u8*)m_uniform_buffer_ring_info.map(scale_offset_offset, 256);

	/**
	* NOTE: While VK's coord system resembles GLs, the clip volume is no longer symetrical in z
	* Its like D3D without the flip in y (depending on how you build the spir-v)
	*/
	fill_scale_offset_data(buf, false, false);
	fill_user_clip_data(buf + 64);

	m_uniform_buffer_ring_info.unmap();

	const size_t vertex_constants_offset = m_uniform_buffer_ring_info.alloc<256>(512 * 4 * sizeof(float));
	buf = (u8*)m_uniform_buffer_ring_info.map(vertex_constants_offset, 512 * 4 * sizeof(float));
	fill_vertex_program_constants_data(buf);
	*(reinterpret_cast<u32*>(buf + (468 * 4 * sizeof(float)))) = rsx::method_registers.transform_branch_bits();
	m_uniform_buffer_ring_info.unmap();

	const size_t fragment_constants_sz = m_prog_buffer.get_fragment_constants_buffer_size(fragment_program);
	const size_t fragment_buffer_sz = fragment_constants_sz + (17 * 4 * sizeof(float));
	const size_t fragment_constants_offset = m_uniform_buffer_ring_info.alloc<256>(fragment_buffer_sz);

	buf = (u8*)m_uniform_buffer_ring_info.map(fragment_constants_offset, fragment_buffer_sz);
	if (fragment_constants_sz)
		m_prog_buffer.fill_fragment_constants_buffer({ reinterpret_cast<float*>(buf), ::narrow<int>(fragment_constants_sz) }, fragment_program);

	fill_fragment_state_buffer(buf+fragment_constants_sz, fragment_program);
	m_uniform_buffer_ring_info.unmap();

	m_program->bind_uniform({ m_uniform_buffer_ring_info.heap->value, scale_offset_offset, 256 }, SCALE_OFFSET_BIND_SLOT, descriptor_sets);
	m_program->bind_uniform({ m_uniform_buffer_ring_info.heap->value, vertex_constants_offset, 512 * 4 * sizeof(float) }, VERTEX_CONSTANT_BUFFERS_BIND_SLOT, descriptor_sets);	
	m_program->bind_uniform({ m_uniform_buffer_ring_info.heap->value, fragment_constants_offset, fragment_buffer_sz }, FRAGMENT_CONSTANT_BUFFERS_BIND_SLOT, descriptor_sets);

	vk::leave_uninterruptible();

	return true;
}

static const u32 mr_color_offset[rsx::limits::color_buffers_count] =
{
	NV4097_SET_SURFACE_COLOR_AOFFSET,
	NV4097_SET_SURFACE_COLOR_BOFFSET,
	NV4097_SET_SURFACE_COLOR_COFFSET,
	NV4097_SET_SURFACE_COLOR_DOFFSET
};

static const u32 mr_color_dma[rsx::limits::color_buffers_count] =
{
	NV4097_SET_CONTEXT_DMA_COLOR_A,
	NV4097_SET_CONTEXT_DMA_COLOR_B,
	NV4097_SET_CONTEXT_DMA_COLOR_C,
	NV4097_SET_CONTEXT_DMA_COLOR_D
};

static const u32 mr_color_pitch[rsx::limits::color_buffers_count] =
{
	NV4097_SET_SURFACE_PITCH_A,
	NV4097_SET_SURFACE_PITCH_B,
	NV4097_SET_SURFACE_PITCH_C,
	NV4097_SET_SURFACE_PITCH_D
};

void VKGSRender::init_buffers(bool skip_reading)
{
	//Clear any pending swap requests
	process_swap_request();

	prepare_rtts();

	if (!skip_reading)
	{
		read_buffers();
	}

	set_viewport();
}

void VKGSRender::read_buffers()
{
}

void VKGSRender::write_buffers()
{
}

void VKGSRender::close_and_submit_command_buffer(const std::vector<VkSemaphore> &semaphores, VkFence fence, VkPipelineStageFlags pipeline_stage_flags)
{
	CHECK_RESULT(vkEndCommandBuffer(*m_current_command_buffer));

	VkCommandBuffer cmd = *m_current_command_buffer;

	VkSubmitInfo infos = {};
	infos.commandBufferCount = 1;
	infos.pCommandBuffers = &cmd;
	infos.pWaitDstStageMask = &pipeline_stage_flags;
	infos.pWaitSemaphores = semaphores.data();
	infos.waitSemaphoreCount = static_cast<uint32_t>(semaphores.size());
	infos.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	CHECK_RESULT(vkQueueSubmit(m_swap_chain->get_present_queue(), 1, &infos, fence));
}

void VKGSRender::open_command_buffer()
{
	VkCommandBufferInheritanceInfo inheritance_info = {};
	inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;

	VkCommandBufferBeginInfo begin_infos = {};
	begin_infos.pInheritanceInfo = &inheritance_info;
	begin_infos.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_infos.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	CHECK_RESULT(vkBeginCommandBuffer(*m_current_command_buffer, &begin_infos));
}


void VKGSRender::prepare_rtts()
{
	if (!m_rtts_dirty)
		return;

	copy_render_targets_to_dma_location();

	m_rtts_dirty = false;

	u32 clip_width = rsx::method_registers.surface_clip_width();
	u32 clip_height = rsx::method_registers.surface_clip_height();
	u32 clip_x = rsx::method_registers.surface_clip_origin_x();
	u32 clip_y = rsx::method_registers.surface_clip_origin_y();

	auto surface_addresses = get_color_surface_addresses();
	auto zeta_address = get_zeta_surface_address();
	
	const u32 surface_pitchs[] = { rsx::method_registers.surface_a_pitch(), rsx::method_registers.surface_b_pitch(),
			rsx::method_registers.surface_c_pitch(), rsx::method_registers.surface_d_pitch() };

	m_rtts.prepare_render_target(&*m_current_command_buffer,
		rsx::method_registers.surface_color(), rsx::method_registers.surface_depth_fmt(),
		clip_width, clip_height,
		rsx::method_registers.surface_color_target(),
		surface_addresses, zeta_address,
		(*m_device), &*m_current_command_buffer, m_optimal_tiling_supported_formats, m_memory_type_mapping);

	//Reset framebuffer information
	for (u8 i = 0; i < rsx::limits::color_buffers_count; ++i)
	{
		m_surface_info[i].address = m_surface_info[i].pitch = 0;
		m_surface_info[i].width = clip_width;
		m_surface_info[i].height = clip_height;
		m_surface_info[i].color_format = rsx::method_registers.surface_color();
	}

	m_depth_surface_info.address = m_depth_surface_info.pitch = 0;
	m_depth_surface_info.width = clip_width;
	m_depth_surface_info.height = clip_height;
	m_depth_surface_info.depth_format = rsx::method_registers.surface_depth_fmt();

	//Bind created rtts as current fbo...
	std::vector<u8> draw_buffers = vk::get_draw_buffers(rsx::method_registers.surface_color_target());
	std::vector<std::unique_ptr<vk::image_view>> fbo_images;

	for (u8 index : draw_buffers)
	{
		vk::image *raw = std::get<1>(m_rtts.m_bound_render_targets[index]);

		VkImageSubresourceRange subres = {};
		subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subres.baseArrayLayer = 0;
		subres.baseMipLevel = 0;
		subres.layerCount = 1;
		subres.levelCount = 1;

		fbo_images.push_back(std::make_unique<vk::image_view>(*m_device, raw->value, VK_IMAGE_VIEW_TYPE_2D, raw->info.format, vk::default_component_map(), subres));

		m_surface_info[index].address = surface_addresses[index];
		m_surface_info[index].pitch = surface_pitchs[index];

		if (surface_pitchs[index] <= 64)
		{
			if (clip_width > surface_pitchs[index])
				//Ignore this buffer (usually set to 64)
				m_surface_info[index].pitch = 0;
		}
	}

	m_draw_buffers_count = static_cast<u32>(fbo_images.size());

	if (std::get<1>(m_rtts.m_bound_depth_stencil) != nullptr)
	{
		vk::image *raw = (std::get<1>(m_rtts.m_bound_depth_stencil));

		VkImageSubresourceRange subres = {};
		subres.aspectMask = (rsx::method_registers.surface_depth_fmt() == rsx::surface_depth_format::z24s8) ? (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT) : VK_IMAGE_ASPECT_DEPTH_BIT;
		subres.baseArrayLayer = 0;
		subres.baseMipLevel = 0;
		subres.layerCount = 1;
		subres.levelCount = 1;

		fbo_images.push_back(std::make_unique<vk::image_view>(*m_device, raw->value, VK_IMAGE_VIEW_TYPE_2D, raw->info.format, vk::default_component_map(), subres));

		m_depth_surface_info.address = zeta_address;
		m_depth_surface_info.pitch = rsx::method_registers.surface_z_pitch();

		if (m_depth_surface_info.pitch <= 64 && clip_width > m_depth_surface_info.pitch)
			m_depth_surface_info.pitch = 0;
	}

	if (g_cfg.video.write_color_buffers)
	{
		for (u8 index : draw_buffers)
		{
			if (!m_surface_info[index].address || !m_surface_info[index].pitch) continue;
			const u32 range = m_surface_info[index].pitch * m_surface_info[index].height;

			m_texture_cache.lock_memory_region(std::get<1>(m_rtts.m_bound_render_targets[index]), m_surface_info[index].address, range,
					m_surface_info[index].width, m_surface_info[index].height);
		}
	}

	if (g_cfg.video.write_depth_buffer)
	{
		if (m_depth_surface_info.address && m_depth_surface_info.pitch)
		{
			u32 pitch = m_depth_surface_info.width * 2;
			if (m_depth_surface_info.depth_format != rsx::surface_depth_format::z16) pitch *= 2;

			const u32 range = pitch * m_depth_surface_info.height;
			m_texture_cache.lock_memory_region(std::get<1>(m_rtts.m_bound_depth_stencil), m_depth_surface_info.address, range,
				m_depth_surface_info.width, m_depth_surface_info.height);
		}
	}

	size_t idx = vk::get_render_pass_location(vk::get_compatible_surface_format(rsx::method_registers.surface_color()).first, vk::get_compatible_depth_surface_format(m_optimal_tiling_supported_formats, rsx::method_registers.surface_depth_fmt()), (u8)draw_buffers.size());
	VkRenderPass current_render_pass = m_render_passes[idx];

	m_framebuffer_to_clean.push_back(std::make_unique<vk::framebuffer>(*m_device, current_render_pass, clip_width, clip_height, std::move(fbo_images)));
}


void VKGSRender::flip(int buffer)
{
	bool resize_screen = false;

	if (m_client_height != m_frame->client_height() ||
		m_client_width != m_frame->client_width())
	{
		if (!!m_frame->client_height() && !!m_frame->client_width())
			resize_screen = true;
	}

	std::chrono::time_point<steady_clock> flip_start = steady_clock::now();

	process_swap_request();

	if (!resize_screen)
	{
		u32 buffer_width = gcm_buffers[buffer].width;
		u32 buffer_height = gcm_buffers[buffer].height;
		u32 buffer_pitch = gcm_buffers[buffer].pitch;

		rsx::tiled_region buffer_region = get_tiled_address(gcm_buffers[buffer].offset, CELL_GCM_LOCATION_LOCAL);

		areai screen_area = coordi({}, { (int)buffer_width, (int)buffer_height });

		coordi aspect_ratio;

		sizei csize = { m_frame->client_width(), m_frame->client_height() };
		sizei new_size = csize;

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

		aspect_ratio.size = new_size;

		//Prepare surface for new frame
		CHECK_RESULT(vkAcquireNextImageKHR((*m_device), (*m_swap_chain), 0, m_present_semaphore, VK_NULL_HANDLE, &m_current_present_image));

		//Blit contents to screen..
		VkImage image_to_flip = nullptr;

		if (std::get<1>(m_rtts.m_bound_render_targets[0]) != nullptr)
			image_to_flip = std::get<1>(m_rtts.m_bound_render_targets[0])->value;
		else if (std::get<1>(m_rtts.m_bound_render_targets[1]) != nullptr)
			image_to_flip = std::get<1>(m_rtts.m_bound_render_targets[1])->value;

		VkImage target_image = m_swap_chain->get_swap_chain_image(m_current_present_image);
		if (image_to_flip)
		{
			vk::copy_scaled_image(*m_current_command_buffer, image_to_flip, target_image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				0, 0, buffer_width, buffer_height, aspect_ratio.x, aspect_ratio.y, aspect_ratio.width, aspect_ratio.height, 1, VK_IMAGE_ASPECT_COLOR_BIT);
		}
		else
		{
			//No draw call was issued!
			VkImageSubresourceRange range = vk::get_image_subresource_range(0, 0, 1, 1, VK_IMAGE_ASPECT_COLOR_BIT);
			VkClearColorValue clear_black = { 0 };
			vk::change_image_layout(*m_current_command_buffer, m_swap_chain->get_swap_chain_image(m_current_present_image), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_GENERAL, range);
			vkCmdClearColorImage(*m_current_command_buffer, m_swap_chain->get_swap_chain_image(m_current_present_image), VK_IMAGE_LAYOUT_GENERAL, &clear_black, 1, &range);
			vk::change_image_layout(*m_current_command_buffer, m_swap_chain->get_swap_chain_image(m_current_present_image), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, range);
		}

		std::unique_ptr<vk::framebuffer> direct_fbo;
		std::vector<std::unique_ptr<vk::image_view>> swap_image_view;
		if (g_cfg.video.overlay)
		{
			//Change the image layout whilst setting up a dependency on waiting for the blit op to finish before we start writing
			auto subres = vk::get_image_subresource_range(0, 0, 1, 1, VK_IMAGE_ASPECT_COLOR_BIT);
			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			barrier.image = target_image;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange = subres;

			vkCmdPipelineBarrier(*m_current_command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &barrier);

			size_t idx = vk::get_render_pass_location(m_swap_chain->get_surface_format(), VK_FORMAT_UNDEFINED, 1);
			VkRenderPass single_target_pass = m_render_passes[idx];

			swap_image_view.push_back(std::make_unique<vk::image_view>(*m_device, target_image, VK_IMAGE_VIEW_TYPE_2D, m_swap_chain->get_surface_format(), vk::default_component_map(), subres));
			direct_fbo.reset(new vk::framebuffer(*m_device, single_target_pass, m_client_width, m_client_height, std::move(swap_image_view)));
			
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0, 0, direct_fbo->width(), direct_fbo->height(), "draw calls: " + std::to_string(m_draw_calls));
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0, 18, direct_fbo->width(), direct_fbo->height(), "draw call setup: " + std::to_string(m_setup_time) + "us");
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0, 36, direct_fbo->width(), direct_fbo->height(), "vertex upload time: " + std::to_string(m_vertex_upload_time) + "us");
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0, 54, direct_fbo->width(), direct_fbo->height(), "texture upload time: " + std::to_string(m_textures_upload_time) + "us");
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0, 72, direct_fbo->width(), direct_fbo->height(), "draw call execution: " + std::to_string(m_draw_time) + "us");
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0, 90, direct_fbo->width(), direct_fbo->height(), "submit and flip: " + std::to_string(m_flip_time) + "us");
			
			vk::change_image_layout(*m_current_command_buffer, target_image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, subres);
		}

		m_framebuffer_to_clean.push_back(std::move(direct_fbo));
		queue_swap_request();
	}
	else
	{
		/**
		* Since we are about to destroy the old swapchain and its images, we just discard the commandbuffer.
		* Waiting for the commands to process does not work reliably as the fence can be signaled before swap images are released
		* and there are no explicit methods to ensure that the presentation engine is not using the images at all.
		*/

		//NOTE: This operation will create a hard sync point
		CHECK_RESULT(vkEndCommandBuffer(*m_current_command_buffer));

		//Will have to block until rendering is completed
		VkFence resize_fence = VK_NULL_HANDLE;
		VkFenceCreateInfo infos = {};
		infos.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

		vkQueueWaitIdle(m_swap_chain->get_present_queue());
		vkDeviceWaitIdle(*m_device);

		vkCreateFence((*m_device), &infos, nullptr, &resize_fence);

		//Wait for all grpahics tasks to complete
		VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
		VkSubmitInfo submit_infos = {};
		submit_infos.commandBufferCount = 0;
		submit_infos.pCommandBuffers = nullptr;
		submit_infos.pWaitDstStageMask = &pipe_stage_flags;
		submit_infos.pWaitSemaphores = nullptr;
		submit_infos.waitSemaphoreCount = 0;
		submit_infos.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		CHECK_RESULT(vkQueueSubmit(m_swap_chain->get_present_queue(), 1, &submit_infos, resize_fence));

		vkWaitForFences((*m_device), 1, &resize_fence, VK_TRUE, UINT64_MAX);
		vkResetFences((*m_device), 1, &resize_fence);

		vkDeviceWaitIdle(*m_device);

		//Rebuild swapchain. Old swapchain destruction is handled by the init_swapchain call
		m_client_width = m_frame->client_width();
		m_client_height = m_frame->client_height();
		m_swap_chain->init_swapchain(m_client_width, m_client_height);

		//Prepare new swapchain images for use
		m_current_command_buffer->reset();
		open_command_buffer();

		for (u32 i = 0; i < m_swap_chain->get_swap_image_count(); ++i)
		{
			vk::change_image_layout(*m_current_command_buffer, m_swap_chain->get_swap_chain_image(i),
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
				vk::get_image_subresource_range(0, 0, 1, 1, VK_IMAGE_ASPECT_COLOR_BIT));

			VkClearColorValue clear_color{};
			auto range = vk::get_image_subresource_range(0, 0, 1, 1, VK_IMAGE_ASPECT_COLOR_BIT);
			vkCmdClearColorImage(*m_current_command_buffer, m_swap_chain->get_swap_chain_image(i), VK_IMAGE_LAYOUT_GENERAL, &clear_color, 1, &range);
			vk::change_image_layout(*m_current_command_buffer, m_swap_chain->get_swap_chain_image(i),
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				vk::get_image_subresource_range(0, 0, 1, 1, VK_IMAGE_ASPECT_COLOR_BIT));
		}

		//Flush the command buffer
		close_and_submit_command_buffer({}, resize_fence);
		CHECK_RESULT(vkWaitForFences((*m_device), 1, &resize_fence, VK_TRUE, UINT64_MAX));
		vkDestroyFence((*m_device), resize_fence, nullptr);

		m_current_command_buffer->reset();
		open_command_buffer();

		//Do cleanup
		m_swap_command_buffer = m_current_command_buffer;
		process_swap_request();
	}

	std::chrono::time_point<steady_clock> flip_end = steady_clock::now();
	m_flip_time = std::chrono::duration_cast<std::chrono::microseconds>(flip_end - flip_start).count();

	m_uniform_buffer_ring_info.reset_allocation_stats();
	m_index_buffer_ring_info.reset_allocation_stats();
	m_attrib_ring_info.reset_allocation_stats();
	m_texture_upload_buffer_ring_info.reset_allocation_stats();

	//Resource destruction is handled within the real swap handler

	m_draw_calls = 0;
	m_draw_time = 0;
	m_setup_time = 0;
	m_vertex_upload_time = 0;
	m_textures_upload_time = 0;
	m_frame->flip(m_context);
}
