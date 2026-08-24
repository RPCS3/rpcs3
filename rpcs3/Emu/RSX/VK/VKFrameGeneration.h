#pragma once

#include "VulkanAPI.h"
#include "util/types.hpp"

#include <memory>
#include <string>
#include <vector>

namespace vk
{
	class render_device;

	constexpr u32 max_generated_frames = 3;

	struct frame_generation_settings
	{
		bool enabled = false;
		u32 multiplier = 2;
		u32 target_rate = 0;
		u32 flow_scale_percent = 100;
	};

	struct frame_generation_status
	{
		bool ready = false;
		bool unsupported = false;
		u32 width = 0;
		u32 height = 0;
	};

	void set_frame_generation_shader_cache(std::string path);
	std::string get_frame_generation_shader_cache();

	u64 frame_generation_shader_revision();
	void invalidate_frame_generation_shaders();

	void set_frame_generation_settings(const frame_generation_settings& settings);
	frame_generation_settings get_frame_generation_settings();

	u32 frame_generation_reserved_images();

	u64 frame_generation_presented_count();
	u64 frame_generation_generated_count();
	void frame_generation_count_presented(u32 frames);
	void frame_generation_count_generated(u32 frames);

	frame_generation_status get_frame_generation_status();
	void set_frame_generation_status(const frame_generation_status& status);

	class frame_generator
	{
	public:
		explicit frame_generator(const vk::render_device& dev);
		~frame_generator();

		frame_generator(const frame_generator&) = delete;
		frame_generator& operator=(const frame_generator&) = delete;

		bool is_usable() const { return m_shaders_ready && !m_unavailable; }

		bool prepare(u32 width, u32 height);

		u32 plan(u32 capacity);

		void process(VkCommandBuffer cmd, VkImage source, VkImageLayout source_layout, u32 width, u32 height);

		u32 generated_count() const;

		void emit(VkCommandBuffer cmd, u32 index, VkImage target, VkImageLayout present_layout, u32 width, u32 height);

		VkSemaphore next_acquire_semaphore();
		VkSemaphore present_semaphore(u32 image_index);

		void reset();

	private:
		struct impl;

		VkSemaphore create_semaphore() const;

		const vk::render_device& m_device;
		std::unique_ptr<impl> m_impl;
		std::vector<VkSemaphore> m_acquire_semaphores;
		std::vector<VkSemaphore> m_present_semaphores;
		usz m_acquire_cursor = 0;
		bool m_shaders_ready = false;
		bool m_unavailable = false;
	};
}
