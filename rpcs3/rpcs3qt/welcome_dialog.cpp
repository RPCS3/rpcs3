#include "welcome_dialog.h"
#include "ui_welcome_dialog.h"

#include "gui_settings.h"

#include <QPushButton>
#include <QCheckBox>

welcome_dialog::welcome_dialog(QWidget* parent) : QDialog(parent), ui(new Ui::welcome_dialog)
{
	ui->setupUi(this);

	setWindowFlags(windowFlags() & Qt::WindowTitleHint);

	gui_settings* settings = new gui_settings(this);

	ui->okay->setEnabled(false);

	connect(ui->i_have_read, &QCheckBox::clicked, [=, this](bool checked)
	{
		ui->okay->setEnabled(checked);
	});

	connect(ui->do_not_show, &QCheckBox::clicked, [=, this](bool checked)
	{
		settings->SetValue(gui::ib_show_welcome, QVariant(!checked));
	});

	connect(ui->okay, &QPushButton::clicked, this, &QDialog::accept);

	layout()->setSizeConstraint(QLayout::SetFixedSize);
}

welcome_dialog::~welcome_dialog()
{
	delete ui;
}
