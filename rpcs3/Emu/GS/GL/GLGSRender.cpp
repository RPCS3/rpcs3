#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "GLGSRender.h"
#include "Emu/Cell/PPCInstrTable.h"

#define CMD_DEBUG 0
#define DUMP_VERTEX_DATA 0

#if CMD_DEBUG
	#define CMD_LOG(...) LOG_NOTICE(RSX, __VA_ARGS__)
#else
	#define CMD_LOG(...)
#endif

gcmBuffer gcmBuffers[8];
GLuint g_flip_tex, g_depth_tex, g_pbo[6];
int last_width = 0, last_height = 0, last_depth_format = 0;

GLenum g_last_gl_error = GL_NO_ERROR;
void printGlError(GLenum err, const char* situation)
{
	if(err != GL_NO_ERROR)
	{
		LOG_ERROR(RSX, "%s: opengl error 0x%04x", situation, err);
		Emu.Pause();
	}
}
void printGlError(GLenum err, const std::string& situation)
{
	printGlError(err, situation.c_str());
}

#if 0
	#define checkForGlError(x) /*x*/
#endif

GLGSRender::GLGSRender()
	: GSRender()
	, m_frame(nullptr)
	, m_fp_buf_num(-1)
	, m_vp_buf_num(-1)
	, m_context(nullptr)
{
	m_frame = new rGLFrame();// new GLGSFrame();
}

GLGSRender::~GLGSRender()
{
	m_frame->Close();
//	delete m_context; // This won't do anything (deleting void* instead of wglContext*)
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
	Stop();

	if(m_frame->IsShown()) m_frame->Hide();
	m_ctrl = nullptr;
}

void GLGSRender::EnableVertexData(bool indexed_draw)
{
	static u32 offset_list[m_vertex_count];
	u32 cur_offset = 0;

	const u32 data_offset = indexed_draw ? 0 : m_draw_array_first;

	for(u32 i=0; i<m_vertex_count; ++i)
	{
		offset_list[i] = cur_offset;

		if(!m_vertex_data[i].IsEnabled() || !m_vertex_data[i].addr) continue;

		const size_t item_size = m_vertex_data[i].GetTypeSize() * m_vertex_data[i].size;
		const size_t data_size = m_vertex_data[i].data.size() - data_offset * item_size;
		const u32 pos = m_vdata.size();

		cur_offset += data_size;
		m_vdata.resize(m_vdata.size() + data_size);
		memcpy(&m_vdata[pos], &m_vertex_data[i].data[data_offset * item_size], data_size);
	}

	m_vao.Create();
	m_vao.Bind();
	checkForGlError("initializing vao");

	m_vbo.Create(indexed_draw ? 2 : 1);
	m_vbo.Bind(0);
	m_vbo.SetData(&m_vdata[0], m_vdata.size());

	if(indexed_draw)
	{
		m_vbo.Bind(GL_ELEMENT_ARRAY_BUFFER, 1);
		m_vbo.SetData(GL_ELEMENT_ARRAY_BUFFER, &m_indexed_array.m_data[0], m_indexed_array.m_data.size());
	}

	checkForGlError("initializing vbo");

#if	DUMP_VERTEX_DATA
	rFile dump("VertexDataArray.dump", rFile::write);
#endif

	for(u32 i=0; i<m_vertex_count; ++i)
	{
		if(!m_vertex_data[i].IsEnabled()) continue;

#if	DUMP_VERTEX_DATA
		dump.Write(wxString::Format("VertexData[%d]:\n", i));
		switch(m_vertex_data[i].type)
		{
		case CELL_GCM_VERTEX_S1:
			for(u32 j = 0; j<m_vertex_data[i].data.size(); j+=2)
			{
				dump.Write(wxString::Format("%d\n", *(u16*)&m_vertex_data[i].data[j]));
				if(!(((j+2) / 2) % m_vertex_data[i].size)) dump.Write("\n");
			}
		break;

		case CELL_GCM_VERTEX_F:
			for(u32 j = 0; j<m_vertex_data[i].data.size(); j+=4)
			{
				dump.Write(wxString::Format("%.01f\n", *(float*)&m_vertex_data[i].data[j]));
				if(!(((j+4) / 4) % m_vertex_data[i].size)) dump.Write("\n");
			}
		break;

		case CELL_GCM_VERTEX_SF:
			for(u32 j = 0; j<m_vertex_data[i].data.size(); j+=2)
			{
				dump.Write(wxString::Format("%.01f\n", *(float*)&m_vertex_data[i].data[j]));
				if(!(((j+2) / 2) % m_vertex_data[i].size)) dump.Write("\n");
			}
		break;

		case CELL_GCM_VERTEX_UB:
			for(u32 j = 0; j<m_vertex_data[i].data.size(); ++j)
			{
				dump.Write(wxString::Format("%d\n", m_vertex_data[i].data[j]));
				if(!((j+1) % m_vertex_data[i].size)) dump.Write("\n");
			}
		break;

		case CELL_GCM_VERTEX_S32K:
			for(u32 j = 0; j<m_vertex_data[i].data.size(); j+=2)
			{
				dump.Write(wxString::Format("%d\n", *(u16*)&m_vertex_data[i].data[j]));
				if(!(((j+2) / 2) % m_vertex_data[i].size)) dump.Write("\n");
			}
		break;
		
		// case CELL_GCM_VERTEX_CMP:
		
		case CELL_GCM_VERTEX_UB256:
			for(u32 j = 0; j<m_vertex_data[i].data.size(); ++j)
			{
				dump.Write(wxString::Format("%d\n", m_vertex_data[i].data[j]));
				if(!((j+1) % m_vertex_data[i].size)) dump.Write("\n");
			}
		break;

		default:
			LOG_ERROR(HLE, "Bad cv type! %d", m_vertex_data[i].type);
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
				case CELL_GCM_VERTEX_S32K:
				case CELL_GCM_VERTEX_S1:
					switch(m_vertex_data[i].size)
					{
					case 1: glVertexAttrib1s(i, (GLshort&)m_vertex_data[i].data[0]); break;
					case 2: glVertexAttrib2sv(i, (GLshort*)&m_vertex_data[i].data[0]); break;
					case 3: glVertexAttrib3sv(i, (GLshort*)&m_vertex_data[i].data[0]); break;
					case 4: glVertexAttrib4sv(i, (GLshort*)&m_vertex_data[i].data[0]); break;
					}
				break;

				case CELL_GCM_VERTEX_F:
					switch(m_vertex_data[i].size)
					{
					case 1: glVertexAttrib1f(i, (GLfloat&)m_vertex_data[i].data[0]); break;
					case 2: glVertexAttrib2fv(i, (GLfloat*)&m_vertex_data[i].data[0]); break;
					case 3: glVertexAttrib3fv(i, (GLfloat*)&m_vertex_data[i].data[0]); break;
					case 4: glVertexAttrib4fv(i, (GLfloat*)&m_vertex_data[i].data[0]); break;
					}
				break;

				case CELL_GCM_VERTEX_CMP:
				case CELL_GCM_VERTEX_UB:
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
				glVertexAttribPointer(i, m_vertex_data[i].size, gltype, normalized, 0, reinterpret_cast<void*>(offset_list[i]));
				checkForGlError("glVertexAttribPointer");
			}
		}
	}
}

void GLGSRender::DisableVertexData()
{
	m_vdata.clear();
	for(u32 i=0; i<m_vertex_count; ++i)
	{
		if(!m_vertex_data[i].IsEnabled() || !m_vertex_data[i].addr) continue;
		glDisableVertexAttribArray(i);
		checkForGlError("glDisableVertexAttribArray");
	}
	m_vao.Unbind();
}

void GLGSRender::InitVertexData()
{
	GLfloat scaleOffsetMat[16] = {1.0f, 0.0f, 0.0f, 0.0f, 
								  0.0f, 1.0f, 0.0f, 0.0f, 
								  0.0f, 0.0f, 1.0f, 0.0f, 
								  0.0f, 0.0f, 0.0f, 1.0f};
	int l;

	for(u32 i=0; i<m_transform_constants.size(); ++i)
	{
		const RSXTransformConstant& c = m_transform_constants[i];
		const std::string name = fmt::Format("vc[%u]", c.id);
		l = m_program.GetLocation(name);
		checkForGlError("glGetUniformLocation " + name);

		//ConLog.Write(name + " x: %.02f y: %.02f z: %.02f w: %.02f", c.x, c.y, c.z, c.w);
		glUniform4f(l, c.x, c.y, c.z, c.w);
		checkForGlError("glUniform4f " + name + fmt::Format(" %d [%f %f %f %f]", l, c.x, c.y, c.z, c.w));
	}

	// Scale
	scaleOffsetMat[0] = (GLfloat&)methodRegisters[NV4097_SET_VIEWPORT_SCALE + (0x4 * 0)] / (RSXThread::m_width / RSXThread::m_width_scale);
	scaleOffsetMat[5] = (GLfloat&)methodRegisters[NV4097_SET_VIEWPORT_SCALE + (0x4 * 1)] / (RSXThread::m_height / RSXThread::m_height_scale);
	scaleOffsetMat[10] = (GLfloat&)methodRegisters[NV4097_SET_VIEWPORT_SCALE + (0x4 * 2)];

	// Offset
	scaleOffsetMat[3] = (GLfloat&)methodRegisters[NV4097_SET_VIEWPORT_OFFSET + (0x4 * 0)] - (RSXThread::m_width / RSXThread::m_width_scale);
	scaleOffsetMat[7] = (GLfloat&)methodRegisters[NV4097_SET_VIEWPORT_OFFSET + (0x4 * 1)] - (RSXThread::m_height / RSXThread::m_height_scale);
	scaleOffsetMat[11] = (GLfloat&)methodRegisters[NV4097_SET_VIEWPORT_OFFSET + (0x4 * 2)] - 1 / 2.0f;

	scaleOffsetMat[3] /= RSXThread::m_width / RSXThread::m_width_scale;
	scaleOffsetMat[7] /= RSXThread::m_height / RSXThread::m_height_scale;

	l = m_program.GetLocation("scaleOffsetMat");
	glUniformMatrix4fv(l, 1, false, scaleOffsetMat);
	checkForGlError("glUniformMatrix4fv");
}

void GLGSRender::InitFragmentData()
{
	if(!m_cur_shader_prog)
	{
		LOG_ERROR(RSX, "InitFragmentData: m_cur_shader_prog == NULL");
		return;
	}

	for(u32 i=0; i<m_fragment_constants.size(); ++i)
	{
		const RSXTransformConstant& c = m_fragment_constants[i];

		u32 id = c.id - m_cur_shader_prog->offset;

		//LOG_WARNING(RSX,"fc%u[0x%x - 0x%x] = (%f, %f, %f, %f)", id, c.id, m_cur_shader_prog->offset, c.x, c.y, c.z, c.w);

		const std::string name = fmt::Format("fc%u", id);
		const int l = m_program.GetLocation(name);
		checkForGlError("glGetUniformLocation " + name);

		glUniform4f(l, c.x, c.y, c.z, c.w);
		checkForGlError("glUniform4f " + name + fmt::Format(" %u [%f %f %f %f]", l, c.x, c.y, c.z, c.w));
	}

	//if(m_fragment_constants.GetCount())
	//	LOG_NOTICE(HLE, "");
}

bool GLGSRender::LoadProgram()
{
	if(!m_cur_shader_prog)
	{
		LOG_WARNING(RSX, "LoadProgram: m_cur_shader_prog == NULL");
		return false;
	}
	
	m_cur_shader_prog->ctrl = m_shader_ctrl;
	
	if(!m_cur_vertex_prog)
	{
		LOG_WARNING(RSX, "LoadProgram: m_cur_vertex_prog == NULL");
		return false;
	}
	
	m_fp_buf_num = m_prog_buffer.SearchFp(*m_cur_shader_prog, m_shader_prog);
	m_vp_buf_num = m_prog_buffer.SearchVp(*m_cur_vertex_prog, m_vertex_prog);

	//ConLog.Write("Create program");

	if(m_fp_buf_num == -1)
	{
		LOG_WARNING(RSX, "FP not found in buffer!");
		//m_shader_prog.DecompileAsync(*m_cur_shader_prog);
		//m_shader_prog.Wait();
		m_shader_prog.Decompile(*m_cur_shader_prog);
		m_shader_prog.Compile();
		checkForGlError("m_shader_prog.Compile");

		rFile f(rGetCwd() + "/FragmentProgram.txt", rFile::write);
		f.Write(m_shader_prog.GetShaderText());
	}

	if(m_vp_buf_num == -1)
	{
		LOG_WARNING(RSX, "VP not found in buffer!");
		//m_vertex_prog.DecompileAsync(*m_cur_vertex_prog);
		//m_vertex_prog.Wait();
		m_vertex_prog.Decompile(*m_cur_vertex_prog);
		m_vertex_prog.Compile();
		checkForGlError("m_vertex_prog.Compile");

		rFile f(rGetCwd() + "/VertexProgram.txt", rFile::write);
		f.Write(m_vertex_prog.shader);
	}

	if(m_fp_buf_num != -1 && m_vp_buf_num != -1)
	{
		m_program.id = m_prog_buffer.GetProg(m_fp_buf_num, m_vp_buf_num);
	}

	if(m_program.id)
	{
		// RSX Debugger: Check if this program was modified and update it
		if (Ini.GSLogPrograms.GetValue())
		{
			for(auto& program : m_debug_programs)
			{
				if (program.id == m_program.id && program.modified)
				{
					// TODO: This isn't working perfectly. Is there any better/shorter way to update the program
					m_vertex_prog.shader = program.vp_shader;
					m_shader_prog.SetShaderText(program.fp_shader);
					m_vertex_prog.Wait();
					m_vertex_prog.Compile();
					checkForGlError("m_vertex_prog.Compile");
					m_shader_prog.Wait();
					m_shader_prog.Compile();
					checkForGlError("m_shader_prog.Compile");
					glAttachShader(m_program.id, m_vertex_prog.id);
					glAttachShader(m_program.id, m_shader_prog.GetId());
					glLinkProgram(m_program.id);
					checkForGlError("glLinkProgram");
					glDetachShader(m_program.id, m_vertex_prog.id);
					glDetachShader(m_program.id, m_shader_prog.GetId());
					program.vp_id = m_vertex_prog.id;
					program.fp_id = m_shader_prog.GetId();
					program.modified = false;
				}
			}
		}
		m_program.Use();
	}
	else
	{
		m_program.Create(m_vertex_prog.id, m_shader_prog.GetId());
		checkForGlError("m_program.Create");
		m_prog_buffer.Add(m_program, m_shader_prog, *m_cur_shader_prog, m_vertex_prog, *m_cur_vertex_prog);
		checkForGlError("m_prog_buffer.Add");
		m_program.Use();

		// RSX Debugger
		if (Ini.GSLogPrograms.GetValue())
		{
			RSXDebuggerProgram program;
			program.id = m_program.id;
			program.vp_id = m_vertex_prog.id;
			program.fp_id = m_shader_prog.GetId();
			program.vp_shader = m_vertex_prog.shader;
			program.fp_shader = m_shader_prog.GetShaderText();
			m_debug_programs.push_back(program);
		}
	}

	return true;
}

void GLGSRender::WriteBuffers()
{
	if (Ini.GSDumpDepthBuffer.GetValue())
	{
		WriteDepthBuffer();
	}

	if (Ini.GSDumpColorBuffers.GetValue())
	{
		WriteColorBuffers();
	}
}

void GLGSRender::WriteDepthBuffer()
{
	if (!m_set_context_dma_z)
	{
		return;
	}

	u32 address = GetAddress(m_surface_offset_z, m_context_dma_z - 0xfeed0000);
	if (!Memory.IsGoodAddr(address))
	{
		LOG_WARNING(RSX, "Bad depth buffer address: address=0x%x, offset=0x%x, dma=0x%x", address, m_surface_offset_z, m_context_dma_z);
		return;
	}

	glBindBuffer(GL_PIXEL_PACK_BUFFER, g_pbo[5]);
	checkForGlError("WriteDepthBuffer(): glBindBuffer");
	glBufferData(GL_PIXEL_PACK_BUFFER, RSXThread::m_buffer_width * RSXThread::m_buffer_height * 4, 0, GL_DYNAMIC_READ);
	checkForGlError("WriteDepthBuffer(): glBufferData");
	glReadPixels(0, 0, RSXThread::m_buffer_width, RSXThread::m_buffer_height, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
	checkForGlError("WriteDepthBuffer(): glReadPixels");
	GLubyte *packed = (GLubyte *)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	if (packed)
	{
		memcpy(&Memory[address], packed, RSXThread::m_buffer_width * RSXThread::m_buffer_height * 4);
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
		checkForGlError("WriteDepthBuffer(): glUnmapBuffer");
	}
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	checkForGlError("WriteDepthBuffer(): glBindBuffer");

	glBindTexture(GL_TEXTURE_2D, g_depth_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, RSXThread::m_buffer_width, RSXThread::m_buffer_height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, &Memory[address]);
	checkForGlError("WriteDepthBuffer(): glTexImage2D");
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &Memory[address]);
	checkForGlError("WriteDepthBuffer(): glGetTexImage");
}

void GLGSRender::WriteColorBufferA()
{
	if (!m_set_context_dma_color_a)
	{
		return;
	}

	u32 address = GetAddress(m_surface_offset_a, m_context_dma_color_a - 0xfeed0000);
	if (!Memory.IsGoodAddr(address))
	{
		LOG_ERROR(RSX, "Bad color buffer A address: address=0x%x, offset=0x%x, dma=0x%x", address, m_surface_offset_a, m_context_dma_color_a);
		return;
	}

	glReadBuffer(GL_COLOR_ATTACHMENT0);
	checkForGlError("WriteColorBufferA(): glReadBuffer(GL_COLOR_ATTACHMENT0)");
	glBindBuffer(GL_PIXEL_PACK_BUFFER, g_pbo[0]);
	checkForGlError("WriteColorBufferA(): glBindBuffer");
	glBufferData(GL_PIXEL_PACK_BUFFER, RSXThread::m_buffer_width * RSXThread::m_buffer_height * 4, 0, GL_STREAM_READ);
	checkForGlError("WriteColorBufferA(): glBufferData");
	glReadPixels(0, 0, RSXThread::m_buffer_width, RSXThread::m_buffer_height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, 0);
	checkForGlError("WriteColorBufferA(): glReadPixels");
	GLubyte *packed = (GLubyte *)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	if (packed)
	{
		memcpy(&Memory[address], packed, RSXThread::m_buffer_width * RSXThread::m_buffer_height * 4);
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
		checkForGlError("WriteColorBufferA(): glUnmapBuffer");
	}
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	checkForGlError("WriteColorBufferA(): glBindBuffer");
}

void GLGSRender::WriteColorBufferB()
{
	if (!m_set_context_dma_color_b)
	{
		return;
	}

	u32 address = GetAddress(m_surface_offset_b, m_context_dma_color_b - 0xfeed0000);
	if (!Memory.IsGoodAddr(address))
	{
		LOG_ERROR(RSX, "Bad color buffer B address: address=0x%x, offset=0x%x, dma=0x%x", address, m_surface_offset_b, m_context_dma_color_b);
		return;
	}

	glReadBuffer(GL_COLOR_ATTACHMENT1);
	checkForGlError("WriteColorBufferB(): glReadBuffer(GL_COLOR_ATTACHMENT1)");
	glBindBuffer(GL_PIXEL_PACK_BUFFER, g_pbo[1]);
	checkForGlError("WriteColorBufferB(): glBindBuffer");
	glBufferData(GL_PIXEL_PACK_BUFFER, RSXThread::m_buffer_width * RSXThread::m_buffer_height * 4, 0, GL_STREAM_READ);
	checkForGlError("WriteColorBufferB(): glBufferData");
	glReadPixels(0, 0, RSXThread::m_buffer_width, RSXThread::m_buffer_height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, 0);
	checkForGlError("WriteColorBufferB(): glReadPixels");
	GLubyte *packed = (GLubyte *)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	if (packed)
	{
		memcpy(&Memory[address], packed, RSXThread::m_buffer_width * RSXThread::m_buffer_height * 4);
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
		checkForGlError("WriteColorBufferB(): glUnmapBuffer");
	}
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	checkForGlError("WriteColorBufferB(): glBindBuffer");

}

void GLGSRender::WriteColorBufferC()
{
	if (!m_set_context_dma_color_c)
	{
		return;
	}

	u32 address = GetAddress(m_surface_offset_c, m_context_dma_color_c - 0xfeed0000);
	if (!Memory.IsGoodAddr(address))
	{
		LOG_ERROR(RSX, "Bad color buffer C address: address=0x%x, offset=0x%x, dma=0x%x", address, m_surface_offset_c, m_context_dma_color_c);
		return;
	}

	glReadBuffer(GL_COLOR_ATTACHMENT2);
	checkForGlError("WriteColorBufferC(): glReadBuffer(GL_COLOR_ATTACHMENT2)");
	glBindBuffer(GL_PIXEL_PACK_BUFFER, g_pbo[2]);
	checkForGlError("WriteColorBufferC(): glBindBuffer");
	glBufferData(GL_PIXEL_PACK_BUFFER, RSXThread::m_buffer_width * RSXThread::m_buffer_height * 4, 0, GL_STREAM_READ);
	checkForGlError("WriteColorBufferC(): glBufferData");
	glReadPixels(0, 0, RSXThread::m_buffer_width, RSXThread::m_buffer_height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, 0);
	checkForGlError("WriteColorBufferC(): glReadPixels");
	GLubyte *packed = (GLubyte *)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	if (packed)
	{
		memcpy(&Memory[address], packed, RSXThread::m_buffer_width * RSXThread::m_buffer_height * 4);
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
		checkForGlError("WriteColorBufferC(): glUnmapBuffer");
	}
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	checkForGlError("WriteColorBufferC(): glBindBuffer");
}

void GLGSRender::WriteColorBufferD()
{
	if (!m_set_context_dma_color_d)
	{
		return;
	}

	u32 address = GetAddress(m_surface_offset_d, m_context_dma_color_d - 0xfeed0000);
	if (!Memory.IsGoodAddr(address))
	{
		LOG_ERROR(RSX, "Bad color buffer D address: address=0x%x, offset=0x%x, dma=0x%x", address, m_surface_offset_d, m_context_dma_color_d);
		return;
	}

	glReadBuffer(GL_COLOR_ATTACHMENT3);
	checkForGlError("WriteColorBufferD(): glReadBuffer(GL_COLOR_ATTACHMENT3)");
	glBindBuffer(GL_PIXEL_PACK_BUFFER, g_pbo[3]);
	checkForGlError("WriteColorBufferD(): glBindBuffer");
	glBufferData(GL_PIXEL_PACK_BUFFER, RSXThread::m_buffer_width * RSXThread::m_buffer_height * 4, 0, GL_STREAM_READ);
	checkForGlError("WriteColorBufferD(): glBufferData");
	glReadPixels(0, 0, RSXThread::m_buffer_width, RSXThread::m_buffer_height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, 0);
	checkForGlError("WriteColorBufferD(): glReadPixels");
	GLubyte *packed = (GLubyte *)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	if (packed)
	{
		memcpy(&Memory[address], packed, RSXThread::m_buffer_width * RSXThread::m_buffer_height * 4);
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
		checkForGlError("WriteColorBufferD(): glUnmapBuffer");

	}
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	checkForGlError("WriteColorBufferD(): glBindBuffer");
}

void GLGSRender::WriteColorBuffers()
{
	glPixelStorei(GL_PACK_ROW_LENGTH, 0);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);

	switch(m_surface_color_target)
	{
	case CELL_GCM_SURFACE_TARGET_NONE:
		return;

	case CELL_GCM_SURFACE_TARGET_0:
		WriteColorBufferA();
	break;

	case CELL_GCM_SURFACE_TARGET_1:
		WriteColorBufferB();
	break;

	case CELL_GCM_SURFACE_TARGET_MRT1:
		WriteColorBufferA();
		WriteColorBufferB();
	break;

	case CELL_GCM_SURFACE_TARGET_MRT2:
		WriteColorBufferA();
		WriteColorBufferB();
		WriteColorBufferC();
	break;

	case CELL_GCM_SURFACE_TARGET_MRT3:
		WriteColorBufferA();
		WriteColorBufferB();
		WriteColorBufferC();
		WriteColorBufferD();
	break;
	}
}

void GLGSRender::OnInit()
{
	m_draw_frames = 1;
	m_skip_frames = 0;
	RSXThread::m_width = 720;
	RSXThread::m_height = 576;
	RSXThread::m_width_scale = 2.0f;
	RSXThread::m_height_scale = 2.0f;

	last_width = 0;
	last_height = 0;
	last_depth_format = 0;

	m_frame->Show();
}

void GLGSRender::OnInitThread()
{
	m_context = m_frame->GetNewContext();

	m_frame->SetCurrent(m_context);

	InitProcTable();

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

	glGenTextures(1, &g_depth_tex);
	glGenTextures(1, &g_flip_tex);
	glGenBuffers(6, g_pbo);

#ifdef _WIN32
	glSwapInterval(Ini.GSVSyncEnable.GetValue() ? 1 : 0);
// Undefined reference: glXSwapIntervalEXT
/*#else
	if (GLXDrawable drawable = glXGetCurrentDrawable()){
		glXSwapIntervalEXT(glXGetCurrentDisplay(), drawable, Ini.GSVSyncEnable.GetValue() ? 1 : 0);
	}*/
#endif

}

void GLGSRender::OnExitThread()
{
	glDeleteTextures(1, &g_flip_tex);
	glDeleteTextures(1, &g_depth_tex);
	glDeleteBuffers(6, g_pbo);
	
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);

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

void GLGSRender::InitDrawBuffers()
{
	if(!m_fbo.IsCreated() || RSXThread::m_width != last_width || RSXThread::m_height != last_height || last_depth_format != m_surface_depth_format)
	{
		LOG_WARNING(RSX, "New FBO (%dx%d)", RSXThread::m_width, RSXThread::m_height);
		last_width = RSXThread::m_width;
		last_height = RSXThread::m_height;
		last_depth_format = m_surface_depth_format;

		m_fbo.Create();
		checkForGlError("m_fbo.Create");
		m_fbo.Bind();

		m_rbo.Create(4 + 1);
		checkForGlError("m_rbo.Create");

		for(int i=0; i<4; ++i)
		{
			m_rbo.Bind(i);
			m_rbo.Storage(GL_RGBA, RSXThread::m_width, RSXThread::m_height);
			checkForGlError("m_rbo.Storage(GL_RGBA)");
		}

		m_rbo.Bind(4);

		switch(m_surface_depth_format)
		{
		// case 0 found in BLJM60410-[Suzukaze no Melt - Days in the Sanctuary]  
		// [E : RSXThread]: Bad depth format! (0)
		// [E : RSXThread]: glEnable: opengl error 0x0506
		// [E : RSXThread]: glDrawArrays: opengl error 0x0506
		case 0:
			m_rbo.Storage(GL_DEPTH_COMPONENT, RSXThread::m_width, RSXThread::m_height);
			checkForGlError("m_rbo.Storage(GL_DEPTH_COMPONENT)");
		break;
		
		case CELL_GCM_SURFACE_Z16:
			m_rbo.Storage(GL_DEPTH_COMPONENT16, RSXThread::m_width, RSXThread::m_height);
			checkForGlError("m_rbo.Storage(GL_DEPTH_COMPONENT16)");
		break;

		case CELL_GCM_SURFACE_Z24S8:
			m_rbo.Storage(GL_DEPTH24_STENCIL8, RSXThread::m_width, RSXThread::m_height);
			checkForGlError("m_rbo.Storage(GL_DEPTH24_STENCIL8)");
		break;

		default:
			LOG_ERROR(RSX, "Bad depth format! (%d)", m_surface_depth_format);
			assert(0);
		break;
		}

		for(int i=0; i<4; ++i)
		{
			m_fbo.Renderbuffer(GL_COLOR_ATTACHMENT0 + i, m_rbo.GetId(i));
			checkForGlError(fmt::Format("m_fbo.Renderbuffer(GL_COLOR_ATTACHMENT%d)", i));
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
		m_surface_clip_w = RSXThread::m_width;
	}

	if(!m_set_surface_clip_vertical)
	{
		m_surface_clip_y = 0;
		m_surface_clip_h = RSXThread::m_height;
	}
		
	m_fbo.Bind();

	static const GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };

	switch(m_surface_color_target)
	{
	case CELL_GCM_SURFACE_TARGET_NONE:
		break;

	case CELL_GCM_SURFACE_TARGET_0:
		glDrawBuffer(draw_buffers[0]);
	break;

	case CELL_GCM_SURFACE_TARGET_1:
		glDrawBuffer(draw_buffers[1]);
	break;

	case CELL_GCM_SURFACE_TARGET_MRT1:
		glDrawBuffers(2, draw_buffers);
	break;

	case CELL_GCM_SURFACE_TARGET_MRT2:
		glDrawBuffers(3, draw_buffers);
	break;

	case CELL_GCM_SURFACE_TARGET_MRT3:
		glDrawBuffers(4, draw_buffers);
	break;

	checkForGlError("glDrawBuffers");

	default:
		LOG_ERROR(RSX, "Bad surface color target: %d", m_surface_color_target);
	break;
	}
}

void GLGSRender::ExecCMD(u32 cmd)
{
	assert(cmd == NV4097_CLEAR_SURFACE);
	
	InitDrawBuffers();

	if(m_set_color_mask)
	{
		glColorMask(m_color_mask_r, m_color_mask_g, m_color_mask_b, m_color_mask_a);
		checkForGlError("glColorMask");
	}

	if (m_set_scissor_horizontal && m_set_scissor_vertical)
	{
		glScissor(m_scissor_x, m_scissor_y, m_scissor_w, m_scissor_h);
		checkForGlError("glScissor");
	}

	GLbitfield f = 0;

	if (m_clear_surface_mask & 0x1)
	{
		glClearDepth(m_clear_surface_z / (float)0xffffff);
		checkForGlError("glClearDepth");

		f |= GL_DEPTH_BUFFER_BIT;
	}

	if (m_clear_surface_mask & 0x2)
	{
		glClearStencil(m_clear_surface_s);
		checkForGlError("glClearStencil");

		f |= GL_STENCIL_BUFFER_BIT;
	}

	if (m_clear_surface_mask & 0xF0)
	{
		glClearColor(
			m_clear_surface_color_r / 255.0f,
			m_clear_surface_color_g / 255.0f,
			m_clear_surface_color_b / 255.0f,
			m_clear_surface_color_a / 255.0f);
		checkForGlError("glClearColor");

		f |= GL_COLOR_BUFFER_BIT;
	}

	glClear(f);
	checkForGlError("glClear");
	
	WriteBuffers();
}

void GLGSRender::ExecCMD()
{
	//return;
	if(!LoadProgram())
	{
		LOG_ERROR(RSX, "LoadProgram failed.");
		Emu.Pause();
		return;
	}

	InitDrawBuffers();

	if (m_set_color_mask)
	{
		glColorMask(m_color_mask_r, m_color_mask_g, m_color_mask_b, m_color_mask_a);
		checkForGlError("glColorMask");
	}

	Enable(m_set_depth_test, GL_DEPTH_TEST);
	Enable(m_set_alpha_test, GL_ALPHA_TEST);
	Enable(m_set_depth_bounds_test, GL_DEPTH_BOUNDS_TEST_EXT);
	Enable(m_set_blend || m_set_blend_mrt1 || m_set_blend_mrt2 || m_set_blend_mrt3, GL_BLEND);
	Enable(m_set_scissor_horizontal && m_set_scissor_vertical, GL_SCISSOR_TEST);
	Enable(m_set_logic_op, GL_LOGIC_OP);
	Enable(m_set_cull_face, GL_CULL_FACE);
	Enable(m_set_dither, GL_DITHER);
	Enable(m_set_stencil_test, GL_STENCIL_TEST);
	Enable(m_set_line_smooth, GL_LINE_SMOOTH);
	Enable(m_set_poly_smooth, GL_POLYGON_SMOOTH);
	Enable(m_set_point_sprite_control, GL_POINT_SPRITE);
	Enable(m_set_specular, GL_LIGHTING);
	Enable(m_set_poly_offset_fill, GL_POLYGON_OFFSET_FILL);
	Enable(m_set_poly_offset_line, GL_POLYGON_OFFSET_LINE);
	Enable(m_set_poly_offset_point, GL_POLYGON_OFFSET_POINT);
	Enable(m_set_restart_index, GL_PRIMITIVE_RESTART);
	Enable(m_set_line_stipple, GL_LINE_STIPPLE);
	Enable(m_set_polygon_stipple, GL_POLYGON_STIPPLE);
	
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

	if (m_set_front_polygon_mode)
	{
		glPolygonMode(GL_FRONT, m_front_polygon_mode);
		checkForGlError("glPolygonMode(Front)");
	}

	if (m_set_back_polygon_mode)
	{
		glPolygonMode(GL_BACK, m_back_polygon_mode);
		checkForGlError("glPolygonMode(Back)");
	}

	if (m_set_point_size)
	{
		glPointSize(m_point_size);
		checkForGlError("glPointSize");
	}

	if (m_set_poly_offset_mode)
	{
		glPolygonOffset(m_poly_offset_scale_factor, m_poly_offset_bias);
		checkForGlError("glPolygonOffset");
	}

	if (m_set_logic_op)
	{
		glLogicOp(m_logic_op);
		checkForGlError("glLogicOp");
	}

	if (m_set_scissor_horizontal && m_set_scissor_vertical)
	{
		glScissor(m_scissor_x, m_scissor_y, m_scissor_w, m_scissor_h);
		checkForGlError("glScissor");
	}
	
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

	if(m_set_depth_bounds)
	{
		glDepthBoundsEXT(m_depth_bounds_min, m_depth_bounds_max);
		checkForGlError("glDepthBounds");
	}

	if(m_set_clip)
	{
		glDepthRangef(m_clip_min, m_clip_max);
		checkForGlError("glDepthRangef");
	}

	if(m_set_line_width)
	{
		glLineWidth(m_line_width);
		checkForGlError("glLineWidth");
	}

	if (m_set_line_stipple)
	{
		glLineStipple(m_line_stipple_factor, m_line_stipple_pattern);
		checkForGlError("glLineStipple");
	}

	if (m_set_polygon_stipple)
	{
		glPolygonStipple((const GLubyte*)m_polygon_stipple_pattern);
		checkForGlError("glPolygonStipple");
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

	if (m_set_front_face)
	{
		glFrontFace(m_front_face);
		checkForGlError("glFrontFace");
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

	if(m_set_restart_index)
	{
		glPrimitiveRestartIndex(m_restart_index);
		checkForGlError("glPrimitiveRestartIndex");
	}

	if(m_indexed_array.m_count && m_draw_array_count)
	{
		LOG_WARNING(RSX, "m_indexed_array.m_count && draw_array_count");
	}

	for(u32 i=0; i<m_textures_count; ++i)
	{
		if(!m_textures[i].IsEnabled()) continue;

		glActiveTexture(GL_TEXTURE0 + i);
		checkForGlError("glActiveTexture");
		m_gl_textures[i].Create();
		m_gl_textures[i].Bind();
		checkForGlError(fmt::Format("m_gl_textures[%d].Bind", i));
		m_program.SetTex(i);
		m_gl_textures[i].Init(m_textures[i]);
		checkForGlError(fmt::Format("m_gl_textures[%d].Init", i));
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
		case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_32:
			glDrawElements(m_draw_mode - 1, m_indexed_array.m_count, GL_UNSIGNED_INT, nullptr);
			checkForGlError("glDrawElements #4");
		break;

		case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16:
			glDrawElements(m_draw_mode - 1, m_indexed_array.m_count, GL_UNSIGNED_SHORT, nullptr);
			checkForGlError("glDrawElements #2");
		break;

		default:
			LOG_ERROR(RSX, "Bad indexed array type (%d)", m_indexed_array.m_type);
		break;
		}

		DisableVertexData();
		m_indexed_array.Reset();
	}

	if(m_draw_array_count)
	{
		//LOG_WARNING(RSX,"glDrawArrays(%d,%d,%d)", m_draw_mode - 1, m_draw_array_first, m_draw_array_count);
		glDrawArrays(m_draw_mode - 1, 0, m_draw_array_count);
		checkForGlError("glDrawArrays");
		DisableVertexData();
	}
	
	WriteBuffers();
}

void GLGSRender::Flip()
{
	// Set scissor to FBO size 
	if (m_set_scissor_horizontal && m_set_scissor_vertical)
	{
		glScissor(0, 0, RSXThread::m_width, RSXThread::m_height);
		checkForGlError("glScissor");
	}
	
	switch (m_surface_color_target)
	{
	case CELL_GCM_SURFACE_TARGET_0:
	case CELL_GCM_SURFACE_TARGET_1:
	case CELL_GCM_SURFACE_TARGET_MRT1:
	case CELL_GCM_SURFACE_TARGET_MRT2:
	case CELL_GCM_SURFACE_TARGET_MRT3:
	{
		// Fast path for non-MRT using glBlitFramebuffer.
		GLfbo::Bind(GL_DRAW_FRAMEBUFFER, 0);
		// Renderbuffer is upside turn , swapped srcY0 and srcY1
		GLfbo::Blit(0, RSXThread::m_height, RSXThread::m_width, 0, 0, 0, RSXThread::m_width, RSXThread::m_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	}
	break;

	case CELL_GCM_SURFACE_TARGET_NONE:
	{
		// Slow path for MRT/None target using glReadPixels.
		static u8* src_buffer = nullptr;
		static u32 width = 0;
		static u32 height = 0;
		GLenum format = GL_RGBA;

		if (m_read_buffer)
		{
			format = GL_BGRA;
			gcmBuffer* buffers = (gcmBuffer*)Memory.GetMemFromAddr(m_gcm_buffers_addr);
			u32 addr = GetAddress(re(buffers[m_gcm_current_buffer].offset), CELL_GCM_LOCATION_LOCAL);

			if (Memory.IsGoodAddr(addr))
			{
				width = re(buffers[m_gcm_current_buffer].width);
				height = re(buffers[m_gcm_current_buffer].height);
				src_buffer = (u8*)Memory.VirtualToRealAddr(addr);
			}
			else
			{
				src_buffer = nullptr;
			}
		}
		else if (m_fbo.IsCreated())
		{
			format = GL_RGBA;
			static std::vector<u8> pixels;
			pixels.resize(RSXThread::m_width * RSXThread::m_height * 4);
			m_fbo.Bind(GL_READ_FRAMEBUFFER);
			glBindBuffer(GL_PIXEL_PACK_BUFFER, g_pbo[5]);
			glBufferData(GL_PIXEL_PACK_BUFFER, RSXThread::m_width * RSXThread::m_height * 4, 0, GL_STREAM_READ);
			glReadPixels(0, 0, RSXThread::m_width, RSXThread::m_height, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, 0);
			checkForGlError("Flip(): glReadPixels(GL_BGRA, GL_UNSIGNED_INT_8_8_8_8)");
			GLubyte *packed = (GLubyte *)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
			memcpy(pixels.data(), packed, RSXThread::m_width * RSXThread::m_height * 4);
			glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
			glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

			src_buffer = pixels.data();
			width = RSXThread::m_width;
			height = RSXThread::m_height;
		}
		else
			src_buffer = nullptr;

		if (src_buffer)
		{
			glDisable(GL_STENCIL_TEST);
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_CLIP_PLANE0);
			glDisable(GL_CLIP_PLANE1);
			glDisable(GL_CLIP_PLANE2);
			glDisable(GL_CLIP_PLANE3);
			glDisable(GL_CLIP_PLANE4);
			glDisable(GL_CLIP_PLANE5);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, g_flip_tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, GL_UNSIGNED_INT_8_8_8_8, src_buffer);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(0, 1, 0, 1, 0, 1);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			GLfbo::Bind(GL_DRAW_FRAMEBUFFER, 0);

			m_program.UnUse();
			m_program.Use();
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_ACCUM_BUFFER_BIT);

			glColor3f(1, 1, 1);
			glBegin(GL_QUADS);
			glTexCoord2i(0, 1);
			glVertex2i(0, 0);

			glTexCoord2i(1, 1);
			glVertex2i(1, 0);

			glTexCoord2i(1, 0);
			glVertex2i(1, 1);

			glTexCoord2i(0, 0);
			glVertex2i(0, 1);
			glEnd();
		}
	}
	break;
	}

	// Draw Objects
	for (uint i = 0; i<m_post_draw_objs.size(); ++i)
	{
		m_post_draw_objs[i].Draw();
	}

	m_frame->Flip(m_context);
	
	// Restore scissor
	if (m_set_scissor_horizontal && m_set_scissor_vertical)
	{
		glScissor(m_scissor_x, m_scissor_y, m_scissor_w, m_scissor_h);
		checkForGlError("glScissor");
	}

}

u32 LinearToSwizzleAddress(u32 x, u32 y, u32 z, u32 log2_width, u32 log2_height, u32 log2_depth)
{
	u32 offset = 0;
	u32 shift_count = 0;
	while(log2_width | log2_height | log2_depth){
		if(log2_width){
			offset |= (x & 0x01) << shift_count;
			x >>= 1;
			++shift_count;
			--log2_width;
		}
		if(log2_height){
			offset |= (y & 0x01) << shift_count;
			y >>= 1;
			++shift_count;
			--log2_height;
		}
		if(log2_depth){
			offset |= (z & 0x01) << shift_count;
			z >>= 1;
			++shift_count;
			--log2_depth;
		}
	}
	return offset;
}
