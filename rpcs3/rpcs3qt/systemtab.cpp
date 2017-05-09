#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "systemtab.h"

SystemTab::SystemTab(std::shared_ptr<EmuSettings> xEmuSettings, QWidget *parent) : QWidget(parent)
{
	// Language
	QGroupBox *sysLang = new QGroupBox(tr("Language"));

	QComboBox *sysLangBox = xEmuSettings->CreateEnhancedComboBox(EmuSettings::Language, this);

	QVBoxLayout *sysLangVbox = new QVBoxLayout;
	sysLangVbox->addWidget(sysLangBox);
	sysLang->setLayout(sysLangVbox);

	// Checkboxes
	QCheckBox *enableHostRoot = xEmuSettings->CreateEnhancedCheckBox(EmuSettings::EnableHostRoot, this);

	// Main layout
	QVBoxLayout *vbox = new QVBoxLayout;
	vbox->addWidget(enableHostRoot);
	vbox->addWidget(sysLang);
	vbox->addStretch();

	QHBoxLayout *hbox = new QHBoxLayout;
	hbox->addLayout(vbox);
	hbox->addStretch();
	setLayout(hbox);
}
