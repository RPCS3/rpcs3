#include "stdafx.h"
#include "rpcs3/Ini.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/RSX/GSManager.h"
#include "Emu/RSX/RSXDMA.h"
#include "RSXThread.h"

#include "Emu/SysCalls/Callback.h"
#include "Emu/SysCalls/CB_FUNC.h"
#include "Emu/SysCalls/lv2/sys_time.h"

extern "C"
{
#include "libswscale/swscale.h"
}

//#define ARGS(x) (x >= count ? OutOfArgsCount(x, cmd, count, args.addr()) : args[x].value())
#define CMD_DEBUG 0

u32 methodRegisters[0xffff];

u32 LinearToSwizzleAddress(u32 x, u32 y, u32 z, u32 log2_width, u32 log2_height, u32 log2_depth)
{
	u32 offset = 0;
	u32 shift_count = 0;
	while (log2_width | log2_height | log2_depth)
	{
		if (log2_width)
		{
			offset |= (x & 0x01) << shift_count;
			x >>= 1;
			++shift_count;
			--log2_width;
		}

		if (log2_height)
		{
			offset |= (y & 0x01) << shift_count;
			y >>= 1;
			++shift_count;
			--log2_height;
		}

		if (log2_depth)
		{
			offset |= (z & 0x01) << shift_count;
			z >>= 1;
			++shift_count;
			--log2_depth;
		}
	}
	return offset;
}


u32 GetAddress(u32 offset, u32 location)
{
	u32 res = 0;

	switch (location)
	{
	case CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER:
	case CELL_GCM_LOCATION_LOCAL:
	{
		res = (u32)Memory.RSXFBMem.GetStartAddr() + offset;
		break;
	}

	case CELL_GCM_CONTEXT_DMA_MEMORY_HOST_BUFFER:
	case CELL_GCM_LOCATION_MAIN:
	{
		res = (u32)Memory.RSXIOMem.RealAddr(offset); // TODO: Error Check?
		if (res == 0)
		{
			throw fmt::format("GetAddress(offset=0x%x, location=0x%x): RSXIO memory not mapped", offset, location);
		}

		if (Emu.GetGSManager().GetRender().m_strict_ordering[offset >> 20])
		{
			_mm_mfence(); // probably doesn't have any effect on current implementation
		}

		break;
	}
	default:
	{
		throw fmt::format("GetAddress(offset=0x%x, location=0x%x): invalid location", offset, location);
	}
	}

	return res;
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

void RSXVertexData::Load(u32 start, u32 count, u32 baseOffset, u32 baseIndex = 0)
{
	if (!addr) return;

	const u32 tsize = GetTypeSize();

	data.resize((start + count) * tsize * size);

	for (u32 i = start; i < start + count; ++i)
	{
		auto src = vm::get_ptr<const u8>(addr + baseOffset + stride * (i + baseIndex));
		u8* dst = &data[i * tsize * size];

		switch (tsize)
		{
		case 1:
		{
			memcpy(dst, src, size);
			break;
		}

		case 2:
		{
			const u16* c_src = (const u16*)src;
			u16* c_dst = (u16*)dst;
			for (u32 j = 0; j < size; ++j) *c_dst++ = re16(*c_src++);
			break;
		}

		case 4:
		{
			const u32* c_src = (const u32*)src;
			u32* c_dst = (u32*)dst;
			for (u32 j = 0; j < size; ++j) *c_dst++ = re32(*c_src++);
			break;
		}
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

#define case_2(offset, step) \
    case offset: \
    case ((offset) + (step))
#define case_3(offset, step) \
    case_2(offset, step): \
    case (offset) + (step) * 2
#define case_4(offset, step) \
    case_2(offset, step): \
    case_2(offset + 2*step, step)
#define case_8(offset, step) \
    case_4(offset, step): \
    case_4(offset + 4*step, step)
#define case_16(offset, step) \
    case_8(offset, step): \
    case_8(offset + 8*step, step)
#define case_32(offset, step) \
    case_16(offset, step): \
    case_16(offset + 16*step, step)
#define case_64(offset, step) \
    case_32(offset, step): \
    case_32(offset + 32*step, step)
#define case_128(offset, step) \
    case_64(offset, step): \
    case_64(offset + 64*step, step)
#define case_256(offset, step) \
    case_128(offset, step): \
    case_128(offset + 128*step, step)
#define case_512(offset, step) \
    case_256(offset, step): \
    case_256(offset + 256*step, step)
#define case_range(n, offset, step) \
    case_##n(offset, step):\
    index = (reg - offset) / step

void RSXThread::update_reg(u32 reg, u32 value)
{
#if	CMD_DEBUG
	std::string debug = GetMethodName(cmd);
	debug += "(";
	for (u32 i = 0; i < count; ++i) debug += (i ? ", " : "") + fmt::Format("0x%x", ARGS(i));
	debug += ")";
	LOG_NOTICE(RSX, debug);
#endif

	u32 index = 0;

	//m_used_gcm_commands.insert(reg);

	switch (reg)
	{
	// NV406E
	case NV406E_SET_REFERENCE:
	{
		m_ctrl->ref.exchange(be_t<u32>::make(value));
		break;
	}

	case NV406E_SET_CONTEXT_DMA_SEMAPHORE:
	{
		if (value)
		{
			LOG_WARNING(RSX, "TODO: NV406E_SET_CONTEXT_DMA_SEMAPHORE: 0x%x", value);
		}
		break;
	}

	case NV4097_SET_SEMAPHORE_OFFSET:
	case NV406E_SEMAPHORE_OFFSET:
	{
		m_set_semaphore_offset = true;
		m_semaphore_offset = value;
		break;
	}

	case NV406E_SEMAPHORE_ACQUIRE:
	{
		LOG_WARNING(RSX, "NV406E_SEMAPHORE_ACQUIRE: 0x%x", value);
		while (!Emu.IsStopped() && vm::read32(m_label_addr + m_semaphore_offset) != value)
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		break;
	}

	case NV406E_SEMAPHORE_RELEASE:
	case NV4097_TEXTURE_READ_SEMAPHORE_RELEASE:
	{
		vm::write32(m_label_addr + m_semaphore_offset, value);
		break;
	}

	case NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE:
	{
		if (m_set_semaphore_offset)
		{
			m_set_semaphore_offset = false;
			vm::write32(m_label_addr + m_semaphore_offset, (value & 0xff00ff00) | ((value & 0xff) << 16) | ((value >> 16) & 0xff));
		}
		break;
	}

	// NV4097
	case GCM_FLIP_COMMAND:
	{
		m_gcm_current_buffer = value;
		Flip(value);

		m_last_flip_time = get_system_time();
		m_gcm_current_buffer = value;
		m_read_buffer = true;
		m_flip_status = 0;

		if (m_flip_handler)
		{
			auto cb = m_flip_handler;
			Emu.GetCallbackManager().Async([cb](PPUThread& CPU)
			{
				cb(CPU, 1);
			});
		}

		auto sync = [&]()
		{
			double limit;
			switch (Ini.GSFrameLimit.GetValue())
			{
			case 1: limit = 50.; break;
			case 2: limit = 59.94; break;
			case 3: limit = 30.; break;
			case 4: limit = 60.; break;
			case 5: limit = m_fps_limit; break; //TODO

			case 0:
			default:
				return;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds((s64)(1000.0 / limit - m_timer_sync.GetElapsedTimeInMilliSec())));
			m_timer_sync.Start();
		};

		sync();

		//Emu.Pause();
		break;
	}

	case NV4097_NO_OPERATION:
	{
		// Nothing to do here 
		break;
	}

	case NV4097_SET_CONTEXT_DMA_REPORT:
	{
		if (value)
		{
			LOG_WARNING(RSX, "TODO: NV4097_SET_CONTEXT_DMA_REPORT: 0x%x", value);
			dma_report = value;
		}
		break;
	}

	case NV4097_NOTIFY:
	{
		if (value)
		{
			LOG_WARNING(RSX, "TODO: NV4097_NOTIFY: 0x%x", value);
		}
		break;
	}

	case NV4097_WAIT_FOR_IDLE:
	{
		if (value)
		{
			LOG_WARNING(RSX, "TODO: NV4097_WAIT_FOR_IDLE: 0x%x", value);
		}
		break;
	}

	case NV4097_PM_TRIGGER:
	{
		if (value)
		{
			LOG_WARNING(RSX, "TODO: NV4097_PM_TRIGGER: 0x%x", value);
		}
		break;
	}

	// Texture
	case_range(16, NV4097_SET_TEXTURE_FORMAT, 0x20);
	case_range(16, NV4097_SET_TEXTURE_OFFSET, 0x20);
	case_range(16, NV4097_SET_TEXTURE_FILTER, 0x20);
	case_range(16, NV4097_SET_TEXTURE_ADDRESS, 0x20);
	case_range(16, NV4097_SET_TEXTURE_IMAGE_RECT, 32);
	case_range(16, NV4097_SET_TEXTURE_BORDER_COLOR, 0x20);
	case_range(16, NV4097_SET_TEXTURE_CONTROL0, 0x20);
	case_range(16, NV4097_SET_TEXTURE_CONTROL1, 0x20);
	case_range(16, NV4097_SET_TEXTURE_CONTROL3, 4);
	{
		// Done using methodRegisters in RSXTexture.cpp
		break;
	}

	case_range(16, NV4097_SET_TEX_COORD_CONTROL, 4);
	{
		LOG_WARNING(RSX, "TODO: NV4097_SET_TEX_COORD_CONTROL");
		break;
	}

		// Vertex Texture
	case_range(4, NV4097_SET_VERTEX_TEXTURE_FORMAT, 0x20);
	case_range(4, NV4097_SET_VERTEX_TEXTURE_OFFSET, 0x20);
	case_range(4, NV4097_SET_VERTEX_TEXTURE_FILTER, 0x20);
	case_range(4, NV4097_SET_VERTEX_TEXTURE_ADDRESS, 0x20);
	case_range(4, NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT, 0x20);
	case_range(4, NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR, 0x20);
	case_range(4, NV4097_SET_VERTEX_TEXTURE_CONTROL0, 0x20);
	case_range(4, NV4097_SET_VERTEX_TEXTURE_CONTROL3, 0x20);
	{
		// Done using methodRegisters in RSXTexture.cpp
		break;
	}

		// Vertex data
	case_range(16, NV4097_SET_VERTEX_DATA4UB_M, 4);
	{
		int row = index;

		if (m_vertex_data[row].type != CELL_GCM_VERTEX_UB || m_vertex_data[row].size != 4)
		{
			m_vertex_data[row].Reset();
			m_vertex_data[row].type = CELL_GCM_VERTEX_UB;
			m_vertex_data[row].size = 4;
		}

		u32 pos = m_vertex_data[row].data.size();
		m_vertex_data[row].data.resize(pos + sizeof(u8) * 4);
		(u32&)m_vertex_data[row].data[pos] = value;

		//LOG_WARNING(RSX, "NV4097_SET_VERTEX_DATA4UB_M: index = %d, v0 = 0x%x, v1 = 0x%x, v2 = 0x%x, v3 = 0x%x", index, v0, v1, v2, v3);
		break;
	}

	case_range(32, NV4097_SET_VERTEX_DATA2F_M, 4);
	{
		int row = index / 2;
		int col = index % 2;

		if (m_vertex_data[row].type != CELL_GCM_VERTEX_F || m_vertex_data[row].size != 2)
		{
			m_vertex_data[row].Reset();
			m_vertex_data[row].type = CELL_GCM_VERTEX_F;
			m_vertex_data[row].size = 2;
		}

		u32 pos = m_vertex_data[row].data.size();
		m_vertex_data[row].data.resize(pos + sizeof(f32) * col);
		(u32&)m_vertex_data[row].data[pos] = value;

		//LOG_WARNING(RSX, "NV4097_SET_VERTEX_DATA2F_M: index = %d, v0 = %f, v1 = %f", index, v0, v1);
		break;
	}

	case_range(64, NV4097_SET_VERTEX_DATA4F_M, 4);
	{
		int row = index / 4;
		int col = index % 4;

		if (m_vertex_data[row].type != CELL_GCM_VERTEX_F || m_vertex_data[row].size != 4)
		{
			m_vertex_data[row].Reset();
			m_vertex_data[row].type = CELL_GCM_VERTEX_F;
			m_vertex_data[row].size = 4;
		}

		u32 pos = m_vertex_data[row].data.size();
		m_vertex_data[row].data.resize(pos + sizeof(f32));
		(u32&)m_vertex_data[row].data[pos] = value;

		//LOG_WARNING(RSX, "NV4097_SET_VERTEX_DATA4F_M: index = %d, v0 = %f, v1 = %f, v2 = %f, v3 = %f", index, v0, v1, v2, v3);
		break;
	}

	case_range(16, NV4097_SET_VERTEX_DATA_ARRAY_OFFSET, 4);
	{
		m_vertex_data[index].addr = GetAddress(value & 0x7fffffff, value >> 31);
		//m_vertex_data[index].data.clear();
		break;
	}

	case_range(32, NV4097_SET_VERTEX_DATA_ARRAY_FORMAT, 4);
	{
		m_vertex_data[index].frequency = value >> 16;
		m_vertex_data[index].stride = (value >> 8) & 0xff;
		m_vertex_data[index].size = (value >> 4) & 0xf;
		m_vertex_data[index].type = value & 0xf;

		//LOG_WARNING(RSX, "NV4097_SET_VERTEX_DATA_ARRAY_FORMAT: index=%d, frequency=%d, stride=%d, size=%d, type=%d", index, frequency, stride, size, type);

		break;
	}

	// Vertex Attribute
	case NV4097_SET_VERTEX_ATTRIB_INPUT_MASK:
	{
		if (u32 mask = value)
		{
			LOG_WARNING(RSX, "TODO: NV4097_SET_VERTEX_ATTRIB_INPUT_MASK: 0x%x", mask);
		}

		//VertexData[0].prog.attributeInputMask = value;
		break;
	}

	case NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK:
	{
		if (u32 mask = value)
		{
			LOG_WARNING(RSX, "TODO: NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK: 0x%x", mask);
		}

		//VertexData[0].prog.attributeOutputMask = value;
		//FragmentData.prog.attributeInputMask = value/* & ~0x20*/;
		break;
	}

	// Color Mask
	case NV4097_SET_COLOR_MASK:
	{
		m_set_color_mask = true;
		m_color_mask_a = value & 0x01000000 ? true : false;
		m_color_mask_r = value & 0x00010000 ? true : false;
		m_color_mask_g = value & 0x00000100 ? true : false;
		m_color_mask_b = value & 0x00000001 ? true : false;
		break;
	}

	case NV4097_SET_COLOR_MASK_MRT:
	{
		if (u32 mask = value)
		{
			LOG_WARNING(RSX, "TODO: NV4097_SET_COLOR_MASK_MRT: 0x%x", mask);
		}
		break;
	}

	// Alpha testing
	case NV4097_SET_ALPHA_TEST_ENABLE:
	{
		m_set_alpha_test = value ? true : false;
		break;
	}

	case NV4097_SET_ALPHA_FUNC:
	{
		m_set_alpha_func = true;
		m_alpha_func = value;
		break;
	}

	case NV4097_SET_ALPHA_REF:
	{
		m_set_alpha_ref = true;
		const u32 a0 = value;
		m_alpha_ref = (float&)a0;
		break;
	}

	// Cull face
	case NV4097_SET_CULL_FACE_ENABLE:
	{
		m_set_cull_face = value ? true : false;
		break;
	}

	case NV4097_SET_CULL_FACE:
	{
		m_cull_face = value;
		break;
	}

	// Front face 
	case NV4097_SET_FRONT_FACE:
	{
		m_front_face = (value == 0x0900) || (value == 0x0901) ? value : 0x0900/*default GL_CW*/;
		break;
	}

	// Blending
	case NV4097_SET_BLEND_ENABLE:
	{
		m_set_blend = value ? true : false;
		break;
	}

	case NV4097_SET_BLEND_ENABLE_MRT:
	{
		m_set_blend_mrt1 = value & 0x02 ? true : false;
		m_set_blend_mrt2 = value & 0x04 ? true : false;
		m_set_blend_mrt3 = value & 0x08 ? true : false;
		break;
	}

	case NV4097_SET_BLEND_FUNC_SFACTOR:
	{
		m_set_blend_sfactor = true;
		m_blend_sfactor_rgb = value & 0xffff;
		m_blend_sfactor_alpha = value >> 16;
		break;
	}

	case NV4097_SET_BLEND_FUNC_DFACTOR:
	{
		m_set_blend_dfactor = true;
		m_blend_dfactor_rgb = value & 0xffff;
		m_blend_dfactor_alpha = value >> 16;
		break;
	}

	case NV4097_SET_BLEND_COLOR:
	{
		m_set_blend_color = true;
		m_blend_color_r = value & 0xff;
		m_blend_color_g = (value >> 8) & 0xff;
		m_blend_color_b = (value >> 16) & 0xff;
		m_blend_color_a = (value >> 24) & 0xff;
		break;
	}

	case NV4097_SET_BLEND_COLOR2:
	{
		if (value)
		{
			LOG_WARNING(RSX, "TODO : NV4097_SET_BLEND_COLOR2: 0x%x", value);
		}
		break;
	}

	case NV4097_SET_BLEND_EQUATION:
	{
		m_set_blend_equation = true;
		m_blend_equation_rgb = value & 0xffff;
		m_blend_equation_alpha = value >> 16;
		break;
	}

	case NV4097_SET_REDUCE_DST_COLOR:
	{
		if (value)
		{
			LOG_WARNING(RSX, "TODO: NV4097_SET_REDUCE_DST_COLOR: 0x%x", value);
		}
		break;
	}

	// Depth bound testing
	case NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE:
	{
		m_set_depth_bounds_test = value ? true : false;
		break;
	}

	case NV4097_SET_DEPTH_BOUNDS_MIN:
	{
		m_set_depth_bounds = true;
		const u32 a0 = value;
		m_depth_bounds_min = (float&)a0;
		break;
	}

	case NV4097_SET_DEPTH_BOUNDS_MAX:
	{
		m_set_depth_bounds = true;
		const u32 a0 = value;
		m_depth_bounds_max = (float&)a0;
		break;
	}

	// Viewport
	case NV4097_SET_VIEWPORT_HORIZONTAL:
	{
		m_set_viewport_horizontal = true;
		m_viewport_x = value & 0xffff;
		m_viewport_w = value >> 16;

		//LOG_NOTICE(RSX, "NV4097_SET_VIEWPORT_HORIZONTAL: x=%d, y=%d, w=%d, h=%d", m_viewport_x, m_viewport_y, m_viewport_w, m_viewport_h);
		break;
	}

	case NV4097_SET_VIEWPORT_VERTICAL:
	{
		m_set_viewport_vertical = true;
		m_viewport_y = value & 0xffff;
		m_viewport_h = value >> 16;

		//LOG_NOTICE(RSX, "NV4097_SET_VIEWPORT_VERTICAL: y=%d, h=%d", m_viewport_y, m_viewport_h);
		break;
	}

	case_4(NV4097_SET_VIEWPORT_SCALE, 4):
	case_4(NV4097_SET_VIEWPORT_OFFSET, 4):
	{
		// Done in Vertex Shader
		break;
	}

	// Clipping
	case NV4097_SET_CLIP_MIN:
	{
		m_set_clip = true;
		m_clip_min = (float&)value;

		//LOG_NOTICE(RSX, "NV4097_SET_CLIP_MIN: clip_min=%.01f, clip_max=%.01f", m_clip_min, m_clip_max);
		break;
	}

	case NV4097_SET_CLIP_MAX:
	{
		m_set_clip = true;
		m_clip_max = (float&)value;

		//LOG_NOTICE(RSX, "NV4097_SET_CLIP_MAX: clip_max=%.01f", m_clip_max);
		break;
	}

	// Depth testing
	case NV4097_SET_DEPTH_TEST_ENABLE:
	{
		m_set_depth_test = value ? true : false;
		break;
	}

	case NV4097_SET_DEPTH_FUNC:
	{
		m_set_depth_func = true;
		m_depth_func = value;
		break;
	}

	case NV4097_SET_DEPTH_MASK:
	{
		m_set_depth_mask = true;
		m_depth_mask = value;
		break;
	}

	// Polygon mode/offset
	case NV4097_SET_FRONT_POLYGON_MODE:
	{
		m_set_front_polygon_mode = true;
		m_front_polygon_mode = value;
		break;
	}

	case NV4097_SET_BACK_POLYGON_MODE:
	{
		m_set_back_polygon_mode = true;
		m_back_polygon_mode = value;
		break;
	}

	case NV4097_SET_POLY_OFFSET_FILL_ENABLE:
	{
		m_set_poly_offset_fill = value ? true : false;
		break;
	}

	case NV4097_SET_POLY_OFFSET_LINE_ENABLE:
	{
		m_set_poly_offset_line = value ? true : false;
		break;
	}

	case NV4097_SET_POLY_OFFSET_POINT_ENABLE:
	{
		m_set_poly_offset_point = value ? true : false;
		break;
	}

	case NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR:
	{
		m_set_depth_test = true;
		m_set_poly_offset_mode = true;

		const u32 a0 = value;
		m_poly_offset_scale_factor = (float&)a0;
		break;
	}

	case NV4097_SET_POLYGON_OFFSET_BIAS:
	{
		m_set_depth_test = true;
		m_set_poly_offset_mode = true;

		const u32 a0 = value;
		m_poly_offset_bias = (float&)a0;
		break;
	}

	case NV4097_SET_CYLINDRICAL_WRAP:
	{
		if (value)
		{
			LOG_WARNING(RSX, "TODO: NV4097_SET_CYLINDRICAL_WRAP: 0x%x", value);
		}
		break;
	}

	// Clearing
	case NV4097_CLEAR_ZCULL_SURFACE:
	{
		u32 a0 = value;

		if (a0 & 0x01) m_clear_surface_z = m_clear_z;
		if (a0 & 0x02) m_clear_surface_s = m_clear_s;

		m_clear_surface_mask |= a0 & 0x3;
		break;
	}

	case NV4097_CLEAR_SURFACE:
	{
		const u32 a0 = value;

		if (a0 & 0x01) m_clear_surface_z = m_clear_z;
		if (a0 & 0x02) m_clear_surface_s = m_clear_s;
		if (a0 & 0x10) m_clear_surface_color_r = m_clear_color_r;
		if (a0 & 0x20) m_clear_surface_color_g = m_clear_color_g;
		if (a0 & 0x40) m_clear_surface_color_b = m_clear_color_b;
		if (a0 & 0x80) m_clear_surface_color_a = m_clear_color_a;

		m_clear_surface_mask = a0;
		ExecCMD(NV4097_CLEAR_SURFACE);
		break;
	}

	case NV4097_SET_ZSTENCIL_CLEAR_VALUE:
	{
		m_clear_s = value & 0xff;
		m_clear_z = value >> 8;
		break;
	}

	case NV4097_SET_COLOR_CLEAR_VALUE:
	{
		const u32 color = value;
		m_clear_color_a = (color >> 24) & 0xff;
		m_clear_color_r = (color >> 16) & 0xff;
		m_clear_color_g = (color >> 8) & 0xff;
		m_clear_color_b = color & 0xff;
		break;
	}

	case NV4097_SET_CLEAR_RECT_HORIZONTAL:
	{
		if (value)
		{
			LOG_WARNING(RSX, "TODO: NV4097_SET_CLEAR_RECT_HORIZONTAL: 0x%x", value);
		}
		break;
	}

	case NV4097_SET_CLEAR_RECT_VERTICAL:
	{
		if (value)
		{
			LOG_WARNING(RSX, "TODO: NV4097_SET_CLEAR_RECT_VERTICAL: 0x%x", value);
		}
		break;
	}

	// Arrays
	case NV4097_INLINE_ARRAY:
	{
		LOG_ERROR(RSX, "TODO: NV4097_INLINE_ARRAY: 0x%x", value);
		break;
	}

	case NV4097_DRAW_ARRAYS:
	{
		const u32 first = value & 0xffffff;
		const u32 _count = (value >> 24) + 1;

		//LOG_WARNING(RSX, "NV4097_DRAW_ARRAYS: %d - %d", first, _count);

		LoadVertexData(first, _count);

		if (first < m_draw_array_first)
		{
			m_draw_array_first = first;
		}

		m_draw_array_count += _count;
		break;
	}

	case NV4097_SET_INDEX_ARRAY_ADDRESS:
	{
		m_indexed_array.m_addr = GetAddress(value, methodRegisters[NV4097_SET_INDEX_ARRAY_ADDRESS] & 0xf);
		break;
	}

	case NV4097_SET_INDEX_ARRAY_DMA:
	{
		m_indexed_array.m_addr = GetAddress(methodRegisters[NV4097_SET_INDEX_ARRAY_ADDRESS], value & 0xf);
		m_indexed_array.m_type = value >> 4;
		break;
	}

	case NV4097_DRAW_INDEX_ARRAY:
	{
		const u32 first = value & 0xffffff;
		const u32 _count = (value >> 24) + 1;

		if (first < m_indexed_array.m_first) m_indexed_array.m_first = first;

		for (u32 i=first; i<_count; ++i)
		{
			u32 index;
			switch(m_indexed_array.m_type)
			{
			case 0:
			{
				int pos = (int)m_indexed_array.m_data.size();
				m_indexed_array.m_data.resize(m_indexed_array.m_data.size() + 4);
				index = vm::read32(m_indexed_array.m_addr + i * 4);
				*(u32*)&m_indexed_array.m_data[pos] = index;
				//LOG_WARNING(RSX, "index 4: %d", *(u32*)&m_indexed_array.m_data[pos]);
			}
				break;

			case 1:
			{
				int pos = (int)m_indexed_array.m_data.size();
				m_indexed_array.m_data.resize(m_indexed_array.m_data.size() + 2);
				index = vm::read16(m_indexed_array.m_addr + i * 2);
				//LOG_WARNING(RSX, "index 2: %d", index);
				*(u16*)&m_indexed_array.m_data[pos] = index;
			}
				break;
			}

			if (index < m_indexed_array.index_min) m_indexed_array.index_min = index;
			if (index > m_indexed_array.index_max) m_indexed_array.index_max = index;
		}

		m_indexed_array.m_count += _count;
		break;
	}

	case NV4097_SET_VERTEX_DATA_BASE_OFFSET:
	{
		m_vertex_data_base_offset = value;

		//LOG_WARNING(RSX, "NV4097_SET_VERTEX_DATA_BASE_OFFSET: 0x%x", m_vertex_data_base_offset);
		break;
	}

	case NV4097_SET_VERTEX_DATA_BASE_INDEX:
	{
		m_vertex_data_base_index = value;
		break;
	}

	case NV4097_SET_BEGIN_END:
	{
		const u32 a0 = value;

		//LOG_WARNING(RSX, "NV4097_SET_BEGIN_END: 0x%x", a0);

		if (!m_indexed_array.m_count && !m_draw_array_count)
		{
			u32 min_vertex_size = ~0;
			for (auto &i : m_vertex_data)
			{
				if (!i.size)
					continue;

				u32 vertex_size = i.data.size() / (i.size * i.GetTypeSize());

				if (min_vertex_size > vertex_size)
					min_vertex_size = vertex_size;
			}

			m_draw_array_count = min_vertex_size;
			m_draw_array_first = 0;
		}

		if (a0)
		{
			Begin(a0);
		}
		else
		{
			End();
		}
		break;
	}

	// Shader
	case NV4097_SET_SHADER_PROGRAM:
	{
		m_cur_fragment_prog = &m_fragment_progs[m_cur_fragment_prog_num];

		const u32 a0 = value;
		m_cur_fragment_prog->offset = value & ~0x3;
		m_cur_fragment_prog->addr = GetAddress(m_cur_fragment_prog->offset, (value & 0x3) - 1);
		m_cur_fragment_prog->ctrl = 0x40;
		break;
	}

	case NV4097_SET_SHADER_CONTROL:
	{
		m_shader_ctrl = value;
		break;
	}

	case NV4097_SET_SHADE_MODE:
	{
		m_set_shade_mode = true;
		m_shade_mode = value;
		break;
	}

	case NV4097_SET_SHADER_PACKER:
	{
		if (value)
		{
			LOG_WARNING(RSX, "TODO: NV4097_SET_SHADER_PACKER: 0x%x", value);
		}
	}

	case NV4097_SET_SHADER_WINDOW:
	{
		const u32 a0 = value;
		m_shader_window_height = a0 & 0xfff;
		m_shader_window_origin = (a0 >> 12) & 0xf;
		m_shader_window_pixel_centers = a0 >> 16;
		break;
	}

	// Transform 
	case NV4097_SET_TRANSFORM_PROGRAM_LOAD:
	{
		//LOG_WARNING(RSX, "NV4097_SET_TRANSFORM_PROGRAM_LOAD: prog = %d", value);

		//m_cur_vertex_prog = &m_vertex_progs[value];
		//m_cur_vertex_prog->data.clear();

		break;
	}

	case NV4097_SET_TRANSFORM_PROGRAM_START:
	{
		break;
	}

	case_range(512, NV4097_SET_TRANSFORM_PROGRAM, 4);
	{
		//LOG_WARNING(RSX, "NV4097_SET_TRANSFORM_PROGRAM[%d](%d)", index, count);

		m_vertex_program_data[methodRegisters[NV4097_SET_TRANSFORM_PROGRAM_LOAD] * 4 + index % 4] = value;
		if (index % 4 == 3)
		{
			methodRegisters[NV4097_SET_TRANSFORM_PROGRAM_LOAD]++;
		}
		break;
	}

	case NV4097_SET_TRANSFORM_TIMEOUT:
	{
		// TODO:
		// (cmd)[1] = CELL_GCM_ENDIAN_SWAP((count) | ((registerCount) << 16)); \
		//m_cur_vertex_prog->Decompile();
		break;
	}

	case NV4097_SET_TRANSFORM_BRANCH_BITS:
	{
		if (value)
		{
			LOG_WARNING(RSX, "TODO: NV4097_SET_TRANSFORM_BRANCH_BITS: 0x%x", value);
		}
		break;
	}

	case NV4097_SET_TRANSFORM_CONSTANT_LOAD:
	{
		/*
		for (u32 id = value, i = 1; i<count; ++id)
		{
			const u32 x = ARGS(i); i++;
			const u32 y = ARGS(i); i++;
			const u32 z = ARGS(i); i++;
			const u32 w = ARGS(i); i++;

			RSXTransformConstant c(id, (float&)x, (float&)y, (float&)z, (float&)w);

			m_transform_constants.push_back(c);

			//LOG_NOTICE(RSX, "NV4097_SET_TRANSFORM_CONSTANT_LOAD: [%d : %d] = (%f, %f, %f, %f)", i, id, c.x, c.y, c.z, c.w);
		}
		*/
		break;
	}

	case_range(32, NV4097_SET_TRANSFORM_CONSTANT, 4);
	{
		m_transform_constants[methodRegisters[NV4097_SET_TRANSFORM_CONSTANT_LOAD] + index / 4].xyzw[index % 4] = (f32&)value;
		break;
	}

	// Invalidation
	case NV4097_INVALIDATE_L2:
	{
		if (value)
		{
			LOG_WARNING(RSX, "TODO: NV4097_INVALIDATE_L2: 0x%x", value);
		}
		break;
	}

	case NV4097_INVALIDATE_VERTEX_CACHE_FILE:
	{
		// Nothing to do here
		break;
	}

	case NV4097_INVALIDATE_VERTEX_FILE:
	{
		// Nothing to do here
		break;
	}

	case NV4097_INVALIDATE_ZCULL:
	{
		if (value)
		{
			LOG_WARNING(RSX, "TODO: NV4097_INVALIDATE_ZCULL: 0x%x", value);
		}
		break;
	}

	// Logic Ops
	case NV4097_SET_LOGIC_OP_ENABLE:
	{
		m_set_logic_op = value ? true : false;
		break;
	}

	case NV4097_SET_LOGIC_OP:
	{
		m_logic_op = value;
		break;
	}

	// Dithering
	case NV4097_SET_DITHER_ENABLE:
	{
		m_set_dither = value ? true : false;
		break;
	}

	// Stencil testing
	case NV4097_SET_STENCIL_TEST_ENABLE:
	{
		m_set_stencil_test = value ? true : false;
		break;
	}

	case NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE:
	{
		m_set_two_sided_stencil_test_enable = value ? true : false;
		break;
	}

	case NV4097_SET_TWO_SIDE_LIGHT_EN:
	{
		m_set_two_side_light_enable = value ? true : false;
		break;
	}

	case NV4097_SET_STENCIL_MASK:
	{
		m_set_stencil_mask = true;
		m_stencil_mask = value;
		break;
	}

	case NV4097_SET_STENCIL_FUNC:
	{
		m_set_stencil_func = true;
		m_stencil_func = value;
		break;
	}

	case NV4097_SET_STENCIL_FUNC_REF:
	{
		m_set_stencil_func_ref = true;
		m_stencil_func_ref = value;
		break;
	}

	case NV4097_SET_STENCIL_FUNC_MASK:
	{
		m_set_stencil_func_mask = true;
		m_stencil_func_mask = value;
		break;
	}

	case NV4097_SET_STENCIL_OP_FAIL:
	{
		m_set_stencil_fail = true;
		m_stencil_fail = value;
		break;
	}

	case NV4097_SET_STENCIL_OP_ZFAIL:
	{
		m_set_stencil_zfail = true;
		m_stencil_zfail = value;
		break;
	}

	case NV4097_SET_STENCIL_OP_ZPASS:
	{
		m_set_stencil_zpass = true;
		m_stencil_zpass = value;
		break;
	}

	case NV4097_SET_BACK_STENCIL_MASK:
	{
		m_set_back_stencil_mask = true;
		m_back_stencil_mask = value;
		break;
	}

	case NV4097_SET_BACK_STENCIL_FUNC:
	{
		m_set_back_stencil_func = true;
		m_back_stencil_func = value;
		break;
	}

	case NV4097_SET_BACK_STENCIL_FUNC_REF:
	{
		m_set_back_stencil_func_ref = true;
		m_back_stencil_func_ref = value;
		break;
	}

	case NV4097_SET_BACK_STENCIL_FUNC_MASK:
	{
		m_set_back_stencil_func_mask = true;
		m_back_stencil_func_mask = value;
		break;
	}

	case NV4097_SET_BACK_STENCIL_OP_FAIL:
	{
		m_set_stencil_fail = true;
		m_stencil_fail = value;
		break;
	}

	case NV4097_SET_SCULL_CONTROL:
	{
		if (value)
		{
			LOG_WARNING(RSX, "TODO: NV4097_SET_SCULL_CONTROL: 0x%x", value);
		}
		break;
	}

	// Primitive restart index
	case NV4097_SET_RESTART_INDEX_ENABLE:
	{
		m_set_restart_index = value ? true : false;
		break;
	}

	case NV4097_SET_RESTART_INDEX:
	{
		m_restart_index = value;
		break;
	}

	// Point size
	case NV4097_SET_POINT_SIZE:
	{
		m_set_point_size = true;
		m_point_size = (float&)value;
		break;
	}

	// Point sprite
	case NV4097_SET_POINT_PARAMS_ENABLE:
	{
		if (value)
		{
			LOG_WARNING(RSX, "TODO: NV4097_SET_POINT_PARAMS_ENABLE: 0x%x", value);
		}
		break;
	}

	case NV4097_SET_POINT_SPRITE_CONTROL:
	{
		m_set_point_sprite_control = value ? true : false;

		// TODO:
		//(cmd)[1] = CELL_GCM_ENDIAN_SWAP((enable) | ((rmode) << 1) | (texcoordMask));
		break;
	}

	// Lighting
	case NV4097_SET_SPECULAR_ENABLE:
	{
		m_set_specular = value ? true : false;
		break;
	}

	// Scissor
	case NV4097_SET_SCISSOR_HORIZONTAL:
	{
		m_set_scissor_horizontal = true;
		m_scissor_x = value & 0xffff;
		m_scissor_w = value >> 16;
		break;
	}

	case NV4097_SET_SCISSOR_VERTICAL:
	{
		m_set_scissor_vertical = true;
		m_scissor_y = value & 0xffff;
		m_scissor_h = value >> 16;
		break;
	}

	// Depth/Color buffer usage 
	case NV4097_SET_SURFACE_FORMAT:
	{
		const u32 a0 = value;
		m_set_surface_format = true;
		m_surface_color_format = a0 & 0x1f;
		m_surface_depth_format = (a0 >> 5) & 0x7;
		m_surface_type = (a0 >> 8) & 0xf;
		m_surface_antialias = (a0 >> 12) & 0xf;
		m_surface_width = (a0 >> 16) & 0xff;
		m_surface_height = (a0 >> 24) & 0xff;
		break;
	}

	case NV4097_SET_SURFACE_COLOR_TARGET:
	{
		m_surface_color_target = value;
		break;
	}

	case NV4097_SET_SURFACE_COLOR_AOFFSET:
	{
		m_surface_offset[0] = value;
		break;
	}

	case NV4097_SET_SURFACE_COLOR_BOFFSET:
	{
		m_surface_offset[1] = value;
		break;
	}

	case NV4097_SET_SURFACE_COLOR_COFFSET:
	{
		m_surface_offset[2] = value;
		break;
	}

	case NV4097_SET_SURFACE_COLOR_DOFFSET:
	{
		m_surface_offset[3] = value;
		break;
	}

	case NV4097_SET_SURFACE_ZETA_OFFSET:
	{
		m_surface_offset_z = value;
		break;
	}

	case NV4097_SET_SURFACE_PITCH_A:
	{
		m_surface_pitch[0] = value;
		break;
	}

	case NV4097_SET_SURFACE_PITCH_B:
	{
		m_surface_pitch[1] = value;
		break;
	}

	case NV4097_SET_SURFACE_PITCH_C:
	{
		m_surface_pitch[2] = value;
		break;
	}

	case NV4097_SET_SURFACE_PITCH_D:
	{
		m_surface_pitch[3] = value;
		break;
	}

	case NV4097_SET_SURFACE_PITCH_Z:
	{
		m_surface_pitch_z = value;
		break;
	}

	case NV4097_SET_CONTEXT_DMA_COLOR_A:
	{
		m_context_dma_color[0] = value;
		break;
	}

	case NV4097_SET_CONTEXT_DMA_COLOR_B:
	{
		m_context_dma_color[1] = value;
		break;
	}

	case NV4097_SET_CONTEXT_DMA_COLOR_C:
	{
		m_context_dma_color[2] = value;
		break;
	}

	case NV4097_SET_CONTEXT_DMA_COLOR_D:
	{
		m_context_dma_color[3] = value;
		break;
	}

	case NV4097_SET_CONTEXT_DMA_ZETA:
	{
		m_context_dma_z = value;
		break;
	}

	case NV4097_SET_CONTEXT_DMA_SEMAPHORE:
	{
		if (value)
		{
			LOG_WARNING(RSX, "TODO: NV4097_SET_CONTEXT_DMA_SEMAPHORE: 0x%x", value);
		}
		break;
	}

	case NV4097_SET_CONTEXT_DMA_NOTIFIES:
	{
		if (value)
		{
			LOG_WARNING(RSX, "TODO: NV4097_SET_CONTEXT_DMA_NOTIFIES: 0x%x", value);
		}
		break;
	}

	case NV4097_SET_SURFACE_CLIP_HORIZONTAL:
	{
		const u32 a0 = value;

		m_set_surface_clip_horizontal = true;
		m_surface_clip_x = a0;
		m_surface_clip_w = a0 >> 16;
		break;
	}

	case NV4097_SET_SURFACE_CLIP_VERTICAL:
	{
		const u32 a0 = value;
		m_set_surface_clip_vertical = true;
		m_surface_clip_y = a0;
		m_surface_clip_h = a0 >> 16;
		break;
	}

	// Anti-aliasing
	case NV4097_SET_ANTI_ALIASING_CONTROL:
	{
		const u32 a0 = value;

		const u8 enable = a0 & 0xf;
		const u8 alphaToCoverage = (a0 >> 4) & 0xf;
		const u8 alphaToOne = (a0 >> 8) & 0xf;
		const u16 sampleMask = a0 >> 16;

		if (a0)
		{
			LOG_WARNING(RSX, "TODO: NV4097_SET_ANTI_ALIASING_CONTROL: 0x%x", a0);
		}
		break;
	}

	// Line/Polygon smoothing
	case NV4097_SET_LINE_SMOOTH_ENABLE:
	{
		m_set_line_smooth = value ? true : false;
		break;
	}

	case NV4097_SET_POLY_SMOOTH_ENABLE:
	{
		m_set_poly_smooth = value ? true : false;
		break;
	}

	// Line width
	case NV4097_SET_LINE_WIDTH:
	{
		m_set_line_width = true;
		const u32 a0 = value;
		m_line_width = (float)a0 / 8.0f;
		break;
	}

	// Line/Polygon stipple
	case NV4097_SET_LINE_STIPPLE:
	{
		m_set_line_stipple = value ? true : false;
		break;
	}

	case NV4097_SET_LINE_STIPPLE_PATTERN:
	{
		m_set_line_stipple = true;
		const u32 a0 = value;
		m_line_stipple_factor = a0 & 0xffff;
		m_line_stipple_pattern = a0 >> 16;
		break;
	}

	case NV4097_SET_POLYGON_STIPPLE:
	{
		m_set_polygon_stipple = value ? true : false;
		break;
	}

	case_range(32, NV4097_SET_POLYGON_STIPPLE_PATTERN, 4);
	{
		m_polygon_stipple_pattern[index] = value;
		break;
	}

	// Zcull
	case NV4097_SET_ZCULL_EN:
	{
		const u32 a0 = value;

		m_set_depth_test = a0 & 0x1 ? true : false;
		m_set_stencil_test = a0 & 0x2 ? true : false;
		break;
	}

	case NV4097_SET_ZCULL_CONTROL0:
	{
		if (value)
		{
			LOG_WARNING(RSX, "TODO: NV4097_SET_ZCULL_CONTROL0: 0x%x", value);
		}
		break;
	}

	case NV4097_SET_ZCULL_CONTROL1:
	{
		if (value)
		{
			LOG_WARNING(RSX, "TODO: NV4097_SET_ZCULL_CONTROL1: 0x%x", value);
		}
		break;
	}

	case NV4097_SET_ZCULL_STATS_ENABLE:
	{
		if (value)
		{
			LOG_WARNING(RSX, "TODO: NV4097_SET_ZCULL_STATS_ENABLE: 0x%x", value);
		}
		break;
	}

	case NV4097_ZCULL_SYNC:
	{
		if (value)
		{
			LOG_WARNING(RSX, "TODO: NV4097_ZCULL_SYNC: 0x%x", value);
		}
		break;
	}

	// Reports
	case NV4097_GET_REPORT:
	{
		const u32 a0 = value;
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
			LOG_ERROR(RSX, "NV4097_GET_REPORT: Bad type %d", type);
			break;
		}

		// Get timestamp, and convert it from microseconds to nanoseconds
		u64 timestamp = get_system_time() * 1000;

		// NOTE: DMA broken, implement proper lpar mapping (sys_rsx)
		//dma_write64(dma_report, offset + 0x0, timestamp);
		//dma_write32(dma_report, offset + 0x8, value);
		//dma_write32(dma_report, offset + 0xc, 0);

		vm::write64(m_local_mem_addr + offset + 0x0, timestamp);
		vm::write32(m_local_mem_addr + offset + 0x8, value);
		vm::write32(m_local_mem_addr + offset + 0xc, 0);
		break;
	}

	case NV4097_CLEAR_REPORT_VALUE:
	{
		const u32 type = value;

		switch (type)
		{
		case CELL_GCM_ZPASS_PIXEL_CNT:
			LOG_WARNING(RSX, "TODO: NV4097_CLEAR_REPORT_VALUE: ZPASS_PIXEL_CNT");
			break;
		case CELL_GCM_ZCULL_STATS:
			LOG_WARNING(RSX, "TODO: NV4097_CLEAR_REPORT_VALUE: ZCULL_STATS");
			break;
		default:
			LOG_ERROR(RSX, "NV4097_CLEAR_REPORT_VALUE: Bad type: %d", type);
			break;
		}
		break;
	}

	// Clip Plane
	case NV4097_SET_USER_CLIP_PLANE_CONTROL:
	{
		const u32 a0 = value;
		m_set_clip_plane = true;
		m_clip_plane_0 = (a0 & 0xf) ? true : false;
		m_clip_plane_1 = ((a0 >> 4)) & 0xf ? true : false;
		m_clip_plane_2 = ((a0 >> 8)) & 0xf ? true : false;
		m_clip_plane_3 = ((a0 >> 12)) & 0xf ? true : false;
		m_clip_plane_4 = ((a0 >> 16)) & 0xf ? true : false;
		m_clip_plane_5 = (a0 >> 20) ? true : false;
		break;
	}

	// Fog
	case NV4097_SET_FOG_MODE:
	{
		m_set_fog_mode = true;
		m_fog_mode = value;
		break;
	}

	case NV4097_SET_FOG_PARAMS:
	{
		m_set_fog_params = true;
		m_fog_param0 = (float&)value;
		break;
	}

	// Zmin_max
	case NV4097_SET_ZMIN_MAX_CONTROL:
	{
		const u8 cullNearFarEnable = value & 0xf;
		const u8 zclampEnable = (value >> 4) & 0xf;
		const u8 cullIgnoreW = (value >> 8) & 0xf;

		LOG_WARNING(RSX, "TODO: NV4097_SET_ZMIN_MAX_CONTROL: cullNearFarEnable=%d, zclampEnable=%d, cullIgnoreW=%d", cullNearFarEnable, zclampEnable, cullIgnoreW);
		break;
	}

	case NV4097_SET_WINDOW_OFFSET:
	{
		const u16 x = value;
		const u16 y = value >> 16;

		LOG_WARNING(RSX, "TODO: NV4097_SET_WINDOW_OFFSET: x=%d, y=%d", x, y);
		break;
	}

	case NV4097_SET_FREQUENCY_DIVIDER_OPERATION:
	{
		m_set_frequency_divider_operation = value;

		LOG_WARNING(RSX, "TODO: NV4097_SET_FREQUENCY_DIVIDER_OPERATION: %d", m_set_frequency_divider_operation);
		break;
	}

	case NV4097_SET_RENDER_ENABLE:
	{
		const u32 offset = value & 0xffffff;
		const u8 mode = value >> 24;

		LOG_WARNING(RSX, "TODO: NV4097_SET_RENDER_ENABLE: Offset=0x%06x, Mode=0x%x", offset, mode);
		break;
	}

	case NV4097_SET_ZPASS_PIXEL_COUNT_ENABLE:
	{
		const u32 enable = value;

		LOG_WARNING(RSX, "TODO: NV4097_SET_ZPASS_PIXEL_COUNT_ENABLE: %d", enable);
		break;
	}

	// NV0039
	case NV0039_SET_CONTEXT_DMA_BUFFER_IN:
		m_context_dma_buffer_in_src = value;
		break;

	case NV0039_SET_CONTEXT_DMA_BUFFER_OUT:
		m_context_dma_buffer_in_dst = value;
		break;

	case NV0039_OFFSET_IN:
		break;

	case NV0039_OFFSET_OUT:
		break;

	case NV0039_PITCH_IN:
		break;

	case NV0039_LINE_LENGTH_IN:
		break;

	case NV0039_BUFFER_NOTIFY:
	{
		const u32 inOffset = methodRegisters[NV0039_OFFSET_IN];
		const u32 outOffset = methodRegisters[NV0039_OFFSET_OUT];
		const u32 inPitch = methodRegisters[NV0039_PITCH_IN];
		const u32 outPitch = methodRegisters[NV0039_PITCH_OUT];
		const u32 lineLength = methodRegisters[NV0039_LINE_LENGTH_IN];
		const u32 lineCount = methodRegisters[NV0039_LINE_COUNT];
		const u8 outFormat = methodRegisters[NV0039_FORMAT] >> 8;
		const u8 inFormat = methodRegisters[NV0039_FORMAT];
		const u32 notify = value;

		// The existing GCM commands use only the value 0x1 for inFormat and outFormat
		if (inFormat != 0x01 || outFormat != 0x01)
		{
			LOG_ERROR(RSX, "NV0039_OFFSET_IN: Unsupported format: inFormat=%d, outFormat=%d", inFormat, outFormat);
		}

		if (lineCount == 1 && !inPitch && !outPitch && !notify)
		{
			memcpy(
				vm::get_ptr<void>(GetAddress(outOffset, methodRegisters[NV0039_SET_CONTEXT_DMA_BUFFER_OUT])),
				vm::get_ptr<void>(GetAddress(inOffset, methodRegisters[NV0039_SET_CONTEXT_DMA_BUFFER_IN])),
				lineLength);
		}
		else
		{
			LOG_ERROR(RSX, "NV0039_OFFSET_IN: bad offset(in=0x%x, out=0x%x), pitch(in=0x%x, out=0x%x), line(len=0x%x, cnt=0x%x), fmt(in=0x%x, out=0x%x), notify=0x%x",
				inOffset, outOffset, inPitch, outPitch, lineLength, lineCount, inFormat, outFormat, notify);
		}
		break;
	}

	// NV3062
	case NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN:
		m_context_dma_img_dst = value;
		break;

	case NV3062_SET_OFFSET_DESTIN:
		m_dst_offset = value;
		break;

	case NV3062_SET_COLOR_FORMAT:
		m_color_format = value;
		break;

	case NV3062_SET_PITCH:
		m_color_format_src_pitch = value;
		m_color_format_dst_pitch = value >> 16;
		break;

	case NV3062_SET_OFFSET_SOURCE:
		break;

	// NV309E
	case NV309E_SET_CONTEXT_DMA_IMAGE:
		m_context_dma_img_src = value;
		break;

	case NV309E_SET_FORMAT:
		m_swizzle_format = value;
		m_swizzle_width = value >> 16;
		m_swizzle_height = value >> 24;
		break;

	case NV309E_SET_OFFSET:
		m_swizzle_offset = value;
		break;

	// NV308A
	case NV308A_POINT:
		m_point_x = value & 0xffff;
		m_point_y = value >> 16;
		break;

	case_range(32, NV308A_COLOR, 4);
	{
		//TODO
		u32 address = GetAddress(m_dst_offset + (m_point_x << 2) + index, methodRegisters[NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN]);
		vm::write32(address, value);
		//(u32&)m_fragment_constants[(m_dst_offset | ((u32)m_point_x << 2)) + index / 4].rgba[index % 4] = (value << 16) | (value >> 16);
		//LOG_WARNING(RSX, "NV308A_COLOR: [%d]: %f, %f, %f, %f", c.id, c.x, c.y, c.z, c.w);
		break;
	}

	// NV3089
	case NV3089_SET_CONTEXT_DMA_IMAGE:
	{
		m_context_dma_img_src = value;
		break;
	}

	case NV3089_SET_CONTEXT_SURFACE:
	{
		m_context_surface = value;

		if (m_context_surface != CELL_GCM_CONTEXT_SURFACE2D && m_context_surface != CELL_GCM_CONTEXT_SWIZZLE2D)
		{
			LOG_ERROR(RSX, "NV3089_SET_CONTEXT_SURFACE: unknown surface (0x%x)", value);
		}
		break;
	}

	case NV3089_IMAGE_IN_SIZE:
	case NV3089_IMAGE_IN_FORMAT:
	case NV3089_IMAGE_IN_OFFSET:
		break;

	case NV3089_IMAGE_IN:
	{
		const u16 width = methodRegisters[NV3089_IMAGE_IN_SIZE];
		const u16 height = methodRegisters[NV3089_IMAGE_IN_SIZE] >> 16;
		const u16 pitch = methodRegisters[NV3089_IMAGE_IN_FORMAT];
		const u8 origin = methodRegisters[NV3089_IMAGE_IN_FORMAT] >> 16;
		const u8 inter = methodRegisters[NV3089_IMAGE_IN_FORMAT] >> 24;

		if (origin != 2 /* CELL_GCM_TRANSFER_ORIGIN_CORNER */)
		{
			LOG_ERROR(RSX, "NV3089_IMAGE_IN_SIZE: unknown origin (%d)", origin);
		}

		if (inter != 0 /* CELL_GCM_TRANSFER_INTERPOLATOR_ZOH */ && inter != 1 /* CELL_GCM_TRANSFER_INTERPOLATOR_FOH */)
		{
			LOG_ERROR(RSX, "NV3089_IMAGE_IN_SIZE: unknown inter (%d)", inter);
		}

		const u32 src_offset = methodRegisters[NV3089_IMAGE_IN_OFFSET];
		const u32 src_dma = methodRegisters[NV3089_SET_CONTEXT_DMA_IMAGE];

		u32 dst_offset;
		u32 dst_dma = 0;

		switch (methodRegisters[NV3089_SET_CONTEXT_SURFACE])
		{
		case CELL_GCM_CONTEXT_SURFACE2D:
			dst_dma = methodRegisters[NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN];
			dst_offset = methodRegisters[NV3062_SET_OFFSET_DESTIN];
			break;

		case CELL_GCM_CONTEXT_SWIZZLE2D:
			dst_dma = methodRegisters[NV309E_SET_CONTEXT_DMA_IMAGE];
			dst_offset = methodRegisters[NV309E_SET_OFFSET];
			break;

		default:
			LOG_ERROR(RSX, "NV3089_IMAGE_IN_SIZE: unknown m_context_surface (0x%x)", m_context_surface);
			break;
		}

		if (!dst_dma)
			break;

		LOG_ERROR(RSX, "NV3089_IMAGE_IN_SIZE: src = 0x%x, dst = 0x%x", src_offset, dst_offset);

		const u16 u = value; // inX (currently ignored)
		const u16 v = value >> 16; // inY (currently ignored)

		u8* pixels_src = vm::get_ptr<u8>(GetAddress(src_offset, src_dma));
		u8* pixels_dst = vm::get_ptr<u8>(GetAddress(dst_offset, dst_dma));

		if (m_color_format != 4 /* CELL_GCM_TRANSFER_SURFACE_FORMAT_R5G6B5 */ && m_color_format != 10 /* CELL_GCM_TRANSFER_SURFACE_FORMAT_A8R8G8B8 */)
		{
			LOG_ERROR(RSX, "NV3089_IMAGE_IN_SIZE: unknown m_color_format (%d)", m_color_format);
		}

		const u32 in_bpp = m_color_format == 4 ? 2 : 4; // bytes per pixel
		const u32 out_bpp = m_color_conv_fmt == 7 ? 2 : 4;

		const s32 out_w = (s32)(u64(width) * (1 << 20) / m_color_conv_dsdx);
		const s32 out_h = (s32)(u64(height) * (1 << 20) / m_color_conv_dtdy);

		if (methodRegisters[NV3089_SET_CONTEXT_SURFACE] == CELL_GCM_CONTEXT_SWIZZLE2D)
		{
			u8* linear_pixels = pixels_src;
			u8* swizzled_pixels = new u8[in_bpp * width * height];

			int sw_width = 1 << (int)log2(width);
			int sw_height = 1 << (int)log2(height);

			for (int y = 0; y < sw_height; y++)
			{
				for (int x = 0; x < sw_width; x++)
				{
					switch (in_bpp)
					{
					case 1:
						swizzled_pixels[LinearToSwizzleAddress(x, y, 0, sw_width, sw_height, 0)] = linear_pixels[y * sw_height + x];
						break;
					case 2:
						((u16*)swizzled_pixels)[LinearToSwizzleAddress(x, y, 0, sw_width, sw_height, 0)] = ((u16*)linear_pixels)[y * sw_height + x];
						break;
					case 4:
						((u32*)swizzled_pixels)[LinearToSwizzleAddress(x, y, 0, sw_width, sw_height, 0)] = ((u32*)linear_pixels)[y * sw_height + x];
						break;
					}
					
				}
			}

			pixels_src = swizzled_pixels;
		}

		LOG_WARNING(RSX, "NV3089_IMAGE_IN_SIZE: w=%d, h=%d, pitch=%d, offset=0x%x, inX=%f, inY=%f, scaleX=%f, scaleY=%f",
			width, height, pitch, src_offset, double(u) / 16, double(v) / 16, double(1 << 20) / (m_color_conv_dsdx), double(1 << 20) / (m_color_conv_dtdy));

		std::unique_ptr<u8[]> temp;
		
		if (in_bpp != out_bpp && width != out_w && height != out_h)
		{
			// resize/convert if necessary

			temp.reset(new u8[out_bpp * out_w * out_h]);

			AVPixelFormat in_format = m_color_format == 4 ? AV_PIX_FMT_RGB565BE : AV_PIX_FMT_ARGB; // ???
			AVPixelFormat out_format = m_color_conv_fmt == 7 ? AV_PIX_FMT_RGB565BE : AV_PIX_FMT_ARGB; // ???

			std::unique_ptr<SwsContext, void(*)(SwsContext*)> sws(sws_getContext(width, height, in_format, out_w, out_h, out_format,
				inter ? SWS_FAST_BILINEAR : SWS_POINT, NULL, NULL, NULL), sws_freeContext);

			int in_line = in_bpp * width;
			u8* out_ptr = temp.get();
			int out_line = out_bpp * out_w;

			sws_scale(sws.get(), &pixels_src, &in_line, 0, height, &out_ptr, &out_line);

			pixels_src = temp.get(); // use resized image as a source
		}

		if (m_color_conv_out_w != m_color_conv_clip_w || m_color_conv_out_w != out_w ||
			m_color_conv_out_h != m_color_conv_clip_h || m_color_conv_out_h != out_h ||
			m_color_conv_out_x || m_color_conv_out_y || m_color_conv_clip_x || m_color_conv_clip_y)
		{
			// clip if necessary

			for (s32 y = m_color_conv_clip_y, dst_y = m_color_conv_out_y; y < out_h; y++, dst_y++)
			{
				if (dst_y >= 0 && dst_y < m_color_conv_out_h)
				{
					// destination line
					u8* dst_line = pixels_dst + dst_y * out_bpp * m_color_conv_out_w + std::min<s32>(std::max<s32>(m_color_conv_out_x, 0), m_color_conv_out_w);
					size_t dst_max = std::min<s32>(std::max<s32>((s32)m_color_conv_out_w - m_color_conv_out_x, 0), m_color_conv_out_w) * out_bpp;

					if (y >= 0 && y < std::min<s32>(m_color_conv_clip_h, out_h))
					{
						// source line
						u8* src_line = pixels_src + y * out_bpp * out_w + std::min<s32>(std::max<s32>(m_color_conv_clip_x, 0), m_color_conv_clip_w);
						size_t src_max = std::min<s32>(std::max<s32>((s32)m_color_conv_clip_w - m_color_conv_clip_x, 0), m_color_conv_clip_w) * out_bpp;

						std::pair<u8*, size_t>
							z0 = { src_line + 0, std::min<size_t>(dst_max, std::max<s64>(0, m_color_conv_clip_x)) },
							d0 = { src_line + z0.second, std::min<size_t>(dst_max - z0.second, src_max) },
							z1 = { src_line + d0.second, dst_max - z0.second - d0.second };

						memset(z0.first, 0, z0.second);
						memcpy(d0.first, src_line, d0.second);
						memset(z1.first, 0, z1.second);
					}
					else
					{
						memset(dst_line, 0, dst_max);
					}
				}
			}
		}
		else
		{
			memcpy(pixels_dst, pixels_src, out_w * out_h * out_bpp);
		}

		if (methodRegisters[NV3089_SET_CONTEXT_SURFACE] == CELL_GCM_CONTEXT_SWIZZLE2D)
		{
			delete[] pixels_src;
		}

		break;
	}

	case NV3089_SET_COLOR_CONVERSION:
		m_color_conv = value;
		if (m_color_conv != 1 /* CELL_GCM_TRANSFER_CONVERSION_TRUNCATE */)
		{
			LOG_ERROR(RSX, "NV3089_SET_COLOR_CONVERSION: unknown color conv (%d)", m_color_conv);
		}
		break;
		
	case NV3089_SET_COLOR_FORMAT:
		m_color_conv_fmt = value;
		if (m_color_conv_fmt != 3 /* CELL_GCM_TRANSFER_SCALE_FORMAT_A8R8G8B8 */ && m_color_conv_fmt != 7 /* CELL_GCM_TRANSFER_SCALE_FORMAT_R5G6B5 */)
		{
			LOG_ERROR(RSX, "NV3089_SET_COLOR_FORMAT: unknown format (%d)", m_color_conv_fmt);
		}
		break;

	case NV3089_SET_OPERATION:
		m_color_conv_op = value;
		if (m_color_conv_op != 3 /* CELL_GCM_TRANSFER_OPERATION_SRCCOPY */)
		{
			LOG_ERROR(RSX, "NV3089_SET_OPERATION: unknown color conv op (%d)", m_color_conv_op);
		}
		break;

	case NV3089_CLIP_POINT:
		m_color_conv_clip_x = value;
		m_color_conv_clip_y = value >> 16;
		break;

	case NV3089_CLIP_SIZE:
		m_color_conv_clip_w = value;
		m_color_conv_clip_h = value >> 16;
		break;

	case NV3089_IMAGE_OUT_POINT:
		m_color_conv_out_x = value;
		m_color_conv_out_y = value >> 16;
		break;

	case NV3089_IMAGE_OUT_SIZE:
		m_color_conv_out_w = value;
		m_color_conv_out_h = value >> 16;
		break;

	case NV3089_DS_DX:
		m_color_conv_dsdx = value;
		break;

	case NV3089_DT_DY:
		m_color_conv_dtdy = value;
		break;

	case GCM_SET_USER_COMMAND:
	{
		const u32 cause = value;
		auto cb = m_user_handler;
		Emu.GetCallbackManager().Async([cb, cause](PPUThread& CPU)
		{
			cb(CPU, cause);
		});
		break;
	}

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
		LOG_WARNING(RSX, "Unused NV4097 method 0x%x detected!", reg);
		break;
	}

	case NV0039_PITCH_OUT:
	case NV0039_LINE_COUNT:
	case NV0039_FORMAT:
	case NV0039_SET_OBJECT:
	case NV0039_SET_CONTEXT_DMA_NOTIFIES:
	{
		LOG_WARNING(RSX, "Unused NV0039 method 0x%x detected!", reg);
		break;
	}

	case NV3062_SET_OBJECT:
	case NV3062_SET_CONTEXT_DMA_NOTIFIES:
	case NV3062_SET_CONTEXT_DMA_IMAGE_SOURCE:
	{
		LOG_WARNING(RSX, "Unused NV3062 method 0x%x detected!", reg);
		break;
	}

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
		LOG_WARNING(RSX, "Unused NV308A method 0x%x detected!", reg);
		break;
	}

	case NV309E_SET_OBJECT:
	case NV309E_SET_CONTEXT_DMA_NOTIFIES:
	{
		LOG_WARNING(RSX, "Unused NV309E method 0x%x detected!", reg);
		break;
	}

	case NV3089_SET_OBJECT:
	case NV3089_SET_CONTEXT_DMA_NOTIFIES:
	case NV3089_SET_CONTEXT_PATTERN:
	case NV3089_SET_CONTEXT_ROP:
	case NV3089_SET_CONTEXT_BETA1:
	case NV3089_SET_CONTEXT_BETA4:
	{
		LOG_WARNING(RSX, "Unused NV3089 methods 0x%x detected!", reg);
		break;
	}

	default:
	{
		LOG_ERROR(RSX, "TODO: %s", (GetMethodName(reg) + " = " + std::to_string(value)).c_str());
		break;
	}
	}
}

void RSXThread::Begin(u32 draw_mode)
{
	m_begin_end = 1;
	m_draw_mode = draw_mode;
	m_draw_array_count = 0;
	m_draw_array_first = ~0;
}

void RSXThread::End()
{
	ExecCMD();

	for (auto &vdata : m_vertex_data)
	{
		vdata.data.clear();
	}

	m_indexed_array.Reset();
	m_fragment_constants.clear();
	m_transform_constants.clear();
	m_cur_fragment_prog_num = 0;

	m_clear_surface_mask = 0;
	m_begin_end = 0;

	OnReset();
}

void RSXThread::Task()
{
	u8 inc;
	LOG_NOTICE(RSX, "RSX thread started");

	OnInitThread();

	m_last_flip_time = get_system_time() - 1000000;

	thread_t vblank("VBlank thread", true /* autojoin */, [this]()
	{
		const u64 start_time = get_system_time();

		m_vblank_count = 0;

		while (!TestDestroy() && !Emu.IsStopped())
		{
			if (get_system_time() - start_time > m_vblank_count * 1000000 / 60)
			{
				m_vblank_count++;
				if (m_vblank_handler)
				{
					auto cb = m_vblank_handler;
					Emu.GetCallbackManager().Async([cb](PPUThread& CPU)
					{
						cb(CPU, 1);
					});
				}
				continue;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
		}
	});

	while (!TestDestroy()) try
	{
		if (Emu.IsStopped())
		{
			LOG_WARNING(RSX, "RSX thread aborted");
			break;
		}
		std::lock_guard<std::mutex> lock(m_cs_main);

		inc = 1;

		u32 get = m_ctrl->get.read_sync();
		u32 put = m_ctrl->put.read_sync();

		if (put == get || !Emu.IsRunning())
		{
			if (put == get)
			{
				if (m_flip_status == 0)
					m_sem_flip.post_and_wait();

				m_sem_flush.post_and_wait();
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
			continue;
		}

		const u32 cmd = ReadIO32(get);
		const u32 count = (cmd >> 18) & 0x7ff;
	
		if (cmd & CELL_GCM_METHOD_FLAG_JUMP)
		{
			u32 offs = cmd & 0x1fffffff;
			//LOG_WARNING(RSX, "rsx jump(0x%x) #addr=0x%x, cmd=0x%x, get=0x%x, put=0x%x", offs, m_ioAddress + get, cmd, get, put);
			m_ctrl->get.exchange(be_t<u32>::make(offs));
			continue;
		}
		if (cmd & CELL_GCM_METHOD_FLAG_CALL)
		{
			m_call_stack.push(get + 4);
			u32 offs = cmd & ~3;
			//LOG_WARNING(RSX, "rsx call(0x%x) #0x%x - 0x%x", offs, cmd, get);
			m_ctrl->get.exchange(be_t<u32>::make(offs));
			continue;
		}
		if (cmd == CELL_GCM_METHOD_FLAG_RETURN)
		{
			u32 get = m_call_stack.top();
			m_call_stack.pop();
			//LOG_WARNING(RSX, "rsx return(0x%x)", get);
			m_ctrl->get.exchange(be_t<u32>::make(get));
			continue;
		}
		if (cmd & CELL_GCM_METHOD_FLAG_NON_INCREMENT)
		{
			//LOG_WARNING(RSX, "rsx non increment cmd! 0x%x", cmd);
			inc = 0;
		}

		if (cmd == 0) //nop
		{
			m_ctrl->get.atomic_op([](be_t<u32>& value)
			{
				value += 4;
			});
			continue;
		}

		auto args = vm::ptr<u32>::make((u32)Memory.RSXIOMem.RealAddr(get + 4));

		for (u32 i = 0; i < count; i++)
		{
			u32 reg = (cmd & 0xffff) + (i * 4 * inc);
			u32 value = args[i];

			if (Ini.RSXLogging.GetValue())
			{
				LOG_NOTICE(Log::RSX, "%s(0x%x) = 0x%x", GetMethodName(reg).c_str(), reg, value);
			}

			update_reg(reg, value);
			methodRegisters[reg] = value;
		}

		m_ctrl->get.atomic_op([count](be_t<u32>& value)
		{
			value += (count + 1) * 4;
		});
	}
	catch (const std::string& e)
	{
		LOG_ERROR(RSX, "Exception: %s", e.c_str());
		Emu.Pause();
	}
	catch (const char* e)
	{
		LOG_ERROR(RSX, "Exception: %s", e);
		Emu.Pause();
	}

	LOG_NOTICE(RSX, "RSX thread ended");

	OnExitThread();
}

void RSXThread::Init(const u32 ioAddress, const u32 ioSize, const u32 ctrlAddress, const u32 localAddress)
{
	m_ctrl = vm::get_ptr<CellGcmControl>(ctrlAddress);
	m_ioAddress = ioAddress;
	m_ioSize = ioSize;
	m_ctrlAddress = ctrlAddress;
	m_local_mem_addr = localAddress;

	m_cur_fragment_prog = nullptr;
	m_cur_fragment_prog_num = 0;

	m_used_gcm_commands.clear();

	OnInit();
	ThreadBase::Start();
}

u32 RSXThread::ReadIO32(u32 addr)
{
	u32 value;
	if (!Memory.RSXIOMem.Read32(addr, &value))
	{
		throw fmt::Format("%s(addr=0x%x): RSXIO memory not mapped", __FUNCTION__, addr);
	}
	return value;
}

void RSXThread::WriteIO32(u32 addr, u32 value)
{
	if (!Memory.RSXIOMem.Write32(addr, value))
	{
		throw fmt::Format("%s(addr=0x%x): RSXIO memory not mapped", __FUNCTION__, addr);
	}
}
