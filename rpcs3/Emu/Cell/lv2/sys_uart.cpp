#include "stdafx.h"
#include "Emu/System.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/lv2/sys_event.h"
#include "Emu/Cell/lv2/sys_timer.h"

#include "sys_uart.h"

#include <deque>


LOG_CHANNEL(sys_uart);

std::deque<uart_payload> payloads;
semaphore<> mutex;

error_code sys_uart_initialize()
{
	sys_uart.todo("sys_uart_initialize()");

	return CELL_OK;
}

error_code sys_uart_receive(ppu_thread& ppu, vm::ptr<void> buffer, u64 size, u32 unk)
{
	sys_uart.todo("sys_uart_receive(buffer=*0x%x, size=0x%llx, unk=0x%x)", buffer, size, unk);

	// blocking this for 0.85, not sure if correct for newer kernels
	u32 rtnSize = 0;
	while (!ppu.is_stopped())
	{
		sys_timer_usleep(ppu, 1000);

		std::lock_guard lock(mutex);
		if (rtnSize && payloads.size() == 0)
		{
			break;
		}

		//if (payloads.size() == 0)
		//	return CELL_OK;
		u32 addr = buffer.addr();

		//for (const auto payload : payloads)
		while (payloads.size())
		{
			const auto payload = payloads.front();
			if (payload.data.size())
			{
				u32 cid = payload.data[0];
				switch (cid)
				{
				case PS3AV_CID_VIDEO_INIT:
				case PS3AV_CID_AUDIO_INIT:
				case PS3AV_CID_AV_GET_KSV_LIST_SIZE:
				case PS3AV_CID_VIDEO_FORMAT:
				case PS3AV_CID_VIDEO_PITCH:
				case PS3AV_CID_VIDEO_ROUTE:
				case PS3AV_CID_AV_VIDEO_DISABLE_SIG:
				case PS3AV_CID_AV_TV_MUTE:
				case PS3AV_CID_AV_UNK:
				case PS3AV_CID_AV_UNK2:
				case PS3AV_CID_VIDEO_GET_HW_CONF:
				case PS3AV_CID_AV_HDMI_MODE:
				case PS3AV_CID_AV_SET_ACP_PACKET:
				case PS3AV_CID_AV_VIDEO_UNK4:
				case PS3AV_CID_AV_ENABLE_EVENT: // this one has another u32, mask of enabled events
				case PS3AV_CID_AV_AUDIO_MUTE: // this should have another u32 for mute/unmute
				case PS3AV_CID_AUDIO_MUTE:
				case PS3AV_CID_AUDIO_MODE:
				case PS3AV_CID_AUDIO_CTRL:
				case PS3AV_CID_AUDIO_ACTIVE:
				case PS3AV_CID_AUDIO_INACTIVE:
				case PS3AV_CID_AUDIO_SPDIF_BIT:
				case PS3AV_CID_AVB_PARAM:
				case PS3AV_CID_AV_INIT: // this one has another u32 in data, event_bits or something?
				case 0xa0002:           // unk
				{
					auto out = vm::ptr<ps3av_reply_cmd_hdr>::make(addr);
					out->cid = cid | PS3AV_REPLY_BIT;
					out->length = 8;
					out->status = 0;
					out->version = payload.version;
					addr += sizeof(ps3av_reply_cmd_hdr);
					rtnSize += sizeof(ps3av_reply_cmd_hdr);
					break;
				}
				case PS3AV_CID_AV_GET_HW_CONF:
				{
					auto out = vm::ptr<ps3av_get_hw_info>::make(addr);
					out->hdr.version = payload.version;
					out->hdr.length = sizeof(ps3av_get_hw_info) - sizeof(ps3av_header);
					out->cid = cid | PS3AV_REPLY_BIT;

					out->status = 0;
					out->num_of_hdmi = 1;
					out->num_of_avmulti = 0;
					out->num_of_spdif = 1;
					addr += sizeof(ps3av_get_hw_info);
					rtnSize += sizeof(ps3av_get_hw_info);
					break;
				}
				case PS3AV_CID_AV_GET_MONITOR_INFO:
				{
					auto evnt = vm::ptr<ps3av_get_monitor_info>::make(addr);
					evnt->hdr.version = payload.version;
					evnt->hdr.length = (sizeof(ps3av_get_monitor_info) - sizeof(ps3av_header));
					evnt->cid = cid | PS3AV_REPLY_BIT;
					evnt->status = 0;

					evnt->info.avport = 0; // this looks to be hardcoded check
					//	evnt->info.monitor_id =
					evnt->info.monitor_type = PS3AV_MONITOR_TYPE_HDMI;

					//	evnt->info.monitor_name;
					evnt->info.res_60.native = PS3AV_RESBIT_1280x720P;
					evnt->info.res_60.res_bits = 0xf;
					evnt->info.res_50.native = PS3AV_RESBIT_1280x720P;
					evnt->info.res_50.res_bits = 0xf;
					evnt->info.res_vesa.res_bits = 1;

					evnt->info.cs.rgb = 1; // full rbg?
					evnt->info.cs.yuv444 = 1;
					evnt->info.cs.yuv422 = 1;

					evnt->info.color.blue_x = 0xFFFF;
					evnt->info.color.blue_y = 0xFFFF;
					evnt->info.color.green_x = 0xFFFF;
					evnt->info.color.green_y = 0xFFFF;
					evnt->info.color.red_x = 0xFFFF;
					evnt->info.color.red_y = 0xFFFF;
					evnt->info.color.white_x = 0xFFFF;
					evnt->info.color.white_x = 0xFFFF;
					evnt->info.color.gamma = 100;

					evnt->info.supported_ai = 0; // ????
					evnt->info.speaker_info = 0; // ????
					evnt->info.num_of_audio_block = 0;

					addr += sizeof(ps3av_get_monitor_info);
					rtnSize += sizeof(ps3av_get_monitor_info);
					break;
				}
				default: fmt::throw_exception("unhandled packet 0x%x", cid); break;
				}
			}
			payloads.pop_front();
		}

		//payloads.clear();
	}

	return not_an_error(rtnSize);
}

error_code sys_uart_send(vm::cptr<void> buffer, u64 size, u64 flags)
{
	sys_uart.todo("sys_uart_send(buffer=0x%x, size=0x%llx, flags=0x%x)", buffer, size, flags);

	if ((flags & ~2ull) != 0)
	{
		return CELL_EINVAL;
	}

	u64 counter = size;

	if (flags == 0x2)
	{
		// avset
		if (counter < 4)
		{
			fmt::throw_exception("unhandled packet size, no header?");
		}

		std::lock_guard lock(mutex);
		vm::ptr<u32> data;
		u32 addr = buffer.addr();

		while (counter > 0)
		{
			auto hdr = vm::ptr<ps3av_header>::make(addr);
			//if (hdr->version != PS3AV_VERSION)
			//	fmt::throw_exception("invalid ps3av_version: 0x%x", hdr->version);

			u32 length = hdr->length;
			if (length == 0)
			{
				fmt::throw_exception("empty packet?");
			}

			data = vm::ptr<u32>::make(addr + sizeof(ps3av_header));
			uart_payload pl;
			for (int i = 0; i < length; i += 4)
			{
				pl.data.push_back(data[i / 4]);
			}

			for (const auto& d : pl.data)
			{
				sys_uart.error("uart: 0x%x", d);
			}

			sys_uart.error("end");
			pl.version = hdr->version;
			payloads.emplace_back(pl);

			counter -= (length + sizeof(ps3av_header));
			addr += sizeof(ps3av_header) + length;
		}
	}
	else
	{
		// two other types, dismatch manager and system manager?
	}
	return not_an_error(size);
}

error_code sys_uart_get_params(vm::ptr<char> buffer)
{
	// buffer's size should be 0x10
	sys_uart.todo("sys_uart_get_params(buffer=0x%x)", buffer);

	return CELL_OK;
}
