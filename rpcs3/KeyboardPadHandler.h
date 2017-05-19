#pragma once

#include "Utilities/Config.h"
#include "Emu/Io/PadHandler.h"
#include "stdafx.h"
#include "Emu/System.h"

#include <QKeyEvent>
#include <QObject>

struct KeyboardPadConfig final : cfg::node
{
	const std::string cfg_name = fs::get_config_dir() + "/config_kbpad.yml";

	cfg::int32_entry left_stick_left{ *this, "Left Analog Stick Left", Qt::Key_A };
	cfg::int32_entry left_stick_down{ *this, "Left Analog Stick Down", Qt::Key_S };
	cfg::int32_entry left_stick_right{ *this, "Left Analog Stick Right", Qt::Key_D };
	cfg::int32_entry left_stick_up{ *this, "Left Analog Stick Up", Qt::Key_W };
	cfg::int32_entry right_stick_left{ *this, "Right Analog Stick Left", Qt::Key_Home };
	cfg::int32_entry right_stick_down{ *this, "Right Analog Stick Down", Qt::Key_PageDown };
	cfg::int32_entry right_stick_right{ *this, "Right Analog Stick Right", Qt::Key_End };
	cfg::int32_entry right_stick_up{ *this, "Right Analog Stick Up", Qt::Key_PageUp };
	cfg::int32_entry start{ *this, "Start", Qt::Key_Return };
	cfg::int32_entry select{ *this, "Select", Qt::Key_Space };
	cfg::int32_entry square{ *this, "Square", Qt::Key_Z };
	cfg::int32_entry cross{ *this, "Cross", Qt::Key_X };
	cfg::int32_entry circle{ *this, "Circle", Qt::Key_C };
	cfg::int32_entry triangle{ *this, "Triangle", Qt::Key_V };
	cfg::int32_entry left{ *this, "Left", Qt::Key_Left };
	cfg::int32_entry down{ *this, "Down", Qt::Key_Down };
	cfg::int32_entry right{ *this, "Right", Qt::Key_Right };
	cfg::int32_entry up{ *this, "Up", Qt::Key_Up };
	cfg::int32_entry r1{ *this, "R1", Qt::Key_E };
	cfg::int32_entry r2{ *this, "R2", Qt::Key_T };
	cfg::int32_entry r3{ *this, "R3", Qt::Key_G };
	cfg::int32_entry l1{ *this, "L1", Qt::Key_Q };
	cfg::int32_entry l2{ *this, "L2", Qt::Key_R };
	cfg::int32_entry l3{ *this, "L3", Qt::Key_F };

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

class KeyboardPadHandler final : public QObject, public PadHandlerBase
{
public:
	virtual void Init(const u32 max_connect) override;

	KeyboardPadHandler(QObject* target, QObject* parent);

	void keyPressEvent(QKeyEvent* event);
	void keyReleaseEvent(QKeyEvent* event);
	void LoadSettings();

	bool eventFilter(QObject* obj, QEvent* ev);
private:
	QObject* m_target;
};
