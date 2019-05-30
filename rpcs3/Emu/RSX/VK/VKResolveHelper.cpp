#include "stdafx.h"

#include "VKResolveHelper.h"
#include "VKRenderPass.h"

namespace
{
	const char *get_format_prefix(VkFormat format)
	{
		switch (format)
		{
			case VK_FORMAT_R5G6B5_UNORM_PACK16:
				return "r16ui";
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
	std::unique_ptr<vk::depthstencil_resolve_AMD> g_depthstencil_resolverAMD;
	std::unique_ptr<vk::depthstencil_unresolve_AMD> g_depthstencil_unresolverAMD;

	void resolve_image(vk::command_buffer& cmd, vk::viewable_image* dst, vk::viewable_image* src)
	{
		if (src->aspect() == VK_IMAGE_ASPECT_COLOR_BIT)
		{
			auto &job = g_resolve_helpers[src->format()];

			if (!job)
			{
				job.reset(new vk::cs_resolve_task(get_format_prefix(src->format())));
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
				if (!g_depthstencil_resolverAMD)
				{
					g_depthstencil_resolverAMD.reset(new vk::depthstencil_resolve_AMD());
					g_depthstencil_resolverAMD->create(dev);
				}

				g_depthstencil_resolverAMD->run(cmd, src, dst, renderpass);
			}
			else
			{
				if (!g_depth_resolver)
				{
					g_depth_resolver.reset(new vk::depthonly_resolve());
					g_depth_resolver->create(dev);
				}

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
				job.reset(new vk::cs_unresolve_task(get_format_prefix(src->format())));
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
				if (!g_depthstencil_unresolverAMD)
				{
					g_depthstencil_unresolverAMD.reset(new vk::depthstencil_unresolve_AMD());
					g_depthstencil_unresolverAMD->create(dev);
				}

				g_depthstencil_unresolverAMD->run(cmd, dst, src, renderpass);
			}
			else
			{
				if (!g_depth_unresolver)
				{
					g_depth_unresolver.reset(new vk::depthonly_unresolve());
					g_depth_unresolver->create(dev);
				}

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

		if (g_depthstencil_resolverAMD)
		{
			g_depthstencil_resolverAMD->destroy();
			g_depthstencil_resolverAMD.reset();
		}

		if (g_depth_unresolver)
		{
			g_depth_unresolver->destroy();
			g_depth_unresolver.reset();
		}

		if (g_depthstencil_unresolverAMD)
		{
			g_depthstencil_unresolverAMD->destroy();
			g_depthstencil_unresolverAMD.reset();
		}
	}

	void reset_resolve_resources()
	{
		for (auto &e : g_resolve_helpers) e.second->free_resources();
		for (auto &e : g_unresolve_helpers) e.second->free_resources();

		if (g_depth_resolver) g_depth_resolver->free_resources();
		if (g_depth_unresolver) g_depth_unresolver->free_resources();
		if (g_depthstencil_resolverAMD) g_depthstencil_resolverAMD->free_resources();
		if (g_depthstencil_unresolverAMD) g_depthstencil_unresolverAMD->free_resources();
	}
}