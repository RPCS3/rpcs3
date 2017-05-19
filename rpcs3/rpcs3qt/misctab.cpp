#include <QCheckBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>

#include "misctab.h"
#include "mainwindow.h"

MiscTab::MiscTab(std::shared_ptr<EmuSettings> xEmuSettings, QWidget *parent) : QWidget(parent)
{
	// Left Widgets
	QCheckBox *exitOnStop = xEmuSettings->CreateEnhancedCheckBox(EmuSettings::ExitRPCS3OnFinish, this);
	QCheckBox *alwaysStart = xEmuSettings->CreateEnhancedCheckBox(EmuSettings::StartOnBoot, this);
	QCheckBox *apSystemcall = xEmuSettings->CreateEnhancedCheckBox(EmuSettings::AutoPauseSysCall, this);
	QCheckBox *apFunctioncall = xEmuSettings->CreateEnhancedCheckBox(EmuSettings::AutoPauseFuncCall, this);
	QCheckBox *startGameFullscreen = xEmuSettings->CreateEnhancedCheckBox(EmuSettings::StartGameFullscreen, this);

	// Left layout
	QVBoxLayout *vbox_left = new QVBoxLayout;
	vbox_left->addWidget(exitOnStop);
	vbox_left->addWidget(alwaysStart);
	vbox_left->addWidget(apSystemcall);
	vbox_left->addWidget(apFunctioncall);
	vbox_left->addWidget(startGameFullscreen);
	vbox_left->addStretch(1);

	// Main Layout
	QHBoxLayout *hbox = new QHBoxLayout;
	hbox->addLayout(vbox_left);
	setLayout(hbox);
}