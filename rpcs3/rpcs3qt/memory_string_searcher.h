#pragma once

#include <QDialog>

class QLineEdit;
class QCheckBox;
class QComboBox;

class memory_string_searcher : public QDialog
{
	Q_OBJECT

	QLineEdit* m_addr_line;
	QCheckBox* m_chkbox_case_insensitive = nullptr;
	QComboBox* m_cbox_input_mode = nullptr;

public:
	memory_string_searcher(QWidget* parent);

private Q_SLOTS:
	void OnSearch();
};
