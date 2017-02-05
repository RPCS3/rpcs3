#ifdef QT_UI

#include <QComboBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "networkingtab.h"

NetworkingTab::NetworkingTab(QWidget *parent) : QWidget(parent)
{
	// Connection status
	QGroupBox *netStatus = new QGroupBox(tr("Connection status"));

	QComboBox *netStatusBox = new QComboBox;
	netStatusBox->addItem(tr("Disconnected"));
	netStatusBox->addItem(tr("Connecting"));
	netStatusBox->addItem(tr("Obtaining IP"));
	netStatusBox->addItem(tr("IP obtained"));

	QVBoxLayout *netStatusVbox = new QVBoxLayout;
	netStatusVbox->addWidget(netStatusBox);
	netStatus->setLayout(netStatusVbox);

	// Main layout
	QVBoxLayout *vbox = new QVBoxLayout;
	vbox->addWidget(netStatus);
	vbox->addStretch();

	QHBoxLayout *hbox = new QHBoxLayout;
	hbox->addLayout(vbox);
	hbox->addStretch();
	setLayout(hbox);
}

#endif // QT_UI
