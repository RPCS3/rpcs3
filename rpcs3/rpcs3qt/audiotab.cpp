#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "audiotab.h"

AudioTab::AudioTab(std::shared_ptr<EmuSettings> xEmuSettings, QWidget *parent) : QWidget(parent)
{
	// Audio Out
	QGroupBox *audioOut = new QGroupBox(tr("Audio Out"));

	QComboBox *audioOutBox = xEmuSettings->CreateEnhancedComboBox(EmuSettings::AudioRenderer, this);

	QVBoxLayout *audioOutVbox = new QVBoxLayout;
	audioOutVbox->addWidget(audioOutBox);
	audioOut->setLayout(audioOutVbox);

	// Checkboxes
	QCheckBox *audioDump = xEmuSettings->CreateEnhancedCheckBox(EmuSettings::DumpToFile, this);
	QCheckBox *conv = xEmuSettings->CreateEnhancedCheckBox(EmuSettings::ConvertTo16Bit, this);
	QCheckBox *downmix = xEmuSettings->CreateEnhancedCheckBox(EmuSettings::DownmixStereo, this);

	// Main layout
	QVBoxLayout *vbox = new QVBoxLayout;
	vbox->addWidget(audioOut);
	vbox->addWidget(audioDump);
	vbox->addWidget(conv);
	vbox->addWidget(downmix);
	vbox->addStretch();

	QHBoxLayout *hbox = new QHBoxLayout;
	hbox->addLayout(vbox);
	hbox->addStretch();
	setLayout(hbox);
}
