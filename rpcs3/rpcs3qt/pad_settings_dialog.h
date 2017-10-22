#pragma once

#include <QDialog>
#include <QEvent>
#include <QKeyEvent>
#include <QTimer>
#include <QButtonGroup>

#include "keyboard_pad_handler.h"
#include "Utilities/types.h"
#include "Utilities/Config.h"
#include "Emu/Io/PadHandler.h"
#include "stdafx.h"
#include "Emu/System.h"
#include "gui_settings.h"

#ifdef WIN32
#include "xinput_pad_handler.h"
#endif

enum button_ids
{
	id_pad_begin, // begin

	id_pad_lstick_left,
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

	id_pad_end, // end

	id_reset_parameters,
	id_ok,
	id_cancel
};

namespace Ui
{
	class pad_settings_dialog;
}

class pad_settings_dialog : public QDialog
{
	Q_OBJECT

	enum handler_type
	{
		HANDLER_TYPE_KEYBOARD,
		HANDLER_TYPE_XINPUT,
		HANDLER_TYPE_DS4,
		HANDLER_TYPE_MMJOYSTICK
	};

	struct PAD_BUTTON
	{
		cfg::int32* cfg_id;
		std::string key;
		QString text;
	};

private Q_SLOTS:
	void OnPadButtonClicked(int id);

private:
	Ui::pad_settings_dialog *ui;

	// Button Mapping
	QButtonGroup* m_padButtons;
	u32 m_button_id;
	std::map<int /*id*/, PAD_BUTTON /*info*/> m_cfg_entries;

	// Backup for standard button palette
	QPalette m_palette;

	// Pad Handlers 
	PadHandlerBase* m_handler;
	handler_type m_handler_type;
	pad_config* m_handler_cfg;
	std::string m_device_name;

	// Remap Timer
	const int MAX_SECONDS = 5;
	int m_seconds = MAX_SECONDS;
	QTimer m_timer;

	// XInput timer. Its Callback handles XInput input
	QTimer m_timer_xinput;

	/** Resets the view to default. Resets the Remap Timer */
	void ReactivateButtons();

public:
	explicit pad_settings_dialog(pad_config* pad_cfg, const std::string& device, PadHandlerBase& handler, QWidget *parent = nullptr);
	~pad_settings_dialog();

	/** Handle keyboard handler input */
	void keyPressEvent(QKeyEvent *keyEvent) override;

	/** Update all the Button Labels with current button mapping */
	void UpdateLabel(bool is_reset = false);

	/** Enable/Disable Buttons while trying to remap an other */
	void SwitchButtons(bool is_enabled);

	/** Save the current Button Mapping to the current Pad Handler Config File */
	void SaveButtons();

	/** Get the name of a Qt Key Sequence. Only for keyboard handler */
	const QString GetKeyName(const u32 keyCode);
};
