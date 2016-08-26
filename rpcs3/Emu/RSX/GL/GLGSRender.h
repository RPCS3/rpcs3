#pragma once
#include "Emu/RSX/GSRender.h"
#include "gl_helpers.h"
#include "rsx_gl_texture.h"
#include "gl_texture_cache.h"
#include "gl_render_targets.h"
#include <Utilities/optional.hpp>
#include "GLProgramBuffer.h"

#pragma comment(lib, "opengl32.lib")

class GLGSRender : public GSRender
{
private:
	GLFragmentProgram m_fragment_prog;
	GLVertexProgram m_vertex_prog;

	rsx::gl::texture m_gl_textures[rsx::limits::fragment_textures_count];
	rsx::gl::texture m_gl_vertex_textures[rsx::limits::vertex_textures_count];

	gl::glsl::program *m_program;

	gl_render_targets m_rtts;

	gl::gl_texture_cache m_gl_texture_cache;

	gl::texture m_gl_attrib_buffers[rsx::limits::vertex_count];
	gl::ring_buffer m_attrib_ring_buffer;
	gl::ring_buffer m_uniform_ring_buffer;
	gl::ring_buffer m_index_ring_buffer;

	u32 m_draw_calls = 0;
	u32 m_begin_time = 0;
	u32 m_draw_time = 0;
	u32 m_vertex_upload_time = 0;
	
	GLint m_min_texbuffer_alignment = 256;
	GLint m_uniform_buffer_offset_align = 256;

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

	// Return element to draw and in case of indexed draw index type and offset in index buffer
	std::tuple<u32, std::optional<std::tuple<GLenum, u32> > > set_vertex_buffer();

	void upload_vertex_buffers(u32 min_index, u32 max_index, const u32& max_vertex_attrib_size, const u32& texture_index_offset);

	// Returns vertex count
	u32 upload_inline_array(const u32 &max_vertex_attrib_size, const u32 &texture_index_offset);

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
