#include "stdafx.h"
#include "Emu/Memory/vm.h"
#include "Emu/IdManager.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu//Audio/audio_utils.h"
#include "util/video_provider.h"

#include "sys_rsxaudio.h"

#include <cmath>
#include <bitset>
#include <optional>

#ifdef __linux__
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/eventfd.h>
#include <unistd.h>
#elif defined(BSD) || defined(__APPLE__)
#include <unistd.h>
#endif

LOG_CHANNEL(sys_rsxaudio);

extern atomic_t<recording_mode> g_recording_mode;

namespace rsxaudio_ringbuf_reader
{
	static constexpr void clean_buf(rsxaudio_shmem::ringbuf_t& ring_buf)
	{
		ring_buf.unk2             = 100;
		ring_buf.read_idx         = 0;
		ring_buf.write_idx        = 0;
		ring_buf.queue_notify_idx = 0;
		ring_buf.next_blk_idx     = 0;

		for (auto& ring_entry : ring_buf.entries)
		{
			ring_entry.valid         = 0;
			ring_entry.audio_blk_idx = 0;
			ring_entry.timestamp     = 0;
		}
	}

	static void set_timestamp(rsxaudio_shmem::ringbuf_t& ring_buf, u64 timestamp)
	{
		const s32 entry_idx_raw = (ring_buf.read_idx + ring_buf.rw_max_idx - (ring_buf.rw_max_idx > 2) - 1) % ring_buf.rw_max_idx;
		const s32 entry_idx = std::clamp<s32>(entry_idx_raw, 0, SYS_RSXAUDIO_RINGBUF_SZ);

		ring_buf.entries[entry_idx].timestamp = convert_to_timebased_time(timestamp);
	}

	static std::tuple<bool /*notify*/, u64 /*blk_idx*/, u64 /*timestamp*/> update_status(rsxaudio_shmem::ringbuf_t& ring_buf)
	{
		const s32 read_idx = std::clamp<s32>(ring_buf.read_idx, 0, SYS_RSXAUDIO_RINGBUF_SZ);

		if ((ring_buf.entries[read_idx].valid & 1) == 0U)
		{
			return {};
		}

		const s32 entry_idx_raw = (ring_buf.read_idx + ring_buf.rw_max_idx - (ring_buf.rw_max_idx > 2)) % ring_buf.rw_max_idx;
		const s32 entry_idx = std::clamp<s32>(entry_idx_raw, 0, SYS_RSXAUDIO_RINGBUF_SZ);

		ring_buf.entries[read_idx].valid = 0;
		ring_buf.queue_notify_idx = (ring_buf.queue_notify_idx + 1) % ring_buf.queue_notify_step;
		ring_buf.read_idx         = (ring_buf.read_idx + 1) % ring_buf.rw_max_idx;

		return std::make_tuple(((ring_buf.rw_max_idx > 2) ^ ring_buf.queue_notify_idx) == 0, ring_buf.entries[entry_idx].audio_blk_idx, ring_buf.entries[entry_idx].timestamp);
	}

	static std::pair<bool /*entry_valid*/, u32 /*addr*/> get_addr(const rsxaudio_shmem::ringbuf_t& ring_buf)
	{
		const s32 read_idx = std::clamp<s32>(ring_buf.read_idx, 0, SYS_RSXAUDIO_RINGBUF_SZ);

		if (ring_buf.entries[read_idx].valid & 1)
		{
			return std::make_pair(true, ring_buf.entries[read_idx].dma_addr);
		}

		return std::make_pair(false, ring_buf.dma_silence_addr);
	}

	[[maybe_unused]]
	static std::optional<u64> get_spdif_channel_data(RsxaudioPort dst, rsxaudio_shmem& shmem)
	{
		if (dst == RsxaudioPort::SPDIF_0)
		{
			if (shmem.ctrl.spdif_ch0_channel_data_tx_cycles)
			{
				shmem.ctrl.spdif_ch0_channel_data_tx_cycles--;
				return static_cast<u64>(shmem.ctrl.spdif_ch0_channel_data_hi) << 32 | shmem.ctrl.spdif_ch0_channel_data_lo;
			}
		}
		else
		{
			if (shmem.ctrl.spdif_ch1_channel_data_tx_cycles)
			{
				shmem.ctrl.spdif_ch1_channel_data_tx_cycles--;
				return static_cast<u64>(shmem.ctrl.spdif_ch1_channel_data_hi) << 32 | shmem.ctrl.spdif_ch1_channel_data_lo;
			}
		}

		return std::nullopt;
	}
}

lv2_rsxaudio::lv2_rsxaudio(utils::serial& ar) noexcept
	: lv2_obj{1}
	, init(ar)
{
	if (init)
	{
		ar(shmem);

		for (auto& port : event_queue)
		{
			port = lv2_event_queue::load_ptr(ar, port, "rsxaudio");
		}
	}
}

void lv2_rsxaudio::save(utils::serial& ar)
{
	USING_SERIALIZATION_VERSION(LLE);

	ar(init);

	if (init)
	{
		ar(shmem);

		for (const auto& port : event_queue)
		{
			lv2_event_queue::save_ptr(ar, port.get());
		}
	}
}

error_code sys_rsxaudio_initialize(vm::ptr<u32> handle)
{
	sys_rsxaudio.trace("sys_rsxaudio_initialize(handle=*0x%x)", handle);

	auto& rsxaudio_thread = g_fxo->get<rsx_audio_data>();

	if (rsxaudio_thread.rsxaudio_ctx_allocated.test_and_set())
	{
		return CELL_EINVAL;
	}

	if (!vm::check_addr(handle.addr(), vm::page_writable, sizeof(u32)))
	{
		rsxaudio_thread.rsxaudio_ctx_allocated = false;
		return CELL_EFAULT;
	}

	const u32 id = idm::make<lv2_obj, lv2_rsxaudio>();

	if (!id)
	{
		rsxaudio_thread.rsxaudio_ctx_allocated = false;
		return CELL_ENOMEM;
	}

	const auto rsxaudio_obj = idm::get_unlocked<lv2_obj, lv2_rsxaudio>(id);
	std::lock_guard lock(rsxaudio_obj->mutex);

	rsxaudio_obj->shmem = vm::addr_t{vm::alloc(sizeof(rsxaudio_shmem), vm::main)};

	if (!rsxaudio_obj->shmem)
	{
		idm::remove<lv2_obj, lv2_rsxaudio>(id);
		rsxaudio_thread.rsxaudio_ctx_allocated = false;
		return CELL_ENOMEM;
	}

	rsxaudio_obj->page_lock();

	rsxaudio_shmem* sh_page = rsxaudio_obj->get_rw_shared_page();
	sh_page->ctrl = {};

	for (auto& uf : sh_page->ctrl.channel_uf)
	{
		uf.uf_event_cnt = 0;
		uf.unk1         = 0;
	}

	sh_page->ctrl.unk4             = 0x8000;
	sh_page->ctrl.intr_thread_prio = 0xDEADBEEF;
	sh_page->ctrl.unk5             = 0;

	rsxaudio_obj->init = true;
	*handle = id;

	return CELL_OK;
}

error_code sys_rsxaudio_finalize(u32 handle)
{
	sys_rsxaudio.trace("sys_rsxaudio_finalize(handle=0x%x)", handle);

	const auto rsxaudio_obj = idm::get_unlocked<lv2_obj, lv2_rsxaudio>(handle);

	if (!rsxaudio_obj)
	{
		return CELL_ESRCH;
	}

	std::lock_guard lock(rsxaudio_obj->mutex);

	if (!rsxaudio_obj->init)
	{
		return CELL_ESRCH;
	}

	auto& rsxaudio_thread = g_fxo->get<rsx_audio_data>();

	{
		std::lock_guard ra_obj_lock{rsxaudio_thread.rsxaudio_obj_upd_m};
		rsxaudio_thread.rsxaudio_obj_ptr = null_ptr;
	}

	rsxaudio_obj->init = false;
	vm::dealloc(rsxaudio_obj->shmem, vm::main);

	idm::remove<lv2_obj, lv2_rsxaudio>(handle);
	rsxaudio_thread.rsxaudio_ctx_allocated = false;

	return CELL_OK;
}

error_code sys_rsxaudio_import_shared_memory(u32 handle, vm::ptr<u64> addr)
{
	sys_rsxaudio.trace("sys_rsxaudio_import_shared_memory(handle=0x%x, addr=*0x%x)", handle, addr);

	const auto rsxaudio_obj = idm::get_unlocked<lv2_obj, lv2_rsxaudio>(handle);

	if (!rsxaudio_obj)
	{
		return CELL_ESRCH;
	}

	std::lock_guard<shared_mutex> lock(rsxaudio_obj->mutex);

	if (!rsxaudio_obj->init)
	{
		return CELL_ESRCH;
	}

	if (!vm::check_addr(addr.addr(), vm::page_writable, sizeof(u64)))
	{
		return CELL_EFAULT;
	}

	*addr = rsxaudio_obj->shmem;
	rsxaudio_obj->page_unlock();

	return CELL_OK;
}

error_code sys_rsxaudio_unimport_shared_memory(u32 handle, vm::ptr<u64> addr /* unused */)
{
	sys_rsxaudio.trace("sys_rsxaudio_unimport_shared_memory(handle=0x%x, addr=*0x%x)", handle, addr);

	const auto rsxaudio_obj = idm::get_unlocked<lv2_obj, lv2_rsxaudio>(handle);

	if (!rsxaudio_obj)
	{
		return CELL_ESRCH;
	}

	std::lock_guard<shared_mutex> lock(rsxaudio_obj->mutex);

	if (!rsxaudio_obj->init)
	{
		return CELL_ESRCH;
	}

	rsxaudio_obj->page_lock();

	return CELL_OK;
}

error_code sys_rsxaudio_create_connection(u32 handle)
{
	sys_rsxaudio.trace("sys_rsxaudio_create_connection(handle=0x%x)", handle);

	const auto rsxaudio_obj = idm::get_unlocked<lv2_obj, lv2_rsxaudio>(handle);

	if (!rsxaudio_obj)
	{
		return CELL_ESRCH;
	}

	std::lock_guard<shared_mutex> lock(rsxaudio_obj->mutex);

	if (!rsxaudio_obj->init)
	{
		return CELL_ESRCH;
	}

	rsxaudio_shmem* sh_page = rsxaudio_obj->get_rw_shared_page();

	const error_code port_create_status = [&]() -> error_code
	{
		if (auto queue1 = idm::get_unlocked<lv2_obj, lv2_event_queue>(sh_page->ctrl.event_queue_1_id))
		{
			rsxaudio_obj->event_queue[0] = queue1;

			if (auto queue2 = idm::get_unlocked<lv2_obj, lv2_event_queue>(sh_page->ctrl.event_queue_2_id))
			{
				rsxaudio_obj->event_queue[1] = queue2;

				if (auto queue3 = idm::get_unlocked<lv2_obj, lv2_event_queue>(sh_page->ctrl.event_queue_3_id))
				{
					rsxaudio_obj->event_queue[2] = queue3;

					return CELL_OK;
				}
			}
		}

		return CELL_ESRCH;
	}();

	if (port_create_status != CELL_OK)
	{
		return port_create_status;
	}

	for (auto& rb : sh_page->ctrl.ringbuf)
	{
		rb.dma_silence_addr = rsxaudio_obj->dma_io_base + offsetof(rsxaudio_shmem, dma_silence_region);
		rb.unk2             = 100;
	}

	for (u32 entry_idx = 0; entry_idx < SYS_RSXAUDIO_RINGBUF_SZ; entry_idx++)
	{
		sh_page->ctrl.ringbuf[static_cast<u32>(RsxaudioPort::SERIAL)].entries[entry_idx].dma_addr  = rsxaudio_obj->dma_io_base + u32{offsetof(rsxaudio_shmem, dma_serial_region)} + SYS_RSXAUDIO_RINGBUF_BLK_SZ_SERIAL * entry_idx;
		sh_page->ctrl.ringbuf[static_cast<u32>(RsxaudioPort::SPDIF_0)].entries[entry_idx].dma_addr = rsxaudio_obj->dma_io_base + u32{offsetof(rsxaudio_shmem, dma_spdif_0_region)} + SYS_RSXAUDIO_RINGBUF_BLK_SZ_SPDIF * entry_idx;
		sh_page->ctrl.ringbuf[static_cast<u32>(RsxaudioPort::SPDIF_1)].entries[entry_idx].dma_addr = rsxaudio_obj->dma_io_base + u32{offsetof(rsxaudio_shmem, dma_spdif_1_region)} + SYS_RSXAUDIO_RINGBUF_BLK_SZ_SPDIF * entry_idx;
	}

	return CELL_OK;
}

error_code sys_rsxaudio_close_connection(u32 handle)
{
	sys_rsxaudio.trace("sys_rsxaudio_close_connection(handle=0x%x)", handle);

	const auto rsxaudio_obj = idm::get_unlocked<lv2_obj, lv2_rsxaudio>(handle);

	if (!rsxaudio_obj)
	{
		return CELL_ESRCH;
	}

	std::lock_guard<shared_mutex> lock(rsxaudio_obj->mutex);

	if (!rsxaudio_obj->init)
	{
		return CELL_ESRCH;
	}

	{
		auto& rsxaudio_thread = g_fxo->get<rsx_audio_data>();
		std::lock_guard ra_obj_lock{rsxaudio_thread.rsxaudio_obj_upd_m};
		rsxaudio_thread.rsxaudio_obj_ptr = null_ptr;
	}

	for (u32 q_idx = 0; q_idx < SYS_RSXAUDIO_PORT_CNT; q_idx++)
	{
		rsxaudio_obj->event_queue[q_idx].reset();
	}

	return CELL_OK;
}

error_code sys_rsxaudio_prepare_process(u32 handle)
{
	sys_rsxaudio.trace("sys_rsxaudio_prepare_process(handle=0x%x)", handle);

	const auto rsxaudio_obj = idm::get_unlocked<lv2_obj, lv2_rsxaudio>(handle);

	if (!rsxaudio_obj)
	{
		return CELL_ESRCH;
	}

	std::lock_guard<shared_mutex> lock(rsxaudio_obj->mutex);

	if (!rsxaudio_obj->init)
	{
		return CELL_ESRCH;
	}

	auto& rsxaudio_thread = g_fxo->get<rsx_audio_data>();
	std::lock_guard ra_obj_lock{rsxaudio_thread.rsxaudio_obj_upd_m};

	if (rsxaudio_thread.rsxaudio_obj_ptr)
	{
		return -1;
	}

	rsxaudio_thread.rsxaudio_obj_ptr = rsxaudio_obj;

	return CELL_OK;
}

error_code sys_rsxaudio_start_process(u32 handle)
{
	sys_rsxaudio.trace("sys_rsxaudio_start_process(handle=0x%x)", handle);

	const auto rsxaudio_obj = idm::get_unlocked<lv2_obj, lv2_rsxaudio>(handle);

	if (!rsxaudio_obj)
	{
		return CELL_ESRCH;
	}

	std::lock_guard<shared_mutex> lock(rsxaudio_obj->mutex);

	if (!rsxaudio_obj->init)
	{
		return CELL_ESRCH;
	}

	rsxaudio_shmem* sh_page = rsxaudio_obj->get_rw_shared_page();

	for (auto& rb : sh_page->ctrl.ringbuf)
	{
		if (rb.active) rsxaudio_ringbuf_reader::clean_buf(rb);
	}

	for (auto& uf : sh_page->ctrl.channel_uf)
	{
		uf.uf_event_cnt = 0;
		uf.unk1         = 0;
	}

	auto& rsxaudio_thread = g_fxo->get<rsx_audio_data>();
	rsxaudio_thread.update_hw_param([&](auto& param)
	{
		if (sh_page->ctrl.ringbuf[static_cast<u32>(RsxaudioPort::SERIAL)].active)  param.serial.dma_en   = true;
		if (sh_page->ctrl.ringbuf[static_cast<u32>(RsxaudioPort::SPDIF_0)].active) param.spdif[0].dma_en = true;
		if (sh_page->ctrl.ringbuf[static_cast<u32>(RsxaudioPort::SPDIF_1)].active) param.spdif[1].dma_en = true;
	});

	for (u32 q_idx = 0; q_idx < SYS_RSXAUDIO_PORT_CNT; q_idx++)
	{
		if (const auto& queue = rsxaudio_obj->event_queue[q_idx]; queue && sh_page->ctrl.ringbuf[q_idx].active)
		{
			queue->send(rsxaudio_obj->event_port_name[q_idx], q_idx, 0, 0);
		}
	}

	return CELL_OK;
}

error_code sys_rsxaudio_stop_process(u32 handle)
{
	sys_rsxaudio.trace("sys_rsxaudio_stop_process(handle=0x%x)", handle);

	const auto rsxaudio_obj = idm::get_unlocked<lv2_obj, lv2_rsxaudio>(handle);

	if (!rsxaudio_obj)
	{
		return CELL_ESRCH;
	}

	std::lock_guard<shared_mutex> lock(rsxaudio_obj->mutex);

	if (!rsxaudio_obj->init)
	{
		return CELL_ESRCH;
	}

	auto& rsxaudio_thread = g_fxo->get<rsx_audio_data>();

	rsxaudio_thread.update_hw_param([&](auto& param)
	{
		param.serial.dma_en  = false;
		param.serial.muted   = true;
		param.serial.en      = false;

		for (auto& spdif : param.spdif)
		{
			spdif.dma_en = false;
			if (!spdif.use_serial_buf)
			{
				spdif.en = false;
			}
		}

		param.spdif[1].muted  = true;
	});

	rsxaudio_shmem* sh_page = rsxaudio_obj->get_rw_shared_page();

	for (auto& rb : sh_page->ctrl.ringbuf)
	{
		if (rb.active) rsxaudio_ringbuf_reader::clean_buf(rb);
	}

	return CELL_OK;
}

error_code sys_rsxaudio_get_dma_param(u32 handle, u32 flag, vm::ptr<u64> out)
{
	sys_rsxaudio.trace("sys_rsxaudio_get_dma_param(handle=0x%x, flag=0x%x, out=0x%x)", handle, flag, out);

	const auto rsxaudio_obj = idm::get_unlocked<lv2_obj, lv2_rsxaudio>(handle);

	if (!rsxaudio_obj)
	{
		return CELL_ESRCH;
	}

	std::lock_guard lock(rsxaudio_obj->mutex);

	if (!rsxaudio_obj->init)
	{
		return CELL_ESRCH;
	}

	if (!vm::check_addr(out.addr(), vm::page_writable, sizeof(u64)))
	{
		return CELL_EFAULT;
	}

	if (flag == rsxaudio_dma_flag::IO_ID)
	{
		*out = rsxaudio_obj->dma_io_id;
	}
	else if (flag == rsxaudio_dma_flag::IO_BASE)
	{
		*out = rsxaudio_obj->dma_io_base;
	}

	return CELL_OK;
}

rsxaudio_data_container::rsxaudio_data_container(const rsxaudio_hw_param_t& hw_param, const buf_t& buf, bool serial_rdy, bool spdif_0_rdy, bool spdif_1_rdy) : hwp(hw_param), out_buf(buf)
{
	if (serial_rdy)
	{
		avport_data_avail[static_cast<u8>(RsxaudioAvportIdx::AVMULTI)] = true;

		if (hwp.spdif[0].use_serial_buf)
		{
			avport_data_avail[static_cast<u8>(RsxaudioAvportIdx::SPDIF_0)] = true;
		}

		if (hwp.spdif[1].use_serial_buf)
		{
			avport_data_avail[static_cast<u8>(RsxaudioAvportIdx::SPDIF_1)] = true;
		}
	}

	if (spdif_0_rdy && !hwp.spdif[0].use_serial_buf)
	{
		avport_data_avail[static_cast<u8>(RsxaudioAvportIdx::SPDIF_0)] = true;
	}

	if (spdif_1_rdy && !hwp.spdif[1].use_serial_buf)
	{
		avport_data_avail[static_cast<u8>(RsxaudioAvportIdx::SPDIF_1)] = true;
	}

	if (hwp.hdmi[0].init)
	{
		if (hwp.hdmi[0].use_spdif_1)
		{
			avport_data_avail[static_cast<u8>(RsxaudioAvportIdx::HDMI_0)] = avport_data_avail[static_cast<u8>(RsxaudioAvportIdx::SPDIF_1)];
		}
		else
		{
			avport_data_avail[static_cast<u8>(RsxaudioAvportIdx::HDMI_0)] = serial_rdy;
		}

		hdmi_stream_cnt[0] = static_cast<u8>(hwp.hdmi[0].ch_cfg.total_ch_cnt) / SYS_RSXAUDIO_CH_PER_STREAM;
	}

	if (hwp.hdmi[1].init)
	{
		if (hwp.hdmi[1].use_spdif_1)
		{
			avport_data_avail[static_cast<u8>(RsxaudioAvportIdx::HDMI_1)] = avport_data_avail[static_cast<u8>(RsxaudioAvportIdx::SPDIF_1)];
		}
		else
		{
			avport_data_avail[static_cast<u8>(RsxaudioAvportIdx::HDMI_1)] = serial_rdy;
		}

		hdmi_stream_cnt[1] = static_cast<u8>(hwp.hdmi[1].ch_cfg.total_ch_cnt) / SYS_RSXAUDIO_CH_PER_STREAM;
	}
}

u32 rsxaudio_data_container::get_data_size(RsxaudioAvportIdx avport)
{
	if (!avport_data_avail[static_cast<u8>(avport)])
	{
		return 0;
	}

	switch (avport)
	{
	case RsxaudioAvportIdx::HDMI_0:
	{
		const RsxaudioSampleSize depth = hwp.hdmi[0].use_spdif_1 ? hwp.spdif[1].depth : hwp.serial.depth;

		return (depth == RsxaudioSampleSize::_16BIT ? SYS_RSXAUDIO_STREAM_SIZE * 2 : SYS_RSXAUDIO_STREAM_SIZE) * hdmi_stream_cnt[0];
	}
	case RsxaudioAvportIdx::HDMI_1:
	{
		const RsxaudioSampleSize depth = hwp.hdmi[1].use_spdif_1 ? hwp.spdif[1].depth : hwp.serial.depth;

		return (depth == RsxaudioSampleSize::_16BIT ? SYS_RSXAUDIO_STREAM_SIZE * 2 : SYS_RSXAUDIO_STREAM_SIZE) * hdmi_stream_cnt[1];
	}
	case RsxaudioAvportIdx::AVMULTI:
	{
		return hwp.serial.depth == RsxaudioSampleSize::_16BIT ? SYS_RSXAUDIO_STREAM_SIZE * 2 : SYS_RSXAUDIO_STREAM_SIZE;
	}
	case RsxaudioAvportIdx::SPDIF_0:
	{
		const RsxaudioSampleSize depth = hwp.spdif[0].use_serial_buf ? hwp.serial.depth : hwp.spdif[0].depth;

		return depth == RsxaudioSampleSize::_16BIT ? SYS_RSXAUDIO_STREAM_SIZE * 2 : SYS_RSXAUDIO_STREAM_SIZE;
	}
	case RsxaudioAvportIdx::SPDIF_1:
	{
		const RsxaudioSampleSize depth = hwp.spdif[1].use_serial_buf ? hwp.serial.depth : hwp.spdif[1].depth;

		return depth == RsxaudioSampleSize::_16BIT ? SYS_RSXAUDIO_STREAM_SIZE * 2 : SYS_RSXAUDIO_STREAM_SIZE;
	}
	default:
	{
		return 0;
	}
	}
}

void rsxaudio_data_container::get_data(RsxaudioAvportIdx avport, data_blk_t& data_out)
{
	if (!avport_data_avail[static_cast<u8>(avport)])
	{
		return;
	}

	data_was_written = true;

	auto spdif_filter_map = [&](u8 hdmi_idx)
	{
		std::array<u8, SYS_RSXAUDIO_SERIAL_MAX_CH> result;

		for (u64 i = 0; i < SYS_RSXAUDIO_SERIAL_MAX_CH; i++)
		{
			const u8 old_val = hwp.hdmi[hdmi_idx].ch_cfg.map[i];
			result[i] = old_val >= SYS_RSXAUDIO_SPDIF_MAX_CH ? rsxaudio_hw_param_t::hdmi_param_t::MAP_SILENT_CH : old_val;
		}

		return result;
	};

	switch (avport)
	{
	case RsxaudioAvportIdx::HDMI_0:
	case RsxaudioAvportIdx::HDMI_1:
	{
		const u8 hdmi_idx = avport == RsxaudioAvportIdx::HDMI_1;

		switch (hdmi_stream_cnt[hdmi_idx])
		{
		default:
		case 0:
		{
			return;
		}
		case 1:
		{
			if (hwp.hdmi[hdmi_idx].use_spdif_1)
			{
				if (hwp.spdif[1].use_serial_buf)
				{
					mix<2>(spdif_filter_map(hdmi_idx), hwp.serial.depth, out_buf.serial, data_out);
				}
				else
				{
					mix<2>(hwp.hdmi[hdmi_idx].ch_cfg.map, hwp.spdif[1].depth, out_buf.spdif[1], data_out);
				}
			}
			else
			{
				mix<2>(hwp.hdmi[hdmi_idx].ch_cfg.map, hwp.serial.depth, out_buf.serial, data_out);
			}
			break;
		}
		case 3:
		{
			if (hwp.hdmi[hdmi_idx].use_spdif_1)
			{
				if (hwp.spdif[1].use_serial_buf)
				{
					mix<6>(spdif_filter_map(hdmi_idx), hwp.serial.depth, out_buf.serial, data_out);
				}
				else
				{
					mix<6>(hwp.hdmi[hdmi_idx].ch_cfg.map, hwp.spdif[1].depth, out_buf.spdif[1], data_out);
				}
			}
			else
			{
				mix<6>(hwp.hdmi[hdmi_idx].ch_cfg.map, hwp.serial.depth, out_buf.serial, data_out);
			}
			break;
		}
		case 4:
		{
			if (hwp.hdmi[hdmi_idx].use_spdif_1)
			{
				if (hwp.spdif[1].use_serial_buf)
				{
					mix<8>(spdif_filter_map(hdmi_idx), hwp.serial.depth, out_buf.serial, data_out);
				}
				else
				{
					mix<8>(hwp.hdmi[hdmi_idx].ch_cfg.map, hwp.spdif[1].depth, out_buf.spdif[1], data_out);
				}
			}
			else
			{
				mix<8>(hwp.hdmi[hdmi_idx].ch_cfg.map, hwp.serial.depth, out_buf.serial, data_out);
			}
			break;
		}
		}

		break;
	}
	case RsxaudioAvportIdx::AVMULTI:
	{
		mix<2>({2, 3}, hwp.serial.depth, out_buf.serial, data_out);
		break;
	}
	case RsxaudioAvportIdx::SPDIF_0:
	case RsxaudioAvportIdx::SPDIF_1:
	{
		const u8 spdif_idx = avport == RsxaudioAvportIdx::SPDIF_1;

		if (hwp.spdif[spdif_idx].use_serial_buf)
		{
			mix<2>({0, 1}, hwp.serial.depth, out_buf.serial, data_out);
		}
		else
		{
			mix<2>({0, 1}, hwp.spdif[spdif_idx].depth, out_buf.spdif[spdif_idx], data_out);
		}
		break;
	}
	default:
	{
		return;
	}
	}
}

bool rsxaudio_data_container::data_was_used()
{
	return data_was_written;
}

rsxaudio_data_thread::rsxaudio_data_thread() {}

void rsxaudio_data_thread::operator()()
{
	thread_ctrl::scoped_priority high_prio(+1);

	while (thread_ctrl::state() != thread_state::aborting)
	{
		static const std::function<void()> tmr_callback = [this]() { extract_audio_data(); };

		switch (timer.wait(tmr_callback))
		{
		case rsxaudio_periodic_tmr::wait_result::SUCCESS:
		case rsxaudio_periodic_tmr::wait_result::TIMEOUT:
		case rsxaudio_periodic_tmr::wait_result::TIMER_CANCELED:
		{
			continue;
		}
		case rsxaudio_periodic_tmr::wait_result::INVALID_PARAM:
		case rsxaudio_periodic_tmr::wait_result::TIMER_ERROR:
		default:
		{
			fmt::throw_exception("rsxaudio_periodic_tmr::wait() failed");
		}
		}
	}
}

rsxaudio_data_thread& rsxaudio_data_thread::operator=(thread_state /* state */)
{
	timer.cancel_wait();
	return *this;
}

void rsxaudio_data_thread::advance_all_timers()
{
	const u64 crnt_time = get_system_time();

	timer.vtimer_skip_periods(static_cast<u32>(RsxaudioPort::SERIAL), crnt_time);
	timer.vtimer_skip_periods(static_cast<u32>(RsxaudioPort::SPDIF_0), crnt_time);
	timer.vtimer_skip_periods(static_cast<u32>(RsxaudioPort::SPDIF_1), crnt_time);
}

void rsxaudio_data_thread::extract_audio_data()
{
	// Accessing timer state is safe here, since we're in timer::wait()

	const auto rsxaudio_obj = [&]()
	{
		std::lock_guard ra_obj_lock{rsxaudio_obj_upd_m};
		return rsxaudio_obj_ptr;
	}();

	if (Emu.IsPausedOrReady() || !rsxaudio_obj)
	{
		advance_all_timers();
		return;
	}

	std::lock_guard<shared_mutex> rsxaudio_lock(rsxaudio_obj->mutex);

	if (!rsxaudio_obj->init)
	{
		advance_all_timers();
		return;
	}

	rsxaudio_shmem* sh_page = rsxaudio_obj->get_rw_shared_page();
	const auto hw_cfg       = hw_param_ts.get_current();
	const u64 crnt_time     = get_system_time();

	auto process_rb = [&](RsxaudioPort dst, bool dma_en)
	{
		// SPDIF channel data and underflow events are always disabled by lv1

		const u32 dst_raw = static_cast<u32>(dst);
		rsxaudio_ringbuf_reader::set_timestamp(sh_page->ctrl.ringbuf[dst_raw], timer.vtimer_get_sched_time(dst_raw));

		const auto [data_present, rb_addr] = get_ringbuf_addr(dst, *rsxaudio_obj);
		bool reset_periods = !enqueue_data(dst, rb_addr == nullptr, rb_addr, *hw_cfg);

		if (dma_en)
		{
			if (const auto [notify, blk_idx, timestamp] = rsxaudio_ringbuf_reader::update_status(sh_page->ctrl.ringbuf[dst_raw]); notify)
			{
				// Too late to recover
				reset_periods = true;

				if (const auto& queue = rsxaudio_obj->event_queue[dst_raw])
				{
					queue->send(rsxaudio_obj->event_port_name[dst_raw], dst_raw, blk_idx, timestamp);
				}
			}
		}

		if (reset_periods)
		{
			timer.vtimer_skip_periods(dst_raw, crnt_time);
		}
		else
		{
			timer.vtimer_incr(dst_raw, crnt_time);
		}
	};

	if (timer.is_vtimer_behind(static_cast<u32>(RsxaudioPort::SERIAL), crnt_time))
	{
		process_rb(RsxaudioPort::SERIAL, hw_cfg->serial.dma_en);
	}

	if (timer.is_vtimer_behind(static_cast<u32>(RsxaudioPort::SPDIF_0), crnt_time))
	{
		process_rb(RsxaudioPort::SPDIF_0, hw_cfg->spdif[0].dma_en);
	}

	if (timer.is_vtimer_behind(static_cast<u32>(RsxaudioPort::SPDIF_1), crnt_time))
	{
		process_rb(RsxaudioPort::SPDIF_1, hw_cfg->spdif[1].dma_en);
	}
}

std::pair<bool, void*> rsxaudio_data_thread::get_ringbuf_addr(RsxaudioPort dst, const lv2_rsxaudio& rsxaudio_obj)
{
	ensure(dst <= RsxaudioPort::SPDIF_1);

	rsxaudio_shmem* sh_page = rsxaudio_obj.get_rw_shared_page();
	const auto [data_present, addr] = rsxaudio_ringbuf_reader::get_addr(sh_page->ctrl.ringbuf[static_cast<u32>(dst)]);
	const u32 buf_size = dst == RsxaudioPort::SERIAL ? SYS_RSXAUDIO_RINGBUF_BLK_SZ_SERIAL : SYS_RSXAUDIO_RINGBUF_BLK_SZ_SPDIF;

	if (addr >= rsxaudio_obj.dma_io_base && addr < rsxaudio_obj.dma_io_base + sizeof(rsxaudio_shmem) - buf_size)
	{
		return std::make_pair(data_present, reinterpret_cast<u8*>(rsxaudio_obj.get_rw_shared_page()) + addr - rsxaudio_obj.dma_io_base);
	}

	// Buffer address is invalid
	return std::make_pair(false, nullptr);
}

void rsxaudio_data_thread::reset_hw()
{
	update_hw_param([&](rsxaudio_hw_param_t& current)
	{
		const bool serial_dma_en = current.serial.dma_en;
		current.serial = {};
		current.serial.dma_en = serial_dma_en;

		for (auto& spdif : current.spdif)
		{
			const bool spdif_dma_en = spdif.dma_en;
			spdif = {};
			spdif.dma_en = spdif_dma_en;
		}

		current.serial_freq_base = SYS_RSXAUDIO_FREQ_BASE_384K;
		current.spdif_freq_base = SYS_RSXAUDIO_FREQ_BASE_352K;
		current.avport_src.fill(RsxaudioPort::INVALID);
	});
}

void rsxaudio_data_thread::update_hw_param(std::function<void(rsxaudio_hw_param_t&)> update_callback)
{
	ensure(update_callback);

	hw_param_ts.add_op([&]()
	{
		auto new_hw_param = std::make_shared<rsxaudio_hw_param_t>(*hw_param_ts.get_current());

		update_callback(*new_hw_param);

		const bool serial_active = calc_port_active_state(RsxaudioPort::SERIAL, *new_hw_param);
		const bool spdif_active[SYS_RSXAUDIO_SPDIF_CNT] =
		{
			calc_port_active_state(RsxaudioPort::SPDIF_0, *new_hw_param),
			calc_port_active_state(RsxaudioPort::SPDIF_1, *new_hw_param)
		};

		std::array<rsxaudio_backend_thread::port_config, SYS_RSXAUDIO_AVPORT_CNT> port_cfg{};
		port_cfg[static_cast<u8>(RsxaudioAvportIdx::AVMULTI)] = {static_cast<AudioFreq>(new_hw_param->serial_freq_base / new_hw_param->serial.freq_div), AudioChannelCnt::STEREO};

		auto gen_spdif_port_cfg = [&](u8 spdif_idx)
		{
			if (new_hw_param->spdif[spdif_idx].use_serial_buf)
			{
				return port_cfg[static_cast<u8>(RsxaudioAvportIdx::AVMULTI)];
			}

			return rsxaudio_backend_thread::port_config{static_cast<AudioFreq>(new_hw_param->spdif_freq_base / new_hw_param->spdif[spdif_idx].freq_div), AudioChannelCnt::STEREO};
		};

		port_cfg[static_cast<u8>(RsxaudioAvportIdx::SPDIF_0)] = gen_spdif_port_cfg(0);
		port_cfg[static_cast<u8>(RsxaudioAvportIdx::SPDIF_1)] = gen_spdif_port_cfg(1);

		auto gen_hdmi_port_cfg = [&](u8 hdmi_idx)
		{
			if (new_hw_param->hdmi[hdmi_idx].use_spdif_1)
			{
				return rsxaudio_backend_thread::port_config{port_cfg[static_cast<u8>(RsxaudioAvportIdx::SPDIF_1)].freq, new_hw_param->hdmi[hdmi_idx].ch_cfg.total_ch_cnt};
			}

			return rsxaudio_backend_thread::port_config{port_cfg[static_cast<u8>(RsxaudioAvportIdx::AVMULTI)].freq, new_hw_param->hdmi[hdmi_idx].ch_cfg.total_ch_cnt};
		};

		port_cfg[static_cast<u8>(RsxaudioAvportIdx::HDMI_0)] = gen_hdmi_port_cfg(0);
		port_cfg[static_cast<u8>(RsxaudioAvportIdx::HDMI_1)] = gen_hdmi_port_cfg(1);
		// TODO: ideally, old data must be flushed from backend buffers if channel became inactive or its src changed
		g_fxo->get<rsx_audio_backend>().set_new_stream_param(port_cfg, calc_avport_mute_state(*new_hw_param));

		timer.vtimer_access_sec([&]()
		{
			const u64 crnt_time = get_system_time();

			if (serial_active)
			{
				// 2 channels per stream, streams go in parallel
				const u32 new_timer_rate = static_cast<u32>(port_cfg[static_cast<u8>(RsxaudioAvportIdx::AVMULTI)].freq) *
											static_cast<u8>(new_hw_param->serial.depth) *
											SYS_RSXAUDIO_CH_PER_STREAM;

				timer.enable_vtimer(static_cast<u32>(RsxaudioPort::SERIAL), new_timer_rate, crnt_time);
			}
			else
			{
				timer.disable_vtimer(static_cast<u32>(RsxaudioPort::SERIAL));
			}

			for (u8 spdif_idx = 0; spdif_idx < SYS_RSXAUDIO_SPDIF_CNT; spdif_idx++)
			{
				const u32 vtimer_id = static_cast<u32>(RsxaudioPort::SPDIF_0) + spdif_idx;

				if (spdif_active[spdif_idx] && !new_hw_param->spdif[spdif_idx].use_serial_buf)
				{
					// 2 channels per stream, single stream
					const u32 new_timer_rate = static_cast<u32>(port_cfg[static_cast<u8>(RsxaudioAvportIdx::SPDIF_0) + spdif_idx].freq) *
												static_cast<u8>(new_hw_param->spdif[spdif_idx].depth) *
												SYS_RSXAUDIO_CH_PER_STREAM;

					timer.enable_vtimer(vtimer_id, new_timer_rate, crnt_time);
				}
				else
				{
					timer.disable_vtimer(vtimer_id);
				}
			}
		});

		return new_hw_param;
	});
}

void rsxaudio_data_thread::update_mute_state(RsxaudioPort port, bool muted)
{
	hw_param_ts.add_op([&]()
	{
		auto new_hw_param = std::make_shared<rsxaudio_hw_param_t>(*hw_param_ts.get_current());

		switch (port)
		{
		case RsxaudioPort::SERIAL:
		{
			new_hw_param->serial.muted = muted;
			break;
		}
		case RsxaudioPort::SPDIF_0:
		{
			new_hw_param->spdif[0].muted = muted;
			break;
		}
		case RsxaudioPort::SPDIF_1:
		{
			new_hw_param->spdif[1].muted = muted;
			break;
		}
		default:
		{
			fmt::throw_exception("Invalid RSXAudio port: %u", static_cast<u8>(port));
		}
		}

		g_fxo->get<rsx_audio_backend>().set_mute_state(calc_avport_mute_state(*new_hw_param));

		return new_hw_param;
	});
}

void rsxaudio_data_thread::update_av_mute_state(RsxaudioAvportIdx avport, bool muted, bool force_mute, bool set)
{
	hw_param_ts.add_op([&]()
	{
		auto new_hw_param = std::make_shared<rsxaudio_hw_param_t>(*hw_param_ts.get_current());

		switch (avport)
		{
		case RsxaudioAvportIdx::HDMI_0:
		case RsxaudioAvportIdx::HDMI_1:
		{
			const u32 hdmi_idx = avport == RsxaudioAvportIdx::HDMI_1;

			if (muted)
			{
				new_hw_param->hdmi[hdmi_idx].muted = set;
			}

			if (force_mute)
			{
				new_hw_param->hdmi[hdmi_idx].force_mute = set;
			}
			break;
		}
		case RsxaudioAvportIdx::AVMULTI:
		{
			if (muted)
			{
				new_hw_param->avmulti_av_muted = set;
			}
			break;
		}
		default:
		{
			fmt::throw_exception("Invalid RSXAudio avport: %u", static_cast<u8>(avport));
		}
		}

		g_fxo->get<rsx_audio_backend>().set_mute_state(calc_avport_mute_state(*new_hw_param));

		return new_hw_param;
	});
}

rsxaudio_backend_thread::avport_bit rsxaudio_data_thread::calc_avport_mute_state(const rsxaudio_hw_param_t& hwp)
{
	const bool serial_active = calc_port_active_state(RsxaudioPort::SERIAL, hwp);

	const bool spdif_active[SYS_RSXAUDIO_SPDIF_CNT] =
	{
		calc_port_active_state(RsxaudioPort::SPDIF_0, hwp),
		calc_port_active_state(RsxaudioPort::SPDIF_1, hwp)
	};

	const bool avmulti = !serial_active || hwp.serial.muted || hwp.avmulti_av_muted;

	auto spdif_muted = [&](u8 spdif_idx)
	{
		const u8 spdif_port = spdif_idx == 1;

		if (hwp.spdif[spdif_port].use_serial_buf)
		{
			// TODO: HW test if both serial and spdif mutes are used in serial mode for spdif
			return !serial_active || hwp.spdif[spdif_port].freq_div != hwp.serial.freq_div || hwp.serial.muted || hwp.spdif[spdif_port].muted;
		}

		return !spdif_active[spdif_port] || hwp.spdif[spdif_port].muted;
	};

	auto hdmi_muted  = [&](u8 hdmi_idx)
	{
		const u8 hdmi_port = hdmi_idx == 1;

		if (hwp.hdmi[hdmi_idx].use_spdif_1)
		{
			return spdif_muted(1) || hwp.hdmi[hdmi_port].muted || hwp.hdmi[hdmi_port].force_mute || !hwp.hdmi[hdmi_port].init;
		}

		return !serial_active || hwp.serial.muted || hwp.hdmi[hdmi_port].muted || hwp.hdmi[hdmi_port].force_mute || !hwp.hdmi[hdmi_port].init;
	};

	return { hdmi_muted(0), hdmi_muted(1), avmulti, spdif_muted(0), spdif_muted(1) };
}

bool rsxaudio_data_thread::calc_port_active_state(RsxaudioPort port, const rsxaudio_hw_param_t& hwp)
{
	auto gen_serial_active = [&]()
	{
		return hwp.serial.dma_en && hwp.serial.buf_empty_en && hwp.serial.en;
	};

	auto gen_spdif_active = [&](u8 spdif_idx)
	{
		if (hwp.spdif[spdif_idx].use_serial_buf)
		{
			return gen_serial_active() && (hwp.spdif[spdif_idx].freq_div == hwp.serial.freq_div);
		}

		return hwp.spdif[spdif_idx].dma_en && hwp.spdif[spdif_idx].buf_empty_en && hwp.spdif[spdif_idx].en;
	};

	switch (port)
	{
	case RsxaudioPort::SERIAL:
	{
		return gen_serial_active();
	}
	case RsxaudioPort::SPDIF_0:
	{
		return gen_spdif_active(0);
	}
	case RsxaudioPort::SPDIF_1:
	{
		return gen_spdif_active(1);
	}
	default:
	{
		return false;
	}
	}
}

f32 rsxaudio_data_thread::pcm_to_float(s32 sample)
{
	return sample * (1.0f / 2147483648.0f);
}

f32 rsxaudio_data_thread::pcm_to_float(s16 sample)
{
	return sample * (1.0f / 32768.0f);
}

void rsxaudio_data_thread::pcm_serial_process_channel(RsxaudioSampleSize word_bits, ra_stream_blk_t& buf_out_l, ra_stream_blk_t& buf_out_r, const void* buf_in, u8 src_stream)
{
	const u8 input_word_sz = static_cast<u8>(word_bits);
	u64 ch_dst = 0;

	for (u64 blk_idx = 0; blk_idx < SYS_RSXAUDIO_STREAM_DATA_BLK_CNT; blk_idx++)
	{
		for (u64 offset = 0; offset < SYS_RSXAUDIO_DATA_BLK_SIZE / 2; offset += input_word_sz, ch_dst++)
		{
			const u64 left_ch_src = (blk_idx * SYS_RSXAUDIO_STREAM_SIZE + src_stream * SYS_RSXAUDIO_DATA_BLK_SIZE + offset) / input_word_sz;
			const u64 right_ch_src = left_ch_src + (SYS_RSXAUDIO_DATA_BLK_SIZE / 2) / input_word_sz;

			if (word_bits == RsxaudioSampleSize::_16BIT)
			{
				buf_out_l[ch_dst] = pcm_to_float(static_cast<const be_t<s16>*>(buf_in)[left_ch_src]);
				buf_out_r[ch_dst] = pcm_to_float(static_cast<const be_t<s16>*>(buf_in)[right_ch_src]);
			}
			else
			{
				// Looks like rsx treats 20bit/24bit samples as 32bit ones
				buf_out_l[ch_dst] = pcm_to_float(static_cast<const be_t<s32>*>(buf_in)[left_ch_src]);
				buf_out_r[ch_dst] = pcm_to_float(static_cast<const be_t<s32>*>(buf_in)[right_ch_src]);
			}
		}
	}
}

void rsxaudio_data_thread::pcm_spdif_process_channel(RsxaudioSampleSize word_bits, ra_stream_blk_t& buf_out_l, ra_stream_blk_t& buf_out_r, const void* buf_in)
{
	const u8 input_word_sz = static_cast<u8>(word_bits);

	for (u64 offset = 0; offset < SYS_RSXAUDIO_RINGBUF_BLK_SZ_SPDIF / (input_word_sz * SYS_RSXAUDIO_SPDIF_MAX_CH); offset++)
	{
		const u64 left_ch_src = offset * SYS_RSXAUDIO_SPDIF_MAX_CH;
		const u64 right_ch_src = left_ch_src + 1;

		if (word_bits == RsxaudioSampleSize::_16BIT)
		{
			buf_out_l[offset] = pcm_to_float(static_cast<const be_t<s16>*>(buf_in)[left_ch_src]);
			buf_out_r[offset] = pcm_to_float(static_cast<const be_t<s16>*>(buf_in)[right_ch_src]);
		}
		else
		{
			// Looks like rsx treats 20bit/24bit samples as 32bit ones
			buf_out_l[offset] = pcm_to_float(static_cast<const be_t<s32>*>(buf_in)[left_ch_src]);
			buf_out_r[offset] = pcm_to_float(static_cast<const be_t<s32>*>(buf_in)[right_ch_src]);
		}
	}
}

bool rsxaudio_data_thread::enqueue_data(RsxaudioPort dst, bool silence, const void* src_addr, const rsxaudio_hw_param_t& hwp)
{
	auto& backend_thread = g_fxo->get<rsx_audio_backend>();

	if (dst == RsxaudioPort::SERIAL)
	{
		if (!silence)
		{
			for (u8 stream_idx = 0; stream_idx < SYS_RSXAUDIO_SERIAL_STREAM_CNT; stream_idx++)
			{
				pcm_serial_process_channel(hwp.serial.depth, output_buf.serial[stream_idx * 2], output_buf.serial[stream_idx * 2 + 1], src_addr, stream_idx);
			}
		}
		else
		{
			output_buf.serial.fill({});
		}

		rsxaudio_data_container cont{hwp, output_buf, true, false, false};
		backend_thread.add_data(cont);
		return cont.data_was_used();
	}
	else if (dst == RsxaudioPort::SPDIF_0)
	{
		if (!silence)
		{
			pcm_spdif_process_channel(hwp.spdif[0].depth, output_buf.spdif[0][0], output_buf.spdif[0][1], src_addr);
		}
		else
		{
			output_buf.spdif[0].fill({});
		}

		rsxaudio_data_container cont{hwp, output_buf, false, true, false};
		backend_thread.add_data(cont);
		return cont.data_was_used();
	}
	else if (dst == RsxaudioPort::SPDIF_1)
	{
		if (!silence)
		{
			pcm_spdif_process_channel(hwp.spdif[1].depth, output_buf.spdif[1][0], output_buf.spdif[1][1], src_addr);
		}
		else
		{
			output_buf.spdif[1].fill({});
		}

		rsxaudio_data_container cont{hwp, output_buf, false, false, true};
		backend_thread.add_data(cont);
		return cont.data_was_used();
	}

	return false;
}

namespace audio
{
	extern void configure_rsxaudio()
	{
		if (g_cfg.audio.provider == audio_provider::rsxaudio && g_fxo->is_init<rsx_audio_backend>())
		{
			g_fxo->get<rsx_audio_backend>().update_emu_cfg();
		}
	}
}

rsxaudio_backend_thread::rsxaudio_backend_thread()
{
	new_emu_cfg = get_emu_cfg();

	const f32 new_vol = audio::get_volume();

	callback_cfg.atomic_op([&](callback_config& val)
	{
		val.target_volume = static_cast<u16>(new_vol * callback_config::VOL_NOMINAL);
		val.initial_volume = val.current_volume;
	});
}

rsxaudio_backend_thread::~rsxaudio_backend_thread()
{
	if (backend)
	{
		backend->Close();
		backend->SetWriteCallback(nullptr);
		backend->SetStateCallback(nullptr);
		backend = nullptr;
	}
}

void rsxaudio_backend_thread::update_emu_cfg()
{
	std::unique_lock lock(state_update_m);
	const emu_audio_cfg _new_emu_cfg = get_emu_cfg();
	const f32 new_vol = audio::get_volume();

	callback_cfg.atomic_op([&](callback_config& val)
	{
		val.target_volume = static_cast<u16>(new_vol * callback_config::VOL_NOMINAL);
		val.initial_volume = val.current_volume;
	});

	if (new_emu_cfg != _new_emu_cfg)
	{
		new_emu_cfg = _new_emu_cfg;
		emu_cfg_changed = true;
		lock.unlock();
		state_update_c.notify_all();
	}
}

u32 rsxaudio_backend_thread::get_sample_rate() const
{
	return callback_cfg.load().freq;
}

u8 rsxaudio_backend_thread::get_channel_count() const
{
	return callback_cfg.load().input_ch_cnt;
}

rsxaudio_backend_thread::emu_audio_cfg rsxaudio_backend_thread::get_emu_cfg()
{
	// Get max supported channel count
	AudioChannelCnt out_ch_cnt = AudioBackend::get_max_channel_count(0); // CELL_AUDIO_OUT_PRIMARY

	emu_audio_cfg cfg =
	{
		.audio_device = g_cfg.audio.audio_device,
		.desired_buffer_duration = g_cfg.audio.desired_buffer_duration,
		.time_stretching_threshold = g_cfg.audio.time_stretching_threshold / 100.0,
		.buffering_enabled = static_cast<bool>(g_cfg.audio.enable_buffering),
		.convert_to_s16 = static_cast<bool>(g_cfg.audio.convert_to_s16),
		.enable_time_stretching = static_cast<bool>(g_cfg.audio.enable_time_stretching),
		.dump_to_file = static_cast<bool>(g_cfg.audio.dump_to_file),
		.channels = out_ch_cnt,
		.channel_layout = g_cfg.audio.channel_layout,
		.renderer = g_cfg.audio.renderer,
		.provider = g_cfg.audio.provider,
		.avport = convert_avport(g_cfg.audio.rsxaudio_port)
	};

	cfg.buffering_enabled = cfg.buffering_enabled && cfg.renderer != audio_renderer::null;
	cfg.enable_time_stretching = cfg.buffering_enabled && cfg.enable_time_stretching && cfg.time_stretching_threshold > 0.0;

	return cfg;
}

void rsxaudio_backend_thread::operator()()
{
	if (g_cfg.audio.provider != audio_provider::rsxaudio)
	{
		return;
	}

	static rsxaudio_state ra_state{};
	static emu_audio_cfg emu_cfg{};
	static bool backend_failed = false;

	for (;;)
	{
		bool should_update_backend = false;
		bool reset_backend = false;
		bool checkDefaultDevice = false;
		bool should_service_stream = false;

		{
			std::unique_lock lock(state_update_m);
			for (;;)
			{
				// Unsafe to access backend under lock (state_changed_callback uses state_update_m -> possible deadlock)

				if (thread_ctrl::state() == thread_state::aborting)
				{
					lock.unlock();
					backend_stop();
					return;
				}

				if (backend_device_changed)
				{
					should_update_backend = true;
					checkDefaultDevice = true;
					backend_device_changed = false;
				}

				// Emulated state changed
				if (ra_state_changed)
				{
					const callback_config cb_cfg = callback_cfg.observe();
					ra_state_changed = false;
					ra_state = new_ra_state;

					if (cb_cfg.cfg_changed)
					{
						should_update_backend = true;
						checkDefaultDevice = false;
						callback_cfg.atomic_op([&](callback_config& val)
						{
							val.cfg_changed = false; // Acknowledge cfg update
						});
					}
				}

				// Update emu config
				if (emu_cfg_changed)
				{
					reset_backend |= emu_cfg.renderer != new_emu_cfg.renderer;
					emu_cfg_changed = false;
					emu_cfg = new_emu_cfg;
					should_update_backend = true;
					checkDefaultDevice = false;
				}

				// Handle backend error notification
				if (backend_error_occured)
				{
					reset_backend = true;
					should_update_backend = true;
					checkDefaultDevice = false;
					backend_error_occured = false;
				}

				if (should_update_backend)
				{
					backend_current_cfg.cfg = ra_state.port[static_cast<u8>(emu_cfg.avport)];
					backend_current_cfg.avport = emu_cfg.avport;
					break;
				}

				if (backend_failed)
				{
					state_update_c.wait(state_update_m, ERROR_SERVICE_PERIOD);
					break;
				}

				if (use_aux_ringbuf)
				{
					const u64 next_period_time = get_time_until_service();
					should_service_stream = next_period_time <= SERVICE_THRESHOLD;

					if (should_service_stream)
					{
						break;
					}

					state_update_c.wait(state_update_m, next_period_time);
				}
				else
				{
					// Nothing to do - wait for events
					state_update_c.wait(state_update_m, umax);
				}
			}
		}

		if (should_update_backend && (!checkDefaultDevice || backend->DefaultDeviceChanged()))
		{
			backend_init(ra_state, emu_cfg, reset_backend);

			if (emu_cfg.enable_time_stretching)
			{
				resampler.set_params(backend_current_cfg.cfg.ch_cnt, backend_current_cfg.cfg.freq);
				resampler.set_tempo(RESAMPLER_MAX_FREQ_VAL);
			}

			if (emu_cfg.dump_to_file)
			{
				dumper.Open(backend_current_cfg.cfg.ch_cnt, backend_current_cfg.cfg.freq, AudioSampleSize::FLOAT);
			}
			else
			{
				dumper.Close();
			}
		}

		if (!backend->Operational())
		{
			if (!backend_failed)
			{
				sys_rsxaudio.warning("Backend stopped unexpectedly (likely device change). Attempting to recover...");
			}

			backend_init(ra_state, emu_cfg);
			backend_failed = true;
			continue;
		}

		if (backend_failed)
		{
			sys_rsxaudio.warning("Backend recovered");
			backend_failed = false;
		}

		if (!Emu.IsPausedOrReady() || !use_aux_ringbuf) // Don't pause if thread is in direct mode
		{
			if (!backend_playing())
			{
				backend_start();
				reset_service_time();
				continue;
			}

			if (should_service_stream)
			{
				void* crnt_buf = thread_tmp_buf.data();

				const u64 bytes_req = ringbuf.get_free_size();
				const u64 bytes_read = aux_ringbuf.pop(crnt_buf, bytes_req, true);
				u64 crnt_buf_size = bytes_read;

				if (emu_cfg.enable_time_stretching)
				{
					const u64 input_ch_cnt = static_cast<u64>(ra_state.port[static_cast<u8>(emu_cfg.avport)].ch_cnt);
					const u64 bytes_per_sample = static_cast<u32>(AudioSampleSize::FLOAT) * input_ch_cnt;
					const u64 samples_req = bytes_req / bytes_per_sample;
					const u64 samples_avail = crnt_buf_size / bytes_per_sample;
					const f64 resampler_ratio = resampler.get_resample_ratio();
					f64 fullness_ratio = static_cast<f64>(samples_avail + resampler.samples_available()) / samples_req;

					if (fullness_ratio < emu_cfg.time_stretching_threshold)
					{
						fullness_ratio /= emu_cfg.time_stretching_threshold;
						const f64 new_resampler_ratio = (resampler_ratio + fullness_ratio) / 2.0;
						if (std::abs(new_resampler_ratio - resampler_ratio) >= TIME_STRETCHING_STEP)
						{
							resampler.set_tempo(new_resampler_ratio);
						}
					}
					else if (resampler_ratio != RESAMPLER_MAX_FREQ_VAL)
					{
						resampler.set_tempo(RESAMPLER_MAX_FREQ_VAL);
					}

					resampler.put_samples(static_cast<f32*>(crnt_buf), static_cast<u32>(samples_avail));
					const auto [resampled_data, sample_cnt] = resampler.get_samples(static_cast<u32>(samples_req));
					crnt_buf = resampled_data;
					crnt_buf_size = sample_cnt * bytes_per_sample;
				}

				// Dump audio if enabled
				if (emu_cfg.dump_to_file)
				{
					dumper.WriteData(crnt_buf, static_cast<u32>(crnt_buf_size));
				}

				ringbuf.push(crnt_buf, crnt_buf_size);

				update_service_time();
			}
		}
		else
		{
			if (backend_playing())
			{
				backend_stop();
			}

			if (should_service_stream)
			{
				update_service_time();
			}
		}
	}
}

rsxaudio_backend_thread& rsxaudio_backend_thread::operator=(thread_state /* state */)
{
	{
		std::lock_guard lock(state_update_m);
	}
	state_update_c.notify_all();
	return *this;
}

void rsxaudio_backend_thread::set_new_stream_param(const std::array<port_config, SYS_RSXAUDIO_AVPORT_CNT> &cfg, avport_bit muted_avports)
{
	std::unique_lock lock(state_update_m);

	const auto new_mute_state = gen_mute_state(muted_avports);
	const bool should_update = backend_current_cfg.cfg != cfg[static_cast<u8>(backend_current_cfg.avport)];

	callback_cfg.atomic_op([&](callback_config& val)
	{
		val.mute_state = new_mute_state;

		if (should_update)
		{
			val.ready = false; // Prevent audio playback until backend is reconfigured
			val.cfg_changed = true;
		}
	});

	if (new_ra_state.port != cfg)
	{
		new_ra_state.port = cfg;
		ra_state_changed = true;
		lock.unlock();
		state_update_c.notify_all();
	}
}

void rsxaudio_backend_thread::set_mute_state(avport_bit muted_avports)
{
	const auto new_mute_state = gen_mute_state(muted_avports);

	callback_cfg.atomic_op([&](callback_config& val)
	{
		val.mute_state = new_mute_state;
	});
}

u8 rsxaudio_backend_thread::gen_mute_state(avport_bit avports)
{
	std::bitset<SYS_RSXAUDIO_AVPORT_CNT> mute_state{0};

	if (avports.hdmi_0)  mute_state[static_cast<u8>(RsxaudioAvportIdx::HDMI_0)]  = true;
	if (avports.hdmi_1)  mute_state[static_cast<u8>(RsxaudioAvportIdx::HDMI_1)]  = true;
	if (avports.avmulti) mute_state[static_cast<u8>(RsxaudioAvportIdx::AVMULTI)] = true;
	if (avports.spdif_0) mute_state[static_cast<u8>(RsxaudioAvportIdx::SPDIF_0)] = true;
	if (avports.spdif_1) mute_state[static_cast<u8>(RsxaudioAvportIdx::SPDIF_1)] = true;

	return static_cast<u8>(mute_state.to_ulong());
}

void rsxaudio_backend_thread::add_data(rsxaudio_data_container& cont)
{
	std::unique_lock lock(ringbuf_mutex, std::try_to_lock);
	if (!lock.owns_lock())
	{
		return;
	}

	const callback_config cb_cfg = callback_cfg.observe();
	if (!cb_cfg.ready || !cb_cfg.callback_active)
	{
		return;
	}

	static rsxaudio_data_container::data_blk_t in_data_blk{};

	if (u32 len = cont.get_data_size(cb_cfg.avport_idx))
	{
		if (use_aux_ringbuf)
		{
			if (aux_ringbuf.get_free_size() >= len)
			{
				cont.get_data(cb_cfg.avport_idx, in_data_blk);
				aux_ringbuf.push(in_data_blk.data(), len);
			}
		}
		else
		{
			if (ringbuf.get_free_size() >= len)
			{
				cont.get_data(cb_cfg.avport_idx, in_data_blk);
				ringbuf.push(in_data_blk.data(), len);
			}
		}
	}
}

RsxaudioAvportIdx rsxaudio_backend_thread::convert_avport(audio_avport avport)
{
	switch (avport)
	{
	case audio_avport::hdmi_0:  return RsxaudioAvportIdx::HDMI_0;
	case audio_avport::hdmi_1:  return RsxaudioAvportIdx::HDMI_1;
	case audio_avport::avmulti: return RsxaudioAvportIdx::AVMULTI;
	case audio_avport::spdif_0: return RsxaudioAvportIdx::SPDIF_0;
	case audio_avport::spdif_1: return RsxaudioAvportIdx::SPDIF_1;
	default:
	{
		fmt::throw_exception("Invalid RSXAudio avport: %u", static_cast<u8>(avport));
	}
	}
}

void rsxaudio_backend_thread::backend_init(const rsxaudio_state& ra_state, const emu_audio_cfg& emu_cfg, bool reset_backend)
{
	if (reset_backend || !backend)
	{
		backend = nullptr;
		backend = Emu.GetCallbacks().get_audio();
		backend->SetWriteCallback(std::bind(&rsxaudio_backend_thread::write_data_callback, this, std::placeholders::_1, std::placeholders::_2));
		backend->SetStateCallback(std::bind(&rsxaudio_backend_thread::state_changed_callback, this, std::placeholders::_1));
	}

	const port_config& port_cfg = ra_state.port[static_cast<u8>(emu_cfg.avport)];
	const AudioSampleSize sample_size = emu_cfg.convert_to_s16 ? AudioSampleSize::S16 : AudioSampleSize::FLOAT;
	const AudioChannelCnt ch_cnt = static_cast<AudioChannelCnt>(std::min<u32>(static_cast<u32>(port_cfg.ch_cnt), static_cast<u32>(emu_cfg.channels)));

	f64 cb_frame_len = 0.0;
	audio_channel_layout backend_channel_layout = audio_channel_layout::stereo;

	if (backend->Open(emu_cfg.audio_device, port_cfg.freq, sample_size, ch_cnt, emu_cfg.channel_layout))
	{
		cb_frame_len = backend->GetCallbackFrameLen();
		backend_channel_layout = backend->get_channel_layout();
		sys_rsxaudio.notice("Opened audio backend (sampling_rate=%d, sample_size=%d, channels=%d, layout=%s)", backend->get_sampling_rate(), backend->get_sample_size(), backend->get_channels(), backend->get_channel_layout());
	}
	else
	{
		sys_rsxaudio.error("Failed to open audio backend. Make sure that no other application is running that might block audio access (e.g. Netflix).");
	}

	static constexpr f64 _10ms = 512.0 / 48000.0;
	const f64 buffering_len = emu_cfg.buffering_enabled ? (emu_cfg.desired_buffer_duration / 1000.0) : 0.0;
	const u64 bytes_per_sec = static_cast<u32>(AudioSampleSize::FLOAT) * static_cast<u32>(port_cfg.ch_cnt) * static_cast<u32>(port_cfg.freq);

	{
		std::lock_guard lock(ringbuf_mutex);
		use_aux_ringbuf = emu_cfg.enable_time_stretching || emu_cfg.dump_to_file;

		if (use_aux_ringbuf)
		{
			const f64 frame_len = std::max<f64>(buffering_len * 0.5, SERVICE_PERIOD_SEC) + cb_frame_len + _10ms;
			const u64 frame_len_bytes = static_cast<u64>(std::round(frame_len * bytes_per_sec));
			aux_ringbuf.set_buf_size(frame_len_bytes);
			ringbuf.set_buf_size(frame_len_bytes);
			thread_tmp_buf.resize(frame_len_bytes);
		}
		else
		{
			const f64 frame_len = std::max<f64>(buffering_len, cb_frame_len) + _10ms;
			ringbuf.set_buf_size(static_cast<u64>(std::round(frame_len * bytes_per_sec)));
			thread_tmp_buf.resize(0);
		}

		callback_tmp_buf.resize(static_cast<usz>((cb_frame_len + _10ms) * static_cast<u32>(AudioSampleSize::FLOAT) * static_cast<u32>(port_cfg.ch_cnt) * static_cast<u32>(port_cfg.freq)));
	}

	callback_cfg.atomic_op([&](callback_config& val)
	{
		val.callback_active = false; // Backend may take some time to activate. This prevents overflows on input side.

		if (!val.cfg_changed)
		{
			val.freq = static_cast<u32>(port_cfg.freq);
			val.input_ch_cnt = static_cast<u32>(port_cfg.ch_cnt);
			val.output_channel_layout = static_cast<u8>(backend_channel_layout);
			val.convert_to_s16 = emu_cfg.convert_to_s16;
			val.avport_idx = emu_cfg.avport;
			val.ready = true;
		}
	});
}

void rsxaudio_backend_thread::backend_start()
{
	ensure(backend != nullptr);

	if (use_aux_ringbuf)
	{
		resampler.set_tempo(RESAMPLER_MAX_FREQ_VAL);
		resampler.flush();
		aux_ringbuf.reader_flush();
	}

	ringbuf.reader_flush();
	backend->Play();
}

void rsxaudio_backend_thread::backend_stop()
{
	if (backend == nullptr)
	{
		return;
	}

	backend->Pause();
	callback_cfg.atomic_op([&](callback_config& val)
	{
		val.callback_active = false;
	});
}

bool rsxaudio_backend_thread::backend_playing()
{
	if (backend == nullptr)
	{
		return false;
	}

	return backend->IsPlaying();
}

u32 rsxaudio_backend_thread::write_data_callback(u32 bytes, void* buf)
{
	const callback_config cb_cfg = callback_cfg.atomic_op([&](callback_config& val)
	{
		val.callback_active = true;
		return val;
	});

	const std::bitset<SYS_RSXAUDIO_AVPORT_CNT> mute_state{cb_cfg.mute_state};

	if (cb_cfg.ready && !mute_state[static_cast<u8>(cb_cfg.avport_idx)] && Emu.IsRunning())
	{
		const audio_channel_layout output_channel_layout = static_cast<audio_channel_layout>(cb_cfg.output_channel_layout);
		const u32 output_ch_cnt = AudioBackend::default_layout_channel_count(output_channel_layout);
		const u32 bytes_ch_adjusted = bytes / output_ch_cnt * cb_cfg.input_ch_cnt;
		const u32 bytes_from_rb = cb_cfg.convert_to_s16 ? bytes_ch_adjusted / static_cast<u32>(AudioSampleSize::S16) * static_cast<u32>(AudioSampleSize::FLOAT) : bytes_ch_adjusted;

		ensure(callback_tmp_buf.size() * static_cast<u32>(AudioSampleSize::FLOAT) >= bytes_from_rb);

		const u32 byte_cnt = static_cast<u32>(ringbuf.pop(callback_tmp_buf.data(), bytes_from_rb, true));
		const u32 sample_cnt = byte_cnt / static_cast<u32>(AudioSampleSize::FLOAT);
		const u32 sample_cnt_out = sample_cnt / cb_cfg.input_ch_cnt * output_ch_cnt;

		// Buffer is in weird state - drop acquired data
		if (sample_cnt == 0 || sample_cnt % cb_cfg.input_ch_cnt != 0)
		{
			memset(buf, 0, bytes);
			return bytes;
		}

		// Record audio if enabled
		if (g_recording_mode != recording_mode::stopped)
		{
			utils::video_provider& provider = g_fxo->get<utils::video_provider>();
			provider.present_samples(reinterpret_cast<const u8*>(callback_tmp_buf.data()), sample_cnt / cb_cfg.input_ch_cnt, cb_cfg.input_ch_cnt);
		}

		// Downmix if necessary
		AudioBackend::downmix(sample_cnt, cb_cfg.input_ch_cnt, output_channel_layout, callback_tmp_buf.data(), callback_tmp_buf.data());

		if (cb_cfg.target_volume != cb_cfg.current_volume)
		{
			const AudioBackend::VolumeParam param =
			{
				.initial_volume = cb_cfg.initial_volume * callback_config::VOL_NOMINAL_INV,
				.current_volume = cb_cfg.current_volume * callback_config::VOL_NOMINAL_INV,
				.target_volume = cb_cfg.target_volume * callback_config::VOL_NOMINAL_INV,
				.freq = cb_cfg.freq,
				.ch_cnt = cb_cfg.input_ch_cnt
			};

			const u16 new_vol = static_cast<u16>(std::round(AudioBackend::apply_volume(param, sample_cnt_out, callback_tmp_buf.data(), callback_tmp_buf.data()) * callback_config::VOL_NOMINAL));
			callback_cfg.atomic_op([&](callback_config& val)
			{
				if (val.target_volume != cb_cfg.target_volume)
				{
					val.initial_volume = new_vol;
				}

				// We don't care about proper volume adjustment if underflow has occured
				val.current_volume = bytes_from_rb != byte_cnt ? val.target_volume : new_vol;
			});
		}
		else if (cb_cfg.current_volume != callback_config::VOL_NOMINAL)
		{
			AudioBackend::apply_volume_static(cb_cfg.current_volume * callback_config::VOL_NOMINAL_INV, sample_cnt_out, callback_tmp_buf.data(), callback_tmp_buf.data());
		}

		if (cb_cfg.convert_to_s16)
		{
			AudioBackend::convert_to_s16(sample_cnt_out, callback_tmp_buf.data(), buf);
			return sample_cnt_out * static_cast<u32>(AudioSampleSize::S16);
		}

		AudioBackend::normalize(sample_cnt_out, callback_tmp_buf.data(), static_cast<f32*>(buf));
		return sample_cnt_out * static_cast<u32>(AudioSampleSize::FLOAT);
	}

	ringbuf.reader_flush();
	memset(buf, 0, bytes);
	return bytes;
}

void rsxaudio_backend_thread::state_changed_callback(AudioStateEvent event)
{
	{
		std::lock_guard lock(state_update_m);
		switch (event)
		{
		case AudioStateEvent::UNSPECIFIED_ERROR:
		{
			backend_error_occured = true;
			break;
		}
		case AudioStateEvent::DEFAULT_DEVICE_MAYBE_CHANGED:
		{
			backend_device_changed = true;
			break;
		}
		default:
		{
			fmt::throw_exception("Unknown audio state event");
		}
		}
	}
	state_update_c.notify_all();
}

u64 rsxaudio_backend_thread::get_time_until_service()
{
	const u64 next_service_time = start_time + time_period_idx * SERVICE_PERIOD;
	const u64 current_time = get_system_time();

	return next_service_time >= current_time ? next_service_time - current_time : 0;
}

void rsxaudio_backend_thread::update_service_time()
{
	if (get_time_until_service() <= SERVICE_THRESHOLD) time_period_idx++;
}

void rsxaudio_backend_thread::reset_service_time()
{
	start_time = get_system_time();
	time_period_idx = 1;
}

void rsxaudio_periodic_tmr::sched_timer()
{
	u64 interval = get_rel_next_time();

	if (interval == 0)
	{
		zero_period = true;
	}
	else if (interval == UINT64_MAX)
	{
		interval = 0;
		zero_period = false;
	}
	else
	{
		zero_period = false;
	}

#if defined(_WIN32)
	if (interval)
	{
		LARGE_INTEGER due_time{};
		due_time.QuadPart = -static_cast<s64>(interval * 10);
		ensure(SetWaitableTimerEx(timer_handle, &due_time, 0, nullptr, nullptr, nullptr, 0));
	}
	else
	{
		ensure(CancelWaitableTimer(timer_handle));
	}
#elif defined(__linux__)
	const time_t secs = interval / 1'000'000;
	const long nsecs = (interval - secs * 1'000'000) * 1000;
	const itimerspec tspec = {{}, { secs, nsecs }};
	ensure(timerfd_settime(timer_handle, 0, &tspec, nullptr) == 0);
#elif defined(BSD) || defined(__APPLE__)
	handle[TIMER_ID].data = interval * 1000;
	if (interval)
	{
		handle[TIMER_ID].flags = (handle[TIMER_ID].flags & ~EV_DISABLE) | EV_ENABLE;
	}
	else
	{
		handle[TIMER_ID].flags = (handle[TIMER_ID].flags & ~EV_ENABLE) | EV_DISABLE;
	}
	ensure(kevent(kq, &handle[TIMER_ID], 1, nullptr, 0, nullptr) >= 0);
#else
#error "Implement"
#endif
}

void rsxaudio_periodic_tmr::cancel_timer_unlocked()
{
	zero_period = false;

#if defined(_WIN32)
	ensure(CancelWaitableTimer(timer_handle));
	if (in_wait)
	{
		ensure(SetEvent(cancel_event));
	}
#elif defined(__linux__)
	const itimerspec tspec{};
	ensure(timerfd_settime(timer_handle, 0, &tspec, nullptr) == 0);
	if (in_wait)
	{
		const u64 flag = 1;
		const auto wr_res = write(cancel_event, &flag, sizeof(flag));
		ensure(wr_res == sizeof(flag) || wr_res == -EAGAIN);
	}
#elif defined(BSD) || defined(__APPLE__)
	handle[TIMER_ID].flags = (handle[TIMER_ID].flags & ~EV_ENABLE) | EV_DISABLE;
	handle[TIMER_ID].data = 0;
	if (in_wait)
	{
		ensure(kevent(kq, handle, 2, nullptr, 0, nullptr) >= 0);
	}
	else
	{
		ensure(kevent(kq, &handle[TIMER_ID], 1, nullptr, 0, nullptr) >= 0);
	}
#else
#error "Implement"
#endif
}

void rsxaudio_periodic_tmr::reset_cancel_flag()
{
#if defined(_WIN32)
	ensure(ResetEvent(cancel_event));
#elif defined(__linux__)
	u64 tmp_buf{};
	[[maybe_unused]] const auto nread = read(cancel_event, &tmp_buf, sizeof(tmp_buf));
#elif defined(BSD) || defined(__APPLE__)
	// Cancel event is reset automatically
#else
#error "Implement"
#endif
}

rsxaudio_periodic_tmr::rsxaudio_periodic_tmr()
{
#if defined(_WIN32)
	ensure(cancel_event = CreateEvent(nullptr, false, false, nullptr));
	ensure(timer_handle = CreateWaitableTimer(nullptr, false, nullptr));
#elif defined(__linux__)
	timer_handle = timerfd_create(CLOCK_MONOTONIC, 0);
	ensure((epoll_fd = epoll_create(2)) >= 0);
	epoll_event evnt{ EPOLLIN, {} };
	evnt.data.fd = timer_handle;
	ensure(timer_handle >= 0 && epoll_ctl(epoll_fd, EPOLL_CTL_ADD, timer_handle, &evnt) == 0);
	cancel_event = eventfd(0, EFD_NONBLOCK);
	evnt.data.fd = cancel_event;
	ensure(cancel_event >= 0 && epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cancel_event, &evnt) == 0);
#elif defined(BSD) || defined(__APPLE__)

#if defined(__APPLE__)
	static constexpr unsigned int TMR_CFG = NOTE_NSECONDS | NOTE_CRITICAL;
#else
	static constexpr unsigned int TMR_CFG = NOTE_NSECONDS;
#endif

	ensure((kq = kqueue()) >= 0);
	EV_SET(&handle[TIMER_ID], TIMER_ID, EVFILT_TIMER, EV_ADD | EV_ENABLE | EV_ONESHOT, TMR_CFG, 0, nullptr);
	EV_SET(&handle[CANCEL_ID], CANCEL_ID, EVFILT_USER, EV_ADD | EV_ENABLE | EV_CLEAR, NOTE_FFNOP, 0, nullptr);
	ensure(kevent(kq, &handle[CANCEL_ID], 1, nullptr, 0, nullptr) >= 0);
	handle[CANCEL_ID].fflags |= NOTE_TRIGGER;
#else
#error "Implement"
#endif
}

rsxaudio_periodic_tmr::~rsxaudio_periodic_tmr()
{
#if defined(_WIN32)
	CloseHandle(timer_handle);
	CloseHandle(cancel_event);
#elif defined(__linux__)
	close(epoll_fd);
	close(timer_handle);
	close(cancel_event);
#elif defined(BSD) || defined(__APPLE__)
	close(kq);
#else
#error "Implement"
#endif
}

rsxaudio_periodic_tmr::wait_result rsxaudio_periodic_tmr::wait(const std::function<void()> &callback)
{
	std::unique_lock lock(mutex);

	if (in_wait || !callback)
	{
		return wait_result::INVALID_PARAM;
	}

	in_wait = true;

	bool tmr_error     = false;
	bool timeout       = false;
	bool wait_canceled = false;

	if (!zero_period)
	{
		lock.unlock();
		constexpr u8 obj_wait_cnt = 2;

#if defined(_WIN32)
		const HANDLE wait_arr[obj_wait_cnt] = { timer_handle, cancel_event };
		const auto wait_status = WaitForMultipleObjects(obj_wait_cnt, wait_arr, false, INFINITE);

		if (wait_status == WAIT_FAILED || (wait_status >= WAIT_ABANDONED_0 && wait_status < WAIT_ABANDONED_0 + obj_wait_cnt))
		{
			tmr_error = true;
		}
		else if (wait_status == WAIT_TIMEOUT)
		{
			timeout = true;
		}
		else if (wait_status == WAIT_OBJECT_0 + 1)
		{
			wait_canceled = true;
		}
#elif defined(__linux__)
		epoll_event event[obj_wait_cnt]{};
		int wait_status = 0;
		do
		{
			wait_status = epoll_wait(epoll_fd, event, obj_wait_cnt, -1);
		}
		while (wait_status == -1 && errno == EINTR);

		if (wait_status < 0 || wait_status > obj_wait_cnt)
		{
			tmr_error = true;
		}
		else if (wait_status == 0)
		{
			timeout = true;
		}
		else
		{
			for (int i = 0; i < wait_status; i++)
			{
				if (event[i].data.fd == cancel_event)
				{
					wait_canceled = true;
					break;
				}
			}
		}
#elif defined(BSD) || defined(__APPLE__)
		struct kevent event[obj_wait_cnt]{};
		int wait_status = 0;
		do
		{
			wait_status = kevent(kq, nullptr, 0, event, obj_wait_cnt, nullptr);
		}
		while (wait_status == -1 && errno == EINTR);

		if (wait_status < 0 || wait_status > obj_wait_cnt)
		{
			tmr_error = true;
		}
		else if (wait_status == 0)
		{
			timeout = true;
		}
		else
		{
			for (int i = 0; i < wait_status; i++)
			{
				if (event[i].ident == CANCEL_ID)
				{
					wait_canceled = true;
					break;
				}
			}
		}
#else
#error "Implement"
#endif
		lock.lock();
	}
	else
	{
		zero_period = false;
	}

	in_wait = false;

	if (wait_canceled)
	{
		reset_cancel_flag();
	}

	if (tmr_error)
	{
		return wait_result::TIMER_ERROR;
	}

	if (timeout)
	{
		return wait_result::TIMEOUT;
	}

	if (wait_canceled)
	{
		sched_timer();
		return wait_result::TIMER_CANCELED;
	}

	callback();
	sched_timer();
	return wait_result::SUCCESS;
}

u64 rsxaudio_periodic_tmr::get_rel_next_time()
{
	const u64 crnt_time = get_system_time();
	u64 next_time = UINT64_MAX;

	for (vtimer& vtimer : vtmr_pool)
	{
		if (!vtimer.active) continue;

		u64 next_blk_time = static_cast<u64>(vtimer.blk_cnt * vtimer.blk_time);

		if (crnt_time >= next_blk_time)
		{
			const u64 crnt_blk = get_crnt_blk(crnt_time, vtimer.blk_time);

			if (crnt_blk > vtimer.blk_cnt + MAX_BURST_PERIODS)
			{
				vtimer.blk_cnt = std::max(vtimer.blk_cnt, crnt_blk - MAX_BURST_PERIODS);
				next_blk_time = static_cast<u64>(vtimer.blk_cnt * vtimer.blk_time);
			}
		}

		if (crnt_time >= next_blk_time)
		{
			next_time = 0;
		}
		else
		{
			next_time = std::min(next_time, next_blk_time - crnt_time);
		}
	}

	return next_time;
}

void rsxaudio_periodic_tmr::cancel_wait()
{
	std::lock_guard lock(mutex);
	cancel_timer_unlocked();
}

void rsxaudio_periodic_tmr::enable_vtimer(u32 vtimer_id, u32 rate, u64 crnt_time)
{
	ensure(vtimer_id < VTIMER_MAX && rate);

	vtimer& vtimer = vtmr_pool[vtimer_id];
	const f64 new_blk_time = get_blk_time(rate);

	// Avoid timer reset when possible
	if (!vtimer.active || new_blk_time != vtimer.blk_time)
	{
		vtimer.blk_cnt = get_crnt_blk(crnt_time, new_blk_time);
	}

	vtimer.blk_time = new_blk_time;
	vtimer.active = true;
}

void rsxaudio_periodic_tmr::disable_vtimer(u32 vtimer_id)
{
	ensure(vtimer_id < VTIMER_MAX);

	vtimer& vtimer = vtmr_pool[vtimer_id];
	vtimer.active = false;
}

bool rsxaudio_periodic_tmr::is_vtimer_behind(u32 vtimer_id, u64 crnt_time) const
{
	ensure(vtimer_id < VTIMER_MAX);

	const vtimer& vtimer = vtmr_pool[vtimer_id];

	return is_vtimer_behind(vtimer, crnt_time);
}

void rsxaudio_periodic_tmr::vtimer_skip_periods(u32 vtimer_id, u64 crnt_time)
{
	ensure(vtimer_id < VTIMER_MAX);

	vtimer& vtimer = vtmr_pool[vtimer_id];

	if (is_vtimer_behind(vtimer, crnt_time))
	{
		vtimer.blk_cnt = get_crnt_blk(crnt_time, vtimer.blk_time);
	}
}

void rsxaudio_periodic_tmr::vtimer_incr(u32 vtimer_id, u64 crnt_time)
{
	ensure(vtimer_id < VTIMER_MAX);

	vtimer& vtimer = vtmr_pool[vtimer_id];

	if (is_vtimer_behind(vtimer, crnt_time))
	{
		vtimer.blk_cnt++;
	}
}

bool rsxaudio_periodic_tmr::is_vtimer_active(u32 vtimer_id) const
{
	ensure(vtimer_id < VTIMER_MAX);

	const vtimer& vtimer = vtmr_pool[vtimer_id];

	return vtimer.active;
}

u64 rsxaudio_periodic_tmr::vtimer_get_sched_time(u32 vtimer_id) const
{
	ensure(vtimer_id < VTIMER_MAX);

	const vtimer& vtimer = vtmr_pool[vtimer_id];

	return static_cast<u64>(vtimer.blk_cnt * vtimer.blk_time);
}

bool rsxaudio_periodic_tmr::is_vtimer_behind(const vtimer& vtimer, u64 crnt_time) const
{
	return vtimer.active && vtimer.blk_cnt < get_crnt_blk(crnt_time, vtimer.blk_time);
}

u64 rsxaudio_periodic_tmr::get_crnt_blk(u64 crnt_time, f64 blk_time) const
{
	return static_cast<u64>(std::floor(static_cast<f64>(crnt_time) / blk_time)) + 1;
}

f64 rsxaudio_periodic_tmr::get_blk_time(u32 data_rate) const
{
	return static_cast<f64>(SYS_RSXAUDIO_STREAM_SIZE * 1'000'000) / data_rate;
}
