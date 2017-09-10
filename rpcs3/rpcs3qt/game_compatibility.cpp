#include "game_compatibility.h"

const std::map<QString, game_compatibility_status> g_game_compatibility_status_data = {
	{ "Playable", { QObject::tr("Playable"), QObject::tr("Games that can be properly played from start to finish"), "#2ecc71" } },
	{ "Ingame", { QObject::tr("Ingame"), QObject::tr("Games that go somewhere but not far enough to be considered playable"), "#f1c40f" } },
	{ "Intro", { QObject::tr("Intro"), QObject::tr("Games that only display some screens"), "#f39c12" } },
	{ "Loadable", { QObject::tr("Loadable"), QObject::tr("Games that display a black screen with an active framerate"), "#e74c3c" } },
	{ "Nothing", { QObject::tr("Nothing"), QObject::tr("Games that show nothing"), "#2c3e50" } }
};

game_compatibility::game_compatibility(int compatibilityColumn, int serialColumn, std::shared_ptr<gui_settings> settings, QTableWidget *tableWidget)
		: m_compatibilityColumn(compatibilityColumn), m_serialColumn(serialColumn), xgui_settings(settings), m_tableWidget(tableWidget)
{
	m_networkAccessManager = std::make_unique<QNetworkAccessManager>();
}

bool game_compatibility::isEnabled()
{
	return xgui_settings->GetValue(GUI::gl_compatibility).toBool();
}

void game_compatibility::update()
{
	if (!isEnabled())
	{
		for (int i = 0; i < m_tableWidget->rowCount(); i++)
		{
			m_tableWidget->item(i, m_compatibilityColumn)->setText(QObject::tr("Disabled"));
		}
		return;
	}

	for (int i = 0; i < m_tableWidget->rowCount(); i++)
	{
		QTableWidgetItem *item = m_tableWidget->item(i, m_compatibilityColumn);
		item->setText(QObject::tr("Checking..."));

		QString serial = m_tableWidget->item(i, m_serialColumn)->text();

		QNetworkRequest networkRequest = QNetworkRequest(QUrl("https://rpcs3.net/compatibility?api=v1&g=" + serial));
		QNetworkReply *reply = m_networkAccessManager->get(networkRequest);
		QObject::connect(reply, &QNetworkReply::finished, [=]()
		{
			if (reply->error() != QNetworkReply::NoError)
			{
				item->setText(reply->errorString());
				reply->deleteLater();
				return;
			}

			QByteArray responseData = reply->readAll();
			reply->deleteLater();

			QJsonObject jsonObject = QJsonDocument::fromJson(responseData).object();
			if ((jsonObject["return_code"].toInt() != 0) || !jsonObject["results"].isObject())
			{
				item->setText(QObject::tr("No results found"));
				return;
			}

			QJsonObject results = jsonObject["results"].toObject();
			if (!results[serial].isObject())
			{
				item->setText(QObject::tr("No results found"));
				return;
			}

			QJsonValue jsonStatus = results[serial].toObject().value(QString::fromStdString("status"));
			if (!jsonStatus.isString())
			{
				item->setText(QObject::tr("No results found"));
				return;
			}

			game_compatibility_status compatibilityStatus = g_game_compatibility_status_data.at(jsonStatus.toString());

			QJsonValue jsonDate = results[serial].toObject().value(QString::fromStdString("date"));
			if (!jsonDate.isString())
			{
				item->setText(compatibilityStatus.m_text);
			}
			else
			{
				item->setText(compatibilityStatus.m_text + " (" + jsonDate.toString() + ")");
			}

			item->setData(Qt::DecorationRole, game_compatibility_pixmap(compatibilityStatus.m_color));
			item->setToolTip(compatibilityStatus.m_tooltip);
		});
	}
}
