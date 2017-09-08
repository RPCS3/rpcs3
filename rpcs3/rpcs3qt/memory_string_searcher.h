#pragma once

#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>

class memory_string_searcher : public QDialog
{
	Q_OBJECT

	QLineEdit* m_addr_line;

public:
	memory_string_searcher(QWidget* parent);

private Q_SLOTS:
	void OnSearch();
};
