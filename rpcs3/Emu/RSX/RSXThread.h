#pragma once

#include <queue>
#include <deque>

#include "RSXFIFO.h"
#include "RSXOffload.h"
#include "RSXZCULL.h"
#include "rsx_utils.h"
#include "Common/bitfield.hpp"
#include "Common/profiling_timer.hpp"
#include "Common/texture_cache_types.h"
#include "Common/TextureUtils.h"
#include "Program/RSXVertexProgram.h"
#include "Program/RSXFragmentProgram.h"

#include "Utilities/Thread.h"
#include "Utilities/geometry.h"
#include "Capture/rsx_trace.h"
#include "Capture/rsx_replay.h"

#include "Emu/Cell/lv2/sys_rsx.h"
#include "Emu/IdManager.h"

#include "Core/RSXDisplay.h"
#include "Core/RSXDrawCommands.h"
#include "Core/RSXDriverState.h"
#include "Core/RSXFrameBuffer.h"
#include "Core/RSXContext.h"
#include "Core/RSXVertexTypes.h"

#include "NV47/FW/GRAPH_backend.h"

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

	enum framebuffer_creation_context : u8
	{
		context_draw = 0,
		context_clear_color = 1,
		context_clear_depth = 2,
		context_clear_all = context_clear_color | context_clear_depth
	};

	enum eng_interrupt_reason : u32
	{
		backend_interrupt       = 0x0001,        // Backend-related interrupt
		memory_config_interrupt = 0x0002,        // Memory configuration changed
		display_interrupt       = 0x0004,        // Display handling
		pipe_flush_interrupt    = 0x0008,        // Flush pipelines
		dma_control_interrupt   = 0x0010,        // DMA interrupt

		all_interrupt_bits = memory_config_interrupt | backend_interrupt | display_interrupt | pipe_flush_interrupt
	};

	enum result_flags: u8
	{
		result_none = 0,
		result_error = 1,
		result_zcull_intr = 2
	};

	u32 get_vertex_type_size_on_host(vertex_base_type type, u32 size);

	u32 get_address(u32 offset, u32 location, u32 size_to_check = 0, std::source_location src_loc = std::source_location::current());

	struct backend_configuration
	{
		bool supports_multidraw;               // Draw call batching
		bool supports_hw_a2c;                  // Alpha to coverage
		bool supports_hw_a2c_1spp;             // Alpha to coverage at 1 sample per pixel
		bool supports_hw_renormalization;      // Should be true on NV hardware which matches PS3 texture renormalization behaviour
		bool supports_hw_msaa;                 // MSAA support
		bool supports_hw_a2one;                // Alpha to one
		bool supports_hw_conditional_render;   // Conditional render
		bool supports_passthrough_dma;         // DMA passthrough
		bool supports_asynchronous_compute;    // Async compute
		bool supports_host_gpu_labels;         // Advanced host synchronization
		bool supports_normalized_barycentrics; // Basically all GPUs except NVIDIA have properly normalized barycentrics
	};

	struct desync_fifo_cmd_info
	{
		u32 cmd;
		u64 timestamp;
	};

	// TODO: This class is a mess, this needs to be broken into smaller chunks, like I did for RSXFIFO and RSXZCULL (kd)
	class thread : public cpu_thread, public GCM_context, public GRAPH_backend
	{
		u64 timestamp_ctrl = 0;
		u64 timestamp_subvalue = 0;
		u64 m_cycles_counter = 0;

		display_flip_info_t m_queued_flip{};

		void cpu_task() override;
	protected:

		s32 m_skip_frame_ctr = 0;
		bool skip_current_frame = false;

		primitive_class m_current_draw_mode = primitive_class::polygon;

		backend_configuration backend_config{};

		// FIFO
	public:
		std::unique_ptr<FIFO::FIFO_control> fifo_ctrl;
		atomic_t<bool> rsx_thread_running{ false };
		std::vector<std::pair<u32, u32>> dump_callstack_list() const override;
		std::string dump_misc() const override;

	protected:
		FIFO::flattening_helper m_flattener;
		u32 fifo_ret_addr = RSX_CALL_STACK_EMPTY;
		u32 saved_fifo_ret = RSX_CALL_STACK_EMPTY;
		u32 restore_fifo_cmd = 0;
		u32 restore_fifo_count = 0;

		// Occlusion query
		bool zcull_surface_active = false;
		std::unique_ptr<reports::ZCULL_control> zcull_ctrl;

		// Framebuffer setup
		rsx::gcm_framebuffer_info m_surface_info[rsx::limits::color_buffers_count];
		rsx::gcm_framebuffer_info m_depth_surface_info;
		framebuffer_layout m_framebuffer_layout{};

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
		frame_statistics_t m_frame_stats{};

		// Savestates related
		u32 m_pause_after_x_flips = 0;

		// Context
		context* m_ctx = nullptr;

		// Host DMA
		std::unique_ptr<RSXDMAWriter> m_host_dma_ctrl;

		// Draw call management
		draw_command_processor m_draw_processor;

	public:
		atomic_t<u64> new_get_put = u64{umax};
		u32 restore_point = 0;
		u32 dbg_step_pc = 0;
		u32 last_known_code_start = 0;
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

		void capture_frame(const std::string& name);
		const backend_configuration& get_backend_config() const { return backend_config; }

		const draw_command_processor* draw_processor() const { return &m_draw_processor; }

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

		std::array<bool, 16> m_textures_dirty;
		std::array<bool, 4> m_vertex_textures_dirty;
		rsx::framebuffer_creation_context m_current_framebuffer_context = rsx::framebuffer_creation_context::context_draw;

		rsx::atomic_bitmask_t<rsx::eng_interrupt_reason> m_eng_interrupt_mask;
		rsx::bitmask_t<rsx::pipeline_state> m_graphics_state;

		u64 ROP_sync_timestamp = 0;

		program_hash_util::fragment_program_utils::fragment_program_metadata current_fp_metadata = {};
		program_hash_util::vertex_program_utils::vertex_program_metadata current_vp_metadata = {};

		std::array<std::unique_ptr<rsx::sampled_image_descriptor_base>, rsx::limits::fragment_textures_count> fs_sampler_state = {};
		std::array<std::unique_ptr<rsx::sampled_image_descriptor_base>, rsx::limits::vertex_textures_count> vs_sampler_state = {};

		std::array<u32, 4> get_color_surface_addresses() const;
		u32 get_zeta_surface_address() const;

	protected:
		void get_framebuffer_layout(rsx::framebuffer_creation_context context, framebuffer_layout &layout);
		bool get_scissor(areau& region, bool clip_viewport);

		RSXVertexProgram current_vertex_program = {};
		RSXFragmentProgram current_fragment_program = {};

		vertex_program_texture_state current_vp_texture_state = {};
		fragment_program_texture_state current_fp_texture_state = {};

		program_cache_hint_t m_program_cache_hint;

		// Runs shader prefetch and resolves pipeline status flags
		void analyse_current_rsx_pipeline();

		// Prefetch and analyze the currently active fragment program ucode
		void prefetch_fragment_program();

		// Prefetch and analyze the currently active vertex program ucode
		void prefetch_vertex_program();

		void get_current_vertex_program(const std::array<std::unique_ptr<rsx::sampled_image_descriptor_base>, rsx::limits::vertex_textures_count>& sampler_descriptors);

		/**
		 * Gets current fragment program and associated fragment state
		 */
		void get_current_fragment_program(const std::array<std::unique_ptr<rsx::sampled_image_descriptor_base>, rsx::limits::fragment_textures_count>& sampler_descriptors);

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
		bool in_begin_end = false;

		std::queue<desync_fifo_cmd_info> recovered_fifo_cmds_history;
		std::deque<frame_time_t> frame_times;
		u32 prevent_preempt_increase_tickets = 0;
		u64 preempt_fail_old_preempt_count = 0;

		atomic_t<s32> async_tasks_pending{ 0 };

		reports::conditional_render_eval cond_render_ctrl;

		virtual u64 get_cycles() = 0;
		virtual ~thread();

		static constexpr auto thread_name = "rsx::thread"sv;

	protected:
		thread(utils::serial* ar);

		thread() : thread(static_cast<utils::serial*>(nullptr)) {}

		virtual void on_task();
		virtual void on_exit();

		/**
		 * Execute a backend local task queue
		 */
		virtual void do_local_task(FIFO::state state);

		virtual void emit_geometry(u32) {}

		void run_FIFO();

	public:
		thread(const thread&) = delete;
		thread& operator=(const thread&) = delete;
		void save(utils::serial& ar);

		virtual void clear_surface(u32 /*arg*/) {}
		virtual void begin();
		virtual void end();
		virtual void execute_nop_draw();

		virtual void on_init_thread() = 0;
		virtual void on_frame_end(u32 buffer, bool forced = false);
		virtual void flip(const display_flip_info_t& info) = 0;
		virtual u64 timestamp();
		virtual bool on_access_violation(u32 /*address*/, bool /*is_writing*/) { return false; }
		virtual void on_invalidate_memory_range(const address_range32 & /*range*/, rsx::invalidation_cause) {}
		virtual void notify_tile_unbound(u32 /*tile*/) {}

		// control
		virtual void renderctl(u32 request_code, void* args);

		// zcull
		void notify_zcull_info_changed();
		void clear_zcull_stats(u32 type);
		void check_zcull_status(bool framebuffer_swap);
		void get_zcull_stats(u32 type, vm::addr_t sink);
		u32  copy_zcull_stats(u32 memory_range_start, u32 memory_range, u32 destination);

		void enable_conditional_rendering(vm::addr_t ref);
		void disable_conditional_rendering();
		virtual void begin_conditional_rendering(const std::vector<reports::occlusion_query_info*>& sources);
		virtual void end_conditional_rendering();

		// sync
		void sync();
		flags32_t read_barrier(u32 memory_address, u32 memory_range, bool unconditional);
		virtual void sync_hint(FIFO::interrupt_hint hint, reports::sync_hint_payload_t payload);
		virtual bool release_GCM_label(u32 /*address*/, u32 /*value*/) { return false; }

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

		/**
		 * Notify to check internal state during semaphore wait
		 */
		virtual void on_semaphore_acquire_wait() {}

		/**
		 * Load an image from memory with optional scaling and rotation.
		 * Returns false to tell the HW decoder to perform the operation on the CPU as a fallback when the operation cannot be safely accelerated.
		 */
		virtual bool scaled_image_from_memory(const blit_src_info& /*src_info*/, const blit_dst_info& /*dst_info*/, bool /*interpolate*/) { return false; }


		// Program public "get" handlers
		virtual std::pair<std::string, std::string> get_programs() const { return std::make_pair("", ""); }

		bool is_current_vertex_program_instanced() const { return !!(current_vertex_program.ctrl & RSX_SHADER_CONTROL_INSTANCED_CONSTANTS); }

		virtual bool is_current_program_interpreted() const { return false; }

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
		frame_statistics_t& get_stats() { return m_frame_stats; }

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

	inline const backend_configuration& get_renderer_backend_config()
	{
		return g_fxo->get<rsx::thread>().get_backend_config();
	}
}
