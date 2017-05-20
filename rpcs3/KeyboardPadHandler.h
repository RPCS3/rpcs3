#pragma once

#include "Utilities/Config.h"
#include "Emu/Io/PadHandler.h"

struct KeyboardPadConfig final : cfg::node
{
	const std::string cfg_name = fs::get_config_dir() + "/config_kbpad.yml";

	cfg::int32 left_stick_left{this, "Left Analog Stick Left", static_cast<int>('A')};
	cfg::int32 left_stick_down{this, "Left Analog Stick Down", static_cast<int>('S')};
	cfg::int32 left_stick_right{this, "Left Analog Stick Right", static_cast<int>('D')};
	cfg::int32 left_stick_up{this, "Left Analog Stick Up", static_cast<int>('W')};
	cfg::int32 right_stick_left{this, "Right Analog Stick Left", 313};
	cfg::int32 right_stick_down{this, "Right Analog Stick Down", 367};
	cfg::int32 right_stick_right{this, "Right Analog Stick Right", 312};
	cfg::int32 right_stick_up{this, "Right Analog Stick Up", 366};
	cfg::int32 start{this, "Start", 13};
	cfg::int32 select{this, "Select", 32};
	cfg::int32 square{this, "Square", static_cast<int>('Z')};
	cfg::int32 cross{this, "Cross", static_cast<int>('X')};
	cfg::int32 circle{this, "Circle", static_cast<int>('C')};
	cfg::int32 triangle{this, "Triangle", static_cast<int>('V')};
	cfg::int32 left{this, "Left", 314};
	cfg::int32 down{this, "Down", 317};
	cfg::int32 right{this, "Right", 316};
	cfg::int32 up{this, "Up", 315};
	cfg::int32 r1{this, "R1", static_cast<int>('E')};
	cfg::int32 r2{this, "R2", static_cast<int>('T')};
	cfg::int32 r3{this, "R3", static_cast<int>('G')};
	cfg::int32 l1{this, "L1", static_cast<int>('Q')};
	cfg::int32 l2{this, "L2", static_cast<int>('R')};
	cfg::int32 l3{this, "L3", static_cast<int>('F')};

	bool load()
	{
		if (fs::file cfg_file{ cfg_name, fs::read })
		{
			return from_string(cfg_file.to_string());
		}

		return false;
	}

	void save()
	{
		fs::file(cfg_name, fs::rewrite).write(to_string());
	}
};

class KeyboardPadHandler final : public PadHandlerBase, public wxWindow
{
public:
	virtual void Init(const u32 max_connect) override;

	KeyboardPadHandler();

	void KeyDown(wxKeyEvent& event);
	void KeyUp(wxKeyEvent& event);
	void LoadSettings();
};
