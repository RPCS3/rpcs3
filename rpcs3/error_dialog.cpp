
#include "error_dialog.h"
#include <QString>

// Todo: move to utils
void show_boot_error_dialog(QWidget* parent, emulator_result_code code)
{
	QString error_message = nullptr;

	switch (code)
	{
	case emulator_result_code::executable_invalid:
		error_message = "Invalid executable.";
		break;

	case emulator_result_code::executable_no_access:
		error_message = "Can't open executable.";
		break;

	case emulator_result_code::decryption_failed:
		error_message = "Failed to decrypt executable.";
		break;

	case emulator_result_code::decryption_rap_missing:
		error_message = "Failed to decrypt executable because RAP file is missing.";
		break;

	case emulator_result_code::disc_failed_relocation:
	case emulator_result_code::disc_mount_failure:
		error_message = "Failed to mount disc filesystem.";
		break;

	case emulator_result_code::pkg_install_failure:
		error_message = "Failed to install PKG.";
		break;

	case emulator_result_code::exception_thrown:
	case emulator_result_code::pause_failed:
	case emulator_result_code::system_not_ready:
		error_message = "Internal error."; // unlikely
		break;
	}

	QMessageBox::warning(parent, "Failed to boot", error_message, QMessageBox::Ok, QMessageBox::Ok);
}