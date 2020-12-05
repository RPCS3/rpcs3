#pragma once
#include "Utilities/types.h"
#include "Utilities/geometry.h"
#include "OpenGL.h"

#include <unordered_map>

namespace gl
{
	class capabilities
	{
	public:
		bool EXT_dsa_supported = false;
		bool EXT_depth_bounds_test = false;
		bool ARB_dsa_supported = false;
		bool ARB_buffer_storage_supported = false;
		bool ARB_texture_buffer_supported = false;
		bool ARB_shader_draw_parameters_supported = false;
		bool ARB_depth_buffer_float_supported = false;
		bool ARB_texture_barrier_supported = false;
		bool NV_texture_barrier_supported = false;
		bool NV_gpu_shader5_supported = false;
		bool AMD_gpu_shader_half_float_supported = false;
		bool ARB_compute_shader_supported = false;
		bool NV_depth_buffer_float_supported = false;
		bool initialized = false;
		bool vendor_INTEL = false;  // has broken GLSL compiler
		bool vendor_AMD = false;    // has broken ARB_multidraw
		bool vendor_NVIDIA = false; // has NaN poisoning issues
		bool vendor_MESA = false;   // requires CLIENT_STORAGE bit set for streaming buffers

		bool check(const std::string& ext_name, const char* test)
		{
			if (ext_name == test)
			{
				rsx_log.notice("Extension %s is supported", ext_name);
				return true;
			}

			return false;
		}

		void initialize()
		{
			int find_count = 13;
			int ext_count = 0;
			glGetIntegerv(GL_NUM_EXTENSIONS, &ext_count);
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
			}

			// Check GL_VERSION and GL_RENDERER for the presence of Mesa
			if (version_string.find("Mesa") != umax || renderer_string.find("Mesa") != umax)
			{
				vendor_MESA = true;
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

				//Texture buffers moved into core at GL 3.3
				if (version_major > 3 || (version_major == 3 && version_minor >= 3))
					ARB_texture_buffer_supported = true;

				//Check for expected library entry-points for some required functions
				if (!ARB_buffer_storage_supported && glBufferStorage && glMapBufferRange)
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
			}
#endif

			initialized = true;
		}
	};

	const capabilities& get_driver_caps();

	struct driver_state
	{
		const u32 DEPTH_BOUNDS_MIN = 0xFFFF0001;
		const u32 DEPTH_BOUNDS_MAX = 0xFFFF0002;
		const u32 DEPTH_RANGE_MIN = 0xFFFF0003;
		const u32 DEPTH_RANGE_MAX = 0xFFFF0004;

		std::unordered_map<GLenum, u32> properties = {};
		std::unordered_map<GLenum, std::array<u32, 4>> indexed_properties = {};

		bool enable(u32 test, GLenum cap)
		{
			auto found = properties.find(cap);
			if (found != properties.end() && found->second == test)
				return !!test;

			properties[cap] = test;

			if (test)
				glEnable(cap);
			else
				glDisable(cap);

			return !!test;
		}

		bool enablei(u32 test, GLenum cap, u32 index)
		{
			auto found = indexed_properties.find(cap);
			const bool exists = found != indexed_properties.end();

			if (!exists)
			{
				indexed_properties[cap] = {};
				indexed_properties[cap][index] = test;
			}
			else
			{
				if (found->second[index] == test)
					return !!test;

				found->second[index] = test;
			}

			if (test)
				glEnablei(cap, index);
			else
				glDisablei(cap, index);

			return !!test;
		}

		inline bool test_property(GLenum property, u32 test) const
		{
			auto found = properties.find(property);
			if (found == properties.end())
				return false;

			return (found->second == test);
		}

		inline bool test_propertyi(GLenum property, u32 test, GLint index) const
		{
			auto found = indexed_properties.find(property);
			if (found == indexed_properties.end())
				return false;

			return found->second[index] == test;
		}

		void depth_func(GLenum func)
		{
			if (!test_property(GL_DEPTH_FUNC, func))
			{
				glDepthFunc(func);
				properties[GL_DEPTH_FUNC] = func;
			}
		}

		void depth_mask(GLboolean mask)
		{
			if (!test_property(GL_DEPTH_WRITEMASK, mask))
			{
				glDepthMask(mask);
				properties[GL_DEPTH_WRITEMASK] = mask;
			}
		}

		void clear_depth(GLfloat depth)
		{
			u32 value = std::bit_cast<u32>(depth);
			if (!test_property(GL_DEPTH_CLEAR_VALUE, value))
			{
				glClearDepth(depth);
				properties[GL_DEPTH_CLEAR_VALUE] = value;
			}
		}

		void stencil_mask(GLuint mask)
		{
			if (!test_property(GL_STENCIL_WRITEMASK, mask))
			{
				glStencilMask(mask);
				properties[GL_STENCIL_WRITEMASK] = mask;
			}
		}

		void clear_stencil(GLint stencil)
		{
			u32 value = std::bit_cast<u32>(stencil);
			if (!test_property(GL_STENCIL_CLEAR_VALUE, value))
			{
				glClearStencil(stencil);
				properties[GL_STENCIL_CLEAR_VALUE] = value;
			}
		}

		void color_maski(GLint index, u32 mask)
		{
			if (!test_propertyi(GL_COLOR_WRITEMASK, mask, index))
			{
				glColorMaski(index, ((mask & 0x10) ? 1 : 0), ((mask & 0x20) ? 1 : 0), ((mask & 0x40) ? 1 : 0), ((mask & 0x80) ? 1 : 0));
				indexed_properties[GL_COLOR_WRITEMASK][index] = mask;
			}
		}

		void color_maski(GLint index, bool r, bool g, bool b, bool a)
		{
			u32 mask = 0;
			if (r) mask |= 0x10;
			if (g) mask |= 0x20;
			if (b) mask |= 0x40;
			if (a) mask |= 0x80;

			color_maski(index, mask);
		}

		void clear_color(u8 r, u8 g, u8 b, u8 a)
		{
			u32 value = u32{r} | u32{g} << 8 | u32{b} << 16 | u32{a} << 24;
			if (!test_property(GL_COLOR_CLEAR_VALUE, value))
			{
				glClearColor(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
				properties[GL_COLOR_CLEAR_VALUE] = value;
			}
		}

		void clear_color(const color4f& color)
		{
			clear_color(static_cast<u8>(color.r * 255), static_cast<u8>(color.g * 255), static_cast<u8>(color.b * 255), static_cast<u8>(color.a * 255));
		}

		void depth_bounds(float min, float max)
		{
			u32 depth_min = std::bit_cast<u32>(min);
			u32 depth_max = std::bit_cast<u32>(max);

			if (!test_property(DEPTH_BOUNDS_MIN, depth_min) || !test_property(DEPTH_BOUNDS_MAX, depth_max))
			{
				if (get_driver_caps().NV_depth_buffer_float_supported)
				{
					glDepthBoundsdNV(min, max);
				}
				else
				{
					glDepthBoundsEXT(min, max);
				}

				properties[DEPTH_BOUNDS_MIN] = depth_min;
				properties[DEPTH_BOUNDS_MAX] = depth_max;
			}
		}

		void depth_range(float min, float max)
		{
			u32 depth_min = std::bit_cast<u32>(min);
			u32 depth_max = std::bit_cast<u32>(max);

			if (!test_property(DEPTH_RANGE_MIN, depth_min) || !test_property(DEPTH_RANGE_MAX, depth_max))
			{
				if (get_driver_caps().NV_depth_buffer_float_supported)
				{
					glDepthRangedNV(min, max);
				}
				else
				{
					glDepthRange(min, max);
				}

				properties[DEPTH_RANGE_MIN] = depth_min;
				properties[DEPTH_RANGE_MAX] = depth_max;
			}
		}

		void logic_op(GLenum op)
		{
			if (!test_property(GL_COLOR_LOGIC_OP, op))
			{
				glLogicOp(op);
				properties[GL_COLOR_LOGIC_OP] = op;
			}
		}

		void line_width(GLfloat width)
		{
			u32 value = std::bit_cast<u32>(width);

			if (!test_property(GL_LINE_WIDTH, value))
			{
				glLineWidth(width);
				properties[GL_LINE_WIDTH] = value;
			}
		}

		void front_face(GLenum face)
		{
			if (!test_property(GL_FRONT_FACE, face))
			{
				glFrontFace(face);
				properties[GL_FRONT_FACE] = face;
			}
		}

		void cull_face(GLenum mode)
		{
			if (!test_property(GL_CULL_FACE_MODE, mode))
			{
				glCullFace(mode);
				properties[GL_CULL_FACE_MODE] = mode;
			}
		}

		void polygon_offset(float factor, float units)
		{
			u32 _units = std::bit_cast<u32>(units);
			u32 _factor = std::bit_cast<u32>(factor);

			if (!test_property(GL_POLYGON_OFFSET_UNITS, _units) || !test_property(GL_POLYGON_OFFSET_FACTOR, _factor))
			{
				glPolygonOffset(factor, units);

				properties[GL_POLYGON_OFFSET_UNITS] = _units;
				properties[GL_POLYGON_OFFSET_FACTOR] = _factor;
			}
		}
	};

	struct command_context
	{
		driver_state* drv;

		command_context()
			: drv(nullptr)
		{}

		command_context(driver_state& drv_)
			: drv(&drv_)
		{}
	};
}