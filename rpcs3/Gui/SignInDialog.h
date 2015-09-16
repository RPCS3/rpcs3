#pragma once

class SignInDialogFrame
{
	std::unique_ptr<wxDialog> m_dialog;

public:
	virtual void Create();
	virtual void Destroy();
};
