#ifdef QT_UI

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "systemtab.h"

SystemTab::SystemTab(QWidget *parent) : QWidget(parent)
{
	// Language
	QGroupBox *sysLang = new QGroupBox(tr("Language"));

	QComboBox *sysLangBox = new QComboBox;
	sysLangBox->addItem(tr("Japanese"));
	sysLangBox->addItem(tr("English (US)"));
	sysLangBox->addItem(tr("French"));
	sysLangBox->addItem(tr("Spanish"));
	sysLangBox->addItem(tr("German"));
	sysLangBox->addItem(tr("Italian"));
	sysLangBox->addItem(tr("Dutch"));
	sysLangBox->addItem(tr("Portuguese (PT)"));
	sysLangBox->addItem(tr("Russian"));
	sysLangBox->addItem(tr("Korean"));
	sysLangBox->addItem(tr("Chinese (Trad.)"));
	sysLangBox->addItem(tr("Chinese (Simp.)"));
	sysLangBox->addItem(tr("Finnish"));
	sysLangBox->addItem(tr("Swedish"));
	sysLangBox->addItem(tr("Danish"));
	sysLangBox->addItem(tr("Norwegian"));
	sysLangBox->addItem(tr("Polish"));
	sysLangBox->addItem(tr("English (UK)"));

	QVBoxLayout *sysLangVbox = new QVBoxLayout;
	sysLangVbox->addWidget(sysLangBox);
	sysLang->setLayout(sysLangVbox);

	// Checkboxes
	QCheckBox *enableHostRoot = new QCheckBox(tr("Enable /host_root/"));

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

#endif // QT_UI
