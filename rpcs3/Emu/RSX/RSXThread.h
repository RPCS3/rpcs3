#pragma once
#include "GCM.h"
#include "RSXTexture.h"
#include "RSXVertexProgram.h"
#include "RSXFragmentProgram.h"

#include <stack>
#include "Utilities/Semaphore.h"
#include "Utilities/Thread.h"
#include "Utilities/Timer.h"
#include "Utilities/types.h"


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

	void reset() noexcept
	{
		command_queue.clear();
		draw_calls.clear();
	}
};

extern bool user_asked_for_frame_capture;
extern frame_capture_data frame_debug;

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

	//TODO
	union alignas(4) method_registers_t
	{
		u8 _u8[0x10000];
		u32 _u32[0x10000 >> 2];
/*
		struct alignas(4)
		{
			u8 pad[NV4097_SET_TEXTURE_OFFSET - 4];

			struct alignas(4) texture_t
			{
				u32 offset;

				union format_t
				{
					u32 _u32;

					struct
					{
						u32: 1;
						u32 location : 1;
						u32 cubemap : 1;
						u32 border_type : 1;
						u32 dimension : 4;
						u32 format : 8;
						u32 mipmap : 16;
					};
				} format;

				union address_t
				{
					u32 _u32;

					struct
					{
						u32 wrap_s : 4;
						u32 aniso_bias : 4;
						u32 wrap_t : 4;
						u32 unsigned_remap : 4;
						u32 wrap_r : 4;
						u32 gamma : 4;
						u32 signed_remap : 4;
						u32 zfunc : 4;
					};
				} address;

				u32 control0;
				u32 control1;
				u32 filter;
				u32 image_rect;
				u32 border_color;
			} textures[limits::textures_count];
		};
*/
		u32& operator[](int index)
		{
			return _u32[index >> 2];
		}
	};

	extern u32 method_registers[0x10000 >> 2];

	u32 get_vertex_type_size(u32 type);

	u32 get_address(u32 offset, u32 location);

	template<typename T>
	void pad_texture(void* inputPixels, void* outputPixels, u16 inputWidth, u16 inputHeight, u16 outputWidth, u16 outputHeight) 
	{
		T *src, *dst;
		src = static_cast<T*>(inputPixels);
		dst = static_cast<T*>(outputPixels);

		for (u16 h = 0; h < inputHeight; ++h)
		{
			const u32 padded_pos = h * outputWidth;
			const u32 pos = h * inputWidth;
			for (u16 w = 0; w < inputWidth; ++w)
			{
				dst[padded_pos + w] = src[pos + w];
			}
		}
	}

	/*   Note: What the ps3 calls swizzling in this case is actually z-ordering / morton ordering of pixels
	*       - Input can be swizzled or linear, bool flag handles conversion to and from
	*       - It will handle any width and height that are a power of 2, square or non square
	*	 Restriction: It has mixed results if the height or width is not a power of 2
	*/
	template<typename T>
	void convert_linear_swizzle(void* inputPixels, void* outputPixels, u16 width, u16 height, bool inputIsSwizzled)
	{
		u32 log2width, log2height;

		log2width = log2(width);
		log2height = log2(height);

		// Max mask possible for square texture
		u32 x_mask = 0x55555555;
		u32 y_mask = 0xAAAAAAAA;

		// We have to limit the masks to the lower of the two dimensions to allow for non-square textures
		u32 limit_mask = (log2width < log2height) ? log2width : log2height;
		// double the limit mask to account for bits in both x and y
		limit_mask = 1 << (limit_mask << 1);

		//x_mask, bits above limit are 1's for x-carry
		x_mask = (x_mask | ~(limit_mask - 1));
		//y_mask. bits above limit are 0'd, as we use a different method for y-carry over
		y_mask = (y_mask & (limit_mask - 1));

		u32 offs_y = 0;
		u32 offs_x = 0;
		u32 offs_x0 = 0; //total y-carry offset for x
		u32 y_incr = limit_mask;

		T *src, *dst;

		if (!inputIsSwizzled)
		{
			for (int y = 0; y < height; ++y) 
			{
				src = static_cast<T*>(inputPixels) + y*width;
				dst = static_cast<T*>(outputPixels) + offs_y;
				offs_x = offs_x0;
				for (int x = 0; x < width; ++x) 
				{
					dst[offs_x] = src[x];
					offs_x = (offs_x - x_mask) & x_mask;
				}
				offs_y = (offs_y - y_mask) & y_mask;
				if (offs_y == 0) offs_x0 += y_incr;
			}
		}
		else 
		{
			for (int y = 0; y < height; ++y) 
			{
				src = static_cast<T*>(inputPixels) + offs_y;
				dst = static_cast<T*>(outputPixels) + y*width;
				offs_x = offs_x0;
				for (int x = 0; x < width; ++x) 
				{
					dst[x] = src[offs_x];
					offs_x = (offs_x - x_mask) & x_mask;
				}
				offs_y = (offs_y - y_mask) & y_mask;
				if (offs_y == 0) offs_x0 += y_incr;
			}
		}
	}

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
		u8 type = CELL_GCM_VERTEX_F;
		bool array = false;

		void unpack(u32 data_array_format)
		{
			frequency = data_array_format >> 16;
			stride = (data_array_format >> 8) & 0xff;
			size = (data_array_format >> 4) & 0xf;
			type = data_array_format & 0xf;
		}
	};

	class thread : protected named_thread_t
	{
	protected:
		std::stack<u32> m_call_stack;

	public:
		CellGcmControl* ctrl = nullptr;

		Timer timer_sync;

		GcmTileInfo tiles[limits::tiles_count];
		GcmZcullInfo zculls[limits::zculls_count];

		rsx::texture textures[limits::textures_count];
		rsx::vertex_texture vertex_textures[limits::vertex_textures_count];

		data_array_format_info vertex_arrays_info[limits::vertex_count];
		std::vector<u8> vertex_arrays[limits::vertex_count];
		std::vector<u8> vertex_index_array;
		u32 vertex_draw_count = 0;

		std::unordered_map<u32, color4_base<f32>> transform_constants;

		// Constant stored for whole frame
		std::unordered_map<u32, color4f> local_transform_constants;

		u32 transform_program[512 * 4] = {};

		virtual void load_vertex_data(u32 first, u32 count);
		virtual void load_vertex_index_data(u32 first, u32 count);

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
		u32 draw_mode;

		u32 local_mem_addr, main_mem_addr;
		bool strict_ordering[0x1000];

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

	public:
		virtual void begin();
		virtual void end();

		virtual void oninit() = 0;
		virtual void oninit_thread() = 0;
		virtual void onexit_thread() = 0;
		virtual bool domethod(u32 cmd, u32 value) { return false; }
		virtual void flip(int buffer) = 0;
		virtual u64 timestamp() const;

		void task();

		/**
		 * Fill buffer with 4x4 scale offset matrix.
		 * Vertex shader's position is to be multiplied by this matrix.
		 * if is_d3d is set, the matrix is modified to use d3d convention.
		 */
		void fill_scale_offset_data(void *buffer, bool is_d3d = true) const noexcept;

		/**
		* Fill buffer with vertex program constants.
		* Buffer must be at least 512 float4 wide.
		*/
		void fill_vertex_program_constants_data(void *buffer) noexcept;

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

		u32 ReadIO32(u32 addr);
		void WriteIO32(u32 addr, u32 value);
	};
}
