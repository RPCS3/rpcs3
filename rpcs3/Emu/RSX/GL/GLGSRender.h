#pragma once
#include "Emu/RSX/GSRender.h"
#include "gl_helpers.h"
#include "rsx_gl_texture.h"
#include "gl_texture_cache.h"

#define RSX_DEBUG 1

#include "GLProgramBuffer.h"

#pragma comment(lib, "opengl32.lib")

struct texture_buffer_pair
{
	gl::texture texture;
	gl::buffer buffer;
};

class GLGSRender : public GSRender
{
private:
	GLFragmentProgram m_fragment_prog;
	GLVertexProgram m_vertex_prog;

	gl::glsl::program *m_program;

	rsx::surface_info m_surface;

	texture_buffer_pair m_gl_attrib_buffers[rsx::limits::vertex_count];

public:
	gl::texture_cache texture_cache;
	gl::fbo draw_fbo;

private:
	GLProgramBuffer m_prog_buffer;

	gl::texture m_draw_tex_color[rsx::limits::color_buffers_count];
	gl::texture m_draw_tex_depth_stencil;

public:
	gl::cached_texture *cached_color_buffers[rsx::limits::color_buffers_count];
	gl::cached_texture *cached_depth_buffer;

private:
	//buffer
	gl::fbo m_flip_fbo;
	gl::texture m_flip_tex_color;

	gl::buffer m_scale_offset_buffer;
	gl::buffer m_vertex_constants_buffer;
	gl::buffer m_fragment_constants_buffer;

	gl::buffer m_vbo;
	gl::buffer m_ebo;
	gl::vao m_vao;

public:
	GLGSRender();

private:
	static u32 enable(u32 enable, u32 cap);
	static u32 enable(u32 enable, u32 cap, u32 index);

public:
	bool load_program();
	void init_buffers(bool skip_reading = false);
	void invalidate_buffers();
	void set_viewport();

protected:
	void begin() override;
	void end() override;

	void on_init_thread() override;
	void on_exit() override;
	bool do_method(u32 id, u32 arg) override;
	void flip(int buffer) override;
	u64 timestamp() const override;

	bool on_access_violation(u32 address, bool is_writing) override;

public:
	void for_each_active_surface(std::function<void(gl::cached_texture& texture)> callback);
};
