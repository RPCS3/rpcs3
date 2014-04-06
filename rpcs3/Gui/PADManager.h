#include "Ini.h"
#include <time.h>

enum ButtonIDs
{
	id_pad_lstick_left = 0x777,
	id_pad_lstick_down,
	id_pad_lstick_right,
	id_pad_lstick_up,

	id_pad_left,
	id_pad_down,
	id_pad_right,
	id_pad_up,
	
	id_pad_l1,
	id_pad_l2,
	id_pad_l3,

	id_pad_start,
	id_pad_select,
	
	id_pad_r1,
	id_pad_r2,
	id_pad_r3,

	id_pad_square,
	id_pad_cross,
	id_pad_circle,
	id_pad_triangle,

	id_pad_rstick_left,
	id_pad_rstick_down,
	id_pad_rstick_right,
	id_pad_rstick_up,

	id_reset_parameters,
};

struct PadButtons
{
	wxButton* b_up_lstick;
	wxButton* b_down_lstick;
	wxButton* b_left_lstick;
	wxButton* b_right_lstick;

	wxButton* b_up;
	wxButton* b_down;
	wxButton* b_left;
	wxButton* b_right;
	
	wxButton* b_shift_l1;
	wxButton* b_shift_l2;
	wxButton* b_shift_l3;

	wxButton* b_start;
	wxButton* b_select;

	wxButton* b_shift_r1;
	wxButton* b_shift_r2;
	wxButton* b_shift_r3;


	wxButton* b_square;
	wxButton* b_cross;
	wxButton* b_circle;
	wxButton* b_triangle;

	wxButton* b_up_rstick;
	wxButton* b_down_rstick;
	wxButton* b_left_rstick;
	wxButton* b_right_rstick;

	wxButton* b_ok;
	wxButton* b_cancel;
	wxButton* b_reset;
};

class PADManager : public wxDialog, PadButtons
{
private:
	AppConnector m_app_connector;
	u32 m_seconds;
	u32 m_button_id;
	bool m_key_pressed;

public:
	PADManager(wxWindow* parent);
	~PADManager()
	{
	}

	void OnKeyDown(wxKeyEvent &keyEvent);
	void OnKeyUp(wxKeyEvent &keyEvent);
	void OnButtonClicked(wxCommandEvent &event);
	void UpdateLabel();
	void ResetParameters();
	void UpdateTimerLabel(const u32 id);
	void SwitchButtons(const bool IsEnabled);
	const wxString GetKeyName(const u32 keyCode);
	void RunTimer(const u32 seconds, const u32 id);
};