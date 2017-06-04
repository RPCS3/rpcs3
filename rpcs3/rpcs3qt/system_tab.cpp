#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "system_tab.h"

system_tab::system_tab(std::shared_ptr<emu_settings> xemu_settings, QWidget *parent) : QWidget(parent)
{
	// Language
	QGroupBox *sysLang = new QGroupBox(tr("Language"));

	QComboBox *sysLangBox = xemu_settings->CreateEnhancedComboBox(emu_settings::Language, this);

	QVBoxLayout *sysLangVbox = new QVBoxLayout;
	sysLangVbox->addWidget(sysLangBox);
	sysLang->setLayout(sysLangVbox);

	// Checkboxes
	QCheckBox *enableHostRoot = xemu_settings->CreateEnhancedCheckBox(emu_settings::EnableHostRoot, this);

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
