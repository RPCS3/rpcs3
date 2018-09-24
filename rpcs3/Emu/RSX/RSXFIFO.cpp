#include "stdafx.h"

#include "RSXFIFO.h"
#include "RSXThread.h"
#include "Capture/rsx_capture.h"

extern rsx::frame_capture_data frame_capture;
//#pragma optimize("", off)
#define ENABLE_OPTIMIZATION_DEBUGGING 0

namespace rsx
{
	namespace FIFO
	{
		FIFO_control::FIFO_control(::rsx::thread* pctrl)
		{
			m_ctrl = pctrl->ctrl;
		}

		bool FIFO_control::is_blocking_cmd(u32 cmd)
		{
			switch (cmd)
			{
			case NV4097_WAIT_FOR_IDLE:
			case NV406E_SEMAPHORE_ACQUIRE:
			case NV406E_SEMAPHORE_RELEASE:
			case NV3089_IMAGE_IN:
			case NV0039_BUFFER_NOTIFY:
				return false;
			default:
				return true;
			}
		}

		bool FIFO_control::is_sync_cmd(u32 cmd)
		{
			switch (cmd)
			{
			case NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE:
			case NV4097_TEXTURE_READ_SEMAPHORE_RELEASE:
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
			m_queue.clear();
			m_command_index = 0;
		}

		void FIFO_control::read_ahead()
		{
			m_internal_get = m_ctrl->get;

			while (true)
			{
				const u32 get = m_ctrl->get;
				const u32 put = m_ctrl->put;

				if (get == put)
				{
					break;
				}

				// Validate put and get registers before reading the command
				// TODO: Who should handle graphics exceptions??
				u32 cmd;

				if (u32 addr = RSXIOMem.RealAddr(get))
				{
					cmd = vm::read32(addr);
				}
				else
				{
					// TODO: Optional recovery
					break;
				}

				if ((cmd & RSX_METHOD_OLD_JUMP_CMD_MASK) == RSX_METHOD_OLD_JUMP_CMD ||
					(cmd & RSX_METHOD_NEW_JUMP_CMD_MASK) == RSX_METHOD_NEW_JUMP_CMD ||
					(cmd & RSX_METHOD_CALL_CMD_MASK) == RSX_METHOD_CALL_CMD ||
					(cmd & RSX_METHOD_RETURN_MASK) == RSX_METHOD_RETURN_CMD)
				{
					// Flow control, stop read ahead
					m_queue.push_back({ cmd, 0, m_internal_get });
					break;
				}

				if ((cmd & RSX_METHOD_NOP_MASK) == RSX_METHOD_NOP_CMD)
				{
					if (m_queue.back().reg)
					{
						// Insert one NOP only
						m_queue.push_back({ cmd, 0, m_internal_get });
					}

					verify(HERE), m_ctrl->get == get;
					m_ctrl->get = m_internal_get = get + 4;
					continue;
				}

				if (cmd & 0x3)
				{
					// Malformed command, optional recovery
					break;
				}

				u32 count = (cmd >> 18) & 0x7ff;

				//Validate the args ptr if the command attempts to read from it
				auto args = vm::ptr<u32>::make(RSXIOMem.RealAddr(get + 4));

				if (!args && count)
				{
					// Optional recovery
					break;
				}

				// Stop command execution if put will be equal to get ptr during the execution itself
				if (count * 4 + 4 > put - get)
				{
					count = (put - get) / 4 - 1;
				}

				if (count > 1)
				{
					// Queue packet header
					m_queue.push_back({ FIFO_PACKET_BEGIN, count, m_internal_get });

					const bool no_increment = (cmd & RSX_METHOD_NON_INCREMENT_CMD_MASK) == RSX_METHOD_NON_INCREMENT_CMD;
					u32 reg = cmd & 0xfffc;
					m_internal_get += 4; // First executed command is at data[0]

					for (u32 i = 0; i < count; i++, m_internal_get += 4)
					{
						m_queue.push_back({ reg, args[i], m_internal_get });

						if (!no_increment) reg += 4;
					}
				}
				else
				{
					m_queue.push_back({ cmd & 0xfffc, args[0], m_internal_get });
					m_internal_get += 8;
				}

				verify(HERE), m_ctrl->get == get;
				m_ctrl->get = m_internal_get;
			}
		}

		void FIFO_control::optimize()
		{
			if (m_queue.empty())
			{
				// Nothing to do
				return;
			}

			for (auto &opt : m_optimization_passes)
			{
				opt->optimize(m_queue, rsx::method_registers.registers.data());
			}
		}

		void FIFO_control::set_put(u32 put)
		{
			if (m_ctrl->put == put)
			{
				return;
			}

			m_ctrl->put = put;
		}

		void FIFO_control::set_get(u32 get)
		{
			if (m_ctrl->get == get)
			{
				return;
			}

			clear_buffer();
			m_ctrl->get = get;
		}

		register_pair FIFO_control::read()
		{
			if (!m_queue.empty() && m_internal_get != m_ctrl->get)
			{
				// Control register changed
				clear_buffer();
			}

			if (m_command_index && m_command_index >= m_queue.size())
			{
				// Whole queue consumed
				verify(HERE), !m_queue.empty();
				clear_buffer();
			}

			if (m_queue.empty())
			{
				// Empty queue, read ahead
				read_ahead();
				optimize();
			}

			if (!m_queue.empty())
			{
				verify(HERE), m_command_index < m_queue.size();
				return m_queue[m_command_index++];
			}

			return { FIFO_EMPTY, 0 };
		}

		// Optimization passes
		void flattening_pass::optimize(std::vector<register_pair>& commands, const u32* registers) const
		{
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
				//LOG_ERROR(RSX, "[0x%x] %s(0x%x)", command.loc, _get_method_name(command.reg), command.value);

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
					const auto cmd = method_registers.current_draw_clause.command;
					if (cmd != rsx::draw_command::array && cmd != rsx::draw_command::none)
						break;

					flush_commands_flag = false;
					break;
				}
				case NV4097_DRAW_INDEX_ARRAY:
				{
					const auto cmd = method_registers.current_draw_clause.command;
					if (cmd != rsx::draw_command::indexed && cmd != rsx::draw_command::none)
						break;

					flush_commands_flag = false;
					break;
				}
				default:
				{
					// TODO: Reorder draw commands between synchronization events to maximize batched sizes
					static const std::pair<u32, u32> skippable_ranges[] =
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
						{ NV4097_SET_TRANSFORM_PROGRAM, 512 }
					};

					if (has_deferred_call)
					{
						// Hopefully this is skippable so the batch can keep growing
						for (const auto &method : skippable_ranges)
						{
							if (reg < method.first)
								continue;

							if (reg - method.first < method.second)
							{
								// Safe to ignore if value has not changed
								if (test_register(reg, value))
								{
									execute_method_flag = false;
									flush_commands_flag = false;
								}
								else
								{
									set_register(reg, value);
								}

								break;
							}
						}
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

		void reordering_pass::optimize(std::vector<register_pair>& commands, const u32* registers) const
		{
#if 0
			// Define a draw call
			struct texture_entry
			{
				u32 index = -1u;
				u32 address = 0;
				u32 filter = 0;
				u32 control0 = 0;
				u32 control1 = 0;
				u32 control2 = 0;
				u32 control3 = 0;
			};

			struct draw_call
			{
				std::vector<u32> instructions;
				std::array<texture_entry, 16> fragment_texture_state{};
				std::array<texture_entry, 4> vertex_texture_state{};
			};

			std::vector<draw_call> draw_calls;
#endif
		}
	}

	void thread::run_FIFO()
	{
		auto command = fifo_ctrl->read();
		const auto cmd = command.reg;

		if (cmd == FIFO::FIFO_EMPTY || !Emu.IsRunning())
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
			u32 offs = cmd & 0x1ffffffc;
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
			fifo_ctrl->set_get(offs);
			return;
		}
		if ((cmd & RSX_METHOD_NEW_JUMP_CMD_MASK) == RSX_METHOD_NEW_JUMP_CMD)
		{
			u32 offs = cmd & 0xfffffffc;
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
			fifo_ctrl->set_get(offs);
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

			u32 offs = cmd & 0xfffffffc;
			//LOG_WARNING(RSX, "rsx call(0x%x) #0x%x - 0x%x", offs, cmd, get);
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

			//LOG_WARNING(RSX, "rsx return(0x%x)", get);
			fifo_ctrl->set_get(m_return_addr);
			m_return_addr = -1;
			return;
		}
		if ((cmd & RSX_METHOD_NOP_MASK) == RSX_METHOD_NOP_CMD)
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
		if (cmd == FIFO::FIFO_PACKET_BEGIN)
		{
			count = command.value;
			command = fifo_ctrl->read();
		}

		for (u32 i = 0; i < count; ++i)
		{
			if (i) command = fifo_ctrl->read();

			const u32 reg = command.reg >> 2;
			const u32 value = command.value;

			if (capture_current_frame)
			{
				frame_debug.command_queue.push_back(std::make_pair(reg, value));

				if (!(reg == NV406E_SET_REFERENCE || reg == NV406E_SEMAPHORE_RELEASE || reg == NV406E_SEMAPHORE_ACQUIRE))
				{
					// todo: handle nv406e methods better?, do we care about call/jumps?
					rsx::frame_capture_data::replay_command replay_cmd;
					replay_cmd.rsx_command = std::make_pair(i == 0 ? cmd : 0, value);

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

			if (command.reg == FIFO::FIFO_DISABLED_COMMAND)
			{
				// Placeholder for dropped commands
				continue;
			}

			method_registers.decode(reg, value);

			if (auto method = methods[reg])
			{
				method(this, reg, value);
			}
		}
	}
}