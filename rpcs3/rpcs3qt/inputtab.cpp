#include <QComboBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "inputtab.h"

InputTab::InputTab(std::shared_ptr<EmuSettings> xEmuSettings, QWidget *parent) : QWidget(parent)
{
	// Pad Handler
	QGroupBox *padHandler = new QGroupBox(tr("Pad Handler"));

	QComboBox *padHandlerBox = xEmuSettings->CreateEnhancedComboBox(EmuSettings::PadHandler, this);

	QVBoxLayout *padHandlerVbox = new QVBoxLayout;
	padHandlerVbox->addWidget(padHandlerBox);
	padHandler->setLayout(padHandlerVbox);

	// Keyboard Handler
	QGroupBox *keyboardHandler = new QGroupBox(tr("Keyboard Handler"));

	QComboBox *keyboardHandlerBox = xEmuSettings->CreateEnhancedComboBox(EmuSettings::KeyboardHandler, this);

	QVBoxLayout *keyboardHandlerVbox = new QVBoxLayout;
	keyboardHandlerVbox->addWidget(keyboardHandlerBox);
	keyboardHandler->setLayout(keyboardHandlerVbox);

	// Mouse Handler
	QGroupBox *mouseHandler = new QGroupBox(tr("Mouse Handler"));

	QComboBox *mouseHandlerBox = xEmuSettings->CreateEnhancedComboBox(EmuSettings::MouseHandler, this);

	QVBoxLayout *mouseHandlerVbox = new QVBoxLayout;
	mouseHandlerVbox->addWidget(mouseHandlerBox);
	mouseHandler->setLayout(mouseHandlerVbox);

	// Camera
	QGroupBox *camera = new QGroupBox(tr("Camera"));

	QComboBox *cameraBox = xEmuSettings->CreateEnhancedComboBox(EmuSettings::Camera, this);

	QVBoxLayout *cameraVbox = new QVBoxLayout;
	cameraVbox->addWidget(cameraBox);
	camera->setLayout(cameraVbox);

	// Camera type
	QGroupBox *cameraType = new QGroupBox(tr("Camera type"));

	QComboBox *cameraTypeBox = xEmuSettings->CreateEnhancedComboBox(EmuSettings::CameraType, this);

	QVBoxLayout *cameraTypeVbox = new QVBoxLayout;
	cameraTypeVbox->addWidget(cameraTypeBox);
	cameraType->setLayout(cameraTypeVbox);

	// Main layout
	QVBoxLayout *vbox1 = new QVBoxLayout;
	vbox1->addWidget(padHandler);
	vbox1->addWidget(keyboardHandler);
	vbox1->addWidget(mouseHandler);
	vbox1->addStretch();

	QVBoxLayout *vbox2 = new QVBoxLayout;
	vbox2->addWidget(camera);
	vbox2->addWidget(cameraType);
	vbox2->addStretch();

	QHBoxLayout *hbox = new QHBoxLayout;
	hbox->addLayout(vbox1);
	hbox->addLayout(vbox2);
	setLayout(hbox);
}

