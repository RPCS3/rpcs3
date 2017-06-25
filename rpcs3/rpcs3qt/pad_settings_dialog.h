#pragma once

#include <QDialog>
#include <QEvent>
#include <QKeyEvent>
#include "keyboard_pad_handler.h"
#include "Utilities/types.h"
#include "Utilities/Config.h"
#include "Emu/Io/PadHandler.h"
#include "stdafx.h"
#include "Emu/System.h"

enum button_ids
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

struct pad_buttons
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

class pad_settings_dialog : public QDialog, pad_buttons, PadHandlerBase
{
	Q_OBJECT

private Q_SLOTS:
	void OnPadButtonClicked(int id);

private:
	u32 m_seconds;
	u32 m_button_id;
	bool m_key_pressed;
	QAction *onButtonClickedAct;

public:
	// TODO get Init to work
	virtual void Init(const u32 max_connect) override;
	explicit pad_settings_dialog(QWidget *parent = 0);
	void keyPressEvent(QKeyEvent *keyEvent);
	void UpdateLabel();
	void UpdateTimerLabel(const u32 id);
	void SwitchButtons(const bool IsEnabled);
	void RunTimer(const u32 seconds, const u32 id);
	void LoadSettings();
	const QString GetKeyName(const u32 keyCode);
};
