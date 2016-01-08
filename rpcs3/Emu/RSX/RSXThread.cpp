#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/state.h"
#include "Emu/RSX/GSManager.h"
#include "RSXThread.h"

#include "Emu/SysCalls/Callback.h"
#include "Emu/SysCalls/CB_FUNC.h"
#include "Emu/SysCalls/lv2/sys_time.h"

#include "Common/BufferUtils.h"
#include "rsx_methods.h"
#include "rsx_cache.h"

#define CMD_DEBUG 0

bool user_asked_for_frame_capture = false;
frame_capture_data frame_debug;

namespace rsx
{
	u32 get_address(u32 offset, u32 location)
	{
		u32 res = 0;

		switch (location)
		{
		case CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER:
		case CELL_GCM_LOCATION_LOCAL:
		{
			//TODO: don't use not named constants like 0xC0000000
			res = 0xC0000000 + offset;
			break;
		}

		case CELL_GCM_CONTEXT_DMA_MEMORY_HOST_BUFFER:
		case CELL_GCM_LOCATION_MAIN:
		{
			res = (u32)RSXIOMem.RealAddr(offset); // TODO: Error Check?
			if (res == 0)
			{
				throw EXCEPTION("GetAddress(offset=0x%x, location=0x%x): RSXIO memory not mapped", offset, location);
			}

			//if (Emu.GetGSManager().GetRender().strict_ordering[offset >> 20])
			//{
			//	_mm_mfence(); // probably doesn't have any effect on current implementation
			//}

			break;
		}
		default:
		{
			throw EXCEPTION("Invalid location (offset=0x%x, location=0x%x)", offset, location);
		}
		}

		return res;
	}

	u32 get_vertex_type_size(u32 type)
	{
		switch (type)
		{
		case CELL_GCM_VERTEX_S1:    return sizeof(u16);
		case CELL_GCM_VERTEX_F:     return sizeof(f32);
		case CELL_GCM_VERTEX_SF:    return sizeof(f16);
		case CELL_GCM_VERTEX_UB:    return sizeof(u8);
		case CELL_GCM_VERTEX_S32K:  return sizeof(u32);
		case CELL_GCM_VERTEX_CMP:   return sizeof(u32);
		case CELL_GCM_VERTEX_UB256: return sizeof(u8) * 4;

		default:
			LOG_ERROR(RSX, "RSXVertexData::GetTypeSize: Bad vertex data type (%d)!", type);
			assert(0);
			return 1;
		}
	}
	
	void tiled_region::write(const void *src, u32 width, u32 height, u32 pitch)
	{
		if (!tile)
		{
			memcpy(ptr, src, height * pitch);
			return;
		}

		u32 offset_x = base % tile->pitch;
		u32 offset_y = base / tile->pitch;

		switch (tile->comp)
		{
		case CELL_GCM_COMPMODE_C32_2X1:
		case CELL_GCM_COMPMODE_DISABLED:
			for (int y = 0; y < height; ++y)
			{
				memcpy(ptr + (offset_y + y) * tile->pitch + offset_x, (u8*)src + pitch * y, pitch);
			}
			break;
			/*
		case CELL_GCM_COMPMODE_C32_2X1:
			for (u32 y = 0; y < height; ++y)
			{
				for (u32 x = 0; x < width; ++x)
				{
					u32 value = *(u32*)((u8*)src + pitch * y + x * sizeof(u32));

					*(u32*)(ptr + (offset_y + y) * tile->pitch + offset_x + (x * 2 + 0) * sizeof(u32)) = value;
					*(u32*)(ptr + (offset_y + y) * tile->pitch + offset_x + (x * 2 + 1) * sizeof(u32)) = value;
				}
			}
			break;
			*/
		case CELL_GCM_COMPMODE_C32_2X2:
			for (u32 y = 0; y < height; ++y)
			{
				for (u32 x = 0; x < width; ++x)
				{
					u32 value = *(u32*)((u8*)src + pitch * y + x * sizeof(u32));

					*(u32*)(ptr + (offset_y + y * 2 + 0) * tile->pitch + offset_x + (x * 2 + 0) * sizeof(u32)) = value;
					*(u32*)(ptr + (offset_y + y * 2 + 0) * tile->pitch + offset_x + (x * 2 + 1) * sizeof(u32)) = value;
					*(u32*)(ptr + (offset_y + y * 2 + 1) * tile->pitch + offset_x + (x * 2 + 0) * sizeof(u32)) = value;
					*(u32*)(ptr + (offset_y + y * 2 + 1) * tile->pitch + offset_x + (x * 2 + 1) * sizeof(u32)) = value;
				}
			}
			break;
		default:
			throw;
		}
	}

	void tiled_region::read(void *dst, u32 width, u32 height, u32 pitch)
	{
		if (!tile)
		{
			memcpy(dst, ptr, height * pitch);
			return;
		}

		u32 offset_x = base % tile->pitch;
		u32 offset_y = base / tile->pitch;

		switch (tile->comp)
		{
		case CELL_GCM_COMPMODE_C32_2X1:
		case CELL_GCM_COMPMODE_DISABLED:
			for (int y = 0; y < height; ++y)
			{
				memcpy((u8*)dst + pitch * y, ptr + (offset_y + y) * tile->pitch + offset_x, pitch);
			}
			break;
			/*
		case CELL_GCM_COMPMODE_C32_2X1:
			for (u32 y = 0; y < height; ++y)
			{
				for (u32 x = 0; x < width; ++x)
				{
					u32 value = *(u32*)(ptr + (offset_y + y) * tile->pitch + offset_x + (x * 2 + 0) * sizeof(u32));

					*(u32*)((u8*)dst + pitch * y + x * sizeof(u32)) = value;
				}
			}
			break;
			*/
		case CELL_GCM_COMPMODE_C32_2X2:
			for (u32 y = 0; y < height; ++y)
			{
				for (u32 x = 0; x < width; ++x)
				{
					u32 value = *(u32*)(ptr + (offset_y + y * 2 + 0) * tile->pitch + offset_x + (x * 2 + 0) * sizeof(u32));

					*(u32*)((u8*)dst + pitch * y + x * sizeof(u32)) = value;
				}
			}
			break;
		default:
			throw;
		}
	}

	thread::~thread()
	{
	}

	void thread::load_vertex_data(u32 first, u32 count)
	{
		vertex_draw_count += count;

		for (int index = 0; index < limits::vertex_count; ++index)
		{
			const auto &info = vertex_arrays_info[index];

			if (info.size == 0) // disabled
				continue;

			auto &data = vertex_arrays[index];

			u32 type_size = get_vertex_type_size(info.type);
			u32 element_size = type_size * info.size;

			u32 dst_position = (u32)data.size();
			data.resize(dst_position + count * element_size);
			write_vertex_array_data_to_buffer(data.data() + dst_position, first, count, index, info);
		}
	}

	void thread::load_vertex_index_data(u32 first, u32 count)
	{
		u32 address = get_address(method_registers[NV4097_SET_INDEX_ARRAY_ADDRESS], method_registers[NV4097_SET_INDEX_ARRAY_DMA] & 0xf);
		u32 type = method_registers[NV4097_SET_INDEX_ARRAY_DMA] >> 4;

		u32 type_size = type == CELL_GCM_DRAW_INDEX_ARRAY_TYPE_32 ? sizeof(u32) : sizeof(u16);
		u32 dst_offset = (u32)vertex_index_array.size();
		vertex_index_array.resize(dst_offset + count * type_size);

		u32 base_offset = method_registers[NV4097_SET_VERTEX_DATA_BASE_OFFSET];
		u32 base_index = method_registers[NV4097_SET_VERTEX_DATA_BASE_INDEX];

		switch (type)
		{
		case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_32:
			for (u32 i = 0; i < count; ++i)
			{
				(u32&)vertex_index_array[dst_offset + i * sizeof(u32)] = vm::read32(address + (first + i) * sizeof(u32));
			}
			break;

		case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16:
			for (u32 i = 0; i < count; ++i)
			{
				(u16&)vertex_index_array[dst_offset + i * sizeof(u16)] = vm::read16(address + (first + i) * sizeof(u16));
			}
			break;
		}
	}

	void thread::capture_frame(const std::string &name)
	{
		frame_capture_data::draw_state draw_state = {};

		int clip_w = rsx::method_registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL] >> 16;
		int clip_h = rsx::method_registers[NV4097_SET_SURFACE_CLIP_VERTICAL] >> 16;
		size_t pitch = clip_w * 4;
		std::vector<size_t> color_index_to_record;
		switch (method_registers[NV4097_SET_SURFACE_COLOR_TARGET])
		{
		case CELL_GCM_SURFACE_TARGET_0:
			color_index_to_record = { 0 };
			break;
		case CELL_GCM_SURFACE_TARGET_1:
			color_index_to_record = { 1 };
			break;
		case CELL_GCM_SURFACE_TARGET_MRT1:
			color_index_to_record = { 0, 1 };
			break;
		case CELL_GCM_SURFACE_TARGET_MRT2:
			color_index_to_record = { 0, 1, 2 };
			break;
		case CELL_GCM_SURFACE_TARGET_MRT3:
			color_index_to_record = { 0, 1, 2, 3 };
			break;
		}
/*		for (size_t i : color_index_to_record)
		{
			draw_state.color_buffer[i].width = clip_w;
			draw_state.color_buffer[i].height = clip_h;
			draw_state.color_buffer[i].data.resize(pitch * clip_h);
			copy_render_targets_to_memory(draw_state.color_buffer[i].data.data(), i);
		}
		if (get_address(method_registers[NV4097_SET_SURFACE_ZETA_OFFSET], method_registers[NV4097_SET_CONTEXT_DMA_ZETA]))
		{
			draw_state.depth.width = clip_w;
			draw_state.depth.height = clip_h;
			draw_state.depth.data.resize(clip_w * clip_h * 4);
			copy_depth_buffer_to_memory(draw_state.depth.data.data());
			draw_state.stencil.width = clip_w;
			draw_state.stencil.height = clip_h;
			draw_state.stencil.data.resize(clip_w * clip_h * 4);
			copy_stencil_buffer_to_memory(draw_state.stencil.data.data());
		}*/
		draw_state.programs = get_programs();
		draw_state.name = name;
		frame_debug.draw_calls.push_back(draw_state);
	}

	void thread::begin()
	{
		draw_mode = method_registers[NV4097_SET_BEGIN_END];
	}

	void thread::end()
	{
		vertex_index_array.clear();

		for (auto &vertex_array : vertex_arrays)
		{
			vertex_array.clear();
		}

		transform_constants.clear();

		if (capture_current_frame)
		{
			capture_frame("Draw " + std::to_string(vertex_draw_count));
		}
	}

	void thread::on_task()
	{
		on_init_thread();

		reset();

		last_flip_time = get_system_time() - 1000000;

		scope_thread_t vblank(PURE_EXPR("VBlank Thread"s), [this]()
		{
			const u64 start_time = get_system_time();

			vblank_count = 0;

			// TODO: exit condition
			while (!Emu.IsStopped())
			{
				if (get_system_time() - start_time > vblank_count * 1000000 / 60)
				{
					vblank_count++;

					if (vblank_handler)
					{
						Emu.GetCallbackManager().Async([func = vblank_handler](PPUThread& ppu)
						{
							func(ppu, 1);
						});
					}

					continue;
				}

				std::this_thread::sleep_for(1ms); // hack
			}
		});

		// TODO: exit condition
		while (true)
		{
			CHECK_EMU_STATUS;

			be_t<u32> get = ctrl->get;
			be_t<u32> put = ctrl->put;

			if (put == get || !Emu.IsRunning())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
				continue;
			}

			const u32 cmd = ReadIO32(get);
			const u32 count = (cmd >> 18) & 0x7ff;

			if (cmd & CELL_GCM_METHOD_FLAG_JUMP)
			{
				u32 offs = cmd & 0x1fffffff;
				//LOG_WARNING(RSX, "rsx jump(0x%x) #addr=0x%x, cmd=0x%x, get=0x%x, put=0x%x", offs, m_ioAddress + get, cmd, get, put);
				ctrl->get = offs;
				continue;
			}
			if (cmd & CELL_GCM_METHOD_FLAG_CALL)
			{
				m_call_stack.push(get + 4);
				u32 offs = cmd & ~3;
				//LOG_WARNING(RSX, "rsx call(0x%x) #0x%x - 0x%x", offs, cmd, get);
				ctrl->get = offs;
				continue;
			}
			if (cmd == CELL_GCM_METHOD_FLAG_RETURN)
			{
				u32 get = m_call_stack.top();
				m_call_stack.pop();
				//LOG_WARNING(RSX, "rsx return(0x%x)", get);
				ctrl->get = get;
				continue;
			}

			if (cmd == 0) //nop
			{
				ctrl->get = get + 4;
				continue;
			}

			auto args = vm::ptr<u32>::make((u32)RSXIOMem.RealAddr(get + 4));

			u32 first_cmd = (cmd & 0xffff) >> 2;

			if (cmd & 0x3)
			{
				LOG_WARNING(Log::RSX, "unaligned command: %s (0x%x from 0x%x)", get_method_name(first_cmd).c_str(), first_cmd, cmd & 0xffff);
			}

			for (u32 i = 0; i < count; i++)
			{
				u32 reg = cmd & CELL_GCM_METHOD_FLAG_NON_INCREMENT ? first_cmd : first_cmd + i;
				u32 value = args[i];

				if (rpcs3::config.misc.log.rsx_logging.value())
				{
					LOG_NOTICE(Log::RSX, "%s(0x%x) = 0x%x", get_method_name(reg).c_str(), reg, value);
				}

				method_registers[reg] = value;
				if (capture_current_frame)
					frame_debug.command_queue.push_back(std::make_pair(reg, value));

				if (auto method = methods[reg])
					method(this, value);
			}

			ctrl->get = get + (count + 1) * 4;
		}
	}

	std::string thread::get_name() const
	{
		return "rsx::thread"s;
	}

	void thread::fill_scale_offset_data(void *buffer) const
	{
		int clip_w = rsx::method_registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL] >> 16;
		int clip_h = rsx::method_registers[NV4097_SET_SURFACE_CLIP_VERTICAL] >> 16;

		float scale_x = (float&)rsx::method_registers[NV4097_SET_VIEWPORT_SCALE] / (clip_w / 2.f);
		float offset_x = (float&)rsx::method_registers[NV4097_SET_VIEWPORT_OFFSET] - (clip_w / 2.f);
		offset_x /= clip_w / 2.f;

		float scale_y = (float&)rsx::method_registers[NV4097_SET_VIEWPORT_SCALE + 1] / (clip_h / 2.f);
		float offset_y = ((float&)rsx::method_registers[NV4097_SET_VIEWPORT_OFFSET + 1] - (clip_h / 2.f));
		offset_y /= clip_h / 2.f;
		scale_y *= -1;
		offset_y *= -1;

		float scale_z = (float&)rsx::method_registers[NV4097_SET_VIEWPORT_SCALE + 2];
		float offset_z = (float&)rsx::method_registers[NV4097_SET_VIEWPORT_OFFSET + 2];

		stream_vector(buffer, scale_x, 0, 0, offset_x);
		stream_vector((char*)buffer + 16, 0, scale_y, 0, offset_y);
		stream_vector((char*)buffer + 32, 0, 0, scale_z, offset_z);
		stream_vector((char*)buffer + 48, 0, 0, 0, 1.0f);
	}

	/**
	* Fill buffer with vertex program constants.
	* Buffer must be at least 512 float4 wide.
	*/
	void thread::fill_vertex_program_constants_data(void *buffer)
	{
		for (const auto &entry : transform_constants)
			local_transform_constants[entry.first] = entry.second;
		for (const auto &entry : local_transform_constants)
			stream_vector_from_memory((char*)buffer + entry.first * 4 * sizeof(float), (void*)entry.second.rgba);
	}

	void thread::write_inline_array_to_buffer(void *dst_buffer)
	{
		u8* src = reinterpret_cast<u8*>(inline_vertex_array.data());
		u8* dst = (u8*)dst_buffer;

		size_t bytes_written = 0;
		while (bytes_written < inline_vertex_array.size() * sizeof(u32))
		{
			for (int index = 0; index < rsx::limits::vertex_count; ++index)
			{
				const auto &info = vertex_arrays_info[index];

				if (!info.size) // disabled
					continue;

				u32 type_size = rsx::get_vertex_type_size(info.type);
				u32 element_size = type_size * info.size;

				if (type_size == 1 && info.size == 4)
				{
					dst[0] = src[3];
					dst[1] = src[2];
					dst[2] = src[1];
					dst[3] = src[0];
				}
				else
					memcpy(dst, src, element_size);

				src += element_size;
				dst += element_size;

				bytes_written += element_size;
			}
		}
	}

	u64 thread::timestamp() const
	{
		// Get timestamp, and convert it from microseconds to nanoseconds
		return get_system_time() * 1000;
	}

	raw_program thread::get_raw_program() const
	{
		raw_program result;

		u32 fp_info = rsx::method_registers[NV4097_SET_SHADER_PROGRAM];

		result.state.output_attributes = rsx::method_registers[NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK];
		result.state.ctrl = rsx::method_registers[NV4097_SET_SHADER_CONTROL];

		result.vertex_shader.ucode_ptr = transform_program;
		result.vertex_shader.offset = rsx::method_registers[NV4097_SET_TRANSFORM_PROGRAM_START];

		result.fragment_shader.ucode_ptr = vm::base(rsx::get_address(fp_info & ~0x3, (fp_info & 0x3) - 1));
		result.fragment_shader.offset = 0;

		return result;
	}

	void thread::reset()
	{
		//setup method registers
		std::memset(method_registers, 0, sizeof(method_registers));

		method_registers[NV4097_SET_COLOR_MASK] = CELL_GCM_COLOR_MASK_R | CELL_GCM_COLOR_MASK_G | CELL_GCM_COLOR_MASK_B | CELL_GCM_COLOR_MASK_A;
		method_registers[NV4097_SET_SCISSOR_HORIZONTAL] = (4096 << 16) | 0;
		method_registers[NV4097_SET_SCISSOR_VERTICAL] = (4096 << 16) | 0;

		method_registers[NV4097_SET_ALPHA_FUNC] = CELL_GCM_ALWAYS;
		method_registers[NV4097_SET_ALPHA_REF] = 0;

		method_registers[NV4097_SET_BLEND_FUNC_SFACTOR] = (CELL_GCM_ONE << 16) | CELL_GCM_ONE;
		method_registers[NV4097_SET_BLEND_FUNC_DFACTOR] = (CELL_GCM_ZERO << 16) | CELL_GCM_ZERO;
		method_registers[NV4097_SET_BLEND_COLOR] = 0;
		method_registers[NV4097_SET_BLEND_COLOR2] = 0;
		method_registers[NV4097_SET_BLEND_EQUATION] = (CELL_GCM_FUNC_ADD << 16) | CELL_GCM_FUNC_ADD;

		method_registers[NV4097_SET_STENCIL_MASK] = 0xff;
		method_registers[NV4097_SET_STENCIL_FUNC] = CELL_GCM_ALWAYS;
		method_registers[NV4097_SET_STENCIL_FUNC_REF] = 0x00;
		method_registers[NV4097_SET_STENCIL_FUNC_MASK] = 0xff;
		method_registers[NV4097_SET_STENCIL_OP_FAIL] = CELL_GCM_KEEP;
		method_registers[NV4097_SET_STENCIL_OP_ZFAIL] = CELL_GCM_KEEP;
		method_registers[NV4097_SET_STENCIL_OP_ZPASS] = CELL_GCM_KEEP;

		method_registers[NV4097_SET_BACK_STENCIL_MASK] = 0xff;
		method_registers[NV4097_SET_BACK_STENCIL_FUNC] = CELL_GCM_ALWAYS;
		method_registers[NV4097_SET_BACK_STENCIL_FUNC_REF] = 0x00;
		method_registers[NV4097_SET_BACK_STENCIL_FUNC_MASK] = 0xff;
		method_registers[NV4097_SET_BACK_STENCIL_OP_FAIL] = CELL_GCM_KEEP;
		method_registers[NV4097_SET_BACK_STENCIL_OP_ZFAIL] = CELL_GCM_KEEP;
		method_registers[NV4097_SET_BACK_STENCIL_OP_ZPASS] = CELL_GCM_KEEP;

		method_registers[NV4097_SET_SHADE_MODE] = CELL_GCM_SMOOTH;

		method_registers[NV4097_SET_LOGIC_OP] = CELL_GCM_COPY;

		(f32&)method_registers[NV4097_SET_DEPTH_BOUNDS_MIN] = 0.f;
		(f32&)method_registers[NV4097_SET_DEPTH_BOUNDS_MAX] = 1.f;

		(f32&)method_registers[NV4097_SET_CLIP_MIN] = 0.f;
		(f32&)method_registers[NV4097_SET_CLIP_MAX] = 1.f;

		method_registers[NV4097_SET_LINE_WIDTH] = 1 << 3;

		method_registers[NV4097_SET_FOG_MODE] = CELL_GCM_FOG_MODE_EXP;

		method_registers[NV4097_SET_DEPTH_FUNC] = CELL_GCM_LESS;
		method_registers[NV4097_SET_DEPTH_MASK] = CELL_GCM_TRUE;
		(f32&)method_registers[NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR] = 0.f;
		(f32&)method_registers[NV4097_SET_POLYGON_OFFSET_BIAS] = 0.f;
		method_registers[NV4097_SET_FRONT_POLYGON_MODE] = CELL_GCM_POLYGON_MODE_FILL;
		method_registers[NV4097_SET_BACK_POLYGON_MODE] = CELL_GCM_POLYGON_MODE_FILL;
		method_registers[NV4097_SET_CULL_FACE] = CELL_GCM_BACK;
		method_registers[NV4097_SET_FRONT_FACE] = CELL_GCM_CCW;
		method_registers[NV4097_SET_RESTART_INDEX] = -1;

		method_registers[NV4097_SET_CLEAR_RECT_HORIZONTAL] = (4096 << 16) | 0;
		method_registers[NV4097_SET_CLEAR_RECT_VERTICAL] = (4096 << 16) | 0;

		method_registers[NV4097_SET_ZSTENCIL_CLEAR_VALUE] = 0xffffffff;

		// Reset vertex attrib array
		for (int i = 0; i < limits::vertex_count; i++)
		{
			vertex_arrays_info[i].size = 0;
		}

		// Construct Textures
		for (int i = 0; i < limits::textures_count; i++)
		{
			textures[i].init(i);
		}

		for (int i = 0; i < limits::vertex_textures_count; i++)
		{
			vertex_textures[i].init(i);
		}
	}

	void thread::init(const u32 ioAddress, const u32 ioSize, const u32 ctrlAddress, const u32 localAddress)
	{
		ctrl = vm::_ptr<CellGcmControl>(ctrlAddress);
		this->ioAddress = ioAddress;
		this->ioSize = ioSize;
		local_mem_addr = localAddress;
		flip_status = 0;

		m_used_gcm_commands.clear();

		on_init();
		start();
	}

	GcmTileInfo *thread::find_tile(u32 offset, u32 location)
	{
		for (GcmTileInfo &tile : tiles)
		{
			if (!tile.binded || tile.location != location)
			{
				continue;
			}

			if (offset >= tile.offset && offset < tile.offset + tile.size)
			{
				return &tile;
			}
		}

		return nullptr;
	}

	tiled_region thread::get_tiled_address(u32 offset, u32 location)
	{
		u32 address = get_address(offset, location);

		GcmTileInfo *tile = find_tile(offset, location);
		u32 base = 0;
		
		if (tile)
		{
			base = offset - tile->offset;
			address = get_address(tile->offset, location);
		}

		return{ address, base, tile, (u8*)vm::base(address) };
	}

	u32 thread::ReadIO32(u32 addr)
	{
		u32 value;
	
		if (!RSXIOMem.Read32(addr, &value))
		{
			throw EXCEPTION("%s(addr=0x%x): RSXIO memory not mapped", __FUNCTION__, addr);
		}

		return value;
	}

	void thread::WriteIO32(u32 addr, u32 value)
	{
		if (!RSXIOMem.Write32(addr, value))
		{
			throw EXCEPTION("%s(addr=0x%x): RSXIO memory not mapped", __FUNCTION__, addr);
		}
	}
}
