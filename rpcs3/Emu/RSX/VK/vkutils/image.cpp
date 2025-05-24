#include "stdafx.h"
#include "barriers.h"
#include "device.h"
#include "image.h"
#include "image_helpers.h"

#include "../VKResourceManager.h"
#include <memory>

namespace vk
{
	void image::validate(const vk::render_device& dev, const VkImageCreateInfo& info) const
	{
		const auto& gpu_limits = dev.gpu().get_limits();
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
		rsx::format_class format_class)
		: m_device(dev)
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

		std::array<u32, 2> concurrency_queue_families = {
			dev.get_graphics_queue_family(),
			dev.get_transfer_queue_family()
		};

		if (image_flags & VK_IMAGE_CREATE_SHAREABLE_RPCS3)
		{
			info.sharingMode = VK_SHARING_MODE_CONCURRENT;
			info.queueFamilyIndexCount = ::size32(concurrency_queue_families);
			info.pQueueFamilyIndices = concurrency_queue_families.data();
		}

		create_impl(dev, access_flags, memory_type, allocation_pool);
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

	void image::create_impl(const vk::render_device& dev, u32 access_flags, const memory_type_info& memory_type, vmm_allocation_pool allocation_pool)
	{
		ensure(!value && !memory);
		validate(dev, info);

		const bool nullable = !!(info.flags & VK_IMAGE_CREATE_ALLOW_NULL_RPCS3);
		info.flags &= ~VK_IMAGE_CREATE_SPECIAL_FLAGS_RPCS3;

		CHECK_RESULT(vkCreateImage(m_device, &info, nullptr, &value));

		VkMemoryRequirements memory_req;
		vkGetImageMemoryRequirements(m_device, value, &memory_req);

		const auto allocation_type_info = memory_type.get(dev, access_flags, memory_req.memoryTypeBits);
		if (!allocation_type_info)
		{
			fmt::throw_exception("No compatible memory type was found!");
		}

		memory = std::make_shared<vk::memory_block>(m_device, memory_req.size, memory_req.alignment, allocation_type_info, allocation_pool, nullable);
		if (auto device_mem = memory->get_vk_device_memory();
			device_mem != VK_NULL_HANDLE) [[likely]]
		{
			CHECK_RESULT(vkBindImageMemory(m_device, value, device_mem, memory->get_vk_device_memory_offset()));
			current_layout = info.initialLayout;
		}
		else
		{
			ensure(nullable);
			vkDestroyImage(m_device, value, nullptr);
			value = VK_NULL_HANDLE;
		}
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

	VkImageType image::type() const
	{
		return info.imageType;
	}

	VkSharingMode image::sharing_mode() const
	{
		return info.sharingMode;
	}

	VkImageAspectFlags image::aspect() const
	{
		return m_storage_aspect;
	}

	rsx::format_class image::format_class() const
	{
		return m_format_class;
	}

	void image::push_layout(const command_buffer& cmd, VkImageLayout layout)
	{
		ensure(current_queue_family == VK_QUEUE_FAMILY_IGNORED || current_queue_family == cmd.get_queue_family());

		m_layout_stack.push(current_layout);
		change_image_layout(cmd, this, layout);
	}

	void image::push_barrier(const command_buffer& cmd, VkImageLayout layout)
	{
		ensure(current_queue_family == VK_QUEUE_FAMILY_IGNORED || current_queue_family == cmd.get_queue_family());

		m_layout_stack.push(current_layout);
		insert_texture_barrier(cmd, this, layout);
	}

	void image::pop_layout(const command_buffer& cmd)
	{
		ensure(current_queue_family == VK_QUEUE_FAMILY_IGNORED || current_queue_family == cmd.get_queue_family());
		ensure(!m_layout_stack.empty());

		auto layout = m_layout_stack.top();
		m_layout_stack.pop();
		change_image_layout(cmd, this, layout);
	}

	void image::queue_acquire(const command_buffer& cmd, VkImageLayout new_layout)
	{
		ensure(m_layout_stack.empty());
		ensure(current_queue_family != cmd.get_queue_family());

		if (info.sharingMode == VK_SHARING_MODE_EXCLUSIVE || current_layout != new_layout)
		{
			VkImageSubresourceRange range = { aspect(), 0, mipmaps(), 0, layers() };
			const u32 src_queue_family = info.sharingMode == VK_SHARING_MODE_EXCLUSIVE ? current_queue_family : VK_QUEUE_FAMILY_IGNORED;
			const u32 dst_queue_family = info.sharingMode == VK_SHARING_MODE_EXCLUSIVE ? cmd.get_queue_family() : VK_QUEUE_FAMILY_IGNORED;
			change_image_layout(cmd, value, current_layout, new_layout, range, src_queue_family, dst_queue_family, 0u, ~0u);
		}

		current_layout = new_layout;
		current_queue_family = cmd.get_queue_family();
	}

	void image::queue_release(const command_buffer& src_queue_cmd, u32 dst_queue_family, VkImageLayout new_layout)
	{
		ensure(current_queue_family == src_queue_cmd.get_queue_family());
		ensure(m_layout_stack.empty());

		if (info.sharingMode == VK_SHARING_MODE_EXCLUSIVE || current_layout != new_layout)
		{
			VkImageSubresourceRange range = { aspect(), 0, mipmaps(), 0, layers() };
			const u32 src_queue_family = info.sharingMode == VK_SHARING_MODE_EXCLUSIVE ? current_queue_family : VK_QUEUE_FAMILY_IGNORED;
			const u32 dst_queue_family2 = info.sharingMode == VK_SHARING_MODE_EXCLUSIVE ? dst_queue_family : VK_QUEUE_FAMILY_IGNORED;
			change_image_layout(src_queue_cmd, value, current_layout, new_layout, range, src_queue_family, dst_queue_family2, ~0u, 0u);
		}

		current_layout = new_layout;
		current_queue_family = dst_queue_family;
	}

	void image::change_layout(const command_buffer& cmd, VkImageLayout new_layout)
	{
		// This is implicitly an acquire op
		if (const auto new_queue_family = cmd.get_queue_family();
			current_queue_family == VK_QUEUE_FAMILY_IGNORED)
		{
			current_queue_family = new_queue_family;
		}
		else if (current_queue_family != new_queue_family)
		{
			queue_acquire(cmd, new_layout);
			return;
		}

		if (current_layout == new_layout)
		{
			return;
		}

		ensure(m_layout_stack.empty());
		change_image_layout(cmd, this, new_layout);

		current_queue_family = cmd.get_queue_family();
	}

	void image::set_debug_name(const std::string& name)
	{
		if (g_render_device->get_debug_utils_support())
		{
			VkDebugUtilsObjectNameInfoEXT name_info{};
			name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			name_info.objectType = VK_OBJECT_TYPE_IMAGE;
			name_info.objectHandle = reinterpret_cast<u64>(value);
			name_info.pObjectName = name.c_str();

			_vkSetDebugUtilsObjectNameEXT(m_device, &name_info);
		}
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

	viewable_image* viewable_image::clone()
	{
		// Destructive cloning. The clone grabs the GPU objects owned by this instance.
		// This instance can be rebuilt in-place by calling create_impl() which will create a duplicate now owned by this.
		auto result = new viewable_image();
		result->m_device = this->m_device;
		result->info = this->info;
		result->value = this->value;
		result->memory = std::move(this->memory);
		result->views = std::move(this->views);
		this->value = VK_NULL_HANDLE;
		return result;
	}

	image_view* viewable_image::get_view(const rsx::texture_channel_remap_t& remap, VkImageAspectFlags mask)
	{
		u32 remap_encoding = remap.encoded;
		if (remap_encoding == VK_REMAP_IDENTITY)
		{
			if (native_component_map.a == VK_COMPONENT_SWIZZLE_A &&
				native_component_map.r == VK_COMPONENT_SWIZZLE_R &&
				native_component_map.g == VK_COMPONENT_SWIZZLE_G &&
				native_component_map.b == VK_COMPONENT_SWIZZLE_B)
			{
				remap_encoding = RSX_TEXTURE_REMAP_IDENTITY;
			}
		}

		const u64 storage_key = remap_encoding | (static_cast<u64>(mask) << 32);
		auto found = views.find(storage_key);
		if (found != views.end())
		{
			ensure(found->second->info.subresourceRange.aspectMask & mask);
			return found->second.get();
		}

		VkComponentMapping real_mapping;
		switch (remap_encoding)
		{
		case VK_REMAP_IDENTITY:
			real_mapping = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
			break;
		case RSX_TEXTURE_REMAP_IDENTITY:
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
		views.emplace(storage_key, std::move(view));
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

			// Safely discard existing views
			auto gc = vk::get_resource_manager();
			for (auto& p : views)
			{
				gc->dispose(p.second);
			}
			views.clear();
		}
	}
}
