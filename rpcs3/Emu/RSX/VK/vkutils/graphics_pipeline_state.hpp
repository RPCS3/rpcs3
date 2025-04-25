#pragma once

#include "../VulkanAPI.h"

namespace vk
{
	class graphics_pipeline_state
	{
	public:
		VkPipelineInputAssemblyStateCreateInfo ia;
		VkPipelineDepthStencilStateCreateInfo ds;
		VkPipelineColorBlendAttachmentState att_state[4];
		VkPipelineColorBlendStateCreateInfo cs;
		VkPipelineRasterizationStateCreateInfo rs;
		VkPipelineMultisampleStateCreateInfo ms;

		struct extra_parameters
		{
			VkSampleMask msaa_sample_mask;
		}
		temp_storage;

		graphics_pipeline_state()
		{
			// NOTE: Vk** structs have padding bytes
			std::memset(static_cast<void*>(this), 0, sizeof(graphics_pipeline_state));

			ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			cs.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

			rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rs.polygonMode = VK_POLYGON_MODE_FILL;
			rs.cullMode = VK_CULL_MODE_NONE;
			rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			rs.lineWidth = 1.f;

			ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			temp_storage.msaa_sample_mask = 0xFFFFFFFF;
		}

		graphics_pipeline_state(const graphics_pipeline_state& other)
		{
			// NOTE: Vk** structs have padding bytes
			std::memcpy(static_cast<void*>(this), &other, sizeof(graphics_pipeline_state));

			if (other.cs.pAttachments == other.att_state)
			{
				// Rebase pointer
				cs.pAttachments = att_state;
			}
		}

		~graphics_pipeline_state() = default;

		graphics_pipeline_state& operator = (const graphics_pipeline_state& other)
		{
			if (this != &other)
			{
				// NOTE: Vk** structs have padding bytes
				std::memcpy(static_cast<void*>(this), &other, sizeof(graphics_pipeline_state));

				if (other.cs.pAttachments == other.att_state)
				{
					// Rebase pointer
					cs.pAttachments = att_state;
				}
			}

			return *this;
		}

		void set_primitive_type(VkPrimitiveTopology type)
		{
			ia.topology = type;
		}

		void enable_primitive_restart(VkBool32 enable = VK_TRUE)
		{
			ia.primitiveRestartEnable = enable;
		}

		void set_color_mask(int index, bool r, bool g, bool b, bool a)
		{
			VkColorComponentFlags mask = 0;
			if (a) mask |= VK_COLOR_COMPONENT_A_BIT;
			if (b) mask |= VK_COLOR_COMPONENT_B_BIT;
			if (g) mask |= VK_COLOR_COMPONENT_G_BIT;
			if (r) mask |= VK_COLOR_COMPONENT_R_BIT;

			att_state[index].colorWriteMask = mask;
		}

		void set_depth_mask(bool enable)
		{
			ds.depthWriteEnable = enable ? VK_TRUE : VK_FALSE;
		}

		void set_stencil_mask(u32 mask)
		{
			ds.front.writeMask = mask;
			ds.back.writeMask = mask;
		}

		void set_stencil_mask_separate(int face, u32 mask)
		{
			if (!face)
				ds.front.writeMask = mask;
			else
				ds.back.writeMask = mask;
		}

		void enable_depth_test(VkCompareOp op)
		{
			ds.depthTestEnable = VK_TRUE;
			ds.depthCompareOp = op;
		}

		void enable_depth_clamp(bool enable = true)
		{
			rs.depthClampEnable = enable ? VK_TRUE : VK_FALSE;
		}

		void enable_depth_bias(bool enable = true)
		{
			rs.depthBiasEnable = enable ? VK_TRUE : VK_FALSE;
		}

		void enable_depth_bounds_test(bool enable = true)
		{
			ds.depthBoundsTestEnable = enable? VK_TRUE : VK_FALSE;
		}

		void enable_blend(int mrt_index,
			VkBlendFactor src_factor_rgb, VkBlendFactor src_factor_a,
			VkBlendFactor dst_factor_rgb, VkBlendFactor dst_factor_a,
			VkBlendOp equation_rgb, VkBlendOp equation_a)
		{
			att_state[mrt_index].srcColorBlendFactor = src_factor_rgb;
			att_state[mrt_index].srcAlphaBlendFactor = src_factor_a;
			att_state[mrt_index].dstColorBlendFactor = dst_factor_rgb;
			att_state[mrt_index].dstAlphaBlendFactor = dst_factor_a;
			att_state[mrt_index].colorBlendOp = equation_rgb;
			att_state[mrt_index].alphaBlendOp = equation_a;
			att_state[mrt_index].blendEnable = VK_TRUE;
		}

		void enable_stencil_test(VkStencilOp fail, VkStencilOp zfail, VkStencilOp pass,
			VkCompareOp func, u32 func_mask, u32 ref)
		{
			ds.front.failOp = fail;
			ds.front.passOp = pass;
			ds.front.depthFailOp = zfail;
			ds.front.compareOp = func;
			ds.front.compareMask = func_mask;
			ds.front.reference = ref;
			ds.back = ds.front;

			ds.stencilTestEnable = VK_TRUE;
		}

		void enable_stencil_test_separate(int face, VkStencilOp fail, VkStencilOp zfail, VkStencilOp pass,
			VkCompareOp func, u32 func_mask, u32 ref)
		{
			auto& face_props = (face ? ds.back : ds.front);
			face_props.failOp = fail;
			face_props.passOp = pass;
			face_props.depthFailOp = zfail;
			face_props.compareOp = func;
			face_props.compareMask = func_mask;
			face_props.reference = ref;

			ds.stencilTestEnable = VK_TRUE;
		}

		void enable_logic_op(VkLogicOp op)
		{
			cs.logicOpEnable = VK_TRUE;
			cs.logicOp = op;
		}

		void enable_cull_face(VkCullModeFlags cull_mode)
		{
			rs.cullMode = cull_mode;
		}

		void set_front_face(VkFrontFace face)
		{
			rs.frontFace = face;
		}

		void set_attachment_count(u32 count)
		{
			cs.attachmentCount = count;
			cs.pAttachments = att_state;
		}

		void set_multisample_state(u8 sample_count, u32 sample_mask, bool msaa_enabled, bool alpha_to_coverage, bool alpha_to_one)
		{
			temp_storage.msaa_sample_mask = sample_mask;

			ms.rasterizationSamples = static_cast<VkSampleCountFlagBits>(sample_count);
			ms.alphaToCoverageEnable = alpha_to_coverage;
			ms.alphaToOneEnable = alpha_to_one;

			if (!msaa_enabled)
			{
				// This register is likely glMinSampleShading but in reverse; probably sets max sample shading rate of 1
				// I (kd-11) suspect its what the control panel setting affects when MSAA is set to disabled
			}
		}

		void set_multisample_shading_rate(float shading_rate)
		{
			ms.sampleShadingEnable = VK_TRUE;
			ms.minSampleShading = shading_rate;
		}
	};
}
