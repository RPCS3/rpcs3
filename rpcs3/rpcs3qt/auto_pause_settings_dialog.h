#pragma once

#include "stdafx.h"

#include <QDialog>
#include <QLabel>
#include <QTableWidget>
#include <QLineEdit>

class auto_pause_settings_dialog : public QDialog
{
	Q_OBJECT

	enum
	{
		id_add,
		id_remove,
		id_config,
	};

	std::vector<u32> m_entries;

public:
	explicit auto_pause_settings_dialog(QWidget* parent);
	QTableWidget *pauseList;
	void UpdateList(void);
	void LoadEntries(void);
	void SaveEntries(void);

public Q_SLOTS:
	void OnRemove();
private Q_SLOTS:
	void ShowContextMenu(const QPoint &pos);
	void keyPressEvent(QKeyEvent *event) override;
};

class AutoPauseConfigDialog : public QDialog
{
	Q_OBJECT

	u32 m_entry;
	u32* m_presult;
	bool m_newEntry;
	QLineEdit* m_id;
	QLabel* m_current_converted;
	auto_pause_settings_dialog* m_apsd;

public:
	explicit AutoPauseConfigDialog(QWidget* parent, auto_pause_settings_dialog* apsd, bool newEntry, u32* entry);

private Q_SLOTS:
	void OnOk();
	void OnCancel();
	void OnUpdateValue();
};
