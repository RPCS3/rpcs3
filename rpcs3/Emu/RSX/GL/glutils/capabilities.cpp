#include "stdafx.h"
#include "capabilities.h"

#include "Utilities/StrUtil.h"

#include <unordered_set>

namespace gl
{
	version_info::version_info(const char* version_string, int major_scale)
	{
		auto tokens = fmt::split(version_string, { "." });
		if (tokens.size() < 2)
		{
			rsx_log.warning("Invalid version string: '%s'", version_string);
			version = version_major = version_minor = 0;
			return;
		}

		version_major = static_cast<u8>(std::stoi(tokens[0]));
		version_minor = static_cast<u8>(std::stoi(tokens[1]));
		version = static_cast<u16>(version_major * major_scale) + version_minor;
	}

	void capabilities::initialize()
	{
		int ext_count = 0;
		glGetIntegerv(GL_NUM_EXTENSIONS, &ext_count);

		if (!ext_count)
		{
			rsx_log.error("Could not initialize GL driver capabilities. Is OpenGL initialized?");
			return;
		}

		std::string vendor_string = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
		std::string version_string = reinterpret_cast<const char*>(glGetString(GL_VERSION));
		std::string renderer_string = reinterpret_cast<const char*>(glGetString(GL_RENDERER));

		std::unordered_set<std::string> all_extensions;
		for (int i = 0; i < ext_count; i++)
		{
			all_extensions.emplace(reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i)));
		}

#define CHECK_EXTENSION_SUPPORT(extension_short_name)\
	do {\
		if (all_extensions.contains("GL_"#extension_short_name)) {\
			extension_short_name##_supported = true;\
			rsx_log.success("[CAPS] Using GL_"#extension_short_name);\
			continue;\
		} \
	} while (0)

		CHECK_EXTENSION_SUPPORT(ARB_shader_draw_parameters);

		CHECK_EXTENSION_SUPPORT(EXT_direct_state_access);

		CHECK_EXTENSION_SUPPORT(ARB_direct_state_access);

		CHECK_EXTENSION_SUPPORT(ARB_bindless_texture);

		CHECK_EXTENSION_SUPPORT(ARB_buffer_storage);

		CHECK_EXTENSION_SUPPORT(ARB_texture_buffer_object);

		CHECK_EXTENSION_SUPPORT(ARB_depth_buffer_float);

		CHECK_EXTENSION_SUPPORT(ARB_texture_barrier);

		CHECK_EXTENSION_SUPPORT(NV_texture_barrier);

		CHECK_EXTENSION_SUPPORT(NV_gpu_shader5);

		CHECK_EXTENSION_SUPPORT(AMD_gpu_shader_half_float);

		CHECK_EXTENSION_SUPPORT(ARB_compute_shader);

		CHECK_EXTENSION_SUPPORT(EXT_depth_bounds_test);

		CHECK_EXTENSION_SUPPORT(NV_depth_buffer_float);

		CHECK_EXTENSION_SUPPORT(ARB_shader_stencil_export);

		CHECK_EXTENSION_SUPPORT(NV_fragment_shader_barycentric);

		CHECK_EXTENSION_SUPPORT(AMD_pinned_memory);

		CHECK_EXTENSION_SUPPORT(ARB_shader_texture_image_samples);

		CHECK_EXTENSION_SUPPORT(EXT_texture_compression_s3tc);

#undef CHECK_EXTENSION_SUPPORT

		// Set GLSL version
		glsl_version = version_info(reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));

		// Check GL_VERSION and GL_RENDERER for the presence of Mesa
		if (version_string.find("Mesa") != umax || renderer_string.find("Mesa") != umax)
		{
			vendor_MESA = true;

			if (vendor_string.find("nouveau") != umax)
			{
				subvendor_NOUVEAU = true;
			}
			else if (vendor_string.find("AMD") != umax)
			{
				subvendor_RADEONSI = true;
			}
		}

		// Workaround for intel drivers which have terrible capability reporting
		if (!vendor_string.empty())
		{
			std::transform(vendor_string.begin(), vendor_string.end(), vendor_string.begin(), ::tolower);
		}
		else
		{
			rsx_log.error("Failed to get vendor string from driver. Are we missing a context?");
			vendor_string = "intel"; // lowest acceptable value
		}

		if (!vendor_MESA && vendor_string.find("intel") != umax)
		{
			int version_major = 0;
			int version_minor = 0;

			glGetIntegerv(GL_MAJOR_VERSION, &version_major);
			glGetIntegerv(GL_MINOR_VERSION, &version_minor);

			vendor_INTEL = true;

			// Texture buffers moved into core at GL 3.3
			if (version_major > 3 || (version_major == 3 && version_minor >= 3))
				ARB_texture_buffer_object_supported = true;

			// Check for expected library entry-points for some required functions
			if (!ARB_buffer_storage_supported && glNamedBufferStorage && glMapNamedBufferRange)
				ARB_buffer_storage_supported = true;

			if (!ARB_direct_state_access_supported && glGetTextureImage && glTextureBufferRange)
				ARB_direct_state_access_supported = true;

			if (!EXT_direct_state_access_supported && glGetTextureImageEXT && glTextureBufferRangeEXT)
				EXT_direct_state_access_supported = true;
		}
		else if (!vendor_MESA && vendor_string.find("nvidia") != umax)
		{
			vendor_NVIDIA = true;
		}
#ifdef _WIN32
		else if (vendor_string.find("amd") != umax || vendor_string.find("ati") != umax)
		{
			vendor_AMD = true;

			// NOTE: Some of the later rebrands ended up in the 7000 line with 'AMD Radeon' branding.
			// However, they all are stuck at GLSL 4.40 and below.
			subvendor_ATI = renderer_string.find("ATI Radeon") != umax || glsl_version.version < 450;
		}
#endif

		initialized = true;
	}
}
