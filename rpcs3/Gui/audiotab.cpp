#ifdef QT_UI

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "audiotab.h"

AudioTab::AudioTab(QWidget *parent) : QWidget(parent)
{
	// Audio Out
	QGroupBox *audioOut = new QGroupBox(tr("Audio Out"));

	QComboBox *audioOutBox = new QComboBox;
	audioOutBox->addItem(tr("Null"));
	audioOutBox->addItem(tr("OpenAL"));
#ifdef _WIN32
	audioOutBox->addItem(tr("XAudio2"));
#endif // _WIN32
#ifdef __LINUX__
	audioOutBox->addItem(tr("ALSA"));
#endif // __LINUX__

	QVBoxLayout *audioOutVbox = new QVBoxLayout;
	audioOutVbox->addWidget(audioOutBox);
	audioOut->setLayout(audioOutVbox);

	// Checkboxes
	QCheckBox *audioDump = new QCheckBox(tr("Dump to file"));
	QCheckBox *conv = new QCheckBox(tr("Convert to 16 bit"));
	QCheckBox *downmix = new QCheckBox(tr("Downmix to Stereo"));

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

#endif // QT_UI
