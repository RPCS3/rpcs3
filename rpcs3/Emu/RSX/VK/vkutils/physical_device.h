#pragma once

#include "../VulkanAPI.h"
#include "chip_class.h"
#include "pipeline_binding_table.h"

#include <string>
#include <vector>
#include <unordered_map>

namespace vk
{
	struct gpu_formats_support
	{
		bool d24_unorm_s8;
		bool d32_sfloat_s8;
		bool bgra8_linear;
		bool argb8_linear;
	};

	struct gpu_shader_types_support
	{
		bool allow_float64;
		bool allow_float16;
		bool allow_int8;
	};

	struct memory_type_mapping
	{
		u32 host_visible_coherent;
		u32 device_local;
	};

	class physical_device
	{
		VkInstance parent = VK_NULL_HANDLE;
		VkPhysicalDevice dev = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties props;
		VkPhysicalDeviceFeatures features;
		VkPhysicalDeviceMemoryProperties memory_properties;
		std::vector<VkQueueFamilyProperties> queue_props;

		std::unordered_map<VkFormat, VkFormatProperties> format_properties;
		gpu_shader_types_support shader_types_support{};
		VkPhysicalDeviceDriverPropertiesKHR driver_properties{};

		bool stencil_export_support = false;
		bool conditional_render_support = false;
		bool unrestricted_depth_range_support = false;

		friend class render_device;
	private:
		void get_physical_device_features(bool allow_extensions);

	public:

		physical_device() = default;
		~physical_device() = default;

		void create(VkInstance context, VkPhysicalDevice pdev, bool allow_extensions);

		std::string get_name() const;

		driver_vendor get_driver_vendor() const;
		std::string get_driver_version() const;
		chip_class get_chip_class() const;

		u32 get_queue_count() const;

		VkQueueFamilyProperties get_queue_properties(u32 queue);
		VkPhysicalDeviceMemoryProperties get_memory_properties() const;
		VkPhysicalDeviceLimits get_limits() const;

		operator VkPhysicalDevice() const;
		operator VkInstance() const;
	};

	memory_type_mapping get_memory_mapping(const physical_device& dev);
	gpu_formats_support get_optimal_tiling_supported_formats(const physical_device& dev);
	pipeline_binding_table get_pipeline_binding_table(const physical_device& dev);
}
