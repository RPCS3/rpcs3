#pragma once

#include "Emu\System.h"
#include <QMessageBox>

// Todo: move to utils
void show_boot_error_dialog(QWidget* parent, emulator_result_code code);