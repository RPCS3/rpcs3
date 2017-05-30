#include <QCheckBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>

#include "misc_tab.h"
#include "main_window.h"

misc_tab::misc_tab(std::shared_ptr<emu_settings> xemu_settings, QWidget *parent) : QWidget(parent)
{
	// Left Widgets
	QCheckBox *exitOnStop = xemu_settings->CreateEnhancedCheckBox(emu_settings::ExitRPCS3OnFinish, this);
	QCheckBox *alwaysStart = xemu_settings->CreateEnhancedCheckBox(emu_settings::StartOnBoot, this);
	QCheckBox *startGameFullscreen = xemu_settings->CreateEnhancedCheckBox(emu_settings::StartGameFullscreen, this);
	QCheckBox *showFPSInTitle = xemu_settings->CreateEnhancedCheckBox(emu_settings::ShowFPSInTitle, this);

	// Left layout
	QVBoxLayout *vbox_left = new QVBoxLayout;
	vbox_left->addWidget(exitOnStop);
	vbox_left->addWidget(alwaysStart);
	vbox_left->addWidget(startGameFullscreen);
	vbox_left->addWidget(showFPSInTitle);
	vbox_left->addStretch(1);

	// Main Layout
	QHBoxLayout *hbox = new QHBoxLayout;
	hbox->addLayout(vbox_left);
	setLayout(hbox);
}