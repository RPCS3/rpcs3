#include "ex.h"
#include "buffer_object.h"
#include "image.h"
#include "sampler.h"

namespace vk
{
	VkDescriptorImageInfoEx::VkDescriptorImageInfoEx(const vk::image_view& view, const vk::sampler& sampler, VkImageLayout layout)
		: VkDescriptorImageInfo(sampler.value, view.value, layout)
		, resourceId(view.image()->uid())
	{}

	VkDescriptorImageInfoEx::VkDescriptorImageInfoEx(const vk::image_view& view, const vk::sampler& sampler)
		: VkDescriptorImageInfo(sampler.value, view.value, view.image()->current_layout)
		, resourceId(view.image()->uid())
	{}

	VkDescriptorImageInfoEx::VkDescriptorImageInfoEx(const vk::image_view& view, VkSampler sampler)
		: VkDescriptorImageInfo(sampler, view.value, view.image()->current_layout)
		, resourceId(view.image()->uid())
	{}

	VkDescriptorImageInfoEx::VkDescriptorImageInfoEx(const vk::image_view& view)
		: VkDescriptorImageInfo(VK_NULL_HANDLE, view.value, view.image()->current_layout)
		, resourceId(view.image()->uid())
	{}

	VkDescriptorBufferViewEx::VkDescriptorBufferViewEx(const vk::buffer_view& view)
		: resourceId(view.uid())
		, view(view.value)
	{}

	VkDescriptorBufferInfoEx::VkDescriptorBufferInfoEx(const vk::buffer& buffer, u64 offset, u64 range)
		: VkDescriptorBufferInfo(buffer.value, offset, range)
		, resourceId(buffer.uid())
	{}
}
