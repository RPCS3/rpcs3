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
			zculls_count = 8
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

	struct vertex_data_t
	{
		std::vector<u8> data;

		void load(u32 start, u32 count);
	};

	struct vertex_data_array_t
	{
		vertex_data_t vertex_data[limits::vertex_count];

		vertex_data_t& operator[](int index)
		{
			return vertex_data[index];
		}

		//returns first - data, second - offsets, third - vertex_data indexes
		std::tuple<std::vector<u8>, std::vector<size_t>, std::vector<int>> load(u32 first, u32 count)
		{
			std::vector<u8> result_data;
			std::vector<size_t> offsets;
			std::vector<int> indexes;

			for (int i = 0; i < limits::vertex_count; ++i)
			{
				vertex_data[i].load(first, count);

				if (vertex_data[i].data.empty())
					continue;

				size_t offset = result_data.size();
				size_t size = vertex_data[i].data.size();

				offsets.push_back(offset);
				indexes.push_back(i);
				result_data.resize(offset + size);
				memcpy(result_data.data() + offset, vertex_data[i].data.data(), size);
			}

			return std::make_tuple(result_data, offsets, indexes);
		}
	};

	struct index_array_info_t
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

	class thread : public ThreadBase
	{
	protected:
		std::stack<u32> m_call_stack;
		CellGcmControl* m_ctrl;

	public:
		Timer timer_sync;

		GcmTileInfo m_tiles[limits::tiles_count];
		GcmZcullInfo m_zculls[limits::zculls_count];

		rsx::texture m_textures[limits::textures_count];
		rsx::vertex_texture m_vertex_textures[limits::textures_count];

		vertex_data_array_t vertex_data_array;
		index_array_info_t index_array;
		std::unordered_map<u32, color4_base<f32>> m_fragment_constants;
		std::unordered_map<u32, color4_base<f32>> m_transform_constants;

		u32 m_shader_ctrl, m_cur_fragment_prog_num;
		u32 m_vertex_program_data[512 * 4] = {};
		RSXFragmentProgram m_fragment_progs[limits::fragment_count];
		RSXFragmentProgram* m_cur_fragment_prog;
		//RSXVertexProgram m_vertex_progs[m_vertex_count];
		//RSXVertexProgram* m_cur_vertex_prog;

	public:
		u32 ioAddress, ioSize, ctrlAddress;
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
		bool m_draw_mode = false;

	protected:
		thread()
			: ThreadBase("RSXThread")
			, m_ctrl(nullptr)
			, m_shader_ctrl(0x40)
			, m_draw_mode(0)
		{
			flip_handler.set(0);
			vblank_handler.set(0);
			user_handler.set(0);

			// Construct Textures
			for (int i = 0; i < 16; i++)
			{
				m_textures[i].init(i);
			}
		}

		virtual ~thread() {}

	public:
		void begin();
		void end();

		virtual void oninit() = 0;
		virtual void oninit_thread() = 0;
		virtual void onexit_thread() = 0;
		virtual void onreset() = 0;
		virtual bool domethod(u32 cmd, u32 value) { return false; }
		virtual void flip(int buffer) = 0;

		void Task();

	public:
		void init(const u32 ioAddress, const u32 ioSize, const u32 ctrlAddress, const u32 localAddress);

		u32 ReadIO32(u32 addr);
		void WriteIO32(u32 addr, u32 value);
	};
}
