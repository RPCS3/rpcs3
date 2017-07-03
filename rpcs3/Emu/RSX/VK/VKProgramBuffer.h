#pragma once
#include "VKVertexProgram.h"
#include "VKFragmentProgram.h"
#include "../Common/ProgramStateCache.h"


namespace vk
{
	struct pipeline_props
	{
		VkPipelineInputAssemblyStateCreateInfo ia;
		VkPipelineDepthStencilStateCreateInfo ds;
		VkPipelineColorBlendAttachmentState att_state[4];
		VkPipelineColorBlendStateCreateInfo cs;
		VkPipelineRasterizationStateCreateInfo rs;
		
		VkRenderPass render_pass;
		int num_targets;

		bool operator==(const pipeline_props& other) const
		{
			if (memcmp(&ia, &other.ia, sizeof(VkPipelineInputAssemblyStateCreateInfo)))
				return false;
			if (memcmp(&ds, &other.ds, sizeof(VkPipelineDepthStencilStateCreateInfo)))
				return false;
			if (memcmp(&att_state[0], &other.att_state[0], sizeof(VkPipelineColorBlendAttachmentState)))
				return false;
			if (memcmp(&cs, &other.cs, sizeof(VkPipelineColorBlendStateCreateInfo)))
				return false;
			if (memcmp(&rs, &other.rs, sizeof(VkPipelineRasterizationStateCreateInfo)))
				return false;
			if (render_pass != other.render_pass)
				return false;

			return num_targets == other.num_targets;
		}
	};
}

namespace
{
	template<typename T>
	size_t hash_struct(const T& structure)
	{
		char *data = (char*)(&structure);
		size_t result = 0;
		for (unsigned i = 0; i < sizeof(T); i++)
			result ^= std::hash<char>()(data[i]);
		return result;
	}
}

namespace std
{
	template <>
	struct hash<vk::pipeline_props> {
		size_t operator()(const vk::pipeline_props &pipelineProperties) const {
			size_t seed = hash<unsigned>()(pipelineProperties.num_targets);
			seed ^= hash_struct(pipelineProperties.ia);
			seed ^= hash_struct(pipelineProperties.ds);
			seed ^= hash_struct(pipelineProperties.rs);
			seed ^= hash_struct(pipelineProperties.cs);
			seed ^= hash_struct(pipelineProperties.att_state[0]);
			return hash<size_t>()(seed);
		}
	};
}

struct VKTraits
{
	using vertex_program_type = VKVertexProgram;
	using fragment_program_type = VKFragmentProgram;
	using pipeline_storage_type = std::unique_ptr<vk::glsl::program>;
	using pipeline_properties = vk::pipeline_props;

	static
	void recompile_fragment_program(const RSXFragmentProgram &RSXFP, fragment_program_type& fragmentProgramData, size_t /*ID*/)
	{
		fragmentProgramData.Decompile(RSXFP);
		fragmentProgramData.Compile();
	}

	static
	void recompile_vertex_program(const RSXVertexProgram &RSXVP, vertex_program_type& vertexProgramData, size_t /*ID*/)
	{
		vertexProgramData.Decompile(RSXVP);
		vertexProgramData.Compile();
	}

	static
	pipeline_storage_type build_pipeline(const vertex_program_type &vertexProgramData, const fragment_program_type &fragmentProgramData, const vk::pipeline_props &pipelineProperties, VkDevice dev, VkPipelineLayout common_pipeline_layout)
	{
//		pstate.dynamic_state.pDynamicStates = pstate.dynamic_state_descriptors;
//		pstate.cb.pAttachments = pstate.att_state;
//		pstate.cb.attachmentCount = pstate.num_targets;

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
		info.pInputAssemblyState = &pipelineProperties.ia;
		info.pRasterizationState = &pipelineProperties.rs;
		info.pColorBlendState = &pipelineProperties.cs;
		info.pMultisampleState = &ms;
		info.pViewportState = &vp;
		info.pDepthStencilState = &pipelineProperties.ds;
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
public:
	void clear()
	{
		program_state_cache<VKTraits>::clear();
		m_vertex_shader_cache.clear();
		m_fragment_shader_cache.clear();
	}
};
