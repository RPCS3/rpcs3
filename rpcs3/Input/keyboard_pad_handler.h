#pragma once

#include "stdafx.h"
#include "Emu/Io/PadHandler.h"

#include <QWindow>
#include <QKeyEvent>

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

class keyboard_pad_handler final : public QObject, public PadHandlerBase
{
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

	bool eventFilter(QObject* obj, QEvent* ev) override;

	void init_config(pad_config* cfg, const std::string& name) override;
	std::vector<std::string> ListDevices() override;
	void get_next_button_press(const std::string& /*padId*/, const pad_callback& /*callback*/, const pad_fail_callback& /*fail_callback*/, bool /*get_blacklist*/ = false, const std::vector<std::string>& /*buttons*/ = {}) override {};
	bool bindPadToDevice(std::shared_ptr<Pad> pad, const std::string& device) override;
	void ThreadProc() override;

	std::string GetMouseName(const QMouseEvent* event);
	std::string GetMouseName(u32 button);
	QStringList GetKeyNames(const QKeyEvent* keyEvent);
	std::string GetKeyName(const QKeyEvent* keyEvent);
	std::string GetKeyName(const u32& keyCode);
	u32 GetKeyCode(const std::string& keyName);
	u32 GetKeyCode(const QString& keyName);

protected:
	void Key(const u32 code, bool pressed, u16 value = 255);
	int GetModifierCode(QKeyEvent* e);

private:
	QWindow* m_target = nullptr;
	std::vector<std::shared_ptr<Pad>> bindings;

	// Button Movements
	std::chrono::steady_clock::time_point m_button_time;
	f32 m_analog_lerp_factor  = 1.0f;
	f32 m_trigger_lerp_factor = 1.0f;

	// Stick Movements
	std::chrono::steady_clock::time_point m_stick_time;
	f32 m_l_stick_lerp_factor = 1.0f;
	f32 m_r_stick_lerp_factor = 1.0f;
	u8 m_stick_min[4] = { 0, 0, 0, 0 };
	u8 m_stick_max[4] = { 128, 128, 128, 128 };
	u8 m_stick_val[4] = { 128, 128, 128, 128 };

	// Mouse Movements
	std::chrono::steady_clock::time_point m_last_mouse_move_left;
	std::chrono::steady_clock::time_point m_last_mouse_move_right;
	std::chrono::steady_clock::time_point m_last_mouse_move_up;
	std::chrono::steady_clock::time_point m_last_mouse_move_down;
	int m_deadzone_x = 60;
	int m_deadzone_y = 60;
	double m_multi_x = 2;
	double m_multi_y = 2.5;

	// Mousewheel
	std::chrono::steady_clock::time_point m_last_wheel_move_up;
	std::chrono::steady_clock::time_point m_last_wheel_move_down;
	std::chrono::steady_clock::time_point m_last_wheel_move_left;
	std::chrono::steady_clock::time_point m_last_wheel_move_right;
};
