#pragma once

#include "Emu/Memory/vm_ptr.h"
#include "Utilities/mutex.h"
#include "Utilities/cond.h"
#include "Utilities/simple_ringbuf.h"

enum : u32
{
	PS3AV_RX_BUF_SIZE                       = 0x800,
	PS3AV_TX_BUF_SIZE                       = 0x800,

	PS3AV_VERSION                           = 0x205,

	PS3AV_CID_AV_INIT                       = 0x00000001,
	PS3AV_CID_AV_FIN                        = 0x00000002,
	PS3AV_CID_AV_GET_HW_CONF                = 0x00000003,
	PS3AV_CID_AV_GET_MONITOR_INFO           = 0x00000004,
	PS3AV_CID_AV_GET_BKSV_LIST              = 0x00000005,
	PS3AV_CID_AV_ENABLE_EVENT               = 0x00000006,
	PS3AV_CID_AV_DISABLE_EVENT              = 0x00000007,
	PS3AV_CID_AV_GET_PORT_STATE             = 0x00000009,
	PS3AV_CID_AV_TV_MUTE                    = 0x0000000A,
	PS3AV_CID_AV_NULL_CMD                   = 0x0000000B,
	PS3AV_CID_AV_GET_AKSV                   = 0x0000000C,
	PS3AV_CID_AV_UNK4                       = 0x0000000D,
	PS3AV_CID_AV_UNK5                       = 0x0000000E,

	PS3AV_CID_AV_VIDEO_MUTE                 = 0x00010002,
	PS3AV_CID_AV_VIDEO_DISABLE_SIG          = 0x00010003,
	PS3AV_CID_AV_VIDEO_YTRAPCONTROL         = 0x00010004,
	PS3AV_CID_AV_VIDEO_UNK5                 = 0x00010005,
	PS3AV_CID_AV_VIDEO_UNK6                 = 0x00010006,
	PS3AV_CID_AV_AUDIO_MUTE                 = 0x00020002,
	PS3AV_CID_AV_ACP_CTRL                   = 0x00020003,
	PS3AV_CID_AV_SET_ACP_PACKET             = 0x00020004,
	PS3AV_CID_AV_ADD_SIGNAL_CTL             = 0x00030001,
	PS3AV_CID_AV_SET_CC_CODE                = 0x00030002,
	PS3AV_CID_AV_SET_CGMS_WSS               = 0x00030003,
	PS3AV_CID_AV_SET_MACROVISION            = 0x00030004,
	PS3AV_CID_AV_UNK7                       = 0x00030005,
	PS3AV_CID_AV_UNK8                       = 0x00030006,
	PS3AV_CID_AV_UNK9                       = 0x00030007,
	PS3AV_CID_AV_HDMI_MODE                  = 0x00040001,
	PS3AV_CID_AV_UNK15                      = 0x00050001,

	PS3AV_CID_AV_CEC_MESSAGE                = 0x000A0001,
	PS3AV_CID_AV_GET_CEC_CONFIG             = 0x000A0002,
	PS3AV_CID_AV_UNK11                      = 0x000A0003,
	PS3AV_CID_AV_UNK12                      = 0x000A0004,
	PS3AV_CID_AV_UNK13                      = 0x000A0005,
	PS3AV_CID_AV_UNK14                      = 0x000A0006,

	PS3AV_CID_VIDEO_INIT                    = 0x01000001,
	PS3AV_CID_VIDEO_MODE                    = 0x01000002,
	PS3AV_CID_VIDEO_ROUTE                   = 0x01000003,
	PS3AV_CID_VIDEO_FORMAT                  = 0x01000004,
	PS3AV_CID_VIDEO_PITCH                   = 0x01000005,
	PS3AV_CID_VIDEO_GET_HW_CONF             = 0x01000006,
	PS3AV_CID_VIDEO_GET_REG                 = 0x01000008,
	PS3AV_CID_VIDEO_UNK                     = 0x01000009,
	PS3AV_CID_VIDEO_UNK1                    = 0x0100000A,
	PS3AV_CID_VIDEO_UNK2                    = 0x0100000B,
	PS3AV_CID_VIDEO_UNK3                    = 0x0100000C,

	PS3AV_CID_AUDIO_INIT                    = 0x02000001,
	PS3AV_CID_AUDIO_MODE                    = 0x02000002,
	PS3AV_CID_AUDIO_MUTE                    = 0x02000003,
	PS3AV_CID_AUDIO_ACTIVE                  = 0x02000004,
	PS3AV_CID_AUDIO_INACTIVE                = 0x02000005,
	PS3AV_CID_AUDIO_SPDIF_BIT               = 0x02000006,
	PS3AV_CID_AUDIO_CTRL                    = 0x02000007,

	PS3AV_CID_AVB_PARAM                     = 0x04000001,

	PS3AV_CID_EVENT_UNPLUGGED               = 0x10000001,
	PS3AV_CID_EVENT_PLUGGED                 = 0x10000002,
	PS3AV_CID_EVENT_HDCP_DONE               = 0x10000003,
	PS3AV_CID_EVENT_HDCP_FAIL               = 0x10000004,
	PS3AV_CID_EVENT_HDCP_REAUTH             = 0x10000005,
	PS3AV_CID_EVENT_HDCP_ERROR              = 0x10000006,

	PS3AV_REPLY_BIT                         = 0x80000000,

	PS3AV_RESBIT_720x480P                   = 0x0003, /* 0x0001 | 0x0002 */
	PS3AV_RESBIT_720x576P                   = 0x0003, /* 0x0001 | 0x0002 */
	PS3AV_RESBIT_1280x720P                  = 0x0004,
	PS3AV_RESBIT_1920x1080I                 = 0x0008,
	PS3AV_RESBIT_1920x1080P                 = 0x4000,

	PS3AV_MONITOR_TYPE_NONE                 = 0,
	PS3AV_MONITOR_TYPE_HDMI                 = 1,
	PS3AV_MONITOR_TYPE_DVI                  = 2,
	PS3AV_MONITOR_TYPE_AVMULTI              = 3,

	PS3AV_COLORIMETRY_xvYCC_601             = 1,
	PS3AV_COLORIMETRY_xvYCC_709             = 2,
	PS3AV_COLORIMETRY_MD0                   = 1 << 4,
	PS3AV_COLORIMETRY_MD1                   = 1 << 5,
	PS3AV_COLORIMETRY_MD2                   = 1 << 6,

	PS3AV_CS_SUPPORTED                      = 1,
	PS3AV_RGB_SELECTABLE_QAUNTIZATION_RANGE = 8,
	PS3AV_12BIT_COLOR                       = 16,

	PS3AV_MON_INFO_AUDIO_BLK_MAX            = 16,

	PS3AV_MON_INFO_AUDIO_TYPE_LPCM          = 1,
	PS3AV_MON_INFO_AUDIO_TYPE_AC3           = 2,
	PS3AV_MON_INFO_AUDIO_TYPE_AAC           = 6,
	PS3AV_MON_INFO_AUDIO_TYPE_DTS           = 7,
	PS3AV_MON_INFO_AUDIO_TYPE_DDP           = 10,
	PS3AV_MON_INFO_AUDIO_TYPE_DTS_HD        = 11,
	PS3AV_MON_INFO_AUDIO_TYPE_DOLBY_THD     = 12,

	PS3AV_HDMI_BEHAVIOR_HDCP_OFF            = 0x01,
	PS3AV_HDMI_BEHAVIOR_DVI                 = 0x40,
	PS3AV_HDMI_BEHAVIOR_EDID_PASS           = 0x80,
	PS3AV_HDMI_BEHAVIOR_NORMAL              = 0xFF,

	PS3AV_EVENT_BIT_UNPLUGGED               = 0x01,
	PS3AV_EVENT_BIT_PLUGGED                 = 0x02,
	PS3AV_EVENT_BIT_HDCP_DONE               = 0x04,
	PS3AV_EVENT_BIT_HDCP_FAIL               = 0x08,
	PS3AV_EVENT_BIT_HDCP_REAUTH             = 0x10,
	PS3AV_EVENT_BIT_HDCP_TOPOLOGY           = 0x20,
	PS3AV_EVENT_BIT_UNK                     = 0x80000000,

	PS3AV_HEAD_A_HDMI                       = 0,
	PS3AV_HEAD_B_ANALOG                     = 1,

	PS3AV_AUDIO_PORT_HDMI_0                 = 1 << 0,
	PS3AV_AUDIO_PORT_HDMI_1                 = 1 << 1,
	PS3AV_AUDIO_PORT_AVMULTI                = 1 << 10,
	PS3AV_AUDIO_PORT_SPDIF_0                = 1 << 20,
	PS3AV_AUDIO_PORT_SPDIF_1                = 1 << 21,

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
	PS3AV_STATUS_UNK_0x14                   = 0x14,
};

const u8 PS3AV_AKSV_VALUE[5] = { 0x00, 0x00, 0x0F, 0xFF, 0xFF };
const u8 PS3AV_BKSV_VALUE[5] = { 0xFF, 0xFF, 0xF0, 0x00, 0x00 };

enum PS3_AV_OP_MODE : u32
{
	// BIG operation modes could send more then 4096 bytes

	NOT_BLOCKING_BIG_OP = 0,
	BLOCKING_BIG_OP     = 1,
	NOT_BLOCKING_OP     = 2,
};

enum class UartHdmiEvent : u8
{
	NONE       = 0,
	UNPLUGGED  = 1,
	PLUGGED    = 2,
	HDCP_DONE  = 3,
};

enum class UartAudioCtrlID : u32
{
	DAC_RESET       = 0,
	DAC_DE_EMPHASIS = 1,
	AVCLK           = 2,
};

enum class UartAudioAvport : u8
{
	HDMI_0    = 0x0,
	HDMI_1    = 0x1,
	AVMULTI_0 = 0x10,
	AVMULTI_1 = 0x11,
	SPDIF_0   = 0x20,
	SPDIF_1   = 0x21,
};

enum class UartAudioSource : u32
{
	SERIAL = 0,
	SPDIF  = 1,
};

enum class UartAudioFreq : u32
{
	_32K  = 1,
	_44K  = 2,
	_48K  = 3,
	_88K  = 4,
	_96K  = 5,
	_176K = 6,
	_192K = 7,
};

enum class UartAudioFormat : u32
{
	PCM       = 1,
	BITSTREAM = 0xFF,
};

enum class UartAudioSampleSize : u32
{
	_16BIT = 1,
	_20BIT = 2,
	_24BIT = 3,
};

class vuart_hdmi_event_handler
{
public:

	vuart_hdmi_event_handler(u64 time_offset = 0);

	void set_target_state(UartHdmiEvent start_state, UartHdmiEvent end_state);
	bool events_available();

	u64 time_until_next();
	UartHdmiEvent get_occured_event();

private:

	static constexpr u64 EVENT_TIME_DURATION = 20000;
	static constexpr u64 EVENT_TIME_THRESHOLD = 1000;

	u64 time_of_next_event = 0;
	const u64 time_offset = 0;

	// Assume that syscon initialized hdmi to plugged state
	UartHdmiEvent current_state = UartHdmiEvent::PLUGGED;
	UartHdmiEvent current_to_state = UartHdmiEvent::PLUGGED;

	UartHdmiEvent base_state = UartHdmiEvent::NONE;
	UartHdmiEvent target_state = UartHdmiEvent::NONE;

	void schedule_next();
	void advance_state();
};

class vuart_av_thread;

struct ps3av_cmd
{
	virtual u16 get_size(vuart_av_thread &vuart, const void *pkt_buf) = 0;
	virtual void execute(vuart_av_thread &vuart, const void *pkt_buf) = 0;
	virtual ~ps3av_cmd() {};
};

class vuart_av_thread
{
public:

	atomic_t<bool> initialized{};

	shared_mutex rx_mutex{};
	shared_mutex tx_mutex{};

	shared_mutex tx_wake_m{};
	cond_variable tx_wake_c{};
	shared_mutex tx_rdy_m{};
	cond_variable tx_rdy_c{};
	shared_mutex rx_wake_m{};
	cond_variable rx_wake_c{};

	bool head_b_initialized = false;
	u8 hdmi_behavior_mode = PS3AV_HDMI_BEHAVIOR_NORMAL;
	u16 av_cmd_ver = 0;
	u32 hdmi_events_bitmask = 0;
	bool hdmi_res_set[2]{ false, false };

	void operator()();
	void parse_tx_buffer(u32 buf_size);
	vuart_av_thread &operator=(thread_state);

	u32 enque_tx_data(const void *data, u32 data_sz);
	u32 get_tx_bytes();
	u32 read_rx_data(void *data, u32 data_sz);

	u32 get_reply_buf_free_size();

	template<bool UseScBuffer = false>
	void write_resp(u32 cid, u32 status, const void *data = nullptr, u16 data_size = 0);

	void add_hdmi_events(UartHdmiEvent first_event, UartHdmiEvent last_event, bool hdmi_0, bool hdmi_1);
	void add_hdmi_events(UartHdmiEvent last_event, bool hdmi_0, bool hdmi_1);

	static RsxaudioAvportIdx avport_to_idx(UartAudioAvport avport);

	static constexpr auto thread_name = "VUART AV Thread"sv;

private:

	struct temp_buf
	{
		u32 crnt_size = 0;
		u8 buf[PS3AV_RX_BUF_SIZE]{};
	};

	simple_ringbuf tx_buf{PS3AV_TX_BUF_SIZE};
	simple_ringbuf rx_buf{PS3AV_RX_BUF_SIZE};

	// uart_mngr could sometimes read past the tx_buffer due to weird size checks in FW,
	// but no further than size of largest packet
	u8 temp_tx_buf[PS3AV_TX_BUF_SIZE * 2]{};
	temp_buf temp_rx_buf{};
	temp_buf temp_rx_sc_buf{};

	vuart_hdmi_event_handler hdmi_event_handler[2]{ 0, 5000 };
	bool hdcp_first_auth[2]{ true, true };

	u32 read_tx_data(void *data, u32 data_sz);
	std::shared_ptr<ps3av_cmd> get_cmd(u32 cid);
	void commit_rx_buf(bool syscon_buf);

	void add_unplug_event(bool hdmi_0, bool hdmi_1);
	void add_plug_event(bool hdmi_0, bool hdmi_1);
	void add_hdcp_done_event(bool hdmi_0, bool hdmi_1);
	void commit_event_data(const void *data, u16 data_size);
	void dispatch_hdmi_event(UartHdmiEvent event, UartAudioAvport hdmi);
};

using vuart_av = named_thread<vuart_av_thread>;

struct vuart_params
{
	be_t<u64, 1> rx_buf_size;
	be_t<u64, 1> tx_buf_size;
};

static_assert(sizeof(vuart_params) == 16);

struct ps3av_pkt_reply_hdr
{
	be_t<u16, 1> version;
	be_t<u16, 1> length;
	be_t<u32, 1> cid;
	be_t<u32, 1> status;
};

static_assert(sizeof(ps3av_pkt_reply_hdr) == 12);

struct ps3av_header
{
	be_t<u16, 1> version;
	be_t<u16, 1> length;
	be_t<u32, 1> cid;
};

static_assert(sizeof(ps3av_header) == 8);

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
	u8 colorimetry_data;
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
	ps3av_info_audio audio_info[PS3AV_MON_INFO_AUDIO_BLK_MAX];
	be_t<u16, 1> hor_screen_size;
	be_t<u16, 1> ver_screen_size;
	u8 supported_content_types;
	u8 reserved_1[3];
	ps3av_info_resolution res_60_packed_3D;
	ps3av_info_resolution res_50_packed_3D;
	ps3av_info_resolution res_other_3D;
	ps3av_info_resolution res_60_sbs_3D;
	ps3av_info_resolution res_50_sbs_3D;
	u8 vendor_specific_flags;
	u8 reserved_2[7];
};

static_assert(sizeof(ps3av_get_monitor_info_reply) == 208);

struct ps3av_get_monitor_info
{
	ps3av_header hdr;
	be_t<u16, 1> avport;
	be_t<u16, 1> reserved;
};

static_assert(sizeof(ps3av_get_monitor_info) == 12);

struct ps3av_get_hw_info_reply
{
	be_t<u16, 1> num_of_hdmi;
	be_t<u16, 1> num_of_avmulti;
	be_t<u16, 1> num_of_spdif;
	be_t<u16, 1> extra_bistream_support;
};

static_assert(sizeof(ps3av_get_hw_info_reply) == 8);

struct ps3av_pkt_set_hdmi_mode
{
	ps3av_header hdr;
	u8 mode;
	u8 resv[3];
};

static_assert(sizeof(ps3av_pkt_set_hdmi_mode) == 12);

struct ps3av_pkt_audio_mode
{
	ps3av_header hdr;
	UartAudioAvport avport;
	u8 reserved0[3];
	be_t<u32, 1> mask;
	be_t<u32, 1> audio_num_of_ch;
	be_t<UartAudioFreq, 1> audio_fs;
	be_t<UartAudioSampleSize, 1> audio_word_bits;
	be_t<UartAudioFormat, 1> audio_format;
	be_t<UartAudioSource, 1> audio_source;
	u8 audio_enable[4];
	u8 audio_swap[4];
	u8 audio_map[4];
	be_t<u32, 1> audio_layout;
	be_t<u32, 1> audio_downmix;
	be_t<u32, 1> audio_downmix_level;
	u8 audio_cs_info[8];
};

static_assert(sizeof(ps3av_pkt_audio_mode) == 68);

struct ps3av_pkt_audio_mute
{
	ps3av_header hdr;
	UartAudioAvport avport;
	u8 reserved0[3];
	u8 mute;
};

static_assert(sizeof(ps3av_pkt_audio_mute) == 13);

struct ps3av_pkt_audio_set_active
{
	ps3av_header hdr;
	be_t<u32, 1> audio_port;
};

static_assert(sizeof(ps3av_pkt_audio_set_active) == 12);

struct ps3av_pkt_audio_spdif_bit
{
	ps3av_header hdr;
	UartAudioAvport avport;
	u8 reserved0[3];
	be_t<u32, 1> audio_port;
	be_t<u32, 1> spdif_bit_data[12];
};

static_assert(sizeof(ps3av_pkt_audio_spdif_bit) == 64);

struct ps3av_pkt_audio_ctrl
{
	ps3av_header hdr;
	be_t<UartAudioCtrlID, 1> audio_ctrl_id;
	be_t<u32, 1> audio_ctrl_data[4];
};

static_assert(sizeof(ps3av_pkt_audio_ctrl) == 28);

struct ps3av_pkt_hdmi_plugged_event
{
	ps3av_header hdr;
	ps3av_get_monitor_info_reply minfo;
};

static_assert(sizeof(ps3av_pkt_hdmi_plugged_event) == 216);

struct ps3av_pkt_hdmi_hdcp_done_event
{
	ps3av_header hdr;
	be_t<u32, 1> ksv_cnt;
	u8 ksv_arr[20][5];
};

static_assert(sizeof(ps3av_pkt_hdmi_hdcp_done_event) == 112);

struct ps3av_pkt_av_init
{
	ps3av_header hdr;
	be_t<u32, 1> event_bit;
};

static_assert(sizeof(ps3av_pkt_av_init) == 12);

struct ps3av_pkt_av_init_reply
{
	be_t<u32, 1> unk;
};

static_assert(sizeof(ps3av_pkt_av_init_reply) == 4);

struct ps3av_pkt_enable_event
{
	ps3av_header hdr;
	be_t<u32, 1> event_bit;
};

static_assert(sizeof(ps3av_pkt_enable_event) == 12);

struct ps3av_pkt_get_bksv
{
	ps3av_header hdr;
	be_t<u16, 1> avport;
	u8 resv[2];
};

static_assert(sizeof(ps3av_pkt_get_bksv) == 12);

struct ps3av_pkt_get_bksv_reply
{
	be_t<u16, 1> avport;
	u8 resv[2];
	be_t<u32, 1> ksv_cnt;
	u8 ksv_arr[20][5];
};

static_assert(sizeof(ps3av_pkt_get_bksv_reply) == 108);

struct ps3av_pkt_video_get_hw_cfg_reply
{
	be_t<u32, 1> gx_available;
};

static_assert(sizeof(ps3av_pkt_video_get_hw_cfg_reply) == 4);

struct ps3av_pkt_video_set_pitch
{
	ps3av_header hdr;
	be_t<u32, 1> video_head;
	be_t<u32, 1> pitch;
};

static_assert(sizeof(ps3av_pkt_video_set_pitch) == 16);

struct ps3av_pkt_get_aksv_reply
{
	be_t<u32, 1> ksv_size;
	u8 ksv_arr[2][5];
	u8 resv[2];
};

static_assert(sizeof(ps3av_pkt_get_aksv_reply) == 16);

struct ps3av_pkt_inc_avset
{
	ps3av_header hdr;
	be_t<u16, 1> num_of_video_pkt;
	be_t<u16, 1> num_of_audio_pkt;
	be_t<u16, 1> num_of_av_video_pkt;
	be_t<u16, 1> num_of_av_audio_pkt;
};

static_assert(sizeof(ps3av_pkt_inc_avset) == 16);

struct ps3av_pkt_av_audio_param
{
	ps3av_header hdr;
	be_t<u16, 1> avport;
	be_t<u16, 1> resv;
	u8 mclk;
	u8 ns[3];
	u8 enable;
	u8 swaplr;
	u8 fifomap;
	u8 inputctrl;
	u8 inputlen;
	u8 layout;
	u8 info[5];
	u8 chstat[5];
};

static_assert(sizeof(ps3av_pkt_av_audio_param) == 32);

struct ps3av_pkt_av_video_cs
{
	ps3av_header hdr;
	be_t<u16, 1> avport;
	be_t<u16, 1> av_vid;
	be_t<u16, 1> av_cs_out;
	be_t<u16, 1> av_cs_in;
	u8 dither;
	u8 bitlen_out;
	u8 super_white;
	u8 aspect;
	u8 unk1;
	u8 unk2;
	u8 resv[2];
};

static_assert(sizeof(ps3av_pkt_av_video_cs) == 24);

struct ps3av_pkt_video_mode
{
	ps3av_header hdr;
	be_t<u32, 1> video_head;
	be_t<u16, 1> unk1;
	be_t<u16, 1> unk2;
	be_t<u32, 1> video_vid;
	be_t<u32, 1> width;
	be_t<u32, 1> height;
	be_t<u32, 1> pitch;
	be_t<u32, 1> video_out_format;
	be_t<u32, 1> video_format;
	be_t<u16, 1> unk3;
	be_t<u16, 1> video_order;
	be_t<u32, 1> unk4;
};

static_assert(sizeof(ps3av_pkt_video_mode) == 48);

struct ps3av_pkt_av_video_ytrapcontrol
{
	ps3av_header hdr;
	be_t<u16, 1> unk1;
	be_t<u16, 1> unk2;
};

static_assert(sizeof(ps3av_pkt_av_video_ytrapcontrol) == 12);

struct ps3av_pkt_av_get_cec_config_reply
{
	be_t<u32, 1> cec_present;
};

struct ps3av_pkt_video_format
{
	ps3av_header hdr;
	be_t<u32, 1> video_head;
	be_t<u32, 1> video_format;
	be_t<u16, 1> unk;
	be_t<u16, 1> video_order;
};

static_assert(sizeof(ps3av_pkt_video_format) == 20);

struct ps3av_pkt_av_set_cgms_wss
{
	ps3av_header hdr;
	be_t<u16, 1> avport;
	u8 resv[2];
	be_t<u32, 1> cgms_wss;
};

static_assert(sizeof(ps3av_pkt_av_set_cgms_wss) == 16);

struct ps3av_pkt_set_acp_packet
{
	ps3av_header hdr;
	u8 avport;
	u8 pkt_type;
	u8 resv[2];
	u8 pkt_data[32];
};

static_assert(sizeof(ps3av_pkt_set_acp_packet) == 44);

struct ps3av_pkt_acp_ctrl
{
	ps3av_header hdr;
	u8 avport;
	u8 packetctl;
	u8 resv[2];
};

static_assert(sizeof(ps3av_pkt_acp_ctrl) == 12);

struct ps3av_pkt_add_signal_ctl
{
	ps3av_header hdr;
	be_t<u16, 1> avport;
	be_t<u16, 1> signal_ctl;
};

static_assert(sizeof(ps3av_pkt_add_signal_ctl) == 12);

struct ps3av_pkt_av_audio_mute
{
	ps3av_header hdr;
	be_t<u16, 1> avport;
	be_t<u16, 1> mute;
};

static_assert(sizeof(ps3av_pkt_av_audio_mute) == 12);

struct ps3av_pkt_video_disable_sig
{
	ps3av_header hdr;
	be_t<u16, 1> avport;
	be_t<u16, 1> resv;
};

static_assert(sizeof(ps3av_pkt_video_disable_sig) == 12);

// SysCalls

error_code sys_uart_initialize(ppu_thread &ppu);
error_code sys_uart_receive(ppu_thread &ppu, vm::ptr<void> buffer, u64 size, u32 mode);
error_code sys_uart_send(ppu_thread &ppu, vm::cptr<void> buffer, u64 size, u32 mode);
error_code sys_uart_get_params(vm::ptr<vuart_params> buffer);
