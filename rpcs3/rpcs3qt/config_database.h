#pragma once

#include <QJsonObject>
#include <optional>

class downloader;

class config_database : public QObject
{
	Q_OBJECT

public:
	config_database(QWidget* parent);
	virtual ~config_database();

	bool has_config(const std::string& title_id) const;
	std::optional<std::string> get_config(const std::string& title_id);

	/** Reads database. If online set to true: Downloads and writes the database to file */
	void request_config_database(bool online = false);

Q_SIGNALS:
	void download_started();
	void download_finished();
	void download_canceled();
	void download_error(const QString& error);

private Q_SLOTS:
	void handle_download_error(const QString& error);
	void handle_download_finished(const QByteArray& content);
	void handle_download_canceled();

private:
	/** Creates new set from the database. Returns config for the optional serial. */
	std::optional<std::string> read_json(const QByteArray& data, bool after_download, const std::string& serial = "");

	QString m_filepath;
	downloader* m_downloader = nullptr;

	std::set<std::string> m_config_database;
};
