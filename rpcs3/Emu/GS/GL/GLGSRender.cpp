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
	, m_rsx_thread(nullptr)
	, m_fp_buf_num(-1)
	, m_vp_buf_num(-1)
{
	m_draw = false;
	m_frame = new GLGSFrame();
}

GLGSRender::~GLGSRender()
{
	Close();
	m_frame->Close();
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

GLRSXThread::GLRSXThread(wxWindow* parent)
	: ThreadBase(true, "OpenGL Thread")
	, m_parent(parent)
{
}

extern CellGcmContextData current_context;

void GLRSXThread::Task()
{
	ConLog.Write("GL RSX thread entry");

	GLGSRender& p = *(GLGSRender*)m_parent;
	wxGLContext cntxt(p.m_frame->GetCanvas());
	p.m_frame->GetCanvas()->SetCurrent(cntxt);
	InitProcTable();

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_SCISSOR_TEST);
	glSwapInterval(Ini.GSVSyncEnable.GetValue() ? 1 : 0);

	bool draw = true;
	u32 drawed = 0;
	u32 skipped = 0;

	p.Init();

	while(!TestDestroy() && p.m_frame && !p.m_frame->IsBeingDeleted())
	{
		wxCriticalSectionLocker lock(p.m_cs_main);

		const u32 get = re(p.m_ctrl->get);
		const u32 put = re(p.m_ctrl->put);
		if(put == get || !Emu.IsRunning())
		{
			if(put == get)
				SemaphorePostAndWait(p.m_sem_flush);

			Sleep(1);
			continue;
		}

		//ConLog.Write("addr = 0x%x", p.m_ioAddress + get);
		const u32 cmd = Memory.Read32(p.m_ioAddress + get);
		const u32 count = (cmd >> 18) & 0x7ff;
		//if(cmd == 0) continue;

		if(cmd & CELL_GCM_METHOD_FLAG_JUMP)
		{
			u32 addr = cmd & ~(CELL_GCM_METHOD_FLAG_JUMP | CELL_GCM_METHOD_FLAG_NON_INCREMENT);
			//ConLog.Warning("rsx jump(0x%x) #addr=0x%x, cmd=0x%x, get=0x%x, put=0x%x", addr, p.m_ioAddress + get, cmd, get, put);
			re(p.m_ctrl->get, addr);
			continue;
		}
		if(cmd & CELL_GCM_METHOD_FLAG_CALL)
		{
			call_stack.Push(get + 4);
			u32 offs = cmd & ~CELL_GCM_METHOD_FLAG_CALL;
			u32 addr = p.m_ioAddress + offs;
			//ConLog.Warning("rsx call(0x%x) #0x%x - 0x%x - 0x%x", offs, addr, cmd, get);
			p.m_ctrl->get = re32(offs);
			continue;
		}
		if(cmd == CELL_GCM_METHOD_FLAG_RETURN)
		{
			//ConLog.Warning("rsx return!");
			u32 get = call_stack.Pop();
			//ConLog.Warning("rsx return(0x%x)", get);
			p.m_ctrl->get = re32(get);
			continue;
		}
		if(cmd & CELL_GCM_METHOD_FLAG_NON_INCREMENT)
		{
			//ConLog.Warning("non increment cmd! 0x%x", cmd);
		}

		if(cmd == 0)
		{
			ConLog.Warning("null cmd: addr=0x%x, put=0x%x, get=0x%x", p.m_ioAddress + get, re(p.m_ctrl->put), get);
			Emu.Pause();
			continue;
		}

		mem32_ptr_t args(p.m_ioAddress + get + 4);

		if(!draw)
		{
			if((cmd & 0x3ffff) == NV406E_SET_REFERENCE)
			{
				p.m_ctrl->ref = re32(args[0]);
			}
		}
		else
		{
			p.DoCmd(cmd, cmd & 0x3ffff, args, count);
		}

		re(p.m_ctrl->get, get + (count + 1) * 4);
		//memset(Memory.GetMemFromAddr(p.m_ioAddress + get), 0, (count + 1) * 4);
	}

	ConLog.Write("GL RSX thread exit...");

	call_stack.Clear();
	p.CloseOpenGL();
}

void GLGSRender::Init(const u32 ioAddress, const u32 ioSize, const u32 ctrlAddress, const u32 localAddress)
{
	if(m_frame->IsShown()) return;

	m_draw_frames = 1;
	m_skip_frames = 0;
	m_width = 720;
	m_height = 576;

	m_frame->Show();

	m_ioAddress = ioAddress;
	m_ioSize = ioSize;
	m_ctrlAddress = ctrlAddress;
	m_local_mem_addr = localAddress;
	m_ctrl = (CellGcmControl*)Memory.GetMemFromAddr(m_ctrlAddress);

	m_cur_vertex_prog = nullptr;
	m_cur_shader_prog = nullptr;
	m_cur_shader_prog_num = 0;

	(m_rsx_thread = new GLRSXThread(this))->Start();
}

void GLGSRender::Draw()
{
	m_draw = true;
	//if(m_frame && !m_frame->IsBeingDeleted()) m_frame->Flip();
}

void GLGSRender::Close()
{
	if(m_rsx_thread)
	{
		m_rsx_thread->Stop();
		delete m_rsx_thread;
	}

	if(m_frame->IsShown()) m_frame->Hide();
	m_ctrl = nullptr;
}

void GLGSRender::EnableVertexData(bool indexed_draw)
{
	static u32 offset_list[16];
	u32 cur_offset = 0;

	for(u32 i=0; i<16; ++i)
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
		const TransformConstant& c = m_transform_constants[i];
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
		const TransformConstant& c = m_fragment_constants[i];

		u32 id = c.id - m_cur_shader_prog->offset + 2 * 4 * 4;

		const wxString name = wxString::Format("fc%u", id);
		const int l = m_program.GetLocation(name);
		checkForGlError("glGetUniformLocation " + name);

		//ConLog.Write(name + " x: %.02f y: %.02f z: %.02f w: %.02f", c.x, c.y, c.z, c.w);
		glUniform4f(l, c.x, c.y, c.z, c.w);
		checkForGlError("glUniform4f " + name + wxString::Format(" %d [%f %f %f %f]", l, c.x, c.y, c.z, c.w));
	}
}

#define case_16(a, m) \
	case a + m: \
	case a + m * 2: \
	case a + m * 3: \
	case a + m * 4: \
	case a + m * 5: \
	case a + m * 6: \
	case a + m * 7: \
	case a + m * 8: \
	case a + m * 9: \
	case a + m * 10: \
	case a + m * 11: \
	case a + m * 12: \
	case a + m * 13: \
	case a + m * 14: \
	case a + m * 15: \
	index = (cmd - a) / m; \
	case a \

void GLGSRender::DoCmd(const u32 fcmd, const u32 cmd, mem32_ptr_t& args, const u32 count)
{
#if	CMD_DEBUG
		wxString debug = GetMethodName(cmd);
		debug += "(";
		for(u32 i=0; i<count; ++i) debug += (i ? ", " : "") + wxString::Format("0x%x", args[i]);
		debug += ")";
		ConLog.Write(debug);
#endif

	u32 index = 0;

	//static u32 draw_array_count = 0;

	switch(cmd)
	{
	case 0x3fead:
		//if(cmd == 0xfeadffff)
		{
			//if(p.m_draw)
			{
				//p.m_draw = false;
				/*
				if(m_skip_frames)
				{
					if(!draw)
					{
						if(skipped++ >= m_skip_frames)
						{
							skipped = 0;
							draw = true;
						}
					}
					else
					{
						if(drawed++ >= m_draw_frames)
						{
							drawed = 0;
							draw = false;
						}
					}
				}*/
				
				//if(draw)
				{
					//if(m_frame->GetClientSize() != wxSize(m_viewport_w, m_viewport_h))
					//	m_frame->SetClientSize(m_viewport_w, m_viewport_h);

					if(m_fbo.IsCreated())
					{
						m_fbo.Bind(GL_READ_FRAMEBUFFER);
						m_fbo.Bind(GL_DRAW_FRAMEBUFFER, 0);
						m_fbo.Blit(
							m_surface_clip_x, m_surface_clip_y, m_surface_clip_x + m_surface_clip_w, m_surface_clip_y + m_surface_clip_h,
							m_surface_clip_x, m_surface_clip_y, m_surface_clip_x + m_surface_clip_w, m_surface_clip_y + m_surface_clip_h,
							GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
						m_fbo.Bind();
						checkForGlError("m_fbo.Blit");
					}

					m_frame->Flip();
					//glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				}

				m_gcm_current_buffer = args[0];

				m_flip_status = 0;
				if(m_flip_handler)
				{
					m_flip_handler.Handle(1, 0, 0);
					m_flip_handler.Branch(false);
				}

				SemaphorePostAndWait(m_sem_flip);
			}
		}
	break;

	case NV4097_NO_OPERATION:
	break;

	case NV406E_SET_REFERENCE:
		m_ctrl->ref = re32(args[0]);
	break;

	case_16(NV4097_SET_TEXTURE_OFFSET, 0x20):
	{
		GLTexture& tex = m_frame->GetTexture(index);
		const u32 offset = args[0];
		u32 a1 = args[1];
		u8 location = (a1 & 0x3) - 1;
		const bool cubemap = (a1 >> 2) & 0x1;
		const u8 dimension = (a1 >> 4) & 0xf;
		const u8 format = (a1 >> 8) & 0xff;
		const u16 mipmap = (a1 >> 16) & 0xffff;
		CMD_LOG("index = %d, offset=0x%x, location=0x%x, cubemap=0x%x, dimension=0x%x, format=0x%x, mipmap=0x%x",
			index, offset, location, cubemap, dimension, format, mipmap);

		if(location == 2)
		{
			ConLog.Error("Bad texture location.");
			location = 1;
		}
		u32 tex_addr = GetAddress(offset, location);
		//ConLog.Warning("texture addr = 0x%x #offset = 0x%x, location=%d", tex_addr, offset, location);
		tex.SetOffset(tex_addr);
		tex.SetFormat(cubemap, dimension, format, mipmap);
	}
	break;

	case_16(NV4097_SET_TEXTURE_CONTROL0, 0x20):
	{
		GLTexture& tex = m_frame->GetTexture(index);
		u32 a0 = args[0];
		bool enable = a0 >> 31 ? true : false;
		u16 minlod = (a0 >> 19) & 0xfff;
		u16 maxlod = (a0 >> 7) & 0xfff;
		u8 maxaniso = (a0 >> 2) & 0x7;
		tex.SetControl0(enable, minlod, maxlod, maxaniso);
	}
	break;
	
	case_16(NV4097_SET_VERTEX_DATA4UB_M, 4):
	{
		u32 v = args[0];
		u8 v0 = v;
		u8 v1 = v >> 8;
		u8 v2 = v >> 16;
		u8 v3 = v >> 24;

		m_vertex_data[index].size = 4;
		m_vertex_data[index].type = 4;
		m_vertex_data[index].data.AddCpy(v0);
		m_vertex_data[index].data.AddCpy(v1);
		m_vertex_data[index].data.AddCpy(v2);
		m_vertex_data[index].data.AddCpy(v3);
		//ConLog.Warning("index = %d, v0 = 0x%x, v1 = 0x%x, v2 = 0x%x, v3 = 0x%x", index, v0, v1, v2, v3);
	}
	break;

	case_16(NV4097_SET_VERTEX_DATA2F_M, 8):
	{
		u32 a0 = args[0];
		u32 a1 = args[1];

		float v0 = (float&)a0;
		float v1 = (float&)a1;
		
		m_vertex_data[index].type = 2;
		m_vertex_data[index].size = 2;
		m_vertex_data[index].data.SetCount(sizeof(float) * 2);
		(float&)m_vertex_data[index].data[sizeof(float)*0] = v0;
		(float&)m_vertex_data[index].data[sizeof(float)*1] = v1;

		//ConLog.Warning("index = %d, v0 = %f, v1 = %f", index, v0, v1);
	}
	break;

	case_16(NV4097_SET_VERTEX_DATA4F_M, 16):
	{
		u32 a0 = args[0];
		u32 a1 = args[1];
		u32 a2 = args[2];
		u32 a3 = args[3];

		float v0 = (float&)a0;
		float v1 = (float&)a1;
		float v2 = (float&)a2;
		float v3 = (float&)a3;

		m_vertex_data[index].type = 2;
		m_vertex_data[index].size = 4;
		m_vertex_data[index].data.SetCount(sizeof(float) * 4);
		(float&)m_vertex_data[index].data[sizeof(float)*0] = v0;
		(float&)m_vertex_data[index].data[sizeof(float)*1] = v1;
		(float&)m_vertex_data[index].data[sizeof(float)*2] = v2;
		(float&)m_vertex_data[index].data[sizeof(float)*3] = v3;

		//ConLog.Warning("index = %d, v0 = %f, v1 = %f, v2 = %f, v3 = %f", index, v0, v1, v2, v3);
	}
	break;

	case_16(NV4097_SET_TEXTURE_CONTROL1, 0x20):
	{
		GLTexture& tex = m_frame->GetTexture(index);
		tex.SetControl1(args[0]);
	}
	break;

	case_16(NV4097_SET_TEXTURE_CONTROL3, 4):
	{
		GLTexture& tex = m_frame->GetTexture(index);
		u32 a0 = args[0];
		u32 pitch = a0 & 0xFFFFF;
		u16 depth = a0 >> 20;
		tex.SetControl3(pitch, depth);
	}
	break;

	case_16(NV4097_SET_TEXTURE_FILTER, 0x20):
	{
		GLTexture& tex = m_frame->GetTexture(index);
		u32 a0 = args[0];
		u16 bias = a0 & 0x1fff;
		u8 conv = (a0 >> 13) & 0xf;
		u8 min = (a0 >> 16) & 0x7;
		u8 mag = (a0 >> 24) & 0x7;
		u8 a_signed = (a0 >> 28) & 0x1;
		u8 r_signed = (a0 >> 29) & 0x1;
		u8 g_signed = (a0 >> 30) & 0x1;
		u8 b_signed = (a0 >> 31) & 0x1;

		tex.SetFilter(bias, min, mag, conv, a_signed, r_signed, g_signed, b_signed);
	}
	break;

	case_16(NV4097_SET_TEXTURE_ADDRESS, 0x20):
	{
		GLTexture& tex = m_frame->GetTexture(index);

		u32 a0 = args[0];
		u8 wraps			= a0 & 0xf;
		u8 aniso_bias		= (a0 >> 4) & 0xf;
		u8 wrapt			= (a0 >> 8) & 0xf;
		u8 unsigned_remap	= (a0 >> 12) & 0xf;
		u8 wrapr			= (a0 >> 16) & 0xf;
		u8 gamma			= (a0 >> 20) & 0xf;
		u8 signed_remap		= (a0 >> 24) & 0xf;
		u8 zfunc			= a0 >> 28;

		tex.SetAddress(wraps, wrapt, wrapr, unsigned_remap, zfunc, gamma, aniso_bias, signed_remap);
	}
	break;

	case_16(NV4097_SET_TEX_COORD_CONTROL, 4):
		//TODO
	break;

	case_16(NV4097_SET_TEXTURE_IMAGE_RECT, 32):
	{
		GLTexture& tex = m_frame->GetTexture(index);

		const u16 height = args[0] & 0xffff;
		const u16 width = args[0] >> 16;
		CMD_LOG("width=%d, height=%d", width, height);
		tex.SetRect(width, height);
	}
	break;

	case NV4097_SET_SURFACE_FORMAT:
	{
		u32 a0 = args[0];
		m_set_surface_format = true;
		m_surface_color_format = a0 & 0x1f;
		m_surface_depth_format = (a0 >> 5) & 0x7;
		m_surface_type = (a0 >> 8) & 0xf;
		m_surface_antialias = (a0 >> 12) & 0xf;
		m_surface_width = (a0 >> 16) & 0xff;
		m_surface_height = (a0 >> 24) & 0xff;
		m_surface_pitch_a = args[1];
		m_surface_offset_a = args[2];
		m_surface_offset_z = args[3];
		m_surface_offset_b = args[4];
		m_surface_pitch_b = args[5];

		m_depth_offset = m_surface_offset_z;

		gcmBuffer* buffers = (gcmBuffer*)Memory.GetMemFromAddr(m_gcm_buffers_addr);
		m_width = re(buffers[m_gcm_current_buffer].width);
		m_height = re(buffers[m_gcm_current_buffer].height);

		break;
		m_rbo.Create(2);
		checkForGlError("m_rbo.Create");
		m_rbo.Bind(0);
		m_rbo.Storage(GL_RGBA, m_width, m_height);
		checkForGlError("m_rbo.Storage(GL_RGBA)");
		m_rbo.Bind(1);

		switch(m_surface_depth_format)
		{
		case 1:
			m_rbo.Storage(GL_DEPTH_COMPONENT16, m_width, m_height);
		break;

		case 2:
			m_rbo.Storage(GL_DEPTH24_STENCIL8, m_width, m_height);
		break;

		default:
			ConLog.Error("Bad depth format! (%d)", m_surface_depth_format);
			assert(0);
		break;
		}
		checkForGlError("m_rbo.Storage(GL_DEPTH24_STENCIL8)");
		m_fbo.Create();
		checkForGlError("m_fbo.Create");
		m_fbo.Bind();
		m_fbo.Renderbuffer(GL_COLOR_ATTACHMENT0, m_rbo.GetId(0));
		checkForGlError("m_fbo.Renderbuffer(GL_COLOR_ATTACHMENT0)");

		if(m_surface_depth_format == 2)
		{
			m_fbo.Renderbuffer(GL_DEPTH_STENCIL_ATTACHMENT, m_rbo.GetId(1));
			checkForGlError("m_fbo.Renderbuffer(GL_DEPTH_STENCIL_ATTACHMENT)");
		}
		else
		{
			m_fbo.Renderbuffer(GL_DEPTH_ATTACHMENT, m_rbo.GetId(1));
			checkForGlError("m_fbo.Renderbuffer(GL_DEPTH_STENCIL_ATTACHMENT)");
		}
		//CMD_LOG("color_format=%d, depth_format=%d, type=%d, antialias=%d, width=%d, height=%d, pitch_a=%d, offset_a=0x%x, offset_z=0x%x, offset_b=0x%x, pitch_b=%d",
		//	color_format, depth_format, type, antialias, width, height, pitch_a, offset_a, offset_z, offset_b, pitch_b);
	}
	break;

	case NV4097_SET_COLOR_MASK_MRT:
	{
	}
	break;

	case NV4097_SET_COLOR_MASK:
	{
		const u32 flags = args[0];

		m_set_color_mask = true;
		m_color_mask_a = flags & 0x1000000 ? true : false;
		m_color_mask_r = flags & 0x0010000 ? true : false;
		m_color_mask_g = flags & 0x0000100 ? true : false;
		m_color_mask_b = flags & 0x0000001 ? true : false;
	}
	break;

	case NV4097_SET_ALPHA_TEST_ENABLE:
		m_set_alpha_test = args[0] ? true : false;
	break;

	case NV4097_SET_BLEND_ENABLE:
		m_set_blend = args[0] ? true : false;
	break;

	case NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE:
		m_set_depth_bounds_test = args[0] ? true : false;
	break;

	case NV4097_SET_ALPHA_FUNC:
		m_set_alpha_func = true;
		m_alpha_func = args[0];

		if(count >= 2)
		{
			m_set_alpha_ref = true;
			m_alpha_ref = args[1];
		}
	break;

	case NV4097_SET_ALPHA_REF:
		m_set_alpha_ref = true;
		m_alpha_ref = args[0];
	break;

	case NV4097_SET_CULL_FACE:
		m_set_cull_face = true;
		m_cull_face = args[0];
	break;

	case NV4097_SET_VIEWPORT_VERTICAL:
	{
		m_set_viewport_vertical = true;
		m_viewport_y = args[0] & 0xffff;
		m_viewport_h = args[0] >> 16;
	}
	break;

	case NV4097_SET_VIEWPORT_HORIZONTAL:
	{
		m_set_viewport_horizontal = true;
		m_viewport_x = args[0] & 0xffff;
		m_viewport_w = args[0] >> 16;

		if(count == 2)
		{
			m_set_viewport_vertical = true;
			m_viewport_y = args[1] & 0xffff;
			m_viewport_h = args[1] >> 16;
		}

		CMD_LOG("x=%d, y=%d, w=%d, h=%d", m_viewport_x, m_viewport_y, m_viewport_w, m_viewport_h);
	}
	break;

	case NV4097_SET_CLIP_MIN:
	{
		const u32 clip_min = args[0];
		const u32 clip_max = args[1];

		m_set_clip = true;
		m_clip_min = (float&)clip_min;
		m_clip_max = (float&)clip_max;

		CMD_LOG("clip_min=%.01f, clip_max=%.01f", m_clip_min, m_clip_max);
	}
	break;

	case NV4097_SET_DEPTH_FUNC:
		m_set_depth_func = true;
		m_depth_func = args[0];
	break;

	case NV4097_SET_DEPTH_TEST_ENABLE:
		m_depth_test_enable = args[0] ? true : false;
	break;

	case NV4097_SET_FRONT_POLYGON_MODE:
		m_set_front_polygon_mode = true;
		m_front_polygon_mode = args[0];
	break;

	case NV4097_CLEAR_SURFACE:
	{
		if(m_set_clear_surface)
		{
			m_clear_surface_mask |= args[0];
		}
		else
		{
			m_clear_surface_mask = args[0];
			m_set_clear_surface = true;
		}
	}
	break;

	case NV4097_SET_BLEND_FUNC_SFACTOR:
	{
		m_set_blend_sfactor = true;
		m_blend_sfactor_rgb = args[0] & 0xffff;
		m_blend_sfactor_alpha = args[0] >> 16;

		if(count >= 2)
		{
			m_set_blend_dfactor = true;
			m_blend_dfactor_rgb = args[1] & 0xffff;
			m_blend_dfactor_alpha = args[1] >> 16;
		}
	}
	break;

	case NV4097_SET_BLEND_FUNC_DFACTOR:
	{
		m_set_blend_dfactor = true;
		m_blend_dfactor_rgb = args[0] & 0xffff;
		m_blend_dfactor_alpha = args[0] >> 16;
	}
	break;

	case_16(NV4097_SET_VERTEX_DATA_ARRAY_OFFSET, 4):
	{
		const u32 addr = GetAddress(args[0] & 0x7fffffff, args[0] >> 31);
		CMD_LOG("num=%d, addr=0x%x", index, addr);
		m_vertex_data[index].addr = addr;
	}
	break;

	case_16(NV4097_SET_VERTEX_DATA_ARRAY_FORMAT, 4):
	{
		u32 a0 = args[0];
		const u16 frequency = a0 >> 16;
		const u8 stride = (a0 >> 8) & 0xff;
		const u8 size = (a0 >> 4) & 0xf;
		const u8 type = a0 & 0xf;

		CMD_LOG("index=%d, frequency=%d, stride=%d, size=%d, type=%d",
			index, frequency, stride, size, type);

		VertexData& cv = m_vertex_data[index];
		cv.frequency = frequency;
		cv.stride = stride;
		cv.size = size;
		cv.type = type;
	}
	break;

	case NV4097_DRAW_ARRAYS:
	{
		for(u32 c=0; c<count; ++c)
		{
			u32 ac = args[c];
			const u32 first = ac & 0xffffff;
			const u32 _count = (ac >> 24) + 1;

			LoadVertexData(first, _count);

			m_draw_array_count += _count;
		}
	}
	break;

	case NV4097_SET_INDEX_ARRAY_ADDRESS:
	{
		m_indexed_array.m_addr = GetAddress(args[0], args[1] & 0xf);
		m_indexed_array.m_type = args[1] >> 4;
	}
	break;

	case NV4097_DRAW_INDEX_ARRAY:
	{
		for(u32 c=0; c<count; ++c)
		{
			const u32 first = args[c] & 0xffffff;
			const u32 _count = (args[c] >> 24) + 1;

			if(first < m_indexed_array.m_first) m_indexed_array.m_first = first;

			for(u32 i=first; i<_count; ++i)
			{
				u32 index;
				switch(m_indexed_array.m_type)
				{
					case 0:
					{
						int pos = m_indexed_array.m_data.GetCount();
						m_indexed_array.m_data.InsertRoomEnd(4);
						index = Memory.Read32(m_indexed_array.m_addr + i * 4);
						*(u32*)&m_indexed_array.m_data[pos] = index;
						//ConLog.Warning("index 4: %d", *(u32*)&m_indexed_array.m_data[pos]);
					}
					break;

					case 1:
					{
						int pos = m_indexed_array.m_data.GetCount();
						m_indexed_array.m_data.InsertRoomEnd(2);
						index = Memory.Read16(m_indexed_array.m_addr + i * 2);
						//ConLog.Warning("index 2: %d", index);
						*(u16*)&m_indexed_array.m_data[pos] = index;
					}
					break;
				}

				if(index < m_indexed_array.index_min) m_indexed_array.index_min = index;
				if(index > m_indexed_array.index_max) m_indexed_array.index_max = index;
			}

			m_indexed_array.m_count += _count;
		}
	}
	break;

	case NV4097_SET_BEGIN_END:
	{
		if(args[0])
		{
			//begin
			if(Emu.GetCallbackManager().m_exit_callback.m_callbacks.GetCount())
			{
				//Emu.GetCallbackManager().m_exit_callback.Handle(0x0121, 0);
			}
			m_draw_mode = args[0] - 1;
		}
		else
		{
			//end
			ExecCMD();
			if(Emu.GetCallbackManager().m_exit_callback.m_callbacks.GetCount())
			{
				//Emu.GetCallbackManager().m_exit_callback.Handle(0x0122, 0);
			}
		}
	}
	break;

	case NV4097_SET_COLOR_CLEAR_VALUE:
	{
		const u32 color = args[0];
		//m_set_clear_color = true;
		m_clear_color_a = (color >> 24) & 0xff;
		m_clear_color_r = (color >> 16) & 0xff;
		m_clear_color_g = (color >> 8) & 0xff;
		m_clear_color_b = color & 0xff;

		glClearColor(
				m_clear_color_r / 255.0f,
				m_clear_color_g / 255.0f,
				m_clear_color_b / 255.0f,
				m_clear_color_a / 255.0f);
		checkForGlError("glClearColor");
	}
	break;

	case NV4097_SET_SHADER_PROGRAM:
	{
		m_cur_shader_prog = &m_shader_progs[m_cur_shader_prog_num++];
		m_cur_shader_prog->Delete();
		u32 a0 = args[0];
		m_cur_shader_prog->offset = a0 & ~0x3;
		m_cur_shader_prog->addr = GetAddress(m_cur_shader_prog->offset, (a0 & 0x3) - 1);
	}
	break;

	case NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK:
	{
		//VertexData[0].prog.attributeOutputMask = args[0];
		//FragmentData.prog.attributeInputMask = args[0]/* & ~0x20*/;
	}
	break;

	case NV4097_SET_SHADER_CONTROL:
	{
		const u32 arg0 = args[0];

		//const u8 controlTxp = (arg0 >> 15) & 0x1;
		//FragmentData.prog.registerCount = arg0 >> 24;
		//FragmentData.prog.
	}
	break;

	case NV4097_SET_TRANSFORM_PROGRAM_LOAD:
	{
		m_cur_vertex_prog = &m_vertex_progs[args[0]];
		m_cur_vertex_prog->Delete();

		if(count == 2)
		{
			const u32 start = args[1];
			if(start)
				ConLog.Warning("NV4097_SET_TRANSFORM_PROGRAM_LOAD: start = %d", start);
		}
	}
	break;

	case NV4097_SET_TRANSFORM_PROGRAM:
	{
		if(!m_cur_vertex_prog)
		{
			ConLog.Warning("NV4097_SET_TRANSFORM_PROGRAM: m_cur_vertex_prog == NULL");
			break;
		}

		for(u32 i=0; i<count; ++i) m_cur_vertex_prog->data.AddCpy(args[i]);
	}
	break;

	case NV4097_SET_TRANSFORM_TIMEOUT:

		if(!m_cur_vertex_prog)
		{
			ConLog.Warning("NV4097_SET_TRANSFORM_TIMEOUT: m_cur_vertex_prog == NULL");
			break;
		}

		//m_cur_vertex_prog->Decompile();
	break;

	case NV4097_SET_VERTEX_ATTRIB_INPUT_MASK:
		//VertexData[0].prog.attributeInputMask = args[0];
	break;

	case NV4097_INVALIDATE_VERTEX_CACHE_FILE:
	break;

	case NV4097_SET_TRANSFORM_CONSTANT_LOAD:
	{
		if((count - 1) % 4)
		{
			CMD_LOG("NV4097_SET_TRANSFORM_CONSTANT_LOAD [%d]", count);
			break;
		}

		for(u32 id = args[0], i = 1; i<count; ++id)
		{
			const u32 x = args[i++];
			const u32 y = args[i++];
			const u32 z = args[i++];
			const u32 w = args[i++];

			TransformConstant c(id, (float&)x, (float&)y, (float&)z, (float&)w);

			CMD_LOG("SET_TRANSFORM_CONSTANT_LOAD[%d : %d] = (%f, %f, %f, %f)", i, id, c.x, c.y, c.z, c.w);
			m_transform_constants.AddCpy(c);
		}
	}
	break;

	case NV4097_SET_LOGIC_OP_ENABLE:
		m_set_logic_op = args[0] ? true : false;
	break;

	case NV4097_SET_CULL_FACE_ENABLE:
		m_set_cull_face_enable = args[0] ? true : false;
	break;

	case NV4097_SET_DITHER_ENABLE:
		m_set_dither = args[0] ? true : false;
	break;

	case NV4097_SET_STENCIL_TEST_ENABLE:
		m_set_stencil_test = args[0] ? true : false;
	break;

	case NV4097_SET_STENCIL_MASK:
		m_set_stencil_mask = true;
		m_stencil_mask = args[0];
	break;

	case NV4097_SET_STENCIL_FUNC:
		m_set_stencil_func = true;
		m_stencil_func = args[0];
		if(count >= 2)
		{
			m_set_stencil_func_ref = true;
			m_stencil_func_ref = args[1];

			if(count >= 3)
			{
				m_set_stencil_func_mask = true;
				m_stencil_func_mask = args[2];
			}
		}
	break;

	case NV4097_SET_STENCIL_FUNC_REF:
		m_set_stencil_func_ref = true;
		m_stencil_func_ref = args[0];
	break;

	case NV4097_SET_STENCIL_FUNC_MASK:
		m_set_stencil_func_mask = true;
		m_stencil_func_mask = args[0];
	break;

	case NV4097_SET_STENCIL_OP_FAIL:
		m_set_stencil_fail = true;
		m_stencil_fail = args[0];
		if(count >= 2)
		{
			m_set_stencil_zfail = true;
			m_stencil_zfail = args[1];

			if(count >= 3)
			{
				m_set_stencil_zpass = true;
				m_stencil_zpass = args[2];
			}
		}
	break;

	case NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE:
		m_set_two_sided_stencil_test_enable = args[0] ? true : false;
	break;

	case NV4097_SET_BACK_STENCIL_MASK:
		m_set_back_stencil_mask = true;
		m_back_stencil_mask = args[0];
	break;

	case NV4097_SET_BACK_STENCIL_FUNC:
		m_set_back_stencil_func = true;
		m_back_stencil_func = args[0];
		if(count >= 2)
		{
			m_set_back_stencil_func_ref = true;
			m_back_stencil_func_ref = args[1];

			if(count >= 3)
			{
				m_set_back_stencil_func_mask = true;
				m_back_stencil_func_mask = args[2];
			}
		}
	break;

	case NV4097_SET_BACK_STENCIL_FUNC_REF:
		m_set_back_stencil_func_ref = true;
		m_back_stencil_func_ref = args[0];
	break;

	case NV4097_SET_BACK_STENCIL_FUNC_MASK:
		m_set_back_stencil_func_mask = true;
		m_back_stencil_func_mask = args[0];
	break;

	case NV4097_SET_BACK_STENCIL_OP_FAIL:
		m_set_stencil_fail = true;
		m_stencil_fail = args[0];
		if(count >= 2)
		{
			m_set_back_stencil_zfail = true;
			m_back_stencil_zfail = args[1];

			if(count >= 3)
			{
				m_set_back_stencil_zpass = true;
				m_back_stencil_zpass = args[2];
			}
		}
	break;

	case NV4097_SET_POLY_OFFSET_FILL_ENABLE:
		if(args[0]) ConLog.Error("NV4097_SET_POLY_OFFSET_FILL_ENABLE");
	break;

	case NV4097_SET_RESTART_INDEX_ENABLE:
		if(args[0]) ConLog.Error("NV4097_SET_RESTART_INDEX_ENABLE");
	break;

	case NV4097_SET_POINT_PARAMS_ENABLE:
		if(args[0]) ConLog.Error("NV4097_SET_POINT_PARAMS_ENABLE");
	break;

	case NV4097_SET_POINT_SPRITE_CONTROL:
		if(args[0] & 0x1)
		{
			ConLog.Error("NV4097_SET_POINT_SPRITE_CONTROL enable");
		}
	break;

	case NV4097_SET_POLY_SMOOTH_ENABLE:
		m_set_poly_smooth = args[0] ? true : false;
	break;

	case NV4097_SET_BLEND_COLOR:
		m_set_blend_color = true;
		m_blend_color_r = args[0] & 0xff;
		m_blend_color_g = (args[0] >> 8) & 0xff;
		m_blend_color_b = (args[0] >> 16) & 0xff;
		m_blend_color_a = (args[0] >> 24) & 0xff;
	break;

	case NV4097_SET_BLEND_COLOR2:
		if(args[0]) ConLog.Error("NV4097_SET_BLEND_COLOR2");
	break;

	case NV4097_SET_BLEND_EQUATION:
		m_set_blend_equation = true;
		m_blend_equation_rgb = args[0] & 0xffff;
		m_blend_equation_alpha = args[0] >> 16;
	break;

	case NV4097_SET_REDUCE_DST_COLOR:
		if(args[0]) ConLog.Error("NV4097_SET_REDUCE_DST_COLOR");
	break;

	case NV4097_SET_DEPTH_MASK:
		m_set_depth_mask = true;
		m_depth_mask = args[0];
	break;

	case NV4097_SET_SCISSOR_VERTICAL:
	{
		m_set_scissor_vertical = true;
		m_scissor_y = args[0] & 0xffff;
		m_scissor_h = args[0] >> 16;
	}
	break;

	case NV4097_SET_SCISSOR_HORIZONTAL:
	{
		m_set_scissor_horizontal = true;
		m_scissor_x = args[0] & 0xffff;
		m_scissor_w = args[0] >> 16;

		if(count == 2)
		{
			m_set_scissor_vertical = true;
			m_scissor_y = args[1] & 0xffff;
			m_scissor_h = args[1] >> 16;
		}
	}
	break;

	case NV4097_INVALIDATE_VERTEX_FILE: break;

	case NV4097_SET_VIEWPORT_OFFSET:
	{
		//TODO
	}
	break;

	case NV4097_SET_SEMAPHORE_OFFSET:
	case NV406E_SEMAPHORE_OFFSET:
	{
		m_set_semaphore_offset = true;
		m_semaphore_offset = args[0];
	}
	break;

	case NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE:
	{
		if(m_set_semaphore_offset)
		{
			m_set_semaphore_offset = false;
			u32 value = args[0];
			value = (value & 0xff00ff00) | ((value & 0xff) << 16) | ((value >> 16) & 0xff);

			Memory.Write32(Memory.RSXCMDMem.GetStartAddr() + m_semaphore_offset, value);
		}
	}
	break;

	case NV406E_SEMAPHORE_RELEASE:
	case NV4097_TEXTURE_READ_SEMAPHORE_RELEASE:
		if(m_set_semaphore_offset)
		{
			m_set_semaphore_offset = false;
			Memory.Write32(Memory.RSXCMDMem.GetStartAddr() + m_semaphore_offset, args[0]);
		}
	break;

	case NV406E_SEMAPHORE_ACQUIRE:
	{
		//TODO
	}
	break;

	case NV4097_SET_RESTART_INDEX:
	{
		//TODO
	}
	break;

	case NV4097_INVALIDATE_L2:
	{
		//TODO
	}
	break;

	case NV4097_SET_CONTEXT_DMA_COLOR_A:
	{
		m_set_context_dma_color_a = true;
		m_context_dma_color_a = args[0];
	}
	break;

	case NV4097_SET_CONTEXT_DMA_COLOR_B:
	{
		m_set_context_dma_color_b = true;
		m_context_dma_color_b = args[0];
	}
	break;

	case NV4097_SET_CONTEXT_DMA_COLOR_C:
	{
		m_set_context_dma_color_c = true;
		m_context_dma_color_c = args[0];
	}
	break;

	case NV4097_SET_CONTEXT_DMA_ZETA:
	{
		m_set_context_dma_z = true;
		m_context_dma_z = args[0];
	}
	break;

	case NV4097_SET_SURFACE_PITCH_A:
	{
		//TODO
	}
	break;

	case NV4097_SET_SURFACE_PITCH_B:
	{
		//TODO
	}
	break;

	case NV4097_SET_SURFACE_PITCH_C:
	{
		//TODO
	}
	break;

	case NV4097_SET_SURFACE_PITCH_Z:
	{
		m_depth_pitch = args[0];
	}
	break;

	case NV4097_SET_SHADER_WINDOW:
	{
		//TODO
	}
	break;

	case NV4097_SET_SURFACE_CLIP_VERTICAL:
	{
		u32 a0 = args[0];
		m_set_surface_clip_vertical = true;
		m_surface_clip_y = a0;
		m_surface_clip_h = a0 >> 16;
	}
	break;

	case NV4097_SET_SURFACE_CLIP_HORIZONTAL:
	{
		u32 a0 = args[0];

		m_set_surface_clip_horizontal = true;
		m_surface_clip_x = a0;
		m_surface_clip_w = a0 >> 16;

		if(count >= 2)
		{
			u32 a1 = args[1];
			m_set_surface_clip_vertical = true;
			m_surface_clip_y = a1;
			m_surface_clip_h = a1 >> 16;
		}
	}
	break;

	case NV4097_SET_WINDOW_OFFSET:
	{
		//TODO
	}
	break;

	case NV4097_SET_SURFACE_COLOR_TARGET:
	{
		//TODO
	}
	break;

	case NV4097_SET_ANTI_ALIASING_CONTROL:
	{
		//TODO
	}
	break;

	case NV4097_SET_LINE_SMOOTH_ENABLE:
		m_set_line_smooth = args[0] ? true : false;
	break;

	case NV4097_SET_LINE_WIDTH:
		m_set_line_width = true;
		m_line_width = args[0];
	break;

	case NV4097_SET_SHADE_MODE:
		m_set_shade_mode = true;
		m_shade_mode = args[0];
	break;

	case NV4097_SET_ZSTENCIL_CLEAR_VALUE:
	{
		u32 clear_valuei = args[0];
		double clear_valuef = (double)clear_valuei / 0xffffffff;
		glClearDepth(clear_valuef);
		glClearStencil(clear_valuei);
	}
	break;

	case NV4097_SET_ZCULL_CONTROL0:
	{
		m_set_depth_func = true;
		m_depth_func = args[0] >> 4;
	}
	break;

	case NV4097_SET_ZCULL_CONTROL1:
	{
		//TODO
	}
	break;

	case NV4097_SET_SCULL_CONTROL:
	{
		u32 a0 = args[0];
		m_set_stencil_func = m_set_stencil_func_ref = m_set_stencil_func_mask = true;

		m_stencil_func = a0 & 0xffff;
		m_stencil_func_ref = (a0 >> 16) & 0xff;
		m_stencil_func_mask = (a0 >> 24) & 0xff;
	}
	break;

	case NV4097_SET_ZCULL_EN:
	{
		u32 a0 = args[0];

		m_depth_test_enable = a0 & 0x1 ? true : false;
		m_set_stencil_test = a0 & 0x2 ? true : false;
	}
	break;

	case NV4097_GET_REPORT:
	{
		u32 a0 = args[0];
		u8 type = a0 >> 24;
		u32 offset = a0 & 0xffffff;

		u64 data;
		switch(type)
		{
		case 1:
			data = std::chrono::steady_clock::now().time_since_epoch().count();
			data *= 1000000;
		break;

		default:
			data = 0;
			ConLog.Error("NV4097_GET_REPORT: bad type %d", type);
		break;
		}

		Memory.Write64(m_local_mem_addr + offset, data);
	}
	break;

	case NV3062_SET_OFFSET_DESTIN:
		m_dst_offset = args[0];
	break;

	case NV308A_COLOR:
	{
		TransformConstant c;
		c.id = m_dst_offset;

		if(count >= 1)
		{
			u32 a = args[0];
			a = a << 16 | a >> 16;
			c.x = (float&)a;
		}

		if(count >= 2)
		{
			u32 a = args[1];
			a = a << 16 | a >> 16;
			c.y = (float&)a;
		}

		if(count >= 3)
		{
			u32 a = args[2];
			a = a << 16 | a >> 16;
			c.z = (float&)a;
		}

		if(count >= 4)
		{
			u32 a = args[3];
			a = a << 16 | a >> 16;
			c.w = (float&)a;
		}

		if(count >= 5)
		{
			ConLog.Warning("NV308A_COLOR: count = %d", count);
		}

		//ConLog.Warning("NV308A_COLOR: [%d]: %f, %f, %f, %f", c.id, c.x, c.y, c.z, c.w);
		m_fragment_constants.AddCpy(c);
	}
	break;

	case NV308A_POINT:
		//TODO
	break;

	case NV3062_SET_COLOR_FORMAT:
	{
		m_color_format = args[0];
		m_color_format_src_pitch = args[1];
		m_color_format_dst_pitch = args[1] >> 16;
	}
	break;

	case NV3089_SET_COLOR_CONVERSION:
	{
		m_color_conv = args[0];
		m_color_conv_fmt = args[1];
		m_color_conv_op = args[2];
		m_color_conv_in_x = args[3];
		m_color_conv_in_y = args[3] >> 16;
		m_color_conv_in_w = args[4];
		m_color_conv_in_h = args[4] >> 16;
		m_color_conv_out_x = args[5];
		m_color_conv_out_y = args[5] >> 16;
		m_color_conv_out_w = args[6];
		m_color_conv_out_h = args[6] >> 16;
		m_color_conv_dsdx = args[7];
		m_color_conv_dtdy = args[8];
	}
	break;

	case NV3089_IMAGE_IN_SIZE:
	{
		u16 w = args[0];
		u16 h = args[0] >> 16;
		u16 pitch = args[1];
		u8 origin = args[1] >> 16;
		u8 inter = args[1] >> 24;
		u32 offset = args[2];
		u16 u = args[3];
		u16 v = args[3] >> 16;

		u8* pixels_src = &Memory[GetAddress(offset, m_context_dma_img_src - 0xfeed0000)];
		u8* pixels_dst = &Memory[GetAddress(m_dst_offset, m_context_dma_img_dst - 0xfeed0000)];

		for(u16 y=0; y<m_color_conv_in_h; ++y)
		{
			for(u16 x=0; x<m_color_format_src_pitch/4/*m_color_conv_in_w*/; ++x)
			{
				const u32 src_offset = (m_color_conv_in_y + y) * m_color_format_src_pitch + (m_color_conv_in_x + x) * 4;
				const u32 dst_offset = (m_color_conv_out_y + y) * m_color_format_dst_pitch + (m_color_conv_out_x + x) * 4;
				(u32&)pixels_dst[dst_offset] = (u32&)pixels_src[src_offset];
			}
		}
	}
	break;
	
	case NV3089_SET_CONTEXT_DMA_IMAGE:
		m_context_dma_img_src = args[0];
	break;

	case NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN:
		m_context_dma_img_dst = args[0];
	break;


	case NV3089_SET_CONTEXT_SURFACE:
		if(args[0] != 0x313371C3)
		{
			ConLog.Warning("NV3089_SET_CONTEXT_SURFACE: Unsupported surface (0x%x)", args[0]);
		}
	break;

	case NV4097_SET_FOG_MODE:
		m_set_fog_mode = true;
		m_fog_mode = args[0];
	break;

	case NV4097_SET_USER_CLIP_PLANE_CONTROL:
	{
		u32 a0 = args[0];
		m_set_clip_plane = true;
		m_clip_plane_0 = a0 & 0xf;
		m_clip_plane_1 = (a0 >> 4) & 0xf;
		m_clip_plane_2 = (a0 >> 8) & 0xf;
		m_clip_plane_3 = (a0 >> 12) & 0xf;
		m_clip_plane_4 = (a0 >> 16) & 0xf;
		m_clip_plane_5 = a0 >> 20;
	}
	break;

	case NV4097_SET_FOG_PARAMS:
	{
		m_set_fog_params = true;
		u32 a0 = args[0];
		u32 a1 = args[1];
		m_fog_param0 = (float&)a0;
		m_fog_param1 = (float&)a1;
	}
	break;

	default:
	{
		wxString log = GetMethodName(cmd);
		log += "(";
		for(u32 i=0; i<count; ++i) log += (i ? ", " : "") + wxString::Format("0x%x", args[i]);
		log += ")";
		ConLog.Error("TODO: " + log);
		//Emu.Pause();
	}
	break;
	}
}

bool GLGSRender::LoadProgram()
{
	if(!m_cur_shader_prog)
	{
		ConLog.Warning("LoadProgram: m_cur_shader_prog == NULL");

		return false;
	}

	m_fp_buf_num = m_prog_buffer.SearchFp(*m_cur_shader_prog);

	if(m_fp_buf_num == -1) m_cur_shader_prog->Decompile();

	if(!m_cur_vertex_prog)
	{
		ConLog.Warning("LoadProgram: m_cur_vertex_prog == NULL");

		return false;
	}

	if(m_program.IsCreated())
	{
		m_program.Use();
		return true;
	}
	//ConLog.Write("Create program");
	m_vp_buf_num = m_prog_buffer.SearchVp(*m_cur_vertex_prog);

	if(m_vp_buf_num == -1) 
	{
		ConLog.Warning("VP not found in buffer!");
		m_cur_vertex_prog->Decompile();
	}

	if(m_fp_buf_num == -1)
	{
		ConLog.Warning("FP not found in buffer!");
		m_cur_shader_prog->Wait();
		m_cur_shader_prog->Compile();

		wxFile f(wxGetCwd() + "/FragmentProgram.txt", wxFile::write);
		f.Write(m_cur_shader_prog->shader);
	}

	if(m_vp_buf_num == -1)
	{
		m_cur_vertex_prog->Wait();
		m_cur_vertex_prog->Compile();

		wxFile f(wxGetCwd() + "/VertexProgram.txt", wxFile::write);
		f.Write(m_cur_vertex_prog->shader);
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
		m_program.Create(m_cur_vertex_prog->id, m_cur_shader_prog->id);
		checkForGlError("m_program.Create");
		m_prog_buffer.Add(m_program, *m_cur_shader_prog, *m_cur_vertex_prog);
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

	u32 address = GetAddress(m_depth_offset, m_context_dma_z - 0xfeed0000);
	if(!Memory.IsGoodAddr(address))
	{
		ConLog.Warning("Bad depth address: address=0x%x, offset=0x%x, dma=0x%x", address, m_depth_offset, m_context_dma_z);
		return;
	}

	u8* dst = &Memory[address];
	//m_fbo.Bind(GL_READ_FRAMEBUFFER);
	//glPixelStorei(GL_PACK_ROW_LENGTH, m_depth_pitch);

	m_fbo.Bind(GL_READ_FRAMEBUFFER);
	GLuint depth_tex;
	glGenTextures(1, &depth_tex);
	glBindTexture(GL_TEXTURE_2D, depth_tex);
	glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 0, 0, m_width, m_height, 0);
	checkForGlError("glCopyTexImage2D");
	glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, dst);
	checkForGlError("glGetTexImage");
	glDeleteTextures(1, &depth_tex);
	wxFile f("depth.raw", wxFile::write);
	f.Write(dst, m_width * m_height * 4);
	m_fbo.Bind();
	/*
	GLBufferObject pbo;
	pbo.Create(GL_PIXEL_PACK_BUFFER);
	pbo.Bind();
	glReadPixels(0, 0, m_width, m_height, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	GLuint *src = (GLuint*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	if(src)
	{
		wxFile f("depth.raw", wxFile::write);
		f.Write(src, m_width * m_height * 4);
		memcpy(dst, src, m_width * m_height * 4);
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	}
	else
	{
		ConLog.Error("glMapBuffer failed.");
	}
	pbo.UnBind();
	pbo.Delete();
	*/
	Emu.Pause();
}

void GLGSRender::ExecCMD()
{
	if(LoadProgram())
	{
		if(m_set_surface_clip_horizontal && m_set_surface_clip_vertical)
		{
			//ConLog.Write("surface clip width: %d, height: %d, x: %d, y: %d", m_width, m_height, m_surface_clip_x, m_surface_clip_y);
		}

		if(m_set_color_mask)
		{
			glColorMask(m_color_mask_r, m_color_mask_g, m_color_mask_b, m_color_mask_a);
			checkForGlError("glColorMask");
		}

		if(m_set_viewport_horizontal && m_set_viewport_vertical)
		{
			glViewport(m_scissor_x, m_height-m_viewport_y-m_viewport_h, m_viewport_w, m_viewport_h);
			checkForGlError("glViewport");
		}

		if(m_set_scissor_horizontal && m_set_scissor_vertical)
		{
			glScissor(m_scissor_x, m_height-m_scissor_y-m_scissor_h, m_scissor_w, m_scissor_h);
			checkForGlError("glScissor");
		}

		if(m_set_clear_color)
		{
			glClearColor(
				m_clear_color_r / 255.0f,
				m_clear_color_g / 255.0f,
				m_clear_color_b / 255.0f,
				m_clear_color_a / 255.0f);
		}

		if(m_set_clear_surface)
		{
			GLbitfield f = 0;
			if (m_clear_surface_mask & 0x1) f |= GL_DEPTH_BUFFER_BIT;
			if (m_clear_surface_mask & 0x2) f |= GL_STENCIL_BUFFER_BIT;
			if ((m_clear_surface_mask & 0xF0) == 0xF0) f |= GL_COLOR_BUFFER_BIT;
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
		checkForGlError("glEnable");

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

		if(m_set_two_sided_stencil_test_enable)
		{
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
			GLTexture& tex = m_frame->GetTexture(i);
			if(!tex.IsEnabled()) continue;

			glActiveTexture(GL_TEXTURE0 + i);
			checkForGlError("glActiveTexture");
			tex.Create();
			tex.Bind();
			checkForGlError("tex.Bind");
			m_program.SetTex(i);
			tex.Init();
			checkForGlError("tex.Init");
			//tex.Save();
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
				glDrawElements(m_draw_mode, m_indexed_array.m_count, GL_UNSIGNED_INT, nullptr);
				checkForGlError("glDrawElements #4");
			break;

			case 1:
				glDrawElements(m_draw_mode, m_indexed_array.m_count, GL_UNSIGNED_SHORT, nullptr);
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
			glDrawArrays(m_draw_mode, 0, m_draw_array_count);
			checkForGlError("glDrawArrays");
			DisableVertexData();
			m_draw_array_count = 0;
		}

		m_fragment_constants.Clear();

		//WriteDepthBuffer();
	}
	else
	{
		ConLog.Error("LoadProgram failed.");
		Emu.Pause();
	}

	Reset();
}

void GLGSRender::Reset()
{
	m_program.UnUse();
	//m_prog_buffer.Clear();
	if(m_cur_shader_prog)
		m_cur_shader_prog->id = 0;

	if(m_cur_vertex_prog)
		m_cur_vertex_prog->id = 0;

	m_cur_shader_prog_num = 0;
	m_transform_constants.Clear();
	for(uint i=0; i<m_vertex_count; ++i)
	{
		m_vertex_data[i].Reset();
	}

	if(m_vbo.IsCreated())
	{
		m_vbo.UnBind();
		m_vbo.Delete();
	}

	m_vao.Delete();

	Init();
}

void GLGSRender::Init()
{
	m_draw_array_count = 0;
	m_draw_mode = 0;
	ExecRSXCMDdata::Reset();
}

void GLGSRender::CloseOpenGL()
{
	//Reset();
}