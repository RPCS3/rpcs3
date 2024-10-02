#pragma once

#include "../VulkanAPI.h"
#include "../../Common/TextureUtils.h"

#include "commands.h"
#include "device.h"
#include "memory.h"

#include <stack>

//using enum rsx::format_class;
using namespace ::rsx::format_class_;

#define VK_DISABLE_COMPONENT_SWIZZLE 0

namespace vk
{
	enum : u32// special remap_encoding enums
	{
		VK_REMAP_IDENTITY = 0xCAFEBABE,                   // Special view encoding to return an identity image view
		VK_REMAP_VIEW_MULTISAMPLED = 0xDEADBEEF,          // Special encoding for multisampled images; returns a multisampled image view
		VK_IMAGE_CREATE_ALLOW_NULL_RPCS3 = 0x80000000,    // Special flag that allows null images to be created if there is no memory
		VK_IMAGE_CREATE_SHAREABLE_RPCS3  = 0x40000000,    // Special flag to create a shareable image

		VK_IMAGE_CREATE_SPECIAL_FLAGS_RPCS3 = (VK_IMAGE_CREATE_ALLOW_NULL_RPCS3 | VK_IMAGE_CREATE_SHAREABLE_RPCS3)
	};

	class image
	{
		std::stack<VkImageLayout> m_layout_stack;
		VkImageAspectFlags m_storage_aspect = 0;

		rsx::format_class m_format_class = RSX_FORMAT_CLASS_UNDEFINED;

		void validate(const vk::render_device& dev, const VkImageCreateInfo& info) const;

	protected:
		image() = default;
		void create_impl(const vk::render_device& dev, u32 access_flags, const memory_type_info& memory_type, vmm_allocation_pool allocation_pool);

	public:
		VkImage value = VK_NULL_HANDLE;
		VkComponentMapping native_component_map = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		VkImageLayout current_layout = VK_IMAGE_LAYOUT_UNDEFINED;
		u32 current_queue_family = VK_QUEUE_FAMILY_IGNORED;
		VkImageCreateInfo info = {};
		std::shared_ptr<vk::memory_block> memory;

		image(const vk::render_device& dev,
			const memory_type_info& memory_type,
			u32 access_flags,
			VkImageType image_type,
			VkFormat format,
			u32 width, u32 height, u32 depth,
			u32 mipmaps, u32 layers,
			VkSampleCountFlagBits samples,
			VkImageLayout initial_layout,
			VkImageTiling tiling,
			VkImageUsageFlags usage,
			VkImageCreateFlags image_flags,
			vmm_allocation_pool allocation_pool,
			rsx::format_class format_class = RSX_FORMAT_CLASS_UNDEFINED);

		virtual ~image();

		image(const image&) = delete;
		image(image&&) = delete;

		// Properties
		u32 width() const;
		u32 height() const;
		u32 depth() const;
		u32 mipmaps() const;
		u32 layers() const;
		u8 samples() const;
		VkFormat format() const;
		VkImageType type() const;
		VkSharingMode sharing_mode() const;
		VkImageAspectFlags aspect() const;
		rsx::format_class format_class() const;

		// Pipeline management
		void push_layout(const command_buffer& cmd, VkImageLayout layout);
		void push_barrier(const command_buffer& cmd, VkImageLayout layout);
		void pop_layout(const command_buffer& cmd);
		void change_layout(const command_buffer& cmd, VkImageLayout new_layout);

		// Queue transfer
		void queue_acquire(const command_buffer& cmd, VkImageLayout new_layout);
		void queue_release(const command_buffer& src_queue_cmd, u32 dst_queue_family, VkImageLayout new_layout);

		// Debug utils
		void set_debug_name(const std::string& name);

	protected:
		VkDevice m_device;
	};

	struct image_view
	{
		VkImageView value = VK_NULL_HANDLE;
		VkImageViewCreateInfo info = {};

		image_view(VkDevice dev, VkImage image, VkImageViewType view_type, VkFormat format, VkComponentMapping mapping, VkImageSubresourceRange range);
		image_view(VkDevice dev, VkImageViewCreateInfo create_info);

		image_view(VkDevice dev, vk::image* resource,
			VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_MAX_ENUM,
			const VkComponentMapping& mapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
			const VkImageSubresourceRange& range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

		~image_view();

		u32 encoded_component_map() const;
		vk::image* image() const;

		image_view(const image_view&) = delete;
		image_view(image_view&&) = delete;

	private:
		VkDevice m_device;
		vk::image* m_resource = nullptr;

		void create_impl();
	};

	class viewable_image : public image
	{
	protected:
		std::unordered_map<u64, std::unique_ptr<vk::image_view>> views;
		viewable_image* clone();

	public:
		using image::image;

		virtual image_view* get_view(const rsx::texture_channel_remap_t& remap,
			VkImageAspectFlags mask = VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT);

		void set_native_component_layout(VkComponentMapping new_layout);
	};
}
