#include "stdafx.h"
#include "rsx_replay.h"

#include "Emu/System.h"
#include "Emu/Cell/lv2/sys_rsx.h"
#include "Emu/Memory/vm.h"
#include "Emu/RSX/GSRender.h"

#include <map>
#include <exception>

namespace rsx
{
	be_t<u32> rsx_replay_thread::allocate_context()
	{
		// 'fake' initialize usermemory
		// todo: seriously, need to probly watch the replay memory map and just make sure its mapped before we copy rather than do this
		user_mem_addr = vm::falloc(vm::get(vm::user1m)->addr, 0x10000000);
		verify(HERE), user_mem_addr != 0;

		const u32 contextAddr = vm::alloc(sizeof(rsx_context), vm::main);
		if (contextAddr == 0)
			fmt::throw_exception("Capture Replay: context alloc failed");
		const auto& contextInfo = vm::_ref<rsx_context>(contextAddr);

		if (sys_rsx_device_map(vm::get_addr(&contextInfo.dev_addr), vm::null, 0x8) != CELL_OK)
			fmt::throw_exception("Capture Replay: sys_rsx_device_map failed!");

		if (sys_rsx_memory_allocate(vm::get_addr(&contextInfo.mem_handle), vm::get_addr(&contextInfo.mem_addr), 0x0F900000, 0, 0, 0, 0) != CELL_OK)
			fmt::throw_exception("Capture Replay: sys_rsx_memory_allocate failed!");

		if (sys_rsx_context_allocate(vm::get_addr(&contextInfo.context_id), vm::get_addr(&contextInfo.dma_addr), vm::get_addr(&contextInfo.driver_info), vm::get_addr(&contextInfo.reports_addr), contextInfo.mem_handle, 0) != CELL_OK)
			fmt::throw_exception("Capture Replay: sys_rsx_context_allocate failed!");

		// 1024Mb, the extra 512Mb memory is needed to allocate FIFO commands on
		// So there wont be any conflicts with memory used in the capture
		get_current_renderer()->main_mem_size = 0x40000000;

		return contextInfo.context_id;
	}

	std::vector<u32> rsx_replay_thread::alloc_write_fifo(be_t<u32> context_id)
	{
		u32 fifo_size = 4;

		// run through replay commands to figure out how big command buffer needs to be
		for (const auto& rc : frame->replay_commands)
		{
			const u32 count = (rc.rsx_command.first >> 18) & 0x7ff;
			// allocate for register plus w/e number of arguments it has
			fifo_size += (count * 4) + 4;
		}

		fifo_size = ::align<u32>(fifo_size, 0x100000);

		const u32 fifo_mem = vm::alloc(fifo_size, vm::main, 0x100000);
		if (fifo_mem == 0)
			fmt::throw_exception("Capture Replay: fifo alloc failed! size: 0x%x", fifo_size);

		// copy commands into fifo buffer
		// todo: could change rsx_command to just be values to avoid this loop,
		auto fifo_addr = vm::ptr<u32>::make(fifo_mem);
		u32 count = 0;
		std::vector<u32> fifo_stops;
		u32 currentOffset = 0x20000000;
		for (const auto& rc : frame->replay_commands)
		{
			bool hasState = (rc.memory_state.size() > 0) || (rc.display_buffer_state != 0) || (rc.tile_state != 0);
			if (hasState)
			{
				if (count != 0)
				{
					// todo: support memory state in the middle of incremented command
					// This shouldn't ever happen as long as captures stay in 'strict' aka non-multidraw mode
					fmt::throw_exception("capture replay: state change not supported between increment commands");
				}

				fifo_stops.emplace_back(currentOffset);
			}

			// spit out command
			if (count == 0)
			{
				count = (rc.rsx_command.first >> 18) & 0x7ff;
				*fifo_addr = rc.rsx_command.first;
				fifo_addr++;
				currentOffset += 4;
			}

			if (count != 0)
			{
				*fifo_addr = rc.rsx_command.second;
				fifo_addr++;
				count--;
				currentOffset += 4;
			}
		}

		fifo_stops.emplace_back(currentOffset);

		if (sys_rsx_context_iomap(context_id, 0x20000000, fifo_mem, fifo_size, 0) != CELL_OK)
			fmt::throw_exception("Capture Replay: fifo mapping failed");

		return fifo_stops;
	}

	void rsx_replay_thread::apply_frame_state(be_t<u32> context_id, const frame_capture_data::replay_command& replay_cmd)
	{
		// apply memory needed for command
		for (const auto& state : replay_cmd.memory_state)
		{
			auto it = frame->memory_map.find(state);
			if (it == frame->memory_map.end())
				fmt::throw_exception("requested memory state for command not found in memory_map");

			const auto& memblock = it->second;
			auto it_data = frame->memory_data_map.find(it->second.data_state);
			if (it_data == frame->memory_data_map.end())
				fmt::throw_exception("requested memory data state for command not found in memory_data_map");

			const auto& data_block = it_data->second;
			std::memcpy(vm::base(get_address(memblock.offset, memblock.location)), data_block.data.data(), data_block.data.size());
		}

		if (replay_cmd.display_buffer_state != 0 && replay_cmd.display_buffer_state != cs.display_buffer_hash)
		{
			auto it = frame->display_buffers_map.find(replay_cmd.display_buffer_state);
			if (it == frame->display_buffers_map.end())
				fmt::throw_exception("requested display buffer for command not found");

			const auto& dbstate = it->second;
			for (u32 i = 0; i < dbstate.count; ++i)
			{
				const auto& buf = dbstate.buffers[i];
				if (cs.display_buffer_hash != 0 && memcmp(&cs.buffer_state.buffers[i], &buf, sizeof(rsx::frame_capture_data::buffer_state)) == 0)
					continue;

				cs.buffer_state.buffers[i] = buf;
				sys_rsx_context_attribute(context_id, 0x104, i,
					(u64)dbstate.buffers[i].width << 32 | dbstate.buffers[i].height, (u64)dbstate.buffers[i].pitch << 32 | dbstate.buffers[i].offset, 0);
			}
			cs.display_buffer_hash = replay_cmd.display_buffer_state;
		}

		if (replay_cmd.tile_state != 0 && replay_cmd.tile_state != cs.tile_hash)
		{
			auto it = frame->tile_map.find(replay_cmd.tile_state);
			if (it == frame->tile_map.end())
				fmt::throw_exception("requested tile state command not found");

			const auto& tstate = it->second;
			for (u32 i = 0; i < limits::tiles_count; ++i)
			{
				const auto& ti = tstate.tiles[i];
				if (cs.tile_hash != 0 && memcmp(&cs.tile_state.tiles[i], &ti, sizeof(rsx::frame_capture_data::tile_info)) == 0)
					continue;

				cs.tile_state.tiles[i] = ti;
				sys_rsx_context_attribute(context_id, 0x300, i, (u64)ti.tile << 32 | ti.limit, (u64)ti.pitch << 32 | ti.format, 0);
			}

			for (u32 i = 0; i < limits::zculls_count; ++i)
			{
				const auto& zci = tstate.zculls[i];
				if (cs.tile_hash != 0 && memcmp(&cs.tile_state.zculls[i], &zci, sizeof(rsx::frame_capture_data::zcull_info)) == 0)
					continue;

				cs.tile_state.zculls[i] = zci;
				sys_rsx_context_attribute(context_id, 0x301, i, (u64)zci.region << 32 | zci.size, (u64)zci.start << 32 | zci.offset, (u64)zci.status0 << 32 | zci.status1);
			}

			cs.tile_hash = replay_cmd.tile_state;
		}
	}

	void rsx_replay_thread::on_task()
	{
		be_t<u32> context_id = allocate_context();

		auto fifo_stops = alloc_write_fifo(context_id);

		// map game io
		if (sys_rsx_context_iomap(context_id, 0x0, user_mem_addr, 0x10000000, 0) != CELL_OK)
			fmt::throw_exception("rsx io map failed");

		while (!Emu.IsStopped())
		{
			// Load registers while the RSX is still idle
			method_registers = frame->reg_state;
			_mm_mfence();

			// start up fifo buffer by dumping the put ptr to first stop
			sys_rsx_context_attribute(context_id, 0x001, 0x20000000, fifo_stops[0], 0, 0);

			auto render = get_current_renderer();
			auto last_flip = render->int_flip_index;

			size_t stopIdx = 0;
			for (const auto& replay_cmd : frame->replay_commands)
			{
				while (Emu.IsPaused())
					std::this_thread::sleep_for(10ms);

				if (Emu.IsStopped())
					break;

				// Loop and hunt down our next state change that needs to be done
				if (!((replay_cmd.memory_state.size() > 0) || (replay_cmd.display_buffer_state != 0) || (replay_cmd.tile_state != 0)))
					continue;

				// wait until rsx idle and at our first 'stop' to apply state
				while (!Emu.IsStopped() && (render->ctrl->get != render->ctrl->put) && (render->ctrl->get != fifo_stops[stopIdx]))
				{
					while (Emu.IsPaused())
						std::this_thread::sleep_for(10ms);
					std::this_thread::yield();
				}

				stopIdx++;

				apply_frame_state(context_id, replay_cmd);

				// move put ptr to next stop
				if (stopIdx >= fifo_stops.size())
					fmt::throw_exception("Capture Replay: StopIdx greater than size of fifo_stops");

				render->ctrl->put = fifo_stops[stopIdx];
			}

			// dump put to end of stops, which should have actual end
			u32 end = fifo_stops.back();
			render->ctrl->put = end;

			while (render->ctrl->get != end && !Emu.IsStopped())
			{
				while (Emu.IsPaused())
					std::this_thread::sleep_for(10ms);
			}

			// Check if the captured application used syscall instead of a gcm command to flip
			if (render->int_flip_index == last_flip)
			{
				// Capture did not include a display flip, flip manually
				render->request_emu_flip(1u);
			}

			// random pause to not destroy gpu
			std::this_thread::sleep_for(10ms);
		}
	}

	void rsx_replay_thread::operator()()
	{
		try
		{
			on_task();
		}
		catch (const std::exception& e)
		{
			LOG_FATAL(RSX, "%s thrown: %s", typeid(e).name(), e.what());
			Emu.Pause();
		}
	}
}
