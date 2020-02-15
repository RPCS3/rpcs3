#pragma once

#include <string>

// Do not confuse this with the "user" in Emu/System.h. 
// That user is read from config.yml, and it only represents the currently "logged in" user.
// The UserAccount class will represent all users in the home directory for the User Manager dialog.
// Selecting a user account in this dialog and saving writes it to config.yml.
class UserAccount
{
public:
	explicit UserAccount(const std::string& user_id = "00000001");

	std::string GetUserId() { return m_user_id; }
	std::string GetUserDir() { return m_user_dir; }
	std::string GetUsername() { return m_username; }
	~UserAccount();

private:
	std::string m_user_id;
	std::string m_user_dir;
	std::string m_username;
};
