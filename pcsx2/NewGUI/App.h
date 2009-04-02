

#include <wx/wx.h>
#include <wx/fileconf.h>

#include "System.h"

struct ConsoleLogOptions
{
	bool Show;
	// if true, DisplayPos is ignored and the console is automatically docked to the main window.
	bool AutoDock;
	// Display position used if AutoDock is false (ignored otherwise)
	wxPoint DisplayPos;
	wxSize DisplaySize;
};

//////////////////////////////////////////////////////////////////////////////////////////
//
class AppConfig : public wxFileConfig
{
public:
	ConsoleLogOptions ConLogBox;
	wxPoint MainGuiPosition;
	
public:
	AppConfig( const wxString& filename );
	void LoadSettings();
	void SaveSettings();
};

//////////////////////////////////////////////////////////////////////////////////////////
//
class ConsoleLogFrame : public wxFrame, public NoncopyableObject
{
protected:
	wxTextCtrl&	m_TextCtrl;

public:
	// ctor & dtor
	ConsoleLogFrame(wxWindow *pParent, const wxString& szTitle);
	virtual ~ConsoleLogFrame();

	// menu callbacks
	virtual void OnClose(wxCommandEvent& event);
	virtual void OnCloseWindow(wxCloseEvent& event);

	virtual void OnSave (wxCommandEvent& event);
	virtual void OnClear(wxCommandEvent& event);

	virtual void Write( const wxChar* text );
	void Newline();

	void SetColor( Console::Colors color );
	void ClearColor();

protected:
	// use standard ids for our commands!
	enum
	{
		Menu_Close = wxID_CLOSE,
		Menu_Save  = wxID_SAVE,
		Menu_Clear = wxID_CLEAR
	};

	// common part of OnClose() and OnCloseWindow()
	virtual void DoClose();

	DECLARE_EVENT_TABLE()
	
	void OnMoveAround( wxMoveEvent& evt );
};

//////////////////////////////////////////////////////////////////////////////////////////
//
class Pcsx2App : public wxApp
{
protected:
	ConsoleLogFrame* m_ConsoleFrame;
	AppConfig* m_GlobalConfig;

public:
	Pcsx2App();

	bool OnInit();
	int OnExit();

	ConsoleLogFrame* GetConsoleFrame() const { return m_ConsoleFrame; }
	void SetConsoleFrame( ConsoleLogFrame& frame ) { m_ConsoleFrame = &frame; }
	
	AppConfig& GetActiveConfig() const;
};

DECLARE_APP(Pcsx2App)

static AppConfig& Conf() { return wxGetApp().GetActiveConfig(); }
