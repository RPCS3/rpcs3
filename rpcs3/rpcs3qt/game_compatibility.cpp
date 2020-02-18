﻿#include "game_compatibility.h"

#include <QLabel>
#include <QMessageBox>

LOG_CHANNEL(compat_log, "Compat");

constexpr auto qstr = QString::fromStdString;
inline std::string sstr(const QString& _in) { return _in.toStdString(); }

game_compatibility::game_compatibility(std::shared_ptr<gui_settings> settings) : m_xgui_settings(settings)
{
	m_filepath = m_xgui_settings->GetSettingsDir() + "/compat_database.dat";
	m_url = "https://rpcs3.net/compatibility?api=v1&export";
	m_network_request = QNetworkRequest(QUrl(m_url));

	RequestCompatibility();
}

bool game_compatibility::ReadJSON(const QJsonObject& json_data, bool after_download)
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
				error_message = "Server Error - Internal Error";
				break;
			case -2:
				error_message = "Server Error - Maintenance Mode";
				break;
			default:
				error_message = "Server Error - Unknown Error";
				break;
			}
			compat_log.error("%s: return code %d", error_message, return_code);
			Q_EMIT DownloadError(qstr(error_message) + " " + QString::number(return_code));
		}
		else
		{
			compat_log.error("Database Error - Invalid: return code %d", return_code);
		}
		return false;
	}

	if (!json_data["results"].isObject())
	{
		compat_log.error("Database Error - No Results found");
		return false;
	}

	m_compat_database.clear();

	QJsonObject json_results = json_data["results"].toObject();

	// Retrieve status data for every valid entry
	for (const auto& key : json_results.keys())
	{
		if (!json_results[key].isObject())
		{
			compat_log.error("Database Error - Unusable object %s", sstr(key));
			continue;
		}

		QJsonObject json_result = json_results[key].toObject();

		// Retrieve compatibility information from json
		compat_status status = Status_Data.at(json_result.value("status").toString("NoResult"));

		// Add date if possible
		status.date = json_result.value("date").toString();

		// Add latest version if possible
		status.latest_version = json_result.value("update").toString();

		// Add status to map
		m_compat_database.emplace(std::pair<std::string, compat_status>(sstr(key), status));
	}

	return true;
}

void game_compatibility::RequestCompatibility(bool online)
{
	if (!online)
	{
		// Retrieve database from file
		QFile file(m_filepath);

		if (!file.exists())
		{
			compat_log.notice("Database file not found: %s", sstr(m_filepath));
			return;
		}

		if (!file.open(QIODevice::ReadOnly))
		{
			compat_log.error("Database Error - Could not read database from file: %s", sstr(m_filepath));
			return;
		}

		QByteArray data = file.readAll();
		file.close();

		compat_log.notice("Finished reading database from file: %s", sstr(m_filepath));

		// Create new map from database
		ReadJSON(QJsonDocument::fromJson(data).object(), online);

		return;
	}

	if (QSslSocket::supportsSsl() == false)
	{
		compat_log.error("Can not retrieve the online database! Please make sure your system supports SSL. Visit our quickstart guide for more information: https://rpcs3.net/quickstart");
		const QString message = tr("Can not retrieve the online database!<br>Please make sure your system supports SSL.<br>Visit our <a href='https://rpcs3.net/quickstart'>quickstart guide</a> for more information.");
		QMessageBox box(QMessageBox::Icon::Warning, tr("Warning!"), message, QMessageBox::StandardButton::Ok, nullptr);
		box.setTextFormat(Qt::RichText);
		box.exec();
		return;
	}

	compat_log.notice("SSL supported! Beginning compatibility database download from: %s", sstr(m_url));

	// Send request and wait for response
	m_network_access_manager.reset(new QNetworkAccessManager());
	QNetworkReply* network_reply = m_network_access_manager->get(m_network_request);

	// Show Progress
	m_progress_dialog = new progress_dialog(tr("Downloading Database"), tr(".Please wait."), tr("Abort"), 0, 100, true);
	m_progress_dialog->show();

	// Animate progress dialog a bit more
	m_progress_timer.reset(new QTimer(this));
	connect(m_progress_timer.get(), &QTimer::timeout, [&]()
	{
		switch (++m_timer_count % 3)
		{
		case 0:
			m_timer_count = 0;
			m_progress_dialog->setLabelText(tr(".Please wait."));
			break;
		case 1:
			m_progress_dialog->setLabelText(tr("..Please wait.."));
			break;
		default:
			m_progress_dialog->setLabelText(tr("...Please wait..."));
			break;
		}
	});
	m_progress_timer->start(500);

	// Handle abort
	connect(m_progress_dialog, &QProgressDialog::canceled, network_reply, &QNetworkReply::abort);
	connect(m_progress_dialog, &QProgressDialog::finished, m_progress_dialog, &QProgressDialog::deleteLater);

	// Handle progress
	connect(network_reply, &QNetworkReply::downloadProgress, [&](qint64 bytesReceived, qint64 bytesTotal)
	{
		m_progress_dialog->setMaximum(bytesTotal);
		m_progress_dialog->setValue(bytesReceived);
	});

	// Handle network error
	connect(network_reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), [=, this](QNetworkReply::NetworkError error)
	{
		if (error == QNetworkReply::NoError)
		{
			return;
		}

		if (error != QNetworkReply::OperationCanceledError)
		{
			// We failed to retrieve a new database, therefore refresh gamelist to old state
			const QString error = network_reply->errorString();
			Q_EMIT DownloadError(error);
			compat_log.error("Network Error - %s", sstr(error));
		}

		// Clean up Progress Dialog
		if (m_progress_dialog)
		{
			m_progress_dialog->close();
		}
		if (m_progress_timer)
		{
			m_progress_timer->stop();
		}

		network_reply->deleteLater();
	});

	// Handle response according to its contents
	connect(network_reply, &QNetworkReply::finished, [=, this]()
	{
		if (network_reply->error() != QNetworkReply::NoError)
		{
			return;
		}

		// Clean up Progress Dialog
		if (m_progress_dialog)
		{
			m_progress_dialog->close();
		}
		if (m_progress_timer)
		{
			m_progress_timer->stop();
		}

		compat_log.notice("Database download finished");

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
				compat_log.notice("Database file found: %s", sstr(m_filepath));
			}

			if (!file.open(QIODevice::WriteOnly))
			{
				compat_log.error("Database Error - Could not write database to file: %s", sstr(m_filepath));
				return;
			}

			file.write(data);
			file.close();

			compat_log.success("Write database to file: %s", sstr(m_filepath));
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
