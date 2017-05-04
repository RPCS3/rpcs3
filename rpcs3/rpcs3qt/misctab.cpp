#include <QCheckBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>

#include "misctab.h"
#include "mainwindow.h"

MiscTab::MiscTab(QWidget *parent) : QWidget(parent)
{
	// Left Widgets
	QCheckBox *exitOnStop = new QCheckBox(tr("Exit RPCS3 when process finishes"));
	QCheckBox *alwaysStart = new QCheckBox(tr("Always start after boot"));
	QCheckBox *apSystemcall = new QCheckBox(tr("Auto Pause at System Call"));
	QCheckBox *apFunctioncall = new QCheckBox(tr("Auto Pause at Function Call"));

	// Left layout
	QVBoxLayout *vbox_left = new QVBoxLayout;
	vbox_left->addWidget(exitOnStop);
	vbox_left->addWidget(alwaysStart);
	vbox_left->addWidget(apSystemcall);
	vbox_left->addWidget(apFunctioncall);
	vbox_left->addStretch(1);


	// Main Layout
	QHBoxLayout *hbox = new QHBoxLayout;
	hbox->addLayout(vbox_left);
	setLayout(hbox);
}