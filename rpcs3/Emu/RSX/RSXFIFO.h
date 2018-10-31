#pragma once

#include <Utilities/types.h>
#include <Utilities/Atomic.h>
#include <Utilities/mutex.h>
#include <Utilities/thread.h>

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

		struct register_pair
		{
			u32 reg;
			u32 value;
			u32 loc;
			u32 reserved;
		};

		class FIFO_control
		{
		private:
			enum register_props : u8
			{
				none = 0,
				skip_on_match = 1,
				always_ignore = 2
			};

		private:
			RsxDmaControl* m_ctrl = nullptr;
			u32 m_internal_get = 0;

			u32 m_memwatch_addr = 0;
			u32 m_memwatch_cmp = 0;

			u32 m_command_reg = 0;
			u32 m_command_inc = 0;
			u32 m_remaining_commands = 0;
			u32 m_args_ptr = 0;

			std::array<u8, 0x10000 / 4> m_register_properties;
			bool has_deferred_draw = false;

		public:
			FIFO_control(rsx::thread* pctrl);
			~FIFO_control() {}

			void set_get(u32 get, bool spinning = false);
			void set_put(u32 put);

			void read(register_pair& data);
			inline void read_unsafe(register_pair& data);
			inline bool has_next() const;

		public:
			static bool is_blocking_cmd(u32 cmd);
			static bool is_sync_cmd(u32 cmd);
		};
	}
}