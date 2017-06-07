#include "welcome_dialog.h"

#include "gui_settings.h"

#include "Emu/System.h"

#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

welcome_dialog::welcome_dialog(QWidget* parent) : QDialog(parent)
{
	gui_settings* settings = new gui_settings(this);

	QPushButton* okay = new QPushButton(tr("Okay"));
	okay->setEnabled(false);

	QCheckBox* do_not_show = new QCheckBox(tr("Do not show again"));
	QCheckBox* i_have_read = new QCheckBox(tr("I have read the quickstart guide (required)"));

	connect(i_have_read, &QCheckBox::clicked, [=](bool checked)
	{
		okay->setEnabled(checked);
	});

	connect(do_not_show, &QCheckBox::clicked, [=](bool checked)
	{
		settings->SetValue(GUI::ib_show_welcome, QVariant(!checked));
	});

	connect(okay, &QPushButton::pressed, this, &QDialog::accept);

	QIcon rpcs3_icon = QIcon(":/rpcs3.ico");
	QLabel* icon = new QLabel(this);
	icon->setPixmap(rpcs3_icon.pixmap(120, 120));
	icon->setAlignment(Qt::AlignRight);

	QLabel* header_1 = new QLabel(tr(
		"<h1>Welcome to RPCS3</h1>"
	));

	QFont header_font;
	header_font.setPointSize(12);

	header_1->setFont(header_font);
	header_1->setFixedWidth(header_1->sizeHint().width());
	header_1->setWordWrap(true);

	QLabel* header_2 = new QLabel(tr(
		"<h2>An open-source PlayStation 3 emulator for Windows and Linux funded with <a href =\"https://www.patreon.com/Nekotekina\">Patreon</a>!</h2>"
	));

	header_2->setFixedWidth(header_1->sizeHint().width() * 1.2);
	header_2->setWordWrap(true);

	QLabel* caption = new QLabel(tr(
		"To get started, you need to install the PlayStation 3 firmware.<br>"
		"Check out our <a href=\"https://rpcs3.net/quickstart\">quickstart guide</a> for further information.<br>"
		"If you have any questions, please have a look at our <a href =\"https://rpcs3.net/faq\">FAQ</a>.<br>"
		"Otherwise, further discussion and support can be found at our <a href =\"http://www.emunewz.net/forum/forumdisplay.php?fid=172\">forums</a> "
		"or on our <a href =\"https://discord.me/RPCS3\">discord server</a>.<br>"
	));

	QFont caption_font;
	caption_font.setPointSize(10);
	caption_font.setWeight(QFont::Medium);

	caption->setFont(caption_font);
	caption->setFixedWidth(caption->sizeHint().width());
	caption->setWordWrap(true);
	caption->setOpenExternalLinks(true);
	caption->setAlignment(Qt::AlignLeft);

	QLabel* piracy = new QLabel(tr(
		"<font color='red'><u><b>RPCS3 does NOT condone piracy. You must dump your own games.</b></u></font>"
	));
	piracy->setWordWrap(true);
	piracy->setAlignment(Qt::AlignCenter);

	// Header Layout
	QVBoxLayout* header_layout = new QVBoxLayout();
	header_layout->setAlignment(Qt::AlignLeft);
	header_layout->addWidget(header_1);
	header_layout->addWidget(header_2);

	// Caption Layout
	QVBoxLayout* caption_layout = new QVBoxLayout();
	caption_layout->addWidget(caption);
	caption_layout->addWidget(piracy);

	// Top Layout
	QHBoxLayout* top_layout = new QHBoxLayout();
	top_layout->setAlignment(Qt::AlignCenter);
	top_layout->addStretch();
	top_layout->addWidget(icon);
	top_layout->addSpacing(icon->sizeHint().width() / 10);
	top_layout->addLayout(header_layout);
	top_layout->addStretch();

	// Bottom Layout
	QHBoxLayout* bottom_layout = new QHBoxLayout();
	bottom_layout->setAlignment(Qt::AlignCenter);
	bottom_layout->addLayout(caption_layout);

	// Button Layout
	QHBoxLayout* button_layout = new QHBoxLayout();
	button_layout->addWidget(okay);
	button_layout->addSpacing(15);
	button_layout->addWidget(i_have_read);
	button_layout->addStretch();
	button_layout->addWidget(do_not_show);

	// Main Layout
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addLayout(top_layout);
	layout->addSpacing(25);
	layout->addLayout(bottom_layout);
	layout->addSpacing(25);
	layout->addLayout(button_layout);

	setWindowIcon(rpcs3_icon);
	setWindowTitle(tr("Welcome to RPCS3"));
	setWindowFlags(Qt::WindowTitleHint);
	setLayout(layout);

	setFixedSize(sizeHint());
}
