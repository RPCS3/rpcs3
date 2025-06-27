#pragma once

#include "../VulkanAPI.h"
#include "chip_class.h"
#include "pipeline_binding_table.h"
#include "memory.h"

#include <string>
#include <vector>
#include <unordered_map>

#define DESCRIPTOR_MAX_DRAW_CALLS 32768

namespace vk
{
	struct gpu_formats_support
	{
		bool d24_unorm_s8 : 1;
		bool d32_sfloat_s8 : 1;
		bool bgra8_linear : 1;
		bool argb8_linear : 1;
	};

	struct gpu_shader_types_support
	{
		bool allow_float64 : 1;
		bool allow_float16 : 1;
		bool allow_int8 : 1;
	};

	struct memory_type_mapping
	{
		memory_type_info host_visible_coherent;
		memory_type_info device_local;
		memory_type_info device_bar;

		u64 device_local_total_bytes;
		u64 host_visible_total_bytes;
		u64 device_bar_total_bytes;
	};

	struct descriptor_indexing_features
	{
		bool supported = false;
		u64 update_after_bind_mask = 0;

		descriptor_indexing_features(bool supported = false)
			: supported(supported) {}

		operator bool() const { return supported; }
	};

	struct custom_border_color_features
	{
		bool supported = false;
		bool swizzle_extension_supported = false;
		bool require_border_color_remap = true;  // Assume that without the swizzle extension and explicit remap the device just samples border color as replacement.

		operator bool() const { return supported; }
	};

	struct multidraw_features
	{
		bool supported;
		u32 max_batch_size;

		operator bool() const { return supported; }
	};

	class physical_device
	{
		VkInstance parent = VK_NULL_HANDLE;
		VkPhysicalDevice dev = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties props;
		VkPhysicalDeviceFeatures features;
		VkPhysicalDeviceMemoryProperties memory_properties;
		std::vector<VkQueueFamilyProperties> queue_props;

		mutable std::unordered_map<VkFormat, VkFormatProperties> format_properties;
		gpu_shader_types_support shader_types_support{};
		VkPhysicalDeviceDriverPropertiesKHR driver_properties{};

		u32 descriptor_max_draw_calls = DESCRIPTOR_MAX_DRAW_CALLS;
		descriptor_indexing_features descriptor_indexing_support{};

		custom_border_color_features custom_border_color_support{};

		multidraw_features multidraw_support{};

		struct
		{
			bool barycentric_coords = false;
			bool conditional_rendering = false;
			bool debug_utils = false;
			bool external_memory_host = false;
			bool framebuffer_loops = false;
			bool shader_stencil_export = false;
			bool surface_capabilities_2 = false;
			bool synchronization_2 = false;
			bool unrestricted_depth_range = false;
			bool extended_device_fault = false;
			bool texture_compression_bc = false;
		} optional_features_support;

		friend class render_device;
	private:
		void get_physical_device_features(bool allow_extensions);
		void get_physical_device_properties_0(bool allow_extensions);
		void get_physical_device_properties_1(bool allow_extensions);

	public:

		physical_device() = default;
		~physical_device() = default;

		void create(VkInstance context, VkPhysicalDevice pdev, bool allow_extensions);

		std::string get_name() const;

		driver_vendor get_driver_vendor() const;
		std::string get_driver_version() const;
		chip_class get_chip_class() const;

		u32 get_queue_count() const;

		// Device properties. These structs can be large so use with care.
		const VkQueueFamilyProperties& get_queue_properties(u32 queue);
		const VkPhysicalDeviceMemoryProperties& get_memory_properties() const;
		const VkPhysicalDeviceLimits& get_limits() const;

		operator VkPhysicalDevice() const;
		operator VkInstance() const;
	};

	class render_device
	{
		physical_device* pgpu = nullptr;
		memory_type_mapping memory_map{};
		gpu_formats_support m_formats_support{};
		std::unique_ptr<mem_allocator_base> m_allocator;
		VkDevice dev = VK_NULL_HANDLE;

		VkQueue m_graphics_queue = VK_NULL_HANDLE;
		VkQueue m_present_queue = VK_NULL_HANDLE;
		VkQueue m_transfer_queue = VK_NULL_HANDLE;

		u32 m_graphics_queue_family = 0;
		u32 m_present_queue_family = 0;
		u32 m_transfer_queue_family = 0;

		void dump_debug_info(
			const std::vector<const char*>& requested_extensions,
			const VkPhysicalDeviceFeatures& requested_features) const;

	public:
		render_device() = default;
		~render_device() = default;

		void create(vk::physical_device& pdev, u32 graphics_queue_idx, u32 present_queue_idx, u32 transfer_queue_idx);
		void destroy();

		const VkFormatProperties get_format_properties(VkFormat format) const;

		bool get_compatible_memory_type(u32 typeBits, u32 desired_mask, u32* type_index) const;
		void rebalance_memory_type_usage();

		const physical_device& gpu() const { return *pgpu; }
		const memory_type_mapping& get_memory_mapping() const { return memory_map; }
		const gpu_formats_support& get_formats_support() const { return m_formats_support; }
		const gpu_shader_types_support& get_shader_types_support() const { return pgpu->shader_types_support; }
		const custom_border_color_features& get_custom_border_color_support() const { return pgpu->custom_border_color_support; }
		const multidraw_features get_multidraw_support() const { return pgpu->multidraw_support; }

		bool get_shader_stencil_export_support() const { return pgpu->optional_features_support.shader_stencil_export; }
		bool get_depth_bounds_support() const { return pgpu->features.depthBounds != VK_FALSE; }
		bool get_alpha_to_one_support() const { return pgpu->features.alphaToOne != VK_FALSE; }
		bool get_anisotropic_filtering_support() const { return pgpu->features.samplerAnisotropy != VK_FALSE; }
		bool get_wide_lines_support() const { return pgpu->features.wideLines != VK_FALSE; }
		bool get_conditional_render_support() const { return pgpu->optional_features_support.conditional_rendering; }
		bool get_unrestricted_depth_range_support() const { return pgpu->optional_features_support.unrestricted_depth_range; }
		bool get_external_memory_host_support() const { return pgpu->optional_features_support.external_memory_host; }
		bool get_surface_capabilities_2_support() const { return pgpu->optional_features_support.surface_capabilities_2; }
		bool get_debug_utils_support() const { return g_cfg.video.renderdoc_compatiblity && pgpu->optional_features_support.debug_utils; }
		bool get_framebuffer_loops_support() const { return pgpu->optional_features_support.framebuffer_loops; }
		bool get_barycoords_support() const { return pgpu->optional_features_support.barycentric_coords; }
		bool get_synchronization2_support() const { return pgpu->optional_features_support.synchronization_2; }
		bool get_extended_device_fault_support() const { return pgpu->optional_features_support.extended_device_fault; }
		bool get_texture_compression_bc_support() const { return pgpu->optional_features_support.texture_compression_bc; }

		u64 get_descriptor_update_after_bind_support() const { return pgpu->descriptor_indexing_support.update_after_bind_mask; }
		u32 get_descriptor_max_draw_calls() const { return pgpu->descriptor_max_draw_calls; }

		VkQueue get_present_queue() const { return m_present_queue; }
		VkQueue get_graphics_queue() const { return m_graphics_queue; }
		VkQueue get_transfer_queue() const { return m_transfer_queue; }
		u32 get_graphics_queue_family() const { return m_graphics_queue_family; }
		u32 get_present_queue_family() const { return m_graphics_queue_family; }
		u32 get_transfer_queue_family() const { return m_transfer_queue_family; }

		mem_allocator_base* get_allocator() const { return m_allocator.get(); }

		operator VkDevice() const { return dev; }
	};

	memory_type_mapping get_memory_mapping(const physical_device& dev);
	gpu_formats_support get_optimal_tiling_supported_formats(const physical_device& dev);

	extern const render_device* g_render_device;
}
