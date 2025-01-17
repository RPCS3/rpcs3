#pragma once

#include "vkutils/shared.h"
#include "Emu/RSX/Common/simple_array.hpp"

namespace vk
{
	// Grab standard layout for decompiled RSX programs. Also used by the interpreter.
	// FIXME: This generates a bloated monstrosity that needs to die.
	std::tuple<VkPipelineLayout, VkDescriptorSetLayout> get_common_pipeline_layout(VkDevice dev);

	// Returns the standard binding layout without texture slots. Those have special handling depending on the consumer.
	rsx::simple_array<VkDescriptorSetLayoutBinding> get_common_binding_table();
}
