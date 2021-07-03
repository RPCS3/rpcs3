#include "stdafx.h"

#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/lv2/sys_sync.h"

#include "sys_uart.h"

LOG_CHANNEL(sys_uart);

struct AV_GET_MONITOR_INFO_CMD : public ps3av_cmd
{
	u16 get_size(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		static_cast<void>(vuart);
		static_cast<void>(pkt_buf);
		return 12;
	}

	bool execute(vuart_av_thread &vuart, const void *pkt_buf, u32 reply_max_size) override
	{
		auto pkt = static_cast<const ps3av_get_monitor_info *>(pkt_buf);

		if (reply_max_size < sizeof(ps3av_get_monitor_info))
		{
			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_BUFFER_OVERFLOW);
			return false;
		}

		// TODO:
		auto evnt = std::make_unique<ps3av_get_monitor_info_reply>();
		evnt->avport = 0;
		// evnt->monitor_id =
		evnt->monitor_type = PS3AV_MONITOR_TYPE_HDMI;

		// evnt->monitor_name;
		evnt->res_60.native = PS3AV_RESBIT_1280x720P;
		evnt->res_60.res_bits = 0xf;
		evnt->res_50.native = PS3AV_RESBIT_1280x720P;
		evnt->res_50.res_bits = 0xf;
		evnt->res_vesa.res_bits = 1;

		evnt->cs.rgb = 1;
		evnt->cs.yuv444 = 1;
		evnt->cs.yuv422 = 1;

		evnt->color.blue_x = 0xFFFF;
		evnt->color.blue_y = 0xFFFF;
		evnt->color.green_x = 0xFFFF;
		evnt->color.green_y = 0xFFFF;
		evnt->color.red_x = 0xFFFF;
		evnt->color.red_y = 0xFFFF;
		evnt->color.white_x = 0xFFFF;
		evnt->color.white_x = 0xFFFF;
		evnt->color.gamma = 100;

		evnt->supported_ai = 0;
		evnt->speaker_info = 0;
		evnt->num_of_audio_block = 0;

		vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_SUCCESS, evnt.get(), sizeof(ps3av_get_monitor_info_reply));
		return true;
	}
};

struct AV_GET_HW_CONF_CMD : public ps3av_cmd
{
	u16 get_size(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		static_cast<void>(vuart);
		static_cast<void>(pkt_buf);
		return 8;
	}

	bool execute(vuart_av_thread &vuart, const void *pkt_buf, u32 reply_max_size) override
	{
		auto pkt = static_cast<const ps3av_header *>(pkt_buf);

		if (reply_max_size < sizeof(ps3av_get_hw_info_reply) + sizeof(ps3av_pkt_reply_hdr))
		{
			vuart.write_resp(pkt->cid, PS3AV_STATUS_BUFFER_OVERFLOW);
			return false;
		}

		// TODO:
		auto out = std::make_unique<ps3av_get_hw_info_reply>();
		out->num_of_hdmi = 1;
		out->num_of_avmulti = 0;
		out->num_of_spdif = 1;
		out->resv = 0;

		vuart.write_resp(pkt->cid, PS3AV_STATUS_SUCCESS, out.get(), sizeof(ps3av_get_hw_info_reply));
		return true;
	}
};

struct GENERIC_REPLY_CMD : public ps3av_cmd
{
	u16 get_size(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		static_cast<void>(vuart);
		static_cast<void>(pkt_buf);
		return 0;
	}

	bool execute(vuart_av_thread &vuart, const void *pkt_buf, u32 reply_max_size) override
	{
		auto pkt = static_cast<const ps3av_header *>(pkt_buf);

		vuart.write_resp(pkt->cid, PS3AV_STATUS_SUCCESS);
		return true;
	}
};

error_code sys_uart_initialize(ppu_thread &ppu)
{
	ppu.state += cpu_flag::wait;

	sys_uart.trace("sys_uart_initialize()");

	static shared_mutex m{};
	std::lock_guard lock(m);

	auto &vuart_thread = g_fxo->get<vuart_av>();

	if (!g_fxo->is_init<vuart_av>())
	{
		g_fxo->init<vuart_av>();
	}

	if (vuart_thread.init)
	{
		return CELL_EPERM;
	}

	vuart_thread.init = 1;

	return CELL_OK;
}

error_code sys_uart_receive(ppu_thread &ppu, vm::ptr<void> buffer, u64 size, u32 mode)
{
	sys_uart.trace("sys_uart_receive(buffer=*0x%x, size=0x%llx, mode=0x%x)", buffer, size, mode);

	if (!size)
	{
		return CELL_OK;
	}

	if ((mode & ~1UL) != 0)
	{
		return CELL_EINVAL;
	}

	auto &vuart_thread = g_fxo->get<vuart_av>();

	if (!g_fxo->is_init<vuart_av>() || !vuart_thread.init)
	{
		return CELL_ESRCH;
	}

	if (size > 0x20000U)
	{
		// kmalloc restriction
		fmt::throw_exception("Buffer is too big");
	}

	if (mode > 1)
	{
		return CELL_EINVAL;
	}

	const std::unique_ptr<u8[]> data = std::make_unique<u8[]>(size);
	u32 read_size = 0;

	auto vuart_read = [&](u8 *buf, u32 buf_size) -> s32
	{
		const u32 ITER_SIZE = 4096;
		std::unique_lock lock(vuart_thread.rx_mutex, std::defer_lock);

		if (!lock.try_lock())
		{
			return CELL_EBUSY;
		}

		const u32 first_iter_size = std::min(ITER_SIZE, buf_size);
		const u32 first_iter_read = vuart_thread.read_rx_data(buf, first_iter_size);

		if (first_iter_read < first_iter_size || buf_size <= ITER_SIZE)
		{
			return first_iter_read;
		}

		u32 read_size;

		for (read_size = ITER_SIZE; read_size + ITER_SIZE < buf_size; read_size += ITER_SIZE)
		{
			const u32 in_loop_read = vuart_thread.read_rx_data(buf + read_size, ITER_SIZE);

			if (in_loop_read < ITER_SIZE)
			{
				return read_size + in_loop_read;
			}
		}

		return read_size + vuart_thread.read_rx_data(buf + read_size, buf_size - read_size);
	};

	if (mode == 1)
	{
		// Yield before checking for packets
		lv2_obj::sleep(ppu);

		for (;;)
		{
			if (ppu.is_stopped())
			{
				return {};
			}

			std::unique_lock<std::mutex> lock(vuart_thread.rx_wake_m);
			const s32 read_result = vuart_read(data.get(), static_cast<u32>(size));

			if (read_result > CELL_OK)
			{
				read_size = read_result;
				break;
			}

			vuart_thread.rx_wake_c.wait_unlock(5000, lock);
		}

		if (ppu.check_state())
		{
			return {};
		}
	}
	else
	{
		const s32 read_result = vuart_read(data.get(), static_cast<u32>(size));

		if (read_result <= CELL_OK)
		{
			return read_result;
		}

		read_size = read_result;
	}

	if (!buffer || !vm::check_addr(buffer.addr(), vm::page_writable, read_size))
	{
		return CELL_EFAULT;
	}

	memcpy(buffer.get_ptr(), data.get(), read_size);
	return not_an_error(read_size);
}

error_code sys_uart_send(ppu_thread &ppu, vm::cptr<void> buffer, u64 size, u32 mode)
{
	sys_uart.trace("sys_uart_send(buffer=0x%x, size=0x%llx, mode=0x%x)", buffer, size, mode);

	if (!size)
	{
		return CELL_OK;
	}

	if ((mode & ~3ul) != 0)
	{
		return CELL_EINVAL;
	}

	auto &vuart_thread = g_fxo->get<vuart_av>();

	if (!g_fxo->is_init<vuart_av>() || !vuart_thread.init)
	{
		return CELL_ESRCH;
	}

	if (size > 0x20000U)
	{
		// kmalloc restriction
		fmt::throw_exception("Buffer is too big");
	}

	if (!vm::check_addr(buffer.addr(), vm::page_readable, static_cast<u32>(size)))
	{
		return CELL_EFAULT;
	}

	const std::unique_ptr<u8[]> data = std::make_unique<u8[]>(size);
	memcpy(data.get(), buffer.get_ptr(), size);

	std::unique_lock lock(vuart_thread.tx_mutex, std::defer_lock);

	const u32 ITER_SIZE = 4096;

	if (mode & 1)
	{
		// Yield before sending packets
		lv2_obj::sleep(ppu);

		lock.lock();

		auto vuart_send_all = [&](const u8 *data, u32 data_sz)
		{
			u32 rem_size = data_sz;

			while (rem_size)
			{
				if (ppu.is_stopped())
				{
					return false;
				}

				std::unique_lock<std::mutex> lock(vuart_thread.tx_rdy_m);
				if (vuart_thread.get_tx_bytes() >= PS3AV_TX_BUF_SIZE)
				{
					vuart_thread.tx_rdy_c.wait_unlock(5000, lock);
				}
				else
				{
					lock.unlock();
				}
				rem_size -= vuart_thread.enque_tx_data(data + data_sz - rem_size, rem_size);
			}

			return true;
		};

		if (!vuart_send_all(data.get(), std::min(ITER_SIZE, static_cast<u32>(size)))) return {};

		if (size > ITER_SIZE)
		{
			u32 sent_size;

			for (sent_size = ITER_SIZE; sent_size + ITER_SIZE < size; sent_size += ITER_SIZE)
			{
				if (!vuart_send_all(data.get() + sent_size, ITER_SIZE)) return {};
			}

			if (!vuart_send_all(data.get() + sent_size, static_cast<u32>(size) - sent_size)) return {};
		}

		if (ppu.check_state())
		{
			return {};
		}
	}
	else if (mode & 2)
	{
		if (!lock.try_lock())
		{
			return CELL_EBUSY;
		}

		if (PS3AV_TX_BUF_SIZE - vuart_thread.get_tx_bytes() < size)
		{
			return CELL_EAGAIN;
		}

		return not_an_error(vuart_thread.enque_tx_data(data.get(), static_cast<u32>(size)));
	}
	else
	{
		if (!lock.try_lock())
		{
			return CELL_EBUSY;
		}

		const u32 first_iter_sz   = std::min(ITER_SIZE, static_cast<u32>(size));
		const u32 first_iter_sent = vuart_thread.enque_tx_data(data.get(), first_iter_sz);

		if (first_iter_sent < first_iter_sz || size <= ITER_SIZE)
		{
			return not_an_error(first_iter_sz);
		}

		u32 sent_size;

		for (sent_size = ITER_SIZE; sent_size + ITER_SIZE < size; sent_size += ITER_SIZE)
		{
			const u32 in_loop_sent = vuart_thread.enque_tx_data(data.get() + sent_size, ITER_SIZE);

			if (in_loop_sent < ITER_SIZE)
			{
				return not_an_error(sent_size + in_loop_sent);
			}
		}

		vuart_thread.enque_tx_data(data.get() + sent_size, static_cast<u32>(size) - sent_size);
	}

	return not_an_error(size);
}

error_code sys_uart_get_params(vm::ptr<vuart_params> buffer)
{
	sys_uart.trace("sys_uart_get_params(buffer=0x%x)", buffer);

	auto &vuart_thread = g_fxo->get<vuart_av>();

	if (!g_fxo->is_init<vuart_av>() || !vuart_thread.init)
	{
		return CELL_ESRCH;
	}

	if (!buffer || !vm::check_addr(buffer.addr(), vm::page_writable, sizeof(vuart_params)))
	{
		return CELL_EFAULT;
	}

	buffer->rx_buf_size = PS3AV_RX_BUF_SIZE;
	buffer->tx_buf_size = PS3AV_TX_BUF_SIZE;

	return CELL_OK;
}

void vuart_av_thread::operator()()
{
	while (thread_ctrl::state() != thread_state::aborting)
	{
		std::unique_lock<std::mutex> lock(tx_wake_m);
		if (!tx_buf.get_used_size())
		{
			tx_wake_c.wait_unlock(-1, lock);
		}
		else
		{
			lock.unlock();
		}

		auto read_size = read_tx_data(temp_tx_buf, sizeof(temp_tx_buf));
		if (!read_size) continue;

		[&]()
		{
			if (read_size >= PS3AV_TX_BUF_SIZE)
			{
				while (read_tx_data(temp_tx_buf, sizeof(temp_tx_buf)) >= PS3AV_TX_BUF_SIZE);
				write_resp(reinterpret_cast<be_t<u16, 1>*>(temp_tx_buf)[3], PS3AV_STATUS_BUFFER_OVERFLOW);
				return;
			}

			u32 read_ptr = 0;

			while (read_size)
			{
				const ps3av_header *const hdr = reinterpret_cast<ps3av_header *>(&temp_tx_buf[read_ptr]);
				const u16 pkt_size = hdr->length + 4;

				if (hdr->length == 0xFFFCU)
				{
					write_resp(0xDEAD, PS3AV_STATUS_FAILURE);
					return;
				}

				if (hdr->version != PS3AV_VERSION)
				{
					if (hdr->version >= 0x100 && hdr->version < PS3AV_VERSION)
					{
						sys_uart.error("Unimplemented AV version: %x", hdr->version);
					}

					write_resp(static_cast<u16>(hdr->cid.get()), PS3AV_STATUS_INVALID_COMMAND);
					return;
				}

				const void *const pkt_storage = &temp_tx_buf[read_ptr];
				read_ptr += pkt_size;
				read_size = read_size < pkt_size ? 0 : read_size - pkt_size;

				auto cmd = get_cmd(hdr->cid);

				if (!cmd.get())
				{
					sys_uart.error("Unknown AV cmd: 0x%x", hdr->cid);
					continue;
				}

				const auto cmd_size = cmd->get_size(*this, pkt_storage);

				if (cmd_size != pkt_size && cmd_size)
				{
					write_resp(static_cast<u16>(hdr->cid.get()), PS3AV_STATUS_INVALID_SAMPLE_SIZE);
					return;
				}

				if (!cmd->execute(*this, pkt_storage, sizeof(temp_rx_buf) - temp_rx_buf_size))
				{
					return;
				}
			}
		}();
		// TODO: handle HDMI events
		commit_rx_buf();
	}
}

vuart_av_thread &vuart_av_thread::operator=(thread_state)
{
	tx_wake_c.notify_all();
	return *this;
}

u32 vuart_av_thread::enque_tx_data(const void *data, u32 data_sz)
{
	std::unique_lock<std::mutex> lock(tx_wake_m);
	if (auto size = tx_buf.push(data, data_sz))
	{
		lock.unlock();
		tx_wake_c.notify_all();
		return size;
	}

	return 0;
}

u32 vuart_av_thread::get_tx_bytes()
{
	return tx_buf.get_used_size();
}

u32 vuart_av_thread::read_rx_data(void *data, u32 data_sz)
{
	return rx_buf.pop(data, data_sz);
}

u32 vuart_av_thread::read_tx_data(void *data, u32 data_sz)
{
	std::unique_lock<std::mutex> lock(tx_rdy_m);
	if (auto size = tx_buf.pop(data, data_sz))
	{
		lock.unlock();
		tx_rdy_c.notify_all();
		return size;
	}
	return 0;
}

void vuart_av_thread::write_resp(u32 cid, u32 status, const void *data, u32 data_size)
{
	ps3av_pkt_reply_hdr pkt_hdr;

	pkt_hdr.version = PS3AV_VERSION;
	pkt_hdr.status = status;
	pkt_hdr.length = data_size + 8;
	pkt_hdr.cid = cid | PS3AV_REPLY_BIT;

	const u32 total_size = sizeof(pkt_hdr) + data_size;

	if (temp_rx_buf_size + total_size <= PS3AV_RX_BUF_SIZE)
	{
		memcpy(&temp_rx_buf[temp_rx_buf_size], &pkt_hdr, sizeof(pkt_hdr));
		memcpy(&temp_rx_buf[temp_rx_buf_size + sizeof(pkt_hdr)], data, data_size);
		temp_rx_buf_size += total_size;
	}
}

std::shared_ptr<ps3av_cmd> vuart_av_thread::get_cmd(u32 cid)
{
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
	case PS3AV_CID_AV_ENABLE_EVENT:
	case PS3AV_CID_AV_AUDIO_MUTE:
	case PS3AV_CID_AUDIO_MUTE:
	case PS3AV_CID_AUDIO_MODE:
	case PS3AV_CID_AUDIO_CTRL:
	case PS3AV_CID_AUDIO_ACTIVE:
	case PS3AV_CID_AUDIO_INACTIVE:
	case PS3AV_CID_AUDIO_SPDIF_BIT:
	case PS3AV_CID_AVB_PARAM:
	case PS3AV_CID_AV_INIT:
	case 0xA0002:
		return std::make_shared<GENERIC_REPLY_CMD>();

	case PS3AV_CID_AV_GET_HW_CONF: return std::make_shared<AV_GET_HW_CONF_CMD>();
	case PS3AV_CID_AV_GET_MONITOR_INFO: return std::make_shared<AV_GET_MONITOR_INFO_CMD>();
	default: return {};
	}
}

void vuart_av_thread::commit_rx_buf()
{
	std::unique_lock<std::mutex> lock(rx_wake_m);
	rx_buf.push(temp_rx_buf, temp_rx_buf_size);
	temp_rx_buf_size = 0;

	if (rx_buf.get_used_size())
	{
		lock.unlock();
		rx_wake_c.notify_all();
	}
}
