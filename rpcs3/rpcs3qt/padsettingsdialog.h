#ifndef PADSETTINGS_H
#define PADSETTINGS_H

#include <QDialog>
#include <QEvent>
#include <QKeyEvent>
#include "Utilities/types.h"
#include "Utilities/Config.h"
#include "Emu/Io/PadHandler.h"
#include "stdafx.h"
#include "Emu/System.h"

struct KeyboardPadConfig final : cfg::node
{
	const std::string cfg_name = fs::get_config_dir() + "/config_kbpad.yml";

	cfg::int32_entry left_stick_left{ *this, "Left Analog Stick Left", static_cast<int>('A') };
	cfg::int32_entry left_stick_down{ *this, "Left Analog Stick Down", static_cast<int>('S') };
	cfg::int32_entry left_stick_right{ *this, "Left Analog Stick Right", static_cast<int>('D') };
	cfg::int32_entry left_stick_up{ *this, "Left Analog Stick Up", static_cast<int>('W') };
	cfg::int32_entry right_stick_left{ *this, "Right Analog Stick Left", Qt::Key_Home };
	cfg::int32_entry right_stick_down{ *this, "Right Analog Stick Down", Qt::Key_PageDown };
	cfg::int32_entry right_stick_right{ *this, "Right Analog Stick Right", Qt::Key_End };
	cfg::int32_entry right_stick_up{ *this, "Right Analog Stick Up", Qt::Key_PageUp };
	cfg::int32_entry start{ *this, "Start", Qt::Key_Return };
	cfg::int32_entry select{ *this, "Select", Qt::Key_Space };
	cfg::int32_entry square{ *this, "Square", static_cast<int>('Z') };
	cfg::int32_entry cross{ *this, "Cross", static_cast<int>('X') };
	cfg::int32_entry circle{ *this, "Circle", static_cast<int>('C') };
	cfg::int32_entry triangle{ *this, "Triangle", static_cast<int>('V') };
	cfg::int32_entry left{ *this, "Left", Qt::Key_Left };
	cfg::int32_entry down{ *this, "Down", Qt::Key_Down };
	cfg::int32_entry right{ *this, "Right", Qt::Key_Right };
	cfg::int32_entry up{ *this, "Up", Qt::Key_Up };
	cfg::int32_entry r1{ *this, "R1", static_cast<int>('E') };
	cfg::int32_entry r2{ *this, "R2", static_cast<int>('T') };
	cfg::int32_entry r3{ *this, "R3", static_cast<int>('G') };
	cfg::int32_entry l1{ *this, "L1", static_cast<int>('Q') };
	cfg::int32_entry l2{ *this, "L2", static_cast<int>('R') };
	cfg::int32_entry l3{ *this, "L3", static_cast<int>('F') };

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

enum ButtonIDs
{
	id_pad_lstick_left = 0x1,
	id_pad_lstick_down,
	id_pad_lstick_right,
	id_pad_lstick_up,

	id_pad_left,
	id_pad_down,
	id_pad_right,
	id_pad_up,

	id_pad_l1,
	id_pad_l2,
	id_pad_l3,

	id_pad_start,
	id_pad_select,

	id_pad_r1,
	id_pad_r2,
	id_pad_r3,

	id_pad_square,
	id_pad_cross,
	id_pad_circle,
	id_pad_triangle,

	id_pad_rstick_left,
	id_pad_rstick_down,
	id_pad_rstick_right,
	id_pad_rstick_up,

	id_reset_parameters,
	id_ok,
	id_cancel
};

struct PadButtons
{
	QPushButton* b_up_lstick;
	QPushButton* b_down_lstick;
	QPushButton* b_left_lstick;
	QPushButton* b_right_lstick;

	QPushButton* b_up;
	QPushButton* b_down;
	QPushButton* b_left;
	QPushButton* b_right;

	QPushButton* b_shift_l1;
	QPushButton* b_shift_l2;
	QPushButton* b_shift_l3;

	QPushButton* b_start;
	QPushButton* b_select;

	QPushButton* b_shift_r1;
	QPushButton* b_shift_r2;
	QPushButton* b_shift_r3;

	QPushButton* b_square;
	QPushButton* b_cross;
	QPushButton* b_circle;
	QPushButton* b_triangle;

	QPushButton* b_up_rstick;
	QPushButton* b_down_rstick;
	QPushButton* b_left_rstick;
	QPushButton* b_right_rstick;

	QPushButton* b_ok;
	QPushButton* b_cancel;
	QPushButton* b_reset;
};

class PadSettingsDialog : public QDialog, PadButtons, PadHandlerBase
{
	Q_OBJECT

private slots :
	void OnPadButtonClicked(int id);

private:
	u32 m_seconds;
	u32 m_button_id;
	bool m_key_pressed;
	KeyboardPadConfig g_kbpad_config;
	QAction *onButtonClickedAct;

public:
	// TODO get Init to work
	virtual void Init(const u32 max_connect) override;
	explicit PadSettingsDialog(QWidget *parent = 0);
	void keyPressEvent(QKeyEvent *keyEvent);
	void UpdateLabel();
	void ResetParameters();
	void UpdateTimerLabel(const u32 id);
	void SwitchButtons(const bool IsEnabled);
	void RunTimer(const u32 seconds, const u32 id);
	void LoadSettings();
	const QString GetKeyName(const u32 keyCode);
};

#endif // PADSETTINGS_H
