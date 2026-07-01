#include "stdafx.h"
#include "Crypto/utils.h"
#include "Emu/Cell/lv2/sys_fs.h"
#include "Emu/Cell/lv2/sys_lwmutex.h"
#include "Emu/Cell/lv2/sys_memory.h"
#include "Emu/Cell/lv2/sys_process.h"
#include "Emu/Cell/lv2/sys_prx.h"
#include "Emu/Cell/lv2/sys_ss.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/savestate_utils.hpp"
#include "sysPrxForUser.h"
#include "util/bit_set.hpp"

#include "cellSysmodule.h"
#include <ranges>

LOG_CHANNEL(cellSysmodule);

template<>
void fmt_class_string<CellSysmoduleError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_SYSMODULE_ERROR_DUPLICATED);
			STR_CASE(CELL_SYSMODULE_ERROR_UNKNOWN);
			STR_CASE(CELL_SYSMODULE_ERROR_UNLOADED);
			STR_CASE(CELL_SYSMODULE_ERROR_INVALID_MEMCONTAINER);
			STR_CASE(CELL_SYSMODULE_ERROR_FATAL);
		}

		return unknown;
	});
}

namespace
{
	constexpr u16 INTERNAL_MODULE_ID_BASE = 0xf000;
	constexpr u16 INTERNAL_MODULE_ID_MASK = 0x0fff;
	constexpr u16 UNK_MODULE_ID_BASE = 0xff00;
	constexpr u8 UNK_MODULE_ID_MASK = 0x00ff;

	constexpr std::array<u16, 1> DEPENDENCIES_NET     { CELL_SYSMODULE_NETCTL };
	constexpr std::array<u16, 2> DEPENDENCIES_HTTP    { CELL_SYSMODULE_NET, CELL_SYSMODULE_RTC };
	constexpr std::array<u16, 1> DEPENDENCIES_HTTPUTIL{ CELL_SYSMODULE_HTTP };
	constexpr std::array<u16, 2> DEPENDENCIES_HTTPS   { CELL_SYSMODULE_SSL, CELL_SYSMODULE_HTTP };
	constexpr std::array<u16, 3> DEPENDENCIES_SSL     { CELL_SYSMODULE_NET, CELL_SYSMODULE_RTC, CELL_SYSMODULE_FS };
	constexpr std::array<u16, 1> DEPENDENCIES_NP2     { CELL_SYSMODULE_SYSUTIL_NP };
	constexpr std::array<u16, 2> DEPENDENCIES_CLANS   { CELL_SYSMODULE_SYSUTIL_NP, CELL_SYSMODULE_HTTPS };
	constexpr std::array<u16, 1> DEPENDENCIES_FS      { CELL_SYSMODULE_FS };
	constexpr std::array<u16, 2> DEPENDENCIES_AT3P    { CELL_SYSMODULE_ADEC_AT3, CELL_SYSMODULE_ADEC_ATX };
	constexpr std::array<u16, 1> DEPENDENCIES_AT3M    { 0xf053 };
	constexpr std::array<u16, 1> DEPENDENCIES_SPURS   { CELL_SYSMODULE_SPURS };
	constexpr std::array<u16, 1> DEPENDENCIES_VDEC_AL { CELL_SYSMODULE_VDEC_AL };
	constexpr std::array<u16, 1> DEPENDENCIES_ADEC_AL { CELL_SYSMODULE_ADEC_AL };
	constexpr std::array<u16, 1> DEPENDENCIES_ADEC2   { 0xf03f };
	constexpr std::array<u16, 1> DEPENDENCIES_DMUX_AL { CELL_SYSMODULE_DMUX_AL };
	constexpr std::array<u16, 3> DEPENDENCIES_VDEC    { CELL_SYSMODULE_VDEC_MPEG2, CELL_SYSMODULE_VDEC_AVC, CELL_SYSMODULE_VDEC_AL };
	constexpr std::array<u16, 5> DEPENDENCIES_ADEC    { CELL_SYSMODULE_ADEC_LPCM, CELL_SYSMODULE_ADEC_AC3, CELL_SYSMODULE_ADEC_AT3, CELL_SYSMODULE_ADEC_ATX, CELL_SYSMODULE_ADEC_AL };
	constexpr std::array<u16, 2> DEPENDENCIES_DMUX    { CELL_SYSMODULE_DMUX_PAMF, CELL_SYSMODULE_DMUX_AL };
	constexpr std::array<u16, 1> DEPENDENCIES_PSPCM   { CELL_SYSMODULE_USBD };
	constexpr std::array<u16, 2> DEPENDENCIES_COMM    { CELL_SYSMODULE_SYSUTIL_NP2, CELL_SYSMODULE_HTTPS };
	constexpr std::array<u16, 1> DEPENDENCIES_NP_TUS  { CELL_SYSMODULE_SYSUTIL_NP2 };
	constexpr std::array<u16, 3> DEPENDENCIES_AD_CORE { CELL_SYSMODULE_FS, CELL_SYSMODULE_NET, CELL_SYSMODULE_SYSUTIL_NP };
	constexpr std::array<u16, 1> DEPENDENCIES_AD_ASYNC{ 0x4b };
	constexpr std::array<u16, 1> DEPENDENCIES_SPURS_JQ{ CELL_SYSMODULE_FIBER };
	constexpr std::array<u16, 2> DEPENDENCIES_MEDI    { CELL_SYSMODULE_NET, CELL_SYSMODULE_SYSUTIL_NP };

	struct module_info
	{
		std::string_view path;
		std::span<const u16> dependencies;
	};

	constexpr std::array<module_info, 0xbd> MODULE_INFOS
	{
		// 93 external modules
		module_info
		{ .path = "external/libnet.sprx",                           .dependencies = DEPENDENCIES_NET },      // CELL_SYSUTIL_NET
		{ .path = "external/libhttp.sprx",                          .dependencies = DEPENDENCIES_HTTP },     // CELL_SYSMODULE_HTTP
		{ .path = {},                                               .dependencies = DEPENDENCIES_HTTPUTIL }, // CELL_SYSMODULE_HTTP_UTIL
		{ .path = "external/libssl.sprx",                           .dependencies = DEPENDENCIES_SSL },      // CELL_SYSMODULE_SSL
		{ .path = {},                                               .dependencies = DEPENDENCIES_HTTPS },    // CELL_SYSMODULE_HTTPS
		{ .path = {},                                               .dependencies = DEPENDENCIES_VDEC },     // CELL_SYSMODULE_VDEC
		{ .path = {},                                               .dependencies = DEPENDENCIES_ADEC },     // CELL_SYSMODULE_ADEC
		{ .path = {},                                               .dependencies = DEPENDENCIES_DMUX },     // CELL_SYSMODULE_DMUX
		{ .path = "external/libvpost.sprx",                         .dependencies = {} },                    // CELL_SYSMODULE_VPOST
		{ .path = "external/librtc.sprx",                           .dependencies = {} },                    // CELL_SYSMODULE_RTC
		{ .path = "external/libsre.sprx",                           .dependencies = {} },                    // CELL_SYSMODULE_SPURS
		{ .path = {},                                               .dependencies = DEPENDENCIES_SPURS },    // CELL_SYSMODULE_OVIS
		{ .path = {},                                               .dependencies = DEPENDENCIES_SPURS },    // CELL_SYSMODULE_SHEAP
		{ .path = {},                                               .dependencies = DEPENDENCIES_SPURS },    // CELL_SYSMODULE_SYNC
		{ .path = "external/libfs.sprx",                            .dependencies = {} },                    // CELL_SYSMODULE_FS
		{ .path = "external/libjpgdec.sprx",                        .dependencies = DEPENDENCIES_FS },       // CELL_SYSMODULE_JPGDEC
		{ .path = "external/libgcm_sys.sprx",                       .dependencies = {} },                    // CELL_SYSMODULE_GCM_SYS
		{ .path = "external/libaudio.sprx",                         .dependencies = {} },                    // CELL_SYSMODULE_AUDIO
		{ .path = "external/libpamf.sprx",                          .dependencies = {} },                    // CELL_SYSMODULE_PAMF
		{ .path = "external/libatrac3plus.sprx",                    .dependencies = DEPENDENCIES_AT3P },     // CELL_SYSMODULE_ATRAC3PLUS
		{ .path = "external/libnetctl.sprx",                        .dependencies = {} },                    // CELL_SYSMODULE_NETCTL
		{ .path = "external/libsysutil.sprx",                       .dependencies = {} },                    // CELL_SYSMODULE_SYSUTIL
		{ .path = "external/libsysutil_np.sprx",                    .dependencies = {} },                    // CELL_SYSMODULE_SYSUTIL_NP
		{ .path = "external/libio.sprx",                            .dependencies = {} },                    // CELL_SYSMODULE_IO
		{ .path = "external/libpngdec.sprx",                        .dependencies = DEPENDENCIES_FS },       // CELL_SYSMODULE_PNGDEC
		{ .path = "external/libfont.sprx",                          .dependencies = DEPENDENCIES_FS },       // CELL_SYSMODULE_FONT
		{ .path = "external/libfontFT.sprx",                        .dependencies = {} },                    // CELL_SYSMODULE_FONTFT
		{ .path = "external/libfreetype.sprx",                      .dependencies = DEPENDENCIES_FS },       // CELL_SYSMODULE_FREETYPE
		{ .path = "external/libusbd.sprx",                          .dependencies = {} },                    // CELL_SYSMODULE_USBD
		{ .path = "external/libsail.sprx",                          .dependencies = {} },                    // CELL_SYSMODULE_SAIL
		{ .path = "external/libl10n.sprx",                          .dependencies = {} },                    // CELL_SYSMODULE_L10N
		{ .path = "external/libresc.sprx",                          .dependencies = {} },                    // CELL_SYSMODULE_RESC
		{ .path = {},                                               .dependencies = DEPENDENCIES_SPURS },    // CELL_SYSMODULE_DAISY
		{ .path = "external/libkey2char.sprx",                      .dependencies = {} },                    // CELL_SYSMODULE_KEY2CHAR
		{ .path = "external/libmic.sprx",                           .dependencies = {} },                    // CELL_SYSMODULE_MIC
		{ .path = "external/libcamera.sprx",                        .dependencies = {} },                    // CELL_SYSMODULE_CAMERA
		{ .path = "external/libsmvd2.sprx",                         .dependencies = DEPENDENCIES_VDEC_AL },  // CELL_SYSMODULE_VDEC_MPEG2
		{ .path = "external/libavcdec.sprx",                        .dependencies = DEPENDENCIES_VDEC_AL },  // CELL_SYSMODULE_VDEC_AVC
		{ .path = {},                                               .dependencies = DEPENDENCIES_ADEC_AL },  // CELL_SYSMODULE_ADEC_LPCM
		{ .path = "external/libac3dec.sprx",                        .dependencies = DEPENDENCIES_ADEC_AL },  // CELL_SYSMODULE_ADEC_AC3
		{ .path = "external/libatxdec.sprx",                        .dependencies = DEPENDENCIES_ADEC_AL },  // CELL_SYSMODULE_ADEC_ATX
		{ .path = "external/libat3dec.sprx",                        .dependencies = DEPENDENCIES_ADEC_AL },  // CELL_SYSMODULE_ADEC_AT3
		{ .path = "external/libdmuxpamf.sprx",                      .dependencies = DEPENDENCIES_DMUX_AL },  // CELL_SYSMODULE_DMUX_PAMF
		{ .path = "external/libvdec.sprx",                          .dependencies = {} },                    // CELL_SYSMODULE_VDEC_AL
		{ .path = "external/libadec.sprx",                          .dependencies = {} },                    // CELL_SYSMODULE_ADEC_AL
		{ .path = "external/libdmux.sprx",                          .dependencies = {} },                    // CELL_SYSMODULE_DMUX_AL
		{ .path = "external/liblv2dbg_for_dex.sprx",                .dependencies = {} },                    // CELL_SYSMODULE_LV2DBG
		{ .path = "external/libsysutil_avc_ext.sprx",               .dependencies = {} },                    // 0x2f
		{ .path = "external/libusbpspcm.sprx",                      .dependencies = DEPENDENCIES_PSPCM },    // CELL_SYSMODULE_USBPSPCM
		{ .path = "external/libsysutil_avconf_ext.sprx",            .dependencies = {} },                    // CELL_SYSMODULE_AVCONF_EXT
		{ .path = "external/libsysutil_userinfo.sprx",              .dependencies = {} },                    // CELL_SYSMODULE_SYSUTIL_USERINFO
		{ .path = "external/libsysutil_savedata.sprx",              .dependencies = {} },                    // CELL_SYSMODULE_SYSUTIL_SAVEDATA
		{ .path = "external/libsysutil_subdisplay.sprx",            .dependencies = {} },                    // CELL_SYSMODULE_SUBDISPLAY
		{ .path = "external/libsysutil_rec.sprx",                   .dependencies = {} },                    // CELL_SYSMODULE_SYSUTIL_REC
		{ .path = "external/libsysutil_video_export.sprx",          .dependencies = {} },                    // CELL_SYSMODULE_VIDEO_EXPORT
		{ .path = "external/libsysutil_game_exec.sprx",             .dependencies = {} },                    // CELL_SYSMODULE_SYSUTIL_GAME_EXEC
		{ .path = "external/libsysutil_np2.sprx",                   .dependencies = DEPENDENCIES_NP2 },      // CELL_SYSMODULE_SYSUTIL_NP2
		{ .path = "external/libsysutil_ap.sprx",                    .dependencies = {} },                    // CELL_SYSMODULE_SYSUTIL_AP
		{ .path = "external/libsysutil_np_clans.sprx",              .dependencies = DEPENDENCIES_CLANS },    // CELL_SYSMODULE_SYSUTIL_NP_CLANS
		{ .path = "external/libsysutil_oskdialog_ext.sprx",         .dependencies = {} },                    // CELL_SYSMODULE_SYSUTIL_OSK_EXT
		{ .path = "external/libdivxdec.sprx",                       .dependencies = DEPENDENCIES_VDEC_AL },  // CELL_SYSMODULE_VDEC_DIVX
		{ .path = "external/libjpgenc.sprx",                        .dependencies = DEPENDENCIES_FS },       // CELL_SYSMODULE_JPGENC
		{ .path = "external/libsysutil_game.sprx",                  .dependencies = {} },                    // CELL_SYSMODULE_SYSUTIL_GAME
		{ .path = "external/libsysutil_bgdl.sprx",                  .dependencies = {} },                    // CELL_SYSMODULE_BGDL
		{ .path = "external/libfreetypeTT.sprx",                    .dependencies = DEPENDENCIES_FS },       // CELL_SYSMODULE_FREETYPE_TT
		{ .path = "external/libsysutil_video_upload.sprx",          .dependencies = {} },                    // CELL_SYSMODULE_SYSUTIL_VIDEO_UPLOAD
		{ .path = "external/libsysutil_sysconf_ext.sprx",           .dependencies = {} },                    // CELL_SYSMODULE_SYSUTIL_SYSCONF_EXT
		{ .path = "external/libfiber.sprx",                         .dependencies = {} },                    // CELL_SYSMODULE_FIBER
		{ .path = "external/libsysutil_np_commerce2.sprx",          .dependencies = DEPENDENCIES_COMM },     // CELL_SYSMODULE_SYSUTIL_NP_COMMERCE2
		{ .path = "external/libsysutil_np_tus.sprx",                .dependencies = DEPENDENCIES_NP_TUS },   // CELL_SYSMODULE_SYSUTIL_NP_TUS
		{ .path = "external/libvoice.sprx",                         .dependencies = {} },                    // CELL_SYSMODULE_VOICE
		{ .path = "external/libcelp8dec.sprx",                      .dependencies = DEPENDENCIES_ADEC_AL },  // CELL_SYSMODULE_ADEC_CELP8
		{ .path = "external/libcelp8enc.sprx",                      .dependencies = {} },                    // CELL_SYSMODULE_CELP8ENC
		{ .path = "external/libsysutil_misc.sprx",                  .dependencies = {} },                    // CELL_SYSMODULE_SYSUTIL_LICENSEAREA
		{ .path = "external/libsysutil_music.sprx",                 .dependencies = {} },                    // CELL_SYSMODULE_SYSUTIL_MUSIC2
		{ .path = "external/libad_core.sprx",                       .dependencies = DEPENDENCIES_AD_CORE },  // 0x4b
		{ .path = "external/libad_async.sprx",                      .dependencies = DEPENDENCIES_AD_ASYNC }, // 0x4c
		{ .path = "external/libad_billboard_util.sprx",             .dependencies = DEPENDENCIES_AD_CORE },  // 0x4d
		{ .path = "external/libsysutil_screenshot.sprx",            .dependencies = {} },                    // CELL_SYSMODULE_SYSUTIL_SCREENSHOT
		{ .path = "external/libsysutil_music_decode.sprx",          .dependencies = {} },                    // CELL_SYSMODULE_SYSUTIL_MUSIC_DECODE
		{ .path = "external/libspurs_jq.sprx",                      .dependencies = DEPENDENCIES_SPURS_JQ }, // CELL_SYSMODULE_SPURS_JQ
		{ .path = "external/libsysutil_authdialog.sprx",            .dependencies = {} },                    // 0x51
		{ .path = "external/libpngenc.sprx",                        .dependencies = DEPENDENCIES_FS },       // CELL_SYSMODULE_PNGENC
		{ .path = "external/libsysutil_music_decode.sprx",          .dependencies = {} },                    // CELL_SYSMODULE_SYSUTIL_MUSIC_DECODE2
		{ .path = "external/libmedi.sprx",                          .dependencies = DEPENDENCIES_MEDI },     // 0x54
		{ .path = "external/libsync2.sprx",                         .dependencies = DEPENDENCIES_SPURS_JQ }, // CELL_SYSMODULE_SYNC2
		{ .path = "external/libsysutil_np_util.sprx",               .dependencies = {} },                    // CELL_SYSMODULE_SYSUTIL_NP_UTIL
		{ .path = "external/librudp.sprx",                          .dependencies = {} },                    // CELL_SYSMODULE_RUDP
		{ .path = "external/libsysutil_syschat.sprx",               .dependencies = {} },                    // 0x58
		{ .path = "external/libsysutil_np_sns.sprx",                .dependencies = {} },                    // CELL_SYSMODULE_SYSUTIL_NP_SNS
		{ .path = "external/libgem.sprx",                           .dependencies = {} },                    // CELL_SYSMODULE_GEM
		{ .path = "external/libsysutil_photo_network_sharing.sprx", .dependencies = {} },                    // 0x5b
		{ .path = "external/libsysutil_cross_controller.sprx",      .dependencies = {} },                    // CELL_SYSMODULE_SYSUTIL_CROSS_CONTROLLER

		// 87 internal modules
		{ .path = "internal/libat3enc.sprx",                .dependencies = {} },                   // 0xf000
		{ .path = "internal/libatxenc.sprx",                .dependencies = {} },                   // 0xf001
		{ .path = "external/libvdec.sprx",                  .dependencies = {} },                   // 0xf002
		{ .path = "internal/libvpost.sprx",                 .dependencies = {} },                   // 0xf003
		{ .path = "internal/libmp3enc.sprx",                .dependencies = DEPENDENCIES_FS },      // 0xf004
		{ .path = "external/libaacenc.sprx",                .dependencies = {} },                   // 0xf005
		{ .path = "external/libadec_internal.sprx",         .dependencies = {} },                   // 0xf006
		{ .path = "internal/libapostsrc.sprx",              .dependencies = {} },                   // 0xf007
		{ .path = "internal/libaudio_internal.sprx",        .dependencies = {} },                   // 0xf008
		{ .path = "external/libavchatjpgdec.sprx",          .dependencies = {} },                   // 0xf009
		{ .path = "external/libcelpenc.sprx",               .dependencies = {} },                   // CELL_SYSMODULE_CELPENC
		{ .path = "internal/libddlenc.sprx",                .dependencies = {} },                   // 0xf00b
		{ .path = "external/libdmux.sprx",                  .dependencies = {} },                   // 0xf00c
		{ .path = "internal/libexif.sprx",                  .dependencies = DEPENDENCIES_FS },      // 0xf00d
		{ .path = "internal/libft2d.sprx",                  .dependencies = {} },                   // 0xf00e
		{ .path = "internal/libgcm_osd.sprx",               .dependencies = {} },                   // 0xf00f
		{ .path = "external/libgifdec.sprx",                .dependencies = DEPENDENCIES_FS },      // CELL_SYSMODULE_GIFDEC
		{ .path = "external/libjpgdec.sprx",                .dependencies = DEPENDENCIES_FS },      // 0xf011
		{ .path = "external/libjpgenc.sprx",                .dependencies = DEPENDENCIES_FS },      // 0xf012
		{ .path = "external/libm4venc.sprx",                .dependencies = {} },                   // 0xf013
		{ .path = "external/libpamf.sprx",                  .dependencies = {} },                   // 0xf014
		{ .path = "external/libpngdec.sprx",                .dependencies = DEPENDENCIES_FS },      // 0xf015
		{ .path = "external/libpngenc.sprx",                .dependencies = DEPENDENCIES_FS },      // 0xf016
		{ .path = "internal/libps2savedata.sprx",           .dependencies = {} },                   // 0xf017
		{ .path = "internal/libtiffdec.sprx",               .dependencies = DEPENDENCIES_FS },      // 0xf018
		{ .path = "external/libcelpdec.sprx",               .dependencies = DEPENDENCIES_ADEC_AL }, // CELL_SYSMODULE_ADEC_CELP
		{ .path = "internal/libdtsdec.sprx",                .dependencies = DEPENDENCIES_ADEC_AL }, // 0xf01a
		{ .path = "external/libm2bcdec.sprx",               .dependencies = DEPENDENCIES_ADEC_AL }, // CELL_SYSMODULE_ADEC_M2BC
		{ .path = "internal/libm2aacdec.sprx",              .dependencies = DEPENDENCIES_ADEC_AL }, // 0xf01c
		{ .path = "external/libm4aacdec.sprx",              .dependencies = DEPENDENCIES_ADEC_AL }, // CELL_SYSMODULE_ADEC_M4AAC
		{ .path = "external/libmp3dec.sprx",                .dependencies = DEPENDENCIES_ADEC_AL }, // CELL_SYSMODULE_ADEC_MP3
		{ .path = "internal/libtrhddec.sprx",               .dependencies = DEPENDENCIES_ADEC_AL }, // 0xf01f
		{ .path = "external/libsvc1d.sprx",                 .dependencies = DEPENDENCIES_VDEC_AL }, // 0xf020
		{ .path = "external/libsmvd4.sprx",                 .dependencies = DEPENDENCIES_VDEC_AL }, // 0xf021
		{ .path = "external/libsysutil_remoteplay.sprx",    .dependencies = {} },                   // 0xf022
		{ .path = "external/libsysutil_imejp.sprx",         .dependencies = {} },                   // CELL_SYSMODULE_IMEJP
		{ .path = "external/libwmadec.sprx",                .dependencies = DEPENDENCIES_ADEC_AL }, // 0xf024
		{ .path = "internal/libasfparser.sprx",             .dependencies = {} },                   // 0xf025
		{ .path = "external/libddpdec.sprx",                .dependencies = DEPENDENCIES_ADEC_AL }, // 0xf026
		{ .path = "internal/libdtslbrdec.sprx",             .dependencies = DEPENDENCIES_ADEC_AL }, // 0xf027
		{ .path = "external/libsysutil_music.sprx",         .dependencies = {} },                   // CELL_SYSMODULE_SYSUTIL_MUSIC
		{ .path = "external/libsysutil_photo_export.sprx",  .dependencies = {} },                   // CELL_SYSMODULE_PHOTO_EXPORT
		{ .path = "external/libsysutil_print.sprx",         .dependencies = {} },                   // CELL_SYSMODULE_PRINT
		{ .path = "external/libsysutil_photo_import.sprx",  .dependencies = {} },                   // CELL_SYSMODULE_PHOTO_IMPORT
		{ .path = "external/libsysutil_music_export.sprx",  .dependencies = {} },                   // CELL_SYSMODULE_MUSIC_EXPORT
		{ .path = "external/libavcenc_small.sprx",          .dependencies = {} },                   // 0xf02d
		{ .path = "external/libsysutil_photo_decode.sprx",  .dependencies = {} },                   // CELL_SYSMODULE_PHOTO_DECODE
		{ .path = "external/libsysutil_search.sprx",        .dependencies = {} },                   // CELL_SYSMODULE_SYSUTIL_SEARCH
		{ .path = "external/libsysutil_avc2.sprx",          .dependencies = {} },                   // CELL_SYSMODULE_SYSUTIL_AVCHAT2
		{ .path = "external/libmp4.sprx",                   .dependencies = {} },                   // 0xf031
		{ .path = "external/libsysutil_rtcalarm.sprx",      .dependencies = {} },                   // 0xf032
		{ .path = "external/libavcenc_small.sprx",          .dependencies = {} },                   // 0xf033
		{ .path = "external/libsail_rec.sprx",              .dependencies = {} },                   // CELL_SYSMODULE_SAIL_REC
		{ .path = "external/libsysutil_np_trophy.sprx",     .dependencies = {} },                   // CELL_SYSMODULE_SYSUTIL_NP_TROPHY
		{ .path = "external/libsjvtd.sprx",                 .dependencies = DEPENDENCIES_VDEC_AL }, // 0xf036
		{ .path = "internal/libdtshddec.sprx",              .dependencies = DEPENDENCIES_ADEC_AL }, // 0xf037
		{ .path = "internal/libmp3sdec.sprx",               .dependencies = DEPENDENCIES_ADEC_AL }, // 0xf038
		{ .path = "external/libsail_avi.sprx",              .dependencies = {} },                   // 0xf039
		{ .path = "external/libavcenc.sprx",                .dependencies = {} },                   // 0xf03a
		{ .path = "external/libapostsrc_mini.sprx",         .dependencies = {} },                   // 0xf03b
		{ .path = "external/libvoice_internal.sprx",        .dependencies = {} },                   // 0xf03c
		{ .path = "external/libmpl1dec.sprx",               .dependencies = DEPENDENCIES_ADEC_AL }, // 0xf03d
		{ .path = "external/libasfparser2_astd.sprx",       .dependencies = {} },                   // 0xf03e
		{ .path = "external/libadec2.sprx",                 .dependencies = {} },                   // 0xf03f
		{ .path = "external/libac3dec2.sprx",               .dependencies = DEPENDENCIES_ADEC2 },   // 0xf040
		{ .path = "external/libatxdec2.sprx",               .dependencies = DEPENDENCIES_ADEC2 },   // 0xf041
		{ .path = "internal/libdivx311dec.sprx",            .dependencies = DEPENDENCIES_VDEC_AL }, // 0xf042
		{ .path = "external/libvpost2.sprx",                .dependencies = {} },                   // 0xf043
		{ .path = "external/libsysutil_np_eula.sprx",       .dependencies = {} },                   // 0xf044
		{ .path = "external/libsysutil_storagedata.sprx",   .dependencies = {} },                   // 0xf045
		{ .path = "external/libm4hdenc.sprx",               .dependencies = {} },                   // 0xf046
		{ .path = "external/libsysutil_savedata_psp.sprx",  .dependencies = {} },                   // 0xf047
		{ .path = "external/libsysutil_video_player.sprx",  .dependencies = {} },                   // 0xf048
		{ .path = "external/libmvcdec.sprx",                .dependencies = DEPENDENCIES_VDEC_AL }, // 0xf049
		{ .path = "external/libaacenc_spurs.sprx",          .dependencies = {} },                   // 0xf04a
		{ .path = "internal/libdtshdcoredec.sprx",          .dependencies = DEPENDENCIES_ADEC_AL }, // 0xf04b
		{ .path = "internal/libpidvd.sprx",                 .dependencies = {} },                   // 0xf04c
		{ .path = "external/libsysutil_dtcp_ip.sprx",       .dependencies = {} },                   // 0xf04d
		{ .path = "external/libsysutil_syschat.sprx",       .dependencies = {} },                   // 0xf04e
		{ .path = "external/libsysutil_np_installer.sprx",  .dependencies = {} },                   // 0xf04f
		{ .path = "external/libbemp2sys.sprx",              .dependencies = {} },                   // 0xf050
		{ .path = "external/libbeisobmf.sprx",              .dependencies = {} },                   // 0xf051
		{ .path = "external/libsysutil_photo_export2.sprx", .dependencies = {} },                   // 0xf052
		{ .path = "external/libat3multidec.sprx",           .dependencies = DEPENDENCIES_ADEC_AL }, // 0xf053
		{ .path = "external/libatrac3multi.sprx",           .dependencies = DEPENDENCIES_AT3M },    // CELL_SYSMODULE_LIBATRAC3MULTI
		{ .path = "external/libsysutil_dec_psnvideo.sprx",  .dependencies = {} },                   // 0xf055
		{ .path = "external/libdtslbrdec.sprx",             .dependencies = DEPENDENCIES_ADEC_AL }, // 0xf056

		// Special modules
		{ .path = "external/liblv2dbg_for_dex.sprx", .dependencies = {} },
		{ .path = "external/liblv2dbg_for_cex.sprx", .dependencies = {} },

		{ .path = "external/libgcm_sys_deh.sprx",    .dependencies = {} },
		{ .path = "external/libgcm_sys.sprx",        .dependencies = {} },

		{ .path = "external/libfs.sprx",             .dependencies = {} },
		{ .path = "external/libfs_155.sprx",         .dependencies = {} },

		{ .path = "external/libprof.sprx",           .dependencies = {} },
		{ .path = "external/liblv2coredump.sprx",    .dependencies = {} },
		{ .path = "external/libgpad.sprx",           .dependencies = {} }
	};

	constexpr u8 INTERNAL_MODULES_OFFSET = 93;
	constexpr u8 INTERNAL_MODULES_COUNT = 87;
	constexpr u8 LV2_DBG_IDX = INTERNAL_MODULES_OFFSET + INTERNAL_MODULES_COUNT;
	constexpr u8 GCM_SYS_DEH_IDX = LV2_DBG_IDX + 2;
	constexpr u8 FS_IDX = LV2_DBG_IDX + 4;
	constexpr u8 PROF_IDX = LV2_DBG_IDX + 6;
	constexpr u8 LV2COREDUMP_IDX = LV2_DBG_IDX + 7;
	constexpr u8 GPAD_IDX = LV2_DBG_IDX + 8;

	constexpr std::string_view MODULE_BASE_PATH = "/dev_flash/sys/";
	constexpr std::string_view MODULE_BASE_PATH_DEBUG = "/app_home/";
	constexpr u8 BASE_PATH_MAX_SIZE = 0x10;
	constexpr u8 PATH_MAX_SIZE = 0x40;
	static_assert(MODULE_BASE_PATH.size() < BASE_PATH_MAX_SIZE && MODULE_BASE_PATH_DEBUG.size() < BASE_PATH_MAX_SIZE);
	static_assert(std::ranges::all_of(MODULE_INFOS, [](const auto& path) { return path.size() + BASE_PATH_MAX_SIZE < PATH_MAX_SIZE; }, &module_info::path));

	constexpr std::array<u16, 7> DEFAULT_MODULES
	{
		CELL_SYSMODULE_SYSUTIL,
		CELL_SYSMODULE_GCM_SYS,
		CELL_SYSMODULE_AUDIO,
		CELL_SYSMODULE_IO,
		CELL_SYSMODULE_SPURS,
		CELL_SYSMODULE_FS,
		CELL_SYSMODULE_SYSUTIL_NP_TROPHY
	};

	constexpr bit_set<INTERNAL_MODULES_COUNT> LOADABLE_INTERNAL_MODULES
	{
		"0010000"          // 0xf054
		"0000000000010000" // 0xf044
		"0000000000110001" // 0xf030, 0xf034, 0xf035
		"1101111100001000" // 0xf023, 0xf028, 0xf029, 0xf02a, 0xf02b, 0xf02c, 0xf02e, 0xf02f
		"0110101000000001" // 0xf010, 0xf019, 0xf01b, 0xf01d, 0xf01e
		"0000010000000000" // 0xf00a
	};

	constexpr bit_set<INTERNAL_MODULES_COUNT> LOADABLE_INTERNAL_MODULES_EX
	{
		"1110111"          // 0xf050, 0xf051, 0xf052, 0xf054, 0xf055, 0xf056
		"1110011111111011" // 0xf040, 0xf041, 0xf043, 0xf044, 0xf045, 0xf046, 0xf047, 0xf048, 0xf049, 0xf04a, 0xf04d, 0xf04e, 0xf04f
		"1110011001111101" // 0xf030, 0xf032, 0xf033, 0xf034, 0xf035, 0xf036, 0xf039, 0xf03a, 0xf03d, 0xf03e, 0xf03f
		"1101111101011111" // 0xf020, 0xf021, 0xf022, 0xf023, 0xf024, 0xf026, 0xf028, 0xf029, 0xf02a, 0xf02b, 0xf02c, 0xf02e, 0xf02f
		"0110101000001001" // 0xf010, 0xf013, 0xf019, 0xf01b, 0xf01d, 0xf01e
		"0000010000100000" // 0xf005, 0xf00a
	};

	constexpr std::array<u8, 0x10> KEY_UNK{ 'S', 'r', 'e', 'k', 'i', 'r', 't', 'S', 'a', 'h', 'o', 'n', 'a', 'N', 'l', 'a' };

	struct module_state
	{
		s32 loaded_count;
		s32 prx_id;
	};

	struct sysmodule_context
	{
		sys_lwmutex_t mutex;

		be_t<u32> unk_addr;

		u32 mem_container_id;
		b8 use_mem_container;

		module_state module_states[MODULE_INFOS.size()];

		char module_base_path[BASE_PATH_MAX_SIZE];
		u8 module_base_path_size;
	};

	vm::gvar<sysmodule_context> s_sysmodule_context;
}

template <CellSysmoduleModuleID module_id>
[[nodiscard]] static bool check_special_handling(ppu_thread& ppu)
{
	// TODO replace with proper struct
	const vm::var<char[]> paramsfo(0x40);
	std::memset(paramsfo.get_ptr(), 0, paramsfo.get_count());

	const error_code ret = ppu_execute<&sys_process_get_paramsfo>(ppu, +paramsfo);

	switch (module_id)
	{
	case CELL_SYSMODULE_LV2DBG:
	{
		// If the LSB of the u64 at offset 0x18 and the extra load flag "EnableLv2ExceptionHandler" is set,
		// or a certain bit in bootflag.dat, loads liblv2dbg_for_dex.sprx instead of liblv2dbg_for_cex
		const vm::var<std::byte[]> boot_flag(8);
		std::memset(boot_flag.get_ptr(), 0, boot_flag.get_count());

		if (ret != CELL_OK || !read_from_ptr_unsafe<bf_t<u64, 0, 1>>(paramsfo.get_ptr(), 0x18))
		{
			const vm::var<u32> file_descriptor;

			if (sys_fs_open(ppu, vm::make_str("/dev_hdd0/data/bootflag.dat"), 0, file_descriptor, 0, vm::null, 0) != CELL_OK)
			{
				return true;
			}

			const vm::var<u64> nread;
			ensure(sys_fs_read(ppu, *file_descriptor, boot_flag, 8, nread) == CELL_OK && *nread == 8ull); // Not checked on LLE
			ensure(sys_fs_close(ppu, *file_descriptor) == CELL_OK); // Not checked on LLE

			return !!read_from_ptr_unsafe<bf_t<u64, 4, 1>>(boot_flag.get_ptr(), 1);
		}

		return !read_from_ptr_unsafe<bf_t<u64, 0, 1>>(paramsfo.get_ptr(), 0x10);
	}
	case CELL_SYSMODULE_GCM_SYS:
	{
		// If the extra load flag "EnableGCMDebug" is set, loads libgcm_sys_deh.sprx instead of libgcm_sys.sprx
		return ret != CELL_OK || !read_from_ptr_unsafe<bf_t<u64, 2, 1>>(paramsfo.get_ptr(), 0x10);
	}
	case CELL_SYSMODULE_FS:
	{
		// "F.E.A.R. First Encounter Assault Recon" needs an older version of cellFs
		return std::ranges::contains(std::array{ "BLUS30003", "BLES00035", "BLES00036" }, std::string_view{ paramsfo.get_ptr() + 1, 9 });
	}
	case CELL_SYSMODULE_SAIL:
	{
		// "Armored Core 4" needs a frame buffer release delay of three frames
		return std::ranges::contains(std::array{ "BLJM60012", "BLES00039", "BLUS30027", "BLKS20001" }, std::string_view{ paramsfo.get_ptr() + 1, 9 });
	}
	default:
		std::unreachable();
	}
}

static error_code load_module(ppu_thread& ppu, u8 module_idx, u32 args, vm::ptr<void> argp)
{
	cellSysmodule.notice("load_module(module_idx=%d, args=%d, argp=*0x%x)", module_idx, args, argp);

	ensure(module_idx < MODULE_INFOS.size());
	auto& [loaded_count, prx_id] = s_sysmodule_context->module_states[module_idx];
	const auto& [path, dependencies] = MODULE_INFOS[module_idx];

	cellSysmodule.notice("load_module(): path=\"%s\", loaded_count=%d, prx_id=%d", path, loaded_count, prx_id);

	if (path.empty())
	{
		return CELL_OK;
	}

	if (loaded_count == smax
		|| ppu_execute<&sys_lwmutex_lock>(ppu, s_sysmodule_context.ptr(&sysmodule_context::mutex), 0) != CELL_OK)
	{
		return CELL_SYSMODULE_ERROR_FATAL;
	}

	if (loaded_count == 0)
	{
		const vm::var<char[]> module_path(72);
		std::memcpy(module_path.get_ptr(), s_sysmodule_context->module_base_path, s_sysmodule_context->module_base_path_size);
		std::memcpy(module_path.get_ptr() + s_sysmodule_context->module_base_path_size, path.data(), path.size() + 1);

		const error_code ret = s_sysmodule_context->use_mem_container
			? ppu_execute<&sys_prx_load_module_on_memcontainer>(ppu, +module_path, s_sysmodule_context->mem_container_id, 0, vm::null)
			: ppu_execute<&sys_prx_load_module>(ppu, +module_path, 0, vm::null);

		prx_id = ret;

		if (ret < 1)
		{
			ensure(ppu_execute<&sys_lwmutex_unlock>(ppu, s_sysmodule_context.ptr(&sysmodule_context::mutex)) == CELL_OK); // Not checked on LLE
			return ret;
		}

		if (const error_code ret = ppu_execute<&sys_prx_start_module>(ppu, prx_id, args, argp, +vm::make_var(0), 0, vm::null); ret != CELL_OK)
		{
			ensure(ppu_execute<&sys_prx_unload_module>(ppu, prx_id, 0, vm::null) == CELL_OK); // Not checked on LLE
			ensure(ppu_execute<&sys_lwmutex_unlock>(ppu, s_sysmodule_context.ptr(&sysmodule_context::mutex)) == CELL_OK); // Not checked on LLE
			return ret;
		}
	}

	loaded_count++;

	ensure(ppu_execute<&sys_lwmutex_unlock>(ppu, s_sysmodule_context.ptr(&sysmodule_context::mutex)) == CELL_OK); // Not checked on LLE
	return CELL_OK;
}

template <bool force_unload>
static error_code unload_module(ppu_thread& ppu, u8 module_idx)
{
	cellSysmodule.notice("unload_module(module_idx=%d)", module_idx);

	ensure(module_idx < MODULE_INFOS.size());
	auto& [loaded_count, prx_id] = s_sysmodule_context->module_states[module_idx];
	const std::string_view& path = MODULE_INFOS[module_idx].path;

	cellSysmodule.notice("unload_module(): path=\"%s\", loaded_count=%d, prx_id=%d", path, loaded_count, prx_id);

	if (path.empty())
	{
		return CELL_OK;
	}

	if (ppu_execute<&sys_lwmutex_lock>(ppu, s_sysmodule_context.ptr(&sysmodule_context::mutex), 0) != CELL_OK)
	{
		return CELL_SYSMODULE_ERROR_FATAL;
	}

	if (loaded_count == 1 || force_unload)
	{
		ensure(ppu_execute<&sys_prx_stop_module>(ppu, prx_id, 0, vm::null, +vm::make_var(0), 0, vm::null) == CELL_OK); // Not checked on LLE
		ensure(ppu_execute<&sys_prx_unload_module>(ppu, prx_id, 0, vm::null) == CELL_OK); // Not checked on LLE
		loaded_count = 0;
	}
	else if (loaded_count > 1)
	{
		loaded_count--;
	}

	ensure(ppu_execute<&sys_lwmutex_unlock>(ppu, s_sysmodule_context.ptr(&sysmodule_context::mutex)) == CELL_OK); // Not checked on LLE
	return CELL_OK;
}

extern error_code sysmoduleModuleStop(ppu_thread& ppu)
{
	// Blocking savestate creation due to ppu_thread::fast_call()
	const std::unique_lock savestate_lock{ g_fxo->get<hle_locks_t>(), std::try_to_lock };

	if (!savestate_lock)
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellSysmodule.notice("module_stop()");

	if (s_sysmodule_context->unk_addr)
	{
		if (sys_memory_free(ppu, s_sysmodule_context->unk_addr) != CELL_OK)
		{
			return 1; // SYS_PRX_STOP_FAILED
		}

		s_sysmodule_context->unk_addr = 0;
	}

	if (ppu_execute<&sys_lwmutex_destroy>(ppu, s_sysmodule_context.ptr(&sysmodule_context::mutex)) != CELL_OK)
	{
		return 1; // SYS_PRX_STOP_FAILED
	}

	return CELL_OK; // SYS_PRX_STOP_OK
}

extern error_code sysmoduleModuleStart(ppu_thread& ppu, u32 args, vm::ptr<void> argp)
{
	// Blocking savestate creation due to ppu_thread::fast_call()
	const std::unique_lock savestate_lock{ g_fxo->get<hle_locks_t>(), std::try_to_lock };

	if (!savestate_lock)
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellSysmodule.notice("module_start(args=%d, argp=*0x%x)", args, argp);

	s_sysmodule_context->use_mem_container = false;

	if (const vm::var<s64> auth_id; sys_ss_access_control_engine(1, sys_process_getpid(), auth_id.addr()) == CELL_OK)
	{
		if ((*auth_id & 0x00ffffff) == 1 || (*auth_id & 0x00ffffff) == 2)
		{
			return 1; // SYS_PRX_NO_RESIDENT
		}
	}
	else if (sys_ss_access_control_engine(2, auth_id.addr(), 0) != CELL_OK || *auth_id == PAID_44 || *auth_id == 0x1070000056000001ll)
	{
		return 1; // SYS_PRX_NO_RESIDENT
	}

	const vm::var<sys_lwmutex_attribute_t> lwmutex_attr
	{{
		.protocol = SYS_SYNC_PRIORITY,
		.recursive = SYS_SYNC_NOT_RECURSIVE,
		.name_u64 = "_smolwm"_u64
	}};

	if (ppu_execute<&sys_lwmutex_create>(ppu, s_sysmodule_context.ptr(&sysmodule_context::mutex), +lwmutex_attr) != CELL_OK)
	{
		return 1; // SYS_PRX_NO_RESIDENT
	}

	// TODO replace with proper struct
	const vm::var<char[]> paramsfo(0x40);
	std::memset(paramsfo.get_ptr(), 0, paramsfo.get_count());

	error_code get_paramsfo_ret = ppu_execute<&sys_process_get_paramsfo>(ppu, +paramsfo);
	const bool retail_gcm_sys = check_special_handling<CELL_SYSMODULE_GCM_SYS>(ppu);

	// Load default modules
	if (const vm::var<s32> sdk_version;
		ensure(sys_process_get_sdk_version(sys_process_getpid(), sdk_version) == CELL_OK), // Not checked on LLE
		*sdk_version < 0x330000)
	{
		for (const u16 module_id : DEFAULT_MODULES | std::views::take(5) )
		{
			if (module_id == CELL_SYSMODULE_GCM_SYS)
			{
				if (get_paramsfo_ret == CELL_OK && read_from_ptr_unsafe<bf_t<u64, 6, 1>>(paramsfo.get_ptr(), 0x10) && !retail_gcm_sys)
				{
					if (load_module(ppu, GCM_SYS_DEH_IDX, 8, paramsfo + 0x28) != CELL_OK)
					{
						sysmoduleModuleStop(ppu);
						return 1; // SYS_PRX_NO_RESIDENT
					}
				}
				else
				{
					if (load_module(ppu, GCM_SYS_DEH_IDX + retail_gcm_sys, 0, vm::null) != CELL_OK)
					{
						sysmoduleModuleStop(ppu);
						return 1; // SYS_PRX_NO_RESIDENT
					}
				}

				s_sysmodule_context->module_states[CELL_SYSMODULE_GCM_SYS].loaded_count =
					s_sysmodule_context->module_states[GCM_SYS_DEH_IDX + retail_gcm_sys].loaded_count;
			}
			else
			{
				if (cellSysmoduleLoadModule(ppu, module_id) != CELL_OK)
				{
					sysmoduleModuleStop(ppu);
					return 1; // SYS_PRX_NO_RESIDENT
				}
			}
		}
	}
	else
	{
		// Build a path list of the default modules
		const vm::var<char[]> paths(PATH_MAX_SIZE * size32(DEFAULT_MODULES));
		const vm::var<vm::bcptr<char>[]> path_list(size32(DEFAULT_MODULES));

		for (u32 i = 0; i < DEFAULT_MODULES.size(); i++)
		{
			const std::string_view path = DEFAULT_MODULES[i] == CELL_SYSMODULE_GCM_SYS
				? MODULE_INFOS[GCM_SYS_DEH_IDX + retail_gcm_sys].path
				: DEFAULT_MODULES[i] >= INTERNAL_MODULE_ID_BASE
					? MODULE_INFOS[INTERNAL_MODULES_OFFSET + (DEFAULT_MODULES[i] & INTERNAL_MODULE_ID_MASK)].path
					: MODULE_INFOS[DEFAULT_MODULES[i]].path;

			std::memcpy(paths.get_ptr() + (i * PATH_MAX_SIZE), MODULE_BASE_PATH.data(), MODULE_BASE_PATH.size());
			std::memcpy(paths.get_ptr() + (i * PATH_MAX_SIZE) + MODULE_BASE_PATH.size(), path.data(), path.size() + 1);

			path_list[i] = paths + i * PATH_MAX_SIZE;
		}

		// Load them all at once
		const vm::var<u32[]> id_list(size32(DEFAULT_MODULES));

		if (ppu_execute<&sys_prx_load_module_list>(ppu, DEFAULT_MODULES.size(), +path_list, 0, vm::null, +id_list) != CELL_OK)
		{
			sysmoduleModuleStop(ppu);
			return 1; // SYS_PRX_NO_RESIDENT
		}

		// Start the modules
		for (const auto [module_id, prx_ix] : std::views::zip(DEFAULT_MODULES, std::span{ id_list.begin().get_ptr(), id_list.get_count() }))
		{
			if (module_id == CELL_SYSMODULE_GCM_SYS && read_from_ptr_unsafe<bf_t<u64, 6, 1>>(paramsfo.get_ptr(), 0x10) && !retail_gcm_sys)
			{
				s_sysmodule_context->module_states[CELL_SYSMODULE_GCM_SYS].prx_id = static_cast<s32>(prx_ix);

				if (ppu_execute<&sys_prx_start_module>(ppu, +prx_ix, 8, paramsfo + 0x28, +vm::make_var(0), 0, vm::null) != CELL_OK)
				{
					sysmoduleModuleStop(ppu);
					return 1; // SYS_PRX_NO_RESIDENT
				}

				s_sysmodule_context->module_states[CELL_SYSMODULE_GCM_SYS].loaded_count++;
			}
			else
			{
				const u8 module_idx = module_id >= INTERNAL_MODULE_ID_BASE ? INTERNAL_MODULES_OFFSET + (module_id & INTERNAL_MODULE_ID_MASK) : module_id;

				s_sysmodule_context->module_states[module_idx].prx_id = static_cast<s32>(prx_ix);

				if (ppu_execute<&sys_prx_start_module>(ppu, +prx_ix, 0, vm::null, +vm::make_var(0), 0, vm::null) != CELL_OK)
				{
					sysmoduleModuleStop(ppu);
					return 1;// SYS_PRX_NO_RESIDENT
				}

				s_sysmodule_context->module_states[module_idx].loaded_count++;
			}

			if (module_id == CELL_SYSMODULE_GCM_SYS)
			{
				s_sysmodule_context->module_states[GCM_SYS_DEH_IDX + retail_gcm_sys].loaded_count =
					s_sysmodule_context->module_states[CELL_SYSMODULE_GCM_SYS].loaded_count;
			}
			else if (module_id == CELL_SYSMODULE_FS)
			{
				s_sysmodule_context->module_states[FS_IDX + check_special_handling<CELL_SYSMODULE_FS>(ppu)].loaded_count =
					s_sysmodule_context->module_states[CELL_SYSMODULE_FS].loaded_count;
			}
		}

		// LLE overwrites the return value of sys_process_get_paramsfo() here and won't return an error if the allocation fails
		get_paramsfo_ret = sys_memory_allocate(ppu, (!retail_gcm_sys + 1) * 0x10000ull, SYS_MEMORY_PAGE_SIZE_64K, s_sysmodule_context.ptr(&sysmodule_context::unk_addr));
	}

	if (get_paramsfo_ret != CELL_OK)
	{
		return CELL_OK; // SYS_PRX_RESIDENT
	}

	// Load additional modules if certain ExtraLoadFlags are set

	if (read_from_ptr_unsafe<bf_t<u64, 6, 1>>(paramsfo.get_ptr(), 0x10) && !retail_gcm_sys && read_from_ptr_unsafe<bf_t<u64, 0, 1>>(paramsfo.get_ptr(), 0x18))
	{
		cellSysmoduleSetDebugmode(true);

		if (load_module(ppu, GPAD_IDX, 8, paramsfo + 0x28) != CELL_OK)
		{
			ensure(unload_module<true>(ppu, GPAD_IDX) == CELL_OK); // Not checked on LLE
		}

		cellSysmoduleSetDebugmode(false);
	}

	if (read_from_ptr_unsafe<bf_t<u64, 3, 1>>(paramsfo.get_ptr(), 0x10) && read_from_ptr_unsafe<bf_t<u64, 0, 1>>(paramsfo.get_ptr(), 0x18))
	{
		if (load_module(ppu, PROF_IDX, 0, vm::null) != CELL_OK)
		{
			ensure(unload_module<true>(ppu, PROF_IDX) == CELL_OK); // Not checked on LLE
		}
	}

	if (read_from_ptr_unsafe<bf_t<u64, 4, 1>>(paramsfo.get_ptr(), 0x10))
	{
		if (load_module(ppu, LV2COREDUMP_IDX, 0, vm::null) != CELL_OK)
		{
			ensure(unload_module<true>(ppu, LV2COREDUMP_IDX) == CELL_OK); // Not checked on LLE
		}
	}

	if (read_from_ptr_unsafe<bf_t<u64, 1, 1>>(paramsfo.get_ptr(), 0x10))
	{
		if (cellSysmoduleLoadModuleInternal(ppu, 0xf022) != CELL_OK)
		{
			sysmoduleModuleStop(ppu);
			return 1; // SYS_PRX_NO_RESIDENT
		}
	}

	return CELL_OK; // SYS_PRX_RESIDENT
}

error_code cellSysmoduleInitialize()
{
	cellSysmodule.notice("cellSysmoduleInitialize()");
	// Doesn't do anything
	return CELL_OK;
}

error_code cellSysmoduleFinalize()
{
	cellSysmodule.notice("cellSysmoduleFinalize()");
	// Doesn't do anything
	return CELL_OK;
}

error_code cellSysmoduleSetMemcontainer(ppu_thread& ppu, u32 ct_id)
{
	// Blocking savestate creation due to ppu_thread::fast_call()
	const std::unique_lock savestate_lock{ g_fxo->get<hle_locks_t>(), std::try_to_lock };

	if (!savestate_lock)
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellSysmodule.notice("cellSysmoduleSetMemcontainer(ct_id=0x%x)", ct_id);

	if (ppu_execute<&sys_lwmutex_lock>(ppu, s_sysmodule_context.ptr(&sysmodule_context::mutex), 0) != CELL_OK)
	{
		return CELL_SYSMODULE_ERROR_FATAL;
	}

	if (ct_id == SYS_MEMORY_CONTAINER_ID_INVALID)
	{
		s_sysmodule_context->use_mem_container = false;
	}
	else
	{
		if (const vm::var<sys_memory_info_t> memory_info;
			sys_memory_container_get_size(ppu, +memory_info, ct_id) != CELL_OK || memory_info->available_user_memory < 0x60000)
		{
			s_sysmodule_context->use_mem_container = false;
			ensure(ppu_execute<&sys_lwmutex_unlock>(ppu, s_sysmodule_context.ptr(&sysmodule_context::mutex)) == CELL_OK); // Not checked on LLE
			return CELL_SYSMODULE_ERROR_INVALID_MEMCONTAINER;
		}

		s_sysmodule_context->use_mem_container = true;
		s_sysmodule_context->mem_container_id = ct_id;
	}

	ensure(ppu_execute<&sys_lwmutex_unlock>(ppu, s_sysmodule_context.ptr(&sysmodule_context::mutex)) == CELL_OK); // Not checked on LLE
	return CELL_OK;
}

error_code cellSysmoduleLoadModule(ppu_thread& ppu, u16 id)
{
	// Blocking savestate creation due to ppu_thread::fast_call()
	const std::unique_lock savestate_lock{ g_fxo->get<hle_locks_t>(), std::try_to_lock };

	if (!savestate_lock)
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellSysmodule.notice("cellSysmoduleLoadModule(id=0x%x)", id);

	if (id < INTERNAL_MODULES_OFFSET)
	{
		switch (id)
		{
		case CELL_SYSMODULE_FS:
		{
			const bool fs_155 = check_special_handling<CELL_SYSMODULE_FS>(ppu);
			const error_code ret = load_module(ppu, FS_IDX + fs_155, 0, vm::null);
			s_sysmodule_context->module_states[CELL_SYSMODULE_FS].loaded_count = s_sysmodule_context->module_states[FS_IDX + fs_155].loaded_count;
			return ret;
		}
		case CELL_SYSMODULE_GCM_SYS:
		{
			const bool retail_gcm_sys = check_special_handling<CELL_SYSMODULE_GCM_SYS>(ppu);
			const error_code ret = load_module(ppu, GCM_SYS_DEH_IDX + retail_gcm_sys, 0, vm::null);
			s_sysmodule_context->module_states[CELL_SYSMODULE_GCM_SYS].loaded_count = s_sysmodule_context->module_states[GCM_SYS_DEH_IDX + retail_gcm_sys].loaded_count;
			return ret;
		}
		case CELL_SYSMODULE_SAIL:
		{
			if (check_special_handling<CELL_SYSMODULE_SAIL>(ppu))
			{
				const vm::var<u32[]> sail_parameters(2);
				sail_parameters[0] = 0x19; // Parameter type? Must be set to this, or else cellSail will not set the frame buffer release delay below (see module_start() of cellSail)
				sail_parameters[1] = 3;    // Delay the release of the frame buffer by three frames instead of two
				return load_module(ppu, CELL_SYSMODULE_SAIL, 8, +sail_parameters);
			}

			return load_module(ppu, CELL_SYSMODULE_SAIL, 0, vm::null);
		}
		case CELL_SYSMODULE_LV2DBG:
		{
			const bool cex_lv2dgb = check_special_handling<CELL_SYSMODULE_LV2DBG>(ppu);
			const error_code ret = load_module(ppu, LV2_DBG_IDX + cex_lv2dgb, 0, vm::null);
			s_sysmodule_context->module_states[CELL_SYSMODULE_LV2DBG].loaded_count = s_sysmodule_context->module_states[LV2_DBG_IDX + cex_lv2dgb].loaded_count;
			return ret;
		}
		case 0x58:
		{
			return CELL_SYSMODULE_ERROR_UNKNOWN;
		}
		default:
		{
			// Load dependencies
			for (const u16 module_id : MODULE_INFOS[id].dependencies)
			{
				if (module_id >= INTERNAL_MODULE_ID_BASE)
				{
					return CELL_SYSMODULE_ERROR_UNKNOWN;
				}

				if (const error_code ret = cellSysmoduleLoadModule(ppu, module_id);
					ret != CELL_OK && ret != static_cast<s32>(CELL_SYSMODULE_ERROR_DUPLICATED))
				{
					return ret;
				}
			}

			return load_module(ppu, static_cast<u8>(id), 0, vm::null);
		}
		}
	}

	if ((id & INTERNAL_MODULE_ID_MASK) >= INTERNAL_MODULES_COUNT
		|| !LOADABLE_INTERNAL_MODULES[id & INTERNAL_MODULE_ID_MASK])
	{
		return CELL_SYSMODULE_ERROR_UNKNOWN;
	}

	return cellSysmoduleLoadModuleInternal(ppu, id);
}

error_code cellSysmoduleUnloadModule(ppu_thread& ppu, u16 id)
{
	// Blocking savestate creation due to ppu_thread::fast_call()
	const std::unique_lock savestate_lock{ g_fxo->get<hle_locks_t>(), std::try_to_lock };

	if (!savestate_lock)
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellSysmodule.notice("cellSysmoduleUnloadModule(id=0x%x)", id);

	if (id == 0x58)
	{
		return CELL_SYSMODULE_ERROR_UNKNOWN;
	}

	if (id < INTERNAL_MODULES_OFFSET)
	{
		const vm::var<s32> sdk_version;
		ensure(sys_process_get_sdk_version(sys_process_getpid(), sdk_version) == CELL_OK); // Not checked on LLE

		// Do not unload a default module
		for (const u16 module_id : DEFAULT_MODULES | std::views::take(*sdk_version < 0x330000 ? 5 : DEFAULT_MODULES.size()))
		{
			if (ppu_execute<&sys_lwmutex_lock>(ppu, s_sysmodule_context.ptr(&sysmodule_context::mutex), 0) != CELL_OK)
			{
				return CELL_SYSMODULE_ERROR_FATAL;
			}

			if (module_id == id && s_sysmodule_context->module_states[id].loaded_count == 1)
			{
				ensure(ppu_execute<&sys_lwmutex_unlock>(ppu, s_sysmodule_context.ptr(&sysmodule_context::mutex)) == CELL_OK); // Not checked on LLE
				return CELL_OK;
			}

			ensure(ppu_execute<&sys_lwmutex_unlock>(ppu, s_sysmodule_context.ptr(&sysmodule_context::mutex)) == CELL_OK); // Not checked on LLE
		}

		error_code ret;
		switch (id)
		{
		case CELL_SYSMODULE_FS:
		{
			const bool fs_155 = check_special_handling<CELL_SYSMODULE_FS>(ppu);
			ret = unload_module<false>(ppu, FS_IDX + fs_155);
			s_sysmodule_context->module_states[CELL_SYSMODULE_FS].loaded_count = s_sysmodule_context->module_states[FS_IDX + fs_155].loaded_count;
			break;
		}
		case CELL_SYSMODULE_GCM_SYS:
		{
			const bool retail_gcm_sys = check_special_handling<CELL_SYSMODULE_GCM_SYS>(ppu);
			ret = unload_module<false>(ppu, GCM_SYS_DEH_IDX + retail_gcm_sys);
			s_sysmodule_context->module_states[CELL_SYSMODULE_GCM_SYS].loaded_count = s_sysmodule_context->module_states[GCM_SYS_DEH_IDX + retail_gcm_sys].loaded_count;
			break;
		}
		case CELL_SYSMODULE_LV2DBG:
		{
			const bool cex_lv2dbg = check_special_handling<CELL_SYSMODULE_LV2DBG>(ppu);
			ret = unload_module<false>(ppu, LV2_DBG_IDX + cex_lv2dbg);
			s_sysmodule_context->module_states[CELL_SYSMODULE_LV2DBG].loaded_count = s_sysmodule_context->module_states[LV2_DBG_IDX + cex_lv2dbg].loaded_count;
			break;
		}
		default:
		{
			ret = unload_module<false>(ppu, static_cast<u8>(id));
		}
		}

		if (ret < CELL_OK)
		{
			return ret;
		}

		// Unload dependencies in reverse order
		for (const u16 module_id : MODULE_INFOS[id].dependencies | std::views::reverse)
		{
			if (module_id >= INTERNAL_MODULE_ID_BASE)
			{
				return CELL_SYSMODULE_ERROR_UNKNOWN;
			}

			if (const error_code ret = cellSysmoduleUnloadModule(ppu, module_id); ret != CELL_OK)
			{
				return ret;
			}
		}

		return CELL_OK;
	}

	if ((id & INTERNAL_MODULE_ID_MASK) >= INTERNAL_MODULES_COUNT
		|| !LOADABLE_INTERNAL_MODULES[id & INTERNAL_MODULE_ID_MASK])
	{
		return CELL_SYSMODULE_ERROR_UNKNOWN;
	}

	return cellSysmoduleUnloadModuleInternal(ppu, id);
}

error_code cellSysmoduleIsLoaded(ppu_thread& ppu, u16 id)
{
	// Blocking savestate creation due to ppu_thread::fast_call()
	const std::unique_lock savestate_lock{ g_fxo->get<hle_locks_t>(), std::try_to_lock };

	if (!savestate_lock)
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellSysmodule.notice("cellSysmoduleIsLoaded(id=0x%x)", id);

	if (id == 0x58)
	{
		return CELL_SYSMODULE_ERROR_UNKNOWN;
	}

	if (id < INTERNAL_MODULES_OFFSET)
	{
		if (ppu_execute<&sys_lwmutex_lock>(ppu, s_sysmodule_context.ptr(&sysmodule_context::mutex), 0) != CELL_OK)
		{
			return CELL_SYSMODULE_ERROR_FATAL;
		}

		const u16 module_id = MODULE_INFOS[id].path.empty() ? *MODULE_INFOS[id].dependencies.rbegin() : id;
		const s32 loaded_count = s_sysmodule_context->module_states[module_id].loaded_count;

		ensure(ppu_execute<&sys_lwmutex_unlock>(ppu, s_sysmodule_context.ptr(&sysmodule_context::mutex)) == CELL_OK); // Not checked on LLE

		return loaded_count > 0 ? CELL_OK : not_an_error(CELL_SYSMODULE_ERROR_UNLOADED);
	}

	if ((id & INTERNAL_MODULE_ID_MASK) >= INTERNAL_MODULES_COUNT
		|| !LOADABLE_INTERNAL_MODULES[id & INTERNAL_MODULE_ID_MASK])
	{
		return CELL_SYSMODULE_ERROR_UNKNOWN;
	}

	return cellSysmoduleIsLoadedEx(ppu, id);
}

error_code cellSysmoduleGetImagesize(ppu_thread& ppu, u16 id, vm::ptr<u32> image_size)
{
	cellSysmodule.notice("cellSysmoduleGetImagesize(id=0x%x, image_size=*0x%x)", id, image_size);

	ensure(!!image_size); // Not checked on LLE

	*image_size = 0;

	if (id < UNK_MODULE_ID_BASE || (id & UNK_MODULE_ID_MASK) >= 2)
	{
		return CELL_SYSMODULE_ERROR_FATAL;
	}

	const vm::var<u32> file_descriptor;

	if (const vm::var<char[]> path = vm::make_str((id & UNK_MODULE_ID_MASK) == 0 ? "/dev_flash/sys/external/flashATRAC.pic" : ""); // The second string is empty on LLE
		sys_fs_open(ppu, path, 0, file_descriptor, 0, vm::null, 0) != CELL_OK)
	{
		return CELL_SYSMODULE_ERROR_FATAL;
	}

	const vm::var<CellFsStat> sb;

	if (sys_fs_fstat(ppu, *file_descriptor, sb) != CELL_OK)
	{
		ensure(sys_fs_close(ppu, *file_descriptor) == CELL_OK); // Not checked on LLE
		return CELL_SYSMODULE_ERROR_FATAL;
	}

	*image_size = static_cast<u32>(sb->size);

	ensure(sys_fs_close(ppu, *file_descriptor) == CELL_OK); // Not checked on LLE
	return CELL_OK;
}

error_code cellSysmoduleFetchImage(ppu_thread& ppu, u16 id, vm::ptr<void> image_buf, vm::ptr<u32> buf_size)
{
	cellSysmodule.notice("cellSysmoduleFetchImage(id=0x%x, image_buf=*0x%x, buf_size=*0x%x)", id, image_buf, buf_size);

	if (id < UNK_MODULE_ID_BASE)
	{
		return CELL_SYSMODULE_ERROR_FATAL;
	}

	const vm::var<u32> file_descriptor;

	ensure(!!buf_size); // Not checked on LLE

	if (const vm::var<char[]> path = vm::make_str((id & UNK_MODULE_ID_MASK) == 0 ? "/dev_flash/sys/external/flashATRAC.pic" : ""); // The second string is empty on LLE
		(id & UNK_MODULE_ID_MASK) >= 2 || sys_fs_open(ppu, path, 0, file_descriptor, 0, vm::null, 0) != CELL_OK)
	{
		*buf_size = 0;
		return CELL_SYSMODULE_ERROR_FATAL;
	}

	const vm::var<CellFsStat> sb;

	if (sys_fs_fstat(ppu, *file_descriptor, sb) != CELL_OK)
	{
		ensure(sys_fs_close(ppu, *file_descriptor) == CELL_OK); // Not checked on LLE
		*buf_size = 0;
		return CELL_SYSMODULE_ERROR_FATAL;
	}

	const u64 read_size = std::min<u64>(*buf_size, sb->size);

	if (const vm::var<u64> nread; sys_fs_read(ppu, *file_descriptor, image_buf, read_size, nread) != CELL_OK)
	{
		*buf_size = 0;
	}
	else
	{
		*buf_size = static_cast<u32>(*nread);
	}

	ensure(sys_fs_close(ppu, *file_descriptor) == CELL_OK); // Not checked on LLE
	return CELL_OK;
}

error_code cellSysmoduleUnloadModuleInternal(ppu_thread& ppu, u16 id)
{
	// Blocking savestate creation due to ppu_thread::fast_call()
	const std::unique_lock savestate_lock{ g_fxo->get<hle_locks_t>(), std::try_to_lock };

	if (!savestate_lock)
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellSysmodule.notice("cellSysmoduleUnloadModuleInternal(id=0x%x)", id);

	if (id < INTERNAL_MODULE_ID_BASE || id == CELL_SYSMODULE_INVALID || (id & INTERNAL_MODULE_ID_MASK) >= INTERNAL_MODULES_COUNT)
	{
		return CELL_SYSMODULE_ERROR_UNKNOWN;
	}

	if (const error_code ret = unload_module<false>(ppu, INTERNAL_MODULES_OFFSET + (id & INTERNAL_MODULE_ID_MASK)); ret != CELL_OK)
	{
		return ret;
	}

	// Unload dependencies in reverse order
	for (const u16 module_id : MODULE_INFOS[INTERNAL_MODULES_OFFSET + (id & INTERNAL_MODULE_ID_MASK)].dependencies | std::views::reverse)
	{
		if (const error_code ret = module_id < INTERNAL_MODULE_ID_BASE ? cellSysmoduleUnloadModule(ppu, module_id) : cellSysmoduleUnloadModuleInternal(ppu, module_id);
			ret != CELL_OK)
		{
			return ret;
		}
	}

	return CELL_OK;
}

error_code cellSysmoduleLoadModuleInternal(ppu_thread& ppu, u16 id)
{
	// Blocking savestate creation due to ppu_thread::fast_call()
	const std::unique_lock savestate_lock{ g_fxo->get<hle_locks_t>(), std::try_to_lock };

	if (!savestate_lock)
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellSysmodule.notice("cellSysmoduleLoadModuleInternal(id=0x%x)", id);

	if (id < INTERNAL_MODULE_ID_BASE || id == CELL_SYSMODULE_INVALID || (id & INTERNAL_MODULE_ID_MASK) >= INTERNAL_MODULES_COUNT)
	{
		return CELL_SYSMODULE_ERROR_UNKNOWN;
	}

	if (id == 0xf024 || id == 0xf03e || id == 0xf020)
	{
		// TODO replace with proper struct
		const vm::var<char[]> paramsfo(0x40);
		std::memset(paramsfo.get_ptr(), 0, paramsfo.get_count());

		if (ppu_execute<&sys_process_get_paramsfo>(ppu, +paramsfo) != CELL_OK
			|| (!read_from_ptr_unsafe<bf_t<u64, 0, 1>>(paramsfo.get_ptr(), 0x18) && !read_from_ptr_unsafe<bf_t<u64, 0, 1>>(paramsfo.get_ptr(), 0x30)))
		{
			return CELL_SYSMODULE_ERROR_UNKNOWN;
		}
	}

	// Load dependencies
	for (const u16 module_id : MODULE_INFOS[INTERNAL_MODULES_OFFSET + (id & INTERNAL_MODULE_ID_MASK)].dependencies)
	{
		if (const error_code ret = module_id < INTERNAL_MODULE_ID_BASE ? cellSysmoduleLoadModule(ppu, module_id) : cellSysmoduleLoadModuleInternal(ppu, module_id);
			ret != CELL_OK && ret != static_cast<s32>(CELL_SYSMODULE_ERROR_DUPLICATED))
		{
			return ret;
		}
	}

	return load_module(ppu, INTERNAL_MODULES_OFFSET + (id & INTERNAL_MODULE_ID_MASK), 0, vm::null);
}

error_code cellSysmoduleUnloadModuleEx(ppu_thread& ppu, u16 id)
{
	cellSysmodule.notice("cellSysmoduleUnloadModuleEx(id=0x%x)", id);

	if (id >= INTERNAL_MODULE_ID_BASE && (id & INTERNAL_MODULE_ID_MASK) < INTERNAL_MODULES_COUNT
		&& LOADABLE_INTERNAL_MODULES_EX[id & INTERNAL_MODULE_ID_MASK])
	{
		return cellSysmoduleUnloadModuleInternal(ppu, id);
	}

	return cellSysmoduleUnloadModuleInternal(ppu, CELL_SYSMODULE_INVALID);
}

error_code cellSysmoduleLoadModuleEx(ppu_thread& ppu, u16 id)
{
	cellSysmodule.notice("cellSysmoduleLoadModuleEx(id=0x%x)", id);

	if (id >= INTERNAL_MODULE_ID_BASE && (id & INTERNAL_MODULE_ID_MASK) < INTERNAL_MODULES_COUNT
		&& LOADABLE_INTERNAL_MODULES_EX[id & INTERNAL_MODULE_ID_MASK])
	{
		return cellSysmoduleLoadModuleInternal(ppu, id);
	}

	return cellSysmoduleLoadModuleInternal(ppu, CELL_SYSMODULE_INVALID);
}

error_code cellSysmoduleIsLoadedEx(ppu_thread& ppu, u16 id)
{
	// Blocking savestate creation due to ppu_thread::fast_call()
	const std::unique_lock savestate_lock{ g_fxo->get<hle_locks_t>(), std::try_to_lock };

	if (!savestate_lock)
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellSysmodule.notice("cellSysmoduleIsLoadedEx(id=0x%x)", id);

	if (id < INTERNAL_MODULE_ID_BASE || id == CELL_SYSMODULE_INVALID || (id & INTERNAL_MODULE_ID_MASK) >= INTERNAL_MODULES_COUNT)
	{
		return CELL_SYSMODULE_ERROR_UNKNOWN;
	}

	if (ppu_execute<&sys_lwmutex_lock>(ppu, s_sysmodule_context.ptr(&sysmodule_context::mutex), 0) != CELL_OK)
	{
		return CELL_SYSMODULE_ERROR_FATAL;
	}

	if (s_sysmodule_context->module_states[INTERNAL_MODULES_OFFSET + (id & INTERNAL_MODULE_ID_MASK)].loaded_count == 0)
	{
		ensure(ppu_execute<&sys_lwmutex_unlock>(ppu, s_sysmodule_context.ptr(&sysmodule_context::mutex)) == CELL_OK); // Not checked on LLE
		return not_an_error(CELL_SYSMODULE_ERROR_UNLOADED);
	}

	ensure(ppu_execute<&sys_lwmutex_unlock>(ppu, s_sysmodule_context.ptr(&sysmodule_context::mutex)) == CELL_OK); // Not checked on LLE
	return CELL_OK;
}

error_code cellSysmoduleLoadModuleFile(ppu_thread& ppu, vm::cptr<char> path, vm::ptr<SysmoduleUnk> unk)
{
	// Blocking savestate creation due to ppu_thread::fast_call()
	const std::unique_lock savestate_lock{ g_fxo->get<hle_locks_t>(), std::try_to_lock };

	if (!savestate_lock)
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellSysmodule.notice("cellSysmoduleLoadModuleFile(path=*0x%x=%s, unk=*0x%x)", path, path, unk);

	if (ppu_execute<&sys_lwmutex_lock>(ppu, s_sysmodule_context.ptr(&sysmodule_context::mutex), 0) != CELL_OK)
	{
		return CELL_SYSMODULE_ERROR_FATAL;
	}

	ensure(!!unk); // Not checked on LLE

	const error_code ret = ppu_execute<&sys_prx_load_module>(ppu, path, 0, vm::null);
	unk->prx_id = ret;
	unk->unk = -1;

	if (ret < 1)
	{
		ensure(ppu_execute<&sys_lwmutex_unlock>(ppu, s_sysmodule_context.ptr(&sysmodule_context::mutex)) == CELL_OK); // Not checked on LLE
		return CELL_SYSMODULE_ERROR_FATAL;
	}

	const vm::var<char[]> file_name(0x200);
	const vm::var<sys_prx_segment_info_t[]> segments(10);
	const vm::var<sys_prx_module_info_t> prx_module_info
	{{
		.size = sizeof(sys_prx_module_info_t),
		.filename = file_name,
		.filename_size = file_name.get_count(),
		.segments = segments,
		.segments_num = segments.get_count()
	}};

	static_cast<void>(ppu_execute<&sys_prx_get_module_info>(ppu, +unk->prx_id, 0, +prx_module_info));
	// LLE does "for (int i = 0; i < 0; i++) ..." after this, where the name of the prx would have been compared to "cellATRAC3enc_Library" and "cellATRACXenc_Library"

	if (const error_code ret = ppu_execute<&sys_prx_start_module>(ppu, +unk->prx_id, 0, vm::null, +vm::make_var(0), 0, vm::null); ret != CELL_OK)
	{
		ensure(ppu_execute<&sys_prx_unload_module>(ppu, +unk->prx_id, 0, vm::null) == CELL_OK); // Not checked on LLE
		ensure(ppu_execute<&sys_lwmutex_unlock>(ppu, s_sysmodule_context.ptr(&sysmodule_context::mutex)) == CELL_OK); // Not checked on LLE
		return ret;
	}

	ensure(ppu_execute<&sys_lwmutex_unlock>(ppu, s_sysmodule_context.ptr(&sysmodule_context::mutex)) == CELL_OK); // Not checked on LLE
	return CELL_OK;
}

error_code cellSysmoduleUnloadModuleFile(ppu_thread& ppu, vm::cptr<SysmoduleUnk> unk)
{
	// Blocking savestate creation due to ppu_thread::fast_call()
	const std::unique_lock savestate_lock{ g_fxo->get<hle_locks_t>(), std::try_to_lock };

	if (!savestate_lock)
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellSysmodule.notice("cellSysmoduleUnloadModuleFile(unk=*0x%x)", unk);

	if (ppu_execute<&sys_lwmutex_lock>(ppu, s_sysmodule_context.ptr(&sysmodule_context::mutex), 0) != CELL_OK)
	{
		return CELL_SYSMODULE_ERROR_FATAL;
	}

	ensure(!!unk); // Not checked on LLE

	if (ppu_execute<&sys_prx_stop_module>(ppu, +unk->prx_id, 0, vm::null, +vm::make_var(0), 0, vm::null) != CELL_OK
		|| ppu_execute<&sys_prx_unload_module>(ppu, +unk->prx_id, 0, vm::null) != CELL_OK)
	{
		ensure(ppu_execute<&sys_lwmutex_unlock>(ppu, s_sysmodule_context.ptr(&sysmodule_context::mutex)) == CELL_OK); // Not checked on LLE
		return CELL_SYSMODULE_ERROR_FATAL;
	}

	ensure(unk->unk >= -1); // If this is not the case, LLE accesses its equivalent of sysmodule_context::modules_states with a negative index

	ensure(ppu_execute<&sys_lwmutex_unlock>(ppu, s_sysmodule_context.ptr(&sysmodule_context::mutex)) == CELL_OK); // Not checked on LLE
	return CELL_OK;
}

error_code cellSysmoduleSetDebugmode(u32 debug_mode)
{
	cellSysmodule.notice("cellSysmoduleSetDebugmode(debug_mode=%d)", debug_mode);

	if (debug_mode == 1)
	{
		std::memcpy(s_sysmodule_context->module_base_path, MODULE_BASE_PATH_DEBUG.data(), MODULE_BASE_PATH_DEBUG.size() + 1);
		s_sysmodule_context->module_base_path_size = static_cast<u8>(MODULE_BASE_PATH_DEBUG.size());
	}
	else
	{
		std::memcpy(s_sysmodule_context->module_base_path, MODULE_BASE_PATH.data(), MODULE_BASE_PATH.size() + 1);
		s_sysmodule_context->module_base_path_size = static_cast<u8>(MODULE_BASE_PATH.size());
	}

	return CELL_OK;
}

error_code cellSysmoduleSetInternalmode()
{
	cellSysmodule.notice("cellSysmoduleSetInternalmode()");
	// Doesn't do anything
	return CELL_OK;
}

error_code cellSysmodule_0x59521326(vm::ptr<u8> output)
{
	cellSysmodule.notice("cellSysmodule_0x59521326(output=*0x%x)", output);

	const vm::var<CellSsOpenPSID> psid;

	if (!output || sys_ss_get_open_psid(psid) != CELL_OK)
	{
		return -22;
	}

	std::swap(psid->high, psid->low);

	u8 iv[0x10];
	std::memcpy(iv, psid.get_ptr(), sizeof(iv));

	ensure(vm::check_addr(output.addr(), vm::page_readable | vm::page_writable, 0x10));

	aescbc128_decrypt(KEY_UNK.data(), iv, reinterpret_cast<u8*>(psid.get_ptr()), output.get_ptr(), 0x10);

	return CELL_OK;
}

DECLARE(ppu_module_manager::cellSysmodule)("cellSysmodule", []()
{
	REG_VAR(cellSysmodule, s_sysmodule_context).flag(MFF_HIDDEN).init = []
	{
		s_sysmodule_context->unk_addr = 0;
		s_sysmodule_context->use_mem_container = false;
		std::ranges::fill(s_sysmodule_context->module_states, module_state{ .loaded_count = 0, .prx_id = -1 });
		cellSysmoduleSetDebugmode(false);
	};

	REG_FUNC(cellSysmodule, cellSysmoduleInitialize);
	REG_FUNC(cellSysmodule, cellSysmoduleFinalize);
	REG_FUNC(cellSysmodule, cellSysmoduleSetMemcontainer);
	REG_FUNC(cellSysmodule, cellSysmoduleLoadModule);
	REG_FUNC(cellSysmodule, cellSysmoduleUnloadModule);
	REG_FUNC(cellSysmodule, cellSysmoduleIsLoaded);
	REG_FUNC(cellSysmodule, cellSysmoduleGetImagesize);
	REG_FUNC(cellSysmodule, cellSysmoduleFetchImage);
	REG_FUNC(cellSysmodule, cellSysmoduleUnloadModuleInternal);
	REG_FUNC(cellSysmodule, cellSysmoduleLoadModuleInternal);
	REG_FUNC(cellSysmodule, cellSysmoduleUnloadModuleEx);
	REG_FUNC(cellSysmodule, cellSysmoduleLoadModuleEx);
	REG_FUNC(cellSysmodule, cellSysmoduleIsLoadedEx);
	REG_FUNC(cellSysmodule, cellSysmoduleLoadModuleFile);
	REG_FUNC(cellSysmodule, cellSysmoduleUnloadModuleFile);
	REG_FUNC(cellSysmodule, cellSysmoduleSetDebugmode);
	REG_FUNC(cellSysmodule, cellSysmoduleSetInternalmode);
	REG_FNID(cellSysmodule, 0x59521326, cellSysmodule_0x59521326);

	REG_HIDDEN_FUNC(sysmoduleModuleStart);
	REG_HIDDEN_FUNC(sysmoduleModuleStop);
});
