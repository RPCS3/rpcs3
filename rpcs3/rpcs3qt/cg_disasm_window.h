#ifndef CGDISASMWINDOW_H
#define CGDISASMWINDOW_H

#include <QTextEdit>

class cg_disasm_window : public QWidget
{
	Q_OBJECT

private slots:
	void ShowContextMenu(const QPoint &pos);

private:
	QString m_path_last;
	QTextEdit* m_disasm_text;
	QTextEdit* m_glsl_text;

	QAction *openCgBinaryProgram;

public:
	explicit cg_disasm_window(QWidget *parent);
};

#endif	// CGDISASMWINDOW_H
