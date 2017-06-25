#pragma once

#include <QTextEdit>
#include <QDropEvent>

#include "stdafx.h"
#include "gui_settings.h"

class cg_disasm_window : public QWidget
{
	Q_OBJECT

private Q_SLOTS:
	void ShowContextMenu(const QPoint &pos);
	void ShowDisasm();
	bool IsValidFile(const QMimeData& md, bool save = false);

private:
	QString m_path_last;
	QTextEdit* m_disasm_text;
	QTextEdit* m_glsl_text;
	QList<QUrl> m_urls;

	QAction *openCgBinaryProgram;

	std::shared_ptr<gui_settings> xgui_settings;

public:
	explicit cg_disasm_window(std::shared_ptr<gui_settings> xSettings, QWidget *parent);

protected:
	void dropEvent(QDropEvent* ev);
	void dragEnterEvent(QDragEnterEvent* ev);
	void dragMoveEvent(QDragMoveEvent* ev);
	void dragLeaveEvent(QDragLeaveEvent* ev);
};
