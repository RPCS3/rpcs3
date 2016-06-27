#pragma once

#include <stack>
#include <deque>
#include <set>
#include <mutex>
#include "GCM.h"
#include "rsx_cache.h"
#include "RSXTexture.h"
#include "RSXVertexProgram.h"
#include "RSXFragmentProgram.h"

#include "Utilities/Thread.h"
#include "Utilities/Timer.h"
#include "Utilities/geometry.h"

extern u64 get_system_time();

struct frame_capture_data
{
	struct draw_state
	{
		std::string name;
		std::pair<std::string, std::string> programs;
		size_t width = 0, height = 0;
		rsx::surface_color_format color_format;
		std::array<std::vector<gsl::byte>, 4> color_buffer;
		rsx::surface_depth_format depth_format;
		std::array<std::vector<gsl::byte>, 2> depth_stencil;
		rsx::index_array_type index_type;
		std::vector<gsl::byte> index;
		u32 vertex_count;
	};
	std::vector<std::pair<u32, u32> > command_queue;
	std::vector<draw_state> draw_calls;

	void reset()
	{
		command_queue.clear();
		draw_calls.clear();
	}
};

extern bool user_asked_for_frame_capture;
extern frame_capture_data frame_debug;

namespace rsx
{
	namespace old_shaders_cache
	{
		enum class shader_language
		{
			glsl,
			hlsl,
		};
	}
}

template<>
struct unveil<rsx::old_shaders_cache::shader_language>
{
	static inline const char* get(rsx::old_shaders_cache::shader_language in)
	{
		switch (in)
		{
		case rsx::old_shaders_cache::shader_language::glsl: return "glsl";
		case rsx::old_shaders_cache::shader_language::hlsl: return "hlsl";
		}

		return "";
	}
};

namespace rsx
{
	namespace limits
	{
		enum
		{
			textures_count = 16,
			vertex_textures_count = 4,
			vertex_count = 16,
			fragment_count = 32,
			tiles_count = 15,
			zculls_count = 8,
			color_buffers_count = 4
		};
	}

	namespace old_shaders_cache
	{
		struct decompiled_shader
		{
			std::string code;
		};

		struct finalized_shader
		{
			u64 ucode_hash;
			std::string code;
		};

		template<typename Type, typename KeyType = u64, typename Hasher = std::hash<KeyType>>
		struct cache
		{
		private:
			std::unordered_map<KeyType, Type, Hasher> m_entries;

		public:
			const Type* find(u64 key) const
			{
				auto found = m_entries.find(key);

				if (found == m_entries.end())
					return nullptr;

				return &found->second;
			}

			void insert(KeyType key, const Type &shader)
			{
				m_entries.insert({ key, shader });
			}
		};

		struct shaders_cache
		{
			cache<decompiled_shader> decompiled_fragment_shaders;
			cache<decompiled_shader> decompiled_vertex_shaders;
			cache<finalized_shader> finailized_fragment_shaders;
			cache<finalized_shader> finailized_vertex_shaders;

			void load(const std::string &path, shader_language lang);
			void load(shader_language lang);

			static std::string path_to_root();
		};
	}

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

	struct surface_info
	{
		u8 log2height;
		u8 log2width;
		surface_antialiasing antialias;
		surface_depth_format depth_format;
		surface_color_format color_format;

		u32 width;
		u32 height;
		u32 format;

		void unpack(u32 surface_format)
		{
			format = surface_format;

			log2height = surface_format >> 24;
			log2width = (surface_format >> 16) & 0xff;
			antialias = to_surface_antialiasing((surface_format >> 12) & 0xf);
			depth_format = to_surface_depth_format((surface_format >> 5) & 0x7);
			color_format = to_surface_color_format(surface_format & 0x1f);

			width = 1 << (u32(log2width) + 1);
			height = 1 << (u32(log2width) + 1);
		}
	};

	struct data_array_format_info
	{
		u16 frequency = 0;
		u8 stride = 0;
		u8 size = 0;
		vertex_base_type type = vertex_base_type::f;

		void unpack_array(u32 data_array_format)
		{
			frequency = data_array_format >> 16;
			stride = (data_array_format >> 8) & 0xff;
			size = (data_array_format >> 4) & 0xf;
			type = to_vertex_base_type(data_array_format & 0xf);
		}
	};

	enum class draw_command
	{
		none,
		array,
		inlined_array,
		indexed,
	};

	class thread : public named_thread
	{
	protected:
		std::stack<u32> m_call_stack;

	public:
		old_shaders_cache::shaders_cache shaders_cache;
		rsx::programs_cache programs_cache;

		CellGcmControl* ctrl = nullptr;

		Timer timer_sync;

		GcmTileInfo tiles[limits::tiles_count];
		GcmZcullInfo zculls[limits::zculls_count];

		rsx::texture textures[limits::textures_count];
		rsx::vertex_texture vertex_textures[limits::vertex_textures_count];


		/**
		 * RSX can sources vertex attributes from 2 places:
		 * - Immediate values passed by NV4097_SET_VERTEX_DATA*_M + ARRAY_ID write.
		 * For a given ARRAY_ID the last command of this type defines the actual type of the immediate value.
		 * Since there can be only a single value per ARRAY_ID passed this way, all vertex in the draw call
		 * shares it.
		 * - Vertex array values passed by offset/stride/size/format description.
		 *
		 * A given ARRAY_ID can have both an immediate value and a vertex array enabled at the same time
		 * (See After Burner Climax intro cutscene). In such case the vertex array has precedence over the
		 * immediate value. As soon as the vertex array is disabled (size set to 0) the immediate value
		 * must be used if the vertex attrib mask request it.
		 *
		 * Note that behavior when both vertex array and immediate value system are disabled but vertex attrib mask
		 * request inputs is unknow.
		 */
		data_array_format_info register_vertex_info[limits::vertex_count];
		std::vector<u8> register_vertex_data[limits::vertex_count];
		data_array_format_info vertex_arrays_info[limits::vertex_count];
		u32 vertex_draw_count = 0;

		std::unordered_map<u32, color4_base<f32>> transform_constants;

		/**
		* Stores the first and count argument from draw/draw indexed parameters between begin/end clauses.
		*/
		std::vector<std::pair<u32, u32> > first_count_commands;

		// Constant stored for whole frame
		std::unordered_map<u32, color4f> local_transform_constants;

		u32 transform_program[512 * 4] = {};

		bool capture_current_frame = false;
		void capture_frame(const std::string &name);

	public:
		u32 ioAddress, ioSize;
		int flip_status;
		int flip_mode;
		int debug_level;
		int frequency_mode;

		u32 tiles_addr;
		u32 zculls_addr;
		vm::ps3::ptr<CellGcmDisplayInfo> gcm_buffers;
		u32 gcm_buffers_count;
		u32 gcm_current_buffer;
		u32 ctxt_addr;
		u32 label_addr;
		rsx::draw_command draw_command;
		primitive_type draw_mode;

		u32 local_mem_addr, main_mem_addr;
		bool strict_ordering[0x1000];

		bool draw_inline_vertex_array;
		std::vector<u32> inline_vertex_array;

		bool m_rtts_dirty;
		bool m_transform_constants_dirty;
		bool m_textures_dirty[16];
	protected:
		std::array<u32, 4> get_color_surface_addresses() const;
		u32 get_zeta_surface_address() const;
		RSXVertexProgram get_current_vertex_program() const;
		RSXFragmentProgram get_current_fragment_program() const;
	public:
		u32 draw_array_count;
		u32 draw_array_first;
		double fps_limit = 59.94;

	public:
		u64 last_flip_time;
		vm::ps3::ptr<void(u32)> flip_handler = vm::null;
		vm::ps3::ptr<void(u32)> user_handler = vm::null;
		vm::ps3::ptr<void(u32)> vblank_handler = vm::null;
		u64 vblank_count;

	public:
		std::set<u32> m_used_gcm_commands;

	protected:
		thread();
		virtual ~thread();

		virtual void on_task() override;

	public:
		virtual std::string get_name() const override;

		virtual void on_init() override {} // disable start() (TODO)
		virtual void on_stop() override {} // disable join()

		virtual void begin();
		virtual void end();

		virtual void on_init_rsx() = 0;
		virtual void on_init_thread() = 0;
		virtual bool do_method(u32 cmd, u32 value) { return false; }
		virtual void flip(int buffer) = 0;
		virtual u64 timestamp() const;
		virtual bool on_access_violation(u32 address, bool is_writing) { return false; }

	private:
		std::mutex m_mtx_task;

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

		/**
		 * Fill buffer with 4x4 scale offset matrix.
		 * Vertex shader's position is to be multiplied by this matrix.
		 * if is_d3d is set, the matrix is modified to use d3d convention.
		 */
		void fill_scale_offset_data(void *buffer, bool is_d3d = true) const;

		/**
		* Fill buffer with vertex program constants.
		* Buffer must be at least 512 float4 wide.
		*/
		void fill_vertex_program_constants_data(void *buffer);

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

		struct raw_program get_raw_program() const;
	public:
		void reset();
		void init(const u32 ioAddress, const u32 ioSize, const u32 ctrlAddress, const u32 localAddress);

		tiled_region get_tiled_address(u32 offset, u32 location);
		GcmTileInfo *find_tile(u32 offset, u32 location);

		u32 ReadIO32(u32 addr);
		void WriteIO32(u32 addr, u32 value);
	};
}
