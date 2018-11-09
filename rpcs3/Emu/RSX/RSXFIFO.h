#pragma once

#include <Utilities/types.h>
#include <Utilities/Atomic.h>
#include <Utilities/mutex.h>
#include <Utilities/Thread.h>

#include "rsx_utils.h"

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

#ifndef __unused
#define __unused(expression) do { (void)(expression); } while(0)
#endif

struct RsxDmaControl;

namespace rsx
{
	class thread;

	namespace FIFO
	{
		enum internal_commands : u32
		{
			NOP = 0,
			FIFO_EMPTY = 0xDEADF1F0,
			FIFO_BUSY = 0xBABEF1F0,
			FIFO_ERROR = 0xDEADBEEF,
			FIFO_PACKET_BEGIN = 0xF1F0,
			FIFO_DISABLED_COMMAND = 0xF1F4,
			FIFO_DRAW_BARRIER = 0xF1F8,
		};

		enum flatten_op : u32
		{
			NOTHING = 0,
			EMIT_END = 1,
			EMIT_BARRIER = 2
		};

		struct register_pair
		{
			u32 reg;
			u32 value;
			u32 loc;
			u32 reserved;
		};

		class flattening_helper
		{
			enum register_props : u8
			{
				none = 0,
				skip_on_match = 1,
				always_ignore = 2
			};

			enum optimization_hint : u8
			{
				unknown,
				load_low,
				load_unoptimizable
			};

			std::array<u8, 0x10000 / 4> m_register_properties;
			u32 deferred_primitive = 0;
			u32 draw_count = 0;
			u32 begin_end_ctr = 0;

			bool enabled = false;
			u32  num_collapsed = 0;
			optimization_hint fifo_hint = unknown;

		public:
			flattening_helper();
			~flattening_helper() {}

			u32 get_primitive() const { return deferred_primitive; }
			bool is_enabled() const { return enabled; }

			void evaluate_performance(u32 total_draw_count);
			inline flatten_op test(register_pair& command);
		};

		class FIFO_control
		{
		private:
			RsxDmaControl* m_ctrl = nullptr;
			u32 m_internal_get = 0;

			u32 m_memwatch_addr = 0;
			u32 m_memwatch_cmp = 0;

			u32 m_command_reg = 0;
			u32 m_command_inc = 0;
			u32 m_remaining_commands = 0;
			u32 m_args_ptr = 0;

		public:
			FIFO_control(rsx::thread* pctrl);
			~FIFO_control() {}

			void set_get(u32 get, bool spinning = false);
			void set_put(u32 put);

			void read(register_pair& data);
			inline void read_unsafe(register_pair& data);
		};
	}
}