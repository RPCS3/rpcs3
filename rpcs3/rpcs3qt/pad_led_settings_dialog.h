#pragma once

#include <QDialog>

#include "ui_pad_led_settings_dialog.h"

namespace Ui
{
	class pad_led_settings_dialog;
}

class pad_led_settings_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit pad_led_settings_dialog(QDialog* parent, int colorR, int colorG, int colorB, bool has_rgb, bool has_player_led, bool player_led_enabled, bool has_battery, bool has_battery_led, bool led_low_battery_blink, bool led_battery_indicator, int led_battery_indicator_brightness);
	~pad_led_settings_dialog();

	struct led_settings
	{
		int color_r = 255;
		int color_g = 255;
		int color_b = 255;
		bool player_led_enabled = true;
		bool low_battery_blink = true;
		bool battery_indicator = false;
		int battery_indicator_brightness = 50;
	};

Q_SIGNALS:
	void pass_led_settings(const led_settings& settings);

private Q_SLOTS:
	void update_slider_label(int val) const;
	void battery_indicator_checked(bool checked) const;

private:
	void redraw_color_sample() const;
	void read_form_values();
	std::unique_ptr<Ui::pad_led_settings_dialog> ui;
	led_settings m_initial;
	led_settings m_new;
};
