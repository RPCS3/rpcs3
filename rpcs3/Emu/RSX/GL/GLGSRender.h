#pragma once
#include "Emu/RSX/GSRender.h"
#include "GLBuffers.h"
#include "GLProgramBuffer.h"

#pragma comment(lib, "opengl32.lib")

#define RSX_DEBUG 1

extern GLenum g_last_gl_error;
void printGlError(GLenum err, const char* situation);
void printGlError(GLenum err, const std::string& situation);
u32 LinearToSwizzleAddress(u32 x, u32 y, u32 z, u32 log2_width, u32 log2_height, u32 log2_depth);

#if RSX_DEBUG
#define checkForGlError(sit) if((g_last_gl_error = glGetError()) != GL_NO_ERROR) printGlError(g_last_gl_error, sit)
#else
#define checkForGlError(sit)
#endif

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

	void Init(RSXTexture& tex);
	void Save(RSXTexture& tex, const std::string& name);
	void Save(RSXTexture& tex);
	void Bind();
	void Unbind();
	void Delete();
};

class GSFrameBase
{
public:
	GSFrameBase() {}
	GSFrameBase(const GSFrameBase&) = delete;
	virtual void Close() = 0;

	virtual bool IsShown() = 0;
	virtual void Hide() = 0;
	virtual void Show() = 0;

	virtual void* GetNewContext() = 0;
	virtual void SetCurrent(void* ctx) = 0;
	virtual void DeleteContext(void* ctx) = 0;
	virtual void Flip(void* ctx) = 0;
	virtual sizei GetClientSize() = 0;
};

typedef GSFrameBase*(*GetGSFrameCb)();

void SetGetGSFrameCallback(GetGSFrameCb value);

class GLGSRender : public GSRender
{
private:
	std::vector<u8> m_vdata;

	GLTexture m_gl_textures[m_textures_count];
	GLTexture m_gl_vertex_textures[m_textures_count];

	GLvao m_vao;
	GLvbo m_vbo;
	gl::texture m_textures_color[4];
	gl::texture m_texture_depth;
	gl::fbo m_fbo;
	gl::fbo m_draw_buffer_fbo;
	gl::texture m_draw_buffer_color;
	gl::pbo m_pbo_color[4];
	gl::pbo m_pbo_depth;
	gl::glsl::program m_glsl_draw_texture_program;

	void* m_context;

public:
	GSFrameBase* m_frame;
	u32 m_draw_frames;
	u32 m_skip_frames;
	bool is_intel_vendor;
	
	GLGSRender();
	virtual ~GLGSRender();

private:
	void EnableVertexData(bool indexed_draw = false);
	void DisableVertexData();

	void Enable(bool enable, const u32 cap);
	virtual void Close();
	bool LoadProgram();
	void WriteBuffers();
	void ReadBuffers();

	void DrawObjects();
	void InitFBO();
	void SetupFBO();
	void InitDrawBuffers();

protected:
	void OnInit() override;
	void OnInitThread() override;
	void OnExitThread() override;
	void OnReset() override;
	void ExecCMD(u32 cmd) override;
	void ExecCMD() override;
	void Flip(int buffer) override;
};
