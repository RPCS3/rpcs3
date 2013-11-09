#pragma once
#include "Emu/GS/GSRender.h"
#include "Emu/GS/RSXThread.h"
#include "wx/glcanvas.h"
#include "GLBuffers.h"
#include "GLProgram.h"
#include "OpenGL.h"
#include "GLProgramBuffer.h"

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "gl.lib")

void printGlError(GLenum err, const char* situation);
void checkForGlError(const char* situation);

class GLTexture
{
	u32 m_id;
	
public:
	GLTexture() : m_id(0)
	{
	}

	void Create()
	{
		if(m_id)
		{
			Delete();
		}

		if(!m_id)
		{
			glGenTextures(1, &m_id);
			checkForGlError("GLTexture::Init() -> glGenTextures");
			Bind();
		}
	}

	int GetGlWrap(int wrap)
	{
		switch(wrap)
		{
		case 1: return GL_REPEAT;
		case 2: return GL_MIRRORED_REPEAT;
		case 3: return GL_CLAMP_TO_EDGE;
		case 4: return GL_TEXTURE_BORDER;
		case 5: return GL_CLAMP_TO_EDGE;//GL_CLAMP;
		//case 6: return GL_MIRROR_CLAMP_TO_EDGE_EXT;
		}

		ConLog.Error("Texture wrap error: bad wrap (%d).", wrap);
		return GL_REPEAT;
	}

	void Init(RSXTexture& tex)
	{
		Bind();
		if(!Memory.IsGoodAddr(tex.GetOffset()))
		{
			ConLog.Error("Bad texture address=0x%x", tex.GetOffset());
			return;
		}
		//ConLog.Warning("texture addr = 0x%x, width = %d, height = %d, max_aniso=%d, mipmap=%d, remap=0x%x, zfunc=0x%x, wraps=0x%x, wrapt=0x%x, wrapr=0x%x, minlod=0x%x, maxlod=0x%x", 
		//	m_offset, m_width, m_height, m_maxaniso, m_mipmap, m_remap, m_zfunc, m_wraps, m_wrapt, m_wrapr, m_minlod, m_maxlod);
		//TODO: safe init
		checkForGlError("GLTexture::Init() -> glBindTexture");

		int format = tex.GetFormat() & ~(0x20 | 0x40);
		bool is_swizzled = (tex.GetFormat() & 0x20) == 0;

		glPixelStorei(GL_PACK_ALIGNMENT, tex.m_pitch);
		char* pixels = (char*)Memory.GetMemFromAddr(tex.GetOffset());

		switch(format)
		{
		case 0x81:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.m_width, tex.m_height, 0, GL_BLUE, GL_UNSIGNED_BYTE, pixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D");
			
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_BLUE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_BLUE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_BLUE);

			checkForGlError("GLTexture::Init() -> glTexParameteri");
		break;

		case 0x85:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.m_width, tex.m_height, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, pixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D");
		break;

		case 0x86:
		{
			u32 size = ((tex.m_width + 3) / 4) * ((tex.m_height + 3) / 4) * 8;

			glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, tex.m_width, tex.m_height, 0, size, pixels);
			checkForGlError("GLTexture::Init() -> glCompressedTexImage2D");
		}
		break;

		case 0x87:
		{
			u32 size = ((tex.m_width + 3) / 4) * ((tex.m_height + 3) / 4) * 16;

			glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, tex.m_width, tex.m_height, 0, size, pixels);
			checkForGlError("GLTexture::Init() -> glCompressedTexImage2D");
		}
		break;

		case 0x88:
		{
			u32 size = ((tex.m_width + 3) / 4) * ((tex.m_height + 3) / 4) * 16;

			glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, tex.m_width, tex.m_height, 0, size, pixels);
			checkForGlError("GLTexture::Init() -> glCompressedTexImage2D");
		}
		break;
		
		case 0x94:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.m_width, tex.m_height, 0, GL_RED, GL_SHORT, pixels);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ONE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_ONE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ONE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);
			checkForGlError("GLTexture::Init() -> glTexImage2D");
		break;

		case 0x9a:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.m_width, tex.m_height, 0, GL_BGRA, GL_HALF_FLOAT, pixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D");
		break;

		case 0x9e:
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.m_width, tex.m_height, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, pixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D");
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
		}
		break;

		default: ConLog.Error("Init tex error: Bad tex format (0x%x | 0x%x | 0x%x)", format, tex.GetFormat() & 0x20, tex.GetFormat() & 0x40); break;
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, tex.m_mipmap - 1);
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, tex.m_mipmap > 1);

		if(format != 0x81 && format != 0x94)
		{
			u8 remap_a = tex.m_remap & 0x3;
			u8 remap_r = (tex.m_remap >> 2) & 0x3;
			u8 remap_g = (tex.m_remap >> 4) & 0x3;
			u8 remap_b = (tex.m_remap >> 6) & 0x3;

			static const int gl_remap[] =
			{
				GL_ALPHA,
				GL_RED,
				GL_GREEN,
				GL_BLUE,
			};

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, gl_remap[remap_a]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, gl_remap[remap_r]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, gl_remap[remap_g]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, gl_remap[remap_b]);
		}
		
		static const int gl_tex_zfunc[] =
		{
			GL_NEVER,
			GL_LESS,
			GL_EQUAL,
			GL_LEQUAL,
			GL_GREATER,
			GL_NOTEQUAL,
			GL_GEQUAL,
			GL_ALWAYS,
		};
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GetGlWrap(tex.m_wraps));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GetGlWrap(tex.m_wrapt));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GetGlWrap(tex.m_wrapr));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, gl_tex_zfunc[tex.m_zfunc]);
		
		glTexEnvi(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, tex.m_bias);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, tex.m_minlod);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, tex.m_maxlod);
		
		static const int gl_tex_filter[] =
		{
			GL_NEAREST,
			GL_NEAREST,
			GL_LINEAR,
			GL_NEAREST_MIPMAP_NEAREST,
			GL_LINEAR_MIPMAP_NEAREST,
			GL_NEAREST_MIPMAP_LINEAR,
			GL_LINEAR_MIPMAP_LINEAR,
			GL_NEAREST,
		};

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_tex_filter[tex.m_min_filter]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_tex_filter[tex.m_mag_filter]);
		//Unbind();
	}

	void Save(RSXTexture& tex, const wxString& name)
	{
		if(!m_id || !tex.m_offset || !tex.m_width || !tex.m_height) return;

		u32* alldata = new u32[tex.m_width * tex.m_height];

		Bind();

		switch(tex.m_format & ~(0x20 | 0x40))
		{
		case 0x81:
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, alldata);
		break;

		case 0x85:
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, alldata);
		break;

		default:
			delete[] alldata;
		return;
		}

		{
			wxFile f(name + ".raw", wxFile::write);
			f.Write(alldata, tex.m_width * tex.m_height * 4);
		}
		u8* data = new u8[tex.m_width * tex.m_height * 3];
		u8* alpha = new u8[tex.m_width * tex.m_height];

		u8* src = (u8*)alldata;
		u8* dst_d = data;
		u8* dst_a = alpha;
		for(u32 i=0; i<tex.m_width*tex.m_height;i++)
		{
			*dst_d++ = *src++;
			*dst_d++ = *src++;
			*dst_d++ = *src++;
			*dst_a++ = *src++;
		}

		wxImage out;
		out.Create(tex.m_width, tex.m_height, data, alpha);
		out.SaveFile(name, wxBITMAP_TYPE_PNG);

		free(alldata);
		//free(data);
		//free(alpha);
	}

	void Save(RSXTexture& tex)
	{
		static const wxString& dir_path = "textures";
		static const wxString& file_fmt = dir_path + "\\" + "tex[%d].png";

		if(!wxDirExists(dir_path)) wxMkDir(dir_path);

		u32 count = 0;
		while(wxFileExists(wxString::Format(file_fmt, count))) count++;
		Save(tex, wxString::Format(file_fmt, count));
	}

	void Bind()
	{
		glBindTexture(GL_TEXTURE_2D, m_id);
	}

	void Unbind()
	{
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void Delete()
	{
		if(m_id)
		{
			glDeleteTextures(1, &m_id);
			m_id = 0;
		}
	}
};

struct GLGSFrame : public GSFrame
{
	wxGLCanvas* canvas;
	u32 m_frames;

	GLGSFrame();
	~GLGSFrame() {}

	void Flip();

	wxGLCanvas* GetCanvas() const { return canvas; }

	virtual void SetViewport(int x, int y, u32 w, u32 h);

private:
	virtual void OnSize(wxSizeEvent& event);
};

class PostDrawObj
{
protected:
	GLShaderProgram m_fp;
	GLVertexProgram m_vp;
	GLProgram m_program;
	GLfbo m_fbo;
	GLrbo m_rbo;

public:
	virtual void Draw()
	{
		static bool s_is_initialized = false;

		if(!s_is_initialized)
		{
			s_is_initialized = true;
			Initialize();
		}
	}

	virtual void InitializeShaders() = 0;
	virtual void InitializeLocations() = 0;

	void Initialize()
	{
		InitializeShaders();
		m_fp.Compile();
		m_vp.Compile();
		m_program.Create(m_vp.id, m_fp.id);
		m_program.Use();
		InitializeLocations();
	}
};

class DrawCursorObj : PostDrawObj
{
	u32 m_tex_id;

public:
	DrawCursorObj() : m_tex_id(0)
	{
	}

	virtual void Draw()
	{
		PostDrawObj::Draw();
	}

	virtual void InitializeShaders()
	{
		m_vp.shader =
			"layout (location = 0) in vec4 in_pos;\n"
			"layout (location = 1) in vec2 in_tc;\n"
			"out vec2 tc;\n"
			"\n"
			"void main()\n"
			"{\n"
			"	tc = in_tc;\n"
			"	gl_Position = in_pos;\n"
			"}\n";

		m_fp.shader =
			"#version 330\n"
			"\n"
			"in vec2 tc;\n"
			"uniform sampler2D tex;\n"
			"layout (location = 0) out vec4 res;\n"
			"\n"
			"void main()\n"
			"{\n"
			"	res = texture(tex, tc);\n"
			"}\n";
	}

	void SetTexture(void* pixels, int width, int hight)
	{
		glUniform2i(1, width, hight);
		if(!m_tex_id)
		{
			glGenTextures(1, &m_tex_id);
		}

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_tex_id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, hight, 0, GL_BLUE, GL_UNSIGNED_BYTE, pixels);
		m_program.SetTex(0);
	}

	void SetPosition(float x, float y, float z = 0.0f)
	{
		glUniform4f(0, x, y, z, 1.0f);
	}

	void InitializeLocations()
	{
	}
};

class GLGSRender
	: public wxWindow
	, public GSRender
{
private:
	Array<u8> m_vdata;
	ArrayF<PostDrawObj> m_post_draw_objs;

	GLProgram m_program;
	int m_fp_buf_num;
	int m_vp_buf_num;
	GLProgramBuffer m_prog_buffer;

	GLShaderProgram m_shader_prog;
	GLVertexProgram m_vertex_prog;

	GLTexture m_gl_textures[m_textures_count];

	GLvao m_vao;
	GLvbo m_vbo;
	GLrbo m_rbo;
	GLfbo m_fbo;

	wxGLContext* m_context;

public:
	GLGSFrame* m_frame;
	u32 m_draw_frames;
	u32 m_skip_frames;

	GLGSRender();
	virtual ~GLGSRender();

private:
	void EnableVertexData(bool indexed_draw=false);
	void DisableVertexData();
	void LoadVertexData(u32 first, u32 count);
	void InitVertexData();
	void InitFragmentData();

	void Enable(bool enable, const u32 cap);
	virtual void Close();
	bool LoadProgram();
	void WriteDepthBuffer();
	void WriteColourBufferA();
	void WriteColourBufferB();
	void WriteColourBufferC();
	void WriteColourBufferD();
	void WriteBuffers();

	void DrawObjects();

protected:
	virtual void OnInit();
	virtual void OnInitThread();
	virtual void OnExitThread();
	virtual void OnReset();
	virtual void ExecCMD();
	virtual void Flip();
};