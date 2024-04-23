#include "stdafx.h"
#include "midi_config_types.h"
#include "Utilities/StrUtil.h"
#include "Utilities/Config.h"

template <>
void fmt_class_string<midi_device_type>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](midi_device_type value)
	{
		switch (value)
		{
		case midi_device_type::guitar: return "Guitar (17 frets)";
		case midi_device_type::guitar_22fret: return "Guitar (22 frets)";
		case midi_device_type::keyboard: return "Keyboard";
		case midi_device_type::drums: return "Drums";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<midi_device>::format(std::string& out, u64 arg)
{
	const midi_device& obj = get_object(arg);
	fmt::append(out, "%sßßß%s", obj.type, obj.name);
}

midi_device midi_device::from_string(const std::string& str)
{
	midi_device res{};

	if (const std::vector<std::string> parts = fmt::split(str, {"ßßß"}); !parts.empty())
	{
		u64 result;

		if (cfg::try_to_enum_value(&result, &fmt_class_string<midi_device_type>::format, ::at32(parts, 0)))
		{
			res.type = static_cast<midi_device_type>(static_cast<std::underlying_type_t<midi_device_type>>(result));
		}

		if (parts.size() == 2)
		{
			res.name = ::at32(parts, 1);
		}
	}

	return res;
}
