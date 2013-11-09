#include "stdafx.h"
#include "GLGSRender.h"
#include "Emu/Cell/PPCInstrTable.h"

#define CMD_DEBUG 0
#define DUMP_VERTEX_DATA 0

#if	CMD_DEBUG
	#define CMD_LOG ConLog.Write
#else
	#define CMD_LOG(...)
#endif

gcmBuffer gcmBuffers[8];

int last_width = 0, last_height = 0, last_depth_format = 0;

void printGlError(GLenum err, const char* situation)
{
	if(err != GL_NO_ERROR)
	{
		ConLog.Error("%s: opengl error 0x%04x", situation, err);
		Emu.Pause();
	}
}

void checkForGlError(const char* situation)
{
	printGlError(glGetError(), situation);
}

#if 0
	#define checkForGlError(x) /*x*/
#endif

GLGSFrame::GLGSFrame()
	: GSFrame(nullptr, "GSFrame[OpenGL]")
	, m_frames(0)
{
	canvas = new wxGLCanvas(this, wxID_ANY, NULL);
	canvas->SetSize(GetClientSize());

	canvas->Connect(wxEVT_LEFT_DCLICK, wxMouseEventHandler(GSFrame::OnLeftDclick), (wxObject*)0, this);
}

void GLGSFrame::Flip()
{
	if(!canvas) return;
	canvas->SetCurrent();

	static Timer fps_t;
	canvas->SwapBuffers();
	m_frames++;

	if(fps_t.GetElapsedTimeInSec() >= 0.5)
	{
		SetTitle(wxString::Format("FPS: %.2f", (double)m_frames / fps_t.GetElapsedTimeInSec()));
		m_frames = 0;
		fps_t.Start();
	}
}

void GLGSFrame::OnSize(wxSizeEvent& event)
{
	if(canvas) canvas->SetSize(GetClientSize());
	event.Skip();
}

void GLGSFrame::SetViewport(int x, int y, u32 w, u32 h)
{
	/*
	//ConLog.Warning("SetViewport(x=%d, y=%d, w=%d, h=%d)", x, y, w, h);

	const wxSize client = GetClientSize();
	const wxSize viewport = AspectRatio(client, wxSize(w, h));

	const int vx = (client.GetX() - viewport.GetX()) / 2;
	const int vy = (client.GetY() - viewport.GetY()) / 2;

	glViewport(vx + x, vy + y, viewport.GetWidth(), viewport.GetHeight());
	*/
}

GLGSRender::GLGSRender()
	: GSRender()
	, m_frame(nullptr)
	, m_fp_buf_num(-1)
	, m_vp_buf_num(-1)
	, m_context(nullptr)
{
	m_frame = new GLGSFrame();
}

GLGSRender::~GLGSRender()
{
	m_frame->Close();
	delete m_context;
}

void GLGSRender::Enable(bool enable, const u32 cap)
{
	if(enable)
	{
		glEnable(cap);
	}
	else
	{
		glDisable(cap);
	}
}

extern CellGcmContextData current_context;

void GLGSRender::Close()
{
	if(IsAlive()) Stop();

	if(m_frame->IsShown()) m_frame->Hide();
	m_ctrl = nullptr;
}

void GLGSRender::EnableVertexData(bool indexed_draw)
{
	static u32 offset_list[m_vertex_count];
	u32 cur_offset = 0;

	for(u32 i=0; i<m_vertex_count; ++i)
	{
		offset_list[i] = cur_offset;

		if(!m_vertex_data[i].IsEnabled() || !m_vertex_data[i].addr) continue;

		cur_offset += m_vertex_data[i].data.GetCount();
		const u32 pos = m_vdata.GetCount();
		m_vdata.InsertRoomEnd(m_vertex_data[i].data.GetCount());
		memcpy(&m_vdata[pos], &m_vertex_data[i].data[0], m_vertex_data[i].data.GetCount());
	}

	m_vao.Create();
	m_vao.Bind();
	checkForGlError("initializing vao");

	m_vbo.Create(indexed_draw ? 2 : 1);
	m_vbo.Bind(0);
	m_vbo.SetData(&m_vdata[0], m_vdata.GetCount());

	if(indexed_draw)
	{
		m_vbo.Bind(GL_ELEMENT_ARRAY_BUFFER, 1);
		m_vbo.SetData(GL_ELEMENT_ARRAY_BUFFER, &m_indexed_array.m_data[0], m_indexed_array.m_data.GetCount());
	}

	checkForGlError("initializing vbo");

#if	DUMP_VERTEX_DATA
	wxFile dump("VertexDataArray.dump", wxFile::write);
#endif

	for(u32 i=0; i<m_vertex_count; ++i)
	{
		if(!m_vertex_data[i].IsEnabled()) continue;

#if	DUMP_VERTEX_DATA
		dump.Write(wxString::Format("VertexData[%d]:\n", i));
		switch(m_vertex_data[i].type)
		{
		case 1:
			for(u32 j = 0; j<m_vertex_data[i].data.GetCount(); j+=2)
			{
				dump.Write(wxString::Format("%d\n", *(u16*)&m_vertex_data[i].data[j]));
				if(!(((j+2) / 2) % m_vertex_data[i].size)) dump.Write("\n");
			}
		break;

		case 2:
			for(u32 j = 0; j<m_vertex_data[i].data.GetCount(); j+=4)
			{
				dump.Write(wxString::Format("%.01f\n", *(float*)&m_vertex_data[i].data[j]));
				if(!(((j+4) / 4) % m_vertex_data[i].size)) dump.Write("\n");
			}
		break;

		case 3:
			for(u32 j = 0; j<m_vertex_data[i].data.GetCount(); j+=2)
			{
				dump.Write(wxString::Format("%.01f\n", *(float*)&m_vertex_data[i].data[j]));
				if(!(((j+2) / 2) % m_vertex_data[i].size)) dump.Write("\n");
			}
		break;

		case 4:
			for(u32 j = 0; j<m_vertex_data[i].data.GetCount(); ++j)
			{
				dump.Write(wxString::Format("%d\n", m_vertex_data[i].data[j]));
				if(!((j+1) % m_vertex_data[i].size)) dump.Write("\n");
			}
		break;

		case 5:
			for(u32 j = 0; j<m_vertex_data[i].data.GetCount(); j+=2)
			{
				dump.Write(wxString::Format("%d\n", *(u16*)&m_vertex_data[i].data[j]));
				if(!(((j+2) / 2) % m_vertex_data[i].size)) dump.Write("\n");
			}
		break;

		case 7:
			for(u32 j = 0; j<m_vertex_data[i].data.GetCount(); ++j)
			{
				dump.Write(wxString::Format("%d\n", m_vertex_data[i].data[j]));
				if(!((j+1) % m_vertex_data[i].size)) dump.Write("\n");
			}
		break;

		default:
			ConLog.Error("Bad cv type! %d", m_vertex_data[i].type);
		return;
		}

		dump.Write("\n");
#endif

		static const u32 gl_types[] =
		{
			GL_SHORT,
			GL_FLOAT,
			GL_HALF_FLOAT,
			GL_UNSIGNED_BYTE,
			GL_SHORT,
			GL_UNSIGNED_BYTE,
		};

		static const bool gl_normalized[] =
		{
			true,
			false,
			false,
			true,
			false,
			false,
		};

		if(m_vertex_data[i].type >= 1 && m_vertex_data[i].type <= 7)
		{
			if(!m_vertex_data[i].addr)
			{
				switch(m_vertex_data[i].type)
				{
				case 5:
				case 1:
					switch(m_vertex_data[i].size)
					{
					case 1: glVertexAttrib1s(i, (GLshort&)m_vertex_data[i].data[0]); break;
					case 2: glVertexAttrib2sv(i, (GLshort*)&m_vertex_data[i].data[0]); break;
					case 3: glVertexAttrib3sv(i, (GLshort*)&m_vertex_data[i].data[0]); break;
					case 4: glVertexAttrib4sv(i, (GLshort*)&m_vertex_data[i].data[0]); break;
					}
				break;

				case 2:
					switch(m_vertex_data[i].size)
					{
					case 1: glVertexAttrib1f(i, (GLfloat&)m_vertex_data[i].data[0]); break;
					case 2: glVertexAttrib2fv(i, (GLfloat*)&m_vertex_data[i].data[0]); break;
					case 3: glVertexAttrib3fv(i, (GLfloat*)&m_vertex_data[i].data[0]); break;
					case 4: glVertexAttrib4fv(i, (GLfloat*)&m_vertex_data[i].data[0]); break;
					}
				break;

				case 6:
				case 4:
					glVertexAttrib4ubv(i, (GLubyte*)&m_vertex_data[i].data[0]);
				break;
				}

				checkForGlError("glVertexAttrib");
			}
			else
			{
				u32 gltype = gl_types[m_vertex_data[i].type - 1];
				bool normalized = gl_normalized[m_vertex_data[i].type - 1];

				glEnableVertexAttribArray(i);
				checkForGlError("glEnableVertexAttribArray");
				glVertexAttribPointer(i, m_vertex_data[i].size, gltype, normalized, 0, (void*)offset_list[i]);
				checkForGlError("glVertexAttribPointer");
			}
		}
	}
}

void GLGSRender::DisableVertexData()
{
	m_vdata.Clear();
	for(u32 i=0; i<m_vertex_count; ++i)
	{
		if(!m_vertex_data[i].IsEnabled() || !m_vertex_data[i].addr) continue;
		m_vertex_data[i].data.Clear();
		glDisableVertexAttribArray(i);
		checkForGlError("glDisableVertexAttribArray");
	}
	m_vao.Unbind();
}

void GLGSRender::LoadVertexData(u32 first, u32 count)
{
	for(u32 i=0; i<m_vertex_count; ++i)
	{
		if(!m_vertex_data[i].IsEnabled()) continue;

		m_vertex_data[i].Load(first, count);
	}
}

void GLGSRender::InitVertexData()
{
	for(u32 i=0; i<m_transform_constants.GetCount(); ++i)
	{
		const RSXTransformConstant& c = m_transform_constants[i];
		const wxString name = wxString::Format("vc%u", c.id);
		const int l = m_program.GetLocation(name);
		checkForGlError("glGetUniformLocation " + name);

		//ConLog.Write(name + " x: %.02f y: %.02f z: %.02f w: %.02f", c.x, c.y, c.z, c.w);
		glUniform4f(l, c.x, c.y, c.z, c.w);
		checkForGlError("glUniform4f " + name + wxString::Format(" %d [%f %f %f %f]", l, c.x, c.y, c.z, c.w));
	}
}

void GLGSRender::InitFragmentData()
{
	if(!m_cur_shader_prog)
	{
		ConLog.Error("InitFragmentData: m_cur_shader_prog == NULL");
		return;
	}

	for(u32 i=0; i<m_fragment_constants.GetCount(); ++i)
	{
		const RSXTransformConstant& c = m_fragment_constants[i];

		u32 id = c.id - m_cur_shader_prog->offset + 2 * 4 * 4;

		const wxString name = wxString::Format("fc%u", id);
		const int l = m_program.GetLocation(name);
		checkForGlError("glGetUniformLocation " + name);

		//ConLog.Write(name + " x: %.02f y: %.02f z: %.02f w: %.02f", c.x, c.y, c.z, c.w);
		glUniform4f(l, c.x, c.y, c.z, c.w);
		checkForGlError("glUniform4f " + name + wxString::Format(" %d [%f %f %f %f]", l, c.x, c.y, c.z, c.w));
	}
}

bool GLGSRender::LoadProgram()
{
	if(!m_cur_shader_prog)
	{
		ConLog.Warning("LoadProgram: m_cur_shader_prog == NULL");

		return false;
	}

	m_fp_buf_num = m_prog_buffer.SearchFp(*m_cur_shader_prog, m_shader_prog);

	if(m_fp_buf_num == -1) m_shader_prog.Decompile(*m_cur_shader_prog);

	if(!m_cur_vertex_prog)
	{
		ConLog.Warning("LoadProgram: m_cur_vertex_prog == NULL");

		return false;
	}

	//ConLog.Write("Create program");
	m_vp_buf_num = m_prog_buffer.SearchVp(*m_cur_vertex_prog, m_vertex_prog);

	if(m_vp_buf_num == -1) 
	{
		ConLog.Warning("VP not found in buffer!");
		m_vertex_prog.Decompile(*m_cur_vertex_prog);
	}

	if(m_fp_buf_num == -1)
	{
		ConLog.Warning("FP not found in buffer!");
		m_shader_prog.Wait();
		m_shader_prog.Compile();
		checkForGlError("m_shader_prog.Compile");

		wxFile f(wxGetCwd() + "/FragmentProgram.txt", wxFile::write);
		f.Write(m_shader_prog.shader);
	}

	if(m_vp_buf_num == -1)
	{
		m_vertex_prog.Wait();
		m_vertex_prog.Compile();
		checkForGlError("m_vertex_prog.Compile");

		wxFile f(wxGetCwd() + "/VertexProgram.txt", wxFile::write);
		f.Write(m_vertex_prog.shader);
	}

	if(m_fp_buf_num != -1 && m_vp_buf_num != -1)
	{
		m_program.id = m_prog_buffer.GetProg(m_fp_buf_num, m_vp_buf_num);
	}

	if(m_program.id)
	{
		m_program.Use();
	}
	else
	{
		m_program.Create(m_vertex_prog.id, m_shader_prog.id);
		checkForGlError("m_program.Create");
		m_prog_buffer.Add(m_program, m_shader_prog, *m_cur_shader_prog, m_vertex_prog, *m_cur_vertex_prog);
		checkForGlError("m_prog_buffer.Add");
		m_program.Use();

		GLint r = GL_FALSE;
		glGetProgramiv(m_program.id, GL_VALIDATE_STATUS, &r);
		if(r != GL_TRUE)
		{
			glGetProgramiv(m_program.id, GL_INFO_LOG_LENGTH, &r);

			if(r)
			{
				char* buf = new char[r+1];
				GLsizei len;
				memset(buf, 0, r+1);
				glGetProgramInfoLog(m_program.id, r, &len, buf);
				ConLog.Error("Failed to validate program: %s", buf);
				delete[] buf;
			}

			Emu.Pause();
		}
	}

	return true;
}

void GLGSRender::WriteDepthBuffer()
{
	if(!m_set_context_dma_z)
	{
		return;
	}

	u32 address = GetAddress(m_surface_offset_z, m_context_dma_z - 0xfeed0000);
	if(!Memory.IsGoodAddr(address))
	{
		ConLog.Warning("Bad depth address: address=0x%x, offset=0x%x, dma=0x%x", address, m_surface_offset_z, m_context_dma_z);
		return;
	}

	glReadPixels(0, 0, m_width, m_height, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, &Memory[address]);
	checkForGlError("glReadPixels");

	GLuint depth_tex;
	glGenTextures(1, &depth_tex);
	glBindTexture(GL_TEXTURE_2D, depth_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, &Memory[address]);
	checkForGlError("glTexImage2D");
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &Memory[address]);
	checkForGlError("glGetTexImage");
	glDeleteTextures(1, &depth_tex);
}

void GLGSRender::WriteColourBufferA()
{
	if(!m_set_context_dma_color_a)
	{
		return;
	}

	u32 address = GetAddress(m_surface_offset_a, m_context_dma_color_a - 0xfeed0000);
	if(!Memory.IsGoodAddr(address))
	{
		ConLog.Warning("Bad colour buffer a address: address=0x%x, offset=0x%x, dma=0x%x", address, m_surface_offset_a, m_context_dma_color_a);
		return;
	}

	glReadBuffer(GL_COLOR_ATTACHMENT0);
	checkForGlError("glReadBuffer(GL_COLOR_ATTACHMENT0)");
	glReadPixels(0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &Memory[address]);
	checkForGlError("glReadPixels(GL_RGBA, GL_UNSIGNED_INT_8_8_8_8)");
}

void GLGSRender::WriteColourBufferB()
{
	if(!m_set_context_dma_color_b)
	{
		return;
	}

	u32 address = GetAddress(m_surface_offset_b, m_context_dma_color_b - 0xfeed0000);
	if(!Memory.IsGoodAddr(address))
	{
		ConLog.Warning("Bad colour buffer b address: address=0x%x, offset=0x%x, dma=0x%x", address, m_surface_offset_b, m_context_dma_color_b);
		return;
	}

	glReadBuffer(GL_COLOR_ATTACHMENT1);
	checkForGlError("glReadBuffer(GL_COLOR_ATTACHMENT1)");
	glReadPixels(0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &Memory[address]);
	checkForGlError("glReadPixels(GL_RGBA, GL_UNSIGNED_INT_8_8_8_8)");
}

void GLGSRender::WriteColourBufferC()
{
	if(!m_set_context_dma_color_c)
	{
		return;
	}

	u32 address = GetAddress(m_surface_offset_c, m_context_dma_color_c - 0xfeed0000);
	if(!Memory.IsGoodAddr(address))
	{
		ConLog.Warning("Bad colour buffer c address: address=0x%x, offset=0x%x, dma=0x%x", address, m_surface_offset_c, m_context_dma_color_c);
		return;
	}

	glReadBuffer(GL_COLOR_ATTACHMENT2);
	checkForGlError("glReadBuffer(GL_COLOR_ATTACHMENT2)");
	glReadPixels(0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &Memory[address]);
	checkForGlError("glReadPixels(GL_RGBA, GL_UNSIGNED_INT_8_8_8_8)");
}

void GLGSRender::WriteColourBufferD()
{
	if(!m_set_context_dma_color_d)
	{
		return;
	}

	u32 address = GetAddress(m_surface_offset_d, m_context_dma_color_d - 0xfeed0000);
	if(!Memory.IsGoodAddr(address))
	{
		ConLog.Warning("Bad colour buffer d address: address=0x%x, offset=0x%x, dma=0x%x", address, m_surface_offset_d, m_context_dma_color_d);
		return;
	}

	glReadBuffer(GL_COLOR_ATTACHMENT3);
	checkForGlError("glReadBuffer(GL_COLOR_ATTACHMENT3)");
	glReadPixels(0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &Memory[address]);
	checkForGlError("glReadPixels(GL_RGBA, GL_UNSIGNED_INT_8_8_8_8)");
}

void GLGSRender::WriteBuffers()
{
	glPixelStorei(GL_PACK_ROW_LENGTH, 0);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);

	switch(m_surface_colour_target)
	{
	case 0x0:
		return;

	case 0x1:
		WriteColourBufferA();
	break;

	case 0x2:
		WriteColourBufferB();
	break;

	case 0x13:
		WriteColourBufferA();
		WriteColourBufferB();
	break;

	case 0x17:
		WriteColourBufferA();
		WriteColourBufferB();
		WriteColourBufferC();
	break;

	case 0x1f:
		WriteColourBufferA();
		WriteColourBufferB();
		WriteColourBufferC();
		WriteColourBufferD();
	break;
	}
}

void GLGSRender::OnInit()
{
	m_draw_frames = 1;
	m_skip_frames = 0;
	m_width = 720;
	m_height = 576;

	last_width = 0;
	last_height = 0;
	last_depth_format = 0;

	m_frame->Show();
}

void GLGSRender::OnInitThread()
{
	m_context = new wxGLContext(m_frame->GetCanvas());

	m_frame->GetCanvas()->SetCurrent(*m_context);
	InitProcTable();

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_SCISSOR_TEST);
	glSwapInterval(Ini.GSVSyncEnable.GetValue() ? 1 : 0);
}

void GLGSRender::OnExitThread()
{
	m_program.Delete();
	m_rbo.Delete();
	m_fbo.Delete();
	m_vbo.Delete();
	m_vao.Delete();
	m_prog_buffer.Clear();
}

void GLGSRender::OnReset()
{
	m_program.UnUse();

	//m_shader_prog.id = 0;
	//m_vertex_prog.id = 0;

	if(m_vbo.IsCreated())
	{
		m_vbo.UnBind();
		m_vbo.Delete();
	}

	m_vao.Delete();
}

void GLGSRender::ExecCMD()
{
	if(!LoadProgram())
	{
		ConLog.Error("LoadProgram failed.");
		Emu.Pause();
		return;
	}

	if(!m_fbo.IsCreated() || m_width != last_width || m_height != last_height || last_depth_format != m_surface_depth_format)
	{
		ConLog.Warning("New FBO (%dx%d)", m_width, m_height);
		last_width = m_width;
		last_height = m_height;
		last_depth_format = m_surface_depth_format;

		m_fbo.Create();
		checkForGlError("m_fbo.Create");
		m_fbo.Bind();

		m_rbo.Create(4 + 1);
		checkForGlError("m_rbo.Create");

		for(int i=0; i<4; ++i)
		{
			m_rbo.Bind(i);
			m_rbo.Storage(GL_RGBA, m_width, m_height);
			checkForGlError("m_rbo.Storage(GL_RGBA)");
		}

		m_rbo.Bind(4);

		switch(m_surface_depth_format)
		{
		case 1:
			m_rbo.Storage(GL_DEPTH_COMPONENT16, m_width, m_height);
			checkForGlError("m_rbo.Storage(GL_DEPTH_COMPONENT16)");
		break;

		case 2:
			m_rbo.Storage(GL_DEPTH24_STENCIL8, m_width, m_height);
			checkForGlError("m_rbo.Storage(GL_DEPTH24_STENCIL8)");
		break;

		default:
			ConLog.Error("Bad depth format! (%d)", m_surface_depth_format);
			assert(0);
		break;
		}

		for(int i=0; i<4; ++i)
		{
			m_fbo.Renderbuffer(GL_COLOR_ATTACHMENT0 + i, m_rbo.GetId(i));
			checkForGlError(wxString::Format("m_fbo.Renderbuffer(GL_COLOR_ATTACHMENT%d)", i));
		}

		m_fbo.Renderbuffer(GL_DEPTH_ATTACHMENT, m_rbo.GetId(4));
		checkForGlError("m_fbo.Renderbuffer(GL_DEPTH_ATTACHMENT)");

		if(m_surface_depth_format == 2)
		{
			m_fbo.Renderbuffer(GL_STENCIL_ATTACHMENT, m_rbo.GetId(4));
			checkForGlError("m_fbo.Renderbuffer(GL_STENCIL_ATTACHMENT)");
		}
	}

	if(!m_set_surface_clip_horizontal)
	{
		m_surface_clip_x = 0;
		m_surface_clip_w = m_width;
	}

	if(!m_set_surface_clip_vertical)
	{
		m_surface_clip_y = 0;
		m_surface_clip_h = m_height;
	}
		
	m_fbo.Bind();
	WriteDepthBuffer();
	static const GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };

	switch(m_surface_colour_target)
	{
	case 0x0:
		break;

	case 0x1:
		glDrawBuffer(draw_buffers[0]);
	break;

	case 0x2:
		glDrawBuffer(draw_buffers[1]);
	break;

	case 0x13:
		glDrawBuffers(2, draw_buffers);
	break;

	case 0x17:
		glDrawBuffers(3, draw_buffers);
	break;

	case 0x1f:
		glDrawBuffers(4, draw_buffers);
	break;

	default:
		ConLog.Error("Bad surface colour target: %d", m_surface_colour_target);
	break;
	}

	if(m_set_color_mask)
	{
		glColorMask(m_color_mask_r, m_color_mask_g, m_color_mask_b, m_color_mask_a);
		checkForGlError("glColorMask");
	}

	if(m_set_viewport_horizontal && m_set_viewport_vertical)
	{
		glViewport(m_viewport_x, m_height-m_viewport_y-m_viewport_h, m_viewport_w, m_viewport_h);
		checkForGlError("glViewport");
	}

	if(m_set_scissor_horizontal && m_set_scissor_vertical)
	{
		glScissor(m_scissor_x, m_height-m_scissor_y-m_scissor_h, m_scissor_w, m_scissor_h);
		checkForGlError("glScissor");
	}

	if(m_clear_surface_mask)
	{
		GLbitfield f = 0;

		if (m_clear_surface_mask & 0x1)
		{
			glClearDepth(m_clear_surface_z / (float)0xffffff);

			f |= GL_DEPTH_BUFFER_BIT;
		}

		if (m_clear_surface_mask & 0x2)
		{
			glClearStencil(m_clear_surface_s);

			f |= GL_STENCIL_BUFFER_BIT;
		}

		if (m_clear_surface_mask & 0xF0)
		{
			glClearColor(
				m_clear_surface_color_r / 255.0f,
				m_clear_surface_color_g / 255.0f,
				m_clear_surface_color_b / 255.0f,
				m_clear_surface_color_a / 255.0f);

			f |= GL_COLOR_BUFFER_BIT;
		}

		glClear(f);
	}

	if(m_set_front_polygon_mode)
	{
		glPolygonMode(GL_FRONT, m_front_polygon_mode);
		checkForGlError("glPolygonMode");
	}

	Enable(m_depth_test_enable, GL_DEPTH_TEST);
	Enable(m_set_alpha_test, GL_ALPHA_TEST);
	Enable(m_set_depth_bounds_test, GL_DEPTH_CLAMP);
	Enable(m_set_blend, GL_BLEND);
	Enable(m_set_logic_op, GL_LOGIC_OP);
	Enable(m_set_cull_face_enable, GL_CULL_FACE);
	Enable(m_set_dither, GL_DITHER);
	Enable(m_set_stencil_test, GL_STENCIL_TEST);
	Enable(m_set_line_smooth, GL_LINE_SMOOTH);
	Enable(m_set_poly_smooth, GL_POLYGON_SMOOTH);

	if(m_set_clip_plane)
	{
		Enable(m_clip_plane_0, GL_CLIP_PLANE0);
		Enable(m_clip_plane_1, GL_CLIP_PLANE1);
		Enable(m_clip_plane_2, GL_CLIP_PLANE2);
		Enable(m_clip_plane_3, GL_CLIP_PLANE3);
		Enable(m_clip_plane_4, GL_CLIP_PLANE4);
		Enable(m_clip_plane_5, GL_CLIP_PLANE5);

		checkForGlError("m_set_clip_plane");
	}

	checkForGlError("glEnable");

	if(m_set_two_sided_stencil_test_enable)
	{
		if(m_set_stencil_fail && m_set_stencil_zfail && m_set_stencil_zpass)
		{
			glStencilOpSeparate(GL_FRONT, m_stencil_fail, m_stencil_zfail, m_stencil_zpass);
			checkForGlError("glStencilOpSeparate");
		}

		if(m_set_stencil_mask)
		{
			glStencilMaskSeparate(GL_FRONT, m_stencil_mask);
			checkForGlError("glStencilMaskSeparate");
		}

		if(m_set_stencil_func && m_set_stencil_func_ref && m_set_stencil_func_mask)
		{
			glStencilFuncSeparate(GL_FRONT, m_stencil_func, m_stencil_func_ref, m_stencil_func_mask);
			checkForGlError("glStencilFuncSeparate");
		}

		if(m_set_back_stencil_fail && m_set_back_stencil_zfail && m_set_back_stencil_zpass)
		{
			glStencilOpSeparate(GL_BACK, m_back_stencil_fail, m_back_stencil_zfail, m_back_stencil_zpass);
			checkForGlError("glStencilOpSeparate(GL_BACK)");
		}

		if(m_set_back_stencil_mask)
		{
			glStencilMaskSeparate(GL_BACK, m_back_stencil_mask);
			checkForGlError("glStencilMaskSeparate(GL_BACK)");
		}

		if(m_set_back_stencil_func && m_set_back_stencil_func_ref && m_set_back_stencil_func_mask)
		{
			glStencilFuncSeparate(GL_BACK, m_back_stencil_func, m_back_stencil_func_ref, m_back_stencil_func_mask);
			checkForGlError("glStencilFuncSeparate(GL_BACK)");
		}
	}
	else
	{
		if(m_set_stencil_fail && m_set_stencil_zfail && m_set_stencil_zpass)
		{
			glStencilOp(m_stencil_fail, m_stencil_zfail, m_stencil_zpass);
			checkForGlError("glStencilOp");
		}

		if(m_set_stencil_mask)
		{
			glStencilMask(m_stencil_mask);
			checkForGlError("glStencilMask");
		}

		if(m_set_stencil_func && m_set_stencil_func_ref && m_set_stencil_func_mask)
		{
			glStencilFunc(m_stencil_func, m_stencil_func_ref, m_stencil_func_mask);
			checkForGlError("glStencilFunc");
		}
	}

	if(m_set_shade_mode)
	{
		glShadeModel(m_shade_mode);
		checkForGlError("glShadeModel");
	}

	if(m_set_depth_mask)
	{
		glDepthMask(m_depth_mask);
		checkForGlError("glDepthMask");
	}

	if(m_set_depth_func)
	{
		glDepthFunc(m_depth_func);
		checkForGlError("glDepthFunc");
	}

	if(m_set_clip)
	{
		glDepthRangef(m_clip_min, m_clip_max);
		checkForGlError("glDepthRangef");
	}

	if(m_set_line_width)
	{
		glLineWidth(m_line_width / 255.f);
		checkForGlError("glLineWidth");
	}

	if(m_set_blend_equation)
	{
		glBlendEquationSeparate(m_blend_equation_rgb, m_blend_equation_alpha);
		checkForGlError("glBlendEquationSeparate");
	}

	if(m_set_blend_sfactor && m_set_blend_dfactor)
	{
		glBlendFuncSeparate(m_blend_sfactor_rgb, m_blend_dfactor_rgb, m_blend_sfactor_alpha, m_blend_dfactor_alpha);
		checkForGlError("glBlendFuncSeparate");
	}

	if(m_set_blend_color)
	{
		glBlendColor(m_blend_color_r, m_blend_color_g, m_blend_color_b, m_blend_color_a);
		checkForGlError("glBlendColor");
	}

	if(m_set_cull_face)
	{
		glCullFace(m_cull_face);
		checkForGlError("glCullFace");
	}

	if(m_set_alpha_func && m_set_alpha_ref)
	{
		glAlphaFunc(m_alpha_func, m_alpha_ref);
		checkForGlError("glAlphaFunc");
	}

	if(m_set_fog_mode)
	{
		glFogi(GL_FOG_MODE, m_fog_mode);
		checkForGlError("glFogi(GL_FOG_MODE)");
	}

	if(m_set_fog_params)
	{
		glFogf(GL_FOG_START, m_fog_param0);
		checkForGlError("glFogf(GL_FOG_START)");
		glFogf(GL_FOG_END, m_fog_param1);
		checkForGlError("glFogf(GL_FOG_END)");
	}

	if(m_indexed_array.m_count && m_draw_array_count)
	{
		ConLog.Warning("m_indexed_array.m_count && draw_array_count");
	}

	for(u32 i=0; i<m_textures_count; ++i)
	{
		if(!m_textures[i].IsEnabled()) continue;

		glActiveTexture(GL_TEXTURE0 + i);
		checkForGlError("glActiveTexture");
		m_gl_textures[i].Create();
		m_gl_textures[i].Bind();
		checkForGlError(wxString::Format("m_gl_textures[%d].Bind", i));
		m_program.SetTex(i);
		m_gl_textures[i].Init(m_textures[i]);
		checkForGlError(wxString::Format("m_gl_textures[%d].Init", i));
	}

	m_vao.Bind();
	if(m_indexed_array.m_count)
	{
		LoadVertexData(m_indexed_array.index_min, m_indexed_array.index_max - m_indexed_array.index_min + 1);
	}

	EnableVertexData(m_indexed_array.m_count ? true : false);

	InitVertexData();
	InitFragmentData();

	if(m_indexed_array.m_count)
	{
		switch(m_indexed_array.m_type)
		{
		case 0:
			glDrawElements(m_draw_mode - 1, m_indexed_array.m_count, GL_UNSIGNED_INT, nullptr);
			checkForGlError("glDrawElements #4");
		break;

		case 1:
			glDrawElements(m_draw_mode - 1, m_indexed_array.m_count, GL_UNSIGNED_SHORT, nullptr);
			checkForGlError("glDrawElements #2");
		break;

		default:
			ConLog.Error("Bad indexed array type (%d)", m_indexed_array.m_type);
		break;
		}

		DisableVertexData();
		m_indexed_array.Reset();
	}

	if(m_draw_array_count)
	{
		glDrawArrays(m_draw_mode - 1, 0, m_draw_array_count);
		checkForGlError("glDrawArrays");
		DisableVertexData();
		m_draw_array_count = 0;
	}

	WriteBuffers();
}

void GLGSRender::Flip()
{
	if(m_fbo.IsCreated())
	{
		m_fbo.Bind(GL_READ_FRAMEBUFFER);
		GLfbo::Bind(GL_DRAW_FRAMEBUFFER, 0);
		GLfbo::Blit(
			m_surface_clip_x, m_surface_clip_y, m_surface_clip_x + m_surface_clip_w, m_surface_clip_y + m_surface_clip_h,
			m_surface_clip_x, m_surface_clip_y, m_surface_clip_x + m_surface_clip_w, m_surface_clip_y + m_surface_clip_h,
			GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		m_fbo.Bind();
	}

	m_frame->Flip();
}