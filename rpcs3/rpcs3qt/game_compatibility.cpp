#include "game_compatibility.h"

#include <QLabel>
#include <QMessageBox>

constexpr auto qstr = QString::fromStdString;
inline std::string sstr(const QString& _in) { return _in.toStdString(); }

game_compatibility::game_compatibility(std::shared_ptr<gui_settings> settings) : m_xgui_settings(settings)
{
	m_filepath = m_xgui_settings->GetSettingsDir() + "/compat_database.dat";
	m_url = "https://rpcs3.net/compatibility?api=v1&export";
	m_network_request = QNetworkRequest(QUrl(m_url));

	RequestCompatibility();
}

void game_compatibility::RequestCompatibility(bool online)
{
	// Creates new map from database
	auto ReadJSON = [=](const QJsonObject& json_data, bool after_download)
	{
		int return_code = json_data["return_code"].toInt();

		if (return_code < 0)
		{
			if (after_download)
			{
				std::string error_message;
				switch (return_code)
				{
				case -1:
					error_message = u8"\u4F3A\u670D\u5668\u932F\u8AA4 - \u5167\u90E8\u932F\u8AA4";
					break;
				case -2:
					error_message = u8"\u4F3A\u670D\u5668\u932F\u8AA4 - \u7DAD\u8B77\u6A21\u5F0F";
					break;
				default:
					error_message = u8"\u4F3A\u670D\u5668\u932F\u8AA4 - \u672A\u77E5\u932F\u8AA4";
					break;
				}
				LOG_ERROR(GENERAL, u8"\u76F8\u5BB9\u6027\u932F\u8AA4: { %s: \u8FD4\u56DE\u78BC %d }", error_message, return_code);
				Q_EMIT DownloadError(qstr(error_message) + " " + QString::number(return_code));
			}
			else
			{
				LOG_ERROR(GENERAL, u8"\u76F8\u5BB9\u6027\u932F\u8AA4: { \u8CC7\u6599\u5EAB\u932F\u8AA4 - \u7121\u6548: \u8FD4\u56DE\u78BC %d }", return_code);
			}
			return false;
		}

		if (!json_data["results"].isObject())
		{
			LOG_ERROR(GENERAL, u8"\u76F8\u5BB9\u6027\u932F\u8AA4: { \u8CC7\u6599\u5EAB\u932F\u8AA4 - \u672A\u627E\u5230\u7D50\u679C }");
			return false;
		}

		m_compat_database.clear();

		QJsonObject json_results = json_data["results"].toObject();

		// Retrieve status data for every valid entry
		for (const auto& key : json_results.keys())
		{
			if (!json_results[key].isObject())
			{
				LOG_ERROR(GENERAL, u8"\u76F8\u5BB9\u6027\u932F\u8AA4: { \u8CC7\u6599\u5EAB\u932F\u8AA4 - \u4E0D\u53EF\u7528\u7684\u76EE\u6A19 %s }", sstr(key));
				continue;
			}

			QJsonObject json_result = json_results[key].toObject();

			// Retrieve compatibility information from json
			compat_status status = Status_Data.at(json_result.value("status").toString("NoResult"));

			// Add date if possible
			status.date = json_result.value("date").toString();

			// Add status to map
			m_compat_database.emplace(std::pair<std::string, compat_status>(sstr(key), status));
		}

		return true;
	};

	if (!online)
	{
		// Retrieve database from file
		QFile file(m_filepath);

		if (!file.exists())
		{
			LOG_NOTICE(GENERAL, u8"\u76F8\u5BB9\u6027\u901A\u77E5: { \u672A\u627E\u5230\u8CC7\u6599\u5EAB\u6A94\u6848: %s }", sstr(m_filepath));
			return;
		}

		if (!file.open(QIODevice::ReadOnly))
		{
			LOG_ERROR(GENERAL, u8"\u76F8\u5BB9\u6027\u932F\u8AA4: { \u8CC7\u6599\u5EAB\u932F\u8AA4 - \u7121\u6CD5\u5F9E\u6A94\u6848\u4E2D\u8B80\u53D6\u8CC7\u6599\u5EAB: %s }", sstr(m_filepath));
			return;
		}

		QByteArray data = file.readAll();
		file.close();

		LOG_NOTICE(GENERAL, u8"\u76F8\u5BB9\u6027\u901A\u77E5: { \u5F9E\u6A94\u6848\u4E2D\u5DF2\u5B8C\u6210\u8B80\u53D6\u8CC7\u6599\u5EAB: %s }", sstr(m_filepath));

		// Create new map from database
		ReadJSON(QJsonDocument::fromJson(data).object(), online);

		return;
	}

	if (QSslSocket::supportsSsl() == false)
	{
		LOG_ERROR(GENERAL, u8"\u7121\u6CD5\u53D6\u5F97\u7DDA\u4E0A\u8CC7\u6599\u5EAB! \u8ACB\u78BA\u5B9A\u60A8\u7684\u7CFB\u7D71\u652F\u63F4 SSL\u3002");
		QMessageBox::warning(nullptr, tr(u8"\u8B66\u544A!"), tr(u8"\u7121\u6CD5\u53D6\u5F97\u7DDA\u4E0A\u8CC7\u6599\u5EAB! \u8ACB\u78BA\u5B9A\u60A8\u7684\u7CFB\u7D71\u652F\u63F4 SSL\u3002"));
		return;
	}

	LOG_NOTICE(GENERAL, u8"SSL \u652F\u63F4! \u521D\u59CB\u76F8\u5BB9\u6027\u8CC7\u6599\u5EAB\u4E0B\u8F09\u5F9E: %s", sstr(m_url));

	// Send request and wait for response
	m_network_access_manager.reset(new QNetworkAccessManager());
	QNetworkReply* network_reply = m_network_access_manager->get(m_network_request);

	// Show Progress
	m_progress_dialog.reset(new QProgressDialog(tr(u8".\u8ACB\u7A0D\u5019."), tr(u8"\u4E2D\u6B62"), 0, 100));
	m_progress_dialog->setWindowTitle(tr(u8"\u6B63\u5728\u4E0B\u8F09\u8CC7\u6599\u5EAB"));
	m_progress_dialog->setFixedWidth(QLabel(u8"\u7531\u65BC HiDPI \u7684\u539F\u56E0\uFF0C\u9019\u662F\u9032\u5EA6\u689D\u7684\u9577\u5EA6\u3002").sizeHint().width());
	m_progress_dialog->setValue(0);
	m_progress_dialog->show();

	// Animate progress dialog a bit more
	m_progress_timer.reset(new QTimer(this));
	connect(m_progress_timer.get(), &QTimer::timeout, [&]()
	{
		switch (++m_timer_count % 3)
		{
		case 0:
			m_timer_count = 0;
			m_progress_dialog->setLabelText(tr(u8".\u8ACB\u7A0D\u5019."));
			break;
		case 1:
			m_progress_dialog->setLabelText(tr(u8"..\u8ACB\u7A0D\u5019.."));
			break;
		default:
			m_progress_dialog->setLabelText(tr(u8"...\u8ACB\u7A0D\u5019..."));
			break;
		}
	});
	m_progress_timer->start(500);

	// Handle abort
	connect(m_progress_dialog.get(), &QProgressDialog::rejected, network_reply, &QNetworkReply::abort);

	// Handle progress
	connect(network_reply, &QNetworkReply::downloadProgress, [&](qint64 bytesReceived, qint64 bytesTotal)
	{
		m_progress_dialog->setMaximum(bytesTotal);
		m_progress_dialog->setValue(bytesReceived);
	});

	// Handle response according to its contents
	connect(network_reply, &QNetworkReply::finished, [=]()
	{
		// Clean up Progress Dialog
		if (m_progress_dialog)
		{
			m_progress_dialog->close();
		}
		if (m_progress_timer)
		{
			m_progress_timer->stop();
		}

		// Handle Errors
		if (network_reply->error() != QNetworkReply::NoError)
		{
			// We failed to retrieve a new database, therefore refresh gamelist to old state
			QString error = network_reply->errorString();
			Q_EMIT DownloadError(error);
			LOG_ERROR(GENERAL, u8"\u76F8\u5BB9\u6027\u932F\u8AA4: { \u7DB2\u8DEF\u932F\u8AA4 - %s }", sstr(error));
			return;
		}

		LOG_NOTICE(GENERAL, u8"\u76F8\u5BB9\u6027\u901A\u77E5: { \u8CC7\u6599\u5EAB\u4E0B\u8F09\u5B8C\u6210 }");

		// Read data from network reply
		QByteArray data = network_reply->readAll();
		network_reply->deleteLater();

		// Create new map from database and write database to file if database was valid
		if (ReadJSON(QJsonDocument::fromJson(data).object(), online))
		{
			// We have a new database in map, therefore refresh gamelist to new state
			Q_EMIT DownloadFinished();

			// Write database to file
			QFile file(m_filepath);

			if (file.exists())
			{
				LOG_NOTICE(GENERAL, u8"\u76F8\u5BB9\u6027\u901A\u77E5: { \u627E\u5230\u8CC7\u6599\u5EAB\u6A94\u6848: %s }", sstr(m_filepath));
			}

			if (!file.open(QIODevice::WriteOnly))
			{
				LOG_ERROR(GENERAL, u8"\u76F8\u5BB9\u6027\u932F\u8AA4: { \u8CC7\u6599\u5EAB\u932F\u8AA4 - \u7121\u6CD5\u5C07\u8CC7\u6599\u5EAB\u5BEB\u5165\u6A94\u6848: %s }", sstr(m_filepath));
				return;
			}

			file.write(data);
			file.close();

			LOG_SUCCESS(GENERAL, u8"\u76F8\u5BB9\u6027\u6210\u529F: { \u5C07\u8CC7\u6599\u5EAB\u5BEB\u5165\u6A94\u6848: %s }", sstr(m_filepath));
		}
	});

	// We want to retrieve a new database, therefore refresh gamelist and indicate that
	Q_EMIT DownloadStarted();
}

compat_status game_compatibility::GetCompatibility(const std::string& title_id)
{
	if (m_compat_database.empty())
	{
		return Status_Data.at("NoData");
	}
	else if (m_compat_database.count(title_id) > 0)
	{
		return m_compat_database[title_id];
	}

	return Status_Data.at("NoResult");
}

compat_status game_compatibility::GetStatusData(const QString& status)
{
	return Status_Data.at(status);
}
