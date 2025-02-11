#include "stdafx.h"
#include "capabilities.h"

#include "Utilities/StrUtil.h"

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

	bool capabilities::check(const std::string& ext_name, const char* test)
	{
		if (ext_name == test)
		{
			rsx_log.notice("Extension %s is supported", ext_name);
			return true;
		}

		return false;
	}

	void capabilities::initialize()
	{
		int find_count = 18;
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

		for (int i = 0; i < ext_count; i++)
		{
			if (!find_count) break;

			const std::string ext_name = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));

			if (check(ext_name, "GL_ARB_shader_draw_parameters"))
			{
				ARB_shader_draw_parameters_supported = true;
				find_count--;
				continue;
			}

			if (check(ext_name, "GL_EXT_direct_state_access"))
			{
				EXT_dsa_supported = true;
				find_count--;
				continue;
			}

			if (check(ext_name, "GL_ARB_direct_state_access"))
			{
				ARB_dsa_supported = true;
				find_count--;
				continue;
			}

			if (check(ext_name, "GL_ARB_bindless_texture"))
			{
				ARB_bindless_texture_supported = true;
				find_count--;
				continue;
			}

			if (check(ext_name, "GL_ARB_buffer_storage"))
			{
				ARB_buffer_storage_supported = true;
				find_count--;
				continue;
			}

			if (check(ext_name, "GL_ARB_texture_buffer_object"))
			{
				ARB_texture_buffer_supported = true;
				find_count--;
				continue;
			}

			if (check(ext_name, "GL_ARB_depth_buffer_float"))
			{
				ARB_depth_buffer_float_supported = true;
				find_count--;
				continue;
			}

			if (check(ext_name, "GL_ARB_texture_barrier"))
			{
				ARB_texture_barrier_supported = true;
				find_count--;
				continue;
			}

			if (check(ext_name, "GL_NV_texture_barrier"))
			{
				NV_texture_barrier_supported = true;
				find_count--;
				continue;
			}

			if (check(ext_name, "GL_NV_gpu_shader5"))
			{
				NV_gpu_shader5_supported = true;
				find_count--;
				continue;
			}

			if (check(ext_name, "GL_AMD_gpu_shader_half_float"))
			{
				AMD_gpu_shader_half_float_supported = true;
				find_count--;
				continue;
			}

			if (check(ext_name, "GL_ARB_compute_shader"))
			{
				ARB_compute_shader_supported = true;
				find_count--;
				continue;
			}

			if (check(ext_name, "GL_EXT_depth_bounds_test"))
			{
				EXT_depth_bounds_test = true;
				find_count--;
				continue;
			}

			if (check(ext_name, "GL_NV_depth_buffer_float"))
			{
				NV_depth_buffer_float_supported = true;
				find_count--;
				continue;
			}

			if (check(ext_name, "GL_ARB_shader_stencil_export"))
			{
				ARB_shader_stencil_export_supported = true;
				find_count--;
				continue;
			}

			if (check(ext_name, "GL_NV_fragment_shader_barycentric"))
			{
				NV_fragment_shader_barycentric_supported = true;
				find_count--;
				continue;
			}

			if (check(ext_name, "GL_AMD_pinned_memory"))
			{
				AMD_pinned_memory = true;
				find_count--;
				continue;
			}

			if (check(ext_name, "GL_ARB_shader_texture_image_samples"))
			{
				ARB_shader_texture_image_samples = true;
				find_count--;
				continue;
			}
		}

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
				ARB_texture_buffer_supported = true;

			// Check for expected library entry-points for some required functions
			if (!ARB_buffer_storage_supported && glNamedBufferStorage && glMapNamedBufferRange)
				ARB_buffer_storage_supported = true;

			if (!ARB_dsa_supported && glGetTextureImage && glTextureBufferRange)
				ARB_dsa_supported = true;

			if (!EXT_dsa_supported && glGetTextureImageEXT && glTextureBufferRangeEXT)
				EXT_dsa_supported = true;
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
