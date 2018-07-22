#pragma once

#include <QButtonGroup>
#include <QDialog>
#include <QEvent>
#include <QLabel>
#include <QTabWidget>
#include <QTimer>

#include "Emu/Io/PadHandler.h"

namespace Ui
{
	class pad_settings_dialog;
}

class pad_settings_dialog : public QDialog
{
	Q_OBJECT

	const int MAX_PLAYERS = 7;

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
		id_refresh,
		id_ok,
		id_cancel
	};

	struct pad_button
	{
		cfg::string* cfg_name;
		std::string key;
		QString text;
	};

public:
	explicit pad_settings_dialog(QWidget *parent = nullptr);
	~pad_settings_dialog();

private Q_SLOTS:
	void OnPadButtonClicked(int id);
	void OnTabChanged(int index);
	void RefreshInputTypes();
	void ChangeInputType();
	/** Save the Pad Configuration to the current Pad Handler Config File */
	void SaveProfile();
	void SaveExit();
	void CancelExit();

private:
	Ui::pad_settings_dialog *ui;

	// TabWidget
	QTabWidget* m_tabs;

	// Button Mapping
	QButtonGroup* m_padButtons;
	u32 m_button_id = id_pad_begin;
	std::map<int /*id*/, pad_button /*info*/> m_cfg_entries;

	// Real time stick values
	int lx = 0;
	int ly = 0;
	int rx = 0;
	int ry = 0;

	// Rumble
	s32 m_min_force;
	s32 m_max_force;

	// Backup for standard button palette
	QPalette m_palette;

	// Pad Handlers
	std::shared_ptr<PadHandlerBase> m_handler;
	pad_config m_handler_cfg;
	std::string m_device_name;
	std::string m_profile;

	// Remap Timer
	const int MAX_SECONDS = 5;
	int m_seconds = MAX_SECONDS;
	QTimer m_timer;

	// Input timer. Its Callback handles the input
	QTimer m_timer_input;

	/** Update all the Button Labels with current button mapping */
	void UpdateLabel(bool is_reset = false);

	/** Enable/Disable Buttons while trying to remap an other */
	void SwitchButtons(bool is_enabled);

	/** Resets the view to default. Resets the Remap Timer */
	void ReactivateButtons();

	void InitButtons();
	void ReloadButtons();

	void ChangeProfile();

	/** Repaints a stick deadzone preview label */
	void RepaintPreviewLabel(QLabel* l, int dz, int w, int x, int y);

	std::shared_ptr<PadHandlerBase> GetHandler(pad_handler type);

protected:
	/** Handle keyboard handler input */
	void keyPressEvent(QKeyEvent *keyEvent) override;
	void mousePressEvent(QMouseEvent *event) override;
	bool eventFilter(QObject* object, QEvent* event) override;
};
