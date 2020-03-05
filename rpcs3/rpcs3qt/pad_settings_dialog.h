#pragma once

#include <QButtonGroup>
#include <QDialog>
#include <QLabel>
#include <QTabWidget>
#include <QTimer>

#include "Emu/Io/pad_config.h"
#include "Emu/GameInfo.h"

class PadHandlerBase;

namespace Ui
{
	class pad_settings_dialog;
}

struct pad_device_info
{
	std::string name;
	bool is_connected{false};
};

Q_DECLARE_METATYPE(pad_device_info)

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

		id_led,
		id_reset_parameters,
		id_blacklist,
		id_refresh,
		id_add_profile,
		id_ok,
		id_cancel
	};

	struct pad_button
	{
		cfg::string* cfg_name = nullptr;
		std::string key;
		QString text;
	};

	const QString Disconnected_suffix = tr(" (disconnected)");

public:
	explicit pad_settings_dialog(QWidget *parent = nullptr, const GameInfo *game = nullptr);
	~pad_settings_dialog();

public Q_SLOTS:
	void apply_led_settings(int colorR, int colorG, int colorB, bool led_low_battery_blink, bool led_battery_indicator, int led_battery_indicator_brightness);

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
	std::string m_title_id;

	// TabWidget
	QTabWidget* m_tabs = nullptr;

	// Capabilities
	bool m_enable_buttons{ false };
	bool m_enable_rumble{ false };
	bool m_enable_deadzones{ false };
	bool m_enable_led{ false };
	bool m_enable_battery{ false };

	// Button Mapping
	QButtonGroup* m_padButtons = nullptr;
	u32 m_button_id = id_pad_begin;
	std::map<int /*id*/, pad_button /*info*/> m_cfg_entries;

	// Real time stick values
	int lx = 0;
	int ly = 0;
	int rx = 0;
	int ry = 0;

	// Rumble
	s32 m_min_force = 0;
	s32 m_max_force = 0;

	// Backup for standard button palette
	QPalette m_palette;

	// Pad Handlers
	std::shared_ptr<PadHandlerBase> m_handler;
	pad_config m_handler_cfg;
	std::string m_device_name;
	std::string m_profile;
	QTimer m_timer_pad_refresh;

	// Remap Timer
	const int MAX_SECONDS = 5;
	int m_seconds = MAX_SECONDS;
	QTimer m_timer;

	// Mouse Move
	QPoint m_last_pos;

	// Input timer. Its Callback handles the input
	QTimer m_timer_input;

	// Set vibrate data while keeping the current color
	void SetPadData(u32 large_motor, u32 small_motor);

	/** Update all the Button Labels with current button mapping */
	void UpdateLabel(bool is_reset = false);
	void SwitchPadInfo(const std::string& name, bool is_connected);

	/** Enable/Disable Buttons while trying to remap an other */
	void SwitchButtons(bool is_enabled);

	/** Resets the view to default. Resets the Remap Timer */
	void ReactivateButtons();

	void InitButtons();
	void ReloadButtons();

	void ChangeProfile();

	/** Repaints a stick deadzone preview label */
	void RepaintPreviewLabel(QLabel* l, int deadzone, int desired_width, int x, int y);

	std::shared_ptr<PadHandlerBase> GetHandler(pad_handler type);

protected:
	/** Handle keyboard handler input */
	void keyPressEvent(QKeyEvent *keyEvent) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;
	bool eventFilter(QObject* object, QEvent* event) override;
};
