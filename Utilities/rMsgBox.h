#pragma once

extern std::string rMessageBoxCaptionStr;// = "Message";

enum MsgBoxParams : unsigned long
{
	rOK = 0x4
	, rYES =0x2//res
	, rNO = 0x8 //res
	, rID_YES = 5103 //resDialog
	, rCANCEL = 0x10
	, rYES_NO = 0xA 
	, rHELP = 0x1000
	, rNO_DEFAULT = 0x80
	, rCANCEL_DEFAULT = 0x80000000
	, rYES_DEFAULT = 0x0
	, rOK_DEFAULT = 0x0
	, rICON_NONE = 0x40000
	, rICON_EXCLAMATION = 0x100
	, rICON_ERROR = 0x200
	, rICON_HAND = 0x200
	, rICON_QUESTION = 0x400
	, rICON_INFORMATION = 0x800
	, rICON_AUTH_NEEDED = 0x80000
	, rSTAY_ON_TOP = 0x8000
	, rCENTRE = 0x1
};

struct rMessageDialog
{
	rMessageDialog(void *parent, const std::string& msg, const std::string& title = rMessageBoxCaptionStr, long style = rOK | rCENTRE);
	rMessageDialog(const rMessageDialog& other) = delete;
	~rMessageDialog();
	long ShowModal();
	void *handle;
};

long rMessageBox(const std::string& message, const std::string& title,long style);

struct dummyApp
{
	dummyApp();
	std::string GetAppName();
	void* handle;
};

dummyApp& rGetApp();