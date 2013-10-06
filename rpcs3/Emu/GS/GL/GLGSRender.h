#pragma once
#include "Emu/GS/GSRender.h"
#include "Emu/GS/RSXThread.h"
#include "wx/glcanvas.h"
#include "GLBuffers.h"
#include "Program.h"
#include "OpenGL.h"
#include "ProgramBuffer.h"

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "gl.lib")

void printGlError(GLenum err, const char* situation);
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

	u32 m_pitch;
	u16 m_depth;

	u16 m_minlod;
	u16 m_maxlod;
	u8 m_maxaniso;

	u8 m_wraps;
	u8 m_wrapt;
	u8 m_wrapr;
	u8 m_unsigned_remap;
	u8 m_zfunc;
	u8 m_gamma;
	u8 m_aniso_bias;
	u8 m_signed_remap;

	u16 m_bias;
	u8 m_min_filter;
	u8 m_mag_filter;
	u8 m_conv;
	u8 m_a_signed;
	u8 m_r_signed;
	u8 m_g_signed;
	u8 m_b_signed;

	u32 m_remap;
	
public:
	GLTexture()
		: m_width(0), m_height(0)
		, m_id(0)
		, m_offset(0)
		, m_enabled(false)

		, m_cubemap(false)
		, m_dimension(0)
		, m_format(0)
		, m_mipmap(0)
		, m_minlod(0)
		, m_maxlod(1000)
		, m_maxaniso(0)
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

			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		}
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

	void SetAddress(u8 wraps, u8 wrapt, u8 wrapr, u8 unsigned_remap, u8 zfunc, u8 gamma, u8 aniso_bias, u8 signed_remap)
	{
		m_wraps = wraps;
		m_wrapt = wrapt;
		m_wrapr = wrapr;
		m_unsigned_remap = unsigned_remap;
		m_zfunc = zfunc;
		m_gamma = gamma;
		m_aniso_bias = aniso_bias;
		m_signed_remap = signed_remap;
	}

	void SetControl0(const bool enable, const u16 minlod, const u16 maxlod, const u8 maxaniso)
	{
		m_enabled = enable;
		m_minlod = minlod;
		m_maxlod = maxlod;
		m_maxaniso = maxaniso;
	}

	void SetControl1(u32 remap)
	{
		m_remap = remap;
	}

	void SetControl3(u16 depth, u32 pitch)
	{
		m_depth = depth;
		m_pitch = pitch;
	}

	void SetFilter(u16 bias, u8 min, u8 mag, u8 conv, u8 a_signed, u8 r_signed, u8 g_signed, u8 b_signed)
	{
		m_bias = bias;
		m_min_filter = min;
		m_mag_filter = mag;
		m_conv = conv;
		m_a_signed = a_signed;
		m_r_signed = r_signed;
		m_g_signed = g_signed;
		m_b_signed = b_signed;
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

	void Init()
	{
		Bind();
		if(!Memory.IsGoodAddr(m_offset))
		{
			ConLog.Error("Bad texture address=0x%x", m_offset);
			return;
		}
		//ConLog.Warning("texture addr = 0x%x, width = %d, height = %d, max_aniso=%d, mipmap=%d, remap=0x%x, zfunc=0x%x, wraps=0x%x, wrapt=0x%x, wrapr=0x%x, minlod=0x%x, maxlod=0x%x", 
		//	m_offset, m_width, m_height, m_maxaniso, m_mipmap, m_remap, m_zfunc, m_wraps, m_wrapt, m_wrapr, m_minlod, m_maxlod);
		//TODO: safe init
		checkForGlError("GLTexture::Init() -> glBindTexture");

		glPixelStorei(GL_PACK_ROW_LENGTH, m_pitch);

		int format = m_format & ~(0x20 | 0x40);
		bool is_swizzled = (m_format & 0x20) == 0;

		char* pixels = (char*)Memory.GetMemFromAddr(m_offset);

		switch(format)
		{
		case 0x81:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_BLUE, GL_UNSIGNED_BYTE, pixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D");
			
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_BLUE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_BLUE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_BLUE);

			checkForGlError("GLTexture::Init() -> glTexParameteri");
		break;

		case 0x85:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, pixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D");
		break;

		case 0x86:
		{
			u32 size = ((m_width + 3) / 4) * ((m_height + 3) / 4) * 8;

			glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, m_width, m_height, 0, size, pixels);
			checkForGlError("GLTexture::Init() -> glCompressedTexImage2D");
		}
		break;

		case 0x87:
		{
			u32 size = ((m_width + 3) / 4) * ((m_height + 3) / 4) * 16;

			glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, m_width, m_height, 0, size, pixels);
			checkForGlError("GLTexture::Init() -> glCompressedTexImage2D");
		}
		break;

		case 0x88:
		{
			u32 size = ((m_width + 3) / 4) * ((m_height + 3) / 4) * 16;

			glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, m_width, m_height, 0, size, pixels);
			checkForGlError("GLTexture::Init() -> glCompressedTexImage2D");
		}
		break;
		
		case 0x94:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RED, GL_SHORT, pixels);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ONE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_ONE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ONE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);
			checkForGlError("GLTexture::Init() -> glTexImage2D");
		break;

		case 0x9a:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_BGRA, GL_HALF_FLOAT, pixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D");
		break;

		case 0x9e:
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, pixels);
			checkForGlError("GLTexture::Init() -> glTexImage2D");
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
		}
		break;

		default: ConLog.Error("Init tex error: Bad tex format (0x%x | 0x%x | 0x%x)", format, m_format & 0x20, m_format & 0x40); break;
		}

		if(m_mipmap > 1)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
		}

		if(format != 0x81 && format != 0x94)
		{
			u8 remap_a = m_remap & 0x3;
			u8 remap_r = (m_remap >> 2) & 0x3;
			u8 remap_g = (m_remap >> 4) & 0x3;
			u8 remap_b = (m_remap >> 6) & 0x3;

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

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GetGlWrap(m_wraps));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GetGlWrap(m_wrapt));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GetGlWrap(m_wrapr));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, gl_tex_zfunc[m_zfunc]);

		glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, m_bias);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, m_minlod);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, m_maxlod);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, m_maxaniso);

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

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_tex_filter[m_min_filter]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_tex_filter[m_mag_filter]);
		//Unbind();
	}

	void Save(const wxString& name)
	{
		if(!m_id || !m_offset || !m_width || !m_height) return;

		u32* alldata = new u32[m_width * m_height];

		Bind();

		switch(m_format & ~(0x20 | 0x40))
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
			f.Write(alldata, m_width * m_height * 4);
		}
		u8* data = new u8[m_width * m_height * 3];
		u8* alpha = new u8[m_width * m_height];

		u8* src = (u8*)alldata;
		u8* dst_d = data;
		u8* dst_a = alpha;
		for(u32 i=0; i<m_width*m_height;i++)
		{
			*dst_d++ = *src++;
			*dst_d++ = *src++;
			*dst_d++ = *src++;
			*dst_a++ = *src++;
		}

		wxImage out;
		out.Create(m_width, m_height, data, alpha);
		out.SaveFile(name, wxBITMAP_TYPE_PNG);

		free(alldata);
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

	bool IsEnabled() const { return m_enabled; }
};

struct TransformConstant
{
	u32 id;
	float x, y, z, w;

	TransformConstant()
		: x(0.0f)
		, y(0.0f)
		, z(0.0f)
		, w(0.0f)
	{
	}

	TransformConstant(u32 id, float x, float y, float z, float w)
		: id(id)
		, x(x)
		, y(y)
		, z(z)
		, w(w)
	{
	}
};

struct IndexArrayData
{
	Array<u8> m_data;
	int m_type;
	u32 m_first;
	u32 m_count;
	u32 m_addr;
	u32 index_max;
	u32 index_min;

	IndexArrayData()
	{
		Reset();
	}

	void Reset()
	{
		m_type = 0;
		m_first = ~0;
		m_count = 0;
		m_addr = 0;
		index_min = ~0;
		index_max = 0;
		m_data.Clear();
	}
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

struct GLRSXThread : public ThreadBase
{
	wxWindow* m_parent;
	Stack<u32> call_stack;

	GLRSXThread(wxWindow* parent);

	virtual void Task();
};

class GLGSRender
	: public wxWindow
	, public GSRender
	, public ExecRSXCMDdata
{
public:
	static const uint m_vertex_count = 16;
	static const uint m_fragment_count = 16;
	static const uint m_textures_count = 16;

private:
	GLRSXThread* m_rsx_thread;

	IndexArrayData m_indexed_array;

	ShaderProgram m_shader_progs[m_fragment_count];
	ShaderProgram* m_cur_shader_prog;
	int m_cur_shader_prog_num;
	VertexData m_vertex_data[m_vertex_count];
	Array<u8> m_vdata;
	VertexProgram m_vertex_progs[m_vertex_count];
	VertexProgram* m_cur_vertex_prog;
	Array<TransformConstant> m_transform_constants;
	Array<TransformConstant> m_fragment_constants;

	Program m_program;
	int m_fp_buf_num;
	int m_vp_buf_num;
	int m_draw_array_count;
	int m_draw_mode;
	ProgramBuffer m_prog_buffer;

	u32 m_width;
	u32 m_height;

	GLvao m_vao;
	GLvbo m_vbo;
	GLrbo m_rbo;
	GLfbo m_fbo;

public:
	GLGSFrame* m_frame;
	u32 m_draw_frames;
	u32 m_skip_frames;

	GLGSRender();
	~GLGSRender();

private:
	void EnableVertexData(bool indexed_draw=false);
	void DisableVertexData();
	void LoadVertexData(u32 first, u32 count);
	void InitVertexData();
	void InitFragmentData();

	void Enable(bool enable, const u32 cap);
	virtual void Init(const u32 ioAddress, const u32 ioSize, const u32 ctrlAddress, const u32 localAddress);
	virtual void Draw();
	virtual void Close();
	bool LoadProgram();
	void WriteDepthBuffer();

public:
	void DoCmd(const u32 fcmd, const u32 cmd, mem32_ptr_t& args, const u32 count);
	void CloseOpenGL();

	virtual void ExecCMD();
	virtual void Reset();
	void Init();
};