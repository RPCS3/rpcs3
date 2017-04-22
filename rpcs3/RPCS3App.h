#ifndef RPCS3_APP_H
#define RPCS3_APP_H

#include <QApplication>

#include "Gui/mainwindow.h"

/** RPCS3 Application Class
 * The point of this class is to do application initialization and to hold onto the main window. The main thing I intend this class to do, for now, is to initialize callbacks and the mainwindow.
*/

class RPCS3App : public QApplication
{
	Q_OBJECT
public:
	RPCS3App(int argc, char* argv[]);
	/** Call this method before calling app.exec
	*/
	void Init();
private:
	void InitializeCallbacks();

	MainWindow* RPCS3MainWin;
};
#endif
