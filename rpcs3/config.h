#pragma once
#include "Utilities/convert.h"
#include "Utilities/config_context.h"

enum class audio_output_type
{
	Null,
	OpenAL,
	XAudio2
};

namespace convert
{
	template<>
	struct to_impl_t<std::string, audio_output_type>
	{
		static std::string func(audio_output_type value)
		{
			switch (value)
			{
			case audio_output_type::Null: return "Null";
			case audio_output_type::OpenAL: return "OpenAL";
			case audio_output_type::XAudio2: return "XAudio2";
			}

			return "Unknown";
		}
	};

	template<>
	struct to_impl_t<audio_output_type, std::string>
	{
		static audio_output_type func(const std::string &value)
		{
			if (value == "Null")
				return audio_output_type::Null;

			if (value == "OpenAL")
				return audio_output_type::OpenAL;

			if (value == "XAudio2")
				return audio_output_type::XAudio2;

			return audio_output_type::Null;
		}
	};
}

enum class rsx_renderer_type
{
	Null,
	OpenGL,
	DX12
};

enum class rsx_aspect_ratio
{
	_4x3 = 1,
	_16x9 
};

enum class rsx_frame_limit
{
	Off,
	_50,
	_59_94,
	_30,
	_60,
	Auto
};

enum class rsx_resolution
{
	_1920x1080 = 1,
	_1280x720 = 2,
	_720x480 = 4,
	_720x576 = 5,
	_1600x1080 = 10,
	_1440x1080 = 11,
	_1280x1080 = 12,
	_960x1080 = 13
};

namespace convert
{
	template<>
	struct to_impl_t<std::string, rsx_renderer_type>
	{
		static std::string func(rsx_renderer_type value)
		{
			switch (value)
			{
			case rsx_renderer_type::Null: return "Null";
			case rsx_renderer_type::OpenGL: return "OpenGL";
			case rsx_renderer_type::DX12: return "DX12";
			}

			return "Unknown";
		}
	};

	template<>
	struct to_impl_t<rsx_renderer_type, std::string>
	{
		static rsx_renderer_type func(const std::string &value)
		{
			if (value == "Null")
				return rsx_renderer_type::Null;

			if (value == "OpenGL")
				return rsx_renderer_type::OpenGL;

			if (value == "DX12")
				return rsx_renderer_type::DX12;

			return rsx_renderer_type::Null;
		}
	};

	template<>
	struct to_impl_t<std::string, rsx_aspect_ratio>
	{
		static std::string func(rsx_aspect_ratio value)
		{
			switch (value)
			{
			case rsx_aspect_ratio::_16x9: return "16x9";
			case rsx_aspect_ratio::_4x3: return "4x3";
			}

			return "Unknown";
		}
	};

	template<>
	struct to_impl_t<rsx_aspect_ratio, std::string>
	{
		static rsx_aspect_ratio func(const std::string &value)
		{
			if (value == "16x9")
				return rsx_aspect_ratio::_16x9;

			if (value == "4x3")
				return rsx_aspect_ratio::_4x3;

			return rsx_aspect_ratio::_16x9;
		}
	};

	template<>
	struct to_impl_t<std::string, rsx_frame_limit>
	{
		static std::string func(rsx_frame_limit value)
		{
			switch (value)
			{
			case rsx_frame_limit::Off: return "Off";
			case rsx_frame_limit::_50: return "50";
			case rsx_frame_limit::_59_94: return "59.94";
			case rsx_frame_limit::_30: return "30";
			case rsx_frame_limit::_60: return "60";
			case rsx_frame_limit::Auto: return "Auto";
			}

			return "Unknown";
		}
	};

	template<>
	struct to_impl_t<rsx_frame_limit, std::string>
	{
		static rsx_frame_limit func(const std::string &value)
		{
			if (value == "Off")
				return rsx_frame_limit::Off;

			if (value == "50")
				return rsx_frame_limit::_50;

			if (value == "59.94")
				return rsx_frame_limit::_59_94;

			if (value == "30")
				return rsx_frame_limit::_30;

			if (value == "60")
				return rsx_frame_limit::_60;

			if (value == "Auto")
				return rsx_frame_limit::Auto;

			return rsx_frame_limit::Off;
		}
	};

	template<>
	struct to_impl_t<std::string, rsx_resolution>
	{
		static std::string func(rsx_resolution value)
		{
			switch (value)
			{
			case rsx_resolution::_1920x1080: return "1920x1080";
			case rsx_resolution::_1280x720: return "1280x720";
			case rsx_resolution::_720x480: return "720x480";
			case rsx_resolution::_720x576: return "720x576";
			case rsx_resolution::_1600x1080: return "1600x1080";
			case rsx_resolution::_1440x1080: return "1440x1080";
			case rsx_resolution::_1280x1080: return "1280x1080";
			case rsx_resolution::_960x1080: return "960x1080";
			}

			return "Unknown";
		}
	};

	template<>
	struct to_impl_t<rsx_resolution, std::string>
	{
		static rsx_resolution func(const std::string &value)
		{
			if (value == "1920x1080")
				return rsx_resolution::_1920x1080;

			if (value == "1280x720")
				return rsx_resolution::_1280x720;

			if (value == "720x480")
				return rsx_resolution::_720x480;

			if (value == "720x576")
				return rsx_resolution::_720x576;

			if (value == "1600x1080")
				return rsx_resolution::_1600x1080;

			if (value == "1440x1080")
				return rsx_resolution::_1440x1080;

			if (value == "1280x1080")
				return rsx_resolution::_1280x1080;

			if (value == "960x1080")
				return rsx_resolution::_960x1080;

			return rsx_resolution::_720x480;
		}
	};
}

enum class ppu_decoder_type
{
	interpreter,
	interpreter2,
	recompiler_llvm
};

namespace convert
{
	template<>
	struct to_impl_t<std::string, ppu_decoder_type>
	{
		static std::string func(ppu_decoder_type value)
		{
			switch (value)
			{
			case ppu_decoder_type::interpreter: return "interpreter";
			case ppu_decoder_type::interpreter2: return "interpreter2";
			case ppu_decoder_type::recompiler_llvm: return "recompiler_llvm";
			}

			return "Unknown";
		}
	};

	template<>
	struct to_impl_t<ppu_decoder_type, std::string>
	{
		static ppu_decoder_type func(const std::string &value)
		{
			if (value == "interpreter")
				return ppu_decoder_type::interpreter;

			if (value == "interpreter2")
				return ppu_decoder_type::interpreter2;

			if (value == "recompiler_llvm")
				return ppu_decoder_type::recompiler_llvm;

			return ppu_decoder_type::interpreter;
		}
	};
}

enum class spu_decoder_type
{
	interpreter_precise,
	interpreter_fast,
	recompiler_asmjit
};

namespace convert
{
	template<>
	struct to_impl_t<std::string, spu_decoder_type>
	{
		static std::string func(spu_decoder_type value)
		{
			switch (value)
			{
			case spu_decoder_type::interpreter_precise: return "interpreter_precise";
			case spu_decoder_type::interpreter_fast: return "interpreter_fast";
			case spu_decoder_type::recompiler_asmjit: return "recompiler_asmjit";
			}

			return "Unknown";
		}
	};

	template<>
	struct to_impl_t<spu_decoder_type, std::string>
	{
		static spu_decoder_type func(const std::string &value)
		{
			if (value == "interpreter_precise")
				return spu_decoder_type::interpreter_precise;

			if (value == "interpreter_fast")
				return spu_decoder_type::interpreter_fast;

			if (value == "recompiler_asmjit")
				return spu_decoder_type::recompiler_asmjit;

			return spu_decoder_type::interpreter_precise;
		}
	};
}

enum class io_camera_state
{
	null,
	connected
};

enum class io_camera_type
{
	unknown,
	eye_toy,
	play_station_eye,
	usb_video_class_1_1
};

enum class io_handler_mode
{
	null,
	windows,
	xinput
};

namespace convert
{
	template<>
	struct to_impl_t<std::string, io_camera_state>
	{
		static std::string func(io_camera_state value)
		{
			switch (value)
			{
			case io_camera_state::null: return "null";
			case io_camera_state::connected: return "connected";
			}

			return "Unknown";
		}
	};

	template<>
	struct to_impl_t<io_camera_state, std::string>
	{
		static io_camera_state func(const std::string &value)
		{
			if (value == "null")
				return io_camera_state::null;

			if (value == "connected")
				return io_camera_state::connected;

			return io_camera_state::null;
		}
	};

	template<>
	struct to_impl_t<std::string, io_camera_type>
	{
		static std::string func(io_camera_type value)
		{
			switch (value)
			{
				case io_camera_type::unknown: return "Unknown";
				case io_camera_type::eye_toy: return "EyeToy";
				case io_camera_type::play_station_eye: return "PlayStationEye";
				case io_camera_type::usb_video_class_1_1: return "UsbVideoClass1.1";
			}
			return "Unknown";
		}
	};

	template<>
	struct to_impl_t<io_camera_type, std::string>
	{
		static io_camera_type func(const std::string &value)
		{
			if (value == "Unknown")
				return io_camera_type::unknown;

			if (value == "EyeToy")
				return io_camera_type::eye_toy;

			if (value == "PlayStationEye")
				return io_camera_type::play_station_eye;

			if (value == "UsbVideoClass1.1")
				return io_camera_type::usb_video_class_1_1;

			return io_camera_type::unknown;
		}
	};

	template<>
	struct to_impl_t<std::string, io_handler_mode>
	{
		static std::string func(io_handler_mode value)
		{
			switch (value)
			{
			case io_handler_mode::null: return "null";
			case io_handler_mode::windows: return "windows";
			case io_handler_mode::xinput: return "xinput";
			}
			return "Unknown";
		}
	};

	template<>
	struct to_impl_t<io_handler_mode, std::string>
	{
		static io_handler_mode func(const std::string &value)
		{
			if (value == "null")
				return io_handler_mode::null;

			if (value == "windows")
				return io_handler_mode::windows;

			if (value == "xinput")
				return io_handler_mode::xinput;

			return io_handler_mode::null;
		}
	};
}

enum class misc_log_level
{
	all,
	warnings,
	success,
	errors,
	nothing
};

enum class misc_net_status
{
	ip_obtained,
	obtaining_ip,
	connecting,
	disconnected
};

namespace convert
{
	template<>
	struct to_impl_t<std::string, misc_log_level>
	{
		static std::string func(misc_log_level value)
		{
			switch (value)
			{
			case misc_log_level::all: return "All";
			case misc_log_level::warnings: return "Warnings";
			case misc_log_level::success: return "Success";
			case misc_log_level::errors: return "Errors";
			case misc_log_level::nothing: return "Nothing";
			}

			return "Unknown";
		}
	};

	template<>
	struct to_impl_t<misc_log_level, std::string>
	{
		static misc_log_level func(const std::string &value)
		{
			if (value == "All")
				return misc_log_level::all;

			if (value == "Warnings")
				return misc_log_level::warnings;

			if (value == "Success")
				return misc_log_level::success;

			if (value == "Errors")
				return misc_log_level::errors;

			if (value == "Nothing")
				return misc_log_level::nothing;

			return misc_log_level::errors;
		}
	};

	template<>
	struct to_impl_t<std::string, misc_net_status>
	{
		static std::string func(misc_net_status value)
		{
			switch (value)
			{
			case misc_net_status::ip_obtained: return "IPObtained";
			case misc_net_status::obtaining_ip: return "ObtainingIP";
			case misc_net_status::connecting: return "Connecting";
			case misc_net_status::disconnected: return "Disconnected";
			}

			return "Unknown";
		}
	};

	template<>
	struct to_impl_t<misc_net_status, std::string>
	{
		static misc_net_status func(const std::string &value)
		{
			if (value == "IPObtained")
				return misc_net_status::ip_obtained;

			if (value == "ObtainingIP")
				return misc_net_status::obtaining_ip;

			if (value == "Connecting")
				return misc_net_status::connecting;

			if (value == "Disconnected")
				return misc_net_status::disconnected;

			return misc_net_status::connecting;
		}
	};
}


namespace rpcs3
{
	class config_t : public config_context_t
	{
		std::string m_path;

	public:
		struct gui_group : protected group
		{
			gui_group(config_context_t *cfg) : group{ cfg, "gui" } {}

			entry<size2i> size{ this, "size",{ 900, 600 } };
			entry<position2i> position{ this, "position",{ -1, -1 } };
			entry<std::string> aui_mgr_perspective{ this, "main_frame_aui", "" };

		} gui{ this };

		struct core_group : protected group
		{
			core_group(config_context_t *cfg) : group{ cfg, "core" } {}

			struct llvm_group : protected group
			{
				llvm_group(group *grp) : group{ grp, "llvm" } {}

				entry<bool> exclusion_range     { this, "Compiled blocks exclusion", false };
				entry<u32> min_id               { this, "Excluded block range min",  200 };
				entry<u32> max_id               { this, "Excluded block range max",  250 };
				entry<u32> threshold            { this, "Compilation threshold",     1000 };

#define MACRO_PPU_INST_MAIN_EXPANDERS(MACRO) \
	/*MACRO(HACK)*/ \
	/*MACRO(TDI)*/ \
	MACRO(TWI) \
	MACRO(MULLI) \
	MACRO(SUBFIC) \
	MACRO(CMPLI) \
	MACRO(CMPI) \
	MACRO(ADDIC) \
	MACRO(ADDIC_) \
	MACRO(ADDI) \
	MACRO(ADDIS) \
	/*MACRO(BC)*/ \
	/*MACRO(SC)*/ \
	/*MACRO(B)*/ \
	MACRO(RLWIMI) \
	MACRO(RLWINM) \
	MACRO(RLWNM) \
	MACRO(ORI) \
	MACRO(ORIS) \
	MACRO(XORI) \
	MACRO(XORIS) \
	MACRO(ANDI_) \
	MACRO(ANDIS_) \
	MACRO(LWZ) \
	/*MACRO(LWZU)*/ \
	MACRO(LBZ) \
	/*MACRO(LBZU)*/ \
	MACRO(STW) \
	/*MACRO(STWU)*/ \
	/*MACRO(STB)*/ \
	/*MACRO(STBU)*/ \
	MACRO(LHZ) \
	/*MACRO(LHZU)*/ \
	/*MACRO(LHA)*/ \
	/*MACRO(LHAU)*/ \
	/*MACRO(STH)*/ \
	/*MACRO(STHU)*/ \
	/*MACRO(LMW)*/ \
	/*MACRO(STMW)*/ \
	MACRO(LFS) \
	/*MACRO(LFSU)*/ \
	/*MACRO(LFD)*/ \
	/*MACRO(LFDU)*/ \
	MACRO(STFS) \
	/*MACRO(STFSU)*/ \
	/*MACRO(STFD)*/ \
	/*MACRO(STFDU)*/ \
	/*MACRO(LFQ)*/ \
	/*MACRO(LFQU)*/

#define MACRO_PPU_INST_G_13_EXPANDERS(MACRO) \
	MACRO(MCRF) \
	/*MACRO(BCLR)*/ \
	MACRO(CRNOR) \
	MACRO(CRANDC) \
	/*MACRO(ISYNC)*/ \
	MACRO(CRXOR) \
	MACRO(CRNAND) \
	MACRO(CRAND) \
	MACRO(CREQV) \
	MACRO(CRORC) \
	MACRO(CROR) \
	/*MACRO(BCCTR)*/

#define MACRO_PPU_INST_G_1E_EXPANDERS(MACRO) \
	MACRO(RLDICL) \
	MACRO(RLDICR) \
	MACRO(RLDIC) \
	MACRO(RLDIMI) \
	MACRO(RLDC_LR)

#define MACRO_PPU_INST_G_1F_EXPANDERS(MACRO) \
	MACRO(CMP) \
	/*MACRO(TW)*/ \
	/*MACRO(LVSL)*/ \
	/*MACRO(LVEBX)*/ \
	/*MACRO(SUBFC)*/ \
	/*MACRO(MULHDU)*/ \
	/*MACRO(ADDC)*/ \
	/*MACRO(MULHWU)*/ \
	MACRO(MFOCRF) \
	MACRO(LWARX) \
	/*MACRO(LDX)*/ \
	/*MACRO(LWZX)*/ \
	/*MACRO(CNTLZW)*/ \
	/*MACRO(SLD)*/ \
	MACRO(AND) \
	MACRO(CMPL) \
	/*MACRO(LVSR)*/ \
	/*MACRO(LVEHX)*/ \
	MACRO(SUBF) \
	/*MACRO(LDUX)*/ \
	/*MACRO(DCBST)*/ \
	/*MACRO(LWZUX)*/ \
	/*MACRO(CNTLZD)*/ \
	/*MACRO(ANDC)*/ \
	/*MACRO(TD)*/ \
	/*MACRO(LVEWX)*/ \
	/*MACRO(MULHD) \
	MACRO(MULHW) \
	MACRO(LDARX) \
	MACRO(DCBF) \
	MACRO(LBZX) \
	MACRO(LVX)*/ \
	MACRO(NEG) \
	/*MACRO(LBZUX) \
	MACRO(NOR) \
	MACRO(STVEBX) \
	MACRO(SUBFE) \
	MACRO(ADDE)*/ \
	MACRO(MTOCRF) \
	/*MACRO(STDX)*/ \
	MACRO(STWCX_) \
	/*MACRO(STWX) \
	MACRO(STVEHX) \
	MACRO(STDUX) \
	MACRO(STWUX) \
	MACRO(STVEWX) \
	MACRO(SUBFZE) \
	MACRO(ADDZE) \
	MACRO(STDCX_) \
	MACRO(STBX) \
	MACRO(STVX) \
	MACRO(SUBFME) \
	MACRO(MULLD) \
	MACRO(ADDME)*/ \
	MACRO(MULLW) \
	/*MACRO(DCBTST) \
	MACRO(STBUX) \
	MACRO(DOZ)*/ \
	MACRO(ADD) \
	/*MACRO(DCBT) \
	MACRO(LHZX) \
	MACRO(EQV) \
	MACRO(ECIWX) \
	MACRO(LHZUX) \
	MACRO(XOR)*/ \
	/*MACRO(MFSPR)*/ \
	/*MACRO(LWAX) \
	MACRO(DST) \
	MACRO(LHAX) \
	MACRO(LVXL) \
	MACRO(MFTB) \
	MACRO(LWAUX) \
	MACRO(DSTST) \
	MACRO(LHAUX) \
	MACRO(STHX) \
	MACRO(ORC) \
	MACRO(ECOWX) \
	MACRO(STHUX)*/ \
	MACRO(OR) \
	/*MACRO(DIVDU)*/ \
	MACRO(DIVWU) \
	/*MACRO(MTSPR)*/ \
	/*MACRO(DCBI) \
	MACRO(NAND) \
	MACRO(STVXL) \
	MACRO(DIVD)*/ \
	MACRO(DIVW) \
	/*MACRO(LVLX) \
	MACRO(SUBFCO) \
	MACRO(ADDCO) \
	MACRO(LDBRX) \
	MACRO(LSWX) \
	MACRO(SRW) \
	MACRO(SRD) \
	MACRO(LVRX) \
	MACRO(SUBFO) \
	MACRO(LFSUX) \
	MACRO(LSWI) \
	MACRO(SYNC) \
	MACRO(LFDX) \
	MACRO(NEGO) \
	MACRO(LFDUX) \
	MACRO(STVLX) \
	MACRO(SUBFEO) \
	MACRO(ADDEO) \
	MACRO(STDBRX) \
	MACRO(STSWX) \
	MACRO(STWBRX) \
	MACRO(STFSX) \
	MACRO(STVRX) \
	MACRO(STFSUX) \
	MACRO(SUBFZEO) \
	MACRO(ADDZEO) \
	MACRO(STSWI) \
	MACRO(STFDX) \
	MACRO(SUBFMEO) \
	MACRO(MULLDO) \
	MACRO(ADDMEO) \
	MACRO(MULLWO) \
	MACRO(STFDUX) \
	MACRO(LVLXL) \
	MACRO(ADDO) \
	MACRO(LHBRX) \
	MACRO(SRAW) \
	MACRO(SRAD) \
	MACRO(LVRXL) \
	MACRO(DSS)*/ \
	MACRO(SRAWI) \
	/*MACRO(SRADI1) \
	MACRO(SRADI2) \
	MACRO(EIEIO) \
	MACRO(STVLXL) \
	MACRO(STHBRX) \
	MACRO(EXTSH) \
	MACRO(STVRXL)*/ \
	MACRO(EXTSB) \
	/*MACRO(DIVDUO) \
	MACRO(DIVWUO) \
	MACRO(STFIWX)*/ \
	MACRO(EXTSW) \
	/*MACRO(ICBI) \
	MACRO(DIVDO) \
	MACRO(DIVWO) \
	MACRO(DCBZ)*/

#define MACRO_PPU_INST_G_3A_EXPANDERS(MACRO) \
	MACRO(LD) \
	/*MACRO(LDU)*/ \
	/*MACRO(LWA)*/

#define MACRO_PPU_INST_G_3E_EXPANDERS(MACRO) \
	MACRO(STD) \
	MACRO(STDU)




#define INSTRUCTION_ENTRY(inst) \
	entry<bool> enable_##inst { this, "Enable instruction " #inst, true};

				MACRO_PPU_INST_MAIN_EXPANDERS(INSTRUCTION_ENTRY)
				MACRO_PPU_INST_G_13_EXPANDERS(INSTRUCTION_ENTRY)
				MACRO_PPU_INST_G_1E_EXPANDERS(INSTRUCTION_ENTRY)
				MACRO_PPU_INST_G_1F_EXPANDERS(INSTRUCTION_ENTRY)
				MACRO_PPU_INST_G_3A_EXPANDERS(INSTRUCTION_ENTRY)
				MACRO_PPU_INST_G_3E_EXPANDERS(INSTRUCTION_ENTRY)

			} llvm{ this };

			entry<ppu_decoder_type> ppu_decoder { this, "PPU Decoder",               ppu_decoder_type::interpreter };
			entry<spu_decoder_type> spu_decoder { this, "SPU Decoder",               spu_decoder_type::interpreter_precise };
			entry<bool> hook_st_func            { this, "Hook static functions",     false };
			entry<bool> load_liblv2             { this, "Load liblv2.sprx",          false };

		} core{ this };

		struct rsx_group : protected group
		{
			struct opengl_group : protected group
			{
				opengl_group(group *grp) : group{ grp, "opengl" } {}

				entry<bool> write_color_buffers { this, "Write Color Buffers", true };
				entry<bool> write_depth_buffer  { this, "Write Depth Buffer",  true };
				entry<bool> read_color_buffers  { this, "Read Color Buffers",  true };
				entry<bool> read_depth_buffer   { this, "Read Depth Buffer",   true };
			} opengl{ this };

			struct d3d12_group : protected group
			{
				d3d12_group(group *grp) : group{ grp, "d3d12" } {}
				
				entry<u32>  adaptater           { this, "D3D Adaptater",       1 };
				entry<bool> debug_output        { this, "Debug Output",        false };
				entry<bool> overlay             { this, "Debug overlay",       false };
			} d3d12{ this };

			rsx_group(config_context_t *cfg) : group{ cfg, "rsx" } {}

			entry<rsx_renderer_type> renderer   { this, "Renderer",            rsx_renderer_type::OpenGL };
			entry<rsx_resolution> resolution    { this, "Resolution",          rsx_resolution::_720x480 };
			entry<rsx_aspect_ratio> aspect_ratio{ this, "Aspect ratio",        rsx_aspect_ratio::_16x9 };
			entry<rsx_frame_limit> frame_limit  { this, "Frame limit",         rsx_frame_limit::Off };
			entry<bool> log_programs            { this, "Log shader programs", false };
			entry<bool> vsync                   { this, "VSync",               false };
			entry<bool> _3dtv                   { this, "3D Monitor",          false };

		} rsx{ this };

		struct audio_group : protected group
		{
			audio_group(config_context_t *cfg) : group{ cfg, "audio" } {}

			entry<audio_output_type> out{ this, "Audio Out",         audio_output_type::OpenAL };
			entry<bool> dump_to_file    { this, "Dump to file",      false };
			entry<bool> convert_to_u16  { this, "Convert to 16 bit", false };
		} audio{ this };

		struct io_group : protected group
		{
			io_group(config_context_t *cfg) : group{ cfg, "io" } {}

			entry<io_camera_state> camera               { this, "Camera",           io_camera_state::connected };
			entry<io_camera_type> camera_type           { this, "Camera type",      io_camera_type::play_station_eye };
			entry<io_handler_mode> pad_handler_mode     { this, "Pad Handler",      io_handler_mode::windows };
			entry<io_handler_mode> keyboard_handler_mode{ this, "Keyboard Handler", io_handler_mode::null };
			entry<io_handler_mode> mouse_handler_mode   { this, "Mouse Handler",    io_handler_mode::null };

			struct pad_group : protected group
			{
				pad_group(group *grp) : group{ grp, "pad" } {}

				entry<int> left_stick_left  { this, "Left Analog Stick Left",     314 };
				entry<int> left_stick_down  { this, "Left Analog Stick Down",     317 };
				entry<int> left_stick_right { this, "Left Analog Stick Right",    316 };
				entry<int> left_stick_up    { this, "Left Analog Stick Up",       315 };
				entry<int> right_stick_left { this, "Right Analog Stick Left",    313 };
				entry<int> right_stick_down { this, "Right Analog Stick Down",    367 };
				entry<int> right_stick_right{ this, "Right Analog Stick Right",   312 };
				entry<int> right_stick_up   { this, "Right Analog Stick Up",      366 };
				entry<int> start            { this, "Start",                       13 };
				entry<int> select           { this, "Select",                      32 };
				entry<int> square           { this, "Square",   static_cast<int>('J') };
				entry<int> cross            { this, "Cross",    static_cast<int>('K') };
				entry<int> circle           { this, "Circle",   static_cast<int>('L') };
				entry<int> triangle         { this, "Triangle", static_cast<int>('I') };
				entry<int> left             { this, "Left",     static_cast<int>('A') };
				entry<int> down             { this, "Down",     static_cast<int>('S') };
				entry<int> right            { this, "Right",    static_cast<int>('D') };
				entry<int> up               { this, "Up",       static_cast<int>('W') };
				entry<int> r1               { this, "R1",       static_cast<int>('3') };
				entry<int> r2               { this, "R2",       static_cast<int>('E') };
				entry<int> r3               { this, "R3",       static_cast<int>('C') };
				entry<int> l1               { this, "L1",       static_cast<int>('1') };
				entry<int> l2               { this, "L2",       static_cast<int>('Q') };
				entry<int> l3               { this, "L3",       static_cast<int>('Z') };
			} pad{ this };

		} io{ this };

		struct misc_group : protected group
		{
			misc_group(config_context_t *cfg) : group{ cfg, "misc" } {}

			struct log_group : protected group
			{
				log_group(group *grp) : group{ grp, "log" } {}

				entry<misc_log_level> level  { this, "Log Level",               misc_log_level::errors };
				entry<bool> hle_logging      { this, "Log everything",          false };
				entry<bool> rsx_logging      { this, "RSX Logging",             false };
				entry<bool> save_tty         { this, "Save TTY output to file", false };
			} log{ this };

			struct net_group : protected group
			{
				net_group(group *grp) : group{ grp, "net" } {}

				entry<misc_net_status> status{ this, "Connection status", misc_net_status::ip_obtained };
				entry<u32> _interface        { this, "Network adapter",   0 };
			} net{ this };

			struct debug_group : protected group
			{
				debug_group(group *grp) : group{ grp, "debug" } {}

				entry<bool> auto_pause_syscall   { this, "Auto Pause at System Call",        false };
				entry<bool> auto_pause_func_call { this, "Auto Pause at Function Call",      false };
			} debug{ this };

			entry<bool> exit_on_stop             { this, "Exit RPCS3 when process finishes", false };
			entry<bool> always_start             { this, "Always start after boot",          true };
			entry<bool> use_default_ini          { this, "Use default configuration",        true };
		} misc{ this };

		struct system_group : protected group
		{
			system_group(config_context_t *cfg) : group{ cfg, "system" } {}

			entry<u32> language                  { this, "Language",                         1 };
			entry<std::string> emulation_dir_path{ this, "Emulation dir path",               "" };
			entry<bool> emulation_dir_path_enable{ this, "Use path below as EmulationDir",   false };
		} system{ this };

		struct vfs_group : public group
		{
			vfs_group(config_context_t *cfg) : group{ cfg, "vfs" } {}
			entry<int> count{ this, "count", 0 };
			entry<int> hdd_count{ this, "hdd_count", 0 };
		} vfs{ this };

		struct lle_group : public group
		{
			lle_group(config_context_t *cfg) : group{ cfg, "lle" } {}
		} lle{ this };

		struct gw_group : public group
		{
			gw_group(config_context_t *cfg) : group{ cfg, "game_viewer" } {}
		} game_viewer{ this };

		config_t() = default;
		config_t(const std::string &path);
		config_t(const config_t& rhs);
		config_t(config_t&& rhs) = delete;

		config_t& operator =(const config_t& rhs);
		config_t& operator =(config_t&& rhs) = delete;

		void path(const std::string &new_path);
		std::string path() const;

		void load();
		void save() const;
	};

	extern config_t config;
}
