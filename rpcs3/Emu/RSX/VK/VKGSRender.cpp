#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "VKGSRender.h"
#include "../rsx_methods.h"
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
		throw EXCEPTION("Unknow depth format");
	}

	u8 get_pixel_size(rsx::surface_depth_format format)
	{
		switch (format)
		{
		case rsx::surface_depth_format::z16: return 2;
		case rsx::surface_depth_format::z24s8: return 4;
		}
		throw EXCEPTION("Unknow depth format");
	}
}

namespace vk
{
	VkCompareOp compare_op(u32 gl_name)
	{
		switch (gl_name)
		{
		case CELL_GCM_NEVER:
			return VK_COMPARE_OP_NEVER;
		case CELL_GCM_GREATER:
			return VK_COMPARE_OP_GREATER;
		case CELL_GCM_LESS:
			return VK_COMPARE_OP_LESS;
		case CELL_GCM_LEQUAL:
			return VK_COMPARE_OP_LESS_OR_EQUAL;
		case CELL_GCM_GEQUAL:
			return VK_COMPARE_OP_GREATER_OR_EQUAL;
		case CELL_GCM_EQUAL:
			return VK_COMPARE_OP_EQUAL;
		case CELL_GCM_ALWAYS:
			return VK_COMPARE_OP_ALWAYS;
		default:
			throw EXCEPTION("Unsupported compare op: 0x%X", gl_name);
		}
	}

	std::pair<VkFormat, VkComponentMapping> get_compatible_surface_format(rsx::surface_color_format color_format)
	{
		switch (color_format)
		{
		case rsx::surface_color_format::r5g6b5:
			return std::make_pair(VK_FORMAT_R5G6B5_UNORM_PACK16, vk::default_component_map());

		case rsx::surface_color_format::a8r8g8b8:
			return std::make_pair(VK_FORMAT_B8G8R8A8_UNORM, vk::default_component_map());

		case rsx::surface_color_format::x8b8g8r8_o8b8g8r8:
		case rsx::surface_color_format::x8b8g8r8_z8b8g8r8:
		case rsx::surface_color_format::x8r8g8b8_z8r8g8b8:
		case rsx::surface_color_format::x8r8g8b8_o8r8g8b8:
		case rsx::surface_color_format::a8b8g8r8:
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
			LOG_ERROR(RSX, "Surface color buffer: Unsupported surface color format (0x%x)", color_format);
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

		EXPECTS(color_count < 5);

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
			LOG_ERROR(RSX, "Bad surface color target: %d", fmt);
			return{};
		}
	}

	VkLogicOp get_logic_op(u32 op)
	{
		switch (op)
		{
		case CELL_GCM_CLEAR: return VK_LOGIC_OP_CLEAR;
		case CELL_GCM_AND: return VK_LOGIC_OP_AND;
		case CELL_GCM_AND_REVERSE: return VK_LOGIC_OP_AND_REVERSE;
		case CELL_GCM_COPY: return VK_LOGIC_OP_COPY;
		case CELL_GCM_AND_INVERTED: return VK_LOGIC_OP_AND_INVERTED;
		case CELL_GCM_NOOP: return VK_LOGIC_OP_NO_OP;
		case CELL_GCM_XOR: return VK_LOGIC_OP_XOR;
		case CELL_GCM_OR: return VK_LOGIC_OP_OR;
		case CELL_GCM_NOR: return VK_LOGIC_OP_NOR;
		case CELL_GCM_EQUIV: return VK_LOGIC_OP_EQUIVALENT;
		case CELL_GCM_INVERT: return VK_LOGIC_OP_INVERT;
		case CELL_GCM_OR_REVERSE: return VK_LOGIC_OP_OR_REVERSE;
		case CELL_GCM_COPY_INVERTED: return VK_LOGIC_OP_COPY_INVERTED;
		case CELL_GCM_OR_INVERTED: return VK_LOGIC_OP_OR_INVERTED;
		case CELL_GCM_NAND: return VK_LOGIC_OP_NAND;
		default:
			throw EXCEPTION("Unknown logic op 0x%X", op);
		}
	}

	VkBlendFactor get_blend_factor(u16 factor)
	{
		switch (factor)
		{
		case CELL_GCM_ONE: return VK_BLEND_FACTOR_ONE;
		case CELL_GCM_ZERO: return VK_BLEND_FACTOR_ZERO;
		case CELL_GCM_SRC_ALPHA: return VK_BLEND_FACTOR_SRC_ALPHA;
		case CELL_GCM_DST_ALPHA: return VK_BLEND_FACTOR_DST_ALPHA;
		case CELL_GCM_SRC_COLOR: return VK_BLEND_FACTOR_SRC_COLOR;
		case CELL_GCM_DST_COLOR: return VK_BLEND_FACTOR_DST_COLOR;
		case CELL_GCM_CONSTANT_COLOR: return VK_BLEND_FACTOR_CONSTANT_COLOR;
		case CELL_GCM_CONSTANT_ALPHA: return VK_BLEND_FACTOR_CONSTANT_ALPHA;
		case CELL_GCM_ONE_MINUS_SRC_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		case CELL_GCM_ONE_MINUS_DST_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		case CELL_GCM_ONE_MINUS_SRC_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		case CELL_GCM_ONE_MINUS_DST_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		case CELL_GCM_ONE_MINUS_CONSTANT_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
		case CELL_GCM_ONE_MINUS_CONSTANT_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
		default:
			throw EXCEPTION("Unknown blend factor 0x%X", factor);
		}
	};

	VkBlendOp get_blend_op(u16 op)
	{
		switch (op)
		{
		case CELL_GCM_FUNC_ADD: return VK_BLEND_OP_ADD;
		case CELL_GCM_FUNC_SUBTRACT: return VK_BLEND_OP_SUBTRACT;
		case CELL_GCM_FUNC_REVERSE_SUBTRACT: return VK_BLEND_OP_REVERSE_SUBTRACT;
		case CELL_GCM_MIN: return VK_BLEND_OP_MIN;
		case CELL_GCM_MAX: return VK_BLEND_OP_MAX;
		default:
			throw EXCEPTION("Unknown blend op: 0x%X", op);
		}
	}
	
	VkStencilOp get_stencil_op(u32 op)
	{
		switch (op)
		{
		case CELL_GCM_KEEP: return VK_STENCIL_OP_KEEP;
		case CELL_GCM_ZERO: return VK_STENCIL_OP_ZERO;
		case CELL_GCM_REPLACE: return VK_STENCIL_OP_REPLACE;
		case CELL_GCM_INCR: return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
		case CELL_GCM_DECR: return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
		case CELL_GCM_INVERT: return VK_STENCIL_OP_INVERT;
		case CELL_GCM_INCR_WRAP: return VK_STENCIL_OP_INCREMENT_AND_WRAP;
		case CELL_GCM_DECR_WRAP: return VK_STENCIL_OP_DECREMENT_AND_WRAP;
		default:
			throw EXCEPTION("Unknown stencil op: 0x%X", op);
		}
	}
	
	VkFrontFace get_front_face_ccw(u32 ffv)
	{
		switch (ffv)
		{ 
		case CELL_GCM_CW: return VK_FRONT_FACE_CLOCKWISE;
		case CELL_GCM_CCW: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
		default:
			throw EXCEPTION("Unknown front face value: 0x%X", ffv);
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

		VkRenderPassCreateInfo rp_info = {};
		rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		rp_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		rp_info.pAttachments = attachments.data();
		rp_info.subpassCount = 1;
		rp_info.pSubpasses = &subpass;

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
		std::array<VkDescriptorSetLayoutBinding, 35> bindings = {};

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

VKGSRender::VKGSRender() : GSRender(frame_type::Vulkan)
{
	shaders_cache.load(rsx::old_shaders_cache::shader_language::glsl);

	m_thread_context.createInstance("RPCS3");
	m_thread_context.makeCurrentInstance(1);
	m_thread_context.enable_debugging();

#ifdef _WIN32
	HINSTANCE hInstance = NULL;
	HWND hWnd = (HWND)m_frame->handle();

	std::vector<vk::physical_device>& gpus = m_thread_context.enumerateDevices();
	m_swap_chain = m_thread_context.createSwapChain(hInstance, hWnd, gpus[0]);
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
	m_command_buffer.create(m_command_buffer_pool);
	open_command_buffer();

	for (u32 i = 0; i < m_swap_chain->get_swap_image_count(); ++i)
	{
		vk::change_image_layout(m_command_buffer, m_swap_chain->get_swap_chain_image(i),
								VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
								vk::get_image_subresource_range(0, 0, 1, 1, VK_IMAGE_ASPECT_COLOR_BIT));

		VkClearColorValue clear_color{};
		auto range = vk::get_image_subresource_range(0, 0, 1, 1, VK_IMAGE_ASPECT_COLOR_BIT);
		vkCmdClearColorImage(m_command_buffer, m_swap_chain->get_swap_chain_image(i), VK_IMAGE_LAYOUT_GENERAL, &clear_color, 1, &range);
		vk::change_image_layout(m_command_buffer, m_swap_chain->get_swap_chain_image(i),
			VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			vk::get_image_subresource_range(0, 0, 1, 1, VK_IMAGE_ASPECT_COLOR_BIT));

	}


#define RING_BUFFER_SIZE 16 * 1024 * DESCRIPTOR_MAX_DRAW_CALLS
	m_uniform_buffer_ring_info.init(RING_BUFFER_SIZE);
	m_uniform_buffer_ring_info.heap.reset(new vk::buffer(*m_device, RING_BUFFER_SIZE, m_memory_type_mapping.host_visible_coherent, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 0));
	m_index_buffer_ring_info.init(RING_BUFFER_SIZE);
	m_index_buffer_ring_info.heap.reset(new vk::buffer(*m_device, RING_BUFFER_SIZE, m_memory_type_mapping.host_visible_coherent, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 0));
	m_texture_upload_buffer_ring_info.init(8 * RING_BUFFER_SIZE);
	m_texture_upload_buffer_ring_info.heap.reset(new vk::buffer(*m_device, 8 * RING_BUFFER_SIZE, m_memory_type_mapping.host_visible_coherent, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 0));

	m_render_passes = get_precomputed_render_passes(*m_device, m_optimal_tiling_supported_formats);

	std::tie(pipeline_layout, descriptor_layouts) = get_shared_pipeline_layout(*m_device);

	VkDescriptorPoolSize uniform_buffer_pool = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 3 * DESCRIPTOR_MAX_DRAW_CALLS };
	VkDescriptorPoolSize uniform_texel_pool = { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER , 16 * DESCRIPTOR_MAX_DRAW_CALLS };
	VkDescriptorPoolSize texture_pool = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 16 * DESCRIPTOR_MAX_DRAW_CALLS };

	std::vector<VkDescriptorPoolSize> sizes{ uniform_buffer_pool, uniform_texel_pool, texture_pool };

	descriptor_pool.create(*m_device, sizes.data(), static_cast<uint32_t>(sizes.size()));


	null_buffer = std::make_unique<vk::buffer>(*m_device, 32, m_memory_type_mapping.host_visible_coherent, VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, 0);
	null_buffer_view = std::make_unique<vk::buffer_view>(*m_device, null_buffer->value, VK_FORMAT_R32_SFLOAT, 0, 32);

	VkFenceCreateInfo fence_info = {};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	CHECK_RESULT(vkCreateFence(*m_device, &fence_info, nullptr, &m_submit_fence));

	VkSemaphoreCreateInfo semaphore_info = {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	vkCreateSemaphore((*m_device), &semaphore_info, nullptr, &m_present_semaphore);
}

VKGSRender::~VKGSRender()
{
	vkQueueWaitIdle(m_swap_chain->get_present_queue());

	if (m_present_semaphore)
	{
		vkDestroySemaphore((*m_device), m_present_semaphore, nullptr);
		m_present_semaphore = nullptr;
	}

	vk::destroy_global_resources();

	//TODO: Properly destroy shader modules instead of calling clear...
	m_prog_buffer.clear();

	m_index_buffer_ring_info.heap.release();
	m_uniform_buffer_ring_info.heap.release();
	m_attrib_ring_info.heap.release();
	m_texture_upload_buffer_ring_info.heap.release();
	null_buffer.release();
	null_buffer_view.release();
	m_buffer_view_to_clean.clear();
	m_sampler_to_clean.clear();
	m_framebuffer_to_clean.clear();

	for (auto &render_pass : m_render_passes)
		if (render_pass)
			vkDestroyRenderPass(*m_device, render_pass, nullptr);

	m_rtts.destroy();

	vkDestroyPipelineLayout(*m_device, pipeline_layout, nullptr);
	vkDestroyDescriptorSetLayout(*m_device, descriptor_layouts, nullptr);

	descriptor_pool.destroy();

	m_command_buffer.destroy();
	m_command_buffer_pool.destroy();

	m_swap_chain->destroy();

	m_thread_context.close();
	delete m_swap_chain;
}

bool VKGSRender::on_access_violation(u32 address, bool is_writing)
{
	if (is_writing)
		return m_texture_cache.invalidate_address(address);

	return false;
}

void VKGSRender::begin()
{
	rsx::thread::begin();

	//TODO: Fence sync, ring-buffers, etc
	//CHECK_RESULT(vkDeviceWaitIdle((*m_device)));

	//Ease resource pressure if the number of draw calls becomes too high
	if (m_used_descriptors >= DESCRIPTOR_MAX_DRAW_CALLS)
	{
		close_and_submit_command_buffer({}, m_submit_fence);
		CHECK_RESULT(vkWaitForFences((*m_device), 1, &m_submit_fence, VK_TRUE, ~0ULL));

		vkResetDescriptorPool(*m_device, descriptor_pool, 0);
		CHECK_RESULT(vkResetFences(*m_device, 1, &m_submit_fence));
		CHECK_RESULT(vkResetCommandPool(*m_device, m_command_buffer_pool, 0));
		open_command_buffer();

		m_used_descriptors = 0;
		m_uniform_buffer_ring_info.m_get_pos = m_uniform_buffer_ring_info.get_current_put_pos_minus_one();
		m_index_buffer_ring_info.m_get_pos = m_index_buffer_ring_info.get_current_put_pos_minus_one();
		m_attrib_ring_info.m_get_pos = m_attrib_ring_info.get_current_put_pos_minus_one();
		m_texture_upload_buffer_ring_info.m_get_pos = m_texture_upload_buffer_ring_info.get_current_put_pos_minus_one();
	}

	VkDescriptorSetAllocateInfo alloc_info = {};
	alloc_info.descriptorPool = descriptor_pool;
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts = &descriptor_layouts;
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

	VkDescriptorSet new_descriptor_set;
	CHECK_RESULT(vkAllocateDescriptorSets(*m_device, &alloc_info, &new_descriptor_set));

	descriptor_sets = new_descriptor_set;

	init_buffers();

	if (!load_program())
		return;

	u32 line_width = rsx::method_registers[NV4097_SET_LINE_WIDTH];
	float actual_line_width = (line_width >> 3) + (line_width & 7) / 8.f;

	vkCmdSetLineWidth(m_command_buffer, actual_line_width);

	//TODO: Set up other render-state parameters into the program pipeline

	m_draw_calls++;
	m_used_descriptors++;
}

namespace
{
	bool normalize(rsx::vertex_base_type type)
	{
		switch (type)
		{
		case rsx::vertex_base_type::s1:
		case rsx::vertex_base_type::ub:
		case rsx::vertex_base_type::cmp:
			return true;
		case rsx::vertex_base_type::f:
		case rsx::vertex_base_type::sf:
		case rsx::vertex_base_type::ub256:
		case rsx::vertex_base_type::s32k:
			return false;
		}
		throw EXCEPTION("unknown vertex type");
	}
}



void VKGSRender::end()
{
	size_t idx = vk::get_render_pass_location(
		vk::get_compatible_surface_format(m_surface.color_format).first,
		vk::get_compatible_depth_surface_format(m_optimal_tiling_supported_formats, m_surface.depth_format),
		(u8)vk::get_draw_buffers(rsx::to_surface_target(rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET])).size());
	VkRenderPass current_render_pass = m_render_passes[idx];

	for (int i = 0; i < rsx::limits::textures_count; ++i)
	{
		if (m_program->has_uniform("tex" + std::to_string(i)))
		{
			if (!textures[i].enabled())
			{
				m_program->bind_uniform({ vk::null_sampler(), vk::null_image_view(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, "tex" + std::to_string(i), descriptor_sets);
				continue;
			}
			vk::image_view  *texture0 = m_texture_cache.upload_texture(m_command_buffer, textures[i], m_rtts, m_memory_type_mapping, m_texture_upload_buffer_ring_info, m_texture_upload_buffer_ring_info.heap.get());

			VkFilter min_filter;
			VkSamplerMipmapMode mip_mode;
			std::tie(min_filter, mip_mode) = vk::get_min_filter_and_mip(textures[i].min_filter());
			m_sampler_to_clean.push_back(std::make_unique<vk::sampler>(
				*m_device,
				vk::vk_wrap_mode(textures[i].wrap_s()), vk::vk_wrap_mode(textures[i].wrap_t()), vk::vk_wrap_mode(textures[i].wrap_r()),
				!!(textures[i].format() & CELL_GCM_TEXTURE_UN),
				textures[i].bias(), vk::max_aniso(textures[i].max_aniso()), textures[i].min_lod(), textures[i].max_lod(),
				min_filter, vk::get_mag_filter(textures[i].mag_filter()), mip_mode, vk::get_border_color(textures[i].border_color())
				));
			m_program->bind_uniform({ m_sampler_to_clean.back()->value, texture0->value, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, "tex" + std::to_string(i), descriptor_sets);
		}
	}

	VkRenderPassBeginInfo rp_begin = {};
	rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rp_begin.renderPass = current_render_pass;
	rp_begin.framebuffer = m_framebuffer_to_clean.back()->value;
	rp_begin.renderArea.offset.x = 0;
	rp_begin.renderArea.offset.y = 0;
	rp_begin.renderArea.extent.width = m_frame->client_width();
	rp_begin.renderArea.extent.height = m_frame->client_height();

	vkCmdBeginRenderPass(m_command_buffer, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

	auto upload_info = upload_vertex_data();

	vkCmdBindPipeline(m_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_program->pipeline);
	vkCmdBindDescriptorSets(m_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets, 0, nullptr);

	if (!std::get<1>(upload_info))
		vkCmdDraw(m_command_buffer, vertex_draw_count, 1, 0, 0);
	else
	{
		VkIndexType index_type;
		u32 index_count;
		VkDeviceSize offset;
		std::tie(std::ignore, std::ignore, index_count, offset, index_type) = upload_info;

		vkCmdBindIndexBuffer(m_command_buffer, m_index_buffer_ring_info.heap->value, offset, index_type);
		vkCmdDrawIndexed(m_command_buffer, index_count, 1, 0, 0, 0);
	}

	vkCmdEndRenderPass(m_command_buffer);


	rsx::thread::end();
}

void VKGSRender::set_viewport()
{
	u32 viewport_horizontal = rsx::method_registers[NV4097_SET_VIEWPORT_HORIZONTAL];
	u32 viewport_vertical = rsx::method_registers[NV4097_SET_VIEWPORT_VERTICAL];

	u16 viewport_x = viewport_horizontal & 0xffff;
	u16 viewport_y = viewport_vertical & 0xffff;
	u16 viewport_w = viewport_horizontal >> 16;
	u16 viewport_h = viewport_vertical >> 16;

	u32 scissor_horizontal = rsx::method_registers[NV4097_SET_SCISSOR_HORIZONTAL];
	u32 scissor_vertical = rsx::method_registers[NV4097_SET_SCISSOR_VERTICAL];
	u16 scissor_x = scissor_horizontal;
	u16 scissor_w = scissor_horizontal >> 16;
	u16 scissor_y = scissor_vertical;
	u16 scissor_h = scissor_vertical >> 16;

//	u32 shader_window = rsx::method_registers[NV4097_SET_SHADER_WINDOW];
//	rsx::window_origin shader_window_origin = rsx::to_window_origin((shader_window >> 12) & 0xf);

	VkViewport viewport = {};
	viewport.x = viewport_x;
	viewport.y = viewport_y;
	viewport.width = viewport_w;
	viewport.height = viewport_h;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	vkCmdSetViewport(m_command_buffer, 0, 1, &viewport);

	VkRect2D scissor = {};
	scissor.extent.height = scissor_h;
	scissor.extent.width = scissor_w;
	scissor.offset.x = scissor_x;
	scissor.offset.y = scissor_y;

	vkCmdSetScissor(m_command_buffer, 0, 1, &scissor);
}

void VKGSRender::on_init_thread()
{
	GSRender::on_init_thread();
	m_attrib_ring_info.init(8 * RING_BUFFER_SIZE);
	m_attrib_ring_info.heap.reset(new vk::buffer(*m_device, 8 * RING_BUFFER_SIZE, m_memory_type_mapping.host_visible_coherent, VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, 0));
}

void VKGSRender::on_exit()
{
	m_texture_cache.destroy();
}

void VKGSRender::clear_surface(u32 mask)
{
	//TODO: Build clear commands into current renderpass descriptor set
	if (!(mask & 0xF3)) return;
	if (m_current_present_image== 0xFFFF) return;
	if (!rsx::method_registers[NV4097_SET_SURFACE_FORMAT]) return;

	init_buffers();

	float depth_clear = 1.f;
	u32   stencil_clear = 0;

	VkClearValue depth_stencil_clear_values, color_clear_values;
	VkImageSubresourceRange depth_range = vk::get_image_subresource_range(0, 0, 1, 1, 0);

	rsx::surface_depth_format surface_depth_format = rsx::to_surface_depth_format((rsx::method_registers[NV4097_SET_SURFACE_FORMAT] >> 5) & 0x7);

	if (mask & 0x1)
	{
		u32 max_depth_value = get_max_depth_value(surface_depth_format);

		u32 clear_depth = rsx::method_registers[NV4097_SET_ZSTENCIL_CLEAR_VALUE] >> 8;
		float depth_clear = (float)clear_depth / max_depth_value;

		depth_range.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
		depth_stencil_clear_values.depthStencil.depth = depth_clear;
		depth_stencil_clear_values.depthStencil.stencil = stencil_clear;
	}

	if (mask & 0x2)
	{
		u8 clear_stencil = rsx::method_registers[NV4097_SET_ZSTENCIL_CLEAR_VALUE] & 0xff;
		u32 stencil_mask = rsx::method_registers[NV4097_SET_STENCIL_MASK];

		//TODO set stencil mask
		depth_range.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		depth_stencil_clear_values.depthStencil.stencil = stencil_mask;
	}

	if (mask & 0xF0)
	{
		u32 clear_color = rsx::method_registers[NV4097_SET_COLOR_CLEAR_VALUE];
		u8 clear_a = clear_color >> 24;
		u8 clear_r = clear_color >> 16;
		u8 clear_g = clear_color >> 8;
		u8 clear_b = clear_color;

		//TODO set color mask
		/*VkBool32 clear_red = (VkBool32)!!(mask & 0x20);
		VkBool32 clear_green = (VkBool32)!!(mask & 0x40);
		VkBool32 clear_blue = (VkBool32)!!(mask & 0x80);
		VkBool32 clear_alpha = (VkBool32)!!(mask & 0x10);*/

		color_clear_values.color.float32[0] = (float)clear_r / 255;
		color_clear_values.color.float32[1] = (float)clear_g / 255;
		color_clear_values.color.float32[2] = (float)clear_b / 255;
		color_clear_values.color.float32[3] = (float)clear_a / 255;

		for (u32 i = 0; i < m_rtts.m_bound_render_targets.size(); ++i)
		{
			VkImageSubresourceRange range = vk::get_image_subresource_range(0, 0, 1, 1, VK_IMAGE_ASPECT_COLOR_BIT);
			if (std::get<1>(m_rtts.m_bound_render_targets[i]) == nullptr) continue;

			VkImage color_image = std::get<1>(m_rtts.m_bound_render_targets[i])->value;
			change_image_layout(m_command_buffer, color_image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, range);
			vkCmdClearColorImage(m_command_buffer, color_image, VK_IMAGE_LAYOUT_GENERAL, &color_clear_values.color, 1, &range);
			change_image_layout(m_command_buffer, color_image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, range);
		}
	}

	if (mask & 0x3)
	{
		VkImageAspectFlags depth_stencil_aspect = (surface_depth_format == rsx::surface_depth_format::z24s8) ? (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT) : VK_IMAGE_ASPECT_DEPTH_BIT;
		VkImage depth_stencil_image = std::get<1>(m_rtts.m_bound_depth_stencil)->value;
		change_image_layout(m_command_buffer, depth_stencil_image, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, vk::get_image_subresource_range(0, 0, 1, 1, depth_stencil_aspect));
		vkCmdClearDepthStencilImage(m_command_buffer, std::get<1>(m_rtts.m_bound_depth_stencil)->value, VK_IMAGE_LAYOUT_GENERAL, &depth_stencil_clear_values.depthStencil, 1, &depth_range);
		change_image_layout(m_command_buffer, depth_stencil_image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, vk::get_image_subresource_range(0, 0, 1, 1, depth_stencil_aspect));
	}

}

void VKGSRender::sync_at_semaphore_release()
{
	close_and_submit_command_buffer({}, m_submit_fence);
	CHECK_RESULT(vkWaitForFences((*m_device), 1, &m_submit_fence, VK_TRUE, ~0ULL));

	CHECK_RESULT(vkResetFences(*m_device, 1, &m_submit_fence));
	CHECK_RESULT(vkResetCommandPool(*m_device, m_command_buffer_pool, 0));
	open_command_buffer();
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
	RSXVertexProgram vertex_program = get_current_vertex_program();
	RSXFragmentProgram fragment_program = get_current_fragment_program();

	vk::pipeline_props properties = {};


	properties.ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	bool unused;
	properties.ia.topology = vk::get_appropriate_topology(draw_mode, unused);

	if (rsx::method_registers[NV4097_SET_RESTART_INDEX_ENABLE])
	{
		if (rsx::method_registers[NV4097_SET_RESTART_INDEX] != 0xFFFF &&
			rsx::method_registers[NV4097_SET_RESTART_INDEX] != 0xFFFFFFFF)
		{
			LOG_ERROR(RSX, "Custom primitive restart index 0x%X. Should rewrite index buffer with proper value!", rsx::method_registers[NV4097_SET_RESTART_INDEX]);
		}
		properties.ia.primitiveRestartEnable = VK_TRUE;
	}
	else
		properties.ia.primitiveRestartEnable = VK_FALSE;


	for (int i = 0; i < 4; ++i)
	{
		properties.att_state[i].colorWriteMask = 0xf;
		properties.att_state[i].blendEnable = VK_FALSE;
	}

	u32 color_mask = rsx::method_registers[NV4097_SET_COLOR_MASK];
	bool color_mask_b = !!(color_mask & 0xff);
	bool color_mask_g = !!((color_mask >> 8) & 0xff);
	bool color_mask_r = !!((color_mask >> 16) & 0xff);
	bool color_mask_a = !!((color_mask >> 24) & 0xff);

	VkColorComponentFlags mask = 0;
	if (color_mask_a) mask |= VK_COLOR_COMPONENT_A_BIT;
	if (color_mask_b) mask |= VK_COLOR_COMPONENT_B_BIT;
	if (color_mask_g) mask |= VK_COLOR_COMPONENT_G_BIT;
	if (color_mask_r) mask |= VK_COLOR_COMPONENT_R_BIT;

	VkColorComponentFlags color_masks[4] = { mask };

	u8 render_targets[] = { 0, 1, 2, 3 };

	for (u8 idx = 0; idx < m_draw_buffers_count; ++idx)
	{
		properties.att_state[render_targets[idx]].colorWriteMask = mask;
	}

	if (rsx::method_registers[NV4097_SET_BLEND_ENABLE])
	{
		u32 sfactor = rsx::method_registers[NV4097_SET_BLEND_FUNC_SFACTOR];
		u32 dfactor = rsx::method_registers[NV4097_SET_BLEND_FUNC_DFACTOR];

		VkBlendFactor sfactor_rgb = vk::get_blend_factor(sfactor);
		VkBlendFactor sfactor_a = vk::get_blend_factor(sfactor >> 16);
		VkBlendFactor dfactor_rgb = vk::get_blend_factor(dfactor);
		VkBlendFactor dfactor_a = vk::get_blend_factor(dfactor >> 16);

		u32 equation = rsx::method_registers[NV4097_SET_BLEND_EQUATION];
		VkBlendOp equation_rgb = vk::get_blend_op(equation);
		VkBlendOp equation_a = vk::get_blend_op(equation >> 16);

		//TODO: Separate target blending
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
	}
	else
	{
		for (u8 idx = 0; idx < m_draw_buffers_count; ++idx)
		{
			properties.att_state[render_targets[idx]].blendEnable = VK_FALSE;
		}
	}

	if (rsx::method_registers[NV4097_SET_LOGIC_OP_ENABLE])
	{
		properties.cs.logicOpEnable = true;
		properties.cs.logicOp = vk::get_logic_op(rsx::method_registers[NV4097_SET_LOGIC_OP]);
	}

	properties.ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	properties.ds.depthWriteEnable = (!!rsx::method_registers[NV4097_SET_DEPTH_MASK]) ? VK_TRUE : VK_FALSE;

	if (rsx::method_registers[NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE])
	{
		properties.ds.depthBoundsTestEnable = VK_TRUE;
		properties.ds.minDepthBounds = (f32&)rsx::method_registers[NV4097_SET_DEPTH_BOUNDS_MIN];
		properties.ds.maxDepthBounds = (f32&)rsx::method_registers[NV4097_SET_DEPTH_BOUNDS_MAX];
	}
	else
		properties.ds.depthBoundsTestEnable = VK_FALSE;

	if (rsx::method_registers[NV4097_SET_STENCIL_TEST_ENABLE])
	{
		properties.ds.stencilTestEnable = VK_TRUE;
		properties.ds.front.writeMask = rsx::method_registers[NV4097_SET_STENCIL_MASK];
		properties.ds.front.compareMask = rsx::method_registers[NV4097_SET_STENCIL_FUNC_MASK];
		properties.ds.front.reference = rsx::method_registers[NV4097_SET_STENCIL_FUNC_REF];
		properties.ds.front.failOp = vk::get_stencil_op(rsx::method_registers[NV4097_SET_STENCIL_OP_FAIL]);
		properties.ds.front.passOp = vk::get_stencil_op(rsx::method_registers[NV4097_SET_STENCIL_OP_ZPASS]);
		properties.ds.front.depthFailOp = vk::get_stencil_op(rsx::method_registers[NV4097_SET_STENCIL_OP_ZFAIL]);
		properties.ds.front.compareOp = vk::compare_op(rsx::method_registers[NV4097_SET_STENCIL_FUNC]);

		if (rsx::method_registers[NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE])
		{
			properties.ds.back.failOp = vk::get_stencil_op(rsx::method_registers[NV4097_SET_BACK_STENCIL_OP_FAIL]);
			properties.ds.back.passOp = vk::get_stencil_op(rsx::method_registers[NV4097_SET_BACK_STENCIL_OP_ZPASS]);
			properties.ds.back.depthFailOp = vk::get_stencil_op(rsx::method_registers[NV4097_SET_BACK_STENCIL_OP_ZFAIL]);
			properties.ds.back.compareOp = vk::compare_op(rsx::method_registers[NV4097_SET_BACK_STENCIL_FUNC]);
		}
		else
			properties.ds.back = properties.ds.front;
	}
	else
		properties.ds.stencilTestEnable = VK_FALSE;
		
	if (!!rsx::method_registers[NV4097_SET_DEPTH_TEST_ENABLE])
	{
		properties.ds.depthTestEnable = VK_TRUE;
		properties.ds.depthCompareOp = vk::compare_op(rsx::method_registers[NV4097_SET_DEPTH_FUNC]);
	}
	else
		properties.ds.depthTestEnable = VK_FALSE;

	if (!!rsx::method_registers[NV4097_SET_CULL_FACE_ENABLE])
	{
		switch (rsx::method_registers[NV4097_SET_CULL_FACE])
		{
		case CELL_GCM_FRONT:
			properties.rs.cullMode = VK_CULL_MODE_FRONT_BIT;
			break;
		case CELL_GCM_BACK:
			properties.rs.cullMode = VK_CULL_MODE_BACK_BIT;
			break;
		case CELL_GCM_FRONT_AND_BACK:
			properties.rs.cullMode = VK_CULL_MODE_FRONT_AND_BACK;
			break;
		default:
			properties.rs.cullMode = VK_CULL_MODE_NONE;
			break;
		}
	}
	else
		properties.rs.cullMode = VK_CULL_MODE_NONE;
	
	properties.rs.frontFace = vk::get_front_face_ccw(rsx::method_registers[NV4097_SET_FRONT_FACE]);
	
	size_t idx = vk::get_render_pass_location(
		vk::get_compatible_surface_format(m_surface.color_format).first,
		vk::get_compatible_depth_surface_format(m_optimal_tiling_supported_formats, m_surface.depth_format),
		(u8)vk::get_draw_buffers(rsx::to_surface_target(rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET])).size());
	properties.render_pass = m_render_passes[idx];

	properties.num_targets = m_draw_buffers_count;

	//Load current program from buffer
	m_program = m_prog_buffer.getGraphicPipelineState(vertex_program, fragment_program, properties, *m_device, pipeline_layout).get();

	//TODO: Update constant buffers..
	//1. Update scale-offset matrix
	//2. Update vertex constants
	//3. Update fragment constants
	const size_t scale_offset_offset = m_uniform_buffer_ring_info.alloc<256>(256);

	u8 *buf = (u8*)m_uniform_buffer_ring_info.map(scale_offset_offset, 256);

	//TODO: Add case for this in RSXThread
	/**
	 * NOTE: While VK's coord system resembles GLs, the clip volume is no longer symetrical in z
	 * Its like D3D without the flip in y (depending on how you build the spir-v)
	 */
	{
		int clip_w = rsx::method_registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL] >> 16;
		int clip_h = rsx::method_registers[NV4097_SET_SURFACE_CLIP_VERTICAL] >> 16;

		float scale_x = (float&)rsx::method_registers[NV4097_SET_VIEWPORT_SCALE] / (clip_w / 2.f);
		float offset_x = (float&)rsx::method_registers[NV4097_SET_VIEWPORT_OFFSET] - (clip_w / 2.f);
		offset_x /= clip_w / 2.f;

		float scale_y = (float&)rsx::method_registers[NV4097_SET_VIEWPORT_SCALE + 1] / (clip_h / 2.f);
		float offset_y = ((float&)rsx::method_registers[NV4097_SET_VIEWPORT_OFFSET + 1] - (clip_h / 2.f));
		offset_y /= clip_h / 2.f;

		float scale_z = (float&)rsx::method_registers[NV4097_SET_VIEWPORT_SCALE + 2];
		float offset_z = (float&)rsx::method_registers[NV4097_SET_VIEWPORT_OFFSET + 2];

		float one = 1.f;

		stream_vector(buf, (u32&)scale_x, 0, 0, (u32&)offset_x);
		stream_vector((char*)buf + 16, 0, (u32&)scale_y, 0, (u32&)offset_y);
		stream_vector((char*)buf + 32, 0, 0, (u32&)scale_z, (u32&)offset_z);
		stream_vector((char*)buf + 48, 0, 0, 0, (u32&)one);
	}

	u32 is_alpha_tested = !!(rsx::method_registers[NV4097_SET_ALPHA_TEST_ENABLE]);
	u8 alpha_ref_raw = (u8)(rsx::method_registers[NV4097_SET_ALPHA_REF] & 0xFF);
	float alpha_ref = alpha_ref_raw / 255.f;

	memcpy((char*)buf + 64, &rsx::method_registers[NV4097_SET_FOG_PARAMS], sizeof(float));
	memcpy((char*)buf + 68, &rsx::method_registers[NV4097_SET_FOG_PARAMS + 1], sizeof(float));
	memcpy((char*)buf + 72, &is_alpha_tested, sizeof(u32));
	memcpy((char*)buf + 76, &alpha_ref, sizeof(float));
	m_uniform_buffer_ring_info.unmap();

	const size_t vertex_constants_offset = m_uniform_buffer_ring_info.alloc<256>(512 * 4 * sizeof(float));
	buf = (u8*)m_uniform_buffer_ring_info.map(vertex_constants_offset, 512 * 4 * sizeof(float));
	fill_vertex_program_constants_data(buf);
	m_uniform_buffer_ring_info.unmap();

	const size_t fragment_constants_sz = m_prog_buffer.get_fragment_constants_buffer_size(fragment_program);
	const size_t fragment_constants_offset = m_uniform_buffer_ring_info.alloc<256>(fragment_constants_sz);
	buf = (u8*)m_uniform_buffer_ring_info.map(fragment_constants_offset, fragment_constants_sz);
	m_prog_buffer.fill_fragment_constans_buffer({ reinterpret_cast<float*>(buf), gsl::narrow<int>(fragment_constants_sz) }, fragment_program);
	m_uniform_buffer_ring_info.unmap();

	m_program->bind_uniform({ m_uniform_buffer_ring_info.heap->value, scale_offset_offset, 256 }, SCALE_OFFSET_BIND_SLOT, descriptor_sets);
	m_program->bind_uniform({ m_uniform_buffer_ring_info.heap->value, vertex_constants_offset, 512 * 4 * sizeof(float) }, VERTEX_CONSTANT_BUFFERS_BIND_SLOT, descriptor_sets);
	m_program->bind_uniform({ m_uniform_buffer_ring_info.heap->value, fragment_constants_offset, fragment_constants_sz }, FRAGMENT_CONSTANT_BUFFERS_BIND_SLOT, descriptor_sets);

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

void VKGSRender::close_and_submit_command_buffer(const std::vector<VkSemaphore> &semaphores, VkFence fence)
{
	CHECK_RESULT(vkEndCommandBuffer(m_command_buffer));

	VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	VkCommandBuffer cmd = m_command_buffer;

	VkSubmitInfo infos = {};
	infos.commandBufferCount = 1;
	infos.pCommandBuffers = &cmd;
	infos.pWaitDstStageMask = &pipe_stage_flags;
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
	CHECK_RESULT(vkBeginCommandBuffer(m_command_buffer, &begin_infos));
}


void VKGSRender::prepare_rtts()
{
	u32 surface_format = rsx::method_registers[NV4097_SET_SURFACE_FORMAT];

	if (!m_rtts_dirty)
		return;

	m_rtts_dirty = false;

	if (m_surface.format != surface_format)
		m_surface.unpack(surface_format);

	u32 clip_horizontal = rsx::method_registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL];
	u32 clip_vertical = rsx::method_registers[NV4097_SET_SURFACE_CLIP_VERTICAL];

	u32 clip_width = clip_horizontal >> 16;
	u32 clip_height = clip_vertical >> 16;
	u32 clip_x = clip_horizontal;
	u32 clip_y = clip_vertical;

	m_rtts.prepare_render_target(&m_command_buffer,
		surface_format,
		clip_horizontal, clip_vertical,
		rsx::to_surface_target(rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET]),
		get_color_surface_addresses(), get_zeta_surface_address(),
		(*m_device), &m_command_buffer, m_optimal_tiling_supported_formats, m_memory_type_mapping);

	//Bind created rtts as current fbo...
	std::vector<u8> draw_buffers = vk::get_draw_buffers(rsx::to_surface_target(rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET]));

	std::vector<std::unique_ptr<vk::image_view>> fbo_images;

	for (u8 index: draw_buffers)
	{
		vk::image *raw = std::get<1>(m_rtts.m_bound_render_targets[index]);

		VkImageSubresourceRange subres = {};
		subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subres.baseArrayLayer = 0;
		subres.baseMipLevel = 0;
		subres.layerCount = 1;
		subres.levelCount = 1;

		fbo_images.push_back(std::make_unique<vk::image_view>(*m_device, raw->value, VK_IMAGE_VIEW_TYPE_2D, raw->info.format, vk::default_component_map(), subres));
	}

	m_draw_buffers_count = static_cast<u32>(fbo_images.size());

	if (std::get<1>(m_rtts.m_bound_depth_stencil) != nullptr)
	{
		vk::image *raw = (std::get<1>(m_rtts.m_bound_depth_stencil));

		VkImageSubresourceRange subres = {};
		subres.aspectMask = (m_surface.depth_format == rsx::surface_depth_format::z24s8) ? (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT) : VK_IMAGE_ASPECT_DEPTH_BIT;
		subres.baseArrayLayer = 0;
		subres.baseMipLevel = 0;
		subres.layerCount = 1;
		subres.levelCount = 1;

		fbo_images.push_back(std::make_unique<vk::image_view>(*m_device, raw->value, VK_IMAGE_VIEW_TYPE_2D, raw->info.format, vk::default_component_map(), subres));
	}

	size_t idx = vk::get_render_pass_location(vk::get_compatible_surface_format(m_surface.color_format).first, vk::get_compatible_depth_surface_format(m_optimal_tiling_supported_formats, m_surface.depth_format), (u8)draw_buffers.size());
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

		VkSwapchainKHR swap_chain = (VkSwapchainKHR)(*m_swap_chain);

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
			vk::copy_scaled_image(m_command_buffer, image_to_flip, target_image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				buffer_width, buffer_height, aspect_ratio.width, aspect_ratio.height, 1, VK_IMAGE_ASPECT_COLOR_BIT);
		}
		else
		{
			//No draw call was issued!
			VkImageSubresourceRange range = vk::get_image_subresource_range(0, 0, 1, 1, VK_IMAGE_ASPECT_COLOR_BIT);
			VkClearColorValue clear_black = { 0 };
			vk::change_image_layout(m_command_buffer, m_swap_chain->get_swap_chain_image(m_current_present_image), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_GENERAL, range);
			vkCmdClearColorImage(m_command_buffer, m_swap_chain->get_swap_chain_image(m_current_present_image), VK_IMAGE_LAYOUT_GENERAL, &clear_black, 1, &range);
			vk::change_image_layout(m_command_buffer, m_swap_chain->get_swap_chain_image(m_current_present_image), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, range);
		}

		close_and_submit_command_buffer({ m_present_semaphore }, m_submit_fence);
		CHECK_RESULT(vkWaitForFences((*m_device), 1, &m_submit_fence, VK_TRUE, ~0ULL));

		VkPresentInfoKHR present = {};
		present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present.pNext = nullptr;
		present.swapchainCount = 1;
		present.pSwapchains = &swap_chain;
		present.pImageIndices = &m_current_present_image;
		CHECK_RESULT(m_swap_chain->queuePresentKHR(m_swap_chain->get_present_queue(), &present));
	}
	else
	{
		/**
		* Since we are about to destroy the old swapchain and its images, we just discard the commandbuffer.
		* Waiting for the commands to process does not work reliably as the fence can be signaled before swap images are released
		* and there are no explicit methods to ensure that the presentation engine is not using the images at all.
		*/

		CHECK_RESULT(vkEndCommandBuffer(m_command_buffer));

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
		CHECK_RESULT(vkResetCommandPool(*m_device, m_command_buffer_pool, 0));
		open_command_buffer();

		for (u32 i = 0; i < m_swap_chain->get_swap_image_count(); ++i)
		{
			vk::change_image_layout(m_command_buffer, m_swap_chain->get_swap_chain_image(i),
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
				vk::get_image_subresource_range(0, 0, 1, 1, VK_IMAGE_ASPECT_COLOR_BIT));

			VkClearColorValue clear_color{};
			auto range = vk::get_image_subresource_range(0, 0, 1, 1, VK_IMAGE_ASPECT_COLOR_BIT);
			vkCmdClearColorImage(m_command_buffer, m_swap_chain->get_swap_chain_image(i), VK_IMAGE_LAYOUT_GENERAL, &clear_color, 1, &range);
			vk::change_image_layout(m_command_buffer, m_swap_chain->get_swap_chain_image(i),
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				vk::get_image_subresource_range(0, 0, 1, 1, VK_IMAGE_ASPECT_COLOR_BIT));
		}

		//Flush the command buffer
		close_and_submit_command_buffer({}, resize_fence);
		CHECK_RESULT(vkWaitForFences((*m_device), 1, &resize_fence, VK_TRUE, UINT64_MAX));
		vkDestroyFence((*m_device), resize_fence, nullptr);
	}

	m_uniform_buffer_ring_info.m_get_pos = m_uniform_buffer_ring_info.get_current_put_pos_minus_one();
	m_index_buffer_ring_info.m_get_pos = m_index_buffer_ring_info.get_current_put_pos_minus_one();
	m_attrib_ring_info.m_get_pos = m_attrib_ring_info.get_current_put_pos_minus_one();
	m_texture_upload_buffer_ring_info.m_get_pos = m_texture_upload_buffer_ring_info.get_current_put_pos_minus_one();

	//Feed back damaged resources to the main texture cache for management...
//	m_texture_cache.merge_dirty_textures(m_rtts.invalidated_resources);
	m_rtts.invalidated_resources.clear();
	m_texture_cache.flush();

	m_buffer_view_to_clean.clear();
	m_sampler_to_clean.clear();
	m_framebuffer_to_clean.clear();

	vkResetDescriptorPool(*m_device, descriptor_pool, 0);
	CHECK_RESULT(vkResetFences(*m_device, 1, &m_submit_fence));
	CHECK_RESULT(vkResetCommandPool(*m_device, m_command_buffer_pool, 0));
	open_command_buffer();

	m_draw_calls = 0;
	m_used_descriptors = 0;
	m_frame->flip(m_context);
}
