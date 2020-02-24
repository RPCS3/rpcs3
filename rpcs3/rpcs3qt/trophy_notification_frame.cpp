#include "trophy_notification_frame.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>

static const int TROPHY_TIMEOUT_MS = 7500;

constexpr auto qstr = QString::fromStdString;

trophy_notification_frame::trophy_notification_frame(const std::vector<uchar>& imgBuffer, const SceNpTrophyDetails& trophy, int height) : QWidget()
{
	setObjectName("trophy_notification_frame");
	setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
	setAttribute(Qt::WA_ShowWithoutActivating);

	// Fill the background with black
	QPalette black_background;
	black_background.setColor(QPalette::Window, Qt::black);
	black_background.setColor(QPalette::WindowText, Qt::white);

	// Make the label
	QLabel* trophyImgLabel = new QLabel;
	trophyImgLabel->setAutoFillBackground(true);
	trophyImgLabel->setPalette(black_background);

	QImage trophyImg;
	if (!imgBuffer.empty() && trophyImg.loadFromData(imgBuffer.data(), static_cast<int>(imgBuffer.size()), "PNG"))
	{
		trophyImg = trophyImg.scaledToHeight(height); // I might consider adding ability to change size since on hidpi this will be rather small.
		trophyImgLabel->setPixmap(QPixmap::fromImage(trophyImg));
	}
	else
	{
		// This looks hideous, but it's a good placeholder.
		trophyImgLabel->setPixmap(QPixmap::fromImage(QImage(":/rpcs3.ico")));
	}

	QLabel* trophyName = new QLabel;
	trophyName->setWordWrap(true);
	trophyName->setAlignment(Qt::AlignCenter);

	QString trophyType = "";
	switch (trophy.trophyGrade)
	{
	case SCE_NP_TROPHY_GRADE_BRONZE:   trophyType = "bronze";   break;
	case SCE_NP_TROPHY_GRADE_SILVER:   trophyType = "silver";   break;
	case SCE_NP_TROPHY_GRADE_GOLD:     trophyType = "gold";     break;
	case SCE_NP_TROPHY_GRADE_PLATINUM: trophyType = "platinum"; break;
	default: break;
	}

	trophyName->setText(tr("You have earned the %1 trophy\n").arg(trophyType) + qstr(trophy.name));
	trophyName->setAutoFillBackground(true);
	trophyName->setPalette(black_background);

	QHBoxLayout* globalLayout = new QHBoxLayout;
	globalLayout->addWidget(trophyImgLabel);
	globalLayout->addWidget(trophyName);
	setLayout(globalLayout);
	setPalette(black_background);

	// I may consider moving this code later to be done at a better location.
	QTimer::singleShot(TROPHY_TIMEOUT_MS, [this]()
	{
		deleteLater();
	});
}
