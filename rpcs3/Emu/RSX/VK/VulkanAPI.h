#pragma once

// Configure vulkan.h
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__APPLE__)
#define VK_USE_PLATFORM_METAL_EXT
#elif defined(ANDROID)
#define VK_USE_PLATFORM_ANDROID_KHR
#define VK_NO_PROTOTYPES
#else
#if defined(HAVE_X11)
 #define VK_USE_PLATFORM_XLIB_KHR
#endif
#if defined(HAVE_WAYLAND)
 #define VK_USE_PLATFORM_WAYLAND_KHR
#endif
#endif

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4005 )
#endif

#include <vulkan/vulkan.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

// Undefine header configuration variables
#undef VK_USE_PLATFORM_WIN32_KHR
#undef VK_USE_PLATFORM_METAL_EXT
#undef VK_USE_PLATFORM_ANDROID_KHR
#undef VK_USE_PLATFORM_XLIB_KHR
#undef VK_USE_PLATFORM_WAYLAND_KHR

#include <util/types.hpp>

#if VK_HEADER_VERSION < 287
constexpr VkDriverId VK_DRIVER_ID_MESA_HONEYKRISP = static_cast<VkDriverId>(26);
#endif

#if VK_HEADER_VERSION < 332
#define VK_EXT_shader_uniform_buffer_unsized_array 1
#define VK_EXT_SHADER_UNIFORM_BUFFER_UNSIZED_ARRAY_SPEC_VERSION 1
#define VK_EXT_SHADER_UNIFORM_BUFFER_UNSIZED_ARRAY_EXTENSION_NAME "VK_EXT_shader_uniform_buffer_unsized_array"
typedef struct VkPhysicalDeviceShaderUniformBufferUnsizedArrayFeaturesEXT {
	VkStructureType    sType;
	void* pNext;
	VkBool32           shaderUniformBufferUnsizedArray;
} VkPhysicalDeviceShaderUniformBufferUnsizedArrayFeaturesEXT;

constexpr VkStructureType VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_UNIFORM_BUFFER_UNSIZED_ARRAY_FEATURES_EXT = static_cast<VkStructureType>(1000642000);
#endif

#define DECLARE_VK_FUNCTION_HEADER 1
#include "VKProcTable.h"

#ifdef ANDROID
#include <string>
#include <utility>
#include <vector>

namespace vk
{
	template <usz N>
	struct symbol_name
	{
		char data[N];

		consteval symbol_name(const char (&str)[N])
		{
			for (usz i = 0; i < N; ++i)
			{
				data[i] = str[i];
			}
		}
	};

	class symbol_cache
	{
		std::vector<std::pair<const char*, void**>> registered_symbols;

	public:
		void initialize(void* loader);
		void clear();

		void register_symbol(const char* name, void** ptr);

		static symbol_cache& cache_instance()
		{
			static symbol_cache result;
			return result;
		}
	};

	template <auto V>
	class symbol_cache_entry
	{
		void* ptr = nullptr;

	public:
		symbol_cache_entry()
		{
			symbol_cache::cache_instance().register_symbol(V.data, &ptr);
		}

		void* get() const { return ptr; }
	};

	template <auto V>
	symbol_cache_entry<V> cached_symbol;
}

#define VK_GET_SYMBOL(func) reinterpret_cast<PFN_##func>(::vk::cached_symbol<::vk::symbol_name{#func}>.get())
#else
#define VK_GET_SYMBOL(func) func
#endif

namespace vk
{
	void init();
}
