#ifdef QT_UI

#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>

#include "AutoPauseSettingsDialog.h"

AutoPauseSettingsDialog::AutoPauseSettingsDialog(QWidget *parent) : QDialog(parent)
{
	QLabel *desc = new QLabel(tr("To use auto pause: enter the ID(s) of a function or a system call.\nRestart of the game is required to apply. You can enable/disable this in the settings."));

	QTableWidget *pauseList = new QTableWidget;
	pauseList->setColumnCount(2);
	pauseList->setHorizontalHeaderItem(0, new QTableWidgetItem(tr("Call ID")));
	pauseList->setHorizontalHeaderItem(1, new QTableWidgetItem(tr("Type")));

	QPushButton *clearButton = new QPushButton(tr("Clear"));

	QPushButton *reloadButton = new QPushButton(tr("Reload"));

	QPushButton *saveButton = new QPushButton(tr("Save"));
	connect(saveButton, &QAbstractButton::clicked, this, &QDialog::accept);

	QPushButton *cancelButton = new QPushButton(tr("Cancel"));
	cancelButton->setDefault(true);
	connect(cancelButton, &QAbstractButton::clicked, this, &QWidget::close);

	QHBoxLayout *buttonsLayout = new QHBoxLayout;
	buttonsLayout->addWidget(clearButton);
	buttonsLayout->addWidget(reloadButton);
	buttonsLayout->addStretch();
	buttonsLayout->addWidget(saveButton);
	buttonsLayout->addWidget(cancelButton);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(desc);
	mainLayout->addWidget(pauseList);
	mainLayout->addLayout(buttonsLayout);
	setLayout(mainLayout);

	setMinimumSize(QSize(400, 360));

	setWindowTitle(tr("Auto Pause Manager"));
}

#endif // QT_UI

