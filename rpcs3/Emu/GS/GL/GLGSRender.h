#pragma once
#include "Emu/GS/GSRender.h"
#include "Emu/GS/RSXThread.h"
#include "GLBuffers.h"
#include "GLProgramBuffer.h"
#include <wx/glcanvas.h>

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
		case 4: return GL_CLAMP_TO_BORDER;
		case 5: return GL_CLAMP_TO_EDGE;
		case 6: return GL_MIRROR_CLAMP_TO_EDGE_EXT;
		case 7: return GL_MIRROR_CLAMP_TO_BORDER_EXT;
		case 8: return GL_MIRROR_CLAMP_EXT;
		}

		ConLog.Error("Texture wrap error: bad wrap (%d).", wrap);
		return GL_REPEAT;
	}

	void Init(RSXTexture& tex)
	{
		Bind();
		if(!Memory.IsGoodAddr(GetAddress(tex.GetOffset(), tex.GetLocation())))
		{
			ConLog.Error("Bad texture address=0x%x", GetAddress(tex.GetOffset(), tex.GetLocation()));
			return;
		}
		//ConLog.Warning("texture addr = 0x%x, width = %d, height = %d, max_aniso=%d, mipmap=%d, remap=0x%x, zfunc=0x%x, wraps=0x%x, wrapt=0x%x, wrapr=0x%x, minlod=0x%x, maxlod=0x%x", 
		//	m_offset, m_width, m_height, m_maxaniso, m_mipmap, m_remap, m_zfunc, m_wraps, m_wrapt, m_wrapr, m_minlod, m_maxlod);
		//TODO: safe init
		checkForGlError("GLTexture::Init() -> glBindTexture");

		int format = tex.GetFormat() & ~(0x20 | 0x40);
		bool is_swizzled = !(tex.GetFormat() & CELL_GCM_TEXTURE_LN);

		glPixelStorei(GL_PACK_ALIGNMENT, tex.m_pitch);
		char* pixels = (char*)Memory.GetMemFromAddr(GetAddress(tex.GetOffset(), tex.GetLocation()));
		char* unswizzledPixels;

		switch(format)
		{
		case CELL_GCM_TEXTURE_B8:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.GetWidth(), tex.GetHeight(), 0, GL_BLUE, GL_UNSIGNED_BYTE, pixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D");
			
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_BLUE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_BLUE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_BLUE);

			checkForGlError("GLTexture::Init() -> glTexParameteri");
		break;

		case CELL_GCM_TEXTURE_A8R8G8B8:
			if(is_swizzled)
			{
				u32 *src, *dst;
				u32 log2width, log2height;

				unswizzledPixels = (char*)malloc(tex.GetWidth() * tex.GetHeight() * 4);
				src = (u32*)pixels;
				dst = (u32*)unswizzledPixels;

				log2width = log(tex.GetWidth())/log(2);
				log2height = log(tex.GetHeight())/log(2);

				for(int i=0; i<tex.GetHeight(); i++)
				{
					for(int j=0; j<tex.GetWidth(); j++)
					{
						dst[(i*tex.GetHeight()) + j] = src[LinearToSwizzleAddress(j, i, 0, log2width, log2height, 0)];
					}
				}
			}

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.GetWidth(), tex.GetHeight(), 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, is_swizzled ? unswizzledPixels : pixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D");
		break;

		case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
		{
			u32 size = ((tex.GetWidth() + 3) / 4) * ((tex.GetHeight() + 3) / 4) * 8;

			glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, tex.GetWidth(), tex.GetHeight(), 0, size, pixels);
			checkForGlError("GLTexture::Init() -> glCompressedTexImage2D");
		}
		break;

		case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
		{
			u32 size = ((tex.GetWidth() + 3) / 4) * ((tex.GetHeight() + 3) / 4) * 16;

			glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, tex.GetWidth(), tex.GetHeight(), 0, size, pixels);
			checkForGlError("GLTexture::Init() -> glCompressedTexImage2D");
		}
		break;

		case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
		{
			u32 size = ((tex.GetWidth() + 3) / 4) * ((tex.GetHeight() + 3) / 4) * 16;

			glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, tex.GetWidth(), tex.GetHeight(), 0, size, pixels);
			checkForGlError("GLTexture::Init() -> glCompressedTexImage2D");
		}
		break;
		
		case CELL_GCM_TEXTURE_X16:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.GetWidth(), tex.GetHeight(), 0, GL_RED, GL_SHORT, pixels);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ONE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_ONE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ONE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);
			checkForGlError("GLTexture::Init() -> glTexImage2D");
		break;

		case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.GetWidth(), tex.GetHeight(), 0, GL_BGRA, GL_HALF_FLOAT, pixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D");
		break;

		case CELL_GCM_TEXTURE_D8R8G8B8:
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.GetWidth(), tex.GetHeight(), 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, pixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D");
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
		}
		break;

		default: ConLog.Error("Init tex error: Bad tex format (0x%x | %s | 0x%x)", format,
					 (is_swizzled ? "swizzled" : "linear"), tex.GetFormat() & 0x40); break;
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, tex.GetMipmap() - 1);
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, tex.GetMipmap() > 1);

		if(format != CELL_GCM_TEXTURE_B8 && format != CELL_GCM_TEXTURE_X16)
		{
			u8 remap_a = tex.GetRemap() & 0x3;
			u8 remap_r = (tex.GetRemap() >> 2) & 0x3;
			u8 remap_g = (tex.GetRemap() >> 4) & 0x3;
			u8 remap_b = (tex.GetRemap() >> 6) & 0x3;

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
		
		checkForGlError("GLTexture::Init() -> remap");

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
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GetGlWrap(tex.GetWrapS()));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GetGlWrap(tex.GetWrapT()));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GetGlWrap(tex.GetWrapR()));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, gl_tex_zfunc[tex.GetZfunc()]);

		checkForGlError("GLTexture::Init() -> parameters1");
		
		glTexEnvi(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, tex.GetBias());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, (tex.GetMinLOD() >> 8));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, (tex.GetMaxLOD() >> 8));

		checkForGlError("GLTexture::Init() -> parameters2");
		
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

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_tex_filter[tex.GetMinFilter()]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_tex_filter[tex.GetMagFilter()]);

		checkForGlError("GLTexture::Init() -> filters");

		//Unbind();

		if(is_swizzled && format == CELL_GCM_TEXTURE_A8R8G8B8)
		{
			free(unswizzledPixels);
		}
	}

	void Save(RSXTexture& tex, const wxString& name)
	{
		if(!m_id || !tex.GetOffset() || !tex.GetWidth() || !tex.GetHeight()) return;

		u32* alldata = new u32[tex.GetWidth() * tex.GetHeight()];

		Bind();

		switch(tex.GetFormat() & ~(0x20 | 0x40))
		{
		case CELL_GCM_TEXTURE_B8:
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, alldata);
		break;

		case CELL_GCM_TEXTURE_A8R8G8B8:
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, alldata);
		break;

		default:
			delete[] alldata;
		return;
		}

		{
			wxFile f(name + ".raw", wxFile::write);
			f.Write(alldata, tex.GetWidth() * tex.GetHeight() * 4);
		}
		u8* data = new u8[tex.GetWidth() * tex.GetHeight() * 3];
		u8* alpha = new u8[tex.GetWidth() * tex.GetHeight()];

		u8* src = (u8*)alldata;
		u8* dst_d = data;
		u8* dst_a = alpha;
		for(u32 i=0; i<tex.GetWidth()*tex.GetHeight();i++)
		{
			*dst_d++ = *src++;
			*dst_d++ = *src++;
			*dst_d++ = *src++;
			*dst_a++ = *src++;
		}

		wxImage out;
		out.Create(tex.GetWidth(), tex.GetHeight(), data, alpha);
		out.SaveFile(name, wxBITMAP_TYPE_PNG);

		delete[] alldata;
		//free(data);
		//free(alpha);
	}

	void Save(RSXTexture& tex)
	{
		static const wxString& dir_path = "textures";
		static const wxString& file_fmt = dir_path + "/" + "tex[%d].png";

		if(!wxDirExists(dir_path)) wxMkdir(dir_path);

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

	void Flip(wxGLContext *context);

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
		else
		{
			m_program.Use();
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

	virtual void Draw()
	{
		checkForGlError("PostDrawObj : Unknown error.");

		PostDrawObj::Draw();
		checkForGlError("PostDrawObj::Draw");

		if(!m_fbo.IsCreated())
		{
			m_fbo.Create();
			checkForGlError("DrawCursorObj : m_fbo.Create");
			m_fbo.Bind();
			checkForGlError("DrawCursorObj : m_fbo.Bind");

			m_rbo.Create();
			checkForGlError("DrawCursorObj : m_rbo.Create");
			m_rbo.Bind();
			checkForGlError("DrawCursorObj : m_rbo.Bind");
			m_rbo.Storage(GL_RGBA, m_width, m_height);
			checkForGlError("DrawCursorObj : m_rbo.Storage");

			m_fbo.Renderbuffer(GL_COLOR_ATTACHMENT0, m_rbo.GetId());
			checkForGlError("DrawCursorObj : m_fbo.Renderbuffer");
		}

		m_fbo.Bind();
		checkForGlError("DrawCursorObj : m_fbo.Bind");
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		checkForGlError("DrawCursorObj : glDrawBuffer");

		m_program.Use();
		checkForGlError("DrawCursorObj : m_program.Use");

		if(m_update_texture)
		{
			//m_update_texture = false;

			glUniform2f(m_program.GetLocation("in_tc"), m_width, m_height);
			checkForGlError("DrawCursorObj : glUniform2f");
			if(!m_tex_id)
			{
				glGenTextures(1, &m_tex_id);
				checkForGlError("DrawCursorObj : glGenTextures");
			}

			glActiveTexture(GL_TEXTURE0);
			checkForGlError("DrawCursorObj : glActiveTexture");
			glBindTexture(GL_TEXTURE_2D, m_tex_id);
			checkForGlError("DrawCursorObj : glBindTexture");
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_pixels);
			checkForGlError("DrawCursorObj : glTexImage2D");
			m_program.SetTex(0);
		}

		if(m_update_pos)
		{
			//m_update_pos = false;

			glUniform4f(m_program.GetLocation("in_pos"), m_pos_x, m_pos_y, m_pos_z, 1.0f);
			checkForGlError("DrawCursorObj : glUniform4f");
		}

		glDrawArrays(GL_QUADS, 0, 4);
		checkForGlError("DrawCursorObj : glDrawArrays");

		m_fbo.Bind(GL_READ_FRAMEBUFFER);
		checkForGlError("DrawCursorObj : m_fbo.Bind(GL_READ_FRAMEBUFFER)");
		GLfbo::Bind(GL_DRAW_FRAMEBUFFER, 0);
		checkForGlError("DrawCursorObj : GLfbo::Bind(GL_DRAW_FRAMEBUFFER, 0)");
		GLfbo::Blit(
			0, 0, m_width, m_height,
			0, 0, m_width, m_height,
			GL_COLOR_BUFFER_BIT, GL_NEAREST);
		checkForGlError("DrawCursorObj : GLfbo::Blit");
		m_fbo.Bind();
		checkForGlError("DrawCursorObj : m_fbo.Bind");
	}

	virtual void InitializeShaders()
	{
		m_vp.shader =
			"#version 330\n"
			"\n"
			"uniform vec4 in_pos;\n"
			"uniform vec2 in_tc;\n"
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
			"uniform sampler2D tex0;\n"
			"layout (location = 0) out vec4 res;\n"
			"\n"
			"void main()\n"
			"{\n"
			"	res = texture(tex0, tc);\n"
			"}\n";
	}

	void SetTexture(void* pixels, int width, int height)
	{
		m_pixels = pixels;
		m_width = width;
		m_height = height;

		m_update_texture = true;
	}

	void SetPosition(float x, float y, float z = 0.0f)
	{
		m_pos_x = x;
		m_pos_y = y;
		m_pos_z = z;
		m_update_pos = true;
	}

	void InitializeLocations()
	{
		//ConLog.Warning("tex0 location = 0x%x", m_program.GetLocation("tex0"));
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
