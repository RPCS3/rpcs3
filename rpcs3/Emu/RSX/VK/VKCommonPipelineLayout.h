#pragma once

#include "vkutils/shared.h"
#include "Emu/RSX/Common/simple_array.hpp"

namespace vk
{
	// Returns an array of pool sizes that can be used to generate a proper descriptor pool
	rsx::simple_array<VkDescriptorPoolSize> get_descriptor_pool_sizes(const rsx::simple_array<VkDescriptorSetLayoutBinding>& bindings);
}
