#pragma once

#include "Emu/System.h"
#include "Utilities/File.h"
#include "Utilities/StrFmt.h"

// Do not confuse this with the "user" in Emu/System.h. 
// That user is read from config.yml, and it only represents the currently "logged in" user.
// The UserAccount class will represent all users in the home directory for the User Manager dialog.
// Selecting a user account in this dialog and saving writes it to config.yml.
class UserAccount
{
public:
	explicit UserAccount(const std::string& userId);

	std::string GetUserId() { return m_userId; };
	std::string GetUserDir() { return m_userDir; };
	std::string GetUserName() { return m_userName; };
	~UserAccount();

private:
	std::string m_userId;
	std::string m_userDir;
	std::string m_userName;

};
