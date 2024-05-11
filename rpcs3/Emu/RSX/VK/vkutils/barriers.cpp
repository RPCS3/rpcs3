#include "barriers.h"
#include "commands.h"
#include "image.h"

#include "../../rsx_methods.h"
#include "../VKRenderPass.h"

namespace vk
{
	void insert_image_memory_barrier(
		const vk::command_buffer& cmd, VkImage image,
		VkImageLayout current_layout, VkImageLayout new_layout,
		VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage,
		VkAccessFlags src_mask, VkAccessFlags dst_mask,
		const VkImageSubresourceRange& range,
		bool preserve_renderpass)
	{
		if (!preserve_renderpass && vk::is_renderpass_open(cmd))
		{
			vk::end_renderpass(cmd);
		}

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.newLayout = new_layout;
		barrier.oldLayout = current_layout;
		barrier.image = image;
		barrier.srcAccessMask = src_mask;
		barrier.dstAccessMask = dst_mask;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange = range;

		vkCmdPipelineBarrier(cmd, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

	void insert_buffer_memory_barrier(
		const vk::command_buffer& cmd,
		VkBuffer buffer,
		VkDeviceSize offset, VkDeviceSize length,
		VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage,
		VkAccessFlags src_mask, VkAccessFlags dst_mask,
		bool preserve_renderpass)
	{
		if (!preserve_renderpass && vk::is_renderpass_open(cmd))
		{
			vk::end_renderpass(cmd);
		}

		VkBufferMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.buffer = buffer;
		barrier.offset = offset;
		barrier.size = length;
		barrier.srcAccessMask = src_mask;
		barrier.dstAccessMask = dst_mask;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		vkCmdPipelineBarrier(cmd, src_stage, dst_stage, 0, 0, nullptr, 1, &barrier, 0, nullptr);
	}

	void insert_global_memory_barrier(
		const vk::command_buffer& cmd,
		VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage,
		VkAccessFlags src_access, VkAccessFlags dst_access,
		bool preserve_renderpass)
	{
		if (!preserve_renderpass && vk::is_renderpass_open(cmd))
		{
			vk::end_renderpass(cmd);
		}

		VkMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		barrier.srcAccessMask = src_access;
		barrier.dstAccessMask = dst_access;
		vkCmdPipelineBarrier(cmd, src_stage, dst_stage, 0, 1, &barrier, 0, nullptr, 0, nullptr);
	}

	void insert_texture_barrier(
		const vk::command_buffer& cmd,
		VkImage image,
		VkImageLayout current_layout, VkImageLayout new_layout,
		VkImageSubresourceRange range,
		bool preserve_renderpass)
	{
		// NOTE: Sampling from an attachment in ATTACHMENT_OPTIMAL layout on some hw ends up with garbage output
		// Transition to GENERAL if this resource is both input and output
		// TODO: This implicitly makes the target incompatible with the renderpass declaration; investigate a proper workaround
		// TODO: This likely throws out hw optimizations on the rest of the renderpass, manage carefully
		if (!preserve_renderpass && vk::is_renderpass_open(cmd))
		{
			vk::end_renderpass(cmd);
		}

		VkAccessFlags src_access;
		VkPipelineStageFlags src_stage;
		if (range.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT)
		{
			src_access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			src_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		}
		else
		{
			src_access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			src_stage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		}

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.newLayout = new_layout;
		barrier.oldLayout = current_layout;
		barrier.image = image;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange = range;
		barrier.srcAccessMask = src_access;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(cmd, src_stage, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

	void insert_texture_barrier(const vk::command_buffer& cmd, vk::image* image, VkImageLayout new_layout, bool preserve_renderpass)
	{
		insert_texture_barrier(cmd, image->value, image->current_layout, new_layout, { image->aspect(), 0, 1, 0, 1 }, preserve_renderpass);
		image->current_layout = new_layout;
	}
}
