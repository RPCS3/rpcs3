#pragma once

#include <QButtonGroup>
#include <QDialog>
#include <QEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QTimer>

#include "keyboard_pad_handler.h"
#include "Utilities/types.h"
#include "Utilities/Config.h"
#include "Emu/Io/PadHandler.h"
#include "stdafx.h"
#include "Emu/System.h"

#ifdef _WIN32
#include "xinput_pad_handler.h"
#endif
#ifdef _MSC_VER
#include "mm_joystick_handler.h"
#endif
#ifdef HAVE_LIBEVDEV
#include "evdev_joystick_handler.h"
#endif
#include "ds4_pad_handler.h"

namespace Ui
{
	class pad_settings_dialog;
}

class pad_settings_dialog : public QDialog
{
	Q_OBJECT

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
		id_pad_ps,

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
		id_blacklist,
		id_ok,
		id_cancel
	};

	struct pad_button
	{
		cfg::string* cfg_name;
		std::string key;
		QString text;
	};

private Q_SLOTS:
	void OnPadButtonClicked(int id);

private:
	Ui::pad_settings_dialog *ui;

	// Button Mapping
	QButtonGroup* m_padButtons;
	u32 m_button_id = id_pad_begin;
	std::map<int /*id*/, pad_button /*info*/> m_cfg_entries;

	// Real time stick values
	int lx = 0;
	int ly = 0;
	int rx = 0;
	int ry = 0;

	// Backup for standard button palette
	QPalette m_palette;

	// Pad Handlers 
	pad_handler m_handler_type;
	std::shared_ptr<PadHandlerBase> m_handler;
	pad_config m_handler_cfg;
	std::string m_device_name;

	// Remap Timer
	const int MAX_SECONDS = 5;
	int m_seconds = MAX_SECONDS;
	QTimer m_timer;

	// Input timer. Its Callback handles the input
	QTimer m_timer_input;

	/** Resets the view to default. Resets the Remap Timer */
	void ReactivateButtons();

	/** Repaints a stick deadzone preview label */
	void RepaintPreviewLabel(QLabel* l, int dz, int w, int x, int y);

public:
	explicit pad_settings_dialog(const std::string& device, const std::string& profile, std::shared_ptr<PadHandlerBase> handler, QWidget *parent = nullptr);
	~pad_settings_dialog();

	/** Handle keyboard handler input */
	void keyPressEvent(QKeyEvent *keyEvent) override;
	void mousePressEvent(QMouseEvent *event) override;
	bool eventFilter(QObject* object, QEvent* event) override;

	/** Update all the Button Labels with current button mapping */
	void UpdateLabel(bool is_reset = false);

	/** Enable/Disable Buttons while trying to remap an other */
	void SwitchButtons(bool is_enabled);

	/** Save the Pad Configuration to the current Pad Handler Config File */
	void SaveConfig();
};
