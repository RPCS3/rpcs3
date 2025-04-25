#pragma once
#ifdef HAVE_LIBEVDEV

#include "util/types.hpp"
#include "Utilities/File.h"
#include "Emu/Io/PadHandler.h"
#include <libevdev/libevdev.h>
#include <memory>
#include <unordered_map>
#include <array>
#include <vector>
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

	bool load();
	void save() const;
	bool exist() const;
};

class evdev_joystick_handler final : public PadHandlerBase
{
	static constexpr u32 NO_BUTTON = u32{umax}; // Some event code that doesn't exist in evdev (code type is U16)

	// Unique button names for the config files and our pad settings dialog
	const std::unordered_map<u32, std::string> button_list =
	{
		{ NO_BUTTON           , ""            },
		// Xbox One S Controller returns some buttons as key when connected through bluetooth
		{ KEY_BACK            , "Back Key"    },
		{ KEY_HOMEPAGE        , "Homepage Key"},
		// Wii/Wii U Controller drivers use the following few keys
		{ KEY_UP              , "Up Key"      },
		{ KEY_LEFT            , "Left Key"    },
		{ KEY_RIGHT           , "Right Key"   },
		{ KEY_DOWN            , "Down Key"    },
		{ KEY_NEXT            , "Next Key"    },
		{ KEY_PREVIOUS        , "Previous Key"},
		// 8bitdo Pro 2 controller uses this for the button below "b"
		{ KEY_MENU            , "Menu"        },
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
		{ 0x10a               , "0x10a"       },
		{ 0x10b               , "0x10b"       },
		{ 0x10c               , "0x10c"       },
		{ 0x10d               , "0x10d"       },
		{ 0x10e               , "0x10e"       },
		{ 0x10f               , "0x10f"       },
		//{ BTN_MOUSE           , "Mouse"       }, same as BTN_LEFT
		{ BTN_LEFT            , "Left"        },
		{ BTN_RIGHT           , "Right"       },
		{ BTN_MIDDLE          , "Middle"      },
		{ BTN_SIDE            , "Side"        },
		{ BTN_EXTRA           , "Extra"       },
		{ BTN_FORWARD         , "Forward"     },
		{ BTN_BACK            , "Back"        },
		{ BTN_TASK            , "Task"        },
		{ 0x118               , "0x118"       },
		{ 0x119               , "0x119"       },
		{ 0x11a               , "0x11a"       },
		{ 0x11b               , "0x11b"       },
		{ 0x11c               , "0x11c"       },
		{ 0x11d               , "0x11d"       },
		{ 0x11e               , "0x11e"       },
		{ 0x11f               , "0x11f"       },
		//{ BTN_JOYSTICK        , "Joystick"    }, same as BTN_TRIGGER
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
		{ 0x12c               , "0x12c"       },
		{ 0x12d               , "0x12d"       },
		{ 0x12e               , "0x12e"       },
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
		{ 0x13f               , "0x13f"       },
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
		{ 0x149               , "0x149"       },
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
		{ BTN_TRIGGER_HAPPY40 , "Happy 40"    }
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

	// Unique motion axis names for the config files and our pad settings dialog
	const std::unordered_map<u32, std::string> motion_axis_list =
	{
		{ ABS_X  , "X"  },
		{ ABS_Y  , "Y"  },
		{ ABS_Z  , "Z"  },
		{ ABS_RX , "RX" },
		{ ABS_RY , "RY" },
		{ ABS_RZ , "RZ" },
	};

	struct EvdevButton
	{
		u32 code = 0; // key code of our button or axis
		int dir = 0;  // dir is -1 in case of a button, 0 in case of a regular axis and 1 in case of a reverse axis
		int type = 0; // EV_KEY or EV_ABS
	};

	struct evdev_sensor : public EvdevButton
	{
		bool mirrored = false;
		s32 shift = 0;
	};

	struct input_event_wrapper
	{
		int type{}; // EV_KEY or EV_ABS
		bool is_initialized{};
	};

	struct key_event_wrapper : public input_event_wrapper
	{
		key_event_wrapper() { type = EV_KEY; }
		u16 value{};
	};

	struct axis_event_wrapper : public input_event_wrapper
	{
		axis_event_wrapper() { type = EV_ABS; }
		std::map<bool, u16> values{}; // direction (negative = true)
		bool is_trigger{};
		int min{};
		int max{};
		int flat{};
	};

	struct EvdevDevice : public PadDevice
	{
		libevdev* device{ nullptr };
		std::string path;
		std::vector<EvdevButton> all_buttons;
		std::set<u32> trigger_left{};
		std::set<u32> trigger_right{};
		std::array<std::set<u32>, 4> axis_left{};
		std::array<std::set<u32>, 4> axis_right{};
		std::array<evdev_sensor, 4> axis_motion{};
		std::map<u32, std::shared_ptr<input_event_wrapper>> events_by_code;
		int cur_dir = 0;
		int cur_type = 0;
		int effect_id = -1;
		bool has_rumble = false;
		bool has_motion = false;
	};

public:
	evdev_joystick_handler();
	~evdev_joystick_handler();

	void init_config(cfg_pad* cfg) override;
	bool Init() override;
	std::vector<pad_list_entry> list_devices() override;
	bool bindPadToDevice(std::shared_ptr<Pad> pad) override;
	connection get_next_button_press(const std::string& padId, const pad_callback& callback, const pad_fail_callback& fail_callback, gui_call_type call_type, const std::vector<std::string>& buttons) override;
	void get_motion_sensors(const std::string& padId, const motion_callback& callback, const motion_fail_callback& fail_callback, motion_preview_values preview_values, const std::array<AnalogSensor, 4>& sensors) override;
	std::unordered_map<u32, std::string> get_motion_axis_list() const override;
	void SetPadData(const std::string& padId, u8 player_id, u8 large_motor, u8 small_motor, s32 r, s32 g, s32 b, bool player_led, bool battery_led, u32 battery_led_brightness) override;

private:
	void close_devices();
	std::shared_ptr<EvdevDevice> get_evdev_device(const std::string& device);
	std::string get_device_name(const libevdev* dev);
	bool update_device(const std::shared_ptr<PadDevice>& device);
	std::shared_ptr<evdev_joystick_handler::EvdevDevice> add_device(const std::string& device, bool in_settings = false);
	std::shared_ptr<evdev_joystick_handler::EvdevDevice> add_motion_device(const std::string& device, bool in_settings);
	std::unordered_map<u64, std::pair<u16, bool>> GetButtonValues(const std::shared_ptr<EvdevDevice>& device);
	void SetRumble(EvdevDevice* device, u8 large, u8 small);

	positive_axis m_pos_axis_config;
	std::set<u32> m_positive_axis;
	std::set<std::string> m_blacklist;
	std::unordered_map<std::string, u16> m_min_button_values;
	std::unordered_map<std::string, std::shared_ptr<evdev_joystick_handler::EvdevDevice>> m_settings_added;
	std::unordered_map<std::string, std::shared_ptr<evdev_joystick_handler::EvdevDevice>> m_motion_settings_added;
	std::shared_ptr<EvdevDevice> m_dev;

	bool check_button_set(const std::set<u32>& indices, const u32 code);
	bool check_button_sets(const std::array<std::set<u32>, 4>& sets, const u32 code);

	void apply_input_events(const std::shared_ptr<Pad>& pad);

	u16 get_sensor_value(const libevdev* dev, const AnalogSensor& sensor, const input_event& evt) const;

protected:
	PadHandlerBase::connection update_connection(const std::shared_ptr<PadDevice>& device) override;
	void get_mapping(const pad_ensemble& binding) override;
	void get_extended_info(const pad_ensemble& binding) override;
	void apply_pad_data(const pad_ensemble& binding) override;
	bool get_is_left_trigger(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	bool get_is_right_trigger(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	bool get_is_left_stick(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	bool get_is_right_stick(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
};

#endif
