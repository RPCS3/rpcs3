#include "stdafx.h"
#include "Emu/ConLog.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "RSXThread.h"
#include "Emu/SysCalls/lv2/SC_Time.h"

#define ARGS(x) (x >= count ? OutOfArgsCount(x, cmd, count) : Memory.Read32(Memory.RSXIOMem.GetStartAddr() + m_ctrl->get + (4*(x+1))))

u32 methodRegisters[0xffff];

u32 GetAddress(u32 offset, u8 location)
{
	switch(location)
	{
	case CELL_GCM_LOCATION_LOCAL: return Memory.RSXFBMem.GetStartAddr() + offset;
	case CELL_GCM_LOCATION_MAIN: 
		u64 realAddr;
		Memory.RSXIOMem.getRealAddr(Memory.RSXIOMem.GetStartAddr() + offset, realAddr); // TODO: Error Check?
		return realAddr;
	}

	ConLog.Error("GetAddress(offset=0x%x, location=0x%x)", location);
	assert(0);
	return 0;
}

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
	data.clear();
}

void RSXVertexData::Load(u32 start, u32 count)
{
	if(!addr) return;

	const u32 tsize = GetTypeSize();

	data.resize((start + count) * tsize * size);

	for(u32 i=start; i<start + count; ++i)
	{
		const u8* src = Memory.GetMemFromAddr(addr) + stride * i;
		u8* dst = &data[i * tsize * size];

		switch(tsize)
		{
		case 1:
		{
			memcpy(dst, src, size); // may be dangerous
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

u32 RSXThread::OutOfArgsCount(const uint x, const u32 cmd, const u32 count)
{
	std::string debug = GetMethodName(cmd);
	debug += "(";
	for(u32 i=0; i<count; ++i) debug += (i ? ", " : "") + fmt::Format("0x%x", ARGS(i));
	debug += ")";
	ConLog.Write("OutOfArgsCount(x=%u, count=%u): " + debug, x, count);

	return 0;
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

#define case_32(a, m) \
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
	case a + m * 16: \
	case a + m * 17: \
	case a + m * 18: \
	case a + m * 19: \
	case a + m * 20: \
	case a + m * 21: \
	case a + m * 22: \
	case a + m * 23: \
	case a + m * 24: \
	case a + m * 25: \
	case a + m * 26: \
	case a + m * 27: \
	case a + m * 28: \
	case a + m * 29: \
	case a + m * 30: \
	case a + m * 31: \
	index = (cmd - a) / m; \
	case a \

void RSXThread::DoCmd(const u32 fcmd, const u32 cmd, mem32_ptr_t& args, const u32 count)
{
#if	CMD_DEBUG
		std::string debug = GetMethodName(cmd);
		debug += "(";
		for(u32 i=0; i<count; ++i) debug += (i ? ", " : "") + fmt::Format("0x%x", ARGS(i));
		debug += ")";
		ConLog.Write(debug);
#endif

	u32 index = 0;

	m_used_gcm_commands.insert(cmd);

	//static u32 draw_array_count = 0;

	switch(cmd)
	{
	case NV4097_SET_FLIP:
		//if(cmd == 0xfeadffff)
		{
			Flip();

			m_gcm_current_buffer = ARGS(0);
			m_read_buffer = true;
			m_flip_status = 0;

			if(m_flip_handler)
			{
				m_flip_handler.Handle(1, 0, 0);
				m_flip_handler.Branch(false);
			}

			//Emu.Pause();
		}
	break;

	case NV4097_NO_OPERATION:
	{
		ConLog.Warning("NV4097_NO_OPERATION");
	}
	break;

	case NV406E_SET_REFERENCE:
	{
		m_ctrl->ref = ARGS(0);
	}
	break;
	
	// Texture
	case_16(NV4097_SET_TEXTURE_FORMAT, 0x20) :
	{
		// Done in methodRegisters[NV4097_SET_TEXTURE_FORMAT + (m_index*32)]
	}
	break;
	
	case_16(NV4097_SET_TEXTURE_OFFSET, 0x20):
	{
		// Done in methodRegisters[NV4097_SET_TEXTURE_OFFSET + (m_index*32)]
	}
	break;

	case_16(NV4097_SET_TEXTURE_FILTER, 0x20) :
	{
		// Done in methodRegisters[NV4097_SET_TEXTURE_FILTER + (m_index*32)]
	}
	break;

	case_16(NV4097_SET_TEXTURE_ADDRESS, 0x20) :
	{
		// Done in methodRegisters[NV4097_SET_TEXTURE_ADDRESS + (m_index * 32)]
	}
	break;

	case_16(NV4097_SET_TEXTURE_IMAGE_RECT, 32) :
	{
		// Done in methodRegisters[NV4097_SET_TEXTURE_IMAGE_RECT + (m_index*32)]
	}
	break;

	case_16(NV4097_SET_TEXTURE_BORDER_COLOR, 0x20) :
	{
		// Done in methodRegisters[NV4097_SET_TEXTURE_BORDER_COLOR + (m_index*32)]
	}
	break;
	case_16(NV4097_SET_TEXTURE_CONTROL0, 0x20):
	{
		// Done in methodRegisters[NV4097_SET_TEXTURE_CONTROL0 + (m_index*32)]
	}
	break;

	case_16(NV4097_SET_TEXTURE_CONTROL1, 0x20) :
	{
		// Done in methodRegisters[NV4097_SET_TEXTURE_CONTROL1 + (m_index*32)]
	}
	break;

	case_16(NV4097_SET_TEX_COORD_CONTROL, 4) :
	{
		ConLog.Warning("NV4097_SET_TEX_COORD_CONTROL");
	}
	break;

	case_16(NV4097_SET_TEXTURE_CONTROL3, 4) :
	{
		RSXTexture& tex = m_textures[index];
		const u32 a0 = ARGS(0);
		u32 pitch = a0 & 0xFFFFF;
		u16 depth = a0 >> 20;
		tex.SetControl3(depth, pitch);
	}
	break;
	
	// Vertex data
	case_16(NV4097_SET_VERTEX_DATA4UB_M, 4):
	{
		const u32 a0 = ARGS(0);
		u8 v0 = a0;
		u8 v1 = a0 >> 8;
		u8 v2 = a0 >> 16;
		u8 v3 = a0 >> 24;

		m_vertex_data[index].Reset();
		m_vertex_data[index].size = 4;
		m_vertex_data[index].type = 4;
		m_vertex_data[index].data.push_back(v0);
		m_vertex_data[index].data.push_back(v1);
		m_vertex_data[index].data.push_back(v2);
		m_vertex_data[index].data.push_back(v3);
		//ConLog.Warning("index = %d, v0 = 0x%x, v1 = 0x%x, v2 = 0x%x, v3 = 0x%x", index, v0, v1, v2, v3);
	}
	break;

	case_16(NV4097_SET_VERTEX_DATA2F_M, 8):
	{
		const u32 a0 = ARGS(0);
		const u32 a1 = ARGS(1);

		float v0 = (float&)a0;
		float v1 = (float&)a1;
		
		m_vertex_data[index].Reset();
		m_vertex_data[index].type = 2;
		m_vertex_data[index].size = 2;
		m_vertex_data[index].data.resize(sizeof(float) * 2);
		(float&)m_vertex_data[index].data[sizeof(float)*0] = v0;
		(float&)m_vertex_data[index].data[sizeof(float)*1] = v1;

		//ConLog.Warning("index = %d, v0 = %f, v1 = %f", index, v0, v1);
	}
	break;

	case_16(NV4097_SET_VERTEX_DATA4F_M, 16):
	{
		const u32 a0 = ARGS(0);
		const u32 a1 = ARGS(1);
		const u32 a2 = ARGS(2);
		const u32 a3 = ARGS(3);

		float v0 = (float&)a0;
		float v1 = (float&)a1;
		float v2 = (float&)a2;
		float v3 = (float&)a3;

		m_vertex_data[index].Reset();
		m_vertex_data[index].type = 2;
		m_vertex_data[index].size = 4;
		m_vertex_data[index].data.resize(sizeof(float) * 4);
		(float&)m_vertex_data[index].data[sizeof(float)*0] = v0;
		(float&)m_vertex_data[index].data[sizeof(float)*1] = v1;
		(float&)m_vertex_data[index].data[sizeof(float)*2] = v2;
		(float&)m_vertex_data[index].data[sizeof(float)*3] = v3;

		//ConLog.Warning("index = %d, v0 = %f, v1 = %f, v2 = %f, v3 = %f", index, v0, v1, v2, v3);
	}
	break;

	case_16(NV4097_SET_VERTEX_DATA_ARRAY_OFFSET, 4) :
	{
		const u32 addr = GetAddress(ARGS(0) & 0x7fffffff, ARGS(0) >> 31);
		CMD_LOG("num=%d, addr=0x%x", index, addr);
		m_vertex_data[index].addr = addr;
		m_vertex_data[index].data.clear();
	}
	break;

	case_16(NV4097_SET_VERTEX_DATA_ARRAY_FORMAT, 4) :
	{
		const u32 a0 = ARGS(0);
		u16 frequency = a0 >> 16;
		u8 stride = (a0 >> 8) & 0xff;
		u8 size = (a0 >> 4) & 0xf;
		u8 type = a0 & 0xf;

		CMD_LOG("index=%d, frequency=%d, stride=%d, size=%d, type=%d", index, frequency, stride, size, type);

		RSXVertexData& cv = m_vertex_data[index];
		cv.frequency = frequency;
		cv.stride = stride;
		cv.size = size;
		cv.type = type;
	}
	break;

	// Vertex Attribute
	case NV4097_SET_VERTEX_ATTRIB_INPUT_MASK:
	{
		if (ARGS(0))
			ConLog.Warning("NV4097_SET_VERTEX_ATTRIB_INPUT_MASK: %x", ARGS(0));

		//VertexData[0].prog.attributeInputMask = ARGS(0);
	}
	break;

	case NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK:
	{
		if (ARGS(0))
			ConLog.Warning("NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK: %x", ARGS(0));

		//VertexData[0].prog.attributeOutputMask = ARGS(0);
		//FragmentData.prog.attributeInputMask = ARGS(0)/* & ~0x20*/;
	}
	break;

	// Color Mask
	case NV4097_SET_COLOR_MASK:
	{
		const u32 a0 = ARGS(0);

		m_set_color_mask = true;
		m_color_mask_a = a0 & 0x1000000 ? true : false;
		m_color_mask_r = a0 & 0x0010000 ? true : false;
		m_color_mask_g = a0 & 0x0000100 ? true : false;
		m_color_mask_b = a0 & 0x0000001 ? true : false;
	}
	break;

	case NV4097_SET_COLOR_MASK_MRT:
	{
		if (ARGS(0))
			ConLog.Warning("NV4097_SET_COLOR_MASK_MRT: %x", ARGS(0));
	}
	break;

	// Alpha testing
	case NV4097_SET_ALPHA_TEST_ENABLE:
	{
		m_set_alpha_test = ARGS(0) ? true : false;
	}
	break;

	case NV4097_SET_ALPHA_FUNC:
	{
		m_set_alpha_func = true;
		m_alpha_func = ARGS(0);

		if (count >= 2)
		{
			m_set_alpha_ref = true;

			const u32 a1 = ARGS(1);
			m_alpha_ref = (float&)a1;
		}
	}
	break;

	case NV4097_SET_ALPHA_REF:
	{
		m_set_alpha_ref = true;
		const u32 a0 = ARGS(0);
		m_alpha_ref = (float&)a0;
	}
	break;

	// Cull face
	case NV4097_SET_CULL_FACE_ENABLE:
	{
		m_set_cull_face = ARGS(0) ? true : false;
	}
	break;

	case NV4097_SET_CULL_FACE:
	{
		m_cull_face = ARGS(0);
	}
	break;

	// Front face 
	case NV4097_SET_FRONT_FACE:
	{
		m_front_face = ARGS(0);
	}
	break;

	// Blending
	case NV4097_SET_BLEND_ENABLE:
	{
		m_set_blend = ARGS(0) ? true : false;
	}
	break;

	case NV4097_SET_BLEND_ENABLE_MRT:
	{
		if (ARGS(0))
			ConLog.Warning("NV4097_SET_BLEND_ENABLE_MRT: %x", ARGS(0));
	}
	break;

	case NV4097_SET_BLEND_FUNC_SFACTOR:
	{
		m_set_blend_sfactor = true;
		m_blend_sfactor_rgb = ARGS(0) & 0xffff;
		m_blend_sfactor_alpha = ARGS(0) >> 16;

		if (count >= 2)
		{
			m_set_blend_dfactor = true;
			m_blend_dfactor_rgb = ARGS(1) & 0xffff;
			m_blend_dfactor_alpha = ARGS(1) >> 16;
		}
	}
	break;

	case NV4097_SET_BLEND_FUNC_DFACTOR:
	{
		m_set_blend_dfactor = true;
		m_blend_dfactor_rgb = ARGS(0) & 0xffff;
		m_blend_dfactor_alpha = ARGS(0) >> 16;
	}
	break;

	case NV4097_SET_BLEND_COLOR:
	{
		m_set_blend_color = true;
		m_blend_color_r = ARGS(0) & 0xff;
		m_blend_color_g = (ARGS(0) >> 8) & 0xff;
		m_blend_color_b = (ARGS(0) >> 16) & 0xff;
		m_blend_color_a = (ARGS(0) >> 24) & 0xff;
	}
	break;

	case NV4097_SET_BLEND_COLOR2:
	{
		if (ARGS(0))
			ConLog.Warning("NV4097_SET_BLEND_COLOR2: 0x % x", ARGS(0));
	}
	break;

	case NV4097_SET_BLEND_EQUATION:
	{
		m_set_blend_equation = true;
		m_blend_equation_rgb = ARGS(0) & 0xffff;
		m_blend_equation_alpha = ARGS(0) >> 16;
	}
	break;

	case NV4097_SET_REDUCE_DST_COLOR:
	{
		if (ARGS(0))
			ConLog.Warning("NV4097_SET_REDUCE_DST_COLOR: 0x % x", ARGS(0));
	}
	break;

	// Depth bound testing
	case NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE:
	{
		m_set_depth_bounds_test = ARGS(0) ? true : false;
	}
	break;

	case NV4097_SET_DEPTH_BOUNDS_MIN:
	{
		m_set_depth_bounds = true;
		const u32 a0 = ARGS(0);
		m_depth_bounds_min = (float&)a0;
		if (count > 1)
		{
			const u32 a1 = ARGS(1);
			m_depth_bounds_max = (float&)a1;
		}
	}
	break;

	// Viewport
	case NV4097_SET_VIEWPORT_VERTICAL:
	{
		m_set_viewport_vertical = true;
		m_viewport_y = ARGS(0) & 0xffff;
		m_viewport_h = ARGS(0) >> 16;
	}
	break;

	case NV4097_SET_VIEWPORT_HORIZONTAL:
	{
		m_set_viewport_horizontal = true;
		m_viewport_x = ARGS(0) & 0xffff;
		m_viewport_w = ARGS(0) >> 16;

		if(count == 2)
		{
			m_set_viewport_vertical = true;
			m_viewport_y = ARGS(1) & 0xffff;
			m_viewport_h = ARGS(1) >> 16;
		}

		CMD_LOG("x=%d, y=%d, w=%d, h=%d", m_viewport_x, m_viewport_y, m_viewport_w, m_viewport_h);
	}
	break;

	case NV4097_SET_VIEWPORT_SCALE:
	case NV4097_SET_VIEWPORT_OFFSET:
	{
		// Done in Vertex Shader
	}
	break;

	// Clipping
	case NV4097_SET_CLIP_MIN:
	{
		const u32 a0 = ARGS(0);
		const u32 a1 = ARGS(1);

		m_set_clip = true;
		m_clip_min = (float&)a0;
		m_clip_max = (float&)a1;

		CMD_LOG("clip_min=%.01f, clip_max=%.01f", m_clip_min, m_clip_max);
	}
	break;

	case NV4097_SET_CLIP_MAX:
	{
		if (ARGS(0))
			ConLog.Warning("NV4097_SET_CLIP_MAX: %x", ARGS(0));
	}
	break;

	// Depth testing
	case NV4097_SET_DEPTH_TEST_ENABLE:
	{
		m_set_depth_test = ARGS(0) ? true : false;
	}
	break;
	
	case NV4097_SET_DEPTH_FUNC:
	{
		m_set_depth_func = true;
		m_depth_func = ARGS(0);
	}
	break;

	case NV4097_SET_DEPTH_MASK:
	{
		m_set_depth_mask = true;
		m_depth_mask = ARGS(0);
	}
	break;

	// Polygon
	case NV4097_SET_FRONT_POLYGON_MODE:
	{
		m_set_front_polygon_mode = true;
		m_front_polygon_mode = ARGS(0);
	}
	break;

	case NV4097_SET_BACK_POLYGON_MODE:
	{
		m_set_back_polygon_mode = true;
		m_back_polygon_mode = ARGS(0);
	}
	break;
	
	case NV4097_SET_POLY_OFFSET_FILL_ENABLE:
	{
		m_set_poly_offset_fill = ARGS(0) ? true : false;
	}
	break;

	case NV4097_SET_POLY_OFFSET_LINE_ENABLE:
	{
		m_set_poly_offset_line = ARGS(0) ? true : false;
	}
	break;

	case NV4097_SET_POLY_OFFSET_POINT_ENABLE:
	{
		m_set_poly_offset_point = ARGS(0) ? true : false;
	}
	break;

	case NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR:
	{
		m_set_depth_test = true;
		m_set_poly_offset_mode = true;

		const u32 a0 = ARGS(0);
		m_poly_offset_scale_factor = (float&)a0;
	}
	break;

	case NV4097_SET_POLYGON_OFFSET_BIAS:
	{
		m_set_depth_test = true;
		m_set_poly_offset_mode = true;

		const u32 a0 = ARGS(0);
		m_poly_offset_bias = (float&)a0;
	}
	break;

	// Clearing
	case NV4097_CLEAR_ZCULL_SURFACE:
	{
		u32 a0 = ARGS(0);

		if(a0 & 0x01) m_clear_surface_z = m_clear_z;
		if(a0 & 0x02) m_clear_surface_s = m_clear_s;

		m_clear_surface_mask |= a0 & 0x3;
	}
	break;

	case NV4097_CLEAR_SURFACE:
	{
		const u32 a0 = ARGS(0);

		if(a0 & 0x01) m_clear_surface_z = m_clear_z;
		if(a0 & 0x02) m_clear_surface_s = m_clear_s;
		if(a0 & 0x10) m_clear_surface_color_r = m_clear_color_r;
		if(a0 & 0x20) m_clear_surface_color_g = m_clear_color_g;
		if(a0 & 0x40) m_clear_surface_color_b = m_clear_color_b;
		if(a0 & 0x80) m_clear_surface_color_a = m_clear_color_a;

		m_clear_surface_mask |= a0;
	}
	break;

	case NV4097_SET_ZSTENCIL_CLEAR_VALUE:
	{
		const u32 a0 = ARGS(0);
		m_clear_s = a0 & 0xff;
		m_clear_z = a0 >> 8;
	}
	break;

	case NV4097_SET_COLOR_CLEAR_VALUE:
	{
		const u32 color = ARGS(0);
		m_clear_color_a = (color >> 24) & 0xff;
		m_clear_color_r = (color >> 16) & 0xff;
		m_clear_color_g = (color >> 8) & 0xff;
		m_clear_color_b = color & 0xff;
	}
	break;

	case NV4097_SET_CLEAR_RECT_HORIZONTAL:
	{
		if (ARGS(0))
			ConLog.Warning("NV4097_SET_CLEAR_RECT_HORIZONTAL: %x", ARGS(0));
	}
	break;

	case NV4097_SET_CLEAR_RECT_VERTICAL:
	{
		if (ARGS(0))
			ConLog.Warning("NV4097_SET_CLEAR_RECT_VERTICAL: %x", ARGS(0));
	}
	break;

	// Arrays
	case NV4097_DRAW_ARRAYS:
	{
		for(u32 c=0; c<count; ++c)
		{
			u32 ac = ARGS(c);
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
		m_indexed_array.m_addr = GetAddress(ARGS(0), ARGS(1) & 0xf);
		m_indexed_array.m_type = ARGS(1) >> 4;
	}
	break;

	case NV4097_DRAW_INDEX_ARRAY:
	{
		for(u32 c=0; c<count; ++c)
		{
			const u32 first = ARGS(c) & 0xffffff;
			const u32 _count = (ARGS(c) >> 24) + 1;

			if(first < m_indexed_array.m_first) m_indexed_array.m_first = first;

			for(u32 i=first; i<_count; ++i)
			{
				u32 index;
				switch(m_indexed_array.m_type)
				{
					case 0:
					{
						int pos = m_indexed_array.m_data.size();
						m_indexed_array.m_data.resize(m_indexed_array.m_data.size() + 4);
						index = Memory.Read32(m_indexed_array.m_addr + i * 4);
						*(u32*)&m_indexed_array.m_data[pos] = index;
						//ConLog.Warning("index 4: %d", *(u32*)&m_indexed_array.m_data[pos]);
					}
					break;

					case 1:
					{
						int pos = m_indexed_array.m_data.size();
						m_indexed_array.m_data.resize(m_indexed_array.m_data.size() + 2);
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
		const u32 a0 = ARGS(0);

		//ConLog.Warning("NV4097_SET_BEGIN_END: %x", a0);

		m_read_buffer = false;

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

	// Shader
	case NV4097_SET_SHADER_PROGRAM:
	{
		m_cur_shader_prog = &m_shader_progs[m_cur_shader_prog_num];
		//m_cur_shader_prog_num = (m_cur_shader_prog_num + 1) % 16;
		const  u32 a0 = ARGS(0);
		m_cur_shader_prog->offset = a0 & ~0x3;
		m_cur_shader_prog->addr = GetAddress(m_cur_shader_prog->offset, (a0 & 0x3) - 1);
		m_cur_shader_prog->ctrl = 0x40;
	}
	break;

	case NV4097_SET_SHADER_CONTROL:
	{
		if(!m_cur_shader_prog)
		{
			ConLog.Error("NV4097_SET_SHADER_CONTROL: m_cur_shader_prog == NULL");
			break;
		}

		m_cur_shader_prog->ctrl = ARGS(0);
	}
	break;

	case NV4097_SET_SHADE_MODE:
	{
		m_set_shade_mode = true;
		m_shade_mode = ARGS(0);
	}
	break;

	case NV4097_SET_SHADER_WINDOW:
	{
		const u32 a0 = ARGS(0);
		m_shader_window_height = a0 & 0xfff;
		m_shader_window_origin = (a0 >> 12) & 0xf;
		m_shader_window_pixel_centers = a0 >> 16;
	}
	break;

	// Transform 
	case NV4097_SET_TRANSFORM_PROGRAM_LOAD:
	{
		//ConLog.Warning("NV4097_SET_TRANSFORM_PROGRAM_LOAD: prog = %d", ARGS(0));

		m_cur_vertex_prog = &m_vertex_progs[ARGS(0)];
		m_cur_vertex_prog->data.clear();

		if(count == 2)
		{
			const u32 start = ARGS(1);
			if(start)
				ConLog.Warning("NV4097_SET_TRANSFORM_PROGRAM_LOAD: start = %d", start);
		}
	}
	break;

	case NV4097_SET_TRANSFORM_PROGRAM_START:
	{
		if (ARGS(0)) 
			ConLog.Warning("NV4097_SET_TRANSFORM_PROGRAM_START: 0x%x", ARGS(0));
	}
	break;

	case_32(NV4097_SET_TRANSFORM_PROGRAM, 4):
	{
		//ConLog.Warning("NV4097_SET_TRANSFORM_PROGRAM[%d](%d)", index, count);

		if(!m_cur_vertex_prog)
		{
			ConLog.Warning("NV4097_SET_TRANSFORM_PROGRAM: m_cur_vertex_prog == NULL");
			break;
		}

		for(u32 i=0; i<count; ++i) m_cur_vertex_prog->data.push_back(ARGS(i));
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

	case NV4097_SET_TRANSFORM_CONSTANT_LOAD:
	{
		if((count - 1) % 4)
		{
			CMD_LOG("NV4097_SET_TRANSFORM_CONSTANT_LOAD [%d]", count);
			break;
		}

		for(u32 id = ARGS(0), i = 1; i<count; ++id)
		{
			const u32 x = ARGS(i); i++;
			const u32 y = ARGS(i); i++;
			const u32 z = ARGS(i); i++;
			const u32 w = ARGS(i); i++;

			RSXTransformConstant c(id, (float&)x, (float&)y, (float&)z, (float&)w);

			CMD_LOG("SET_TRANSFORM_CONSTANT_LOAD[%d : %d] = (%f, %f, %f, %f)", i, id, c.x, c.y, c.z, c.w);
			m_transform_constants.push_back(c);
		}
	}
	break;

	// Invalidation
	case NV4097_INVALIDATE_L2:
	{
		if (ARGS(0))
			ConLog.Warning("NV4097_INVALIDATE_L2: %x", ARGS(0));
	}
	break;

	case NV4097_INVALIDATE_VERTEX_CACHE_FILE:
	{
		if (ARGS(0))
			ConLog.Warning("NV4097_INVALIDATE_VERTEX_CACHE_FILE: %x", ARGS(0));
	}
	break;

	case NV4097_INVALIDATE_VERTEX_FILE:
	{
		if (ARGS(0))
			ConLog.Warning("NV4097_INVALIDATE_VERTEX_FILE: %x", ARGS(0));
	}
	break;

	case NV4097_INVALIDATE_ZCULL:
	{
		if (ARGS(0))
			ConLog.Warning("NV4097_INVALIDATE_ZCULL: %x", ARGS(0));
	}
	break;

	// Logic Ops
	case NV4097_SET_LOGIC_OP_ENABLE:
	{
		m_set_logic_op = ARGS(0) ? true : false;
	}
	break;

	case NV4097_SET_LOGIC_OP:
	{
		m_logic_op = ARGS(0);
	}
	break;
	
	// Dithering
	case NV4097_SET_DITHER_ENABLE:
	{
		m_set_dither = ARGS(0) ? true : false;
	}
	break;

	// Stencil testing
	case NV4097_SET_STENCIL_TEST_ENABLE:
	{
		m_set_stencil_test = ARGS(0) ? true : false;
	}
	break;

	case NV4097_SET_STENCIL_MASK:
	{
		m_set_stencil_mask = true;
		m_stencil_mask = ARGS(0);
	}
	break;

	case NV4097_SET_STENCIL_FUNC:
	{
		m_set_stencil_func = true;
		m_stencil_func = ARGS(0);
		if(count >= 2)
		{
			m_set_stencil_func_ref = true;
			m_stencil_func_ref = ARGS(1);

			if(count >= 3)
			{
				m_set_stencil_func_mask = true;
				m_stencil_func_mask = ARGS(2);
			}
		}
	}
	break;

	case NV4097_SET_STENCIL_FUNC_REF:
	{
		m_set_stencil_func_ref = true;
		m_stencil_func_ref = ARGS(0);
	}
	break;

	case NV4097_SET_STENCIL_FUNC_MASK:
	{
		m_set_stencil_func_mask = true;
		m_stencil_func_mask = ARGS(0);
	}
	break;

	case NV4097_SET_STENCIL_OP_FAIL:
	{
		m_set_stencil_fail = true;
		m_stencil_fail = ARGS(0);
		if(count >= 2)
		{
			m_set_stencil_zfail = true;
			m_stencil_zfail = ARGS(1);

			if(count >= 3)
			{
				m_set_stencil_zpass = true;
				m_stencil_zpass = ARGS(2);
			}
		}
	}
	break;

	case NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE:
	{
		m_set_two_sided_stencil_test_enable = ARGS(0) ? true : false;
	}
	break;

	case NV4097_SET_BACK_STENCIL_MASK:
	{
		m_set_back_stencil_mask = true;
		m_back_stencil_mask = ARGS(0);
	}
	break;

	case NV4097_SET_BACK_STENCIL_FUNC:
	{
		m_set_back_stencil_func = true;
		m_back_stencil_func = ARGS(0);
		if(count >= 2)
		{
			m_set_back_stencil_func_ref = true;
			m_back_stencil_func_ref = ARGS(1);

			if(count >= 3)
			{
				m_set_back_stencil_func_mask = true;
				m_back_stencil_func_mask = ARGS(2);
			}
		}
	}
	break;

	case NV4097_SET_BACK_STENCIL_FUNC_REF:
	{
		m_set_back_stencil_func_ref = true;
		m_back_stencil_func_ref = ARGS(0);
	}
	break;

	case NV4097_SET_BACK_STENCIL_FUNC_MASK:
	{
		m_set_back_stencil_func_mask = true;
		m_back_stencil_func_mask = ARGS(0);
	}
	break;

	case NV4097_SET_BACK_STENCIL_OP_FAIL:
	{
		m_set_stencil_fail = true;
		m_stencil_fail = ARGS(0);
		if(count >= 2)
		{
			m_set_back_stencil_zfail = true;
			m_back_stencil_zfail = ARGS(1);

			if(count >= 3)
			{
				m_set_back_stencil_zpass = true;
				m_back_stencil_zpass = ARGS(2);
			}
		}
	}
	break;

	case NV4097_SET_SCULL_CONTROL:
	{
		const u32 a0 = ARGS(0);
		m_set_stencil_func = m_set_stencil_func_ref = m_set_stencil_func_mask = true;

		m_stencil_func = a0 & 0xffff;
		m_stencil_func_ref = (a0 >> 16) & 0xff;
		m_stencil_func_mask = (a0 >> 24) & 0xff;
	}
	break;

	// Primitive restart index
	case NV4097_SET_RESTART_INDEX_ENABLE:
	{
		m_set_restart_index = ARGS(0) ? true : false;
	}
	break;

	case NV4097_SET_RESTART_INDEX:
	{
		m_restart_index = ARGS(0);
	}
	break;

	// Point 
	case NV4097_SET_POINT_SIZE:
	{
		m_set_point_size = true;
		const u32 a0 = ARGS(0);
		m_point_size = (float&)a0;
	}
	break;

	case NV4097_SET_POINT_PARAMS_ENABLE:
	{
		if (ARGS(0))
			ConLog.Error("NV4097_SET_POINT_PARAMS_ENABLE");
	}
	break;

	case NV4097_SET_POINT_SPRITE_CONTROL:
	{
		m_set_point_sprite_control = ARGS(0) ? true : false;
	}
	break;

	// Lighting
	case NV4097_SET_SPECULAR_ENABLE:
	{
		m_set_specular = ARGS(0) ? true : false;
	}
	break;

	// Scissor
	case NV4097_SET_SCISSOR_VERTICAL:
	{
		m_set_scissor_vertical = true;
		m_scissor_y = ARGS(0) & 0xffff;
		m_scissor_h = ARGS(0) >> 16;
	}
	break;

	case NV4097_SET_SCISSOR_HORIZONTAL:
	{
		m_set_scissor_horizontal = true;
		m_scissor_x = ARGS(0) & 0xffff;
		m_scissor_w = ARGS(0) >> 16;

		if(count == 2)
		{
			m_set_scissor_vertical = true;
			m_scissor_y = ARGS(1) & 0xffff;
			m_scissor_h = ARGS(1) >> 16;
		}
	}
	break;

	// Semaphore
	case NV4097_SET_SEMAPHORE_OFFSET:
	case NV406E_SEMAPHORE_OFFSET:
	{
		m_set_semaphore_offset = true;
		m_semaphore_offset = ARGS(0);
	}
	break;

	case NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE:
	{
		if(m_set_semaphore_offset)
		{
			m_set_semaphore_offset = false;
			u32 value = ARGS(0);
			value = (value & 0xff00ff00) | ((value & 0xff) << 16) | ((value >> 16) & 0xff);

			Memory.Write32(Memory.RSXCMDMem.GetStartAddr() + m_semaphore_offset, value);
		}
	}
	break;

	case NV406E_SEMAPHORE_RELEASE:
	case NV4097_TEXTURE_READ_SEMAPHORE_RELEASE:
	{
		if(m_set_semaphore_offset)
		{
			m_set_semaphore_offset = false;
			Memory.Write32(Memory.RSXCMDMem.GetStartAddr() + m_semaphore_offset, ARGS(0));
		}
	}
	break;

	case NV406E_SEMAPHORE_ACQUIRE:
	{
		if (ARGS(0))
			ConLog.Warning("NV406E_SEMAPHORE_ACQUIRE: %x", ARGS(0));
	}
	break;


	// Depth/ Color buffer usage 
	case NV4097_SET_SURFACE_FORMAT:
	{
		const u32 a0 = ARGS(0);
		m_set_surface_format = true;
		m_surface_color_format = a0 & 0x1f;
		m_surface_depth_format = (a0 >> 5) & 0x7;
		m_surface_type = (a0 >> 8) & 0xf;
		m_surface_antialias = (a0 >> 12) & 0xf;
		m_surface_width = (a0 >> 16) & 0xff;
		m_surface_height = (a0 >> 24) & 0xff;

		switch (std::min((u32)6, count))
		{
		case 6: m_surface_pitch_b  = ARGS(5);
		case 5: m_surface_offset_b = ARGS(4);
		case 4: m_surface_offset_z = ARGS(3);
		case 3: m_surface_offset_a = ARGS(2);
		case 2: m_surface_pitch_a  = ARGS(1);
		}

		gcmBuffer* buffers = (gcmBuffer*)Memory.GetMemFromAddr(m_gcm_buffers_addr);
		m_buffer_width = re(buffers[m_gcm_current_buffer].width);
		m_buffer_height = re(buffers[m_gcm_current_buffer].height);
		
		m_width = m_buffer_width;
		m_height = m_buffer_height;

		if (Ini.GSDownscale.GetValue() && Ini.GSResolution.GetValue() == 4)
		{
			if (m_width == 1280 && m_height == 720)
			{
				// Set scale ratio for 720p
				m_width_scale = 1.125f;
				m_height_scale = 1.33f;

				// Downscale 720p to 480p
				m_width = 720;
				m_height = 480;
			}

			if (m_width == 1920 && m_height == 1080)
			{
				// Set scale ratio for 1080p
				m_width_scale = 0.75f;
				m_height_scale = 0.88f;

				// Downscale 1080p to 480p
				m_width = 720;
				m_height = 480;
			}
		}
	}
	break;

	case NV4097_SET_SURFACE_COLOR_TARGET:
	{
		m_surface_colour_target = ARGS(0);
	}
	break;

	case NV4097_SET_SURFACE_COLOR_AOFFSET:
	{
		m_surface_offset_a = ARGS(0);
	}
	break;

	case NV4097_SET_SURFACE_COLOR_BOFFSET:
	{
		m_surface_offset_b = ARGS(0);
	}
	break;

	case NV4097_SET_SURFACE_COLOR_COFFSET:
	{
		m_surface_offset_c = ARGS(0);
	}
	break;

	case NV4097_SET_SURFACE_COLOR_DOFFSET:
	{
		m_surface_offset_d = ARGS(0);
	}
	break;

	case NV4097_SET_SURFACE_ZETA_OFFSET:
	{
		m_surface_offset_z = ARGS(0);
	}
	break;

	case NV4097_SET_SURFACE_PITCH_A:
	{
		m_surface_pitch_a = ARGS(0);
	}
	break;

	case NV4097_SET_SURFACE_PITCH_B:
	{
		m_surface_pitch_b = ARGS(0);
	}
	break;

	case NV4097_SET_SURFACE_PITCH_C:
	{
		if (count != 4)
		{
			ConLog.Error("NV4097_SET_SURFACE_PITCH_C: Bad count (%d)", count);
			break;
		}

		m_surface_pitch_c = ARGS(0);
		m_surface_pitch_d = ARGS(1);
		m_surface_offset_c = ARGS(2);
		m_surface_offset_d = ARGS(3);
	}
	break;

	case NV4097_SET_SURFACE_PITCH_D:
	{
		if (ARGS(0))
			ConLog.Warning("NV4097_SET_SURFACE_PITCH_D: %x", ARGS(0));
	}
	break;

	case NV4097_SET_SURFACE_PITCH_Z:
	{
		m_surface_pitch_z = ARGS(0);
	}
	break;

	case NV4097_SET_CONTEXT_DMA_COLOR_A:
	{
		m_set_context_dma_color_a = true;
		m_context_dma_color_a = ARGS(0);
	}
	break;

	case NV4097_SET_CONTEXT_DMA_COLOR_B:
	{
		m_set_context_dma_color_b = true;
		m_context_dma_color_b = ARGS(0);
	}
	break;

	case NV4097_SET_CONTEXT_DMA_COLOR_C:
	{
		m_set_context_dma_color_c = true;
		m_context_dma_color_c = ARGS(0);

		if(count > 1)
		{
			m_set_context_dma_color_d = true;
			m_context_dma_color_d = ARGS(1);
		}
	}
	break;

	case NV4097_SET_CONTEXT_DMA_COLOR_D:
	{
		if (ARGS(0))
			ConLog.Warning("NV4097_SET_CONTEXT_DMA_COLOR_D: %x", ARGS(0));
	}
	break;

	case NV4097_SET_CONTEXT_DMA_ZETA:
	{
		m_set_context_dma_z = true;
		m_context_dma_z = ARGS(0);
	}
	break;

	case NV4097_SET_SURFACE_CLIP_VERTICAL:
	{
		const u32 a0 = ARGS(0);
		m_set_surface_clip_vertical = true;
		m_surface_clip_y = a0;
		m_surface_clip_h = a0 >> 16;
	}
	break;

	case NV4097_SET_SURFACE_CLIP_HORIZONTAL:
	{
		const u32 a0 = ARGS(0);

		m_set_surface_clip_horizontal = true;
		m_surface_clip_x = a0;
		m_surface_clip_w = a0 >> 16;

		if(count >= 2)
		{
			const u32 a1 = ARGS(1);
			m_set_surface_clip_vertical = true;
			m_surface_clip_y = a1;
			m_surface_clip_h = a1 >> 16;
		}
	}
	break;

	// Antialiasing
	case NV4097_SET_ANTI_ALIASING_CONTROL:
	{
		if (ARGS(0))
			ConLog.Warning("NV4097_SET_ANTI_ALIASING_CONTROL: %x", ARGS(0));
	}
	break;

	// Line/Polygon Smoothing
	case NV4097_SET_LINE_SMOOTH_ENABLE:
	{
		m_set_line_smooth = ARGS(0) ? true : false;
	}
	break;

	case NV4097_SET_POLY_SMOOTH_ENABLE:
	{
		m_set_poly_smooth = ARGS(0) ? true : false;
	}
	break;

	// Line width
	case NV4097_SET_LINE_WIDTH:
	{
		m_set_line_width = true;
		const u32 a0 = ARGS(0);
		m_line_width = (float)a0 / 8.0f;
	}
	break;

	// Line/Polygon Stipple
	case NV4097_SET_LINE_STIPPLE:
	{
		m_set_line_stipple = ARGS(0) ? true : false;
	}
	break;

	case NV4097_SET_LINE_STIPPLE_PATTERN:
	{
		m_set_line_stipple = true;
		const u32 a0 = ARGS(0);
		m_line_stipple_factor = a0 & 0xffff;
		m_line_stipple_pattern = a0 >> 16;
	}
	break;

	// Zcull
	case NV4097_SET_ZCULL_EN:
	{
		const u32 a0 = ARGS(0);

		m_set_depth_test = a0 & 0x1 ? true : false;
		m_set_stencil_test = a0 & 0x2 ? true : false;
	}
	break;

	case NV4097_SET_ZCULL_CONTROL0:
	{
		if (ARGS(0))
			ConLog.Warning("NV4097_SET_ZCULL_CONTROL0: %x", ARGS(0));

		//m_set_depth_func = true;
		//m_depth_func = ARGS(0) >> 4;
	}
	break;
	
	case NV4097_SET_ZCULL_CONTROL1:
	{
		if (ARGS(0))
			ConLog.Warning("NV4097_SET_ZCULL_CONTROL1: %x", ARGS(0));

		//m_set_depth_func = true;
		//m_depth_func = ARGS(0) >> 4;
	}
	break;

	// Reporting
	case NV4097_GET_REPORT:
	{
		const u32 a0 = ARGS(0);
		u8 type = a0 >> 24;
		u32 offset = a0 & 0xffffff;

		u32 value;
		switch(type)
		{
		case CELL_GCM_ZPASS_PIXEL_CNT:
		case CELL_GCM_ZCULL_STATS:
		case CELL_GCM_ZCULL_STATS1:
		case CELL_GCM_ZCULL_STATS2:
		case CELL_GCM_ZCULL_STATS3:
			value = 0;
			ConLog.Warning("NV4097_GET_REPORT: Unimplemented type %d", type);
		break;

		default:
			value = 0;
			ConLog.Error("NV4097_GET_REPORT: Bad type %d", type);
		break;
		}

		// Get timestamp, and convert it from microseconds to nanoseconds
		u64 timestamp = get_system_time() * 1000;

		// TODO: Reports can be written to the main memory or the local memory (controlled by NV4097_SET_CONTEXT_DMA_REPORT)
		Memory.Write64(m_local_mem_addr + offset + 0x0, timestamp);
		Memory.Write32(m_local_mem_addr + offset + 0x8, value);
		Memory.Write32(m_local_mem_addr + offset + 0xc, 0);
	}
	break;

	// Clip Plane
	case NV4097_SET_USER_CLIP_PLANE_CONTROL:
	{
		const u32 a0 = ARGS(0);
		m_set_clip_plane = true;
		m_clip_plane_0 = a0 & 0xf;
		m_clip_plane_1 = (a0 >> 4) & 0xf;
		m_clip_plane_2 = (a0 >> 8) & 0xf;
		m_clip_plane_3 = (a0 >> 12) & 0xf;
		m_clip_plane_4 = (a0 >> 16) & 0xf;
		m_clip_plane_5 = a0 >> 20;
	}
	break;

	// Fog
	case NV4097_SET_FOG_PARAMS:
	{
		m_set_fog_params = true;
		const u32 a0 = ARGS(0);
		const u32 a1 = ARGS(1);
		m_fog_param0 = (float&)a0;
		m_fog_param1 = (float&)a1;
	}
	break;

	case NV4097_SET_FOG_MODE:
	{
		m_set_fog_mode = true;
		m_fog_mode = ARGS(0);
	}
	break;

	// Zmin_max
	case NV4097_SET_ZMIN_MAX_CONTROL:
	{
		if (ARGS(0))
			ConLog.Warning("NV4097_SET_ZMIN_MAX_CONTROL: %x", ARGS(0));
	}
	break;

	// Windows Clipping
	case NV4097_SET_WINDOW_OFFSET:
	{
		if (ARGS(0))
			ConLog.Warning("NV4097_SET_WINDOW_OFFSET: %x", ARGS(0));
	}
	break;

	case NV4097_SET_WINDOW_CLIP_TYPE:
	{
		if (ARGS(0))
			ConLog.Warning("NV4097_SET_WINDOW_CLIP_TYPE: %x", ARGS(0));
	}
	break;

	case NV4097_SET_WINDOW_CLIP_HORIZONTAL:
	{
		if (ARGS(0))
			ConLog.Warning("NV4097_SET_WINDOW_CLIP_HORIZONTAL: %x", ARGS(0));
	}
	break;

	case NV4097_SET_WINDOW_CLIP_VERTICAL:
	{
		if (ARGS(0))
			ConLog.Warning("NV4097_SET_WINDOW_CLIP_VERTICAL: %x", ARGS(0));
	}
	break;

	case 0x000002c8:
	case 0x000002d0:
	case 0x000002d8:
	case 0x000002e0:
	case 0x000002e8:
	case 0x000002f0:
	case 0x000002f8:
	break;

	case NV0039_SET_CONTEXT_DMA_BUFFER_IN: // [E : RSXThread]: TODO: unknown/illegal method [0x00002184](0xfeed0000, 0xfeed0000)
	{
		const u32 srcContext = ARGS(0);
		const u32 dstContext = ARGS(1);

		if (srcContext == 0xfeed0000 && dstContext == 0xfeed0000)
		{
		}
		else
		{
			ConLog.Warning("NV0039_SET_CONTEXT_DMA_BUFFER_IN: TODO: srcContext=0x%x, dstContext=0x%x", srcContext, dstContext);
		}
	}
	break;

	case NV0039_OFFSET_IN: // [E : RSXThread]: TODO: unknown/illegal method [0x0000230c](0x0, 0xb00400, 0x0, 0x0, 0x384000, 0x1, 0x101, 0x0)
	{
		const u32 inOffset = ARGS(0);
		const u32 outOffset = ARGS(1);
		const u32 inPitch = ARGS(2);
		const u32 outPitch = ARGS(3);
		const u32 lineLength = ARGS(4);
		const u32 lineCount = ARGS(5);
		const u32 format = ARGS(6);
		const u8 outFormat = (format >> 8);
		const u8 inFormat = (format >> 0);
		const u32 notify = ARGS(7);

		if (lineCount == 1 && !inPitch && !outPitch && !notify && format == 0x101)
		{
			memcpy(&Memory[GetAddress(outOffset, 0)], &Memory[GetAddress(inOffset, 0)], lineLength);
		}
		else
		{
			ConLog.Warning("NV0039_OFFSET_IN: TODO: offset(in=0x%x, out=0x%x), pitch(in=0x%x, out=0x%x), line(len=0x%x, cnt=0x%x), fmt(in=0x%x, out=0x%x), notify=0x%x",
				inOffset, outOffset, inPitch, outPitch, lineLength, lineCount, inFormat, outFormat, notify);
		}
	}
	break;

	case NV0039_OFFSET_OUT: // [E : RSXThread]: TODO: unknown/illegal method [0x00002310](0x0)
	{
		const u32 offset = ARGS(0);

		if (!offset)
		{
		}
		else
		{
			ConLog.Warning("NV0039_OFFSET_OUT: TODO: offset=0x%x", offset);
		}
	}
	break;

	case NV3062_SET_OFFSET_DESTIN:
	{
		m_dst_offset = ARGS(0);
	}
	break;

	case NV308A_COLOR:
	{
		RSXTransformConstant c;
		c.id = m_dst_offset | ((u32)m_point_x << 2);

		if (count >= 1)
		{
			u32 a = ARGS(0);
			a = a << 16 | a >> 16;
			c.x = (float&)a;
		}

		if (count >= 2)
		{
			u32 a = ARGS(1);
			a = a << 16 | a >> 16;
			c.y = (float&)a;
		}

		if(count >= 3)
		{
			u32 a = ARGS(2);
			a = a << 16 | a >> 16;
			c.z = (float&)a;
		}

		if(count >= 4)
		{
			u32 a = ARGS(3);
			a = a << 16 | a >> 16;
			c.w = (float&)a;
		}

		if(count >= 5)
		{
			ConLog.Warning("NV308A_COLOR: count = %d", count);
		}

		//ConLog.Warning("NV308A_COLOR: [%d]: %f, %f, %f, %f", c.id, c.x, c.y, c.z, c.w);
		m_fragment_constants.push_back(c);
	}
	break;

	case NV308A_POINT:
	{
		const u32 a0 = ARGS(0);
		m_point_x = a0 & 0xffff;
		m_point_y = a0 >> 16;
	}
	break;

	case NV3062_SET_COLOR_FORMAT:
	{
		m_color_format = ARGS(0);
		m_color_format_src_pitch = ARGS(1);
		m_color_format_dst_pitch = ARGS(1) >> 16;
	}
	break;

	case NV3089_SET_COLOR_CONVERSION:
	{
		m_color_conv = ARGS(0);
		m_color_conv_fmt = ARGS(1);
		m_color_conv_op = ARGS(2);
		m_color_conv_in_x = ARGS(3);
		m_color_conv_in_y = ARGS(3) >> 16;
		m_color_conv_in_w = ARGS(4);
		m_color_conv_in_h = ARGS(4) >> 16;
		m_color_conv_out_x = ARGS(5);
		m_color_conv_out_y = ARGS(5) >> 16;
		m_color_conv_out_w = ARGS(6);
		m_color_conv_out_h = ARGS(6) >> 16;
		m_color_conv_dsdx = ARGS(7);
		m_color_conv_dtdy = ARGS(8);
	}
	break;

	case NV3089_IMAGE_IN_SIZE:
	{
		u16 width = ARGS(0);
		u16 height = ARGS(0) >> 16;

		u16 pitch = ARGS(1);
		u8 origin = ARGS(1) >> 16;
		u8 inter = ARGS(1) >> 24;

		u32 offset = ARGS(2);

		u16 u = ARGS(3);
		u16 v = ARGS(3) >> 16;

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
	{
		m_context_dma_img_src = ARGS(0);
	}
	break;

	case NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN:
	{
		m_context_dma_img_dst = ARGS(0);
	}
	break;

	case NV3089_SET_CONTEXT_SURFACE:
	{
		if (ARGS(0) != CELL_GCM_CONTEXT_SURFACE2D)
		{
			ConLog.Warning("NV3089_SET_CONTEXT_SURFACE: Unsupported surface (0x%x)", ARGS(0));
		}
	}
	break;

	default:
	{
		std::string log = GetMethodName(cmd);
		log += "(";
		for(u32 i=0; i<count; ++i) log += (i ? ", " : "") + fmt::Format("0x%x", ARGS(i));
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

	if(Emu.GetCallbackManager().m_exit_callback.m_callbacks.size())
	{
		//Emu.GetCallbackManager().m_exit_callback.Handle(0x0121, 0);
	}
}

void RSXThread::End()
{
	ExecCMD();

	if(Emu.GetCallbackManager().m_exit_callback.m_callbacks.size())
	{
		//Emu.GetCallbackManager().m_exit_callback.Handle(0x0122, 0);
	}

	m_indexed_array.Reset();
	m_fragment_constants.clear();
	m_transform_constants.clear();
	m_cur_shader_prog_num = 0;
	//m_cur_shader_prog = nullptr;

	m_clear_surface_mask = 0;
	m_begin_end = 0;

	//Reset();
	OnReset();
}

void RSXThread::Task()
{
	u8 inc;
	ConLog.Write("RSX thread started");

	OnInitThread();

	while(!TestDestroy())
	{
		if (Emu.IsStopped())
		{
			ConLog.Warning("RSX thread aborted");
			return;
		}
		std::lock_guard<std::mutex> lock(m_cs_main);

		inc=1;

		u32 put, get;
		// this code produces only mov + bswap:
		se_t<u32>::func(put, std::atomic_load((volatile std::atomic<u32>*)((u8*)m_ctrl + offsetof(CellGcmControl, put))));
		se_t<u32>::func(get, std::atomic_load((volatile std::atomic<u32>*)((u8*)m_ctrl + offsetof(CellGcmControl, get))));
		/*
		se_t<u32>::func(put, InterlockedCompareExchange((volatile unsigned long*)((u8*)m_ctrl + offsetof(CellGcmControl, put)), 0, 0));
		se_t<u32>::func(get, InterlockedCompareExchange((volatile unsigned long*)((u8*)m_ctrl + offsetof(CellGcmControl, get)), 0, 0));
		*/

		if(put == get || !Emu.IsRunning())
		{
			if(put == get)
			{
				if(m_flip_status == 0)
					m_sem_flip.post_and_wait();

				m_sem_flush.post_and_wait();
			}

			Sleep(1);
			continue;
		}

		//ConLog.Write("addr = 0x%x", m_ioAddress + get);
		const u32 cmd = Memory.Read32(Memory.RSXIOMem.GetStartAddr() + get);
		const u32 count = (cmd >> 18) & 0x7ff;
		//if(cmd == 0) continue;

		if(cmd & CELL_GCM_METHOD_FLAG_JUMP)
		{
			u32 addr = cmd & ~(CELL_GCM_METHOD_FLAG_JUMP | CELL_GCM_METHOD_FLAG_NON_INCREMENT);
			//ConLog.Warning("rsx jump(0x%x) #addr=0x%x, cmd=0x%x, get=0x%x, put=0x%x", addr, m_ioAddress + get, cmd, get, put);
			m_ctrl->get = addr;
			continue;
		}
		if(cmd & CELL_GCM_METHOD_FLAG_CALL)
		{
			m_call_stack.push(get + 4);
			u32 offs = cmd & ~CELL_GCM_METHOD_FLAG_CALL;
			u32 addr = Memory.RSXIOMem.GetStartAddr() + offs;
			//ConLog.Warning("rsx call(0x%x) #0x%x - 0x%x - 0x%x", offs, addr, cmd, get);
			m_ctrl->get = offs;
			continue;
		}
		if(cmd == CELL_GCM_METHOD_FLAG_RETURN)
		{
			//ConLog.Warning("rsx return!");
			u32 get = m_call_stack.top();
			m_call_stack.pop();
			//ConLog.Warning("rsx return(0x%x)", get);
			m_ctrl->get = get;
			continue;
		}
		if(cmd & CELL_GCM_METHOD_FLAG_NON_INCREMENT)
		{
			//ConLog.Warning("non increment cmd! 0x%x", cmd);
			inc=0;
		}

		if(cmd == 0)
		{
			//HACK! We couldn't be here
			//ConLog.Error("null cmd: addr=0x%x, put=0x%x, get=0x%x", Memory.RSXIOMem.GetStartAddr() + get, m_ctrl->put, get);
			//Emu.Pause();
			m_ctrl->get = get + (count + 1) * 4;
			continue;
		}

		for(u32 i=0; i<count; i++)
		{
			methodRegisters[(cmd & 0xffff) + (i*4*inc)] = ARGS(i);
		}

		mem32_ptr_t args(Memory.RSXIOMem.GetStartAddr() + get + 4);
		DoCmd(cmd, cmd & 0x3ffff, args, count);

		m_ctrl->get = get + (count + 1) * 4;
		//memset(Memory.GetMemFromAddr(p.m_ioAddress + get), 0, (count + 1) * 4);
	}

	ConLog.Write("RSX thread ended");

	OnExitThread();
}
