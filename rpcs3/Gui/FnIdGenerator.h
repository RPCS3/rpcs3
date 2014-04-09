#include "TextInputDialog.h"
#include "../Crypto/aes.h"
#include "../Crypto/sha1.h"

class FnIdGenerator : public wxDialog
{
private:
	Array<std::string> m_func_name;
	Array<u32> m_func_id;
	wxListView* m_list;

public:
	FnIdGenerator(wxWindow* parent);
	~FnIdGenerator();

	void OnInput(wxCommandEvent &event);
	void OnClear(wxCommandEvent &event);
	u32 GenerateFnId(const std::string& func_name);
	void UpdateInformation();
	void PrintId();
};