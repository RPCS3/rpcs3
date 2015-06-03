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

class GLGSRender : public GSRender
{
private:
	GLFragmentProgram m_fragment_prog;
	GLVertexProgram m_vertex_prog;

	GLTexture m_gl_textures[rsx::limits::textures_count];
	GLTexture m_gl_vertex_textures[rsx::limits::textures_count];

	void* m_context = nullptr;

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
	void read_buffers();
	void write_buffers();

protected:
	void oninit() override;
	void oninit_thread() override;
	void onexit_thread() override;
	void onreset() override;
	bool domethod(u32 id, u32 arg) override;
	void flip(int buffer) override;
};
