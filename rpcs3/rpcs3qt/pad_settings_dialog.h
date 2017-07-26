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

namespace Ui {
	class pad_settings_dialog;
}

class pad_settings_dialog : public QDialog, PadHandlerBase
{
	Q_OBJECT

private Q_SLOTS:
	void OnPadButtonClicked(int id);

private:
	u32 m_seconds;
	u32 m_button_id;
	bool m_key_pressed;
	QAction *onButtonClickedAct;
	Ui::pad_settings_dialog *ui;

public:
	// TODO get Init to work
	virtual void Init(const u32 max_connect) override;
	explicit pad_settings_dialog(QWidget *parent = 0);
	~pad_settings_dialog();
	void keyPressEvent(QKeyEvent *keyEvent);
	void UpdateLabel();
	void UpdateTimerLabel(const u32 id);
	void SwitchButtons(const bool IsEnabled);
	void RunTimer(const u32 seconds, const u32 id);
	void LoadSettings();
	const QString GetKeyName(const u32 keyCode);
};
