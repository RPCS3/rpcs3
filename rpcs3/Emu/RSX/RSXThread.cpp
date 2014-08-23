#include "stdafx.h"
#include "rpcs3/Ini.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "RSXThread.h"
#include "Emu/SysCalls/lv2/sys_time.h"

#define ARGS(x) (x >= count ? OutOfArgsCount(x, cmd, count, args.GetAddr()) : args[x].ToLE())

u32 methodRegisters[0xffff];

void RSXThread::nativeRescale(float width, float height)
{
	switch (Ini.GSResolution.GetValue())
	{
	case 1: // 1920x1080 window size
		m_width_scale = 1920 / width  * 2.0f;
		m_height_scale = 1080 / height * 2.0f;
		m_width = 1920;
		m_height = 1080;
		break;
	case 2: // 1280x720 window size
		m_width_scale = 1280 / width * 2.0f;
		m_height_scale = 720 / height * 2.0f;
		m_width = 1280;
		m_height = 720;
		break;
	case 4: // 720x480 window size
		m_width_scale = 720 / width * 2.0f;
		m_height_scale = 480 / height * 2.0f;
		m_width = 720;
		m_height = 480;
		break;
	case 5: // 720x576 window size
		m_width_scale = 720 / width * 2.0f;
		m_height_scale = 576 / height * 2.0f;
		m_width = 720;
		m_height = 576;
		break;
	}
}

u32 GetAddress(u32 offset, u8 location)
{
	switch(location)
	{
	case CELL_GCM_LOCATION_LOCAL: return Memory.RSXFBMem.GetStartAddr() + offset;
	case CELL_GCM_LOCATION_MAIN: return Memory.RSXIOMem.RealAddr(Memory.RSXIOMem.GetStartAddr() + offset); // TODO: Error Check?
	}

	LOG_ERROR(RSX, "GetAddress(offset=0x%x, location=0x%x)", location);
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

void RSXVertexData::Load(u32 start, u32 count, u32 baseOffset, u32 baseIndex=0)
{
	if(!addr) return;

	const u32 tsize = GetTypeSize();

	data.resize((start + count) * tsize * size);

	for(u32 i=start; i<start + count; ++i)
	{
		const u8* src = Memory.GetMemFromAddr(addr) + baseOffset + stride * (i+baseIndex);
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
	case CELL_GCM_VERTEX_S1:    return 2;
	case CELL_GCM_VERTEX_F:     return 4;
	case CELL_GCM_VERTEX_SF:    return 2;
	case CELL_GCM_VERTEX_UB:    return 1;
	case CELL_GCM_VERTEX_S32K:  return 2;
	case CELL_GCM_VERTEX_CMP:   return 4;
	case CELL_GCM_VERTEX_UB256: return 1;

	default:
		LOG_ERROR(RSX, "RSXVertexData::GetTypeSize: Bad vertex data type (%d)!", type);
		return 1;
	}
}

#define CMD_DEBUG 0

#if	CMD_DEBUG
	#define CMD_LOG(...) LOG_NOTICE(RSX, __VA_ARGS__)
#else
	#define CMD_LOG(...)
#endif

u32 RSXThread::OutOfArgsCount(const uint x, const u32 cmd, const u32 count, const u32 args_addr)
{
	mem32_ptr_t args(args_addr);
	std::string debug = GetMethodName(cmd);
	debug += "(";
	for(u32 i=0; i<count; ++i) debug += (i ? ", " : "") + fmt::Format("0x%x", ARGS(i));
	debug += ")";
	LOG_NOTICE(RSX, "OutOfArgsCount(x=%u, count=%u): " + debug, x, count);

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

void RSXThread::DoCmd(const u32 fcmd, const u32 cmd, const u32 args_addr, const u32 count)
{
	mem32_ptr_t args(args_addr);

#if	CMD_DEBUG
		std::string debug = GetMethodName(cmd);
		debug += "(";
		for(u32 i=0; i<count; ++i) debug += (i ? ", " : "") + fmt::Format("0x%x", ARGS(i));
		debug += ")";
		LOG_NOTICE(RSX, debug);
#endif

	u32 index = 0;

	m_used_gcm_commands.insert(cmd);

	//static u32 draw_array_count = 0;

	switch(cmd)
	{
	// NV406E
	case NV406E_SET_REFERENCE:
	{
		m_ctrl->ref = ARGS(0);
	}
	break;

	case NV406E_SET_CONTEXT_DMA_SEMAPHORE:
	{
		if (ARGS(0))
			LOG_WARNING(RSX, "NV406E_SET_CONTEXT_DMA_SEMAPHORE: %x", ARGS(0));
	}
	break;

	case NV4097_SET_SEMAPHORE_OFFSET:
	case NV406E_SEMAPHORE_OFFSET:
	{
		m_set_semaphore_offset = true;
		m_semaphore_offset = ARGS(0);
	}
	break;

	case NV406E_SEMAPHORE_ACQUIRE:
	{
		if (ARGS(0))
			LOG_WARNING(RSX, "NV406E_SEMAPHORE_ACQUIRE: %x", ARGS(0));
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

	// NV4097
	case 0x0003fead:
		//if(cmd == 0xfeadffff)
		{
			Flip();
			m_last_flip_time = get_system_time();

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
		// Nothing to do here 
	}
	break;

	case NV4097_NOTIFY:
	{
		if (ARGS(0))
			LOG_WARNING(RSX, "NV4097_NOTIFY: %x", ARGS(0));
	}
	break;

	case NV4097_WAIT_FOR_IDLE:
	{
		if (ARGS(0))
			LOG_WARNING(RSX, "NV4097_WAIT_FOR_IDLE: %x", ARGS(0));
	}
	break;

	case NV4097_PM_TRIGGER:
	{
		if (ARGS(0))
			LOG_WARNING(RSX, "NV4097_PM_TRIGGER: %x", ARGS(0));
	}
	break;

	// Texture
	case_16(NV4097_SET_TEXTURE_FORMAT, 0x20):
	case_16(NV4097_SET_TEXTURE_OFFSET, 0x20):
	case_16(NV4097_SET_TEXTURE_FILTER, 0x20):
	case_16(NV4097_SET_TEXTURE_ADDRESS, 0x20):
	case_16(NV4097_SET_TEXTURE_IMAGE_RECT, 32):
	case_16(NV4097_SET_TEXTURE_BORDER_COLOR, 0x20):
	case_16(NV4097_SET_TEXTURE_CONTROL0, 0x20):
	case_16(NV4097_SET_TEXTURE_CONTROL1, 0x20):
	{
		// Done using methodRegisters in RSXTexture.cpp
	}
	break;

	case_16(NV4097_SET_TEX_COORD_CONTROL, 4):
	{
		LOG_WARNING(RSX, "NV4097_SET_TEX_COORD_CONTROL");
	}
	break;

	case_16(NV4097_SET_TEXTURE_CONTROL3, 4):
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
		//LOG_WARNING(RSX, "index = %d, v0 = 0x%x, v1 = 0x%x, v2 = 0x%x, v3 = 0x%x", index, v0, v1, v2, v3);
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

		//LOG_WARNING(RSX, "index = %d, v0 = %f, v1 = %f", index, v0, v1);
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

		//LOG_WARNING(RSX, "index = %d, v0 = %f, v1 = %f, v2 = %f, v3 = %f", index, v0, v1, v2, v3);
	}
	break;

	case_16(NV4097_SET_VERTEX_DATA_ARRAY_OFFSET, 4):
	{
		const u32 addr = GetAddress(ARGS(0) & 0x7fffffff, ARGS(0) >> 31);
		CMD_LOG("num=%d, addr=0x%x", index, addr);
		m_vertex_data[index].addr = addr;
		m_vertex_data[index].data.clear();
	}
	break;

	case_16(NV4097_SET_VERTEX_DATA_ARRAY_FORMAT, 4):
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
			LOG_WARNING(RSX, "NV4097_SET_VERTEX_ATTRIB_INPUT_MASK: %x", ARGS(0));

		//VertexData[0].prog.attributeInputMask = ARGS(0);
	}
	break;

	case NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK:
	{
		if (ARGS(0))
			LOG_WARNING(RSX, "NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK: %x", ARGS(0));

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
			LOG_WARNING(RSX, "NV4097_SET_COLOR_MASK_MRT: %x", ARGS(0));
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

		if (count == 2)
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
		m_set_blend_mrt1 = ARGS(0) & 0x02 ? true : false;
		m_set_blend_mrt2 = ARGS(0) & 0x04 ? true : false;
		m_set_blend_mrt3 = ARGS(0) & 0x08 ? true : false;
	}
	break;

	case NV4097_SET_BLEND_FUNC_SFACTOR:
	{
		m_set_blend_sfactor = true;
		m_blend_sfactor_rgb = ARGS(0) & 0xffff;
		m_blend_sfactor_alpha = ARGS(0) >> 16;

		if (count == 2)
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
			LOG_WARNING(RSX, "NV4097_SET_BLEND_COLOR2: 0x % x", ARGS(0));
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
			LOG_WARNING(RSX, "NV4097_SET_REDUCE_DST_COLOR: 0x%x", ARGS(0));
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

		if (count == 2)
		{
			const u32 a1 = ARGS(1);
			m_depth_bounds_max = (float&)a1;
		}
	}
	break;

	case NV4097_SET_DEPTH_BOUNDS_MAX:
	{
		m_set_depth_bounds = true;
		const u32 a0 = ARGS(0);
		m_depth_bounds_max = (float&)a0;
	}
	break;

	// Viewport
	case NV4097_SET_VIEWPORT_HORIZONTAL:
	{
		m_set_viewport_horizontal = true;
		m_viewport_x = ARGS(0) & 0xffff;
		m_viewport_w = ARGS(0) >> 16;

		if (count == 2)
		{
			m_set_viewport_vertical = true;
			m_viewport_y = ARGS(1) & 0xffff;
			m_viewport_h = ARGS(1) >> 16;
		}

		CMD_LOG("x=%d, y=%d, w=%d, h=%d", m_viewport_x, m_viewport_y, m_viewport_w, m_viewport_h);
	}
	break;

	case NV4097_SET_VIEWPORT_VERTICAL:
	{
		m_set_viewport_vertical = true;
		m_viewport_y = ARGS(0) & 0xffff;
		m_viewport_h = ARGS(0) >> 16;
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
		const u32 a0 = ARGS(0);

		m_set_clip = true;
		m_clip_max = (float&)a0;

		CMD_LOG("clip_max=%.01f", m_clip_max);
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

	// Polygon mode/offset
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

		if (count == 2)
		{
			const u32 a1 = ARGS(1);
			m_poly_offset_bias = (float&)a1;
		}
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

		m_clear_surface_mask = a0;
		ExecCMD(NV4097_CLEAR_SURFACE);
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
			LOG_WARNING(RSX, "NV4097_SET_CLEAR_RECT_HORIZONTAL: %x", ARGS(0));
	}
	break;

	case NV4097_SET_CLEAR_RECT_VERTICAL:
	{
		if (ARGS(0))
			LOG_WARNING(RSX, "NV4097_SET_CLEAR_RECT_VERTICAL: %x", ARGS(0));
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

			//LOG_WARNING(RSX, "NV4097_DRAW_ARRAYS: %d - %d", first, _count);

			LoadVertexData(first, _count);

			if (first < m_draw_array_first)
			{
				m_draw_array_first = first;
			}

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
						//LOG_WARNING(RSX, "index 4: %d", *(u32*)&m_indexed_array.m_data[pos]);
					}
					break;

					case 1:
					{
						int pos = m_indexed_array.m_data.size();
						m_indexed_array.m_data.resize(m_indexed_array.m_data.size() + 2);
						index = Memory.Read16(m_indexed_array.m_addr + i * 2);
						//LOG_WARNING(RSX, "index 2: %d", index);
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

	case NV4097_SET_VERTEX_DATA_BASE_OFFSET:
	{
		m_vertex_data_base_offset = ARGS(0);
		if (count >= 2) {
			m_vertex_data_base_index = ARGS(1);
		}
	}
	break;

	case NV4097_SET_VERTEX_DATA_BASE_INDEX:
	{
		m_vertex_data_base_index = ARGS(0);
	}
	break;

	case NV4097_SET_BEGIN_END:
	{
		const u32 a0 = ARGS(0);

		//LOG_WARNING(RSX, "NV4097_SET_BEGIN_END: %x", a0);

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
		m_shader_ctrl = ARGS(0);
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
		//LOG_WARNING(RSX, "NV4097_SET_TRANSFORM_PROGRAM_LOAD: prog = %d", ARGS(0));

		m_cur_vertex_prog = &m_vertex_progs[ARGS(0)];
		m_cur_vertex_prog->data.clear();

		if (count == 2)
		{
			const u32 start = ARGS(1);
			if (start)
			{
				LOG_WARNING(RSX, "NV4097_SET_TRANSFORM_PROGRAM_LOAD: start = %d", start);
			}
		}
	}
	break;

	case NV4097_SET_TRANSFORM_PROGRAM_START:
	{
		const u32 start = ARGS(0);
		if (start) 
		{
			LOG_WARNING(RSX, "NV4097_SET_TRANSFORM_PROGRAM_START: start = %d", start);
		}
	}
	break;

	case_32(NV4097_SET_TRANSFORM_PROGRAM, 4):
	{
		//LOG_WARNING(RSX, "NV4097_SET_TRANSFORM_PROGRAM[%d](%d)", index, count);

		if(!m_cur_vertex_prog)
		{
			LOG_WARNING(RSX, "NV4097_SET_TRANSFORM_PROGRAM: m_cur_vertex_prog == NULL");
			break;
		}

		for(u32 i=0; i<count; ++i) m_cur_vertex_prog->data.push_back(ARGS(i));
	}
	break;

	case NV4097_SET_TRANSFORM_TIMEOUT:

		// TODO:
		// (cmd)[1] = CELL_GCM_ENDIAN_SWAP((count) | ((registerCount) << 16)); \

		if(!m_cur_vertex_prog)
		{
			LOG_WARNING(RSX, "NV4097_SET_TRANSFORM_TIMEOUT: m_cur_vertex_prog == NULL");
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
			LOG_WARNING(RSX, "NV4097_INVALIDATE_L2: %x", ARGS(0));
	}
	break;

	case NV4097_INVALIDATE_VERTEX_CACHE_FILE:
	{
		// Nothing to do here
	}
	break;

	case NV4097_INVALIDATE_VERTEX_FILE:
	{
		// Nothing to do here
	}
	break;

	case NV4097_INVALIDATE_ZCULL:
	{
		if (ARGS(0))
			LOG_WARNING(RSX, "NV4097_INVALIDATE_ZCULL: %x", ARGS(0));
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

	case NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE:
	{
		m_set_two_sided_stencil_test_enable = ARGS(0) ? true : false;
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
		if (ARGS(0))
			LOG_WARNING(RSX, "NV4097_SET_SCULL_CONTROL: %x", ARGS(0));
		
		//This is stencil culling , nothing to do with stencil masking on regular color or depth buffer 
		//const u32 a0 = ARGS(0);
		//m_set_stencil_func = m_set_stencil_func_ref = m_set_stencil_func_mask = true;

		//m_stencil_func = a0 & 0xffff;
		//m_stencil_func_ref = (a0 >> 16) & 0xff;
		//m_stencil_func_mask = (a0 >> 24) & 0xff;
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

	// Point size
	case NV4097_SET_POINT_SIZE:
	{
		m_set_point_size = true;
		const u32 a0 = ARGS(0);
		m_point_size = (float&)a0;
	}
	break;

	// Point sprite
	case NV4097_SET_POINT_PARAMS_ENABLE:
	{
		if (ARGS(0))
			LOG_ERROR(RSX, "NV4097_SET_POINT_PARAMS_ENABLE: %x", ARGS(0));
	}
	break;

	case NV4097_SET_POINT_SPRITE_CONTROL:
	{
		m_set_point_sprite_control = ARGS(0) ? true : false;

		// TODO:
		//(cmd)[1] = CELL_GCM_ENDIAN_SWAP((enable) | ((rmode) << 1) | (texcoordMask));
	}
	break;

	// Lighting
	case NV4097_SET_SPECULAR_ENABLE:
	{
		m_set_specular = ARGS(0) ? true : false;
	}
	break;

	// Scissor
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

	case NV4097_SET_SCISSOR_VERTICAL:
	{
		m_set_scissor_vertical = true;
		m_scissor_y = ARGS(0) & 0xffff;
		m_scissor_h = ARGS(0) >> 16;
	}
	break;

	// Depth/Color buffer usage 
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

		CellGcmDisplayInfo* buffers = (CellGcmDisplayInfo*)Memory.GetMemFromAddr(m_gcm_buffers_addr);
		m_width = buffers[m_gcm_current_buffer].width;
		m_height = buffers[m_gcm_current_buffer].height;

		// Rescale native resolution to fit 1080p/720p/480p/576p window size
		nativeRescale((float)m_width, (float)m_height);
	}
	break;

	case NV4097_SET_SURFACE_COLOR_TARGET:
	{
		m_surface_color_target = ARGS(0);
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
			LOG_WARNING(RSX, "NV4097_SET_SURFACE_PITCH_C: Bad count (%d)", count);
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
			LOG_WARNING(RSX, "NV4097_SET_SURFACE_PITCH_D: %x", ARGS(0));
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
			LOG_WARNING(RSX, "NV4097_SET_CONTEXT_DMA_COLOR_D: %x", ARGS(0));
	}
	break;

	case NV4097_SET_CONTEXT_DMA_ZETA:
	{
		m_set_context_dma_z = true;
		m_context_dma_z = ARGS(0);
	}
	break;

	case NV4097_SET_CONTEXT_DMA_SEMAPHORE:
	{
		if (ARGS(0))
			LOG_WARNING(RSX, "NV4097_SET_CONTEXT_DMA_SEMAPHORE: %x", ARGS(0));
	}
	break;

	case NV4097_SET_CONTEXT_DMA_NOTIFIES:
	{
		if (ARGS(0))
			LOG_WARNING(RSX, "NV4097_SET_CONTEXT_DMA_NOTIFIES: %x", ARGS(0));
	}
	break;

	case NV4097_SET_SURFACE_CLIP_HORIZONTAL:
	{
		const u32 a0 = ARGS(0);

		m_set_surface_clip_horizontal = true;
		m_surface_clip_x = a0;
		m_surface_clip_w = a0 >> 16;

		if(count == 2)
		{
			const u32 a1 = ARGS(1);
			m_set_surface_clip_vertical = true;
			m_surface_clip_y = a1;
			m_surface_clip_h = a1 >> 16;
		}
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

	// Anti-aliasing
	case NV4097_SET_ANTI_ALIASING_CONTROL:
	{
		const u32 a0 = ARGS(0);

		const u8 enable = a0 & 0xf;
		const u8 alphaToCoverage = (a0 >> 4) & 0xf;
		const u8 alphaToOne = (a0 >> 8) & 0xf;
		const u16 sampleMask = a0 >> 16;
		
		LOG_WARNING(RSX, "TODO: NV4097_SET_ANTI_ALIASING_CONTROL: %x", a0);
	}
	break;

	// Line/Polygon smoothing
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

	// Line/Polygon stipple
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

	case NV4097_SET_POLYGON_STIPPLE:
	{
		m_set_polygon_stipple = ARGS(0) ? true : false;
	}
	break;

	case NV4097_SET_POLYGON_STIPPLE_PATTERN:
	{
		for (size_t i = 0; i < 32; i++)
		{
			m_polygon_stipple_pattern[i] = ARGS(i);
		}
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
			LOG_WARNING(RSX, "NV4097_SET_ZCULL_CONTROL0: %x", ARGS(0));

		//m_set_depth_func = true;
		//m_depth_func = ARGS(0) >> 4;
	}
	break;
	
	case NV4097_SET_ZCULL_CONTROL1:
	{
		if (ARGS(0))
			LOG_WARNING(RSX, "NV4097_SET_ZCULL_CONTROL1: %x", ARGS(0));

		//m_set_depth_func = true;
		//m_depth_func = ARGS(0) >> 4;
	}
	break;

	case NV4097_SET_ZCULL_STATS_ENABLE:
	{
		if (ARGS(0))
			LOG_WARNING(RSX, "NV4097_SET_ZCULL_STATS_ENABLE: %x", ARGS(0));
	}
	break;

	case NV4097_ZCULL_SYNC:
	{
		if (ARGS(0))
			LOG_WARNING(RSX, "NV4097_ZCULL_SYNC: %x", ARGS(0));
	}
	break;

	// Reports
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
			LOG_WARNING(RSX, "NV4097_GET_REPORT: Unimplemented type %d", type);
		break;

		default:
			value = 0;
			LOG_WARNING(RSX, "NV4097_GET_REPORT: Bad type %d", type);
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

	case NV4097_CLEAR_REPORT_VALUE:
	{
		const u32 type = ARGS(0);

		switch(type)
		{
		case CELL_GCM_ZPASS_PIXEL_CNT:
			LOG_WARNING(RSX, "TODO: NV4097_CLEAR_REPORT_VALUE: ZPASS_PIXEL_CNT");
			break;
		case CELL_GCM_ZCULL_STATS:
			LOG_WARNING(RSX, "TODO: NV4097_CLEAR_REPORT_VALUE: ZCULL_STATS");
			break;
		default:
			LOG_ERROR(RSX, "NV4097_CLEAR_REPORT_VALUE: Bad type: %d", type);
		}	
	}
	break;

	// Clip Plane
	case NV4097_SET_USER_CLIP_PLANE_CONTROL:
	{
		const u32 a0 = ARGS(0);
		m_set_clip_plane = true;
		m_clip_plane_0 = (a0 & 0xf) ? true : false;
		m_clip_plane_1 = ((a0 >> 4)) & 0xf ? true : false;
		m_clip_plane_2 = ((a0 >> 8)) & 0xf ? true : false;
		m_clip_plane_3 = ((a0 >> 12)) & 0xf ? true : false;
		m_clip_plane_4 = ((a0 >> 16)) & 0xf ? true : false;
		m_clip_plane_5 = (a0 >> 20) ? true : false;
	}
	break;

	// Fog
	case NV4097_SET_FOG_MODE:
	{
		m_set_fog_mode = true;
		m_fog_mode = ARGS(0);
	}
	break;

	case NV4097_SET_FOG_PARAMS:
	{
		m_set_fog_params = true;
		const u32 a0 = ARGS(0);
		const u32 a1 = ARGS(1);
		m_fog_param0 = (float&)a0;
		m_fog_param1 = (float&)a1;
	}
	break;

	// Zmin_max
	case NV4097_SET_ZMIN_MAX_CONTROL:
	{
		const u8 cullNearFarEnable = ARGS(0) & 0xf;
		const u8 zclampEnable = (ARGS(0) >> 4) & 0xf;
		const u8 cullIgnoreW = (ARGS(0) >> 8) & 0xf;
		LOG_WARNING(RSX, "TODO: NV4097_SET_ZMIN_MAX_CONTROL: cullNearFarEnable=%d, zclampEnable=%d, cullIgnoreW=%d",
			cullNearFarEnable, zclampEnable, cullIgnoreW);
	}
	break;

	// Windows Clipping
	case NV4097_SET_WINDOW_OFFSET:
	{
		const u16 x = ARGS(0);
		const u16 y = ARGS(0) >> 16;
		LOG_WARNING(RSX, "TODO: NV4097_SET_WINDOW_OFFSET: x=%d, y=%d", x, y);
	}
	break;

	case NV4097_SET_FREQUENCY_DIVIDER_OPERATION:
	{
		m_set_frequency_divider_operation = ARGS(0);
	}
	break;

	case NV4097_SET_RENDER_ENABLE:
	{
		const u32 offset = ARGS(0) & 0xffffff;
		const u8 mode = ARGS(0) >> 24;
		LOG_WARNING(RSX, "NV4097_SET_RENDER_ENABLE: Offset=%06x, Mode=%x", offset, mode);
	}
	break;

	case NV4097_SET_ZPASS_PIXEL_COUNT_ENABLE:
	{
		const u32 enable = ARGS(0);	
		LOG_WARNING(RSX, "TODO: NV4097_SET_ZPASS_PIXEL_COUNT_ENABLE: %d", enable);
	}
	break;

	// NV0039
	case NV0039_SET_CONTEXT_DMA_BUFFER_IN:
	{
		const u32 srcContext = ARGS(0);
		const u32 dstContext = ARGS(1);
		m_context_dma_buffer_in_src = srcContext;
		m_context_dma_buffer_in_dst = dstContext;
	}
	break;

	case NV0039_OFFSET_IN:
	{
		const u32 inOffset = ARGS(0);
		const u32 outOffset = ARGS(1);
		const u32 inPitch = ARGS(2);
		const u32 outPitch = ARGS(3);
		const u32 lineLength = ARGS(4);
		const u32 lineCount = ARGS(5);
		const u8 outFormat = (ARGS(6) >> 8);
		const u8 inFormat = (ARGS(6) >> 0);
		const u32 notify = ARGS(7);

		// The existing GCM commands use only the value 0x1 for inFormat and outFormat
		if (inFormat != 0x01 || outFormat != 0x01) {
			LOG_ERROR(RSX, "NV0039_OFFSET_IN: Unsupported format: inFormat=%d, outFormat=%d", inFormat, outFormat);
		}

		if (lineCount == 1 && !inPitch && !outPitch && !notify)
		{
			memcpy(&Memory[GetAddress(outOffset, 0)], &Memory[GetAddress(inOffset, 0)], lineLength);
		}
		else
		{
			LOG_WARNING(RSX, "NV0039_OFFSET_IN: TODO: offset(in=0x%x, out=0x%x), pitch(in=0x%x, out=0x%x), line(len=0x%x, cnt=0x%x), fmt(in=0x%x, out=0x%x), notify=0x%x",
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
			LOG_WARNING(RSX, "NV0039_OFFSET_OUT: TODO: offset=0x%x", offset);
		}
	}
	break;

	case NV0039_PITCH_IN:
	{
		if (ARGS(0))
			LOG_WARNING(RSX, "NV0039_PITCH_IN: %x", ARGS(0));
	}
	break;

	case NV0039_BUFFER_NOTIFY:
	{
		if (ARGS(0))
			LOG_WARNING(RSX, "NV0039_BUFFER_NOTIFY: %x", ARGS(0));
	}
	break;

	// NV3062
	case NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN:
	{
		m_context_dma_img_dst = ARGS(0);
	}
	break;

	case NV3062_SET_OFFSET_DESTIN:
	{
		m_dst_offset = ARGS(0);
	}
	break;

	case NV3062_SET_COLOR_FORMAT:
	{
		m_color_format = ARGS(0);
		m_color_format_src_pitch = ARGS(1);
		m_color_format_dst_pitch = ARGS(1) >> 16;
	}
	break;

	// NV309E
	case NV309E_SET_CONTEXT_DMA_IMAGE:
	{
		if (ARGS(0))
			LOG_WARNING(RSX, "NV309E_SET_CONTEXT_DMA_IMAGE: %x", ARGS(0));
	}
	break;

	case NV309E_SET_FORMAT:
	{
		const u8 height = ARGS(0) >> 24;
		const u8 width = ARGS(0) >> 16;
		const u8 format = ARGS(0);
		const u32 offset = ARGS(1);
		LOG_WARNING(RSX, "NV309E_SET_FORMAT: Format:0x%x, Width:%d, Height:%d, Offset:0x%x", format, width, height, offset);
	}
	break;

	// NV308A
	case NV308A_POINT:
	{
		const u32 a0 = ARGS(0);
		m_point_x = a0 & 0xffff;
		m_point_y = a0 >> 16;
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
			LOG_WARNING(RSX, "NV308A_COLOR: count = %d", count);
		}

		//LOG_WARNING(RSX, "NV308A_COLOR: [%d]: %f, %f, %f, %f", c.id, c.x, c.y, c.z, c.w);
		m_fragment_constants.push_back(c);
	}
	break;

	// NV3089
	case NV3089_SET_CONTEXT_DMA_IMAGE:
	{
		m_context_dma_img_src = ARGS(0);
	}
	break;

	case NV3089_SET_CONTEXT_SURFACE:
	{
		if (ARGS(0) != CELL_GCM_CONTEXT_SURFACE2D)
		{
			LOG_WARNING(RSX, "NV3089_SET_CONTEXT_SURFACE: Unsupported surface (0x%x)", ARGS(0));
		}
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

	case GCM_SET_USER_COMMAND:
	{
		const u32 cause = ARGS(0);
		m_user_handler.Handle(cause);
		m_user_handler.Branch(false);
	}
	break;

	// Note: What is this? NV4097 offsets?
	case 0x000002c8:
	case 0x000002d0:
	case 0x000002d8:
	case 0x000002e0:
	case 0x000002e8:
	case 0x000002f0:
	case 0x000002f8:
	break;

	// The existing GCM commands don't use any of the following NV4097 / NV0039 / NV3062 / NV309E / NV308A / NV3089 methods
	case NV4097_SET_WINDOW_CLIP_TYPE:
	case NV4097_SET_WINDOW_CLIP_HORIZONTAL:
	case NV4097_SET_WINDOW_CLIP_VERTICAL:
	{
		LOG_WARNING(RSX, "Unused NV4097 method 0x%x detected!", cmd);
	}
	break;

	case NV0039_SET_CONTEXT_DMA_BUFFER_OUT:
	case NV0039_PITCH_OUT:
	case NV0039_LINE_LENGTH_IN:
	case NV0039_LINE_COUNT:
	case NV0039_FORMAT:
	case NV0039_SET_OBJECT:
	case NV0039_SET_CONTEXT_DMA_NOTIFIES:
	{
		LOG_WARNING(RSX, "Unused NV0039 method 0x%x detected!", cmd);
	}
	break;

	case NV3062_SET_OBJECT:
	case NV3062_SET_CONTEXT_DMA_NOTIFIES:
	case NV3062_SET_CONTEXT_DMA_IMAGE_SOURCE:
	case NV3062_SET_PITCH:
	case NV3062_SET_OFFSET_SOURCE:
	{
		LOG_WARNING(RSX, "Unused NV3062 method 0x%x detected!", cmd);
	}
	break;

	case NV308A_SET_OBJECT:
	case NV308A_SET_CONTEXT_DMA_NOTIFIES:
	case NV308A_SET_CONTEXT_COLOR_KEY:
	case NV308A_SET_CONTEXT_CLIP_RECTANGLE:
	case NV308A_SET_CONTEXT_PATTERN:
	case NV308A_SET_CONTEXT_ROP:
	case NV308A_SET_CONTEXT_BETA1:
	case NV308A_SET_CONTEXT_BETA4:
	case NV308A_SET_CONTEXT_SURFACE:
	case NV308A_SET_COLOR_CONVERSION:
	case NV308A_SET_OPERATION:
	case NV308A_SET_COLOR_FORMAT:
	case NV308A_SIZE_OUT:
	case NV308A_SIZE_IN:
	{
		LOG_WARNING(RSX, "Unused NV308A method 0x%x detected!", cmd);
	}
	break;

	case NV309E_SET_OBJECT:
	case NV309E_SET_CONTEXT_DMA_NOTIFIES:
	case NV309E_SET_OFFSET:
	{
		LOG_WARNING(RSX, "Unused NV309E method 0x%x detected!", cmd);
	}
	break;

	case NV3089_SET_OBJECT:
	case NV3089_SET_CONTEXT_DMA_NOTIFIES:
	case NV3089_SET_CONTEXT_PATTERN:
	case NV3089_SET_CONTEXT_ROP:
	case NV3089_SET_CONTEXT_BETA1:
	case NV3089_SET_CONTEXT_BETA4:
	case NV3089_SET_COLOR_FORMAT:
	case NV3089_SET_OPERATION:
	case NV3089_CLIP_POINT:
	case NV3089_CLIP_SIZE:
	case NV3089_IMAGE_OUT_POINT:
	case NV3089_IMAGE_OUT_SIZE:
	case NV3089_DS_DX:
	case NV3089_DT_DY:
	case NV3089_IMAGE_IN_FORMAT:
	case NV3089_IMAGE_IN_OFFSET:
	case NV3089_IMAGE_IN:
	{
		LOG_WARNING(RSX, "Unused NV3089 method 0x%x detected!", cmd);
	}
	break;

	default:
	{
		std::string log = GetMethodName(cmd);
		log += "(";
		for (u32 i=0; i<count; ++i) {
			log += (i ? ", " : "") + fmt::Format("0x%x", ARGS(i));
		}
		log += ")";
		LOG_ERROR(RSX, "TODO: " + log);
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
	LOG_NOTICE(RSX, "RSX thread started");

	OnInitThread();

	m_last_flip_time = get_system_time();
	volatile bool is_vblank_stopped = false;

	thread vblank("VBlank thread", [&]()
	{
		const u64 start_time = get_system_time();

		m_vblank_count = 0;

		while (!TestDestroy())
		{
			if (Emu.IsStopped())
			{
				LOG_WARNING(RSX, "VBlank thread aborted");
				return;
			}

			if (get_system_time() - start_time > m_vblank_count * 1000000 / 60)
			{
				m_vblank_count++;
				if (m_vblank_handler)
				{
					m_vblank_handler.Handle(1);
					m_vblank_handler.Branch(false);
				}
				continue;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		is_vblank_stopped = true;
	});
	vblank.detach();

	while(!TestDestroy())
	{
		if (Emu.IsStopped())
		{
			LOG_WARNING(RSX, "RSX thread aborted");
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

			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}

		//ConLog.Write("addr = 0x%x", m_ioAddress + get);
		const u32 cmd = ReadIO32(get);
		const u32 count = (cmd >> 18) & 0x7ff;
		//if(cmd == 0) continue;

		if(cmd & CELL_GCM_METHOD_FLAG_JUMP)
		{
			u32 addr = cmd & ~(CELL_GCM_METHOD_FLAG_JUMP | CELL_GCM_METHOD_FLAG_NON_INCREMENT);
			//LOG_WARNING(RSX, "rsx jump(0x%x) #addr=0x%x, cmd=0x%x, get=0x%x, put=0x%x", addr, m_ioAddress + get, cmd, get, put);
			m_ctrl->get = addr;
			continue;
		}
		if(cmd & CELL_GCM_METHOD_FLAG_CALL)
		{
			m_call_stack.push(get + 4);
			u32 offs = cmd & ~CELL_GCM_METHOD_FLAG_CALL;
			//u32 addr = Memory.RSXIOMem.GetStartAddr() + offs;
			//LOG_WARNING(RSX, "rsx call(0x%x) #0x%x - 0x%x - 0x%x", offs, addr, cmd, get);
			m_ctrl->get = offs;
			continue;
		}
		if(cmd == CELL_GCM_METHOD_FLAG_RETURN)
		{
			//LOG_WARNING(RSX, "rsx return!");
			u32 get = m_call_stack.top();
			m_call_stack.pop();
			//LOG_WARNING(RSX, "rsx return(0x%x)", get);
			m_ctrl->get = get;
			continue;
		}
		if(cmd & CELL_GCM_METHOD_FLAG_NON_INCREMENT)
		{
			//LOG_WARNING(RSX, "non increment cmd! 0x%x", cmd);
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

		mem32_ptr_t args((u32)Memory.RSXIOMem.RealAddr(Memory.RSXIOMem.GetStartAddr() + get + 4));

		for(u32 i=0; i<count; i++)
		{
			methodRegisters[(cmd & 0xffff) + (i*4*inc)] = ARGS(i);
		}

		DoCmd(cmd, cmd & 0x3ffff, args.GetAddr(), count);

		m_ctrl->get = get + (count + 1) * 4;
		//memset(Memory.GetMemFromAddr(p.m_ioAddress + get), 0, (count + 1) * 4);
	}

	while (!is_vblank_stopped)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	LOG_NOTICE(RSX, "RSX thread ended");

	OnExitThread();
}

void RSXThread::Init(const u32 ioAddress, const u32 ioSize, const u32 ctrlAddress, const u32 localAddress)
{
	m_ctrl = (CellGcmControl*)&Memory[ctrlAddress];
	m_ioAddress = ioAddress;
	m_ioSize = ioSize;
	m_ctrlAddress = ctrlAddress;
	m_local_mem_addr = localAddress;

	m_cur_vertex_prog = nullptr;
	m_cur_shader_prog = nullptr;
	m_cur_shader_prog_num = 0;

	m_used_gcm_commands.clear();

	OnInit();
	ThreadBase::Start();
}

u32 RSXThread::ReadIO32(u32 addr)
{
	u32 value;
	Memory.RSXIOMem.Read32(Memory.RSXIOMem.GetStartAddr() + addr, &value);
	return value;
}

void RSXThread::WriteIO32(u32 addr, u32 value)
{
	Memory.RSXIOMem.Write32(Memory.RSXIOMem.GetStartAddr() + addr, value);
}