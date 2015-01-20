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

	GLGSRender();
	virtual ~GLGSRender();

private:
	void EnableVertexData(bool indexed_draw=false);
	void DisableVertexData();
	void InitVertexData();
	void InitFragmentData();

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
	virtual void OnInit();
	virtual void OnInitThread();
	virtual void OnExitThread();
	virtual void OnReset();
	virtual void ExecCMD();
	virtual void Enable(u32 cmd, u32 enable);
	virtual void ClearColor(u32 a, u32 r, u32 g, u32 b);
	virtual void ClearStencil(u32 stencil);
	virtual void ClearDepth(u32 depth);
	virtual void ClearSurface(u32 mask);
	virtual void ColorMask(bool a, bool r, bool g, bool b);
	virtual void AlphaFunc(u32 func, float ref);
	virtual void DepthFunc(u32 func);
	virtual void DepthMask(u32 flag);
	virtual void PolygonMode(u32 face, u32 mode);
	virtual void PointSize(float size);
	virtual void LogicOp(u32 opcode);
	virtual void LineWidth(float width);
	virtual void LineStipple(u16 factor, u16 pattern);
	virtual void PolygonStipple(u32 pattern);
	virtual void PrimitiveRestartIndex(u32 index);
	virtual void CullFace(u32 mode);
	virtual void FrontFace(u32 mode);
	virtual void Fogi(u32 mode);
	virtual void Fogf(float start, float end);
	virtual void PolygonOffset(float factor, float bias);
	virtual void DepthRangef(float min, float max);
	virtual void BlendEquationSeparate(u16 rgb, u16 a);
	virtual void BlendFuncSeparate(u16 srcRGB, u16 dstRGB, u16 srcAlpha, u16 dstAlpha);
	virtual void BlendColor(u8 r, u8 g, u8 b, u8 a);
	virtual void LightModeli(u32 enable);
	virtual void ShadeModel(u32 mode);
	virtual void DepthBoundsEXT(float min, float max);
	virtual void Scissor(u16 x, u16 y, u16 width, u16 height);
	virtual void StencilOp(u32 fail, u32 zfail, u32 zpass);
	virtual void StencilMask(u32 mask);
	virtual void StencilFunc(u32 func, u32 ref, u32 mask);
	virtual void StencilOpSeparate(u32 mode, u32 fail, u32 zfail, u32 zpass);
	virtual void StencilMaskSeparate(u32 mode, u32 mask);
	virtual void StencilFuncSeparate(u32 mode, u32 func, u32 ref, u32 mask);
	virtual void Flip();
};
