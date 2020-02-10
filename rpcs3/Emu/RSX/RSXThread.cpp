#include "stdafx.h"
#include "Emu/Memory/vm.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "RSXThread.h"

#include "Emu/Cell/PPUCallback.h"

#include "Common/BufferUtils.h"
#include "Common/texture_cache.h"
#include "Common/surface_store.h"
#include "Capture/rsx_capture.h"
#include "rsx_methods.h"
#include "rsx_utils.h"
#include "Emu/Cell/lv2/sys_event.h"
#include "Emu/Cell/Modules/cellGcmSys.h"
#include "Overlays/overlay_perf_metrics.h"

#include "Utilities/span.h"
#include "Utilities/StrUtil.h"

#include <cereal/archives/binary.hpp>

#include <sstream>
#include <thread>
#include <unordered_set>
#include <exception>
#include <cfenv>

class GSRender;

#define CMD_DEBUG 0

bool user_asked_for_frame_capture = false;
bool capture_current_frame = false;
rsx::frame_trace_data frame_debug;
rsx::frame_capture_data frame_capture;
RSXIOTable RSXIOMem;

extern CellGcmOffsetTable offsetTable;
extern thread_local std::string(*g_tls_log_prefix)();

namespace rsx
{
	std::function<bool(u32 addr, bool is_writing)> g_access_violation_handler;

	dma_manager g_dma_manager;

	u32 get_address(u32 offset, u32 location, const char* from)
	{
		const auto render = get_current_renderer();
		std::string_view msg;

		switch (location)
		{
		case CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER:
		case CELL_GCM_LOCATION_LOCAL:
		{
			if (offset < render->local_mem_size)
			{
				return rsx::constants::local_mem_base + offset;
			}

			msg = "Local RSX offset out of range!"sv;
			break;
		}

		case CELL_GCM_CONTEXT_DMA_MEMORY_HOST_BUFFER:
		case CELL_GCM_LOCATION_MAIN:
		{
			if (u32 result = RSXIOMem.RealAddr(offset))
			{
				return result;
			}

			msg = "RSXIO memory not mapped!"sv;
			break;
		}

		case CELL_GCM_CONTEXT_DMA_REPORT_LOCATION_LOCAL:
		{
			if (offset < sizeof(RsxReports::report) /*&& (offset % 0x10) == 0*/)
			{
				return render->label_addr + ::offset32(&RsxReports::report) + offset;
			}

			msg = "Local RSX REPORT offset out of range!"sv;
			break;
		}

		case CELL_GCM_CONTEXT_DMA_REPORT_LOCATION_MAIN:
		{
			if (u32 result = offset < 0x1000000 ? RSXIOMem.RealAddr(0x0e000000 + offset) : 0)
			{
				return result;
			}

			msg = "RSXIO REPORT memory not mapped!"sv;
			break;
		}

		// They are handled elsewhere for targeted methods, so it's unexpected for them to be passed here
		case CELL_GCM_CONTEXT_DMA_TO_MEMORY_GET_NOTIFY0:
			msg = "CELL_GCM_CONTEXT_DMA_TO_MEMORY_GET_NOTIFY0"sv; break;

		case CELL_GCM_CONTEXT_DMA_NOTIFY_MAIN_0:
			msg = "CELL_GCM_CONTEXT_DMA_NOTIFY_MAIN_0"sv; break;

		case CELL_GCM_CONTEXT_DMA_SEMAPHORE_RW:
		case CELL_GCM_CONTEXT_DMA_SEMAPHORE_R:
		{
			if (offset < sizeof(RsxReports::semaphore) /*&& (offset % 0x10) == 0*/)
			{
				return render->label_addr + offset;
			}

			msg = "DMA SEMAPHORE offset out of range!"sv;
			break;
		}

		case CELL_GCM_CONTEXT_DMA_DEVICE_RW:
		case CELL_GCM_CONTEXT_DMA_DEVICE_R:
		{
			if (offset < 0x100000 /*&& (offset % 0x10) == 0*/)
			{
				return render->device_addr + offset;
			}

			// TODO: What happens here? It could wrap around or access other segments of rsx internal memory etc
			// Or can simply throw access violation error
			msg = "DMA DEVICE offset out of range!"sv;
			break;
		}

		default:
		{
			msg = "Invalid location!"sv;
			break;
		}
		}

		// Assume 'from' contains new line at start
		fmt::throw_exception("rsx::get_address(offset=0x%x, location=0x%x): %s%s", offset, location, msg, from);
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
			default:
				break;
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
			default:
				break;
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
			default:
				break;
			}
			fmt::throw_exception("Wrong vector size" HERE);
		case vertex_base_type::cmp: return 4;
		case vertex_base_type::ub256: verify(HERE), (size == 4); return sizeof(u8) * 4;
		default:
			break;
		}
		fmt::throw_exception("RSXVertexData::GetTypeSize: Bad vertex data type (%d)!" HERE, static_cast<u8>(type));
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
				memcpy(ptr + (offset_y + y) * tile->pitch + offset_x, static_cast<const u8*>(src) + pitch * y, pitch);
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
					u32 value = *reinterpret_cast<const u32*>(static_cast<const u8*>(src) + pitch * y + x * sizeof(u32));

					*reinterpret_cast<u32*>(ptr + (offset_y + y * 2 + 0) * tile->pitch + offset_x + (x * 2 + 0) * sizeof(u32)) = value;
					*reinterpret_cast<u32*>(ptr + (offset_y + y * 2 + 0) * tile->pitch + offset_x + (x * 2 + 1) * sizeof(u32)) = value;
					*reinterpret_cast<u32*>(ptr + (offset_y + y * 2 + 1) * tile->pitch + offset_x + (x * 2 + 0) * sizeof(u32)) = value;
					*reinterpret_cast<u32*>(ptr + (offset_y + y * 2 + 1) * tile->pitch + offset_x + (x * 2 + 1) * sizeof(u32)) = value;
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
				memcpy(static_cast<u8*>(dst) + pitch * y, ptr + (offset_y + y) * tile->pitch + offset_x, pitch);
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
					u32 value = *reinterpret_cast<u32*>(ptr + (offset_y + y * 2 + 0) * tile->pitch + offset_x + (x * 2 + 0) * sizeof(u32));

					*reinterpret_cast<u32*>(static_cast<u8*>(dst) + pitch * y + x * sizeof(u32)) = value;
				}
			}
			break;
		default:
			::narrow(tile->comp, "tile->comp" HERE);
		}
	}

	thread::~thread()
	{
		g_access_violation_handler = nullptr;
	}

	thread::thread()
	{
		g_access_violation_handler = [this](u32 address, bool is_writing)
		{
			return on_access_violation(address, is_writing);
		};

		m_rtts_dirty = true;
		memset(m_textures_dirty, -1, sizeof(m_textures_dirty));
		memset(m_vertex_textures_dirty, -1, sizeof(m_vertex_textures_dirty));

		m_graphics_state = pipeline_state::all_dirty;

		if (g_cfg.misc.use_native_interface && (g_cfg.video.renderer == video_renderer::opengl || g_cfg.video.renderer == video_renderer::vulkan))
		{
			m_overlay_manager = g_fxo->init<rsx::overlays::display_manager>(0);
		}
	}

	void thread::capture_frame(const std::string &name)
	{
		frame_trace_data::draw_state draw_state = {};

		draw_state.programs = get_programs();
		draw_state.name = name;
		frame_debug.draw_calls.push_back(draw_state);
	}

	void thread::begin()
	{
		if (cond_render_ctrl.hw_cond_active)
		{
			if (!cond_render_ctrl.eval_pending())
			{
				// End conditional rendering if still active
				end_conditional_rendering();
			}

			// If hw cond render is enabled and evalutation is still pending, do nothing
		}
		else if (cond_render_ctrl.eval_pending())
		{
			// Evaluate conditional rendering test or enable hw cond render until results are available
			if (backend_config.supports_hw_conditional_render)
			{
				// In this mode, it is possible to skip the cond render while the backend is still processing data.
				// The backend guarantees that any draw calls emitted during this time will NOT generate any ROP writes
				verify(HERE), !cond_render_ctrl.hw_cond_active;

				// Pending evaluation, use hardware test
				begin_conditional_rendering(cond_render_ctrl.eval_sources);
			}
			else
			{
				zcull_ctrl->read_barrier(this, cond_render_ctrl.eval_address, 4, reports::sync_no_notify);
				verify(HERE), !cond_render_ctrl.eval_pending();
			}
		}

		if (m_graphics_state & rsx::pipeline_state::fragment_program_dirty)
		{
			// Request for update of fragment constants if the program block is invalidated
			m_graphics_state |= rsx::pipeline_state::fragment_constants_dirty;

			// Request for update of texture parameters if the program is likely to have changed
			m_graphics_state |= rsx::pipeline_state::fragment_texture_state_dirty;
		}

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
		element_push_buffer.push_back(std::bit_cast<u32, be_t<u32>>(index));
	}

	u32 thread::get_push_buffer_index_count() const
	{
		return ::size32(element_push_buffer);
	}

	void thread::end()
	{
		if (capture_current_frame)
			capture::capture_draw_memory(this);

		in_begin_end = false;
		m_frame_stats.draw_calls++;

		method_registers.current_draw_clause.post_execute_cleanup();

		m_graphics_state |= rsx::pipeline_state::framebuffer_reads_dirty;
		ROP_sync_timestamp = get_system_time();

		for (auto & push_buf : vertex_push_buffers)
		{
			//Disabled, see https://github.com/RPCS3/rpcs3/issues/1932
			//rsx::method_registers.register_vertex_info[index].size = 0;

			push_buf.clear();
		}

		element_push_buffer.clear();

		if (zcull_ctrl->active)
			zcull_ctrl->on_draw();

		if (capture_current_frame)
		{
			u32 element_count = rsx::method_registers.current_draw_clause.get_elements_count();
			capture_frame("Draw " + rsx::to_string(rsx::method_registers.current_draw_clause.primitive) + std::to_string(element_count));
		}
	}

	void thread::execute_nop_draw()
	{
		method_registers.current_draw_clause.begin();
		do
		{
			method_registers.current_draw_clause.execute_pipeline_dependencies();
		}
		while (method_registers.current_draw_clause.next());
	}

	void thread::operator()()
	{
		try
		{
			// Wait for startup (TODO)
			while (m_rsx_thread_exiting)
			{
				thread_ctrl::wait_for(1000);

				if (Emu.IsStopped())
				{
					return;
				}
			}

			on_task();
		}
		catch (const std::exception& e)
		{
			rsx_log.fatal("%s thrown: %s", typeid(e).name(), e.what());
			Emu.Pause();
		}

		on_exit();
	}

	void thread::on_task()
	{
		m_rsx_thread = std::this_thread::get_id();

		g_tls_log_prefix = []
		{
			const auto rsx = get_current_renderer();
			return fmt::format("RSX [0x%07x]", +rsx->ctrl->get);
		};

		rsx::overlays::reset_performance_overlay();

		g_dma_manager.init();
		on_init_thread();

		method_registers.init();
		m_profiler.enabled = !!g_cfg.video.overlay;

		if (!zcull_ctrl)
		{
			//Backend did not provide an implementation, provide NULL object
			zcull_ctrl = std::make_unique<::rsx::reports::ZCULL_control>();
		}

		fifo_ctrl = std::make_unique<::rsx::FIFO::FIFO_control>(this);

		last_flip_time = get_system_time() - 1000000;

		vblank_count = 0;

		thread_ctrl::spawn("VBlank Thread", [this]()
		{
			// See sys_timer_usleep for details
#ifdef __linux__
			constexpr u32 host_min_quantum = 50;
#else
			constexpr u32 host_min_quantum = 500;
#endif
			u64 start_time = get_system_time();

			// TODO: exit condition
			while (!Emu.IsStopped() && !m_rsx_thread_exiting)
			{
				const u64 period_time = 1000000 / g_cfg.video.vblank_rate;
				const u64 wait_sleep = period_time - u64{period_time >= host_min_quantum} * host_min_quantum;

				if (get_system_time() - start_time >= period_time)
				{
					do
					{
						start_time += period_time;
						vblank_count++;

						if (isHLE)
						{
							if (vblank_handler)
							{
								intr_thread->cmd_list
								({
									{ ppu_cmd::set_args, 1 }, u64{1},
									{ ppu_cmd::lle_call, vblank_handler },
									{ ppu_cmd::sleep, 0 }
								});

								thread_ctrl::notify(*intr_thread);
							}
						}
						else
						{
							sys_rsx_context_attribute(0x55555555, 0xFED, 1, 0, 0, 0);
						}
					}
					while (get_system_time() - start_time >= period_time);

					thread_ctrl::wait_for(wait_sleep);
					continue;
				}

				if (Emu.IsPaused())
				{
					// Save the difference before pause
					start_time = get_system_time() - start_time;

					while (Emu.IsPaused() && !m_rsx_thread_exiting)
					{
						thread_ctrl::wait_for(wait_sleep);
					}

					// Restore difference
					start_time = get_system_time() - start_time;
				}

				thread_ctrl::wait_for(100);
			}
		});

		thread_ctrl::spawn("RSX Decompiler Thread", [this]
		{
			if (g_cfg.video.disable_asynchronous_shader_compiler)
			{
				// Die
				return;
			}

			on_decompiler_init();

			if (g_cfg.core.thread_scheduler_enabled)
			{
				thread_ctrl::set_thread_affinity_mask(thread_ctrl::get_affinity_mask(thread_class::rsx));
			}

			while (!Emu.IsStopped() && !m_rsx_thread_exiting)
			{
				if (!on_decompiler_task())
				{
					if (Emu.IsPaused())
					{
						std::this_thread::sleep_for(1ms);
					}
					else
					{
						std::this_thread::sleep_for(500us);
					}
				}
			}

			on_decompiler_exit();
		});

		// Raise priority above other threads
		thread_ctrl::set_native_priority(1);

		if (g_cfg.core.thread_scheduler_enabled)
		{
			thread_ctrl::set_thread_affinity_mask(thread_ctrl::get_affinity_mask(thread_class::rsx));
		}

		// Round to nearest to deal with forward/reverse scaling
		fesetround(FE_TONEAREST);

		// TODO: exit condition
		while (true)
		{
			// Wait for external pause events
			if (external_interrupt_lock.load())
			{
				external_interrupt_ack.store(true);
				while (external_interrupt_lock.load()) _mm_pause();
			}

			// Note a possible rollback address
			if (sync_point_request && !in_begin_end)
			{
				restore_point = ctrl->get;
				saved_fifo_ret = fifo_ret_addr;
				sync_point_request = false;
			}

			// Execute backend-local tasks first
			do_local_task(performance_counters.state);

			// Update sub-units
			zcull_ctrl->update(this);

			// Execute FIFO queue
			run_FIFO();

			if (!Emu.IsRunning())
			{
				// Idle if emulation paused
				while (Emu.IsPaused())
				{
					std::this_thread::sleep_for(1ms);
				}

				if (Emu.IsStopped())
				{
					break;
				}
			}
		}
	}

	void thread::on_exit()
	{
		// Deregister violation handler
		g_access_violation_handler = nullptr;

		// Clear any pending flush requests to release threads
		std::this_thread::sleep_for(10ms);
		do_local_task(rsx::FIFO_state::lock_wait);

		m_rsx_thread_exiting = true;
		g_dma_manager.join();
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

		stream_vector(buffer, std::bit_cast<u32>(scale_x), 0, 0, std::bit_cast<u32>(offset_x));
		stream_vector(static_cast<char*>(buffer) + 16, 0, std::bit_cast<u32>(scale_y), 0, std::bit_cast<u32>(offset_y));
		stream_vector(static_cast<char*>(buffer) + 32, 0, 0, std::bit_cast<u32>(scale_z), std::bit_cast<u32>(offset_z));
		stream_vector(static_cast<char*>(buffer) + 48, 0, 0, 0, std::bit_cast<u32>(one));
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

		u8 data_block[64];
		s32* clip_enabled_flags = reinterpret_cast<s32*>(data_block);
		f32* clip_distance_factors = reinterpret_cast<f32*>(data_block + 32);

		for (int index = 0; index < 6; ++index)
		{
			switch (clip_plane_control[index])
			{
			default:
				rsx_log.error("bad clip plane control (0x%x)", static_cast<u8>(clip_plane_control[index]));

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

		memcpy(buffer, data_block, 2 * 8 * sizeof(u32));
	}

	/**
	* Fill buffer with vertex program constants.
	* Buffer must be at least 512 float4 wide.
	*/
	void thread::fill_vertex_program_constants_data(void *buffer)
	{
		memcpy(buffer, rsx::method_registers.transform_constants.data(), 468 * 4 * sizeof(float));
	}

	void thread::fill_fragment_state_buffer(void *buffer, const RSXFragmentProgram &fragment_program)
	{
		//TODO: Properly support alpha-to-coverage and alpha-to-one behavior in shaders
		auto fragment_alpha_func = rsx::method_registers.alpha_func();
		auto alpha_ref = rsx::method_registers.alpha_ref() / 255.f;
		auto rop_control = rsx::method_registers.alpha_test_enabled()? 1u : 0u;

		if (rsx::method_registers.msaa_alpha_to_coverage_enabled() && !backend_config.supports_hw_a2c)
		{
			// Alpha values generate a coverage mask for order independent blending
			// Requires hardware AA to work properly (or just fragment sample stage in fragment shaders)
			// Simulated using combined alpha blend and alpha test
			const u32 mask_bit = rsx::method_registers.msaa_sample_mask() ? 1u : 0u;

			rop_control |= (1u << 4);                 // CSAA enable bit
			rop_control |= (mask_bit << 5);           // MSAA mask enable bit

			// Sample configuration bits
			switch (rsx::method_registers.surface_antialias())
			{
				case rsx::surface_antialiasing::center_1_sample:
					break;
				case rsx::surface_antialiasing::diagonal_centered_2_samples:
					rop_control |= 1u << 6;
					break;
				default:
					rop_control |= 3u << 6;
					break;
			}
		}

		const f32 fog0 = rsx::method_registers.fog_params_0();
		const f32 fog1 = rsx::method_registers.fog_params_1();
		const u32 alpha_func = static_cast<u32>(fragment_alpha_func);
		const u32 fog_mode = static_cast<u32>(rsx::method_registers.fog_equation());

		rop_control |= (alpha_func << 16);

		if (rsx::method_registers.framebuffer_srgb_enabled())
		{
			// Check if framebuffer is actually an XRGB format and not a WZYX format
			switch (rsx::method_registers.surface_color())
			{
			case rsx::surface_color_format::w16z16y16x16:
			case rsx::surface_color_format::w32z32y32x32:
			case rsx::surface_color_format::x32:
				break;
			default:
				rop_control |= (1u << 1);
				break;
			}
		}

		// Generate wpos coefficients
		// wpos equation is now as follows:
		// wpos.y = (frag_coord / resolution_scale) * ((window_origin!=top)?-1.: 1.) + ((window_origin!=top)? window_height : 0)
		// wpos.x = (frag_coord / resolution_scale)
		// wpos.zw = frag_coord.zw

		const auto window_origin = rsx::method_registers.shader_window_origin();
		const u32 window_height = rsx::method_registers.shader_window_height();
		const f32 resolution_scale = (window_height <= static_cast<u32>(g_cfg.video.min_scalable_dimension)) ? 1.f : rsx::get_resolution_scale();
		const f32 wpos_scale = (window_origin == rsx::window_origin::top) ? (1.f / resolution_scale) : (-1.f / resolution_scale);
		const f32 wpos_bias = (window_origin == rsx::window_origin::top) ? 0.f : window_height;

		u32 *dst = static_cast<u32*>(buffer);
		stream_vector(dst, std::bit_cast<u32>(fog0), std::bit_cast<u32>(fog1), rop_control, std::bit_cast<u32>(alpha_ref));
		stream_vector(dst + 4, alpha_func, fog_mode, std::bit_cast<u32>(wpos_scale), std::bit_cast<u32>(wpos_bias));
	}

	void thread::fill_fragment_texture_parameters(void *buffer, const RSXFragmentProgram &fragment_program)
	{
		memcpy(buffer, fragment_program.texture_scale, 16 * 4 * sizeof(float));
	}

	u64 thread::timestamp()
	{
		// Get timestamp, and convert it from microseconds to nanoseconds
		const u64 t = get_guest_system_time() * 1000;
		if (t != timestamp_ctrl)
		{
			timestamp_ctrl = t;
			timestamp_subvalue = 0;
			return t;
		}

		timestamp_subvalue += 10;
		return t + timestamp_subvalue;
	}

	gsl::span<const std::byte> thread::get_raw_index_array(const draw_clause& draw_indexed_clause) const
	{
		if (!element_push_buffer.empty())
		{
			//Indices provided via immediate mode
			return{reinterpret_cast<const std::byte*>(element_push_buffer.data()), ::narrow<u32>(element_push_buffer.size() * sizeof(u32))};
		}

		const rsx::index_array_type type = rsx::method_registers.index_type();
		const u32 type_size = get_index_type_size(type);

		// Force aligned indices as realhw
		const u32 address = (0 - type_size) & get_address(rsx::method_registers.index_array_address(), rsx::method_registers.index_array_location(), HERE);

		const bool is_primitive_restart_enabled = rsx::method_registers.restart_index_enabled();
		const u32 primitive_restart_index = rsx::method_registers.restart_index();

		const u32 first = draw_indexed_clause.min_index();
		const u32 count = draw_indexed_clause.get_elements_count();

		const auto ptr = vm::_ptr<const std::byte>(address);
		return{ ptr + first * type_size, count * type_size };
	}

	std::variant<draw_array_command, draw_indexed_array_command, draw_inlined_array>
	thread::get_draw_command(const rsx::rsx_state& state) const
	{
		if (rsx::method_registers.current_draw_clause.command == rsx::draw_command::array)
		{
			return draw_array_command{};
		}

		if (rsx::method_registers.current_draw_clause.command == rsx::draw_command::indexed)
		{
			return draw_indexed_array_command
			{
				get_raw_index_array(state.current_draw_clause)
			};
		}

		if (rsx::method_registers.current_draw_clause.command == rsx::draw_command::inlined_array)
		{
			return draw_inlined_array{};
		}

		fmt::throw_exception("ill-formed draw command" HERE);
	}

	void thread::do_local_task(FIFO_state state)
	{
		if (async_flip_requested & flip_request::emu_requested)
		{
			// NOTE: This has to be executed immediately
			// Delaying this operation can cause desync due to the delay in firing the flip event
			handle_emu_flip(async_flip_buffer);
		}

		if (!in_begin_end && state != FIFO_state::lock_wait)
		{
			if (atomic_storage<u32>::load(m_invalidated_memory_range.end) != 0)
			{
				std::lock_guard lock(m_mtx_task);

				if (m_invalidated_memory_range.valid())
				{
					handle_invalidated_memory_range();
				}
			}
		}
	}

	namespace
	{
		bool is_int_type(rsx::vertex_base_type type)
		{
			switch (type)
			{
			case rsx::vertex_base_type::s32k:
			case rsx::vertex_base_type::ub256:
				return true;
			default:
				return false;
			}
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
			rsx::get_address(offset_color[0], context_dma_color[0], HERE),
			rsx::get_address(offset_color[1], context_dma_color[1], HERE),
			rsx::get_address(offset_color[2], context_dma_color[2], HERE),
			rsx::get_address(offset_color[3], context_dma_color[3], HERE),
		};
	}

	u32 thread::get_zeta_surface_address() const
	{
		u32 m_context_dma_z = rsx::method_registers.surface_z_dma();
		u32 offset_zeta = rsx::method_registers.surface_z_offset();
		return rsx::get_address(offset_zeta, m_context_dma_z, HERE);
	}

	void thread::get_framebuffer_layout(rsx::framebuffer_creation_context context, framebuffer_layout &layout)
	{
		layout = {};

		layout.ignore_change = true;
		layout.width = rsx::method_registers.surface_clip_width();
		layout.height = rsx::method_registers.surface_clip_height();

		framebuffer_status_valid = false;
		m_framebuffer_state_contested = false;
		m_current_framebuffer_context = context;

		if (layout.width == 0 || layout.height == 0)
		{
			rsx_log.trace("Invalid framebuffer setup, w=%d, h=%d", layout.width, layout.height);
			return;
		}

		const u16 clip_x = rsx::method_registers.surface_clip_origin_x();
		const u16 clip_y = rsx::method_registers.surface_clip_origin_y();

		layout.color_addresses = get_color_surface_addresses();
		layout.zeta_address = get_zeta_surface_address();
		layout.zeta_pitch = rsx::method_registers.surface_z_pitch();
		layout.color_pitch =
		{
			rsx::method_registers.surface_a_pitch(),
			rsx::method_registers.surface_b_pitch(),
			rsx::method_registers.surface_c_pitch(),
			rsx::method_registers.surface_d_pitch(),
		};

		layout.color_format = rsx::method_registers.surface_color();
		layout.depth_format = rsx::method_registers.surface_depth_fmt();
		layout.depth_float = rsx::method_registers.depth_buffer_float_enabled();
		layout.target = rsx::method_registers.surface_color_target();

		const auto mrt_buffers = rsx::utility::get_rtt_indexes(layout.target);
		const auto aa_mode = rsx::method_registers.surface_antialias();
		const u32 aa_factor_u = (aa_mode == rsx::surface_antialiasing::center_1_sample) ? 1 : 2;
		const u32 aa_factor_v = (aa_mode == rsx::surface_antialiasing::center_1_sample || aa_mode == rsx::surface_antialiasing::diagonal_centered_2_samples) ? 1 : 2;
		const u8 sample_count = get_format_sample_count(aa_mode);

		const auto depth_texel_size = (layout.depth_format == rsx::surface_depth_format::z16 ? 2 : 4) * aa_factor_u;
		const auto color_texel_size = get_format_block_size_in_bytes(layout.color_format) * aa_factor_u;
		const bool stencil_test_enabled = layout.depth_format == rsx::surface_depth_format::z24s8 && rsx::method_registers.stencil_test_enabled();
		const bool depth_test_enabled = rsx::method_registers.depth_test_enabled();

		// Check write masks
		layout.zeta_write_enabled = (depth_test_enabled && rsx::method_registers.depth_write_enabled());
		if (!layout.zeta_write_enabled && stencil_test_enabled)
		{
			// Check if stencil data is modified
			auto mask = rsx::method_registers.stencil_mask();
			bool active_write_op = (rsx::method_registers.stencil_op_zpass() != rsx::stencil_op::keep ||
				rsx::method_registers.stencil_op_fail() != rsx::stencil_op::keep ||
				rsx::method_registers.stencil_op_zfail() != rsx::stencil_op::keep);

			if ((!mask || !active_write_op) && rsx::method_registers.two_sided_stencil_test_enabled())
			{
				mask |= rsx::method_registers.back_stencil_mask();
				active_write_op |= (rsx::method_registers.stencil_op_zpass() != rsx::stencil_op::keep ||
					rsx::method_registers.stencil_op_fail() != rsx::stencil_op::keep ||
					rsx::method_registers.stencil_op_zfail() != rsx::stencil_op::keep);
			}

			layout.zeta_write_enabled = (mask && active_write_op);
		}

		// NOTE: surface_target_a is index 1 but is not MRT since only one surface is active
		bool color_write_enabled = false;
		for (int i = 0; i < mrt_buffers.size(); ++i)
		{
			if (rsx::method_registers.color_write_enabled(i))
			{
				const auto real_index = mrt_buffers[i];
				layout.color_write_enabled[real_index] = true;
				color_write_enabled = true;
			}
		}

		bool depth_buffer_unused = false, color_buffer_unused = false;

		switch (context)
		{
		case rsx::framebuffer_creation_context::context_clear_all:
			break;
		case rsx::framebuffer_creation_context::context_clear_depth:
			color_buffer_unused = true;
			break;
		case rsx::framebuffer_creation_context::context_clear_color:
			depth_buffer_unused = true;
			break;
		case rsx::framebuffer_creation_context::context_draw:
			// NOTE: As with all other hw, depth/stencil writes involve the corresponding depth/stencil test, i.e No test = No write
			// NOTE: Depth test is not really using the memory if its set to always or never
			// TODO: Perform similar checks for stencil test
			if (!stencil_test_enabled)
			{
				if (!depth_test_enabled)
				{
					depth_buffer_unused = true;
				}
				else if (!rsx::method_registers.depth_write_enabled())
				{
					// Depth test is enabled but depth write is disabled
					switch (rsx::method_registers.depth_func())
					{
					default:
						break;
					case rsx::comparison_function::never:
					case rsx::comparison_function::always:
						// No access to depth buffer memory
						depth_buffer_unused = true;
						break;
					}
				}
			}

			color_buffer_unused = !color_write_enabled || layout.target == rsx::surface_target::none;
			m_framebuffer_state_contested = color_buffer_unused || depth_buffer_unused;
			break;
		default:
			fmt::throw_exception("Unknown framebuffer context 0x%x" HERE, static_cast<u32>(context));
		}

		// Swizzled render does tight packing of bytes
		bool packed_render = false;
		u32 minimum_color_pitch = 64u;
		u32 minimum_zeta_pitch = 64u;

		switch (const auto mode = rsx::method_registers.surface_type())
		{
		default:
			rsx_log.error("Unknown raster mode 0x%x", static_cast<u32>(mode));
			[[fallthrough]];
		case rsx::surface_raster_type::linear:
			break;
		case rsx::surface_raster_type::swizzle:
			packed_render = true;
			break;
		};

		if (!packed_render)
		{
			// Well, this is a write operation either way (clearing or drawing)
			// We can deduce a minimum pitch for which this operation is guaranteed to require by checking for the lesser of scissor or clip
			const u32 write_limit_x = std::min<u32>(layout.width, rsx::method_registers.scissor_origin_x() + rsx::method_registers.scissor_width());

			minimum_color_pitch = color_texel_size * write_limit_x;
			minimum_zeta_pitch = depth_texel_size * write_limit_x;
		}

		if (depth_buffer_unused)
		{
			layout.zeta_address = 0;
		}
		else if (layout.zeta_pitch < minimum_zeta_pitch)
		{
			layout.zeta_address = 0;
		}
		else if (packed_render)
		{
			layout.actual_zeta_pitch = (layout.width * depth_texel_size);
		}
		else
		{
			const auto packed_zeta_pitch = (layout.width * depth_texel_size);
			if (packed_zeta_pitch > layout.zeta_pitch)
			{
				layout.width = (layout.zeta_pitch / depth_texel_size);
			}

			layout.actual_zeta_pitch = layout.zeta_pitch;
		}

		for (const auto &index : rsx::utility::get_rtt_indexes(layout.target))
		{
			if (color_buffer_unused)
			{
				layout.color_addresses[index] = 0;
				continue;
			}

			if (layout.color_pitch[index] < minimum_color_pitch)
			{
				// Unlike the depth buffer, when given a color target we know it is intended to be rendered to
				rsx_log.error("Framebuffer setup error: Color target failed pitch check, Pitch=[%d, %d, %d, %d] + %d, target=%d, context=%d",
					layout.color_pitch[0], layout.color_pitch[1], layout.color_pitch[2], layout.color_pitch[3],
					layout.zeta_pitch, static_cast<u32>(layout.target), static_cast<u32>(context));

				// Do not remove this buffer for now as it implies something went horribly wrong anyway
				break;
			}

			if (layout.color_addresses[index] == layout.zeta_address)
			{
				rsx_log.warning("Framebuffer at 0x%X has aliasing color/depth targets, color_index=%d, zeta_pitch = %d, color_pitch=%d, context=%d",
					layout.zeta_address, index, layout.zeta_pitch, layout.color_pitch[index], static_cast<u32>(context));

				m_framebuffer_state_contested = true;

				// TODO: Research clearing both depth AND color
				// TODO: If context is creation_draw, deal with possibility of a lost buffer clear
				if (depth_test_enabled || stencil_test_enabled || (!layout.color_write_enabled[index] && layout.zeta_write_enabled))
				{
					// Use address for depth data
					layout.color_addresses[index] = 0;
					continue;
				}
				else
				{
					// Use address for color data
					layout.zeta_address = 0;
				}
			}

			verify(HERE), layout.color_addresses[index];

			const auto packed_pitch = (layout.width * color_texel_size);
			if (packed_render)
			{
				layout.actual_color_pitch[index] = packed_pitch;
			}
			else
			{
				if (packed_pitch > layout.color_pitch[index])
				{
					layout.width = (layout.color_pitch[index] / color_texel_size);
				}

				layout.actual_color_pitch[index] = layout.color_pitch[index];
			}

			framebuffer_status_valid = true;
		}

		if (!framebuffer_status_valid && !layout.zeta_address)
		{
			rsx_log.warning("Framebuffer setup failed. Draw calls may have been lost");
			return;
		}

		// At least one attachment exists
		framebuffer_status_valid = true;

		// Window (raster) offsets
		const auto window_offset_x = rsx::method_registers.window_offset_x();
		const auto window_offset_y = rsx::method_registers.window_offset_y();
		const auto window_clip_width = rsx::method_registers.window_clip_horizontal();
		const auto window_clip_height = rsx::method_registers.window_clip_vertical();

		if (window_offset_x || window_offset_y)
		{
			// Window offset is what affects the raster position!
			// Tested with Turbo: Super stunt squad that only changes the window offset to declare new framebuffers
			// Sampling behavior clearly indicates the addresses are expected to have changed
			if (auto clip_type = rsx::method_registers.window_clip_type())
				rsx_log.error("Unknown window clip type 0x%X" HERE, clip_type);

			for (const auto &index : rsx::utility::get_rtt_indexes(layout.target))
			{
				if (layout.color_addresses[index])
				{
					const u32 window_offset_bytes = (layout.actual_color_pitch[index] * window_offset_y) + (color_texel_size * window_offset_x);
					layout.color_addresses[index] += window_offset_bytes;
				}
			}

			if (layout.zeta_address)
			{
				layout.zeta_address += (layout.actual_zeta_pitch * window_offset_y) + (depth_texel_size * window_offset_x);
			}
		}

		if ((window_clip_width && window_clip_width < layout.width) ||
			(window_clip_height && window_clip_height < layout.height))
		{
			rsx_log.error("Unexpected window clip dimensions: window_clip=%dx%d, surface_clip=%dx%d",
				window_clip_width, window_clip_height, layout.width, layout.height);
		}

		layout.aa_mode = aa_mode;
		layout.aa_factors[0] = aa_factor_u;
		layout.aa_factors[1] = aa_factor_v;

		bool really_changed = false;

		for (u8 i = 0; i < rsx::limits::color_buffers_count; ++i)
		{
			if (m_surface_info[i].address != layout.color_addresses[i])
			{
				really_changed = true;
				break;
			}

			if (layout.color_addresses[i])
			{
				if (m_surface_info[i].width != layout.width ||
					m_surface_info[i].height != layout.height ||
					m_surface_info[i].color_format != layout.color_format ||
					m_surface_info[i].samples != sample_count)
				{
					really_changed = true;
					break;
				}
			}
		}

		if (!really_changed)
		{
			if (layout.zeta_address == m_depth_surface_info.address &&
				layout.depth_format == m_depth_surface_info.depth_format &&
				layout.depth_float == m_depth_surface_info.depth_buffer_float &&
				sample_count == m_depth_surface_info.samples)
			{
				// Same target is reused
				return;
			}
		}

		layout.ignore_change = false;
	}

	bool thread::get_scissor(areau& region, bool clip_viewport)
	{
		if (!(m_graphics_state & rsx::pipeline_state::scissor_config_state_dirty))
		{
			if (clip_viewport == !!(m_graphics_state & rsx::pipeline_state::scissor_setup_clipped))
			{
				// Nothing to do
				return false;
			}
		}

		m_graphics_state &= ~(rsx::pipeline_state::scissor_config_state_dirty | rsx::pipeline_state::scissor_setup_clipped);

		u16 x1, x2, y1, y2;

		u16 scissor_x = rsx::method_registers.scissor_origin_x();
		u16 scissor_w = rsx::method_registers.scissor_width();
		u16 scissor_y = rsx::method_registers.scissor_origin_y();
		u16 scissor_h = rsx::method_registers.scissor_height();

		if (clip_viewport)
		{
			u16 raster_x = rsx::method_registers.viewport_origin_x();
			u16 raster_w = rsx::method_registers.viewport_width();
			u16 raster_y = rsx::method_registers.viewport_origin_y();
			u16 raster_h = rsx::method_registers.viewport_height();

			// Get the minimum area between these two
			x1 = std::max(scissor_x, raster_x);
			y1 = std::max(scissor_y, raster_y);
			x2 = std::min(scissor_x + scissor_w, raster_x + raster_w);
			y2 = std::min(scissor_y + scissor_h, raster_y + raster_h);

			m_graphics_state |= rsx::pipeline_state::scissor_setup_clipped;
		}
		else
		{
			x1 = scissor_x;
			x2 = scissor_x + scissor_w;
			y1 = scissor_y;
			y2 = scissor_y + scissor_h;
		}

		if (x2 <= x1 ||
			y2 <= y1 ||
			x1 >= rsx::method_registers.window_clip_horizontal() ||
			y1 >= rsx::method_registers.window_clip_vertical())
		{
			m_graphics_state |= rsx::pipeline_state::scissor_setup_invalid;
			framebuffer_status_valid = false;
			return false;
		}

		if (m_graphics_state & rsx::pipeline_state::scissor_setup_invalid)
		{
			m_graphics_state &= ~rsx::pipeline_state::scissor_setup_invalid;
			framebuffer_status_valid = true;
		}

		region.x1 = rsx::apply_resolution_scale(x1, false);
		region.x2 = rsx::apply_resolution_scale(x2, true);
		region.y1 = rsx::apply_resolution_scale(y1, false);
		region.y2 = rsx::apply_resolution_scale(y2, true);

		return true;
	}

	void thread::get_current_vertex_program(const std::array<std::unique_ptr<rsx::sampled_image_descriptor_base>, rsx::limits::vertex_textures_count>& sampler_descriptors, bool skip_textures, bool skip_vertex_inputs)
	{
		if (!(m_graphics_state & rsx::pipeline_state::vertex_program_dirty))
			return;

		m_graphics_state &= ~(rsx::pipeline_state::vertex_program_dirty);
		const u32 transform_program_start = rsx::method_registers.transform_program_start();
		current_vertex_program.output_mask = rsx::method_registers.vertex_attrib_output_mask();
		current_vertex_program.skip_vertex_input_check = skip_vertex_inputs;

		current_vertex_program.rsx_vertex_inputs.clear();
		current_vertex_program.data.reserve(512 * 4);
		current_vertex_program.jump_table.clear();
		current_vertex_program.texture_dimensions = 0;

		current_vp_metadata = program_hash_util::vertex_program_utils::analyse_vertex_program
		(
			method_registers.transform_program.data(),  // Input raw block
			transform_program_start,                    // Address of entry point
			current_vertex_program                      // [out] Program object
		);

		if (!skip_textures && current_vp_metadata.referenced_textures_mask != 0)
		{
			for (u32 i = 0; i < rsx::limits::vertex_textures_count; ++i)
			{
				const auto &tex = rsx::method_registers.vertex_textures[i];
				if (tex.enabled() && (current_vp_metadata.referenced_textures_mask & (1 << i)))
				{
					current_vertex_program.texture_dimensions |= (static_cast<u32>(sampler_descriptors[i]->image_type) << (i << 1));
				}
			}
		}

		if (!skip_vertex_inputs)
		{
			const u32 input_mask = rsx::method_registers.vertex_attrib_input_mask();
			const u32 modulo_mask = rsx::method_registers.frequency_divider_operation_mask();

			for (u8 index = 0; index < rsx::limits::vertex_count; ++index)
			{
				bool enabled = !!(input_mask & (1 << index));
				if (!enabled)
					continue;

				if (rsx::method_registers.vertex_arrays_info[index].size() > 0)
				{
					current_vertex_program.rsx_vertex_inputs.push_back(
						{ index,
							rsx::method_registers.vertex_arrays_info[index].size(),
							rsx::method_registers.vertex_arrays_info[index].frequency(),
							!!((modulo_mask >> index) & 0x1),
							true,
							is_int_type(rsx::method_registers.vertex_arrays_info[index].type()), 0 });
				}
				else if (vertex_push_buffers[index].vertex_count > 1)
				{
					current_vertex_program.rsx_vertex_inputs.push_back(
						{ index,
							vertex_push_buffers[index].size,
							1,
							false,
							true,
							is_int_type(vertex_push_buffers[index].type), 0 });
				}
				else if (rsx::method_registers.register_vertex_info[index].size > 0)
				{
					current_vertex_program.rsx_vertex_inputs.push_back(
						{ index,
							rsx::method_registers.register_vertex_info[index].size,
							rsx::method_registers.register_vertex_info[index].frequency,
							!!((modulo_mask >> index) & 0x1),
							false,
							is_int_type(rsx::method_registers.vertex_arrays_info[index].type()), 0 });
				}
			}
		}
	}

	void thread::analyse_inputs_interleaved(vertex_input_layout& result) const
	{
		const rsx_state& state = rsx::method_registers;
		const u32 input_mask = state.vertex_attrib_input_mask();

		result.clear();

		if (state.current_draw_clause.command == rsx::draw_command::inlined_array)
		{
			interleaved_range_info info = {};
			info.interleaved = true;
			info.locations.reserve(8);

			for (u8 index = 0; index < rsx::limits::vertex_count; ++index)
			{
				auto &vinfo = state.vertex_arrays_info[index];

				if (vinfo.size() > 0)
				{
					// Stride must be updated even if the stream is disabled
					info.attribute_stride += rsx::get_vertex_type_size_on_host(vinfo.type(), vinfo.size());
					info.locations.push_back({ index, false, 1 });

					if (input_mask & (1u << index))
					{
						result.attribute_placement[index] = attribute_buffer_placement::transient;
					}
				}
				else if (state.register_vertex_info[index].size > 0 && input_mask & (1u << index))
				{
					//Reads from register
					result.referenced_registers.push_back(index);
					result.attribute_placement[index] = attribute_buffer_placement::transient;
				}
			}

			if (info.attribute_stride)
			{
				// At least one array feed must be enabled for vertex input
				result.interleaved_blocks.emplace_back(std::move(info));
			}

			return;
		}

		const u32 frequency_divider_mask = rsx::method_registers.frequency_divider_operation_mask();
		result.interleaved_blocks.reserve(16);
		result.referenced_registers.reserve(16);

		for (u8 index = 0; index < rsx::limits::vertex_count; ++index)
		{
			// Check if vertex stream is enabled
			if (!(input_mask & (1 << index)))
				continue;

			//Check for interleaving
			const auto &info = state.vertex_arrays_info[index];
			if (rsx::method_registers.current_draw_clause.is_immediate_draw &&
				rsx::method_registers.current_draw_clause.command != rsx::draw_command::indexed)
			{
				// NOTE: In immediate rendering mode, all vertex setup is ignored
				// Observed with GT5, immediate render bypasses array pointers completely, even falling back to fixed-function register defaults
				if (vertex_push_buffers[index].vertex_count > 1)
				{
					// Read temp buffer (register array)
					std::pair<u8, u32> volatile_range_info = std::make_pair(index, static_cast<u32>(vertex_push_buffers[index].data.size() * sizeof(u32)));
					result.volatile_blocks.push_back(volatile_range_info);
					result.attribute_placement[index] = attribute_buffer_placement::transient;
				}
				else if (state.register_vertex_info[index].size > 0)
				{
					// Reads from register
					result.referenced_registers.push_back(index);
					result.attribute_placement[index] = attribute_buffer_placement::transient;
				}

				// Fall back to the default register value if no source is specified via register
				continue;
			}

			if (!info.size())
			{
				if (state.register_vertex_info[index].size > 0)
				{
					//Reads from register
					result.referenced_registers.push_back(index);
					result.attribute_placement[index] = attribute_buffer_placement::transient;
					continue;
				}
			}
			else
			{
				result.attribute_placement[index] = attribute_buffer_placement::persistent;
				const u32 base_address = info.offset() & 0x7fffffff;
				bool alloc_new_block = true;
				bool modulo = !!(frequency_divider_mask & (1 << index));

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
					block.locations.push_back({ index, modulo, info.frequency() });
					block.interleaved = true;
					break;
				}

				if (alloc_new_block)
				{
					interleaved_range_info block = {};
					block.base_offset = base_address;
					block.attribute_stride = info.stride();
					block.memory_location = info.offset() >> 31;
					block.locations.reserve(16);
					block.locations.push_back({ index, modulo, info.frequency() });

					if (block.attribute_stride == 0)
					{
						block.single_vertex = true;
						block.attribute_stride = rsx::get_vertex_type_size_on_host(info.type(), info.size());
					}

					result.interleaved_blocks.emplace_back(std::move(block));
				}
			}
		}

		for (auto &info : result.interleaved_blocks)
		{
			//Calculate real data address to be used during upload
			info.real_offset_address = rsx::get_address(rsx::get_vertex_offset_from_base(state.vertex_data_base_offset(), info.base_offset), info.memory_location, HERE);
		}
	}

	void thread::get_current_fragment_program(const std::array<std::unique_ptr<rsx::sampled_image_descriptor_base>, rsx::limits::fragment_textures_count>& sampler_descriptors)
	{
		if (!(m_graphics_state & rsx::pipeline_state::fragment_program_dirty))
			return;

		m_graphics_state &= ~(rsx::pipeline_state::fragment_program_dirty);
		auto &result = current_fragment_program = {};

		const u32 shader_program = rsx::method_registers.shader_program_address();

		const u32 program_location = (shader_program & 0x3) - 1;
		const u32 program_offset = (shader_program & ~0x3);

		result.addr = vm::base(rsx::get_address(program_offset, program_location, HERE));
		current_fp_metadata = program_hash_util::fragment_program_utils::analyse_fragment_program(result.addr);

		result.addr = (static_cast<u8*>(result.addr) + current_fp_metadata.program_start_offset);
		result.offset = program_offset + current_fp_metadata.program_start_offset;
		result.ucode_length = current_fp_metadata.program_ucode_length;
		result.valid = true;
		result.ctrl = rsx::method_registers.shader_control() & (CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS | CELL_GCM_SHADER_CONTROL_DEPTH_EXPORT);
		result.texcoord_control_mask = rsx::method_registers.texcoord_control_mask();
		result.unnormalized_coords = 0;
		result.two_sided_lighting = rsx::method_registers.two_side_light_en();
		result.redirected_textures = 0;
		result.shadow_textures = 0;

		if (method_registers.current_draw_clause.primitive == primitive_type::points &&
			method_registers.point_sprite_enabled())
		{
			// Set high word of the control mask to store point sprite control
			result.texcoord_control_mask |= u32(method_registers.point_sprite_control_mask()) << 16;
		}

		const auto resolution_scale = rsx::get_resolution_scale();

		for (u32 i = 0; i < rsx::limits::fragment_textures_count; ++i)
		{
			auto &tex = rsx::method_registers.fragment_textures[i];
			result.texture_scale[i][0] = sampler_descriptors[i]->scale_x;
			result.texture_scale[i][1] = sampler_descriptors[i]->scale_y;
			result.texture_scale[i][2] = std::bit_cast<f32>(tex.remap());

			if (tex.enabled() && (current_fp_metadata.referenced_textures_mask & (1 << i)))
			{
				u32 texture_control = 0;
				result.texture_dimensions |= (static_cast<u32>(sampler_descriptors[i]->image_type) << (i << 1));

				if (tex.alpha_kill_enabled())
				{
					//alphakill can be ignored unless a valid comparison function is set
					texture_control |= (1 << 4);
				}

				const u32 texaddr = rsx::get_address(tex.offset(), tex.location(), HERE);
				const u32 raw_format = tex.format();
				const u32 format = raw_format & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);

				if (raw_format & CELL_GCM_TEXTURE_UN)
					result.unnormalized_coords |= (1 << i);

				if (sampler_descriptors[i]->format_class != format_type::color)
				{
					switch (format)
					{
					case CELL_GCM_TEXTURE_X16:
					{
						// A simple way to quickly read DEPTH16 data without shadow comparison
						break;
					}
					case CELL_GCM_TEXTURE_A8R8G8B8:
					case CELL_GCM_TEXTURE_D8R8G8B8:
					{
						// Reading depth data as XRGB8 is supported with in-shader conversion
						// TODO: Optionally add support for 16-bit formats (not necessary since type casts are easy with that)
						u32 control_bits = sampler_descriptors[i]->format_class == format_type::depth_float? (1u << 16) : 0u;
						control_bits |= tex.remap() & 0xFFFF;
						result.redirected_textures |= (1 << i);
						result.texture_scale[i][2] = std::bit_cast<f32>(control_bits);
						break;
					}
					case CELL_GCM_TEXTURE_DEPTH16:
					case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
					case CELL_GCM_TEXTURE_DEPTH24_D8:
					case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
					{
						const auto compare_mode = tex.zfunc();
						if (result.textures_alpha_kill[i] == 0 &&
							compare_mode < rsx::comparison_function::always &&
							compare_mode > rsx::comparison_function::never)
						{
							result.shadow_textures |= (1 << i);
							texture_control |= u32(tex.zfunc()) << 8;
						}
						break;
					}
					default:
						rsx_log.error("Depth texture bound to pipeline with unexpected format 0x%X", format);
					}
				}
				else if (!backend_config.supports_hw_renormalization)
				{
					switch (format)
					{
					case CELL_GCM_TEXTURE_A1R5G5B5:
					case CELL_GCM_TEXTURE_A4R4G4B4:
					case CELL_GCM_TEXTURE_D1R5G5B5:
					case CELL_GCM_TEXTURE_R5G5B5A1:
					case CELL_GCM_TEXTURE_R5G6B5:
					case CELL_GCM_TEXTURE_R6G5B5:
						texture_control |= (1 << 5);
						break;
					default:
						break;
					}
				}

				if (const auto srgb_mask = tex.gamma())
				{
					switch (format)
					{
					case CELL_GCM_TEXTURE_DEPTH24_D8:
					case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
					case CELL_GCM_TEXTURE_DEPTH16:
					case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
					case CELL_GCM_TEXTURE_X16:
					case CELL_GCM_TEXTURE_Y16_X16:
					case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
					case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
					case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
					case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
					case CELL_GCM_TEXTURE_X32_FLOAT:
					case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
						//Special data formats (XY, HILO, DEPTH) are not RGB formats
						//Ignore gamma flags
						break;
					default:
						texture_control |= srgb_mask;
						break;
					}
				}

#ifdef __APPLE__
				texture_control |= (sampler_descriptors[i]->encoded_component_map() << 16);
#endif
				result.texture_scale[i][3] = std::bit_cast<f32>(texture_control);
			}
		}

		//Sanity checks
		if (result.ctrl & CELL_GCM_SHADER_CONTROL_DEPTH_EXPORT)
		{
			//Check that the depth stage is not disabled
			if (!rsx::method_registers.depth_test_enabled())
			{
				rsx_log.error("FS exports depth component but depth test is disabled (INVALID_OPERATION)");
			}
		}
	}

	void thread::reset()
	{
		rsx::method_registers.reset();
	}

	void thread::init(u32 ctrlAddress)
	{
		ctrl = vm::_ptr<RsxDmaControl>(ctrlAddress);
		flip_status = CELL_GCM_DISPLAY_FLIP_STATUS_DONE;

		memset(display_buffers, 0, sizeof(display_buffers));

		on_init_rsx();
		m_rsx_thread_exiting = false;
	}

	GcmTileInfo *thread::find_tile(u32 offset, u32 location)
	{
		for (GcmTileInfo &tile : tiles)
		{
			if (!tile.binded || (tile.location & 1) != (location & 1))
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
		u32 address = get_address(offset, location, HERE);

		GcmTileInfo *tile = find_tile(offset, location);
		u32 base = 0;

		if (tile)
		{
			base = offset - tile->offset;
			address = get_address(tile->offset, location, HERE);
		}

		return{ address, base, tile, vm::_ptr<u8>(address) };
	}

	std::pair<u32, u32> thread::calculate_memory_requirements(const vertex_input_layout& layout, u32 first_vertex, u32 vertex_count)
	{
		u32 persistent_memory_size = 0;
		u32 volatile_memory_size = 0;

		volatile_memory_size += ::size32(layout.referenced_registers) * 16u;

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

			persistent_memory_size = layout.calculate_interleaved_memory_requirements(first_vertex, vertex_count);
		}

		return std::make_pair(persistent_memory_size, volatile_memory_size);
	}

	void thread::fill_vertex_layout_state(const vertex_input_layout& layout, u32 first_vertex, u32 vertex_count, s32* buffer, u32 persistent_offset_base, u32 volatile_offset_base)
	{
		std::array<s32, 16> offset_in_block = {};
		u32 volatile_offset = volatile_offset_base;
		u32 persistent_offset = persistent_offset_base;

		//NOTE: Order is important! Transient ayout is always push_buffers followed by register data
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
			u32 inline_data_offset = volatile_offset;
			for (const auto& attrib : block.locations)
			{
				auto &info = rsx::method_registers.vertex_arrays_info[attrib.index];

				offset_in_block[attrib.index] = inline_data_offset;
				inline_data_offset += rsx::get_vertex_type_size_on_host(info.type(), info.size());
			}
		}
		else
		{
			for (const auto &block : layout.interleaved_blocks)
			{
				for (const auto& attrib : block.locations)
				{
					const u32 local_address = (rsx::method_registers.vertex_arrays_info[attrib.index].offset() & 0x7fffffff);
					offset_in_block[attrib.index] = persistent_offset + (local_address - block.base_offset);
				}

				const auto range = block.calculate_required_range(first_vertex, vertex_count);
				persistent_offset += block.attribute_stride * range.second;
			}
		}

		// Fill the data
		// Each descriptor field is 64 bits wide
		// [0-8] attribute stride
		// [8-24] attribute divisor
		// [24-27] attribute type
		// [27-30] attribute size
		// [30-31] reserved
		// [31-60] starting offset
		// [60-21] swap bytes flag
		// [61-22] volatile flag
		// [62-63] modulo enable flag

		const s32 default_frequency_mask = (1 << 8);
		const s32 swap_storage_mask = (1 << 29);
		const s32 volatile_storage_mask = (1 << 30);
		const s32 modulo_op_frequency_mask = (1 << 31);

		const u32 modulo_mask = rsx::method_registers.frequency_divider_operation_mask();
		const auto max_index = (first_vertex + vertex_count) - 1;

		for (u8 index = 0; index < rsx::limits::vertex_count; ++index)
		{
			if (layout.attribute_placement[index] == attribute_buffer_placement::none)
			{
				reinterpret_cast<u64*>(buffer)[index] = 0;
				continue;
			}

			rsx::vertex_base_type type = {};
			s32 size = 0;
			s32 attrib0 = 0;
			s32 attrib1 = 0;

			if (layout.attribute_placement[index] == attribute_buffer_placement::transient)
			{
				if (rsx::method_registers.current_draw_clause.command == rsx::draw_command::inlined_array)
				{
					const auto &info = rsx::method_registers.vertex_arrays_info[index];

					if (!info.size())
					{
						// Register
						const auto& reginfo = rsx::method_registers.register_vertex_info[index];
						type = reginfo.type;
						size = reginfo.size;

						attrib0 = rsx::get_vertex_type_size_on_host(type, size);
					}
					else
					{
						// Array
						type = info.type();
						size = info.size();

						attrib0 = layout.interleaved_blocks[0].attribute_stride | default_frequency_mask;
					}
				}
				else
				{
					// Data is either from an immediate render or register input
					// Immediate data overrides register input

					if (rsx::method_registers.current_draw_clause.is_immediate_draw &&
						vertex_push_buffers[index].vertex_count > 1)
					{
						// Push buffer
						const auto &info = vertex_push_buffers[index];
						type = info.type;
						size = info.size;

						attrib0 = rsx::get_vertex_type_size_on_host(type, size) | default_frequency_mask;
					}
					else
					{
						// Register
						const auto& info = rsx::method_registers.register_vertex_info[index];
						type = info.type;
						size = info.size;

						attrib0 = rsx::get_vertex_type_size_on_host(type, size);
					}
				}

				attrib1 |= volatile_storage_mask;
			}
			else
			{
				auto &info = rsx::method_registers.vertex_arrays_info[index];
				type = info.type();
				size = info.size();

				auto stride = info.stride();
				attrib0 = stride;

				if (stride > 0) //when stride is 0, input is not an array but a single element
				{
					const u32 frequency = info.frequency();
					switch (frequency)
					{
					case 0:
					case 1:
					{
						attrib0 |= default_frequency_mask;
						break;
					}
					default:
					{
						if (modulo_mask & (1 << index))
						{
							if (max_index >= frequency)
							{
								// Only set modulo mask if a modulo op is actually necessary!
								// This requires that the uploaded range for this attr = [0, freq-1]
								// Ignoring modulo op if the rendered range does not wrap allows for range optimization
								attrib0 |= (frequency << 8);
								attrib1 |= modulo_op_frequency_mask;
							}
							else
							{
								attrib0 |= default_frequency_mask;
							}
						}
						else
						{
							// Division
							attrib0 |= (frequency << 8);
						}
						break;
					}
					}
				}
			} //end attribute placement check

			// If data is passed via registers, it is already received in little endian
			const bool is_be_type = (layout.attribute_placement[index] != attribute_buffer_placement::transient);
			bool to_swap_bytes = is_be_type;

			switch (type)
			{
			case rsx::vertex_base_type::cmp:
				// Compressed 4 components into one 4-byte value
				size = 1;
				break;
			case rsx::vertex_base_type::ub:
			case rsx::vertex_base_type::ub256:
				// These are single byte formats, but inverted order (BGRA vs ARGB) when passed via registers
				to_swap_bytes = (layout.attribute_placement[index] == attribute_buffer_placement::transient);
				break;
			default:
				break;
			}

			if (to_swap_bytes) attrib1 |= swap_storage_mask;

			attrib0 |= (static_cast<s32>(type) << 24);
			attrib0 |= (size << 27);
			attrib1 |= offset_in_block[index];

			buffer[index * 2 + 0] = attrib0;
			buffer[index * 2 + 1] = attrib1;
		}
	}

	void thread::write_vertex_data_to_memory(const vertex_input_layout& layout, u32 first_vertex, u32 vertex_count, void *persistent_data, void *volatile_data)
	{
		auto transient = static_cast<char*>(volatile_data);
		auto persistent = static_cast<char*>(persistent_data);

		auto &draw_call = rsx::method_registers.current_draw_clause;

		if (transient != nullptr)
		{
			if (draw_call.command == rsx::draw_command::inlined_array)
			{
				for (const u8 index : layout.referenced_registers)
				{
					memcpy(transient, rsx::method_registers.register_vertex_info[index].data.data(), 16);
					transient += 16;
				}

				memcpy(transient, draw_call.inline_vertex_array.data(), draw_call.inline_vertex_array.size() * sizeof(u32));
				//Is it possible to reference data outside of the inlined array?
				return;
			}

			//NOTE: Order is important! Transient layout is always push_buffers followed by register data
			if (draw_call.is_immediate_draw)
			{
				//NOTE: It is possible for immediate draw to only contain index data, so vertex data can be in persistent memory
				for (const auto &info : layout.volatile_blocks)
				{
					memcpy(transient, vertex_push_buffers[info.first].data.data(), info.second);
					transient += info.second;
				}
			}

			for (const u8 index : layout.referenced_registers)
			{
				memcpy(transient, rsx::method_registers.register_vertex_info[index].data.data(), 16);
				transient += 16;
			}
		}

		if (persistent != nullptr)
		{
			for (const auto &block : layout.interleaved_blocks)
			{
				auto range = block.calculate_required_range(first_vertex, vertex_count);

				const u32 data_size = range.second * block.attribute_stride;
				const u32 vertex_base = range.first * block.attribute_stride;

				g_dma_manager.copy(persistent, vm::_ptr<char>(block.real_offset_address) + vertex_base, data_size);
				persistent += data_size;
			}
		}
	}

	void thread::flip(const display_flip_info_t& info)
	{
		if (async_flip_requested & flip_request::any)
		{
			// Deferred flip
			if (info.emu_flip)
			{
				async_flip_requested.clear(flip_request::emu_requested);
			}
			else
			{
				async_flip_requested.clear(flip_request::native_ui);
			}
		}

		if (info.emu_flip)
		{
			performance_counters.sampled_frames++;
		}
	}

	void thread::check_zcull_status(bool framebuffer_swap)
	{
		if (g_cfg.video.disable_zcull_queries)
			return;

		bool testing_enabled = zcull_pixel_cnt_enabled || zcull_stats_enabled;

		if (framebuffer_swap)
		{
			zcull_surface_active = false;
			const u32 zeta_address = m_depth_surface_info.address;

			if (zeta_address)
			{
				//Find zeta address in bound zculls
				for (const auto& zcull : zculls)
				{
					if (zcull.binded)
					{
						const u32 rsx_address = rsx::get_address(zcull.offset, CELL_GCM_LOCATION_LOCAL, HERE);
						if (rsx_address == zeta_address)
						{
							zcull_surface_active = true;
							break;
						}
					}
				}
			}
		}

		zcull_ctrl->set_enabled(this, zcull_rendering_enabled);
		zcull_ctrl->set_active(this, zcull_rendering_enabled && testing_enabled && zcull_surface_active);
	}

	void thread::clear_zcull_stats(u32 type)
	{
		if (g_cfg.video.disable_zcull_queries)
			return;

		zcull_ctrl->clear(this);
	}

	void thread::get_zcull_stats(u32 type, vm::addr_t sink)
	{
		u32 value = 0;
		if (!g_cfg.video.disable_zcull_queries)
		{
			switch (type)
			{
			case CELL_GCM_ZPASS_PIXEL_CNT:
			case CELL_GCM_ZCULL_STATS:
			case CELL_GCM_ZCULL_STATS1:
			case CELL_GCM_ZCULL_STATS2:
			case CELL_GCM_ZCULL_STATS3:
			{
				zcull_ctrl->read_report(this, sink, type);
				return;
			}
			default:
				rsx_log.error("Unknown zcull stat type %d", type);
				break;
			}
		}

		vm::_ref<atomic_t<CellGcmReportData>>(sink).store({ timestamp(), value, 0});
	}

	u32 thread::copy_zcull_stats(u32 memory_range_start, u32 memory_range, u32 destination)
	{
		return zcull_ctrl->copy_reports_to(memory_range_start, memory_range, destination);
	}

	void thread::enable_conditional_rendering(vm::addr_t ref)
	{
		cond_render_ctrl.enable_conditional_render(this, ref);

		auto result = zcull_ctrl->find_query(ref, true);
		if (result.found)
		{
			if (!result.queries.empty())
			{
				cond_render_ctrl.set_eval_sources(result.queries);
				sync_hint(FIFO_hint::hint_conditional_render_eval, cond_render_ctrl.eval_sources.front());
			}
			else
			{
				bool failed = (result.raw_zpass_result == 0);
				cond_render_ctrl.set_eval_result(this, failed);
			}
		}
		else
		{
			cond_render_ctrl.eval_result(this);
		}
	}

	void thread::disable_conditional_rendering()
	{
		cond_render_ctrl.disable_conditional_render(this);
	}

	void thread::begin_conditional_rendering(const std::vector<reports::occlusion_query_info*>& /*sources*/)
	{
		cond_render_ctrl.hw_cond_active = true;
		cond_render_ctrl.eval_sources.clear();
	}

	void thread::end_conditional_rendering()
	{
		cond_render_ctrl.hw_cond_active = false;
	}

	void thread::sync()
	{
		if (zcull_ctrl->has_pending())
		{
			if (g_cfg.video.relaxed_zcull_sync)
			{
				// Emit zcull sync hint and update; guarantees results to be written shortly after this event
				zcull_ctrl->update(this, 0, true);
			}
			else
			{
				zcull_ctrl->sync(this);
			}
		}

		// Fragment constants may have been updated
		m_graphics_state |= rsx::pipeline_state::fragment_constants_dirty;

		// DMA sync; if you need this, don't use MTRSX
		// g_dma_manager.sync();

		//TODO: On sync every sub-unit should finish any pending tasks
		//Might cause zcull lockup due to zombie 'unclaimed reports' which are not forcefully removed currently
		//verify (HERE), async_tasks_pending.load() == 0;
	}

	void thread::sync_hint(FIFO_hint /*hint*/, void* args)
	{
		zcull_ctrl->on_sync_hint(args);
	}

	void thread::flush_fifo()
	{
		// Make sure GET value is exposed before sync points
		fifo_ctrl->sync_get();
	}

	void thread::recover_fifo()
	{
		// Error. Should reset the queue
		fifo_ctrl->set_get(restore_point);
		fifo_ret_addr = saved_fifo_ret;
		std::this_thread::sleep_for(1ms);
		invalid_command_interrupt_raised = false;

		if (std::exchange(in_begin_end, false) && !rsx::method_registers.current_draw_clause.empty())
		{
			execute_nop_draw();
			rsx::thread::end();
		}
	}

	void thread::fifo_wake_delay(u64 div)
	{
		// TODO: Nanoseconds accuracy
		u64 remaining = g_cfg.video.driver_wakeup_delay;

		if (!remaining)
		{
			return;
		}

		// Some cases do not need full delay
		remaining = ::aligned_div(remaining, div);
		const u64 until = get_system_time() + remaining;

		while (true)
		{
#ifdef __linux__
			// NOTE: Assumption that timer initialization has succeeded
			u64 host_min_quantum = remaining <= 1000 ? 10 : 50;
#else
			// Host scheduler quantum for windows (worst case)
			// NOTE: On ps3 this function has very high accuracy
			constexpr u64 host_min_quantum = 500;
#endif
			if (remaining >= host_min_quantum)
			{
#ifdef __linux__
				// Do not wait for the last quantum to avoid loss of accuracy
				thread_ctrl::wait_for(remaining - ((remaining % host_min_quantum) + host_min_quantum), false);
#else
				// Wait on multiple of min quantum for large durations to avoid overloading low thread cpus
				thread_ctrl::wait_for(remaining - (remaining % host_min_quantum), false);
#endif
			}
			// TODO: Determine best value for yield delay
			else if (remaining >= host_min_quantum / 2)
			{
				std::this_thread::yield();
			}
			else
			{
				busy_wait(100);
			}

			const u64 current = get_system_time();

			if (current >= until)
			{
				break;
			}

			remaining = until - current;
		}
	}

	u32 thread::get_fifo_cmd()
	{
		// Last fifo cmd for logging and utility
		return fifo_ctrl->last_cmd();
	}

	flags32_t thread::read_barrier(u32 memory_address, u32 memory_range, bool unconditional)
	{
		flags32_t zcull_flags = (unconditional)? reports::sync_none : reports::sync_defer_copy;
		return zcull_ctrl->read_barrier(this, memory_address, memory_range, zcull_flags);
	}

	void thread::notify_zcull_info_changed()
	{
		check_zcull_status(false);
	}

	void thread::on_notify_memory_mapped(u32 address, u32 size)
	{
		// In the case where an unmap is followed shortly after by a remap of the same address space
		// we must block until RSX has invalidated the memory
		// or lock m_mtx_task and do it ourselves

		if (m_rsx_thread_exiting)
			return;

		reader_lock lock(m_mtx_task);

		const auto map_range = address_range::start_length(address, size);

		if (!m_invalidated_memory_range.valid())
			return;

		if (m_invalidated_memory_range.overlaps(map_range))
		{
			lock.upgrade();
			handle_invalidated_memory_range();
		}
	}

	void thread::on_notify_memory_unmapped(u32 address, u32 size)
	{
		if (!m_rsx_thread_exiting && address < rsx::constants::local_mem_base)
		{
			if (!isHLE)
			{
				// Each bit represents io entry to be unmapped
				u64 unmap_status[512 / 64]{};

				for (u32 ea = address >> 20, end = ea + (size >> 20); ea < end; ea++)
				{
					u32 io = RSXIOMem.io[ea];

					if (io < 512)
					{
						unmap_status[io / 64] |= 1ull << (io & 63);
						RSXIOMem.ea[io].raw() = 0xFFFF;
						RSXIOMem.io[ea].raw() = 0xFFFF;
					}
				}

				for (u32 i = 0; i < std::size(unmap_status); i++)
				{
					// TODO: Check order when sending multiple events
					if (u64 to_unmap = unmap_status[i])
					{
						// Each 64 entries are grouped by a bit
						const u64 io_event = 0x100000000ull << i;
						sys_event_port_send(g_fxo->get<lv2_rsx_config>()->rsx_event_port, 0, io_event, to_unmap);
					}
				}
			}
			else
			{
				// TODO: Fix this
				u32 ea = address >> 20, io = RSXIOMem.io[ea];

				for (const u32 end = ea + (size >> 20); ea < end;)
				{
					offsetTable.ioAddress[ea++] = 0xFFFF;
					offsetTable.eaAddress[io++] = 0xFFFF;
				}
			}

			// Queue up memory invalidation
			std::lock_guard lock(m_mtx_task);
			const bool existing_range_valid = m_invalidated_memory_range.valid();
			const auto unmap_range = address_range::start_length(address, size);

			if (existing_range_valid && m_invalidated_memory_range.touches(unmap_range))
			{
				// Merge range-to-invalidate in case of consecutive unmaps
				m_invalidated_memory_range.set_min_max(unmap_range);
			}
			else
			{
				if (existing_range_valid)
				{
					// We can only delay consecutive unmaps.
					// Otherwise, to avoid VirtualProtect failures, we need to do the invalidation here
					handle_invalidated_memory_range();
				}

				m_invalidated_memory_range = unmap_range;
			}
		}
	}

	// NOTE: m_mtx_task lock must be acquired before calling this method
	void thread::handle_invalidated_memory_range()
	{
		if (!m_invalidated_memory_range.valid())
			return;

		on_invalidate_memory_range(m_invalidated_memory_range, rsx::invalidation_cause::unmap);
		m_invalidated_memory_range.invalidate();
	}

	//Pause/cont wrappers for FIFO ctrl. Never call this from rsx thread itself!
	void thread::pause()
	{
		external_interrupt_lock.store(true);
		while (!external_interrupt_ack.load())
		{
			if (Emu.IsStopped())
				break;

			_mm_pause();
		}
		external_interrupt_ack.store(false);
	}

	void thread::unpause()
	{
		// TODO: Clean this shit up
		external_interrupt_lock.store(false);
	}

	u32 thread::get_load()
	{
		//Average load over around 30 frames
		if (!performance_counters.last_update_timestamp || performance_counters.sampled_frames > 30)
		{
			const auto timestamp = get_system_time();
			const auto idle = performance_counters.idle_time.load();
			const auto elapsed = timestamp - performance_counters.last_update_timestamp;

			if (elapsed > idle)
				performance_counters.approximate_load = static_cast<u32>((elapsed - idle) * 100 / elapsed);
			else
				performance_counters.approximate_load = 0u;

			performance_counters.idle_time = 0;
			performance_counters.sampled_frames = 0;
			performance_counters.last_update_timestamp = timestamp;
		}

		return performance_counters.approximate_load;
	}

	void thread::on_frame_end(u32 buffer, bool forced)
	{
		// Marks the end of a frame scope GPU-side
		if (user_asked_for_frame_capture && !capture_current_frame)
		{
			capture_current_frame = true;
			user_asked_for_frame_capture = false;
			frame_debug.reset();
			frame_capture.reset();

			// random number just to jumpstart the size
			frame_capture.replay_commands.reserve(8000);

			// capture first tile state with nop cmd
			rsx::frame_capture_data::replay_command replay_cmd;
			replay_cmd.rsx_command = std::make_pair(NV4097_NO_OPERATION, 0);
			frame_capture.replay_commands.push_back(replay_cmd);
			capture::capture_display_tile_state(this, frame_capture.replay_commands.back());
		}
		else if (capture_current_frame)
		{
			capture_current_frame = false;
			std::stringstream os;
			cereal::BinaryOutputArchive archive(os);
			const std::string& filePath = fs::get_config_dir() + "captures/" + Emu.GetTitleID() + "_" + date_time::current_time_narrow() + "_capture.rrc";
			archive(frame_capture);
			{
				// todo: may want to compress this data?
				fs::file f(filePath, fs::rewrite);
				f.write(os.str());
			}

			rsx_log.success("capture successful: %s", filePath.c_str());

			frame_capture.reset();
			Emu.Pause();
		}

		// Reset ZCULL ctrl
		// NOTE: A semaphore release is part of RSX flip control and will handle ZCULL sync
		// TODO: These routines belong in the state reset routines controlled by sys_rsx and cellGcmSetFlip
		zcull_ctrl->set_active(this, false, true);
		zcull_ctrl->clear(this);

		// Save current state
		m_queued_flip.stats = m_frame_stats;
		m_queued_flip.push(buffer);
		m_queued_flip.skip_frame = skip_current_frame;

		if (!forced) [[likely]]
		{
			if (!g_cfg.video.disable_FIFO_reordering)
			{
				// Try to enable FIFO optimizations
				// Only rarely useful for some games like RE4
				m_flattener.evaluate_performance(m_frame_stats.draw_calls);
			}

			if (g_cfg.video.frame_skip_enabled)
			{
				m_skip_frame_ctr++;

				if (m_skip_frame_ctr == g_cfg.video.consequtive_frames_to_draw)
					m_skip_frame_ctr = -g_cfg.video.consequtive_frames_to_skip;

				skip_current_frame = (m_skip_frame_ctr < 0);
			}
		}
		else
		{
			if (!g_cfg.video.disable_FIFO_reordering)
			{
				// Flattener is unusable due to forced random flips
				m_flattener.force_disable();
			}

			if (g_cfg.video.frame_skip_enabled)
			{
				rsx_log.error("Frame skip is not compatible with this application");
			}
		}

		// Reset current stats
		m_frame_stats = {};
	}

	void thread::request_emu_flip(u32 buffer)
	{
		if (is_current_thread()) // requested through command buffer
		{
			// NOTE: The flip will clear any queued flip requests
			handle_emu_flip(buffer);
		}
		else // requested 'manually' through ppu syscall
		{
			if (async_flip_requested & flip_request::emu_requested)
			{
				// ignore multiple requests until previous happens
				return;
			}

			async_flip_buffer = buffer;
			async_flip_requested |= flip_request::emu_requested;
		}
	}

	void thread::handle_emu_flip(u32 buffer)
	{
		if (m_queued_flip.in_progress)
		{
			// Rescursion not allowed!
			return;
		}

		if (!m_queued_flip.pop(buffer))
		{
			// Frame was not queued before flipping
			on_frame_end(buffer, true);
			verify(HERE), m_queued_flip.pop(buffer);
		}

		double limit = 0.;
		switch (g_cfg.video.frame_limit)
		{
		case frame_limit_type::none: limit = 0.; break;
		case frame_limit_type::_59_94: limit = 59.94; break;
		case frame_limit_type::_50: limit = 50.; break;
		case frame_limit_type::_60: limit = 60.; break;
		case frame_limit_type::_30: limit = 30.; break;
		case frame_limit_type::_auto: limit = fps_limit; break; // TODO
		default:
			break;
		}

		if (limit)
		{
			const u64 time = get_system_time() - Emu.GetPauseTime() - start_rsx_time;

			if (int_flip_index == 0)
			{
				start_rsx_time = time;
			}
			else
			{
				// Convert limit to expected time value
				double expected = int_flip_index * 1000000. / limit;

				while (time >= expected + 1000000. / limit)
				{
					expected = int_flip_index++ * 1000000. / limit;
				}

				if (expected > time + 1000)
				{
					const auto delay_us = static_cast<s64>(expected - time);
					std::this_thread::sleep_for(std::chrono::milliseconds{ delay_us / 1000 });
					performance_counters.idle_time += delay_us;
				}
			}
		}

		int_flip_index++;

		current_display_buffer = buffer;
		m_queued_flip.emu_flip = true;
		m_queued_flip.in_progress = true;

		flip(m_queued_flip);

		last_flip_time = get_system_time() - 1000000;
		flip_status = CELL_GCM_DISPLAY_FLIP_STATUS_DONE;
		m_queued_flip.in_progress = false;

		if (flip_handler)
		{
			intr_thread->cmd_list
			({
				{ ppu_cmd::set_args, 1 }, u64{ 1 },
				{ ppu_cmd::lle_call, flip_handler },
				{ ppu_cmd::sleep, 0 }
			});

			thread_ctrl::notify(*intr_thread);
		}

		sys_rsx_context_attribute(0x55555555, 0xFEC, buffer, 0, 0, 0);
	}


	namespace reports
	{
		ZCULL_control::ZCULL_control()
		{
			for (auto& query : m_occlusion_query_data)
			{
				m_free_occlusion_pool.push(&query);
			}
		}

		ZCULL_control::~ZCULL_control()
		{}

		void ZCULL_control::set_enabled(class ::rsx::thread* ptimer, bool state, bool flush_queue)
		{
			if (state != enabled)
			{
				enabled = state;

				if (active && !enabled)
					set_active(ptimer, false, flush_queue);
			}
		}

		void ZCULL_control::set_active(class ::rsx::thread* ptimer, bool state, bool flush_queue)
		{
			if (state != active)
			{
				active = state;

				if (state)
				{
					verify(HERE), enabled && m_current_task == nullptr;
					allocate_new_query(ptimer);
					begin_occlusion_query(m_current_task);
				}
				else
				{
					verify(HERE), m_current_task;
					if (m_current_task->num_draws)
					{
						end_occlusion_query(m_current_task);
						m_current_task->active = false;
						m_current_task->pending = true;
						m_current_task->sync_tag = m_timer++;
						m_current_task->timestamp = m_tsc;

						m_pending_writes.push_back({});
						m_pending_writes.back().query = m_current_task;
						ptimer->async_tasks_pending++;
					}
					else
					{
						discard_occlusion_query(m_current_task);
						free_query(m_current_task);
						m_current_task->active = false;
					}

					m_current_task = nullptr;
					update(ptimer, 0u, flush_queue);
				}
			}
		}

		void ZCULL_control::read_report(::rsx::thread* ptimer, vm::addr_t sink, u32 type)
		{
			if (m_current_task && type == CELL_GCM_ZPASS_PIXEL_CNT)
			{
				m_current_task->owned = true;
				end_occlusion_query(m_current_task);
				m_pending_writes.push_back({});

				m_current_task->active = false;
				m_current_task->pending = true;
				m_current_task->timestamp = m_tsc;
				m_current_task->sync_tag = m_timer++;
				m_pending_writes.back().query = m_current_task;

				allocate_new_query(ptimer);
				begin_occlusion_query(m_current_task);
			}
			else
			{
				// Spam; send null query down the pipeline to copy the last result
				// Might be used to capture a timestamp (verify)

				if (m_pending_writes.empty())
				{
					// No need to queue this if there is no pending request in the pipeline anyway
					write(sink, ptimer->timestamp(), type, m_statistics_map[m_statistics_tag_id]);
					return;
				}

				m_pending_writes.push_back({});
			}

			auto forwarder = &m_pending_writes.back();
			for (auto It = m_pending_writes.rbegin(); It != m_pending_writes.rend(); It++)
			{
				if (!It->sink)
				{
					It->counter_tag = m_statistics_tag_id;
					It->sink = sink;
					It->type = type;

					if (forwarder != &(*It))
					{
						// Not the last one in the chain, forward the writing operation to the last writer
						// Usually comes from truncated queries caused by disabling the testing
						verify(HERE), It->query;

						It->forwarder = forwarder;
						It->query->owned = true;
					}

					continue;
				}

				break;
			}

			ptimer->async_tasks_pending++;

			if (m_statistics_map[m_statistics_tag_id] != 0)
			{
				// Flush guaranteed results; only one positive is needed
				update(ptimer);
			}
		}

		void ZCULL_control::allocate_new_query(::rsx::thread* ptimer)
		{
			int retries = 0;
			while (true)
			{
				if (!m_free_occlusion_pool.empty())
				{
					m_current_task = m_free_occlusion_pool.top();
					m_free_occlusion_pool.pop();

					m_current_task->num_draws = 0;
					m_current_task->result = 0;
					m_current_task->active = true;
					m_current_task->owned = false;
					m_current_task->sync_tag = 0;
					m_current_task->timestamp = 0;
					return;
				}

				if (retries > 0)
				{
					fmt::throw_exception("Allocation failed!");
				}

				// All slots are occupied, try to pop the earliest entry

				if (!m_pending_writes.front().query)
				{
					// If this happens, the assert above will fire. There should never be a queue header with no work to be done
					rsx_log.error("Close to our death.");
				}

				m_next_tsc = 0;
				update(ptimer, m_pending_writes.front().sink);

				retries++;
			}
		}

		void ZCULL_control::free_query(occlusion_query_info* query)
		{
			query->pending = false;
			m_free_occlusion_pool.push(query);
		}

		void ZCULL_control::clear(class ::rsx::thread* ptimer)
		{
			if (!m_pending_writes.empty())
			{
				//Remove any dangling/unclaimed queries as the information is lost anyway
				auto valid_size = m_pending_writes.size();
				for (auto It = m_pending_writes.rbegin(); It != m_pending_writes.rend(); ++It)
				{
					if (!It->sink)
					{
						discard_occlusion_query(It->query);
						free_query(It->query);
						valid_size--;
						ptimer->async_tasks_pending--;
						continue;
					}

					break;
				}

				m_pending_writes.resize(valid_size);
			}

			m_statistics_tag_id++;
			m_statistics_map[m_statistics_tag_id] = 0;
		}

		void ZCULL_control::on_draw()
		{
			if (m_current_task)
			{
				m_current_task->num_draws++;
				m_current_task->sync_tag = m_timer++;
			}
		}

		void ZCULL_control::on_sync_hint(void* args)
		{
			auto query = static_cast<occlusion_query_info*>(args);
			m_sync_tag = std::max(m_sync_tag, query->sync_tag);
		}

		void ZCULL_control::write(vm::addr_t sink, u64 timestamp, u32 type, u32 value)
		{
			verify(HERE), sink;

			switch (type)
			{
			case CELL_GCM_ZPASS_PIXEL_CNT:
				value = value ? UINT16_MAX : 0;
				break;
			case CELL_GCM_ZCULL_STATS3:
				value = value ? 0 : UINT16_MAX;
				break;
			case CELL_GCM_ZCULL_STATS2:
			case CELL_GCM_ZCULL_STATS1:
			case CELL_GCM_ZCULL_STATS:
			default:
				//Not implemented
				value = UINT32_MAX;
				break;
			}

			vm::_ref<atomic_t<CellGcmReportData>>(sink).store({ timestamp, value, 0});
		}

		void ZCULL_control::write(queued_report_write* writer, u64 timestamp, u32 value)
		{
			write(writer->sink, timestamp, writer->type, value);

			for (auto &addr : writer->sink_alias)
			{
				write(addr, timestamp, writer->type, value);
			}
		}

		void ZCULL_control::sync(::rsx::thread* ptimer)
		{
			if (!m_pending_writes.empty())
			{
				// Quick reverse scan to push commands ahead of time
				for (auto It = m_pending_writes.rbegin(); It != m_pending_writes.rend(); ++It)
				{
					if (It->query && It->query->num_draws)
					{
						if (It->query->sync_tag > m_sync_tag)
						{
							// rsx_log.trace("[Performance warning] Query hint emit during sync command.");
							ptimer->sync_hint(FIFO_hint::hint_zcull_sync, It->query);
						}

						break;
					}
				}

				u32 processed = 0;
				const bool has_unclaimed = (m_pending_writes.back().sink == 0);

				//Write all claimed reports unconditionally
				for (auto &writer : m_pending_writes)
				{
					if (!writer.sink)
						break;

					auto query = writer.query;
					u32 result = m_statistics_map[writer.counter_tag];

					if (query)
					{
						verify(HERE), query->pending;

						const bool implemented = (writer.type == CELL_GCM_ZPASS_PIXEL_CNT || writer.type == CELL_GCM_ZCULL_STATS3);
						if (implemented && !result && query->num_draws)
						{
							get_occlusion_query_result(query);

							if (query->result)
							{
								result += query->result;
								m_statistics_map[writer.counter_tag] = result;
							}
						}
						else
						{
							//Already have a hit, no need to retest
							discard_occlusion_query(query);
						}

						free_query(query);
					}

					if (!writer.forwarder)
					{
						// No other queries in the chain, write result
						write(&writer, ptimer->timestamp(), result);

						if (query && query->sync_tag == ptimer->cond_render_ctrl.eval_sync_tag)
						{
							const bool eval_failed = (result == 0);
							ptimer->cond_render_ctrl.set_eval_result(ptimer, eval_failed);
						}
					}

					processed++;
				}

				if (!has_unclaimed)
				{
					verify(HERE), processed == m_pending_writes.size();
					m_pending_writes.clear();
				}
				else
				{
					auto remaining = m_pending_writes.size() - processed;
					verify(HERE), remaining > 0;

					if (remaining == 1)
					{
						m_pending_writes[0] = std::move(m_pending_writes.back());
						m_pending_writes.resize(1);
					}
					else
					{
						std::move(m_pending_writes.begin() + processed, m_pending_writes.end(), m_pending_writes.begin());
						m_pending_writes.resize(remaining);
					}
				}

				//Delete all statistics caches but leave the current one
				for (auto It = m_statistics_map.begin(); It != m_statistics_map.end(); )
				{
					if (It->first == m_statistics_tag_id)
						++It;
					else
						It = m_statistics_map.erase(It);
				}

				//Decrement jobs counter
				ptimer->async_tasks_pending -= processed;
			}
		}

		void ZCULL_control::update(::rsx::thread* ptimer, u32 sync_address, bool hint)
		{
			if (m_pending_writes.empty())
			{
				return;
			}

			const auto& front = m_pending_writes.front();
			if (!front.sink)
			{
				// No writables in queue, abort
				return;
			}

			if (!sync_address)
			{
				if (hint || ptimer->async_tasks_pending >= max_safe_queue_depth)
				{
					// Prepare the whole queue for reading. This happens when zcull activity is disabled or queue is too long
					for (auto It = m_pending_writes.rbegin(); It != m_pending_writes.rend(); ++It)
					{
						if (It->query)
						{
							if (It->query->num_draws && It->query->sync_tag > m_sync_tag)
							{
								ptimer->sync_hint(FIFO_hint::hint_zcull_sync, It->query);
								verify(HERE), It->query->sync_tag <= m_sync_tag;
							}

							break;
						}
					}
				}

				if (m_tsc = get_system_time(); m_tsc < m_next_tsc)
				{
					return;
				}
				else
				{
					// Schedule ahead
					m_next_tsc = m_tsc + min_zcull_tick_us;

					// Schedule a queue flush if needed
					if (!g_cfg.video.relaxed_zcull_sync &&
						front.query && front.query->num_draws && front.query->sync_tag > m_sync_tag)
					{
						const auto elapsed = m_tsc - front.query->timestamp;
						if (elapsed > max_zcull_delay_us)
						{
							ptimer->sync_hint(FIFO_hint::hint_zcull_sync, front.query);
							verify(HERE), front.query->sync_tag <= m_sync_tag;
						}

						return;
					}
				}
			}

			u32 stat_tag_to_remove = m_statistics_tag_id;
			u32 processed = 0;
			for (auto &writer : m_pending_writes)
			{
				if (!writer.sink)
					break;

				if (writer.counter_tag != stat_tag_to_remove &&
					stat_tag_to_remove != m_statistics_tag_id)
				{
					//If the stat id is different from this stat id and the queue is advancing,
					//its guaranteed that the previous tag has no remaining writes as the queue is ordered
					m_statistics_map.erase(stat_tag_to_remove);
					stat_tag_to_remove = m_statistics_tag_id;
				}

				auto query = writer.query;
				u32 result = m_statistics_map[writer.counter_tag];

				const bool force_read = (sync_address != 0);
				if (force_read && writer.sink == sync_address)
				{
					// Forced reads end here
					sync_address = 0;
				}

				if (query)
				{
					verify(HERE), query->pending;

					const bool implemented = (writer.type == CELL_GCM_ZPASS_PIXEL_CNT || writer.type == CELL_GCM_ZCULL_STATS3);
					if (force_read)
					{
						if (implemented && !result && query->num_draws)
						{
							get_occlusion_query_result(query);

							if (query->result)
							{
								result += query->result;
								m_statistics_map[writer.counter_tag] = result;
							}
						}
						else
						{
							//No need to read this
							discard_occlusion_query(query);
						}
					}
					else
					{
						if (implemented && !result && query->num_draws)
						{
							//Maybe we get lucky and results are ready
							if (check_occlusion_query_status(query))
							{
								get_occlusion_query_result(query);
								if (query->result)
								{
									result += query->result;
									m_statistics_map[writer.counter_tag] = result;
								}
							}
							else
							{
								//Too early; abort
								break;
							}
						}
						else
						{
							//Not necessary to read the result anymore
							discard_occlusion_query(query);
						}
					}

					free_query(query);
				}

				stat_tag_to_remove = writer.counter_tag;

				// only zpass supported right now
				if (!writer.forwarder)
				{
					// No other queries in the chain, write result
					write(&writer, ptimer->timestamp(), result);

					if (query && query->sync_tag == ptimer->cond_render_ctrl.eval_sync_tag)
					{
						const bool eval_failed = (result == 0);
						ptimer->cond_render_ctrl.set_eval_result(ptimer, eval_failed);
					}
				}

				processed++;
			}

			if (stat_tag_to_remove != m_statistics_tag_id)
				m_statistics_map.erase(stat_tag_to_remove);

			if (processed)
			{
				auto remaining = m_pending_writes.size() - processed;
				if (remaining == 1)
				{
					m_pending_writes[0] = std::move(m_pending_writes.back());
					m_pending_writes.resize(1);
				}
				else if (remaining)
				{
					std::move(m_pending_writes.begin() + processed, m_pending_writes.end(), m_pending_writes.begin());
					m_pending_writes.resize(remaining);
				}
				else
				{
					m_pending_writes.clear();
				}

				ptimer->async_tasks_pending -= processed;
			}
		}

		flags32_t ZCULL_control::read_barrier(::rsx::thread* ptimer, u32 memory_address, u32 memory_range, flags32_t flags)
		{
			if (m_pending_writes.empty())
				return result_none;

			const auto memory_end = memory_address + memory_range;
			u32 sync_address = 0;
			occlusion_query_info* query = nullptr;

			for (auto It = m_pending_writes.crbegin(); It != m_pending_writes.crend(); ++It)
			{
				if (sync_address)
				{
					if (It->query)
					{
						sync_address = It->sink;
						query = It->query;
						break;
					}

					continue;
				}

				if (It->sink >= memory_address && It->sink < memory_end)
				{
					sync_address = It->sink;

					// NOTE: If application is spamming requests, there may be no query attached
					if (It->query)
					{
						query = It->query;
						break;
					}
				}
			}

			if (!sync_address || !query)
				return result_none;

			if (!(flags & sync_defer_copy))
			{
				if (!(flags & sync_no_notify))
				{
					if (query->sync_tag > m_sync_tag) [[unlikely]]
					{
						ptimer->sync_hint(FIFO_hint::hint_zcull_sync, query);
						verify(HERE), m_sync_tag >= query->sync_tag;
					}
				}

				update(ptimer, sync_address);
				return result_none;
			}

			return result_zcull_intr;
		}

		query_search_result ZCULL_control::find_query(vm::addr_t sink_address, bool all)
		{
			query_search_result result{};
			u32 stat_id = 0;

			for (auto It = m_pending_writes.crbegin(); It != m_pending_writes.crend(); ++It)
			{
				if (stat_id) [[unlikely]]
				{
					if (It->counter_tag != stat_id)
					{
						if (result.found)
						{
							// Some result was found, return it instead
							break;
						}

						// Zcull stats were cleared between this query and the required stats, result can only be 0
						return { true, 0, {} };
					}

					if (It->query && It->query->num_draws)
					{
						result.found = true;
						result.queries.push_back(It->query);

						if (!all)
						{
							break;
						}
					}
				}
				else if (It->sink == sink_address)
				{
					if (It->query && It->query->num_draws)
					{
						result.found = true;
						result.queries.push_back(It->query);

						if (!all)
						{
							break;
						}
					}

					stat_id = It->counter_tag;
				}
			}

			return result;
		}

		u32 ZCULL_control::copy_reports_to(u32 start, u32 range, u32 dest)
		{
			u32 bytes_to_write = 0;
			const auto memory_range = utils::address_range::start_length(start, range);
			for (auto &writer : m_pending_writes)
			{
				if (!writer.sink)
					break;

				if (!writer.forwarder && memory_range.overlaps(writer.sink))
				{
					u32 address = (writer.sink - start) + dest;
					writer.sink_alias.push_back(vm::cast(address));
				}
			}

			return bytes_to_write;
		}


		// Conditional rendering helpers
		void conditional_render_eval::reset()
		{
			eval_address = 0;
			eval_sync_tag = 0;
			eval_sources.clear();

			eval_failed = false;
		}

		bool conditional_render_eval::disable_rendering() const
		{
			return (enabled && eval_failed);
		}

		bool conditional_render_eval::eval_pending() const
		{
			return (enabled && eval_address);
		}

		void conditional_render_eval::enable_conditional_render(::rsx::thread* pthr, u32 address)
		{
			if (hw_cond_active)
			{
				verify(HERE), enabled;
				pthr->end_conditional_rendering();
			}

			reset();

			enabled = true;
			eval_address = address;
		}

		void conditional_render_eval::disable_conditional_render(::rsx::thread* pthr)
		{
			if (hw_cond_active)
			{
				verify(HERE), enabled;
				pthr->end_conditional_rendering();
			}

			reset();
			enabled = false;
		}

		void conditional_render_eval::set_eval_sources(std::vector<occlusion_query_info*>& sources)
		{
			eval_sources = std::move(sources);
			eval_sync_tag = eval_sources.front()->sync_tag;
		}

		void conditional_render_eval::set_eval_result(::rsx::thread* pthr, bool failed)
		{
			if (hw_cond_active)
			{
				verify(HERE), enabled;
				pthr->end_conditional_rendering();
			}

			reset();
			eval_failed = failed;
		}

		void conditional_render_eval::eval_result(::rsx::thread* pthr)
		{
			vm::ptr<CellGcmReportData> result = vm::cast(eval_address);
			const bool failed = (result->value == 0);
			set_eval_result(pthr, failed);
		}
	}
}
