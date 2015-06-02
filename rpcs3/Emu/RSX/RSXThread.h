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
	extern u32 method_registers[0x10000 >> 2];

	u32 get_address(u32 offset, u32 location);
	u32 linear_to_swizzle(u32 x, u32 y, u32 z, u32 log2_width, u32 log2_height, u32 log2_depth);

	struct RSXVertexData
	{
		u32 frequency;
		u32 stride;
		u32 size;
		u32 type;
		u32 addr;
		u32 constant_count;

		std::vector<u8> data;

		RSXVertexData();

		void Reset();
		bool IsEnabled() const { return size > 0; }
		void Load(u32 start, u32 count, u32 baseOffset, u32 baseIndex);

		u32 GetTypeSize();
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
	public:
		static const uint m_textures_count = 16;
		static const uint m_vertex_count = 16;
		static const uint m_fragment_count = 32;
		static const uint m_tiles_count = 15;
		static const uint m_zculls_count = 8;

	protected:
		std::stack<u32> m_call_stack;
		CellGcmControl* m_ctrl;

	public:
		Timer timer_sync;

		GcmTileInfo m_tiles[m_tiles_count];
		GcmZcullInfo m_zculls[m_zculls_count];

		rsx::texture m_textures[m_textures_count];
		rsx::vertex_texture m_vertex_textures[m_textures_count];

		RSXVertexData m_vertex_data[m_vertex_count];
		index_array_info_t index_array;
		std::unordered_map<u32, color4_base<f32>> m_fragment_constants;
		std::unordered_map<u32, color4_base<f32>> m_transform_constants;

		u32 m_shader_ctrl, m_cur_fragment_prog_num;
		u32 m_vertex_program_data[512 * 4] = {};
		RSXFragmentProgram m_fragment_progs[m_fragment_count];
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
		void LoadVertexData(u32 first, u32 count)
		{
			for (u32 i = 0; i < m_vertex_count; ++i)
			{
				if (!m_vertex_data[i].IsEnabled()) continue;

				//m_vertex_data[i].Load(first, count, m_vertex_data_base_offset, m_vertex_data_base_index);
			}
		}

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
