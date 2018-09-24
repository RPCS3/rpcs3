#pragma once

#include <Utilities/types.h>
#include <Utilities/Atomic.h>

#include <vector>

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
			FIFO_PACKET_BEGIN = 0xF1F0,
			FIFO_DISABLED_COMMAND = 0xF1F4,
		};

		struct register_pair
		{
			u32 reg;
			u32 value;
			u32 loc;
		};

		struct optimization_pass
		{
			virtual void optimize(std::vector<register_pair>& commands, const u32* registers) const = 0;
		};

		struct flattening_pass : public optimization_pass
		{
			void optimize(std::vector<register_pair>& commands, const u32* registers) const override;
		};

		struct reordering_pass : public optimization_pass
		{
			void optimize(std::vector<register_pair>& commands, const u32* registers) const override;
		};

		class FIFO_control
		{
			RsxDmaControl* m_ctrl = nullptr;
			u32 m_internal_get = 0;

			std::vector<std::unique_ptr<optimization_pass>> m_optimization_passes;

			std::vector<register_pair> m_queue;
			atomic_t<u32> m_command_index{ 0 };

			bool is_blocking_cmd(u32 cmd);
			bool is_sync_cmd(u32 cmd);

			void read_ahead();
			void optimize();
			void clear_buffer();

		public:
			FIFO_control(rsx::thread* pctrl);
			~FIFO_control() {}

			void set_get(u32 get);
			void set_put(u32 put);

			register_pair read();

			void register_optimization_pass(optimization_pass* pass);
		};
	}
}