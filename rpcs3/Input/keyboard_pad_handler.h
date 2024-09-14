#pragma once

#include "util/types.hpp"
#include "Emu/Io/PadHandler.h"

#include <QWindow>
#include <QKeyEvent>
#include <string>
#include <vector>
#include <unordered_map>

enum mouse
{
	move_left   = 0x05555550,
	move_right  = 0x05555551,
	move_up     = 0x05555552,
	move_down   = 0x05555553,
	wheel_up    = 0x05555554,
	wheel_down  = 0x05555555,
	wheel_left  = 0x05555556,
	wheel_right = 0x05555557
};

// Unique button names for the config files and our pad settings dialog
const std::unordered_map<u32, std::string> mouse_list =
{
	{ Qt::NoButton       , ""             },
	{ Qt::LeftButton     , "Mouse Left"   },
	{ Qt::RightButton    , "Mouse Right"  },
	{ Qt::MiddleButton   , "Mouse Middle" },
	{ Qt::BackButton     , "Mouse Back"   },
	{ Qt::ForwardButton  , "Mouse Fwd"    },
	{ Qt::TaskButton     , "Mouse Task"   },
	{ Qt::ExtraButton4   , "Mouse 4"      },
	{ Qt::ExtraButton5   , "Mouse 5"      },
	{ Qt::ExtraButton6   , "Mouse 6"      },
	{ Qt::ExtraButton7   , "Mouse 7"      },
	{ Qt::ExtraButton8   , "Mouse 8"      },
	{ Qt::ExtraButton9   , "Mouse 9"      },
	{ Qt::ExtraButton10  , "Mouse 10"     },
	{ Qt::ExtraButton11  , "Mouse 11"     },
	{ Qt::ExtraButton12  , "Mouse 12"     },
	{ Qt::ExtraButton13  , "Mouse 13"     },
	{ Qt::ExtraButton14  , "Mouse 14"     },
	{ Qt::ExtraButton15  , "Mouse 15"     },
	{ Qt::ExtraButton16  , "Mouse 16"     },
	{ Qt::ExtraButton17  , "Mouse 17"     },
	{ Qt::ExtraButton18  , "Mouse 18"     },
	{ Qt::ExtraButton19  , "Mouse 19"     },
	{ Qt::ExtraButton20  , "Mouse 20"     },
	{ Qt::ExtraButton21  , "Mouse 21"     },
	{ Qt::ExtraButton22  , "Mouse 22"     },
	{ Qt::ExtraButton23  , "Mouse 23"     },
	{ Qt::ExtraButton24  , "Mouse 24"     },

	{ mouse::move_left   , "Mouse MLeft"  },
	{ mouse::move_right  , "Mouse MRight" },
	{ mouse::move_up     , "Mouse MUp"    },
	{ mouse::move_down   , "Mouse MDown"  },

	{ mouse::wheel_up    , "Wheel Up"     },
	{ mouse::wheel_down  , "Wheel Down"   },
	{ mouse::wheel_left  , "Wheel Left"   },
	{ mouse::wheel_right , "Wheel Right"  },
};

class keyboard_pad_handler final : public QObject, public PadHandlerBase
{
public:
	bool Init() override;

	keyboard_pad_handler();

	void SetTargetWindow(QWindow* target);
	void processKeyEvent(QKeyEvent* event, bool pressed);
	void keyPressEvent(QKeyEvent* event);
	void keyReleaseEvent(QKeyEvent* event);
	void mousePressEvent(QMouseEvent* event);
	void mouseReleaseEvent(QMouseEvent* event);
	void mouseMoveEvent(QMouseEvent* event);
	void mouseWheelEvent(QWheelEvent* event);

	bool eventFilter(QObject* target, QEvent* ev) override;

	void init_config(cfg_pad* cfg) override;
	std::vector<pad_list_entry> list_devices() override;
	connection get_next_button_press(const std::string& /*padId*/, const pad_callback& /*callback*/, const pad_fail_callback& /*fail_callback*/, gui_call_type /*call_type*/, const std::vector<std::string>& /*buttons*/) override { return connection::connected; }
	bool bindPadToDevice(std::shared_ptr<Pad> pad) override;
	void process() override;

	static std::string GetMouseName(const QMouseEvent* event);
	static std::string GetMouseName(u32 button);
	static QStringList GetKeyNames(const QKeyEvent* keyEvent);
	static std::string GetKeyName(const QKeyEvent* keyEvent, bool with_modifiers);
	static std::string GetKeyName(const u32& keyCode);
	static std::set<u32> GetKeyCodes(const cfg::string& cfg_string);
	static u32 GetKeyCode(const QString& keyName);

	static int native_scan_code_from_string(const std::string& key);
	static std::string native_scan_code_to_string(int native_scan_code);

protected:
	void Key(const u32 code, bool pressed, u16 value = 255);

private:
	QWindow* m_target = nullptr;
	mouse_movement_mode m_mouse_movement_mode = mouse_movement_mode::relative;
	bool m_keys_released = false;
	bool m_mouse_move_used = false;
	bool m_mouse_wheel_used = false;
	bool get_mouse_lock_state() const;
	void release_all_keys();

	std::vector<Pad> m_pads_internal; // Accumulates input until the next poll. Only used for user input!

	// Button Movements
	steady_clock::time_point m_button_time;
	f32 m_analog_lerp_factor  = 1.0f;
	f32 m_trigger_lerp_factor = 1.0f;
	bool m_analog_limiter_toggle_mode = false;
	bool m_pressure_intensity_toggle_mode = false;
	u32 m_pressure_intensity_deadzone = 0;

	// Stick Movements
	steady_clock::time_point m_stick_time;
	f32 m_l_stick_lerp_factor = 1.0f;
	f32 m_r_stick_lerp_factor = 1.0f;
	u32 m_l_stick_multiplier = 100;
	u32 m_r_stick_multiplier = 100;

	static constexpr usz max_sticks = 4;
	std::array<u8, max_sticks> m_stick_min{ 0, 0, 0, 0 };
	std::array<u8, max_sticks> m_stick_max{ 128, 128, 128, 128 };
	std::array<u8, max_sticks> m_stick_val{ 128, 128, 128, 128 };

	// Mouse Movements
	steady_clock::time_point m_last_mouse_move_left;
	steady_clock::time_point m_last_mouse_move_right;
	steady_clock::time_point m_last_mouse_move_up;
	steady_clock::time_point m_last_mouse_move_down;
	int m_deadzone_x = 60;
	int m_deadzone_y = 60;
	double m_multi_x = 2;
	double m_multi_y = 2.5;

	// Mousewheel
	steady_clock::time_point m_last_wheel_move_up;
	steady_clock::time_point m_last_wheel_move_down;
	steady_clock::time_point m_last_wheel_move_left;
	steady_clock::time_point m_last_wheel_move_right;
};
