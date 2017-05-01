#ifndef GUI_SETTINGS_H
#define GUI_SETTINGS_H

#include "Utilities/Log.h"

#include <QSettings.h>

class GuiSettings : public QSettings
{
	Q_OBJECT

public:
	GuiSettings(QObject* parent);
	~GuiSettings();

	QByteArray readGuiGeometry();
	QByteArray readGuiState();
	bool getGameListVisibility();
	bool getLoggerVisibility();
	bool getDebuggerVisibility();

	logs::level getLogLevel();
	bool getTTYLogging();
public slots:
	/** Call this from the main window passing in the result from calling saveGeometry*/
	void writeGuiGeometry(const QByteArray& settings);

	/** Call this from main window passing in the results from saveState*/
	void writeGuiState(const QByteArray& settings);

	/** Sets the visibility of the gamelist. */
	void setGamelistVisibility(bool val);

	/** Sets the visibility of the logger. */
	void setLoggerVisibility(bool val);

	/** Sets the visibility of the debugger. */
	void setDebuggerVisibility(bool val);

	/* I'd love to use the enum, but Qt doesn't like connecting things that aren't meta types.*/
	void setLogLevel(uint lev);

	void setTTYLogging(bool val);
};

#endif
