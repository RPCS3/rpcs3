#pragma once

#include <memory>
#include <QtGui/QPainter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QTableWidget>

#include "gui_settings.h"

class game_compatibility
{
private:
	int m_compatibilityColumn;
	int m_serialColumn;
	std::shared_ptr<gui_settings> xgui_settings;
	std::unique_ptr<QNetworkAccessManager> m_networkAccessManager;
	QTableWidget *m_tableWidget;

public:
	game_compatibility(int compatibilityColumn, int serialColumn, std::shared_ptr<gui_settings> settings, QTableWidget *tableWidget);
	bool isEnabled();
	void update();
};

class game_compatibility_pixmap : public QPixmap
{
public:
	game_compatibility_pixmap(QColor color) : QPixmap(16, 16)
	{
		fill(Qt::transparent);

		QPainter painter(this);
		painter.setPen(color);
		painter.setBrush(color);
		painter.drawEllipse(0, 0, 15, 15);
	}
};

struct game_compatibility_status
{
	QString m_text;
	QString m_tooltip;
	QString m_color;
};
