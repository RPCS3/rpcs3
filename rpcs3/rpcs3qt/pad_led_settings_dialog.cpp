#include "pad_led_settings_dialog.h"

#include <QColorDialog>
#include <QPainter>
#include <QPixmap>
#include <QPainterPath>

pad_led_settings_dialog::pad_led_settings_dialog(QDialog* parent, int colorR, int colorG, int colorB, bool has_rgb, bool has_player_led, bool player_led_enabled, bool has_battery, bool has_battery_led, bool led_low_battery_blink, bool led_battery_indicator, int led_battery_indicator_brightness)
    : QDialog(parent)
    , ui(new Ui::pad_led_settings_dialog)
    , m_initial{colorR, colorG, colorB, player_led_enabled, led_low_battery_blink, led_battery_indicator, led_battery_indicator_brightness}
{
	ui->setupUi(this);
	setModal(true);
	m_new = m_initial;

	ui->hs_indicator_brightness->setValue(m_new.battery_indicator_brightness);
	ui->cb_led_blink->setChecked(m_new.low_battery_blink);
	ui->cb_led_indicate->setChecked(m_new.battery_indicator);
	ui->cb_player_led->setChecked(m_new.player_led_enabled);

	update_slider_label(m_new.battery_indicator_brightness);

	ui->gb_player_led->setEnabled(has_player_led);
	ui->gb_led_color->setEnabled(has_rgb);
	ui->gb_battery_status->setEnabled(has_battery && has_battery_led);
	ui->gb_indicator_brightness->setEnabled(has_battery && has_battery_led && has_rgb); // Let's restrict this to rgb capable devices for now

	connect(ui->bb_dialog_buttons, &QDialogButtonBox::clicked, [this](QAbstractButton* button)
	{
		if (button == ui->bb_dialog_buttons->button(QDialogButtonBox::Ok))
		{
			read_form_values();
			accept();
		}
		else if (button == ui->bb_dialog_buttons->button(QDialogButtonBox::Cancel))
		{
			m_new = m_initial;
			reject();
		}
		else if (button == ui->bb_dialog_buttons->button(QDialogButtonBox::Apply))
		{
			read_form_values();
		}
		Q_EMIT pass_led_settings(m_new);
	});

	if (has_rgb)
	{
		connect(ui->b_colorpicker, &QPushButton::clicked, [this]()
		{
			const QColor led_color(m_new.color_r, m_new.color_g, m_new.color_b);
			QColorDialog dlg(led_color, this);
			dlg.setWindowTitle(tr("LED Color"));
			if (dlg.exec() == QColorDialog::Accepted)
			{
				const QColor new_color = dlg.selectedColor();
				m_new.color_r = new_color.red();
				m_new.color_g = new_color.green();
				m_new.color_b = new_color.blue();
				redraw_color_sample();
			}
		});

		if (ui->gb_battery_status->isEnabled())
		{
			connect(ui->hs_indicator_brightness, &QAbstractSlider::valueChanged, this, &pad_led_settings_dialog::update_slider_label);
		}

		if (ui->cb_led_indicate->isEnabled())
		{
			connect(ui->cb_led_indicate, &QCheckBox::toggled, this, &pad_led_settings_dialog::battery_indicator_checked);
		}

		battery_indicator_checked(ui->cb_led_indicate->isChecked());
	}

	// Draw color sample after showing the dialog, in order to have proper dimensions
	show();
	redraw_color_sample();
	setFixedSize(size());
}

pad_led_settings_dialog::~pad_led_settings_dialog()
{
}

void pad_led_settings_dialog::redraw_color_sample() const
{
	const qreal dpr = devicePixelRatioF();
	const qreal w = ui->w_color_sample->width();
	const qreal h = ui->w_color_sample->height();
	const qreal padding = 5;
	const qreal radius = 5;

	// Create the canvas for our color sample widget
	QPixmap color_sample(w * dpr, h * dpr);
	color_sample.setDevicePixelRatio(dpr);
	color_sample.fill(Qt::transparent);

	// Create the shape for our color sample widget
	QPainterPath path;
	path.addRoundedRect(QRectF(padding, padding, w - padding * 2, h - padding * 2), radius, radius);

	// Get new LED color
	const QColor led_color(m_new.color_r, m_new.color_g, m_new.color_b);

	// Paint the shape with a black border and fill it with the LED color
	QPainter painter(&color_sample);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setPen(QPen(Qt::black, 1));
	painter.fillPath(path, led_color);
	painter.drawPath(path);
	painter.end();

	// Update the color sample widget
	ui->w_color_sample->setPixmap(color_sample.scaled(w * dpr, h * dpr, Qt::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation));
}

void pad_led_settings_dialog::update_slider_label(int val) const
{
	const QString label = QString::number(val) + QStringLiteral("%");
	ui->l_indicator_brightness->setText(label);
}

void pad_led_settings_dialog::battery_indicator_checked(bool checked) const
{
	ui->gb_indicator_brightness->setEnabled(checked);
}

void pad_led_settings_dialog::read_form_values()
{
	m_new.player_led_enabled = ui->cb_player_led->isChecked();
	m_new.low_battery_blink = ui->cb_led_blink->isChecked();
	m_new.battery_indicator = ui->cb_led_indicate->isChecked();
	m_new.battery_indicator_brightness = ui->hs_indicator_brightness->value();
}
