#pragma once
#include "Emu/RSX/GSRender.h"
#include "GLBuffers.h"
#include "gl_helpers.h"

#define RSX_DEBUG 1

#include "GLProgramBuffer.h"

#pragma comment(lib, "opengl32.lib")

class GLTexture
{
	u32 m_id = 0;

public:
	void create();

	int gl_wrap(int wrap);

	float max_aniso(int aniso);

	inline static u8 convert_4_to_8(u8 v)
	{
		// Swizzle bits: 00001234 -> 12341234
		return (v << 4) | (v);
	}

	inline static u8 convert_5_to_8(u8 v)
	{
		// Swizzle bits: 00012345 -> 12345123
		return (v << 3) | (v >> 2);
	}

	inline static u8 convert_6_to_8(u8 v)
	{
		// Swizzle bits: 00123456 -> 12345612
		return (v << 2) | (v >> 4);
	}

	void init(rsx::texture& tex);
	void save(rsx::texture& tex, const std::string& name);
	void save(rsx::texture& tex);
	void bind();
	void unbind();
	void remove();

	u32 id() const;
};

class GLGSRender : public GSRender
{
private:
	GLFragmentProgram m_fragment_prog;
	GLVertexProgram m_vertex_prog;

	GLTexture m_gl_textures[rsx::limits::textures_count];
	GLTexture m_gl_vertex_textures[rsx::limits::vertex_textures_count];

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

	void oninit_thread() override;
	void onexit_thread() override;
	bool domethod(u32 id, u32 arg) override;
	void flip(int buffer) override;
	u64 timestamp() const override;
};
