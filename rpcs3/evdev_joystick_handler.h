#pragma once

#include "Utilities/types.h"
#include "Utilities/Config.h"
#include "Utilities/File.h"
#include "Emu/Io/PadHandler.h"
#include <libevdev/libevdev.h>
#include <vector>
#include <thread>

class evdev_joystick_handler final : public PadHandlerBase
{
	// These are all the non-key standard buttons provided by evdev
	enum EvdevKeyCodes
	{
		BTN_0        = 0x100,
		BTN_1        = 0x101,
		BTN_2        = 0x102,
		BTN_3        = 0x103,
		BTN_4        = 0x104,
		BTN_5        = 0x105,
		BTN_6        = 0x106,
		BTN_7        = 0x107,
		BTN_8        = 0x108,
		BTN_9        = 0x109,
		BTN_MOUSE    = 0x110,
		BTN_LEFT     = 0x110,
		BTN_RIGHT    = 0x111,
		BTN_MIDDLE   = 0x112,
		BTN_SIDE     = 0x113,
		BTN_EXTRA    = 0x114,
		BTN_FORWARD  = 0x115,
		BTN_BACK     = 0x116,
		BTN_JOYSTICK = 0x120,
		BTN_TRIGGER  = 0x120,
		BTN_THUMB    = 0x121,
		BTN_THUMB2   = 0x122,
		BTN_TOP      = 0x123,
		BTN_TOP2     = 0x124,
		BTN_PINKIE   = 0x125,
		BTN_BASE     = 0x126,
		BTN_BASE2    = 0x127,
		BTN_BASE3    = 0x128,
		BTN_BASE4    = 0x129,
		BTN_BASE5    = 0x12a,
		BTN_BASE6    = 0x12b,
		BTN_GAMEPAD	 = 0x130,
		BTN_A        = 0x130,
		BTN_B        = 0x131,
		BTN_C        = 0x132,
		BTN_X        = 0x133,
		BTN_Y        = 0x134,
		BTN_Z        = 0x135,
		BTN_TL       = 0x136,
		BTN_TR       = 0x137,
		BTN_TL2      = 0x138,
		BTN_TR2      = 0x139,
		BTN_SELECT   = 0x13a,
		BTN_START    = 0x13b,
		BTN_MODE     = 0x13c,
		KEY_MAX      = 0x13d
	};

	// These are all the standard axis provided by evdev
	enum EvdevAxisCodes
	{
		ABS_X        = 0x00,
		ABS_Y        = 0x01,
		ABS_Z        = 0x02,
		ABS_RX       = 0x03,
		ABS_RY       = 0x04,
		ABS_RZ       = 0x05,
		ABS_THROTTLE = 0x06,
		ABS_RUDDER   = 0x07,
		ABS_WHEEL    = 0x08,
		ABS_GAS      = 0x09,
		ABS_BRAKE    = 0x0a,
		ABS_HAT0X    = 0x10,
		ABS_HAT0Y    = 0x11,
		ABS_HAT1X    = 0x12,
		ABS_HAT1Y    = 0x13,
		ABS_HAT2X    = 0x14,
		ABS_HAT2Y    = 0x15,
		ABS_HAT3X    = 0x16,
		ABS_HAT3Y    = 0x17,
		ABS_PRESSURE = 0x18,
		ABS_DISTANCE = 0x19,
		ABS_TILT_X   = 0x1a,
		ABS_TILT_Y   = 0x1b,
		ABS_MISC     = 0x1c,
		ABS_MAX      = 0x1f
	};

	// Unique button names for the config files and our pad settings dialog
	const std::unordered_map<u32, std::string> button_list =
	{
		{ EvdevKeyCodes::BTN_0       , "0"        },
		{ EvdevKeyCodes::BTN_1       , "1"        },
		{ EvdevKeyCodes::BTN_2       , "2"        },
		{ EvdevKeyCodes::BTN_3       , "3"        },
		{ EvdevKeyCodes::BTN_4       , "4"        },
		{ EvdevKeyCodes::BTN_5       , "5"        },
		{ EvdevKeyCodes::BTN_6       , "6"        },
		{ EvdevKeyCodes::BTN_7       , "7"        },
		{ EvdevKeyCodes::BTN_8       , "8"        },
		{ EvdevKeyCodes::BTN_9       , "9"        },
		{ EvdevKeyCodes::BTN_MOUSE   , "Mouse"    },
		{ EvdevKeyCodes::BTN_LEFT    , "Left"     },
		{ EvdevKeyCodes::BTN_RIGHT   , "Right"    },
		{ EvdevKeyCodes::BTN_MIDDLE  , "Middle"   },
		{ EvdevKeyCodes::BTN_SIDE    , "Side"     },
		{ EvdevKeyCodes::BTN_EXTRA   , "Extra"    },
		{ EvdevKeyCodes::BTN_FORWARD , "Forward"  },
		{ EvdevKeyCodes::BTN_BACK    , "Back"     },
		{ EvdevKeyCodes::BTN_JOYSTICK, "Joystick" },
		{ EvdevKeyCodes::BTN_TRIGGER , "Trgigger" },
		{ EvdevKeyCodes::BTN_THUMB   , "Thumb"    },
		{ EvdevKeyCodes::BTN_THUMB2  , "Thumb 2"  },
		{ EvdevKeyCodes::BTN_TOP     , "Top"      },
		{ EvdevKeyCodes::BTN_TOP2    , "Top 2"    },
		{ EvdevKeyCodes::BTN_PINKIE  , "Pinkie"   },
		{ EvdevKeyCodes::BTN_BASE    , "Base"     },
		{ EvdevKeyCodes::BTN_BASE2   , "Base 2"   },
		{ EvdevKeyCodes::BTN_BASE3   , "Base 3"   },
		{ EvdevKeyCodes::BTN_BASE4   , "Base 4"   },
		{ EvdevKeyCodes::BTN_BASE5   , "Base 5"   },
		{ EvdevKeyCodes::BTN_BASE6   , "Base 6"   },
		{ EvdevKeyCodes::BTN_GAMEPAD , "Gamepad"  },
		{ EvdevKeyCodes::BTN_A       , "A"        },
		{ EvdevKeyCodes::BTN_B       , "B"        },
		{ EvdevKeyCodes::BTN_C       , "C"        },
		{ EvdevKeyCodes::BTN_X       , "X"        },
		{ EvdevKeyCodes::BTN_Y       , "Y"        },
		{ EvdevKeyCodes::BTN_Z       , "Z"        },
		{ EvdevKeyCodes::BTN_TL      , "TL"       },
		{ EvdevKeyCodes::BTN_TR      , "TR"       },
		{ EvdevKeyCodes::BTN_TL2     , "TL 2"     },
		{ EvdevKeyCodes::BTN_TR2     , "TR 2"     },
		{ EvdevKeyCodes::BTN_SELECT  , "Select"   },
		{ EvdevKeyCodes::BTN_START   , "Start"    },
		{ EvdevKeyCodes::BTN_MODE    , "Mode"     }
	};

	// Unique positive axis names for the config files and our pad settings dialog
	const std::unordered_map<u32, std::string> axis_list =
	{
		{ EvdevAxisCodes::ABS_X        , "LX +"       },
		{ EvdevAxisCodes::ABS_Y        , "LY +"       },
		{ EvdevAxisCodes::ABS_Z        , "LZ +"       },
		{ EvdevAxisCodes::ABS_RX       , "RX +"       },
		{ EvdevAxisCodes::ABS_RY       , "RY +"       },
		{ EvdevAxisCodes::ABS_RZ       , "RZ +"       },
		{ EvdevAxisCodes::ABS_THROTTLE , "Throttle +" },
		{ EvdevAxisCodes::ABS_RUDDER   , "Rudder +"   },
		{ EvdevAxisCodes::ABS_WHEEL    , "Wheel +"    },
		{ EvdevAxisCodes::ABS_GAS      , "Gas +"      },
		{ EvdevAxisCodes::ABS_BRAKE    , "Brake +"    },
		{ EvdevAxisCodes::ABS_HAT0X    , "Hat0 X +"   },
		{ EvdevAxisCodes::ABS_HAT0Y    , "Hat0 Y +"   },
		{ EvdevAxisCodes::ABS_HAT1X    , "Hat1 X +"   },
		{ EvdevAxisCodes::ABS_HAT1Y    , "Hat1 Y +"   },
		{ EvdevAxisCodes::ABS_HAT2X    , "Hat2 X +"   },
		{ EvdevAxisCodes::ABS_HAT2Y    , "Hat2 Y +"   },
		{ EvdevAxisCodes::ABS_HAT3X    , "Hat3 X +"   },
		{ EvdevAxisCodes::ABS_HAT3Y    , "Hat3 Y +"   },
		{ EvdevAxisCodes::ABS_PRESSURE , "Pressure +" },
		{ EvdevAxisCodes::ABS_DISTANCE , "Distance +" },
		{ EvdevAxisCodes::ABS_TILT_X   , "Tilt X +"   },
		{ EvdevAxisCodes::ABS_TILT_Y   , "Tilt Y +"   },
		{ EvdevAxisCodes::ABS_MISC     , "Misc +"     }
	};

	// Unique negative axis names for the config files and our pad settings dialog
	const std::unordered_map<u32, std::string> rev_axis_list =
	{
		{ EvdevAxisCodes::ABS_X        , "LX -"       },
		{ EvdevAxisCodes::ABS_Y        , "LY -"       },
		{ EvdevAxisCodes::ABS_Z        , "LZ -"       },
		{ EvdevAxisCodes::ABS_RX       , "RX -"       },
		{ EvdevAxisCodes::ABS_RY       , "RY -"       },
		{ EvdevAxisCodes::ABS_RZ       , "RZ -"       },
		{ EvdevAxisCodes::ABS_THROTTLE , "Throttle -" },
		{ EvdevAxisCodes::ABS_RUDDER   , "Rudder -"   },
		{ EvdevAxisCodes::ABS_WHEEL    , "Wheel -"    },
		{ EvdevAxisCodes::ABS_GAS      , "Gas -"      },
		{ EvdevAxisCodes::ABS_BRAKE    , "Brake -"    },
		{ EvdevAxisCodes::ABS_HAT0X    , "Hat0 X -"   },
		{ EvdevAxisCodes::ABS_HAT0Y    , "Hat0 Y -"   },
		{ EvdevAxisCodes::ABS_HAT1X    , "Hat1 X -"   },
		{ EvdevAxisCodes::ABS_HAT1Y    , "Hat1 Y -"   },
		{ EvdevAxisCodes::ABS_HAT2X    , "Hat2 X -"   },
		{ EvdevAxisCodes::ABS_HAT2Y    , "Hat2 Y -"   },
		{ EvdevAxisCodes::ABS_HAT3X    , "Hat3 X -"   },
		{ EvdevAxisCodes::ABS_HAT3Y    , "Hat3 Y -"   },
		{ EvdevAxisCodes::ABS_PRESSURE , "Pressure -" },
		{ EvdevAxisCodes::ABS_DISTANCE , "Distance -" },
		{ EvdevAxisCodes::ABS_TILT_X   , "Tilt X -"   },
		{ EvdevAxisCodes::ABS_TILT_Y   , "Tilt Y -"   },
		{ EvdevAxisCodes::ABS_MISC     , "Misc -"     }
	};

public:
	evdev_joystick_handler();
	~evdev_joystick_handler();

	bool Init() override;
	std::vector<std::string> ListDevices() override;
	bool bindPadToDevice(std::shared_ptr<Pad> pad, const std::string& device) override;
	void ThreadProc() override;
	void Close();
	void ConfigController(const std::string& device) override;
	void GetNextButtonPress(const std::string& padid, const std::function<void(std::string)>& callback) override;
	void TestVibration(const std::string& padId, u32 largeMotor, u32 smallMotor) override;
	void TranslateButtonPress(u32 keyCode, bool& pressed, u16& value, bool ignore_threshold = false) override;

private:
	void update_devs();
	int GetButtonInfo(const input_event& evt, int device_number, int& button_code, bool& is_negative);

	std::vector<std::string> joy_paths;
	std::vector<std::shared_ptr<Pad>> pads;
	std::vector<libevdev*> joy_devs;
};
