#pragma once
#include "Emu/RSX/GSRender.h"
#include "gl_helpers.h"
#include "rsx_gl_texture.h"
#include "gl_texture_cache.h"
#include "gl_render_targets.h"

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
	gl_render_targets m_rtts;

	gl::gl_texture_cache m_gl_texture_cache;

	gl::texture m_gl_attrib_buffers[rsx::limits::vertex_count];
	std::unique_ptr<gl::ring_buffer> m_attrib_ring_buffer;
	std::unique_ptr<gl::ring_buffer> m_uniform_ring_buffer;
	std::unique_ptr<gl::ring_buffer> m_index_ring_buffer;

	u32 m_draw_calls = 0;
	u32 m_begin_time = 0;
	u32 m_draw_time = 0;
	u32 m_vertex_upload_time = 0;
	
	GLint m_min_texbuffer_alignment = 256;

public:
	gl::fbo draw_fbo;

private:
	GLProgramBuffer m_prog_buffer;

	//buffer
	gl::fbo m_flip_fbo;
	gl::texture m_flip_tex_color;

	//vaos are mandatory for core profile
	gl::vao m_vao;

public:
	GLGSRender();

private:
	static u32 enable(u32 enable, u32 cap);
	static u32 enable(u32 enable, u32 cap, u32 index);
	u32 set_vertex_buffer();

public:
	bool load_program();
	void init_buffers(bool skip_reading = false);
	void read_buffers();
	void write_buffers();
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

	virtual std::array<std::vector<gsl::byte>, 4> copy_render_targets_to_memory() override;
	virtual std::array<std::vector<gsl::byte>, 2> copy_depth_stencil_buffer_to_memory() override;
};
