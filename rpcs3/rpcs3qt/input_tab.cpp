#include <QComboBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "input_tab.h"

input_tab::input_tab(std::shared_ptr<emu_settings> xemu_settings, QWidget *parent) : QWidget(parent)
{
	// Pad Handler
	QGroupBox *padHandler = new QGroupBox(tr("Pad Handler"));

	QComboBox *padHandlerBox = xemu_settings->CreateEnhancedComboBox(emu_settings::PadHandler, this);

	QVBoxLayout *padHandlerVbox = new QVBoxLayout;
	padHandlerVbox->addWidget(padHandlerBox);
	padHandler->setLayout(padHandlerVbox);

	// Keyboard Handler
	QGroupBox *keyboardHandler = new QGroupBox(tr("Keyboard Handler"));

	QComboBox *keyboardHandlerBox = xemu_settings->CreateEnhancedComboBox(emu_settings::KeyboardHandler, this);

	QVBoxLayout *keyboardHandlerVbox = new QVBoxLayout;
	keyboardHandlerVbox->addWidget(keyboardHandlerBox);
	keyboardHandler->setLayout(keyboardHandlerVbox);

	// Mouse Handler
	QGroupBox *mouseHandler = new QGroupBox(tr("Mouse Handler"));

	QComboBox *mouseHandlerBox = xemu_settings->CreateEnhancedComboBox(emu_settings::MouseHandler, this);

	QVBoxLayout *mouseHandlerVbox = new QVBoxLayout;
	mouseHandlerVbox->addWidget(mouseHandlerBox);
	mouseHandler->setLayout(mouseHandlerVbox);

	// Camera
	QGroupBox *camera = new QGroupBox(tr("Camera"));

	QComboBox *cameraBox = xemu_settings->CreateEnhancedComboBox(emu_settings::Camera, this);

	QVBoxLayout *cameraVbox = new QVBoxLayout;
	cameraVbox->addWidget(cameraBox);
	camera->setLayout(cameraVbox);

	// Camera type
	QGroupBox *cameraType = new QGroupBox(tr("Camera type"));

	QComboBox *cameraTypeBox = xemu_settings->CreateEnhancedComboBox(emu_settings::CameraType, this);

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

