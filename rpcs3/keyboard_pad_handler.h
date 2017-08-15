#pragma once

#include "Utilities/Config.h"
#include "Emu/Io/PadHandler.h"
#include "stdafx.h"
#include "Emu/System.h"

#include <QWindow>
#include <QKeyEvent>

struct keyboard_pad_config final : cfg::node
{
	const std::string cfg_name = fs::get_config_dir() + "/config_kbpad_qt.yml";

	cfg::int32 left_stick_left{ this, "Left Analog Stick Left", Qt::Key_A };
	cfg::int32 left_stick_down{ this, "Left Analog Stick Down", Qt::Key_S };
	cfg::int32 left_stick_right{ this, "Left Analog Stick Right", Qt::Key_D };
	cfg::int32 left_stick_up{ this, "Left Analog Stick Up", Qt::Key_W };
	cfg::int32 right_stick_left{ this, "Right Analog Stick Left", Qt::Key_Home };
	cfg::int32 right_stick_down{ this, "Right Analog Stick Down", Qt::Key_PageDown };
	cfg::int32 right_stick_right{ this, "Right Analog Stick Right", Qt::Key_End };
	cfg::int32 right_stick_up{ this, "Right Analog Stick Up", Qt::Key_PageUp };
	cfg::int32 start{ this, "Start", Qt::Key_Return };
	cfg::int32 select{ this, "Select", Qt::Key_Space };
	cfg::int32 square{ this, "Square", Qt::Key_Z };
	cfg::int32 cross{ this, "Cross", Qt::Key_X };
	cfg::int32 circle{ this, "Circle", Qt::Key_C };
	cfg::int32 triangle{ this, "Triangle", Qt::Key_V };
	cfg::int32 left{ this, "Left", Qt::Key_Left };
	cfg::int32 down{ this, "Down", Qt::Key_Down };
	cfg::int32 right{ this, "Right", Qt::Key_Right };
	cfg::int32 up{ this, "Up", Qt::Key_Up };
	cfg::int32 r1{ this, "R1", Qt::Key_E };
	cfg::int32 r2{ this, "R2", Qt::Key_T };
	cfg::int32 r3{ this, "R3", Qt::Key_G };
	cfg::int32 l1{ this, "L1", Qt::Key_Q };
	cfg::int32 l2{ this, "L2", Qt::Key_R };
	cfg::int32 l3{ this, "L3", Qt::Key_F };

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

class keyboard_pad_handler final : public QObject, public PadHandlerBase
{
public:
	virtual void Init(const u32 max_connect) override;

	keyboard_pad_handler();

	void SetTargetWindow(QWindow* target);
	void keyPressEvent(QKeyEvent* event);
	void keyReleaseEvent(QKeyEvent* event);
	void LoadSettings();

	bool eventFilter(QObject* obj, QEvent* ev) override;
private:
	QWindow* m_target = nullptr;
};
