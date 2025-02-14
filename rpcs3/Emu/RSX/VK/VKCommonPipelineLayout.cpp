#include "stdafx.h"
#include "vkutils/device.h"
#include "vkutils/descriptors.h"
#include "VKCommonPipelineLayout.h"
#include "VKHelpers.h"

#include "Emu/RSX/Common/simple_array.hpp"

namespace vk
{
	rsx::simple_array<VkDescriptorSetLayoutBinding> get_common_binding_table()
	{
		const auto& binding_table = vk::get_current_renderer()->get_pipeline_binding_table();
		rsx::simple_array<VkDescriptorSetLayoutBinding> bindings(binding_table.instancing_constants_buffer_slot + 1);

		u32 idx = 0;

		// Vertex stream, one stream for cacheable data, one stream for transient data
		for (int i = 0; i < 3; i++)
		{
			bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
			bindings[idx].descriptorCount = 1;
			bindings[idx].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			bindings[idx].binding = binding_table.vertex_buffers_first_bind_slot + i;
			bindings[idx].pImmutableSamplers = nullptr;
			idx++;
		}

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[idx].descriptorCount = 1;
		bindings[idx].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[idx].binding = binding_table.fragment_constant_buffers_bind_slot;
		bindings[idx].pImmutableSamplers = nullptr;

		idx++;

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[idx].descriptorCount = 1;
		bindings[idx].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[idx].binding = binding_table.fragment_state_bind_slot;
		bindings[idx].pImmutableSamplers = nullptr;

		idx++;

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[idx].descriptorCount = 1;
		bindings[idx].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[idx].binding = binding_table.fragment_texture_params_bind_slot;
		bindings[idx].pImmutableSamplers = nullptr;

		idx++;

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[idx].descriptorCount = 1;
		bindings[idx].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		bindings[idx].binding = binding_table.vertex_constant_buffers_bind_slot;
		bindings[idx].pImmutableSamplers = nullptr;

		idx++;

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[idx].descriptorCount = 1;
		bindings[idx].stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
		bindings[idx].binding = binding_table.vertex_params_bind_slot;
		bindings[idx].pImmutableSamplers = nullptr;

		idx++;

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		bindings[idx].descriptorCount = 1;
		bindings[idx].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		bindings[idx].binding = binding_table.conditional_render_predicate_slot;
		bindings[idx].pImmutableSamplers = nullptr;

		idx++;

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[idx].descriptorCount = 1;
		bindings[idx].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[idx].binding = binding_table.rasterizer_env_bind_slot;
		bindings[idx].pImmutableSamplers = nullptr;

		idx++;

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		bindings[idx].descriptorCount = 1;
		bindings[idx].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		bindings[idx].binding = binding_table.instancing_lookup_table_bind_slot;
		bindings[idx].pImmutableSamplers = nullptr;

		idx++;

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		bindings[idx].descriptorCount = 1;
		bindings[idx].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		bindings[idx].binding = binding_table.instancing_constants_buffer_slot;
		bindings[idx].pImmutableSamplers = nullptr;

		idx++;

		return bindings;
	}

	std::tuple<VkPipelineLayout, VkDescriptorSetLayout> get_common_pipeline_layout(VkDevice dev)
	{
		const auto& binding_table = vk::get_current_renderer()->get_pipeline_binding_table();
		auto bindings = get_common_binding_table();
		u32 idx = ::size32(bindings);

		bindings.resize(binding_table.total_descriptor_bindings);

		for (auto binding = binding_table.textures_first_bind_slot;
			binding < binding_table.vertex_textures_first_bind_slot;
			binding++)
		{
			bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			bindings[idx].descriptorCount = 1;
			bindings[idx].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			bindings[idx].binding = binding;
			bindings[idx].pImmutableSamplers = nullptr;
			idx++;
		}

		for (int i = 0; i < rsx::limits::vertex_textures_count; i++)
		{
			bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			bindings[idx].descriptorCount = 1;
			bindings[idx].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			bindings[idx].binding = binding_table.vertex_textures_first_bind_slot + i;
			bindings[idx].pImmutableSamplers = nullptr;
			idx++;
		}

		ensure(idx == binding_table.total_descriptor_bindings);

		std::array<VkPushConstantRange, 1> push_constants;
		push_constants[0].offset = 0;
		push_constants[0].size = 16;
		push_constants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		if (vk::emulate_conditional_rendering())
		{
			// Conditional render toggle
			push_constants[0].size = 20;
		}

		const auto set_layout = vk::descriptors::create_layout(bindings);

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
