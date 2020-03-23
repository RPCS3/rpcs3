#include "stdafx.h"
#include "../Overlays/overlay_shader_compile_notification.h"
#include "../Overlays/Shaders/shader_loading_dialog_native.h"
#include "VKGSRender.h"
#include "VKCommonDecompiler.h"
#include "VKRenderPass.h"
#include "VKResourceManager.h"
#include "VKCommandStream.h"

namespace
{
	u32 get_max_depth_value(rsx::surface_depth_format format)
	{
		switch (format)
		{
		case rsx::surface_depth_format::z16: return 0xFFFF;
		case rsx::surface_depth_format::z24s8: return 0xFFFFFF;
		default:
			ASSUME(0);
			break;
		}
		fmt::throw_exception("Unknown depth format" HERE);
	}

	u8 get_pixel_size(rsx::surface_depth_format format)
	{
		switch (format)
		{
		case rsx::surface_depth_format::z16: return 2;
		case rsx::surface_depth_format::z24s8: return 4;
		default:
			ASSUME(0);
			break;
		}
		fmt::throw_exception("Unknown depth format" HERE);
	}
}

namespace vk
{
	VkCompareOp get_compare_func(rsx::comparison_function op, bool reverse_direction = false);

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
			return std::make_pair(VK_FORMAT_A1R5G5B5_UNORM_PACK16, o_rgb);

		case rsx::surface_color_format::x1r5g5b5_z1r5g5b5:
			return std::make_pair(VK_FORMAT_A1R5G5B5_UNORM_PACK16, z_rgb);

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
			return std::make_pair(VK_FORMAT_B8G8R8A8_UNORM, vk::default_component_map());
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
			fmt::throw_exception("Unknown logic op 0x%x" HERE, static_cast<u32>(op));
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
			fmt::throw_exception("Unknown blend factor 0x%x" HERE, static_cast<u32>(factor));
		}
	}

	VkBlendOp get_blend_op(rsx::blend_equation op)
	{
		switch (op)
		{
		case rsx::blend_equation::add_signed:
			rsx_log.trace("blend equation add_signed used. Emulating using FUNC_ADD");
		case rsx::blend_equation::add:
			return VK_BLEND_OP_ADD;
		case rsx::blend_equation::substract: return VK_BLEND_OP_SUBTRACT;
		case rsx::blend_equation::reverse_substract_signed:
			rsx_log.trace("blend equation reverse_subtract_signed used. Emulating using FUNC_REVERSE_SUBTRACT");
		case rsx::blend_equation::reverse_substract: return VK_BLEND_OP_REVERSE_SUBTRACT;
		case rsx::blend_equation::min: return VK_BLEND_OP_MIN;
		case rsx::blend_equation::max: return VK_BLEND_OP_MAX;
		default:
			fmt::throw_exception("Unknown blend op: 0x%x" HERE, static_cast<u32>(op));
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
			fmt::throw_exception("Unknown stencil op: 0x%x" HERE, static_cast<u32>(op));
		}
	}

	VkFrontFace get_front_face(rsx::front_face ffv)
	{
		switch (ffv)
		{
		case rsx::front_face::cw: return VK_FRONT_FACE_CLOCKWISE;
		case rsx::front_face::ccw: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
		default:
			fmt::throw_exception("Unknown front face value: 0x%x" HERE, static_cast<u32>(ffv));
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
			fmt::throw_exception("Unknown cull face value: 0x%x" HERE, static_cast<u32>(cfv));
		}
	}
}

namespace
{
	std::tuple<VkPipelineLayout, VkDescriptorSetLayout> get_shared_pipeline_layout(VkDevice dev)
	{
		const auto& binding_table = vk::get_current_renderer()->get_pipeline_binding_table();
		std::vector<VkDescriptorSetLayoutBinding> bindings(binding_table.total_descriptor_bindings);

		size_t idx = 0;

		// Vertex stream, one stream for cacheable data, one stream for transient data
		for (int i = 0; i < 3; i++)
		{
			bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
			bindings[idx].descriptorCount = 1;
			bindings[idx].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			bindings[idx].binding = binding_table.vertex_buffers_first_bind_slot + i;
			idx++;
		}

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[idx].descriptorCount = 1;
		bindings[idx].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[idx].binding = binding_table.fragment_constant_buffers_bind_slot;

		idx++;

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[idx].descriptorCount = 1;
		bindings[idx].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[idx].binding = binding_table.fragment_state_bind_slot;

		idx++;

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[idx].descriptorCount = 1;
		bindings[idx].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[idx].binding = binding_table.fragment_texture_params_bind_slot;

		idx++;

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[idx].descriptorCount = 1;
		bindings[idx].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		bindings[idx].binding = binding_table.vertex_constant_buffers_bind_slot;

		idx++;

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[idx].descriptorCount = 1;
		bindings[idx].stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
		bindings[idx].binding = binding_table.vertex_params_bind_slot;

		idx++;

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		bindings[idx].descriptorCount = 1;
		bindings[idx].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		bindings[idx].binding = binding_table.conditional_render_predicate_slot;

		idx++;

		for (auto binding = binding_table.textures_first_bind_slot;
			 binding < binding_table.vertex_textures_first_bind_slot;
			 binding++)
		{
			bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			bindings[idx].descriptorCount = 1;
			bindings[idx].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			bindings[idx].binding = binding;
			idx++;
		}

		for (int i = 0; i < rsx::limits::vertex_textures_count; i++)
		{
			bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			bindings[idx].descriptorCount = 1;
			bindings[idx].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			bindings[idx].binding = binding_table.vertex_textures_first_bind_slot + i;
			idx++;
		}

		verify(HERE), idx == binding_table.total_descriptor_bindings;

		std::array<VkPushConstantRange, 1> push_constants;
		push_constants[0].offset = 0;
		push_constants[0].size = 16;
		push_constants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		if (vk::emulate_conditional_rendering())
		{
			// Conditional render toggle
			push_constants[0].size = 20;
		}

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
		layout_info.pushConstantRangeCount = 1;
		layout_info.pPushConstantRanges = push_constants.data();

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
	if (m_thread_context.createInstance("RPCS3"))
	{
		m_thread_context.makeCurrentInstance();
	}
	else
	{
		rsx_log.fatal("Could not find a vulkan compatible GPU driver. Your GPU(s) may not support Vulkan, or you need to install the vulkan runtime and drivers");
		m_device = VK_NULL_HANDLE;
		return;
	}

	std::vector<vk::physical_device>& gpus = m_thread_context.enumerateDevices();

	//Actually confirm  that the loader found at least one compatible device
	//This should not happen unless something is wrong with the driver setup on the target system
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
		rsx_log.fatal("Could not successfully initialize a swapchain");
		return;
	}

	m_device = const_cast<vk::render_device*>(&m_swapchain->get_device());

	vk::set_current_thread_ctx(m_thread_context);
	vk::set_current_renderer(m_swapchain->get_device());

	m_swapchain_dims.width = m_frame->client_width();
	m_swapchain_dims.height = m_frame->client_height();

	if (!m_swapchain->init(m_swapchain_dims.width, m_swapchain_dims.height))
	{
		swapchain_unavailable = true;
	}

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
	m_occlusion_map.resize(occlusion_query_count);

	for (u32 n = 0; n < occlusion_query_count; ++n)
		m_occlusion_query_data[n].driver_handle = n;

	//Generate frame contexts
	const auto& binding_table = m_device->get_pipeline_binding_table();
	const u32 num_fs_samplers = binding_table.vertex_textures_first_bind_slot - binding_table.textures_first_bind_slot;

	std::vector<VkDescriptorPoolSize> sizes;
	sizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 6 * DESCRIPTOR_MAX_DRAW_CALLS });
	sizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER , 3 * DESCRIPTOR_MAX_DRAW_CALLS });
	sizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , (num_fs_samplers + 4) * DESCRIPTOR_MAX_DRAW_CALLS });

	// Conditional rendering predicate slot; refactor to allow skipping this when not needed
	sizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 * DESCRIPTOR_MAX_DRAW_CALLS });

	VkSemaphoreCreateInfo semaphore_info = {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	//VRAM allocation
	m_attrib_ring_info.create(VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, VK_ATTRIB_RING_BUFFER_SIZE_M * 0x100000, "attrib buffer", 0x400000, VK_TRUE);
	m_fragment_env_ring_info.create(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_UBO_RING_BUFFER_SIZE_M * 0x100000, "fragment env buffer");
	m_vertex_env_ring_info.create(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_UBO_RING_BUFFER_SIZE_M * 0x100000, "vertex env buffer");
	m_fragment_texture_params_ring_info.create(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_UBO_RING_BUFFER_SIZE_M * 0x100000, "fragment texture params buffer");
	m_vertex_layout_ring_info.create(VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, VK_UBO_RING_BUFFER_SIZE_M * 0x100000, "vertex layout buffer", 0x10000, VK_TRUE);
	m_fragment_constants_ring_info.create(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_UBO_RING_BUFFER_SIZE_M * 0x100000, "fragment constants buffer");
	m_transform_constants_ring_info.create(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_TRANSFORM_CONSTANTS_BUFFER_SIZE_M * 0x100000, "transform constants buffer");
	m_index_buffer_ring_info.create(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_INDEX_RING_BUFFER_SIZE_M * 0x100000, "index buffer");
	m_texture_upload_buffer_ring_info.create(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_TEXTURE_UPLOAD_RING_BUFFER_SIZE_M * 0x100000, "texture upload buffer", 32 * 0x100000);

	const auto limits = m_device->gpu().get_limits();
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
		ctx.descriptor_pool.create(*m_device, sizes.data(), static_cast<uint32_t>(sizes.size()), DESCRIPTOR_MAX_DRAW_CALLS, 1);
	}

	const auto& memory_map = m_device->get_memory_mapping();
	null_buffer = std::make_unique<vk::buffer>(*m_device, 32, memory_map.device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, 0);
	null_buffer_view = std::make_unique<vk::buffer_view>(*m_device, null_buffer->value, VK_FORMAT_R8_UINT, 0, 32);

	vk::initialize_compiler_context();

	if (g_cfg.video.overlay)
	{
		auto key = vk::get_renderpass_key(m_swapchain->get_surface_format());
		m_text_writer = std::make_unique<vk::text_writer>();
		m_text_writer->init(*m_device, vk::get_renderpass(*m_device, key));
	}

	m_depth_converter = std::make_unique<vk::depth_convert_pass>();
	m_depth_converter->create(*m_device);

	m_attachment_clear_pass = std::make_unique<vk::attachment_clear_pass>();
	m_attachment_clear_pass->create(*m_device);

	m_video_output_pass = std::make_unique<vk::video_out_calibration_pass>();
	m_video_output_pass->create(*m_device);

	m_prog_buffer = std::make_unique<VKProgramBuffer>
	(
		[this](const vk::pipeline_props& props, const RSXVertexProgram& vp, const RSXFragmentProgram& fp)
		{
			// Program was linked or queued for linking
			m_shaders_cache->store(props, vp, fp);
		}
	);

	if (g_cfg.video.disable_vertex_cache || g_cfg.video.multithreaded_rsx)
		m_vertex_cache = std::make_unique<vk::null_vertex_cache>();
	else
		m_vertex_cache = std::make_unique<vk::weak_vertex_cache>();

	m_shaders_cache = std::make_unique<vk::shader_cache>(*m_prog_buffer, "vulkan", "v1.91");

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

	m_ui_renderer = std::make_unique<vk::ui_overlay_renderer>();
	m_ui_renderer->create(*m_current_command_buffer, m_texture_upload_buffer_ring_info);

	m_occlusion_query_pool.initialize(*m_current_command_buffer);

	backend_config.supports_multidraw = true;

	// NOTE: We do not actually need multiple sample support for A2C to work
	// This is here for visual consistency - will be removed when AA problems due to mipmaps are fixed
	if (g_cfg.video.antialiasing_level != msaa_level::none)
	{
		backend_config.supports_hw_a2c = VK_TRUE;
		backend_config.supports_hw_a2one = m_device->get_alpha_to_one_support();
	}

	// NOTE: On NVIDIA cards going back decades (including the PS3) there is a slight normalization inaccuracy in compressed formats.
	// Confirmed in BLES01916 (The Evil Within) which uses RGB565 for some virtual texturing data.
	backend_config.supports_hw_renormalization = (vk::get_driver_vendor() == vk::driver_vendor::NVIDIA);

	// Relaxed query synchronization
	backend_config.supports_hw_conditional_render = !!g_cfg.video.relaxed_zcull_sync;
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

	// Clear flush requests
	m_flush_requests.clear_pending_flag();

	//Texture cache
	m_texture_cache.destroy();

	//Shaders
	vk::finalize_compiler_context();
	m_prog_buffer->clear();

	m_persistent_attribute_storage.reset();
	m_volatile_attribute_storage.reset();
	m_vertex_layout_storage.reset();

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
		vkDestroySemaphore((*m_device), ctx.present_wait_semaphore, nullptr);
		vkDestroySemaphore((*m_device), ctx.acquire_signal_semaphore, nullptr);
		ctx.descriptor_pool.destroy();

		ctx.buffer_views_to_clean.clear();
	}

	//Textures
	m_rtts.destroy();
	m_texture_cache.destroy();

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

	// Video-out calibration (gamma, colorspace, etc)
	m_video_output_pass->destroy();
	m_video_output_pass.reset();

	//Pipeline descriptors
	vkDestroyPipelineLayout(*m_device, pipeline_layout, nullptr);
	vkDestroyDescriptorSetLayout(*m_device, descriptor_layouts, nullptr);

	//Queries
	m_occlusion_query_pool.destroy();
	m_cond_render_buffer.reset();

	//Command buffer
	for (auto &cb : m_primary_cb_list)
		cb.destroy();

	m_command_buffer_pool.destroy();

	m_secondary_command_buffer.destroy();
	m_secondary_command_buffer_pool.destroy();

	//Device handles/contexts
	m_swapchain->destroy();
	m_thread_context.close();

#if defined(HAVE_X11) && defined(HAVE_VULKAN)
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
		if (g_fxo->get<rsx::dma_manager>()->is_current_thread())
		{
			// The offloader thread cannot handle flush requests
			verify(HERE), !(m_queue_status & flush_queue_state::deadlock);

			m_offloader_fault_range = g_fxo->get<rsx::dma_manager>()->get_fault_range(is_writing);
			m_offloader_fault_cause = (is_writing) ? rsx::invalidation_cause::write : rsx::invalidation_cause::read;

			g_fxo->get<rsx::dma_manager>()->set_mem_fault_flag();
			m_queue_status |= flush_queue_state::deadlock;

			// Wait for deadlock to clear
			while (m_queue_status & flush_queue_state::deadlock)
			{
				_mm_pause();
			}

			g_fxo->get<rsx::dma_manager>()->clear_mem_fault_flag();
			return true;
		}

		bool has_queue_ref = false;
		if (!is_current_thread())
		{
			//Always submit primary cb to ensure state consistency (flush pending changes such as image transitions)
			vm::temporary_unlock();

			std::lock_guard lock(m_flush_queue_mutex);

			m_flush_requests.post(false);
			has_queue_ref = true;
		}
		else
		{
			if (vk::is_uninterruptible())
			{
				rsx_log.error("Fault in uninterruptible code!");
			}

			//Flush primary cb queue to sync pending changes (e.g image transitions!)
			flush_command_queue();
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

void VKGSRender::on_invalidate_memory_range(const utils::address_range &range, rsx::invalidation_cause cause)
{
	std::lock_guard lock(m_secondary_cb_guard);

	auto data = std::move(m_texture_cache.invalidate_range(m_secondary_command_buffer, range, cause));
	AUDIT(data.empty());

	if (cause == rsx::invalidation_cause::unmap && data.violation_handled)
	{
		m_texture_cache.purge_unreleased_sections();
		{
			std::lock_guard lock(m_sampler_mutex);
			m_samplers_dirty.store(true);
		}
	}
}

void VKGSRender::on_semaphore_acquire_wait()
{
	if (m_flush_requests.pending() ||
		(async_flip_requested & flip_request::emu_requested) ||
		(m_queue_status & flush_queue_state::deadlock))
	{
		do_local_task(rsx::FIFO_state::lock_wait);
	}
}

void VKGSRender::notify_tile_unbound(u32 tile)
{
	//TODO: Handle texture writeback
	if (false)
	{
		u32 addr = rsx::get_address(tiles[tile].offset, tiles[tile].location, HERE);
		on_notify_memory_unmapped(addr, tiles[tile].size);
		m_rtts.invalidate_surface_address(addr, false);
	}

	{
		std::lock_guard lock(m_sampler_mutex);
		m_samplers_dirty.store(true);
	}
}

void VKGSRender::check_heap_status(u32 flags)
{
	verify(HERE), flags;

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
		m_profiler.start();

		vk::frame_context_t *target_frame = nullptr;
		if (!m_queued_frames.empty())
		{
			if (m_current_frame != &m_aux_frame_context)
			{
				target_frame = m_queued_frames.front();
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
			// Flush the frame context
			frame_context_cleanup(target_frame, true);
		}

		m_frame_stats.flip_time += m_profiler.duration();
	}
}

void VKGSRender::check_present_status()
{
	while (!m_queued_frames.empty())
	{
		auto ctx = m_queued_frames.front();
		if (ctx->swap_command_buffer->pending)
		{
			if (!ctx->swap_command_buffer->poke())
			{
				return;
			}
		}

		frame_context_cleanup(ctx, true);
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

void VKGSRender::set_scissor(bool clip_viewport)
{
	areau scissor;
	if (get_scissor(scissor, clip_viewport))
	{
		m_scissor.extent.height = scissor.height();
		m_scissor.extent.width = scissor.width();
		m_scissor.offset.x = scissor.x1;
		m_scissor.offset.y = scissor.y1;
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

	if (!m_overlay_manager)
	{
		m_frame->hide();
		m_shaders_cache->load(nullptr, *m_device, pipeline_layout);
		m_frame->show();
	}
	else
	{
		rsx::shader_loading_dialog_native dlg(this);

		// TODO: Handle window resize messages during loading on GPUs without OUT_OF_DATE_KHR support
		m_shaders_cache->load(&dlg, *m_device, pipeline_layout);
	}
}

void VKGSRender::on_exit()
{
	zcull_ctrl.release();
	GSRender::on_exit();
}

void VKGSRender::clear_surface(u32 mask)
{
	if (skip_current_frame || swapchain_unavailable) return;

	// If stencil write mask is disabled, remove clear_stencil bit
	if (!rsx::method_registers.stencil_mask()) mask &= ~0x2u;

	// Ignore invalid clear flags
	if (!(mask & 0xF3)) return;

	u8 ctx = rsx::framebuffer_creation_context::context_draw;
	if (mask & 0xF0) ctx |= rsx::framebuffer_creation_context::context_clear_color;
	if (mask & 0x3) ctx |= rsx::framebuffer_creation_context::context_clear_depth;
	init_buffers(rsx::framebuffer_creation_context{ctx});

	if (!framebuffer_status_valid) return;

	float depth_clear = 1.f;
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

	const bool require_mem_load = (scissor_w * scissor_h) < (fb_width * fb_height);
	bool update_color = false, update_z = false;
	auto surface_depth_format = rsx::method_registers.surface_depth_fmt();

	if (auto ds = std::get<1>(m_rtts.m_bound_depth_stencil); mask & 0x3)
	{
		if (mask & 0x1)
		{
			u32 max_depth_value = get_max_depth_value(surface_depth_format);

			u32 clear_depth = rsx::method_registers.z_clear_value(surface_depth_format == rsx::surface_depth_format::z24s8);
			float depth_clear = static_cast<float>(clear_depth) / max_depth_value;

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

				if (ds->samples() > 1)
				{
					if (!require_mem_load) ds->stencil_init_flags &= 0xFF;
					ds->stencil_init_flags |= clear_stencil;
				}
			}

			if ((mask & 0x3) != 0x3 && !require_mem_load && ds->state_flags & rsx::surface_state_flags::erase_bkgnd)
			{
				verify(HERE), depth_stencil_mask;

				if (!g_cfg.video.read_depth_buffer)
				{
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
				else
				{
					ds->write_barrier(*m_current_command_buffer);
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

							m_attachment_clear_pass->run(*m_current_command_buffer, rtt, region.rect, renderpass);

							rtt->change_layout(*m_current_command_buffer, old_layout);
						}
						else
							fmt::throw_exception("Unreachable" HERE);
					}
				}

				for (u8 index = m_rtts.m_bound_render_targets_config.first, count = 0;
					count < m_rtts.m_bound_render_targets_config.second;
					++count, ++index)
				{
					if (require_mem_load)
						m_rtts.m_bound_render_targets[index].second->write_barrier(*m_current_command_buffer);
				}

				update_color = true;
			}
		}
	}

	if (depth_stencil_mask)
	{
		if (m_rtts.m_bound_depth_stencil.first)
		{
			if (require_mem_load) m_rtts.m_bound_depth_stencil.second->write_barrier(*m_current_command_buffer);

			clear_descriptors.push_back({ static_cast<VkImageAspectFlags>(depth_stencil_mask), 0, depth_stencil_clear_values });
			update_z = true;
		}
	}

	if (update_color || update_z)
	{
		const bool write_all_mask[] = { true, true, true, true };
		m_rtts.on_write(update_color ? write_all_mask : nullptr, update_z);
	}

	if (!clear_descriptors.empty())
	{
		begin_render_pass();
		vkCmdClearAttachments(*m_current_command_buffer, ::size32(clear_descriptors), clear_descriptors.data(), 1, &region);
	}
}

void VKGSRender::flush_command_queue(bool hard_sync)
{
	close_and_submit_command_buffer(m_current_command_buffer->submit_fence);

	if (hard_sync)
	{
		// wait for the latest instruction to execute
		m_current_command_buffer->pending = true;
		m_current_command_buffer->reset();

		// Clear all command buffer statuses
		for (auto &cb : m_primary_cb_list)
		{
			if (cb.pending)
				cb.poke();
		}

		// Drain present queue
		while (!m_queued_frames.empty())
		{
			check_present_status();
		}

		m_flush_requests.clear_pending_flag();
	}
	else
	{
		// Mark this queue as pending and proceed
		m_current_command_buffer->pending = true;
	}

	// Grab next cb in line and make it usable
	// NOTE: Even in the case of a hard sync, this is required to free any waiters on the CB (ZCULL)
	m_current_cb_index = (m_current_cb_index + 1) % VK_MAX_ASYNC_CB_COUNT;
	m_current_command_buffer = &m_primary_cb_list[m_current_cb_index];

	if (!m_current_command_buffer->poke())
	{
		rsx_log.error("CB chain has run out of free entries!");
	}

	m_current_command_buffer->reset();

	// Just in case a queued frame holds a ref to this cb, drain the present queue
	check_present_status();

	if (m_occlusion_query_active)
	{
		m_current_command_buffer->flags |= vk::command_buffer::cb_load_occluson_task;
	}

	open_command_buffer();
}

void VKGSRender::sync_hint(rsx::FIFO_hint hint, void* args)
{
	verify(HERE), args;
	rsx::thread::sync_hint(hint, args);

	// Occlusion queries not enabled, do nothing
	if (!(m_current_command_buffer->flags & vk::command_buffer::cb_has_occlusion_task))
		return;

	// Check if the required report is synced to this CB
	auto occlusion_info = static_cast<rsx::reports::occlusion_query_info*>(args);
	auto& data = m_occlusion_map[occlusion_info->driver_handle];

	// NOTE: Currently, a special condition exists where the indices can be empty even with active draw count.
	// This is caused by async compiler and should be removed when ubershaders are added in
	if (!data.is_current(m_current_command_buffer) || data.indices.empty())
		return;

	// Occlusion test result evaluation is coming up, avoid a hard sync
	switch (hint)
	{
	case rsx::FIFO_hint::hint_conditional_render_eval:
	{
		// If a flush request is already enqueued, do nothing
		if (m_flush_requests.pending())
			return;

		// Schedule a sync on the next loop iteration
		m_flush_requests.post(false);
		m_flush_requests.remove_one();
		break;
	}
	case rsx::FIFO_hint::hint_zcull_sync:
	{
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

void VKGSRender::do_local_task(rsx::FIFO_state state)
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

	if (m_overlay_manager)
	{
		if (!in_begin_end && async_flip_requested & flip_request::native_ui)
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
	properties.state.set_attachment_count(::size32(m_draw_buffers));
	properties.state.set_front_face(vk::get_front_face(rsx::method_registers.front_face_mode()));
	properties.state.enable_depth_clamp(rsx::method_registers.depth_clamp_enabled() || !rsx::method_registers.depth_clip_enabled());
	properties.state.enable_depth_bias(true);
	properties.state.enable_depth_bounds_test(m_device->get_depth_bounds_support());

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

	for (uint index = 0; index < m_draw_buffers.size(); ++index)
	{
		bool color_mask_b = rsx::method_registers.color_mask_b(index);
		bool color_mask_g = rsx::method_registers.color_mask_g(index);
		bool color_mask_r = rsx::method_registers.color_mask_r(index);
		bool color_mask_a = rsx::method_registers.color_mask_a(index);

		if (rsx::method_registers.surface_color() == rsx::surface_color_format::g8b8)
			rsx::get_g8b8_r8g8_colormask(color_mask_r, color_mask_g, color_mask_b, color_mask_a);

		properties.state.set_color_mask(index, color_mask_r, color_mask_g, color_mask_b, color_mask_a);
	}

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

		if (auto ds = m_rtts.m_bound_depth_stencil.second;
			ds && ds->samples() > 1 && !(ds->stencil_init_flags & 0xFF00))
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

	const auto rasterization_samples = u8((m_current_renderpass_key >> 16) & 0xF);
	if (backend_config.supports_hw_a2c || rasterization_samples > 1)
	{
		const bool alpha_to_one_enable = rsx::method_registers.msaa_alpha_to_one_enabled() && backend_config.supports_hw_a2one;

		properties.state.set_multisample_state(
			rasterization_samples,
			rsx::method_registers.msaa_sample_mask(),
			rsx::method_registers.msaa_enabled(),
			rsx::method_registers.msaa_alpha_to_coverage_enabled(),
			alpha_to_one_enable);
	}

	properties.renderpass_key = m_current_renderpass_key;

	vk::enter_uninterruptible();

	//Load current program from buffer
	vertex_program.skip_vertex_input_check = true;
	fragment_program.unnormalized_coords = 0;
	m_program = m_prog_buffer->get_graphics_pipeline(vertex_program, fragment_program, properties,
			!g_cfg.video.disable_asynchronous_shader_compiler, true, *m_device, pipeline_layout).get();

	vk::leave_uninterruptible();

	if (m_prog_buffer->check_cache_missed())
	{
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
		auto buf = static_cast<u8*>(m_vertex_env_ring_info.map(mem, 148));

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

			m_prog_buffer->fill_fragment_constants_buffer({ reinterpret_cast<float*>(buf), fragment_constants_size },
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

	const auto& binding_table = m_device->get_pipeline_binding_table();

	m_program->bind_uniform(m_vertex_env_buffer_info, binding_table.vertex_params_bind_slot, m_current_frame->descriptor_set);
	m_program->bind_uniform(m_vertex_constants_buffer_info, binding_table.vertex_constant_buffers_bind_slot, m_current_frame->descriptor_set);
	m_program->bind_uniform(m_fragment_constants_buffer_info, binding_table.fragment_constant_buffers_bind_slot, m_current_frame->descriptor_set);
	m_program->bind_uniform(m_fragment_env_buffer_info, binding_table.fragment_state_bind_slot, m_current_frame->descriptor_set);
	m_program->bind_uniform(m_fragment_texture_params_buffer_info, binding_table.fragment_texture_params_bind_slot, m_current_frame->descriptor_set);

	if (vk::emulate_conditional_rendering())
	{
		auto predicate = m_cond_render_buffer ? m_cond_render_buffer->value : vk::get_scratch_buffer()->value;
		m_program->bind_buffer({ predicate, 0, 4 }, binding_table.conditional_render_predicate_slot, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_current_frame->descriptor_set);
	}

	//Clear flags
	const u32 handled_flags = (rsx::pipeline_state::fragment_state_dirty | rsx::pipeline_state::vertex_state_dirty | rsx::pipeline_state::transform_constants_dirty | rsx::pipeline_state::fragment_constants_dirty | rsx::pipeline_state::fragment_texture_state_dirty);
	m_graphics_state &= ~handled_flags;
}

void VKGSRender::update_vertex_env(u32 id, const vk::vertex_upload_info& vertex_info)
{
	// Actual allocation must have been done previously
	u32 base_offset;
	const u32 offset32 = static_cast<u32>(m_vertex_layout_stream_info.offset);
	const u32 range32 = static_cast<u32>(m_vertex_layout_stream_info.range);

	if (!m_vertex_layout_storage || !m_vertex_layout_storage->in_range(offset32, range32, base_offset))
	{
		verify("Incompatible driver (MacOS?)" HERE), m_texbuffer_view_size >= m_vertex_layout_stream_info.range;

		if (m_vertex_layout_storage)
			m_current_frame->buffer_views_to_clean.push_back(std::move(m_vertex_layout_storage));

		const size_t alloc_addr = m_vertex_layout_stream_info.offset;
		const size_t view_size = (alloc_addr + m_texbuffer_view_size) > m_vertex_layout_ring_info.size() ? m_vertex_layout_ring_info.size() - alloc_addr : m_texbuffer_view_size;
		m_vertex_layout_storage = std::make_unique<vk::buffer_view>(*m_device, m_vertex_layout_ring_info.heap->value, VK_FORMAT_R32G32_UINT, alloc_addr, view_size);
		base_offset = 0;
	}

	u8 data_size = 16;
	u32 draw_info[5];

	draw_info[0] = vertex_info.vertex_index_base;
	draw_info[1] = vertex_info.vertex_index_offset;
	draw_info[2] = id;
	draw_info[3] = (id * 16) + (base_offset / 8);

	if (vk::emulate_conditional_rendering())
	{
		draw_info[4] = cond_render_ctrl.hw_cond_active ? 1 : 0;
		data_size = 20;
	}

	vkCmdPushConstants(*m_current_command_buffer, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, data_size, draw_info);

	const size_t data_offset = (id * 128) + m_vertex_layout_stream_info.offset;
	auto dst = m_vertex_layout_ring_info.map(data_offset, 128);

	fill_vertex_layout_state(m_vertex_layout, vertex_info.first_vertex, vertex_info.allocated_vertex_count, static_cast<s32*>(dst),
		vertex_info.persistent_window_offset, vertex_info.volatile_window_offset);

	m_vertex_layout_ring_info.unmap();
}

void VKGSRender::init_buffers(rsx::framebuffer_creation_context context, bool)
{
	prepare_rtts(context);
}

void VKGSRender::close_and_submit_command_buffer(vk::fence* pFence, VkSemaphore wait_semaphore, VkSemaphore signal_semaphore, VkPipelineStageFlags pipeline_stage_flags)
{
	verify("Recursive calls to submit the current commandbuffer will cause a deadlock" HERE), !m_queue_status.test_and_set(flush_queue_state::flushing);

	// Workaround for deadlock occuring during RSX offloader fault
	// TODO: Restructure command submission infrastructure to avoid this condition
	const bool sync_success = g_fxo->get<rsx::dma_manager>()->sync();
	const VkBool32 force_flush = !sync_success;

	if (vk::test_status_interrupt(vk::heap_dirty))
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

			m_secondary_command_buffer.submit(m_swapchain->get_graphics_queue(),
				VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, force_flush);
		}

		vk::clear_status_interrupt(vk::heap_dirty);
	}

#if 0 // Currently unreachable
	if (m_current_command_buffer->flags & vk::command_buffer::cb_has_conditional_render)
	{
		verify(HERE), m_render_pass_open;
		m_device->cmdEndConditionalRenderingEXT(*m_current_command_buffer);
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
		m_occlusion_query_pool.end_query(*m_current_command_buffer, open_query);
		m_current_command_buffer->flags &= ~vk::command_buffer::cb_has_open_query;
	}

	m_current_command_buffer->end();
	m_current_command_buffer->tag();

	m_current_command_buffer->submit(m_swapchain->get_graphics_queue(),
		wait_semaphore, signal_semaphore, pFence, pipeline_stage_flags, force_flush);

	if (force_flush)
	{
		verify(HERE), m_current_command_buffer->submit_fence->flushed;
	}

	m_queue_status.clear(flush_queue_state::flushing);
}

void VKGSRender::open_command_buffer()
{
	m_current_command_buffer->begin();
}

void VKGSRender::prepare_rtts(rsx::framebuffer_creation_context context)
{
	const bool clipped_scissor = (context == rsx::framebuffer_creation_context::context_draw);
	if (m_current_framebuffer_context == context && !m_rtts_dirty && m_draw_fbo)
	{
		// Fast path
		// Framebuffer usage has not changed, framebuffer exists and config regs have not changed
		set_scissor(clipped_scissor);
		return;
	}

	m_rtts_dirty = false;
	framebuffer_status_valid = false;
	m_framebuffer_state_contested = false;

	get_framebuffer_layout(context, m_framebuffer_layout);
	if (!framebuffer_status_valid)
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
		m_framebuffer_layout.target, m_framebuffer_layout.aa_mode,
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
			const utils::address_range rsx_range = m_surface_info[i].get_memory_range();
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
			const utils::address_range surface_range = m_depth_surface_info.get_memory_range();
			m_texture_cache.set_memory_read_flags(surface_range, rsx::memory_read_flags::flush_once);
			m_texture_cache.flush_if_cache_miss_likely(*m_current_command_buffer, surface_range);
		}

		m_depth_surface_info.address = m_depth_surface_info.pitch = 0;
		m_depth_surface_info.width = m_framebuffer_layout.width;
		m_depth_surface_info.height = m_framebuffer_layout.height;
		m_depth_surface_info.depth_format = m_framebuffer_layout.depth_format;
		m_depth_surface_info.depth_buffer_float = m_framebuffer_layout.depth_float;
		m_depth_surface_info.bpp = (m_framebuffer_layout.depth_format == rsx::surface_depth_format::z16? 2 : 4);
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
			verify("Pitch mismatch!" HERE), surface->rsx_pitch == m_framebuffer_layout.actual_color_pitch[index];

			m_texture_cache.notify_surface_changed(m_surface_info[index].get_memory_range(m_framebuffer_layout.aa_factors));
			m_draw_buffers.push_back(index);
		}
	}

	if (std::get<0>(m_rtts.m_bound_depth_stencil) != 0)
	{
		auto ds = std::get<1>(m_rtts.m_bound_depth_stencil);
		ds->set_depth_render_mode(!m_framebuffer_layout.depth_float);
		m_fbo_images.push_back(ds);

		m_depth_surface_info.address = m_framebuffer_layout.zeta_address;
		m_depth_surface_info.pitch = m_framebuffer_layout.actual_zeta_pitch;
		verify("Pitch mismatch!" HERE), ds->rsx_pitch == m_framebuffer_layout.actual_zeta_pitch;

		m_texture_cache.notify_surface_changed(m_depth_surface_info.get_memory_range(m_framebuffer_layout.aa_factors));
	}

	// Before messing with memory properties, flush command queue if there are dma transfers queued up
	if (m_current_command_buffer->flags & vk::command_buffer::cb_has_dma_transfer)
	{
		flush_command_queue();
	}

	const auto color_fmt_info = get_compatible_gcm_format(m_framebuffer_layout.color_format);
	for (u8 index : m_draw_buffers)
	{
		if (!m_surface_info[index].address || !m_surface_info[index].pitch) continue;

		const utils::address_range surface_range = m_surface_info[index].get_memory_range();
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
		const utils::address_range surface_range = m_depth_surface_info.get_memory_range();
		if (g_cfg.video.write_depth_buffer)
		{
			const u32 gcm_format = (m_depth_surface_info.depth_format != rsx::surface_depth_format::z16) ? CELL_GCM_TEXTURE_DEPTH16 : CELL_GCM_TEXTURE_DEPTH24_D8;
			m_texture_cache.lock_memory_region(
				*m_current_command_buffer, m_rtts.m_bound_depth_stencil.second, surface_range, true,
				m_depth_surface_info.width, m_depth_surface_info.height, m_framebuffer_layout.actual_zeta_pitch, gcm_format, true);
		}
		else
		{
			m_texture_cache.commit_framebuffer_memory_region(*m_current_command_buffer, surface_range);
		}
	}

	if (!m_rtts.orphaned_surfaces.empty())
	{
		u32 gcm_format;
		bool swap_bytes;

		for (auto& surface : m_rtts.orphaned_surfaces)
		{
			const bool lock = surface->is_depth_surface() ? !!g_cfg.video.write_depth_buffer :
				!!g_cfg.video.write_color_buffers;

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
				surface->get_surface_width(rsx::surface_metrics::pixels), surface->get_surface_height(rsx::surface_metrics::pixels), surface->get_rsx_pitch(),
				gcm_format, swap_bytes);
		}

		m_rtts.orphaned_surfaces.clear();
	}

	m_current_renderpass_key = vk::get_renderpass_key(m_fbo_images);
	m_cached_renderpass = vk::get_renderpass(*m_device, m_current_renderpass_key);

	// Search old framebuffers for this same configuration
	const auto fbo_width = rsx::apply_resolution_scale(m_framebuffer_layout.width, true);
	const auto fbo_height = rsx::apply_resolution_scale(m_framebuffer_layout.height, true);

	if (m_draw_fbo)
	{
		// Release old ref
		m_draw_fbo->release();
	}

	m_draw_fbo = vk::get_framebuffer(*m_device, fbo_width, fbo_height, m_cached_renderpass, m_fbo_images);
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
		auto packet = reinterpret_cast<vk::submit_packet*>(args);
		vk::queue_submit(packet->queue, &packet->submit_info, packet->pfence, VK_TRUE);
		free(packet);
		break;
	}
	default:
		fmt::throw_exception("Unhandled request code 0x%x" HERE, request_code);
	}
}

bool VKGSRender::scaled_image_from_memory(rsx::blit_src_info& src, rsx::blit_dst_info& dst, bool interpolate)
{
	if (swapchain_unavailable)
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
	verify(HERE), !m_occlusion_query_active;

	query->result = 0;
	//query->sync_timestamp = get_system_time();
	m_active_query_info = query;
	m_occlusion_query_active = true;
	m_current_command_buffer->flags |= vk::command_buffer::cb_load_occluson_task;
}

void VKGSRender::end_occlusion_query(rsx::reports::occlusion_query_info* query)
{
	verify(HERE), query == m_active_query_info;

	// NOTE: flushing the queue is very expensive, do not flush just because query stopped
	if (m_current_command_buffer->flags & vk::command_buffer::cb_has_open_query)
	{
		// End query
		auto open_query = m_occlusion_map[m_active_query_info->driver_handle].indices.back();
		m_occlusion_query_pool.end_query(*m_current_command_buffer, open_query);
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

	u32 oldest = data.indices.front();
	return m_occlusion_query_pool.check_query_status(oldest);
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

			rsx_log.error("[Performance warning] Unexpected ZCULL read caused a hard sync");
			busy_wait();
		}

		data.sync();

		// Gather data
		for (const auto occlusion_id : data.indices)
		{
			// We only need one hit
			if (auto value = m_occlusion_query_pool.get_query_result(occlusion_id))
			{
				query->result = 1;
				break;
			}
		}
	}

	m_occlusion_query_pool.reset_queries(*m_current_command_buffer, data.indices);
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

	m_occlusion_query_pool.reset_queries(*m_current_command_buffer, data.indices);
	data.indices.clear();
}

void VKGSRender::emergency_query_cleanup(vk::command_buffer* commands)
{
	verify("Command list mismatch" HERE), commands == static_cast<vk::command_buffer*>(m_current_command_buffer);

	if (m_current_command_buffer->flags & vk::command_buffer::cb_has_open_query)
	{
		auto open_query = m_occlusion_map[m_active_query_info->driver_handle].indices.back();
		m_occlusion_query_pool.end_query(*m_current_command_buffer, open_query);
		m_current_command_buffer->flags &= ~vk::command_buffer::cb_has_open_query;
	}
}

void VKGSRender::begin_conditional_rendering(const std::vector<rsx::reports::occlusion_query_info*>& sources)
{
	verify(HERE), !sources.empty();

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
			usage_flags, 0);
	}

	VkPipelineStageFlags dst_stage;
	VkAccessFlags dst_access;

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

	if (sources.size() == 1)
	{
		const auto query = sources.front();
		const auto& query_info = m_occlusion_map[query->driver_handle];

		if (query_info.indices.size() == 1)
		{
			const auto& index = query_info.indices.front();
			m_occlusion_query_pool.get_query_result_indirect(*m_current_command_buffer, index, m_cond_render_buffer->value, 0);

			vk::insert_buffer_memory_barrier(*m_current_command_buffer, m_cond_render_buffer->value, 0, 4,
				VK_PIPELINE_STAGE_TRANSFER_BIT, dst_stage,
				VK_ACCESS_TRANSFER_WRITE_BIT, dst_access);

			rsx::thread::begin_conditional_rendering(sources);
			return;
		}
	}

	auto scratch = vk::get_scratch_buffer();
	u32 dst_offset = 0;
	size_t first = 0;
	size_t last;

	if (!partial_eval) [[likely]]
	{
		last = sources.size();
	}
	else
	{
		last = 1;
	}

	for (size_t i = first; i < last; ++i)
	{
		auto& query_info = m_occlusion_map[sources[i]->driver_handle];
		for (const auto& index : query_info.indices)
		{
			m_occlusion_query_pool.get_query_result_indirect(*m_current_command_buffer, index, scratch->value, dst_offset);
			dst_offset += 4;
		}
	}

	if (dst_offset)
	{
		// Fast path should have been caught above
		verify(HERE), dst_offset > 4;

		if (!partial_eval)
		{
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
	else
	{
		rsx_log.error("Dubious query data pushed to cond render!, Please report to developers(q.pending=%d)", sources.front()->pending);
	}

	rsx::thread::begin_conditional_rendering(sources);
}

void VKGSRender::end_conditional_rendering()
{
	thread::end_conditional_rendering();
}

bool VKGSRender::on_decompiler_task()
{
	return m_prog_buffer->async_update(8, *m_device, pipeline_layout).first;
}
