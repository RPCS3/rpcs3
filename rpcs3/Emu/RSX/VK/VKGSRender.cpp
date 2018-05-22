#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "VKGSRender.h"
#include "../rsx_methods.h"
#include "../rsx_utils.h"
#include "../Common/BufferUtils.h"
#include "VKFormats.h"
#include "VKCommonDecompiler.h"

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
		case VK_FORMAT_R8_UNORM:
			color_format_idx = 4;
			break;
		case VK_FORMAT_R8G8_UNORM:
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
	VkRenderPass precompute_render_pass(VkDevice dev, VkFormat color_format, u8 number_of_color_surface, VkFormat depth_format)
	{
		// Some driver crashes when using empty render pass
		if (number_of_color_surface == 0 && depth_format == VK_FORMAT_UNDEFINED)
			return nullptr;
		/* Describe a render pass and framebuffer attachments */
		std::vector<VkAttachmentDescription> attachments = {};
		std::vector<VkAttachmentReference> attachment_references;

		VkAttachmentDescription color_attachment_description = {};
		color_attachment_description.format = color_format;
		color_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		color_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment_description.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		color_attachment_description.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		for (u32 i = 0; i < number_of_color_surface; ++i)
		{
			attachments.push_back(color_attachment_description);
			attachment_references.push_back({ i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
		}

		if (depth_format != VK_FORMAT_UNDEFINED)
		{
			VkAttachmentDescription depth_attachment_description = {};
			depth_attachment_description.format = depth_format;
			depth_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
			depth_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			depth_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			depth_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			depth_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
			depth_attachment_description.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			depth_attachment_description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			attachments.push_back(depth_attachment_description);

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
		rp_info.pDependencies = nullptr;
		rp_info.dependencyCount = 0;

		VkRenderPass result;
		CHECK_RESULT(vkCreateRenderPass(dev, &rp_info, NULL, &result));
		return result;
	}

	std::array<VkRenderPass, 120> get_precomputed_render_passes(VkDevice dev, const vk::gpu_formats_support &gpu_format_support)
	{
		std::array<VkRenderPass, 120> result = {};

		const std::array<VkFormat, 3> depth_format_list = { VK_FORMAT_UNDEFINED, VK_FORMAT_D16_UNORM, gpu_format_support.d24_unorm_s8 ? VK_FORMAT_D24_UNORM_S8_UINT : VK_FORMAT_D32_SFLOAT_S8_UINT };
		const std::array<VkFormat, 8> color_format_list = { VK_FORMAT_R5G6B5_UNORM_PACK16, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R8_UNORM, VK_FORMAT_R8G8_UNORM, VK_FORMAT_A1R5G5B5_UNORM_PACK16, VK_FORMAT_R32_SFLOAT };


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
	if (gpus.size() == 0)
	{
		//We can't throw in Emulator::Load, so we show error and return
		LOG_FATAL(RSX, "No compatible GPU devices found");
		m_device = VK_NULL_HANDLE;
		return;
	}

	bool gpu_found = false;
	std::string adapter_name = g_cfg.video.vk.adapter;

	display_handle_t display = m_frame->handle();

#ifndef _WIN32
	display.match([this](std::pair<Display*, Window> p) { m_display_handle = p.first; XFlush(m_display_handle); }, [](auto _) {});
#endif

	for (auto &gpu : gpus)
	{
		if (gpu.name() == adapter_name)
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

	// Choose memory allocator (device memory)
	//m_mem_allocator = std::make_shared<vk::mem_allocator_vk>(*m_device, m_device->gpu());
	m_mem_allocator = std::make_shared<vk::mem_allocator_vma>(*m_device, m_device->gpu());
	vk::set_current_mem_allocator(m_mem_allocator);

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
	m_secondary_command_buffer.create(m_secondary_command_buffer_pool);
	m_secondary_command_buffer.access_hint = vk::command_buffer::access_type_hint::all;

	//Precalculated stuff
	m_render_passes = get_precomputed_render_passes(*m_device, m_device->get_formats_support());
	std::tie(pipeline_layout, descriptor_layouts) = get_shared_pipeline_layout(*m_device);

	//Occlusion
	m_occlusion_query_pool.create((*m_device), DESCRIPTOR_MAX_DRAW_CALLS); //Enough for 4k draw calls per pass
	for (int n = 0; n < 128; ++n)
		m_occlusion_query_data[n].driver_handle = n;

	//Generate frame contexts
	VkDescriptorPoolSize uniform_buffer_pool = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 3 * DESCRIPTOR_MAX_DRAW_CALLS };
	VkDescriptorPoolSize uniform_texel_pool = { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER , 16 * DESCRIPTOR_MAX_DRAW_CALLS };
	VkDescriptorPoolSize texture_pool = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 20 * DESCRIPTOR_MAX_DRAW_CALLS };

	std::vector<VkDescriptorPoolSize> sizes{ uniform_buffer_pool, uniform_texel_pool, texture_pool };

	VkSemaphoreCreateInfo semaphore_info = {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	//VRAM allocation
	const auto& memory_map = m_device->get_memory_mapping();
	m_attrib_ring_info.init(VK_ATTRIB_RING_BUFFER_SIZE_M * 0x100000, "attrib buffer", 0x400000);
	m_attrib_ring_info.heap.reset(new vk::buffer(*m_device, VK_ATTRIB_RING_BUFFER_SIZE_M * 0x100000, memory_map.host_visible_coherent, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, 0));
	m_uniform_buffer_ring_info.init(VK_UBO_RING_BUFFER_SIZE_M * 0x100000, "uniform buffer");
	m_uniform_buffer_ring_info.heap.reset(new vk::buffer(*m_device, VK_UBO_RING_BUFFER_SIZE_M * 0x100000, memory_map.host_visible_coherent, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 0));
	m_transform_constants_ring_info.init(VK_TRANSFORM_CONSTANTS_BUFFER_SIZE_M * 0x100000, "transform constants buffer");
	m_transform_constants_ring_info.heap.reset(new vk::buffer(*m_device, VK_TRANSFORM_CONSTANTS_BUFFER_SIZE_M * 0x100000, memory_map.host_visible_coherent, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 0));
	m_index_buffer_ring_info.init(VK_INDEX_RING_BUFFER_SIZE_M * 0x100000, "index buffer");
	m_index_buffer_ring_info.heap.reset(new vk::buffer(*m_device, VK_INDEX_RING_BUFFER_SIZE_M * 0x100000, memory_map.host_visible_coherent, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 0));
	m_texture_upload_buffer_ring_info.init(VK_TEXTURE_UPLOAD_RING_BUFFER_SIZE_M * 0x100000, "texture upload buffer", 32 * 0x100000);
	m_texture_upload_buffer_ring_info.heap.reset(new vk::buffer(*m_device, VK_TEXTURE_UPLOAD_RING_BUFFER_SIZE_M * 0x100000, memory_map.host_visible_coherent, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 0));

	for (auto &ctx : frame_context_storage)
	{
		vkCreateSemaphore((*m_device), &semaphore_info, nullptr, &ctx.present_semaphore);
		ctx.descriptor_pool.create(*m_device, sizes.data(), static_cast<uint32_t>(sizes.size()));
	}

	null_buffer = std::make_unique<vk::buffer>(*m_device, 32, memory_map.host_visible_coherent, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, 0);
	null_buffer_view = std::make_unique<vk::buffer_view>(*m_device, null_buffer->value, VK_FORMAT_R8_UINT, 0, 32);

	vk::initialize_compiler_context();

	if (g_cfg.video.overlay)
	{
		size_t idx = vk::get_render_pass_location( m_swapchain->get_surface_format(), VK_FORMAT_UNDEFINED, 1);
		m_text_writer.reset(new vk::text_writer());
		m_text_writer->init(*m_device, m_render_passes[idx]);
	}

	m_depth_converter.reset(new vk::depth_convert_pass());
	m_depth_converter->create(*m_device);

	m_depth_scaler.reset(new vk::depth_scaling_pass());
	m_depth_scaler->create(*m_device);

	m_attachment_clear_pass.reset(new vk::attachment_clear_pass());
	m_attachment_clear_pass->create(*m_device);

	m_prog_buffer.reset(new VKProgramBuffer(m_render_passes.data()));

	if (g_cfg.video.disable_vertex_cache)
		m_vertex_cache.reset(new vk::null_vertex_cache());
	else
		m_vertex_cache.reset(new vk::weak_vertex_cache());

	m_shaders_cache.reset(new vk::shader_cache(*m_prog_buffer.get(), "vulkan", "v1.3"));

	open_command_buffer();

	for (u32 i = 0; i < m_swapchain->get_swap_image_count(); ++i)
	{
		const auto target_layout = m_swapchain->get_optimal_present_layout();
		const auto target_image = m_swapchain->get_image(i);
		VkClearColorValue clear_color{};
		VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		vk::change_image_layout(*m_current_command_buffer, target_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, range);
		vkCmdClearColorImage(*m_current_command_buffer, target_image, VK_IMAGE_LAYOUT_GENERAL, &clear_color, 1, &range);
		vk::change_image_layout(*m_current_command_buffer, target_image, VK_IMAGE_LAYOUT_GENERAL, target_layout, range);

	}

	m_current_frame = &frame_context_storage[0];

	m_texture_cache.initialize((*m_device), m_swapchain->get_graphics_queue(),
			m_texture_upload_buffer_ring_info);

	m_ui_renderer.reset(new vk::ui_overlay_renderer());
	m_ui_renderer->create(*m_current_command_buffer, m_texture_upload_buffer_ring_info);

	supports_multidraw = !g_cfg.video.strict_rendering_mode;
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
	m_index_buffer_ring_info.heap.reset();
	m_uniform_buffer_ring_info.heap.reset();
	m_transform_constants_ring_info.heap.reset();
	m_attrib_ring_info.heap.reset();
	m_texture_upload_buffer_ring_info.heap.reset();

	//Fallback bindables
	null_buffer.reset();
	null_buffer_view.reset();

	//Frame context
	m_framebuffers_to_clean.clear();

	if (m_current_frame == &m_aux_frame_context)
	{
		//Return resources back to the owner
		m_current_frame = &frame_context_storage[m_current_queue_index];
		m_current_frame->swap_storage(m_aux_frame_context);
		m_current_frame->grab_resources(m_aux_frame_context);
	}

	m_aux_frame_context.buffer_views_to_clean.clear();
	m_aux_frame_context.samplers_to_clean.clear();

	//NOTE: aux_context uses descriptor pools borrowed from the main queues and any allocations will be automatically freed when pool is destroyed
	for (auto &ctx : frame_context_storage)
	{
		vkDestroySemaphore((*m_device), ctx.present_semaphore, nullptr);
		ctx.descriptor_pool.destroy();

		ctx.buffer_views_to_clean.clear();
		ctx.samplers_to_clean.clear();
	}

	m_draw_fbo.reset();

	//Render passes
	for (auto &render_pass : m_render_passes)
		if (render_pass)
			vkDestroyRenderPass(*m_device, render_pass, nullptr);

	//Textures
	m_rtts.destroy();
	m_texture_cache.destroy();

	//Sampler handles
	for (auto& handle : fs_sampler_handles)
		handle.reset();

	for (auto& handle : vs_sampler_handles)
		handle.reset();

	//Overlay text handler
	m_text_writer.reset();

	//Overlay UI renderer
	m_ui_renderer->destroy();
	m_ui_renderer.reset();

	//RGBA->depth cast helper
	m_depth_converter->destroy();
	m_depth_converter.reset();

	//Depth surface blitter
	m_depth_scaler->destroy();
	m_depth_scaler.reset();

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

	// Memory allocator (device memory)
	m_mem_allocator->destroy();

	//Device handles/contexts
	m_swapchain->destroy();
	m_thread_context.close();
	
#if !defined(_WIN32) && defined(HAVE_VULKAN)
	if (m_display_handle)
		XCloseDisplay(m_display_handle);
#endif
}

bool VKGSRender::on_access_violation(u32 address, bool is_writing)
{
	vk::texture_cache::thrashed_set result;
	{
		std::lock_guard<shared_mutex> lock(m_secondary_cb_guard);
		result = std::move(m_texture_cache.invalidate_address(address, is_writing, false, m_secondary_command_buffer, m_swapchain->get_graphics_queue()));
	}

	if (!result.violation_handled)
		return false;

	{
		std::lock_guard<shared_mutex> lock(m_sampler_mutex);
		m_samplers_dirty.store(true);
	}

	if (result.num_flushable > 0)
	{
		const bool is_rsxthr = std::this_thread::get_id() == rsx_thread;
		bool has_queue_ref = false;

		u64 sync_timestamp = 0ull;
		for (const auto& tex : result.sections_to_flush)
			sync_timestamp = std::max(sync_timestamp, tex->get_sync_timestamp());

		if (!is_rsxthr)
		{
			//Always submit primary cb to ensure state consistency (flush pending changes such as image transitions)
			vm::temporary_unlock();

			std::lock_guard<shared_mutex> lock(m_flush_queue_mutex);

			m_flush_requests.post(sync_timestamp == 0ull);
			has_queue_ref = true;
		}
		else
		{
			//Flush primary cb queue to sync pending changes (e.g image transitions!)
			flush_command_queue();
		}

		if (sync_timestamp > 0)
		{
			//Wait for earliest cb submitted after the sync timestamp to finish
			command_buffer_chunk *target_cb = nullptr;
			for (auto &cb : m_primary_cb_list)
			{
				if (cb.pending && cb.last_sync >= sync_timestamp)
				{
					if (target_cb == nullptr || target_cb->last_sync > cb.last_sync)
						target_cb = &cb;
				}
			}

			if (target_cb)
				target_cb->wait();

			if (is_rsxthr)
				m_last_flushable_cb = -1;
		}

		if (has_queue_ref)
		{
			//Wait for the RSX thread to process request if it hasn't already
			m_flush_requests.producer_wait();
		}

		m_texture_cache.flush_all(result, m_secondary_command_buffer, m_swapchain->get_graphics_queue());

		if (has_queue_ref)
		{
			//Release RSX thread
			m_flush_requests.remove_one();
		}
	}

	return false;
}

void VKGSRender::on_notify_memory_unmapped(u32 address_base, u32 size)
{
	std::lock_guard<shared_mutex> lock(m_secondary_cb_guard);
	if (m_texture_cache.invalidate_range(address_base, size, true, true, false,
		m_secondary_command_buffer, m_swapchain->get_graphics_queue()).violation_handled)
	{
		m_texture_cache.purge_dirty();
		{
			std::lock_guard<shared_mutex> lock(m_sampler_mutex);
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
		std::lock_guard<shared_mutex> lock(m_sampler_mutex);
		m_samplers_dirty.store(true);
	}
}

void VKGSRender::check_heap_status()
{
	if (m_attrib_ring_info.is_critical() ||
		m_texture_upload_buffer_ring_info.is_critical() ||
		m_uniform_buffer_ring_info.is_critical() ||
		m_transform_constants_ring_info.is_critical() ||
		m_index_buffer_ring_info.is_critical())
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
			m_uniform_buffer_ring_info.reset_allocation_stats();
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

void VKGSRender::begin()
{
	rsx::thread::begin();

	if (skip_frame || renderer_unavailable ||
		(conditional_render_enabled && conditional_render_test_failed))
		return;

	init_buffers(rsx::framebuffer_creation_context::context_draw);

	if (!framebuffer_status_valid)
		return;

	//Ease resource pressure if the number of draw calls becomes too high or we are running low on memory resources
	if (m_current_frame->used_descriptors >= DESCRIPTOR_MAX_DRAW_CALLS)
	{
		//No need to stall if we have more than one frame queue anyway
		flush_command_queue();

		CHECK_RESULT(vkResetDescriptorPool(*m_device, m_current_frame->descriptor_pool, 0));
		m_current_frame->used_descriptors = 0;
	}

	check_heap_status();

	VkDescriptorSetAllocateInfo alloc_info = {};
	alloc_info.descriptorPool = m_current_frame->descriptor_pool;
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts = &descriptor_layouts;
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

	VkDescriptorSet new_descriptor_set;
	CHECK_RESULT(vkAllocateDescriptorSets(*m_device, &alloc_info, &new_descriptor_set));

	m_current_frame->descriptor_set = new_descriptor_set;
	m_current_frame->used_descriptors++;
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

	set_viewport();

	//TODO: Set up other render-state parameters into the program pipeline

	std::chrono::time_point<steady_clock> stop = steady_clock::now();
	m_setup_time += std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
}

void VKGSRender::begin_render_pass()
{
	if (render_pass_open)
		return;

	VkRenderPassBeginInfo rp_begin = {};
	rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rp_begin.renderPass = m_draw_fbo->info.renderPass;
	rp_begin.framebuffer = m_draw_fbo->value;
	rp_begin.renderArea.offset.x = 0;
	rp_begin.renderArea.offset.y = 0;
	rp_begin.renderArea.extent.width = m_draw_fbo->width();
	rp_begin.renderArea.extent.height = m_draw_fbo->height();

	vkCmdBeginRenderPass(*m_current_command_buffer, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
	render_pass_open = true;
}

void VKGSRender::close_render_pass()
{
	if (!render_pass_open)
		return;

	vkCmdEndRenderPass(*m_current_command_buffer);
	render_pass_open = false;
}

void VKGSRender::end()
{
	if (skip_frame || !framebuffer_status_valid || renderer_unavailable ||
		(conditional_render_enabled && conditional_render_test_failed) ||
		!check_program_status())
	{
		rsx::thread::end();
		return;
	}

	//Programs data is dependent on vertex state
	std::chrono::time_point<steady_clock> vertex_start = steady_clock::now();
	auto upload_info = upload_vertex_data();
	std::chrono::time_point<steady_clock> vertex_end = steady_clock::now();
	m_vertex_upload_time += std::chrono::duration_cast<std::chrono::microseconds>(vertex_end - vertex_start).count();

	std::chrono::time_point<steady_clock> textures_start = vertex_end;

	auto ds = std::get<1>(m_rtts.m_bound_depth_stencil);
	//Clear any 'dirty' surfaces - possible is a recycled cache surface is used
	std::vector<VkClearAttachment> buffers_to_clear;
	buffers_to_clear.reserve(4);
	const auto targets = rsx::utility::get_rtt_indexes(rsx::method_registers.surface_color_target());

	//Check for memory clears
	if (ds && ds->dirty)
	{
		//Clear this surface before drawing on it
		VkClearValue clear_value = {};
		clear_value.depthStencil = { 1.f, 255 };
		buffers_to_clear.push_back({ vk::get_aspect_flags(ds->info.format), 0, clear_value });
		ds->dirty = false;
	}

	for (u32 index = 0; index < targets.size(); ++index)
	{
		if (auto rtt = std::get<1>(m_rtts.m_bound_render_targets[index]))
		{
			if (rtt->dirty)
			{
				buffers_to_clear.push_back({ VK_IMAGE_ASPECT_COLOR_BIT, index, {} });
				rtt->dirty = false;
			}
		}
	}

	if (buffers_to_clear.size() > 0)
	{
		begin_render_pass();

		VkClearRect rect = { {{0, 0}, {m_draw_fbo->width(), m_draw_fbo->height()}}, 0, 1 };
		vkCmdClearAttachments(*m_current_command_buffer, (u32)buffers_to_clear.size(),
			buffers_to_clear.data(), 1, &rect);

		close_render_pass();
	}

	//Check for data casts
	if (ds && ds->old_contents)
	{
		if (ds->old_contents->info.format == VK_FORMAT_B8G8R8A8_UNORM)
		{
			auto rp = vk::get_render_pass_location(VK_FORMAT_UNDEFINED, ds->info.format, 0);
			auto render_pass = m_render_passes[rp];
			m_depth_converter->run(*m_current_command_buffer, ds->width(), ds->height(), ds, ds->old_contents->get_view(0xAAE4, rsx::default_remap_vector), render_pass, m_framebuffers_to_clean);

			ds->old_contents = nullptr;
		}
		else if (!g_cfg.video.strict_rendering_mode)
		{
			//Clear this to avoid dereferencing stale ptr
			ds->old_contents = nullptr;
		}
	}

	if (g_cfg.video.strict_rendering_mode)
	{
		auto copy_rtt_contents = [&](vk::render_target* surface)
		{
			if (surface->info.format == surface->old_contents->info.format)
			{
				const VkImageAspectFlags aspect = surface->attachment_aspect_flag;

				const u16 parent_w = surface->old_contents->width();
				const u16 parent_h = surface->old_contents->height();
				u16 copy_w, copy_h;

				std::tie(std::ignore, std::ignore, copy_w, copy_h) = rsx::clip_region<u16>(parent_w, parent_h, 0, 0, surface->width(), surface->height(), true);

				VkImageSubresourceRange subresource_range = { aspect, 0, 1, 0, 1 };
				VkImageLayout old_layout = surface->current_layout;

				vk::change_image_layout(*m_current_command_buffer, surface, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresource_range);
				vk::change_image_layout(*m_current_command_buffer, surface->old_contents, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresource_range);

				VkImageCopy copy_rgn;
				copy_rgn.srcOffset = { 0, 0, 0 };
				copy_rgn.dstOffset = { 0, 0, 0 };
				copy_rgn.dstSubresource = { aspect, 0, 0, 1 };
				copy_rgn.srcSubresource = { aspect, 0, 0, 1 };
				copy_rgn.extent = { copy_w, copy_h, 1 };

				vkCmdCopyImage(*m_current_command_buffer, surface->old_contents->value, surface->old_contents->current_layout, surface->value, surface->current_layout, 1, &copy_rgn);
				vk::change_image_layout(*m_current_command_buffer, surface, old_layout, subresource_range);

				surface->dirty = false;
			}
			//TODO: download image contents and reupload them or do a memory cast to copy memory contents if not compatible

			surface->old_contents = nullptr;
		};

		//Prepare surfaces if needed
		for (auto &rtt : m_rtts.m_bound_render_targets)
		{
			if (auto surface = std::get<1>(rtt))
			{
				if (surface->old_contents != nullptr)
					copy_rtt_contents(surface);
			}
		}

		if (ds && ds->old_contents)
		{
			copy_rtt_contents(ds);
		}
	}

	//Load textures
	{
		std::lock_guard<shared_mutex> lock(m_sampler_mutex);
		bool update_framebuffer_sourced = false;

		if (surface_store_tag != m_rtts.cache_tag)
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
					*sampler_state = m_texture_cache._upload_texture(*m_current_command_buffer, rsx::method_registers.fragment_textures[i], m_rtts);

					const u32 texture_format = rsx::method_registers.fragment_textures[i].format() & ~(CELL_GCM_TEXTURE_UN | CELL_GCM_TEXTURE_LN);
					const VkBool32 compare_enabled = (texture_format == CELL_GCM_TEXTURE_DEPTH16 || texture_format == CELL_GCM_TEXTURE_DEPTH24_D8 ||
							texture_format == CELL_GCM_TEXTURE_DEPTH16_FLOAT || texture_format == CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT);
					VkCompareOp depth_compare_mode = compare_enabled ? vk::get_compare_func((rsx::comparison_function)rsx::method_registers.fragment_textures[i].zfunc(), true) : VK_COMPARE_OP_NEVER;

					bool replace = !fs_sampler_handles[i];
					VkFilter min_filter;
					VkSamplerMipmapMode mip_mode;
					f32 min_lod = 0.f, max_lod = 0.f;
					f32 lod_bias = 0.f;

					const bool aniso_override = !g_cfg.video.strict_rendering_mode && g_cfg.video.anisotropic_level_override > 0;
					const f32 af_level = aniso_override ? g_cfg.video.anisotropic_level_override : vk::max_aniso(rsx::method_registers.fragment_textures[i].max_aniso());
					const auto wrap_s = vk::vk_wrap_mode(rsx::method_registers.fragment_textures[i].wrap_s());
					const auto wrap_t = vk::vk_wrap_mode(rsx::method_registers.fragment_textures[i].wrap_t());
					const auto wrap_r = vk::vk_wrap_mode(rsx::method_registers.fragment_textures[i].wrap_r());
					const auto mag_filter = vk::get_mag_filter(rsx::method_registers.fragment_textures[i].mag_filter());
					const auto border_color = vk::get_border_color(rsx::method_registers.fragment_textures[i].border_color());

					std::tie(min_filter, mip_mode) = vk::get_min_filter_and_mip(rsx::method_registers.fragment_textures[i].min_filter());

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
							m_current_frame->samplers_to_clean.push_back(std::move(fs_sampler_handles[i]));
							replace = true;
						}
					}

					if (replace)
					{
						fs_sampler_handles[i] = std::make_unique<vk::sampler>(*m_device, wrap_s, wrap_t, wrap_r, false, lod_bias, af_level, min_lod, max_lod,
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
					*sampler_state = m_texture_cache._upload_texture(*m_current_command_buffer, rsx::method_registers.vertex_textures[i], m_rtts);

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
							m_current_frame->samplers_to_clean.push_back(std::move(vs_sampler_handles[i]));
							replace = true;
						}
					}

					if (replace)
					{
						vs_sampler_handles[i] = std::make_unique<vk::sampler>(
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
	}

	std::chrono::time_point<steady_clock> textures_end = steady_clock::now();
	m_textures_upload_time += (u32)std::chrono::duration_cast<std::chrono::microseconds>(textures_end - textures_start).count();

	//Load program
	std::chrono::time_point<steady_clock> program_start = textures_end;
	load_program(upload_info);

	VkBufferView persistent_buffer = m_persistent_attribute_storage ? m_persistent_attribute_storage->value : null_buffer_view->value;
	VkBufferView volatile_buffer = m_volatile_attribute_storage ? m_volatile_attribute_storage->value : null_buffer_view->value;
	m_program->bind_uniform(persistent_buffer, "persistent_input_stream", m_current_frame->descriptor_set);
	m_program->bind_uniform(volatile_buffer, "volatile_input_stream", m_current_frame->descriptor_set);

	std::chrono::time_point<steady_clock> program_stop = steady_clock::now();
	m_setup_time += std::chrono::duration_cast<std::chrono::microseconds>(program_stop - program_start).count();

	textures_start = program_stop;

	for (int i = 0; i < rsx::limits::fragment_textures_count; ++i)
	{
		if (m_program->has_uniform(rsx::constants::fragment_texture_names[i]))
		{
			if (!rsx::method_registers.fragment_textures[i].enabled())
			{
				m_program->bind_uniform({ vk::null_sampler(), vk::null_image_view(*m_current_command_buffer), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, rsx::constants::fragment_texture_names[i], m_current_frame->descriptor_set);
				continue;
			}

			auto sampler_state = static_cast<vk::texture_cache::sampled_image_descriptor*>(fs_sampler_state[i].get());
			auto image_ptr = sampler_state->image_handle;

			if (!image_ptr && sampler_state->external_subresource_desc.external_handle)
			{
				//Requires update, copy subresource
				image_ptr = m_texture_cache.create_temporary_subresource(*m_current_command_buffer, sampler_state->external_subresource_desc);
			}

			if (!image_ptr)
			{
				LOG_ERROR(RSX, "Texture upload failed to texture index %d. Binding null sampler.", i);
				m_program->bind_uniform({ vk::null_sampler(), vk::null_image_view(*m_current_command_buffer), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, rsx::constants::fragment_texture_names[i], m_current_frame->descriptor_set);
				continue;
			}

			m_program->bind_uniform({ fs_sampler_handles[i]->value, image_ptr->value, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, rsx::constants::fragment_texture_names[i], m_current_frame->descriptor_set);
		}
	}

	for (int i = 0; i < rsx::limits::vertex_textures_count; ++i)
	{
		if (m_program->has_uniform(rsx::constants::vertex_texture_names[i]))
		{
			if (!rsx::method_registers.vertex_textures[i].enabled())
			{
				m_program->bind_uniform({ vk::null_sampler(), vk::null_image_view(*m_current_command_buffer), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, rsx::constants::vertex_texture_names[i], m_current_frame->descriptor_set);
				continue;
			}

			auto sampler_state = static_cast<vk::texture_cache::sampled_image_descriptor*>(vs_sampler_state[i].get());
			auto image_ptr = sampler_state->image_handle;

			if (!image_ptr && sampler_state->external_subresource_desc.external_handle)
			{
				image_ptr = m_texture_cache.create_temporary_subresource(*m_current_command_buffer, sampler_state->external_subresource_desc);
				m_vertex_textures_dirty[i] = true;
			}

			if (!image_ptr)
			{
				LOG_ERROR(RSX, "Texture upload failed to vtexture index %d. Binding null sampler.", i);
				m_program->bind_uniform({ vk::null_sampler(), vk::null_image_view(*m_current_command_buffer), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, rsx::constants::vertex_texture_names[i], m_current_frame->descriptor_set);
				continue;
			}

			m_program->bind_uniform({ vs_sampler_handles[i]->value, image_ptr->value, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, rsx::constants::vertex_texture_names[i], m_current_frame->descriptor_set);
		}
	}

	textures_end = steady_clock::now();
	m_textures_upload_time += std::chrono::duration_cast<std::chrono::microseconds>(textures_end - textures_start).count();

	//While vertex upload is an interruptible process, if we made it this far, there's no need to sync anything that occurs past this point
	//Only textures are synchronized tightly with the GPU and they have been read back above
	vk::enter_uninterruptible();

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
				LOG_ERROR(RSX, "Occlusion pool overflow");
				if (m_current_task) m_current_task->result = 1;
			}
		}
	}

	vkCmdBindPipeline(*m_current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_program->pipeline);
	vkCmdBindDescriptorSets(*m_current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &m_current_frame->descriptor_set, 0, nullptr);

	update_draw_state();

	begin_render_pass();

	bool primitive_emulated = false;
	vk::get_appropriate_topology(rsx::method_registers.current_draw_clause.primitive, primitive_emulated);

	const bool single_draw = (!supports_multidraw ||
		rsx::method_registers.current_draw_clause.first_count_commands.size() <= 1 ||
		rsx::method_registers.current_draw_clause.is_disjoint_primitive);

	if (m_occlusion_query_active && (occlusion_id != UINT32_MAX))
	{
		//Begin query
		m_occlusion_query_pool.begin_query(*m_current_command_buffer, occlusion_id);
		m_occlusion_map[m_active_query_info->driver_handle].indices.push_back(occlusion_id);
		m_occlusion_map[m_active_query_info->driver_handle].command_buffer_to_wait = m_current_command_buffer;
	}

	if (!upload_info.index_info)
	{
		if (single_draw)
		{
			vkCmdDraw(*m_current_command_buffer, upload_info.vertex_draw_count, 1, 0, 0);
		}
		else
		{
			const auto base_vertex = rsx::method_registers.current_draw_clause.first_count_commands.front().first;
			for (const auto &range : rsx::method_registers.current_draw_clause.first_count_commands)
			{
				vkCmdDraw(*m_current_command_buffer, range.second, 1, range.first - base_vertex, 0);
			}
		}
	}
	else
	{
		VkIndexType index_type;
		const u32 index_count = upload_info.vertex_draw_count;
		VkDeviceSize offset;

		std::tie(offset, index_type) = upload_info.index_info.value();
		vkCmdBindIndexBuffer(*m_current_command_buffer, m_index_buffer_ring_info.heap->value, offset, index_type);

		if (single_draw)
		{
			vkCmdDrawIndexed(*m_current_command_buffer, index_count, 1, 0, 0, 0);
		}
		else
		{
			u32 first_vertex = 0;
			for (const auto &range : rsx::method_registers.current_draw_clause.first_count_commands)
			{
				const auto verts = get_index_count(rsx::method_registers.current_draw_clause.primitive, range.second);
				vkCmdDrawIndexed(*m_current_command_buffer, verts, 1, first_vertex, 0, 0);
				first_vertex += verts;
			}
		}
	}

	if (m_occlusion_query_active && (occlusion_id != UINT32_MAX))
	{
		//End query
		m_occlusion_query_pool.end_query(*m_current_command_buffer, occlusion_id);
	}

	close_render_pass();
	vk::leave_uninterruptible();

	std::chrono::time_point<steady_clock> draw_end = steady_clock::now();
	m_draw_time += std::chrono::duration_cast<std::chrono::microseconds>(draw_end - textures_end).count();

	copy_render_targets_to_dma_location();
	m_draw_calls++;

	rsx::thread::end();
}

void VKGSRender::set_viewport()
{
	const auto clip_width = rsx::apply_resolution_scale(rsx::method_registers.surface_clip_width(), true);
	const auto clip_height = rsx::apply_resolution_scale(rsx::method_registers.surface_clip_height(), true);
	u16 scissor_x = rsx::apply_resolution_scale(rsx::method_registers.scissor_origin_x(), false);
	u16 scissor_w = rsx::apply_resolution_scale(rsx::method_registers.scissor_width(), true);
	u16 scissor_y = rsx::apply_resolution_scale(rsx::method_registers.scissor_origin_y(), false);
	u16 scissor_h = rsx::apply_resolution_scale(rsx::method_registers.scissor_height(), true);

	//NOTE: The scale_offset matrix already has viewport matrix factored in
	VkViewport viewport = {};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = clip_width;
	viewport.height = clip_height;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	vkCmdSetViewport(*m_current_command_buffer, 0, 1, &viewport);

	VkRect2D scissor = {};
	scissor.extent.height = scissor_h;
	scissor.extent.width = scissor_w;
	scissor.offset.x = scissor_x;
	scissor.offset.y = scissor_y;

	vkCmdSetScissor(*m_current_command_buffer, 0, 1, &scissor);

	if (scissor_x >= viewport.width || scissor_y >= viewport.height || scissor_w == 0 || scissor_h == 0)
	{
		if (!g_cfg.video.strict_rendering_mode)
		{
			framebuffer_status_valid = false;
			return;
		}
	}
}

void VKGSRender::on_init_thread()
{
	if (m_device == VK_NULL_HANDLE)
	{
		fmt::throw_exception("No vulkan device was created");
	}

	GSRender::on_init_thread();
	rsx_thread = std::this_thread::get_id();
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
			rsx::overlays::message_dialog *dlg = nullptr;

			native_helper(VKGSRender *ptr) :
				owner(ptr) {}

			void create() override
			{
				MsgDialogType type = {};
				type.disable_cancel = true;
				type.progress_bar_count = 1;

				dlg = fxm::get<rsx::overlays::display_manager>()->create<rsx::overlays::message_dialog>();
				dlg->show("Loading precompiled shaders from disk...", type, [](s32 status)
				{
					if (status != CELL_OK)
						Emu.Stop();
				});
			}

			void update_msg(u32 processed, u32 entry_count) override
			{
				dlg->progress_bar_set_message(0, fmt::format("Loading pipeline object %u of %u", processed, entry_count));
				owner->flip(0);
			}

			void inc_value(u32 value) override
			{
				dlg->progress_bar_increment(0, (f32)value);
				owner->flip(0);
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
	}
}

void VKGSRender::on_exit()
{
	zcull_ctrl.release();
	return GSRender::on_exit();
}

void VKGSRender::clear_surface(u32 mask)
{
	if (skip_frame || renderer_unavailable) return;

	// Ignore invalid clear flags
	if (!(mask & 0xF3)) return;

	u8 ctx = rsx::framebuffer_creation_context::context_draw;
	if (mask & 0xF0) ctx |= rsx::framebuffer_creation_context::context_clear_color;
	if (mask & 0x3) ctx |= rsx::framebuffer_creation_context::context_clear_depth;
	init_buffers((rsx::framebuffer_creation_context)ctx);

	if (!framebuffer_status_valid) return;

	copy_render_targets_to_dma_location();

	float depth_clear = 1.f;
	u32   stencil_clear = 0;
	u32   depth_stencil_mask = 0;

	std::vector<VkClearAttachment> clear_descriptors;
	VkClearValue depth_stencil_clear_values = {}, color_clear_values = {};

	const auto scale = rsx::get_resolution_scale();
	u16 scissor_x = rsx::apply_resolution_scale(rsx::method_registers.scissor_origin_x(), false);
	u16 scissor_w = rsx::apply_resolution_scale(rsx::method_registers.scissor_width(), true);
	u16 scissor_y = rsx::apply_resolution_scale(rsx::method_registers.scissor_origin_y(), false);
	u16 scissor_h = rsx::apply_resolution_scale(rsx::method_registers.scissor_height(), true);

	const u16 fb_width = m_draw_fbo->width();
	const u16 fb_height = m_draw_fbo->height();

	//clip region
	std::tie(scissor_x, scissor_y, scissor_w, scissor_h) = rsx::clip_region<u16>(fb_width, fb_height, scissor_x, scissor_y, scissor_w, scissor_h, true);
	VkClearRect region = { { { scissor_x, scissor_y },{ scissor_w, scissor_h } }, 0, 1 };

	auto targets = rsx::utility::get_rtt_indexes(rsx::method_registers.surface_color_target());
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
		if (surface_depth_format == rsx::surface_depth_format::z24s8 &&
			rsx::method_registers.stencil_mask() != 0)
		{
			u8 clear_stencil = rsx::method_registers.stencil_clear_value();
			depth_stencil_clear_values.depthStencil.stencil = clear_stencil;

			depth_stencil_mask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}

	if (auto colormask = (mask & 0xF0))
	{
		if (m_draw_buffers_count > 0)
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
			};

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
					for (u32 index = 0; index < m_draw_buffers_count; ++index)
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

					const auto fbo_format = vk::get_compatible_surface_format(rsx::method_registers.surface_color()).first;
					const auto rp_index = vk::get_render_pass_location(fbo_format, VK_FORMAT_UNDEFINED, 1);
					const auto renderpass = m_render_passes[rp_index];

					m_attachment_clear_pass->update_config(colormask, clear_color);

					for (u32 index = 0; index < m_draw_buffers_count; ++index)
					{
						if (auto rtt = std::get<1>(m_rtts.m_bound_render_targets[index]))
						{
							vk::insert_texture_barrier(*m_current_command_buffer, rtt);
							m_attachment_clear_pass->run(*m_current_command_buffer, rtt,
								region.rect, renderpass, m_framebuffers_to_clean);
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
					if (std::get<0>(rtt) != 0)
					{
						std::get<1>(rtt)->dirty = false;
						std::get<1>(rtt)->old_contents = nullptr;
					}
				}
			}
		}
	}

	if (mask & 0x3)
	{
		if (std::get<0>(m_rtts.m_bound_depth_stencil) != 0)
		{
			std::get<1>(m_rtts.m_bound_depth_stencil)->dirty = false;
			std::get<1>(m_rtts.m_bound_depth_stencil)->old_contents = nullptr;

			clear_descriptors.push_back({ (VkImageAspectFlags)depth_stencil_mask, 0, depth_stencil_clear_values });
		}
	}

	if (clear_descriptors.size() > 0)
	{
		begin_render_pass();
		vkCmdClearAttachments(*m_current_command_buffer, (u32)clear_descriptors.size(), clear_descriptors.data(), 1, &region);
		close_render_pass();
	}
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

			m_texture_cache.flush_memory_to_cache(m_surface_info[index].address, m_surface_info[index].pitch * m_surface_info[index].height, true,
					*m_current_command_buffer, m_swapchain->get_graphics_queue());
		}
	}

	if (g_cfg.video.write_depth_buffer)
	{
		if (m_depth_surface_info.pitch)
		{
			m_texture_cache.flush_memory_to_cache(m_depth_surface_info.address, m_depth_surface_info.pitch * m_depth_surface_info.height, true,
				*m_current_command_buffer, m_swapchain->get_graphics_queue());
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

		m_last_flushable_cb = -1;
		m_flush_requests.clear_pending_flag();
	}
	else
	{
		//Mark this queue as pending
		m_current_command_buffer->pending = true;
		
		//Grab next cb in line and make it usable
		m_current_cb_index = (m_current_cb_index + 1) % VK_MAX_ASYNC_CB_COUNT;
		m_current_command_buffer = &m_primary_cb_list[m_current_cb_index];

		//Soft sync if a present has not yet occured before consuming the wait event
		for (auto &ctx : frame_context_storage)
		{
			if (ctx.swap_command_buffer == m_current_command_buffer)
				process_swap_request(&ctx, true);
		}

		m_current_command_buffer->reset();

		if (m_last_flushable_cb == m_current_cb_index)
			m_last_flushable_cb = -1;
	}

	open_command_buffer();
}

void VKGSRender::advance_queued_frames()
{
	//Check all other frames for completion and clear resources
	for (auto &ctx : frame_context_storage)
	{
		if (&ctx == m_current_frame)
			continue;

		if (ctx.swap_command_buffer)
		{
			ctx.swap_command_buffer->poke();
			if (ctx.swap_command_buffer->pending)
				continue;

			//Present the bound image
			process_swap_request(&ctx, true);
		}
	}

	//m_rtts storage is double buffered and should be safe to tag on frame boundary
	m_rtts.free_invalidated();

	//texture cache is also double buffered to prevent use-after-free
	m_texture_cache.on_frame_end();
	m_samplers_dirty.store(true);

	//Remove stale framebuffers. Ref counted to prevent use-after-free
	m_framebuffers_to_clean.remove_if([](std::unique_ptr<vk::framebuffer_holder>& fbo)
	{
		if (fbo->deref_count >= 2) return true;
		fbo->deref_count++;
		return false;
	});

	m_vertex_cache->purge();
	m_current_frame->tag_frame_end(m_attrib_ring_info.get_current_put_pos_minus_one(),
		m_uniform_buffer_ring_info.get_current_put_pos_minus_one(),
		m_transform_constants_ring_info.get_current_put_pos_minus_one(),
		m_index_buffer_ring_info.get_current_put_pos_minus_one(),
		m_texture_upload_buffer_ring_info.get_current_put_pos_minus_one());

	m_current_queue_index = (m_current_queue_index + 1) % VK_MAX_ASYNC_FRAMES;
	m_current_frame = &frame_context_storage[m_current_queue_index];

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
		case VK_SUBOPTIMAL_KHR:
			break;
		case VK_ERROR_OUT_OF_DATE_KHR:
			present_surface_dirty_flag = true;
			break;
		default:
			vk::die_with_error(HERE, error);
		}
	}

	//Presentation image released; reset value
	ctx->present_image = UINT32_MAX;

	vk::advance_completed_frame_counter();
}

void VKGSRender::queue_swap_request()
{
	//buffer the swap request and return
	if (m_current_frame->swap_command_buffer &&
		m_current_frame->swap_command_buffer->pending)
	{
		//Its probable that no actual drawing took place
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
		close_and_submit_command_buffer({ m_current_frame->present_semaphore }, m_current_command_buffer->submit_fence);
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
		//Perform hard swap here
		ctx->swap_command_buffer->wait();
		free_resources = true;
	}

	//Always present
	present(ctx);

	if (free_resources)
	{
		//Cleanup of reference sensitive resources
		//TODO: These should be double buffered as well to prevent destruction of anything in use
		if (g_cfg.video.overlay)
		{
			m_text_writer->reset_descriptors();
		}

		if (m_overlay_manager && m_overlay_manager->has_dirty())
		{
			for (const auto& view : m_overlay_manager->get_dirty())
			{
				m_ui_renderer->remove_temp_resources(view->uid);
			}

			m_overlay_manager->clear_dirty();
		}

		m_attachment_clear_pass->free_resources();
		m_depth_converter->free_resources();
		m_depth_scaler->free_resources();
		m_ui_renderer->free_resources();

		ctx->buffer_views_to_clean.clear();
		ctx->samplers_to_clean.clear();

		if (ctx->last_frame_sync_time > m_last_heap_sync_time)
		{
			m_last_heap_sync_time = ctx->last_frame_sync_time;

			//Heap cleanup; deallocates memory consumed by the frame if it is still held
			m_attrib_ring_info.m_get_pos = ctx->attrib_heap_ptr;
			m_uniform_buffer_ring_info.m_get_pos = ctx->ubo_heap_ptr;
			m_transform_constants_ring_info.m_get_pos = ctx->vtxconst_heap_ptr;
			m_index_buffer_ring_info.m_get_pos = ctx->index_heap_ptr;
			m_texture_upload_buffer_ring_info.m_get_pos = ctx->texture_upload_heap_ptr;

			m_attrib_ring_info.notify();
			m_uniform_buffer_ring_info.notify();
			m_transform_constants_ring_info.notify();
			m_index_buffer_ring_info.notify();
			m_texture_upload_buffer_ring_info.notify();
		}
	}

	ctx->swap_command_buffer = nullptr;
}

void VKGSRender::do_local_task(bool /*idle*/)
{
	if (m_flush_requests.pending())
	{
		std::lock_guard<shared_mutex> lock(m_flush_queue_mutex);

		//TODO: Determine if a hard sync is necessary
		//Pipeline barriers later may do a better job synchronizing than wholly stalling the pipeline
		flush_command_queue();

		m_flush_requests.clear_pending_flag();
		m_flush_requests.consumer_wait();
	}
	else if (!in_begin_end)
	{
		//This will re-engage locks and break the texture cache if another thread is waiting in access violation handler!
		//Only call when there are no waiters
		m_texture_cache.do_update();
	}

	if (m_last_flushable_cb > -1)
	{
		auto cb = &m_primary_cb_list[m_last_flushable_cb];

		if (cb->pending)
			cb->poke();

		if (!cb->pending)
			m_last_flushable_cb = -1;
	}

#ifdef _WIN32

	switch (m_frame->get_wm_event())
	{
	case wm_event::none:
		break;
	case wm_event::geometry_change_notice:
	{
		//Stall until finish notification is received. Also, raise surface dirty flag
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
				timeout += 10; //extend timeout to wait for user to finish resizing
				break;
			case wm_event::window_restored:
			case wm_event::window_visibility_changed:
			case wm_event::window_minimized:
			case wm_event::window_moved:
				handled = true; //ignore these events as they do not alter client area
				break;
			}

			if (handled)
				break;
			else
			{
				//wait for window manager event
				std::this_thread::sleep_for(10ms);
				timeout -= 10;
			}

			//reset renderer availability if something has changed about the window
			renderer_unavailable = false;
		}

		if (!timeout)
		{
			LOG_ERROR(RSX, "wm event handler timed out");
		}

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

#else

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

	if (m_overlay_manager)
	{
		if (!in_begin_end && native_ui_flip_request.load())
		{
			native_ui_flip_request.store(false);
			flush_command_queue(true);
			flip((s32)current_display_buffer);
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
	case NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE:
		sync_at_semaphore_release();
		return false; //call rsx::thread method implementation
	default:
		return false;
	}
}

bool VKGSRender::check_program_status()
{
	return (rsx::method_registers.shader_program_address() != 0);
}

void VKGSRender::load_program(const vk::vertex_upload_info& vertex_info)
{
	if (m_graphics_state & rsx::pipeline_state::invalidate_pipeline_bits)
	{
		get_current_fragment_program(fs_sampler_state);
		verify(HERE), current_fragment_program.valid;

		get_current_vertex_program();
	}

	auto &vertex_program = current_vertex_program;
	auto &fragment_program = current_fragment_program;
	auto old_program = m_program;

	vk::pipeline_props properties = {};

	// Input assembly
	bool emulated_primitive_type;
	properties.state.set_primitive_type(vk::get_appropriate_topology(rsx::method_registers.current_draw_clause.primitive, emulated_primitive_type));

	const bool restarts_valid = rsx::method_registers.current_draw_clause.command == rsx::draw_command::indexed && !emulated_primitive_type && !rsx::method_registers.current_draw_clause.is_disjoint_primitive;
	if (rsx::method_registers.restart_index_enabled() && !vk::emulate_primitive_restart(rsx::method_registers.current_draw_clause.primitive) && restarts_valid)
		properties.state.enable_primitive_restart();

	// Rasterizer state
	properties.state.set_attachment_count(m_draw_buffers_count);
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

	bool blend_equation_override = false;
	VkBlendFactor sfactor_rgb, sfactor_a, dfactor_rgb, dfactor_a;
	VkBlendOp equation_rgb, equation_a;

	if (rsx::method_registers.msaa_alpha_to_coverage_enabled() &&
		!rsx::method_registers.alpha_test_enabled())
	{
		if (rsx::method_registers.msaa_enabled() &&
			rsx::method_registers.msaa_sample_mask() &&
			rsx::method_registers.surface_antialias() != rsx::surface_antialiasing::center_1_sample)
		{
			//fake alpha-to-coverage
			//blend used in conjunction with alpha test to fake order-independent edge transparency
			mrt_blend_enabled[0] = mrt_blend_enabled[1] = mrt_blend_enabled[2] = mrt_blend_enabled[3] = true;
			blend_equation_override = true;
			sfactor_rgb = sfactor_a = VK_BLEND_FACTOR_SRC_ALPHA;
			dfactor_rgb = dfactor_a = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			equation_rgb = equation_a = VK_BLEND_OP_ADD;
		}
	}

	if (mrt_blend_enabled[0] || mrt_blend_enabled[1] || mrt_blend_enabled[2] || mrt_blend_enabled[3])
	{
		if (!blend_equation_override)
		{
			sfactor_rgb = vk::get_blend_factor(rsx::method_registers.blend_func_sfactor_rgb());
			sfactor_a = vk::get_blend_factor(rsx::method_registers.blend_func_sfactor_a());
			dfactor_rgb = vk::get_blend_factor(rsx::method_registers.blend_func_dfactor_rgb());
			dfactor_a = vk::get_blend_factor(rsx::method_registers.blend_func_dfactor_a());
			equation_rgb = vk::get_blend_op(rsx::method_registers.blend_equation_rgb());
			equation_a = vk::get_blend_op(rsx::method_registers.blend_equation_a());
		}

		for (u8 idx = 0; idx < m_draw_buffers_count; ++idx)
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

	properties.render_pass = m_render_passes[m_current_renderpass_id];
	properties.render_pass_location = (int)m_current_renderpass_id;
	properties.num_targets = m_draw_buffers_count;

	vk::enter_uninterruptible();

	//Load current program from buffer
	vertex_program.skip_vertex_input_check = true;
	fragment_program.unnormalized_coords = 0;
	m_program = m_prog_buffer->getGraphicPipelineState(vertex_program, fragment_program, properties, *m_device, pipeline_layout).get();

	if (m_prog_buffer->check_cache_missed())
	{
		m_shaders_cache->store(properties, vertex_program, fragment_program);

		//Notify the user with HUD notification
		if (g_cfg.misc.show_shader_compilation_hint)
		{
			if (m_overlay_manager)
			{
				if (auto dlg = m_overlay_manager->get<rsx::overlays::shader_compile_notification>())
				{
					//Extend duration
					dlg->touch();
				}
				else
				{
					//Create dialog but do not show immediately
					m_overlay_manager->create<rsx::overlays::shader_compile_notification>();
				}
			}
		}
	}

	vk::leave_uninterruptible();

	if (1)//m_graphics_state & (rsx::pipeline_state::fragment_state_dirty | rsx::pipeline_state::vertex_state_dirty))
	{
		const size_t fragment_constants_sz = m_prog_buffer->get_fragment_constants_buffer_size(fragment_program);
		const size_t fragment_buffer_sz = fragment_constants_sz + (18 * 4 * sizeof(float));
		const size_t required_mem = 512 + fragment_buffer_sz;

		const size_t vertex_state_offset = m_uniform_buffer_ring_info.alloc<256>(required_mem);
		const size_t fragment_constants_offset = vertex_state_offset + 512;

		//We do this in one go
		u8 *buf = (u8*)m_uniform_buffer_ring_info.map(vertex_state_offset, required_mem);

		//Vertex state
		fill_scale_offset_data(buf, false);
		fill_user_clip_data(buf + 64);
		*(reinterpret_cast<u32*>(buf + 128)) = rsx::method_registers.transform_branch_bits();
		*(reinterpret_cast<u32*>(buf + 132)) = vertex_info.vertex_index_base;
		*(reinterpret_cast<f32*>(buf + 136)) = rsx::method_registers.point_size();
		*(reinterpret_cast<f32*>(buf + 140)) = rsx::method_registers.clip_min();
		*(reinterpret_cast<f32*>(buf + 144)) = rsx::method_registers.clip_max();

		fill_vertex_layout_state(m_vertex_layout, vertex_info.allocated_vertex_count, reinterpret_cast<s32*>(buf + 160),
				vertex_info.persistent_window_offset, vertex_info.volatile_window_offset);
		
		//Fragment constants
		buf = buf + 512;
		if (fragment_constants_sz)
		{
			m_prog_buffer->fill_fragment_constants_buffer({ reinterpret_cast<float*>(buf), ::narrow<int>(fragment_constants_sz) },
					fragment_program, vk::sanitize_fp_values());
		}

		fill_fragment_state_buffer(buf + fragment_constants_sz, fragment_program);
		
		m_uniform_buffer_ring_info.unmap();

		m_vertex_state_buffer_info = { m_uniform_buffer_ring_info.heap->value, vertex_state_offset, 512 };
		m_fragment_state_buffer_info = { m_uniform_buffer_ring_info.heap->value, fragment_constants_offset, fragment_buffer_sz };
	}

	if (m_graphics_state & rsx::pipeline_state::transform_constants_dirty)
	{
		//Vertex constants
		const size_t vertex_constants_offset = m_transform_constants_ring_info.alloc<256>(8192);
		auto buf = m_transform_constants_ring_info.map(vertex_constants_offset, 8192);

		fill_vertex_program_constants_data(buf);
		m_transform_constants_ring_info.unmap();
		m_vertex_constants_buffer_info = { m_transform_constants_ring_info.heap->value, vertex_constants_offset, 8192 };
	}

	if (1)//m_graphics_state || old_program != m_program)
	{
		m_program->bind_uniform(m_vertex_state_buffer_info, SCALE_OFFSET_BIND_SLOT, m_current_frame->descriptor_set);
		m_program->bind_uniform(m_vertex_constants_buffer_info, VERTEX_CONSTANT_BUFFERS_BIND_SLOT, m_current_frame->descriptor_set);
		m_program->bind_uniform(m_fragment_state_buffer_info, FRAGMENT_CONSTANT_BUFFERS_BIND_SLOT, m_current_frame->descriptor_set);
	}

	//Clear flags
	m_graphics_state = 0;
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

void VKGSRender::init_buffers(rsx::framebuffer_creation_context context, bool skip_reading)
{
	//Clear any pending swap requests
	//TODO: Decide on what to do if we circle back to a new frame before the previous frame waiting on it is still pending
	//Dropping the frame would in theory allow the thread to advance faster
	for (auto &ctx : frame_context_storage)
	{
		if (ctx.swap_command_buffer)
		{
			if (ctx.swap_command_buffer->pending)
			{
				ctx.swap_command_buffer->poke();

				if (&ctx == m_current_frame && ctx.swap_command_buffer->pending)
				{
					//Instead of stopping to wait, use the aux storage to ease pressure
					m_aux_frame_context.grab_resources(*m_current_frame);
					m_current_frame = &m_aux_frame_context;
				}
			}

			if (!ctx.swap_command_buffer->pending)
			{
				//process swap without advancing the frame base
				process_swap_request(&ctx, true);
			}
		}
	}

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
	if (m_draw_fbo && !m_rtts_dirty)
		return;

	copy_render_targets_to_dma_location();
	m_rtts_dirty = false;

	u32 clip_width = rsx::method_registers.surface_clip_width();
	u32 clip_height = rsx::method_registers.surface_clip_height();
	u32 clip_x = rsx::method_registers.surface_clip_origin_x();
	u32 clip_y = rsx::method_registers.surface_clip_origin_y();

	framebuffer_status_valid = false;
	m_framebuffer_state_contested = false;

	if (clip_width == 0 || clip_height == 0)
	{
		LOG_ERROR(RSX, "Invalid framebuffer setup, w=%d, h=%d", clip_width, clip_height);
		return;
	}

	auto surface_addresses = get_color_surface_addresses();
	auto zeta_address = get_zeta_surface_address();

	const auto zeta_pitch = rsx::method_registers.surface_z_pitch();
	const u32 surface_pitchs[] = { rsx::method_registers.surface_a_pitch(), rsx::method_registers.surface_b_pitch(),
		rsx::method_registers.surface_c_pitch(), rsx::method_registers.surface_d_pitch() };

	const auto color_fmt = rsx::method_registers.surface_color();
	const auto depth_fmt = rsx::method_registers.surface_depth_fmt();
	const auto target = rsx::method_registers.surface_color_target();

	//NOTE: Its is possible that some renders are done on a swizzled context. Pitch is meaningless in that case
	//Seen in Nier (color) and GT HD concept (z buffer)
	//Restriction is that the dimensions are powers of 2. Also, dimensions are passed via log2w and log2h entries
	const auto required_zeta_pitch = std::max<u32>((u32)(depth_fmt == rsx::surface_depth_format::z16 ? clip_width * 2 : clip_width * 4), 64u);
	const auto required_color_pitch = std::max<u32>((u32)rsx::utility::get_packed_pitch(color_fmt, clip_width), 64u);
	const bool stencil_test_enabled = depth_fmt == rsx::surface_depth_format::z24s8 && rsx::method_registers.stencil_test_enabled();
	const auto lg2w = rsx::method_registers.surface_log2_width();
	const auto lg2h = rsx::method_registers.surface_log2_height();
	const auto clipw_log2 = (u32)floor(log2(clip_width));
	const auto cliph_log2 = (u32)floor(log2(clip_height));

	if (zeta_address)
	{
		if (!rsx::method_registers.depth_test_enabled() &&
			!stencil_test_enabled &&
			target != rsx::surface_target::none)
		{
			//Disable depth buffer if depth testing is not enabled, unless a clear command is targeting the depth buffer
			const bool is_depth_clear = !!(context & rsx::framebuffer_creation_context::context_clear_depth);
			if (!is_depth_clear)
			{
				zeta_address = 0;
				m_framebuffer_state_contested = true;
			}
		}

		if (zeta_address && zeta_pitch < required_zeta_pitch)
		{
			if (lg2w < clipw_log2 || lg2h < cliph_log2)
			{
				//Cannot fit
				zeta_address = 0;

				if (lg2w > 0 || lg2h > 0)
				{
					//Something was actually declared for the swizzle context dimensions
					LOG_WARNING(RSX, "Invalid swizzled context depth surface dims, LG2W=%d, LG2H=%d, clip_w=%d, clip_h=%d", lg2w, lg2h, clip_width, clip_height);
				}
			}
			else
			{
				LOG_TRACE(RSX, "Swizzled context depth surface, LG2W=%d, LG2H=%d, clip_w=%d, clip_h=%d", lg2w, lg2h, clip_width, clip_height);
			}
		}
	}

	for (const auto &index : rsx::utility::get_rtt_indexes(target))
	{
		if (surface_pitchs[index] < required_color_pitch)
		{
			if (lg2w < clipw_log2 || lg2h < cliph_log2)
			{
				surface_addresses[index] = 0;

				if (lg2w > 0 || lg2h > 0)
				{
					//Something was actually declared for the swizzle context dimensions
					LOG_WARNING(RSX, "Invalid swizzled context color surface dims, LG2W=%d, LG2H=%d, clip_w=%d, clip_h=%d", lg2w, lg2h, clip_width, clip_height);
				}
			}
			else
			{
				LOG_TRACE(RSX, "Swizzled context color surface, LG2W=%d, LG2H=%d, clip_w=%d, clip_h=%d", lg2w, lg2h, clip_width, clip_height);
			}
		}

		if (surface_addresses[index] == zeta_address)
		{
			LOG_TRACE(RSX, "Framebuffer at 0x%X has aliasing color/depth targets, zeta_pitch = %d, color_pitch=%d", zeta_address, zeta_pitch, surface_pitchs[index]);
			if (context == rsx::framebuffer_creation_context::context_clear_depth ||
				rsx::method_registers.depth_test_enabled() || stencil_test_enabled ||
				(!rsx::method_registers.color_write_enabled() && rsx::method_registers.depth_write_enabled()))
			{
				// Use address for depth data
				// TODO: create a temporary render buffer for this to keep MRT outputs aligned
				surface_addresses[index] = 0;
			}
			else
			{
				// Use address for color data
				zeta_address = 0;
				m_framebuffer_state_contested = true;
				break;
			}
		}

		if (surface_addresses[index])
			framebuffer_status_valid = true;
	}

	if (!framebuffer_status_valid && !zeta_address)
	{
		LOG_WARNING(RSX, "Framebuffer setup failed. Draw calls may have been lost");
		return;
	}

	//At least one attachment exists
	framebuffer_status_valid = true;

	const auto fbo_width = rsx::apply_resolution_scale(clip_width, true);
	const auto fbo_height = rsx::apply_resolution_scale(clip_height, true);
	const auto aa_mode = rsx::method_registers.surface_antialias();
	const auto bpp = get_format_block_size_in_bytes(color_fmt);
	const u32 aa_factor = (aa_mode == rsx::surface_antialiasing::center_1_sample || aa_mode == rsx::surface_antialiasing::diagonal_centered_2_samples) ? 1 : 2;

	//Window (raster) offsets
	const auto window_offset_x = rsx::method_registers.window_offset_x();
	const auto window_offset_y = rsx::method_registers.window_offset_y();
	const auto window_clip_width = rsx::method_registers.window_clip_horizontal();
	const auto window_clip_height = rsx::method_registers.window_clip_vertical();

	if (window_offset_x || window_offset_y)
	{
		//Window offset is what affects the raster position!
		//Tested with Turbo: Super stunt squad that only changes the window offset to declare new framebuffers
		//Sampling behavior clearly indicates the addresses are expected to have changed
		if (auto clip_type = rsx::method_registers.window_clip_type())
			LOG_ERROR(RSX, "Unknown window clip type 0x%X" HERE, clip_type);

		for (const auto &index : rsx::utility::get_rtt_indexes(target))
		{
			if (surface_addresses[index])
			{
				const u32 window_offset_bytes = (std::max<u32>(surface_pitchs[index], clip_width * aa_factor * bpp) * window_offset_y) + ((aa_factor * bpp) * window_offset_x);
				surface_addresses[index] += window_offset_bytes;
			}
		}

		if (zeta_address)
		{
			const auto depth_bpp = (depth_fmt == rsx::surface_depth_format::z16 ? 2 : 4);
			zeta_address += (std::max<u32>(zeta_pitch, clip_width * aa_factor * depth_bpp) * window_offset_y) + ((aa_factor * depth_bpp) * window_offset_x);
		}
	}

	if ((window_clip_width && window_clip_width < clip_width) ||
		(window_clip_height && window_clip_height < clip_height))
	{
		LOG_ERROR(RSX, "Unexpected window clip dimensions: window_clip=%dx%d, surface_clip=%dx%d",
			window_clip_width, window_clip_height, clip_width, clip_height);
	}

	if (m_draw_fbo)
	{
		bool really_changed = false;

		if (m_draw_fbo->width() == fbo_width && m_draw_fbo->height() == fbo_height)
		{
			for (u8 i = 0; i < rsx::limits::color_buffers_count; ++i)
			{
				if (m_surface_info[i].address != surface_addresses[i])
				{
					really_changed = true;
					break;
				}
			}

			if (!really_changed)
			{
				if (zeta_address == m_depth_surface_info.address)
				{
					//Nothing has changed, we're still using the same framebuffer
					return;
				}
			}
		}
	}

	m_rtts.prepare_render_target(&*m_current_command_buffer,
		color_fmt, depth_fmt,
		clip_width, clip_height,
		target,
		surface_addresses, zeta_address,
		(*m_device), &*m_current_command_buffer);

	//Reset framebuffer information
	VkFormat old_format = VK_FORMAT_UNDEFINED;
	for (u8 i = 0; i < rsx::limits::color_buffers_count; ++i)
	{
		//Flush old address if we keep missing it
		if (m_surface_info[i].pitch && g_cfg.video.write_color_buffers)
		{
			if (old_format == VK_FORMAT_UNDEFINED)
				old_format = vk::get_compatible_surface_format(m_surface_info[i].color_format).first;

			m_texture_cache.set_memory_read_flags(m_surface_info[i].address, m_surface_info[i].pitch * m_surface_info[i].height, rsx::memory_read_flags::flush_once);
			m_texture_cache.flush_if_cache_miss_likely(old_format, m_surface_info[i].address, m_surface_info[i].pitch * m_surface_info[i].height,
				*m_current_command_buffer, m_swapchain->get_graphics_queue());
		}

		m_surface_info[i].address = m_surface_info[i].pitch = 0;
		m_surface_info[i].width = clip_width;
		m_surface_info[i].height = clip_height;
		m_surface_info[i].color_format = color_fmt;
	}

	//Process depth surface as well
	{
		if (m_depth_surface_info.pitch && g_cfg.video.write_depth_buffer)
		{
			auto old_format = vk::get_compatible_depth_surface_format(m_device->get_formats_support(), m_depth_surface_info.depth_format);
			m_texture_cache.set_memory_read_flags(m_depth_surface_info.address, m_depth_surface_info.pitch * m_depth_surface_info.height, rsx::memory_read_flags::flush_once);
			m_texture_cache.flush_if_cache_miss_likely(old_format, m_depth_surface_info.address, m_depth_surface_info.pitch * m_depth_surface_info.height,
				*m_current_command_buffer, m_swapchain->get_graphics_queue());
		}

		m_depth_surface_info.address = m_depth_surface_info.pitch = 0;
		m_depth_surface_info.width = clip_width;
		m_depth_surface_info.height = clip_height;
		m_depth_surface_info.depth_format = depth_fmt;
	}

	//Bind created rtts as current fbo...
	std::vector<u8> draw_buffers = rsx::utility::get_rtt_indexes(target);
	m_draw_buffers_count = 0;

	std::vector<vk::image*> bound_images;
	bound_images.reserve(5);

	for (u8 index : draw_buffers)
	{
		if (auto surface = std::get<1>(m_rtts.m_bound_render_targets[index]))
		{
			bound_images.push_back(surface);

			m_surface_info[index].address = surface_addresses[index];
			m_surface_info[index].pitch = surface_pitchs[index];
			surface->rsx_pitch = surface_pitchs[index];

			surface->aa_mode = aa_mode;
			m_texture_cache.notify_surface_changed(surface_addresses[index]);

			m_texture_cache.tag_framebuffer(surface_addresses[index]);
			m_draw_buffers_count++;
		}
	}

	if (std::get<0>(m_rtts.m_bound_depth_stencil) != 0)
	{
		auto ds = std::get<1>(m_rtts.m_bound_depth_stencil);
		bound_images.push_back(ds);

		m_depth_surface_info.address = zeta_address;
		m_depth_surface_info.pitch = rsx::method_registers.surface_z_pitch();
		ds->rsx_pitch = m_depth_surface_info.pitch;

		ds->aa_mode = aa_mode;
		m_texture_cache.notify_surface_changed(zeta_address);

		m_texture_cache.tag_framebuffer(zeta_address);
	}

	if (g_cfg.video.write_color_buffers)
	{
		const auto color_fmt_info = vk::get_compatible_gcm_format(color_fmt);
		for (u8 index : draw_buffers)
		{
			if (!m_surface_info[index].address || !m_surface_info[index].pitch) continue;

			const u32 range = m_surface_info[index].pitch * m_surface_info[index].height * aa_factor;
			m_texture_cache.lock_memory_region(std::get<1>(m_rtts.m_bound_render_targets[index]), m_surface_info[index].address, range,
				m_surface_info[index].width, m_surface_info[index].height, m_surface_info[index].pitch, color_fmt_info.first, color_fmt_info.second);
		}
	}

	if (g_cfg.video.write_depth_buffer)
	{
		if (m_depth_surface_info.address && m_depth_surface_info.pitch)
		{
			u32 pitch = m_depth_surface_info.width * 2;
			u32 gcm_format = CELL_GCM_TEXTURE_DEPTH16;

			if (m_depth_surface_info.depth_format != rsx::surface_depth_format::z16)
			{
				gcm_format = CELL_GCM_TEXTURE_DEPTH24_D8;
				pitch *= 2;
			}

			const u32 range = pitch * m_depth_surface_info.height * aa_factor;
			m_texture_cache.lock_memory_region(std::get<1>(m_rtts.m_bound_depth_stencil), m_depth_surface_info.address, range,
				m_depth_surface_info.width, m_depth_surface_info.height, m_depth_surface_info.pitch, gcm_format, false);
		}
	}

	auto vk_depth_format = (zeta_address == 0) ? VK_FORMAT_UNDEFINED : vk::get_compatible_depth_surface_format(m_device->get_formats_support(), depth_fmt);
	m_current_renderpass_id = vk::get_render_pass_location(vk::get_compatible_surface_format(color_fmt).first, vk_depth_format, m_draw_buffers_count);

	//Search old framebuffers for this same configuration
	bool framebuffer_found = false;

	for (auto &fbo : m_framebuffers_to_clean)
	{
		if (fbo->matches(bound_images, fbo_width, fbo_height))
		{
			m_draw_fbo.swap(fbo);
			m_draw_fbo->reset_refs();
			framebuffer_found = true;
			break;
		}
	}

	if (!framebuffer_found)
	{
		std::vector<std::unique_ptr<vk::image_view>> fbo_images;
		fbo_images.reserve(5);

		for (u8 index : draw_buffers)
		{
			if (vk::image *raw = std::get<1>(m_rtts.m_bound_render_targets[index]))
			{
				VkImageSubresourceRange subres = {};
				subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				subres.baseArrayLayer = 0;
				subres.baseMipLevel = 0;
				subres.layerCount = 1;
				subres.levelCount = 1;

				fbo_images.push_back(std::make_unique<vk::image_view>(*m_device, raw->value, VK_IMAGE_VIEW_TYPE_2D, raw->info.format, vk::default_component_map(), subres));
			}
		}

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
		}

		VkRenderPass current_render_pass = m_render_passes[m_current_renderpass_id];

		if (m_draw_fbo)
			m_framebuffers_to_clean.push_back(std::move(m_draw_fbo));

		m_draw_fbo.reset(new vk::framebuffer_holder(*m_device, current_render_pass, fbo_width, fbo_height, std::move(fbo_images)));
	}

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

		//Release present image by presenting it
		ctx.swap_command_buffer->wait();
		ctx.swap_command_buffer = nullptr;
		present(&ctx);
	}

	//Wait for completion
	vkDeviceWaitIdle(*m_device);

	//Remove any old refs to the old images as they are about to be destroyed
	m_framebuffers_to_clean.clear();

	//Rebuild swapchain. Old swapchain destruction is handled by the init_swapchain call
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

		vk::change_image_layout(*m_current_command_buffer, target_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, range);
		vkCmdClearColorImage(*m_current_command_buffer, target_image, VK_IMAGE_LAYOUT_GENERAL, &clear_color, 1, &range);
		vk::change_image_layout(*m_current_command_buffer, target_image, VK_IMAGE_LAYOUT_GENERAL, target_layout, range);
	}

	//Will have to block until rendering is completed
	VkFence resize_fence = VK_NULL_HANDLE;
	VkFenceCreateInfo infos = {};
	infos.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	vkCreateFence((*m_device), &infos, nullptr, &resize_fence);

	//Flush the command buffer
	close_and_submit_command_buffer({}, resize_fence);
	CHECK_RESULT(vkWaitForFences((*m_device), 1, &resize_fence, VK_TRUE, UINT64_MAX));
	vkDestroyFence((*m_device), resize_fence, nullptr);

	m_current_command_buffer->reset();
	open_command_buffer();

	present_surface_dirty_flag = false;
	renderer_unavailable = false;
}

void VKGSRender::flip(int buffer)
{
	if (skip_frame || renderer_unavailable)
	{
		m_frame->flip(m_context);
		rsx::thread::flip(buffer);

		if (!skip_frame)
		{
			m_draw_calls = 0;
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
			//Always present if pending swap is present.
			//Its possible this flip request is triggered by overlays and the flip queue is in undefined state
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
			//Unreachable
			fmt::throw_exception("Possible data corruption on frame context storage detected");
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

	if ((u32)buffer < display_buffers_count && buffer_width && buffer_height && buffer_pitch)
	{
		rsx::tiled_region buffer_region = get_tiled_address(display_buffers[buffer].offset, CELL_GCM_LOCATION_LOCAL);
		u32 absolute_address = buffer_region.address + buffer_region.base;

		if (auto render_target_texture = m_rtts.get_texture_from_render_target_if_applicable(absolute_address))
		{
			image_to_flip = render_target_texture;
		}
		else if (auto surface = m_texture_cache.find_texture_from_dimensions(absolute_address))
		{
			//Hack - this should be the first location to check for output
			//The render might have been done offscreen or in software and a blit used to display
			image_to_flip = surface->get_raw_texture();
		}
		else
		{
			//Read from cell
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
			0, 0, image_to_flip->width(), image_to_flip->height(), aspect_ratio.x, aspect_ratio.y, aspect_ratio.width, aspect_ratio.height, 1, VK_IMAGE_ASPECT_COLOR_BIT, false);

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
		vk::change_image_layout(*m_current_command_buffer, m_swapchain->get_image(m_current_frame->present_image), present_layout, VK_IMAGE_LAYOUT_GENERAL, range);
		vkCmdClearColorImage(*m_current_command_buffer, m_swapchain->get_image(m_current_frame->present_image), VK_IMAGE_LAYOUT_GENERAL, &clear_black, 1, &range);
		vk::change_image_layout(*m_current_command_buffer, m_swapchain->get_image(m_current_frame->present_image), VK_IMAGE_LAYOUT_GENERAL, present_layout, range);
	}

	std::unique_ptr<vk::framebuffer_holder> direct_fbo;
	std::vector<std::unique_ptr<vk::image_view>> swap_image_view;
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

		size_t idx = vk::get_render_pass_location(m_swapchain->get_surface_format(), VK_FORMAT_UNDEFINED, 1);
		VkRenderPass single_target_pass = m_render_passes[idx];

		for (auto It = m_framebuffers_to_clean.begin(); It != m_framebuffers_to_clean.end(); It++)
		{
			auto &fbo = *It;
			if (fbo->attachments[0]->info.image == target_image)
			{
				direct_fbo.swap(fbo);
				direct_fbo->reset_refs();
				m_framebuffers_to_clean.erase(It);
				break;
			}
		}

		if (!direct_fbo)
		{
			swap_image_view.push_back(std::make_unique<vk::image_view>(*m_device, target_image, VK_IMAGE_VIEW_TYPE_2D, m_swapchain->get_surface_format(), vk::default_component_map(), subres));
			direct_fbo.reset(new vk::framebuffer_holder(*m_device, single_target_pass, m_client_width, m_client_height, std::move(swap_image_view)));
		}

		if (has_overlay)
		{
			for (const auto& view : m_overlay_manager->get_views())
			{
				m_ui_renderer->run(*m_current_command_buffer, direct_fbo->width(), direct_fbo->height(), direct_fbo.get(), single_target_pass, m_texture_upload_buffer_ring_info, *view.get());
			}
		}

		if (g_cfg.video.overlay)
		{
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0, 0, direct_fbo->width(), direct_fbo->height(), "RSX Load: " + std::to_string(get_load()) + "%");
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0, 18, direct_fbo->width(), direct_fbo->height(), "draw calls: " + std::to_string(m_draw_calls));
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0, 36, direct_fbo->width(), direct_fbo->height(), "draw call setup: " + std::to_string(m_setup_time) + "us");
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0, 54, direct_fbo->width(), direct_fbo->height(), "vertex upload time: " + std::to_string(m_vertex_upload_time) + "us");
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0, 72, direct_fbo->width(), direct_fbo->height(), "texture upload time: " + std::to_string(m_textures_upload_time) + "us");
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0, 90, direct_fbo->width(), direct_fbo->height(), "draw call execution: " + std::to_string(m_draw_time) + "us");
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0, 108, direct_fbo->width(), direct_fbo->height(), "submit and flip: " + std::to_string(m_flip_time) + "us");

			const  auto num_dirty_textures = m_texture_cache.get_unreleased_textures_count();
			const auto texture_memory_size = m_texture_cache.get_texture_memory_in_use() / (1024 * 1024);
			const auto tmp_texture_memory_size = m_texture_cache.get_temporary_memory_in_use() / (1024 * 1024);
			const auto num_flushes = m_texture_cache.get_num_flush_requests();
			const auto num_mispredict = m_texture_cache.get_num_cache_mispredictions();
			const auto cache_miss_ratio = (u32)ceil(m_texture_cache.get_cache_miss_ratio() * 100);
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0, 144, direct_fbo->width(), direct_fbo->height(), "Unreleased textures: " + std::to_string(num_dirty_textures));
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0, 162, direct_fbo->width(), direct_fbo->height(), "Texture cache memory: " + std::to_string(texture_memory_size) + "M");
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0, 180, direct_fbo->width(), direct_fbo->height(), "Temporary texture memory: " + std::to_string(tmp_texture_memory_size) + "M");
			m_text_writer->print_text(*m_current_command_buffer, *direct_fbo, 0, 198, direct_fbo->width(), direct_fbo->height(), fmt::format("Flush requests: %d (%d%% hard faults, %d mispredictions)", num_flushes, cache_miss_ratio, num_mispredict));
		}

		vk::change_image_layout(*m_current_command_buffer, target_image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, present_layout, subres);
		m_framebuffers_to_clean.push_back(std::move(direct_fbo));
	}

	queue_swap_request();

	std::chrono::time_point<steady_clock> flip_end = steady_clock::now();
	m_flip_time = std::chrono::duration_cast<std::chrono::microseconds>(flip_end - flip_start).count();

	//NOTE:Resource destruction is handled within the real swap handler

	m_frame->flip(m_context);
	rsx::thread::flip(buffer);

	//Do not reset perf counters if we are skipping the next frame
	if (skip_frame) return;

	m_draw_calls = 0;
	m_draw_time = 0;
	m_setup_time = 0;
	m_vertex_upload_time = 0;
	m_textures_upload_time = 0;
}

bool VKGSRender::scaled_image_from_memory(rsx::blit_src_info& src, rsx::blit_dst_info& dst, bool interpolate)
{
	if (renderer_unavailable)
		return false;

	//Verify enough memory exists before attempting to handle data transfer
	check_heap_status();

	//Stop all parallel operations until this is finished
	std::lock_guard<shared_mutex> lock(m_secondary_cb_guard);

	auto result = m_texture_cache.blit(src, dst, interpolate, m_rtts, *m_current_command_buffer);
	m_current_command_buffer->begin();

	if (result.succeeded)
	{
		bool require_flush = false;
		if (result.deferred)
		{
			//Requires manual scaling; depth/stencil surface
			auto rp = vk::get_render_pass_location(VK_FORMAT_UNDEFINED, result.dst_image->info.format, 0);
			auto render_pass = m_render_passes[rp];

			auto old_src_layout = result.src_image->current_layout;
			auto old_dst_layout = result.dst_image->current_layout;

			vk::change_image_layout(*m_current_command_buffer, result.src_image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			vk::change_image_layout(*m_current_command_buffer, result.dst_image, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

			m_depth_scaler->run(*m_current_command_buffer, result.dst_image->width(), result.dst_image->height(), result.dst_image,
				result.src_view, render_pass, m_framebuffers_to_clean);

			vk::change_image_layout(*m_current_command_buffer, result.src_image, old_src_layout);
			vk::change_image_layout(*m_current_command_buffer, result.dst_image, old_dst_layout);

			require_flush = true;
		}

		if (result.dst_image)
		{
			if (m_texture_cache.flush_if_cache_miss_likely(result.dst_image->info.format, result.real_dst_address, result.real_dst_size,
				*m_current_command_buffer, m_swapchain->get_graphics_queue()))
				require_flush = true;
		}

		if (require_flush)
			flush_command_queue();

		m_samplers_dirty.store(true);
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

	//Avoid stalling later if this query is already tied to a report
	if (query->num_draws && query->owned && !m_flush_requests.pending())
	{
		m_flush_requests.post(false);
		m_flush_requests.remove_one();
	}
}

bool VKGSRender::check_occlusion_query_status(rsx::reports::occlusion_query_info* query)
{
	if (!query->num_draws)
		return true;

	auto found = m_occlusion_map.find(query->driver_handle);
	if (found == m_occlusion_map.end())
		return true;

	auto &data = found->second;
	if (data.indices.size() == 0)
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
	if (data.indices.size() == 0)
		return;

	if (query->num_draws)
	{
		if (data.command_buffer_to_wait == m_current_command_buffer)
		{
			flush_command_queue();

			//Clear any deferred flush requests from previous call to get_query_status()
			if (m_flush_requests.pending())
			{
				m_flush_requests.clear_pending_flag();
				m_flush_requests.consumer_wait();
			}
		}

		if (data.command_buffer_to_wait->pending)
			data.command_buffer_to_wait->wait();

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
	if (data.indices.size() == 0)
		return;

	m_occlusion_query_pool.reset_queries(*m_current_command_buffer, data.indices);
	m_occlusion_map.erase(query->driver_handle);
}