#include "welcome_dialog.h"
#include "ui_welcome_dialog.h"

#include "gui_settings.h"

#include <QPushButton>
#include <QCheckBox>
#include <QSvgWidget>

welcome_dialog::welcome_dialog(std::shared_ptr<gui_settings> gui_settings, QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::welcome_dialog)
	, m_gui_settings(std::move(gui_settings))
{
	ui->setupUi(this);

	setWindowFlags(windowFlags() & Qt::WindowTitleHint);

	ui->okay->setEnabled(false);
	ui->icon_label->load(QStringLiteral(":/rpcs3.svg"));

	connect(ui->i_have_read, &QCheckBox::clicked, [this](bool checked)
	{
		ui->okay->setEnabled(checked);
	});

	connect(ui->do_not_show, &QCheckBox::clicked, [this](bool checked)
	{
		m_gui_settings->SetValue(gui::ib_show_welcome, QVariant(!checked));
	});

	connect(ui->okay, &QPushButton::clicked, this, &QDialog::accept);

	layout()->setSizeConstraint(QLayout::SetFixedSize);
}

welcome_dialog::~welcome_dialog()
{
}
