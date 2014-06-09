#include "stdafx.h"


std::string rMessageBoxCaptionStr = "Message";

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

std::string dummyApp::GetAppName()
{
	if (handle)
	{
		return fmt::ToUTF8(reinterpret_cast<wxApp*>(handle)->GetAppName());
	}
	else
	{
		return "NULL";
	}
}
dummyApp::dummyApp() : handle(nullptr)
{

}
static dummyApp app;

dummyApp& rGetApp()
{
	return app;
}