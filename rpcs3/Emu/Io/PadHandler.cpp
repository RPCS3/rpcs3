#include "stdafx.h"
#include "PadHandler.h"

cfg_input g_cfg_input;

PadHandlerBase::PadHandlerBase(pad_handler type) : m_type(type)
{
}

// Search an unordered map for a string value and return found keycode
int PadHandlerBase::FindKeyCode(std::unordered_map<u32, std::string> map, const cfg::string& name, bool fallback)
{
	std::string def = name.def;
	std::string nam = name.to_string();
	int def_code = -1;

	for (auto it = map.begin(); it != map.end(); ++it)
	{
		if (it->second == nam)
			return it->first;

		if (fallback && it->second == def)
			def_code = it->first;
	}

	if (fallback)
	{
		LOG_ERROR(HLE, "int FindKeyCode for [name = %s] returned with [def_code = %d] for [def = %s]", nam, def_code, def);
		if (def_code < 0)
			def_code = 0;
	}

	return def_code;
}

long PadHandlerBase::FindKeyCode(std::unordered_map<u64, std::string> map, const cfg::string& name, bool fallback)
{
	std::string def = name.def;
	std::string nam = name.to_string();
	long def_code = -1;

	for (auto it = map.begin(); it != map.end(); ++it)
	{
		if (it->second == nam)
			return static_cast<long>(it->first);

		if (fallback && it->second == def)
			def_code = static_cast<long>(it->first);
	}

	if (fallback)
	{
		LOG_ERROR(HLE, "long FindKeyCode for [name = %s] returned with [def_code = %d] for [def = %s]", nam, def_code, def);
		if (def_code < 0)
			def_code = 0;
	}

	return def_code;
}

// Search an unordered map for a string value and return found keycode
int PadHandlerBase::FindKeyCodeByString(std::unordered_map<u32, std::string> map, const std::string& name, bool fallback)
{
	for (auto it = map.begin(); it != map.end(); ++it)
	{
		if (it->second == name)
			return it->first;
	}

	if (fallback)
	{
		LOG_ERROR(HLE, "long FindKeyCodeByString fohr [name = %s] returned with 0", name);
		return 0;
	}

	return -1;
}

// Search an unordered map for a string value and return found keycode
long PadHandlerBase::FindKeyCodeByString(std::unordered_map<u64, std::string> map, const std::string& name, bool fallback)
{
	for (auto it = map.begin(); it != map.end(); ++it)
	{
		if (it->second == name)
			return static_cast<long>(it->first);
	}

	if (fallback)
	{
		LOG_ERROR(HLE, "long FindKeyCodeByString fohr [name = %s] returned with 0", name);
		return 0;
	}

	return -1;
}

// Get new scaled value between 0 and 255 based on its minimum and maximum
float PadHandlerBase::ScaleStickInput(s32 raw_value, int minimum, int maximum)
{
	// value based on max range converted to [0, 1]
	float val = float(Clamp(static_cast<f32>(raw_value), minimum, maximum) - minimum) / float(abs(maximum) + abs(minimum));
	return 255.0f * val;
}

// Get new scaled value between -255 and 255 based on its minimum and maximum
float PadHandlerBase::ScaleStickInput2(s32 raw_value, int minimum, int maximum)
{
	// value based on max range converted to [0, 1]
	float val = float(Clamp(static_cast<f32>(raw_value), minimum, maximum) - minimum) / float(abs(maximum) + abs(minimum));
	return (510.0f * val) - 255.0f;
}

// Get normalized trigger value based on the range defined by a threshold
u16 PadHandlerBase::NormalizeTriggerInput(u16 value, int threshold)
{
	if (value <= threshold || threshold >= trigger_max)
	{
		return static_cast<u16>(0);
	}
	else if (threshold <= trigger_min)
	{
		return value;
	}
	else
	{
		return (u16)(float(trigger_max) * float(value - threshold) / float(trigger_max - threshold));
	}
}

// normalizes a directed input, meaning it will correspond to a single "button" and not an axis with two directions
// the input values must lie in 0+
u16 PadHandlerBase::NormalizeDirectedInput(u16 raw_value, s32 threshold, s32 maximum)
{
	if (threshold >= maximum || maximum <= 0)
	{
		return static_cast<u16>(0);
	}

	float val = float(Clamp(raw_value, 0, maximum)) / float(maximum); // value based on max range converted to [0, 1]

	if (threshold <= 0)
	{
		return static_cast<u16>(255.0f * val);
	}
	else
	{
		float thresh = float(threshold) / float(maximum); // threshold converted to [0, 1]
		return static_cast<u16>(255.0f * std::min(1.0f, (val - thresh) / (1.0f - thresh)));
	}
}

u16 PadHandlerBase::NormalizeStickInput(u16 raw_value, int threshold, bool ignore_threshold)
{
	if (ignore_threshold)
	{
		return static_cast<u16>(ScaleStickInput(raw_value, 0, thumb_max));
	}
	else
	{
		return NormalizeDirectedInput(raw_value, threshold, thumb_max);
	}
}

// This function normalizes stick deadzone based on the DS3's deadzone, which is ~13%
// X and Y is expected to be in (-255) to 255 range, deadzone should be in terms of thumb stick range
// return is new x and y values in 0-255 range
std::tuple<u16, u16> PadHandlerBase::NormalizeStickDeadzone(s32 inX, s32 inY, u32 deadzone)
{
	const float dzRange = deadzone / float((std::abs(thumb_max) + std::abs(thumb_min)));

	float X = inX / 255.0f;
	float Y = inY / 255.0f;

	if (dzRange > 0.f)
	{
		const float mag = std::min(sqrtf(X*X + Y*Y), 1.f);

		if (mag <= 0)
		{
			return std::tuple<u16, u16>(ConvertAxis(X), ConvertAxis(Y));
		}

		if (mag > dzRange) {
			float pos = lerp(0.13f, 1.f, (mag - dzRange) / (1 - dzRange));
			float scale = pos / mag;
			X = X * scale;
			Y = Y * scale;
		}
		else {
			float pos = lerp(0.f, 0.13f, mag / dzRange);
			float scale = pos / mag;
			X = X * scale;
			Y = Y * scale;
		}
	}
	return std::tuple<u16, u16>(ConvertAxis(X), ConvertAxis(Y));
}

// get clamped value between min and max
s32 PadHandlerBase::Clamp(f32 input, s32 min, s32 max)
{
	if (input > max)
		return max;
	else if (input < min)
		return min;
	else return static_cast<s32>(input);
}

// get clamped value between 0 and 255
u16 PadHandlerBase::Clamp0To255(f32 input)
{
	return static_cast<u16>(Clamp(input, 0, 255));
}

// get clamped value between 0 and 1023
u16 PadHandlerBase::Clamp0To1023(f32 input)
{
	return static_cast<u16>(Clamp(input, 0, 1023));
}

// input has to be [-1,1]. result will be [0,255]
u16 PadHandlerBase::ConvertAxis(float value)
{
	return static_cast<u16>((value + 1.0)*(255.0 / 2.0));
}

// The DS3, (and i think xbox controllers) give a 'square-ish' type response, so that the corners will give (almost)max x/y instead of the ~30x30 from a perfect circle
// using a simple scale/sensitivity increase would *work* although it eats a chunk of our usable range in exchange
// this might be the best for now, in practice it seems to push the corners to max of 20x20, with a squircle_factor of 8000
// This function assumes inX and inY is already in 0-255
std::tuple<u16, u16> PadHandlerBase::ConvertToSquirclePoint(u16 inX, u16 inY, int squircle_factor)
{
	// convert inX and Y to a (-1, 1) vector;
	const f32 x = ((f32)inX - 127.5f) / 127.5f;
	const f32 y = ((f32)inY - 127.5f) / 127.5f;

	// compute angle and len of given point to be used for squircle radius
	const f32 angle = std::atan2(y, x);
	const f32 r = std::sqrt(std::pow(x, 2.f) + std::pow(y, 2.f));

	// now find len/point on the given squircle from our current angle and radius in polar coords
	// https://thatsmaths.com/2016/07/14/squircles/
	const f32 newLen = (1 + std::pow(std::sin(2 * angle), 2.f) / (float(squircle_factor) / 1000.f)) * r;

	// we now have len and angle, convert to cartesian
	const int newX = Clamp0To255(((newLen * std::cos(angle)) + 1) * 127.5f);
	const int newY = Clamp0To255(((newLen * std::sin(angle)) + 1) * 127.5f);
	return std::tuple<u16, u16>(newX, newY);
}

std::string PadHandlerBase::name_string()
{
	return m_name_string;
}

int PadHandlerBase::max_devices()
{
	return m_max_devices;
}

bool PadHandlerBase::has_config()
{
	return b_has_config;
}

bool PadHandlerBase::has_rumble()
{
	return b_has_rumble;
}

bool PadHandlerBase::has_deadzones()
{
	return b_has_deadzones;
}

std::string PadHandlerBase::get_config_dir(pad_handler type)
{
	return fs::get_config_dir() + "/InputConfigs/" + fmt::format("%s", type) + "/";
}

std::string PadHandlerBase::get_config_filename(int i)
{
	return fs::get_config_dir() + "/InputConfigs/" + g_cfg_input.player[i]->handler.to_string() + "/" + g_cfg_input.player[i]->profile.to_string() + ".yml";
}

void PadHandlerBase::init_configs()
{
	int index = 0;

	for (int i = 0; i < MAX_GAMEPADS; i++)
	{
		if (g_cfg_input.player[i]->handler == m_type)
		{
			init_config(&m_pad_configs[index], get_config_filename(i));
			index++;
		}
	}
}
