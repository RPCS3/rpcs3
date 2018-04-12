#pragma once
#include "VKVertexProgram.h"
#include "VKFragmentProgram.h"
#include "../Common/ProgramStateCache.h"
#include "Utilities/hash.h"
#include "VKHelpers.h"

namespace vk
{
	struct pipeline_props
	{
		graphics_pipeline_state state;
		VkRenderPass render_pass;
		int num_targets;
		int render_pass_location;

		bool operator==(const pipeline_props& other) const
		{
			if (memcmp(&state.att_state[0], &other.state.att_state[0], sizeof(VkPipelineColorBlendAttachmentState)))
				return false;

			if (render_pass_location != other.render_pass_location)
				return false;

			if (memcmp(&state.rs, &other.state.rs, sizeof(VkPipelineRasterizationStateCreateInfo)))
				return false;

			//Cannot memcmp cs due to pAttachments being a pointer to memory
			if (state.cs.attachmentCount != other.state.cs.attachmentCount ||
				state.cs.flags != other.state.cs.flags ||
				state.cs.logicOp != other.state.cs.logicOp ||
				state.cs.logicOpEnable != other.state.cs.logicOpEnable ||
				state.cs.sType != other.state.cs.sType ||
				memcmp(state.cs.blendConstants, other.state.cs.blendConstants, 4 * sizeof(f32)))
				return false;

			if (memcmp(&state.ia, &other.state.ia, sizeof(VkPipelineInputAssemblyStateCreateInfo)))
				return false;

			if (memcmp(&state.ds, &other.state.ds, sizeof(VkPipelineDepthStencilStateCreateInfo)))
				return false;

			if (num_targets != other.num_targets)
				return false;

			return num_targets == other.num_targets;
		}
	};
}

namespace rpcs3
{
	template <>
	size_t hash_struct<vk::pipeline_props>(const vk::pipeline_props &pipelineProperties)
	{
		size_t seed = hash_base<int>(pipelineProperties.num_targets);
		seed ^= hash_struct(pipelineProperties.state.ia);
		seed ^= hash_struct(pipelineProperties.state.ds);
		seed ^= hash_struct(pipelineProperties.state.rs);

		//Do not compare pointers to memory!
		auto tmp = pipelineProperties.state.cs;
		tmp.pAttachments = nullptr;
		seed ^= hash_struct(tmp);

		seed ^= hash_struct(pipelineProperties.state.att_state[0]);
		return hash_base<size_t>(seed);
	}
}

struct VKTraits
{
	using vertex_program_type = VKVertexProgram;
	using fragment_program_type = VKFragmentProgram;
	using pipeline_storage_type = std::unique_ptr<vk::glsl::program>;
	using pipeline_properties = vk::pipeline_props;

	static
	void recompile_fragment_program(const RSXFragmentProgram &RSXFP, fragment_program_type& fragmentProgramData, size_t ID)
	{
		fragmentProgramData.Decompile(RSXFP);
		fragmentProgramData.id = static_cast<u32>(ID);
		fragmentProgramData.Compile();
	}

	static
	void recompile_vertex_program(const RSXVertexProgram &RSXVP, vertex_program_type& vertexProgramData, size_t ID)
	{
		vertexProgramData.Decompile(RSXVP);
		vertexProgramData.id = static_cast<u32>(ID);
		vertexProgramData.Compile();
	}

	static
	void validate_pipeline_properties(const VKVertexProgram&, const VKFragmentProgram &fp, vk::pipeline_props& properties)
	{
		//Explicitly disable writing to undefined registers
		properties.state.att_state[0].colorWriteMask &= fp.output_color_masks[0];
		properties.state.att_state[1].colorWriteMask &= fp.output_color_masks[1];
		properties.state.att_state[2].colorWriteMask &= fp.output_color_masks[2];
		properties.state.att_state[3].colorWriteMask &= fp.output_color_masks[3];
	}

	static
	pipeline_storage_type build_pipeline(const vertex_program_type &vertexProgramData, const fragment_program_type &fragmentProgramData,
			const vk::pipeline_props &pipelineProperties, VkDevice dev, VkPipelineLayout common_pipeline_layout)
	{
		VkPipelineShaderStageCreateInfo shader_stages[2] = {};
		shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shader_stages[0].module = vertexProgramData.handle;
		shader_stages[0].pName = "main";

		shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shader_stages[1].module = fragmentProgramData.handle;
		shader_stages[1].pName = "main";

		VkDynamicState dynamic_state_descriptors[VK_DYNAMIC_STATE_RANGE_SIZE] = {};
		VkPipelineDynamicStateCreateInfo dynamic_state_info = {};
		dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_state_descriptors[dynamic_state_info.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
		dynamic_state_descriptors[dynamic_state_info.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;
		dynamic_state_descriptors[dynamic_state_info.dynamicStateCount++] = VK_DYNAMIC_STATE_LINE_WIDTH;
		dynamic_state_descriptors[dynamic_state_info.dynamicStateCount++] = VK_DYNAMIC_STATE_DEPTH_BOUNDS;
		dynamic_state_descriptors[dynamic_state_info.dynamicStateCount++] = VK_DYNAMIC_STATE_BLEND_CONSTANTS;
		dynamic_state_descriptors[dynamic_state_info.dynamicStateCount++] = VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK;
		dynamic_state_descriptors[dynamic_state_info.dynamicStateCount++] = VK_DYNAMIC_STATE_STENCIL_WRITE_MASK;
		dynamic_state_descriptors[dynamic_state_info.dynamicStateCount++] = VK_DYNAMIC_STATE_STENCIL_REFERENCE;
		dynamic_state_descriptors[dynamic_state_info.dynamicStateCount++] = VK_DYNAMIC_STATE_DEPTH_BIAS;
		dynamic_state_info.pDynamicStates = dynamic_state_descriptors;

		VkPipelineVertexInputStateCreateInfo vi = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

		VkPipelineViewportStateCreateInfo vp = {};
		vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		vp.viewportCount = 1;
		vp.scissorCount = 1;

		VkPipelineMultisampleStateCreateInfo ms = {};
		ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		ms.pSampleMask = NULL;
		ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipeline pipeline;
		VkGraphicsPipelineCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		info.pVertexInputState = &vi;
		info.pInputAssemblyState = &pipelineProperties.state.ia;
		info.pRasterizationState = &pipelineProperties.state.rs;
		info.pColorBlendState = &pipelineProperties.state.cs;
		info.pMultisampleState = &ms;
		info.pViewportState = &vp;
		info.pDepthStencilState = &pipelineProperties.state.ds;
		info.stageCount = 2;
		info.pStages = shader_stages;
		info.pDynamicState = &dynamic_state_info;
		info.layout = common_pipeline_layout;
		info.basePipelineIndex = -1;
		info.basePipelineHandle = VK_NULL_HANDLE;
		info.renderPass = pipelineProperties.render_pass;

		CHECK_RESULT(vkCreateGraphicsPipelines(dev, nullptr, 1, &info, NULL, &pipeline));
		pipeline_storage_type result = std::make_unique<vk::glsl::program>(dev, pipeline, vertexProgramData.uniforms, fragmentProgramData.uniforms);

		return result;
	}
};

class VKProgramBuffer : public program_state_cache<VKTraits>
{
	const VkRenderPass *m_render_pass_data;

public:
	VKProgramBuffer(VkRenderPass *renderpass_list)
		: m_render_pass_data(renderpass_list)
	{}

	void clear()
	{
		program_state_cache<VKTraits>::clear();
		m_vertex_shader_cache.clear();
		m_fragment_shader_cache.clear();
	}

	u64 get_hash(vk::pipeline_props &props)
	{
		return rpcs3::hash_struct<vk::pipeline_props>(props);
	}

	u64 get_hash(RSXVertexProgram &prog)
	{
		return program_hash_util::vertex_program_utils::get_vertex_program_ucode_hash(prog);
	}

	u64 get_hash(RSXFragmentProgram &prog)
	{
		return program_hash_util::fragment_program_utils::get_fragment_program_ucode_hash(prog);
	}

	template <typename... Args>
	void add_pipeline_entry(RSXVertexProgram &vp, RSXFragmentProgram &fp, vk::pipeline_props &props, Args&& ...args)
	{
		//Extract pointers from pipeline props
		props.render_pass = m_render_pass_data[props.render_pass_location];
		props.state.cs.pAttachments = props.state.att_state;
		vp.skip_vertex_input_check = true;
		getGraphicPipelineState(vp, fp, props, std::forward<Args>(args)...);
	}

	bool check_cache_missed() const
	{
		return m_cache_miss_flag;
	}
};
