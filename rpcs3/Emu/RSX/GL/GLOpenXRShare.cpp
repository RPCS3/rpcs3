#include "stdafx.h"
#include "GLOpenXRShare.h"
#include "GLHelpers.h"
#include "glutils/blitter.h"
#include "glutils/capabilities.h"
#include "glutils/image.h"
#include "glutils/state_tracker.hpp"

#include <cstring>

#ifndef _WIN32
#include <unistd.h>
#endif

namespace gl
{
	void openxr_frame_share::destroy()
	{
		destroy_slots();
		m_blit_src.remove();
		m_blit_dst.remove();

		m_overlay_textures = {};
	}

	gl::texture* openxr_frame_share::add_overlays(gl::command_context& cmd, gl::texture* src, u32 index, std::initializer_list<areai> areas, const overlay_callback& draw_overlays)
	{
		return copy_texture_with_overlays(cmd, ::at32(m_overlay_textures, index), src, [&](gl::texture* dst)
		{
			for (const areai& area : areas)
			{
				draw_overlays(dst, areau(area));
			}
		});
	}

	void openxr_frame_share::destroy_slots()
	{
		// OpenGL keeps the objects alive until pending commands that use them are finished
		for (imported_slot& slot : m_slots)
		{
			if (slot.fence) glDeleteSync(slot.fence);
			if (slot.texture) glDeleteTextures(1, &slot.texture);
			if (slot.memory) glDeleteMemoryObjectsEXT(1, &slot.memory);
			if (slot.semaphore) glDeleteSemaphoresEXT(1, &slot.semaphore);
		}

		m_slots.clear();
	}

	bool openxr_frame_share::update_imports(gl::command_context& cmd)
	{
		auto& shared = openxr::g_shared_frames;
		std::lock_guard lock(shared.mutex);

		if (shared.generation != m_generation)
		{
			// The OpenXR handler recreated or destroyed its images. Copies in flight are dropped.
			destroy_slots();
			m_generation = shared.generation;
			m_import_failed = false;
		}

		if (!shared.active || m_import_failed)
		{
			return false;
		}

		if (!m_slots.empty())
		{
			return true;
		}

		if (!import_slots(cmd))
		{
			destroy_slots();
			m_import_failed = true;
			return false;
		}

		rsx_log.notice("Imported %d OpenXR frame images", m_slots.size());
		return true;
	}

	bool openxr_frame_share::import_slots(gl::command_context& cmd)
	{
		// The shared mutex is held by the caller
		const auto& shared = openxr::g_shared_frames;
		const auto& caps = gl::get_driver_caps();

#ifdef _WIN32
		const bool has_extensions = caps.EXT_memory_object_supported && caps.EXT_memory_object_win32_supported && caps.EXT_semaphore_supported && caps.EXT_semaphore_win32_supported;
		constexpr GLenum handle_type = GL_HANDLE_TYPE_OPAQUE_WIN32_EXT;
#else
		const bool has_extensions = caps.EXT_memory_object_supported && caps.EXT_memory_object_fd_supported && caps.EXT_semaphore_supported && caps.EXT_semaphore_fd_supported;
		constexpr GLenum handle_type = GL_HANDLE_TYPE_OPAQUE_FD_EXT;
#endif

		if (!has_extensions)
		{
			rsx_log.error("OpenXR headset rendering with OpenGL requires GL_EXT_memory_object and GL_EXT_semaphore");
			return false;
		}

		if (!shared.has_semaphores)
		{
			rsx_log.error("OpenXR headset rendering with OpenGL requires external semaphore support in the OpenXR runtime's Vulkan device");
			return false;
		}

		// OpenGL has no BGRA storage format
		if (shared.format != openxr::vk_format_r8g8b8a8_unorm)
		{
			rsx_log.error("OpenXR headset rendering with OpenGL requires an RGBA swapchain, but the OpenXR runtime uses VkFormat %d", shared.format);
			return false;
		}

		// Both devices have to use the same GPU
		std::array<u8, GL_UUID_SIZE_EXT> device_uuid{};
		static_assert(sizeof(device_uuid) == sizeof(shared.device_uuid));
		glGetUnsignedBytei_vEXT(GL_DEVICE_UUID_EXT, 0, device_uuid.data());

		if (std::memcmp(device_uuid.data(), shared.device_uuid.data(), device_uuid.size()) != 0)
		{
			rsx_log.error("OpenXR headset rendering requires RSX to use the same GPU as the OpenXR runtime");
			return false;
		}

		if (!m_blit_src.created()) m_blit_src.create();
		if (!m_blit_dst.created()) m_blit_dst.create();

		m_slots.resize(openxr::shared_frames::slot_count);

		// Clear older errors, so that we only check our own calls
		while (glGetError() != GL_NO_ERROR) {}

		for (u32 i = 0; i < openxr::shared_frames::slot_count; i++)
		{
			imported_slot& slot = m_slots[i];

			// The exporter used a dedicated allocation, so we have to as well
			glCreateMemoryObjectsEXT(1, &slot.memory);
			const GLint dedicated = GL_TRUE;
			glMemoryObjectParameterivEXT(slot.memory, GL_DEDICATED_MEMORY_OBJECT_EXT, &dedicated);

			glGenSemaphoresEXT(1, &slot.semaphore);

#ifdef _WIN32
			// Importing a Win32 handle does not take ownership of it
			glImportMemoryWin32HandleEXT(slot.memory, shared.allocation_size, handle_type, reinterpret_cast<void*>(shared.slots[i].memory_handle));
			glImportSemaphoreWin32HandleEXT(slot.semaphore, handle_type, reinterpret_cast<void*>(shared.slots[i].semaphore_handle));
#else
			// Importing a file descriptor takes ownership of it, so we import duplicates
			const int memory_fd = dup(static_cast<int>(shared.slots[i].memory_handle));
			const int semaphore_fd = dup(static_cast<int>(shared.slots[i].semaphore_handle));

			if (memory_fd < 0 || semaphore_fd < 0)
			{
				rsx_log.error("OpenXR: Failed to duplicate the shared file descriptors");
				if (memory_fd >= 0) close(memory_fd);
				if (semaphore_fd >= 0) close(semaphore_fd);
				return false;
			}

			glImportMemoryFdEXT(slot.memory, shared.allocation_size, handle_type, memory_fd);
			glImportSemaphoreFdEXT(slot.semaphore, handle_type, semaphore_fd);
#endif

			// The Vulkan image uses optimal tiling
			glGenTextures(1, &slot.texture);
			cmd->bind_texture(GL_TEMP_IMAGE_SLOT(0), GL_TEXTURE_2D, slot.texture, GL_TRUE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_TILING_EXT, GL_OPTIMAL_TILING_EXT);
			glTexStorageMem2DEXT(GL_TEXTURE_2D, 1, GL_RGBA8, openxr::shared_frames::image_width, openxr::shared_frames::image_height, slot.memory, 0);

			if (const GLenum err = glGetError(); err != GL_NO_ERROR)
			{
				rsx_log.error("OpenXR: Failed to import the shared images (error=0x%x)", err);
				return false;
			}
		}

		return true;
	}

	void openxr_frame_share::check_finished_copies()
	{
		for (u32 i = 0; i < m_slots.size(); i++)
		{
			imported_slot& slot = m_slots[i];
			if (!slot.fence)
			{
				continue;
			}

			// Don't wait. We check again on the next flip.
			const GLenum status = glClientWaitSync(slot.fence, 0, 0);
			if (status == GL_TIMEOUT_EXPIRED)
			{
				continue;
			}

			glDeleteSync(slot.fence);
			slot.fence = nullptr;

			if (status == GL_WAIT_FAILED)
			{
				// The semaphore may not have been signaled. We'll never use this slot again in this generation.
				rsx_log.error("OpenXR: glClientWaitSync failed");
				continue;
			}

			// Hand the frame to the OpenXR handler, unless it recreated its images in the meantime
			openxr::g_shared_frames.publish_slot(static_cast<s32>(i), m_generation, true);
		}
	}

	void openxr_frame_share::present(gl::command_context& cmd, gl::texture* left_image, gl::texture* right_image, const openxr::eye_layout& eyes, const overlay_callback& draw_overlays)
	{
		check_finished_copies();

		if (!update_imports(cmd))
		{
			return;
		}

		// Skip this frame if the headset is still busy with all slots.
		// OpenXR waits for the semaphore and acquires the image in GL_LAYOUT_TRANSFER_SRC_EXT.
		const s32 slot_index = openxr::g_shared_frames.acquire_slot(m_generation, true);
		if (slot_index < 0)
		{
			return;
		}

		const auto overlays = [&](gl::texture* src, u32 index, std::initializer_list<areai> areas)
		{
			return add_overlays(cmd, src, index, areas, draw_overlays);
		};

		gl::texture* left = nullptr;
		gl::texture* right = nullptr;

		if (!openxr::get_eye_images(left_image, right_image, eyes, !!draw_overlays, overlays, left, right))
		{
			openxr::g_shared_frames.publish_slot(slot_index, m_generation, false);
			return;
		}

		imported_slot& slot = m_slots[slot_index];

		cmd->disable(GL_SCISSOR_TEST);

		gl::fbo::attachment src_att{ m_blit_src, gl::fbo::attachment::type::color };
		gl::fbo::attachment dst_att{ m_blit_dst, gl::fbo::attachment::type::color };
		dst_att = slot.texture;

		// Scale each eye into its half of the shared image.
		// RSX images and the shared image both store the top row first, so no vertical flip is needed.
		const auto blit_eye = [&](gl::texture* src, const areai& src_rect, u32 eye)
		{
			src_att = *src;
			m_blit_src.blit(m_blit_dst, src_rect, openxr::get_shared_eye_area(eye), gl::buffers::color, gl::filter::linear);
		};

		blit_eye(left, eyes.left, 0);
		blit_eye(right, eyes.right, 1);

		// Release the attachments
		src_att = GL_NONE;
		dst_att = GL_NONE;

		// Hand the image over to Vulkan in a defined layout
		const GLenum layout = GL_LAYOUT_TRANSFER_SRC_EXT;
		glSignalSemaphoreEXT(slot.semaphore, 0, nullptr, 1, &slot.texture, &layout);

		// The frame is handed to OpenXR once this fence was reached (see check_finished_copies)
		slot.fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		glFlush();
	}
}
