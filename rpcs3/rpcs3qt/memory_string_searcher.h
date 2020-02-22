#pragma once

#include <QDialog>
#include <QLineEdit>

class memory_string_searcher : public QDialog
{
	Q_OBJECT

	QLineEdit* m_addr_line;

public:
	memory_string_searcher(QWidget* parent);

private Q_SLOTS:
	void OnSearch();
};
