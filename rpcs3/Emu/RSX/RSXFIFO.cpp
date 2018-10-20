#include "stdafx.h"

#include "RSXFIFO.h"
#include "RSXThread.h"
#include "Capture/rsx_capture.h"

extern rsx::frame_capture_data frame_capture;
extern bool user_asked_for_frame_capture;

#define ENABLE_OPTIMIZATION_DEBUGGING 0

namespace rsx
{
	namespace FIFO
	{
		template <int From, int To>
		struct scoped_priority
		{
			scoped_priority()
			{
				thread_ctrl::set_native_priority(To);
			}

			~scoped_priority()
			{
				thread_ctrl::set_native_priority(From);
			}
		};

		static inline void __prefetcher_sleep() { std::this_thread::sleep_for(100us); }
		static inline void __prefetcher_yield() { std::this_thread::yield(); }

		FIFO_control::FIFO_control(::rsx::thread* pctrl)
		{
			m_ctrl = pctrl->ctrl;
			m_queue.reserve(16384);
			m_prefetched_queue.reserve(16384);

			thread_ctrl::spawn(m_prefetcher_thread, "FIFO Prefetch Thread", [this]()
			{
				// TODO:
				return;

				if (g_cfg.core.thread_scheduler_enabled)
				{
					thread_ctrl::set_thread_affinity_mask(thread_ctrl::get_affinity_mask(thread_class::rsx));
				}

				u32 internal_get;
				u32 target_addr;

				while (!Emu.IsStopped())
				{
					target_addr = -1u;

					if (m_prefetched_queue.empty() && m_ctrl->put != m_ctrl->get)
					{
						// Get address to read ahead at/to
						const u64 control_tag = m_ctrl_tag;
						internal_get = m_internal_get;

						if (m_memwatch_addr)
						{
							// Spinning
							__prefetcher_sleep();
							continue;
						}
						else
						{
							// This is normal
							m_prefetch_get = m_ctrl->get;
							m_prefetcher_speculating = false;
						}

						// Check again
						if (control_tag != m_ctrl_tag)
						{
							// Race condition
							continue;
						}

						if (m_prefetch_get != -1u)
						{
							// Check for special conditions in the existing queue
							{
								std::lock_guard<shared_mutex> lock(m_queue_mutex);
								if (!m_queue.empty())
								{
									const auto cmd = m_queue.back().reg;

									if ((cmd >> 2) == NV406E_SEMAPHORE_ACQUIRE)
									{
										// Blocking command, cannot read ahead
										__prefetcher_sleep();
										continue;
									}

									if ((cmd & RSX_METHOD_OLD_JUMP_CMD_MASK) == RSX_METHOD_OLD_JUMP_CMD)
									{
										m_prefetch_get = cmd & 0x1ffffffc;
									}
									else if ((cmd & RSX_METHOD_NEW_JUMP_CMD_MASK) == RSX_METHOD_NEW_JUMP_CMD)
									{
										m_prefetch_get = cmd & 0xfffffffc;
									}
									else if ((cmd & RSX_METHOD_CALL_CMD_MASK) == RSX_METHOD_CALL_CMD)
									{
										m_prefetch_get = cmd & 0xfffffffc;
									}
									else if ((cmd & RSX_METHOD_RETURN_MASK) == RSX_METHOD_RETURN_CMD)
									{
										// Cannot determine RET address safely, cannot read ahead
										__prefetcher_sleep();
										continue;
									}
								}
							}

							scoped_priority<0, 1> priority;
							if (m_prefetch_mutex.try_lock())
							{
								if (control_tag != m_ctrl_tag)
								{
									// Do not stall with the prefetch mutex held!
									m_prefetch_mutex.unlock();
									continue;
								}

								m_prefetcher_busy.store(true);

								read_ahead(m_prefetcher_info, m_prefetched_queue, m_prefetch_get);
								optimize(m_prefetcher_info, m_prefetched_queue);

								m_prefetcher_busy.store(false);
								m_prefetch_mutex.unlock();
							}
						}
					}

					__prefetcher_sleep();
				}
			});
		}

		void FIFO_control::finalize()
		{
			if (m_prefetcher_thread)
			{
				m_prefetcher_thread->join();
				m_prefetcher_thread.reset();
			}
		}

		bool FIFO_control::is_blocking_cmd(u32 cmd)
		{
			switch (cmd)
			{
			// Sync
			case NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE:
			case NV4097_TEXTURE_READ_SEMAPHORE_RELEASE:
			case NV406E_SEMAPHORE_ACQUIRE:
			case NV406E_SEMAPHORE_RELEASE:
			case NV406E_SET_REFERENCE:

			// Data xfer
			case NV3089_IMAGE_IN:
			case NV0039_BUFFER_NOTIFY:
			case NV308A_COLOR:
				return true;
			default:
				return false;
			}
		}

		bool FIFO_control::is_sync_cmd(u32 cmd)
		{
			switch (cmd)
			{
			case NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE:
			case NV4097_TEXTURE_READ_SEMAPHORE_RELEASE:
			case NV406E_SEMAPHORE_ACQUIRE:
			case NV406E_SEMAPHORE_RELEASE:
			case NV406E_SET_REFERENCE:
				return true;
			default:
				return false;
			}
		}

		void FIFO_control::register_optimization_pass(optimization_pass* pass)
		{
			m_optimization_passes.emplace_back(pass);
		}

		void FIFO_control::clear_buffer()
		{
			std::lock_guard<shared_mutex> lock(m_queue_mutex);

			m_queue.clear();
			m_command_index = 0;
		}

		void FIFO_control::read_ahead(fifo_buffer_info_t& info, simple_array<register_pair> &commands, u32& get_pointer)
		{
			const u32 put = m_ctrl->put;

			info.start_loc = get_pointer;
			info.num_draw_calls = 0;

			u32 cmd;
			u32 count;

			while (true)
			{
				if (get_pointer == put)
				{
					// Nothing to do
					break;
				}

				// Validate put and get registers before reading the command
				// TODO: Who should handle graphics exceptions??
				if (u32 addr = RSXIOMem.RealAddr(get_pointer))
				{
					cmd = vm::read32(addr);
				}
				else
				{
					// TODO: Optional recovery
					break;
				}

				if (UNLIKELY(cmd & 0xe0030003))
				{
					if ((cmd & RSX_METHOD_OLD_JUMP_CMD_MASK) == RSX_METHOD_OLD_JUMP_CMD ||
						(cmd & RSX_METHOD_NEW_JUMP_CMD_MASK) == RSX_METHOD_NEW_JUMP_CMD ||
						(cmd & RSX_METHOD_CALL_CMD_MASK) == RSX_METHOD_CALL_CMD ||
						(cmd & RSX_METHOD_RETURN_MASK) == RSX_METHOD_RETURN_CMD)
					{
						// Flow control, stop read ahead
						commands.push_back({ cmd, 0, get_pointer });
						break;
					}
				}
				else if (UNLIKELY((cmd & RSX_METHOD_NOP_MASK) == RSX_METHOD_NOP_CMD))
				{
					if (commands.empty() || commands.back().reg != RSX_METHOD_NOP_CMD)
					{
						// Insert one NOP only
						commands.push_back({ RSX_METHOD_NOP_CMD, 0, get_pointer });
					}

					get_pointer += 4;
					continue;
				}
				else if (UNLIKELY(cmd & 0x3))
				{
					// Malformed command, optional recovery
					break;
				}

				//Validate the args ptr if the command attempts to read from it
				auto args = vm::ptr<u32>::make(RSXIOMem.RealAddr(get_pointer + 4));
				if (UNLIKELY(!args))
				{
					// Optional recovery
					break;
				}

				count = (cmd >> 18) & 0x7ff;
				if (count > 1)
				{
					// Stop command execution if put will be equal to get ptr during the execution itself
					if (UNLIKELY(count * 4 + 4 > put - get_pointer))
					{
						count = (put - get_pointer) / 4 - 1;
					}

					// Queue packet header
					commands.push_back({ FIFO_PACKET_BEGIN, count, get_pointer });

					// First executed command is at data[0]
					get_pointer += 4;

					if (UNLIKELY((cmd & RSX_METHOD_NON_INCREMENT_CMD_MASK) == RSX_METHOD_NON_INCREMENT_CMD))
					{
						const u32 reg = cmd & 0xfffc;
						for (u32 i = 0; i < count; i++, get_pointer += 4)
						{
							commands.push_back({ reg, args[i], get_pointer });
						}
					}
					else
					{
						u32 reg = cmd & 0xfffc;
						for (u32 i = 0; i < count; i++, get_pointer += 4, reg += 4)
						{
							commands.push_back({ reg, args[i], get_pointer });
						}
					}
				}
				else
				{
					const u32 reg = cmd & 0xfffc;
					commands.push_back({ reg, args[0], get_pointer });
					get_pointer += 8;

					if (reg == (NV4097_SET_BEGIN_END << 2))
					{
						info.num_draw_calls++;
					}
					else if (reg == (NV406E_SEMAPHORE_ACQUIRE << 2))
					{
						// Hard sync, stop read ahead
						break;
					}
				}
			}

			info.length = get_pointer - info.start_loc;
			if (info.num_draw_calls < 2)
			{
				return;
			}

			info.num_draw_calls /= 2; // Begin+End pairs
		}

		void FIFO_control::report_branch_hit(u32 source, u32 target)
		{
			const auto range = m_branch_prediction_table.equal_range(source);
			for (auto It = range.first; It != range.second; It++)
			{
				if (It->second.branch_target == target)
				{
					It->second.weight++;
					return;
				}
			}

			fmt::throw_exception("Unreachable" HERE);
		}

		void FIFO_control::report_branch_miss(u32 source, u32 target, u32 actual)
		{
			const auto range = m_branch_prediction_table.equal_range(source);
			for (auto It = range.first; It != range.second; It++)
			{
				if (target < -1u && It->second.branch_target == target)
				{
					It->second.weight--;
					target = -1u;

					if (actual == -1u)
						return;
				}
				else if (actual < -1u && It->second.branch_target == actual)
				{
					It->second.weight++;
					actual = -1u;

					if (target == -1u)
						return;
				}
			}

			if (target != -1u)
			{
				branch_target_info_t info;
				info.branch_origin = source;
				info.branch_target = target;
				info.checksum_16 = 0;
				info.weight = 0;

				m_branch_prediction_table.emplace(source, info);
			}

			if (actual != -1u)
			{
				branch_target_info_t info;
				info.branch_origin = source;
				info.branch_target = actual;
				info.checksum_16 = 0;
				info.weight = 1;

				m_branch_prediction_table.emplace(source, info);
			}
		}

		u32 FIFO_control::get_likely_target(u32 source)
		{
			s64 weight = 0;
			u32 target = -1u;

			const auto range = m_branch_prediction_table.equal_range(source);
			for (auto It = range.first; It != range.second; It++)
			{
				if (It->second.weight > weight)
				{
					target = It->second.branch_target;
				}
			}

			return target;
		}

		void FIFO_control::optimize(const fifo_buffer_info_t& info, simple_array<register_pair>& commands)
		{
			if (commands.empty() || user_asked_for_frame_capture || g_cfg.video.disable_FIFO_reordering)
			{
				// Nothing to do
				return;
			}

			for (auto &opt : m_optimization_passes)
			{
				opt->optimize(info, commands, rsx::method_registers.registers.data());
			}
		}

		bool FIFO_control::test_prefetcher_correctness(u32 target)
		{
			m_fifo_busy.store(true);

			if (!m_prefetched_queue.empty())
			{
				const u32 guessed_target = m_prefetcher_info.start_loc;
				bool result = true;

				if (guessed_target != m_ctrl->get)
				{
					const u32 ctrl_get = m_ctrl->get;
					LOG_ERROR(RSX, "fifo::Prefetcher was seemingly wrong!, guessed=0x%x, get=0x%x",
						guessed_target, ctrl_get);
//					report_branch_miss(m_internal_get, guessed_target, get);

					// Kick
//					m_ctrl->get = get;
					m_prefetched_queue.clear();
					result = false;
				}
				else
				{
					// Nothing to do, guessed correctly
//					report_branch_hit(m_internal_get, guessed_target);
				}

				m_fifo_busy.store(false);
				return result;
			}

			m_fifo_busy.store(false);
			return false;
		}

		void FIFO_control::set_put(u32 put)
		{
			if (m_ctrl->put == put)
			{
				return;
			}

			m_ctrl->put = put;
		}

		void FIFO_control::set_get(u32 get, bool spinning)
		{
			if (m_ctrl->get == get)
			{
				if (spinning)
				{
					verify(HERE), !m_queue.empty();

					const auto& last_cmd = m_queue.back();
					m_memwatch_addr = get;
					m_memwatch_cmp = last_cmd.reg;

					m_ctrl_tag++;
				}

				return;
			}

			// Update ctrl registers
			m_ctrl->get = get;
			m_internal_get = get;

			// Clear memwatch spinner
			m_memwatch_addr = 0;

			// Update control tag
			m_ctrl_tag++;

			// NOTE: This will 'free' the prefetcher in case it was stopped by a sync command
			clear_buffer();
		}

		const register_pair& FIFO_control::read_unsafe()
		{
			// Fast read with no processing, only safe inside a PACKET_BEGIN+count block
			AUDIT(m_command_index < m_queue.size());
			return m_queue[m_command_index++];
		}

		const register_pair& FIFO_control::read()
		{
			bool registers_changed = false;
			const auto queue_size = m_queue.size();

			if (queue_size > 0)
			{
				if (UNLIKELY(m_internal_get != m_ctrl->get))
				{
					// Control register changed
					registers_changed = true;
					clear_buffer();
				}
				else if (m_command_index >= queue_size)
				{
					// Consumed whole queue previously
					clear_buffer();
				}
				else
				{
					const auto& inst = m_queue[m_command_index++];
					if (inst.reg == FIFO_DISABLED_COMMAND)
					{
						// Jump to the first safe command
						for (u32 n = m_command_index; n < m_queue.size(); ++n)
						{
							const auto& _inst = m_queue[n];
							if (_inst.reg != FIFO_DISABLED_COMMAND)
							{
								m_command_index = ++n;
								return _inst;
							}
						}

						// Whole remainder is just disabled commands
						clear_buffer();
					}
					else
					{
						// Command is 'ok'
						return inst;
					}
				}
			}

			//verify(HERE), m_queue.empty();

			if (m_ctrl->put == m_ctrl->get)
			{
				// Nothing to do
				return empty_cmd;
			}

			if (m_memwatch_addr)
			{
				if (m_ctrl->get == m_memwatch_addr)
				{
					if (const auto addr = RSXIOMem.RealAddr(m_memwatch_addr))
					{
						if (vm::read32(addr) == m_memwatch_cmp)
						{
							// Still spinning in place
							return empty_cmd;
						}
					}
				}

				m_memwatch_addr = 0;
				m_memwatch_cmp = 0;
				m_ctrl_tag++;
			}

			// Lock to disable the prefetcher
			if (0)//!m_prefetch_mutex.try_lock())
			{
				return busy_cmd;
			}

			if (UNLIKELY(registers_changed))
			{
				if (!m_prefetched_queue.empty())
				{
					if (m_prefetcher_info.start_loc != m_ctrl->get)
					{
						// Guessed wrong, discard results
						m_prefetched_queue.clear();
					}
				}
			}

			if (!m_prefetched_queue.empty())
			{
				m_ctrl->get = m_internal_get = m_prefetch_get;
				m_ctrl_tag++;

				m_queue.swap(m_prefetched_queue);
			}
			else
			{
				m_internal_get = m_ctrl->get;
				read_ahead(m_fifo_info, m_queue, m_internal_get);
				optimize(m_fifo_info, m_queue);

				m_ctrl->get = m_internal_get;
				m_ctrl_tag++;
			}

			//m_prefetch_mutex.unlock();

			if (!m_queue.empty())
			{
				// A few guarantees here..
				// First command is not really skippable even if useless
				// Queue size is at least 1
				return m_queue[m_command_index++];
			}

			return empty_cmd;
		}

		// Optimization passes
		flattening_pass::flattening_pass()
		{
			const std::pair<u32, u32> skippable_ranges[] =
			{
				// Texture configuration
				{ NV4097_SET_TEXTURE_OFFSET, 8 * 16 },
				{ NV4097_SET_TEXTURE_CONTROL2, 16 },
				{ NV4097_SET_TEXTURE_CONTROL3, 16 },
				{ NV4097_SET_VERTEX_TEXTURE_OFFSET, 8 * 4 },
				// Surface configuration
				{ NV4097_SET_SURFACE_CLIP_HORIZONTAL, 1 },
				{ NV4097_SET_SURFACE_CLIP_VERTICAL, 1 },
				{ NV4097_SET_SURFACE_COLOR_AOFFSET, 1 },
				{ NV4097_SET_SURFACE_COLOR_BOFFSET, 1 },
				{ NV4097_SET_SURFACE_COLOR_COFFSET, 1 },
				{ NV4097_SET_SURFACE_COLOR_DOFFSET, 1 },
				{ NV4097_SET_SURFACE_ZETA_OFFSET, 1 },
				{ NV4097_SET_CONTEXT_DMA_COLOR_A, 1 },
				{ NV4097_SET_CONTEXT_DMA_COLOR_B, 1 },
				{ NV4097_SET_CONTEXT_DMA_COLOR_C, 1 },
				{ NV4097_SET_CONTEXT_DMA_COLOR_D, 1 },
				{ NV4097_SET_CONTEXT_DMA_ZETA, 1 },
				{ NV4097_SET_SURFACE_FORMAT, 1 },
				{ NV4097_SET_SURFACE_PITCH_A, 1 },
				{ NV4097_SET_SURFACE_PITCH_B, 1 },
				{ NV4097_SET_SURFACE_PITCH_C, 1 },
				{ NV4097_SET_SURFACE_PITCH_D, 1 },
				{ NV4097_SET_SURFACE_PITCH_Z, 1 },
				// Program configuration
				{ NV4097_SET_TRANSFORM_PROGRAM_START, 1 },
				{ NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK, 1 },
				{ NV4097_SET_TRANSFORM_PROGRAM, 512 },
				// Vertex
				{ NV4097_SET_VERTEX_DATA_ARRAY_FORMAT, 16 },
				{ NV4097_SET_VERTEX_DATA_ARRAY_OFFSET, 16 },
			};

			const std::pair<u32, u32> ignorable_ranges[] =
			{
				// General
				{ NV4097_INVALIDATE_VERTEX_FILE, 3 }, // PSLight clears VERTEX_FILE[0-2]
				{ NV4097_INVALIDATE_VERTEX_CACHE_FILE, 1 },
				{ NV4097_INVALIDATE_L2, 1 },
				{ NV4097_INVALIDATE_ZCULL, 1 },
				// FIFO
				{ (FIFO_DISABLED_COMMAND >> 2), 1},
				{ (FIFO_PACKET_BEGIN >> 2), 1 },
				{ (FIFO_DRAW_BARRIER >> 2), 1 },
				// ROP
				{ NV4097_SET_ALPHA_FUNC, 1 },
				{ NV4097_SET_ALPHA_REF, 1 },
				{ NV4097_SET_ALPHA_TEST_ENABLE, 1 },
				{ NV4097_SET_ANTI_ALIASING_CONTROL, 1 },
				// Program
				{ NV4097_SET_SHADER_PACKER, 1 },
				{ NV4097_SET_SHADER_WINDOW, 1 },
				// Vertex data offsets
				{ NV4097_SET_VERTEX_DATA_BASE_OFFSET, 1 },
				{ NV4097_SET_VERTEX_DATA_BASE_INDEX, 1 }
			};

			std::fill(m_register_properties.begin(), m_register_properties.end(), 0u);

			for (const auto &method : skippable_ranges)
			{
				for (int i = 0; i < method.second; ++i)
				{
					m_register_properties[method.first + i] = register_props::skippable;
				}
			}

			for (const auto &method : ignorable_ranges)
			{
				for (int i = 0; i < method.second; ++i)
				{
					m_register_properties[method.first + i] |= register_props::ignorable;
				}
			}
		}

		void flattening_pass::optimize(const fifo_buffer_info_t& info, simple_array<register_pair>& commands, const u32* registers)
		{
			if (info.num_draw_calls < 20)
			{
				// Not enough draw calls
				return;
			}

#if (ENABLE_OPTIMIZATION_DEBUGGING)
			auto copy = commands;
#endif
			// Removes commands that have no effect on the pipeline

			register_pair* last_begin = nullptr;
			register_pair* last_end = nullptr;

			u32 deferred_primitive_type = UINT32_MAX;
			bool has_deferred_call = false;

			std::unordered_map<u32, u32> register_tracker; // Tracks future register writes
			auto test_register = [&](u32 reg, u32 value)
			{
				u32 test;
				auto found = register_tracker.find(reg);
				if (found == register_tracker.end())
				{
					test = registers[reg];
				}
				else
				{
					test = found->second;
				}

				return (value == test);
			};

			auto set_register = [&](u32 reg, u32 value)
			{
				register_tracker[reg] = value;
			};

			auto patch_draw_calls = [&]()
			{
				if (last_end)
				{
					// Restore scope end
					last_end->reg = (NV4097_SET_BEGIN_END << 2);
				}

				if (last_begin > last_end)
				{
					// Dangling clause, restore scope open
					last_begin->reg = (NV4097_SET_BEGIN_END << 2);
				}
			};

			for (auto &command : commands)
			{
				bool flush_commands_flag = has_deferred_call;
				bool execute_method_flag = true;

				const auto reg = command.reg >> 2;
				const auto value = command.value;

				switch (reg)
				{
				case NV4097_SET_BEGIN_END:
				{
					if (value && value != deferred_primitive_type)
					{
						// Begin call with different primitive type
						deferred_primitive_type = value;
					}
					else
					{
						// This is either an End call or another Begin with the same primitive type
						has_deferred_call = true;
						flush_commands_flag = false;
						execute_method_flag = false;
					}
					break;
				}
				case NV4097_DRAW_ARRAYS:
				{
					if (has_deferred_call)
					{
						const auto cmd = method_registers.current_draw_clause.command;
						if (cmd != rsx::draw_command::array && cmd != rsx::draw_command::none)
							break;

						flush_commands_flag = false;
					}
					break;
				}
				case NV4097_DRAW_INDEX_ARRAY:
				{
					if (has_deferred_call)
					{
						const auto cmd = method_registers.current_draw_clause.command;
						if (cmd != rsx::draw_command::indexed && cmd != rsx::draw_command::none)
							break;

						flush_commands_flag = false;
					}
					break;
				}
				default:
				{
					if (reg >= m_register_properties.size())
					{
						// Flow control or special command
						break;
					}

					const auto properties = m_register_properties[reg];
					if (properties & register_props::ignorable)
					{
						// These have no effect on rendering behavior or can be handled within begin/end
						flush_commands_flag = false;
						break;
					}

					if (properties & register_props::skippable)
					{
						if (has_deferred_call)
						{
							// Safe to ignore if value has not changed
							if (test_register(reg, value))
							{
								execute_method_flag = false;
								flush_commands_flag = false;
								break;
							}
						}

						set_register(reg, value);
					}

					break;
				}
				}

				if (!execute_method_flag)
				{
					command.reg = FIFO_DISABLED_COMMAND;

					if (reg == NV4097_SET_BEGIN_END)
					{
						if (command.value)
						{
							last_begin = &command;
						}
						else
						{
							last_end = &command;
						}
					}
				}

				if (flush_commands_flag)
				{
					has_deferred_call = false;
					deferred_primitive_type = UINT32_MAX;

					patch_draw_calls();
				}
			}

			if (has_deferred_call)
			{
				verify(HERE), deferred_primitive_type != UINT32_MAX;
				patch_draw_calls();
			}

#if (ENABLE_OPTIMIZATION_DEBUGGING)

			bool mismatch = false;
			for (int n = 0; n < commands.size(); ++n)
			{
				auto command = commands[n];
				auto old = copy[n];

				if (command.reg != old.reg)
				{
					if (old.reg == (NV4097_SET_BEGIN_END << 2) && old.value)
					{
						mismatch = true;
						break;
					}
				}
			}

			if (!mismatch)
			{
				return;
			}

			auto _get_method_name = [&](u32 reg) -> std::string
			{
				if (reg == FIFO_DISABLED_COMMAND)
				{
					return "COMMAND DISABLED";
				}

				if (reg == FIFO_PACKET_BEGIN)
				{
					return "PACKET BEGIN";
				}

				return rsx::get_method_name(reg >> 2);
			};

			LOG_ERROR(RSX, "------------------- DUMP BEGINS--------------------");
			for (int n = 0; n < commands.size(); ++n)
			{
				auto command = commands[n];
				auto old = copy[n];

				if (old.reg != command.reg || command.value != command.value)
				{
					LOG_ERROR(RSX, "[0x%x] %s(0x%x) -> %s(0x%x)", command.loc, _get_method_name(old.reg), old.value, _get_method_name(command.reg), command.value);
				}
				else
				{
					LOG_ERROR(RSX, "[0x%x] %s(0x%x)", command.loc, _get_method_name(old.reg), old.value);
				}
			}
			LOG_ERROR(RSX, "------------------- DUMP ENDS--------------------");
#endif
		}

		void reordering_pass::optimize(const fifo_buffer_info_t& info, simple_array<register_pair>& commands, const u32* registers)
		{
			if (info.num_draw_calls < 8)
			{
				// TODO: Better threshold checking
				return;
			}

			std::unordered_map<u32, u32> register_tracker; // Tracks future register writes
			auto get_register = [&](u32 reg)
			{
				auto found = register_tracker.find(reg);
				if (found == register_tracker.end())
				{
					return registers[reg];
				}
				else
				{
					return found->second;
				}
			};

			auto set_register = [&](u32 reg, u32 value)
			{
				register_tracker[reg] = value;
			};

			bool recording_changes = false;
			bool writing_draw_call = false;
			bool has_merged = false;
			u32  num_draws_processed = 0;
			u32  num_draws_merged = 0;

			draw_call *target_bin = nullptr;
			const register_pair *rollback_pos = nullptr;

			auto flush_commands = [&](const register_pair* end_pos) mutable
			{
				if (has_merged)
				{
					register_pair* mem_ptr = const_cast<register_pair*>(bins.front().start_pos);
					for (const auto& draw : bins)
					{
						if (draw.write_prologue)
						{
							for (u32 n = 0; n < draw.prologue.size(); ++n)
							{
								const auto e = draw.prologue.get(n);
								mem_ptr->reg = e.first;
								mem_ptr->value = e.second;
								mem_ptr++;
							}
						}

						mem_ptr->reg = (NV4097_SET_BEGIN_END << 2);
						mem_ptr->value = draw.primitive_type;
						mem_ptr++;

						for (const auto &inst : draw.draws)
						{
							*mem_ptr = inst;
							mem_ptr++;
						}

						mem_ptr->reg = (NV4097_SET_BEGIN_END << 2);
						mem_ptr->value = 0;
						mem_ptr++;
					}

					verify(HERE), mem_ptr <= end_pos;

					for (; mem_ptr <= end_pos; mem_ptr++)
					{
						mem_ptr->reg = FIFO_DISABLED_COMMAND;
					}
				}

				bins.clear();
				has_merged = false;
			};

			auto allowed = [](u32 reg)
			{
				if (reg & ~0xfffc)
					return false;

				if (FIFO_control::is_blocking_cmd(reg >> 2))
					return false;

				return true;
			};

#if (ENABLE_OPTIMIZATION_DEBUGGING)

			auto _get_method_name = [&](u32 reg) -> std::string
			{
				if (reg == FIFO_DISABLED_COMMAND)
				{
					return "COMMAND DISABLED";
				}

				if (reg == FIFO_PACKET_BEGIN)
				{
					return "PACKET BEGIN";
				}

				return rsx::get_method_name(reg >> 2);
			};

			if (user_asked_for_frame_capture)
			{
				LOG_ERROR(RSX, "-----------------PRE DUMP BEGINS--------------------");
				for (const auto& command : commands)
				{
					LOG_ERROR(RSX, "[0x%x] %s(0x%x)", command.loc, _get_method_name(command.reg), command.value);
				}
				LOG_ERROR(RSX, "------------------- DUMP ENDS--------------------");
			}
#endif

			for (const auto& command : commands)
			{
				bool write = false;
				switch (const u32 reg = (command.reg >> 2))
				{
				case NV4097_INVALIDATE_VERTEX_FILE: // PSLight clears VERTEX_FILE[0-2]
				case NV4097_PIPE_NOP:
				case NV4097_INVALIDATE_VERTEX_FILE + 2:
				case NV4097_INVALIDATE_VERTEX_CACHE_FILE:
				case NV4097_INVALIDATE_L2:
				case NV4097_INVALIDATE_ZCULL:
				case (FIFO_DISABLED_COMMAND >> 2):
				case (FIFO_PACKET_BEGIN >> 2):
				case (FIFO_DRAW_BARRIER >> 2):
				case (FIFO_EMPTY >> 2):
				case (FIFO_BUSY >> 2):				
				{
					break;
				}
				case NV4097_SET_BEGIN_END:
				{
					if (!command.value)
					{
						target_bin = nullptr;
						recording_changes = true;
						writing_draw_call = false;
						rollback_pos = &command;
					}
					else
					{
						if (bins.empty())
						{
							registers_changed.clear();
							target_bin = &bins.emplace_back();
							target_bin->write_prologue = false;
							target_bin->start_pos = &command;
							target_bin->primitive_type = command.value;
						}
						else
						{
							target_bin = nullptr;

							for (auto& draw : bins)
							{
								if (draw.matches(registers_changed, command.value))
								{
									num_draws_merged++;
									has_merged = true;
									target_bin = &draw;
									//target_bin->draws.push_back({ FIFO_DRAW_BARRIER << 2 });
									break;
								}
							}

							if (!target_bin)
							{
								target_bin = &bins.emplace_back();
								target_bin->write_prologue = true;
								target_bin->prologue.swap(registers_changed);
								target_bin->start_pos = &command;
								target_bin->primitive_type = command.value;
							}
						}

						recording_changes = false;
						writing_draw_call = true;
						num_draws_processed++;
					}

					break;
				}
				default:
				{
					write = true;

					if (bins.empty())
					{
						break;
					}

					if (recording_changes)
					{
						// Stop if any of the following conditions is met
						// The draw 'bin' changes more than 16 instructions (scanning performance)
						// The number of unique bins is greater than 4 making it non-trivial and likely not worthwhile to scan

						if (!allowed(command.reg))
						{
							// TODO: Maintain list of mergable commands
							target_bin = nullptr;

							if (recording_changes)
							{
								recording_changes = false;
								registers_changed.clear();
							}

							flush_commands(rollback_pos);
							break;
						}

						if (bins.size() == 1)
						{
							bins[0].prologue.add_cmd(command.reg, get_register(reg));
						}

						registers_changed.add_cmd(command.reg, command.value);
					}
					else if (writing_draw_call)
					{
						target_bin->draws.push_back(command);
					}

					break;
				}
				}

				if (write)
				{
					set_register(command.reg >> 2, command.value);
				}
			}

			flush_commands(rollback_pos);

			if (num_draws_merged)
			{
				LOG_ERROR(RSX, "Merges happened: Draws before: %d, draws merged %d", info.num_draw_calls, num_draws_merged);
			}

#if (ENABLE_OPTIMIZATION_DEBUGGING)
			if (user_asked_for_frame_capture)
			{
				LOG_ERROR(RSX, "----------------POST DUMP BEGINS--------------------");
				for (const auto& command : commands)
				{
					LOG_ERROR(RSX, "[0x%x] %s(0x%x)", command.loc, _get_method_name(command.reg), command.value);
				}
				LOG_ERROR(RSX, "------------------- DUMP ENDS--------------------");
			}
#endif
		}
	}

	void thread::run_FIFO()
	{
		const auto& command = fifo_ctrl->read();
		const auto cmd = command.reg;

		if (cmd == FIFO::FIFO_BUSY)
		{
			// Do something else
			return;
		}

		if (cmd == FIFO::FIFO_EMPTY)
		{
			if (performance_counters.state == FIFO_state::running)
			{
				performance_counters.FIFO_idle_timestamp = get_system_time();
				performance_counters.state = FIFO_state::empty;
			}

			return;
		}

		// Validate put and get registers before reading the command
		// TODO: Who should handle graphics exceptions??
		if ((cmd & RSX_METHOD_OLD_JUMP_CMD_MASK) == RSX_METHOD_OLD_JUMP_CMD)
		{
			const u32 offs = cmd & 0x1ffffffc;
			if (offs == command.loc)
			{
				//Jump to self. Often preceded by NOP
				if (performance_counters.state == FIFO_state::running)
				{
					performance_counters.FIFO_idle_timestamp = get_system_time();
				}

				performance_counters.state = FIFO_state::spinning;
			}

			//LOG_WARNING(RSX, "rsx jump(0x%x) #addr=0x%x, cmd=0x%x, get=0x%x, put=0x%x", offs, m_ioAddress + get, cmd, get, put);
			fifo_ctrl->set_get(offs, offs == command.loc);
			return;
		}
		if ((cmd & RSX_METHOD_NEW_JUMP_CMD_MASK) == RSX_METHOD_NEW_JUMP_CMD)
		{
			const u32 offs = cmd & 0xfffffffc;
			if (offs == command.loc)
			{
				//Jump to self. Often preceded by NOP
				if (performance_counters.state == FIFO_state::running)
				{
					performance_counters.FIFO_idle_timestamp = get_system_time();
				}

				performance_counters.state = FIFO_state::spinning;
			}

			//LOG_WARNING(RSX, "rsx jump(0x%x) #addr=0x%x, cmd=0x%x, get=0x%x, put=0x%x", offs, m_ioAddress + get, cmd, get, put);
			fifo_ctrl->set_get(offs, offs == command.loc);
			return;
		}
		if ((cmd & RSX_METHOD_CALL_CMD_MASK) == RSX_METHOD_CALL_CMD)
		{
			if (m_return_addr != -1)
			{
				// Only one layer is allowed in the call stack.
				LOG_ERROR(RSX, "FIFO: CALL found inside a subroutine. Discarding subroutine");
				fifo_ctrl->set_get(std::exchange(m_return_addr, -1));
				return;
			}

			const u32 offs = cmd & 0xfffffffc;
			m_return_addr = command.loc + 4;
			fifo_ctrl->set_get(offs);
			return;
		}
		if ((cmd & RSX_METHOD_RETURN_MASK) == RSX_METHOD_RETURN_CMD)
		{
			if (m_return_addr == -1)
			{
				LOG_ERROR(RSX, "FIFO: RET found without corresponding CALL. Discarding queue");
				fifo_ctrl->set_get(ctrl->put);
				return;
			}

			fifo_ctrl->set_get(m_return_addr);
			m_return_addr = -1;
			return;
		}
		if (cmd == RSX_METHOD_NOP_CMD)
		{
			if (performance_counters.state == FIFO_state::running)
			{
				performance_counters.FIFO_idle_timestamp = get_system_time();
				performance_counters.state = FIFO_state::nop;
			}

			return;
		}
		if (cmd & 0x3)
		{
			// TODO: Check for more invalid bits combinations
			LOG_ERROR(RSX, "FIFO: Illegal command(0x%x) was executed. Resetting...", cmd);
			fifo_ctrl->set_get(restore_point.load());
			m_return_addr = restore_ret_addr;
			return;
		}

		if (performance_counters.state != FIFO_state::running)
		{
			//Update performance counters with time spent in idle mode
			performance_counters.idle_time += (get_system_time() - performance_counters.FIFO_idle_timestamp);

			if (performance_counters.state == FIFO_state::spinning)
			{
				//TODO: Properly simulate FIFO wake delay.
				//NOTE: The typical spin setup is a NOP followed by a jump-to-self
				//NOTE: There is a small delay when the jump address is dynamically edited by cell
				busy_wait(3000);
			}

			performance_counters.state = FIFO_state::running;
		}

		u32 count = 1;
		auto *command_ptr = &command;

		if (cmd == FIFO::FIFO_PACKET_BEGIN)
		{
			count = command.value;
			command_ptr = &fifo_ctrl->read_unsafe();
		}

		for (u32 i = 0; i < count; ++i)
		{
			if (i) command_ptr = &fifo_ctrl->read_unsafe();

			if (command_ptr->reg == FIFO::FIFO_DISABLED_COMMAND)
			{
				// Placeholder for dropped commands
				continue;
			}

			const u32 reg = command_ptr->reg >> 2;
			const u32& value = command_ptr->value;

			if (capture_current_frame)
			{
				frame_debug.command_queue.push_back(std::make_pair(reg, value));

				if (!(reg == NV406E_SET_REFERENCE || reg == NV406E_SEMAPHORE_RELEASE || reg == NV406E_SEMAPHORE_ACQUIRE))
				{
					// todo: handle nv406e methods better?, do we care about call/jumps?
					rsx::frame_capture_data::replay_command replay_cmd;
					replay_cmd.rsx_command = std::make_pair(i == 0 ? command_ptr->reg : 0, value);

					frame_capture.replay_commands.push_back(replay_cmd);

					// to make this easier, use the replay command 'i' positions back
					auto it = std::prev(frame_capture.replay_commands.end(), i + 1);

					switch (reg)
					{
					case NV4097_GET_REPORT:
						capture::capture_get_report(this, *it, value);
						break;
					case NV3089_IMAGE_IN:
						capture::capture_image_in(this, *it);
						break;
					case NV0039_BUFFER_NOTIFY:
						capture::capture_buffer_notify(this, *it);
						break;
					case NV4097_CLEAR_SURFACE:
						capture::capture_surface_state(this, *it);
						break;
					default:
						if (reg >= NV308A_COLOR && reg < NV3089_SET_OBJECT)
							capture::capture_inline_transfer(this, *it, reg - NV308A_COLOR, value);
						break;
					}
				}
			}

			method_registers.decode(reg, value);

			if (auto method = methods[reg])
			{
				method(this, reg, value);
			}
		}
	}
}