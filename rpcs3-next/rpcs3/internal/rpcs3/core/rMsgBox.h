#pragma once

enum MsgBoxParams : unsigned long
{
	rYES_DEFAULT      = 0x0,
	rOK_DEFAULT       = 0x0,
	rCENTRE           = 0x1,
	rYES              = 0x2, //res
	rOK               = 0x4,
	rNO               = 0x8, //res
	rCANCEL           = 0x10,
	rYES_NO           = 0xA,
	rNO_DEFAULT       = 0x80,
	rICON_EXCLAMATION = 0x100,
	rICON_ERROR       = 0x200,
	rICON_HAND        = 0x200,
	rICON_QUESTION    = 0x400,
	rICON_INFORMATION = 0x800,
	rHELP             = 0x1000,
	rID_CANCEL        = 0x13ED,
	rID_YES           = 0x13EF, //resDialog
	rSTAY_ON_TOP      = 0x8000,
	rICON_NONE        = 0x40000,
	rICON_AUTH_NEEDED = 0x80000,
	rCANCEL_DEFAULT   = 0x80000000,
};

struct rMessageDialog
{
	rMessageDialog(void *parent, const std::string& msg, const std::string& title = "RPCS3", long style = rOK | rCENTRE);
	rMessageDialog(const rMessageDialog& other) = delete;
	~rMessageDialog();
	long ShowModal();
	void *handle;
};

long rMessageBox(const std::string& message, const std::string& title,long style);

