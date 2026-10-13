#pragma once

#include <queue>
#include <deque>

#include "RSXFIFO.h"
#include "RSXOffload.h"
#include "RSXZCULL.h"
#include "Common/bitfield.hpp"
#include "Common/profiling_timer.hpp"
#include "Utils/rsx_utils.h"

#include "Utilities/Thread.h"
#include "Utilities/geometry.h"
#include "Capture/rsx_trace.h"
#include "Capture/rsx_replay.h"

#include "Emu/Cell/lv2/sys_rsx.h"
#include "Emu/IdManager.h"

#include "Core/RSXDisplay.h"
#include "Core/RSXDisplayOutput.h"
#include "Core/RSXDrawCommands.h"
#include "Core/RSXDriverState.h"
#include "Core/RSXFrameBuffer.h"
#include "Core/RSXContext.h"
#include "Core/RSXCoreRenderer.h"
#include "Core/RSXRenderBackend.h"
#include "Core/RSXVertexTypes.h"


extern atomic_t<bool> g_user_asked_for_frame_capture;
extern atomic_t<bool> g_disable_frame_limit;
extern rsx::frame_trace_data frame_debug;
extern rsx::frame_capture_data frame_capture;

namespace rsx
{
	class RSXDMAWriter;

	struct context;

	namespace overlays
	{
		class display_manager;
	}

	enum result_flags: u8
	{
		result_none = 0,
		result_error = 1,
		result_zcull_intr = 2
	};

	u32 get_vertex_type_size_on_host(vertex_base_type type, u32 size);

	u32 get_address(u32 offset, u32 location, u32 size_to_check = 0, std::source_location src_loc = std::source_location::current());

	struct desync_fifo_cmd_info
	{
		u32 cmd;
		u64 timestamp;
	};

	// TODO: This class is a mess, this needs to be broken into smaller chunks, like I did for RSXFIFO and RSXZCULL (kd)
	class thread : public cpu_thread, public fw_context
	{
		friend class core_renderer;

		u64 timestamp_ctrl = 0;
		u64 timestamp_subvalue = 0;
		u64 m_cycles_counter = 0;

		display_flip_info_t m_queued_flip{};

		void cpu_task() override;
	protected:

		// FIFO
	public:
		std::unique_ptr<FIFO::FIFO_control> fifo_ctrl;
		atomic_t<bool> rsx_thread_running{ false };
		std::vector<std::pair<u32, u32>> dump_callstack_list() const override;
		void dump_misc(std::string& ret, std::any& custom_data) const override;

	protected:
		FIFO::flattening_helper m_flattener;
		u32 fifo_ret_addr = RSX_CALL_STACK_EMPTY;
		u32 saved_fifo_ret = RSX_CALL_STACK_EMPTY;
		u32 restore_fifo_cmd = 0;
		u32 restore_fifo_count = 0;
		u32 restore_fifo_position  = 0;

		// Occlusion query
		std::unique_ptr<reports::ZCULL_control> zcull_ctrl;

		// Overlays
		rsx::overlays::display_manager* m_overlay_manager = nullptr;

		atomic_t<u64> m_display_rate_fetch_count = 0;
		atomic_t<f64> m_cached_display_rate = 60.;
		f64 get_cached_display_refresh_rate();
		virtual f64 get_display_refresh_rate() const = 0;

		// Invalidated memory range
		address_range32 m_invalidated_memory_range;

		// Profiler
		rsx::profiling_timer m_profiler;

		// Savestates related
		u32 m_pause_after_x_flips = 0;

		// Context
		std::unique_ptr<context> m_primary_ctx;
		context* m_ctx = nullptr;

		// Host renderer bridge
		render_backend* m_renderer = nullptr;

		// Host window
		display_output m_display_out;

		// Host DMA
		std::unique_ptr<RSXDMAWriter> m_host_dma_ctrl;

		// Draw call management
		draw_command_processor m_draw_processor;

		// Backend-agnostic renderer core
		core_renderer m_core;

	public:
		atomic_t<u64> new_get_put = u64{umax};
		u32 restore_point = 0;
		u32 dbg_step_pc = 0;
		u32 last_known_code_start = 0;
		u32 last_code_jump = 0;
		u32 last_sema_cmd = 0;
		u32 last_sema_addr = 0;
		atomic_t<u32> external_interrupt_lock{ 0 };
		atomic_t<bool> external_interrupt_ack{ false };
		atomic_t<u32> is_initialized{0};

		bool is_fifo_idle() const;
		void flush_fifo();

		// Returns [count of found commands, PC of their start]
		std::pair<u32, u32> try_get_pc_of_x_cmds_backwards(s32 count, u32 get) const;

		void recover_fifo(std::source_location src_loc = std::source_location::current());

		static void fifo_wake_delay(u64 div = 1);
		u32 get_fifo_cmd() const;

		void dump_regs(std::string&, std::any& custom_data) const override;
		void cpu_wait(bs_t<cpu_flag> old) override;

		static constexpr u32 id_base = 0x5555'5555; // See get_current_cpu_thread()

		// Performance approximation counters
		struct
		{
			atomic_t<u64> idle_time{ 0 };  // Time spent idling in microseconds
			u64 last_update_timestamp = 0; // Timestamp of last load update
			u64 FIFO_idle_timestamp = 0;   // Timestamp of when FIFO queue becomes idle
			FIFO::state state = FIFO::state::running;
			u32 approximate_load = 0;
			u32 sampled_frames = 0;
		}
		performance_counters;

		enum class flip_request : u32
		{
			emu_requested = 1,
			native_ui = 2,

			any = emu_requested | native_ui
		};

		atomic_bitmask_t<flip_request> async_flip_requested{};
		u8 async_flip_buffer{ 0 };

		surface_scaling_config_t resolution_scaling_config{};

		void capture_frame(const std::string& name) const;
		const backend_configuration& get_backend_config() const { return m_renderer->config(); }

		const draw_command_processor* draw_processor() const { return &m_draw_processor; }

		context* get_context() const { return m_ctx; }

		render_backend* renderer() const { return m_renderer; }
		core_renderer& core() { return m_core; }
		display_output& display() { return m_display_out; }

		// Callable from any thread. Asks the RSX thread to run the backend's part of the local task (render_backend::on_local_task) soon.
		inline void raise_backend_interrupt() { m_ctx->eng_interrupt_mask |= rsx::backend_interrupt; }

	public:
		shared_ptr<named_thread<ppu_thread>> intr_thread;

		// I hate this flag, but until hle is closer to lle, its needed
		bool isHLE{ false };
		bool serialized = false;

		u32 flip_status = CELL_GCM_DISPLAY_FLIP_STATUS_DONE;
		int debug_level = CELL_GCM_DEBUG_LEVEL0;

		atomic_t<bool> requested_vsync{true};
		atomic_t<bool> enable_second_vhandler{false};

		bool send_event(u64, u64, u64);

		u64 ROP_sync_timestamp = 0;

		std::array<u32, 4> get_color_surface_addresses() const;
		u32 get_zeta_surface_address() const;

	protected:
		// Forwarded to the GRAPH engine
		bool requires_depth_range_emulation() const;

	public:
		bool invalidate_fragment_program(u32 dst_dma, u32 dst_offset, u32 size);
		void on_framebuffer_options_changed(u32 opt);

	public:
		u64 target_rsx_flip_time = 0;
		u64 int_flip_index = 0;
		u64 last_guest_flip_timestamp = 0;
		u64 last_host_flip_timestamp = 0;

		vm::ptr<void(u32)> flip_handler = vm::null;
		vm::ptr<void(u32)> user_handler = vm::null;
		vm::ptr<void(u32)> vblank_handler = vm::null;
		vm::ptr<void(u32)> queue_handler = vm::null;
		atomic_t<u64> vblank_count{0};
		bool capture_current_frame = false;

		u64 vblank_at_flip = umax;
		u64 flip_notification_count = 0;
		void post_vblank_event(u64 post_event_time);

	public:
		atomic_t<bool> sync_point_request = false;
		atomic_t<bool> pause_on_draw = false;

		std::queue<desync_fifo_cmd_info> recovered_fifo_cmds_history;
		std::deque<frame_time_t> frame_times;
		u32 prevent_preempt_increase_tickets = 0;
		u64 preempt_fail_old_preempt_count = 0;

		atomic_t<s32> async_tasks_pending{ 0 };

		virtual u64 get_cycles() = 0;
		virtual ~thread();

		static constexpr auto thread_name = "rsx::thread"sv;

	protected:
		thread(utils::serial* ar);

		thread() : thread(static_cast<utils::serial*>(nullptr)) {}

		virtual void on_task();
		void on_exit();

		/**
		 * Execute a backend local task queue
		 */
		void do_local_task(FIFO::state state);

		void run_FIFO();

	public:
		thread(const thread&) = delete;
		thread& operator=(const thread&) = delete;
		void save(utils::serial& ar);

		// NV4097_SET_BEGIN_END, forwarded to the renderer core
		void begin();
		void end();
		void discard_draw();

		// NV4097_CLEAR_SURFACE, forwarded to the renderer core
		void clear_surface(u32 arg);

		virtual void on_frame_end(u32 buffer, bool forced = false);
		void flip(const display_flip_info_t& info);
		virtual u64 timestamp();

		// control
		void renderctl(u32 request_code, void* args);

		// zcull
		void notify_zcull_info_changed();
		void clear_zcull_stats(u32 type);
		void check_zcull_status(bool framebuffer_swap);
		void get_zcull_stats(u32 type, vm::addr_t sink);
		u32  copy_zcull_stats(u32 memory_range_start, u32 memory_range, u32 destination);

		void enable_conditional_rendering(vm::addr_t ref);
		void disable_conditional_rendering();
		void begin_conditional_rendering(const std::vector<reports::occlusion_query_info*>& sources);
		void end_conditional_rendering();

		// sync
		void sync();
		flags32_t read_barrier(u32 memory_address, u32 memory_range, bool unconditional);
		void sync_hint(FIFO::interrupt_hint hint, reports::sync_hint_payload_t payload);

	protected:

		/**
		 * Computes VRAM requirements needed to upload raw vertex streams
		 * result.first contains persistent memory requirements
		 * result.second contains volatile memory requirements
		 */
		std::pair<u32, u32> calculate_memory_requirements(const vertex_input_layout& layout, u32 first_vertex, u32 vertex_count);

		void evaluate_cpu_usage_reduction_limits();

	private:
		shared_mutex m_mtx_task;

		void handle_emu_flip(u32 buffer);
		void handle_invalidated_memory_range();

	public:

		draw_command_processor& GRAPH_frontend() { return m_draw_processor; }

		/**
		 * Notify that a section of memory has been mapped
		 * If there is a notify_memory_unmapped request on this range yet to be handled,
		 * handles it immediately.
		 */
		void on_notify_memory_mapped(u32 address_base, u32 size);

		/**
		 * Notify that a section of memory is to be unmapped
		 * Any data held in the defined range is discarded
		 * Sets optional unmap event data
		 */
		void on_notify_pre_memory_unmapped(u32 address_base, u32 size, std::vector<std::pair<u64, u64>>& event_data);

		/**
		 * Notify that a section of memory has been unmapped
		 * Any data held in the defined range is discarded
		 */
		void on_notify_post_memory_unmapped(u64 event_data1, u64 event_data2);

	public:
		void reset();
		void init(u32 ctrlAddress);

		// Emu App/Game flip, only immediately flips when called from rsxthread
		bool request_emu_flip(u32 buffer);

		void pause();
		void unpause();
		void wait_pause();

		// Get RSX approximate load in %
		u32 get_load();

		// Get stats object
		frame_statistics_t& get_stats() { return m_ctx->stats; }

		// Returns true if the current thread is the active RSX thread
		inline bool is_current_thread() const
		{
			return !!cpu_thread::get_current<rsx::thread>();
		}
	};

	inline thread* get_current_renderer()
	{
		return g_fxo->try_get<rsx::thread>();
	}
}
