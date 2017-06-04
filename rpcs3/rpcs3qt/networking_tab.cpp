#include <QComboBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "networking_tab.h"

networking_tab::networking_tab(std::shared_ptr<emu_settings> xemu_settings, QWidget *parent) : QWidget(parent)
{
	// Connection status
	QGroupBox *netStatus = new QGroupBox(tr("Connection status"));

	QComboBox *netStatusBox = xemu_settings->CreateEnhancedComboBox(emu_settings::ConnectionStatus, this);

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
