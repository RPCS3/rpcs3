#pragma once
#include "Emu/RSX/GSRender.h"
#include "GLBuffers.h"
#include "gl_helpers.h"

#define RSX_DEBUG 1

#include "GLProgramBuffer.h"

#pragma comment(lib, "opengl32.lib")

class GLTexture
{
	u32 m_id;

public:
	GLTexture() : m_id(0)
	{
	}

	void Create();

	int GetGlWrap(int wrap);

	float GetMaxAniso(int aniso);

	inline static u8 Convert4To8(u8 v)
	{
		// Swizzle bits: 00001234 -> 12341234
		return (v << 4) | (v);
	}

	inline static u8 Convert5To8(u8 v)
	{
		// Swizzle bits: 00012345 -> 12345123
		return (v << 3) | (v >> 2);
	}

	inline static u8 Convert6To8(u8 v)
	{
		// Swizzle bits: 00123456 -> 12345612
		return (v << 2) | (v >> 4);
	}

	void Init(rsx::texture& tex);
	void Save(rsx::texture& tex, const std::string& name);
	void Save(rsx::texture& tex);
	void Bind();
	void Unbind();
	void Delete();
};

class GLGSRender : public GSRender
{
private:
	GLFragmentProgram m_fragment_prog;
	GLVertexProgram m_vertex_prog;

	GLTexture m_gl_textures[rsx::limits::textures_count];
	GLTexture m_gl_vertex_textures[rsx::limits::textures_count];

	void* m_context = nullptr;

	//TODO: program cache
	gl::glsl::program m_program;

	struct surface_info
	{
		u8 log2height;
		u8 log2width;
		u8 antialias;
		u8 depth_format;
		u8 color_format;

		u32 width;
		u32 height;
		u32 format;

		void unpack(u32 surface_format)
		{
			format = surface_format;

			log2height = surface_format >> 24;
			log2width = (surface_format >> 16) & 0xff;
			antialias = (surface_format >> 12) & 0xf;
			depth_format = (surface_format >> 5) & 0x7;
			color_format = surface_format & 0x1f;

			width = 1 << (u32(log2width) + 1);
			height = 1 << (u32(log2width) + 1);
		}
	};

	surface_info m_surface;
	gl::fbo m_draw_fbo;
	gl::texture m_draw_tex_color[rsx::limits::color_buffers_count];
	gl::texture m_draw_tex_depth_stencil;

	//buffer
	gl::fbo m_flip_fbo;
	gl::texture m_flip_tex_color;

public:
	GSFrameBase* m_frame = nullptr;
	u32 m_draw_frames;
	u32 m_skip_frames;
	bool is_intel_vendor;
	
	GLGSRender();
	virtual ~GLGSRender();

private:
	static void Enable(bool enable, const u32 cap);
	virtual void Close();

public:
	void load_vertex_data();
	void load_fragment_data();
	void load_indexes();

	bool load_program();
	void init_buffers();
	void read_buffers();
	void write_buffers();

protected:
	void begin() override;
	void end() override;

	void oninit() override;
	void oninit_thread() override;
	void onexit_thread() override;
	void onreset() override;
	bool domethod(u32 id, u32 arg) override;
	void flip(int buffer) override;
};
