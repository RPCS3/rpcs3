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

		struct fifo_buffer_info_t
		{
			u32 start_loc;
			u32 length;
			u32 num_draw_calls;
			u32 reserved;
		};

		struct branch_target_info_t
		{
			u32 branch_target;
			u32 branch_origin;
			s64 weight;
			u64 checksum_16;
			u64 reserved;
		};

		struct optimization_pass
		{
			virtual void optimize(const fifo_buffer_info_t& info, simple_array<register_pair>& commands, const u32* registers) = 0;
		};

		struct flattening_pass : public optimization_pass
		{
		private:
			enum register_props : u8
			{
				skippable = 1,
				ignorable = 2
			};

			std::array<u8, 0x10000 / 4> m_register_properties;

		public:
			flattening_pass();
			void optimize(const fifo_buffer_info_t& info, simple_array<register_pair>& commands, const u32* registers) override;
		};

		struct reordering_pass : public optimization_pass
		{
		private:

			struct instruction_buffer_t
			{
				std::unordered_map<u32, u32> m_storage;
				simple_array<u32> m_insertion_order;

				instruction_buffer_t()
				{
					m_insertion_order.reserve(64);
				}

				void add_cmd(u32 reg, u32 value)
				{
					const auto is_new = std::get<1>(m_storage.insert_or_assign(reg, value));
					if (!is_new)
					{
						for (auto &loc : m_insertion_order)
						{
							if (loc == reg)
							{
								loc |= 0x80000000;
								break;
							}
						}
					}

					m_insertion_order.push_back(reg);
				}

				void clear()
				{
					m_storage.clear();
					m_insertion_order.clear();
				}

				void swap(instruction_buffer_t& other)
				{
					m_storage.swap(other.m_storage);
					m_insertion_order.swap(other.m_insertion_order);
				}

				auto size() const
				{
					return m_storage.size();
				}

				inline std::pair<u32, u32> get(int index) const
				{
					const auto key = m_insertion_order[index];
					if (key & 0x80000000)
					{
						// Disabled by a later write to the same register
						// TODO: Track command type registers and avoid this
						return { FIFO_DISABLED_COMMAND, 0 };
					}

					const auto value = m_storage.at(key);
					return { key, value };
				}

				bool operator == (const instruction_buffer_t& other) const
				{
					if (size() == other.size())
					{
						for (const auto &e : other.m_storage)
						{
							const auto found = m_storage.find(e.first);
							if (found == m_storage.end())
								return false;

							if (found->second != e.second)
								return false;
						}

						return true;
					}

					return false;
				}
			};

			struct draw_call
			{
				instruction_buffer_t prologue;
				std::vector<register_pair> draws;
				bool write_prologue;
				u32 primitive_type;
				const register_pair* start_pos;

				bool matches(const instruction_buffer_t setup, u32 prim) const
				{
					if (prim != primitive_type)
						return false;

					return prologue == setup;
				}
			};

			instruction_buffer_t registers_changed;
			std::vector<draw_call> bins;

			std::unordered_multimap<u32, fifo_buffer_info_t> m_results_prediction_table;

		public:
			void optimize(const fifo_buffer_info_t& info, simple_array<register_pair>& commands, const u32* registers) override;
		};

		class FIFO_control
		{
			RsxDmaControl* m_ctrl = nullptr;
			u32 m_internal_get = 0;

			std::shared_ptr<thread_base> m_prefetcher_thread;
			u32 m_prefetch_get = 0;
			atomic_t<bool> m_prefetcher_busy{ false };
			atomic_t<bool> m_fifo_busy{ false };
			fifo_buffer_info_t m_prefetcher_info;
			bool m_prefetcher_speculating;

			std::vector<std::unique_ptr<optimization_pass>> m_optimization_passes;

			simple_array<register_pair> m_queue;
			simple_array<register_pair> m_prefetched_queue;
			atomic_t<u32> m_command_index{ 0 };

			shared_mutex m_prefetch_mutex; // Guards prefetch queue
			shared_mutex m_queue_mutex; // Guards primary queue
			atomic_t<u64> m_ctrl_tag{ 0 }; // 'Guards' control registers

			register_pair empty_cmd { FIFO_EMPTY };
			register_pair busy_cmd { FIFO_BUSY };

			u32 m_memwatch_addr = 0;
			u32 m_memwatch_cmp = 0;

			fifo_buffer_info_t m_fifo_info;
			std::unordered_multimap<u32, branch_target_info_t> m_branch_prediction_table;

			void read_ahead(fifo_buffer_info_t& info, simple_array<register_pair>& commands, u32& get_pointer);
			void optimize(const fifo_buffer_info_t& info, simple_array<register_pair>& commands);
			void clear_buffer();

			u32 get_likely_target(u32 source);
			void report_branch_miss(u32 source, u32 target, u32 actual);
			void report_branch_hit(u32 source, u32 target);
			bool test_prefetcher_correctness(u32 actual_target);

		public:
			FIFO_control(rsx::thread* pctrl);
			~FIFO_control() {}

			void set_get(u32 get, bool spinning = false);
			void set_put(u32 put);

			const register_pair& read();
			inline const register_pair& read_unsafe();

			void register_optimization_pass(optimization_pass* pass);

			void finalize();

		public:
			static bool is_blocking_cmd(u32 cmd);
			static bool is_sync_cmd(u32 cmd);
		};
	}
}