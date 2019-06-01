#include "user_account.h"

UserAccount::UserAccount(const std::string& user_id)
{
	// Setting userId.
	m_user_id = user_id;

	// Setting userDir.
	m_user_dir = Emulator::GetHddDir() + "home/" + m_user_id + "/";

	// Setting userName.
	fs::file file;
	if (file.open(m_user_dir + "localusername", fs::read))
	{
		m_username = file.to_string();
		file.close();

		if (m_username.length() > 16) // max of 16 chars on real PS3
		{
			m_username = m_username.substr(0, 16);
			LOG_WARNING(GENERAL, "UserAccount: localusername of userId=%s was too long, cropped to: %s", m_user_id, m_username);
		}
	}
	else
	{
		LOG_ERROR(GENERAL, "UserAccount: localusername file read error (userId=%s, userDir=%s).", m_user_id, m_user_dir);
	}
}

UserAccount::~UserAccount()
{
}
