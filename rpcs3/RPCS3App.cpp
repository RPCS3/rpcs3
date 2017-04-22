#include "RPCS3App.h"

#include <QDesktopWidget>

// For now, a trivial constructor/destructor.  May add command line usage later.
RPCS3App::RPCS3App(int argc, char* argv[]) : QApplication(argc, argv)
{
}

void RPCS3App::Init()
{
	// Create callsbacks from the emulator.
	InitializeCallbacks();

	// Create and show the main window.
	RPCS3MainWin = new MainWindow(nullptr);
	QSize defaultSize = QDesktopWidget().availableGeometry().size() * 0.7;
	RPCS3MainWin->resize(defaultSize);

	RPCS3MainWin->show();
}

/** RPCS3 emulator has functions it desires to call from the GUI at times. Initialize them in here.
*/
// TODO: Implement these callbacks!
void RPCS3App::InitializeCallbacks()
{

}
