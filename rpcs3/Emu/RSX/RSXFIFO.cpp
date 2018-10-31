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
		FIFO_control::FIFO_control(::rsx::thread* pctrl)
		{
			m_ctrl = pctrl->ctrl;

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
					m_register_properties[method.first + i] = register_props::skip_on_match;
				}
			}

			for (const auto &method : ignorable_ranges)
			{
				for (int i = 0; i < method.second; ++i)
				{
					m_register_properties[method.first + i] |= register_props::always_ignore;
				}
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

		void FIFO_control::set_get(u32 get, bool spinning)
		{
			if (m_ctrl->get == get)
			{
				if (spinning)
				{
					if (const auto addr = RSXIOMem.RealAddr(m_memwatch_addr))
					{
						m_memwatch_addr = get;
						m_memwatch_cmp = vm::read32(addr);
					}
				}
				else
				{
					LOG_ERROR(RSX, "Undetected spinning?");
				}

				return;
			}

			// Update ctrl registers
			m_ctrl->get = get;
			m_internal_get = get;

			// Clear memwatch spinner
			m_memwatch_addr = 0;
		}

		bool FIFO_control::has_next() const
		{
			return (m_remaining_commands > 0);
		}

		void FIFO_control::read_unsafe(register_pair& data)
		{
			// Fast read with no processing, only safe inside a PACKET_BEGIN+count block
			//verify(HERE), m_remaining_commands;

			m_command_reg += m_command_inc;
			m_args_ptr += 4;
			m_remaining_commands--;

			data.reg = m_command_reg;
			data.value = vm::read32(m_args_ptr);
		}

		void FIFO_control::read(register_pair& data)
		{
			const u32 put = m_ctrl->put;
			m_internal_get = m_ctrl->get;

			if (put == m_internal_get)
			{
				// Nothing to do
				data.reg = FIFO_EMPTY;
				return;
			}

			if (m_memwatch_addr)
			{
				if (m_internal_get == m_memwatch_addr)
				{
					if (const auto addr = RSXIOMem.RealAddr(m_memwatch_addr))
					{
						if (vm::read32(addr) == m_memwatch_cmp)
						{
							// Still spinning in place
							data.reg = FIFO_EMPTY;
							return;
						}
					}
				}

				m_memwatch_addr = 0;
				m_memwatch_cmp = 0;
			}

			u32 cmd;

			if (u32 addr = RSXIOMem.RealAddr(m_internal_get))
			{
				cmd = vm::read32(addr);
			}
			else
			{
				// TODO: Optional recovery
				data.reg = FIFO_ERROR;
				return;
			}

			if (UNLIKELY(cmd & 0xe0030003))
			{
				if ((cmd & RSX_METHOD_OLD_JUMP_CMD_MASK) == RSX_METHOD_OLD_JUMP_CMD ||
					(cmd & RSX_METHOD_NEW_JUMP_CMD_MASK) == RSX_METHOD_NEW_JUMP_CMD ||
					(cmd & RSX_METHOD_CALL_CMD_MASK) == RSX_METHOD_CALL_CMD ||
					(cmd & RSX_METHOD_RETURN_MASK) == RSX_METHOD_RETURN_CMD)
				{
					// Flow control, stop reading
					data = { cmd, 0, m_internal_get };
					return;
				}
			}

			if (UNLIKELY((cmd & RSX_METHOD_NOP_MASK) == RSX_METHOD_NOP_CMD))
			{
				m_ctrl->get.store(m_internal_get + 4);
				data = { RSX_METHOD_NOP_CMD, 0, m_internal_get };
				return;
			}
			else if (UNLIKELY(cmd & 0x3))
			{
				// Malformed command, optional recovery
				data.reg = FIFO_ERROR;
				return;
			}

			// Validate the args ptr if the command attempts to read from it
			m_args_ptr = RSXIOMem.RealAddr(m_internal_get + 4);
			if (UNLIKELY(!m_args_ptr))
			{
				// Optional recovery
				data.reg = FIFO_ERROR;
				return;
			}

			verify(HERE), !m_remaining_commands;
			u32 count = (cmd >> 18) & 0x7ff;

			if (count > 1)
			{
				// Stop command execution if put will be equal to get ptr during the execution itself
				if (UNLIKELY(count * 4 + 4 > put - m_internal_get))
				{
					count = (put - m_internal_get) / 4 - 1;
				}

				// Set up readback parameters
				m_command_reg = cmd & 0xfffc;
				m_command_inc = ((cmd & RSX_METHOD_NON_INCREMENT_CMD_MASK) == RSX_METHOD_NON_INCREMENT_CMD) ? 0 : 4;
				m_remaining_commands = count - 1;

				m_ctrl->get.store(m_internal_get + (count * 4 + 4));
				data = { m_command_reg, vm::read32(m_args_ptr), m_internal_get };
			}
			else
			{
				m_ctrl->get.store(m_internal_get + 8);
				data = { cmd & 0xfffc, vm::read32(m_args_ptr), m_internal_get };
			}
		}
	}

	void thread::run_FIFO()
	{
		FIFO::register_pair command;
		fifo_ctrl->read(command);
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
			else
			{
				std::this_thread::yield();
			}

			return;
		}

		// Validate put and get registers before reading the command
		// TODO: Who should handle graphics exceptions??
		if (UNLIKELY(cmd & 0xe0030003))
		{
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

			fmt::throw_exception("Unreachable" HERE);
		}
		else if (cmd == RSX_METHOD_NOP_CMD)
		{
			if (performance_counters.state == FIFO_state::running)
			{
				performance_counters.FIFO_idle_timestamp = get_system_time();
				performance_counters.state = FIFO_state::nop;
			}

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

		for (int i = 0; ; i++, fifo_ctrl->read_unsafe(command))
		{
			const u32 reg = command.reg >> 2;
			const u32 value = command.value;

			if (capture_current_frame)
			{
				frame_debug.command_queue.push_back(std::make_pair(reg, value));

				if (!(reg == NV406E_SET_REFERENCE || reg == NV406E_SEMAPHORE_RELEASE || reg == NV406E_SEMAPHORE_ACQUIRE))
				{
					// todo: handle nv406e methods better?, do we care about call/jumps?
					rsx::frame_capture_data::replay_command replay_cmd;
					replay_cmd.rsx_command = std::make_pair(i == 0 ? command.reg : 0, value);

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

			if (!fifo_ctrl->has_next())
			{
				break;
			}
		}
	}
}