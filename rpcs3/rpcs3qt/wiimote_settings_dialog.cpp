#include "stdafx.h"
#include "wiimote_settings_dialog.h"
#include "Emu/System.h"
#include "Input/wiimote_handler.h"
#include <QTimer>
#include <QPainter>
#include <QPixmap>

wiimote_settings_dialog::wiimote_settings_dialog(QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::wiimote_settings_dialog)
{
	ui->setupUi(this);
	update_list();
	connect(ui->restoreDefaultsButton, &QPushButton::clicked, this, &wiimote_settings_dialog::restore_defaults);

	// Timer updates both state AND device list (auto-refresh)
	QTimer* timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &wiimote_settings_dialog::update_state);
	connect(timer, &QTimer::timeout, this, &wiimote_settings_dialog::update_list);
	timer->start(50);

	populate_mappings();
}

wiimote_settings_dialog::~wiimote_settings_dialog() = default;

void wiimote_settings_dialog::populate_mappings()
{
	auto* wm = wiimote_handler::get_instance();
	if (!wm) return;

	const QPair<QString, wiimote_button> buttons[] = {
		{ tr("None"), wiimote_button::None },
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
		{ tr("D-Pad Right"), wiimote_button::Right },
	};

	QComboBox* boxes[] = {
		ui->cb_trigger, ui->cb_a1, ui->cb_a2, ui->cb_c1,
		ui->cb_b1, ui->cb_b2, ui->cb_b3, ui->cb_a3, ui->cb_c2
	};

	wiimote_guncon_mapping current = wm->get_mapping();
	wiimote_button* targets[] = {
		&current.trigger, &current.a1, &current.a2, &current.c1,
		&current.b1, &current.b2, &current.b3, &current.a3, &current.c2
	};

	for (int i = 0; i < 9; ++i)
	{
		boxes[i]->setMinimumWidth(150); // Make combo boxes wider for better readability

		for (const auto& pair : buttons)
		{
			boxes[i]->addItem(pair.first, QVariant::fromValue(static_cast<u16>(pair.second)));
		}

		// Set current selection
		int index = boxes[i]->findData(QVariant::fromValue(static_cast<u16>(*targets[i])));
		if (index >= 0) boxes[i]->setCurrentIndex(index);

		// Connect change signal
		connect(boxes[i], QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
			apply_mappings();
		});
	}
}

void wiimote_settings_dialog::restore_defaults()
{
	auto* wm = wiimote_handler::get_instance();
	if (!wm) return;

	// Reset to default mapping
	wiimote_guncon_mapping default_map;
	wm->set_mapping(default_map);

	// Update UI
	ui->cb_trigger->blockSignals(true);
	ui->cb_a1->blockSignals(true);
	ui->cb_a2->blockSignals(true);
	ui->cb_c1->blockSignals(true);
	ui->cb_b1->blockSignals(true);
	ui->cb_b2->blockSignals(true);
	ui->cb_b3->blockSignals(true);
	ui->cb_a3->blockSignals(true);
	ui->cb_c2->blockSignals(true);

	auto set_box = [](QComboBox* box, wiimote_button btn) {
		int index = box->findData(QVariant::fromValue(static_cast<u16>(btn)));
		if (index >= 0) box->setCurrentIndex(index);
	};

	set_box(ui->cb_trigger, default_map.trigger);
	set_box(ui->cb_a1, default_map.a1);
	set_box(ui->cb_a2, default_map.a2);
	set_box(ui->cb_c1, default_map.c1);
	set_box(ui->cb_b1, default_map.b1);
	set_box(ui->cb_b2, default_map.b2);
	set_box(ui->cb_b3, default_map.b3);
	set_box(ui->cb_a3, default_map.a3);
	set_box(ui->cb_c2, default_map.c2);

	ui->cb_trigger->blockSignals(false);
	ui->cb_a1->blockSignals(false);
	ui->cb_a2->blockSignals(false);
	ui->cb_c1->blockSignals(false);
	ui->cb_b1->blockSignals(false);
	ui->cb_b2->blockSignals(false);
	ui->cb_b3->blockSignals(false);
	ui->cb_a3->blockSignals(false);
	ui->cb_c2->blockSignals(false);
}

void wiimote_settings_dialog::apply_mappings()
{
	auto* wm = wiimote_handler::get_instance();
	if (!wm) return;

	wiimote_guncon_mapping map;
	auto get_btn = [](QComboBox* box) {
		return static_cast<wiimote_button>(box->currentData().toUInt());
	};

	map.trigger = get_btn(ui->cb_trigger);
	map.a1 = get_btn(ui->cb_a1);
	map.a2 = get_btn(ui->cb_a2);
	map.c1 = get_btn(ui->cb_c1);
	map.b1 = get_btn(ui->cb_b1);
	map.b2 = get_btn(ui->cb_b2);
	map.b3 = get_btn(ui->cb_b3);
	map.a3 = get_btn(ui->cb_a3);
	map.c2 = get_btn(ui->cb_c2);

	// Preserve alts or add UI for them later. For now, keep defaults or sync with main if matched
	// To be safe, we can just leave alts as default Up/Down for now since they are D-Pad shortcuts
	// Or we can reset them if the user maps Up/Down to something else to avoid conflict?
	// For simplicity, let's keep defaults in the struct constructor.

	wm->set_mapping(map);
}

void wiimote_settings_dialog::update_state()
{
	int index = ui->wiimoteList->currentRow();
	auto* wm = wiimote_handler::get_instance();
	if (!wm || index < 0)
	{
		ui->connectionStatus->setText(tr("N/A"));
		ui->buttonState->setText(tr("N/A"));
		ui->irData->setText(tr("N/A"));
		return;
	}

	auto states = wm->get_states();
	if (static_cast<size_t>(index) >= states.size())
	{
		ui->connectionStatus->setText(tr("N/A"));
		ui->buttonState->setText(tr("N/A"));
		ui->irData->setText(tr("N/A"));
		return;
	}

	const auto& state = states[index];
	ui->connectionStatus->setText(state.connected ? tr("Connected") : tr("Disconnected"));

	QStringList pressedButtons;
	if (state.buttons & 0x0001) pressedButtons << tr("Left");
	if (state.buttons & 0x0002) pressedButtons << tr("Right");
	if (state.buttons & 0x0004) pressedButtons << tr("Down");
	if (state.buttons & 0x0008) pressedButtons << tr("Up");
	if (state.buttons & 0x0010) pressedButtons << tr("Plus");
	if (state.buttons & 0x0100) pressedButtons << tr("2");
	if (state.buttons & 0x0200) pressedButtons << tr("1");
	if (state.buttons & 0x0400) pressedButtons << tr("B");
	if (state.buttons & 0x0800) pressedButtons << tr("A");
	if (state.buttons & 0x1000) pressedButtons << tr("Minus");
	if (state.buttons & 0x8000) pressedButtons << tr("Home");

	QString buttonText = QString("0x%1").arg(state.buttons, 4, 16, QChar('0')).toUpper();
	if (!pressedButtons.isEmpty())
	{
		buttonText += " (" + pressedButtons.join(", ") + ")";
	}
	ui->buttonState->setText(buttonText);

	QString irText;
	QPixmap pixmap(ui->irVisual->size());
	pixmap.fill(Qt::black);
	QPainter painter(&pixmap);
	painter.setRenderHint(QPainter::Antialiasing);

	// Draw center crosshair
	painter.setPen(QPen(Qt::darkGray, 1, Qt::DashLine));
	painter.drawLine(pixmap.width() / 2, 0, pixmap.width() / 2, pixmap.height());
	painter.drawLine(0, pixmap.height() / 2, pixmap.width(), pixmap.height() / 2);

	static const QColor colors[] = { Qt::red, Qt::green, Qt::blue, Qt::yellow };

	for (int i = 0; i < 4; ++i)
	{
		if (state.ir[i].size > 0 && state.ir[i].x < 1023 && state.ir[i].y < 1023)
		{
			irText += QString("[%1: %2, %3] ").arg(i).arg(state.ir[i].x).arg(state.ir[i].y);

			// Map 0..1023 X and 0..767 Y to pixmap coordinates
			// Wiimote X/Y are inverted relative to pointing direction
			float x = ((1023 - state.ir[i].x) / 1023.0f) * pixmap.width();
			float y = (state.ir[i].y / 767.0f) * pixmap.height();

			painter.setPen(colors[i]);
			painter.setBrush(colors[i]);
			painter.drawEllipse(QPointF(x, y), 4, 4);
			painter.drawText(QPointF(x + 6, y + 6), QString::number(i));
		}
	}
	ui->irVisual->setPixmap(pixmap);
	ui->irData->setText(irText.isEmpty() ? tr("No IR data") : irText);
}

void wiimote_settings_dialog::update_list()
{
	auto* wm = wiimote_handler::get_instance();
	if (!wm)
	{
		if (ui->wiimoteList->count() != 1 || ui->wiimoteList->item(0)->text() != tr("Wiimote Manager not initialized."))
		{
			ui->wiimoteList->clear();
			ui->wiimoteList->addItem(tr("Wiimote Manager not initialized."));
		}
		return;
	}

	auto states = wm->get_states();

	// Only update if the list content actually changed (avoid flicker)
	if (static_cast<size_t>(ui->wiimoteList->count()) != states.size())
	{
		int current_row = ui->wiimoteList->currentRow();
		ui->wiimoteList->clear();

		for (size_t i = 0; i < states.size(); i++)
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
		// Just update connection status labels without clearing
		for (size_t i = 0; i < states.size(); i++)
		{
			QString label = tr("Wiimote #%1").arg(i + 1);
			if (!states[i].connected) label += " (" + tr("Disconnected") + ")";

			if (static_cast<int>(i) < ui->wiimoteList->count())
			{
				QListWidgetItem* item = ui->wiimoteList->item(static_cast<int>(i));
				if (item && item->text() != label)
				{
					item->setText(label);
				}
			}
		}
	}
}
