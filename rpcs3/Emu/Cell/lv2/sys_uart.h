#pragma once

#include "Emu/Memory/vm_ptr.h"

enum : u32
{
	PS3AV_VERSION = 0x205, /* version of ps3av command */

	PS3AV_CID_AV_INIT              = 0x00000001,
	PS3AV_CID_AV_FIN               = 0x00000002,
	PS3AV_CID_AV_GET_HW_CONF       = 0x00000003,
	PS3AV_CID_AV_GET_MONITOR_INFO  = 0x00000004,
	PS3AV_CID_AV_GET_KSV_LIST_SIZE = 0x00000005,
	PS3AV_CID_AV_ENABLE_EVENT      = 0x00000006,
	PS3AV_CID_AV_DISABLE_EVENT     = 0x00000007,
	PS3AV_CID_AV_GET_ALL_EDID      = 0x00000008,
	PS3AV_CID_AV_TV_MUTE           = 0x0000000a,

	PS3AV_CID_AV_VIDEO_CS          = 0x00010001,
	PS3AV_CID_AV_VIDEO_MUTE        = 0x00010002,
	PS3AV_CID_AV_VIDEO_DISABLE_SIG = 0x00010003,
	PS3AV_CID_AV_VIDEO_UNK4        = 0x00010004,
	PS3AV_CID_AV_AUDIO_PARAM       = 0x00020001,
	PS3AV_CID_AV_AUDIO_MUTE        = 0x00020002,
	PS3AV_CID_AV_ACP_CTRL          = 0x00020003,
	PS3AV_CID_AV_SET_ACP_PACKET    = 0x00020004,
	PS3AV_CID_AV_UNK               = 0x0030001,
	PS3AV_CID_AV_UNK2              = 0x0030003,
	PS3AV_CID_AV_HDMI_MODE         = 0x00040001,

	PS3AV_CID_VIDEO_INIT        = 0x01000001,
	PS3AV_CID_VIDEO_MODE        = 0x01000002,
	PS3AV_CID_VIDEO_ROUTE       = 0x01000003, // 'route and color' ?
	PS3AV_CID_VIDEO_FORMAT      = 0x01000004,
	PS3AV_CID_VIDEO_PITCH       = 0x01000005,
	PS3AV_CID_VIDEO_GET_HW_CONF = 0x01000006,
	PS3AV_CID_VIDEO_REGVAL      = 0x01000008,

	PS3AV_CID_AUDIO_INIT      = 0x02000001,
	PS3AV_CID_AUDIO_MODE      = 0x02000002,
	PS3AV_CID_AUDIO_MUTE      = 0x02000003,
	PS3AV_CID_AUDIO_ACTIVE    = 0x02000004,
	PS3AV_CID_AUDIO_INACTIVE  = 0x02000005,
	PS3AV_CID_AUDIO_SPDIF_BIT = 0x02000006,
	PS3AV_CID_AUDIO_CTRL      = 0x02000007,

	PS3AV_CID_EVENT_UNPLUGGED  = 0x10000001,
	PS3AV_CID_EVENT_PLUGGED    = 0x10000002,
	PS3AV_CID_EVENT_HDCP_DONE  = 0x10000003,
	PS3AV_CID_EVENT_HDCP_FAIL  = 0x10000004,
	PS3AV_CID_EVENT_HDCP_AUTH  = 0x10000005,
	PS3AV_CID_EVENT_HDCP_ERROR = 0x10000006,

	PS3AV_CID_AVB_PARAM = 0x04000001, // 'super packet'

	PS3AV_REPLY_BIT = 0x80000000,

	PS3AV_RESBIT_720x480P   = 0x0003, /* 0x0001 | 0x0002 */
	PS3AV_RESBIT_720x576P   = 0x0003, /* 0x0001 | 0x0002 */
	PS3AV_RESBIT_1280x720P  = 0x0004,
	PS3AV_RESBIT_1920x1080I = 0x0008,
	PS3AV_RESBIT_1920x1080P = 0x4000,

	PS3AV_MONITOR_TYPE_HDMI = 1, /* HDMI */
	PS3AV_MONITOR_TYPE_DVI  = 2, /* DVI */
};

struct uart_payload
{
	u32 version;
	std::vector<u32> data;
};

std::deque<uart_payload> payloads;
semaphore<> mutex;

// helper header for reply commands
struct ps3av_reply_cmd_hdr
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

struct ps3av_info_monitor
{
	u8 avport;                       // 0
	u8 monitor_id[0xA];              // 0x1
	u8 monitor_type;                 // 0xB
	u8 monitor_name[0x10];           // 0xC
	ps3av_info_resolution res_60;    // 0x1C
	ps3av_info_resolution res_50;    // 0x24
	ps3av_info_resolution res_other; // 0x2C
	ps3av_info_resolution res_vesa;  // 0x34
	ps3av_info_cs cs;                // 0x3C
	ps3av_info_color color;          // 0x40
	u8 supported_ai;                 // 0x54
	u8 speaker_info;                 // 0x55
	u8 num_of_audio_block;           // 0x56
	//	struct ps3av_info_audio audio[0]; /* 0 or more audio blocks */
	u8 reserved[0xA9];
};

struct ps3av_evnt_plugged
{
	ps3av_header hdr;
	be_t<u32> cid;
	ps3av_info_monitor info;
};

struct ps3av_get_hw_info
{
	ps3av_header hdr;
	be_t<u32> cid;
	be_t<u32> status;
	be_t<u16> num_of_hdmi;
	be_t<u16> num_of_avmulti;
	be_t<u16> num_of_spdif;
	be_t<u16> resv;
};

struct ps3av_get_monitor_info
{
	ps3av_header hdr;
	be_t<u32> cid;
	be_t<u32> status;
	ps3av_info_monitor info;
};

// SysCalls

error_code sys_uart_initialize();
error_code sys_uart_receive(ppu_thread& ppu, vm::ptr<void> buffer, u64 size, u32 unk);
error_code sys_uart_send(vm::cptr<void> buffer, u64 size, u64 flags);
error_code sys_uart_get_params(vm::ptr<char> buffer);
