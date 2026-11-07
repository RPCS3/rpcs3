#include "stdafx.h"
#include "pad_skateboard_settings_dialog.h"
#include "ui_pad_skateboard_settings_dialog.h"

#include <QPainter>
#include <QSvgRenderer>

pad_skateboard_settings_dialog::pad_skateboard_settings_dialog(QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::pad_skateboard_settings_dialog)
{
	ui->setupUi(this);

	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

	// Prevent the close button from consuming keys like Space or Enter while remapping a button
	for (QAbstractButton* button : ui->buttonBox->buttons())
	{
		button->setFocusPolicy(Qt::NoFocus);
	}

	// Paint the skateboard (top view, nose pointing up)
	QSvgRenderer renderer(QStringLiteral(":/Icons/Skateboard.svg"));
	QPixmap pixmap(renderer.defaultSize() * 4);
	pixmap.fill(Qt::transparent);
	QPainter painter(&pixmap);
	painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
	renderer.render(&painter, pixmap.rect());
	painter.end();
	ui->l_skateboard->setPixmap(pixmap);
}

pad_skateboard_settings_dialog::~pad_skateboard_settings_dialog()
{
}

QPushButton* pad_skateboard_settings_dialog::ir_nose_button() const
{
	return ui->b_ir_nose;
}

QPushButton* pad_skateboard_settings_dialog::ir_tail_button() const
{
	return ui->b_ir_tail;
}

QPushButton* pad_skateboard_settings_dialog::ir_left_button() const
{
	return ui->b_ir_left;
}

QPushButton* pad_skateboard_settings_dialog::ir_right_button() const
{
	return ui->b_ir_right;
}

QPushButton* pad_skateboard_settings_dialog::tilt_left_button() const
{
	return ui->b_tilt_left;
}

QPushButton* pad_skateboard_settings_dialog::tilt_right_button() const
{
	return ui->b_tilt_right;
}
