#pragma once

#include "util/types.hpp"
#include <vector>

#include "VulkanAPI.h"
#include "Utilities/geometry.h"
#include "Emu/RSX/Common/TextureUtils.h"
#include "Emu/RSX/rsx_utils.h"

#define OCCLUSION_MAX_POOL_SIZE   DESCRIPTOR_MAX_DRAW_CALLS

namespace rsx
{
	struct GCM_tile_reference;
}

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
	struct queue_submit_t;

	enum runtime_state
	{
		uninterruptible = 1,
		heap_dirty      = 2,
		heap_changed    = 4,
	};

	struct image_readback_options_t
	{
		bool swap_bytes = false;
		struct
		{
			u64 offset = 0;
			u64 length = 0;

			operator bool() const { return length != 0; }
		} sync_region {};
	};

	const vk::render_device *get_current_renderer();
	void set_current_renderer(const vk::render_device &device);

	// Compatibility workarounds
	bool emulate_primitive_restart(rsx::primitive_type type);
	bool sanitize_fp_values();
	bool fence_reset_disabled();
	bool emulate_conditional_rendering();
	bool use_strict_query_scopes();
	bool force_reuse_query_pools();
	VkFlags get_heap_compatible_buffer_types();

	// Sync helpers around vkQueueSubmit
	void acquire_global_submit_lock();
	void release_global_submit_lock();
	void queue_submit(const vk::queue_submit_t* packet);

	template<class T>
	T* get_compute_task();

	void destroy_global_resources();
	void reset_global_resources();

	enum image_upload_options
	{
		upload_contents_async   = 1,
		initialize_image_layout = 2,
		preserve_image_layout   = 4,
		source_is_gpu_resident  = 8,

		// meta-flags
		upload_contents_inline    = 0,
		upload_heap_align_default = 0
	};

	void upload_image(const vk::command_buffer& cmd, vk::image* dst_image,
		const std::vector<rsx::subresource_layout>& subresource_layout, int format, bool is_swizzled, u16 layer_count,
		VkImageAspectFlags flags, vk::data_heap &upload_heap, u32 heap_align, rsx::flags32_t image_setup_flags);

	std::pair<buffer*, u32> detile_memory_block(
		const vk::command_buffer& cmd, const rsx::GCM_tile_reference& tiled_region, const utils::address_range32& range,
		u16 width, u16 height, u8 bpp);

	// Other texture management helpers
	void copy_image_to_buffer(const vk::command_buffer& cmd, const vk::image* src, const vk::buffer* dst, const VkBufferImageCopy& region, const image_readback_options_t& options = {});
	void copy_buffer_to_image(const vk::command_buffer& cmd, const vk::buffer* src, const vk::image* dst, const VkBufferImageCopy& region);
	u64  calculate_working_buffer_size(u64 base_size, VkImageAspectFlags aspect);

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
	u64 get_current_frame_id();
	u64 get_last_completed_frame_id();

	// Handle unexpected submit with dangling occlusion query
	// TODO: Move queries out of the renderer!
	void do_query_cleanup(vk::command_buffer& cmd);

	struct blitter
	{
		void scale_image(vk::command_buffer& cmd, vk::image* src, vk::image* dst, areai src_area, areai dst_area, bool interpolate, const rsx::typeless_xfer& xfer_info);
	};
}
