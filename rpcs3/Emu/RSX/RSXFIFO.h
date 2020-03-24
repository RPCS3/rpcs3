#pragma once

#include "Utilities/types.h"
#include "util/atomic.hpp"

#include "rsx_utils.h"
#include "Emu/Cell/lv2/sys_rsx.h"

namespace rsx
{
	class thread;
	struct rsx_iomap_table;

	namespace FIFO
	{
		enum internal_commands : u32
		{
			FIFO_NOP = 0xBABEF1F4,
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

			void set(u32 reg, u32 val)
			{
				this->reg = reg;
				this->value = val;
			}
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
				load_unoptimizable,
				application_not_compatible
			};

			// Workaround for MSVC, C2248
			static constexpr u8 register_props_always_ignore = register_props::always_ignore;

			static constexpr std::array<u8, 0x10000 / 4> m_register_properties = []
			{
				constexpr std::array<std::pair<u32, u32>, 4> ignorable_ranges =
				{{
					// General
					{ NV4097_INVALIDATE_VERTEX_FILE, 3 }, // PSLight clears VERTEX_FILE[0-2]
					{ NV4097_INVALIDATE_VERTEX_CACHE_FILE, 1 },
					{ NV4097_INVALIDATE_L2, 1 },
					{ NV4097_INVALIDATE_ZCULL, 1 }
				}};

				std::array<u8, 0x10000 / 4> register_properties{};

				for (const auto &method : ignorable_ranges)
				{
					for (u32 i = 0; i < method.second; ++i)
					{
						register_properties[method.first + i] |= register_props_always_ignore;
					}
				}

				return register_properties;
			}();

			u32 deferred_primitive = 0;
			u32 draw_count = 0;
			u32 begin_end_ctr = 0;

			bool enabled = false;
			u32  num_collapsed = 0;
			optimization_hint fifo_hint = unknown;

			void reset(bool _enabled);

		public:
			flattening_helper() = default;
			~flattening_helper() = default;

			u32 get_primitive() const { return deferred_primitive; }
			bool is_enabled() const { return enabled; }

			void force_disable();
			void evaluate_performance(u32 total_draw_count);
			inline flatten_op test(register_pair& command);
		};

		class FIFO_control
		{
		private:
			RsxDmaControl* m_ctrl = nullptr;
			const rsx::rsx_iomap_table* m_iotable;
			u32 m_internal_get = 0;

			u32 m_memwatch_addr = 0;
			u32 m_memwatch_cmp = 0;

			u32 m_command_reg = 0;
			u32 m_command_inc = 0;
			u32 m_remaining_commands = 0;
			u32 m_args_ptr = 0;
			u32 m_cmd = ~0u;

		public:
			FIFO_control(rsx::thread* pctrl);
			~FIFO_control() = default;

			u32 get_pos() const { return m_internal_get; }
			u32 last_cmd() const { return m_cmd; }
			void sync_get() { m_ctrl->get.release(m_internal_get); }
			u32 get_current_arg_ptr() const { return m_args_ptr; };
			u32 get_remaining_args_count() const { return m_remaining_commands; }
			void inc_get(bool wait);
			void set_get(u32 get);
			void abort();
			template <bool = true> u32 read_put();

			void read(register_pair& data);
			inline bool read_unsafe(register_pair& data);
			bool skip_methods(u32 count);
		};
	}
}