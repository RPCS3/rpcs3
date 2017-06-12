#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "audio_tab.h"

audio_tab::audio_tab(std::shared_ptr<emu_settings> xemu_settings, QWidget *parent) : QWidget(parent)
{
	// Audio Out
	QGroupBox *audioOut = new QGroupBox(tr("Audio Out"));

	QComboBox *audioOutBox = xemu_settings->CreateEnhancedComboBox(emu_settings::AudioRenderer, this);

	QVBoxLayout *audioOutVbox = new QVBoxLayout;
	audioOutVbox->addWidget(audioOutBox);
	audioOut->setLayout(audioOutVbox);

	// Checkboxes
	QCheckBox *audioDump = xemu_settings->CreateEnhancedCheckBox(emu_settings::DumpToFile, this);
	QCheckBox *conv = xemu_settings->CreateEnhancedCheckBox(emu_settings::ConvertTo16Bit, this);
	QCheckBox *downmix = xemu_settings->CreateEnhancedCheckBox(emu_settings::DownmixStereo, this);

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
