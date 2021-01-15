#include "stdafx.h"
#include "barriers.h"
#include "device.h"
#include "image.h"
#include "image_helpers.h"

#include <memory>

namespace vk
{
	void image::validate(const vk::render_device& dev, const VkImageCreateInfo& info) const
	{
		const auto gpu_limits = dev.gpu().get_limits();
		u32 longest_dim, dim_limit;

		switch (info.imageType)
		{
		case VK_IMAGE_TYPE_1D:
			longest_dim = info.extent.width;
			dim_limit = gpu_limits.maxImageDimension1D;
			break;
		case VK_IMAGE_TYPE_2D:
			longest_dim = std::max(info.extent.width, info.extent.height);
			dim_limit = (info.flags == VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) ? gpu_limits.maxImageDimensionCube : gpu_limits.maxImageDimension2D;
			break;
		case VK_IMAGE_TYPE_3D:
			longest_dim = std::max({ info.extent.width, info.extent.height, info.extent.depth });
			dim_limit = gpu_limits.maxImageDimension3D;
			break;
		default:
			fmt::throw_exception("Unreachable");
		}

		if (longest_dim > dim_limit)
		{
			// Longest dimension exceeds the limit. Can happen when using MSAA + very high resolution scaling
			// Just kill the application at this point.
			fmt::throw_exception(
				"The renderer requested an image larger than the limit allowed for by your GPU hardware. "
				"Turn down your resolution scale and/or disable MSAA to fit within the image budget.");
		}
	}

	image::image(const vk::render_device& dev,
		u32 memory_type_index,
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
		rsx::format_class format_class)
		: current_layout(initial_layout)
		, m_device(dev)
	{
		info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.imageType = image_type;
		info.format = format;
		info.extent = { width, height, depth };
		info.mipLevels = mipmaps;
		info.arrayLayers = layers;
		info.samples = samples;
		info.tiling = tiling;
		info.usage = usage;
		info.flags = image_flags;
		info.initialLayout = initial_layout;
		info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		validate(dev, info);
		CHECK_RESULT(vkCreateImage(m_device, &info, nullptr, &value));

		VkMemoryRequirements memory_req;
		vkGetImageMemoryRequirements(m_device, value, &memory_req);

		if (!(memory_req.memoryTypeBits & (1 << memory_type_index)))
		{
			//Suggested memory type is incompatible with this memory type.
			//Go through the bitset and test for requested props.
			if (!dev.get_compatible_memory_type(memory_req.memoryTypeBits, access_flags, &memory_type_index))
				fmt::throw_exception("No compatible memory type was found!");
		}

		memory = std::make_shared<vk::memory_block>(m_device, memory_req.size, memory_req.alignment, memory_type_index);
		CHECK_RESULT(vkBindImageMemory(m_device, value, memory->get_vk_device_memory(), memory->get_vk_device_memory_offset()));

		m_storage_aspect = get_aspect_flags(format);

		if (format_class == RSX_FORMAT_CLASS_UNDEFINED)
		{
			if (m_storage_aspect != VK_IMAGE_ASPECT_COLOR_BIT)
			{
				rsx_log.error("Depth/stencil textures must have format class explicitly declared");
			}
			else
			{
				format_class = RSX_FORMAT_CLASS_COLOR;
			}
		}

		m_format_class = format_class;
	}

	// TODO: Ctor that uses a provided memory heap

	image::~image()
	{
		vkDestroyImage(m_device, value, nullptr);
	}

	u32 image::width() const
	{
		return info.extent.width;
	}

	u32 image::height() const
	{
		return info.extent.height;
	}

	u32 image::depth() const
	{
		return info.extent.depth;
	}

	u32 image::mipmaps() const
	{
		return info.mipLevels;
	}

	u32 image::layers() const
	{
		return info.arrayLayers;
	}

	u8 image::samples() const
	{
		return u8(info.samples);
	}

	VkFormat image::format() const
	{
		return info.format;
	}

	VkImageAspectFlags image::aspect() const
	{
		return m_storage_aspect;
	}

	rsx::format_class image::format_class() const
	{
		return m_format_class;
	}

	void image::push_layout(VkCommandBuffer cmd, VkImageLayout layout)
	{
		m_layout_stack.push(current_layout);
		change_image_layout(cmd, this, layout);
	}

	void image::push_barrier(VkCommandBuffer cmd, VkImageLayout layout)
	{
		m_layout_stack.push(current_layout);
		insert_texture_barrier(cmd, this, layout);
	}

	void image::pop_layout(VkCommandBuffer cmd)
	{
		ensure(!m_layout_stack.empty());

		auto layout = m_layout_stack.top();
		m_layout_stack.pop();
		change_image_layout(cmd, this, layout);
	}

	void image::change_layout(command_buffer& cmd, VkImageLayout new_layout)
	{
		if (current_layout == new_layout)
			return;

		ensure(m_layout_stack.empty());
		change_image_layout(cmd, this, new_layout);
	}

	image_view::image_view(VkDevice dev, VkImage image, VkImageViewType view_type, VkFormat format, VkComponentMapping mapping, VkImageSubresourceRange range)
		: m_device(dev)
	{
		info.format = format;
		info.image = image;
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.components = mapping;
		info.viewType = view_type;
		info.subresourceRange = range;

		create_impl();
	}

	image_view::image_view(VkDevice dev, VkImageViewCreateInfo create_info)
		: info(create_info)
		, m_device(dev)
	{
		create_impl();
	}

	image_view::image_view(VkDevice dev, vk::image* resource, VkImageViewType view_type, const VkComponentMapping& mapping, const VkImageSubresourceRange& range)
		: m_device(dev), m_resource(resource)
	{
		info.format = resource->info.format;
		info.image = resource->value;
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.components = mapping;
		info.subresourceRange = range;

		if (view_type == VK_IMAGE_VIEW_TYPE_MAX_ENUM)
		{
			switch (resource->info.imageType)
			{
			case VK_IMAGE_TYPE_1D:
				info.viewType = VK_IMAGE_VIEW_TYPE_1D;
				break;
			case VK_IMAGE_TYPE_2D:
				if (resource->info.flags == VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
					info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
				else if (resource->info.arrayLayers == 1)
					info.viewType = VK_IMAGE_VIEW_TYPE_2D;
				else
					info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
				break;
			case VK_IMAGE_TYPE_3D:
				info.viewType = VK_IMAGE_VIEW_TYPE_3D;
				break;
			default:
				fmt::throw_exception("Unreachable");
			}

			info.subresourceRange.layerCount = resource->info.arrayLayers;
		}
		else
		{
			info.viewType = view_type;
		}

		create_impl();
	}

	image_view::~image_view()
	{
		vkDestroyImageView(m_device, value, nullptr);
	}

	u32 image_view::encoded_component_map() const
	{
#if	(VK_DISABLE_COMPONENT_SWIZZLE)
		u32 result = static_cast<u32>(info.components.a) - 1;
		result |= (static_cast<u32>(info.components.r) - 1) << 3;
		result |= (static_cast<u32>(info.components.g) - 1) << 6;
		result |= (static_cast<u32>(info.components.b) - 1) << 9;

		return result;
#else
		return 0;
#endif
	}

	vk::image* image_view::image() const
	{
		return m_resource;
	}

	void image_view::create_impl()
	{
#if (VK_DISABLE_COMPONENT_SWIZZLE)
		// Force identity
		const auto mapping = info.components;
		info.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
#endif

		CHECK_RESULT(vkCreateImageView(m_device, &info, nullptr, &value));

#if (VK_DISABLE_COMPONENT_SWIZZLE)
		// Restore requested mapping
		info.components = mapping;
#endif
	}

	image_view* viewable_image::get_view(u32 remap_encoding, const std::pair<std::array<u8, 4>, std::array<u8, 4>>& remap, VkImageAspectFlags mask)
	{
		if (remap_encoding == VK_REMAP_IDENTITY)
		{
			if (native_component_map.a == VK_COMPONENT_SWIZZLE_A &&
				native_component_map.r == VK_COMPONENT_SWIZZLE_R &&
				native_component_map.g == VK_COMPONENT_SWIZZLE_G &&
				native_component_map.b == VK_COMPONENT_SWIZZLE_B)
			{
				remap_encoding = 0xAAE4;
			}
		}

		auto found = views.equal_range(remap_encoding);
		for (auto It = found.first; It != found.second; ++It)
		{
			if (It->second->info.subresourceRange.aspectMask & mask)
			{
				return It->second.get();
			}
		}

		VkComponentMapping real_mapping;
		switch (remap_encoding)
		{
		case VK_REMAP_IDENTITY:
			real_mapping = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
			break;
		case 0xAAE4:
			real_mapping = native_component_map;
			break;
		default:
			real_mapping = vk::apply_swizzle_remap
			(
				{ native_component_map.a, native_component_map.r, native_component_map.g, native_component_map.b },
				remap
			);
			break;
		}

		const VkImageSubresourceRange range = { aspect() & mask, 0, info.mipLevels, 0, info.arrayLayers };
		ensure(range.aspectMask);

		auto view = std::make_unique<vk::image_view>(*g_render_device, this, VK_IMAGE_VIEW_TYPE_MAX_ENUM, real_mapping, range);
		auto result = view.get();
		views.emplace(remap_encoding, std::move(view));
		return result;
	}

	void viewable_image::set_native_component_layout(VkComponentMapping new_layout)
	{
		if (new_layout.r != native_component_map.r ||
			new_layout.g != native_component_map.g ||
			new_layout.b != native_component_map.b ||
			new_layout.a != native_component_map.a)
		{
			native_component_map = new_layout;
			views.clear();
		}
	}
}
