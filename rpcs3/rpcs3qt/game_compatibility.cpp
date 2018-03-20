#include "game_compatibility.h"

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
					error_message = u8"伺服器錯誤 - 內部錯誤";
					break;
				case -2:
					error_message = u8"伺服器錯誤 - 維護模式";
					break;
				default:
					error_message = u8"伺服器錯誤 - 未知錯誤";
					break;
				}
				LOG_ERROR(GENERAL, u8"相容性錯誤: { %s: 返回碼 %d }", error_message, return_code);
				Q_EMIT DownloadError(qstr(error_message) + " " + QString::number(return_code));
			}
			else
			{
				LOG_ERROR(GENERAL, u8"相容性錯誤: { 資料庫錯誤 - 無效: 返回碼 %d }", return_code);
			}
			return false;
		}

		if (!json_data["results"].isObject())
		{
			LOG_ERROR(GENERAL, u8"相容性錯誤: { 資料庫錯誤 - 未找到結果 }");
			return false;
		}

		m_compat_database.clear();

		QJsonObject json_results = json_data["results"].toObject();

		// Retrieve status data for every valid entry
		for (const auto& key : json_results.keys())
		{
			if (!json_results[key].isObject())
			{
				LOG_ERROR(GENERAL, u8"相容性錯誤: { 資料庫錯誤 - 不可用的目標 %s }", sstr(key));
				continue;
			}

			QJsonObject json_result = json_results[key].toObject();

			// Retrieve compatibility information from json
			Compat_Status compat_status = Status_Data.at(json_result.value("status").toString("NoResult"));

			// Add date if possible
			compat_status.date = json_result.value("date").toString();

			// Add status to map
			m_compat_database.emplace(std::pair<std::string, Compat_Status>(sstr(key), compat_status));
		}

		return true;
	};

	if (!online)
	{
		// Retrieve database from file
		QFile file(m_filepath);

		if (!file.exists())
		{
			LOG_NOTICE(GENERAL, u8"相容性通知: { 未找到資料庫檔案: %s }", sstr(m_filepath));
			return;
		}

		if (!file.open(QIODevice::ReadOnly))
		{
			LOG_ERROR(GENERAL, u8"相容性錯誤: { 資料庫錯誤 - 無法從檔案中讀取資料庫: %s }", sstr(m_filepath));
			return;
		}

		QByteArray data = file.readAll();
		file.close();

		LOG_NOTICE(GENERAL, u8"相容性通知: { 從檔案中已完成讀取資料庫: %s }", sstr(m_filepath));

		// Create new map from database
		ReadJSON(QJsonDocument::fromJson(data).object(), online);

		return;
	}

	if (QSslSocket::supportsSsl() == false)
	{
		LOG_ERROR(GENERAL, u8"無法取得線上資料庫! 請確定您的系統支援 SSL。");
		QMessageBox::warning(nullptr, tr(u8"警告!"), tr(u8"無法取得線上資料庫! 請確定您的系統支援 SSL。"));
		return;
	}

	LOG_NOTICE(GENERAL, u8"SSL 支援! 初始相容性資料庫下載從: %s", sstr(m_url));

	// Send request and wait for response
	m_network_access_manager.reset(new QNetworkAccessManager());
	QNetworkReply* network_reply = m_network_access_manager->get(m_network_request);

	// Show Progress
	m_progress_dialog.reset(new QProgressDialog(tr(u8".請稍候."), tr(u8"中止"), 0, 100));
	m_progress_dialog->setWindowTitle(tr(u8"正在下載資料庫"));
	m_progress_dialog->setFixedWidth(QLabel(u8"由於 HiDPI 的原因，這是進度條的長度。").sizeHint().width());
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
			m_progress_dialog->setLabelText(tr(u8".請稍候."));
			break;
		case 1:
			m_progress_dialog->setLabelText(tr(u8"..請稍候.."));
			break;
		default:
			m_progress_dialog->setLabelText(tr(u8"...請稍候..."));
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
			LOG_ERROR(GENERAL, u8"相容性錯誤: { 網路錯誤 - %s }", sstr(error));
			return;
		}

		LOG_NOTICE(GENERAL, u8"相容性通知: { 資料庫下載完成 }");

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
				LOG_NOTICE(GENERAL, u8"相容性通知: { 找到資料庫檔案: %s }", sstr(m_filepath));
			}

			if (!file.open(QIODevice::WriteOnly))
			{
				LOG_ERROR(GENERAL, u8"相容性錯誤: { 資料庫錯誤 - 無法將資料庫寫入檔案: %s }", sstr(m_filepath));
				return;
			}

			file.write(data);
			file.close();

			LOG_SUCCESS(GENERAL, u8"相容性成功: { 將資料庫寫入檔案: %s }", sstr(m_filepath));
		}
	});

	// We want to retrieve a new database, therefore refresh gamelist and indicate that
	Q_EMIT DownloadStarted();
}

Compat_Status game_compatibility::GetCompatibility(const std::string& title_id)
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

Compat_Status game_compatibility::GetStatusData(const QString& status)
{
	return Status_Data.at(status);
}
