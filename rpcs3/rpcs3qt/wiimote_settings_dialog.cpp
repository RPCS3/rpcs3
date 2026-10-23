#include "stdafx.h"
#include "wiimote_settings_dialog.h"
#include "Input/wiimote_handler.h"
#include "Emu/Io/wiimote_config.h"
#include <QTimer>
#include <QPainter>
#include <QPixmap>
#include <QCheckBox>
#include <QPushButton>

wiimote_settings_dialog::wiimote_settings_dialog(QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::wiimote_settings_dialog)
{
	ui->setupUi(this);

	m_boxes = {
		ui->cb_trigger, ui->cb_a1, ui->cb_a2, ui->cb_c1,
		ui->cb_b1, ui->cb_b2, ui->cb_b3, ui->cb_a3, ui->cb_c2
	};

	ui->useForGunCon->setChecked(get_wiimote_config().use_for_guncon.get());
	connect(ui->useForGunCon, &QCheckBox::toggled, this, [](bool checked)
	{
		get_wiimote_config().use_for_guncon.set(checked);
	});

	update_list();
	connect(ui->buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this, &wiimote_settings_dialog::restore_defaults);

	connect(this, &QDialog::finished, this, []()
	{
		get_wiimote_config().save();
	});

	// Timer updates both state AND device list (auto-refresh)
	QTimer* timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &wiimote_settings_dialog::update_state);
	connect(timer, &QTimer::timeout, this, &wiimote_settings_dialog::update_list);
	timer->start(50);

	populate_mappings();
}

void wiimote_settings_dialog::populate_mappings()
{
	const auto& cfg = get_wiimote_config().guncon_mapping;

	const std::array<std::pair<QString, wiimote_button>, 12> buttons = {
		{ { tr("None"), wiimote_button::None },
		{ tr("A"), wiimote_button::A },
		{ tr("B"), wiimote_button::B },
		{ tr("Plus (+)"), wiimote_button::Plus },
		{ tr("Minus (-)"), wiimote_button::Minus },
		{ tr("Home"), wiimote_button::Home },
		{ tr("1"), wiimote_button::One },
		{ tr("2"), wiimote_button::Two },
		{ tr("D-Pad Up"), wiimote_button::Up },
		{ tr("D-Pad Down"), wiimote_button::Down },
		{ tr("D-Pad Left"), wiimote_button::Left },
		{ tr("D-Pad Right"), wiimote_button::Right } }
	};

	const std::array<wiimote_button, 9> targets = {
		cfg.trigger.get(), cfg.a1.get(), cfg.a2.get(), cfg.c1.get(),
		cfg.b1.get(), cfg.b2.get(), cfg.b3.get(), cfg.a3.get(), cfg.c2.get()
	};

	ensure(m_boxes.size() == targets.size());

	for (usz i = 0; i < m_boxes.size(); ++i)
	{
		m_boxes[i]->setMinimumWidth(150); // Make combo boxes wider for better readability

		for (const auto& [name, btn] : buttons)
		{
			m_boxes[i]->addItem(name, QVariant::fromValue(static_cast<u16>(btn)));
		}

		// Set current selection
		const int index = m_boxes[i]->findData(QVariant::fromValue(static_cast<u16>(targets[i])));
		if (index >= 0) m_boxes[i]->setCurrentIndex(index);

		// Connect change signal
		connect(m_boxes[i], &QComboBox::currentIndexChanged, this, [this](int)
		{
			apply_mappings();
		});
	}
}

void wiimote_settings_dialog::restore_defaults()
{
	// Reset to default mapping
	get_wiimote_config().from_default();

	ui->useForGunCon->setChecked(get_wiimote_config().use_for_guncon.get());

	// Update UI
	for (auto* box : m_boxes) box->blockSignals(true);

	const auto& cfg = get_wiimote_config().guncon_mapping;
	const std::array<wiimote_button, 9> targets = {
		cfg.trigger.get(), cfg.a1.get(), cfg.a2.get(), cfg.c1.get(),
		cfg.b1.get(), cfg.b2.get(), cfg.b3.get(), cfg.a3.get(), cfg.c2.get()
	};

	ensure(m_boxes.size() == targets.size());

	for (usz i = 0; i < m_boxes.size(); ++i)
	{
		const int index = m_boxes[i]->findData(QVariant::fromValue(static_cast<u16>(targets[i])));
		if (index >= 0) m_boxes[i]->setCurrentIndex(index);
	}

	for (auto* box : m_boxes) box->blockSignals(false);
}

void wiimote_settings_dialog::apply_mappings()
{
	auto& cfg = get_wiimote_config().guncon_mapping;
	const std::array<cfg::_enum<wiimote_button>*, 9> targets = {
		&cfg.trigger, &cfg.a1, &cfg.a2, &cfg.c1,
		&cfg.b1, &cfg.b2, &cfg.b3, &cfg.a3, &cfg.c2
	};

	ensure(m_boxes.size() == targets.size());

	for (usz i = 0; i < m_boxes.size(); ++i)
	{
		targets[i]->set(static_cast<wiimote_button>(m_boxes[i]->currentData().toUInt()));
	}
}

void wiimote_settings_dialog::update_state()
{
	const int index = ui->wiimoteList->currentRow();
	auto* wm = wiimote_handler::get_instance();
	const auto running = wm && wm->is_running();
	const auto states = running ? wm->get_states() : std::vector<wiimote_state>{};

	if (!running || index < 0 || static_cast<usz>(index) >= states.size())
	{
		ui->connectionStatus->setText(tr("N/A"));
		ui->buttonState->setText(tr("N/A"));
		ui->irData->setText(tr("N/A"));
		return;
	}

	const auto& state = states[index];
	ui->connectionStatus->setText(state.connected ? tr("Connected") : tr("Disconnected"));

	QStringList pressed_buttons;
	const auto is_pressed = [&](wiimote_button btn) { return (state.buttons & static_cast<u16>(btn)) != 0; };
	if (is_pressed(wiimote_button::Left)) pressed_buttons << tr("Left");
	if (is_pressed(wiimote_button::Right)) pressed_buttons << tr("Right");
	if (is_pressed(wiimote_button::Down)) pressed_buttons << tr("Down");
	if (is_pressed(wiimote_button::Up)) pressed_buttons << tr("Up");
	if (is_pressed(wiimote_button::Plus)) pressed_buttons << tr("Plus");
	if (is_pressed(wiimote_button::Two)) pressed_buttons << tr("2");
	if (is_pressed(wiimote_button::One)) pressed_buttons << tr("1");
	if (is_pressed(wiimote_button::B)) pressed_buttons << tr("B");
	if (is_pressed(wiimote_button::A)) pressed_buttons << tr("A");
	if (is_pressed(wiimote_button::Minus)) pressed_buttons << tr("Minus");
	if (is_pressed(wiimote_button::Home)) pressed_buttons << tr("Home");

	QString button_text = QString("0x%1").arg(state.buttons, 4, 16, QChar('0')).toUpper();
	if (!pressed_buttons.isEmpty())
	{
		button_text += " (" + pressed_buttons.join(", ") + ")";
	}
	ui->buttonState->setText(button_text);

	QString ir_text;
	const int w = ui->irVisual->width();
	const int h = ui->irVisual->height();
	QPixmap pixmap(w, h);
	pixmap.fill(Qt::transparent);
	QPainter painter(&pixmap);
	painter.setRenderHint(QPainter::Antialiasing);

	// Calculate 4:3 drawing area (Wiimote IR space is 1024x768)
	int draw_w, draw_h;
	if (w * 3 > h * 4) // wider than 4:3
	{
		draw_h = h;
		draw_w = h * 4 / 3;
	}
	else
	{
		draw_w = w;
		draw_h = w * 3 / 4;
	}
	const int offset_x = (w - draw_w) / 2;
	const int offset_y = (h - draw_h) / 2;

	// Draw center crosshair in the 4:3 area
	painter.setPen(QPen(Qt::darkGray, 1, Qt::DashLine));
	painter.drawLine(offset_x + draw_w / 2, offset_y, offset_x + draw_w / 2, offset_y + draw_h);
	painter.drawLine(offset_x, offset_y + draw_h / 2, offset_x + draw_w, offset_y + draw_h / 2);

	static const std::array<QColor, MAX_WIIMOTE_IR_POINTS> colors = { Qt::red, Qt::green, Qt::blue, Qt::yellow };

	for (usz i = 0; i < state.ir.size(); ++i)
	{
		if (state.ir[i].size > 0 && state.ir[i].x < 1023 && state.ir[i].y < 1023)
		{
			ir_text += QString("[%1: %2, %3] ").arg(i).arg(state.ir[i].x).arg(state.ir[i].y);

			// Map 0..1023 X and 0..767 Y to 4:3 drawing area
			const float x = offset_x + ((1023.0f - state.ir[i].x) / 1023.0f) * draw_w;
			const float y = offset_y + (state.ir[i].y / 767.0f) * draw_h;

			painter.setPen(colors[i]);
			painter.setBrush(colors[i]);
			painter.drawEllipse(QPointF(x, y), 4, 4);
			painter.drawText(QPointF(x + 6, y + 6), QString::number(i));
		}
	}
	ui->irVisual->setPixmap(pixmap);
	ui->irData->setText(ir_text.isEmpty() ? tr("No IR data") : ir_text);
}

void wiimote_settings_dialog::update_list()
{
	auto* wm = wiimote_handler::get_instance();
	if (!wm || !wm->is_running())
	{
		const QString text = tr("Wiimote Manager not initialized.");

		if (ui->wiimoteList->count() != 1 || ui->wiimoteList->item(0)->text() != text)
		{
			ui->wiimoteList->clear();
			ui->wiimoteList->addItem(text);
		}
		return;
	}

	const auto states = wm->get_states();

	if (states.empty())
	{
		const QString text = tr("No Wiimotes found.");
		if (ui->wiimoteList->count() != 1 || ui->wiimoteList->item(0)->text() != text)
		{
			ui->wiimoteList->clear();
			ui->wiimoteList->addItem(text);
		}
		return;
	}

	// Only update if the list count changed (avoid flicker)
	if (static_cast<usz>(ui->wiimoteList->count()) != states.size())
	{
		const int current_row = ui->wiimoteList->currentRow();
		ui->wiimoteList->clear();

		for (usz i = 0; i < states.size(); i++)
		{
			QString label = tr("Wiimote #%1").arg(i + 1);
			if (!states[i].connected) label += " (" + tr("Disconnected") + ")";
			ui->wiimoteList->addItem(label);
		}

		if (current_row >= 0 && current_row < ui->wiimoteList->count())
		{
			ui->wiimoteList->setCurrentRow(current_row);
		}
		else if (ui->wiimoteList->count() > 0)
		{
			ui->wiimoteList->setCurrentRow(0);
		}
	}
	else
	{
		for (usz i = 0; i < std::min(static_cast<usz>(ui->wiimoteList->count()), states.size()); i++)
		{
			QListWidgetItem* item = ui->wiimoteList->item(static_cast<int>(i));
			if (!item) continue;
			QString label = tr("Wiimote #%1").arg(i + 1);
			if (!states[i].connected) label += " (" + tr("Disconnected") + ")";
			if (item->text() != label)
			{
				item->setText(label);
			}
		}
	}
}
