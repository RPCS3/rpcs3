#pragma once

#include "Utilities/types.h"
#include "Utilities/Config.h"
#include "Utilities/File.h"
#include "Emu/Io/PadHandler.h"
#include <libevdev/libevdev.h>
#include <vector>
#include <thread>
#include <ctime>

struct positive_axis : cfg::node
{
	const std::string cfg_name = fs::get_config_dir() + "/evdev_positive_axis.yml";

	cfg::_bool abs_x{ this, "ABS_X", false };
	cfg::_bool abs_y{ this, "ABS_Y", false };
	cfg::_bool abs_z{ this, "ABS_Z", false };
	cfg::_bool abs_rx{ this, "ABS_RX", false };
	cfg::_bool abs_ry{ this, "ABS_RY", false };
	cfg::_bool abs_rz{ this, "ABS_RZ", false };
	cfg::_bool abs_throttle{ this, "ABS_THROTTLE", false };
	cfg::_bool abs_rudder{ this, "ABS_RUDDER", false };
	cfg::_bool abs_wheel{ this, "ABS_WHEEL", false };
	cfg::_bool abs_gas{ this, "ABS_GAS", false };
	cfg::_bool abs_brake{ this, "ABS_BRAKE", false };
	cfg::_bool abs_hat0x{ this, "ABS_HAT0X", false };
	cfg::_bool abs_hat0y{ this, "ABS_HAT0Y", false };
	cfg::_bool abs_hat1x{ this, "ABS_HAT1X", false };
	cfg::_bool abs_hat1y{ this, "ABS_HAT1Y", false };
	cfg::_bool abs_hat2x{ this, "ABS_HAT2X", false };
	cfg::_bool abs_hat2y{ this, "ABS_HAT2Y", false };
	cfg::_bool abs_hat3x{ this, "ABS_HAT3X", false };
	cfg::_bool abs_hat3y{ this, "ABS_HAT3Y", false };
	cfg::_bool abs_pressure{ this, "ABS_PRESSURE", false };
	cfg::_bool abs_distance{ this, "ABS_DISTANCE", false };
	cfg::_bool abs_tilt_x{ this, "ABS_TILT_X", false };
	cfg::_bool abs_tilt_y{ this, "ABS_TILT_Y", false };
	cfg::_bool abs_tool_width{ this, "ABS_TOOL_WIDTH", false };
	cfg::_bool abs_volume{ this, "ABS_VOLUME", false };
	cfg::_bool abs_misc{ this, "ABS_MISC", false };
	cfg::_bool abs_mt_slot{ this, "ABS_MT_SLOT", false };
	cfg::_bool abs_mt_touch_major{ this, "ABS_MT_TOUCH_MAJOR", false };
	cfg::_bool abs_mt_touch_minor{ this, "ABS_MT_TOUCH_MINOR", false };
	cfg::_bool abs_mt_width_major{ this, "ABS_MT_WIDTH_MAJOR", false };
	cfg::_bool abs_mt_width_minor{ this, "ABS_MT_WIDTH_MINOR", false };
	cfg::_bool abs_mt_orientation{ this, "ABS_MT_ORIENTATION", false };
	cfg::_bool abs_mt_position_x{ this, "ABS_MT_POSITION_X", false };
	cfg::_bool abs_mt_position_y{ this, "ABS_MT_POSITION_Y", false };
	cfg::_bool abs_mt_tool_type{ this, "ABS_MT_TOOL_TYPE", false };
	cfg::_bool abs_mt_blob_id{ this, "ABS_MT_BLOB_ID", false };
	cfg::_bool abs_mt_tracking_id{ this, "ABS_MT_TRACKING_ID", false };
	cfg::_bool abs_mt_pressure{ this, "ABS_MT_PRESSURE", false };
	cfg::_bool abs_mt_distance{ this, "ABS_MT_DISTANCE", false };
	cfg::_bool abs_mt_tool_x{ this, "ABS_MT_TOOL_X", false };
	cfg::_bool abs_mt_tool_y{ this, "ABS_MT_TOOL_Y", false };

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

	bool exist()
	{
		return fs::is_file(cfg_name);
	}
};

class evdev_joystick_handler final : public PadHandlerBase
{
	// Unique button names for the config files and our pad settings dialog
	const std::unordered_map<u32, std::string> button_list =
	{
		//{ BTN_MISC            , "Misc"        }, same as BTN_0
		{ BTN_0               , "0"           },
		{ BTN_1               , "1"           },
		{ BTN_2               , "2"           },
		{ BTN_3               , "3"           },
		{ BTN_4               , "4"           },
		{ BTN_5               , "5"           },
		{ BTN_6               , "6"           },
		{ BTN_7               , "7"           },
		{ BTN_8               , "8"           },
		{ BTN_9               , "9"           },
		//{ BTN_MOUSE           , "Mouse"       }, same as BTN_LEFT
		{ BTN_LEFT            , "Left"        },
		{ BTN_RIGHT           , "Right"       },
		{ BTN_MIDDLE          , "Middle"      },
		{ BTN_SIDE            , "Side"        },
		{ BTN_EXTRA           , "Extra"       },
		{ BTN_FORWARD         , "Forward"     },
		{ BTN_BACK            , "Back"        },
		{ BTN_TASK            , "Task"        },
		{ BTN_JOYSTICK        , "Joystick"    },
		{ BTN_TRIGGER         , "Trigger"     },
		{ BTN_THUMB           , "Thumb"       },
		{ BTN_THUMB2          , "Thumb 2"     },
		{ BTN_TOP             , "Top"         },
		{ BTN_TOP2            , "Top 2"       },
		{ BTN_PINKIE          , "Pinkie"      },
		{ BTN_BASE            , "Base"        },
		{ BTN_BASE2           , "Base 2"      },
		{ BTN_BASE3           , "Base 3"      },
		{ BTN_BASE4           , "Base 4"      },
		{ BTN_BASE5           , "Base 5"      },
		{ BTN_BASE6           , "Base 6"      },
		{ BTN_DEAD            , "Dead"        },
		//{ BTN_GAMEPAD         , "Gamepad"     }, same as BTN_A
		//{ BTN_SOUTH           , "South"       }, same as BTN_A
		{ BTN_A               , "A"           },
		//{ BTN_EAST            , "South"       }, same as BTN_B
		{ BTN_B               , "B"           },
		{ BTN_C               , "C"           },
		//{ BTN_NORTH           , "North"       }, same as BTN_X
		{ BTN_X               , "X"           },
		//{ BTN_WEST            , "West"        }, same as BTN_Y
		{ BTN_Y               , "Y"           },
		{ BTN_Z               , "Z"           },
		{ BTN_TL              , "TL"          },
		{ BTN_TR              , "TR"          },
		{ BTN_TL2             , "TL 2"        },
		{ BTN_TR2             , "TR 2"        },
		{ BTN_SELECT          , "Select"      },
		{ BTN_START           , "Start"       },
		{ BTN_MODE            , "Mode"        },
		{ BTN_THUMBL          , "Thumb L"     },
		{ BTN_THUMBR          , "Thumb R"     },
		//{ BTN_DIGI            , "Digi"        }, same as BTN_TOOL_PEN
		{ BTN_TOOL_PEN        , "Pen"         },
		{ BTN_TOOL_RUBBER     , "Rubber"      },
		{ BTN_TOOL_BRUSH      , "Brush"       },
		{ BTN_TOOL_PENCIL     , "Pencil"      },
		{ BTN_TOOL_AIRBRUSH   , "Airbrush"    },
		{ BTN_TOOL_FINGER     , "Finger"      },
		{ BTN_TOOL_MOUSE      , "Mouse"       },
		{ BTN_TOOL_LENS       , "Lense"       },
		{ BTN_TOOL_QUINTTAP   , "Quinttap"    },
		{ BTN_TOUCH           , "Touch"       },
		{ BTN_STYLUS          , "Stylus"      },
		{ BTN_STYLUS2         , "Stylus 2"    },
		{ BTN_TOOL_DOUBLETAP  , "Doubletap"   },
		{ BTN_TOOL_TRIPLETAP  , "Tripletap"   },
		{ BTN_TOOL_QUADTAP    , "Quadtap"     },
		//{ BTN_WHEEL           , "Wheel"       }, same as BTN_GEAR_DOWN
		{ BTN_GEAR_DOWN       , "Gear Up"     },
		{ BTN_GEAR_UP         , "Gear Down"   },
		{ BTN_DPAD_UP         , "D-Pad Up"    },
		{ BTN_DPAD_DOWN       , "D-Pad Down"  },
		{ BTN_DPAD_LEFT       , "D-Pad Left"  },
		{ BTN_DPAD_RIGHT      , "D-Pad Right" },
		{ BTN_TRIGGER_HAPPY   , "Happy"       },
		{ BTN_TRIGGER_HAPPY1  , "Happy 1"     },
		{ BTN_TRIGGER_HAPPY2  , "Happy 2"     },
		{ BTN_TRIGGER_HAPPY3  , "Happy 3"     },
		{ BTN_TRIGGER_HAPPY4  , "Happy 4"     },
		{ BTN_TRIGGER_HAPPY5  , "Happy 5"     },
		{ BTN_TRIGGER_HAPPY6  , "Happy 6"     },
		{ BTN_TRIGGER_HAPPY7  , "Happy 7"     },
		{ BTN_TRIGGER_HAPPY8  , "Happy 8"     },
		{ BTN_TRIGGER_HAPPY9  , "Happy 9"     },
		{ BTN_TRIGGER_HAPPY10 , "Happy 10"    },
		{ BTN_TRIGGER_HAPPY11 , "Happy 11"    },
		{ BTN_TRIGGER_HAPPY12 , "Happy 12"    },
		{ BTN_TRIGGER_HAPPY13 , "Happy 13"    },
		{ BTN_TRIGGER_HAPPY14 , "Happy 14"    },
		{ BTN_TRIGGER_HAPPY15 , "Happy 15"    },
		{ BTN_TRIGGER_HAPPY16 , "Happy 16"    },
		{ BTN_TRIGGER_HAPPY17 , "Happy 17"    },
		{ BTN_TRIGGER_HAPPY18 , "Happy 18"    },
		{ BTN_TRIGGER_HAPPY19 , "Happy 19"    },
		{ BTN_TRIGGER_HAPPY20 , "Happy 20"    },
		{ BTN_TRIGGER_HAPPY21 , "Happy 21"    },
		{ BTN_TRIGGER_HAPPY22 , "Happy 22"    },
		{ BTN_TRIGGER_HAPPY23 , "Happy 23"    },
		{ BTN_TRIGGER_HAPPY24 , "Happy 24"    },
		{ BTN_TRIGGER_HAPPY25 , "Happy 25"    },
		{ BTN_TRIGGER_HAPPY26 , "Happy 26"    },
		{ BTN_TRIGGER_HAPPY27 , "Happy 27"    },
		{ BTN_TRIGGER_HAPPY28 , "Happy 28"    },
		{ BTN_TRIGGER_HAPPY29 , "Happy 29"    },
		{ BTN_TRIGGER_HAPPY30 , "Happy 30"    },
		{ BTN_TRIGGER_HAPPY31 , "Happy 31"    },
		{ BTN_TRIGGER_HAPPY32 , "Happy 32"    },
		{ BTN_TRIGGER_HAPPY33 , "Happy 33"    },
		{ BTN_TRIGGER_HAPPY34 , "Happy 34"    },
		{ BTN_TRIGGER_HAPPY35 , "Happy 35"    },
		{ BTN_TRIGGER_HAPPY36 , "Happy 36"    },
		{ BTN_TRIGGER_HAPPY37 , "Happy 37"    },
		{ BTN_TRIGGER_HAPPY38 , "Happy 38"    },
		{ BTN_TRIGGER_HAPPY39 , "Happy 39"    },
		{ BTN_TRIGGER_HAPPY40 , "Happy 40"    },
	};

	// Unique positive axis names for the config files and our pad settings dialog
	const std::unordered_map<u32, std::string> axis_list =
	{
		{ ABS_X              , "LX+"          },
		{ ABS_Y              , "LY+"          },
		{ ABS_Z              , "LZ+"          },
		{ ABS_RX             , "RX+"          },
		{ ABS_RY             , "RY+"          },
		{ ABS_RZ             , "RZ+"          },
		{ ABS_THROTTLE       , "Throttle+"    },
		{ ABS_RUDDER         , "Rudder+"      },
		{ ABS_WHEEL          , "Wheel+"       },
		{ ABS_GAS            , "Gas+"         },
		{ ABS_BRAKE          , "Brake+"       },
		{ ABS_HAT0X          , "Hat0 X+"      },
		{ ABS_HAT0Y          , "Hat0 Y+"      },
		{ ABS_HAT1X          , "Hat1 X+"      },
		{ ABS_HAT1Y          , "Hat1 Y+"      },
		{ ABS_HAT2X          , "Hat2 X+"      },
		{ ABS_HAT2Y          , "Hat2 Y+"      },
		{ ABS_HAT3X          , "Hat3 X+"      },
		{ ABS_HAT3Y          , "Hat3 Y+"      },
		{ ABS_PRESSURE       , "Pressure+"    },
		{ ABS_DISTANCE       , "Distance+"    },
		{ ABS_TILT_X         , "Tilt X+"      },
		{ ABS_TILT_Y         , "Tilt Y+"      },
		{ ABS_TOOL_WIDTH     , "Width+"       },
		{ ABS_VOLUME         , "Volume+"      },
		{ ABS_MISC           , "Misc+"        },
		{ ABS_MT_SLOT        , "Slot+"        },
		{ ABS_MT_TOUCH_MAJOR , "MT TMaj+"     },
		{ ABS_MT_TOUCH_MINOR , "MT TMin+"     },
		{ ABS_MT_WIDTH_MAJOR , "MT WMaj+"     },
		{ ABS_MT_WIDTH_MINOR , "MT WMin+"     },
		{ ABS_MT_ORIENTATION , "MT Orient+"   },
		{ ABS_MT_POSITION_X  , "MT PosX+"     },
		{ ABS_MT_POSITION_Y  , "MT PosY+"     },
		{ ABS_MT_TOOL_TYPE   , "MT TType+"    },
		{ ABS_MT_BLOB_ID     , "MT Blob ID+"  },
		{ ABS_MT_TRACKING_ID , "MT Track ID+" },
		{ ABS_MT_PRESSURE    , "MT Pressure+" },
		{ ABS_MT_DISTANCE    , "MT Distance+" },
		{ ABS_MT_TOOL_X      , "MT Tool X+"   },
		{ ABS_MT_TOOL_Y      , "MT Tool Y+"   },
	};

	// Unique negative axis names for the config files and our pad settings dialog
	const std::unordered_map<u32, std::string> rev_axis_list =
	{
		{ ABS_X              , "LX-"          },
		{ ABS_Y              , "LY-"          },
		{ ABS_Z              , "LZ-"          },
		{ ABS_RX             , "RX-"          },
		{ ABS_RY             , "RY-"          },
		{ ABS_RZ             , "RZ-"          },
		{ ABS_THROTTLE       , "Throttle-"    },
		{ ABS_RUDDER         , "Rudder-"      },
		{ ABS_WHEEL          , "Wheel-"       },
		{ ABS_GAS            , "Gas-"         },
		{ ABS_BRAKE          , "Brake-"       },
		{ ABS_HAT0X          , "Hat0 X-"      },
		{ ABS_HAT0Y          , "Hat0 Y-"      },
		{ ABS_HAT1X          , "Hat1 X-"      },
		{ ABS_HAT1Y          , "Hat1 Y-"      },
		{ ABS_HAT2X          , "Hat2 X-"      },
		{ ABS_HAT2Y          , "Hat2 Y-"      },
		{ ABS_HAT3X          , "Hat3 X-"      },
		{ ABS_HAT3Y          , "Hat3 Y-"      },
		{ ABS_PRESSURE       , "Pressure-"    },
		{ ABS_DISTANCE       , "Distance-"    },
		{ ABS_TILT_X         , "Tilt X-"      },
		{ ABS_TILT_Y         , "Tilt Y-"      },
		{ ABS_TOOL_WIDTH     , "Width-"       },
		{ ABS_VOLUME         , "Volume-"      },
		{ ABS_MISC           , "Misc-"        },
		{ ABS_MT_SLOT        , "Slot-"        },
		{ ABS_MT_TOUCH_MAJOR , "MT TMaj-"     },
		{ ABS_MT_TOUCH_MINOR , "MT TMin-"     },
		{ ABS_MT_WIDTH_MAJOR , "MT WMaj-"     },
		{ ABS_MT_WIDTH_MINOR , "MT WMin-"     },
		{ ABS_MT_ORIENTATION , "MT Orient-"   },
		{ ABS_MT_POSITION_X  , "MT PosX-"     },
		{ ABS_MT_POSITION_Y  , "MT PosY-"     },
		{ ABS_MT_TOOL_TYPE   , "MT TType-"    },
		{ ABS_MT_BLOB_ID     , "MT Blob ID-"  },
		{ ABS_MT_TRACKING_ID , "MT Track ID-" },
		{ ABS_MT_PRESSURE    , "MT Pressure-" },
		{ ABS_MT_DISTANCE    , "MT Distance-" },
		{ ABS_MT_TOOL_X      , "MT Tool X-"   },
		{ ABS_MT_TOOL_Y      , "MT Tool Y-"   },
	};

	struct EvdevButton
	{
		u32 code;
		int dir;
		int type;
	};

	struct EvdevDevice
	{
		libevdev* device{ nullptr };
		pad_config* config{ nullptr };
		std::string path;
		std::shared_ptr<Pad> pad;
		std::unordered_map<int, bool> axis_orientations; // value is true if key was found in rev_axis_list
		s32 stick_val[4] = { 0, 0, 0, 0 };
		u16 val_min[4] = { 0, 0, 0, 0 };
		u16 val_max[4] = { 0, 0, 0, 0 };
		EvdevButton trigger_left  = { 0, 0, 0 };
		EvdevButton trigger_right = { 0, 0, 0 };
		std::vector<EvdevButton> axis_left  = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };
		std::vector<EvdevButton> axis_right = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };
		int cur_dir = 0;
		int cur_type = 0;
		int effect_id = -1;
		bool has_rumble = false;
		u16 force_large = 0;
		u16 force_small = 0;
		clock_t last_vibration = 0;
	};

	const int BUTTON_COUNT = 17;

public:
	evdev_joystick_handler();
	~evdev_joystick_handler();

	void init_config(pad_config* cfg, const std::string& name) override;
	bool Init() override;
	std::vector<std::string> ListDevices() override;
	bool bindPadToDevice(std::shared_ptr<Pad> pad, const std::string& device) override;
	void ThreadProc() override;
	void Close();
	void GetNextButtonPress(const std::string& padId, const std::function<void(u16, std::string, int[])>& callback, bool get_blacklist = false, std::vector<std::string> buttons = {}) override;
	void TestVibration(const std::string& padId, u32 largeMotor, u32 smallMotor) override;

private:
	void TranslateButtonPress(u64 keyCode, bool& pressed, u16& value, bool ignore_threshold = false) override;
	EvdevDevice* get_device(const std::string& device);
	std::string get_device_name(const libevdev* dev);
	bool update_device(EvdevDevice& device);
	void update_devs();
	int add_device(const std::string& device, bool in_settings = false);
	int GetButtonInfo(const input_event& evt, const EvdevDevice& device, int& button_code);
	std::unordered_map<u64, std::pair<u16, bool>> GetButtonValues(const EvdevDevice& device);
	void SetRumble(EvdevDevice* device, u16 large, u16 small);

	// Search axis_orientations map for the direction by index, returns -1 if not found, 0 for positive and 1 for negative
	int FindAxisDirection(const std::unordered_map<int, bool>& map, int index);

	positive_axis m_pos_axis_config;
	std::vector<u32> m_positive_axis;
	std::vector<std::string> blacklist;
	std::vector<EvdevDevice> devices;
	int m_pad_index = -1;
	EvdevDevice m_dev;
	bool m_is_button_or_trigger;
	bool m_is_negative;
	bool m_is_init = false;
};
