#pragma once

#include "Emu/Memory/vm_ptr.h"
#include "Utilities/mutex.h"
#include "Utilities/cond.h"

enum : u32
{
	PS3AV_RX_BUF_SIZE                       = 0x800,
	PS3AV_TX_BUF_SIZE                       = 0x800,

	PS3AV_VERSION                           = 0x205,

	PS3AV_CID_AV_INIT                       = 0x00000001,
	PS3AV_CID_AV_FIN                        = 0x00000002,
	PS3AV_CID_AV_GET_HW_CONF                = 0x00000003,
	PS3AV_CID_AV_GET_MONITOR_INFO           = 0x00000004,
	PS3AV_CID_AV_GET_KSV_LIST_SIZE          = 0x00000005,
	PS3AV_CID_AV_ENABLE_EVENT               = 0x00000006,
	PS3AV_CID_AV_DISABLE_EVENT              = 0x00000007,
	PS3AV_CID_AV_GET_ALL_EDID               = 0x00000008,
	PS3AV_CID_AV_TV_MUTE                    = 0x0000000A,

	PS3AV_CID_AV_VIDEO_CS                   = 0x00010001,
	PS3AV_CID_AV_VIDEO_MUTE                 = 0x00010002,
	PS3AV_CID_AV_VIDEO_DISABLE_SIG          = 0x00010003,
	PS3AV_CID_AV_VIDEO_UNK4                 = 0x00010004,
	PS3AV_CID_AV_AUDIO_PARAM                = 0x00020001,
	PS3AV_CID_AV_AUDIO_MUTE                 = 0x00020002,
	PS3AV_CID_AV_ACP_CTRL                   = 0x00020003,
	PS3AV_CID_AV_SET_ACP_PACKET             = 0x00020004,
	PS3AV_CID_AV_UNK                        = 0x00030001,
	PS3AV_CID_AV_UNK2                       = 0x00030003,
	PS3AV_CID_AV_HDMI_MODE                  = 0x00040001,

	PS3AV_CID_VIDEO_INIT                    = 0x01000001,
	PS3AV_CID_VIDEO_MODE                    = 0x01000002,
	PS3AV_CID_VIDEO_ROUTE                   = 0x01000003,
	PS3AV_CID_VIDEO_FORMAT                  = 0x01000004,
	PS3AV_CID_VIDEO_PITCH                   = 0x01000005,
	PS3AV_CID_VIDEO_GET_HW_CONF             = 0x01000006,
	PS3AV_CID_VIDEO_REGVAL                  = 0x01000008,

	PS3AV_CID_AUDIO_INIT                    = 0x02000001,
	PS3AV_CID_AUDIO_MODE                    = 0x02000002,
	PS3AV_CID_AUDIO_MUTE                    = 0x02000003,
	PS3AV_CID_AUDIO_ACTIVE                  = 0x02000004,
	PS3AV_CID_AUDIO_INACTIVE                = 0x02000005,
	PS3AV_CID_AUDIO_SPDIF_BIT               = 0x02000006,
	PS3AV_CID_AUDIO_CTRL                    = 0x02000007,

	PS3AV_CID_EVENT_UNPLUGGED               = 0x10000001,
	PS3AV_CID_EVENT_PLUGGED                 = 0x10000002,
	PS3AV_CID_EVENT_HDCP_DONE               = 0x10000003,
	PS3AV_CID_EVENT_HDCP_FAIL               = 0x10000004,
	PS3AV_CID_EVENT_HDCP_AUTH               = 0x10000005,
	PS3AV_CID_EVENT_HDCP_ERROR              = 0x10000006,

	PS3AV_CID_AVB_PARAM                     = 0x04000001,

	PS3AV_REPLY_BIT                         = 0x80000000,

	PS3AV_RESBIT_720x480P                   = 0x0003, /* 0x0001 | 0x0002 */
	PS3AV_RESBIT_720x576P                   = 0x0003, /* 0x0001 | 0x0002 */
	PS3AV_RESBIT_1280x720P                  = 0x0004,
	PS3AV_RESBIT_1920x1080I                 = 0x0008,
	PS3AV_RESBIT_1920x1080P                 = 0x4000,

	PS3AV_MONITOR_TYPE_HDMI                 = 1,
	PS3AV_MONITOR_TYPE_DVI                  = 2,

	PS3AV_STATUS_SUCCESS                    = 0x00,
	PS3AV_STATUS_RECEIVE_VUART_ERROR        = 0x01,
	PS3AV_STATUS_SYSCON_COMMUNICATE_FAIL    = 0x02,
	PS3AV_STATUS_INVALID_COMMAND            = 0x03,
	PS3AV_STATUS_INVALID_PORT               = 0x04,
	PS3AV_STATUS_INVALID_VID                = 0x05,
	PS3AV_STATUS_INVALID_COLOR_SPACE        = 0x06,
	PS3AV_STATUS_INVALID_FS                 = 0x07,
	PS3AV_STATUS_INVALID_AUDIO_CH           = 0x08,
	PS3AV_STATUS_UNSUPPORTED_VERSION        = 0x09,
	PS3AV_STATUS_INVALID_SAMPLE_SIZE        = 0x0A,
	PS3AV_STATUS_FAILURE                    = 0x0B,
	PS3AV_STATUS_UNSUPPORTED_COMMAND        = 0x0C,
	PS3AV_STATUS_BUFFER_OVERFLOW            = 0x0D,
	PS3AV_STATUS_INVALID_VIDEO_PARAM        = 0x0E,
	PS3AV_STATUS_NO_SEL                     = 0x0F,
	PS3AV_STATUS_INVALID_AV_PARAM           = 0x10,
	PS3AV_STATUS_INVALID_AUDIO_PARAM        = 0x11,
	PS3AV_STATUS_UNSUPPORTED_HDMI_MODE      = 0x12,
	PS3AV_STATUS_NO_SYNC_HEAD               = 0x13,
};

class vuart_av_thread;

struct ps3av_cmd
{
	virtual u16 get_size(vuart_av_thread &vuart, const void *pkt_buf) = 0;
	virtual bool execute(vuart_av_thread &vuart, const void *pkt_buf, u32 reply_max_size) = 0;
};

template<u32 Size>
class vuart_av_ringbuf
{
private:

	atomic_t<u32> rd_ptr = 0;
	atomic_t<u32> wr_ptr = 0;
	u8 buf[Size + 1];

public:

	u32 get_free_size()
	{
		const u32 rd = rd_ptr.observe();
		const u32 wr = wr_ptr.observe();

		return wr >= rd ? Size - (wr - rd) : rd - wr - 1U;
	}

	u32 get_used_size()
	{
		return Size - get_free_size();
	}

	u32 push(const void *data, u32 size)
	{
		const u32 old     = wr_ptr.observe();
		const u32 to_push = std::min(size, get_free_size());
		const u8 *b_data  = static_cast<const u8*>(data);

		if (!to_push || !data)
		{
			return 0;
		}

		if (old + to_push > sizeof(buf))
		{
			const u32 first_write_sz = sizeof(buf) - old;
			memcpy(&buf[old], b_data, first_write_sz);
			memcpy(&buf[0], b_data + first_write_sz, to_push - first_write_sz);
		}
		else
		{
			memcpy(&buf[old], b_data, to_push);
		}

		wr_ptr = (old + to_push) % sizeof(buf);

		return to_push;
	}

	u32 pop(void *data, u32 size)
	{
		const u32 old    = rd_ptr.observe();
		const u32 to_pop = std::min(size, get_used_size());
		u8 *b_data       = static_cast<u8*>(data);

		if (!to_pop || !data)
		{
			return 0;
		}

		if (old + to_pop > sizeof(buf))
		{
			const u32 first_read_sz = sizeof(buf) - old;
			memcpy(b_data, &buf[old], first_read_sz);
			memcpy(b_data + first_read_sz, &buf[0], to_pop - first_read_sz);
		}
		else
		{
			memcpy(b_data, &buf[old], to_pop);
		}

		rd_ptr = (old + to_pop) % sizeof(buf);

		return to_pop;
	}
};

class vuart_av_thread
{
public:

	u8 init = 0;

	shared_mutex rx_mutex{};
	shared_mutex tx_mutex{};

	std::mutex tx_wake_m{};
	cond_variable tx_wake_c{};
	std::mutex tx_rdy_m{};
	cond_variable tx_rdy_c{};
	std::mutex rx_wake_m{};
	cond_variable rx_wake_c{};

	void operator()();
	vuart_av_thread &operator=(thread_state);

	u32 enque_tx_data(const void *data, u32 data_sz);
	u32 get_tx_bytes();
	u32 read_rx_data(void *data, u32 data_sz);
	void write_resp(u32 cid, u32 status, const void *data = nullptr, u32 data_size = 0);

	static constexpr auto thread_name = "VUART AV Thread"sv;

private:

	vuart_av_ringbuf<PS3AV_TX_BUF_SIZE> tx_buf{};
	vuart_av_ringbuf<PS3AV_RX_BUF_SIZE> rx_buf{};
	u8 temp_tx_buf[PS3AV_TX_BUF_SIZE];
	u8 temp_rx_buf[PS3AV_TX_BUF_SIZE];
	u32 temp_rx_buf_size = 0;

	u32 read_tx_data(void *data, u32 data_sz);
	std::shared_ptr<ps3av_cmd> get_cmd(u32 cid);
	void commit_rx_buf();
};

using vuart_av = named_thread<vuart_av_thread>;

struct vuart_params
{
	be_t<u64, 1> rx_buf_size;
	be_t<u64, 1> tx_buf_size;
};

struct ps3av_pkt_reply_hdr
{
	be_t<u16, 1> version;
	be_t<u16, 1> length;
	be_t<u32, 1> cid;
	be_t<u32, 1> status;
};

struct ps3av_header
{
	be_t<u16, 1> version;
	be_t<u16, 1> length;
	be_t<u32, 1> cid;
};

struct ps3av_info_resolution
{
	be_t<u32, 1> res_bits;
	be_t<u32, 1> native;
};

struct ps3av_info_cs
{
	u8 rgb;
	u8 yuv444;
	u8 yuv422;
	u8 reserved;
};

struct ps3av_info_color
{
	be_t<u16, 1> red_x;
	be_t<u16, 1> red_y;
	be_t<u16, 1> green_x;
	be_t<u16, 1> green_y;
	be_t<u16, 1> blue_x;
	be_t<u16, 1> blue_y;
	be_t<u16, 1> white_x;
	be_t<u16, 1> white_y;
	be_t<u32, 1> gamma;
};

struct ps3av_info_audio
{
	u8 type;
	u8 max_num_of_ch;
	u8 fs;
	u8 sbit;
};

struct ps3av_get_monitor_info_reply
{
	u8 avport;
	u8 monitor_id[10];
	u8 monitor_type;
	u8 monitor_name[16];
	ps3av_info_resolution res_60;
	ps3av_info_resolution res_50;
	ps3av_info_resolution res_other;
	ps3av_info_resolution res_vesa;
	ps3av_info_cs cs;
	ps3av_info_color color;
	u8 supported_ai;
	u8 speaker_info;
	be_t<u16, 1> num_of_audio_block;
	ps3av_info_audio audio_info[29];
};

struct ps3av_get_monitor_info
{
	ps3av_header hdr;
	be_t<u16, 1> avport;
	be_t<u16, 1> reserved;
};

struct ps3av_get_hw_info_reply
{
	be_t<u16, 1> num_of_hdmi;
	be_t<u16, 1> num_of_avmulti;
	be_t<u16, 1> num_of_spdif;
	be_t<u16, 1> resv;
};

// SysCalls

error_code sys_uart_initialize(ppu_thread &ppu);
error_code sys_uart_receive(ppu_thread &ppu, vm::ptr<void> buffer, u64 size, u32 mode);
error_code sys_uart_send(ppu_thread &ppu, vm::cptr<void> buffer, u64 size, u32 mode);
error_code sys_uart_get_params(vm::ptr<vuart_params> buffer);
