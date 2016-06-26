#include "stdafx.h"
#include "Utilities/Config.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "RSXThread.h"

#include "Emu/Cell/PPUCallback.h"

#include "Common/BufferUtils.h"
#include "rsx_methods.h"

#include "Utilities/GSL.h"
#include "Utilities/StrUtil.h"

#include <thread>

#define CMD_DEBUG 0

cfg::bool_entry g_cfg_rsx_write_color_buffers(cfg::root.video, "Write Color Buffers");
cfg::bool_entry g_cfg_rsx_write_depth_buffer(cfg::root.video, "Write Depth Buffer");
cfg::bool_entry g_cfg_rsx_read_color_buffers(cfg::root.video, "Read Color Buffers");
cfg::bool_entry g_cfg_rsx_read_depth_buffer(cfg::root.video, "Read Depth Buffer");
cfg::bool_entry g_cfg_rsx_log_programs(cfg::root.video, "Log shader programs");
cfg::bool_entry g_cfg_rsx_vsync(cfg::root.video, "VSync");
cfg::bool_entry g_cfg_rsx_3dtv(cfg::root.video, "3D Monitor");
cfg::bool_entry g_cfg_rsx_debug_output(cfg::root.video, "Debug output");
cfg::bool_entry g_cfg_rsx_overlay(cfg::root.video, "Debug overlay");

bool user_asked_for_frame_capture = false;
frame_capture_data frame_debug;

namespace vm { using namespace ps3; }

namespace rsx
{
	std::function<bool(u32 addr, bool is_writing)> g_access_violation_handler;

	std::string old_shaders_cache::shaders_cache::path_to_root()
	{
		return fs::get_executable_dir() + "data/";
	}

	void old_shaders_cache::shaders_cache::load(const std::string &path, shader_language lang)
	{
		const std::string lang_name(::unveil<shader_language>::get(lang));

		auto extract_hash = [](const std::string &string)
		{
			return std::stoull(string.substr(0, string.find('.')).c_str(), 0, 16);
		};

		for (const auto& entry : fs::dir(path))
		{
			if (entry.name == "." || entry.name == "..")
				continue;

			u64 hash;

			try
			{
				hash = extract_hash(entry.name);
			}
			catch (...)
			{
				LOG_ERROR(RSX, "Cache file '%s' ignored", entry.name);
				continue;
			}

			if (fmt::match(entry.name, "*.fs." + lang_name))
			{
				fs::file file{ path + entry.name };
				decompiled_fragment_shaders.insert(hash, { file.to_string() });
				continue;
			}

			if (fmt::match(entry.name, "*.vs." + lang_name))
			{
				fs::file file{ path + entry.name };
				decompiled_vertex_shaders.insert(hash, { file.to_string() });
				continue;
			}
		}
	}

	void old_shaders_cache::shaders_cache::load(shader_language lang)
	{
		std::string root = path_to_root();

		//shared cache
		load(root + "cache/", lang);

		std::string title_id = Emu.GetTitleID();

		if (!title_id.empty())
		{
			load(root + title_id + "/cache/", lang);
		}
	}

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

			//if (fxm::get<GSRender>()->strict_ordering[offset >> 20])
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

	u32 get_vertex_type_size_on_host(vertex_base_type type, u32 size)
	{
		switch (type)
		{
		case vertex_base_type::s1:
		case vertex_base_type::s32k:
			switch (size)
			{
			case 1:
			case 2:
			case 4:
				return sizeof(u16) * size;
			case 3:
				return sizeof(u16) * 4;
			}
			throw EXCEPTION("Wrong vector size");
		case vertex_base_type::f: return sizeof(f32) * size;
		case vertex_base_type::sf:
			switch (size)
			{
			case 1:
			case 2:
			case 4:
				return sizeof(f16) * size;
			case 3:
				return sizeof(f16) * 4;
			}
			throw EXCEPTION("Wrong vector size");
		case vertex_base_type::ub:
			switch (size)
			{
			case 1:
			case 2:
			case 4:
				return sizeof(u8) * size;
			case 3:
				return sizeof(u8) * 4;
			}
			throw EXCEPTION("Wrong vector size");
		case vertex_base_type::cmp: return sizeof(u16) * 4;
		case vertex_base_type::ub256: EXPECTS(size == 4); return sizeof(u8) * 4;
		}
		throw EXCEPTION("RSXVertexData::GetTypeSize: Bad vertex data type (%d)!", type);
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

	thread::thread()
	{
		g_access_violation_handler = [this](u32 address, bool is_writing)
		{
			return on_access_violation(address, is_writing);
		};
		m_rtts_dirty = true;
		memset(m_textures_dirty, -1, sizeof(m_textures_dirty));
		m_transform_constants_dirty = true;
	}

	thread::~thread()
	{
		g_access_violation_handler = nullptr;
	}

	void thread::capture_frame(const std::string &name)
	{
		frame_capture_data::draw_state draw_state = {};

		int clip_w = rsx::method_registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL] >> 16;
		int clip_h = rsx::method_registers[NV4097_SET_SURFACE_CLIP_VERTICAL] >> 16;
		rsx::surface_info surface = {};
		surface.unpack(rsx::method_registers[NV4097_SET_SURFACE_FORMAT]);
		draw_state.width = clip_w;
		draw_state.height = clip_h;
		draw_state.color_format = surface.color_format;
		draw_state.color_buffer = std::move(copy_render_targets_to_memory());
		draw_state.depth_format = surface.depth_format;
		draw_state.depth_stencil = std::move(copy_depth_stencil_buffer_to_memory());

		if (draw_command == rsx::draw_command::indexed)
		{
			draw_state.vertex_count = 0;
			for (const auto &range : first_count_commands)
			{
				draw_state.vertex_count += range.second;
			}
			draw_state.index_type = rsx::to_index_array_type(rsx::method_registers[NV4097_SET_INDEX_ARRAY_DMA] >> 4);

			if (draw_state.index_type == rsx::index_array_type::u16)
			{
				draw_state.index.resize(2 * draw_state.vertex_count);
			}
			if (draw_state.index_type == rsx::index_array_type::u32)
			{
				draw_state.index.resize(4 * draw_state.vertex_count);
			}
			gsl::span<gsl::byte> dst = { (gsl::byte*)draw_state.index.data(), gsl::narrow<int>(draw_state.index.size()) };
			write_index_array_data_to_buffer(dst, draw_state.index_type, draw_mode, first_count_commands);
		}

		draw_state.programs = get_programs();
		draw_state.name = name;
		frame_debug.draw_calls.push_back(draw_state);
	}

	void thread::begin()
	{
		draw_inline_vertex_array = false;
		inline_vertex_array.clear();
		first_count_commands.clear();
		draw_command = rsx::draw_command::none;
		draw_mode = to_primitive_type(method_registers[NV4097_SET_BEGIN_END]);
	}

	void thread::end()
	{
		for (u8 index = 0; index < rsx::limits::vertex_count; ++index)
		{
			register_vertex_info[index].size = 0;
			register_vertex_data[index].clear();
		}

		transform_constants.clear();

		if (capture_current_frame)
		{
			for (const auto &first_count : first_count_commands)
			{
				vertex_draw_count += first_count.second;
			}

			capture_frame("Draw " + std::to_string(vertex_draw_count));
			vertex_draw_count = 0;
		}
	}

	void thread::on_task()
	{
		on_init_thread();

		reset();

		last_flip_time = get_system_time() - 1000000;

		scope_thread vblank("VBlank Thread", [this]()
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

			const u32 get = ctrl->get;
			const u32 put = ctrl->put;

			if (put == get || !Emu.IsRunning())
			{
				do_internal_task();
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
				LOG_WARNING(RSX, "unaligned command: %s (0x%x from 0x%x)", get_method_name(first_cmd).c_str(), first_cmd, cmd & 0xffff);
			}

			for (u32 i = 0; i < count; i++)
			{
				u32 reg = cmd & CELL_GCM_METHOD_FLAG_NON_INCREMENT ? first_cmd : first_cmd + i;
				u32 value = args[i];

				LOG_TRACE(RSX, "%s(0x%x) = 0x%x", get_method_name(reg).c_str(), reg, value);

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

	void thread::fill_scale_offset_data(void *buffer, bool is_d3d) const
	{
		int clip_w = rsx::method_registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL] >> 16;
		int clip_h = rsx::method_registers[NV4097_SET_SURFACE_CLIP_VERTICAL] >> 16;

		float scale_x = (float&)rsx::method_registers[NV4097_SET_VIEWPORT_SCALE] / (clip_w / 2.f);
		float offset_x = (float&)rsx::method_registers[NV4097_SET_VIEWPORT_OFFSET] - (clip_w / 2.f);
		offset_x /= clip_w / 2.f;

		float scale_y = (float&)rsx::method_registers[NV4097_SET_VIEWPORT_SCALE + 1] / (clip_h / 2.f);
		float offset_y = ((float&)rsx::method_registers[NV4097_SET_VIEWPORT_OFFSET + 1] - (clip_h / 2.f));
		offset_y /= clip_h / 2.f;
		if (is_d3d) scale_y *= -1;
		if (is_d3d) offset_y *= -1;

		float scale_z = (float&)rsx::method_registers[NV4097_SET_VIEWPORT_SCALE + 2];
		float offset_z = (float&)rsx::method_registers[NV4097_SET_VIEWPORT_OFFSET + 2];
		if (!is_d3d) offset_z -= .5;

		float one = 1.f;

		stream_vector(buffer, (u32&)scale_x, 0, 0, (u32&)offset_x);
		stream_vector((char*)buffer + 16, 0, (u32&)scale_y, 0, (u32&)offset_y);
		stream_vector((char*)buffer + 32, 0, 0, (u32&)scale_z, (u32&)offset_z);
		stream_vector((char*)buffer + 48, 0, 0, 0, (u32&)one);
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

				u32 element_size = rsx::get_vertex_type_size_on_host(info.type, info.size);

				if (info.type == vertex_base_type::ub && info.size == 4)
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

	void thread::do_internal_task()
	{
		if (m_internal_tasks.empty())
		{
			std::this_thread::sleep_for(1ms);
		}
		else
		{
			EXPECTS(0);
			//std::lock_guard<std::mutex> lock{ m_mtx_task };

			//internal_task_entry &front = m_internal_tasks.front();

			//if (front.callback())
			//{
			//	front.promise.set_value();
			//	m_internal_tasks.pop_front();
			//}
		}
	}

	//std::future<void> thread::add_internal_task(std::function<bool()> callback)
	//{
	//	std::lock_guard<std::mutex> lock{ m_mtx_task };
	//	m_internal_tasks.emplace_back(callback);

	//	return m_internal_tasks.back().promise.get_future();
	//}

	//void thread::invoke(std::function<bool()> callback)
	//{
	//	if (operator->() == thread_ctrl::get_current())
	//	{
	//		while (true)
	//		{
	//			if (callback())
	//			{
	//				break;
	//			}
	//		}
	//	}
	//	else
	//	{
	//		add_internal_task(callback).wait();
	//	}
	//}

	namespace
	{
		bool is_int_type(rsx::vertex_base_type type)
		{
			switch (type)
			{
			case rsx::vertex_base_type::s32k:
			case rsx::vertex_base_type::ub256:
				return true;
			case rsx::vertex_base_type::f:
			case rsx::vertex_base_type::cmp:
			case rsx::vertex_base_type::sf:
			case rsx::vertex_base_type::s1:
			case rsx::vertex_base_type::ub:
				return false;
			}
		}
	}

	std::array<u32, 4> thread::get_color_surface_addresses() const
	{
		u32 offset_color[] =
		{
			rsx::method_registers[NV4097_SET_SURFACE_COLOR_AOFFSET],
			rsx::method_registers[NV4097_SET_SURFACE_COLOR_BOFFSET],
			rsx::method_registers[NV4097_SET_SURFACE_COLOR_COFFSET],
			rsx::method_registers[NV4097_SET_SURFACE_COLOR_DOFFSET]
		};
		u32 context_dma_color[] =
		{
			rsx::method_registers[NV4097_SET_CONTEXT_DMA_COLOR_A],
			rsx::method_registers[NV4097_SET_CONTEXT_DMA_COLOR_B],
			rsx::method_registers[NV4097_SET_CONTEXT_DMA_COLOR_C],
			rsx::method_registers[NV4097_SET_CONTEXT_DMA_COLOR_D]
		};
		return
		{
			rsx::get_address(offset_color[0], context_dma_color[0]),
			rsx::get_address(offset_color[1], context_dma_color[1]),
			rsx::get_address(offset_color[2], context_dma_color[2]),
			rsx::get_address(offset_color[3], context_dma_color[3]),
		};
	}

	u32 thread::get_zeta_surface_address() const
	{
		u32 m_context_dma_z = rsx::method_registers[NV4097_SET_CONTEXT_DMA_ZETA];
		u32 offset_zeta = rsx::method_registers[NV4097_SET_SURFACE_ZETA_OFFSET];
		return rsx::get_address(offset_zeta, m_context_dma_z);
	}

	RSXVertexProgram thread::get_current_vertex_program() const
	{
		RSXVertexProgram result = {};
		u32 transform_program_start = rsx::method_registers[NV4097_SET_TRANSFORM_PROGRAM_START];
		result.data.reserve((512 - transform_program_start) * 4);

		for (int i = transform_program_start; i < 512; ++i)
		{
			result.data.resize((i - transform_program_start) * 4 + 4);
			memcpy(result.data.data() + (i - transform_program_start) * 4, transform_program + i * 4, 4 * sizeof(u32));

			D3 d3;
			d3.HEX = transform_program[i * 4 + 3];

			if (d3.end)
				break;
		}
		result.output_mask = rsx::method_registers[NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK];

		u32 input_mask = rsx::method_registers[NV4097_SET_VERTEX_ATTRIB_INPUT_MASK];
		u32 modulo_mask = rsx::method_registers[NV4097_SET_FREQUENCY_DIVIDER_OPERATION];
		result.rsx_vertex_inputs.clear();
		for (u8 index = 0; index < rsx::limits::vertex_count; ++index)
		{
			bool enabled = !!(input_mask & (1 << index));
			if (!enabled)
				continue;

			if (vertex_arrays_info[index].size > 0)
			{
				result.rsx_vertex_inputs.push_back(
				{
					index,
					vertex_arrays_info[index].size,
					vertex_arrays_info[index].frequency,
					!!((modulo_mask >> index) & 0x1),
					true,
					is_int_type(vertex_arrays_info[index].type)
				}
				);
			}
			else if (register_vertex_info[index].size > 0)
			{
				result.rsx_vertex_inputs.push_back(
				{
					index,
					register_vertex_info[index].size,
					register_vertex_info[index].frequency,
					!!((modulo_mask >> index) & 0x1),
					false,
					is_int_type(vertex_arrays_info[index].type)
				}
				);
			}
		}
		return result;
	}


	RSXFragmentProgram thread::get_current_fragment_program() const
	{
		RSXFragmentProgram result = {};
		u32 shader_program = rsx::method_registers[NV4097_SET_SHADER_PROGRAM];
		result.offset = shader_program & ~0x3;
		result.addr = vm::base(rsx::get_address(result.offset, (shader_program & 0x3) - 1));
		result.ctrl = rsx::method_registers[NV4097_SET_SHADER_CONTROL];
		result.unnormalized_coords = 0;
		result.front_back_color_enabled = !rsx::method_registers[NV4097_SET_TWO_SIDE_LIGHT_EN];
		result.back_color_diffuse_output = !!(rsx::method_registers[NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK] & CELL_GCM_ATTRIB_OUTPUT_MASK_BACKDIFFUSE);
		result.back_color_specular_output = !!(rsx::method_registers[NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK] & CELL_GCM_ATTRIB_OUTPUT_MASK_BACKSPECULAR);
		result.alpha_func = to_comparaison_function(rsx::method_registers[NV4097_SET_ALPHA_FUNC]);
		result.fog_equation = rsx::to_fog_mode(rsx::method_registers[NV4097_SET_FOG_MODE]);
		u32 shader_window = rsx::method_registers[NV4097_SET_SHADER_WINDOW];
		result.origin_mode = rsx::to_window_origin((shader_window >> 12) & 0xF);
		result.pixel_center_mode = rsx::to_window_pixel_center((shader_window >> 16) & 0xF);
		result.height = shader_window & 0xFFF;

		std::array<texture_dimension_extended, 16> texture_dimensions;
		for (u32 i = 0; i < rsx::limits::textures_count; ++i)
		{
			if (!textures[i].enabled())
				texture_dimensions[i] = texture_dimension_extended::texture_dimension_2d;
			else
				texture_dimensions[i] = textures[i].get_extended_texture_dimension();
			if (textures[i].enabled() && (textures[i].format() & CELL_GCM_TEXTURE_UN))
				result.unnormalized_coords |= (1 << i);
		}
		result.set_texture_dimension(texture_dimensions);

		return result;
	}

	raw_program thread::get_raw_program() const
	{
		raw_program result{};

		u32 fp_info = rsx::method_registers[NV4097_SET_SHADER_PROGRAM];

		result.state.input_attributes = rsx::method_registers[NV4097_SET_VERTEX_ATTRIB_INPUT_MASK];
		result.state.output_attributes = rsx::method_registers[NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK];
		result.state.ctrl = rsx::method_registers[NV4097_SET_SHADER_CONTROL];
		result.state.divider_op = rsx::method_registers[NV4097_SET_FREQUENCY_DIVIDER_OPERATION];
		result.state.alpha_func = rsx::method_registers[NV4097_SET_ALPHA_FUNC];
		result.state.fog_mode = (u32)rsx::to_fog_mode(rsx::method_registers[NV4097_SET_FOG_MODE]);
		result.state.is_int = 0;

		for (u8 index = 0; index < rsx::limits::vertex_count; ++index)
		{
			bool is_int = false;

			if (vertex_arrays_info[index].size > 0)
			{
				is_int = is_int_type(vertex_arrays_info[index].type);
				result.state.frequency[index] = vertex_arrays_info[index].frequency;
			}
			else if (register_vertex_info[index].size > 0)
			{
				is_int = is_int_type(register_vertex_info[index].type);
				result.state.frequency[index] = register_vertex_info[index].frequency;
			}
			else
			{
				result.state.frequency[index] = 0;
			}

			if (is_int)
			{
				result.state.is_int |= 1 << index;
			}
		}

		for (u8 index = 0; index < rsx::limits::textures_count; ++index)
		{
			if (!textures[index].enabled())
			{
				result.state.textures_alpha_kill[index] = 0;
				result.state.textures_zfunc[index] = 0;
				result.state.textures[index] = rsx::texture_target::none;
				continue;
			}

			result.state.textures_alpha_kill[index] = textures[index].alpha_kill_enabled() ? 1 : 0;
			result.state.textures_zfunc[index] = textures[index].zfunc();

			switch (textures[index].get_extended_texture_dimension())
			{
			case rsx::texture_dimension_extended::texture_dimension_1d: result.state.textures[index] = rsx::texture_target::_1; break;
			case rsx::texture_dimension_extended::texture_dimension_2d: result.state.textures[index] = rsx::texture_target::_2; break;
			case rsx::texture_dimension_extended::texture_dimension_3d: result.state.textures[index] = rsx::texture_target::_3; break;
			case rsx::texture_dimension_extended::texture_dimension_cubemap: result.state.textures[index] = rsx::texture_target::cube; break;

			default:
				result.state.textures[index] = rsx::texture_target::none;
				break;
			}
		}

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

		// These defaults were found using After Burner Climax (which never set fog mode despite using fog input)
		method_registers[NV4097_SET_FOG_MODE] = 0x2601; // rsx::fog_mode::linear;
		(f32&)method_registers[NV4097_SET_FOG_PARAMS] = 1.;
		(f32&)method_registers[NV4097_SET_FOG_PARAMS + 1] = 1.;

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

		method_registers[NV4097_SET_CONTEXT_DMA_REPORT] = CELL_GCM_CONTEXT_DMA_TO_MEMORY_GET_REPORT;
		rsx::method_registers[NV4097_SET_TWO_SIDE_LIGHT_EN] = true;
		rsx::method_registers[NV4097_SET_ALPHA_FUNC] = CELL_GCM_ALWAYS;

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

		on_init_rsx();
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
