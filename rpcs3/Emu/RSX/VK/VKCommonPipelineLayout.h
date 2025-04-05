#pragma once

#include "vkutils/shared.h"
#include "Emu/RSX/Common/simple_array.hpp"

namespace vk
{
	// Grab standard layout for decompiled RSX programs. Also used by the interpreter.
	// FIXME: This generates a bloated monstrosity that needs to die.
	std::tuple<VkPipelineLayout, VkDescriptorSetLayout, rsx::simple_array<VkDescriptorSetLayoutBinding>> get_common_pipeline_layout(VkDevice dev);

	// Returns the standard binding layout without texture slots. Those have special handling depending on the consumer.
	rsx::simple_array<VkDescriptorSetLayoutBinding> get_common_binding_table();

	// Returns an array of pool sizes that can be used to generate a proper descriptor pool
	rsx::simple_array<VkDescriptorPoolSize> get_descriptor_pool_sizes(const rsx::simple_array<VkDescriptorSetLayoutBinding>& bindings);
}
