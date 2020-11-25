#include "game_compatibility.h"
#include "gui_settings.h"
#include "downloader.h"
#include "localized.h"

#include "Crypto/unpkg.h"
#include "Loader/PSF.h"

#include <QApplication>
#include <QMessageBox>
#include <QJsonArray>
#include <QJsonDocument>

LOG_CHANNEL(compat_log, "Compat");

constexpr auto qstr = QString::fromStdString;
inline std::string sstr(const QString& _in) { return _in.toStdString(); }

game_compatibility::game_compatibility(std::shared_ptr<gui_settings> settings, QWidget* parent)
	: QObject(parent)
	, m_gui_settings(settings)
{
	m_filepath = m_gui_settings->GetSettingsDir() + "/compat_database.dat";
	m_downloader = new downloader(parent);
	RequestCompatibility();

	connect(m_downloader, &downloader::signal_download_error, this, &game_compatibility::handle_download_error);
	connect(m_downloader, &downloader::signal_download_finished, this, &game_compatibility::handle_download_finished);
}

void game_compatibility::handle_download_error(const QString& error)
{
	Q_EMIT DownloadError(error);
}

void game_compatibility::handle_download_finished(const QByteArray& data)
{
	compat_log.notice("Database download finished");

	// Create new map from database and write database to file if database was valid
	if (ReadJSON(QJsonDocument::fromJson(data).object(), true))
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

		compat_log.success("Wrote database to file: %s", sstr(m_filepath));
	}
}

bool game_compatibility::ReadJSON(const QJsonObject& json_data, bool after_download)
{
	const int return_code = json_data["return_code"].toInt();

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
		compat::status status = Status_Data.at(json_result.value("status").toString("NoResult"));

		// Add date if possible
		status.date = json_result.value("date").toString();

		// Add latest version if possible
		status.latest_version = json_result.value("update").toString();

		// Add patchsets if possible
		if (const QJsonValue patchsets_value = json_result.value("patchsets"); patchsets_value.isArray())
		{
			for (const QJsonValue& patch_set : patchsets_value.toArray())
			{
				compat::pkg_patchset set;
				set.tag_id         = patch_set["tag_id"].toString().toStdString();
				set.popup          = patch_set["popup"].toBool();
				set.signoff        = patch_set["signoff"].toBool();
				set.popup_delay    = patch_set["popup_delay"].toInt();
				set.min_system_ver = patch_set["min_system_ver"].toString().toStdString();

				if (const QJsonValue packages_value = patch_set["packages"]; packages_value.isArray())
				{
					for (const QJsonValue& package : packages_value.toArray())
					{
						compat::pkg_package pkg;
						pkg.version        = package["version"].toString().toStdString();
						pkg.size           = package["size"].toInt();
						pkg.sha1sum        = package["sha1sum"].toString().toStdString();
						pkg.ps3_system_ver = package["ps3_system_ver"].toString().toStdString();
						pkg.drm_type       = package["drm_type"].toString().toStdString();

						if (const QJsonValue changelogs_value = package["changelogs"]; changelogs_value.isArray())
						{
							for (const QJsonValue& changelog : changelogs_value.toArray())
							{
								compat::pkg_changelog chl;
								chl.type    = changelog["type"].toString().toStdString();
								chl.content = changelog["content"].toString().toStdString();

								pkg.changelogs.push_back(std::move(chl));
							}
						}

						if (const QJsonValue titles_value = package["titles"]; titles_value.isArray())
						{
							for (const QJsonValue& title : titles_value.toArray())
							{
								compat::pkg_title ttl;
								ttl.type  = title["type"].toString().toStdString();
								ttl.title = title["title"].toString().toStdString();

								pkg.titles.push_back(std::move(ttl));
							}
						}

						set.packages.push_back(std::move(pkg));
					}
				}

				status.patch_sets.push_back(std::move(set));
			}
		}

		// Add status to map
		m_compat_database.emplace(std::pair<std::string, compat::status>(sstr(key), status));
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

		const QByteArray data = file.readAll();
		file.close();

		compat_log.notice("Finished reading database from file: %s", sstr(m_filepath));

		// Create new map from database
		ReadJSON(QJsonDocument::fromJson(data).object(), online);

		return;
	}

	const std::string url = "https://rpcs3.net/compatibility?api=v1&export";
	compat_log.notice("Beginning compatibility database download from: %s", url);

	m_downloader->start(url, true, true, tr("Downloading Database"));

	// We want to retrieve a new database, therefore refresh gamelist and indicate that
	Q_EMIT DownloadStarted();
}

compat::status game_compatibility::GetCompatibility(const std::string& title_id)
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

compat::status game_compatibility::GetStatusData(const QString& status)
{
	return Status_Data.at(status);
}

compat::package_info game_compatibility::GetPkgInfo(const QString& pkg_path, game_compatibility* compat)
{
	package_reader reader(pkg_path.toStdString());
	psf::registry psf = reader.get_psf();

	// TODO: localization of title and changelog
	std::string title_key     = "TITLE";
	std::string changelog_key = "paramhip";

	compat::package_info info;
	info.path     = pkg_path;
	info.title    = qstr(std::string(psf::get_string(psf, title_key))); // Let's read this from the psf first
	info.title_id = qstr(std::string(psf::get_string(psf, "TITLE_ID")));
	info.category = qstr(std::string(psf::get_string(psf, "CATEGORY")));
	info.version  = qstr(std::string(psf::get_string(psf, "APP_VER")));

	if (!info.category.isEmpty())
	{
		const Localized localized;

		if (const auto boot_cat = localized.category.cat_boot.find(info.category); boot_cat != localized.category.cat_boot.end())
		{
			info.local_cat = boot_cat->second;
		}
		else if (const auto data_cat = localized.category.cat_data.find(info.category); data_cat != localized.category.cat_data.end())
		{
			info.local_cat = data_cat->second;
		}

		// Update packages always seem to have an APP_VER, so let's assume it's a DLC otherwise.
		if (info.category == "GD" && info.version.isEmpty())
		{
			info.is_dlc = true;
		}
	}

	if (info.version.isEmpty())
	{
		// Fallback to VERSION
		info.version = qstr(std::string(psf::get_string(psf, "VERSION")));
	}

	if (compat)
	{
		compat::status stat = compat->GetCompatibility(sstr(info.title_id));
		if (!stat.patch_sets.empty())
		{
			// We currently only handle the first patch set
			for (const auto& package : stat.patch_sets.front().packages)
			{
				if (sstr(info.version) == package.version)
				{
					if (const std::string localized_title = package.get_title(title_key); !localized_title.empty())
					{
						info.title= qstr(localized_title);
					}

					if (const std::string localized_changelog = package.get_changelog(changelog_key); !localized_changelog.empty())
					{
						info.changelog = qstr(localized_changelog);
					}

					break;
				}
			}
		}
	}

	if (info.title.isEmpty())
	{
		const QFileInfo file_info(pkg_path);
		info.title = file_info.fileName();
	}

	return info;
}
