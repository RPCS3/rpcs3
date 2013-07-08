#include "stdafx.h"
#include "GLGSRender.h"

#define CMD_DEBUG 0
#define DUMP_VERTEX_DATA 1

#if	CMD_DEBUG
	#define CMD_LOG ConLog.Write
#else
	#define CMD_LOG(...)
#endif

gcmBuffer gcmBuffers[2];

void checkForGlError(const char* situation)
{
	GLenum err = glGetError();
	if(err != GL_NO_ERROR)
	{
		ConLog.Error("%s: opengl error 0x%04x", situation, err);
		Emu.Pause();
	}
}

#if 0
	#define checkForGlError(x) /*x*/
#endif

GLGSFrame::GLGSFrame()
	: GSFrame(NULL, "GSFrame[OpenGL]")
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
	: m_frame(NULL)
	, m_rsx_thread(NULL)
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
	glSwapInterval(Ini.GSVSyncEnable.GetValue() ? 1 : 0);

	bool draw = true;
	u32 drawed = 0;
	u32 skipped = 0;

	p.Init();

	while(!TestDestroy() && p.m_frame && !p.m_frame->IsBeingDeleted())
	{
		wxCriticalSectionLocker lock(p.m_cs_main);

		if(p.m_ctrl->get == p.m_ctrl->put || !Emu.IsRunned())
		{
			SemaphorePostAndWait(p.m_sem_flush);

			if(p.m_draw)
			{
				p.m_draw = false;

				if(p.m_skip_frames)
				{
					if(!draw)
					{
						if(skipped++ >= p.m_skip_frames)
						{
							skipped = 0;
							draw = true;
						}
					}
					else
					{
						if(drawed++ >= p.m_draw_frames)
						{
							drawed = 0;
							draw = false;
						}
					}
				}
				
				if(draw) p.m_frame->Flip();

				p.m_flip_status = 0;
				if(SemaphorePostAndWait(p.m_sem_flip)) continue;
			}

			Sleep(1);
			continue;
		}

		const u32 get = re(p.m_ctrl->get);
		const u32 cmd = Memory.Read32(p.m_ioAddress + get);
		const u32 count = (cmd >> 18) & 0x7ff;

		if(cmd & CELL_GCM_METHOD_FLAG_JUMP)
		{
			u32 addr = cmd & ~(CELL_GCM_METHOD_FLAG_JUMP | CELL_GCM_METHOD_FLAG_NON_INCREMENT);
			p.m_ctrl->get = re32(addr);
			ConLog.Warning("rsx jump(0x%x)", addr);
			continue;
		}
		if(cmd & CELL_GCM_METHOD_FLAG_CALL)
		{
			call_stack.Push(get + 4);
			u32 addr = cmd & ~CELL_GCM_METHOD_FLAG_CALL;
			p.m_ctrl->get = re32(addr);
			ConLog.Warning("rsx call(0x%x)", addr);
			continue;
		}
		if(cmd & CELL_GCM_METHOD_FLAG_RETURN)
		{
			u32 addr = call_stack.Pop();
			p.m_ctrl->get = re32(addr);
			ConLog.Warning("rsx return(0x%x)", addr);
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

		mem32_t args(p.m_ioAddress + get + 4);

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

	m_frame->Show();

	m_ioAddress = ioAddress;
	m_ioSize = ioSize;
	m_ctrlAddress = ctrlAddress;
	m_localAddress = localAddress;
	m_ctrl = (CellGcmControl*)Memory.GetMemFromAddr(m_ctrlAddress);

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
	m_ctrl = NULL;
}

void GLGSRender::EnableVertexData(bool indexed_draw)
{
	Array<u32> offset_list;
	u32 cur_offset = 0;

	for(u32 i=0; i<16; ++i)
	{
		offset_list.AddCpy(cur_offset);

		if(!m_vertex_data[i].IsEnabled()) continue;

		cur_offset += m_vertex_data[i].data.GetCount();
		const u32 pos = m_vdata.GetCount();
		m_vdata.SetCount(pos + m_vertex_data[i].data.GetCount());
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

	for(u32 i=0; i<16; ++i)
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
			u32 gltype = gl_types[m_vertex_data[i].type - 1];
			bool normalized = gl_normalized[m_vertex_data[i].type - 1];

			glEnableVertexAttribArray(i);
			checkForGlError("glEnableVertexAttribArray");
			glVertexAttribPointer(i, m_vertex_data[i].size, gltype, normalized, 0, (void*)offset_list[i]);
			checkForGlError("glVertexAttribPointer");
		}
	}
}

void GLGSRender::DisableVertexData()
{
	m_vdata.Clear();
	for(u32 i=0; i<16; ++i)
	{
		if(!m_vertex_data[i].IsEnabled()) continue;
		m_vertex_data[i].data.Clear();
		glDisableVertexAttribArray(i);
	}
}

void GLGSRender::LoadVertexData(u32 first, u32 count)
{
	for(u32 i=0; i<16; ++i)
	{
		if(!m_vertex_data[i].IsEnabled()) continue;

		m_vertex_data[i].Load(first, count);
	}
}

void GLGSRender::InitVertexData()
{
	for(u32 i=0; i<m_cur_vertex_prog->constants4.GetCount(); ++i)
	{
		const VertexProgram::Constant4& c = m_cur_vertex_prog->constants4[i];
		const wxString& name = wxString::Format("vc%d", c.id);
		//const int l = glGetUniformLocation(m_program.id, name);
		const int l = m_program.GetLocation(name);
		checkForGlError("glGetUniformLocation " + name);

		//ConLog.Write(name + " x: %.02f y: %.02f z: %.02f w: %.02f", c.x, c.y, c.z, c.w);
		glUniform4f(l, c.x, c.y, c.z, c.w);
		checkForGlError("glUniform4f " + name);
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

void GLGSRender::DoCmd(const u32 fcmd, const u32 cmd, mem32_t& args, const u32 count)
{
#if	CMD_DEBUG
		wxString debug = GetMethodName(cmd);
		debug += "(";
		for(u32 i=0; i<count; ++i) debug += (i ? ", " : "") + wxString::Format("0x%x", args[i]);
		debug += ")";
		ConLog.Write(debug);
#endif

	//static int draw_mode = 0;
	static u32 semaphore_offset = 0;
	u32 index = 0;

	//static u32 draw_array_count = 0;

	switch(cmd)
	{
	case NV4097_NO_OPERATION:
	break;

	case NV406E_SET_REFERENCE:
		m_ctrl->ref = re32(args[0]);
	break;

	case_16(NV4097_SET_TEXTURE_OFFSET, 0x20):
	{
		GLTexture& tex = m_frame->GetTexture(index);
		const u32 offset = args[0];
		const u8 location = args[1] & 0x3 - 1;
		const bool cubemap = (args[1] >> 2) & 0x1;
		const u8 dimension = (args[1] >> 4) & 0xf;
		const u8 format = (args[1] >> 8) & 0xff;
		const u16 mipmap = (args[1] >> 16) & 0xffff;
		CMD_LOG("index = %d, offset=0x%x, location=0x%x, cubemap=0x%x, dimension=0x%x, format=0x%x, mipmap=0x%x",
			index, offset, location, cubemap, dimension, format, mipmap);

		tex.SetOffset(GetAddress(offset, location));
		tex.SetFormat(cubemap, dimension, format, mipmap);
	}
	break;

	case_16(NV4097_SET_TEXTURE_CONTROL0, 0x20):
	{
		GLTexture& tex = m_frame->GetTexture(index);
		tex.Enable(args[0] >> 31 ? true : false);
	}
	break;

	case_16(NV4097_SET_VERTEX_DATA4UB_M, 4):
	{
		u32 v = args[0];
		u8 v0 = v;
		u8 v1 = v >> 8;
		u8 v2 = v >> 16;
		u8 v3 = v >> 24;

		m_vertex_data[index].type = 7;
		m_vertex_data[index].data.AddCpy(v0);
		m_vertex_data[index].data.AddCpy(v1);
		m_vertex_data[index].data.AddCpy(v2);
		m_vertex_data[index].data.AddCpy(v3);
		ConLog.Warning("index = %d, v0 = 0x%x, v1 = 0x%x, v2 = 0x%x, v3 = 0x%x", index, v0, v1, v2, v3);
	}
	break;

	case_16(NV4097_SET_TEXTURE_CONTROL1, 0x20):
		//TODO
	break;

	case_16(NV4097_SET_TEXTURE_CONTROL3, 4):
		//TODO
	break;

	case_16(NV4097_SET_TEXTURE_FILTER, 0x20):
		//TODO
	break;

	case_16(NV4097_SET_TEXTURE_ADDRESS, 0x20):
		//TODO
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
		const u32 color_format = args[0] & 0x1f;
		const u32 depth_format = (args[0] >> 5) & 0x7;
		const u32 type = (args[0] >> 8) & 0xf;
		const u32 antialias = (args[0] >> 12) & 0xf;
		const u32 width = (args[0] >> 16) & 0xff;
		const u32 height = (args[0] >> 24) & 0xff;
		const u32 pitch_a = args[1];
		const u32 offset_a = args[2];
		const u32 offset_z = args[3];
		const u32 offset_b = args[4];
		const u32 pitch_b = args[5];

		CMD_LOG("color_format=%d, depth_format=%d, type=%d, antialias=%d, width=%d, height=%d, pitch_a=%d, offset_a=0x%x, offset_z=0x%x, offset_b=0x%x, pitch_b=%d",
			color_format, depth_format, type, antialias, width, height, pitch_a, offset_a, offset_z, offset_b, pitch_b);
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
		//Enable(args[0] ? true : false, GL_ALPHA_TEST);
	break;

	case NV4097_SET_BLEND_ENABLE:
		m_set_blend = args[0] ? true : false;
		//Enable(args[0] ? true : false, GL_BLEND);
	break;

	case NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE:
		m_set_depth_bounds_test = args[0] ? true : false;
		//Enable(args[0] ? true : false, GL_DEPTH_CLAMP);
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

		//m_frame->SetViewport(m_viewport_x, m_viewport_y, m_viewport_w, m_viewport_h);
		//glViewport(x, y, w, h);
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

		//glDepthRangef(m_clip_min, m_clip_max);
	}
	break;

	case NV4097_SET_DEPTH_FUNC:
		m_set_depth_func = true;
		m_depth_func = args[0];
		//glDepthFunc(m_depth_func);
	break;

	case NV4097_SET_DEPTH_TEST_ENABLE:
		m_depth_test_enable = args[0] ? true : false;
		//Enable(args[0] ? true : false, GL_DEPTH_TEST);
	break;

	case NV4097_SET_FRONT_POLYGON_MODE:
		glPolygonMode(GL_FRONT, args[0]);
	break;

	case NV4097_CLEAR_SURFACE:
	{
		const u32 mask = args[0];
		GLbitfield f = 0;
		if (mask & 0x1) f |= GL_DEPTH_BUFFER_BIT;
		if (mask & 0x2) f |= GL_STENCIL_BUFFER_BIT;
		if (mask & 0x10) f |= GL_COLOR_BUFFER_BIT;
		glClear(f);
	}
	break;

	case NV4097_SET_BLEND_FUNC_SFACTOR:
	{
		const u16 src_rgb = args[0] & 0xffff;
		const u16 dst_rgb = args[0] >> 16;
		const u16 src_alpha = args[1] & 0xffff;
		const u16 dst_alpha = args[1] >> 16;
		CMD_LOG("src_rgb=0x%x, dst_rgb=0x%x, src_alpha=0x%x, dst_alpha=0x%x",
			src_rgb, dst_rgb, src_alpha, dst_alpha);

		glBlendFuncSeparate(src_rgb, dst_rgb, src_alpha, dst_alpha);
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
		const u16 frequency = args[0] >> 16;
		const u8 stride = (args[0] >> 8) & 0xff;
		const u8 size = (args[0] >> 4) & 0xf;
		const u8 type = args[0] & 0xf;

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
			const u32 first = args[c] & 0xffffff;
			const u32 _count = (args[c] >> 24) + 1;

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
			m_draw_mode = args[0] - 1;
		}
		else
		{
			//end
			ExecCMD();
		}
	}
	break;

	case NV4097_SET_COLOR_CLEAR_VALUE:
	{
		const u8 a = (args[0] >> 24) & 0xff;
		const u8 r = (args[0] >> 16) & 0xff;
		const u8 g = (args[0] >> 8) & 0xff;
		const u8 b = args[0] & 0xff;
		CMD_LOG("a=%d, r=%d, g=%d, b=%d", a, r, g, b);
		glClearColor(
			(float)r / 255.0f,
			(float)g / 255.0f,
			(float)b / 255.0f,
			(float)a / 255.0f);
	}
	break;

	case NV4097_SET_SHADER_PROGRAM:
	{
		m_shader_prog.Delete();
		m_shader_prog.addr = GetAddress(args[0] & ~0x3, (args[0] & 0x3) - 1);
		//m_program.Delete();
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
		//m_program.Delete();
		m_cur_vertex_prog = &m_vertex_progs[args[0]];
		m_cur_vertex_prog->Delete();

		if(count == 2)
		{
			const u32 start = args[1];
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

		if(!m_cur_vertex_prog)
		{
			ConLog.Warning("NV4097_SET_TRANSFORM_CONSTANT_LOAD: m_cur_vertex_prog == NULL");
			break;
		}

		for(u32 id = args[0], i = 1; i<count; ++id)
		{
			const u32 x = args[i++];
			const u32 y = args[i++];
			const u32 z = args[i++];
			const u32 w = args[i++];

			VertexProgram::Constant4 c;
			c.id = id;
			c.x = (float&)x;
			c.y = (float&)y;
			c.z = (float&)z;
			c.w = (float&)w;

			CMD_LOG("SET_TRANSFORM_CONSTANT_LOAD[%d : %d] = (%f, %f, %f, %f)", i, id, c.x, c.y, c.z, c.w);
			m_cur_vertex_prog->constants4.AddCpy(c);
		}
	}
	break;

	case NV4097_SET_LOGIC_OP_ENABLE:
		Enable(args[0] ? true : false, GL_LOGIC_OP);
	break;

	case NV4097_SET_CULL_FACE_ENABLE:
		Enable(args[0] ? true : false, GL_CULL_FACE);
	break;

	case NV4097_SET_DITHER_ENABLE:
		Enable(args[0] ? true : false, GL_DITHER);
	break;

	case NV4097_SET_STENCIL_TEST_ENABLE:
		Enable(args[0] ? true : false, GL_STENCIL_TEST);
	break;

	case NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE:
		if(args[0]) ConLog.Error("NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE");
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
		if(args[0]) ConLog.Error("NV4097_SET_POLY_SMOOTH_ENABLE");
	break;

	case NV4097_SET_BLEND_COLOR:
		glBlendColor(args[0] & 0xff, (args[0] >> 8) & 0xff, 
			(args[0] >> 16) & 0xff, (args[0] >> 24) & 0xff);
	break;

	case NV4097_SET_BLEND_COLOR2:
		if(args[0]) ConLog.Error("NV4097_SET_BLEND_COLOR2");
	break;

	case NV4097_SET_BLEND_EQUATION:
		glBlendEquationSeparate(args[0] & 0xffff, args[0] >> 16);
		//glBlendEquation
	break;

	case NV4097_SET_REDUCE_DST_COLOR:
		if(args[0]) ConLog.Error("NV4097_SET_REDUCE_DST_COLOR");
	break;

	case NV4097_SET_DEPTH_MASK:
		glDepthMask(args[0]);
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
		
		CMD_LOG("x=%d, y=%d, w=%d, h=%d", m_scissor_x, m_scissor_y, m_scissor_w, m_scissor_h);

		//glScissor(m_scissor_x, m_scissor_y, m_scissor_w, m_scissor_h);
	}
	break;

	case NV4097_INVALIDATE_VERTEX_FILE: break;

	case NV4097_SET_VIEWPORT_OFFSET:
	{
		//TODO
	}
	break;

	case NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE:
	{
		//TODO
	}
	break;

	case NV4097_SET_SEMAPHORE_OFFSET:
	case NV406E_SEMAPHORE_OFFSET:
	{
		semaphore_offset = args[0];
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
		//TODO
	}
	break;

	case NV4097_SET_CONTEXT_DMA_COLOR_B:
	{
		//TODO
	}
	break;

	case NV4097_SET_CONTEXT_DMA_COLOR_C:
	{
		//TODO
	}
	break;

	case NV4097_SET_CONTEXT_DMA_ZETA:
	{
		//TODO
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
		//TODO
	}
	break;

	case NV4097_SET_SHADER_WINDOW:
	{
		//TODO
	}
	break;

	case NV4097_SET_SURFACE_CLIP_HORIZONTAL:
	{
		//TODO
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

	case NV4097_SET_ZSTENCIL_CLEAR_VALUE:
	case NV4097_SET_ZCULL_CONTROL0:
	case NV4097_SET_ZCULL_CONTROL1:
	case NV4097_SET_SCULL_CONTROL:
	break;

	default:
	{
		wxString log = GetMethodName(cmd);
		log += "(";
		for(u32 i=0; i<count; ++i) log += (i ? ", " : "") + wxString::Format("0x%x", args[i]);
		log += ")";
		ConLog.Error("TODO: " + log);
		Emu.Pause();
	}
	break;
	}
}

bool GLGSRender::LoadProgram()
{
	m_fp_buf_num = m_prog_buffer.SearchFp(m_shader_prog);

	if(m_fp_buf_num == -1) m_shader_prog.Decompile();

	if(!m_cur_vertex_prog)
	{
		ConLog.Warning("NV4097_SET_BEGIN_END: m_cur_vertex_prog == NULL");

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
		m_shader_prog.Wait();
		m_shader_prog.Compile();

		wxFile f(wxGetCwd() + "/FragmentProgram.txt", wxFile::write);
		f.Write(m_shader_prog.shader);
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

	if(!m_program.id)
	{
		m_program.Create(m_cur_vertex_prog->id, m_shader_prog.id);
		m_prog_buffer.Add(m_program, m_shader_prog, *m_cur_vertex_prog);

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

void GLGSRender::ExecCMD()
{
	if(LoadProgram())
	{
		if(m_set_color_mask)
		{
			glColorMask(m_color_mask_r, m_color_mask_g, m_color_mask_b, m_color_mask_a);
		}

		if(m_set_viewport_horizontal && m_set_viewport_vertical)
		{
			glViewport(m_viewport_x, m_viewport_y, m_viewport_w, m_viewport_h);
			if(m_frame->GetClientSize() != wxSize(m_viewport_w, m_viewport_h))
				m_frame->SetClientSize(m_viewport_w, m_viewport_h);
			//m_frame->SetViewport(m_viewport_x, m_viewport_y, m_viewport_w, m_viewport_h);
		}

		if(m_set_scissor_horizontal && m_set_scissor_vertical)
		{
			glScissor(m_scissor_x, m_scissor_y, m_scissor_w, m_scissor_h);
		}

		Enable(m_depth_test_enable, GL_DEPTH_TEST);
		Enable(m_set_alpha_test, GL_ALPHA_TEST);
		Enable(m_set_depth_bounds_test, GL_DEPTH_CLAMP);
		Enable(m_set_blend, GL_BLEND);

		if(m_set_depth_func)
		{
			glDepthFunc(m_depth_func);
		}

		if(m_set_clip)
		{
			glDepthRangef(m_clip_min, m_clip_max);
		}

		for(u32 i=0; i<16; ++i)
		{
			GLTexture& tex = m_frame->GetTexture(i);
			if(!tex.IsEnabled()) continue;

			glActiveTexture(GL_TEXTURE0_ARB + i);
			checkForGlError("glActiveTexture");
			tex.Bind();
			checkForGlError("tex.Bind");
			m_program.SetTex(i);
			checkForGlError("m_program.SetTex");
			tex.Init();
			checkForGlError("tex.Init");
			tex.Save();
		}

		if(m_indexed_array.m_count && m_draw_array_count)
		{
			ConLog.Warning("m_indexed_array.m_count && draw_array_count");
		}

		if(m_indexed_array.m_count)
		{
			LoadVertexData(m_indexed_array.index_min, m_indexed_array.index_max - m_indexed_array.index_min + 1);
			EnableVertexData(true);
			InitVertexData();

			wxFile log("IndexedDrawLog.txt", wxFile::write);
			log.Write(wxString::Format("Draw mode: %d\n", m_draw_mode));

			m_vao.Bind();
			switch(m_indexed_array.m_type)
			{
			case 0:
				glDrawElements(m_draw_mode, m_indexed_array.m_count, GL_UNSIGNED_INT, nullptr);
				checkForGlError("glDrawElements #4");
				//for(uint i=0; i<m_indexed_array.m_count; i+=4) log.Write(wxString::Format("index 4: %d\n", (u32&)m_indexed_array.m_data[i]));
			break;

			case 1:
				glDrawElements(m_draw_mode, m_indexed_array.m_count, GL_UNSIGNED_SHORT, nullptr);
				checkForGlError("glDrawElements #2");
				//for(uint i=0; i<m_indexed_array.m_count; i+=2) log.Write(wxString::Format("index 2: %d\n", (u16&)m_indexed_array.m_data[i]));
			break;

			default:
				ConLog.Error("Bad indexed array type (%d)", m_indexed_array.m_type);
			break;
			}

			DisableVertexData();
			m_indexed_array.Reset();
			//Emu.Pause();
		}

		if(m_draw_array_count)
		{
			EnableVertexData();
			InitVertexData();
			m_vao.Bind();
			glDrawArrays(m_draw_mode, 0, m_draw_array_count);
			checkForGlError("glDrawArrays");
			DisableVertexData();
			m_draw_array_count = 0;
		}
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
	m_program.Delete();
	//m_program.id = 0;
	m_shader_prog.id = 0;
	if(m_cur_vertex_prog)
		m_cur_vertex_prog->id = 0;

	memset(m_vertex_data, 0, sizeof(VertexData) * 16);

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
	Reset();
}