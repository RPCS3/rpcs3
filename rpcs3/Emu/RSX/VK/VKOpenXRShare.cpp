#include "stdafx.h"
#include "VKOpenXRShare.h"
#include "VKFramebuffer.h"
#include "VKHelpers.h"
#include "VKRenderPass.h"
#include "VKResourceManager.h"
#include "vkutils/barriers.h"
#include "vkutils/commands.h"
#include "vkutils/device.h"
#include "vkutils/image.h"
#include "vkutils/memory.h"
#include "Emu/Io/openxr/openxr_frame_share_vk.h"

#include <cstring>

#ifndef _WIN32
#include <unistd.h>
#endif

namespace vk
{
	void openxr_frame_share::create(const render_device* dev)
	{
		m_device = dev;
	}

	void openxr_frame_share::destroy()
	{
		// The caller has to make sure that the device is idle
		if (m_current)
		{
			destroy_set(*m_current);
			m_current.reset();
		}

		for (auto& set : m_retired)
		{
			destroy_set(*set);
		}

		m_retired.clear();

		m_overlay_images = {};

		m_device = nullptr;
	}

	image* openxr_frame_share::add_overlays(const command_buffer& cmd, image* src, u32 index, std::initializer_list<areai> areas, const overlay_callback& draw_overlays)
	{
		const VkRenderPass pass = vk::get_renderpass(*m_device, vk::get_renderpass_key(src->format()));
		ensure(pass != VK_NULL_HANDLE);

		return copy_image_with_overlays(*m_device, cmd, ::at32(m_overlay_images, index), src, areai(0, 0, src->width(), src->height()), pass, [&](framebuffer_holder* fbo)
		{
			for (const areai& area : areas)
			{
				draw_overlays(fbo, pass, areau(area));
			}
		});
	}

	void openxr_frame_share::destroy_set(import_set& set)
	{
		for (imported_slot& slot : set.slots)
		{
			if (slot.image) vkDestroyImage(*m_device, slot.image, nullptr);
			if (slot.memory) vkFreeMemory(*m_device, slot.memory, nullptr);
		}

		set.slots.clear();
	}

	bool openxr_frame_share::update_imports()
	{
		auto& shared = openxr::g_shared_frames;
		std::lock_guard lock(shared.mutex);

		if (shared.generation != m_generation)
		{
			// The OpenXR handler recreated or destroyed its images
			if (m_current)
			{
				if (m_current->pending_frames == 0)
				{
					destroy_set(*m_current);
				}
				else
				{
					m_retired.push_back(std::move(m_current));
				}

				m_current.reset();
			}

			m_generation = shared.generation;
			m_import_failed = false;
		}

		if (!shared.active || m_import_failed)
		{
			return false;
		}

		if (m_current)
		{
			return true;
		}

		auto set = std::make_unique<import_set>();
		set->generation = shared.generation;

		if (!import_images(*set))
		{
			destroy_set(*set);
			m_import_failed = true;
			return false;
		}

		rsx_log.notice("Imported %d OpenXR frame images", set->slots.size());
		m_current = std::move(set);
		return true;
	}

	bool openxr_frame_share::import_images(import_set& set)
	{
		// The shared mutex is held by the caller
		const auto& shared = openxr::g_shared_frames;

		if (!m_device->get_external_memory_import_support())
		{
			rsx_log.error("OpenXR headset rendering requires the Vulkan extension '%s'", openxr::shared_memory_extension_name);
			return false;
		}

		// Both devices have to use the same GPU
		VkPhysicalDeviceIDProperties id_properties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES};
		VkPhysicalDeviceProperties2 properties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
		properties.pNext = &id_properties;
		vkGetPhysicalDeviceProperties2(m_device->gpu(), &properties);

		if (std::memcmp(id_properties.deviceUUID, shared.device_uuid.data(), VK_UUID_SIZE) != 0)
		{
			rsx_log.error("OpenXR headset rendering requires RSX to use the same GPU as the OpenXR runtime");
			return false;
		}

		VkExternalMemoryImageCreateInfo external_info{VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO};
		external_info.handleTypes = openxr::shared_memory_handle_type;
		const VkImageCreateInfo image_info = openxr::get_shared_image_info(static_cast<VkFormat>(shared.format), &external_info);

		set.slots.resize(openxr::shared_frames::slot_count);

		for (u32 i = 0; i < openxr::shared_frames::slot_count; i++)
		{
			imported_slot& slot = set.slots[i];

			if (vkCreateImage(*m_device, &image_info, nullptr, &slot.image) != VK_SUCCESS)
			{
				rsx_log.error("OpenXR: vkCreateImage failed");
				return false;
			}

			// The exporter used a dedicated allocation, so we have to as well
			VkMemoryDedicatedAllocateInfo dedicated_info{VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};
			dedicated_info.image = slot.image;

#ifdef _WIN32
			// Importing a Win32 handle does not take ownership of it
			VkImportMemoryWin32HandleInfoKHR import_info{VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR};
			import_info.pNext = &dedicated_info;
			import_info.handleType = openxr::shared_memory_handle_type;
			import_info.handle = reinterpret_cast<HANDLE>(shared.slots[i].memory_handle);
#else
			// Importing a file descriptor takes ownership of it, so we import a duplicate
			const int fd = dup(static_cast<int>(shared.slots[i].memory_handle));
			if (fd < 0)
			{
				rsx_log.error("OpenXR: Failed to duplicate the memory file descriptor");
				return false;
			}

			VkImportMemoryFdInfoKHR import_info{VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR};
			import_info.pNext = &dedicated_info;
			import_info.handleType = openxr::shared_memory_handle_type;
			import_info.fd = fd;
#endif

			// Opaque handles have to be imported with the same size and memory type that were used for the export
			VkMemoryAllocateInfo alloc_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
			alloc_info.pNext = &import_info;
			alloc_info.allocationSize = shared.allocation_size;
			alloc_info.memoryTypeIndex = shared.memory_type_index;

			if (vkAllocateMemory(*m_device, &alloc_info, nullptr, &slot.memory) != VK_SUCCESS)
			{
				rsx_log.error("OpenXR: Failed to import the shared memory");
#ifndef _WIN32
				close(fd);
#endif
				return false;
			}

			if (vkBindImageMemory(*m_device, slot.image, slot.memory, 0) != VK_SUCCESS)
			{
				rsx_log.error("OpenXR: vkBindImageMemory failed");
				return false;
			}
		}

		return true;
	}

	openxr_frame_share::frame_token openxr_frame_share::present(const command_buffer& cmd, image* left_image, image* right_image, const openxr::eye_layout& eyes, const overlay_callback& draw_overlays)
	{
		if (!update_imports())
		{
			return {};
		}

		// Skip this frame if the headset is still busy with all slots.
		// We don't use the semaphore (see openxr_frame_share.h).
		const s32 slot_index = openxr::g_shared_frames.acquire_slot(m_current->generation, false);
		if (slot_index < 0)
		{
			return {};
		}

		const auto overlays = [&](image* src, u32 index, std::initializer_list<areai> areas)
		{
			return add_overlays(cmd, src, index, areas, draw_overlays);
		};

		image* left = nullptr;
		image* right = nullptr;

		if (!openxr::get_eye_images(left_image, right_image, eyes, !!draw_overlays, overlays, left, right))
		{
			openxr::g_shared_frames.publish_slot(slot_index, m_current->generation, false);
			return {};
		}

		const VkImage dst_image = m_current->slots[slot_index].image;
		const VkImageSubresourceRange range{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

		// The old contents are not needed, so we don't need an ownership transfer from OpenXR
		vk::insert_image_memory_barrier(cmd, dst_image, VK_IMAGE_LAYOUT_UNDEFINED, openxr::shared_image_written_layout,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, VK_ACCESS_TRANSFER_WRITE_BIT, range);

		// Scale each eye into its half of the shared image
		const auto blit_eye = [&](image* src, const areai& src_rect, u32 eye)
		{
			const areai dst_rect = openxr::get_shared_eye_area(eye);

			src->push_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

			VkImageBlit region{};
			region.srcSubresource = {src->aspect(), 0, 0, 1};
			region.srcOffsets[0] = {src_rect.x1, src_rect.y1, 0};
			region.srcOffsets[1] = {src_rect.x2, src_rect.y2, 1};
			region.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
			region.dstOffsets[0] = {dst_rect.x1, dst_rect.y1, 0};
			region.dstOffsets[1] = {dst_rect.x2, dst_rect.y2, 1};

			vkCmdBlitImage(cmd, src->value, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst_image, openxr::shared_image_written_layout, 1, &region, VK_FILTER_LINEAR);

			src->pop_layout(cmd);
		};

		blit_eye(left, eyes.left, 0);
		blit_eye(right, eyes.right, 1);

		// Release the image to the OpenXR device. The layouts have to match its acquire barrier.
		VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = 0;
		barrier.oldLayout = openxr::shared_image_written_layout;
		barrier.newLayout = openxr::shared_image_released_layout;
		barrier.srcQueueFamilyIndex = m_device->get_graphics_queue_family();
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL;
		barrier.image = dst_image;
		barrier.subresourceRange = range;

		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		m_current->pending_frames++;
		return { slot_index, m_current->generation };
	}

	void openxr_frame_share::on_frame_finished(const frame_token& token, bool success)
	{
		if (token.slot < 0)
		{
			return;
		}

		// Release our reference to the images
		if (m_current && m_current->generation == token.generation)
		{
			ensure(m_current->pending_frames > 0);
			m_current->pending_frames--;
		}
		else
		{
			for (auto it = m_retired.begin(); it != m_retired.end(); ++it)
			{
				import_set& set = **it;
				if (set.generation != token.generation) continue;

				ensure(set.pending_frames > 0);
				if (--set.pending_frames == 0)
				{
					destroy_set(set);
					m_retired.erase(it);
				}
				break;
			}
		}

		// Hand the frame to the OpenXR handler, unless it recreated its images in the meantime
		openxr::g_shared_frames.publish_slot(token.slot, token.generation, success);
	}
}
