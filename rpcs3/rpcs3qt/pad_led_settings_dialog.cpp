#include "pad_led_settings_dialog.h"

#include <QColorDialog>
#include <QPainter>
#include <QPixmap>

pad_led_settings_dialog::pad_led_settings_dialog(const int& colorR, const int& colorG, const int& colorB, const bool& led_low_battery_blink, const bool& led_battery_indicator, const int& led_battery_indicator_brightness, QDialog * parent)
    : QDialog(parent)
    , ui(new Ui::pad_led_settings_dialog)
    , m_initial{colorR, colorG, colorB, led_low_battery_blink, led_battery_indicator, led_battery_indicator_brightness}
{
	ui->setupUi(this);
	setFixedSize(size());
	m_new = m_initial;
	redraw_color_sample();
	ui->hs_indicator_brightness->setValue(m_new.battery_indicator_brightness);
	ui->cb_led_blink->setChecked(m_new.low_battery_blink);
	ui->cb_led_indicate->setChecked(m_new.battery_indicator);
	switch_groupboxes(m_new.battery_indicator);
	update_slider_label(m_new.battery_indicator_brightness);
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
		Q_EMIT pass_led_settings(m_new.cR, m_new.cG, m_new.cB, m_new.low_battery_blink, m_new.battery_indicator, m_new.battery_indicator_brightness);
	}); 
	connect(ui->b_colorpicker, &QPushButton::clicked, [this]()
	{
		const QColor led_color(m_new.cR, m_new.cG, m_new.cB);
		QColorDialog dlg(led_color, this);
		dlg.setWindowTitle(tr("LED Color"));
		if (dlg.exec() == QColorDialog::Accepted)
		{
			const QColor new_color = dlg.selectedColor();
			m_new.cR = new_color.red();
			m_new.cG = new_color.green();
			m_new.cB = new_color.blue();
			redraw_color_sample();
		}
	});
	connect(ui->hs_indicator_brightness, &QAbstractSlider::valueChanged, this, &pad_led_settings_dialog::update_slider_label);
	connect(ui->cb_led_indicate, &QCheckBox::toggled, this, &pad_led_settings_dialog::switch_groupboxes);
}

pad_led_settings_dialog::~pad_led_settings_dialog()
{
	delete ui;
}

void pad_led_settings_dialog::redraw_color_sample()
{
	const qreal w = ui->w_color_sample->width();
	const qreal h = ui->w_color_sample->height();
	const qreal padding = 5;
	const qreal radius = 5;
	QColor led_color;
	QPixmap color_sample(w, h);
	color_sample.fill(QColor("transparent"));	
	QPainter painter(&color_sample);
	QPainterPath path;
	QPen border(Qt::black, 1);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setPen(border);
	led_color.setRgb(m_new.cR, m_new.cG, m_new.cB);
	path.addRoundedRect(QRectF(padding, padding, w - padding * 2, h - padding * 2), radius, radius);
	painter.fillPath(path, led_color);
	painter.drawPath(path);
	ui->w_color_sample->setPixmap(color_sample);
}

void pad_led_settings_dialog::update_slider_label(int val)
{
	const QString label = QString::number(val) + QStringLiteral("%");
	ui->l_indicator_brightness->setText(label);
}

void pad_led_settings_dialog::switch_groupboxes(bool tick)
{
	ui->gb_indicator_brightness->setEnabled(tick);
}

void pad_led_settings_dialog::read_form_values()
{
	m_new.low_battery_blink = ui->cb_led_blink->isChecked();
	m_new.battery_indicator = ui->cb_led_indicate->isChecked();
	m_new.battery_indicator_brightness = ui->hs_indicator_brightness->value();
}
