#include "user_account.h"

UserAccount::UserAccount(const std::string& userId)
{
	// Setting userId.
	m_userId = userId;

	// Setting userDir.
	m_userDir = Emu.GetHddDir() + "home/" + m_userId + "/";

	// Setting userName.
	fs::file file;
	if (file.open(m_userDir + "localusername", fs::read))
	{
		file.read(m_userName, 16*sizeof(char)); //max of 16 chars on real PS3
		file.close();
	}
	else
	{
		LOG_WARNING(GENERAL, "UserAccount: localusername file read error (userId=%s, userDir=%s).", m_userId, m_userDir);
	}
}

UserAccount::~UserAccount()
{
}

