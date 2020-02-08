#pragma once

#include <deque>
#include <variant>
#include <stack>

#include "GCM.h"
#include "rsx_cache.h"
#include "RSXFIFO.h"
#include "RSXTexture.h"
#include "RSXOffload.h"
#include "RSXVertexProgram.h"
#include "RSXFragmentProgram.h"
#include "rsx_methods.h"
#include "rsx_utils.h"
#include "Overlays/overlays.h"
#include "Common/texture_cache_utils.h"

#include "Utilities/Thread.h"
#include "Utilities/geometry.h"
#include "Capture/rsx_trace.h"
#include "Capture/rsx_replay.h"

#include "Emu/Cell/lv2/sys_rsx.h"
#include "Emu/IdManager.h"

extern u64 get_guest_system_time();
extern u64 get_system_time();

struct RSXIOTable
{
	atomic_t<u16> ea[4096];
	atomic_t<u16> io[3072];

	// try to get the real address given a mapped address
	// return non zero on success
	inline u32 RealAddr(u32 offs)
	{
		u32 result = this->ea[offs >> 20].load();

		if (static_cast<s16>(result) < 0)
		{
			return 0;
		}

		result <<= 20; result |= (offs & 0xFFFFF);

		ASSUME(result != 0);

		return result;
	}
};

extern bool user_asked_for_frame_capture;
extern bool capture_current_frame;
extern rsx::frame_trace_data frame_debug;
extern rsx::frame_capture_data frame_capture;
extern RSXIOTable RSXIOMem;

namespace rsx
{
	enum framebuffer_creation_context : u8
	{
		context_draw = 0,
		context_clear_color = 1,
		context_clear_depth = 2,
		context_clear_all = context_clear_color | context_clear_depth
	};

	enum pipeline_state : u32
	{
		fragment_program_dirty = 0x1,        // Fragment program changed
		vertex_program_dirty = 0x2,          // Vertex program changed
		fragment_state_dirty = 0x4,          // Fragment state changed (alpha test, etc)
		vertex_state_dirty = 0x8,            // Vertex state changed (scale_offset, clip planes, etc)
		transform_constants_dirty = 0x10,    // Transform constants changed
		fragment_constants_dirty = 0x20,     // Fragment constants changed
		framebuffer_reads_dirty = 0x40,      // Framebuffer contents changed
		fragment_texture_state_dirty = 0x80, // Fragment texture parameters changed
		vertex_texture_state_dirty = 0x100,  // Fragment texture parameters changed
		scissor_config_state_dirty = 0x200,  // Scissor region changed

		scissor_setup_invalid = 0x400,       // Scissor configuration is broken
		scissor_setup_clipped = 0x800,       // Scissor region is cropped by viewport constraint

		invalidate_pipeline_bits = fragment_program_dirty | vertex_program_dirty,
		memory_barrier_bits = framebuffer_reads_dirty,
		all_dirty = ~0u
	};

	enum FIFO_state : u8
	{
		running = 0,
		empty = 1,    // PUT == GET
		spinning = 2, // Puller continuously jumps to self addr (synchronization technique)
		nop = 3,      // Puller is processing a NOP command
		lock_wait = 4 // Puller is processing a lock acquire
	};

	enum FIFO_hint : u8
	{
		hint_conditional_render_eval = 1,
		hint_zcull_sync = 2
	};

	enum result_flags: u8
	{
		result_none = 0,
		result_error = 1,
		result_zcull_intr = 2
	};

	u32 get_vertex_type_size_on_host(vertex_base_type type, u32 size);

	// TODO: Replace with std::source_location in c++20
	u32 get_address(u32 offset, u32 location, const char* from);

	struct tiled_region
	{
		u32 address;
		u32 base;
		GcmTileInfo *tile;
		u8 *ptr;

		void write(const void *src, u32 width, u32 height, u32 pitch);
		void read(void *dst, u32 width, u32 height, u32 pitch);
	};

	struct vertex_array_buffer
	{
		rsx::vertex_base_type type;
		u8 attribute_size;
		u8 stride;
		gsl::span<const std::byte> data;
		u8 index;
		bool is_be;
	};

	struct vertex_array_register
	{
		rsx::vertex_base_type type;
		u8 attribute_size;
		std::array<u32, 4> data;
		u8 index;
	};

	struct empty_vertex_array
	{
		u8 index;
	};

	struct draw_array_command
	{
		u32 __dummy;
	};

	struct draw_indexed_array_command
	{
		gsl::span<const std::byte> raw_index_buffer;
	};

	struct draw_inlined_array
	{
		u32 __dummy;
		u32 __dummy2;
	};

	struct interleaved_attribute_t
	{
		u8 index;
		bool modulo;
		u16 frequency;
	};

	struct interleaved_range_info
	{
		bool interleaved = false;
		bool single_vertex = false;
		u32  base_offset = 0;
		u32  real_offset_address = 0;
		u8   memory_location = 0;
		u8   attribute_stride = 0;

		rsx::simple_array<interleaved_attribute_t> locations;

		// Check if we need to upload a full unoptimized range, i.e [0-max_index]
		std::pair<u32, u32> calculate_required_range(u32 first, u32 count) const
		{
			if (single_vertex)
			{
				return { 0, 1 };
			}

			const u32 max_index = (first + count) - 1;
			u32 _max_index = 0;
			u32 _min_index = first;

			for (const auto &attrib : locations)
			{
				if (attrib.frequency <= 1) [[likely]]
				{
					_max_index = max_index;
				}
				else
				{
					if (attrib.modulo)
					{
						if (max_index >= attrib.frequency)
						{
							// Actually uses the modulo operator, cannot safely optimize
							_min_index = 0;
							_max_index = std::max<u32>(_max_index, attrib.frequency - 1);
						}
						else
						{
							// Same as having no modulo
							_max_index = max_index;
						}
					}
					else
					{
						// Division operator
						_min_index = std::min(_min_index, first / attrib.frequency);
						_max_index = std::max<u32>(_max_index, max_index / attrib.frequency);
					}
				}
			}

			verify(HERE), _max_index >= _min_index;
			return { _min_index, (_max_index - _min_index) + 1 };
		}
	};

	enum attribute_buffer_placement : u8
	{
		none = 0,
		persistent = 1,
		transient = 2
	};

	struct vertex_input_layout
	{
		std::vector<interleaved_range_info> interleaved_blocks;  // Interleaved blocks to be uploaded as-is
		std::vector<std::pair<u8, u32>> volatile_blocks;         // Volatile data blocks (immediate draw vertex data for example)
		rsx::simple_array<u8> referenced_registers;              // Volatile register data

		std::array<attribute_buffer_placement, 16> attribute_placement;

		vertex_input_layout()
		{
			attribute_placement.fill(attribute_buffer_placement::none);
		}

		void clear()
		{
			interleaved_blocks.clear();
			volatile_blocks.clear();
			referenced_registers.clear();
		}

		bool validate() const
		{
			// Criteria: At least one array stream has to be defined to feed vertex positions
			// This stream cannot be a const register as the vertices cannot create a zero-area primitive

			if (!interleaved_blocks.empty() && interleaved_blocks.front().attribute_stride != 0)
				return true;

			if (!volatile_blocks.empty())
				return true;

			for (u8 index = 0; index < limits::vertex_count; ++index)
			{
				switch (attribute_placement[index])
				{
				case attribute_buffer_placement::transient:
				{
					// Ignore register reference
					if (std::find(referenced_registers.begin(), referenced_registers.end(), index) != referenced_registers.end())
						continue;

					// The source is inline array or immediate draw push buffer
					return true;
				}
				case attribute_buffer_placement::persistent:
				{
					return true;
				}
				case attribute_buffer_placement::none:
				{
					continue;
				}
				default:
				{
					fmt::throw_exception("Unreachable" HERE);
				}
				}
			}

			return false;
		}

		u32 calculate_interleaved_memory_requirements(u32 first_vertex, u32 vertex_count) const
		{
			u32 mem = 0;
			for (auto &block : interleaved_blocks)
			{
				const auto range = block.calculate_required_range(first_vertex, vertex_count);
				mem += range.second * block.attribute_stride;
			}

			return mem;
		}
	};

	struct framebuffer_layout
	{
		u16 width;
		u16 height;
		std::array<u32, 4> color_addresses;
		std::array<u32, 4> color_pitch;
		std::array<u32, 4> actual_color_pitch;
		std::array<bool, 4> color_write_enabled;
		u32 zeta_address;
		u32 zeta_pitch;
		u32 actual_zeta_pitch;
		bool zeta_write_enabled;
		rsx::surface_target target;
		rsx::surface_color_format color_format;
		rsx::surface_depth_format depth_format;
		rsx::surface_antialiasing aa_mode;
		u32 aa_factors[2];
		bool depth_float;
		bool ignore_change;
	};

	namespace reports
	{
		struct occlusion_query_info
		{
			u32 driver_handle;
			u32 result;
			u32 num_draws;
			u64 sync_tag;
			u64 timestamp;
			bool pending;
			bool active;
			bool owned;
		};

		struct queued_report_write
		{
			u32 type = CELL_GCM_ZPASS_PIXEL_CNT;
			u32 counter_tag;
			occlusion_query_info* query;
			queued_report_write* forwarder;

			vm::addr_t sink;                      // Memory location of the report
			std::vector<vm::addr_t> sink_alias;   // Aliased memory addresses
		};

		struct query_search_result
		{
			bool found;
			u32  raw_zpass_result;
			std::vector<occlusion_query_info*> queries;
		};

		enum sync_control
		{
			sync_none = 0,
			sync_defer_copy = 1, // If set, return a zcull intr code instead of forcefully reading zcull data
			sync_no_notify = 2   // If set, backend hint notifications will not be made
		};

		struct ZCULL_control
		{
			// Delay before a report update operation is forced to retire
			const u32 max_zcull_delay_us = 300;
			const u32 min_zcull_tick_us = 100;

			// Number of occlusion query slots available. Real hardware actually has far fewer units before choking
			const u32 occlusion_query_count = 1024;
			const u32 max_safe_queue_depth = 892;

			bool active = false;
			bool enabled = false;

			std::array<occlusion_query_info, 1024> m_occlusion_query_data = {};
			std::stack<occlusion_query_info*> m_free_occlusion_pool;

			occlusion_query_info* m_current_task = nullptr;
			u32 m_statistics_tag_id = 0;

			// Scheduling clock. Granunlarity is min_zcull_tick value.
			u64 m_tsc = 0;
			u64 m_next_tsc = 0;

			// Incremental tag used for tracking sync events. Hardware clock resolution is too low for the job.
			u64 m_sync_tag = 0;
			u64 m_timer = 0;

			std::vector<queued_report_write> m_pending_writes;
			std::unordered_map<u32, u32> m_statistics_map;

			ZCULL_control();
			~ZCULL_control();

			void set_enabled(class ::rsx::thread* ptimer, bool state, bool flush_queue = false);
			void set_active(class ::rsx::thread* ptimer, bool state, bool flush_queue = false);

			void write(vm::addr_t sink, u64 timestamp, u32 type, u32 value);
			void write(queued_report_write* writer, u64 timestamp, u32 value);

			// Read current zcull statistics into the address provided
			void read_report(class ::rsx::thread* ptimer, vm::addr_t sink, u32 type);

			// Sets up a new query slot and sets it to the current task
			void allocate_new_query(class ::rsx::thread* ptimer);

			// Free a query slot in use
			void free_query(occlusion_query_info* query);

			// Clears current stat block and increments stat_tag_id
			void clear(class ::rsx::thread* ptimer);

			// Forcefully flushes all
			void sync(class ::rsx::thread* ptimer);

			// Conditionally sync any pending writes if range overlaps
			flags32_t read_barrier(class ::rsx::thread* ptimer, u32 memory_address, u32 memory_range, flags32_t flags);

			// Call once every 'tick' to update, optional address provided to partially sync until address is processed
			void update(class ::rsx::thread* ptimer, u32 sync_address = 0, bool hint = false);

			// Draw call notification
			void on_draw();

			// Sync hint notification
			void on_sync_hint(void* args);

			// Check for pending writes
			bool has_pending() const { return !m_pending_writes.empty(); }

			// Search for query synchronized at address
			query_search_result find_query(vm::addr_t sink_address, bool all);

			// Copies queries in range rebased from source range to destination range
			u32 copy_reports_to(u32 start, u32 range, u32 dest);

			// Backend methods (optional, will return everything as always visible by default)
			virtual void begin_occlusion_query(occlusion_query_info* /*query*/) {}
			virtual void end_occlusion_query(occlusion_query_info* /*query*/) {}
			virtual bool check_occlusion_query_status(occlusion_query_info* /*query*/) { return true; }
			virtual void get_occlusion_query_result(occlusion_query_info* query) { query->result = UINT32_MAX; }
			virtual void discard_occlusion_query(occlusion_query_info* /*query*/) {}
		};

		// Helper class for conditional rendering
		struct conditional_render_eval
		{
			bool enabled = false;
			bool eval_failed = false;
			bool hw_cond_active = false;
			bool reserved = false;

			std::vector<occlusion_query_info*> eval_sources;
			u32 eval_sync_tag = 0;
			u32 eval_address = 0;

			// Resets common data
			void reset();

			// Returns true if rendering is disabled as per conditional render test
			bool disable_rendering() const;

			// Returns true if a conditional render is active but not yet evaluated
			bool eval_pending() const;

			// Enable conditional rendering
			void enable_conditional_render(thread* pthr, u32 address);

			// Disable conditional rendering
			void disable_conditional_render(thread* pthr);

			// Sets data sources for predicate evaluation
			void set_eval_sources(std::vector<occlusion_query_info*>& sources);

			// Sets evaluation result. Result is true if conditional evaluation failed
			void set_eval_result(thread* pthr, bool failed);

			// Evaluates the condition by accessing memory directly
			void eval_result(thread* pthr);
		};
	}

	struct frame_statistics_t
	{
		u32 draw_calls;
		s64 setup_time;
		s64 vertex_upload_time;
		s64 textures_upload_time;
		s64 draw_exec_time;
		s64 flip_time;
	};

	struct display_flip_info_t
	{
		std::deque<u32> buffer_queue;
		u32 buffer;
		bool skip_frame;
		bool emu_flip;
		bool in_progress;
		frame_statistics_t stats;

		inline void push(u32 _buffer)
		{
			buffer_queue.push_back(_buffer);
		}

		inline bool pop(u32 _buffer)
		{
			if (buffer_queue.empty())
			{
				return false;
			}

			do
			{
				const auto index = buffer_queue.front();
				buffer_queue.pop_front();

				if (index == _buffer)
				{
					buffer = _buffer;
					return true;
				}
			}
			while (!buffer_queue.empty());

			// Need to observe this happening in the wild
			rsx_log.error("Display queue was discarded while not empty!");
			return false;
		}
	};

	struct backend_configuration
	{
		bool supports_multidraw;             // Draw call batching
		bool supports_hw_a2c;                // Alpha to coverage
		bool supports_hw_renormalization;    // Should be true on NV hardware which matches PS3 texture renormalization behaviour
		bool supports_hw_a2one;              // Alpha to one
		bool supports_hw_conditional_render; // Conditional render
	};

	struct sampled_image_descriptor_base;

	class thread
	{
		u64 timestamp_ctrl = 0;
		u64 timestamp_subvalue = 0;

		display_flip_info_t m_queued_flip{};

	protected:
		std::thread::id m_rsx_thread;
		atomic_t<bool> m_rsx_thread_exiting{ true };

		std::array<push_buffer_vertex_info, 16> vertex_push_buffers;
		std::vector<u32> element_push_buffer;

		s32 m_skip_frame_ctr = 0;
		bool skip_current_frame = false;
		frame_statistics_t stats{};

		backend_configuration backend_config{};

		// FIFO
		std::unique_ptr<FIFO::FIFO_control> fifo_ctrl;
		FIFO::flattening_helper m_flattener;
		u32 fifo_ret_addr = RSX_CALL_STACK_EMPTY;
		u32 saved_fifo_ret = RSX_CALL_STACK_EMPTY;

		// Occlusion query
		bool zcull_surface_active = false;
		std::unique_ptr<reports::ZCULL_control> zcull_ctrl;

		// Framebuffer setup
		rsx::gcm_framebuffer_info m_surface_info[rsx::limits::color_buffers_count];
		rsx::gcm_framebuffer_info m_depth_surface_info;
		framebuffer_layout m_framebuffer_layout;
		bool framebuffer_status_valid = false;

		// Overlays
		rsx::overlays::display_manager* m_overlay_manager = nullptr;

		// Invalidated memory range
		address_range m_invalidated_memory_range;

		// Profiler
		rsx::profiling_timer m_profiler;
		frame_statistics_t m_frame_stats;

	public:
		RsxDmaControl* ctrl = nullptr;
		u32 restore_point = 0;
		atomic_t<bool> external_interrupt_lock{ false };
		atomic_t<bool> external_interrupt_ack{ false };
		void flush_fifo();
		void recover_fifo();
		void fifo_wake_delay(u64 div = 1);
		u32 get_fifo_cmd();

		// Performance approximation counters
		struct
		{
			atomic_t<u64> idle_time{ 0 };  // Time spent idling in microseconds
			u64 last_update_timestamp = 0; // Timestamp of last load update
			u64 FIFO_idle_timestamp = 0;   // Timestamp of when FIFO queue becomes idle
			FIFO_state state = FIFO_state::running;
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

		GcmTileInfo tiles[limits::tiles_count];
		GcmZcullInfo zculls[limits::zculls_count];

		void capture_frame(const std::string &name);

	public:
		std::shared_ptr<named_thread<class ppu_thread>> intr_thread;

		// I hate this flag, but until hle is closer to lle, its needed
		bool isHLE{ false };

		u32 flip_status;
		int debug_level;

		atomic_t<bool> requested_vsync{false};
		atomic_t<bool> enable_second_vhandler{false};

		RsxDisplayInfo display_buffers[8];
		u32 display_buffers_count{0};
		u32 current_display_buffer{0};
		u32 device_addr;
		u32 label_addr;

		u32 main_mem_size{0};
		u32 local_mem_size{0};

		bool m_rtts_dirty;
		bool m_textures_dirty[16];
		bool m_vertex_textures_dirty[4];
		bool m_framebuffer_state_contested = false;
		rsx::framebuffer_creation_context m_current_framebuffer_context = rsx::framebuffer_creation_context::context_draw;

		u32  m_graphics_state = 0;
		u64  ROP_sync_timestamp = 0;

		program_hash_util::fragment_program_utils::fragment_program_metadata current_fp_metadata = {};
		program_hash_util::vertex_program_utils::vertex_program_metadata current_vp_metadata = {};

	protected:
		std::array<u32, 4> get_color_surface_addresses() const;
		u32 get_zeta_surface_address() const;

		void get_framebuffer_layout(rsx::framebuffer_creation_context context, framebuffer_layout &layout);
		bool get_scissor(areau& region, bool clip_viewport);

		/**
		 * Analyze vertex inputs and group all interleaved blocks
		 */
		void analyse_inputs_interleaved(vertex_input_layout&) const;

		RSXVertexProgram current_vertex_program = {};
		RSXFragmentProgram current_fragment_program = {};

		void get_current_vertex_program(const std::array<std::unique_ptr<rsx::sampled_image_descriptor_base>, rsx::limits::vertex_textures_count>& sampler_descriptors, bool skip_textures = false, bool skip_vertex_inputs = true);

		/**
		 * Gets current fragment program and associated fragment state
		 * get_surface_info is a helper takes 2 parameters: rsx_texture_address and surface_is_depth
		 * returns whether surface is a render target and surface pitch in native format
		 */
		void get_current_fragment_program(const std::array<std::unique_ptr<rsx::sampled_image_descriptor_base>, rsx::limits::fragment_textures_count>& sampler_descriptors);

	public:
		double fps_limit = 59.94;

	public:
		u64 start_rsx_time = 0;
		u64 int_flip_index = 0;
		u64 last_flip_time = 0;
		vm::ptr<void(u32)> flip_handler = vm::null;
		vm::ptr<void(u32)> user_handler = vm::null;
		vm::ptr<void(u32)> vblank_handler = vm::null;
		atomic_t<u64> vblank_count{0};

	public:
		bool invalid_command_interrupt_raised = false;
		bool sync_point_request = false;
		bool in_begin_end = false;

		atomic_t<s32> async_tasks_pending{ 0 };

		bool zcull_stats_enabled = false;
		bool zcull_rendering_enabled = false;
		bool zcull_pixel_cnt_enabled = false;

		reports::conditional_render_eval cond_render_ctrl;

		void operator()();
		virtual u64 get_cycles() = 0;
		virtual ~thread();

		static constexpr auto thread_name = "rsx::thread"sv;

	protected:
		thread();
		virtual void on_task();
		virtual void on_exit();

		/**
		 * Execute a backend local task queue
		 */
		virtual void do_local_task(FIFO_state state);

		virtual void on_decompiler_init() {}
		virtual void on_decompiler_exit() {}
		virtual bool on_decompiler_task() { return false; }

		virtual void emit_geometry(u32) {}

		void run_FIFO();

	public:
		virtual void clear_surface(u32 /*arg*/) {};
		virtual void begin();
		virtual void end();
		virtual void execute_nop_draw();

		virtual void on_init_rsx() = 0;
		virtual void on_init_thread() = 0;
		virtual void on_frame_end(u32 buffer, bool forced = false);
		virtual void flip(const display_flip_info_t& info) = 0;
		virtual u64 timestamp();
		virtual bool on_access_violation(u32 /*address*/, bool /*is_writing*/) { return false; }
		virtual void on_invalidate_memory_range(const address_range & /*range*/, rsx::invalidation_cause) {}
		virtual void notify_tile_unbound(u32 /*tile*/) {}

		// control
		virtual void renderctl(u32 /*request_code*/, void* /*args*/) {}

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
		virtual void sync_hint(FIFO_hint hint, void* args);

		gsl::span<const gsl::byte> get_raw_index_array(const draw_clause& draw_indexed_clause) const;

		std::variant<draw_array_command, draw_indexed_array_command, draw_inlined_array>
		get_draw_command(const rsx::rsx_state& state) const;

		/**
		* Immediate mode rendering requires a temp push buffer to hold attrib values
		* Appends a value to the push buffer (currently only supports 32-wide types)
		*/
		void append_to_push_buffer(u32 attribute, u32 size, u32 subreg_index, vertex_base_type type, u32 value);
		u32 get_push_buffer_vertex_count() const;

		void append_array_element(u32 index);
		u32 get_push_buffer_index_count() const;

	protected:

		/**
		 * Computes VRAM requirements needed to upload raw vertex streams
		 * result.first contains persistent memory requirements
		 * result.second contains volatile memory requirements
		 */
		std::pair<u32, u32> calculate_memory_requirements(const vertex_input_layout& layout, u32 first_vertex, u32 vertex_count);

		/**
		 * Generates vertex input descriptors as an array of 16x4 s32s
		 */
		void fill_vertex_layout_state(const vertex_input_layout& layout, u32 first_vertex, u32 vertex_count, s32* buffer, u32 persistent_offset = 0, u32 volatile_offset = 0);

		/**
		 * Uploads vertex data described in the layout descriptor
		 * Copies from local memory to the write-only output buffers provided in a sequential manner
		 */
		void write_vertex_data_to_memory(const vertex_input_layout& layout, u32 first_vertex, u32 vertex_count, void *persistent_data, void *volatile_data);

	private:
		shared_mutex m_mtx_task;

		void handle_emu_flip(u32 buffer);
		void handle_invalidated_memory_range();

	public:
		//std::future<void> add_internal_task(std::function<bool()> callback);
		//void invoke(std::function<bool()> callback);

		/**
		 * Fill buffer with 4x4 scale offset matrix.
		 * Vertex shader's position is to be multiplied by this matrix.
		 * if flip_y is set, the matrix is modified to use d3d convention.
		 */
		void fill_scale_offset_data(void *buffer, bool flip_y) const;

		/**
		 * Fill buffer with user clip information
		 */
		void fill_user_clip_data(void *buffer) const;

		/**
		* Fill buffer with vertex program constants.
		* Buffer must be at least 512 float4 wide.
		*/
		void fill_vertex_program_constants_data(void *buffer);

		/**
		 * Fill buffer with fragment rasterization state.
		 * Fills current fog values, alpha test parameters and texture scaling parameters
		 */
		void fill_fragment_state_buffer(void *buffer, const RSXFragmentProgram &fragment_program);

		/**
		 * Fill buffer with fragment texture parameter constants (texture matrix)
		 */
		void fill_fragment_texture_parameters(void *buffer, const RSXFragmentProgram &fragment_program);

		/**
		 * Notify that a section of memory has been mapped
		 * If there is a notify_memory_unmapped request on this range yet to be handled,
		 * handles it immediately.
		 */
		void on_notify_memory_mapped(u32 address_base, u32 size);

		/**
		 * Notify that a section of memory has been unmapped
		 * Any data held in the defined range is discarded
		 */
		void on_notify_memory_unmapped(u32 address_base, u32 size);

		/**
		 * Notify to check internal state during semaphore wait
		 */
		virtual void on_semaphore_acquire_wait() {}

		/**
		 * Copy rtt values to buffer.
		 * TODO: It's more efficient to combine multiple call of this function into one.
		 */
		virtual std::array<std::vector<std::byte>, 4> copy_render_targets_to_memory() {
			return std::array<std::vector<std::byte>, 4>();
		}

		/**
		* Copy depth and stencil content to buffers.
		* TODO: It's more efficient to combine multiple call of this function into one.
		*/
		virtual std::array<std::vector<std::byte>, 2> copy_depth_stencil_buffer_to_memory() {
			return std::array<std::vector<std::byte>, 2>();
		}

		virtual std::pair<std::string, std::string> get_programs() const { return std::make_pair("", ""); }

		virtual bool scaled_image_from_memory(blit_src_info& /*src_info*/, blit_dst_info& /*dst_info*/, bool /*interpolate*/) { return false; }

	public:
		void reset();
		void init(u32 ctrlAddress);

		tiled_region get_tiled_address(u32 offset, u32 location);
		GcmTileInfo *find_tile(u32 offset, u32 location);

		// Emu App/Game flip, only immediately flips when called from rsxthread
		void request_emu_flip(u32 buffer);

		void pause();
		void unpause();

		// Get RSX approximate load in %
		u32 get_load();

		// Returns true if the current thread is the active RSX thread
		bool is_current_thread() const { return std::this_thread::get_id() == m_rsx_thread; }
	};

	inline thread* get_current_renderer()
	{
		return g_fxo->get<rsx::thread>();
	}
}
