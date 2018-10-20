#pragma once

#include <stack>
#include <deque>
#include <set>
#include <mutex>
#include <atomic>
#include <variant>
#include "GCM.h"
#include "rsx_cache.h"
#include "RSXFIFO.h"
#include "RSXTexture.h"
#include "RSXVertexProgram.h"
#include "RSXFragmentProgram.h"
#include "rsx_methods.h"
#include "rsx_utils.h"
#include "Overlays/overlays.h"

#include "Utilities/Thread.h"
#include "Utilities/geometry.h"
#include "Capture/rsx_trace.h"
#include "Capture/rsx_replay.h"

#include "Emu/Cell/lv2/sys_rsx.h"

extern u64 get_system_time();

struct RSXIOTable
{
	u16 ea[4096];
	u16 io[3072];

	// try to get the real address given a mapped address
	// return non zero on success
	inline u32 RealAddr(u32 offs)
	{
		const u32 upper = this->ea[offs >> 20];

		if (static_cast<s16>(upper) < 0)
		{
			return 0;
		}

		return (upper << 20) | (offs & 0xFFFFF);
	}
};

extern bool user_asked_for_frame_capture;
extern rsx::frame_trace_data frame_debug;
extern rsx::frame_capture_data frame_capture;
extern RSXIOTable RSXIOMem;

namespace rsx
{
	namespace limits
	{
		enum
		{
			fragment_textures_count = 16,
			vertex_textures_count = 4,
			vertex_count = 16,
			fragment_count = 32,
			tiles_count = 15,
			zculls_count = 8,
			color_buffers_count = 4
		};
	}

	namespace constants
	{
		static std::array<const char*, 16> fragment_texture_names =
		{
			"tex0", "tex1", "tex2", "tex3", "tex4", "tex5", "tex6", "tex7",
			"tex8", "tex9", "tex10", "tex11", "tex12", "tex13", "tex14", "tex15",
		};

		static std::array<const char*, 4> vertex_texture_names =
		{
			"vtex0", "vtex1", "vtex2", "vtex3",
		};
	}

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
		vertex_texture_state_dirty = 0x80, // Fragment texture parameters changed

		invalidate_pipeline_bits = fragment_program_dirty | vertex_program_dirty,
		memory_barrier_bits = framebuffer_reads_dirty,
		all_dirty = -1u
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
		hint_conditional_render_eval = 1
	};

	u32 get_vertex_type_size_on_host(vertex_base_type type, u32 size);

	u32 get_address(u32 offset, u32 location);

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
		gsl::span<const gsl::byte> data;
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
		gsl::span<const gsl::byte> raw_index_buffer;
	};

	struct draw_inlined_array
	{
		u32 __dummy;
		u32 __dummy2;
	};

	struct interleaved_range_info
	{
		bool interleaved = false;
		bool all_modulus = false;
		bool single_vertex = false;
		u32  base_offset = 0;
		u32  real_offset_address = 0;
		u8   memory_location = 0;
		u8   attribute_stride = 0;
		u16  min_divisor = 0;

		std::vector<u8> locations;
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
		std::vector<std::pair<u8, u32>> volatile_blocks;  // Volatile data blocks (immediate draw vertex data for example)
		std::vector<u8> referenced_registers;  // Volatile register data

		std::array<attribute_buffer_placement, 16> attribute_placement;

		vertex_input_layout()
		{
			attribute_placement.fill(attribute_buffer_placement::none);
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
	};

	struct framebuffer_layout
	{
		u16 width;
		u16 height;
		std::array<u32, 4> color_addresses;
		std::array<u32, 4> color_pitch;
		std::array<u32, 4> actual_color_pitch;
		u32 zeta_address;
		u32 zeta_pitch;
		u32 actual_zeta_pitch;
		rsx::surface_target target;
		rsx::surface_color_format color_format;
		rsx::surface_depth_format depth_format;
		rsx::surface_antialiasing aa_mode;
		u32 aa_factors[2];
		bool ignore_change;
	};

	namespace reports
	{
		struct occlusion_query_info
		{
			u32 driver_handle;
			u32 result;
			u32 num_draws;
			bool pending;
			bool active;
			bool owned;

			u64 sync_timestamp;
		};

		struct queued_report_write
		{
			u32 type = CELL_GCM_ZPASS_PIXEL_CNT;
			u32 counter_tag;
			occlusion_query_info* query;
			queued_report_write* forwarder;
			vm::addr_t sink;

			u64 due_tsc;
		};

		struct ZCULL_control
		{
			// Delay before a report update operation is forced to retire
			const u32 max_zcull_delay_us = 500;
			const u32 min_zcull_delay_us = 50;

			// Number of occlusion query slots available. Real hardware actually has far fewer units before choking
			const u32 occlusion_query_count = 128;

			bool active = false;
			bool enabled = false;

			std::array<occlusion_query_info, 128> m_occlusion_query_data = {};

			occlusion_query_info* m_current_task = nullptr;
			u32 m_statistics_tag_id = 0;
			u64 m_tsc = 0;
			u32 m_cycles_delay = max_zcull_delay_us;

			std::vector<queued_report_write> m_pending_writes;
			std::unordered_map<u32, u32> m_statistics_map;

			ZCULL_control() {}
			~ZCULL_control() {}

			void set_enabled(class ::rsx::thread* ptimer, bool enabled);
			void set_active(class ::rsx::thread* ptimer, bool active);

			void write(vm::addr_t sink, u32 timestamp, u32 type, u32 value);

			// Read current zcull statistics into the address provided
			void read_report(class ::rsx::thread* ptimer, vm::addr_t sink, u32 type);

			// Sets up a new query slot and sets it to the current task
			void allocate_new_query(class ::rsx::thread* ptimer);

			// Clears current stat block and increments stat_tag_id
			void clear(class ::rsx::thread* ptimer);

			// Forcefully flushes all
			void sync(class ::rsx::thread* ptimer);

			// Conditionally sync any pending writes if range overlaps
			void read_barrier(class ::rsx::thread* ptimer, u32 memory_address, u32 memory_range);

			// Call once every 'tick' to update, optional address provided to partially sync until address is processed
			void update(class ::rsx::thread* ptimer, u32 sync_address = 0);

			// Draw call notification
			void on_draw();

			// Check for pending writes
			bool has_pending() const { return (m_pending_writes.size() != 0); }

			// Backend methods (optional, will return everything as always visible by default)
			virtual void begin_occlusion_query(occlusion_query_info* /*query*/) {}
			virtual void end_occlusion_query(occlusion_query_info* /*query*/) {}
			virtual bool check_occlusion_query_status(occlusion_query_info* /*query*/) { return true; }
			virtual void get_occlusion_query_result(occlusion_query_info* query) { query->result = UINT32_MAX; }
			virtual void discard_occlusion_query(occlusion_query_info* /*query*/) {}
		};
	}

	struct sampled_image_descriptor_base;

	class thread
	{
		u64 timestamp_ctrl = 0;
		u64 timestamp_subvalue = 0;

	protected:
		std::thread::id m_rsx_thread;
		atomic_t<bool> m_rsx_thread_exiting{true};
		s32 m_return_addr{-1}, restore_ret_addr{-1};
		std::array<push_buffer_vertex_info, 16> vertex_push_buffers;
		std::vector<u32> element_push_buffer;

		s32 m_skip_frame_ctr = 0;
		bool skip_frame = false;

		bool supports_multidraw = false;
		bool supports_native_ui = false;

		// FIFO
		friend class FIFO::FIFO_control;
		std::unique_ptr<FIFO::FIFO_control> fifo_ctrl;

		// Occlusion query
		bool zcull_surface_active = false;
		std::unique_ptr<reports::ZCULL_control> zcull_ctrl;

		// Framebuffer setup
		rsx::gcm_framebuffer_info m_surface_info[rsx::limits::color_buffers_count];
		rsx::gcm_framebuffer_info m_depth_surface_info;
		bool framebuffer_status_valid = false;

		// Overlays
		std::shared_ptr<rsx::overlays::display_manager> m_overlay_manager;

		// Invalidated memory range
		address_range m_invalidated_memory_range;

	public:
		RsxDmaControl* ctrl = nullptr;
		atomic_t<u32> restore_point{ 0 };
		atomic_t<bool> external_interrupt_lock{ false };
		atomic_t<bool> external_interrupt_ack{ false };

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

		bool capture_current_frame = false;
		void capture_frame(const std::string &name);

	public:
		std::shared_ptr<named_thread<class ppu_thread>> intr_thread;

		// I hate this flag, but until hle is closer to lle, its needed
		bool isHLE{ false };

		u32 ioAddress, ioSize;
		u32 flip_status;
		int debug_level;

		atomic_t<bool> requested_vsync{false};
		atomic_t<bool> enable_second_vhandler{false};

		RsxDisplayInfo display_buffers[8];
		u32 display_buffers_count{0};
		u32 current_display_buffer{0};
		u32 ctxt_addr;
		u32 label_addr;

		u32 local_mem_addr, main_mem_addr, main_mem_size{0};

		bool m_rtts_dirty;
		bool m_textures_dirty[16];
		bool m_vertex_textures_dirty[4];
		bool m_framebuffer_state_contested = false;
		rsx::framebuffer_creation_context m_framebuffer_contest_type = rsx::framebuffer_creation_context::context_draw;

		u32  m_graphics_state = 0;
		u64  ROP_sync_timestamp = 0;

		program_hash_util::fragment_program_utils::fragment_program_metadata current_fp_metadata = {};
		program_hash_util::vertex_program_utils::vertex_program_metadata current_vp_metadata = {};

	protected:
		std::array<u32, 4> get_color_surface_addresses() const;
		u32 get_zeta_surface_address() const;

		framebuffer_layout get_framebuffer_layout(rsx::framebuffer_creation_context context);

		/**
		 * Analyze vertex inputs and group all interleaved blocks
		 */
		vertex_input_layout analyse_inputs_interleaved() const;

		RSXVertexProgram current_vertex_program = {};
		RSXFragmentProgram current_fragment_program = {};

		void get_current_vertex_program(const std::array<std::unique_ptr<rsx::sampled_image_descriptor_base>, rsx::limits::vertex_textures_count>& sampler_descriptors, bool skip_textures = false, bool skip_vertex_inputs = true);

		/**
		 * Gets current fragment program and associated fragment state
		 * get_surface_info is a helper takes 2 parameters: rsx_texture_address and surface_is_depth
		 * returns whether surface is a render target and surface pitch in native format
		 */
		void get_current_fragment_program(const std::array<std::unique_ptr<rsx::sampled_image_descriptor_base>, rsx::limits::fragment_textures_count>& sampler_descriptors);
		void get_current_fragment_program_legacy(std::function<std::tuple<bool, u16>(u32, fragment_texture&, bool)> get_surface_info);

	public:
		double fps_limit = 59.94;

	public:
		u64 start_rsx_time = 0;
		u64 int_flip_index = 0;
		u64 last_flip_time;
		vm::ptr<void(u32)> flip_handler = vm::null;
		vm::ptr<void(u32)> user_handler = vm::null;
		vm::ptr<void(u32)> vblank_handler = vm::null;
		u64 vblank_count;

	public:
		bool invalid_command_interrupt_raised = false;
		bool sync_point_request = false;
		bool in_begin_end = false;

		atomic_t<s32> async_tasks_pending{ 0 };

		u32  conditional_render_test_address = 0;
		bool conditional_render_test_failed = false;
		bool conditional_render_enabled = false;
		bool zcull_stats_enabled = false;
		bool zcull_rendering_enabled = false;
		bool zcull_pixel_cnt_enabled = false;

		void operator()();
		virtual u64 get_cycles() = 0;

	protected:
		thread();
		virtual ~thread();
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
		virtual void begin();
		virtual void end();

		virtual void on_init_rsx() = 0;
		virtual void on_init_thread() = 0;
		virtual bool do_method(u32 /*cmd*/, u32 /*value*/) { return false; }
		virtual void flip(int buffer) = 0;
		virtual u64 timestamp();
		virtual bool on_access_violation(u32 /*address*/, bool /*is_writing*/) { return false; }
		virtual void on_invalidate_memory_range(const address_range & /*range*/) {}
		virtual void notify_tile_unbound(u32 /*tile*/) {}

		// zcull
		void notify_zcull_info_changed();
		void clear_zcull_stats(u32 type);
		void check_zcull_status(bool framebuffer_swap);
		void get_zcull_stats(u32 type, vm::addr_t sink);

		// sync
		void sync();
		void read_barrier(u32 memory_address, u32 memory_range);
		virtual void sync_hint(FIFO_hint hint) {}

		gsl::span<const gsl::byte> get_raw_index_array(const draw_clause& draw_indexed_clause) const;
		gsl::span<const gsl::byte> get_raw_vertex_buffer(const rsx::data_array_format_info&, u32 base_offset, const draw_clause& draw_array_clause) const;

		std::vector<std::variant<vertex_array_buffer, vertex_array_register, empty_vertex_array>>
		get_vertex_buffers(const rsx::rsx_state& state, const u64 consumed_attrib_mask) const;

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
		std::pair<u32, u32> calculate_memory_requirements(const vertex_input_layout& layout, u32 vertex_count);

		/**
		 * Generates vertex input descriptors as an array of 16x4 s32s
		 */
		void fill_vertex_layout_state(const vertex_input_layout& layout, u32 vertex_count, s32* buffer, u32 persistent_offset = 0, u32 volatile_offset = 0);

		/**
		 * Uploads vertex data described in the layout descriptor
		 * Copies from local memory to the write-only output buffers provided in a sequential manner
		 */
		void write_vertex_data_to_memory(const vertex_input_layout& layout, u32 first_vertex, u32 vertex_count, void *persistent_data, void *volatile_data);

	private:
		shared_mutex m_mtx_task;

		struct internal_task_entry
		{
			std::function<bool()> callback;
			//std::promise<void> promise;

			internal_task_entry(std::function<bool()> callback) : callback(callback)
			{
			}
		};

		std::deque<internal_task_entry> m_internal_tasks;
		void do_internal_task();
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
		 * Write inlined array data to buffer.
		 * The storage of inlined data looks different from memory stored arrays.
		 * There is no swapping required except for 4 u8 (according to Bleach Soul Resurection)
		 */
		void write_inline_array_to_buffer(void *dst_buffer);

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
		void on_semaphore_acquire_wait() { do_local_task(FIFO_state::lock_wait); }

		/**
		 * Copy rtt values to buffer.
		 * TODO: It's more efficient to combine multiple call of this function into one.
		 */
		virtual std::array<std::vector<gsl::byte>, 4> copy_render_targets_to_memory() {
			return  std::array<std::vector<gsl::byte>, 4>();
		};

		/**
		* Copy depth and stencil content to buffers.
		* TODO: It's more efficient to combine multiple call of this function into one.
		*/
		virtual std::array<std::vector<gsl::byte>, 2> copy_depth_stencil_buffer_to_memory() {
			return std::array<std::vector<gsl::byte>, 2>();
		};

		virtual std::pair<std::string, std::string> get_programs() const { return std::make_pair("", ""); };

		virtual bool scaled_image_from_memory(blit_src_info& /*src_info*/, blit_dst_info& /*dst_info*/, bool /*interpolate*/){ return false;  }

	public:
		void reset();
		void init(u32 ioAddress, u32 ioSize, u32 ctrlAddress, u32 localAddress);

		tiled_region get_tiled_address(u32 offset, u32 location);
		GcmTileInfo *find_tile(u32 offset, u32 location);

		// Emu App/Game flip, only immediately flips when called from rsxthread
		void request_emu_flip(u32 buffer);

		u32 ReadIO32(u32 addr);
		void WriteIO32(u32 addr, u32 value);

		void pause();
		void unpause();

		//Get RSX approximate load in %
		u32 get_load();
	};
}
