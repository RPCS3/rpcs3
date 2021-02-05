#pragma once

#include "util/types.hpp"
#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <unordered_map>
#include <variant>
#include <stack>
#include <deque>

#include "VulkanAPI.h"
#include "vkutils/chip_class.h"
#include "Utilities/geometry.h"
#include "Emu/RSX/Common/TextureUtils.h"

#define DESCRIPTOR_MAX_DRAW_CALLS 16384
#define OCCLUSION_MAX_POOL_SIZE   DESCRIPTOR_MAX_DRAW_CALLS

#define FRAME_PRESENT_TIMEOUT 10000000ull // 10 seconds
#define GENERAL_WAIT_TIMEOUT  2000000ull  // 2 seconds

namespace vk
{
	// Forward declarations
	struct buffer;
	class command_buffer;
	class data_heap;
	struct fence;
	class image;
	class instance;
	class render_device;

	//VkAllocationCallbacks default_callbacks();
	enum runtime_state
	{
		uninterruptible = 1,
		heap_dirty = 2,
		heap_changed = 3
	};

	const vk::render_device *get_current_renderer();
	void set_current_renderer(const vk::render_device &device);

	//Compatibility workarounds
	bool emulate_primitive_restart(rsx::primitive_type type);
	bool sanitize_fp_values();
	bool fence_reset_disabled();
	bool emulate_conditional_rendering();
	VkFlags get_heap_compatible_buffer_types();
	driver_vendor get_driver_vendor();

	// Sync helpers around vkQueueSubmit
	void acquire_global_submit_lock();
	void release_global_submit_lock();
	void queue_submit(VkQueue queue, const VkSubmitInfo* info, fence* pfence, VkBool32 flush = VK_FALSE);

	template<class T>
	T* get_compute_task();
	void reset_compute_tasks();

	void destroy_global_resources();
	void reset_global_resources();

	/**
	* Allocate enough space in upload_buffer and write all mipmap/layer data into the subbuffer.
	* Then copy all layers into dst_image.
	* dst_image must be in TRANSFER_DST_OPTIMAL layout and upload_buffer have TRANSFER_SRC_BIT usage flag.
	*/
	void copy_mipmaped_image_using_buffer(const vk::command_buffer& cmd, vk::image* dst_image,
		const std::vector<rsx::subresource_layout>& subresource_layout, int format, bool is_swizzled, u16 mipmap_count,
		VkImageAspectFlags flags, vk::data_heap &upload_heap, u32 heap_align = 0);

	//Other texture management helpers
	void copy_image_to_buffer(VkCommandBuffer cmd, const vk::image* src, const vk::buffer* dst, const VkBufferImageCopy& region, bool swap_bytes = false);
	void copy_buffer_to_image(VkCommandBuffer cmd, const vk::buffer* src, const vk::image* dst, const VkBufferImageCopy& region);

	void copy_image_typeless(const command_buffer &cmd, image *src, image *dst, const areai& src_rect, const areai& dst_rect,
		u32 mipmaps, VkImageAspectFlags src_transfer_mask = 0xFF, VkImageAspectFlags dst_transfer_mask = 0xFF);

	void copy_image(const vk::command_buffer& cmd, vk::image* src, vk::image* dst,
			const areai& src_rect, const areai& dst_rect, u32 mipmaps,
			VkImageAspectFlags src_transfer_mask = 0xFF, VkImageAspectFlags dst_transfer_mask = 0xFF);

	void copy_scaled_image(const vk::command_buffer& cmd, vk::image* src, vk::image* dst,
			const areai& src_rect, const areai& dst_rect, u32 mipmaps,
			bool compatible_formats, VkFilter filter = VK_FILTER_LINEAR);

	std::pair<VkFormat, VkComponentMapping> get_compatible_surface_format(rsx::surface_color_format color_format);

	// Runtime stuff
	void raise_status_interrupt(runtime_state status);
	void clear_status_interrupt(runtime_state status);
	bool test_status_interrupt(runtime_state status);
	void enter_uninterruptible();
	void leave_uninterruptible();
	bool is_uninterruptible();

	void advance_completed_frame_counter();
	void advance_frame_counter();
	const u64 get_current_frame_id();
	const u64 get_last_completed_frame_id();

	// Handle unexpected submit with dangling occlusion query
	// TODO: Move queries out of the renderer!
	void do_query_cleanup(vk::command_buffer& cmd);

	struct blitter
	{
		void scale_image(vk::command_buffer& cmd, vk::image* src, vk::image* dst, areai src_area, areai dst_area, bool interpolate, const rsx::typeless_xfer& xfer_info);
	};
}
