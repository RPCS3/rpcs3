#pragma once
#include "Emu/RSX/GSRender.h"
#include "GLBuffers.h"

#define RSX_DEBUG 1


#include "GLProgramBuffer.h"

#pragma comment(lib, "opengl32.lib")

#if RSX_DEBUG
#define checkForGlError(sit) if((g_last_gl_error = glGetError()) != GL_NO_ERROR) printGlError(g_last_gl_error, sit)
#else
#define checkForGlError(sit)
#endif

extern GLenum g_last_gl_error;
void printGlError(GLenum err, const char* situation);
void printGlError(GLenum err, const std::string& situation);


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

class PostDrawObj
{
protected:
	GLFragmentProgram m_fp;
	GLVertexProgram m_vp;
	GLProgram m_program;
	GLfbo m_fbo;
	GLrbo m_rbo;

public:
	virtual void Draw();

	virtual void InitializeShaders() = 0;
	virtual void InitializeLocations() = 0;

	void Initialize();
};

class DrawCursorObj : public PostDrawObj
{
	u32 m_tex_id;
	void* m_pixels;
	u32 m_width, m_height;
	double m_pos_x, m_pos_y, m_pos_z;
	bool m_update_texture, m_update_pos;

public:
	DrawCursorObj() : PostDrawObj()
		, m_tex_id(0)
		, m_update_texture(false)
		, m_update_pos(false)
	{
	}

	virtual void Draw();

	virtual void InitializeShaders();
	void SetTexture(void* pixels, int width, int height);
	void SetPosition(float x, float y, float z = 0.0f);
	void InitializeLocations();
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

};

typedef GSFrameBase*(*GetGSFrameCb)();

void SetGetGSFrameCallback(GetGSFrameCb value);

class GLGSRender //TODO: find out why this used to inherit from wxWindow
	: //public wxWindow
	/*,*/ public GSRender
{
private:
	std::vector<u8> m_vdata;
	std::vector<PostDrawObj> m_post_draw_objs;

	GLProgram m_program;
	int m_fp_buf_num;
	int m_vp_buf_num;
	GLProgramBuffer m_prog_buffer;

	GLFragmentProgram m_fragment_prog;
	GLVertexProgram m_vertex_prog;

	GLTexture m_gl_textures[m_textures_count];
	GLTexture m_gl_vertex_textures[m_textures_count];

	GLvao m_vao;
	GLvbo m_vbo;
	GLrbo m_rbo;
	GLfbo m_fbo;

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
	void InitVertexData();
	void InitFragmentData();

	void Enable(bool enable, const u32 cap);
	virtual void Close();
	bool LoadProgram();
	void WriteBuffers();
	void WriteDepthBuffer();
	void WriteColorBuffers();
	void WriteColorBufferA();
	void WriteColorBufferB();
	void WriteColorBufferC();
	void WriteColorBufferD();

	void DrawObjects();
	void InitDrawBuffers();

protected:
	void oninit() override;
	void oninit_thread() override;
	void onexit_thread() override;
	void onreset() override;
	bool domethod(u32 id, u32 arg) override;
	void flip(int buffer) override;
};
