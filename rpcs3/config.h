#pragma once
#include "Utilities/convert.h"
#include "Utilities/config_context.h"

enum class audio_output_type
{
	Null,
	OpenAL
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

		return audio_output_type::Null;
	}
};

namespace rpcs3
{
	class config_t : public config_context_t
	{
	public:
		struct rsx_group : protected group
		{
			struct opengl_group : protected group
			{
				opengl_group(group *grp) : group{ grp, "opengl" }
				{
				}

				entry<bool> write_color_buffers{ this, "write_color_buffers", true };
				entry<bool> read_color_buffers{ this, "read_color_buffers", true };
			} opengl{ this };

			rsx_group(config_context_t *cfg) : group{ cfg, "rsx" } {}

			entry<bool> test{ this, "test", false };
		} rsx{ this };

		struct audio_group : protected group
		{
			audio_group(config_context_t *cfg) : group{ cfg, "audio" } {}

			entry<bool> test{ this, "test", false };
		} audio{ this };

		config_t() = default;
		config_t(const std::string &path);
		config_t(const config_t& rhs);
		config_t(config_t&& rhs) = delete;

		config_t& operator =(const config_t& rhs);
		config_t& operator =(config_t&& rhs) = delete;
	};

	extern config_t config;
}