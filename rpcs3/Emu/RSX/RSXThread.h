#pragma once

#include <stack>
#include <deque>
#include <set>
#include <mutex>
#include <atomic>
#include "GCM.h"
#include "rsx_cache.h"
#include "RSXTexture.h"
#include "RSXVertexProgram.h"
#include "RSXFragmentProgram.h"
#include "rsx_methods.h"
#include "rsx_utils.h"
#include "overlays.h"
#include <Utilities/GSL.h>

#include "Utilities/Thread.h"
#include "Utilities/geometry.h"
#include "rsx_trace.h"
#include "restore_new.h"
#include "Utilities/variant.hpp"
#include "define_new_memleakdetect.h"

#include "Emu/Cell/lv2/sys_rsx.h"

extern u64 get_system_time();

extern bool user_asked_for_frame_capture;
extern rsx::frame_capture_data frame_debug;

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

	enum pipeline_state : u8
	{
		fragment_program_dirty = 1,
		vertex_program_dirty = 2,
		fragment_state_dirty = 4,
		vertex_state_dirty = 8,
		transform_constants_dirty = 16,

		invalidate_pipeline_bits = fragment_program_dirty | vertex_program_dirty,
		all_dirty = 255
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
		/**
		* First and count of index subranges.
		*/
		std::vector<std::pair<u32, u32>> indexes_range;
	};

	struct draw_indexed_array_command
	{
		/**
		* First and count of subranges to fetch in index buffer.
		*/
		std::vector<std::pair<u32, u32>> ranges_to_fetch_in_index_buffer;

		gsl::span<const gsl::byte> raw_index_buffer;
	};

	struct draw_inlined_array
	{
		std::vector<u32> inline_vertex_array;
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
		std::vector<interleaved_range_info> interleaved_blocks;  //Interleaved blocks to be uploaded as-is
		std::vector<std::pair<u8, u32>> volatile_blocks;  //Volatile data blocks (immediate draw vertex data for example)
		std::vector<u8> referenced_registers;  //Volatile register data

		std::array<attribute_buffer_placement, 16> attribute_placement;
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

			u32 due_tsc;
		};

		struct ZCULL_control
		{
			//Delay in 'cycles' before a report update operation is forced to retire
			const u32 max_zcull_cycles_delay = 128;
			const u32 min_zcull_cycles_delay = 16;

			//Number of occlusion query slots available. Real hardware actually has far fewer units before choking
			const u32 occlusion_query_count = 128;

			bool active = false;
			bool enabled = false;

			std::array<occlusion_query_info, 128> m_occlusion_query_data = {};

			occlusion_query_info* m_current_task = nullptr;
			u32 m_statistics_tag_id = 0;
			u32 m_tsc = 0;
			u32 m_cycles_delay = max_zcull_cycles_delay;

			std::vector<queued_report_write> m_pending_writes;
			std::unordered_map<u32, u32> m_statistics_map;

			ZCULL_control() {}
			~ZCULL_control() {}

			void set_enabled(class ::rsx::thread* ptimer, bool enabled);
			void set_active(class ::rsx::thread* ptimer, bool active);

			void write(vm::addr_t sink, u32 timestamp, u32 type, u32 value);

			//Read current zcull statistics into the address provided
			void read_report(class ::rsx::thread* ptimer, vm::addr_t sink, u32 type);

			//Sets up a new query slot and sets it to the current task
			void allocate_new_query(class ::rsx::thread* ptimer);

			//clears current stat block and increments stat_tag_id
			void clear(class ::rsx::thread* ptimer);

			//forcefully flushes all
			void sync(class ::rsx::thread* ptimer);

			//conditionally sync any pending writes if range overlaps
			void read_barrier(class ::rsx::thread* ptimer, u32 memory_address, u32 memory_range);

			//call once every 'tick' to update
			void update(class ::rsx::thread* ptimer);

			//Draw call notification
			void on_draw();

			//Check for pending writes
			bool has_pending() const { return (m_pending_writes.size() != 0); }

			//Backend methods (optional, will return everything as always visible by default)
			virtual void begin_occlusion_query(occlusion_query_info* /*query*/) {}
			virtual void end_occlusion_query(occlusion_query_info* /*query*/) {}
			virtual bool check_occlusion_query_status(occlusion_query_info* /*query*/) { return true; }
			virtual void get_occlusion_query_result(occlusion_query_info* query) { query->result = UINT32_MAX; }
			virtual void discard_occlusion_query(occlusion_query_info* /*query*/) {}
		};
	}

	struct sampled_image_descriptor_base;

	class thread : public named_thread
	{
		std::shared_ptr<thread_ctrl> m_vblank_thread;

	protected:
		atomic_t<bool> m_rsx_thread_exiting{false};
		std::stack<u32> m_call_stack;
		std::array<push_buffer_vertex_info, 16> vertex_push_buffers;
		std::vector<u32> element_push_buffer;

		s32 m_skip_frame_ctr = 0;
		bool skip_frame = false;

		bool supports_multidraw = false;
		bool supports_native_ui = false;

		//occlusion query
		bool zcull_surface_active = false;
		std::unique_ptr<reports::ZCULL_control> zcull_ctrl;

		//framebuffer setup
		rsx::gcm_framebuffer_info m_surface_info[rsx::limits::color_buffers_count];
		rsx::gcm_framebuffer_info m_depth_surface_info;
		bool framebuffer_status_valid = false;

		std::unique_ptr<rsx::overlays::user_interface> m_custom_ui;
		std::unique_ptr<rsx::overlays::user_interface> m_invalidated_ui;

	public:
		RsxDmaControl* ctrl = nullptr;
		atomic_t<u32> internal_get{ 0 };
		atomic_t<u32> restore_point{ 0 };
		atomic_t<bool> external_interrupt_lock{ false };
		atomic_t<bool> external_interrupt_ack{ false };

		//performance approximation counters
		struct
		{
			atomic_t<u64> idle_time{ 0 };  //Time spent idling in microseconds
			u64 last_update_timestamp = 0; //Timestamp of last load update
			u64 FIFO_idle_timestamp = 0; //Timestamp of when FIFO queue becomes idle
			bool FIFO_is_idle = false; //True if FIFO is in idle state
			u32 approximate_load = 0;
			u32 sampled_frames = 0;
		}
		performance_counters;

		//native UI interrupts
		atomic_t<bool> native_ui_flip_request{ false };

		GcmTileInfo tiles[limits::tiles_count];
		GcmZcullInfo zculls[limits::zculls_count];

		bool capture_current_frame = false;
		void capture_frame(const std::string &name);

	public:
		std::shared_ptr<class ppu_thread> intr_thread;

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

		u32 local_mem_addr, main_mem_addr;

		bool m_rtts_dirty;
		bool m_textures_dirty[16];
		bool m_vertex_textures_dirty[4];
		bool m_framebuffer_state_contested = false;
		u32  m_graphics_state = 0;

	protected:
		std::array<u32, 4> get_color_surface_addresses() const;
		u32 get_zeta_surface_address() const;

		/**
		 * Analyze vertex inputs and group all interleaved blocks
		 */
		vertex_input_layout analyse_inputs_interleaved() const;

		RSXVertexProgram current_vertex_program = {};
		RSXFragmentProgram current_fragment_program = {};

		program_hash_util::fragment_program_utils::fragment_program_metadata current_fp_metadata = {};
		program_hash_util::vertex_program_utils::vertex_program_metadata current_vp_metadata = {};

		void get_current_vertex_program();

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

		bool conditional_render_test_failed = false;
		bool conditional_render_enabled = false;
		bool zcull_stats_enabled = false;
		bool zcull_rendering_enabled = false;
		bool zcull_pixel_cnt_enabled = false;

	protected:
		thread();
		virtual ~thread();

		virtual void on_task() override;
		virtual void on_exit() override;

		/**
		 * Execute a backend local task queue
		 * Idle argument checks that the FIFO queue is in an idle state
		 */
		virtual void do_local_task(bool /*idle*/) {}

	public:
		virtual std::string get_name() const override;

		virtual void on_init(const std::shared_ptr<void>&) override {} // disable start() (TODO)
		virtual void on_stop() override {} // disable join()

		virtual void begin();
		virtual void end();

		virtual void on_init_rsx() = 0;
		virtual void on_init_thread() = 0;
		virtual bool do_method(u32 /*cmd*/, u32 /*value*/) { return false; }
		virtual void flip(int buffer) = 0;
		virtual u64 timestamp() const;
		virtual bool on_access_violation(u32 /*address*/, bool /*is_writing*/) { return false; }
		virtual void on_notify_memory_unmapped(u32 /*address_base*/, u32 /*size*/) {}
		virtual void notify_tile_unbound(u32 /*tile*/) {}

		//zcull
		void notify_zcull_info_changed();
		void clear_zcull_stats(u32 type);
		void check_zcull_status(bool framebuffer_swap);
		void get_zcull_stats(u32 type, vm::addr_t sink);

		//sync
		void sync();
		void read_barrier(u32 memory_address, u32 memory_range);
		
		gsl::span<const gsl::byte> get_raw_index_array(const std::vector<std::pair<u32, u32> >& draw_indexed_clause) const;
		gsl::span<const gsl::byte> get_raw_vertex_buffer(const rsx::data_array_format_info&, u32 base_offset, const std::vector<std::pair<u32, u32>>& vertex_ranges) const;

		std::vector<std::variant<vertex_array_buffer, vertex_array_register, empty_vertex_array>>
		get_vertex_buffers(const rsx::rsx_state& state, const std::vector<std::pair<u32, u32>>& vertex_ranges, const u64 consumed_attrib_mask) const;

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

	public:
		//std::future<void> add_internal_task(std::function<bool()> callback);
		//void invoke(std::function<bool()> callback);
		void add_user_interface(std::shared_ptr<rsx::overlays::user_interface> iface);
		void remove_user_interface();

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
		* Write inlined array data to buffer.
		* The storage of inlined data looks different from memory stored arrays.
		* There is no swapping required except for 4 u8 (according to Bleach Soul Resurection)
		*/
		void write_inline_array_to_buffer(void *dst_buffer);

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

		u32 ReadIO32(u32 addr);
		void WriteIO32(u32 addr, u32 value);

		void pause();
		void unpause();

		//Get RSX approximate load in %
		u32 get_load();

		//HLE vsh stuff
		//TODO: Move into a separate helper
		virtual rsx::overlays::save_dialog* shell_open_save_dialog();
		virtual rsx::overlays::message_dialog* shell_open_message_dialog();
		virtual rsx::overlays::trophy_notification* shell_open_trophy_notification();
		virtual rsx::overlays::user_interface* shell_get_current_dialog();
		virtual bool shell_close_dialog();
		virtual void shell_do_cleanup(){}
	};
}
