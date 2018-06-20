#include "user_account.h"

UserAccount::UserAccount(const std::string& user_id)
{
	// Setting userId.
	m_user_id = user_id;

	// Setting userDir.
	m_user_dir = Emu.GetHddDir() + "home/" + m_user_id + "/";

	// Setting userName.
	fs::file file;
	if (file.open(m_user_dir + "localusername", fs::read))
	{
		file.read(m_username, 16*sizeof(char)); // max of 16 chars on real PS3
		file.close();
	}
	else
	{
		LOG_WARNING(GENERAL, "UserAccount: localusername file read error (userId=%s, userDir=%s).", m_user_id, m_user_dir);
	}
}

UserAccount::~UserAccount()
{
}
