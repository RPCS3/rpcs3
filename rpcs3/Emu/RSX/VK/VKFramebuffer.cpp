#include "stdafx.h"

#include "VKFramebuffer.h"
#include "vkutils/image.h"
#include "vkutils/image_helpers.h"

#include <unordered_map>
#include "Utilities/mutex.h"

namespace vk
{
	std::unordered_map<u64, std::vector<std::unique_ptr<vk::framebuffer_holder>>> g_framebuffers_cache;
	shared_mutex g_framebuffers_mutex;

	union framebuffer_storage_key
	{
		u64 encoded;

		struct
		{
			u64 width  : 16;   // Width of FBO
			u64 height : 16;   // Height of FBO
			u64 ia_ref : 1;    // Input attachment references?
		};

		framebuffer_storage_key(u16 width_, u16 height_, VkBool32 has_input_attachments)
			: width(width_), height(height_), ia_ref(has_input_attachments)
		{}
	};

	vk::framebuffer_holder* get_framebuffer(VkDevice dev, u16 width, u16 height, VkBool32 has_input_attachments, VkRenderPass renderpass, const std::vector<vk::image*>& image_list)
	{
		framebuffer_storage_key key(width, height, has_input_attachments);

		reader_lock lock(g_framebuffers_mutex);

		if (auto found = g_framebuffers_cache.find(key.encoded);
			found != g_framebuffers_cache.end())
		{
			auto& queue = found->second;
			for (const auto& fbo : queue)
			{
				if (fbo->matches(image_list, width, height))
				{
					return fbo.get();
				}
			}
		}

		lock.upgrade();

		// NOTE: We don't need to recheck the cache here. If a dupe exists, it will be dropped in 2 frames.

		std::vector<std::unique_ptr<vk::image_view>> image_views;
		image_views.reserve(image_list.size());

		for (const auto& e : image_list)
		{
			const VkImageSubresourceRange subres = { e->aspect(), 0, 1, 0, 1 };
			image_views.push_back(std::make_unique<vk::image_view>(dev, e, e->format(), VK_IMAGE_VIEW_TYPE_2D, vk::default_component_map, subres));
		}

		auto value = std::make_unique<vk::framebuffer_holder>(dev, renderpass, width, height, std::move(image_views));
		auto ret = value.get();

		g_framebuffers_cache[key.encoded].push_back(std::move(value));
		return ret;
	}

	vk::framebuffer_holder* get_framebuffer(VkDevice dev, u16 width, u16 height, VkBool32 has_input_attachments, VkRenderPass renderpass, VkFormat format, VkImage attachment)
	{
		framebuffer_storage_key key(width, height, has_input_attachments);

		reader_lock lock(g_framebuffers_mutex);

		if (auto found = g_framebuffers_cache.find(key.encoded);
			found != g_framebuffers_cache.end())
		{
			auto& queue = found->second;
			for (const auto& e : queue)
			{
				if (e->attachments[0]->info.image == attachment)
				{
					return e.get();
				}
			}
		}

		lock.upgrade();

		// NOTE: Cache recheck avoided intentionally here.

		VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		std::vector<std::unique_ptr<vk::image_view>> views;

		views.push_back(std::make_unique<vk::image_view>(dev, attachment, VK_IMAGE_VIEW_TYPE_2D, format, vk::default_component_map, range));
		auto value = std::make_unique<vk::framebuffer_holder>(dev, renderpass, width, height, std::move(views));
		auto ret = value.get();

		g_framebuffers_cache[key.encoded].push_back(std::move(value));
		return ret;
	}

	void remove_framebuffers_with_filter(
		const std::predicate<const std::unique_ptr<vk::framebuffer_holder>&> auto criteria)
	{
		std::lock_guard lock(g_framebuffers_mutex);

		for (auto It = g_framebuffers_cache.begin(); It != g_framebuffers_cache.end();)
		{
			It->second.erase(
				std::remove_if(It->second.begin(), It->second.end(), criteria),
				It->second.end()
			);

			if (It->second.empty())
			{
				It = g_framebuffers_cache.erase(It);
			}
			else
			{
				++It;
			}
		}
	}

	void remove_unused_framebuffers()
	{
		// Remove stale framebuffers. Ref counted to prevent use-after-free
		remove_framebuffers_with_filter([](const auto& fbo)
		{
			return (fbo->unused_check_count() >= 2);
		});
	}

	void remove_framebuffers_with_image(const vk::image* image)
	{
		remove_framebuffers_with_filter([image](const auto& fbo)
		{
			return fbo->references(image);
		});
	}

	void remove_framebuffers_with_image(VkImage handle)
	{
		remove_framebuffers_with_filter([handle](const auto& fbo)
		{
			return fbo->references(handle);
		});
	}

	void clear_framebuffer_cache()
	{
		std::lock_guard lock(g_framebuffers_mutex);
		g_framebuffers_cache.clear();
	}
}
