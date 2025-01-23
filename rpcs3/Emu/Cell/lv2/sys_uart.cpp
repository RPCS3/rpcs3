#include "stdafx.h"

#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/lv2/sys_sync.h"
#include "Emu/Cell/lv2/sys_rsxaudio.h"
#include "Emu/Cell/lv2/sys_process.h"

#include "sys_uart.h"

LOG_CHANNEL(sys_uart);

template <>
void fmt_class_string<UartAudioCtrlID>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](UartAudioCtrlID value)
	{
		switch (value)
		{
		STR_CASE(UartAudioCtrlID::DAC_RESET);
		STR_CASE(UartAudioCtrlID::DAC_DE_EMPHASIS);
		STR_CASE(UartAudioCtrlID::AVCLK);
		}

		return unknown;
	});
}

struct av_init_cmd : public ps3av_cmd
{
	u16 get_size(vuart_av_thread& /*vuart*/, const void* /*pkt_buf*/) override
	{
		return sizeof(ps3av_pkt_av_init);
	}

	void execute(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_pkt_av_init *>(pkt_buf);

		if (vuart.get_reply_buf_free_size() < sizeof(ps3av_pkt_reply_hdr))
		{
			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_BUFFER_OVERFLOW);
			return;
		}

		vuart.av_cmd_ver = pkt->hdr.version;
		vuart.hdmi_events_bitmask |= pkt->event_bit;

		if (pkt->event_bit & PS3AV_EVENT_BIT_UNK)
		{
			// 0 or 255, probably ps2 backwards compatibility (inverted)
			const ps3av_pkt_av_init_reply reply = { 0 };

			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_SUCCESS, &reply, sizeof(ps3av_pkt_av_init_reply));
			return;
		}

		vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_SUCCESS);
	}
};

struct av_fini_cmd : public ps3av_cmd
{
	u16 get_size(vuart_av_thread& /*vuart*/, const void* /*pkt_buf*/) override
	{
		return sizeof(ps3av_header);
	}

	void execute(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_header *>(pkt_buf);

		if (vuart.get_reply_buf_free_size() < sizeof(ps3av_pkt_reply_hdr))
		{
			vuart.write_resp(pkt->cid, PS3AV_STATUS_BUFFER_OVERFLOW);
			return;
		}

		vuart.hdmi_events_bitmask = 0;

		vuart.write_resp(pkt->cid, PS3AV_STATUS_SUCCESS);
	}
};

struct av_get_monitor_info_cmd : public ps3av_cmd
{
	u16 get_size(vuart_av_thread& /*vuart*/, const void* /*pkt_buf*/) override
	{
		return sizeof(ps3av_get_monitor_info);
	}

	void execute(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_get_monitor_info *>(pkt_buf);

		if (vuart.get_reply_buf_free_size() < sizeof(ps3av_pkt_reply_hdr))
		{
			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_BUFFER_OVERFLOW);
			return;
		}

		ps3av_get_monitor_info_reply cfg{};

		if (pkt->avport == static_cast<u16>(UartAudioAvport::AVMULTI_0))
		{
			cfg.avport = static_cast<u8>(UartAudioAvport::AVMULTI_0);
			cfg.monitor_type = PS3AV_MONITOR_TYPE_AVMULTI;
			cfg.res_60.res_bits = UINT32_MAX;
			cfg.res_50.res_bits = UINT32_MAX;
			cfg.res_vesa.res_bits = UINT32_MAX;
			cfg.cs.rgb = PS3AV_CS_SUPPORTED;
			cfg.cs.yuv444 = PS3AV_CS_SUPPORTED;
			cfg.cs.yuv422 = PS3AV_CS_SUPPORTED;
			cfg.speaker_info = 1;
			cfg.num_of_audio_block = 1;
			cfg.audio_info[0].sbit = 7;
			cfg.audio_info[0].max_num_of_ch = 2;
			cfg.audio_info[0].type = PS3AV_MON_INFO_AUDIO_TYPE_LPCM;
			cfg.audio_info[0].fs = 127;

			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_SUCCESS, &cfg, sizeof(ps3av_get_monitor_info_reply));
		}
		else if (pkt->avport <= static_cast<u16>(UartAudioAvport::HDMI_1))
		{
			if (pkt->avport == static_cast<u16>(UartAudioAvport::HDMI_1) && !g_cfg.core.debug_console_mode)
			{
				vuart.write_resp<true>(pkt->hdr.cid, PS3AV_STATUS_SYSCON_COMMUNICATE_FAIL);
				return;
			}

			set_hdmi_display_cfg(vuart, cfg, static_cast<u8>(pkt->avport));

			vuart.write_resp<true>(pkt->hdr.cid, PS3AV_STATUS_SUCCESS, &cfg, sizeof(ps3av_get_monitor_info_reply) - 4); // Length is different for some reason
		}
		else
		{
			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_INVALID_PORT);
		}
	}

	static void set_hdmi_display_cfg(vuart_av_thread &vuart, ps3av_get_monitor_info_reply &cfg, u8 avport)
	{
		if (vuart.hdmi_behavior_mode != PS3AV_HDMI_BEHAVIOR_NORMAL && (vuart.hdmi_behavior_mode & PS3AV_HDMI_BEHAVIOR_EDID_PASS))
		{
			cfg.monitor_type = vuart.hdmi_behavior_mode & PS3AV_HDMI_BEHAVIOR_DVI ? PS3AV_MONITOR_TYPE_DVI : PS3AV_MONITOR_TYPE_HDMI;
			return;
		}

		// Report maximum support

		static constexpr u8 mon_id[sizeof(cfg.monitor_id)] = { 0x4A, 0x13, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15 };
		static constexpr u8 mon_name[sizeof(cfg.monitor_name)] = { 'R', 'P', 'C', 'S', '3', ' ', 'V', 'i', 'r', 't', 'M', 'o', 'n', '\0', '\0', '\0' };
		static constexpr ps3av_info_audio audio_info[PS3AV_MON_INFO_AUDIO_BLK_MAX] =
		{
			{ PS3AV_MON_INFO_AUDIO_TYPE_LPCM,      8, 0x7F, 0x07 },
			{ PS3AV_MON_INFO_AUDIO_TYPE_AC3,       8, 0x7F, 0xFF },
			{ PS3AV_MON_INFO_AUDIO_TYPE_AAC,       8, 0x7F, 0xFF },
			{ PS3AV_MON_INFO_AUDIO_TYPE_DTS,       8, 0x7F, 0xFF },
			{ PS3AV_MON_INFO_AUDIO_TYPE_DDP,       8, 0x7F, 0xFF },
			{ PS3AV_MON_INFO_AUDIO_TYPE_DTS_HD,    8, 0x7F, 0xFF },
			{ PS3AV_MON_INFO_AUDIO_TYPE_DOLBY_THD, 8, 0x7F, 0xFF },
		};

		cfg.avport = avport;
		memcpy(cfg.monitor_id, mon_id, sizeof(cfg.monitor_id));
		cfg.monitor_type = PS3AV_MONITOR_TYPE_HDMI;
		memcpy(cfg.monitor_name, mon_name, sizeof(cfg.monitor_name));

		const u32 native_res = [&]()
		{
			switch (g_cfg.video.resolution)
			{
			case video_resolution::_1080p:
				return PS3AV_RESBIT_1920x1080P;
			case video_resolution::_1080i:
				return PS3AV_RESBIT_1920x1080I;
			case video_resolution::_1600x1080p:
			case video_resolution::_1440x1080p:
			case video_resolution::_1280x1080p:
			case video_resolution::_720p:
				return PS3AV_RESBIT_1280x720P;
			case video_resolution::_576p:
				return PS3AV_RESBIT_720x576P;
			default:
				return PS3AV_RESBIT_720x480P;
			}
		}();

		cfg.res_60.res_bits = UINT32_MAX;
		cfg.res_60.native = native_res;
		cfg.res_50.res_bits = UINT32_MAX;
		cfg.res_50.native = native_res;
		cfg.res_other.res_bits = UINT32_MAX;
		cfg.res_vesa.res_bits = 1; // Always one mode at a time

		cfg.cs.rgb = PS3AV_CS_SUPPORTED | PS3AV_RGB_SELECTABLE_QAUNTIZATION_RANGE | PS3AV_12BIT_COLOR;
		cfg.cs.yuv444 = PS3AV_CS_SUPPORTED | PS3AV_12BIT_COLOR;
		cfg.cs.yuv422 = PS3AV_CS_SUPPORTED;
		cfg.cs.colorimetry_data = PS3AV_COLORIMETRY_xvYCC_601 | PS3AV_COLORIMETRY_xvYCC_709 | PS3AV_COLORIMETRY_MD0 | PS3AV_COLORIMETRY_MD1 | PS3AV_COLORIMETRY_MD2;

		cfg.color.red_x = 1023;
		cfg.color.red_y = 0;
		cfg.color.green_x = 0;
		cfg.color.green_y = 1023;
		cfg.color.blue_x = 0;
		cfg.color.blue_y = 0;
		cfg.color.white_x = 341;
		cfg.color.white_y = 341;
		cfg.color.gamma = 100;

		cfg.supported_ai = 1;
		cfg.speaker_info = 0x4F;

		// Audio formats
		cfg.num_of_audio_block = 7;
		memcpy(cfg.audio_info, audio_info, sizeof(cfg.audio_info));

		// 16:9 27-inch (as a default)
		cfg.hor_screen_size = 60;
		cfg.ver_screen_size = 34;

		cfg.supported_content_types = 0b1111; // Graphics, cinema, photo, game

		// 3D modes, no native formats
		cfg.res_60_packed_3D.res_bits = UINT32_MAX;
		cfg.res_50_packed_3D.res_bits = UINT32_MAX;
		cfg.res_other_3D.res_bits = UINT32_MAX;
		cfg.res_60_sbs_3D.res_bits = UINT32_MAX;
		cfg.res_50_sbs_3D.res_bits = UINT32_MAX;

		cfg.vendor_specific_flags = 0; // values from 0-3 (unk)
	}
};

struct av_get_bksv_list_cmd : public ps3av_cmd
{
	u16 get_size(vuart_av_thread& /*vuart*/, const void* /*pkt_buf*/) override
	{
		return sizeof(ps3av_pkt_get_bksv);
	}

	void execute(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_pkt_get_bksv *>(pkt_buf);

		if (vuart.get_reply_buf_free_size() < sizeof(ps3av_pkt_reply_hdr))
		{
			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_BUFFER_OVERFLOW);
			return;
		}

		if (pkt->avport > static_cast<u16>(UartAudioAvport::HDMI_1))
		{
			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_INVALID_PORT);
			return;
		}

		if (pkt->avport == static_cast<u16>(UartAudioAvport::HDMI_1) && !g_cfg.core.debug_console_mode)
		{
			vuart.write_resp<true>(pkt->hdr.cid, PS3AV_STATUS_SYSCON_COMMUNICATE_FAIL);
			return;
		}

		ps3av_pkt_get_bksv_reply reply{};
		reply.avport = pkt->avport;
		u16 pkt_size = offsetof(ps3av_pkt_get_bksv_reply, ksv_arr);

		if (vuart.hdmi_behavior_mode == PS3AV_HDMI_BEHAVIOR_NORMAL || !(vuart.hdmi_behavior_mode & PS3AV_HDMI_BEHAVIOR_HDCP_OFF))
		{
			reply.ksv_cnt = 1;
			memcpy(reply.ksv_arr[0], PS3AV_BKSV_VALUE, sizeof(PS3AV_BKSV_VALUE));
			pkt_size = (pkt_size + 5 * reply.ksv_cnt + 3) & 0xFFFC;
		}

		vuart.write_resp<true>(pkt->hdr.cid, PS3AV_STATUS_SUCCESS, &reply, pkt_size);
	}
};

struct av_enable_event_cmd : public ps3av_cmd
{
	u16 get_size(vuart_av_thread& /*vuart*/, const void* /*pkt_buf*/) override
	{
		return sizeof(ps3av_pkt_enable_event);
	}

	void execute(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_pkt_enable_event *>(pkt_buf);

		if (vuart.get_reply_buf_free_size() < sizeof(ps3av_pkt_reply_hdr))
		{
			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_BUFFER_OVERFLOW);
			return;
		}

		vuart.hdmi_events_bitmask |= pkt->event_bit;

		vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_SUCCESS);
	}
};

struct av_disable_event_cmd : public ps3av_cmd
{
	u16 get_size(vuart_av_thread& /*vuart*/, const void* /*pkt_buf*/) override
	{
		return sizeof(ps3av_pkt_enable_event);
	}

	void execute(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_pkt_enable_event *>(pkt_buf);

		if (vuart.get_reply_buf_free_size() < sizeof(ps3av_pkt_reply_hdr))
		{
			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_BUFFER_OVERFLOW);
			return;
		}

		vuart.hdmi_events_bitmask &= ~pkt->event_bit;

		vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_SUCCESS);
	}
};

struct av_tv_mute_cmd : public ps3av_cmd
{
	u16 get_size(vuart_av_thread& /*vuart*/, const void* /*pkt_buf*/) override
	{
		return sizeof(ps3av_pkt_av_audio_mute);
	}

	// Behavior is unknown, but it seems that this pkt could be ignored
	void execute(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_pkt_av_audio_mute *>(pkt_buf);

		if (vuart.get_reply_buf_free_size() < sizeof(ps3av_pkt_reply_hdr))
		{
			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_BUFFER_OVERFLOW);
			return;
		}

		if (pkt->avport < (g_cfg.core.debug_console_mode ? 2 : 1))
		{
			sys_uart.notice("[av_tv_mute_cmd] tv mute set to %u", pkt->mute > 0);
		}

		vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_SUCCESS);
	}
};

struct av_null_cmd : public ps3av_cmd
{
	u16 get_size(vuart_av_thread& /*vuart*/, const void* /*pkt_buf*/) override
	{
		return 12;
	}

	void execute(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_header *>(pkt_buf);

		if (vuart.get_reply_buf_free_size() < sizeof(ps3av_pkt_reply_hdr))
		{
			vuart.write_resp(pkt->cid, PS3AV_STATUS_BUFFER_OVERFLOW);
			return;
		}

		vuart.write_resp(pkt->cid, PS3AV_STATUS_SUCCESS);
	}
};

struct av_get_aksv_list_cmd : public ps3av_cmd
{
	u16 get_size(vuart_av_thread& /*vuart*/, const void* /*pkt_buf*/) override
	{
		return sizeof(ps3av_header);
	}

	void execute(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_header *>(pkt_buf);

		if (vuart.get_reply_buf_free_size() < sizeof(ps3av_pkt_reply_hdr) + sizeof(ps3av_pkt_get_aksv_reply))
		{
			vuart.write_resp(pkt->cid, PS3AV_STATUS_BUFFER_OVERFLOW);
			return;
		}

		ps3av_pkt_get_aksv_reply reply{};
		memcpy(reply.ksv_arr[0], PS3AV_AKSV_VALUE, sizeof(PS3AV_AKSV_VALUE));

		if (g_cfg.core.debug_console_mode)
		{
			memcpy(reply.ksv_arr[1], PS3AV_AKSV_VALUE, sizeof(PS3AV_AKSV_VALUE));
			reply.ksv_size = 2 * sizeof(PS3AV_AKSV_VALUE);
		}
		else
		{
			reply.ksv_size = sizeof(PS3AV_AKSV_VALUE);
		}

		vuart.write_resp(pkt->cid, PS3AV_STATUS_SUCCESS, &reply, sizeof(ps3av_pkt_get_aksv_reply));
	}
};

struct video_disable_signal_cmd : public ps3av_cmd
{
	u16 get_size(vuart_av_thread& /*vuart*/, const void* /*pkt_buf*/) override
	{
		return sizeof(ps3av_pkt_video_disable_sig);
	}

	// Cross color reduction filter setting in vsh. (AVMULTI)
	void execute(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_pkt_video_disable_sig *>(pkt_buf);

		if (vuart.get_reply_buf_free_size() < sizeof(ps3av_pkt_reply_hdr))
		{
			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_BUFFER_OVERFLOW);
			return;
		}

		if (pkt->avport <= static_cast<u16>(UartAudioAvport::HDMI_1))
		{
			g_fxo->get<rsx_audio_data>().update_av_mute_state(vuart.avport_to_idx(static_cast<UartAudioAvport>(pkt->avport.get())), false, true);

			if (pkt->avport == static_cast<u16>(UartAudioAvport::HDMI_1) && !g_cfg.core.debug_console_mode)
			{
				vuart.write_resp<true>(pkt->hdr.cid, PS3AV_STATUS_SYSCON_COMMUNICATE_FAIL);
				return;
			}

			vuart.hdmi_res_set[pkt->avport == static_cast<u16>(UartAudioAvport::HDMI_1)] = false;
			vuart.write_resp<true>(pkt->hdr.cid, PS3AV_STATUS_SUCCESS);
		}
		else if (pkt->avport == static_cast<u16>(UartAudioAvport::AVMULTI_0))
		{
			if (vuart.head_b_initialized)
			{
				vuart.write_resp<true>(pkt->hdr.cid, PS3AV_STATUS_SUCCESS);
			}
		}
		else
		{
			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_SUCCESS);
		}
	}
};

struct av_video_ytrapcontrol_cmd : public ps3av_cmd
{
	u16 get_size(vuart_av_thread& /*vuart*/, const void* /*pkt_buf*/) override
	{
		return sizeof(ps3av_pkt_av_video_ytrapcontrol);
	}

	// Cross color reduction filter setting in vsh. (AVMULTI)
	void execute(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_pkt_av_video_ytrapcontrol *>(pkt_buf);

		if (vuart.get_reply_buf_free_size() < sizeof(ps3av_get_hw_info_reply) + sizeof(ps3av_pkt_reply_hdr))
		{
			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_BUFFER_OVERFLOW);
			return;
		}

		if (pkt->unk1 && pkt->unk1 != 5U)
		{
			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_SUCCESS);
			return;
		}

		sys_uart.notice("[av_video_ytrapcontrol_cmd] unk1=0x%04x unk2=0x%04x", pkt->unk1, pkt->unk2);

		vuart.write_resp<true>(pkt->hdr.cid, PS3AV_STATUS_SUCCESS);
	}
};

struct av_audio_mute_cmd : public ps3av_cmd
{
	u16 get_size(vuart_av_thread& /*vuart*/, const void* /*pkt_buf*/) override
	{
		return sizeof(ps3av_pkt_av_audio_mute);
	}

	void execute(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_pkt_av_audio_mute *>(pkt_buf);

		if (pkt->avport == static_cast<u16>(UartAudioAvport::AVMULTI_1))
		{
			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_INVALID_PORT);
			return;
		}

		if ((pkt->avport > static_cast<u16>(UartAudioAvport::HDMI_1) && pkt->avport != static_cast<u16>(UartAudioAvport::AVMULTI_0)) ||
			(pkt->avport == static_cast<u16>(UartAudioAvport::HDMI_1) && !g_cfg.core.debug_console_mode))
		{
			vuart.write_resp<true>(pkt->hdr.cid, PS3AV_STATUS_SYSCON_COMMUNICATE_FAIL);
			return;
		}

		g_fxo->get<rsx_audio_data>().update_av_mute_state(vuart.avport_to_idx(static_cast<UartAudioAvport>(pkt->avport.get())), true, false, pkt->mute);

		vuart.write_resp<true>(pkt->hdr.cid, PS3AV_STATUS_SUCCESS);
	}
};

struct av_acp_ctrl_cmd : public ps3av_cmd
{
	u16 get_size(vuart_av_thread& /*vuart*/, const void* /*pkt_buf*/) override
	{
		return sizeof(ps3av_pkt_acp_ctrl);
	}

	void execute(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_pkt_acp_ctrl *>(pkt_buf);

		if (pkt->avport > static_cast<u8>(UartAudioAvport::HDMI_1) ||
			(pkt->avport == static_cast<u8>(UartAudioAvport::HDMI_1) && !g_cfg.core.debug_console_mode))
		{
			vuart.write_resp<true>(pkt->hdr.cid, PS3AV_STATUS_INVALID_PORT);
			return;
		}

		sys_uart.notice("[av_acp_ctrl_cmd] HDMI_%u data island ctrl pkt ctrl=0x%02x", pkt->avport, pkt->packetctl);

		vuart.write_resp<true>(pkt->hdr.cid, PS3AV_STATUS_SUCCESS);
	}
};

struct av_set_acp_packet_cmd : public ps3av_cmd
{
	u16 get_size(vuart_av_thread& /*vuart*/, const void* /*pkt_buf*/) override
	{
		return sizeof(ps3av_pkt_set_acp_packet);
	}

	void execute(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_pkt_set_acp_packet *>(pkt_buf);

		if (pkt->avport > static_cast<u8>(UartAudioAvport::HDMI_1) ||
			(pkt->avport == static_cast<u8>(UartAudioAvport::HDMI_1) && !g_cfg.core.debug_console_mode) ||
			(pkt->pkt_type > 0x0A && pkt->pkt_type < 0x81) ||
			pkt->pkt_type > 0x85)
		{
			vuart.write_resp<true>(pkt->hdr.cid, PS3AV_STATUS_INVALID_PORT);
			return;
		}

		sys_uart.notice("[av_set_acp_packet_cmd] HDMI_%u data island pkt type=0x%02x", pkt->avport, pkt->pkt_type);

		vuart.write_resp<true>(pkt->hdr.cid, PS3AV_STATUS_SUCCESS);
	}
};

struct av_add_signal_ctl_cmd : public ps3av_cmd
{
	u16 get_size(vuart_av_thread& /*vuart*/, const void* /*pkt_buf*/) override
	{
		return sizeof(ps3av_pkt_add_signal_ctl);
	}

	void execute(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_pkt_add_signal_ctl *>(pkt_buf);

		if (vuart.get_reply_buf_free_size() < sizeof(ps3av_pkt_reply_hdr))
		{
			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_BUFFER_OVERFLOW);
			return;
		}

		if (pkt->avport != static_cast<u16>(UartAudioAvport::AVMULTI_0))
		{
			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_INVALID_PORT);
			return;
		}

		sys_uart.notice("[av_add_signal_ctl_cmd] signal_ctl=0x%04x", pkt->signal_ctl);

		vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_SUCCESS);
	}
};

struct av_set_cgms_wss_cmd : public ps3av_cmd
{
	u16 get_size(vuart_av_thread& /*vuart*/, const void* /*pkt_buf*/) override
	{
		return sizeof(ps3av_pkt_av_set_cgms_wss);
	}

	// Something related to copy control on AVMULTI.
	void execute(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_pkt_av_set_cgms_wss *>(pkt_buf);

		if (vuart.get_reply_buf_free_size() < sizeof(ps3av_pkt_reply_hdr))
		{
			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_BUFFER_OVERFLOW);
			return;
		}

		if (pkt->avport != static_cast<u16>(UartAudioAvport::AVMULTI_0))
		{
			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_INVALID_PORT);
			return;
		}

		sys_uart.notice("[av_set_cgms_wss_cmd] cgms_wss=0x%08x", pkt->cgms_wss);

		vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_SUCCESS);
	}
};

struct av_get_hw_conf_cmd : public ps3av_cmd
{
	u16 get_size(vuart_av_thread& /*vuart*/, const void* /*pkt_buf*/) override
	{
		return sizeof(ps3av_header);
	}

	void execute(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_header *>(pkt_buf);

		if (vuart.get_reply_buf_free_size() < sizeof(ps3av_get_hw_info_reply) + sizeof(ps3av_pkt_reply_hdr))
		{
			vuart.write_resp(pkt->cid, PS3AV_STATUS_BUFFER_OVERFLOW);
			return;
		}

		ps3av_get_hw_info_reply out{};
		out.num_of_hdmi = g_cfg.core.debug_console_mode ? 2 : 1;
		out.num_of_avmulti = 1;
		out.num_of_spdif = 1;
		out.extra_bistream_support = 1;

		vuart.write_resp(pkt->cid, PS3AV_STATUS_SUCCESS, &out, sizeof(ps3av_get_hw_info_reply));
	}
};

struct av_set_hdmi_mode_cmd : public ps3av_cmd
{
	u16 get_size(vuart_av_thread& /*vuart*/, const void* /*pkt_buf*/) override
	{
		return sizeof(ps3av_pkt_set_hdmi_mode);
	}

	void execute(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_pkt_set_hdmi_mode *>(pkt_buf);

		if (pkt->mode != PS3AV_HDMI_BEHAVIOR_NORMAL)
		{
			if ((pkt->mode & PS3AV_HDMI_BEHAVIOR_HDCP_OFF) && !g_cfg.core.debug_console_mode)
			{
				vuart.write_resp<true>(pkt->hdr.cid, PS3AV_STATUS_UNSUPPORTED_HDMI_MODE);
				return;
			}

			if (pkt->mode & ~(PS3AV_HDMI_BEHAVIOR_HDCP_OFF | PS3AV_HDMI_BEHAVIOR_EDID_PASS | PS3AV_HDMI_BEHAVIOR_DVI))
			{
				sys_uart.warning("[av_set_hdmi_mode_cmd] Unknown bits in hdmi mode: 0x%02x", pkt->mode);
			}
		}

		vuart.hdmi_behavior_mode = pkt->mode;

		vuart.add_hdmi_events(UartHdmiEvent::UNPLUGGED, vuart.hdmi_res_set[0] ? UartHdmiEvent::HDCP_DONE : UartHdmiEvent::PLUGGED, true, false);
		vuart.add_hdmi_events(UartHdmiEvent::UNPLUGGED, vuart.hdmi_res_set[1] ? UartHdmiEvent::HDCP_DONE : UartHdmiEvent::PLUGGED, false, true);
		vuart.write_resp<true>(pkt->hdr.cid, PS3AV_STATUS_SUCCESS);
	}
};

struct av_get_cec_status_cmd : public ps3av_cmd
{
	u16 get_size(vuart_av_thread& /*vuart*/, const void* /*pkt_buf*/) override
	{
		return sizeof(ps3av_header);
	}

	void execute(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_header *>(pkt_buf);

		if (vuart.get_reply_buf_free_size() < sizeof(ps3av_pkt_av_get_cec_config_reply) + sizeof(ps3av_pkt_reply_hdr))
		{
			vuart.write_resp(pkt->cid, PS3AV_STATUS_BUFFER_OVERFLOW);
			return;
		}

		const ps3av_pkt_av_get_cec_config_reply reply{1};
		vuart.write_resp(pkt->cid, PS3AV_STATUS_SUCCESS, &reply, sizeof(ps3av_pkt_av_get_cec_config_reply));
	}
};

struct video_init_cmd : public ps3av_cmd
{
	u16 get_size(vuart_av_thread& /*vuart*/, const void* /*pkt_buf*/) override
	{
		return sizeof(ps3av_header);
	}

	void execute(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_header *>(pkt_buf);

		if (vuart.get_reply_buf_free_size() < sizeof(ps3av_pkt_reply_hdr))
		{
			vuart.write_resp(pkt->cid, PS3AV_STATUS_BUFFER_OVERFLOW);
			return;
		}

		vuart.write_resp(pkt->cid, PS3AV_STATUS_SUCCESS);
	}
};

struct video_set_format_cmd : public ps3av_cmd
{
	u16 get_size(vuart_av_thread& /*vuart*/, const void* /*pkt_buf*/) override
	{
		return sizeof(ps3av_pkt_video_format);
	}

	void execute(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_pkt_video_format *>(pkt_buf);

		if (vuart.get_reply_buf_free_size() < sizeof(ps3av_pkt_reply_hdr))
		{
			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_BUFFER_OVERFLOW);
			return;
		}

		if (pkt->video_head > PS3AV_HEAD_B_ANALOG || pkt->video_order > 1 || pkt->video_format > 16)
		{
			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_INVALID_VIDEO_PARAM);
			return;
		}

		vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_SUCCESS);
	}
};

struct video_set_route_cmd : public ps3av_cmd
{
	u16 get_size(vuart_av_thread& /*vuart*/, const void* /*pkt_buf*/) override
	{
		return 24;
	}

	void execute(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_header *>(pkt_buf);

		if (vuart.get_reply_buf_free_size() < sizeof(ps3av_pkt_reply_hdr))
		{
			vuart.write_resp(pkt->cid, PS3AV_STATUS_BUFFER_OVERFLOW);
			return;
		}

		vuart.write_resp(pkt->cid, PS3AV_STATUS_NO_SEL); // Only available in PS2_GX_LPAR
	}
};

struct video_set_pitch_cmd : public ps3av_cmd
{
	u16 get_size(vuart_av_thread& /*vuart*/, const void* /*pkt_buf*/) override
	{
		return sizeof(ps3av_pkt_video_set_pitch);
	}

	void execute(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_pkt_video_set_pitch *>(pkt_buf);

		if (vuart.get_reply_buf_free_size() < sizeof(ps3av_pkt_reply_hdr))
		{
			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_BUFFER_OVERFLOW);
			return;
		}

		if (pkt->video_head > PS3AV_HEAD_B_ANALOG || (pkt->pitch & 7) != 0U || pkt->pitch > UINT16_MAX)
		{
			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_INVALID_VIDEO_PARAM);
			return;
		}

		vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_SUCCESS);
	}
};

struct video_get_hw_cfg_cmd : public ps3av_cmd
{
	u16 get_size(vuart_av_thread& /*vuart*/, const void* /*pkt_buf*/) override
	{
		return sizeof(ps3av_header);
	}

	void execute(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_header *>(pkt_buf);

		if (vuart.get_reply_buf_free_size() < sizeof(ps3av_pkt_video_get_hw_cfg_reply) + sizeof(ps3av_pkt_reply_hdr))
		{
			vuart.write_resp(pkt->cid, PS3AV_STATUS_BUFFER_OVERFLOW);
			return;
		}

		ps3av_pkt_video_get_hw_cfg_reply reply{};
		reply.gx_available = 0; // Set to 1 only in PS2_GX_LPAR

		vuart.write_resp(pkt->cid, PS3AV_STATUS_SUCCESS, &reply, sizeof(ps3av_pkt_video_get_hw_cfg_reply));
	}
};

struct audio_init_cmd : public ps3av_cmd
{
	u16 get_size(vuart_av_thread& /*vuart*/, const void* /*pkt_buf*/) override
	{
		return sizeof(ps3av_header);
	}

	void execute(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_header *>(pkt_buf);

		if (vuart.get_reply_buf_free_size() < sizeof(ps3av_pkt_reply_hdr))
		{
			vuart.write_resp(pkt->cid, PS3AV_STATUS_BUFFER_OVERFLOW);
			return;
		}

		g_fxo->get<rsx_audio_data>().reset_hw();

		vuart.write_resp(pkt->cid, PS3AV_STATUS_SUCCESS);
	}
};

struct audio_set_mode_cmd : public ps3av_cmd
{
	u16 get_size(vuart_av_thread& /*vuart*/, const void* /*pkt_buf*/) override
	{
		return sizeof(ps3av_pkt_audio_mode);
	}

	void execute(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_pkt_audio_mode *>(pkt_buf);

		if (vuart.get_reply_buf_free_size() < sizeof(ps3av_pkt_reply_hdr))
		{
			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_BUFFER_OVERFLOW);
			return;
		}

		if (!set_mode(*pkt))
		{
			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_INVALID_AUDIO_PARAM);
		}
		else
		{
			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_SUCCESS);
		}
	}

private:

	bool set_mode(const ps3av_pkt_audio_mode& pkt)
	{
		bool spdif_use_serial_buf = false;
		RsxaudioPort avport_src, rsxaudio_port;
		RsxaudioAvportIdx avport_idx;

		switch (pkt.avport)
		{
		case UartAudioAvport::HDMI_0:
		{
			avport_idx = RsxaudioAvportIdx::HDMI_0;
			if (pkt.audio_source == UartAudioSource::SPDIF)
			{
				avport_src = rsxaudio_port = RsxaudioPort::SPDIF_1;
			}
			else
			{
				avport_src = rsxaudio_port = RsxaudioPort::SERIAL;
			}
			break;
		}
		case UartAudioAvport::HDMI_1:
		{
			avport_idx = RsxaudioAvportIdx::HDMI_1;
			if (pkt.audio_source == UartAudioSource::SPDIF)
			{
				avport_src = rsxaudio_port = RsxaudioPort::SPDIF_1;
			}
			else
			{
				avport_src = rsxaudio_port = RsxaudioPort::SERIAL;
			}
			break;
		}
		case UartAudioAvport::AVMULTI_0:
		{
			avport_idx = RsxaudioAvportIdx::AVMULTI;
			avport_src = rsxaudio_port = RsxaudioPort::SERIAL;
			break;
		}
		case UartAudioAvport::SPDIF_0:
		{
			avport_idx    = RsxaudioAvportIdx::SPDIF_0;
			rsxaudio_port = RsxaudioPort::SPDIF_0;
			if (pkt.audio_source == UartAudioSource::SERIAL)
			{
				spdif_use_serial_buf = true;
				avport_src           = RsxaudioPort::SERIAL;
			}
			else
			{
				avport_src = RsxaudioPort::SPDIF_0;
			}

			break;
		}
		case UartAudioAvport::SPDIF_1:
		{
			avport_idx    = RsxaudioAvportIdx::SPDIF_1;
			rsxaudio_port = RsxaudioPort::SPDIF_1;
			if (pkt.audio_source == UartAudioSource::SERIAL)
			{
				spdif_use_serial_buf = true;
				avport_src           = RsxaudioPort::SERIAL;
			}
			else
			{
				avport_src = RsxaudioPort::SPDIF_1;
			}

			break;
		}
		default:
		{
			return false;
		}
		}

		if (static_cast<u32>(pkt.audio_fs.value()) > static_cast<u32>(UartAudioFreq::_192K)) return false;

		const auto bit_cnt = [&]()
		{
			if ((rsxaudio_port != RsxaudioPort::SERIAL && pkt.audio_format != UartAudioFormat::PCM) ||
				pkt.audio_word_bits == UartAudioSampleSize::_16BIT)
			{
				return UartAudioSampleSize::_16BIT;
			}
			else
			{
				return UartAudioSampleSize::_24BIT;
			}
		}();

		return commit_param(rsxaudio_port, avport_idx, avport_src, pkt.audio_fs, bit_cnt, spdif_use_serial_buf, pkt.audio_cs_info);
	}

	bool commit_param(RsxaudioPort rsxaudio_port, RsxaudioAvportIdx avport, RsxaudioPort avport_src, UartAudioFreq freq,
						UartAudioSampleSize bit_cnt, bool spdif_use_serial_buf, const u8 *cs_data)
	{
		auto& rsxaudio_thread = g_fxo->get<rsx_audio_data>();
		const auto avport_idx = static_cast<std::underlying_type_t<decltype(avport)>>(avport);
		const auto rsxaudio_word_depth = bit_cnt == UartAudioSampleSize::_16BIT ? RsxaudioSampleSize::_16BIT : RsxaudioSampleSize::_32BIT;
		const auto freq_param = [&]()
		{
			switch (freq)
			{
			case UartAudioFreq::_44K:  return std::make_pair(8, SYS_RSXAUDIO_FREQ_BASE_352K);
			default:
			case UartAudioFreq::_48K:  return std::make_pair(8, SYS_RSXAUDIO_FREQ_BASE_384K);
			case UartAudioFreq::_88K:  return std::make_pair(4, SYS_RSXAUDIO_FREQ_BASE_352K);
			case UartAudioFreq::_96K:  return std::make_pair(4, SYS_RSXAUDIO_FREQ_BASE_384K);
			case UartAudioFreq::_176K: return std::make_pair(2, SYS_RSXAUDIO_FREQ_BASE_352K);
			case UartAudioFreq::_192K: return std::make_pair(2, SYS_RSXAUDIO_FREQ_BASE_384K);
			}
		}();

		switch (rsxaudio_port)
		{
		case RsxaudioPort::SERIAL:
		{
			rsxaudio_thread.update_hw_param([&](auto& obj)
			{
				obj.serial_freq_base = freq_param.second;
				obj.serial.freq_div = freq_param.first;
				obj.serial.depth = rsxaudio_word_depth;
				obj.serial.buf_empty_en = true;
				obj.avport_src[avport_idx] = avport_src;
			});
			break;
		}
		case RsxaudioPort::SPDIF_0:
		case RsxaudioPort::SPDIF_1:
		{
			const u8 spdif_idx = rsxaudio_port == RsxaudioPort::SPDIF_1;

			rsxaudio_thread.update_hw_param([&](auto& obj)
			{
				obj.spdif_freq_base = freq_param.second;
				obj.spdif[spdif_idx].freq_div = freq_param.first;
				obj.spdif[spdif_idx].depth = rsxaudio_word_depth;
				obj.spdif[spdif_idx].use_serial_buf = spdif_use_serial_buf;
				obj.spdif[spdif_idx].buf_empty_en = true;
				obj.avport_src[avport_idx] = avport_src;
				memcpy(obj.spdif[spdif_idx].cs_data.data(), cs_data, sizeof(obj.spdif[spdif_idx].cs_data));
			});
			break;
		}
		default:
		{
			return false;
		}
		}

		return true;
	}
};

struct audio_mute_cmd : public ps3av_cmd
{
	u16 get_size(vuart_av_thread& /*vuart*/, const void* /*pkt_buf*/) override
	{
		// From RE
		return 0;
	}

	void execute(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_pkt_audio_mute *>(pkt_buf);

		if (vuart.get_reply_buf_free_size() < sizeof(ps3av_pkt_reply_hdr))
		{
			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_BUFFER_OVERFLOW);
			return;
		}

		switch (pkt->avport)
		{
		case UartAudioAvport::HDMI_0:
		case UartAudioAvport::HDMI_1:
		case UartAudioAvport::AVMULTI_0:
		case UartAudioAvport::AVMULTI_1:
			g_fxo->get<rsx_audio_data>().update_mute_state(RsxaudioPort::SERIAL, pkt->mute);
			break;
		case UartAudioAvport::SPDIF_0:
			g_fxo->get<rsx_audio_data>().update_mute_state(RsxaudioPort::SPDIF_0, pkt->mute);
			break;
		case UartAudioAvport::SPDIF_1:
			g_fxo->get<rsx_audio_data>().update_mute_state(RsxaudioPort::SPDIF_1, pkt->mute);
			break;
		default:
			break;
		}

		vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_SUCCESS);
	}
};

struct audio_set_active_cmd : public ps3av_cmd
{
	u16 get_size(vuart_av_thread& /*vuart*/, const void* /*pkt_buf*/) override
	{
		return sizeof(ps3av_pkt_audio_set_active);
	}

	void execute(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_pkt_audio_set_active *>(pkt_buf);

		if (vuart.get_reply_buf_free_size() < sizeof(ps3av_pkt_reply_hdr))
		{
			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_BUFFER_OVERFLOW);
			return;
		}

		const bool requested_avports[SYS_RSXAUDIO_AVPORT_CNT] =
		{
			(pkt->audio_port & PS3AV_AUDIO_PORT_HDMI_0) != 0U,
			(pkt->audio_port & PS3AV_AUDIO_PORT_HDMI_1) != 0U,
			(pkt->audio_port & PS3AV_AUDIO_PORT_AVMULTI) != 0U,
			(pkt->audio_port & PS3AV_AUDIO_PORT_SPDIF_0) != 0U,
			(pkt->audio_port & PS3AV_AUDIO_PORT_SPDIF_1) != 0U
		};

		g_fxo->get<rsx_audio_data>().update_hw_param([&](auto &obj)
		{
			for (u8 avport_idx = 0; avport_idx < SYS_RSXAUDIO_AVPORT_CNT; avport_idx++)
			{
				if (requested_avports[avport_idx])
				{
					switch (obj.avport_src[avport_idx])
					{
					case RsxaudioPort::SERIAL:
						obj.serial.en = true;
						break;
					case RsxaudioPort::SPDIF_0:
					case RsxaudioPort::SPDIF_1:
					{
						const u8 spdif_idx = obj.avport_src[avport_idx] == RsxaudioPort::SPDIF_1;
						if (!obj.spdif[spdif_idx].use_serial_buf)
						{
							obj.spdif[spdif_idx].en = true;
						}
						break;
					}
					default:
						break;
					}
				}
			}

			obj.serial.muted = false;
			obj.spdif[1].muted = false;
		});

		vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_SUCCESS);
	}
};

struct audio_set_inactive_cmd : public ps3av_cmd
{
	u16 get_size(vuart_av_thread& /*vuart*/, const void* /*pkt_buf*/) override
	{
		return sizeof(ps3av_pkt_audio_set_active);
	}

	void execute(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_pkt_audio_set_active *>(pkt_buf);

		if (vuart.get_reply_buf_free_size() < sizeof(ps3av_pkt_reply_hdr))
		{
			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_BUFFER_OVERFLOW);
			return;
		}

		g_fxo->get<rsx_audio_data>().update_hw_param([&](auto &obj)
		{
			if ((pkt->audio_port & 0x8000'0000) == 0U)
			{
				obj.avport_src.fill(RsxaudioPort::INVALID);
			}

			obj.serial.en = false;
			obj.serial.muted = true;
			obj.spdif[1].muted = true;
			for (u8 spdif_idx = 0; spdif_idx < SYS_RSXAUDIO_SPDIF_CNT; spdif_idx++)
			{
				if (!obj.spdif[spdif_idx].use_serial_buf)
				{
					obj.spdif[spdif_idx].en = false;
				}
			}
		});

		vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_SUCCESS);
	}
};

struct audio_spdif_bit_cmd : public ps3av_cmd
{
	u16 get_size(vuart_av_thread& /*vuart*/, const void* /*pkt_buf*/) override
	{
		return sizeof(ps3av_pkt_audio_spdif_bit);
	}

	void execute(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_pkt_audio_spdif_bit *>(pkt_buf);

		if (vuart.get_reply_buf_free_size() < sizeof(ps3av_pkt_reply_hdr))
		{
			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_BUFFER_OVERFLOW);
			return;
		}

		const bool requested_avports[SYS_RSXAUDIO_AVPORT_CNT] =
		{
			(pkt->audio_port & PS3AV_AUDIO_PORT_HDMI_0) != 0U,
			(pkt->audio_port & PS3AV_AUDIO_PORT_HDMI_1) != 0U,
			(pkt->audio_port & PS3AV_AUDIO_PORT_AVMULTI) != 0U,
			(pkt->audio_port & PS3AV_AUDIO_PORT_SPDIF_0) != 0U,
			(pkt->audio_port & PS3AV_AUDIO_PORT_SPDIF_1) != 0U
		};

		g_fxo->get<rsx_audio_data>().update_hw_param([&](auto &obj)
		{
			for (u8 avport_idx = 0; avport_idx < SYS_RSXAUDIO_AVPORT_CNT; avport_idx++)
			{
				if (requested_avports[avport_idx] && obj.avport_src[avport_idx] == RsxaudioPort::SPDIF_0)
				{
					auto &b_data = pkt->spdif_bit_data;

					sys_uart.notice("[audio_spdif_bit_cmd] Data 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x",
									b_data[0], b_data[1], b_data[2], b_data[3], b_data[4], b_data[5], b_data[6], b_data[7],
									b_data[8], b_data[9], b_data[10], b_data[11]);
					break;
				}
			}
		});

		vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_SUCCESS);
	}
};

struct audio_ctrl_cmd : public ps3av_cmd
{
	u16 get_size(vuart_av_thread& /*vuart*/, const void* /*pkt_buf*/) override
	{
		return sizeof(ps3av_pkt_audio_ctrl);
	}

	void execute(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_pkt_audio_ctrl *>(pkt_buf);

		if (vuart.get_reply_buf_free_size() < sizeof(ps3av_pkt_reply_hdr))
		{
			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_BUFFER_OVERFLOW);
			return;
		}

		switch (pkt->audio_ctrl_id)
		{
		case UartAudioCtrlID::DAC_RESET:
		case UartAudioCtrlID::DAC_DE_EMPHASIS:
		case UartAudioCtrlID::AVCLK:
			sys_uart.notice("[audio_ctrl_cmd] Option 0x%x", pkt->audio_ctrl_id);
			break;
		default:
			break;
		}

		vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_SUCCESS);
	}
};

struct inc_avset_cmd : public ps3av_cmd
{
	u16 get_size(vuart_av_thread& /*vuart*/, const void* pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_pkt_inc_avset *>(pkt_buf);

		if (pkt->num_of_video_pkt > 2 || pkt->num_of_av_video_pkt > 4 || pkt->num_of_av_audio_pkt > 4)
		{
			return -1;
		}

		const auto data_start = static_cast<const u8 *>(pkt_buf) + sizeof(ps3av_pkt_inc_avset);

		u64 video_pkt_sec_size = 0;
		u64 av_video_pkt_sec_size = 0;
		u64 av_audio_pkt_sec_size = 0;

		for (u16 pkt_idx = 0; pkt_idx < pkt->num_of_video_pkt; pkt_idx++)
		{
			video_pkt_sec_size += reinterpret_cast<const ps3av_header *>(&data_start[video_pkt_sec_size])->length + 4ULL;
		}

		for (u16 pkt_idx = 0; pkt_idx < pkt->num_of_av_video_pkt; pkt_idx++)
		{
			av_video_pkt_sec_size += reinterpret_cast<const ps3av_header *>(&data_start[video_pkt_sec_size + av_video_pkt_sec_size])->length + 4ULL;
		}

		for (u16 pkt_idx = 0; pkt_idx < pkt->num_of_av_audio_pkt; pkt_idx++)
		{
			av_audio_pkt_sec_size += reinterpret_cast<const ps3av_header *>(&data_start[video_pkt_sec_size + av_video_pkt_sec_size + av_audio_pkt_sec_size])->length + 4ULL;
		}

		return static_cast<u16>(sizeof(ps3av_pkt_inc_avset) + video_pkt_sec_size + av_video_pkt_sec_size + av_audio_pkt_sec_size);
	}

	void execute(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_pkt_inc_avset *>(pkt_buf);
		auto pkt_data_addr = static_cast<const u8 *>(pkt_buf) + sizeof(ps3av_pkt_inc_avset);

		if (vuart.get_reply_buf_free_size() < sizeof(ps3av_pkt_reply_hdr))
		{
			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_BUFFER_OVERFLOW);
			return;
		}

		bool syscon_check_passed = true;

		// Video
		u32 video_cmd_status = PS3AV_STATUS_SUCCESS;
		for (u32 video_pkt_idx = 0; video_pkt_idx < pkt->num_of_video_pkt; ++video_pkt_idx)
		{
			const auto video_pkt = reinterpret_cast<const ps3av_pkt_video_mode *>(pkt_data_addr);
			const u32 subcmd_status = video_pkt_parse(*video_pkt);

			if (video_pkt->video_head == PS3AV_HEAD_B_ANALOG)
			{
				vuart.head_b_initialized = true;
			}

			if (subcmd_status != PS3AV_STATUS_SUCCESS)
			{
				video_cmd_status = subcmd_status;
			}

			pkt_data_addr += video_pkt->hdr.length + 4ULL;
		}

		if (pkt->num_of_av_video_pkt == 0U && pkt->num_of_av_audio_pkt == 0U)
		{
			vuart.write_resp(pkt->hdr.cid, video_cmd_status);
			return;
		}

		bool hdcp_done[2]{};

		// AV Video
		for (u32 video_av_pkt_idx = 0; video_av_pkt_idx < pkt->num_of_av_video_pkt; video_av_pkt_idx++)
		{
			const auto av_video_pkt = reinterpret_cast<const ps3av_pkt_av_video_cs *>(pkt_data_addr);
			const av_video_resp subcmd_resp = av_video_pkt_parse(*av_video_pkt, syscon_check_passed);

			if (subcmd_resp.status != PS3AV_STATUS_SUCCESS)
			{
				vuart.write_resp(pkt->hdr.cid, subcmd_resp.status);
				return;
			}

			if (syscon_check_passed)
			{
				hdcp_done[0] |= subcmd_resp.hdcp_done_event[0];
				hdcp_done[1] |= subcmd_resp.hdcp_done_event[1];
			}

			pkt_data_addr += av_video_pkt->hdr.length + 4ULL;
		}

		vuart.hdmi_res_set[0] = hdcp_done[0];
		vuart.hdmi_res_set[1] = hdcp_done[1];
		vuart.add_hdmi_events(UartHdmiEvent::HDCP_DONE, vuart.hdmi_res_set[0], vuart.hdmi_res_set[1]);

		if (vuart.hdmi_res_set[0])
		{
			g_fxo->get<rsx_audio_data>().update_av_mute_state(RsxaudioAvportIdx::HDMI_0, false, true);
		}
		if (vuart.hdmi_res_set[1])
		{
			g_fxo->get<rsx_audio_data>().update_av_mute_state(RsxaudioAvportIdx::HDMI_1, false, true);
		}

		bool valid_av_audio_pkt = false;

		// AV Audio
		for (u32 audio_av_pkt_idx = 0; audio_av_pkt_idx < pkt->num_of_av_audio_pkt; audio_av_pkt_idx++)
		{
			const auto av_audio_pkt = reinterpret_cast<const ps3av_pkt_av_audio_param *>(pkt_data_addr);

			if (av_audio_pkt->avport <= static_cast<u16>(UartAudioAvport::HDMI_1))
			{
				valid_av_audio_pkt = true;

				if (!syscon_check_passed || (av_audio_pkt->avport == static_cast<u16>(UartAudioAvport::HDMI_1) && !g_cfg.core.debug_console_mode))
				{
					syscon_check_passed = false;
					break;
				}

				const u8 hdmi_idx = av_audio_pkt->avport == static_cast<u16>(UartAudioAvport::HDMI_1);

				g_fxo->get<rsx_audio_data>().update_hw_param([&](auto &obj)
				{
					auto &hdmi = obj.hdmi[hdmi_idx];
					hdmi.init = true;

					const std::array<u8, SYS_RSXAUDIO_SERIAL_STREAM_CNT> fifomap =
					{
						static_cast<u8>((av_audio_pkt->fifomap >> 0) & 3U),
						static_cast<u8>((av_audio_pkt->fifomap >> 2) & 3U),
						static_cast<u8>((av_audio_pkt->fifomap >> 4) & 3U),
						static_cast<u8>((av_audio_pkt->fifomap >> 6) & 3U)
					};

					const std::array<bool, SYS_RSXAUDIO_SERIAL_STREAM_CNT> en_streams =
					{
						static_cast<bool>(av_audio_pkt->enable & 0x10),
						static_cast<bool>(av_audio_pkt->enable & 0x20),
						static_cast<bool>(av_audio_pkt->enable & 0x40),
						static_cast<bool>(av_audio_pkt->enable & 0x80)
					};

					// Might be wrong
					const std::array<bool, SYS_RSXAUDIO_SERIAL_STREAM_CNT> swap_lr =
					{
						static_cast<bool>(av_audio_pkt->swaplr & 0x10),
						static_cast<bool>(av_audio_pkt->swaplr & 0x20),
						static_cast<bool>(av_audio_pkt->swaplr & 0x40),
						static_cast<bool>(av_audio_pkt->swaplr & 0x80)
					};

					memcpy(hdmi.info_frame.data(), av_audio_pkt->info, sizeof(av_audio_pkt->info));
					memcpy(hdmi.chstat.data(), av_audio_pkt->chstat, sizeof(av_audio_pkt->chstat));

					hdmi.ch_cfg = hdmi_param_conv(fifomap, en_streams, swap_lr);
				});
			}

			pkt_data_addr += av_audio_pkt->hdr.length + 4ULL;
		}

		if (pkt->num_of_av_video_pkt || valid_av_audio_pkt)
		{
			if (!syscon_check_passed)
			{
				vuart.write_resp<true>(pkt->hdr.cid, PS3AV_STATUS_SYSCON_COMMUNICATE_FAIL);
				return;
			}

			vuart.write_resp<true>(pkt->hdr.cid, PS3AV_STATUS_SUCCESS);
		}
		else
		{
			vuart.write_resp(pkt->hdr.cid, PS3AV_STATUS_SUCCESS);
		}
	}

private:

	struct video_sce_param
	{
		u32 width_div;
		u32 width;
		u32 height;
	};

	struct av_video_resp
	{
		u32 status = PS3AV_STATUS_SUCCESS;
		bool hdcp_done_event[2]{};
	};

	u32 video_pkt_parse(const ps3av_pkt_video_mode &video_head_cfg)
	{
		static constexpr video_sce_param sce_param_arr[28] =
		{
			{ 0, 0,    0    },
			{ 4, 2880, 480  },
			{ 4, 2880, 480  },
			{ 4, 2880, 576  },
			{ 4, 2880, 576  },
			{ 2, 1440, 480  },
			{ 2, 1440, 576  },
			{ 1, 1920, 1080 },
			{ 1, 1920, 1080 },
			{ 1, 1920, 1080 },
			{ 1, 1280, 720  },
			{ 1, 1280, 720  },
			{ 1, 1920, 1080 },
			{ 1, 1920, 1080 },
			{ 1, 1920, 1080 },
			{ 1, 1920, 1080 },
			{ 1, 1280, 768  },
			{ 1, 1280, 1024 },
			{ 1, 1920, 1200 },
			{ 1, 1360, 768  },
			{ 1, 1280, 1470 },
			{ 1, 1280, 1470 },
			{ 1, 1920, 1080 },
			{ 1, 1920, 2205 },
			{ 1, 1920, 2205 },
			{ 1, 1280, 721  },
			{ 1, 720,  481  },
			{ 1, 720,  577  }
		};

		const auto sce_idx = [&]() -> u8
		{
			switch (video_head_cfg.video_vid)
			{
			case 16: return 1;
			case 1:  return 2;
			case 3:  return 4;
			case 17: return 4;
			case 5:  return 5;
			case 6:  return 6;
			case 18: return 7;
			case 7:  return 8;
			case 33: return 8;
			case 8:  return 9;
			case 34: return 9;
			case 9:  return 10;
			case 31: return 10;
			case 10: return 11;
			case 32: return 11;
			case 11: return 12;
			case 35: return 12;
			case 37: return 12;
			case 12: return 13;
			case 36: return 13;
			case 38: return 13;
			case 19: return 14;
			case 20: return 15;
			case 13: return 16;
			case 14: return 17;
			case 15: return 18;
			case 21: return 19;
			case 22: return 20;
			case 27: return 20;
			case 23: return 21;
			case 28: return 21;
			case 24: return 22;
			case 25: return 23;
			case 29: return 23;
			case 26: return 24;
			case 30: return 24;
			case 39: return 25;
			case 40: return 26;
			case 41: return 27;
			default: return umax;
			}
		}();

		const video_sce_param &sce_param = sce_param_arr[sce_idx];
		if (sce_idx == umax ||
			video_head_cfg.video_head > PS3AV_HEAD_B_ANALOG ||
			video_head_cfg.video_order > 1 ||
			video_head_cfg.video_format > 16 ||
			video_head_cfg.video_out_format > 16 ||
			((1ULL << video_head_cfg.video_out_format) & 0x1CE07) == 0U ||
			video_head_cfg.unk2 > 3 ||
			video_head_cfg.pitch & 7 ||
			video_head_cfg.pitch > UINT16_MAX ||
			(video_head_cfg.width != 1280U && ((video_head_cfg.width & 7) != 0U || video_head_cfg.width > UINT16_MAX)) ||
			(sce_param.width != 720 && video_head_cfg.width > sce_param.width / sce_param.width_div) ||
			!((video_head_cfg.height == 1470U && (sce_param.height == 721 || sce_param.height == 481 || sce_param.height == 577)) || (video_head_cfg.height <= sce_param.height && video_head_cfg.height <= UINT16_MAX)))
		{
			return PS3AV_STATUS_INVALID_VIDEO_PARAM;
		}

		sys_uart.notice("[inc_avset_cmd] new resolution on HEAD_%c width=%u height=%u", video_head_cfg.video_head == PS3AV_HEAD_A_HDMI ? 'A' : 'B', video_head_cfg.width, video_head_cfg.height);

		return PS3AV_STATUS_SUCCESS;
	}

	av_video_resp av_video_pkt_parse(const ps3av_pkt_av_video_cs &pkt, bool &syscon_pkt_valid)
	{
		if (pkt.avport <= static_cast<u16>(UartAudioAvport::HDMI_1))
		{
			if (pkt.av_vid > 23)
			{
				return {PS3AV_STATUS_INVALID_AV_PARAM};
			}

			if (pkt.avport == static_cast<u16>(UartAudioAvport::HDMI_1) && !g_cfg.core.debug_console_mode)
			{
				syscon_pkt_valid = false;
			}
			else if (syscon_pkt_valid)
			{
				// HDMI setup, code 0x80

				av_video_resp resp{};
				resp.hdcp_done_event[pkt.avport] = true;
				return resp;
			}
		}
		else
		{
			if ((pkt.avport != static_cast<u16>(UartAudioAvport::AVMULTI_0) && pkt.avport != static_cast<u16>(UartAudioAvport::AVMULTI_1)) ||
				pkt.av_vid > 23 ||
				(pkt.av_vid > 12 && pkt.av_vid != 18U))
			{
				return {PS3AV_STATUS_INVALID_AV_PARAM};
			}

			if (pkt.avport == static_cast<u16>(UartAudioAvport::AVMULTI_1))
			{
				syscon_pkt_valid = false;
			}
			else if (syscon_pkt_valid)
			{
				// AVMULTI setup
			}
		}

		return {};
	}

	static rsxaudio_hw_param_t::hdmi_param_t::hdmi_ch_cfg_t hdmi_param_conv(const std::array<u8, SYS_RSXAUDIO_SERIAL_STREAM_CNT> &map,
                                                                            const std::array<bool, SYS_RSXAUDIO_SERIAL_STREAM_CNT> &en,
                                                                            const std::array<bool, SYS_RSXAUDIO_SERIAL_STREAM_CNT> &swap)
	{
		std::array<u8, SYS_RSXAUDIO_SERIAL_MAX_CH> result{};
		u8 ch_cnt = 0;

		for (usz stream_idx = 0; stream_idx < SYS_RSXAUDIO_SERIAL_STREAM_CNT; stream_idx++)
		{
			const u8 stream_pos = map[stream_idx];
			if (en[stream_pos])
			{
				result[stream_idx * 2 + 0] = stream_pos * 2 + swap[stream_pos];
				result[stream_idx * 2 + 1] = stream_pos * 2 + !swap[stream_pos];
				ch_cnt = static_cast<u8>((stream_idx + 1) * 2);
			}
			else
			{
				result[stream_idx * 2 + 0] = rsxaudio_hw_param_t::hdmi_param_t::MAP_SILENT_CH;
				result[stream_idx * 2 + 1] = rsxaudio_hw_param_t::hdmi_param_t::MAP_SILENT_CH;
			}
		}

		const AudioChannelCnt ch_cnt_conv = [&]()
		{
			switch (ch_cnt)
			{
			default:
			case 0:
			case 2:
				return AudioChannelCnt::STEREO;
			case 4:
			case 6:
				return AudioChannelCnt::SURROUND_5_1;
			case 8:
				return AudioChannelCnt::SURROUND_7_1;
			}
		}();

		return { result, ch_cnt_conv };
	}
};

struct generic_reply_cmd : public ps3av_cmd
{
	u16 get_size(vuart_av_thread& /*vuart*/, const void* /*pkt_buf*/) override
	{
		return 0;
	}

	void execute(vuart_av_thread &vuart, const void *pkt_buf) override
	{
		const auto pkt = static_cast<const ps3av_header *>(pkt_buf);

		if (vuart.get_reply_buf_free_size() < sizeof(ps3av_pkt_reply_hdr))
		{
			vuart.write_resp(pkt->cid, PS3AV_STATUS_BUFFER_OVERFLOW);
			return;
		}

		sys_uart.todo("Unimplemented cid=0x%08x", pkt->cid);

		vuart.write_resp(pkt->cid, PS3AV_STATUS_SUCCESS);
	}
};

error_code sys_uart_initialize(ppu_thread &ppu)
{
	ppu.state += cpu_flag::wait;

	sys_uart.trace("sys_uart_initialize()");

	if (!g_ps3_process_info.has_root_perm())
	{
		return CELL_ENOSYS;
	}

	auto &vuart_thread = g_fxo->get<vuart_av>();

	if (vuart_thread.initialized.test_and_set())
	{
		return CELL_EPERM;
	}

	return CELL_OK;
}

error_code sys_uart_receive(ppu_thread &ppu, vm::ptr<void> buffer, u64 size, u32 mode)
{
	sys_uart.trace("sys_uart_receive(buffer=*0x%x, size=0x%llx, mode=0x%x)", buffer, size, mode);

	if (!g_ps3_process_info.has_root_perm())
	{
		return CELL_ENOSYS;
	}

	if (!size)
	{
		return CELL_OK;
	}

	if (mode & ~(BLOCKING_BIG_OP | NOT_BLOCKING_BIG_OP))
	{
		return CELL_EINVAL;
	}

	auto &vuart_thread = g_fxo->get<vuart_av>();

	if (!vuart_thread.initialized)
	{
		return CELL_ESRCH;
	}

	if (size > 0x20000U)
	{
		// kmalloc restriction
		fmt::throw_exception("Buffer is too big");
	}

	const std::unique_ptr<u8[]> data = std::make_unique<u8[]>(size);
	u32 read_size = 0;

	auto vuart_read = [&](u8 *buf, u32 buf_size) -> s32
	{
		constexpr u32 ITER_SIZE = 4096;
		std::unique_lock lock(vuart_thread.rx_mutex, std::defer_lock);

		if (!lock.try_lock())
		{
			return CELL_EBUSY;
		}

		u32 read_size = 0;
		u32 remaining = buf_size;

		while (read_size < buf_size)
		{
			const u32 packet_size = std::min(remaining, ITER_SIZE);
			const u32 nread = vuart_thread.read_rx_data(buf + read_size, packet_size);
			read_size += nread;
			remaining -= nread;

			if (nread < packet_size)
				break;
		}

		return read_size;
	};

	if (mode & BLOCKING_BIG_OP)
	{
		// Yield before checking for packets
		lv2_obj::sleep(ppu);

		for (;;)
		{
			if (ppu.is_stopped())
			{
				return {};
			}

			std::unique_lock<shared_mutex> lock(vuart_thread.rx_wake_m);
			const s32 read_result = vuart_read(data.get(), static_cast<u32>(size));

			if (read_result > CELL_OK)
			{
				read_size = read_result;
				break;
			}

			vuart_thread.rx_wake_c.wait_unlock(5000, lock);
		}

		ppu.check_state();
	}
	else // NOT_BLOCKING_BIG_OP
	{
		const s32 read_result = vuart_read(data.get(), static_cast<u32>(size));

		if (read_result <= CELL_OK)
		{
			return read_result;
		}

		read_size = read_result;
	}

	if (!vm::check_addr(buffer.addr(), vm::page_writable, read_size))
	{
		return CELL_EFAULT;
	}

	memcpy(buffer.get_ptr(), data.get(), read_size);
	return not_an_error(read_size);
}

error_code sys_uart_send(ppu_thread &ppu, vm::cptr<void> buffer, u64 size, u32 mode)
{
	sys_uart.trace("sys_uart_send(buffer=0x%x, size=0x%llx, mode=0x%x)", buffer, size, mode);

	if (!g_ps3_process_info.has_root_perm())
	{
		return CELL_ENOSYS;
	}

	if (!size)
	{
		return CELL_OK;
	}

	if (mode & ~(BLOCKING_BIG_OP | NOT_BLOCKING_OP | NOT_BLOCKING_BIG_OP))
	{
		return CELL_EINVAL;
	}

	auto &vuart_thread = g_fxo->get<vuart_av>();

	if (!vuart_thread.initialized)
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

	constexpr u32 ITER_SIZE = 4096;

	if (mode & BLOCKING_BIG_OP)
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

				std::unique_lock<shared_mutex> lock(vuart_thread.tx_rdy_m);
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

		u32 sent_size = 0;
		u32 remaining = static_cast<u32>(size);

		while (remaining)
		{
			const u32 packet_size = std::min(remaining, ITER_SIZE);
			if (!vuart_send_all(data.get() + sent_size, packet_size)) return {};
			sent_size += packet_size;
			remaining -= packet_size;
		}

		ppu.check_state();
	}
	else if (mode & NOT_BLOCKING_OP)
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
	else // NOT_BLOCKING_BIG_OP
	{
		if (!lock.try_lock())
		{
			return CELL_EBUSY;
		}

		u32 sent_size = 0;
		u32 remaining = static_cast<u32>(size);

		while (sent_size < size)
		{
			const u32 packet_size = std::min(remaining, ITER_SIZE);
			const u32 nsent = vuart_thread.enque_tx_data(data.get() + sent_size, packet_size);
			remaining -= nsent;

			if (nsent < packet_size)
			{
				// Based on RE
				if (sent_size == 0)
				{
					return not_an_error(packet_size); // First iteration
				}
				else if (sent_size + nsent < size)
				{
					return not_an_error(sent_size + nsent);
				}
				else
				{
					break; // Last iteration
				}
			}

			sent_size += nsent;
		}
	}

	return not_an_error(size);
}

error_code sys_uart_get_params(vm::ptr<vuart_params> buffer)
{
	sys_uart.trace("sys_uart_get_params(buffer=0x%x)", buffer);

	if (!g_ps3_process_info.has_root_perm())
	{
		return CELL_ENOSYS;
	}

	auto &vuart_thread = g_fxo->get<vuart_av>();

	if (!vuart_thread.initialized)
	{
		return CELL_ESRCH;
	}

	if (!vm::check_addr(buffer.addr(), vm::page_writable, sizeof(vuart_params)))
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
		if (Emu.IsPausedOrReady())
		{
			thread_ctrl::wait_for(5000);
			continue;
		}

		const u64 hdmi_event_dist[2] = { hdmi_event_handler[0].time_until_next(), hdmi_event_handler[1].time_until_next() };
		bool update_dist = false;

		if (hdmi_event_dist[0] == 0)
		{
			dispatch_hdmi_event(hdmi_event_handler[0].get_occured_event(), UartAudioAvport::HDMI_0);
			update_dist |= hdmi_event_handler[0].events_available();
		}

		if (hdmi_event_dist[1] == 0)
		{
			dispatch_hdmi_event(hdmi_event_handler[1].get_occured_event(), UartAudioAvport::HDMI_1);
			update_dist |= hdmi_event_handler[1].events_available();
		}

		if (update_dist)
		{
			continue;
		}

		const u64 wait_time = [&]()
		{
			if (hdmi_event_dist[0] != 0 && hdmi_event_dist[1] != 0)
				return std::min(hdmi_event_dist[0], hdmi_event_dist[1]);
			else
				return std::max(hdmi_event_dist[0], hdmi_event_dist[1]);
		}();

		std::unique_lock<shared_mutex> lock(tx_wake_m);
		if (!tx_buf.get_used_size())
		{
			tx_wake_c.wait_unlock(wait_time ? wait_time : -1, lock);
		}
		else
		{
			lock.unlock();
		}

		if (u32 read_size = read_tx_data(temp_tx_buf, PS3AV_TX_BUF_SIZE))
		{
			parse_tx_buffer(read_size);

			// Give vsh some time
			thread_ctrl::wait_for(1000 * 100 / g_cfg.core.clocks_scale);

			commit_rx_buf(false);
			commit_rx_buf(true);
		}
	}
}

void vuart_av_thread::parse_tx_buffer(u32 buf_size)
{
	if (buf_size >= PS3AV_TX_BUF_SIZE)
	{
		while (read_tx_data(temp_tx_buf, PS3AV_TX_BUF_SIZE) >= PS3AV_TX_BUF_SIZE);
		write_resp(reinterpret_cast<be_t<u16, 1>*>(temp_tx_buf)[3], PS3AV_STATUS_BUFFER_OVERFLOW);
		return;
	}

	u32 read_ptr = 0;

	while (buf_size)
	{
		const ps3av_header* const hdr = reinterpret_cast<ps3av_header*>(&temp_tx_buf[read_ptr]);
		const u16 pkt_size            = hdr->length + 4;

		if (hdr->length == 0xFFFCU)
		{
			write_resp(0xDEAD, PS3AV_STATUS_FAILURE);
			return;
		}

		if (hdr->version != PS3AV_VERSION)
		{
			if (hdr->version >= 0x100 && hdr->version < PS3AV_VERSION)
			{
				sys_uart.todo("Unimplemented AV version: 0x%04x", hdr->version);
			}

			write_resp(static_cast<u16>(hdr->cid.get()), PS3AV_STATUS_INVALID_COMMAND);
			return;
		}

		const void* const pkt_storage = &temp_tx_buf[read_ptr];
		read_ptr += pkt_size;
		buf_size = buf_size < pkt_size ? 0 : buf_size - pkt_size;

		auto cmd = get_cmd(hdr->cid);

		if (!cmd.get())
		{
			sys_uart.error("Unknown AV cmd: 0x%08x", hdr->cid);
			continue;
		}

		const auto cmd_size = cmd->get_size(*this, pkt_storage);

		if (cmd_size != pkt_size && cmd_size)
		{
			sys_uart.error("Invalid size for cid=0x%x, expected=0x%x, got=0x%x", static_cast<const ps3av_header *>(pkt_storage)->cid, cmd_size, pkt_size);
			write_resp(static_cast<u16>(hdr->cid.get()), PS3AV_STATUS_INVALID_SAMPLE_SIZE);
			return;
		}

		cmd->execute(*this, pkt_storage);
	}
}

vuart_av_thread &vuart_av_thread::operator=(thread_state)
{
	{
		std::lock_guard lock(tx_wake_m);
	}
	tx_wake_c.notify_all();
	return *this;
}

u32 vuart_av_thread::enque_tx_data(const void *data, u32 data_sz)
{
	std::unique_lock<shared_mutex> lock(tx_wake_m);
	if (u32 size = static_cast<u32>(tx_buf.push(data, data_sz, true)))
	{
		lock.unlock();
		tx_wake_c.notify_all();
		return size;
	}

	return 0;
}

u32 vuart_av_thread::get_tx_bytes()
{
	return static_cast<u32>(tx_buf.get_used_size());
}

u32 vuart_av_thread::read_rx_data(void *data, u32 data_sz)
{
	return static_cast<u32>(rx_buf.pop(data, data_sz, true));
}

u32 vuart_av_thread::read_tx_data(void *data, u32 data_sz)
{
	std::unique_lock<shared_mutex> lock(tx_rdy_m);
	if (u32 size = static_cast<u32>(tx_buf.pop(data, data_sz, true)))
	{
		lock.unlock();
		tx_rdy_c.notify_all();
		return size;
	}
	return 0;
}

u32 vuart_av_thread::get_reply_buf_free_size()
{
	return sizeof(temp_rx_buf.buf) - temp_rx_buf.crnt_size;
}

template<bool UseScBuffer>
void vuart_av_thread::write_resp(u32 cid, u32 status, const void *data, u16 data_size)
{
	const ps3av_pkt_reply_hdr pkt_hdr =
	{
		PS3AV_VERSION,
		data_size + 8U,
		cid | PS3AV_REPLY_BIT,
		status
	};

	if (status != PS3AV_STATUS_SUCCESS)
	{
		sys_uart.error("Packet failed cid=0x%08x status=0x%02x", cid, status);
	}

	temp_buf &buf = UseScBuffer ? temp_rx_sc_buf : temp_rx_buf;
	const u32 total_size = sizeof(pkt_hdr) + data_size;

	if (buf.crnt_size + total_size <= sizeof(buf.buf))
	{
		memcpy(&buf.buf[buf.crnt_size], &pkt_hdr, sizeof(pkt_hdr));
		memcpy(&buf.buf[buf.crnt_size + sizeof(pkt_hdr)], data, data_size);
		buf.crnt_size += total_size;
	}
}

std::shared_ptr<ps3av_cmd> vuart_av_thread::get_cmd(u32 cid)
{
	switch (cid)
	{
	case PS3AV_CID_AV_CEC_MESSAGE:
	case PS3AV_CID_AV_UNK11:
	case PS3AV_CID_AV_UNK12:
		return std::make_shared<generic_reply_cmd>();

	// AV commands
	case PS3AV_CID_AV_INIT:               return std::make_shared<av_init_cmd>();
	case PS3AV_CID_AV_FIN:                return std::make_shared<av_fini_cmd>();
	case PS3AV_CID_AV_GET_HW_CONF:        return std::make_shared<av_get_hw_conf_cmd>();
	case PS3AV_CID_AV_GET_MONITOR_INFO:   return std::make_shared<av_get_monitor_info_cmd>();
	case PS3AV_CID_AV_GET_BKSV_LIST:      return std::make_shared<av_get_bksv_list_cmd>();
	case PS3AV_CID_AV_ENABLE_EVENT:       return std::make_shared<av_enable_event_cmd>();
	case PS3AV_CID_AV_DISABLE_EVENT:      return std::make_shared<av_disable_event_cmd>();
	case PS3AV_CID_AV_TV_MUTE:            return std::make_shared<av_tv_mute_cmd>();
	case PS3AV_CID_AV_NULL_CMD:           return std::make_shared<av_null_cmd>();
	case PS3AV_CID_AV_GET_AKSV:           return std::make_shared<av_get_aksv_list_cmd>();
	case PS3AV_CID_AV_VIDEO_DISABLE_SIG:  return std::make_shared<video_disable_signal_cmd>();
	case PS3AV_CID_AV_VIDEO_YTRAPCONTROL: return std::make_shared<av_video_ytrapcontrol_cmd>();
	case PS3AV_CID_AV_AUDIO_MUTE:         return std::make_shared<av_audio_mute_cmd>();
	case PS3AV_CID_AV_ACP_CTRL:           return std::make_shared<av_acp_ctrl_cmd>();
	case PS3AV_CID_AV_SET_ACP_PACKET:     return std::make_shared<av_set_acp_packet_cmd>();
	case PS3AV_CID_AV_ADD_SIGNAL_CTL:     return std::make_shared<av_add_signal_ctl_cmd>();
	case PS3AV_CID_AV_SET_CGMS_WSS:       return std::make_shared<av_set_cgms_wss_cmd>();
	case PS3AV_CID_AV_HDMI_MODE:          return std::make_shared<av_set_hdmi_mode_cmd>();
	case PS3AV_CID_AV_GET_CEC_CONFIG:     return std::make_shared<av_get_cec_status_cmd>();

	// Video commands
	case PS3AV_CID_VIDEO_INIT:            return std::make_shared<video_init_cmd>();
	case PS3AV_CID_VIDEO_ROUTE:           return std::make_shared<video_set_route_cmd>();
	case PS3AV_CID_VIDEO_FORMAT:          return std::make_shared<video_set_format_cmd>();
	case PS3AV_CID_VIDEO_PITCH:           return std::make_shared<video_set_pitch_cmd>();
	case PS3AV_CID_VIDEO_GET_HW_CONF:     return std::make_shared<video_get_hw_cfg_cmd>();

	// Audio commands
	case PS3AV_CID_AUDIO_INIT:            return std::make_shared<audio_init_cmd>();
	case PS3AV_CID_AUDIO_MODE:            return std::make_shared<audio_set_mode_cmd>();
	case PS3AV_CID_AUDIO_MUTE:            return std::make_shared<audio_mute_cmd>();
	case PS3AV_CID_AUDIO_ACTIVE:          return std::make_shared<audio_set_active_cmd>();
	case PS3AV_CID_AUDIO_INACTIVE:        return std::make_shared<audio_set_inactive_cmd>();
	case PS3AV_CID_AUDIO_SPDIF_BIT:       return std::make_shared<audio_spdif_bit_cmd>();
	case PS3AV_CID_AUDIO_CTRL:            return std::make_shared<audio_ctrl_cmd>();

	// Multipacket
	case PS3AV_CID_AVB_PARAM:             return std::make_shared<inc_avset_cmd>();

	default: return {};
	}
}

void vuart_av_thread::commit_rx_buf(bool syscon_buf)
{
	temp_buf &buf = syscon_buf ? temp_rx_sc_buf : temp_rx_buf;

	std::unique_lock<shared_mutex> lock(rx_wake_m);
	rx_buf.push(buf.buf, buf.crnt_size, true);
	buf.crnt_size = 0;

	if (rx_buf.get_used_size())
	{
		lock.unlock();
		rx_wake_c.notify_all();
	}
}

void vuart_av_thread::add_hdmi_events(UartHdmiEvent first_event, UartHdmiEvent last_event, bool hdmi_0, bool hdmi_1)
{
	if (hdmi_0)
	{
		hdmi_event_handler[0].set_target_state(first_event, last_event);
	}

	if (hdmi_1 && g_cfg.core.debug_console_mode)
	{
		hdmi_event_handler[1].set_target_state(first_event, last_event);
	}
}

void vuart_av_thread::add_hdmi_events(UartHdmiEvent last_event, bool hdmi_0, bool hdmi_1)
{
	add_hdmi_events(last_event, last_event, hdmi_0, hdmi_1);
}

void vuart_av_thread::dispatch_hdmi_event(UartHdmiEvent event, UartAudioAvport hdmi)
{
	const bool hdmi_0 = hdmi == UartAudioAvport::HDMI_0;
	const bool hdmi_1 = hdmi == UartAudioAvport::HDMI_1;

	switch (event)
	{
	case UartHdmiEvent::UNPLUGGED:
	{
		add_unplug_event(hdmi_0, hdmi_1);
		break;
	}
	case UartHdmiEvent::PLUGGED:
	{
		add_plug_event(hdmi_0, hdmi_1);
		break;
	}
	case UartHdmiEvent::HDCP_DONE:
	{
		add_hdcp_done_event(hdmi_0, hdmi_1);
		break;
	}
	default: break;
	}
}

RsxaudioAvportIdx vuart_av_thread::avport_to_idx(UartAudioAvport avport)
{
	switch (avport)
	{
	case UartAudioAvport::HDMI_0:
		return RsxaudioAvportIdx::HDMI_0;
	case UartAudioAvport::HDMI_1:
		return RsxaudioAvportIdx::HDMI_1;
	case UartAudioAvport::AVMULTI_0:
		return RsxaudioAvportIdx::AVMULTI;
	case UartAudioAvport::SPDIF_0:
		return RsxaudioAvportIdx::SPDIF_0;
	case UartAudioAvport::SPDIF_1:
		return RsxaudioAvportIdx::SPDIF_1;
	default:
		ensure(false);
		return RsxaudioAvportIdx::HDMI_0;
	}
}

void vuart_av_thread::add_unplug_event(bool hdmi_0, bool hdmi_1)
{
	if ((hdmi_events_bitmask & PS3AV_EVENT_BIT_UNPLUGGED) == 0) return;

	ps3av_header pkt{};
	pkt.version = av_cmd_ver;
	pkt.length = sizeof(ps3av_header) - 4;
	pkt.cid = PS3AV_CID_EVENT_UNPLUGGED;

	if (hdmi_0)
	{
		g_fxo->get<rsx_audio_data>().update_av_mute_state(RsxaudioAvportIdx::HDMI_0, false, true);
		hdcp_first_auth[0] = true;
		commit_event_data(&pkt, sizeof(pkt));
	}

	if (hdmi_1)
	{
		g_fxo->get<rsx_audio_data>().update_av_mute_state(RsxaudioAvportIdx::HDMI_1, false, true);
		hdcp_first_auth[1] = true;
		pkt.cid |= 0x10000;
		commit_event_data(&pkt, sizeof(pkt));
	}
}

void vuart_av_thread::add_plug_event(bool hdmi_0, bool hdmi_1)
{
	if ((hdmi_events_bitmask & PS3AV_EVENT_BIT_PLUGGED) == 0) return;

	ps3av_pkt_hdmi_plugged_event pkt{};
	pkt.hdr.version = av_cmd_ver;
	pkt.hdr.length = sizeof(ps3av_pkt_hdmi_plugged_event) - 8;
	pkt.hdr.cid = PS3AV_CID_EVENT_PLUGGED;

	if (hdmi_0)
	{
		g_fxo->get<rsx_audio_data>().update_av_mute_state(RsxaudioAvportIdx::HDMI_0, false, true);
		av_get_monitor_info_cmd::set_hdmi_display_cfg(*this, pkt.minfo, static_cast<u8>(UartAudioAvport::HDMI_0));
		commit_event_data(&pkt, sizeof(pkt) - 4);
	}

	if (hdmi_1)
	{
		g_fxo->get<rsx_audio_data>().update_av_mute_state(RsxaudioAvportIdx::HDMI_1, false, true);
		memset(&pkt.minfo, 0, sizeof(pkt.minfo));
		pkt.hdr.cid |= 0x10000;
		av_get_monitor_info_cmd::set_hdmi_display_cfg(*this, pkt.minfo, static_cast<u8>(UartAudioAvport::HDMI_1));
		commit_event_data(&pkt, sizeof(pkt) - 4);
	}
}

void vuart_av_thread::add_hdcp_done_event(bool hdmi_0, bool hdmi_1)
{
	u16 pkt_size = offsetof(ps3av_pkt_hdmi_hdcp_done_event, ksv_arr);
	ps3av_pkt_hdmi_hdcp_done_event pkt{};
	pkt.hdr.version = av_cmd_ver;

	if (hdmi_behavior_mode == PS3AV_HDMI_BEHAVIOR_NORMAL || !(hdmi_behavior_mode & PS3AV_HDMI_BEHAVIOR_HDCP_OFF))
	{
		pkt.ksv_cnt = 1;
		memcpy(&pkt.ksv_arr[0], PS3AV_BKSV_VALUE, sizeof(PS3AV_BKSV_VALUE));
		pkt_size = (pkt_size + 5 * pkt.ksv_cnt + 3) & 0xFFFC;
	}

	pkt.hdr.length = pkt_size - 4;

	if (hdmi_0)
	{
		g_fxo->get<rsx_audio_data>().update_av_mute_state(RsxaudioAvportIdx::HDMI_0, false, true, false);

		if (hdcp_first_auth[0])
		{
			if (hdmi_events_bitmask & PS3AV_EVENT_BIT_HDCP_DONE)
			{
				hdcp_first_auth[0] = false;
				pkt.hdr.cid = PS3AV_CID_EVENT_HDCP_DONE;
				commit_event_data(&pkt, pkt_size);
			}
		}
		else if (hdmi_events_bitmask & PS3AV_EVENT_BIT_HDCP_REAUTH)
		{
			pkt.hdr.cid = PS3AV_CID_EVENT_HDCP_REAUTH;
			commit_event_data(&pkt, pkt_size);
		}
	}

	if (hdmi_1)
	{
		g_fxo->get<rsx_audio_data>().update_av_mute_state(RsxaudioAvportIdx::HDMI_1, false, true, false);

		if (hdcp_first_auth[1])
		{
			if (hdmi_events_bitmask & PS3AV_EVENT_BIT_HDCP_DONE)
			{
				hdcp_first_auth[1] = false;
				pkt.hdr.cid = PS3AV_CID_EVENT_HDCP_DONE | 0x10000;
				commit_event_data(&pkt, pkt_size);
			}
		}
		else if (hdmi_events_bitmask & PS3AV_EVENT_BIT_HDCP_REAUTH)
		{
			pkt.hdr.cid = PS3AV_CID_EVENT_HDCP_REAUTH | 0x10000;
			commit_event_data(&pkt, pkt_size);
		}
	}
}

void vuart_av_thread::commit_event_data(const void *data, u16 data_size)
{
	std::unique_lock<shared_mutex> lock(rx_wake_m);
	rx_buf.push(data, data_size, true);

	if (rx_buf.get_used_size())
	{
		lock.unlock();
		rx_wake_c.notify_all();
	}
}

vuart_hdmi_event_handler::vuart_hdmi_event_handler(u64 time_offset) : time_offset(time_offset)
{
}

void vuart_hdmi_event_handler::set_target_state(UartHdmiEvent start_state, UartHdmiEvent end_state)
{
	ensure(start_state != UartHdmiEvent::NONE && static_cast<u8>(start_state) <= static_cast<u8>(end_state));

	base_state = static_cast<UartHdmiEvent>(std::min<u8>(static_cast<u8>(start_state) - 1, static_cast<u8>(current_to_state)));
	target_state = end_state;

	if (!events_available())
	{
		advance_state();
	}
}

bool vuart_hdmi_event_handler::events_available()
{
	return time_of_next_event != 0;
}

u64 vuart_hdmi_event_handler::time_until_next()
{
	const u64 current_time = get_system_time();

	if (!events_available() || current_time + EVENT_TIME_THRESHOLD >= time_of_next_event)
	{
		return 0;
	}

	return time_of_next_event - current_time;
}

UartHdmiEvent vuart_hdmi_event_handler::get_occured_event()
{
	if (events_available() && time_until_next() == 0)
	{
		const UartHdmiEvent occured = current_to_state;
		advance_state();

		return occured;
	}

	return UartHdmiEvent::NONE;
}

void vuart_hdmi_event_handler::schedule_next()
{
	time_of_next_event = get_system_time() + (EVENT_TIME_DURATION + time_offset) * 100 / g_cfg.core.clocks_scale;
}

void vuart_hdmi_event_handler::advance_state()
{
	current_state = current_to_state;

	while (base_state != target_state)
	{
		base_state = static_cast<UartHdmiEvent>(static_cast<u8>(base_state) + 1);

		if (current_state == UartHdmiEvent::UNPLUGGED && base_state == UartHdmiEvent::UNPLUGGED)
		{
			continue;
		}

		if (current_state == UartHdmiEvent::PLUGGED && base_state == UartHdmiEvent::PLUGGED)
		{
			continue;
		}

		if (current_state == UartHdmiEvent::HDCP_DONE && base_state == UartHdmiEvent::PLUGGED)
		{
			continue;
		}

		current_to_state = base_state;
		schedule_next();
		return;
	}

	time_of_next_event = 0;
}
