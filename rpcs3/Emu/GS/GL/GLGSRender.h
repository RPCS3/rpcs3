#pragma once
#include "Emu/GS/GSRender.h"
#include "wx/glcanvas.h"
#include "GLBuffers.h"
#include "Program.h"
#include "OpenGL.h"

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "gl.lib")

void checkForGlError(const char* situation);

class GLTexture
{
	u32 m_width, m_height;
	u32 m_id;
	u32 m_offset;
	bool m_enabled;

	bool m_cubemap;
	u8 m_dimension;
	u32 m_format;
	u16 m_mipmap;
	
public:
	GLTexture()
		: m_width(0), m_height(0),
		m_id(0),
		m_offset(0),
		m_enabled(false),

		m_cubemap(false),
		m_dimension(0),
		m_format(0),
		m_mipmap(0)
	{
	}

	void SetRect(const u32 width, const u32 height)
	{
		m_width = width;
		m_height = height;
	}

	u32 GetOffset() const { return m_offset; }

	void SetFormat(const bool cubemap, const u8 dimension, const u32 format, const u16 mipmap)
	{
		m_cubemap = cubemap;
		m_dimension = dimension;
		m_format = format;
		m_mipmap = mipmap;
	}

	u32 GetFormat() const { return m_format; }

	void SetOffset(const u32 offset)
	{
		m_offset = offset;
	}

	wxSize GetRect() const
	{
		return wxSize(m_width, m_height);
	}

	void Init()
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		if(!m_id)
		{
			glGenTextures(1, &m_id);
			checkForGlError("GLTexture::Init() -> glGenTextures");
			glBindTexture(GL_TEXTURE_2D, m_id);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, m_id);
		}

		//TODO: safe init
		checkForGlError("GLTexture::Init() -> glBindTexture");

		switch(m_format)
		{
		case 0xA1:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RED, GL_UNSIGNED_BYTE, Memory.GetMemFromAddr(m_offset));
			checkForGlError("GLTexture::Init() -> glTexImage2D");
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);
			checkForGlError("GLTexture::Init() -> glTexParameteri");
		break;

		case 0xA5:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, Memory.GetMemFromAddr(m_offset));
			checkForGlError("GLTexture::Init() -> glTexImage2D");
		break;

		default: ConLog.Error("Init tex error: Bad tex format (0x%x)", m_format); break;
		}

		//glBindTexture(GL_TEXTURE_2D, 0);
	}

	void Save(const wxString& name)
	{
		if(!m_id || !m_offset) return;

		u8* data = new u8[m_width * m_height * 3];
		u8* alpha = new u8[m_width * m_height];
		u8* src = Memory.GetMemFromAddr(m_offset);
		u8* dst = data;
		u8* dst_a = alpha;

		switch(m_format)
		{
		case 0xA1:
			for(u32 y=0; y<m_height; ++y) for(u32 x=0; x<m_width; ++x)
			{
				*dst++ = *src;
				*dst++ = *src;
				*dst++ = *src;
				*dst_a++ = *src;
				src++;
			}
		break;

		case 0xA5:
			for(u32 y=0; y<m_height; ++y) for(u32 x=0; x<m_width; ++x)
			{
				*dst++ = *src++;
				*dst++ = *src++;
				*dst++ = *src++;
				*dst_a++ = *src++;
			}
		break;

		default: ConLog.Error("Save tex error: Bad tex format (0x%x)", m_format); break;
		}

		wxImage out;
		out.Create(m_width, m_height, data, alpha);
		out.SaveFile(name, wxBITMAP_TYPE_PNG);

		//free(data);
		//free(alpha);
	}

	void Save()
	{
		static const wxString& dir_path = "textures";
		static const wxString& file_fmt = dir_path + "\\" + "tex[%d].png";

		if(!wxDirExists(dir_path)) wxMkDir(dir_path);

		u32 count = 0;
		while(wxFileExists(wxString::Format(file_fmt, count))) count++;
		Save(wxString::Format(file_fmt, count));
	}

	void Bind()
	{
		glBindTexture(GL_TEXTURE_2D, m_id);
	}

	void Enable(bool enable) { m_enabled = enable; }
	bool IsEnabled() const { return m_enabled; }
};

struct GLGSFrame : public GSFrame
{
	wxGLCanvas* canvas;
	GLTexture m_textures[16];
	u32 m_frames;

	GLGSFrame();
	~GLGSFrame() {}

	void Flip();

	wxGLCanvas* GetCanvas() const { return canvas; }
	GLTexture& GetTexture(const u32 index) { return m_textures[index]; }

	virtual void SetViewport(int x, int y, u32 w, u32 h);

private:
	virtual void OnSize(wxSizeEvent& event);
};

extern gcmBuffer gcmBuffers[2];

struct GLRSXThread : public wxThread
{
	wxWindow* m_parent;
	Stack<u32> call_stack;

	volatile bool m_paused;

	GLRSXThread(wxWindow* parent);

	virtual void OnExit();
	void Start();
	ExitCode Entry();
};

class GLGSRender
	: public wxWindow
	, public GSRender
{
private:
	GLRSXThread* m_rsx_thread;

public:
	GLGSFrame* m_frame;
	volatile bool m_draw;

	GLGSRender();
	~GLGSRender();

private:
	void Enable(bool enable, const u32 cap);
	virtual void Init(const u32 ioAddress, const u32 ioSize, const u32 ctrlAddress, const u32 localAddress);
	virtual void Draw();
	virtual void Close();
	virtual void Pause();
	virtual void Resume();

public:
	void DoCmd(const u32 fcmd, const u32 cmd, mem32_t& args, const u32 count);
};