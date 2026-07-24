#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include "../metalfx_pass.h"

#include "../../VKHelpers.h"
#include "../../vkutils/commands.h"
#include "../../vkutils/device.h"
#include "../../vkutils/image_helpers.h"
#include "../../vkutils/sync.h"

#include "util/logs.hpp"

#import <Metal/Metal.h>
#import <MetalFX/MetalFX.h>

static constexpr u64 metalfx_wait_timeout = 2000000ull;

namespace vk
{
	namespace
	{
		u32 find_memory_type(const VkPhysicalDeviceMemoryProperties& props, u32 type_bits, VkMemoryPropertyFlags required)
		{
			for (u32 i = 0; i < props.memoryTypeCount; ++i)
			{
				if ((type_bits & (1u << i)) && (props.memoryTypes[i].propertyFlags & required) == required)
				{
					return i;
				}
			}
			return UINT32_MAX;
		}
	}

	struct metalfx_upscale_pass::impl
	{
		struct exportable_image
		{
			VkImage image        = VK_NULL_HANDLE;
			VkDeviceMemory memory = VK_NULL_HANDLE;
			id<MTLTexture> mtl_texture = nil;
			VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
			u32 width = 0, height = 0;
			VkFormat format = VK_FORMAT_UNDEFINED;
		};

		vk::render_device* dev = nullptr;

		id<MTLDevice> mtl_device = nil;
		id<MTLCommandQueue> mtl_queue = nil;

		id<MTLFXSpatialScaler> scaler = nil;
		u32 scaler_in_w = 0, scaler_in_h = 0, scaler_out_w = 0, scaler_out_h = 0;
		MTLPixelFormat scaler_format = MTLPixelFormatInvalid;

		exportable_image input;
		exportable_image output_left;
		exportable_image output_right;

		vk::command_pool cmd_pool;
		vk::command_buffer cmd;
		std::unique_ptr<vk::fence> submit_fence;
		bool owned_cb_ready = false;

		PFN_vkExportMetalObjectsEXT pfn_export = nullptr;

		~impl()
		{
			destroy_image(input);
			destroy_image(output_left);
			destroy_image(output_right);

			if (owned_cb_ready)
			{
				cmd.destroy();
				cmd_pool.destroy();
			}
			submit_fence.reset();

			if (scaler)
			{
				[scaler release];
				scaler = nil;
			}

			// Borrowed from MoltenVK, not released
			mtl_queue = nil;
			mtl_device = nil;
		}

		bool bind_metal_handles()
		{
			if (mtl_device && mtl_queue)
			{
				return true;
			}

			if (!pfn_export)
			{
				pfn_export = reinterpret_cast<PFN_vkExportMetalObjectsEXT>(
					vkGetDeviceProcAddr(*dev, "vkExportMetalObjectsEXT"));
			}
			if (!pfn_export)
			{
				return false;
			}

			VkExportMetalDeviceInfoEXT device_info = { VK_STRUCTURE_TYPE_EXPORT_METAL_DEVICE_INFO_EXT };
			VkExportMetalCommandQueueInfoEXT queue_info = { VK_STRUCTURE_TYPE_EXPORT_METAL_COMMAND_QUEUE_INFO_EXT };
			queue_info.queue = dev->get_graphics_queue();
			queue_info.pNext = &device_info;

			VkExportMetalObjectsInfoEXT objects = { VK_STRUCTURE_TYPE_EXPORT_METAL_OBJECTS_INFO_EXT };
			objects.pNext = &queue_info;
			pfn_export(*dev, &objects);

			mtl_device = device_info.mtlDevice;
			mtl_queue = queue_info.mtlCommandQueue;
			return mtl_device != nil && mtl_queue != nil;
		}

		void destroy_image(exportable_image& img)
		{
			img.mtl_texture = nil;
			if (img.image)  vkDestroyImage(*dev, img.image, nullptr);
			if (img.memory) vkFreeMemory(*dev, img.memory, nullptr);
			img = {};
		}

		// Create a VkImage backed by an exportable MTLTexture and fetch that texture.
		bool create_image(exportable_image& img, u32 w, u32 h, VkFormat format, VkImageUsageFlags usage)
		{
			destroy_image(img);

			VkExportMetalObjectCreateInfoEXT export_ci = { VK_STRUCTURE_TYPE_EXPORT_METAL_OBJECT_CREATE_INFO_EXT };
			export_ci.exportObjectType = VK_EXPORT_METAL_OBJECT_TYPE_METAL_TEXTURE_BIT_EXT;

			VkImageCreateInfo ci = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
			ci.pNext = &export_ci;
			ci.imageType = VK_IMAGE_TYPE_2D;
			ci.format = format;
			ci.extent = { w, h, 1 };
			ci.mipLevels = 1;
			ci.arrayLayers = 1;
			ci.samples = VK_SAMPLE_COUNT_1_BIT;
			ci.tiling = VK_IMAGE_TILING_OPTIMAL;
			ci.usage = usage;
			ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			if (vkCreateImage(*dev, &ci, nullptr, &img.image) != VK_SUCCESS)
			{
				return false;
			}

			VkMemoryRequirements reqs = {};
			vkGetImageMemoryRequirements(*dev, img.image, &reqs);

			const u32 mem_type = find_memory_type(dev->gpu().get_memory_properties(), reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			if (mem_type == UINT32_MAX)
			{
				destroy_image(img);
				return false;
			}

			VkMemoryAllocateInfo alloc = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
			alloc.allocationSize = reqs.size;
			alloc.memoryTypeIndex = mem_type;

			if (vkAllocateMemory(*dev, &alloc, nullptr, &img.memory) != VK_SUCCESS ||
				vkBindImageMemory(*dev, img.image, img.memory, 0) != VK_SUCCESS)
			{
				destroy_image(img);
				return false;
			}

			VkExportMetalTextureInfoEXT tex_info = { VK_STRUCTURE_TYPE_EXPORT_METAL_TEXTURE_INFO_EXT };
			tex_info.image = img.image;
			tex_info.plane = VK_IMAGE_ASPECT_PLANE_0_BIT;

			VkExportMetalObjectsInfoEXT objects = { VK_STRUCTURE_TYPE_EXPORT_METAL_OBJECTS_INFO_EXT };
			objects.pNext = &tex_info;
			pfn_export(*dev, &objects);

			img.mtl_texture = tex_info.mtlTexture;
			img.layout = VK_IMAGE_LAYOUT_UNDEFINED;
			img.width = w;
			img.height = h;
			img.format = format;

			if (!img.mtl_texture)
			{
				destroy_image(img);
				return false;
			}
			return true;
		}

		bool ensure_scaler(u32 in_w, u32 in_h, u32 out_w, u32 out_h, MTLPixelFormat format)
		{
			if (scaler && scaler_in_w == in_w && scaler_in_h == in_h &&
				scaler_out_w == out_w && scaler_out_h == out_h && scaler_format == format)
			{
				return true;
			}

			if (@available(macOS 13.0, *))
			{
				if (scaler)
				{
					[scaler release];
					scaler = nil;
				}

				MTLFXSpatialScalerDescriptor* desc = [[MTLFXSpatialScalerDescriptor alloc] init];
				desc.inputWidth = in_w;
				desc.inputHeight = in_h;
				desc.outputWidth = out_w;
				desc.outputHeight = out_h;
				desc.colorTextureFormat = format;
				desc.outputTextureFormat = format;
				desc.colorProcessingMode = MTLFXSpatialScalerColorProcessingModePerceptual;

				scaler = [desc newSpatialScalerWithDevice:mtl_device];
				[desc release];

				if (!scaler)
				{
					return false;
				}

				scaler_in_w = in_w; scaler_in_h = in_h;
				scaler_out_w = out_w; scaler_out_h = out_h;
				scaler_format = format;
				return true;
			}

			return false;
		}

		bool ensure_owned_cb()
		{
			if (owned_cb_ready)
			{
				return true;
			}

			cmd_pool.create(*dev, dev->get_graphics_queue_family());
			cmd.create(cmd_pool);
			submit_fence = std::make_unique<vk::fence>(*dev);
			owned_cb_ready = true;
			return true;
		}
	};

	metalfx_upscale_pass::metalfx_upscale_pass()
		: m_impl(std::make_unique<impl>())
	{
		m_impl->dev = const_cast<vk::render_device*>(vk::get_current_renderer());
	}

	metalfx_upscale_pass::~metalfx_upscale_pass() = default;

	bool metalfx_upscale_pass::is_available()
	{
		static int cached = -1;
		if (cached != -1)
		{
			return cached == 1;
		}

		auto* dev = const_cast<vk::render_device*>(vk::get_current_renderer());
		if (!dev)
		{
			// No device to probe yet, don't cache
			return false;
		}

		cached = 0;

		if (!dev->get_metal_objects_support())
		{
			return false;
		}

		if (@available(macOS 13.0, *))
		{
			auto probe = std::make_unique<impl>();
			probe->dev = dev;
			if (probe->bind_metal_handles() && [MTLFXSpatialScalerDescriptor supportsDevice:probe->mtl_device])
			{
				cached = 1;
			}
		}

		return cached == 1;
	}

	vk::viewable_image* metalfx_upscale_pass::scale_output(
		const vk::command_buffer& cmd,
		vk::viewable_image* src,
		VkImage present_surface,
		VkImageLayout present_surface_layout,
		const VkImageBlit& request,
		rsx::flags32_t mode)
	{
		size2u input_size, output_size;
		input_size.width   = std::abs(request.srcOffsets[1].x - request.srcOffsets[0].x);
		input_size.height  = std::abs(request.srcOffsets[1].y - request.srcOffsets[0].y);
		output_size.width  = std::abs(request.dstOffsets[1].x - request.dstOffsets[0].x);
		output_size.height = std::abs(request.dstOffsets[1].y - request.dstOffsets[0].y);

		const bool is_upscale = input_size.width < output_size.width && input_size.height < output_size.height;

		// Only handle the direct-to-present path; anything else falls back to a plain blit
		if (!(mode & UPSCALE_AND_COMMIT) || !is_upscale || !present_surface)
		{
			if (mode & UPSCALE_AND_COMMIT)
			{
				ensure(present_surface);
				src->push_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
				vkCmdBlitImage(cmd, src->value, src->current_layout, present_surface, present_surface_layout, 1, &request, VK_FILTER_LINEAR);
				src->pop_layout(cmd);
				return nullptr;
			}
			return src;
		}

		const VkFormat work_format = VK_FORMAT_B8G8R8A8_UNORM;
		const MTLPixelFormat mtl_format = MTLPixelFormatBGRA8Unorm;

		auto& out_img = (mode & UPSCALE_RIGHT_VIEW) ? m_impl->output_right : m_impl->output_left;

		const bool sizes_ok =
			m_impl->bind_metal_handles() &&
			(m_impl->input.width == input_size.width && m_impl->input.height == input_size.height && m_impl->input.format == work_format
				? true
				: m_impl->create_image(m_impl->input, input_size.width, input_size.height, work_format,
					VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)) &&
			(out_img.width == output_size.width && out_img.height == output_size.height && out_img.format == work_format
				? true
				: m_impl->create_image(out_img, output_size.width, output_size.height, work_format,
					VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)) &&
			m_impl->ensure_scaler(input_size.width, input_size.height, output_size.width, output_size.height, mtl_format) &&
			m_impl->ensure_owned_cb();

		const auto bilinear_fallback = [&]()
		{
			rsx_log.warning("MetalFX upscaling unavailable for this frame, falling back to bilinear.");
			src->push_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			vkCmdBlitImage(cmd, src->value, src->current_layout, present_surface, present_surface_layout, 1, &request, VK_FILTER_LINEAR);
			src->pop_layout(cmd);
		};

		if (!sizes_ok)
		{
			bilinear_fallback();
			return nullptr;
		}

		{
			auto& ocmd = m_impl->cmd;
			ocmd.begin();

			src->push_layout(ocmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			vk::change_image_layout(ocmd, m_impl->input.image, m_impl->input.layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

			VkImageBlit in_blit = {};
			in_blit.srcSubresource = { src->aspect(), 0, 0, 1 };
			in_blit.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			in_blit.srcOffsets[0] = request.srcOffsets[0];
			in_blit.srcOffsets[1] = request.srcOffsets[1];
			in_blit.dstOffsets[0] = { 0, 0, 0 };
			in_blit.dstOffsets[1] = { s32(input_size.width), s32(input_size.height), 1 };
			vkCmdBlitImage(ocmd, src->value, src->current_layout, m_impl->input.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &in_blit, VK_FILTER_NEAREST);

			vk::change_image_layout(ocmd, m_impl->input.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
				{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
			m_impl->input.layout = VK_IMAGE_LAYOUT_GENERAL;

			if (out_img.layout == VK_IMAGE_LAYOUT_UNDEFINED)
			{
				vk::change_image_layout(ocmd, out_img.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
					{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
				out_img.layout = VK_IMAGE_LAYOUT_GENERAL;
			}

			src->pop_layout(ocmd);
			ocmd.end();

			m_impl->submit_fence->reset();
			vk::queue_submit_t submit_info{ m_impl->dev->get_graphics_queue(), m_impl->submit_fence.get() };
			ocmd.submit(submit_info);
			vk::wait_for_fence(m_impl->submit_fence.get(), metalfx_wait_timeout);
		}

		bool metal_ok = false;
		if (@available(macOS 13.0, *))
		{
			@autoreleasepool
			{
				m_impl->scaler.colorTexture = m_impl->input.mtl_texture;
				m_impl->scaler.outputTexture = out_img.mtl_texture;

				id<MTLCommandBuffer> mtl_cb = [m_impl->mtl_queue commandBuffer];
				[m_impl->scaler encodeToCommandBuffer:mtl_cb];
				[mtl_cb commit];
				[mtl_cb waitUntilCompleted];

				metal_ok = ([mtl_cb status] == MTLCommandBufferStatusCompleted);
				if (!metal_ok)
				{
					const char* err = [mtl_cb error] ? [[[mtl_cb error] localizedDescription] UTF8String] : "unknown error";
					rsx_log.error("MetalFX: spatial scaler execution failed (%s)", err);
				}
			}
		}

		if (!metal_ok)
		{
			bilinear_fallback();
			return nullptr;
		}

		VkImageBlit out_blit = {};
		out_blit.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		out_blit.dstSubresource = request.dstSubresource;
		out_blit.srcOffsets[0] = { 0, 0, 0 };
		out_blit.srcOffsets[1] = { s32(output_size.width), s32(output_size.height), 1 };
		out_blit.dstOffsets[0] = request.dstOffsets[0];
		out_blit.dstOffsets[1] = request.dstOffsets[1];

		vkCmdBlitImage(cmd, out_img.image, VK_IMAGE_LAYOUT_GENERAL, present_surface, present_surface_layout, 1, &out_blit, VK_FILTER_LINEAR);
		return nullptr;
	}
}

#pragma GCC diagnostic pop
