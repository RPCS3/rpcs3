#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "RSXThread.h"

#include "Emu/Cell/PPUCallback.h"

#include "Common/BufferUtils.h"
#include "rsx_methods.h"

#include "Utilities/GSL.h"
#include "Utilities/StrUtil.h"

#include <thread>

class GSRender;

#define CMD_DEBUG 0

bool user_asked_for_frame_capture = false;
rsx::frame_capture_data frame_debug;

namespace vm { using namespace ps3; }

namespace rsx
{
	std::function<bool(u32 addr, bool is_writing)> g_access_violation_handler;

	//TODO: Restore a working shaders cache

	u32 get_address(u32 offset, u32 location)
	{

		switch (location)
		{
		case CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER:
		case CELL_GCM_LOCATION_LOCAL:
		{
			// TODO: Don't use unnamed constants like 0xC0000000
			return 0xC0000000 + offset;
		}

		case CELL_GCM_CONTEXT_DMA_MEMORY_HOST_BUFFER:
		case CELL_GCM_LOCATION_MAIN:
		{
			if (u32 result = RSXIOMem.RealAddr(offset))
			{
				return result;
			}

			fmt::throw_exception("GetAddress(offset=0x%x, location=0x%x): RSXIO memory not mapped" HERE, offset, location);
		}

		case CELL_GCM_CONTEXT_DMA_REPORT_LOCATION_LOCAL:
			return 0x40301400 + offset;

		case CELL_GCM_CONTEXT_DMA_REPORT_LOCATION_MAIN:
		{
			if (u32 result = RSXIOMem.RealAddr(0x0e000000 + offset))
			{
				return result;
			}

			fmt::throw_exception("GetAddress(offset=0x%x, location=0x%x): RSXIO memory not mapped" HERE, offset, location);
		}

		case CELL_GCM_CONTEXT_DMA_TO_MEMORY_GET_NOTIFY0:
			fmt::throw_exception("Unimplemented CELL_GCM_CONTEXT_DMA_TO_MEMORY_GET_NOTIFY0 (offset=0x%x, location=0x%x)" HERE, offset, location);

		case CELL_GCM_CONTEXT_DMA_NOTIFY_MAIN_0:
			fmt::throw_exception("Unimplemented CELL_GCM_CONTEXT_DMA_NOTIFY_MAIN_0 (offset=0x%x, location=0x%x)" HERE, offset, location);

		case CELL_GCM_CONTEXT_DMA_SEMAPHORE_RW:
		case CELL_GCM_CONTEXT_DMA_SEMAPHORE_R:
			return 0x40300000 + offset;

		case CELL_GCM_CONTEXT_DMA_DEVICE_RW:
			return 0x40000000 + offset;

		case CELL_GCM_CONTEXT_DMA_DEVICE_R:
			return 0x40000000 + offset;

		default:
			fmt::throw_exception("Invalid location (offset=0x%x, location=0x%x)" HERE, offset, location);
		}
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
			fmt::throw_exception("Wrong vector size" HERE);
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
			fmt::throw_exception("Wrong vector size" HERE);
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
			fmt::throw_exception("Wrong vector size" HERE);
		case vertex_base_type::cmp: return 4;
		case vertex_base_type::ub256: verify(HERE), (size == 4); return sizeof(u8) * 4;
		}
		fmt::throw_exception("RSXVertexData::GetTypeSize: Bad vertex data type (%d)!" HERE, (u8)type);
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
			for (u32 y = 0; y < height; ++y)
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
			::narrow(tile->comp, "tile->comp" HERE);
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
			for (u32 y = 0; y < height; ++y)
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
			::narrow(tile->comp, "tile->comp" HERE);
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

		int clip_w = rsx::method_registers.surface_clip_width();
		int clip_h = rsx::method_registers.surface_clip_height();
		draw_state.state = rsx::method_registers;
		draw_state.color_buffer = std::move(copy_render_targets_to_memory());
		draw_state.depth_stencil = std::move(copy_depth_stencil_buffer_to_memory());

		if (draw_state.state.current_draw_clause.command == rsx::draw_command::indexed)
		{
			draw_state.vertex_count = 0;
			draw_state.vertex_count = draw_state.state.current_draw_clause.get_elements_count();
			auto index_raw_data_ptr = get_raw_index_array(draw_state.state.current_draw_clause.first_count_commands);
			draw_state.index.resize(index_raw_data_ptr.size_bytes());
			std::copy(index_raw_data_ptr.begin(), index_raw_data_ptr.end(), draw_state.index.begin());
		}

		draw_state.programs = get_programs();
		draw_state.name = name;
		frame_debug.draw_calls.push_back(draw_state);
	}

	void thread::begin()
	{
		rsx::method_registers.current_draw_clause.inline_vertex_array.resize(0);
		in_begin_end = true;
	}

	void thread::append_to_push_buffer(u32 attribute, u32 size, u32 subreg_index, vertex_base_type type, u32 value)
	{
		vertex_push_buffers[attribute].size = size;
		vertex_push_buffers[attribute].append_vertex_data(subreg_index, type, value);
	}

	u32 thread::get_push_buffer_vertex_count() const
	{
		//There's no restriction on which attrib shall hold vertex data, so we check them all
		u32 max_vertex_count = 0;
		for (auto &buf: vertex_push_buffers)
		{
			max_vertex_count = std::max(max_vertex_count, buf.vertex_count);
		}

		return max_vertex_count;
	}

	void thread::append_array_element(u32 index)
	{
		//Endianness is swapped because common upload code expects input in BE
		//TODO: Implement fast upload path for LE inputs and do away with this
		element_push_buffer.push_back(se_storage<u32>::swap(index));
	}

	u32 thread::get_push_buffer_index_count() const
	{
		return (u32)element_push_buffer.size();
	}

	void thread::end()
	{
		rsx::method_registers.transform_constants.clear();
		in_begin_end = false;

		for (u8 index = 0; index < rsx::limits::vertex_count; ++index)
		{
			//Disabled, see https://github.com/RPCS3/rpcs3/issues/1932
			//rsx::method_registers.register_vertex_info[index].size = 0;

			vertex_push_buffers[index].clear();
		}

		element_push_buffer.resize(0);

		if (capture_current_frame)
		{
			u32 element_count = rsx::method_registers.current_draw_clause.get_elements_count();
			capture_frame("Draw " + rsx::to_string(rsx::method_registers.current_draw_clause.primitive) + std::to_string(element_count));
		}
	}

	void thread::on_task()
	{
		on_init_thread();

		reset();

		last_flip_time = get_system_time() - 1000000;

		thread_ctrl::spawn(m_vblank_thread, "VBlank Thread", [this]()
		{
			const u64 start_time = get_system_time();

			vblank_count = 0;

			// TODO: exit condition
			while (!Emu.IsStopped())
			{
				if (get_system_time() - start_time > vblank_count * 1000000 / 60)
				{
					vblank_count++;
					sys_rsx_context_attribute(0x55555555, 0xFED, 1, 0, 0, 0);
					if (vblank_handler)
					{
						intr_thread->cmd_list
						({
							{ ppu_cmd::set_args, 1 }, u64{1},
							{ ppu_cmd::lle_call, vblank_handler },
							{ ppu_cmd::sleep, 0 }
						});

						intr_thread->notify();
					}

					continue;
				}
				while (Emu.IsPaused())
					std::this_thread::sleep_for(10ms);

				std::this_thread::sleep_for(1ms); // hack
			}
		});

		// Raise priority above other threads
		thread_ctrl::set_native_priority(1);

		// TODO: exit condition
		while (!Emu.IsStopped())
		{
			//Execute backend-local tasks first
			do_local_task();

			const u32 get = ctrl->get;
			const u32 put = ctrl->put;

			if (put == get || !Emu.IsRunning())
			{
				do_internal_task();
				continue;
			}

			//Validate put and get registers
			//TODO: Who should handle graphics exceptions??
			const u32 get_address = RSXIOMem.RealAddr(get);
			
			if (!get_address)
			{
				LOG_ERROR(RSX, "Invalid FIFO queue get/put registers found, get=0x%X, put=0x%X", get, put);

				invalid_command_interrupt_raised = true;
				ctrl->get = put;
				continue;
			}

			const u32 cmd = ReadIO32(get);
			const u32 count = (cmd >> 18) & 0x7ff;

			if ((cmd & RSX_METHOD_OLD_JUMP_CMD_MASK) == RSX_METHOD_OLD_JUMP_CMD)
			{
				u32 offs = cmd & 0x1ffffffc;
				//LOG_WARNING(RSX, "rsx jump(0x%x) #addr=0x%x, cmd=0x%x, get=0x%x, put=0x%x", offs, m_ioAddress + get, cmd, get, put);
				ctrl->get = offs;
				continue;
			}
			if ((cmd & RSX_METHOD_NEW_JUMP_CMD_MASK) == RSX_METHOD_NEW_JUMP_CMD)
			{
				u32 offs = cmd & 0xfffffffc;
				//LOG_WARNING(RSX, "rsx jump(0x%x) #addr=0x%x, cmd=0x%x, get=0x%x, put=0x%x", offs, m_ioAddress + get, cmd, get, put);
				ctrl->get = offs;
				continue;
			}
			if ((cmd & RSX_METHOD_CALL_CMD_MASK) == RSX_METHOD_CALL_CMD)
			{
				m_call_stack.push(get + 4);
				u32 offs = cmd & ~3;
				//LOG_WARNING(RSX, "rsx call(0x%x) #0x%x - 0x%x", offs, cmd, get);
				ctrl->get = offs;
				continue;
			}
			if (cmd == RSX_METHOD_RETURN_CMD)
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

			//Validate the args ptr if the command attempts to read from it
			const u32 args_address = RSXIOMem.RealAddr(get + 4);

			if (!args_address && count)
			{
				LOG_ERROR(RSX, "Invalid FIFO queue args ptr found, get=0x%X, cmd=0x%X, count=%d", get, cmd, count);

				invalid_command_interrupt_raised = true;
				ctrl->get = put;
				continue;
			}

			auto args = vm::ptr<u32>::make(args_address);
			invalid_command_interrupt_raised = false;
			bool unaligned_command = false;

			u32 first_cmd = (cmd & 0xfffc) >> 2;

			if (cmd & 0x3)
			{
				LOG_WARNING(RSX, "unaligned command: %s (0x%x from 0x%x)", get_method_name(first_cmd).c_str(), first_cmd, cmd & 0xffff);
				unaligned_command = true;
			}

			for (u32 i = 0; i < count; i++)
			{
				u32 reg = ((cmd & RSX_METHOD_NON_INCREMENT_CMD_MASK) == RSX_METHOD_NON_INCREMENT_CMD) ? first_cmd : first_cmd + i;
				u32 value = args[i];

				//LOG_NOTICE(RSX, "%s(0x%x) = 0x%x", get_method_name(reg).c_str(), reg, value);
				method_registers.decode(reg, value);

				if (capture_current_frame)
				{
					frame_debug.command_queue.push_back(std::make_pair(reg, value));
				}

				if (auto method = methods[reg])
				{
					method(this, reg, value);
				}

				if (invalid_command_interrupt_raised)
				{
					//Skip the rest of this command
					break;
				}
			}

			if (unaligned_command && invalid_command_interrupt_raised)
			{
				//This is almost guaranteed to be heap corruption at this point
				//Ignore the rest of the chain
				ctrl->get = put;
				continue;
			}

			ctrl->get = get + (count + 1) * 4;
		}
	}

	void thread::on_exit()
	{
		if (m_vblank_thread)
		{
			m_vblank_thread->join();
			m_vblank_thread.reset();
		}
	}

	std::string thread::get_name() const
	{
		return "rsx::thread";
	}

	void thread::fill_scale_offset_data(void *buffer, bool flip_y) const
	{
		int clip_w = rsx::method_registers.surface_clip_width();
		int clip_h = rsx::method_registers.surface_clip_height();

		float scale_x = rsx::method_registers.viewport_scale_x() / (clip_w / 2.f);
		float offset_x = rsx::method_registers.viewport_offset_x() - (clip_w / 2.f);
		offset_x /= clip_w / 2.f;

		float scale_y = rsx::method_registers.viewport_scale_y() / (clip_h / 2.f);
		float offset_y = (rsx::method_registers.viewport_offset_y() - (clip_h / 2.f));
		offset_y /= clip_h / 2.f;
		if (flip_y) scale_y *= -1;
		if (flip_y) offset_y *= -1;

		float scale_z = rsx::method_registers.viewport_scale_z();
		float offset_z = rsx::method_registers.viewport_offset_z();
		float one = 1.f;

		stream_vector(buffer, (u32&)scale_x, 0, 0, (u32&)offset_x);
		stream_vector((char*)buffer + 16, 0, (u32&)scale_y, 0, (u32&)offset_y);
		stream_vector((char*)buffer + 32, 0, 0, (u32&)scale_z, (u32&)offset_z);
		stream_vector((char*)buffer + 48, 0, 0, 0, (u32&)one);
	}

	void thread::fill_user_clip_data(void *buffer) const
	{
		const rsx::user_clip_plane_op clip_plane_control[6] =
		{
			rsx::method_registers.clip_plane_0_enabled(),
			rsx::method_registers.clip_plane_1_enabled(),
			rsx::method_registers.clip_plane_2_enabled(),
			rsx::method_registers.clip_plane_3_enabled(),
			rsx::method_registers.clip_plane_4_enabled(),
			rsx::method_registers.clip_plane_5_enabled(),
		};

		s32 clip_enabled_flags[8] = {};
		f32 clip_distance_factors[8] = {};

		for (int index = 0; index < 6; ++index)
		{
			switch (clip_plane_control[index])
			{
			default:
				LOG_ERROR(RSX, "bad clip plane control (0x%x)", (u8)clip_plane_control[index]);

			case rsx::user_clip_plane_op::disable:
				clip_enabled_flags[index] = 0;
				clip_distance_factors[index] = 0.f;
				break;

			case rsx::user_clip_plane_op::greater_or_equal:
				clip_enabled_flags[index] = 1;
				clip_distance_factors[index] = 1.f;
				break;

			case rsx::user_clip_plane_op::less_than:
				clip_enabled_flags[index] = 1;
				clip_distance_factors[index] = -1.f;
				break;
			}
		}

		memcpy(buffer, clip_enabled_flags, 32);
		memcpy((char*)buffer + 32, clip_distance_factors, 32);
	}

	/**
	* Fill buffer with vertex program constants.
	* Buffer must be at least 512 float4 wide.
	*/
	void thread::fill_vertex_program_constants_data(void *buffer)
	{
		//Some games dont initialize some registers that they use in the vertex stage
		memset(buffer, 0, 512 * 4 * sizeof(float));

		for (const auto &entry : rsx::method_registers.transform_constants)
			local_transform_constants[entry.first] = entry.second;
		for (const auto &entry : local_transform_constants)
			stream_vector_from_memory((char*)buffer + entry.first * 4 * sizeof(float), (void*)entry.second.rgba);
	}

	void thread::fill_fragment_state_buffer(void *buffer, const RSXFragmentProgram &fragment_program)
	{
		const u32 is_alpha_tested = rsx::method_registers.alpha_test_enabled();
		const float alpha_ref = rsx::method_registers.alpha_ref() / 255.f;
		const f32 fog0 = rsx::method_registers.fog_params_0();
		const f32 fog1 = rsx::method_registers.fog_params_1();
		const u32 alpha_func = static_cast<u32>(rsx::method_registers.alpha_func());
		const u32 fog_mode = static_cast<u32>(rsx::method_registers.fog_equation());
		const u32 window_origin = static_cast<u32>(rsx::method_registers.shader_window_origin());
		const u32 window_height = rsx::method_registers.shader_window_height();
		const float one = 1.f;

		u32 *dst = static_cast<u32*>(buffer);

		stream_vector(dst, (u32&)fog0, (u32&)fog1, is_alpha_tested, (u32&)alpha_ref);
		stream_vector(dst + 4, alpha_func, fog_mode, window_origin, window_height);

		size_t offset = 8;
		for (int index = 0; index < 16; ++index)
		{
			stream_vector(&dst[offset], (u32&)fragment_program.texture_pitch_scale[index], (u32&)one, 0U, 0U);
			offset += 4;
		}
	}

	void thread::write_inline_array_to_buffer(void *dst_buffer)
	{
		u8* src =
			reinterpret_cast<u8*>(rsx::method_registers.current_draw_clause.inline_vertex_array.data());
		u8* dst = (u8*)dst_buffer;

		size_t bytes_written = 0;
		while (bytes_written <
			   rsx::method_registers.current_draw_clause.inline_vertex_array.size() * sizeof(u32))
		{
			for (int index = 0; index < rsx::limits::vertex_count; ++index)
			{
				const auto &info = rsx::method_registers.vertex_arrays_info[index];

				if (!info.size()) // disabled
					continue;

				u32 element_size = rsx::get_vertex_type_size_on_host(info.type(), info.size());

				if (info.type() == vertex_base_type::ub && info.size() == 4)
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

	gsl::span<const gsl::byte> thread::get_raw_index_array(const std::vector<std::pair<u32, u32> >& draw_indexed_clause) const
	{
		if (element_push_buffer.size())
		{
			//Indices provided via immediate mode
			return{(const gsl::byte*)element_push_buffer.data(), ::narrow<u32>(element_push_buffer.size() * sizeof(u32))};
		}

		u32 address = rsx::get_address(rsx::method_registers.index_array_address(), rsx::method_registers.index_array_location());
		rsx::index_array_type type = rsx::method_registers.index_type();

		u32 type_size = ::narrow<u32>(get_index_type_size(type));
		bool is_primitive_restart_enabled = rsx::method_registers.restart_index_enabled();
		u32 primitive_restart_index = rsx::method_registers.restart_index();

		// Disjoint first_counts ranges not supported atm
		for (int i = 0; i < draw_indexed_clause.size() - 1; i++)
		{
			const std::tuple<u32, u32> &range = draw_indexed_clause[i];
			const std::tuple<u32, u32> &next_range = draw_indexed_clause[i + 1];
			verify(HERE), (std::get<0>(range) + std::get<1>(range) == std::get<0>(next_range));
		}
		u32 first = std::get<0>(draw_indexed_clause.front());
		u32 count = std::get<0>(draw_indexed_clause.back()) + std::get<1>(draw_indexed_clause.back()) - first;

		const gsl::byte* ptr = static_cast<const gsl::byte*>(vm::base(address));
		return{ ptr + first * type_size, count * type_size };
	}

	gsl::span<const gsl::byte> thread::get_raw_vertex_buffer(const rsx::data_array_format_info& vertex_array_info, u32 base_offset, const std::vector<std::pair<u32, u32>>& vertex_ranges) const
	{
		u32 offset  = vertex_array_info.offset();
		u32 address = base_offset + rsx::get_address(offset & 0x7fffffff, offset >> 31);

		u32 element_size = rsx::get_vertex_type_size_on_host(vertex_array_info.type(), vertex_array_info.size());

		// Disjoint first_counts ranges not supported atm
		for (int i = 0; i < vertex_ranges.size() - 1; i++)
		{
			const std::tuple<u32, u32>& range      = vertex_ranges[i];
			const std::tuple<u32, u32>& next_range = vertex_ranges[i + 1];
			verify(HERE), (std::get<0>(range) + std::get<1>(range) == std::get<0>(next_range));
		}
		u32 first = std::get<0>(vertex_ranges.front());
		u32 count = std::get<0>(vertex_ranges.back()) + std::get<1>(vertex_ranges.back()) - first;

		const gsl::byte* ptr = gsl::narrow_cast<const gsl::byte*>(vm::base(address));
		return {ptr + first * vertex_array_info.stride(), count * vertex_array_info.stride() + element_size};
	}

	std::vector<std::variant<vertex_array_buffer, vertex_array_register, empty_vertex_array>>
	thread::get_vertex_buffers(const rsx::rsx_state& state, const std::vector<std::pair<u32, u32>>& vertex_ranges, const u64 consumed_attrib_mask) const
	{
		std::vector<std::variant<vertex_array_buffer, vertex_array_register, empty_vertex_array>> result;
		result.reserve(rsx::limits::vertex_count);

		u32 input_mask = state.vertex_attrib_input_mask();
		for (u8 index = 0; index < rsx::limits::vertex_count; ++index)
		{
			const bool enabled = !!(input_mask & (1 << index));
			const bool consumed = !!(consumed_attrib_mask & (1ull << index));

			if (!enabled && !consumed)
				continue;

			if (state.vertex_arrays_info[index].size() > 0)
			{
				const rsx::data_array_format_info& info = state.vertex_arrays_info[index];
				result.push_back(vertex_array_buffer{info.type(), info.size(), info.stride(),
					get_raw_vertex_buffer(info, state.vertex_data_base_offset(), vertex_ranges), index});
				continue;
			}

			if (vertex_push_buffers[index].vertex_count > 1)
			{
				const rsx::register_vertex_data_info& info = state.register_vertex_info[index];
				const u8 element_size = info.size * sizeof(u32);

				gsl::span<const gsl::byte> vertex_src = { (const gsl::byte*)vertex_push_buffers[index].data.data(), vertex_push_buffers[index].vertex_count * element_size };
				result.push_back(vertex_array_buffer{ info.type, info.size, element_size, vertex_src, index });
				continue;
			}

			if (state.register_vertex_info[index].size > 0)
			{
				const rsx::register_vertex_data_info& info = state.register_vertex_info[index];
				result.push_back(vertex_array_register{info.type, info.size, info.data, index});
				continue;
			}

			result.push_back(empty_vertex_array{index});
		}

		return result;
	}

	std::variant<draw_array_command, draw_indexed_array_command, draw_inlined_array>
	thread::get_draw_command(const rsx::rsx_state& state) const
	{
		if (rsx::method_registers.current_draw_clause.command == rsx::draw_command::array) {
			return draw_array_command{
				rsx::method_registers.current_draw_clause.first_count_commands};
		}

		if (rsx::method_registers.current_draw_clause.command == rsx::draw_command::indexed) {
			return draw_indexed_array_command{
				rsx::method_registers.current_draw_clause.first_count_commands,
				get_raw_index_array(
					rsx::method_registers.current_draw_clause.first_count_commands)};
		}

		if (rsx::method_registers.current_draw_clause.command == rsx::draw_command::inlined_array) {
			return draw_inlined_array{
				rsx::method_registers.current_draw_clause.inline_vertex_array};
		}

		fmt::throw_exception("ill-formed draw command" HERE);
	}

	void thread::do_internal_task()
	{
		if (m_internal_tasks.empty())
		{
			std::this_thread::yield();
		}
		else
		{
			fmt::throw_exception("Disabled" HERE);
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
	//	if (get() == thread_ctrl::get_current())
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
			}

			return false;
		}
	}

	std::array<u32, 4> thread::get_color_surface_addresses() const
	{
		u32 offset_color[] =
		{
			rsx::method_registers.surface_a_offset(),
			rsx::method_registers.surface_b_offset(),
			rsx::method_registers.surface_c_offset(),
			rsx::method_registers.surface_d_offset(),
		};
		u32 context_dma_color[] =
		{
			rsx::method_registers.surface_a_dma(),
			rsx::method_registers.surface_b_dma(),
			rsx::method_registers.surface_c_dma(),
			rsx::method_registers.surface_d_dma(),
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
		u32 m_context_dma_z = rsx::method_registers.surface_z_dma();
		u32 offset_zeta = rsx::method_registers.surface_z_offset();
		return rsx::get_address(offset_zeta, m_context_dma_z);
	}

	void thread::get_current_vertex_program()
	{
		auto &result = current_vertex_program = {};

		const u32 transform_program_start = rsx::method_registers.transform_program_start();
		result.data.reserve((512 - transform_program_start) * 4);
		result.rsx_vertex_inputs.reserve(rsx::limits::vertex_count);

		for (int i = transform_program_start; i < 512; ++i)
		{
			result.data.resize((i - transform_program_start) * 4 + 4);
			memcpy(result.data.data() + (i - transform_program_start) * 4, rsx::method_registers.transform_program.data() + i * 4, 4 * sizeof(u32));

			D3 d3;
			d3.HEX = rsx::method_registers.transform_program[i * 4 + 3];

			if (d3.end)
				break;
		}
		result.output_mask = rsx::method_registers.vertex_attrib_output_mask();

		const u32 input_mask = rsx::method_registers.vertex_attrib_input_mask();
		const u32 modulo_mask = rsx::method_registers.frequency_divider_operation_mask();

		for (u8 index = 0; index < rsx::limits::vertex_count; ++index)
		{
			bool enabled = !!(input_mask & (1 << index));
			if (!enabled)
				continue;

			if (rsx::method_registers.vertex_arrays_info[index].size() > 0)
			{
				result.rsx_vertex_inputs.push_back(
					{index,
						rsx::method_registers.vertex_arrays_info[index].size(),
						rsx::method_registers.vertex_arrays_info[index].frequency(),
						!!((modulo_mask >> index) & 0x1),
						true,
						is_int_type(rsx::method_registers.vertex_arrays_info[index].type()), 0});
			}
			else if (vertex_push_buffers[index].vertex_count > 1)
			{
				result.rsx_vertex_inputs.push_back(
				{ index,
					rsx::method_registers.register_vertex_info[index].size,
					1,
					false,
					true,
					is_int_type(rsx::method_registers.vertex_arrays_info[index].type()), 0 });
			}
			else if (rsx::method_registers.register_vertex_info[index].size > 0)
			{
				result.rsx_vertex_inputs.push_back(
					{index,
						rsx::method_registers.register_vertex_info[index].size,
						rsx::method_registers.register_vertex_info[index].frequency,
						!!((modulo_mask >> index) & 0x1),
						false,
						is_int_type(rsx::method_registers.vertex_arrays_info[index].type()), 0});
			}
		}
	}

	vertex_input_layout thread::analyse_inputs_interleaved() const
	{
		const rsx_state& state = rsx::method_registers;
		const u32 input_mask = state.vertex_attrib_input_mask();

		if (state.current_draw_clause.command == rsx::draw_command::inlined_array)
		{
			vertex_input_layout result = {};

			interleaved_range_info info = {};
			info.interleaved = true;
			info.locations.reserve(8);

			for (u8 index = 0; index < rsx::limits::vertex_count; ++index)
			{
				const u32 mask = (1u << index);
				auto &vinfo = state.vertex_arrays_info[index];

				if (vinfo.size() > 0)
				{
					info.locations.push_back(index);
					info.attribute_stride += rsx::get_vertex_type_size_on_host(vinfo.type(), vinfo.size());
					result.attribute_placement[index] = attribute_buffer_placement::transient;
				}
			}

			result.interleaved_blocks.push_back(info);
			return result;
		}

		const u32 frequency_divider_mask = rsx::method_registers.frequency_divider_operation_mask();
		vertex_input_layout result = {};

		for (u8 index = 0; index < rsx::limits::vertex_count; ++index)
		{
			const bool enabled = !!(input_mask & (1 << index));
			if (!enabled)
				continue;

			if (vertex_push_buffers[index].size > 0)
			{
				std::pair<u8, u32> volatile_range_info = std::make_pair(index, static_cast<u32>(vertex_push_buffers[index].data.size() * sizeof(u32)));
				result.volatile_blocks.push_back(volatile_range_info);
				result.attribute_placement[index] = attribute_buffer_placement::transient;
				continue;
			}

			//Check for interleaving
			auto &info = state.vertex_arrays_info[index];
			if (info.size() == 0 && state.register_vertex_info[index].size > 0)
			{
				//Reads from register
				result.referenced_registers.push_back(index);
				result.attribute_placement[index] = attribute_buffer_placement::transient;
				continue;
			}

			if (info.size() > 0)
			{
				result.attribute_placement[index] = attribute_buffer_placement::persistent;
				const u32 base_address = info.offset() & 0x7fffffff;
				bool alloc_new_block = true;

				for (auto &block : result.interleaved_blocks)
				{
					if (block.single_vertex)
					{
						//Single vertex definition, continue
						continue;
					}

					if (block.attribute_stride != info.stride())
					{
						//Stride does not match, continue
						continue;
					}

					if (base_address > block.base_offset)
					{
						const u32 diff = base_address - block.base_offset;
						if (diff > info.stride())
						{
							//Not interleaved, continue
							continue;
						}
					}
					else
					{
						const u32 diff = block.base_offset - base_address;
						if (diff > info.stride())
						{
							//Not interleaved, continue
							continue;
						}

						//Matches, and this address is lower than existing
						block.base_offset = base_address;
					}

					alloc_new_block = false;
					block.locations.push_back(index);
					block.interleaved = true;
					block.min_divisor = std::min(block.min_divisor, info.frequency());

					if (block.all_modulus)
						block.all_modulus = !!(frequency_divider_mask & (1 << index));

					break;
				}

				if (alloc_new_block)
				{
					interleaved_range_info block = {};
					block.base_offset = base_address;
					block.attribute_stride = info.stride();
					block.memory_location = info.offset() >> 31;
					block.locations.reserve(4);
					block.locations.push_back(index);
					block.min_divisor = info.frequency();
					block.all_modulus = !!(frequency_divider_mask & (1 << index));

					if (block.attribute_stride == 0)
					{
						block.single_vertex = true;
						block.attribute_stride = rsx::get_vertex_type_size_on_host(info.type(), info.size());
					}

					result.interleaved_blocks.push_back(block);
				}
			}
		}

		for (auto &info : result.interleaved_blocks)
		{
			//Calculate real data address to be used during upload
			info.real_offset_address = state.vertex_data_base_offset() + rsx::get_address(info.base_offset, info.memory_location);
		}

		return result;
	}

	void thread::get_current_fragment_program(std::function<std::tuple<bool, u16>(u32, fragment_texture&, bool)> get_surface_info)
	{
		auto &result = current_fragment_program = {};

		const u32 shader_program = rsx::method_registers.shader_program_address();
		if (shader_program == 0)
			return;

		const u32 program_location = (shader_program & 0x3) - 1;
		const u32 program_offset = (shader_program & ~0x3);

		result.offset = program_offset;
		result.addr = vm::base(rsx::get_address(program_offset, program_location));
		result.valid = true;
		result.ctrl = rsx::method_registers.shader_control();
		result.unnormalized_coords = 0;
		result.front_back_color_enabled = !rsx::method_registers.two_side_light_en();
		result.back_color_diffuse_output = !!(rsx::method_registers.vertex_attrib_output_mask() & CELL_GCM_ATTRIB_OUTPUT_MASK_BACKDIFFUSE);
		result.back_color_specular_output = !!(rsx::method_registers.vertex_attrib_output_mask() & CELL_GCM_ATTRIB_OUTPUT_MASK_BACKSPECULAR);
		result.front_color_diffuse_output = !!(rsx::method_registers.vertex_attrib_output_mask() & CELL_GCM_ATTRIB_OUTPUT_MASK_FRONTDIFFUSE);
		result.front_color_specular_output = !!(rsx::method_registers.vertex_attrib_output_mask() & CELL_GCM_ATTRIB_OUTPUT_MASK_FRONTSPECULAR);
		result.redirected_textures = 0;
		result.shadow_textures = 0;

		std::array<texture_dimension_extended, 16> texture_dimensions;
		for (u32 i = 0; i < rsx::limits::fragment_textures_count; ++i)
		{
			auto &tex = rsx::method_registers.fragment_textures[i];
			result.texture_pitch_scale[i] = 1.f;
			result.textures_alpha_kill[i] = 0;
			result.textures_zfunc[i] = 0;

			if (!tex.enabled())
			{
				texture_dimensions[i] = texture_dimension_extended::texture_dimension_2d;
			}
			else
			{
				texture_dimensions[i] = tex.get_extended_texture_dimension();

				if (tex.alpha_kill_enabled())
				{
					//alphakill can be ignored unless a valid comparison function is set
					const rsx::comparison_function func = (rsx::comparison_function)tex.zfunc();
					if (func < rsx::comparison_function::always && func > rsx::comparison_function::never)
					{
						result.textures_alpha_kill[i] = 1;
						result.textures_zfunc[i] = (u8)func;
					}
				}

				const u32 texaddr = rsx::get_address(tex.offset(), tex.location());
				const u32 raw_format = tex.format();

				if (raw_format & CELL_GCM_TEXTURE_UN)
					result.unnormalized_coords |= (1 << i);

				bool surface_exists;
				u16  surface_pitch;

				std::tie(surface_exists, surface_pitch) = get_surface_info(texaddr, tex, false);

				if (surface_exists && surface_pitch)
				{
					if (raw_format & CELL_GCM_TEXTURE_UN)
						result.texture_pitch_scale[i] = (float)surface_pitch / tex.pitch();
				}
				else
				{
					std::tie(surface_exists, surface_pitch) = get_surface_info(texaddr, tex, true);
					if (surface_exists)
					{
						u32 format = raw_format & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
						switch (format)
						{
						case CELL_GCM_TEXTURE_A8R8G8B8:
						case CELL_GCM_TEXTURE_D8R8G8B8:
						case CELL_GCM_TEXTURE_A4R4G4B4:
						case CELL_GCM_TEXTURE_R5G6B5:
								result.redirected_textures |= (1 << i);
								break;
						case CELL_GCM_TEXTURE_DEPTH16:
						case CELL_GCM_TEXTURE_DEPTH24_D8:
						case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
								result.shadow_textures |= (1 << i);
								break;
						default:
								LOG_ERROR(RSX, "Depth texture bound to pipeline with unexpected format 0x%X", format);
						}
					}
				}
			}
		}

		result.set_texture_dimension(texture_dimensions);
	}

	void thread::reset()
	{
		rsx::method_registers.reset();
	}

	void thread::init(u32 ioAddress, u32 ioSize, u32 ctrlAddress, u32 localAddress)
	{
		ctrl = vm::_ptr<RsxDmaControl>(ctrlAddress);
		this->ioAddress = ioAddress;
		this->ioSize = ioSize;
		local_mem_addr = localAddress;
		flip_status = CELL_GCM_DISPLAY_FLIP_STATUS_DONE;

		m_used_gcm_commands.clear();

		on_init_rsx();
		start_thread(fxm::get<GSRender>());
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
			fmt::throw_exception("%s(addr=0x%x): RSXIO memory not mapped" HERE, __FUNCTION__, addr);
		}

		return value;
	}

	void thread::WriteIO32(u32 addr, u32 value)
	{
		if (!RSXIOMem.Write32(addr, value))
		{
			fmt::throw_exception("%s(addr=0x%x): RSXIO memory not mapped" HERE, __FUNCTION__, addr);
		}
	}

	std::pair<u32, u32> thread::calculate_memory_requirements(vertex_input_layout& layout, const u32 vertex_count)
	{
		u32 persistent_memory_size = 0;
		u32 volatile_memory_size = 0;

		volatile_memory_size += (u32)layout.referenced_registers.size() * 16u;

		if (rsx::method_registers.current_draw_clause.command == rsx::draw_command::inlined_array)
		{
			for (const auto &block : layout.interleaved_blocks)
			{
				volatile_memory_size += block.attribute_stride * vertex_count;
			}
		}
		else
		{
			//NOTE: Immediate commands can be index array only or both index array and vertex data
			//Check both - but only check volatile blocks if immediate_draw flag is set
			if (rsx::method_registers.current_draw_clause.is_immediate_draw)
			{
				for (const auto &info : layout.volatile_blocks)
				{
					volatile_memory_size += info.second;
				}
			}

			for (const auto &block : layout.interleaved_blocks)
			{
				u32 unique_verts;

				if (block.single_vertex)
				{
					unique_verts = 1;
				}
				else if (block.min_divisor > 1)
				{
					if (block.all_modulus)
						unique_verts = block.min_divisor;
					else
					{
						unique_verts = vertex_count / block.min_divisor;
						if (vertex_count % block.min_divisor) unique_verts++;
					}
				}
				else
				{
					unique_verts = vertex_count;
				}

				persistent_memory_size += block.attribute_stride * unique_verts;
			}
		}

		return std::make_pair(persistent_memory_size, volatile_memory_size);
	}

	void thread::fill_vertex_layout_state(vertex_input_layout& layout, const u32 vertex_count, s32* buffer)
	{
		std::array<s32, 16> offset_in_block = {};
		u32 volatile_offset = 0;
		u32 persistent_offset = 0;

		if (rsx::method_registers.current_draw_clause.is_immediate_draw)
		{
			for (const auto &info : layout.volatile_blocks)
			{
				offset_in_block[info.first] = volatile_offset;
				volatile_offset += info.second;
			}
		}

		for (u8 index : layout.referenced_registers)
		{
			offset_in_block[index] = volatile_offset;
			volatile_offset += 16;
		}

		if (rsx::method_registers.current_draw_clause.command == rsx::draw_command::inlined_array)
		{
			const auto &block = layout.interleaved_blocks[0];
			for (const u8 index : block.locations)
			{
				auto &info = rsx::method_registers.vertex_arrays_info[index];

				offset_in_block[index] = persistent_offset; //just because this var is 0 when we enter here; inlined is transient memory
				persistent_offset += rsx::get_vertex_type_size_on_host(info.type(), info.size());
			}
		}
		else
		{
			for (const auto &block : layout.interleaved_blocks)
			{
				for (u8 index : block.locations)
				{
					const u32 local_address = (rsx::method_registers.vertex_arrays_info[index].offset() & 0x7fffffff);
					offset_in_block[index] = persistent_offset + (local_address - block.base_offset);
				}

				u32 unique_verts;

				if (block.single_vertex)
				{
					unique_verts = 1;
				}
				else if (block.min_divisor > 1)
				{
					if (block.all_modulus)
						unique_verts = block.min_divisor;
					else
					{
						unique_verts = vertex_count / block.min_divisor;
						if (vertex_count % block.min_divisor) unique_verts++;
					}
				}
				else
				{
					unique_verts = vertex_count;
				}

				persistent_offset += block.attribute_stride * unique_verts;
			}
		}

		//Fill the data
		memset(buffer, 0, 256);

		const s32 swap_storage_mask = (1 << 8);
		const s32 volatile_storage_mask = (1 << 9);
		const s32 default_frequency_mask = (1 << 10);
		const s32 repeating_frequency_mask = (3 << 10);
		const s32 input_function_modulo_mask = (1 << 12);
		const s32 input_divisor_mask = (0xFFFF << 13);

		const u32 modulo_mask = rsx::method_registers.frequency_divider_operation_mask();

		for (u8 index = 0; index < rsx::limits::vertex_count; ++index)
		{
			if (layout.attribute_placement[index] == attribute_buffer_placement::none)
				continue;

			rsx::vertex_base_type type = {};
			s32 size = 0;
			s32 attributes = 0;

			bool is_be_type = true;

			if (layout.attribute_placement[index] == attribute_buffer_placement::transient)
			{
				if (rsx::method_registers.current_draw_clause.command == rsx::draw_command::inlined_array)
				{
					auto &info = rsx::method_registers.vertex_arrays_info[index];
					type = info.type();
					size = info.size();

					attributes = layout.interleaved_blocks[0].attribute_stride;
					attributes |= default_frequency_mask | volatile_storage_mask;

					is_be_type = false;
				}
				else
				{
					//Data is either from an immediate render or register input
					//Immediate data overrides register input

					if (rsx::method_registers.current_draw_clause.is_immediate_draw && vertex_push_buffers[index].size > 0)
					{
						const auto &info = rsx::method_registers.register_vertex_info[index];
						type = info.type;
						size = info.size;

						attributes = rsx::get_vertex_type_size_on_host(type, size);
						attributes |= default_frequency_mask | volatile_storage_mask;

						is_be_type = true;
					}
					else
					{
						//Register
						const auto& info = rsx::method_registers.register_vertex_info[index];
						type = info.type;
						size = info.size;

						attributes = rsx::get_vertex_type_size_on_host(type, size);
						attributes |= volatile_storage_mask;

						is_be_type = false;
					}
				}
			}
			else
			{
				auto &info = rsx::method_registers.vertex_arrays_info[index];
				type = info.type();
				size = info.size();

				auto stride = info.stride();
				attributes |= stride;

				if (stride > 0) //when stride is 0, input is not an array but a single element
				{
					const u32 frequency = info.frequency();
					switch (frequency)
					{
					case 0:
					case 1:
						attributes |= default_frequency_mask;
						break;
					default:
					{
						if (modulo_mask & (1 << index))
							attributes |= input_function_modulo_mask;

						attributes |= repeating_frequency_mask;
						attributes |= (frequency << 13) & input_divisor_mask;
					}
					}
				}
			} //end attribute placement check

			switch (type)
			{
			case rsx::vertex_base_type::cmp:
				size = 1;
				//fall through
			default:
				if (is_be_type) attributes |= swap_storage_mask;
				break;
			case rsx::vertex_base_type::ub:
			case rsx::vertex_base_type::ub256:
				if (!is_be_type) attributes |= swap_storage_mask;
				break;
			}

			buffer[index * 4 + 0] = static_cast<s32>(type);
			buffer[index * 4 + 1] = size;
			buffer[index * 4 + 2] = offset_in_block[index];
			buffer[index * 4 + 3] = attributes;
		}
	}

	void thread::write_vertex_data_to_memory(vertex_input_layout &layout, const u32 first_vertex, const u32 vertex_count, void *persistent_data, void *volatile_data)
	{
		char *transient = (char *)volatile_data;
		char *persistent = (char *)persistent_data;

		auto &draw_call = rsx::method_registers.current_draw_clause;

		if (transient != nullptr)
		{
			if (draw_call.command == rsx::draw_command::inlined_array)
			{
				memcpy(transient, draw_call.inline_vertex_array.data(), draw_call.inline_vertex_array.size() * sizeof(u32));
				//Is it possible to reference data outside of the inlined array?
				return;
			}

			for (const u8 index : layout.referenced_registers)
			{
				memcpy(transient, rsx::method_registers.register_vertex_info[index].data.data(), 16);
				transient += 16;
			}

			if (draw_call.is_immediate_draw)
			{
				//NOTE: It is possible for immediate draw to only contain index data, so vertex data can be in persistent memory
				for (const auto &info : layout.volatile_blocks)
				{
					memcpy(transient, vertex_push_buffers[info.first].data.data(), info.second);
					transient += info.second;
				}
			}
		}

		if (persistent != nullptr)
		{
			for (const auto &block : layout.interleaved_blocks)
			{
				u32 unique_verts;
				u32 vertex_base = first_vertex * block.attribute_stride;

				if (block.single_vertex)
				{
					unique_verts = 1;
				}
				else if (block.min_divisor > 1)
				{
					if (block.all_modulus)
						unique_verts = block.min_divisor;
					else
					{
						unique_verts = vertex_count / block.min_divisor;
						if (vertex_count % block.min_divisor) unique_verts++;
					}
				}
				else
				{
					unique_verts = vertex_count;
				}

				const u32 data_size = block.attribute_stride * unique_verts;
				memcpy(persistent, (char*)vm::base(block.real_offset_address) + vertex_base, data_size);
				persistent += data_size;
			}
		}
	}

	void thread::flip(int buffer)
	{
		if (g_cfg.video.frame_skip_enabled)
		{
			m_skip_frame_ctr++;

			if (m_skip_frame_ctr == g_cfg.video.consequtive_frames_to_draw)
				m_skip_frame_ctr = -g_cfg.video.consequtive_frames_to_skip;

			skip_frame = (m_skip_frame_ctr < 0);
		}
	}
}
