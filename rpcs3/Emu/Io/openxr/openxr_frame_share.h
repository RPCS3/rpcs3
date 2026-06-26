#pragma once

#include "util/types.hpp"
#include "util/atomic.hpp"
#include "Utilities/geometry.h"
#include "Emu/system_config.h"

#include <array>
#include <initializer_list>
#include <mutex>

// Shares the presented RSX frames with the OpenXR handler, so that they can be shown in the headset.
// The OpenXR side creates the images on its own Vulkan device and exports their memory (and a semaphore per image).
// The RSX Vulkan or OpenGL backend imports them and copies each presented frame into a free slot.
// RSX only marks a slot as ready after it waited for the frame to finish.
// The Vulkan backend needs no semaphores. The OpenGL backend signals the slot's semaphore, because OpenGL can only
// hand the image over to Vulkan in a defined layout through a semaphore (see EXT_external_objects).
namespace openxr
{
	// Set in the config (Video: "OpenXR Render Mode"). This is a dynamic setting, so it can change at any time.
	inline openxr_render_mode get_render_mode()
	{
		return g_cfg.video.openxr_rendering;
	}

	enum class slot_state : u32
	{
		free,    // Can be written by RSX
		writing, // RSX recorded a copy into the slot, which may not be finished yet
		ready,   // The copy is finished and the frame can be shown
		reading, // OpenXR is copying the frame into its swapchain
	};

	struct shared_frame_slot
	{
		u64 memory_handle = 0;    // HANDLE on Windows, file descriptor on other platforms
		u64 semaphore_handle = 0; // Same, only valid if shared_frames::has_semaphores is set
		atomic_t<slot_state> state = slot_state::free;
		atomic_t<u64> frame_number = 0;

		// Set by RSX while writing
		atomic_t<bool> semaphore_signaled = false; // OpenXR has to wait for the semaphore exactly once before reading
	};

	struct shared_frames
	{
		static constexpr u32 slot_count = 3;

		// The images contain both eyes side by side. The left eye is on the left half.
		static constexpr u32 eye_width = 1920;
		static constexpr u32 eye_height = 1080;
		static constexpr u32 image_width = eye_width * 2;
		static constexpr u32 image_height = eye_height;

		// Protects everything except the slot states and frame numbers
		std::mutex mutex;

		u64 generation = 0;   // Incremented whenever the images are created or destroyed
		bool active = false;  // True while the images can be imported
		std::array<u8, 16> device_uuid{}; // Both devices have to use the same GPU
		u32 format = 0;       // VkFormat of the images
		u32 memory_type_index = 0;
		u64 allocation_size = 0;
		bool has_semaphores = false; // Required by the OpenGL backend

		std::array<shared_frame_slot, slot_count> slots;
		u64 frame_counter = 0;

		// Used by RSX: claims a free slot for writing. Returns -1 if all slots are busy or the images of this generation are gone.
		// The generation is checked in the same critical section, so the slot can't belong to newer images.
		s32 acquire_slot(u64 image_generation, bool signals_semaphore)
		{
			std::lock_guard lock(mutex);

			if (!active || generation != image_generation)
			{
				return -1;
			}

			for (u32 i = 0; i < slot_count; i++)
			{
				if (slots[i].state.compare_and_swap_test(slot_state::free, slot_state::writing))
				{
					slots[i].semaphore_signaled = signals_semaphore;
					return static_cast<s32>(i);
				}
			}

			return -1;
		}

		// Used by RSX: hands a slot claimed by acquire_slot to the OpenXR handler, or frees it if the copy failed.
		// Does nothing if the images were destroyed in the meantime. The OpenXR handler frees all slots when it creates new images.
		void publish_slot(s32 index, u64 image_generation, bool success)
		{
			std::lock_guard lock(mutex);

			if (!active || generation != image_generation)
			{
				return;
			}

			shared_frame_slot& slot = ::at32(slots, index);

			if (success)
			{
				slot.frame_number = ++frame_counter;
				slot.state.compare_and_swap_test(slot_state::writing, slot_state::ready);
			}
			else
			{
				slot.state.compare_and_swap_test(slot_state::writing, slot_state::free);
			}
		}
	};

	inline shared_frames g_shared_frames;

	inline bool is_rendering_enabled()
	{
		return get_render_mode() != openxr_render_mode::disabled;
	}

	// Where each eye is in the images that RSX presents
	struct eye_layout
	{
		areai left;
		areai right;
		bool separate_images = false; // The right eye has its own image (the second display buffer)
		bool stacked = false;         // Both eyes are in the same image. The right eye is at the bottom.
	};

	// eye_width/eye_height: the size of one eye in the image. image_height: the height of the first image.
	inline eye_layout get_eye_layout(u32 eye_width, u32 eye_height, u32 image_height, bool stereo_enabled, bool has_second_image)
	{
		eye_layout layout{};
		layout.left = { 0, 0, static_cast<s32>(eye_width), static_cast<s32>(eye_height) };
		layout.right = layout.left;

		if (has_second_image)
		{
			layout.separate_images = true;
		}
		else if (stereo_enabled)
		{
			const s32 right_y = static_cast<s32>(image_height) - static_cast<s32>(eye_height);
			layout.right = { 0, right_y, static_cast<s32>(eye_width), right_y + static_cast<s32>(eye_height) };
			layout.stacked = true;
		}

		return layout;
	}

	// Selects the image of each eye. Returns false if an image is missing.
	// If with_overlays is set, add_overlays(image, index, areas) has to return a copy of the image with the overlays drawn into the eye areas.
	template <typename T, typename F>
	bool get_eye_images(T* left_image, T* right_image, const eye_layout& eyes, bool with_overlays, F&& add_overlays, T*& left, T*& right)
	{
		if (!left_image || (eyes.separate_images && !right_image))
		{
			return false;
		}

		left = left_image;
		right = eyes.separate_images ? right_image : left_image;

		if (!with_overlays)
		{
			return true;
		}

		if (eyes.separate_images)
		{
			left = add_overlays(left_image, 0, std::initializer_list<areai>{ eyes.left });
			right = add_overlays(right_image, 1, std::initializer_list<areai>{ eyes.right });
		}
		else if (eyes.stacked)
		{
			left = right = add_overlays(left_image, 0, std::initializer_list<areai>{ eyes.left, eyes.right });
		}
		else
		{
			left = right = add_overlays(left_image, 0, std::initializer_list<areai>{ eyes.left });
		}

		return true;
	}

	// The area of an eye in the shared images
	inline areai get_shared_eye_area(u32 eye)
	{
		constexpr s32 width = static_cast<s32>(shared_frames::eye_width);
		constexpr s32 height = static_cast<s32>(shared_frames::eye_height);
		return { static_cast<s32>(eye) * width, 0, (static_cast<s32>(eye) + 1) * width, height };
	}

	// Vulkan values needed by the OpenGL backend, which doesn't include the Vulkan headers (checked in openxr_frame_share_vk.h)
	constexpr u32 vk_format_r8g8b8a8_unorm = 37; // VK_FORMAT_R8G8B8A8_UNORM, the only format OpenGL can import (GL_RGBA8)
}
