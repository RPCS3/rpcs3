#include "stdafx.h"
#include "vkutils/device.h"
#include "vkutils/descriptors.h"
#include "VKCommonPipelineLayout.h"
#include "VKHelpers.h"

#include "Emu/RSX/Common/simple_array.hpp"

namespace vk
{
	rsx::simple_array<VkDescriptorPoolSize> get_descriptor_pool_sizes(const rsx::simple_array<VkDescriptorSetLayoutBinding>& bindings)
	{
		// Compile descriptor pool sizes
		const u32 num_ubo = bindings.reduce(0, FN(x + (y.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ? y.descriptorCount : 0)));
		const u32 num_texel_buffers = bindings.reduce(0, FN(x + (y.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER ? y.descriptorCount : 0)));
		const u32 num_combined_image_sampler = bindings.reduce(0, FN(x + (y.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ? y.descriptorCount : 0)));
		const u32 num_ssbo = bindings.reduce(0, FN(x + (y.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ? y.descriptorCount : 0)));

		ensure(num_ubo > 0 && num_texel_buffers > 0 && num_combined_image_sampler > 0 && num_ssbo > 0);

		return
		{
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , num_ubo },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER , num_texel_buffers },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , num_combined_image_sampler },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, num_ssbo }
		};
	}
}
