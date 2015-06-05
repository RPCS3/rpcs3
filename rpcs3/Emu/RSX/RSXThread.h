#pragma once
#include "GCM.h"
#include "RSXTexture.h"
#include "RSXVertexProgram.h"
#include "RSXFragmentProgram.h"

#include <stack>
#include "Utilities/SSemaphore.h"
#include "Utilities/Thread.h"
#include "Utilities/Timer.h"
#include "types.h"

namespace rsx
{
	namespace limits
	{
		enum
		{
			textures_count = 16,
			vertex_count = 16,
			fragment_count = 32,
			tiles_count = 15,
			zculls_count = 8,
			color_buffers_count = 4
		};
	}

#pragma pack(push, 4)
	//TODO
	union method_registers_t
	{
		u8 _u8[0x10000];
		u32 _u32[0x10000 >> 2];

		struct
		{
			u8 pad[NV4097_SET_TEXTURE_OFFSET - 4];

			struct texture_t
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

		u32& operator[](int index)
		{
			return _u32[index >> 2];
		}
	};
#pragma pack(pop)

	extern u32 method_registers[0x10000 >> 2];

	u32 get_address(u32 offset, u32 location);
	u32 linear_to_swizzle(u32 x, u32 y, u32 z, u32 log2_width, u32 log2_height, u32 log2_depth);

	u32 get_vertex_type_size(u32 type);

	struct vertex_array_t
	{
		struct entry
		{
			u32 first;
			u32 count;
		};

		std::vector<u8> data;
		std::vector<entry> entries;

		void clear()
		{
			data.clear(); //check it
			entries.clear();
		}
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
		u8 type = CELL_GCM_VERTEX_F;

		void unpack(u32 data_array_format)
		{
			frequency = data_array_format >> 16;
			stride = (data_array_format >> 8) & 0xff;
			size = (data_array_format >> 4) & 0xf;
			type = data_array_format & 0xf;
		}
	};

	class thread : public ThreadBase
	{
	protected:
		std::stack<u32> m_call_stack;

	public:
		CellGcmControl* ctrl = nullptr;

		Timer timer_sync;

		GcmTileInfo tiles[limits::tiles_count];
		GcmZcullInfo zculls[limits::zculls_count];

		rsx::texture m_textures[limits::textures_count];
		rsx::vertex_texture m_vertex_textures[limits::textures_count];

		vertex_array_t vertex_index_array;

		data_array_format_info vertex_arrays_info[limits::vertex_count];
		vertex_array_t vertex_arrays[limits::vertex_count];
		std::vector<vertex_array_t::entry> vertex_array_draw_info;

		std::unordered_map<u32, color4_base<f32>> fragment_constants;
		std::unordered_map<u32, color4_base<f32>> transform_constants;

		u32 transform_program[512 * 4] = {};

		void load_vertex_data(u32 first, u32 count);

	public:
		u32 ioAddress, ioSize;
		int flip_status;
		int flip_mode;
		int debug_level;
		int frequency_mode;

		u32 tiles_addr;
		u32 zculls_addr;
		vm::ptr<CellGcmDisplayInfo> gcm_buffers;
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
		std::mutex cs_main;
		SSemaphore sem_flush;
		SSemaphore sem_flip;
		u64 last_flip_time;
		vm::ptr<void(u32)> flip_handler;
		vm::ptr<void(u32)> user_handler;
		u64 vblank_count;
		vm::ptr<void(u32)> vblank_handler;

	public:
		std::set<u32> m_used_gcm_commands;

	protected:
		thread() : ThreadBase("RSXThread")
		{
			flip_handler.set(0);
			vblank_handler.set(0);
			user_handler.set(0);
		}

		virtual ~thread() {}

	public:
		virtual void begin();
		virtual void end();

		virtual void oninit() = 0;
		virtual void oninit_thread() = 0;
		virtual void onexit_thread() = 0;
		virtual bool domethod(u32 cmd, u32 value) { return false; }
		virtual void flip(int buffer) = 0;

		void Task() override;

	public:
		void reset();
		void init(const u32 ioAddress, const u32 ioSize, const u32 ctrlAddress, const u32 localAddress);

		u32 ReadIO32(u32 addr);
		void WriteIO32(u32 addr, u32 value);
	};
}
