#include "ex.h"
#include "image.h"
#include "sampler.h"

namespace vk
{
	VkDescriptorImageInfoEx::VkDescriptorImageInfoEx(const vk::image_view& view, const vk::sampler& sampler, VkImageLayout layout)
		: VkDescriptorImageInfo(sampler.value, view.value, layout)
		, resourceId(view.image()->id())
	{}

	VkDescriptorImageInfoEx::VkDescriptorImageInfoEx(const vk::image_view& view, const vk::sampler& sampler)
		: VkDescriptorImageInfo(sampler.value, view.value, view.image()->current_layout)
		, resourceId(view.image()->id())
	{}

	VkDescriptorImageInfoEx::VkDescriptorImageInfoEx(const vk::image_view& view, VkSampler sampler)
		: VkDescriptorImageInfo(sampler, view.value, view.image()->current_layout)
		, resourceId(view.image()->id())
	{}

	VkDescriptorImageInfoEx::VkDescriptorImageInfoEx(const vk::image_view& view)
		: VkDescriptorImageInfo(VK_NULL_HANDLE, view.value, view.image()->current_layout)
		, resourceId(view.image()->id())
	{}
}
