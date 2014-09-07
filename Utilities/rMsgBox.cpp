#include "stdafx.h"
#include "restore_new.h"
#include <wx/msgdlg.h>
#include "define_new_memleakdetect.h"
#include "rMsgBox.h"

#ifndef QT_UI
rMessageDialog::rMessageDialog(void *parent, const std::string& msg, const std::string& title , long style )
{
	handle = reinterpret_cast<void*>(new wxMessageDialog(
		reinterpret_cast<wxWindow*>(parent)
		, fmt::FromUTF8(msg)
		, fmt::FromUTF8(title)
		, style
		));
}

rMessageDialog::~rMessageDialog()
{
	delete reinterpret_cast<wxMessageDialog*>(handle);
}

long rMessageDialog::ShowModal()
{
	return reinterpret_cast<wxMessageDialog*>(handle)->ShowModal();
}

long rMessageBox(const std::string& message, const std::string& title, long style)
{
	return wxMessageBox(fmt::FromUTF8(message), fmt::FromUTF8(title),style);
}

#endif

