#include "stdafx.h"
#include "wiimote_settings_dialog.h"
#include "Emu/System.h"
#include "Emu/Io/WiimoteManager.h"
#include <QTimer>
#include <QPainter>
#include <QPixmap>

wiimote_settings_dialog::wiimote_settings_dialog(QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::wiimote_settings_dialog)
{
	ui->setupUi(this);
	update_list();
	connect(ui->scanButton, &QPushButton::clicked, this, [this] { update_list(); });

	QTimer* timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &wiimote_settings_dialog::update_state);
	timer->start(50);
}

wiimote_settings_dialog::~wiimote_settings_dialog() = default;

void wiimote_settings_dialog::update_state()
{
	int index = ui->wiimoteList->currentRow();
	auto* wm = WiimoteManager::get_instance();
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
	ui->wiimoteList->clear();
	auto* wm = WiimoteManager::get_instance();
	if (!wm)
	{
		ui->wiimoteList->addItem(tr("Wiimote Manager not initialized."));
		return;
	}

	size_t count = wm->get_device_count();
	if (count == 0)
	{
		ui->wiimoteList->addItem(tr("No Wiimotes found."));
	}
	else
	{
		for (size_t i = 0; i < count; i++)
		{
			ui->wiimoteList->addItem(tr("Wiimote #%1").arg(i + 1));
		}
		ui->wiimoteList->setCurrentRow(0);
	}
}
