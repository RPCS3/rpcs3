#include "stdafx.h"
#include "openxr_renderer.h"
#include "openxr_frame_share_vk.h"

#include <algorithm>
#include <cstring>
#include <mutex>

#ifndef _WIN32
#include <unistd.h>
#endif

namespace openxr
{
namespace
{
	// Placement of the virtual screen, relative to the initial head pose (in meters)
	constexpr float virtual_screen_distance = 2.5f;
	constexpr float virtual_screen_width = 2.4f;

	// RSX output is already gamma encoded. Copying it bitwise into an sRGB swapchain lets the compositor decode it correctly.
	struct swapchain_format_pair
	{
		VkFormat swapchain;
		VkFormat shared;
	};

	constexpr std::array<swapchain_format_pair, 4> swapchain_formats =
	{{
		{ VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_R8G8B8A8_UNORM },
		{ VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM },
		{ VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM },
		{ VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM },
	}};

	// Creates a swapchain with the first of our formats that the runtime supports
	const swapchain_format_pair* create_color_swapchain(XrSession session, u32 width, u32 height, XrSwapchain& swapchain)
	{
		u32 format_count = 0;
		if (XR_FAILED(xrEnumerateSwapchainFormats(session, 0, &format_count, nullptr)) || format_count == 0)
		{
			openxr_log.error("xrEnumerateSwapchainFormats failed");
			return nullptr;
		}

		std::vector<s64> formats(format_count);
		if (XR_FAILED(xrEnumerateSwapchainFormats(session, format_count, &format_count, formats.data())))
		{
			openxr_log.error("xrEnumerateSwapchainFormats failed");
			return nullptr;
		}

		const auto format = std::find_if(swapchain_formats.cbegin(), swapchain_formats.cend(), [&formats](const swapchain_format_pair& pair)
		{
			return std::find(formats.cbegin(), formats.cend(), static_cast<s64>(pair.swapchain)) != formats.cend();
		});

		if (format == swapchain_formats.cend())
		{
			openxr_log.error("The OpenXR runtime does not support any of our swapchain formats");
			return nullptr;
		}

		XrSwapchainCreateInfo info{XR_TYPE_SWAPCHAIN_CREATE_INFO};
		info.usageFlags = XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
		info.format = static_cast<s64>(format->swapchain);
		info.sampleCount = 1;
		info.width = width;
		info.height = height;
		info.faceCount = 1;
		info.arraySize = 1;
		info.mipCount = 1;

		if (XR_FAILED(xrCreateSwapchain(session, &info, &swapchain)))
		{
			openxr_log.error("xrCreateSwapchain failed");
			return nullptr;
		}

		return &*format;
	}
}

bool renderer::get_device_extensions(VkPhysicalDevice physical_device, u32 vk_api_version, std::vector<const char*>& extensions)
{
	// We share images with RSX, which requires exportable memory (core in Vulkan 1.1 except for the platform handles)
	if (vk_api_version < VK_API_VERSION_1_1)
	{
		openxr_log.notice("Headset rendering is not available: it requires Vulkan 1.1");
		return false;
	}

	u32 extension_count = 0;
	vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr);

	std::vector<VkExtensionProperties> supported_extensions(extension_count);
	vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, supported_extensions.data());

	const auto is_supported = [&supported_extensions](const char* name)
	{
		return std::any_of(supported_extensions.cbegin(), supported_extensions.cend(), [name](const VkExtensionProperties& ext)
		{
			return std::strcmp(ext.extensionName, name) == 0;
		});
	};

	if (!is_supported(shared_memory_extension_name))
	{
		openxr_log.notice("Headset rendering is not available: it requires the Vulkan extension '%s'", shared_memory_extension_name);
		return false;
	}

	extensions.push_back(shared_memory_extension_name);

	// Only needed by the OpenGL backend of RSX
	if (is_supported(shared_semaphore_extension_name))
	{
		extensions.push_back(shared_semaphore_extension_name);
	}
	else
	{
		openxr_log.notice("Headset rendering with OpenGL is not available: it requires the Vulkan extension '%s'", shared_semaphore_extension_name);
	}

	return true;
}

u32 renderer::render(XrTime display_time, bool should_render, std::array<const XrCompositionLayerBaseHeader*, 2>& layers)
{
	if (!m_rendering_active || !should_render)
	{
		return 0;
	}

	copy_latest_frame();
	return build_layers(display_time, layers);
}

bool renderer::init(const context& ctx)
{
	m_ctx = ctx;

	vkGetDeviceQueue(m_ctx.vk_device, m_ctx.vk_queue_family, m_ctx.vk_queue_index, &m_vk_queue);

	VkCommandPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_info.queueFamilyIndex = m_ctx.vk_queue_family;

	if (vkCreateCommandPool(m_ctx.vk_device, &pool_info, nullptr, &m_vk_command_pool) != VK_SUCCESS)
	{
		openxr_log.error("vkCreateCommandPool failed");
		return false;
	}

	VkCommandBufferAllocateInfo cb_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
	cb_info.commandPool = m_vk_command_pool;
	cb_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cb_info.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(m_ctx.vk_device, &cb_info, &m_vk_command_buffer) != VK_SUCCESS)
	{
		openxr_log.error("vkAllocateCommandBuffers failed");
		return false;
	}

	VkFenceCreateInfo fence_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};

	if (vkCreateFence(m_ctx.vk_device, &fence_info, nullptr, &m_vk_fence) != VK_SUCCESS)
	{
		openxr_log.error("vkCreateFence failed");
		return false;
	}

	if (!create_swapchain())
	{
		openxr_log.error("Failed to create swapchain");
		return false;
	}

	if (!create_shared_images())
	{
		openxr_log.error("Failed to create shared images");
		return false;
	}

	m_rendering_active = true;
	openxr_log.notice("Headset rendering enabled (mode=%s)", get_render_mode());
	return true;
}

void renderer::destroy()
{
	// Stop RSX from using the shared images.
	// RSX keeps its own reference to the memory until it notices the change, so it's safe to free ours.
	{
		auto& shared = openxr::g_shared_frames;
		std::lock_guard lock(shared.mutex);

		if (shared.active)
		{
			shared.active = false;
			shared.generation++;
		}
	}

	m_rendering_active = false;
	m_has_swapchain_image = false;

	if (m_ctx.vk_device)
	{
		vkDeviceWaitIdle(m_ctx.vk_device);

		const auto close_handle = [](u64 handle)
		{
			if (handle == invalid_handle) return;
#ifdef _WIN32
			CloseHandle(reinterpret_cast<HANDLE>(handle));
#else
			close(static_cast<int>(handle));
#endif
		};

		for (shared_image& img : m_shared_images)
		{
			if (img.image) vkDestroyImage(m_ctx.vk_device, img.image, nullptr);
			if (img.memory) vkFreeMemory(m_ctx.vk_device, img.memory, nullptr);
			if (img.semaphore) vkDestroySemaphore(m_ctx.vk_device, img.semaphore, nullptr);

			close_handle(img.handle);
			close_handle(img.semaphore_handle);
		}

		if (m_vk_fence) vkDestroyFence(m_ctx.vk_device, m_vk_fence, nullptr);
		if (m_vk_command_pool) vkDestroyCommandPool(m_ctx.vk_device, m_vk_command_pool, nullptr); // Also frees the command buffer
	}

	m_shared_images.clear();
	m_vk_fence = VK_NULL_HANDLE;
	m_vk_command_pool = VK_NULL_HANDLE;
	m_vk_command_buffer = VK_NULL_HANDLE;
	m_vk_queue = VK_NULL_HANDLE;

	if (m_swapchain)
	{
		xrDestroySwapchain(m_swapchain);
		m_swapchain = XR_NULL_HANDLE;
	}

	m_swapchain_images.clear();

	// The session and the device are owned by the session thread and destroyed after this
	m_ctx = {};
}

bool renderer::create_swapchain()
{
	const swapchain_format_pair* selected = create_color_swapchain(m_ctx.session, openxr::shared_frames::image_width, openxr::shared_frames::image_height, m_swapchain);
	if (!selected)
	{
		return false;
	}

	if (selected->swapchain == selected->shared)
	{
		openxr_log.warning("The OpenXR runtime does not support sRGB swapchains. Colors will look too bright.");
	}

	m_shared_format = selected->shared;

	u32 image_count = 0;
	if (XR_FAILED(xrEnumerateSwapchainImages(m_swapchain, 0, &image_count, nullptr)) || image_count == 0)
	{
		openxr_log.error("xrEnumerateSwapchainImages failed");
		return false;
	}

	std::vector<XrSwapchainImageVulkan2KHR> images(image_count, {XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR});
	if (XR_FAILED(xrEnumerateSwapchainImages(m_swapchain, image_count, &image_count, reinterpret_cast<XrSwapchainImageBaseHeader*>(images.data()))))
	{
		openxr_log.error("xrEnumerateSwapchainImages failed");
		return false;
	}

	m_swapchain_images.clear();
	for (const XrSwapchainImageVulkan2KHR& image : images)
	{
		m_swapchain_images.push_back(image.image);
	}

	openxr_log.notice("Created swapchain with %d images (format=%d)", image_count, static_cast<s32>(selected->swapchain));
	return true;
}

bool renderer::create_shared_images()
{
	// RSX has to use the same GPU to be able to import the images
	VkPhysicalDeviceIDProperties id_properties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES};
	VkPhysicalDeviceProperties2 properties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
	properties.pNext = &id_properties;
	vkGetPhysicalDeviceProperties2(m_ctx.vk_physical, &properties);

#ifdef _WIN32
	const auto get_memory_handle = reinterpret_cast<PFN_vkGetMemoryWin32HandleKHR>(vkGetDeviceProcAddr(m_ctx.vk_device, "vkGetMemoryWin32HandleKHR"));
#else
	const auto get_memory_handle = reinterpret_cast<PFN_vkGetMemoryFdKHR>(vkGetDeviceProcAddr(m_ctx.vk_device, "vkGetMemoryFdKHR"));
#endif

	if (!get_memory_handle)
	{
		openxr_log.error("Failed to get the Vulkan memory export function");
		return false;
	}

	// Optional. Only available if the semaphore extension was enabled (see get_device_extensions).
#ifdef _WIN32
	const auto get_semaphore_handle = reinterpret_cast<PFN_vkGetSemaphoreWin32HandleKHR>(vkGetDeviceProcAddr(m_ctx.vk_device, "vkGetSemaphoreWin32HandleKHR"));
#else
	const auto get_semaphore_handle = reinterpret_cast<PFN_vkGetSemaphoreFdKHR>(vkGetDeviceProcAddr(m_ctx.vk_device, "vkGetSemaphoreFdKHR"));
#endif

	// Returns a HANDLE on Windows and a file descriptor on other platforms, or invalid_handle on failure
	const auto export_handle = [this](auto get_handle, const auto& handle_info) -> u64
	{
#ifdef _WIN32
		HANDLE handle = nullptr;
		if (get_handle(m_ctx.vk_device, &handle_info, &handle) != VK_SUCCESS || !handle)
		{
			return invalid_handle;
		}

		return reinterpret_cast<u64>(handle);
#else
		int fd = -1;
		if (get_handle(m_ctx.vk_device, &handle_info, &fd) != VK_SUCCESS || fd < 0)
		{
			return invalid_handle;
		}

		return static_cast<u64>(fd);
#endif
	};

	const auto create_semaphore = [&](shared_image& img) -> bool
	{
		VkExportSemaphoreCreateInfo export_info{VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO};
		export_info.handleTypes = openxr::shared_semaphore_handle_type;

		VkSemaphoreCreateInfo info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
		info.pNext = &export_info;

		if (vkCreateSemaphore(m_ctx.vk_device, &info, nullptr, &img.semaphore) != VK_SUCCESS)
		{
			openxr_log.error("vkCreateSemaphore failed");
			return false;
		}

#ifdef _WIN32
		VkSemaphoreGetWin32HandleInfoKHR handle_info{VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR};
#else
		VkSemaphoreGetFdInfoKHR handle_info{VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR};
#endif
		handle_info.semaphore = img.semaphore;
		handle_info.handleType = openxr::shared_semaphore_handle_type;

		img.semaphore_handle = export_handle(get_semaphore_handle, handle_info);
		if (img.semaphore_handle == invalid_handle)
		{
			openxr_log.error("Failed to export the semaphore");
			return false;
		}

		return true;
	};

	bool has_semaphores = !!get_semaphore_handle;

	VkPhysicalDeviceMemoryProperties memory_properties{};
	vkGetPhysicalDeviceMemoryProperties(m_ctx.vk_physical, &memory_properties);

	VkExternalMemoryImageCreateInfo external_info{VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO};
	external_info.handleTypes = openxr::shared_memory_handle_type;
	const VkImageCreateInfo image_info = openxr::get_shared_image_info(m_shared_format, &external_info);

	u32 memory_type_index = umax;
	u64 allocation_size = 0;

	m_shared_images.resize(openxr::shared_frames::slot_count);

	for (shared_image& img : m_shared_images)
	{
		if (has_semaphores && !create_semaphore(img))
		{
			// The Vulkan backend of RSX still works without them
			openxr_log.error("Failed to create the shared semaphores. Headset rendering with OpenGL will not be available.");
			has_semaphores = false;
		}

		if (vkCreateImage(m_ctx.vk_device, &image_info, nullptr, &img.image) != VK_SUCCESS)
		{
			openxr_log.error("vkCreateImage failed");
			return false;
		}

		VkMemoryRequirements requirements{};
		vkGetImageMemoryRequirements(m_ctx.vk_device, img.image, &requirements);

		if (memory_type_index == umax)
		{
			for (u32 i = 0; i < memory_properties.memoryTypeCount; i++)
			{
				if ((requirements.memoryTypeBits & (1u << i)) && (memory_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
				{
					memory_type_index = i;
					break;
				}
			}

			if (memory_type_index == umax)
			{
				openxr_log.error("No device local memory type found for the shared images");
				return false;
			}
		}

		// Dedicated allocations are required by some drivers for exported images
		VkMemoryDedicatedAllocateInfo dedicated_info{VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};
		dedicated_info.image = img.image;

		VkExportMemoryAllocateInfo export_info{VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO};
		export_info.pNext = &dedicated_info;
		export_info.handleTypes = openxr::shared_memory_handle_type;

		VkMemoryAllocateInfo alloc_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
		alloc_info.pNext = &export_info;
		alloc_info.allocationSize = requirements.size;
		alloc_info.memoryTypeIndex = memory_type_index;

		if (vkAllocateMemory(m_ctx.vk_device, &alloc_info, nullptr, &img.memory) != VK_SUCCESS)
		{
			openxr_log.error("vkAllocateMemory failed");
			return false;
		}

		if (vkBindImageMemory(m_ctx.vk_device, img.image, img.memory, 0) != VK_SUCCESS)
		{
			openxr_log.error("vkBindImageMemory failed");
			return false;
		}

		allocation_size = requirements.size;

#ifdef _WIN32
		VkMemoryGetWin32HandleInfoKHR handle_info{VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR};
#else
		VkMemoryGetFdInfoKHR handle_info{VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR};
#endif
		handle_info.memory = img.memory;
		handle_info.handleType = openxr::shared_memory_handle_type;

		img.handle = export_handle(get_memory_handle, handle_info);
		if (img.handle == invalid_handle)
		{
			openxr_log.error("Failed to export the image memory");
			return false;
		}
	}

	// Publish the images to RSX
	auto& shared = openxr::g_shared_frames;
	std::lock_guard lock(shared.mutex);

	std::memcpy(shared.device_uuid.data(), id_properties.deviceUUID, VK_UUID_SIZE);
	shared.format = static_cast<u32>(m_shared_format);
	shared.memory_type_index = memory_type_index;
	shared.allocation_size = allocation_size;
	shared.has_semaphores = has_semaphores;

	for (u32 i = 0; i < openxr::shared_frames::slot_count; i++)
	{
		shared.slots[i].memory_handle = m_shared_images[i].handle;
		shared.slots[i].semaphore_handle = has_semaphores ? m_shared_images[i].semaphore_handle : invalid_handle;
		shared.slots[i].state = openxr::slot_state::free;
		shared.slots[i].frame_number = 0;
		shared.slots[i].semaphore_signaled = false;
	}

	shared.generation++;
	shared.active = true;
	return true;
}

void renderer::copy_latest_frame()
{
	auto& shared = openxr::g_shared_frames;

	// Take all finished frames. We only show the newest one, but we still have to wait once
	// for the semaphores of the others, if the OpenGL backend signaled them.
	std::array<bool, openxr::shared_frames::slot_count> taken{};
	s32 newest = -1;
	u64 newest_number = 0;

	for (u32 i = 0; i < openxr::shared_frames::slot_count; i++)
	{
		openxr::shared_frame_slot& slot = shared.slots[i];

		if (!slot.state.compare_and_swap_test(openxr::slot_state::ready, openxr::slot_state::reading))
		{
			continue;
		}

		taken[i] = true;

		if (newest < 0 || slot.frame_number > newest_number)
		{
			newest = static_cast<s32>(i);
			newest_number = slot.frame_number;
		}
	}

	if (newest < 0)
	{
		// No new frame. The runtime keeps showing the last released swapchain image.
		return;
	}

	std::array<VkSemaphore, openxr::shared_frames::slot_count> wait_semaphores{};
	std::array<VkPipelineStageFlags, openxr::shared_frames::slot_count> wait_stages{};
	u32 wait_count = 0;

	for (u32 i = 0; i < openxr::shared_frames::slot_count; i++)
	{
		if (taken[i] && shared.slots[i].semaphore_signaled && m_shared_images[i].semaphore)
		{
			wait_semaphores[wait_count] = m_shared_images[i].semaphore;
			wait_stages[wait_count] = VK_PIPELINE_STAGE_TRANSFER_BIT;
			wait_count++;
		}
	}

	// Get a swapchain image for the newest frame
	u32 image_index = 0;
	bool acquired = false;

	XrSwapchainImageAcquireInfo acquire_info{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};

	if (XR_FAILED(xrAcquireSwapchainImage(m_swapchain, &acquire_info, &image_index)) || image_index >= m_swapchain_images.size())
	{
		openxr_log.error("xrAcquireSwapchainImage failed");
	}
	else
	{
		XrSwapchainImageWaitInfo wait_info{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
		wait_info.timeout = XR_INFINITE_DURATION;

		if (XR_FAILED(xrWaitSwapchainImage(m_swapchain, &wait_info)))
		{
			openxr_log.error("xrWaitSwapchainImage failed");

			XrSwapchainImageReleaseInfo release_info{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
			xrReleaseSwapchainImage(m_swapchain, &release_info);
		}
		else
		{
			acquired = true;
		}
	}

	vkResetCommandBuffer(m_vk_command_buffer, 0);

	VkCommandBufferBeginInfo begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(m_vk_command_buffer, &begin_info);

	// Without a swapchain image we still submit, so that we wait for the semaphores
	if (acquired)
	{
		const VkImage src_image = m_shared_images[newest].image;
		const VkImage dst_image = m_swapchain_images[image_index];
		const VkImageSubresourceRange range{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

		std::array<VkImageMemoryBarrier, 2> barriers{};

		// Take ownership of the shared image from RSX. The old layout has to match the one RSX released it in.
		// The OpenGL backend (the only one that signals the semaphore) hands the image over in GL_LAYOUT_TRANSFER_SRC_EXT.
		barriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barriers[0].srcAccessMask = 0;
		barriers[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barriers[0].oldLayout = shared.slots[newest].semaphore_signaled ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL : openxr::shared_image_written_layout;
		barriers[0].newLayout = openxr::shared_image_released_layout;
		barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL;
		barriers[0].dstQueueFamilyIndex = m_ctx.vk_queue_family;
		barriers[0].image = src_image;
		barriers[0].subresourceRange = range;

		// The runtime hands out swapchain images in the color attachment layout
		barriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barriers[1].srcAccessMask = 0;
		barriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barriers[1].oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		barriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barriers[1].image = dst_image;
		barriers[1].subresourceRange = range;

		vkCmdPipelineBarrier(m_vk_command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, static_cast<u32>(barriers.size()), barriers.data());

		// The formats only differ in their color space, so this is a bitwise copy
		VkImageCopy region{};
		region.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
		region.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
		region.extent = {openxr::shared_frames::image_width, openxr::shared_frames::image_height, 1};

		vkCmdCopyImage(m_vk_command_buffer, src_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		// Return the swapchain image in the layout that the runtime expects
		VkImageMemoryBarrier present_barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
		present_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		present_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		present_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		present_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		present_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		present_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		present_barrier.image = dst_image;
		present_barrier.subresourceRange = range;

		vkCmdPipelineBarrier(m_vk_command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &present_barrier);
	}

	vkEndCommandBuffer(m_vk_command_buffer);

	VkSubmitInfo submit_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
	submit_info.waitSemaphoreCount = wait_count;
	submit_info.pWaitSemaphores = wait_semaphores.data();
	submit_info.pWaitDstStageMask = wait_stages.data();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &m_vk_command_buffer;

	const bool submitted = vkQueueSubmit(m_vk_queue, 1, &submit_info, m_vk_fence) == VK_SUCCESS;
	if (!submitted)
	{
		openxr_log.error("vkQueueSubmit failed");
	}

	// The image has to be released even if nothing was submitted
	bool released = false;
	if (acquired)
	{
		XrSwapchainImageReleaseInfo release_info{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
		released = XR_SUCCEEDED(xrReleaseSwapchainImage(m_swapchain, &release_info));
		if (!released)
		{
			openxr_log.error("xrReleaseSwapchainImage failed");
		}
	}

	// Wait for the copy, so that RSX can reuse the slots. The copy is tiny compared to a frame.
	// A failed submission leaves the fence unsignaled, so it can be used again.
	if (submitted)
	{
		vkWaitForFences(m_ctx.vk_device, 1, &m_vk_fence, VK_TRUE, umax);
		vkResetFences(m_ctx.vk_device, 1, &m_vk_fence);
	}

	for (u32 i = 0; i < openxr::shared_frames::slot_count; i++)
	{
		if (taken[i])
		{
			shared.slots[i].semaphore_signaled = false;
			shared.slots[i].state = openxr::slot_state::free;
		}
	}

	if (submitted && released)
	{
		if (!m_has_swapchain_image)
		{
			openxr_log.notice("Received the first frame from RSX");
		}

		m_has_swapchain_image = true;
	}
}

u32 renderer::build_layers(XrTime display_time, std::array<const XrCompositionLayerBaseHeader*, 2>& layers)
{
	if (!m_has_swapchain_image)
	{
		return 0;
	}

	const auto get_eye_rect = [](u32 eye)
	{
		const areai area = openxr::get_shared_eye_area(eye);

		XrRect2Di rect{};
		rect.offset = { area.x1, area.y1 };
		rect.extent = { area.width(), area.height() };
		return rect;
	};

	const openxr_render_mode mode = get_render_mode();

	switch (mode)
	{
	case openxr_render_mode::virtual_screen:
	case openxr_render_mode::virtual_screen_stereo:
	{
		// A quad in front of the initial head pose. In stereo mode each eye gets its own quad with its own half of the image.
		const bool stereo = mode == openxr_render_mode::virtual_screen_stereo;
		const u32 count = stereo ? 2 : 1;

		for (u32 i = 0; i < count; i++)
		{
			XrCompositionLayerQuad& quad = m_quad_layers[i];
			quad = {XR_TYPE_COMPOSITION_LAYER_QUAD};
			quad.space = m_ctx.reference_space;
			quad.eyeVisibility = stereo ? (i == 0 ? XR_EYE_VISIBILITY_LEFT : XR_EYE_VISIBILITY_RIGHT) : XR_EYE_VISIBILITY_BOTH;
			quad.subImage.swapchain = m_swapchain;
			quad.subImage.imageRect = get_eye_rect(i);
			quad.subImage.imageArrayIndex = 0;
			quad.pose.orientation.w = 1.0f;
			quad.pose.position.z = -virtual_screen_distance;
			quad.size = { virtual_screen_width, virtual_screen_width * openxr::shared_frames::eye_height / openxr::shared_frames::eye_width };

			layers[i] = reinterpret_cast<const XrCompositionLayerBaseHeader*>(&quad);
		}

		return count;
	}
	case openxr_render_mode::full_view:
	{
		// Each eye's image covers the whole view of that eye
		XrViewLocateInfo locate_info{XR_TYPE_VIEW_LOCATE_INFO};
		locate_info.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
		locate_info.displayTime = display_time;
		locate_info.space = m_ctx.reference_space;

		XrViewState view_state{XR_TYPE_VIEW_STATE};
		std::array<XrView, 2> views{};
		views[0].type = XR_TYPE_VIEW;
		views[1].type = XR_TYPE_VIEW;
		u32 view_count = 0;

		if (XR_FAILED(xrLocateViews(m_ctx.session, &locate_info, &view_state, static_cast<u32>(views.size()), &view_count, views.data())) || view_count != views.size())
		{
			openxr_log.error("xrLocateViews failed");
			return 0;
		}

		if (!(view_state.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT))
		{
			return 0;
		}

		for (u32 i = 0; i < view_count; i++)
		{
			XrCompositionLayerProjectionView& view = m_projection_views[i];
			view = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
			view.pose = views[i].pose;
			view.fov = views[i].fov;
			view.subImage.swapchain = m_swapchain;
			view.subImage.imageRect = get_eye_rect(i);
			view.subImage.imageArrayIndex = 0;
		}

		m_projection_layer = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
		m_projection_layer.space = m_ctx.reference_space;
		m_projection_layer.viewCount = view_count;
		m_projection_layer.views = m_projection_views.data();

		layers[0] = reinterpret_cast<const XrCompositionLayerBaseHeader*>(&m_projection_layer);
		return 1;
	}
	case openxr_render_mode::disabled:
		break;
	}

	return 0;
}

bool placeholder_layer::init(XrSession session, XrSpace reference_space)
{
	if (!create_color_swapchain(session, image_size, image_size, m_swapchain))
	{
		return false;
	}

	// A layer can only use a swapchain after one of its images was released.
	// We don't draw anything into it. The quad is too small and too far away to be visible.
	u32 image_index = 0;
	XrSwapchainImageAcquireInfo acquire_info{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
	XrSwapchainImageWaitInfo wait_info{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
	wait_info.timeout = XR_INFINITE_DURATION;
	XrSwapchainImageReleaseInfo release_info{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};

	if (XR_FAILED(xrAcquireSwapchainImage(m_swapchain, &acquire_info, &image_index)) ||
		XR_FAILED(xrWaitSwapchainImage(m_swapchain, &wait_info)) ||
		XR_FAILED(xrReleaseSwapchainImage(m_swapchain, &release_info)))
	{
		openxr_log.error("Failed to release the placeholder swapchain image");
		destroy();
		return false;
	}

	m_layer = {XR_TYPE_COMPOSITION_LAYER_QUAD};
	m_layer.space = reference_space;
	m_layer.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
	m_layer.subImage.swapchain = m_swapchain;
	m_layer.subImage.imageRect = { { 0, 0 }, { static_cast<s32>(image_size), static_cast<s32>(image_size) } };
	m_layer.pose.orientation.w = 1.0f;
	m_layer.pose.position.z = -10.0f;
	m_layer.size = { 0.001f, 0.001f };
	return true;
}

void placeholder_layer::destroy()
{
	if (m_swapchain)
	{
		xrDestroySwapchain(m_swapchain);
		m_swapchain = XR_NULL_HANDLE;
	}

	m_layer = {};
}

}
