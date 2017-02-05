#ifdef QT_UI

#include <QCheckBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "misctab.h"

MiscTab::MiscTab(QWidget *parent) : QWidget(parent)
{
	// Checkboxes
	QCheckBox *exitOnStop = new QCheckBox(tr("Exit RPCS3 when process finishes"));
	QCheckBox *alwaysStart = new QCheckBox(tr("Always start after boot"));
	QCheckBox *apSystemcall = new QCheckBox(tr("Auto Pause at System Call"));
	QCheckBox *apFunctioncall = new QCheckBox(tr("Auto Pause at Function Call"));

	// Main layout
	QVBoxLayout *vbox = new QVBoxLayout;
	vbox->addWidget(exitOnStop);
	vbox->addWidget(alwaysStart);
	vbox->addWidget(apSystemcall);
	vbox->addWidget(apFunctioncall);
	vbox->addStretch(1);

	QHBoxLayout *hbox = new QHBoxLayout;
	hbox->addLayout(vbox);
	hbox->addStretch(1);
	setLayout(hbox);
}

#endif // QT_UI
