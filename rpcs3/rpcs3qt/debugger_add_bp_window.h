#pragma once

#include "breakpoint_list.h"

#include <QDialog>

class debugger_add_bp_window : public QDialog
{
	Q_OBJECT

public:
	explicit debugger_add_bp_window(breakpoint_list* bp_list, QWidget* parent = nullptr);
};
