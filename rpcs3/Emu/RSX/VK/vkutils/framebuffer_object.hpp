#pragma once

#include "../VulkanAPI.h"
#include "image.h"

#include <memory>
#include <vector>

namespace vk
{
	struct framebuffer
	{
		VkFramebuffer value;
		VkFramebufferCreateInfo info = {};
		std::vector<std::unique_ptr<vk::image_view>> attachments;
		u32 m_width = 0;
		u32 m_height = 0;

	public:
		framebuffer(VkDevice dev, VkRenderPass pass, u32 width, u32 height, std::vector<std::unique_ptr<vk::image_view>>&& atts)
			: attachments(std::move(atts))
			, m_device(dev)
		{
			std::vector<VkImageView> image_view_array(attachments.size());
			usz i = 0;
			for (const auto& att : attachments)
			{
				image_view_array[i++] = att->value;
			}

			info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			info.width = width;
			info.height = height;
			info.attachmentCount = static_cast<u32>(image_view_array.size());
			info.pAttachments = image_view_array.data();
			info.renderPass = pass;
			info.layers = 1;

			m_width = width;
			m_height = height;

			CHECK_RESULT(vkCreateFramebuffer(dev, &info, nullptr, &value));
		}

		~framebuffer()
		{
			vkDestroyFramebuffer(m_device, value, nullptr);
		}

		u32 width()
		{
			return m_width;
		}

		u32 height()
		{
			return m_height;
		}

		u8 samples()
		{
			ensure(!attachments.empty());
			return attachments[0]->image()->samples();
		}

		VkFormat format()
		{
			ensure(!attachments.empty());
			return attachments[0]->image()->format();
		}

		VkFormat depth_format()
		{
			ensure(!attachments.empty());
			return attachments.back()->image()->format();
		}

		bool matches(const std::vector<vk::image*>& fbo_images, u32 width, u32 height)
		{
			if (m_width != width || m_height != height)
				return false;

			if (fbo_images.size() != attachments.size())
				return false;

			for (uint n = 0; n < fbo_images.size(); ++n)
			{
				if (attachments[n]->info.image != fbo_images[n]->value ||
					attachments[n]->info.format != fbo_images[n]->info.format)
					return false;
			}

			return true;
		}

		framebuffer(const framebuffer&) = delete;
		framebuffer(framebuffer&&) = delete;

	private:
		VkDevice m_device;
	};
}
