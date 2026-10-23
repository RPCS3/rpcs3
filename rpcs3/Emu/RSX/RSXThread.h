#pragma once

#include <queue>
#include <deque>
#include <map>

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
#include "Core/RSXDisplayEngine.h"
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

	struct desync_fifo_cmd_info
	{
		u32 cmd;
		u64 timestamp;
	};

	// Emulates the CTX microprocessor core and its ucode. It has no internal codename, envydocs just call it CTX and the ISA ctxprog.
	// It's quite an advanced thing for what it is and gets custom ucode at startup to execute. Likely managed by lv1 using the hidden CTXCTL method registers.
	class thread : public cpu_thread, public fw_device_state
	{
		friend class core_renderer;

		u64 timestamp_ctrl = 0;
		u64 timestamp_subvalue = 0;
		u64 m_cycles_counter = 0;

		void cpu_task() override;

	public:
		// FIFO
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

		// Invalidated memory range
		address_range32 m_invalidated_memory_range;

		// Profiler
		rsx::profiling_timer m_profiler;

		// Contexts hashmap.
		// NOTE: The primary context is id 0 and lives as long as the thread. There is always at least one logical context available.
		std::map<u32, std::unique_ptr<context>> m_contexts;
		shared_mutex m_contexts_lock;
		atomic_t<u32> m_context_count = 0;

		// Sheduling. Local-task ticks the bound context has run while another one was waiting
		u32 m_ctx_quantum_ticks = 0;

		// Our currently bound context
		context* m_ctx = nullptr;

		// Host renderer. Created with the thread and destroyed with it.
		std::unique_ptr<render_backend> m_renderer;

		// Guards the renderer and the backend resources of every context against other threads (lv2, fault handler).
		shared_mutex m_renderer_lock;

		// Renderer config. Can indicate that we should replace the current one.
		std::string m_renderer_info{};
		bool m_renderer_switch_requested = false;

		// Host thread running cpu_task, recorded when it starts
		atomic_t<thread_base*> m_host_thread = nullptr;

		// Host window
		display_output m_display_out;

		// Flip, vblank and frame pacing
		display_engine m_display;

	public:
		u32 restore_point = 0;
		u32 dbg_step_pc = 0;
		u32 last_known_code_start = 0;
		u32 last_code_jump = 0;
		u32 last_sema_cmd = 0;
		u32 last_sema_addr = 0;
		atomic_t<u32> external_interrupt_lock{ 0 };
		atomic_t<bool> external_interrupt_ack{ false };
		atomic_t<u32> is_initialized{0};

		// Device-level interrupt, not context-local. It can be raised from any thread.
		atomic_bitmask_t<eng_interrupt_reason> eng_interrupt_mask;

		bool is_fifo_idle(const context* ctx) const;
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

		using flip_request = display_engine::flip_request;

		surface_scaling_config_t resolution_scaling_config{};

		void capture_frame(const std::string& name) const;
		const backend_configuration& get_backend_config() const { return m_renderer->config(); }

		const draw_command_processor* draw_processor() const { return &m_ctx->draw_processor; }
		draw_command_processor* draw_processor() { return &m_ctx->draw_processor; }
		reports::ZCULL_control& zcull() { return *zcull_ctrl; }
		overlays::display_manager* overlay_manager() const { return m_overlay_manager; }

		// Shared with the renderer: the core and the backend chain their timing checkpoints on it within a draw
		rsx::profiling_timer& profiler() { return m_profiler; }

		// Execute a backend local task queue
		void do_local_task(FIFO::state state);

		context* get_context() const { return m_ctx; }

		// Every live context. Callable from any thread.
		template <typename F>
		void for_each_context(F&& func)
		{
			reader_lock lock(m_contexts_lock);

			for (const auto& [id, ctx] : m_contexts)
			{
				func(ctx.get());
			}
		}

		// Called from PPU thread (lv2). Host resources are made on the RSX thread when the context is first bound.
		context* create_context(u32 id);

		// Can be called from any thread. Returns nullptr if there is no such context.
		context* find_context(u32 id);

		// RSX thread only.
		void destroy_context(u32 id);

		// RSX thread only, and must be outside a draw. The renderer and every engine work on the given context from now on.
		void bind_context(u32 id);

		// RSX thread only. The renderer can be replaced at any time.
		render_backend* renderer() const { return m_renderer.get(); }
		void request_renderer_update();

		// Callable from any thread
		void notify_tile_unbound(u32 tile);

		core_renderer& core() { return m_ctx->core; }
		display_output& display() { return m_display_out; }
		display_engine& display_ctrl() { return m_display; }

		// Callable from any thread. Asks the RSX thread to run the backend's part of the local task (render_backend::on_local_task) soon.
		inline void raise_backend_interrupt() { eng_interrupt_mask |= rsx::backend_interrupt; }

	public:
		shared_ptr<named_thread<ppu_thread>> intr_thread;

		// I hate this flag, but until hle is closer to lle, its needed
		bool isHLE{ false };
		bool serialized = false;

		int debug_level = CELL_GCM_DEBUG_LEVEL0;

		atomic_t<bool> requested_vsync{true};
		atomic_t<bool> enable_second_vhandler{false};

		// Sends to the context's lv2 event port
		bool send_event(context* ctx, u64, u64, u64);

		// Events an lv2 event queue could not take yet, on any context
		bool has_unsent_gcm_events();

		std::array<u32, 4> get_color_surface_addresses() const;
		u32 get_zeta_surface_address() const;

		bool invalidate_fragment_program(u32 dst_dma, u32 dst_offset, u32 size);
		void on_framebuffer_options_changed(u32 opt);

	public:
		vm::ptr<void(u32)> flip_handler = vm::null;
		vm::ptr<void(u32)> user_handler = vm::null;
		vm::ptr<void(u32)> vblank_handler = vm::null;
		vm::ptr<void(u32)> queue_handler = vm::null;
		bool capture_current_frame = false;

		// lv2 entry point, forwarded to the display engine
		void post_vblank_event(u64 post_event_time) { m_display.post_vblank_event(post_event_time); }

	public:
		atomic_t<bool> sync_point_request = false;
		atomic_t<bool> pause_on_draw = false;

		std::queue<desync_fifo_cmd_info> recovered_fifo_cmds_history;

		atomic_t<s32> async_tasks_pending{ 0 };

		u64 get_cycles();
		virtual ~thread();

		static constexpr auto thread_name = "rsx::thread"sv;

		// Created as named_thread<rsx::thread> (rsx::create_render_thread)
		thread(utils::serial* ar);

		// Must not be default-constructible, or g_fxo->init()'s bulk pass constructs a bare rsx::thread (and its backend) in this slot
		thread() = delete;

	protected:
		void on_task();
		void on_exit();

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

		void on_frame_end(u32 buffer, bool forced = false);
		void flip(const display_flip_info_t& info) { m_display.flip(info); }
		u64 timestamp();

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

		// id
		std::string_view renderer_info() const { return m_renderer_info; }

	private:
		shared_mutex m_mtx_task;

		void handle_invalidated_memory_range();

		// Render backend resources of every context
		void create_backend_resources();
		void release_backend_resources();
		void release_context_resources(context* ctx);

		// Fault handling requires a walk of the HT, so we must lock it to find the culprit.
		void lock_exclusive(shared_mutex& mutex);

		// Context table walk with renderer lock guarding
		template <typename F>
		void for_each_context_exclusive(F&& func)
		{
			lock_exclusive(m_renderer_lock);
			for_each_context(std::forward<F>(func));
			m_renderer_lock.unlock();
		}

		// Serialize a context
		void save_context(utils::serial& ar, context& ctx);

		// Deferred handler to remove contexts that have been freed on the lv2 side.
		void free_stale_contexts();

		// Host writeback handling
		void flush_guest_data(context* ctx);
		void flush_guest_data();
		atomic_t<bool> m_surface_write_back_requested = false;

		// FIFO channel switch. The puller runs only the current context's channel.
		void save_fifo_channel(FIFO::channel_state& channel) const;
		void load_fifo_channel(const FIFO::channel_state& channel);

		// Contexts switch outside a draw and between FIFO packets
		bool can_switch_context() const;

		// Round-robin over contexts with pending FIFO commands, when the bound one goes idle or uses up its quantum
		void schedule_contexts();

		// Scheduling quantum. Unit is in local-task ticks
		static constexpr u32 context_quantum_ticks = 16;

		// Destroy and rebuild the backend renderer.
		void reset_renderer(bool device_lost);

	public:

		draw_command_processor& GRAPH_frontend() { return m_ctx->draw_processor; }

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

		// lv2 set up a context's FIFO channel
		void init_context(context* ctx, u32 ctrlAddress);

		// lv2 allocated the first context with a FIFO channel: the device starts
		void start_device();

		// Called from lv2 to destroy a context
		void free_context(u32 id);

		// Write back all host mem to guest mem, regardless of WCB/WDB settings.
		void request_guest_write_back()
		{
			m_surface_write_back_requested = true;
			raise_backend_interrupt();
		}

		// Emu App/Game flip, only immediately flips when called from rsxthread
		bool request_emu_flip(u32 buffer) { return m_display.request_emu_flip(buffer); }

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

	// The host renderer of the RSX thread, or nullptr if there is no RSX thread
	inline render_backend* get_current_render_backend()
	{
		const auto rsxthr = get_current_renderer();
		return rsxthr ? rsxthr->renderer() : nullptr;
	}
}
