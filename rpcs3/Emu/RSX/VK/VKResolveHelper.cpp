#include "stdafx.h"

#include "VKResolveHelper.h"
#include "VKRenderPass.h"
#include "VKRenderTargets.h"

namespace
{
	const char *get_format_prefix(VkFormat format)
	{
		switch (format)
		{
			case VK_FORMAT_R5G6B5_UNORM_PACK16:
				return "r16ui";
			case VK_FORMAT_R8G8B8A8_UNORM:
			case VK_FORMAT_B8G8R8A8_UNORM:
				return "rgba8";
			case VK_FORMAT_R16G16B16A16_SFLOAT:
				return "rgba16f";
			case VK_FORMAT_R32G32B32A32_SFLOAT:
				return "rgba32f";
			case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
				return "r16ui";
			case VK_FORMAT_R8_UNORM:
				return "r8";
			case VK_FORMAT_R8G8_UNORM:
				return "rg8";
			case VK_FORMAT_R32_SFLOAT:
				return "r32f";
			default:
				fmt::throw_exception("Unhandled VkFormat 0x%x" HERE, u32(format));
		}
	}
}

namespace vk
{
	std::unordered_map<VkFormat, std::unique_ptr<vk::cs_resolve_task>> g_resolve_helpers;
	std::unordered_map<VkFormat, std::unique_ptr<vk::cs_unresolve_task>> g_unresolve_helpers;
	std::unique_ptr<vk::depthonly_resolve> g_depth_resolver;
	std::unique_ptr<vk::depthonly_unresolve> g_depth_unresolver;
	std::unique_ptr<vk::stencilonly_resolve> g_stencil_resolver;
	std::unique_ptr<vk::stencilonly_unresolve> g_stencil_unresolver;
	std::unique_ptr<vk::depthstencil_resolve_EXT> g_depthstencil_resolver;
	std::unique_ptr<vk::depthstencil_unresolve_EXT> g_depthstencil_unresolver;

	template <typename T, typename ...Args>
	void initialize_pass(std::unique_ptr<T>& ptr, vk::render_device& dev, Args&&... extras)
	{
		if (!ptr)
		{
			ptr = std::make_unique<T>(std::forward<Args>(extras)...);
			ptr->create(dev);
		}
	}

	void resolve_image(vk::command_buffer& cmd, vk::viewable_image* dst, vk::viewable_image* src)
	{
		if (src->aspect() == VK_IMAGE_ASPECT_COLOR_BIT)
		{
			auto &job = g_resolve_helpers[src->format()];

			if (!job)
			{
				const char* format_prefix = get_format_prefix(src->format());
				bool require_bgra_swap = false;

				if (vk::get_chip_family() == vk::chip_class::NV_kepler &&
					src->format() == VK_FORMAT_B8G8R8A8_UNORM)
				{
					// Workaround for NVIDIA kepler's broken image_load_store
					require_bgra_swap = true;
				}

				job.reset(new vk::cs_resolve_task(format_prefix, require_bgra_swap));
			}

			job->run(cmd, src, dst);
		}
		else
		{
			std::vector<vk::image*> surface = { dst };
			auto& dev = cmd.get_command_pool().get_owner();

			const auto key = vk::get_renderpass_key(surface);
			auto renderpass = vk::get_renderpass(dev, key);

			if (src->aspect() & VK_IMAGE_ASPECT_STENCIL_BIT)
			{
				if (dev.get_shader_stencil_export_support())
				{
					initialize_pass(g_depthstencil_resolver, dev);
					g_depthstencil_resolver->run(cmd, src, dst, renderpass);
				}
				else
				{
					initialize_pass(g_depth_resolver, dev);
					g_depth_resolver->run(cmd, src, dst, renderpass);

					// Chance for optimization here: If the stencil buffer was not used, simply perform a clear operation
					const auto stencil_init_flags = vk::as_rtt(src)->stencil_init_flags;
					if (stencil_init_flags & 0xFF00)
					{
						initialize_pass(g_stencil_resolver, dev);
						g_stencil_resolver->run(cmd, src, dst, renderpass);
					}
					else
					{
						VkClearDepthStencilValue clear{ 1.f, stencil_init_flags & 0xFF };
						VkImageSubresourceRange range{ VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 };

						dst->push_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
						vkCmdClearDepthStencilImage(cmd, dst->value, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear, 1, &range);
						dst->pop_layout(cmd);
					}
				}
			}
			else
			{
				initialize_pass(g_depth_resolver, dev);
				g_depth_resolver->run(cmd, src, dst, renderpass);
			}
		}
	}

	void unresolve_image(vk::command_buffer& cmd, vk::viewable_image* dst, vk::viewable_image* src)
	{
		if (src->aspect() == VK_IMAGE_ASPECT_COLOR_BIT)
		{
			auto &job = g_unresolve_helpers[src->format()];

			if (!job)
			{
				const char* format_prefix = get_format_prefix(src->format());
				bool require_bgra_swap = false;

				if (vk::get_chip_family() == vk::chip_class::NV_kepler &&
					src->format() == VK_FORMAT_B8G8R8A8_UNORM)
				{
					// Workaround for NVIDIA kepler's broken image_load_store
					require_bgra_swap = true;
				}

				job.reset(new vk::cs_unresolve_task(format_prefix, require_bgra_swap));
			}

			job->run(cmd, dst, src);
		}
		else
		{
			std::vector<vk::image*> surface = { dst };
			auto& dev = cmd.get_command_pool().get_owner();

			const auto key = vk::get_renderpass_key(surface);
			auto renderpass = vk::get_renderpass(dev, key);

			if (src->aspect() & VK_IMAGE_ASPECT_STENCIL_BIT)
			{
				if (dev.get_shader_stencil_export_support())
				{
					initialize_pass(g_depthstencil_unresolver, dev);
					g_depthstencil_unresolver->run(cmd, dst, src, renderpass);
				}
				else
				{
					initialize_pass(g_depth_unresolver, dev);
					g_depth_unresolver->run(cmd, dst, src, renderpass);

					// Chance for optimization here: If the stencil buffer was not used, simply perform a clear operation
					const auto stencil_init_flags = vk::as_rtt(dst)->stencil_init_flags;
					if (stencil_init_flags & 0xFF00)
					{
						initialize_pass(g_stencil_unresolver, dev);
						g_stencil_unresolver->run(cmd, dst, src, renderpass);
					}
					else
					{
						VkClearDepthStencilValue clear{ 1.f, stencil_init_flags & 0xFF };
						VkImageSubresourceRange range{ VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 };

						dst->push_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
						vkCmdClearDepthStencilImage(cmd, dst->value, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear, 1, &range);
						dst->pop_layout(cmd);
					}
				}
			}
			else
			{
				initialize_pass(g_depth_unresolver, dev);
				g_depth_unresolver->run(cmd, dst, src, renderpass);
			}
		}
	}

	void clear_resolve_helpers()
	{
		for (auto &task : g_resolve_helpers)
		{
			task.second->destroy();
		}

		for (auto &task : g_unresolve_helpers)
		{
			task.second->destroy();
		}

		g_resolve_helpers.clear();
		g_unresolve_helpers.clear();

		if (g_depth_resolver)
		{
			g_depth_resolver->destroy();
			g_depth_resolver.reset();
		}

		if (g_stencil_resolver)
		{
			g_stencil_resolver->destroy();
			g_stencil_resolver.reset();
		}

		if (g_depthstencil_resolver)
		{
			g_depthstencil_resolver->destroy();
			g_depthstencil_resolver.reset();
		}

		if (g_depth_unresolver)
		{
			g_depth_unresolver->destroy();
			g_depth_unresolver.reset();
		}

		if (g_stencil_unresolver)
		{
			g_stencil_unresolver->destroy();
			g_stencil_unresolver.reset();
		}

		if (g_depthstencil_unresolver)
		{
			g_depthstencil_unresolver->destroy();
			g_depthstencil_unresolver.reset();
		}
	}

	void reset_resolve_resources()
	{
		for (auto &e : g_resolve_helpers) e.second->free_resources();
		for (auto &e : g_unresolve_helpers) e.second->free_resources();

		if (g_depth_resolver) g_depth_resolver->free_resources();
		if (g_depth_unresolver) g_depth_unresolver->free_resources();
		if (g_stencil_resolver) g_stencil_resolver->free_resources();
		if (g_stencil_unresolver) g_stencil_unresolver->free_resources();
		if (g_depthstencil_resolver) g_depthstencil_resolver->free_resources();
		if (g_depthstencil_unresolver) g_depthstencil_unresolver->free_resources();
	}
}