#pragma once
#include "Utilities/convert.h"
#include "Utilities/config_context.h"

enum class audio_output_type
{
	Null,
	OpenAL,
	XAudio2
};

template<>
struct convert::to_impl_t<std::string, audio_output_type>
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
struct convert::to_impl_t<audio_output_type, std::string>
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

enum class rsx_renderer_type
{
	Null,
	OpenGL,
	DX12
};

template<>
struct convert::to_impl_t<std::string, rsx_renderer_type>
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
struct convert::to_impl_t<rsx_renderer_type, std::string>
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

enum class ppu_decoder_type
{
	interpreter,
	interpreter2,
	recompiler_llvm
};

template<>
struct convert::to_impl_t<std::string, ppu_decoder_type>
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
struct convert::to_impl_t<ppu_decoder_type, std::string>
{
	static ppu_decoder_type func(const std::string &value)
	{
		if (value == "interpreter")
			return ppu_decoder_type::interpreter;

		if (value == "interpreter2")
			return ppu_decoder_type::interpreter2;

		if (value == "DX12")
			return ppu_decoder_type::recompiler_llvm;

		return ppu_decoder_type::interpreter;
	}
};


enum class spu_decoder_type
{
	interpreter_precise,
	interpreter_fast,
	recompiler_asmjit
};

template<>
struct convert::to_impl_t<std::string, spu_decoder_type>
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
struct convert::to_impl_t<spu_decoder_type, std::string>
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

namespace rpcs3
{
	class config_t : public config_context_t
	{
	public:
		struct core_group : protected group
		{
			core_group(config_context_t *cfg) : group{ cfg, "core" } {}

			entry<ppu_decoder_type> ppu_decoder{ this, "PPU Decoder", ppu_decoder_type::interpreter };
			entry<spu_decoder_type> spu_decoder{ this, "SPU Decoder", spu_decoder_type::interpreter_precise };
			entry<bool> load_liblv2{ this, "Load liblv2.sprx", false };
		} core{ this };

		struct rsx_group : protected group
		{
			struct opengl_group : protected group
			{
				opengl_group(group *grp) : group{ grp, "opengl" } {}

				entry<bool> write_color_buffers{ this, "Write Color Buffers", true };
				entry<bool> write_depth_buffer{ this, "Write Depth Buffer", true };
				entry<bool> read_color_buffers{ this, "Read Color Buffers", true };
				entry<bool> read_depth_buffer{ this, "Read Depth Buffer", true };
			} opengl{ this };

			rsx_group(config_context_t *cfg) : group{ cfg, "rsx" } {}

			entry<rsx_renderer_type> renderer{ this, "Renderer", rsx_renderer_type::OpenGL };
		} rsx{ this };

		struct audio_group : protected group
		{
			audio_group(config_context_t *cfg) : group{ cfg, "audio" } {}

			entry<audio_output_type> test{ this, "Audio Out", audio_output_type::OpenAL };
		} audio{ this };

		struct io_group : protected group
		{
			io_group(config_context_t *cfg) : group{ cfg, "io" } {}
		} io{ this };

		struct misc_group : protected group
		{
			misc_group(config_context_t *cfg) : group{ cfg, "misc" } {}
		} misc{ this };

		struct system_group : protected group
		{
			system_group(config_context_t *cfg) : group{ cfg, "system" } {}
		} system{ this };

		config_t() = default;
		config_t(const std::string &path);
		config_t(const config_t& rhs);
		config_t(config_t&& rhs) = delete;

		config_t& operator =(const config_t& rhs);
		config_t& operator =(config_t&& rhs) = delete;
	};

	extern config_t config;
}