#include "stdafx.h"

#include "RSXFIFO.h"
#include "RSXThread.h"
#include "Capture/rsx_capture.h"

namespace rsx
{
	namespace FIFO
	{
		FIFO_control::FIFO_control(::rsx::thread* pctrl)
		{
			m_ctrl = pctrl->ctrl;
		}

		void FIFO_control::inc_get(bool wait)
		{
			m_internal_get += 4;

			if (wait && read_put<false>() == m_internal_get)
			{
				// NOTE: Only supposed to be invoked to wait for a single arg on command[0] (4 bytes)
				// Wait for put to allow us to procceed execution
				sync_get();

				while (read_put() == m_internal_get && !Emu.IsStopped())
				{
					std::this_thread::yield();
				}
			}
		}

		template <bool full>
		inline u32 FIFO_control::read_put()
		{
			if constexpr (!full)
			{
				return m_ctrl->put & ~3;
			}
			else
			{
				if (u32 put = m_ctrl->put; (put & 3) == 0) [[likely]]
				{
					return put;
				}

				return m_ctrl->put.and_fetch(~3);
			}
		}

		void FIFO_control::set_put(u32 put)
		{
			m_ctrl->put = put;
		}

		void FIFO_control::set_get(u32 get)
		{
			if (m_ctrl->get == get)
			{
				if (const auto addr = RSXIOMem.RealAddr(m_memwatch_addr))
				{
					m_memwatch_addr = get;
					m_memwatch_cmp = vm::read32(addr);
				}

				return;
			}

			// Update ctrl registers
			m_ctrl->get.release(m_internal_get = get);
			m_remaining_commands = 0;

			// Clear memwatch spinner
			m_memwatch_addr = 0;
		}

		bool FIFO_control::read_unsafe(register_pair& data)
		{
			// Fast read with no processing, only safe inside a PACKET_BEGIN+count block
			if (m_remaining_commands &&
				m_internal_get != read_put<false>())
			{
				m_command_reg += m_command_inc;
				m_args_ptr += 4;
				m_remaining_commands--;
				m_internal_get += 4;

				data.set(m_command_reg, vm::read32(m_args_ptr));
				return true;
			}

			return false;
		}

		void FIFO_control::abort()
		{
			m_remaining_commands = 0;
		}

		void FIFO_control::read(register_pair& data)
		{
			const u32 put = read_put();
			m_internal_get = m_ctrl->get;

			if (put == m_internal_get)
			{
				// Nothing to do
				data.reg = FIFO_EMPTY;
				return;
			}

			if (m_remaining_commands && read_unsafe(data))
			{
				// Previous block aborted to wait for PUT pointer
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

			if (u32 addr = RSXIOMem.RealAddr(m_internal_get))
			{
				m_cmd = vm::read32(addr);
			}
			else
			{
				// TODO: Optional recovery
				data.reg = FIFO_ERROR;
				return;
			}

			if (m_cmd & RSX_METHOD_NON_METHOD_CMD_MASK) [[unlikely]]
			{
				if ((m_cmd & RSX_METHOD_OLD_JUMP_CMD_MASK) == RSX_METHOD_OLD_JUMP_CMD ||
					(m_cmd & RSX_METHOD_NEW_JUMP_CMD_MASK) == RSX_METHOD_NEW_JUMP_CMD ||
					(m_cmd & RSX_METHOD_CALL_CMD_MASK) == RSX_METHOD_CALL_CMD ||
					(m_cmd & RSX_METHOD_RETURN_MASK) == RSX_METHOD_RETURN_CMD)
				{
					// Flow control, stop reading
					data.reg = m_cmd;
					return;
				}

				// Malformed command, optional recovery
				data.reg = FIFO_ERROR;
				return;
			}

			// Validate the args ptr if the command attempts to read from it
			m_args_ptr = RSXIOMem.RealAddr(m_internal_get + 4);
			if (!m_args_ptr) [[unlikely]]
			{
				// Optional recovery
				data.reg = FIFO_ERROR;
				return;
			}

			verify(HERE), !m_remaining_commands;
			const u32 count = (m_cmd >> 18) & 0x7ff;

			if (!count)
			{
				m_ctrl->get.release(m_internal_get + 4);
				data.reg = FIFO_NOP;
				return;
			}

			if (count > 1)
			{
				// Set up readback parameters
				m_command_reg = m_cmd & 0xfffc;
				m_command_inc = ((m_cmd & RSX_METHOD_NON_INCREMENT_CMD_MASK) == RSX_METHOD_NON_INCREMENT_CMD) ? 0 : 4;
				m_remaining_commands = count - 1;
			}

			inc_get(true); // Wait for data block to become available
			m_internal_get += 4;

			data.set(m_cmd & 0xfffc, vm::read32(m_args_ptr));
		}

		void flattening_helper::reset(bool _enabled)
		{
			enabled = _enabled;
			num_collapsed = 0;
			begin_end_ctr = 0;
		}

		void flattening_helper::force_disable()
		{
			if (enabled)
			{
				rsx_log.warning("FIFO optimizations have been disabled as the application is not compatible with per-frame analysis");

				reset(false);
				fifo_hint = optimization_hint::application_not_compatible;
			}
		}

		void flattening_helper::evaluate_performance(u32 total_draw_count)
		{
			if (!enabled)
			{
				if (fifo_hint == optimization_hint::application_not_compatible)
				{
					// Not compatible, do nothing
					return;
				}

				if (total_draw_count <= 2000)
				{
					// Low draw call pressure
					fifo_hint = optimization_hint::load_low;
					return;
				}

				if (fifo_hint == optimization_hint::load_unoptimizable)
				{
					// Nope, wait for stats to change
					return;
				}
			}

			if (enabled)
			{
				// Currently activated. Check if there is any benefit
				if (num_collapsed < 500)
				{
					// Not worth it, disable
					enabled = false;
					fifo_hint = load_unoptimizable;
				}

				u32 real_total = total_draw_count + num_collapsed;
				if (real_total <= 2000)
				{
					// Low total number of draws submitted, no need to keep trying for now
					enabled = false;
					fifo_hint = load_low;
				}

				reset(enabled);
			}
			else
			{
				// Not enabled, check if we should try enabling
				verify(HERE), total_draw_count > 2000;
				if (fifo_hint != load_unoptimizable)
				{
					// If its set to unoptimizable, we already tried and it did not work
					// If it resets to load low (usually after some kind of loading screen) we can try again
					verify("Incorrect initial state" HERE), begin_end_ctr == 0, num_collapsed == 0;
					enabled = true;
				}
			}
		}

		flatten_op flattening_helper::test(register_pair& command)
		{
			u32 flush_cmd = ~0u;
			switch (const u32 reg = (command.reg >> 2))
			{
			case NV4097_SET_BEGIN_END:
			{
				begin_end_ctr ^= 1;

				if (command.value)
				{
					// This is a BEGIN call
					if (!deferred_primitive) [[likely]]
					{
						// New primitive block
						deferred_primitive = command.value;
					}
					else if (deferred_primitive == command.value)
					{
						// Same primitive can be chanined; do nothing
						command.reg = FIFO_DISABLED_COMMAND;
					}
					else
					{
						// Primitive command has changed!
						// Flush
						flush_cmd = command.value;
					}
				}
				else if (deferred_primitive)
				{
					command.reg = FIFO_DRAW_BARRIER;
					draw_count++;
				}
				else
				{
					rsx_log.error("Fifo flattener misalignment, disable FIFO reordering and report to developers");
					begin_end_ctr = 0;
					flush_cmd = 0u;
				}

				break;
			}
			case NV4097_DRAW_ARRAYS:
			case NV4097_DRAW_INDEX_ARRAY:
			{
				// TODO: Check type
				break;
			}
			default:
			{
				if (draw_count) [[unlikely]]
				{
					if (m_register_properties[reg] & register_props::always_ignore) [[unlikely]]
					{
						// Always ignore
						command.reg = FIFO_DISABLED_COMMAND;
					}
					else
					{
						// Flush
						flush_cmd = (begin_end_ctr) ? deferred_primitive : 0u;
					}
				}
				else
				{
					// Nothing to do
					return NOTHING;
				}

				break;
			}
			}

			if (flush_cmd != ~0u)
			{
				num_collapsed += draw_count? (draw_count - 1) : 0;
				draw_count = 0;
				deferred_primitive = flush_cmd;

				return (begin_end_ctr == 1)? EMIT_BARRIER : EMIT_END;
			}

			return NOTHING;
		}
	}

	void thread::run_FIFO()
	{
		FIFO::register_pair command;
		fifo_ctrl->read(command);
		const auto cmd = command.reg;

		if (cmd & (0xffff0000 | RSX_METHOD_NON_METHOD_CMD_MASK)) [[unlikely]]
		{
			// Check for special FIFO commands
			switch (cmd)
			{
			case FIFO::FIFO_NOP:
			{
				if (performance_counters.state == FIFO_state::running)
				{
					performance_counters.FIFO_idle_timestamp = get_system_time();
					performance_counters.state = FIFO_state::nop;
				}

				return;
			}
			case FIFO::FIFO_EMPTY:
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
			case FIFO::FIFO_BUSY:
			{
				// Do something else
				return;
			}
			case FIFO::FIFO_ERROR:
			{
				rsx_log.error("FIFO error: possible desync event (last cmd = 0x%x)", fifo_ctrl->last_cmd());
				recover_fifo();
				return;
			}
			}

			// Check for flow control
			if ((cmd & RSX_METHOD_OLD_JUMP_CMD_MASK) == RSX_METHOD_OLD_JUMP_CMD)
			{
				const u32 offs = cmd & RSX_METHOD_OLD_JUMP_OFFSET_MASK;
				if (offs == fifo_ctrl->get_pos())
				{
					//Jump to self. Often preceded by NOP
					if (performance_counters.state == FIFO_state::running)
					{
						performance_counters.FIFO_idle_timestamp = get_system_time();
						sync_point_request = true;
					}

					performance_counters.state = FIFO_state::spinning;
				}

				//rsx_log.warning("rsx jump(0x%x) #addr=0x%x, cmd=0x%x, get=0x%x, put=0x%x", offs, m_ioAddress + get, cmd, get, put);
				fifo_ctrl->set_get(offs);
				return;
			}
			if ((cmd & RSX_METHOD_NEW_JUMP_CMD_MASK) == RSX_METHOD_NEW_JUMP_CMD)
			{
				const u32 offs = cmd & RSX_METHOD_NEW_JUMP_OFFSET_MASK;
				if (offs == fifo_ctrl->get_pos())
				{
					//Jump to self. Often preceded by NOP
					if (performance_counters.state == FIFO_state::running)
					{
						performance_counters.FIFO_idle_timestamp = get_system_time();
						sync_point_request = true;
					}

					performance_counters.state = FIFO_state::spinning;
				}

				//rsx_log.warning("rsx jump(0x%x) #addr=0x%x, cmd=0x%x, get=0x%x, put=0x%x", offs, m_ioAddress + get, cmd, get, put);
				fifo_ctrl->set_get(offs);
				return;
			}
			if ((cmd & RSX_METHOD_CALL_CMD_MASK) == RSX_METHOD_CALL_CMD)
			{
				if (fifo_ret_addr != RSX_CALL_STACK_EMPTY)
				{
					// Only one layer is allowed in the call stack.
					rsx_log.error("FIFO: CALL found inside a subroutine. Discarding subroutine");
					fifo_ctrl->set_get(std::exchange(fifo_ret_addr, RSX_CALL_STACK_EMPTY));
					return;
				}

				const u32 offs = cmd & RSX_METHOD_CALL_OFFSET_MASK;
				fifo_ret_addr = fifo_ctrl->get_pos() + 4;
				fifo_ctrl->set_get(offs);
				return;
			}
			if ((cmd & RSX_METHOD_RETURN_MASK) == RSX_METHOD_RETURN_CMD)
			{
				if (fifo_ret_addr == RSX_CALL_STACK_EMPTY)
				{
					rsx_log.error("FIFO: RET found without corresponding CALL. Discarding queue");
					fifo_ctrl->set_get(ctrl->put);
					return;
				}

				fifo_ctrl->set_get(std::exchange(fifo_ret_addr, RSX_CALL_STACK_EMPTY));
				return;
			}

			// If we reached here, this is likely an error
			fmt::throw_exception("Unexpected command 0x%x" HERE, cmd);
		}

		if (const auto state = performance_counters.state;
			state != FIFO_state::running)
		{
			performance_counters.state = FIFO_state::running;

			// Hack: Delay FIFO wake-up according to setting
			// NOTE: The typical spin setup is a NOP followed by a jump-to-self
			// NOTE: There is a small delay when the jump address is dynamically edited by cell
			if (state != FIFO_state::nop)
			{
				fifo_wake_delay();
			}

			// Update performance counters with time spent in idle mode
			performance_counters.idle_time += (get_system_time() - performance_counters.FIFO_idle_timestamp);
		}

		do
		{
			if (capture_current_frame) [[unlikely]]
			{
				const u32 reg = (command.reg & 0xfffc) >> 2;
				const u32 value = command.value;

				frame_debug.command_queue.emplace_back(reg, value);

				if (!(reg == NV406E_SET_REFERENCE || reg == NV406E_SEMAPHORE_RELEASE || reg == NV406E_SEMAPHORE_ACQUIRE))
				{
					// todo: handle nv406e methods better?, do we care about call/jumps?
					rsx::frame_capture_data::replay_command replay_cmd;
					replay_cmd.rsx_command = std::make_pair((reg << 2) | (1u << 18), value);

					frame_capture.replay_commands.push_back(replay_cmd);
					auto it = frame_capture.replay_commands.back();

					switch (reg)
					{
					case NV3089_IMAGE_IN:
						capture::capture_image_in(this, it);
						break;
					case NV0039_BUFFER_NOTIFY:
						capture::capture_buffer_notify(this, it);
						break;
					default:
						break;
					}
				}
			}

			if (m_flattener.is_enabled()) [[unlikely]]
			{
				switch(m_flattener.test(command))
				{
				case FIFO::NOTHING:
				{
					break;
				}
				case FIFO::EMIT_END:
				{
					// Emit end command to close existing scope
					//verify(HERE), in_begin_end;
					methods[NV4097_SET_BEGIN_END](this, NV4097_SET_BEGIN_END, 0);
					break;
				}
				case FIFO::EMIT_BARRIER:
				{
					//verify(HERE), in_begin_end;
					methods[NV4097_SET_BEGIN_END](this, NV4097_SET_BEGIN_END, 0);
					methods[NV4097_SET_BEGIN_END](this, NV4097_SET_BEGIN_END, m_flattener.get_primitive());
					break;
				}
				default:
				{
					fmt::throw_exception("Unreachable" HERE);
				}
				}

				if (command.reg == FIFO::FIFO_DISABLED_COMMAND)
				{
					// Optimized away
					continue;
				}
			}

			const u32 reg = (command.reg & 0xffff) >> 2;
			const u32 value = command.value;

			method_registers.decode(reg, value);

			if (auto method = methods[reg])
			{
				method(this, reg, value);

				if (invalid_command_interrupt_raised)
				{
					fifo_ctrl->abort();
					recover_fifo();
					return;
				}
			}
		}
		while (fifo_ctrl->read_unsafe(command));

		fifo_ctrl->sync_get();
	}
}
