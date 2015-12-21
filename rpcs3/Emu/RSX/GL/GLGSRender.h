#pragma once
#include "Emu/RSX/GSRender.h"
#include "gl_helpers.h"
#include "rsx_gl_texture.h"

#define RSX_DEBUG 1

#include "GLProgramBuffer.h"

#pragma comment(lib, "opengl32.lib")

class GLGSRender : public GSRender
{
private:
	GLFragmentProgram m_fragment_prog;
	GLVertexProgram m_vertex_prog;

	rsx::gl::texture m_gl_textures[rsx::limits::textures_count];
	rsx::gl::texture m_gl_vertex_textures[rsx::limits::vertex_textures_count];

	gl::glsl::program *m_program;

	rsx::surface_info m_surface;

public:
	gl::fbo draw_fbo;
	gl::buffers clear_surface_buffers = gl::buffers::none;

private:
	GLProgramBuffer m_prog_buffer;

	gl::texture m_draw_tex_color[rsx::limits::color_buffers_count];
	gl::texture m_draw_tex_depth_stencil;

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
	void init_buffers();
	void read_buffers();
	void write_buffers();

protected:
	void begin() override;
	void end() override;

	void on_init_thread() override;
	void on_exit() override;
	bool do_method(u32 id, u32 arg) override;
	void flip(int buffer) override;
	u64 timestamp() const override;
};
