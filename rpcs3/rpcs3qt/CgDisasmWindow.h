#ifndef CGDISASMWINDOW_H
#define CGDISASMWINDOW_H

#include <QTextEdit>
#include <QMainWindow>

class CgDisasmWindow : public QTabWidget
{
	Q_OBJECT

private slots:
	void OpenCg();
	void CgDisasmWindow::ShowContextMenu(const QPoint &pos);
private:
	QTextEdit* m_disasm_text;
	QTextEdit* m_glsl_text;
	QWidget* tab_disasm;
	QWidget* tab_glsl;

	QAction *openCgBinaryProgram;

public:
	explicit CgDisasmWindow(QWidget *parent);
};

#endif	// CGDISASMWINDOW_H
