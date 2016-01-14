#pragma once

#include "GCM.h"
#include "RSXTexture.h"
#include "RSXVertexProgram.h"
#include "RSXFragmentProgram.h"

#include <stack>
#include "Utilities/Semaphore.h"
#include "Utilities/Thread.h"
#include "Utilities/Timer.h"
#include "Utilities/convert.h"

extern u64 get_system_time();

struct frame_capture_data
{
	struct buffer
	{
		std::vector<u8> data;
		size_t width = 0, height = 0;
	};

	struct draw_state
	{
		std::string name;
		std::pair<std::string, std::string> programs;
		buffer color_buffer[4];
		buffer depth;
		buffer stencil;
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
	enum class shader_language
	{
		glsl,
		hlsl
	};
}

namespace convert
{
	template<>
	struct to_impl_t<rsx::shader_language, std::string>
	{
		static rsx::shader_language func(const std::string &from)
		{
			if (from == "glsl")
				return rsx::shader_language::glsl;

			if (from == "hlsl")
				return rsx::shader_language::hlsl;

			throw;
		}
	};

	template<>
	struct to_impl_t<std::string, rsx::shader_language>
	{
		static std::string func(rsx::shader_language from)
		{
			switch (from)
			{
			case rsx::shader_language::glsl:
				return "glsl";
			case rsx::shader_language::hlsl:
				return "hlsl";
			}

			throw;
		}
	};
}
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

	u32 get_vertex_type_size_on_host(Vertex_base_type type, u32 size);

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
		u8 antialias;
		u8 depth_format;
		u8 color_format;

		u32 width;
		u32 height;
		u32 format;

		void unpack(u32 surface_format)
		{
			format = surface_format;

			log2height = surface_format >> 24;
			log2width = (surface_format >> 16) & 0xff;
			antialias = (surface_format >> 12) & 0xf;
			depth_format = (surface_format >> 5) & 0x7;
			color_format = surface_format & 0x1f;

			width = 1 << (u32(log2width) + 1);
			height = 1 << (u32(log2width) + 1);
		}
	};

	struct data_array_format_info
	{
		u16 frequency = 0;
		u8 stride = 0;
		u8 size = 0;
		Vertex_base_type type = Vertex_base_type::f;

		void unpack_array(u32 data_array_format)
		{
			frequency = data_array_format >> 16;
			stride = (data_array_format >> 8) & 0xff;
			size = (data_array_format >> 4) & 0xf;
			type = to_vertex_base_type(data_array_format & 0xf);
		}
	};

	class thread : public named_thread_t
	{
	protected:
		std::stack<u32> m_call_stack;

	public:
		struct shaders_cache shaders_cache;

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
		u32 report_main_addr;
		u32 label_addr;
		enum class Draw_command
		{
			draw_command_array,
			draw_command_inlined_array,
			draw_command_indexed,
		} draw_command;
		Primitive_type draw_mode;

		u32 local_mem_addr, main_mem_addr;
		bool strict_ordering[0x1000];


		bool draw_inline_vertex_array;
		std::vector<u32> inline_vertex_array;

	public:
		u32 draw_array_count;
		u32 draw_array_first;
		double fps_limit = 59.94;

	public:
		semaphore_t sem_flip;
		u64 last_flip_time;
		vm::ps3::ptr<void(u32)> flip_handler = vm::null;
		vm::ps3::ptr<void(u32)> user_handler = vm::null;
		vm::ps3::ptr<void(u32)> vblank_handler = vm::null;
		u64 vblank_count;

	public:
		std::set<u32> m_used_gcm_commands;

	protected:
		virtual ~thread() {}

		virtual void on_task() override;

	public:
		virtual std::string get_name() const override;

		virtual void begin();
		virtual void end();

		virtual void on_init() = 0;
		virtual void on_init_thread() = 0;
		virtual bool do_method(u32 cmd, u32 value) { return false; }
		virtual void flip(int buffer) = 0;
		virtual u64 timestamp() const;

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
		virtual void copy_render_targets_to_memory(void *buffer, u8 rtt) {};

		/**
		* Copy depth content to buffer.
		* TODO: It's more efficient to combine multiple call of this function into one.
		*/
		virtual void copy_depth_buffer_to_memory(void *buffer) {};

		/**
		* Copy stencil content to buffer.
		* TODO: It's more efficient to combine multiple call of this function into one.
		*/
		virtual void copy_stencil_buffer_to_memory(void *buffer) {};

		virtual std::pair<std::string, std::string> get_programs() const { return std::make_pair("", ""); };
	public:
		void reset();
		void init(const u32 ioAddress, const u32 ioSize, const u32 ctrlAddress, const u32 localAddress);

		tiled_region get_tiled_address(u32 offset, u32 location);
		GcmTileInfo *find_tile(u32 offset, u32 location);

		u32 ReadIO32(u32 addr);
		void WriteIO32(u32 addr, u32 value);
	};
}
