#include "stdafx.h"
#include "RSXThread.h"

RSXVertexData::RSXVertexData()
	: frequency(0)
	, stride(0)
	, size(0)
	, type(0)
	, addr(0)
	, data()
{
}

void RSXVertexData::Reset()
{
	frequency = 0;
	stride = 0;
	size = 0;
	type = 0;
	addr = 0;
	data.ClearF();
}

void RSXVertexData::Load(u32 start, u32 count)
{
	if(!addr) return;

	const u32 tsize = GetTypeSize();

	data.SetCount((start + count) * tsize * size);

	for(u32 i=start; i<start + count; ++i)
	{
		const u8* src = Memory.GetMemFromAddr(addr) + stride * i;
		u8* dst = &data[i * tsize * size];

		switch(tsize)
		{
		case 1:
		{
			memcpy(dst, src, size);
		}
		break;

		case 2:
		{
			const u16* c_src = (const u16*)src;
			u16* c_dst = (u16*)dst;
			for(u32 j=0; j<size; ++j) *c_dst++ = re(*c_src++);
		}
		break;

		case 4:
		{
			const u32* c_src = (const u32*)src;
			u32* c_dst = (u32*)dst;
			for(u32 j=0; j<size; ++j) *c_dst++ = re(*c_src++);
		}
		break;
		}
	}
}

u32 RSXVertexData::GetTypeSize()
{
	switch (type)
	{
	case 1: return 2;
	case 2: return 4;
	case 3: return 2;
	case 4: return 1;
	case 5: return 2;
	case 7: return 1;
	}

	ConLog.Error("Bad vertex data type! %d", type);
	return 1;
}

#define CMD_DEBUG 0

#if	CMD_DEBUG
	#define CMD_LOG ConLog.Write
#else
	#define CMD_LOG(...)
#endif

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

void RSXThread::DoCmd(const u32 fcmd, const u32 cmd, mem32_ptr_t& args, const u32 count)
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
			Flip();

			m_gcm_current_buffer = args[0];

			m_flip_status = 0;
			if(m_flip_handler)
			{
				m_flip_handler.Handle(1, 0, 0);
				m_flip_handler.Branch(false);
			}

			SemaphorePostAndWait(m_sem_flip);

			//Emu.Pause();
		}
	break;

	case NV4097_NO_OPERATION:
	break;

	case NV406E_SET_REFERENCE:
		m_ctrl->ref = re32(args[0]);
	break;

	case_16(NV4097_SET_TEXTURE_OFFSET, 0x20):
	{
		RSXTexture& tex = m_textures[index];
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
		RSXTexture& tex = m_textures[index];
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

		m_vertex_data[index].Reset();
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
		
		m_vertex_data[index].Reset();
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

		m_vertex_data[index].Reset();
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
		RSXTexture& tex = m_textures[index];
		tex.SetControl1(args[0]);
	}
	break;

	case_16(NV4097_SET_TEXTURE_CONTROL3, 4):
	{
		RSXTexture& tex = m_textures[index];
		u32 a0 = args[0];
		u32 pitch = a0 & 0xFFFFF;
		u16 depth = a0 >> 20;
		tex.SetControl3(pitch, depth);
	}
	break;

	case_16(NV4097_SET_TEXTURE_FILTER, 0x20):
	{
		RSXTexture& tex = m_textures[index];
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
		RSXTexture& tex = m_textures[index];

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
		RSXTexture& tex = m_textures[index];

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

		gcmBuffer* buffers = (gcmBuffer*)Memory.GetMemFromAddr(m_gcm_buffers_addr);
		m_width = re(buffers[m_gcm_current_buffer].width);
		m_height = re(buffers[m_gcm_current_buffer].height);
	}
	break;

	case NV4097_SET_COLOR_MASK_MRT:
	{
		if(args[0]) ConLog.Warning("NV4097_SET_COLOR_MASK_MRT: %x", args[0]);
	}
	break;

	case NV4097_SET_BLEND_ENABLE_MRT:
	{
		if(args[0]) ConLog.Warning("NV4097_SET_BLEND_ENABLE_MRT: %x", args[0]);
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

	case NV4097_SET_DEPTH_BOUNDS_MIN:
	{
		m_set_depth_bounds = true;
		const u32 depth_bounds_min = args[0];
		m_depth_bounds_min = (float&)depth_bounds_min;
		if (count > 1)
		{
			const u32 depth_bounds_max = args[1];
			m_depth_bounds_max = (float&)depth_bounds_max;
		}
	}
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

	case NV4097_CLEAR_ZCULL_SURFACE:
	{
		u32 a0 = args[0];

		if(a0 & 0x01) m_clear_surface_z = m_clear_z;
		if(a0 & 0x02) m_clear_surface_s = m_clear_s;

		m_clear_surface_mask |= a0 & 0x3;
	}
	break;

	case NV4097_CLEAR_SURFACE:
	{
		u32 a0 = args[0];

		if(a0 & 0x01) m_clear_surface_z = m_clear_z;
		if(a0 & 0x02) m_clear_surface_s = m_clear_s;
		if(a0 & 0x10) m_clear_surface_color_r = m_clear_color_r;
		if(a0 & 0x20) m_clear_surface_color_g = m_clear_color_g;
		if(a0 & 0x40) m_clear_surface_color_b = m_clear_color_b;
		if(a0 & 0x80) m_clear_surface_color_a = m_clear_color_a;

		m_clear_surface_mask |= a0;
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
		m_vertex_data[index].data.ClearF();
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

		RSXVertexData& cv = m_vertex_data[index];
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

			//ConLog.Warning("NV4097_DRAW_ARRAYS: %d - %d", first, _count);

			LoadVertexData(first, _count);

			if(first < m_draw_array_first) m_draw_array_first = first;
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
		u32 a0 = args[0];

		//ConLog.Warning("NV4097_SET_BEGIN_END: %x", a0);

		if(a0)
		{
			Begin(a0);
		}
		else
		{
			End();
		}
	}
	break;

	case NV4097_SET_COLOR_CLEAR_VALUE:
	{
		const u32 color = args[0];
		m_clear_color_a = (color >> 24) & 0xff;
		m_clear_color_r = (color >> 16) & 0xff;
		m_clear_color_g = (color >> 8) & 0xff;
		m_clear_color_b = color & 0xff;
	}
	break;

	case NV4097_SET_SHADER_PROGRAM:
	{
		m_cur_shader_prog = &m_shader_progs[m_cur_shader_prog_num++];
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
		m_cur_vertex_prog->data.Clear();

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

			RSXTransformConstant c(id, (float&)x, (float&)y, (float&)z, (float&)w);

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
		m_set_poly_offset_fill = args[0] ? true : false;
	break;

	case NV4097_SET_POLY_OFFSET_LINE_ENABLE:
		m_set_poly_offset_line = args[0] ? true : false;
	break;

	case NV4097_SET_POLY_OFFSET_POINT_ENABLE:
		m_set_poly_offset_point = args[0] ? true : false;
	break;

	case NV4097_SET_RESTART_INDEX_ENABLE:
		m_set_restart_index = args[0] ? true : false;
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
		m_restart_index = args[0];
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

		if(count > 1)
		{
			m_set_context_dma_color_d = true;
			m_context_dma_color_d = args[1];
		}
	}
	break;

	case NV4097_SET_CONTEXT_DMA_ZETA:
	{
		m_set_context_dma_z = true;
		m_context_dma_z = args[0];
	}
	break;
	/*
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
	*/
	case NV4097_SET_SURFACE_PITCH_C:
	{
		if(count != 4)
		{
			ConLog.Error("NV4097_SET_SURFACE_PITCH_C: Bad count (%d)", count);
			break;
		}

		m_surface_pitch_c = args[0];
		m_surface_pitch_d = args[1];
		m_surface_offset_c = args[2];
		m_surface_offset_d = args[3];
	}
	break;

	case NV4097_SET_SURFACE_PITCH_Z:
	{
		m_surface_pitch_z = args[0];
	}
	break;

	case NV4097_SET_SHADER_WINDOW:
	{
		u32 a0 = args[0];
		m_shader_window_height = a0 & 0xfff;
		m_shader_window_origin = (a0 >> 12) & 0xf;
		m_shader_window_pixel_centers = a0 >> 16;
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
		m_surface_colour_target = args[0];
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
		u32 a0 = args[0];
		m_clear_s = a0 & 0xff;
		m_clear_z = a0 >> 8;
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
		RSXTransformConstant c;
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

void RSXThread::Begin(u32 draw_mode)
{
	m_begin_end = 1;
	m_draw_mode = draw_mode;
	m_draw_array_count = 0;
	m_draw_array_first = ~0;

	if(Emu.GetCallbackManager().m_exit_callback.m_callbacks.GetCount())
	{
		//Emu.GetCallbackManager().m_exit_callback.Handle(0x0121, 0);
	}
}

void RSXThread::End()
{
	ExecCMD();

	if(Emu.GetCallbackManager().m_exit_callback.m_callbacks.GetCount())
	{
		//Emu.GetCallbackManager().m_exit_callback.Handle(0x0122, 0);
	}

	for(uint i=0; i<m_textures_count; ++i)
	{
		m_textures[i].m_enabled = false;
	}

	m_indexed_array.Reset();
	m_fragment_constants.Clear();
	m_transform_constants.Clear();
	m_cur_shader_prog_num = 0;

	m_clear_surface_mask = 0;
	m_begin_end = 0;

	//Reset();
	OnReset();
}

void RSXThread::Task()
{
	ConLog.Write("RSX thread entry");

	OnInitThread();

	while(!TestDestroy())
	{
		wxCriticalSectionLocker lock(m_cs_main);

		const u32 get = re(m_ctrl->get);
		const u32 put = re(m_ctrl->put);
		if(put == get || !Emu.IsRunning())
		{
			if(put == get)
				SemaphorePostAndWait(m_sem_flush);

			Sleep(1);
			continue;
		}

		//ConLog.Write("addr = 0x%x", m_ioAddress + get);
		const u32 cmd = Memory.Read32(m_ioAddress + get);
		const u32 count = (cmd >> 18) & 0x7ff;
		//if(cmd == 0) continue;

		if(cmd & CELL_GCM_METHOD_FLAG_JUMP)
		{
			u32 addr = cmd & ~(CELL_GCM_METHOD_FLAG_JUMP | CELL_GCM_METHOD_FLAG_NON_INCREMENT);
			//ConLog.Warning("rsx jump(0x%x) #addr=0x%x, cmd=0x%x, get=0x%x, put=0x%x", addr, m_ioAddress + get, cmd, get, put);
			re(m_ctrl->get, addr);
			continue;
		}
		if(cmd & CELL_GCM_METHOD_FLAG_CALL)
		{
			m_call_stack.Push(get + 4);
			u32 offs = cmd & ~CELL_GCM_METHOD_FLAG_CALL;
			u32 addr = m_ioAddress + offs;
			//ConLog.Warning("rsx call(0x%x) #0x%x - 0x%x - 0x%x", offs, addr, cmd, get);
			m_ctrl->get = re32(offs);
			continue;
		}
		if(cmd == CELL_GCM_METHOD_FLAG_RETURN)
		{
			//ConLog.Warning("rsx return!");
			u32 get = m_call_stack.Pop();
			//ConLog.Warning("rsx return(0x%x)", get);
			m_ctrl->get = re32(get);
			continue;
		}
		if(cmd & CELL_GCM_METHOD_FLAG_NON_INCREMENT)
		{
			//ConLog.Warning("non increment cmd! 0x%x", cmd);
		}

		if(cmd == 0)
		{
			ConLog.Warning("null cmd: addr=0x%x, put=0x%x, get=0x%x", m_ioAddress + get, re(m_ctrl->put), get);
			Emu.Pause();
			continue;
		}

		mem32_ptr_t args(m_ioAddress + get + 4);
		DoCmd(cmd, cmd & 0x3ffff, args, count);

		re(m_ctrl->get, get + (count + 1) * 4);
		//memset(Memory.GetMemFromAddr(p.m_ioAddress + get), 0, (count + 1) * 4);
	}

	ConLog.Write("RSX thread exit...");

	OnExitThread();
}
