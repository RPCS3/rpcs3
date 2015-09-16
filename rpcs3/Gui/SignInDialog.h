#pragma once

#include "Emu/SysCalls/Modules/cellNetCtl.h"

class SignInDialogFrame : public SignInDialogInstance
{
	std::unique_ptr<wxDialog> m_dialog;

public:
	virtual void Create() override;
	virtual void Destroy() override;
};
