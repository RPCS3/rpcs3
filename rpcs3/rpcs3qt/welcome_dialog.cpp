#include "welcome_dialog.h"
#include "ui_welcome_dialog.h"

#include "gui_settings.h"

#include "Emu/System.h"

#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

welcome_dialog::welcome_dialog(QWidget* parent) : QDialog(parent), ui(new Ui::welcome_dialog)
{
	ui->setupUi(this);

	setWindowFlags(Qt::WindowTitleHint);

	gui_settings* settings = new gui_settings(this);

	ui->okay->setEnabled(false);

	connect(ui->i_have_read, &QCheckBox::clicked, [=](bool checked)
	{
		ui->okay->setEnabled(checked);
	});

	connect(ui->do_not_show, &QCheckBox::clicked, [=](bool checked)
	{
		settings->SetValue(GUI::ib_show_welcome, QVariant(!checked));
	});

	connect(ui->okay, &QPushButton::pressed, this, &QDialog::accept);

	layout()->setSizeConstraint(QLayout::SetFixedSize);
}
