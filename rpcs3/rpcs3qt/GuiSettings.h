#ifndef GUI_SETTINGS_H
#define GUI_SETTINGS_H

#include "Utilities/Log.h"

#include <QSettings>
#include <QDir>

/** Class for GUI settings..
*/
class GuiSettings : public QObject
{
	Q_OBJECT

public:
	explicit GuiSettings(QObject* parent = nullptr);
	~GuiSettings();

	QString GetSettingsDir() {
		return settingsDir.absolutePath();
	}

	/** Changes the settings file to the destination preset*/
	void ChangeToConfig(const QString& destination);


	QByteArray ReadGuiGeometry();
	QByteArray ReadGuiState();
	bool GetGamelistVisibility();
	bool GetLoggerVisibility();
	bool GetDebuggerVisibility();
	
	logs::level GetLogLevel();
	bool GetTTYLogging();
	bool GetGamelistColVisibility(int col);
	int GetGamelistSortCol();
	bool GetGamelistSortAsc();
	QByteArray GetGameListState();
	QStringList GetConfigEntries();
	QString GetCurrentConfig();
	QString GetCurrentStylesheet();
	QString GetCurrentStylesheetPath();
	QStringList GetStylesheetEntries();
public slots:
	void Reset(bool removeMeta = false);

	/** Call this from the main window passing in the result from calling saveGeometry*/
	void WriteGuiGeometry(const QByteArray& settings);

	/** Call this from main window passing in the results from saveState*/
	void WriteGuiState(const QByteArray& settings);

	/** Call this in gamelist's destructor to save the state of the column sizes.*/
	void WriteGameListState(const QByteArray& settings);

	/** Sets the visibility of the gamelist. */
	void SetGamelistVisibility(bool val);

	/** Sets the visibility of the logger. */
	void SetLoggerVisibility(bool val);

	/** Sets the visibility of the debugger. */
	void SetDebuggerVisibility(bool val);

	/* I'd love to use the enum, but Qt doesn't like connecting things that aren't meta types.*/
	void SetLogLevel(uint lev);

	void SetTTYLogging(bool val);

	void SetGamelistColVisibility(int col, bool val);

	void SetGamelistSortCol(int col);

	void SetGamelistSortAsc(bool val);

	void SaveCurrentConfig(const QString& friendlyName);
	
	void SetCurrentConfig(const QString& friendlyName);

	void SetStyleSheet(const QString& friendlyName);
private:
	void BackupSettingsToTarget(const QString& destination);

	QSettings settings;
	QDir settingsDir;
};

#endif
