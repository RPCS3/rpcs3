#pragma once

#include "../OpenGL.h"
#include <util/types.hpp>
#include <util/asm.hpp>
#include <util/logs.hpp>

namespace gl
{
	struct version_info
	{
		u8 version_major = 0;
		u8 version_minor = 0;
		u16 version = 0;

		version_info() = default;
		version_info(const char* version_string, int major_scale = 100);
	};

	class capabilities
	{
	public:
		bool initialized = false;
		version_info glsl_version;

		bool EXT_direct_state_access_supported = false;
		bool EXT_depth_bounds_test_supported = false;
		bool AMD_pinned_memory_supported = false;
		bool ARB_direct_state_access_supported = false;
		bool ARB_bindless_texture_supported = false;
		bool ARB_buffer_storage_supported = false;
		bool ARB_texture_buffer_object_supported = false;
		bool ARB_shader_draw_parameters_supported = false;
		bool ARB_depth_buffer_float_supported = false;
		bool ARB_texture_barrier_supported = false;
		bool ARB_shader_stencil_export_supported = false;
		bool NV_texture_barrier_supported = false;
		bool NV_gpu_shader5_supported = false;
		bool AMD_gpu_shader_half_float_supported = false;
		bool ARB_compute_shader_supported = false;
		bool NV_depth_buffer_float_supported = false;
		bool NV_fragment_shader_barycentric_supported = false;
		bool ARB_shader_texture_image_samples_supported = false;
		bool EXT_texture_compression_s3tc_supported = false;

		bool vendor_INTEL = false;  // has broken GLSL compiler
		bool vendor_AMD = false;    // has broken ARB_multidraw
		bool vendor_NVIDIA = false; // has NaN poisoning issues
		bool vendor_MESA = false;   // requires CLIENT_STORAGE bit set for streaming buffers
		bool subvendor_RADEONSI = false;
		bool subvendor_NOUVEAU = false;
		bool subvendor_ATI = false; // Pre-GCN cards (terascale, evergreen)

		void initialize();
	};

	const capabilities& get_driver_caps();
}
