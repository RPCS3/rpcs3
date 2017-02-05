#ifdef QT_UI

#include <QComboBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "inputtab.h"

InputTab::InputTab(QWidget *parent) : QWidget(parent)
{
	// Pad Handler
	QGroupBox *padHandler = new QGroupBox(tr("Pad Handler"));

	QComboBox *padHandlerBox = new QComboBox;
	padHandlerBox->addItem(tr("Null"));

	QVBoxLayout *padHandlerVbox = new QVBoxLayout;
	padHandlerVbox->addWidget(padHandlerBox);
	padHandler->setLayout(padHandlerVbox);

	// Keyboard Handler
	QGroupBox *keyboardHandler = new QGroupBox(tr("Keyboard Handler"));

	QComboBox *keyboardHandlerBox = new QComboBox;
	keyboardHandlerBox->addItem(tr("Null"));

	QVBoxLayout *keyboardHandlerVbox = new QVBoxLayout;
	keyboardHandlerVbox->addWidget(keyboardHandlerBox);
	keyboardHandler->setLayout(keyboardHandlerVbox);

	// Mouse Handler
	QGroupBox *mouseHandler = new QGroupBox(tr("Mouse Handler"));

	QComboBox *mouseHandlerBox = new QComboBox;
	mouseHandlerBox->addItem(tr("Null"));

	QVBoxLayout *mouseHandlerVbox = new QVBoxLayout;
	mouseHandlerVbox->addWidget(mouseHandlerBox);
	mouseHandler->setLayout(mouseHandlerVbox);

	// Camera
	QGroupBox *camera = new QGroupBox(tr("Camera"));

	QComboBox *cameraBox = new QComboBox;
	cameraBox->addItem(tr("Null"));

	QVBoxLayout *cameraVbox = new QVBoxLayout;
	cameraVbox->addWidget(cameraBox);
	camera->setLayout(cameraVbox);

	// Camera type
	QGroupBox *cameraType = new QGroupBox(tr("Camera type"));

	QComboBox *cameraTypeBox = new QComboBox;
	cameraTypeBox->addItem(tr("Null"));

	QVBoxLayout *cameraTypeVbox = new QVBoxLayout;
	cameraTypeVbox->addWidget(cameraTypeBox);
	cameraType->setLayout(cameraTypeVbox);

	// Main layout
	QVBoxLayout *vbox1 = new QVBoxLayout;
	vbox1->addWidget(padHandler);
	vbox1->addWidget(keyboardHandler);
	vbox1->addWidget(mouseHandler);
	vbox1->addStretch(1);

	QVBoxLayout *vbox2 = new QVBoxLayout;
	vbox2->addWidget(camera);
	vbox2->addWidget(cameraType);
	vbox2->addStretch(1);

	QHBoxLayout *hbox = new QHBoxLayout;
	hbox->addLayout(vbox1);
	hbox->addLayout(vbox2);
	setLayout(hbox);
}

#endif // QT_UI
