#pragma once

#include "Emu/Io/PadHandler.h"
#include <Windows.h>
#include <mmsystem.h>
#include "Utilities/Config.h"

class mm_joystick_handler final : public PadHandlerBase
{
	// Unique names for the config files and our pad settings dialog
	const std::unordered_map<u64, std::string> button_list =
	{
		{ JOY_BUTTON1 , "Button 1"  },
		{ JOY_BUTTON2 , "Button 2"  },
		{ JOY_BUTTON3 , "Button 3"  },
		{ JOY_BUTTON4 , "Button 4"  },
		{ JOY_BUTTON5 , "Button 5"  },
		{ JOY_BUTTON6 , "Button 6"  },
		{ JOY_BUTTON7 , "Button 7"  },
		{ JOY_BUTTON8 , "Button 8"  },
		{ JOY_BUTTON9 , "Button 9"  },
		{ JOY_BUTTON10, "Button 10" },
		{ JOY_BUTTON11, "Button 11" },
		{ JOY_BUTTON12, "Button 12" },
		{ JOY_BUTTON13, "Button 13" },
		{ JOY_BUTTON14, "Button 14" },
		{ JOY_BUTTON15, "Button 15" },
		{ JOY_BUTTON16, "Button 16" },
		{ JOY_BUTTON17, "Button 17" },
		{ JOY_BUTTON18, "Button 18" },
		{ JOY_BUTTON19, "Button 19" },
		{ JOY_BUTTON20, "Button 20" },
		{ JOY_BUTTON21, "Button 21" },
		{ JOY_BUTTON22, "Button 22" },
		{ JOY_BUTTON23, "Button 23" },
		{ JOY_BUTTON24, "Button 24" },
		{ JOY_BUTTON25, "Button 25" },
		{ JOY_BUTTON26, "Button 26" },
		{ JOY_BUTTON27, "Button 27" },
		{ JOY_BUTTON28, "Button 28" },
		{ JOY_BUTTON29, "Button 29" },
		{ JOY_BUTTON30, "Button 30" },
		{ JOY_BUTTON31, "Button 31" },
		{ JOY_BUTTON32, "Button 32" },
	};

	// Unique names for the config files and our pad settings dialog
	const std::unordered_map<u64, std::string> pov_list =
	{
		{ JOY_POVFORWARD,  "POV Up"    },
		{ JOY_POVRIGHT,    "POV Right" },
		{ JOY_POVBACKWARD, "POV Down"  },
		{ JOY_POVLEFT,     "POV Left"  },
	};

	enum mmjoy_axis
	{
		JOY_XPOS = 9700,
		JOY_XNEG,
		JOY_YPOS,
		JOY_YNEG,
		JOY_ZPOS,
		JOY_ZNEG,
		JOY_RPOS,
		JOY_RNEG,
		JOY_UPOS,
		JOY_UNEG,
		JOY_VPOS,
		JOY_VNEG,
	};

	// Unique names for the config files and our pad settings dialog
	const std::unordered_map<u64, std::string> axis_list =
	{
		{ JOY_XPOS, "X+" },
		{ JOY_XNEG, "X-" },
		{ JOY_YPOS, "Y+" },
		{ JOY_YNEG, "Y-" },
		{ JOY_ZPOS, "Z+" },
		{ JOY_ZNEG, "Z-" },
		{ JOY_RPOS, "R+" },
		{ JOY_RNEG, "R-" },
		{ JOY_UPOS, "U+" },
		{ JOY_UNEG, "U-" },
		{ JOY_VPOS, "V+" },
		{ JOY_VNEG, "V-" },
	};

public:
	mm_joystick_handler();
	~mm_joystick_handler();

	bool Init() override;

	std::vector<std::string> ListDevices() override;
	bool bindPadToDevice(std::shared_ptr<Pad> pad, const std::string& device) override;
	void ThreadProc() override;
	void GetNextButtonPress(const std::string& padId, const std::function<void(u16, std::string, int[])>& callback) override;

private:
	void TranslateButtonPress(u64 keyCode, bool& pressed, u16& val, bool ignore_threshold = false) override;
	std::unordered_map<u64, u16> GetButtonValues();

	bool is_init;
	u32 supportedJoysticks;
	JOYINFOEX js_info;
	JOYCAPS js_caps;

	std::vector<std::shared_ptr<Pad>> bindings;
	std::array<bool, 7> last_connection_status = {};
};
