#pragma once
#include "Emu/RSX/GSRender.h"
#include "GLHelpers.h"
#include "GLTexture.h"
#include "GLTextureCache.h"
#include "GLRenderTargets.h"
#include "restore_new.h"
#include "define_new_memleakdetect.h"
#include "GLProgramBuffer.h"
#include "GLTextOut.h"
#include "GLOverlays.h"
#include "../rsx_utils.h"
#include "../rsx_cache.h"

#pragma comment(lib, "opengl32.lib")

namespace gl
{
	using vertex_cache = rsx::vertex_cache::default_vertex_cache<rsx::vertex_cache::uploaded_range<GLenum>, GLenum>;
	using weak_vertex_cache = rsx::vertex_cache::weak_vertex_cache<GLenum>;
	using null_vertex_cache = vertex_cache;

	using shader_cache = rsx::shaders_cache<void*, GLProgramBuffer>;
}

struct work_item
{
	std::condition_variable cv;
	std::mutex guard_mutex;

	u32  address_to_flush = 0;
	gl::texture_cache::thrashed_set section_data;

	volatile bool processed = false;
	volatile bool result = false;
	volatile bool received = false;
};

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

	const bool test_property(GLenum property, u32 test) const
	{
		auto found = properties.find(property);
		if (found == properties.end())
			return false;

		return (found->second == test);
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
		u32 value = (u32&)depth;
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
		u32 value = (u32&)stencil;
		if (!test_property(GL_STENCIL_CLEAR_VALUE, value))
		{
			glClearStencil(stencil);
			properties[GL_STENCIL_CLEAR_VALUE] = value;
		}
	}

	void color_mask(u32 mask)
	{
		if (!test_property(GL_COLOR_WRITEMASK, mask))
		{
			glColorMask(((mask & 0x20) ? 1 : 0), ((mask & 0x40) ? 1 : 0), ((mask & 0x80) ? 1 : 0), ((mask & 0x10) ? 1 : 0));
			properties[GL_COLOR_WRITEMASK] = mask;
		}
	}

	void color_mask(bool r, bool g, bool b, bool a)
	{
		u32 mask = 0;
		if (r) mask |= 0x20;
		if (g) mask |= 0x40;
		if (b) mask |= 0x80;
		if (a) mask |= 0x10;

		color_mask(mask);
	}

	void clear_color(u8 r, u8 g, u8 b, u8 a)
	{
		u32 value = (u32)r | (u32)g << 8 | (u32)b << 16 | (u32)a << 24;
		if (!test_property(GL_COLOR_CLEAR_VALUE, value))
		{
			glClearColor(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
			properties[GL_COLOR_CLEAR_VALUE] = value;
		}
	}

	void depth_bounds(float min, float max)
	{
		u32 depth_min = (u32&)min;
		u32 depth_max = (u32&)max;

		if (!test_property(DEPTH_BOUNDS_MIN, depth_min) || !test_property(DEPTH_BOUNDS_MAX, depth_max))
		{
			glDepthBoundsEXT(min, max);

			properties[DEPTH_BOUNDS_MIN] = depth_min;
			properties[DEPTH_BOUNDS_MAX] = depth_max;
		}
	}

	void depth_range(float min, float max)
	{
		u32 depth_min = (u32&)min;
		u32 depth_max = (u32&)max;

		if (!test_property(DEPTH_RANGE_MIN, depth_min) || !test_property(DEPTH_RANGE_MAX, depth_max))
		{
			glDepthRange(min, max);

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
		u32 value = (u32&)width;

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
		u32 _units = (u32&)units;
		u32 _factor = (u32&)factor;

		if (!test_property(GL_POLYGON_OFFSET_UNITS, _units) || !test_property(GL_POLYGON_OFFSET_FACTOR, _factor))
		{
			glPolygonOffset(factor, units);

			properties[GL_POLYGON_OFFSET_UNITS] = _units;
			properties[GL_POLYGON_OFFSET_FACTOR] = _factor;
		}
	}
};

class GLGSRender : public GSRender
{
private:
	GLFragmentProgram m_fragment_prog;
	GLVertexProgram m_vertex_prog;

	gl::sampler_state m_gl_sampler_states[rsx::limits::fragment_textures_count];

	gl::glsl::program *m_program;

	gl_render_targets m_rtts;

	gl::texture_cache m_gl_texture_cache;

	gl::texture m_gl_persistent_stream_buffer;
	gl::texture m_gl_volatile_stream_buffer;

	std::unique_ptr<gl::ring_buffer> m_attrib_ring_buffer;
	std::unique_ptr<gl::ring_buffer> m_fragment_constants_buffer;
	std::unique_ptr<gl::ring_buffer> m_transform_constants_buffer;
	std::unique_ptr<gl::ring_buffer> m_vertex_state_buffer;
	std::unique_ptr<gl::ring_buffer> m_index_ring_buffer;

	u32 m_draw_calls = 0;
	s64 m_begin_time = 0;
	s64 m_draw_time = 0;
	s64 m_vertex_upload_time = 0;
	s64 m_textures_upload_time = 0;

	std::unique_ptr<gl::vertex_cache> m_vertex_cache;
	std::unique_ptr<gl::shader_cache> m_shaders_cache;

	GLint m_min_texbuffer_alignment = 256;
	GLint m_uniform_buffer_offset_align = 256;

	bool manually_flush_ring_buffers = false;

	gl::text_writer m_text_printer;
	gl::depth_convert_pass m_depth_converter;
	gl::ui_overlay_renderer m_ui_renderer;

	std::mutex queue_guard;
	std::list<work_item> work_queue;

	bool flush_draw_buffers = false;
	std::thread::id m_thread_id;

	GLProgramBuffer m_prog_buffer;

	//buffer
	gl::fbo draw_fbo;
	gl::fbo m_flip_fbo;
	gl::texture m_flip_tex_color;

	//vaos are mandatory for core profile
	gl::vao m_vao;

	std::mutex m_sampler_mutex;
	u64 surface_store_tag = 0;
	std::atomic_bool m_samplers_dirty = {true};
	std::array<std::unique_ptr<rsx::sampled_image_descriptor_base>, rsx::limits::fragment_textures_count> fs_sampler_state = {};
	std::array<std::unique_ptr<rsx::sampled_image_descriptor_base>, rsx::limits::vertex_textures_count> vs_sampler_state = {};

public:
	GLGSRender();

private:

	driver_state gl_state;

	// Return element to draw and in case of indexed draw index type and offset in index buffer
	std::tuple<u32, u32, u32, std::optional<std::tuple<GLenum, u32> > > set_vertex_buffer();
	rsx::vertex_input_layout m_vertex_layout = {};

	void clear_surface(u32 arg);
	void init_buffers(rsx::framebuffer_creation_context context, bool skip_reading = false);

	bool check_program_state();
	void load_program(u32 vertex_base, u32 vertex_count);

	void update_draw_state();

public:
	void read_buffers();
	void write_buffers();
	void set_viewport();

	void synchronize_buffers();
	work_item& post_flush_request(u32 address, gl::texture_cache::thrashed_set& flush_data);

	bool scaled_image_from_memory(rsx::blit_src_info& src_info, rsx::blit_dst_info& dst_info, bool interpolate) override;

	void begin_occlusion_query(rsx::occlusion_query_info* query) override;
	void end_occlusion_query(rsx::occlusion_query_info* query) override;
	bool check_occlusion_query_status(rsx::occlusion_query_info* query) override;
	void get_occlusion_query_result(rsx::occlusion_query_info* query) override;

protected:
	void begin() override;
	void end() override;

	void on_init_thread() override;
	void on_exit() override;
	bool do_method(u32 id, u32 arg) override;
	void flip(int buffer) override;
	u64 timestamp() const override;

	void do_local_task(bool idle) override;

	bool on_access_violation(u32 address, bool is_writing) override;
	void on_notify_memory_unmapped(u32 address_base, u32 size) override;
	void notify_tile_unbound(u32 tile) override;

	std::array<std::vector<gsl::byte>, 4> copy_render_targets_to_memory() override;
	std::array<std::vector<gsl::byte>, 2> copy_depth_stencil_buffer_to_memory() override;

	void shell_do_cleanup() override;
};
