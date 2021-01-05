#pragma once

#include "physical_device.h"
#include <memory>

namespace vk
{
	class mem_allocator_base;

	class render_device
	{
		physical_device* pgpu = nullptr;
		memory_type_mapping memory_map{};
		gpu_formats_support m_formats_support{};
		pipeline_binding_table m_pipeline_binding_table{};
		std::unique_ptr<mem_allocator_base> m_allocator;
		VkDevice dev = VK_NULL_HANDLE;

	public:
		// Exported device endpoints
		PFN_vkCmdBeginConditionalRenderingEXT cmdBeginConditionalRenderingEXT = nullptr;
		PFN_vkCmdEndConditionalRenderingEXT cmdEndConditionalRenderingEXT     = nullptr;

	public:
		render_device()  = default;
		~render_device() = default;

		void create(vk::physical_device& pdev, u32 graphics_queue_idx);
		void destroy();

		const VkFormatProperties get_format_properties(VkFormat format);

		bool get_compatible_memory_type(u32 typeBits, u32 desired_mask, u32* type_index) const;

		const physical_device& gpu() const;
		const memory_type_mapping& get_memory_mapping() const;
		const gpu_formats_support& get_formats_support() const;
		const pipeline_binding_table& get_pipeline_binding_table() const;
		const gpu_shader_types_support& get_shader_types_support() const;

		bool get_shader_stencil_export_support() const;
		bool get_depth_bounds_support() const;
		bool get_alpha_to_one_support() const;
		bool get_conditional_render_support() const;
		bool get_unrestricted_depth_range_support() const;

		mem_allocator_base* get_allocator() const;

		operator VkDevice() const;
	};
}
