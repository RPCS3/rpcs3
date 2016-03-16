#include "stdafx.h"
#include "Utilities/rPlatform.h" // only for rImage
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/state.h"
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
		case CELL_GCM_GREATER:
			return VK_COMPARE_OP_GREATER;
		case CELL_GCM_LESS:
			return VK_COMPARE_OP_LESS;
		case CELL_GCM_LEQUAL:
			return VK_COMPARE_OP_LESS_OR_EQUAL;
		case CELL_GCM_GEQUAL:
			return VK_COMPARE_OP_EQUAL;
		case CELL_GCM_EQUAL:
			return VK_COMPARE_OP_EQUAL;
		case CELL_GCM_ALWAYS:
			return VK_COMPARE_OP_ALWAYS;
		default:
			throw EXCEPTION("Unsupported compare op: 0x%X", gl_name);
		}
	}

	VkFormat get_compatible_surface_format(rsx::surface_color_format color_format)
	{
		switch (color_format)
		{
		case rsx::surface_color_format::r5g6b5:
			return VK_FORMAT_R5G6B5_UNORM_PACK16;

		case rsx::surface_color_format::a8r8g8b8:
			return VK_FORMAT_B8G8R8A8_UNORM;

		case rsx::surface_color_format::x8r8g8b8_o8r8g8b8:
			LOG_ERROR(RSX, "Format 0x%X may be buggy.", color_format);
			return VK_FORMAT_B8G8R8A8_UNORM;

		case rsx::surface_color_format::w16z16y16x16:
			return VK_FORMAT_R16G16B16A16_SFLOAT;

		case rsx::surface_color_format::w32z32y32x32:
			return VK_FORMAT_R32G32B32A32_SFLOAT;

		case rsx::surface_color_format::b8:
		case rsx::surface_color_format::x1r5g5b5_o1r5g5b5:
		case rsx::surface_color_format::x1r5g5b5_z1r5g5b5:
		case rsx::surface_color_format::x8r8g8b8_z8r8g8b8:
		case rsx::surface_color_format::g8b8:
		case rsx::surface_color_format::x32:
		case rsx::surface_color_format::x8b8g8r8_o8b8g8r8:
		case rsx::surface_color_format::x8b8g8r8_z8b8g8r8:
		case rsx::surface_color_format::a8b8g8r8:
		default:
			LOG_ERROR(RSX, "Surface color buffer: Unsupported surface color format (0x%x)", color_format);
			return VK_FORMAT_B8G8R8A8_UNORM;
		}
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
}

VKGSRender::VKGSRender() : GSRender(frame_type::Vulkan)
{
	shaders_cache.load(rsx::shader_language::glsl);

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

	m_swap_chain->init_swapchain(m_frame->client_size().width, m_frame->client_size().height);

	//create command buffer...
	m_command_buffer_pool.create((*m_device));
	m_command_buffer.create(m_command_buffer_pool);

	VkCommandBufferInheritanceInfo inheritance_info = {};
	inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;

	VkCommandBufferBeginInfo begin_infos = {};
	begin_infos.pInheritanceInfo = &inheritance_info;
	begin_infos.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	CHECK_RESULT(vkBeginCommandBuffer(m_command_buffer, &begin_infos));

	for (u32 i = 0; i < m_swap_chain->get_swap_image_count(); ++i)
	{
		vk::change_image_layout(m_command_buffer, m_swap_chain->get_swap_chain_image(i),
								VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
								VK_IMAGE_ASPECT_COLOR_BIT);

		VkClearColorValue clear_color{};
		auto range = vk::default_image_subresource_range();
		vkCmdClearColorImage(m_command_buffer, m_swap_chain->get_swap_chain_image(i), VK_IMAGE_LAYOUT_GENERAL, &clear_color, 1, &range);
		vk::change_image_layout(m_command_buffer, m_swap_chain->get_swap_chain_image(i),
			VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			VK_IMAGE_ASPECT_COLOR_BIT);

	}
	
	CHECK_RESULT(vkEndCommandBuffer(m_command_buffer));
	execute_command_buffer(false);


#define RING_BUFFER_SIZE 16 * 1024 * 1024
	m_uniform_buffer_ring_info.init(RING_BUFFER_SIZE);
	m_uniform_buffer.reset(new vk::buffer(*m_device, RING_BUFFER_SIZE, m_memory_type_mapping.host_visible_coherent, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 0));
	m_index_buffer_ring_info.init(RING_BUFFER_SIZE);
	m_index_buffer.reset(new vk::buffer(*m_device, RING_BUFFER_SIZE, m_memory_type_mapping.host_visible_coherent, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 0));
}

VKGSRender::~VKGSRender()
{
	if (m_submit_fence)
	{
		vkWaitForFences((*m_device), 1, &m_submit_fence, VK_TRUE, 1000000L);
		vkDestroyFence((*m_device), m_submit_fence, nullptr);
		m_submit_fence = nullptr;
	}

	if (m_present_semaphore)
	{
		vkDestroySemaphore((*m_device), m_present_semaphore, nullptr);
		m_present_semaphore = nullptr;
	}

	vk::destroy_global_resources();

	//TODO: Properly destroy shader modules instead of calling clear...
	m_prog_buffer.clear();

	if (m_render_pass)
		destroy_render_pass();

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

	if (!load_program())
		return;

	if (!recording)
		begin_command_buffer_recording();

	init_buffers();

	m_program->set_draw_buffer_count(m_draw_buffers_count);

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
	m_program->set_color_mask(m_draw_buffers_count, render_targets, color_masks);

	//TODO stencil mask
	m_program->set_depth_write_mask(rsx::method_registers[NV4097_SET_DEPTH_MASK]);

	if (rsx::method_registers[NV4097_SET_DEPTH_TEST_ENABLE])
	{
		m_program->set_depth_test_enable(VK_TRUE);
		m_program->set_depth_compare_op(vk::compare_op(rsx::method_registers[NV4097_SET_DEPTH_FUNC]));
	}
	else
		m_program->set_depth_test_enable(VK_FALSE);

	if (rsx::method_registers[NV4097_SET_BLEND_ENABLE])
	{
		u32 sfactor = rsx::method_registers[NV4097_SET_BLEND_FUNC_SFACTOR];
		u32 dfactor = rsx::method_registers[NV4097_SET_BLEND_FUNC_DFACTOR];

		VkBlendFactor sfactor_rgb = vk::get_blend_factor(sfactor);
		VkBlendFactor sfactor_a = vk::get_blend_factor(sfactor >> 16);
		VkBlendFactor dfactor_rgb = vk::get_blend_factor(dfactor);
		VkBlendFactor dfactor_a = vk::get_blend_factor(dfactor >> 16);

		//TODO: Separate target blending

		VkBool32 blend_state = VK_TRUE;

		m_program->set_blend_state(m_draw_buffers_count, render_targets, blend_state);
		m_program->set_blend_func(m_draw_buffers_count, render_targets, sfactor_rgb, dfactor_rgb, sfactor_a, dfactor_a);

		u32 equation = rsx::method_registers[NV4097_SET_BLEND_EQUATION];
		VkBlendOp equation_rgb = vk::get_blend_op(equation);
		VkBlendOp equation_a = vk::get_blend_op(equation >> 16);

		m_program->set_blend_op(m_draw_buffers_count, render_targets, equation_rgb, equation_a);
	}
	else
	{
		VkBool32 blend_state = VK_FALSE;
		m_program->set_blend_state(m_draw_buffers_count, render_targets, blend_state);
	}

	if (rsx::method_registers[NV4097_SET_RESTART_INDEX_ENABLE])
	{
		if (rsx::method_registers[NV4097_SET_RESTART_INDEX] != 0xFFFF &&
			rsx::method_registers[NV4097_SET_RESTART_INDEX] != 0xFFFFFFFF)
		{
			LOG_ERROR(RSX, "Custom primitive restart index 0x%X. Should rewrite index buffer with proper value!", rsx::method_registers[NV4097_SET_RESTART_INDEX]);
		}

		LOG_ERROR(RSX, "Primitive restart enabled!");
		m_program->set_primitive_restart(VK_TRUE);
	}
	else
		m_program->set_primitive_restart(VK_FALSE);

	u32 line_width = rsx::method_registers[NV4097_SET_LINE_WIDTH];
	float actual_line_width = (line_width >> 3) + (line_width & 7) / 8.f;
	
	vkCmdSetLineWidth(m_command_buffer, actual_line_width);

	//TODO: Set up other render-state parameters into the program pipeline

	m_draw_calls++;
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
	VkRenderPassBeginInfo rp_begin = {};
	rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rp_begin.renderPass = m_render_pass;
	rp_begin.framebuffer = m_framebuffer;
	rp_begin.renderArea.offset.x = 0;
	rp_begin.renderArea.offset.y = 0;
	rp_begin.renderArea.extent.width = m_frame->client_size().width;
	rp_begin.renderArea.extent.height = m_frame->client_size().height;

	vkCmdBeginRenderPass(m_command_buffer, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

	vk::texture *texture0 = nullptr;
	for (int i = 0; i < rsx::limits::textures_count; ++i)
	{
		if (m_program->has_uniform(vk::glsl::glsl_fragment_program, "tex" + std::to_string(i)))
		{
			if (!textures[i].enabled())
			{
				m_program->bind_uniform(vk::glsl::glsl_fragment_program, "tex" + std::to_string(i));
				continue;
			}

			vk::texture &tex = (texture0)? (*texture0): m_texture_cache.upload_texture(m_command_buffer, textures[i], m_rtts);
			m_program->bind_uniform(vk::glsl::glsl_fragment_program, "tex" + std::to_string(i), tex);
			texture0 = &tex;
		}
	}

	auto upload_info = upload_vertex_data();

	m_program->set_primitive_topology(std::get<0>(upload_info));
	m_program->use(m_command_buffer, m_render_pass, 0);
	
	if (!std::get<1>(upload_info))
		vkCmdDraw(m_command_buffer, vertex_draw_count, 1, 0, 0);
	else
	{
		VkIndexType index_type;
		u32 index_count;
		VkDeviceSize offset;
		std::tie(std::ignore, std::ignore, index_count, offset, index_type) = upload_info;

		vkCmdBindIndexBuffer(m_command_buffer, m_index_buffer->value, offset, index_type);
		vkCmdDrawIndexed(m_command_buffer, index_count, 1, 0, 0, 0);
	}

	vkCmdEndRenderPass(m_command_buffer);

	m_texture_cache.flush(m_command_buffer);

	end_command_buffer_recording();
	execute_command_buffer(false);

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
	m_attrib_buffers.reset(new vk::buffer(*m_device, 8 * RING_BUFFER_SIZE, m_memory_type_mapping.host_visible_coherent, VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, 0));
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

	bool was_recording = recording;

	if (!was_recording)
		begin_command_buffer_recording();

	init_buffers();

	float depth_clear = 1.f;
	u32   stencil_clear = 0;

	VkClearValue depth_stencil_clear_values, color_clear_values;
	VkImageSubresourceRange depth_range = vk::default_image_subresource_range();
	depth_range.aspectMask = 0;

	if (mask & 0x1)
	{
		rsx::surface_depth_format surface_depth_format = rsx::to_surface_depth_format((rsx::method_registers[NV4097_SET_SURFACE_FORMAT] >> 5) & 0x7);
		u32 max_depth_value = get_max_depth_value(surface_depth_format);

		u32 clear_depth = rsx::method_registers[NV4097_SET_ZSTENCIL_CLEAR_VALUE] >> 8;
		float depth_clear = (float)clear_depth / max_depth_value;

		depth_range.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
		depth_stencil_clear_values.depthStencil.depth = depth_clear;
		depth_stencil_clear_values.depthStencil.stencil = stencil_clear;
	}

/*	if (mask & 0x2)
	{
		u8 clear_stencil = rsx::method_registers[NV4097_SET_ZSTENCIL_CLEAR_VALUE] & 0xff;
		u32 stencil_mask = rsx::method_registers[NV4097_SET_STENCIL_MASK];

		//TODO set stencil mask
		depth_range.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		depth_stencil_clear_values.depthStencil.stencil = stencil_mask;
	}*/

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

		VkImageSubresourceRange range = vk::default_image_subresource_range();

		for (u32 i = 0; i < m_rtts.m_bound_render_targets.size(); ++i)
		{
			if (std::get<1>(m_rtts.m_bound_render_targets[i]) == nullptr) continue;

			VkImage color_image = (*std::get<1>(m_rtts.m_bound_render_targets[i]));
			VkImageLayout old_layout = std::get<1>(m_rtts.m_bound_render_targets[i])->get_layout();
			std::get<1>(m_rtts.m_bound_render_targets[i])->change_layout(m_command_buffer, VK_IMAGE_LAYOUT_GENERAL);
			
			vkCmdClearColorImage(m_command_buffer, color_image, VK_IMAGE_LAYOUT_GENERAL, &color_clear_values.color, 1, &range);
			std::get<1>(m_rtts.m_bound_render_targets[i])->change_layout(m_command_buffer, old_layout);
		}
	}

	if (mask & 0x3)
	{
		VkImageLayout old_layout = std::get<1>(m_rtts.m_bound_depth_stencil)->get_layout();
		std::get<1>(m_rtts.m_bound_depth_stencil)->change_layout(m_command_buffer, VK_IMAGE_LAYOUT_GENERAL);

		vkCmdClearDepthStencilImage(m_command_buffer, (*std::get<1>(m_rtts.m_bound_depth_stencil)), VK_IMAGE_LAYOUT_GENERAL, &depth_stencil_clear_values.depthStencil, 1, &depth_range);
		std::get<1>(m_rtts.m_bound_depth_stencil)->change_layout(m_command_buffer, old_layout);
	}

	if (!was_recording)
	{
		end_command_buffer_recording();
		execute_command_buffer(false);
	}

	recording = was_recording;
}

bool VKGSRender::do_method(u32 cmd, u32 arg)
{
	switch (cmd)
	{
	case NV4097_CLEAR_SURFACE:
		clear_surface(arg);
		return true;
	default:
		return false;
	}
}

void VKGSRender::init_render_pass(VkFormat surface_format, VkFormat depth_format, u8 num_draw_buffers, u8 *draw_buffers)
{
	//TODO: Create buffers as requested by the game. Render to swapchain for now..
	/* Describe a render pass and framebuffer attachments */
	std::array<VkAttachmentDescription, 2> attachments;

	attachments[0].format = surface_format;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;							//Set to clear removes warnings about empty contents after flip; overwrites previous calls
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;	//PRESENT_SRC_KHR??
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	attachments[1].format = depth_format;								/* Depth buffer format. Should be more elegant than this */
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference template_color_reference = {};
	template_color_reference.attachment = VK_ATTACHMENT_UNUSED;
	template_color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_reference = {};
	depth_reference.attachment = num_draw_buffers;
	depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	//Fill in draw_buffers information...
	std::array<VkAttachmentDescription, 4> real_attachments;
	std::array<VkAttachmentReference, 4> color_references;

	for (int i = 0; i < num_draw_buffers; ++i)
	{
		real_attachments[i] = attachments[0];
		
		color_references[i] = template_color_reference;
		color_references[i].attachment = (draw_buffers)? draw_buffers[i]: i;
	}

	real_attachments[num_draw_buffers] = attachments[1];

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = num_draw_buffers;
	subpass.pColorAttachments = num_draw_buffers? color_references.data() : nullptr;
	subpass.pDepthStencilAttachment = &depth_reference;

	VkRenderPassCreateInfo rp_info = {};
	rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	rp_info.attachmentCount = num_draw_buffers + 1;
	rp_info.pAttachments = real_attachments.data();
	rp_info.subpassCount = 1;
	rp_info.pSubpasses = &subpass;

	CHECK_RESULT(vkCreateRenderPass((*m_device), &rp_info, NULL, &m_render_pass));
}

void VKGSRender::destroy_render_pass()
{
	vkDestroyRenderPass((*m_device), m_render_pass, nullptr);
	m_render_pass = nullptr;
}

bool VKGSRender::load_program()
{
	RSXVertexProgram vertex_program = get_current_vertex_program();
	RSXFragmentProgram fragment_program = get_current_fragment_program();

	//Load current program from buffer
	m_program = &m_prog_buffer.getGraphicPipelineState(vertex_program, fragment_program, nullptr);

	//TODO: Update constant buffers..
	//1. Update scale-offset matrix
	//2. Update vertex constants
	//3. Update fragment constants
	const size_t scale_offset_offset = m_uniform_buffer_ring_info.alloc<256>(256);

	u8 *buf = (u8*)m_uniform_buffer->map(scale_offset_offset, 256);

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

	memset((char*)buf+64, 0, 8);
	memcpy((char*)buf + 64, &rsx::method_registers[NV4097_SET_FOG_PARAMS], sizeof(float));
	memcpy((char*)buf + 68, &rsx::method_registers[NV4097_SET_FOG_PARAMS + 1], sizeof(float));
	m_uniform_buffer->unmap();

	const size_t vertex_constants_offset = m_uniform_buffer_ring_info.alloc<256>(512 * 4 * sizeof(float));
	buf = (u8*)m_uniform_buffer->map(vertex_constants_offset, 512 * 4 * sizeof(float));
	fill_vertex_program_constants_data(buf);
	m_uniform_buffer->unmap();

	const size_t fragment_constants_sz = m_prog_buffer.get_fragment_constants_buffer_size(fragment_program);
	const size_t fragment_constants_offset = m_uniform_buffer_ring_info.alloc<256>(fragment_constants_sz);
	buf = (u8*)m_uniform_buffer->map(fragment_constants_offset, fragment_constants_sz);
	m_prog_buffer.fill_fragment_constans_buffer({ reinterpret_cast<float*>(buf), gsl::narrow<int>(fragment_constants_sz) }, fragment_program);
	m_uniform_buffer->unmap();

	m_program->bind_uniform(vk::glsl::glsl_vertex_program, "ScaleOffsetBuffer", m_uniform_buffer->value, scale_offset_offset, 256);
	m_program->bind_uniform(vk::glsl::glsl_vertex_program, "VertexConstantsBuffer", m_uniform_buffer->value, vertex_constants_offset, 512 * 4 * sizeof(float));
	m_program->bind_uniform(vk::glsl::glsl_fragment_program, "ScaleOffsetBuffer", m_uniform_buffer->value, scale_offset_offset, 256);
	m_program->bind_uniform(vk::glsl::glsl_fragment_program, "FragmentConstantsBuffer", m_uniform_buffer->value, fragment_constants_offset, fragment_constants_sz);

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
	if (dirty_frame)
	{
		//Prepare surface for new frame
		VkSemaphoreCreateInfo semaphore_info = {};
		semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		vkCreateSemaphore((*m_device), &semaphore_info, nullptr, &m_present_semaphore);

		VkFence nullFence = VK_NULL_HANDLE;
		CHECK_RESULT(vkAcquireNextImageKHR((*m_device), (*m_swap_chain), 0, m_present_semaphore, nullFence, &m_current_present_image));

		dirty_frame = false;
	}

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

void VKGSRender::begin_command_buffer_recording()
{
	VkCommandBufferInheritanceInfo inheritance_info = {};
	inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;

	VkCommandBufferBeginInfo begin_infos = {};
	begin_infos.pInheritanceInfo = &inheritance_info;
	begin_infos.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (m_submit_fence)
	{
		vkWaitForFences(*m_device, 1, &m_submit_fence, VK_TRUE, ~0ULL);
		vkDestroyFence(*m_device, m_submit_fence, nullptr);
		m_submit_fence = nullptr;

		CHECK_RESULT(vkResetCommandBuffer(m_command_buffer, 0));
	}

	CHECK_RESULT(vkBeginCommandBuffer(m_command_buffer, &begin_infos));
	recording = true;
}

void VKGSRender::end_command_buffer_recording()
{
	recording = false;
	CHECK_RESULT(vkEndCommandBuffer(m_command_buffer));
}

void VKGSRender::prepare_rtts()
{
	u32 surface_format = rsx::method_registers[NV4097_SET_SURFACE_FORMAT];

	if (!m_rtts_dirty)
		return;

	m_rtts_dirty = false;
	bool reconfigure_render_pass = true;

	if (m_surface.format != surface_format)
	{
		m_surface.unpack(surface_format);
		reconfigure_render_pass = true;
	}

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
		(*m_device), &m_command_buffer, m_optimal_tiling_supported_formats);

	//Bind created rtts as current fbo...
	std::vector<u8> draw_buffers = vk::get_draw_buffers(rsx::to_surface_target(rsx::method_registers[NV4097_SET_SURFACE_COLOR_TARGET]));

	m_framebuffer.destroy();
	std::vector<VkImageView> fbo_images;

	for (u8 index: draw_buffers)
	{
		vk::texture *raw = std::get<1>(m_rtts.m_bound_render_targets[index]);
		VkImageView as_image = (*raw);
		fbo_images.push_back(as_image);
	}

	if (std::get<1>(m_rtts.m_bound_depth_stencil) != nullptr)
	{
		vk::texture *raw = (std::get<1>(m_rtts.m_bound_depth_stencil));
		VkImageView depth_image = (*raw);
		fbo_images.push_back(depth_image);
	}

	if (reconfigure_render_pass)
	{
		//Create render pass with draw_buffers information
		//Somewhat simliar to glDrawBuffers

		if (m_render_pass)
			destroy_render_pass();

		init_render_pass(vk::get_compatible_surface_format(m_surface.color_format),
			vk::get_compatible_depth_surface_format(m_optimal_tiling_supported_formats, m_surface.depth_format),
			(u8)draw_buffers.size(),
			draw_buffers.data());
	}

	m_framebuffer.create((*m_device), m_render_pass, fbo_images.data(), fbo_images.size(),
						clip_width, clip_height);

	m_draw_buffers_count = draw_buffers.size();
}

void VKGSRender::execute_command_buffer(bool wait)
{
	if (recording)
		throw EXCEPTION("execute_command_buffer called before end_command_buffer_recording()!");

	if (m_submit_fence)
		throw EXCEPTION("Synchronization deadlock!");

	VkFenceCreateInfo fence_info = {};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	CHECK_RESULT(vkCreateFence(*m_device, &fence_info, nullptr, &m_submit_fence));

	VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	VkCommandBuffer cmd = m_command_buffer;

	VkSubmitInfo infos = {};
	infos.commandBufferCount = 1;
	infos.pCommandBuffers = &cmd;
	infos.pWaitDstStageMask = &pipe_stage_flags;
	infos.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	CHECK_RESULT(vkQueueSubmit(m_swap_chain->get_present_queue(), 1, &infos, m_submit_fence));
	CHECK_RESULT(vkQueueWaitIdle(m_swap_chain->get_present_queue()));
}

void VKGSRender::flip(int buffer)
{
	//LOG_NOTICE(Log::RSX, "flip(%d)", buffer);
	u32 buffer_width = gcm_buffers[buffer].width;
	u32 buffer_height = gcm_buffers[buffer].height;
	u32 buffer_pitch = gcm_buffers[buffer].pitch;

	rsx::tiled_region buffer_region = get_tiled_address(gcm_buffers[buffer].offset, CELL_GCM_LOCATION_LOCAL);

	areai screen_area = coordi({}, { (int)buffer_width, (int)buffer_height });

	coordi aspect_ratio;
	if (1) //enable aspect ratio
	{
		sizei csize = m_frame->client_size();
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
	}
	else
	{
		aspect_ratio.size = m_frame->client_size();
	}

	VkSwapchainKHR swap_chain = (VkSwapchainKHR)(*m_swap_chain);
	uint32_t next_image_temp = 0;

	VkPresentInfoKHR present = {};
	present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present.pNext = nullptr;
	present.swapchainCount = 1;
	present.pSwapchains = &swap_chain;
	present.pImageIndices = &m_current_present_image;
	present.pWaitSemaphores = &m_present_semaphore;
	present.waitSemaphoreCount = 1;

	if (m_render_pass)
	{
		begin_command_buffer_recording();

		if (m_present_semaphore)
		{
			//Blit contents to screen..
			VkImage image_to_flip = nullptr;

			if (std::get<1>(m_rtts.m_bound_render_targets[0]) != nullptr)
				image_to_flip = (*std::get<1>(m_rtts.m_bound_render_targets[0]));
			else
				image_to_flip = (*std::get<1>(m_rtts.m_bound_render_targets[1]));

			VkImage target_image = m_swap_chain->get_swap_chain_image(m_current_present_image);
			vk::copy_scaled_image(m_command_buffer, image_to_flip, target_image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
									buffer_width, buffer_height, aspect_ratio.width, aspect_ratio.height, 1, VK_IMAGE_ASPECT_COLOR_BIT);
		}
		else
		{
			//No draw call was issued!
			//TODO: Properly clear the background to rsx value
			m_swap_chain->acquireNextImageKHR((*m_device), (*m_swap_chain), ~0ULL, VK_NULL_HANDLE, VK_NULL_HANDLE, &next_image_temp);

			VkImageSubresourceRange range = vk::default_image_subresource_range();
			VkClearColorValue clear_black = { 0 };
			vkCmdClearColorImage(m_command_buffer, m_swap_chain->get_swap_chain_image(next_image_temp), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, &clear_black, 1, &range);
			
			present.pImageIndices = &next_image_temp;
			present.waitSemaphoreCount = 0;
		}

		end_command_buffer_recording();
		execute_command_buffer(false);

		//Check if anything is waiting in queue and wait for it if possible..
		if (m_submit_fence)
		{
			CHECK_RESULT(vkWaitForFences((*m_device), 1, &m_submit_fence, VK_TRUE, ~0ULL));

			vkDestroyFence((*m_device), m_submit_fence, nullptr);
			m_submit_fence = nullptr;

			CHECK_RESULT(vkResetCommandBuffer(m_command_buffer, 0));
		}

		CHECK_RESULT(m_swap_chain->queuePresentKHR(m_swap_chain->get_present_queue(), &present));
		CHECK_RESULT(vkQueueWaitIdle(m_swap_chain->get_present_queue()));

		m_uniform_buffer_ring_info.m_get_pos = m_uniform_buffer_ring_info.get_current_put_pos_minus_one();
		m_index_buffer_ring_info.m_get_pos = m_index_buffer_ring_info.get_current_put_pos_minus_one();
		m_attrib_ring_info.m_get_pos = m_attrib_ring_info.get_current_put_pos_minus_one();
		if (m_present_semaphore)
		{
			vkDestroySemaphore((*m_device), m_present_semaphore, nullptr);
			m_present_semaphore = nullptr;
		}
	}

	//Feed back damaged resources to the main texture cache for management...
	m_texture_cache.merge_dirty_textures(m_rtts.invalidated_resources);
	m_rtts.invalidated_resources.clear();

	m_draw_calls = 0;
	dirty_frame = true;
	m_frame->flip(m_context);
}
