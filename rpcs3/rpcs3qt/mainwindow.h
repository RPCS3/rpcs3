#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

signals:

private slots:
	void BootElf();
	void BootGame();
	void InstallPkg();
	void Pause();
	void Stop();
	void SendOpenSysMenu();
	void SendExit();
	void Settings();
	void PadSettings();
	void AutoPauseSettings();
	void VFSManager();
	void VHDDManager();
	void SaveData();
	void ELFCompiler();
	void CgDisasm();
	void KernelExplorer();
	void MemoryViewer();
	void RSXDebugger();
	void StringSearch();
	void DecryptSPRXLibraries();
	void About();

private:
	void CreateActions();
	void CreateMenus();
	void CreateDockWindows();

	QAction *bootElfAct;
	QAction *bootGameAct;
	QAction *bootInstallAct;
	QAction *sysPauseAct;
	QAction *sysStopAct;
	QAction *sysSendOpenMenuAct;
	QAction *sysSendExitAct;
	QAction *confSettingsAct;
	QAction *confPadAct;
	QAction *confAutopauseManagerAct;
	QAction *confVfsManagerAct;
	QAction *confVhddManagerAct;
	QAction *confSavedataManagerAct;
	QAction *toolsCompilerAct;
	QAction *toolsCgDisasmAct;
	QAction *toolsKernelExplorerAct;
	QAction *toolsMemoryViewerAct;
	QAction *toolsRsxDebuggerAct;
	QAction *toolsStringSearchAct;
	QAction *toolsSecryptSprxLibsAct;
	QAction *exitAct;
	QAction *aboutAct;
	QAction *aboutQtAct;
};

#endif // MAINWINDOW_H
