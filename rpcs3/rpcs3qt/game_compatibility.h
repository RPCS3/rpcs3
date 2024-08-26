#pragma once

#include <memory>

#include <QJsonObject>

class downloader;
class gui_settings;

namespace compat
{
	/** Represents the "title" json object */
	struct pkg_title
	{
		std::string type; // TITLE or TITLE_08 etc. (system languages)
		std::string title; // The Last of Arse
	};

	/** Represents the "changelog" json object */
	struct pkg_changelog
	{
		std::string type; // paramhip or paramhip_08 etc. (system languages)
		std::string content; // "This system software update improves system performance."
	};

	/** Represents the "package" json object */
	struct pkg_package
	{
		std::string version; // 01.04
		int size = 0;
		std::string sha1sum;        // a5c83b88394ea3ae99974caedd38690981a80f3e
		std::string ps3_system_ver; // 04.4000
		std::string drm_type;       // local or mbind etc.
		std::vector<pkg_changelog> changelogs;
		std::vector<pkg_title> titles;

		std::string get_changelog(const std::string& type) const
		{
			if (const auto it = std::find_if(changelogs.cbegin(), changelogs.cend(), [type](const pkg_changelog& cl) { return cl.type == type; });
				it != changelogs.cend())
			{
				return it->content;
			}
			if (const auto it = std::find_if(changelogs.cbegin(), changelogs.cend(), [](const pkg_changelog& cl) { return cl.type == "paramhip"; });
				it != changelogs.cend())
			{
				return it->content;
			}
			return "";
		}

		std::string get_title(const std::string& type) const
		{
			if (const auto it = std::find_if(titles.cbegin(), titles.cend(), [type](const pkg_title& t) { return t.type == type; });
				it != titles.cend())
			{
				return it->title;
			}
			if (const auto it = std::find_if(titles.cbegin(), titles.cend(), [](const pkg_title& t) { return t.type == "TITLE"; });
				it != titles.end())
			{
				return it->title;
			}
			return "";
		}
	};

	/** Represents the "patchset" json object */
	struct pkg_patchset
	{
		std::string tag_id; // BLES01269_T7
		bool popup = false;
		bool signoff = false;
		int popup_delay = 0;
		std::string min_system_ver; // 03.60
		std::vector<pkg_package> packages;
	};

	/** Represents the json object that contains an app's information and some additional info that is used in the GUI */
	struct status
	{
		int index = 0;
		QString date;
		QString color;
		QString text;
		QString tooltip;
		QString latest_version;
		std::vector<pkg_patchset> patch_sets;
	};

	// The type of the package. In the future this should signify the proper PKG_CONTENT_TYPE.
	enum class package_type
	{
		update,
		dlc,
		other
	};

	/** Concicely represents a specific pkg's localized information for use in the GUI */
	struct package_info
	{
		bool is_valid = true;

		QString path;        // File path
		QString title_id;    // TEST12345
		QString title;       // Localized
		QString changelog;   // Localized, may be empty
		QString version;     // May be empty
		QString category;    // HG, DG, GD etc.
		QString local_cat;   // Localized category

		package_type type = package_type::other; // The type of package (Update, DLC or other)
	};
}

class game_compatibility : public QObject
{
	Q_OBJECT

private:
	const std::map<QString, compat::status> Status_Data =
	{
		{ "Playable", { 0, "", "#1ebc61", tr("Playable"),         tr("Games that can be properly played from start to finish") } },
		{ "Ingame",   { 1, "", "#f9b32f", tr("Ingame"),           tr("Games that either can't be finished, have serious glitches or have insufficient performance") } },
		{ "Intro",    { 2, "", "#e08a1e", tr("Intro"),            tr("Games that display image but don't make it past the menus") } },
		{ "Loadable", { 3, "", "#e74c3c", tr("Loadable"),         tr("Games that display a black screen with a framerate on the window's title") } },
		{ "Nothing",  { 4, "", "#455556", tr("Nothing"),          tr("Games that don't initialize properly, not loading at all and/or crashing the emulator") } },
		{ "NoResult", { 5, "", "",        tr("No results found"), tr("There is no entry for this game or application in the compatibility database yet.") } },
		{ "NoData",   { 6, "", "",        tr("Database missing"), tr("Right click here and download the current database.\nMake sure you are connected to the internet.") } },
		{ "Download", { 7, "", "",        tr("Retrieving..."),    tr("Downloading the compatibility database. Please wait...") } }
	};
	std::shared_ptr<gui_settings> m_gui_settings;
	QString m_filepath;
	downloader* m_downloader = nullptr;
	std::map<std::string, compat::status> m_compat_database;

	/** Creates new map from the database */
	bool ReadJSON(const QJsonObject& json_data, bool after_download);

public:
	/** Handles reads, writes and downloads for the compatibility database */
	game_compatibility(std::shared_ptr<gui_settings> settings, QWidget* parent);

	/** Reads database. If online set to true: Downloads and writes the database to file */
	void RequestCompatibility(bool online = false);

	/** Returns the compatibility status for the requested title */
	compat::status GetCompatibility(const std::string& title_id);

	/** Returns the data for the requested status */
	compat::status GetStatusData(const QString& status) const;

	/** Returns package information like title, version, changelog etc. */
	static compat::package_info GetPkgInfo(const QString& pkg_path, game_compatibility* compat);

Q_SIGNALS:
	void DownloadStarted();
	void DownloadFinished();
	void DownloadCanceled();
	void DownloadError(const QString& error);

private Q_SLOTS:
	void handle_download_error(const QString& error);
	void handle_download_finished(const QByteArray& content);
	void handle_download_canceled();
};
