#pragma once

#include "../VulkanAPI.h"

namespace vk
{
	class image;
	class command_buffer;

	//Texture barrier applies to a texture to ensure writes to it are finished before any reads are attempted to avoid RAW hazards
	void insert_texture_barrier(const vk::command_buffer& cmd, VkImage image, VkImageLayout current_layout, VkImageLayout new_layout, VkImageSubresourceRange range, bool preserve_renderpass = false);
	void insert_texture_barrier(const vk::command_buffer& cmd, vk::image* image, VkImageLayout new_layout, bool preserve_renderpass = false);

	void insert_buffer_memory_barrier(const vk::command_buffer& cmd, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize length,
		VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage, VkAccessFlags src_mask, VkAccessFlags dst_mask,
		bool preserve_renderpass = false);

	void insert_image_memory_barrier(const vk::command_buffer& cmd, VkImage image, VkImageLayout current_layout, VkImageLayout new_layout,
		VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage, VkAccessFlags src_mask, VkAccessFlags dst_mask,
		const VkImageSubresourceRange& range, bool preserve_renderpass = false);

	void insert_global_memory_barrier(const vk::command_buffer& cmd,
		VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VkAccessFlags src_access = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
		VkAccessFlags dst_access = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
		bool preserve_renderpass = false);
}
