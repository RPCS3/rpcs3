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
#ifdef _WIN32
	audioOutBox->addItem(tr("XAudio2"));
#endif // _WIN32
	audioOutBox->addItem(tr("OpenAL"));

	QVBoxLayout *audioOutVbox = new QVBoxLayout;
	audioOutVbox->addWidget(audioOutBox);
	audioOut->setLayout(audioOutVbox);

	// Checkboxes
	QCheckBox *audioDump = new QCheckBox(tr("Dump to file"));
	QCheckBox *conv = new QCheckBox(tr("Convert to 16 bit"));

	// Main layout
	QVBoxLayout *vbox = new QVBoxLayout;
	vbox->addWidget(audioOut);
	vbox->addWidget(audioDump);
	vbox->addWidget(conv);
	vbox->addStretch();

	QHBoxLayout *hbox = new QHBoxLayout;
	hbox->addLayout(vbox);
	hbox->addStretch();
	setLayout(hbox);
}

#endif // QT_UI
